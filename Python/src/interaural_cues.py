import numpy as np
from numpy.typing import NDArray, ArrayLike
from scipy.fft import irfft, fftshift
import scipy.signal as sig
from dataclasses import dataclass
from typing import Optional, Tuple
from utils import db, ms_to_samps, parabolic_peak_interp, signal_envelope_analytical, xcorr

_EPS = np.finfo(float).eps


@dataclass
class HRTFParams:
    """A dataclass storing the HRTF, ILD and IPD corresponding to each DoA"""

    # number of points in the frequency axis
    num_freq_points: int
    # number of DoAs
    num_doas: int
    # length of HRIR in time domain
    num_samples: int
    # direction of arrival of the source
    doa: ArrayLike
    # frequencies where HRTF is calculated
    freqs: ArrayLike
    # hrtfs in freq domain, of size num_doas x num_freqs x 2
    hrtfs: NDArray
    # hrirs in the time domain
    hrirs: NDArray
    # interaural cues
    ild: NDArray
    ipd: NDArray
    itd: Optional[ArrayLike] = None


def ild_from_hrtf(hrtfs: NDArray,
                  ear_axis: int,
                  right_ear_idx: int = -1,
                  left_ear_idx: int = 0,
                  in_db: bool = False) -> NDArray:
    """
    Calculate interaural level difference for each frequency bin and DoA from the HRTF database
    Args:
        hrtfs (NDArray) : array of hrtfs, num_doas x num_freqs x 2
        ear_axis (int): ear axis (should be -1)
        right_ear_idx (int): index of right ear
        left_ear_idx (int): index of left ear
        in_db (bool): whether to return the ILD in dB
    Returns:
        NDArray: num_doas x num_freqs ILD (ear dimension squeezed)
    """
    if ear_axis != -1:
        hrtfs = np.moveaxis(hrtfs, ear_axis, -1)
    ild = np.abs(
        hrtfs[..., right_ear_idx]) / (np.abs(hrtfs[..., left_ear_idx]) + _EPS)
    return db(ild) if in_db else ild


def ipd_from_hrtf(hrtfs: NDArray,
                  ear_axis: int,
                  right_ear_idx: int = -1,
                  left_ear_idx: int = 0,
                  unwrap_phase: bool = False) -> NDArray:
    """
    Calculate interaural phase difference for each frequency bin and DoA from the HRTF database
    Args:
        hrtfs (NDArray) : complex array of hrtfs, num_doas x num_freqs x 2 in the frequency domain
        ear_axis (int): ear axis (should be -1)
        right_ear_idx (int): index of right ear
        left_ear_idx (int): index of left ear
        unwrap_phase (bool): if true, unwrap phase before returning
    Returns:
        NDArray: num_doas x num_freqs IPD in radians (between -pi/2 and pi/2 if wrapped), ear dimension squeezed
    """

    if ear_axis != -1:
        hrtfs = np.moveaxis(hrtfs, ear_axis, -1)
    ipd = np.angle(hrtfs[..., right_ear_idx] / hrtfs[..., left_ear_idx])
    return np.unwrap(ipd) if unwrap_phase else ipd


def convert_ipd_to_itd(ipd: NDArray, sample_rate: float,
                       norm_frequency_axis: ArrayLike,
                       wrapped_phase: bool) -> NDArray:
    """
    Converts interaural phase difference to interaural time difference
    Args:
        ipd (NDArray) : Array of IPDa
        sample_rate (float) : sampling frequency
        norm_frequency_axis (ArrayLike): normalised positive frequency axis between (0, pi)
        wrapped_phase (bool): whether the IPD is wrapped between -pi/2 and pi/2
    """
    if wrapped_phase:
        ipd = np.unwrap(ipd)
    return -ipd / (norm_frequency_axis * sample_rate + _EPS)


def convert_itd_to_ipd(itd: NDArray, sample_rate: float,
                       norm_frequency_axis: ArrayLike,
                       wrap_phase: bool) -> NDArray:
    """
    Converts interaural time difference to interaural phase difference
    Args:
        itd (NDArray) : Array of ITDs in seconds
        sample_rate (float) : sampling frequency
        norm_frequency_axis (ArrayLike): normalised positive frequency axis between (0, pi)
        wrap_phase (bool): whether to wrap the IPD between -pi/2 and pi/2
    """
    ipd = -norm_frequency_axis * (itd * sample_rate + _EPS)
    if wrap_phase:
        return ((ipd + np.pi) % 2 * np.pi) - np.pi
    else:
        return ipd


