import numpy as np
import numpy.typing as npt
from typing import Tuple, Optional, List, Union, cast
import scipy.signal as sig
import pyfar as pf

EPS = np.finfo(float).eps
# ERB means "Equivalent retangular band(-width)"
# Constants:
_ERB_L = 24.7
_ERB_Q = 9.265

def db2lin(x: npt.ArrayLike, /) -> npt.NDArray:
    """Convert from decibels to linear

    Args:
        x (ArrayLike): value(s) to be converted

    Returns:
        (ArrayLike): values converted to linear
    """
    return np.power(10.0, x * 0.05)

def db(x: npt.NDArray[float], /, *, is_squared: bool = False, allow_inf: bool = False) -> npt.NDArray[float]:
    """Converts values to decibels.

    Args:
        x (NDArray): value(s) to be converted to dB.
        is_squared (bool): Indicates whether `x` represents some power-like quantity (True) or some root-power-like
            quantity (False). Defaults to False, i.e. `x` is a root-power-like auqntity (e.g. Voltage, pressure, ...).
        allow_inf (bool): Whether infinitely small (or 0.0) values should be allowed, in which case `-np.inf` values
            may be returned.

    Returns:
        An array with the converted values, in dB.
    """
    x = np.abs(x)
    if not allow_inf:
        x = np.maximum(x, EPS)
    factor = 10.0 if is_squared else 20.0
    return factor * np.log10(x)

def ms_to_samps(ms: npt.NDArray[float], /, fs: float) -> npt.NDArray[int]:
    """Calculate the nearest integer number of samples corresponding to the given time duration in milliseconds.

    Args:
        ms (NDArray): Duration, in milliseconds
        fs (float): Sample rate, in Hertz.

    Returns:
        An NDArray containing the nearest corresponding number of samples
    """
    return np.round(ms * 1e-3 * fs).astype(int)

def samps_to_ms(samps: npt.NDArray[int], /, fs: float) -> npt.NDArray[float]:
    """Calculate time duration in milliseconds corresponding to the given number of samples.

    Args:
        samps (ArrayLike): Durations in samples.
        fs (float): Sample rate, in Hertz.

    Returns:
        A NDArray ofcontaining the duration(s), in milliseconds.
    """
    return 1e3 * np.array(samps) / fs

def hertz_to_erbscale(frequency: Union[float, np.ndarray]) -> Union[float, np.ndarray]:
    """Convert frequency in Hertz to ERB-scale frequency.

    Equation 16 in Hohmann2002.

    Args:
        frequency (Union[float, np.ndarray]): The frequency value(s) in Hz.

    Returns:
        Union[float, np.ndarray]: The frequency value(s) on ERB-scale.
    """
    return _ERB_Q * np.log(1 + frequency / (_ERB_L * _ERB_Q))


def erbscale_to_hertz(erb: Union[float, np.ndarray]) -> Union[float, np.ndarray]:
    """Convert frequency in ERB-scale to Hertz.

    Args:
        erb (Union[float, np.ndarray]): Frequency value(s) on the ERB-scale.

    Returns:
        Union[float, np.ndarray]: Frequency values in Hz.
    """
    return (np.exp(erb / _ERB_Q) - 1) * _ERB_L * _ERB_Q

def half_hann_fade(length: int, fade_out: bool = False) -> npt.NDArray:
    """Generate a half Hann window for fading out or in a signal.

    Args:
        length (int): The length of the fade.
        fade_out (bool, optional): If True, fade out, if False, fade in. Defaults to False.

    Returns:
        npt.NDArray: The half Hann window signal (one-dimensional)
    """
    n = np.linspace(start=0, stop=1, num=length)
    fade: npt.NDArray = 0.5 - 0.5 * np.cos(np.pi * (n + int(fade_out)))
    return fade

def rms(irs: npt.NDArray[np.float_], axis: int = -1, remove_dc: bool = False) \
        -> Union[np.float_, npt.NDArray[np.float_]]:
    """Calculate the root-mean-square value of a signal.

    Args:
        irs (npt.NDArray): The signal array
        axis (int, optional): The axis along which to make the calculation. Defaults to -1.
        remove_dc (bool, optional): Optionally remove the mean (DC) component of the signal. Defaults to False.

    Returns:
        npt.NDArray: The RMS value(s). One fewer dimension than `irs`.
    """
    rms_irs: npt.NDArray
    if remove_dc:
        rms_irs = np.std(irs, axis=axis)
    else:
        rms_irs = np.sqrt(np.mean(np.square(irs), axis=axis))
    return rms_irs

