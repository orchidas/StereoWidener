import numpy as np
import numpy.typing as npt
from typing import Union
from scipy.signal import sosfilt
from utils import ms_to_samps


def warp_pole_angle(rho: float, pole_freq: Union[float,
                                                 np.ndarray]) -> npt.NDArray:
    """
    Warp pole angles according to warping factor rho
    Args:
        rho (float): warping factor, 0 < rho < 1 will zoom in on lower frequencies.
        pole_freq (float, np.ndarray): pole frequencies in radians/sec
    Returns:
        np.ndarray: the pole warped angles
    """
    poles_warped = np.exp(1j * pole_freq)
    lambdam = np.log((rho + poles_warped) / (1 + rho * poles_warped))
    return np.imag(lambdam)


def decorrelate_allpass_filters(fs: float,
                                nbiquads: int = 250,
                                max_grp_del_ms: float = 30.):
    """
    Return cascaded allpass SOS sections with randomised phase to perform signal decorrelation
    Args:
        fs (float): sample rate in Hz
        nbiquds (int): number of AP biquad sections
        max_grp_del_ms (float): maximum group delay in each frequency band
    Returns:
        np.ndarray: 6 x num_biquads AP filter coefficients
    """

    max_grp_del = (1.0 - max_grp_del_ms * 1e-3) / (1 + max_grp_del_ms * 1e-3)
    # each pole radius should give max group delay of 30ms
    ap_rad = np.random.uniform(high=max_grp_del, low=0.5, size=nbiquads)
    # uniformly distributed pole frequencies
    ap_pole_freq = np.random.uniform(low=0, high=2 * np.pi, size=nbiquads)

    # warp pole angles to ERB filterbank
    warp_factor = 0.7464 * np.sqrt(
        2.0 / np.pi * np.arctan(0.1418 * fs)) + 0.03237
    ap_pole_freq_warped = warp_pole_angle(warp_factor, ap_pole_freq)

    # allpass filter biquad cascade
    poles = ap_rad * np.exp(1j * ap_pole_freq_warped)
    sos_sec = np.zeros((nbiquads, 6))
    # numerator coefficients
    sos_sec[:, 0] = np.abs(poles)**2
    sos_sec[:, 1] = -2 * np.real(poles)
    sos_sec[:, 2] = np.ones(nbiquads)
    # denominator coefficients
    sos_sec[:, 3] = np.ones(nbiquads)
    sos_sec[:, 4] = -2 * np.real(poles)
    sos_sec[:, 5] = np.abs(poles)**2

    return sos_sec


def get_allpass_impulse_response(sos_section: np.ndarray, fs: float,
                                 signal_length_ms: float):
    """Create an impulse response from the sos matrix using a 
    cascade of biquad filters"""
    signal_length_samps = ms_to_samps(signal_length_ms, fs)
    impulse = np.zeros((1, signal_length_samps), dtype=float)
    impulse[0] = 1.0
    impulse_response = sosfilt(sos_section, impulse, zi=None)
    return np.squeeze(impulse_response)


def process_allpass(input_signal: np.ndarray,
                    fs: float,
                    num_biquads: int = 200,
                    max_grp_del_ms: float = 30.0) -> np.ndarray:
    """
    For an input stereo signal, pass both channels through
    cascade of allpass filters, and return the output
    """
    _, num_channels = input_signal.shape
    if num_channels > 2:
        input_signal = input_signal.T
        num_channels = 2
    if num_channels != 2:
        raise RuntimeError("Input signal must be stereo!")

    output_signal = np.zeros_like(input_signal)
    sos_section = np.zeros((num_channels, num_biquads, 6))

    for chan in range(num_channels):
        sos_section[chan, ...] = decorrelate_allpass_filters(
            fs, nbiquads=num_biquads, max_grp_del_ms=max_grp_del_ms)
        output_signal[:, chan] = sosfilt(sos_section[chan, ...],
                                         input_signal[:, chan],
                                         zi=None)

    return output_signal