def get_hrtf_from_spherical_head_model(
        azimuth: ArrayLike,
        fft_freqs: NDArray,
        num_time_samples: int,
        head_radius: float = 0.075,
        speed_sound: float = 340,
        use_tanh_fit: bool = True) -> HRTFParams:
    """
    Get ITD, ILD for a spherical head of specified radius at specified frequencies,
    see Romblom, D. and Bahu, H., “A Revision and Objective Evaluation of the 1-Pole
    1-Zero Spherical Head Shadowing Filter,” in AES AVAR, 2018
    Args:
        azimuth (ArrayLike): the direction of arrival (azimuth only) of the sources in degrees
        fft_freqs(NDArray): frequencies corresponding to the fft bins in radians
        num_time_sample (int): length of the HRIR filters in the time domain
        head_radius (float): head radius in m
        speed_sound (float): speed of sound in air (m/s)
        use_tanh_fit (bool): whther to use hyperbolic tangent fit to get zero of head shadowing, or to use Brown-Duda
    Returns:
        HRTFParams : HRTF object containing the DoAs, HRTFs and ITD and ILD as a function
                     of DoA and frequency

    """

    def calculate_time_delay(head_radius: float, speed_sound: float,
                             incidence_angle: ArrayLike) -> ArrayLike:

        time_delay = head_radius / speed_sound * np.where(
            np.abs(incidence_angle) < 90, -np.cos(np.radians(incidence_angle)),
            np.radians(np.abs(incidence_angle) - 90.0))

        return time_delay

    # azimuth of the sources
    num_doas = len(azimuth)
    num_freqs = len(fft_freqs)
    num_ears = 2
    hrtfs = np.zeros((num_doas, num_freqs, num_ears), dtype=complex)
    hrirs = np.zeros((num_doas, num_time_samples, num_ears), dtype=np.float32)
    phase = np.zeros((num_doas, num_freqs, num_ears), dtype=float)
    fundamental_frequency = speed_sound / head_radius

    for k in range(num_ears):
        # incidence angle, depending on right or left ear
        incidence_angle = 90 - azimuth if k == 0 else 90 + azimuth
        time_delay = calculate_time_delay(head_radius, speed_sound,
                                          incidence_angle)
        phase[..., k] = 2 * np.pi * time_delay[:, np.newaxis] @ fft_freqs[
            np.newaxis, :]
        if use_tanh_fit:
            zero_location_control = 1.15 - (0.85 *
                                            np.tanh(1.7 *
                                                    (incidence_angle - 97.4)))
        else:
            beta_min = 0.1
            min_incidence_angle = 150
            zero_location_control = (1 + beta_min / 2.) + (
                (1 - beta_min / 2.) *
                np.cos(incidence_angle / min_incidence_angle * 180))

        # this is a row variable
        intermediate_var = 1j * 2 * np.pi * fft_freqs[np.newaxis, :] / (
            2 * fundamental_frequency)
        head_shadow_response = (1 + zero_location_control[:, np.newaxis]
                                @ intermediate_var) / (1 + np.ones(
                                    (num_doas, 1)) @ intermediate_var)
        hrtfs[..., k] = head_shadow_response * np.exp(-1j * phase[..., k])

        hrirs[..., k] = fftshift(irfft(
            hrtfs[..., k],
            n=num_time_samples,
            axis=1,
        ),
                                 axes=1)

        ild = ild_from_hrtf(hrtfs, ear_axis=-1)
        ipd = ipd_from_hrtf(hrtfs, ear_axis=-1, unwrap_phase=True)

    return HRTFParams(num_freqs, num_doas, num_time_samples, azimuth,
                      fft_freqs, hrtfs, hrirs, ild, ipd)