def normalise_irs(irs: npt.NDArray[np.float_],
                  fs: int,
                  norm_db: float = -18,
                  window_size: Tuple[float, float] = (5e-4, 1e-3)) -> Tuple[npt.NDArray[np.float_], float]:
    """Normalise an array of IRs to a given value in decibels.

    The RMS signal level around the peak value is calculated using a window.
    The mean across measurements is then used to adjust the level of the entire array.

    Args:
        irs (npt.NDArray): Impulse response data, with time in last axis.
        fs (int): Sample rate in Hertz
        norm_db (float): The normalisation target value in decibels, by default -18
        window_size (tuple): of the Window size before and after peak value in seconds (float),
            By default (5e-4, 1e-3) i.e. 0.5ms before peak and 1ms after.

    Returns:
        tuple: of the normalised array and the gain applied in dB
    """
    win_pre = int(window_size[0] * fs)
    win_post = int(window_size[1] * fs)
    win_len = win_pre + win_post
    win: npt.NDArray
    win = np.concatenate((half_hann_fade(win_pre), half_hann_fade(win_post, fade_out=True)), axis=-1)

    def win_peak(x: npt.NDArray) -> npt.NDArray:
        peak = np.argmax(x)
        start_ix = int(np.maximum(peak - win_pre, 0))
        end_ix = start_ix + win_len
        if end_ix > irs.shape[-1]:
            raise RuntimeError("Window index exceeded length of array")
        return np.array(x[start_ix:end_ix] * win)

    irs_peaks = np.apply_along_axis(win_peak, axis=-1, arr=irs)
    mean_val = np.mean(rms(irs_peaks, axis=-1))
    norm_lin = db2lin(norm_db)
    norm_val = norm_lin / mean_val
    gain_db = cast(float, db(norm_val))
    norm_irs = irs * norm_val
    return norm_irs, gain_db

def filter_in_subbands(input_signal: np.ndarray,
                       fs: int,
                       bands_per_octave: int = 3,
                       freq_range=(20, 16000),
                       filter_length: int = 4096) -> Tuple[npt.NDArray, npt.NDArray]:
    
    signal = pf.classes.audio.Signal(input_signal, fs)
    signal_subband, centre_frequencies = pf.dsp.filter.reconstructing_fractional_octave_bands(signal, 
        bands_per_octave, freq_range, n_samples=filter_length)

    return signal_subband.time, centre_frequencies


def calculate_interchannel_coherence(x: np.ndarray, y: np.ndarray, time_axis: int) -> npt.NDArray:
    return np.abs(np.sum(x * y, axis=time_axis)) / np.sqrt(np.sum(x**2, axis=time_axis) * np.sum(y**2, axis=time_axis))


def calculate_interchannel_cross_correlation_matrix(signals: np.ndarray,
                                                    fs: int,
                                                    num_channels: int,
                                                    time_axis: int = -1,
                                                    channel_axis: int = 0,
                                                    return_single_coeff: bool = False,
                                                    bands_per_octave: int = 3,
                                                    freq_range=(20,16000)):
    """Returns a matrix of ICC values for each channel axis in signals"""
    if time_axis != -1:
        signals = np.moveaxis(signals, 0, 1)
        channel_axis = 0
        time_axis = -1

    # passthrough filterbank
    if not return_single_coeff:
        signals_subband, centre_frequencies = filter_in_subbands(signals, 
                                                                 fs, 
                                                                 bands_per_octave=bands_per_octave, 
                                                                 freq_range=freq_range
                                                                 )
        num_f = len(centre_frequencies)
        # make sure the channel axis is in the beginning
        signals_subband = np.moveaxis(signals_subband, 1, 0)

        icc_matrix = np.ones((num_f, num_channels, num_channels))
    else:
        icc_matrix = np.ones((num_channels, num_channels))

    for i in range(num_channels):
        for j in range(num_channels):
            if i == j:
                continue
            if return_single_coeff:
                icc_matrix[i, j] = calculate_interchannel_coherence(signals[i, :], signals[j, :], time_axis=time_axis)
            else:
                icc_matrix[:, i, j] = calculate_interchannel_coherence(signals_subband[i, :, :],
                                                                       signals_subband[j, :, :],
                                                                       time_axis=time_axis)
    if return_single_coeff:
        return icc_matrix
    else:
        return icc_matrix, centre_frequencies

def signal_envelope_analytical(irs: npt.NDArray, axis: int = -1) -> np.ndarray:
    """Calculate amplitude envelope using Hilbert transform.


    Args:
        irs (npt.NDArray): Impulse responses
        axis (int, optional):  Time axis index, by default -1

    Returns:
        npt.NDArray: Envelope signals
    """
    env: npt.NDArray
    env = np.abs(sig.hilbert(irs, axis=axis))
    return env


def parabolic_peak_interp(ym1: float, y0: float, yp1: float) -> Tuple[float, float, float]:
    """Quadratic interpolation of three adjacent samples to find a peak.

    A parabola is given by y(x) = a*(x-p)^2+b, where y(-1)=ym1, y(0)=y0, y(1)=yp1.

    https://ccrma.stanford.edu/~jos/sasp/Matlab_Parabolic_Peak_Interpolation.html

    Args:
        ym1 (float): Sample before the peak
        y0 (float): Peak value sample
        yp1 (float): Next sample following the peak

    Returns:
        p (float): peak location
        y (float): peak height
        a (float): half-curvature of parabolic fit through the points
    """
    if ym1 < y0 <= yp1 or ym1 > y0 >= yp1:
        raise ValueError(
            f"y0 must be either the largest or the smallest of the three samples. Got: ym1={ym1}, y0={y0}, yp1={yp1}")
    p = (yp1 - ym1) / (2 * (2 * y0 - yp1 - ym1))
    y = y0 - 0.25 * (ym1 - yp1) * p
    a = 0.5 * (ym1 - 2 * y0 + yp1)
    return p, y, a


def xcorr(a: npt.NDArray,
          b: npt.NDArray,
          max_lag: Optional[int] = None,
          norm: bool = False) -> Tuple[npt.NDArray, npt.NDArray]:
    """Estimate the cross-correlation of two signals

    Args:
        a (npt.NDArray): First signal
        b (npt.NDArray): Second signal
        max_lag (int, optional): If provided, limit the delay/lag range to this many samples. Defaults to None.
            Cannot be greater than the longest length of the two signals.
        norm (bool, optional): If True, will do normalised cross-correlation. Defaults to False.
            Cannot be used if a and b have different lengths.

    Raises:
        ValueError: a and/or b are not one-dimensional
        ValueError: norm cannot be True if a and b have different lenghts

    Returns:
        npt.NDArray: Cross-correlation function
        npt.NDArray: Corresponding delays from a to b (note: reversed perspective relative to
            scipy.signal.correlation_lags)
    """
    if a.ndim > 1 or b.ndim > 1:
        raise ValueError("a and b must be one-dimensional arrays")

    max_lag_default = max(b.shape[0], a.shape[0]) - 1
    pad = b.shape[0] - a.shape[0]

    if max_lag is None:
        lag_range = max_lag_default
    else:
        lag_range = abs(max_lag)
        lag_range = min(max_lag, max_lag_default)

    # calculate normalization before zero padding (as this will affect the norms)
    if norm:
        if pad != 0:
            raise ValueError("a and b must have equal length for normalised cross-correlation")
        norm_val = np.linalg.norm(a) * np.linalg.norm(b)

    # zero pad to same length
    if pad > 0:
        a = np.pad(a, (0, abs(pad)), 'constant', constant_values=0)
    elif pad < 0:
        b = np.pad(b, (0, abs(pad)), 'constant', constant_values=0)

    cc = sig.correlate(a, b)

    if lag_range != max_lag_default:
        start_ix = max_lag_default - lag_range
        end_ix = max_lag_default + lag_range + 1
        cc = cc[start_ix:end_ix]

    if norm:
        cc /= norm_val

    lags = np.arange(lag_range, -lag_range - 1, -1)

    return cc, lags


def estimate_onsets_log_threshold(irs: npt.NDArray[np.float32],
                                  thresh_db: float = -20,
                                  axis: int = -1) -> npt.NDArray[np.int_]:
    """Estimate onsets of impulse responses using a log-amplitude threshold below the peak value.

    Args:
        irs (npt.NDArray[np.float32]): Array of impulse responses.
        thresh_db (float): Threshold below the peak value in decibels at which the onset is detected.
        axis (int, optional): The axis of the time series, by default -1 (last).

    Returns:
        npt.NDArray[np.int_]: Onset indices in samples (one less dimension than irs).
    """

    def log_thresh_1d(log_amp_ir: npt.NDArray[np.float32]) -> int:
        peak_ix = np.argmax(log_amp_ir)
        peak = log_amp_ir[peak_ix]
        thresh = peak - abs(thresh_db)  # forces negative dB
        above_thresh = np.nonzero(log_amp_ir[:peak_ix + 1] > thresh)[0]
        return above_thresh[0] if above_thresh.size > 0 else -1

    log_amp_irs = db(np.abs(irs))
    return np.apply_along_axis(log_thresh_1d, axis, log_amp_irs)

    