def itd_maxiacc(
    signal: np.ndarray,
    fs: float,
    time_axis: int = -1,
    ear_axis: int = -2,
    max_lag_ms: float = 1.,
    calc_env: bool = True,
    lowpass_cutoff: Optional[float] = 3000.,
    interp_peak: bool = True,
) -> Tuple[float, float]:
    """Estimate the interaural time difference using the maximum of the interaural cross-correlation function.
    Optionally, the incoming signal can be pre-processed with a low-pass filter and/or calculation of the signal
    envelope. This processing typically makes the estimation more robust and is enabled by default.

    There are lots of other ways of estimating ITD. In the `onsets` module, you can directly estimate the time-of-
    arrival in impulse responses and use the inter-aural difference of these as an ITD estimate. However, this IACC
    method should also work on binaural signals, not just impulse responses.

    Args:
        signal (np.ndarray): The signal to be analysed. Must be at least two-dimensional.
        fs (float): Sampling rate (Hertz)
        time_axis (int, optional): The time axis of signal. Defaults to -1.
        ear_axis (int, optional): The ear axis of signal (must have size 2). Defaults to -2.
        max_lag_ms (float, optional): The maximum . Defaults to 1.5.
        calc_env (bool, optional): _description_. Defaults to True.
        lowpass_cutoff (float, optional): If provided, the signal is low-pass filtered at this cutoff frequency (in
            Hertz) before further processing. Defaults to 3kHz. Setting to None will skip this stage.
        interp_peak (bool, optional): Optionally use parabolic interpolation to find the ITD to sub-sample acurracy.

    Raises:
        ValueError: If signal is not at least two-dimensional
        ValueError: If the ear_axis does not have size 2
        ValueError: If ear_axis and time_axis are equal.

    Returns:
        float: The delay (in seconds) corresponding to the maximum IACC.
        float: The maximum absolute value of the normalised IACC.
    """
    if signal.ndim < 2:
        raise ValueError("`signal` must be at least two-dimensional")
    if signal.shape[ear_axis] != 2:
        raise ValueError("The binaural signal must have size 2 in ear axis")
    if ear_axis == time_axis:
        raise ValueError("ear_axis and time_axis cannot be the same")

    max_lag = ms_to_samps(max_lag_ms, fs)

    # shuffle axes around so -2=ear, -1=time
    time_axis = np.remainder(time_axis, signal.ndim)
    ear_axis = np.remainder(ear_axis, signal.ndim)
    if time_axis != signal.ndim - 1:
        signal = np.moveaxis(signal, time_axis, -1)
    if time_axis < ear_axis:
        ear_axis -= 1
    if ear_axis != signal.ndim - 2:
        signal = np.moveaxis(signal, ear_axis, -2)

    # optional pre-processing steps
    if lowpass_cutoff is not None and lowpass_cutoff > 0:
        lp_sos = sig.butter(N=12, Wn=lowpass_cutoff / (fs / 2), output='sos')
        sig_lp = sig.sosfilt(lp_sos, signal, axis=-1)
    else:
        sig_lp = signal
    if calc_env:
        sig_env = signal_envelope_analytical(sig_lp, axis=-1)
    else:
        sig_env = sig_lp

    # iterate over other dimensions
    chan_shape = signal.shape[:-2]
    iacc = np.zeros(chan_shape, dtype=signal.dtype)
    itd = np.zeros(chan_shape, dtype=signal.dtype)
    for chan_ixs in np.ndindex(chan_shape):
        cc, lags = xcorr(sig_env[chan_ixs + (0, )],
                         sig_env[chan_ixs + (1, )],
                         max_lag=max_lag,
                         norm=True)
        abs_cc = np.abs(cc)
        max_iacc_ix = np.argmax(abs_cc)
        if interp_peak and 0 < max_iacc_ix < lags.shape[0] - 2:
            # use parabolic interpolation to find sub-sample max iacc
            lag_interp, iacc[chan_ixs], _ = parabolic_peak_interp(
                abs_cc[max_iacc_ix - 1], abs_cc[max_iacc_ix],
                abs_cc[max_iacc_ix + 1])
            # get time index using interpolated peak position
            if lag_interp < 0:
                itd[chan_ixs] = abs(lag_interp) * lags[max_iacc_ix - 1] + (
                    1 + lag_interp) * lags[max_iacc_ix]
            else:
                itd[chan_ixs] = (1 - lag_interp) * lags[
                    max_iacc_ix] + lag_interp * lags[max_iacc_ix + 1]
        else:
            itd[chan_ixs] = lags[max_iacc_ix]
            iacc[chan_ixs] = abs_cc[max_iacc_ix]
    itd /= fs
    return itd, iacc
