import numpy as np
import numpy.typing as npt
import sympy as sp
import matplotlib.pyplot as plt
import pyfar as pf
from pathlib import Path
from scipy.optimize import minimize, Bounds
from scipy.signal import fftconvolve
from loguru import logger
from typing import Optional, Tuple, Union

from utils import ms_to_samps, db2lin, db, normalise_irs, rms


def delay_filters(fir_coeffs: np.ndarray, delay_len_samp: int) -> npt.NDArray:
    """
    Given an FIR filter, add delay_len_samp of delays to it
    """
    return np.roll(fir_coeffs, delay_len_samp)


def get_target_decay_from_t60(decay_t60_ms: float,
                              seq_length_ms: float) -> npt.NDArray:
    """
    Given the desired T60, get the decay in dB at the end of the VN sequence
    Args:
        decay_t60_ms (float): desired T60 in ms
        seq_length_ms (float): length of the VN sequence in ms
    Returns
        float: the target decay in db at the end of the sequence
    """
    slope = -np.log(np.power(10, -60 / 20)) / (decay_t60_ms * 1e3)
    total_decay = np.exp(-slope * (seq_length_ms * 1e3))
    return -db(total_decay)


def third_octave_magnitude_smoothing(num_freq_points: int,
                                     freq_bins: npt.NDArray,
                                     mag_spectrum: npt.NDArray) -> npt.NDArray:
    """Third octave smoothing of the magnitude spectrum of a signal
    Args:
        num_freq_points : number of frequency points where the magnitude is calculated
        freq_bins : frequency bins in Hz where the magnitude is calculated
        mag_spectrum: unsmoothed magnitude spectrum of the signal
    Returns:
        ndarray: a third-octave smoothed magnitude spectrum
    """

    upper_lim = np.zeros(num_freq_points)
    lower_lim = np.zeros_like(upper_lim)
    smoothing_window = np.zeros((num_freq_points, num_freq_points))

    for k in range(num_freq_points):
        upper_lim[k] = freq_bins[k] * np.power(2, 1.0 / 6)
        lower_lim[k] = freq_bins[k] / np.power(2, 1.0 / 6)
        index_low = np.where(freq_bins >= lower_lim[k])[0][0]
        index_high = np.where(freq_bins <= upper_lim[k])[0][-1]
        smoothing_window[k, index_low:index_high +
                         1] = 1.0 / (index_high - index_low + 1)

    smoothed_spectrum = smoothing_window @ np.reshape(mag_spectrum,
                                                      (num_freq_points, 1))
    return smoothed_spectrum


def generate_interleaved_velvet_noise(num_seq: int,
                                      grid_size: int,
                                      delay_range_samps: Tuple,
                                      shorten: bool = False,
                                      compact: bool = False) -> npt.NDArray:
    """
    Generate interleaved (non-overlapping) velvet noise according to
    Valimaki et al. 'Late reverberation synthesis with interleaved velvet
    noise' in IEEE Trans. Aud. Speech and Lang. Process.
    Args:
        num_seq (int): number of delay lines
        grid_size (int): 1 impulse in grid_size samples
        delay_range_samps (tuple): minimum and maximum delay line lengths
                                   Prime numbers are generated within this range and then permuted
                                   to get final delay line lengths
        shorten (bool): if True, the generated VN sequence is shortened to max delay_range_samps
        compact (bool) if True, then the impulses are concentrated at the beginning of each sequence
    Returns:
        np.ndarray: num_seq x vn_seq_length velvet noise sequences
    """
    # generate prime sequence length
    prime_nums = np.array(list(
        sp.primerange(delay_range_samps[0], delay_range_samps[1])),
                          dtype=np.int32)
    rand_primes = prime_nums[np.random.permutation(len(prime_nums))]
    # delay line lengths
    delay_lengths = np.array(np.r_[rand_primes[:num_seq - 1],
                                   sp.nextprime(delay_range_samps[1])],
                             dtype=np.int32)
    vn_seq_len = np.max(delay_lengths)
    # scaling factor
    delta = 1.0 / num_seq
    seq_indices = np.arange(num_seq)
    larger_grid_size = grid_size * num_seq
    # add a bit of zero padding for the delay operations later on
    vn_seq = np.zeros(
        (num_seq, larger_grid_size * (np.max(delay_lengths) + 1)))
    if compact:
        new_grid_size = (vn_seq_len / 100) * np.power(
            10, 2 * larger_grid_size * seq_indices / vn_seq.shape[1])

    if shorten:
        vn_seq_short = np.zeros((num_seq, vn_seq_len))

    def fill_noise_sequence(num_iter):
        for k in range(num_iter):
            # generate uniform random numbers
            uniform_random_seq = np.random.uniform(low=0, high=1, size=num_seq)
            # get the impulse locations for the VN sequence
            if compact:
                imp_locations = np.int32(
                    np.round((delta * uniform_random_seq) *
                             (new_grid_size - 1)) + np.cumsum(new_grid_size))
            else:
                imp_locations = np.int32(
                    np.round(seq_indices * larger_grid_size +
                             (delta * uniform_random_seq) *
                             (larger_grid_size - 1)))
            # fill the impulse locations with randomly generated +/- 1s
            vn_seq[i, imp_locations +
                   (k * larger_grid_size)] = 2 * np.random.randint(
                       0, 2, size=num_seq) - 1

            if shorten:
                if np.any(imp_locations + (k * larger_grid_size) > vn_seq_len):
                    vn_seq_short[i, :] = vn_seq[i, :vn_seq_len]
                    return

    # fill the VN sequence
    for i in range(num_seq):
        num_iter = delay_lengths[i] - num_seq
        fill_noise_sequence(num_iter)

    if shorten:
        return vn_seq_short
    else:
        return vn_seq


def generate_non_overlapping_interleaved_velvet_noise(
        vn_seq: np.ndarray, grid_size: int) -> npt.NDArray:
    """
    Take a multichannel VN sequence and shift it to make sure
    that multiple VN sequences do not overlap
    """
    num_seq = vn_seq.shape[0]
    no_vn_seq = np.zeros_like(vn_seq)
    for k in range(num_seq - 1, 0, -1):
        no_vn_seq[k, :] = delay_filters(vn_seq[k, :], k * grid_size)

    no_vn_seq[0, :] = vn_seq[0, :]
    return no_vn_seq


def generate_velvet_noise_sequence(num_seq: int,
                                   fs: float,
                                   grid_size: int,
                                   ir_length_samps: int,
                                   delay_range_ms: Optional[Tuple] = (10, 50),
                                   num_channels: Optional[int] = 1,
                                   plot: bool = False) -> npt.NDArray:
    """
    Combine the interleaved VN sequences in parallel to form a smooth VN sequence
    Args:
        num_seq (int): number of parallel delay lines
        fs (float): sampling frequency
        grid_size (int): VN sequence will have one impulse per grid_size samples.
        ir_length_samps (int): length of VN sequence
        delay_range_ms (tuple, optional): minimum and maximum delay line lengths
        num_channels (int): number of channels of VN to generate
        plot (bool): whether to plot the generated VN sequence
    Returns:
        np.ndarray: (num_channels x ir_length_samps) VN sequence
    """
    noise = np.zeros((num_channels, ir_length_samps))
    delay_range_samps = (ms_to_samps(delay_range_ms[0],
                                     fs), ms_to_samps(delay_range_ms[-1], fs))
    # some seeds end up pushing the value of one sample up, that's why we retry
    # till we get a smooth envelope
    for ch in range(num_channels):
        constant_amplitude = False
        while not constant_amplitude:
            vn_seq = generate_interleaved_velvet_noise(num_seq, grid_size,
                                                       delay_range_samps)
            # stereo decorrelated channels are obtained by permuting the rows of
            # the velvet noise sequence before delaying and summing them
            if ch > 0:
                vn_seq = np.random.permutation(vn_seq.copy())

            total_noise_seq = np.sum(
                generate_non_overlapping_interleaved_velvet_noise(
                    vn_seq.copy(), grid_size),
                axis=0)
            noise[ch, :] = total_noise_seq[:ir_length_samps]
            if plot:
                plt.figure()
                plt.stem(total_noise_seq)

            constant_amplitude = np.where(
                np.abs(total_noise_seq) > 1)[0].size == 0

    return noise


def optimise_velvet_noise_sequence(
    init_vn_seq: npt.NDArray,
    sample_rate: float,
    num_samps_per_second: int,
    init_impulse_locations: npt.NDArray,
    decay_db: float,
    num_freq_bins: int = 2**10,
    max_gain_deviation: float = 2.0,
    max_iter: int = 60,
    verbose: bool = False,
) -> Tuple[npt.NDArray, npt.NDArray, float]:
    """
    Optimise the VN sequence to have a flatter magnitude spectrum (less colouration)
    accoring to Schelecht et a. 'Optimised Velvet Noise Decorrelator' in DAFx, 2018.
    Args:
        init_vn_seq (ndarray): the velvet noise sequence to be optimised
        sample_rate (float): sampling frequency in Hz
        num_samps_per_second (int): density of the sequecnce in samples/second
        init_impulse_locations (ndarray, int): location of the impulses in the sequence
        decay_db (float): the target decay rate of the sequence in dB
        num_freq_bins (int): number of frequency bins used in the cost function
        max_gain_deviation (float): maximum deviation from desired gain
        max_iter (int): maximum number of iterations allowed in optimisation
        verbose (bool): if true, cost function at each iteration is printed.
    Returns
        ndarray, ndarray, float: the optimised VN sequence, and its impulse locations and amplitudes,
                                 the cost function value at the end of minimisation
    """

    def get_sequence_amplitude(impulse_locations: npt.NDArray,
                               decay_slope: float) -> npt.NDArray:
        """
        Get exponentially decaying amplitude sequence
        """
        return np.exp(-impulse_locations * decay_slope)

    def get_velvet_sequence_frequency_response(
            impulse_locations: npt.NDArray, impulse_signs: npt.NDArray,
            impulse_amplitude: npt.NDArray,
            freq_rads: npt.NDArray) -> npt.NDArray:
        """
        Get the frequency response of the VN sequence based on equations 9,10
        Args:
            impulse_locations : location of the impulses in the VN sequence
            impulse_signs : signs of the impulses in the VN sequence
            impulse_amplitude : amplitude at the impulse locations
            freq_rad: frequencies in radian where the frequency response is to be evaluated
        Returns:
            ndarray: frequency response at specified frequency bins
        """
        num_freq_bins = len(freq_rads)
        mag = impulse_amplitude
        # matrix of size num_freq_bins x num_impulses
        mag_matrix = np.tile(mag, (num_freq_bins, 1))
        phase_matrix = -freq_rads[:, np.newaxis] @ impulse_locations[
            np.newaxis, :]
        phase_matrix[:, impulse_signs == -1] += np.pi
        # sum along impulse locations
        freq_response = np.sum(mag_matrix * np.exp(1j * phase_matrix), axis=-1)
        return freq_response

    def compute_spectral_error(impulse_locations: npt.NDArray,
                               impulse_amplitude: float,
                               impulse_signs: npt.NDArray, num_freq_bins: int,
                               sample_rate: float, freq_hz: npt.NDArray,
                               verbose: bool) -> Tuple[float, npt.NDArray]:
        """
        Compute spectral difference error according to eq. 15
        ArgsL:
            impulse_locations : location of the impulses in the VN sequence
            impulse_amplitude : amplitude at the impulse locations
            impulse_signs : signs of the impulses in the VN sequence
            num_freq_bins : number of points in the frequency axis
            sample_rate : sampling frequency in Hz
            freq_hz: frequencies in Hz where the frequency response is to be evaluated
            verbose : whether to print the error in each iteration
        Returns:
        float, np.ndarray : the cost function value, and the bounds for the amplitude
        """
        freqs_rad = 2 * np.pi / sample_rate * freq_hz
        # get frequency response
        freq_response = get_velvet_sequence_frequency_response(
            impulse_locations, impulse_signs, impulse_amplitude, freqs_rad)
        # get magnitude response in db
        magnitude_response = 20 * np.log10(np.abs(freq_response))

        # apply third octave smoothing on magnitude response
        smoothed_magnitude_response = np.squeeze(
            third_octave_magnitude_smoothing(num_freq_bins, freq_hz,
                                             magnitude_response))

        rms_error = rms(smoothed_magnitude_response -
                        np.mean(smoothed_magnitude_response))
        if verbose:
            logger.debug(f'The RMS error is {rms_error} dB')
        return rms_error

    ###########################################################################

    seq_length = len(init_vn_seq)
    decay_slope = -np.log(np.power(10, -decay_db / 20)) / seq_length
    init_amplitude = get_sequence_amplitude(init_impulse_locations,
                                            decay_slope)
    grid_spacing = sample_rate / num_samps_per_second
    impulse_signs = np.sign(init_vn_seq[init_impulse_locations])
    num_impulses = len(init_impulse_locations)

    # get log frequency axis
    lin_freqs_hz = np.linspace(np.log(20), np.log(sample_rate / 2),
                               num_freq_bins)
    log_freqs_hz = np.exp(lin_freqs_hz)

    # cost function
    # vn_params[:num_impulses] are the impulse locations
    # vn_params[num_impulses:] are the amplitudes
    cost_function = lambda vn_params: compute_spectral_error(  # noqa : E731
        vn_params[:num_impulses], vn_params[num_impulses:], impulse_signs,
        num_freq_bins, sample_rate, log_freqs_hz, verbose)

    # initial parameters
    init_vn_params = np.concatenate((init_impulse_locations, init_amplitude))

    # constraints
    # the first impulse location is 0, and the initial amplitude is 1
    inds = [0, num_impulses]
    cons = ({
        'type': 'eq',
        'fun': lambda vn_params: vn_params[inds] - np.array([0, 1.0]),
    })

    # bounds
    impulse_index = np.arange(num_impulses)
    lower_bounds = np.concatenate((grid_spacing * impulse_index - 1,
                                   init_amplitude / max_gain_deviation))
    upper_bounds = np.concatenate(
        (grid_spacing * impulse_index, init_amplitude * max_gain_deviation))
    bnds = Bounds(lower_bounds, upper_bounds)

    # options for convergence
    opts = {'maxiter': max_iter, 'disp': False}

    res = minimize(cost_function,
                   init_vn_params,
                   bounds=bnds,
                   constraints=cons,
                   options=opts)
    logger.debug(f'Optimisation was successful {res.success}')
    opt_vn_params = res.x
    opt_impulse_locations = np.int32(np.round(opt_vn_params[:num_impulses]))
    opt_amplitude = opt_vn_params[num_impulses:]
    opt_vn_sequence = np.zeros_like(init_vn_seq)
    opt_vn_sequence[opt_impulse_locations] = impulse_signs * opt_amplitude

    return opt_vn_sequence, opt_vn_params, res.fun


def generate_parallel_velvet_filters(
        fs: Union[int, float],
        num_parallel_filters: int,
        min_length_ms: float,
        max_length_ms: float,
        num_samps_per_second: int = 1000,
        compact_sequence: bool = False,
        decay_t60_ms: float = 5.0,
        normalise_energy: bool = False) -> npt.NDArray:
    """
    Generate a set of non overlapping multichannel VN filters.
    The impulse sequences in the different channels should not overlap.
    Args:
        fs (int or float): sampling frequency(Hz)
        num_parallel_filters(int): number of parallel filters
        min_length_ms (float): minimum length of filters in the multichannel VN (in ms)
        max_length_ms (float): maximum length of filters in the multichannel VN (in ms)
        num_samps_per_second(int): density of the VN sequence
        compact_sequence (bool): is the sequence concetrated at the beginning?
        decay_t60_ms (float): the -60dB decay rate in ms
        normalise_energy (bool): scale the filters so that their energy is normalised
    Returns:
        NDArray: The multichannel velvet noise sequence
    """
    lower_lim = ms_to_samps(min_length_ms, fs)
    upper_lim = ms_to_samps(max_length_ms, fs)
    # 1 impulse per grid_size samples. Alary suggests 1000 samples per second.
    grid_size = np.int32(fs / num_samps_per_second)
    # this is a very sparse sequence
    vn_sequence = generate_interleaved_velvet_noise(num_parallel_filters,
                                                    grid_size,
                                                    (lower_lim, upper_lim),
                                                    shorten=True,
                                                    compact=compact_sequence)
    no_vn_sequence = generate_non_overlapping_interleaved_velvet_noise(
        vn_sequence, grid_size)
    velvet_filter_length = no_vn_sequence.shape[1]

    # add decay to sequence to prevent smearing of transients
    # decay rate in db/sec
    num_non_zero = np.count_nonzero(no_vn_sequence, axis=1)
    decay_slope = np.log(1000) / (decay_t60_ms / 1e3)
    if compact_sequence:
        # if the sequence is compact, we want the decay to start wherever the VN sequence starts.
        # the decay only gets applied to the non-zero elements.
        decay_env = np.zeros_like(no_vn_sequence)
        for i in range(num_parallel_filters):
            # apply decay to non-zero taps only
            non_zero_inds = np.nonzero(no_vn_sequence[i, :])[0]
            time_axis = np.arange(num_non_zero[i]) / fs
            decay_env[i, non_zero_inds] = np.exp(-decay_slope * time_axis)
    else:
        # if the sequence is not compact, the decay can start at 0
        decay_slope_vec = decay_slope * np.ones((num_parallel_filters, 1))
        time_axis = np.arange(velvet_filter_length) / fs
        decay_env = np.exp(
            -np.matmul(decay_slope_vec, time_axis[np.newaxis, :]))

    decay_vn_sequence = decay_env * no_vn_sequence
    if normalise_energy:
        decay_vn_sequence, _ = normalise_irs(decay_vn_sequence, fs)

    return decay_vn_sequence


def generate_parallel_white_noise_filters(
        fs: Union[int, float],
        num_parallel_filters: int,
        length_ms: float,
        rms_db: float = 0.0,
        decay_t60_ms: float = 5.0,
        seed: int = 155346,
        normalise_energy: bool = False) -> npt.NDArray:
    """Generate a set of exponentially decaying parallel white noise filters
    Args:
        fs (int or float): sampling frequency(Hz)
        num_parallel_filters(int): number of parallel filters
        length_ms (float): length of filters in ms
        rms_db (float): the RMS value of the noise sequence in dB
        decay_t60_ms (float): the -60dB decay rate in ms
        seed (int): the random number generator seed
        normalise_energy (bool): scale the filters so that their energy is normalised

    Returns:
        NDArray: The multichannel white noise sequence
    """
    # filter lengths
    np.random.seed(seed)
    filter_lengths = np.array(ms_to_samps(length_ms, fs) *
                              np.ones(num_parallel_filters),
                              dtype=int)

    white_noise_filter_length = filter_lengths[0]
    shaped_noise_seq = np.zeros(
        (num_parallel_filters, white_noise_filter_length), dtype=float)
    # decay rate in dB / sec
    desired_slope = np.log(1000) / (decay_t60_ms / 1e3)

    for k in range(num_parallel_filters):
        seq_length_samps = filter_lengths[k]
        noise_pf = pf.signals.noise(seq_length_samps,
                                    spectrum='white',
                                    rms=db2lin(rms_db),
                                    sampling_rate=fs)
        noise_seq = noise_pf.time
        time_idx = noise_pf.times
        # impose a decay on the WN sequence
        shaped_noise_seq[k, :] = np.exp(-desired_slope * time_idx) * noise_seq

    if normalise_energy:
        shaped_noise_seq, _ = normalise_irs(shaped_noise_seq, fs)

    return shaped_noise_seq


def plot_interleaved_velvet_noise(vn_seq):
    num_seq = vn_seq.shape[0]
    fig, ax = plt.subplots()
    for i in range(num_seq):
        fig.add_subplot(num_seq, 1, i + 1)
        mkl, _, _ = plt.stem(vn_seq[i, :])
        plt.setp(mkl, markersize=3)

    return fig, ax


def process_velvet(input_signal: np.ndarray,
                   fs: float,
                   vn_seq_path: Optional[Path] = None) -> np.ndarray:
    """Process a stereo input with two channels of VN sequence"""
    _, num_channels = input_signal.shape
    if num_channels > 2:
        input_signal = input_signal.T
        num_channels = 2
    if num_channels != 2:
        raise RuntimeError("Input signal must be stereo!")

    try:
        vn_seq = np.loadtxt(vn_seq_path, dtype='f', delimiter=" ")
    except:
        raise OSError("Error reading file!")

    output_signal = np.zeros_like(input_signal)
    for chan in range(num_channels):
        output_signal[:, chan] = fftconvolve(
            input_signal[:, chan], vn_seq[chan, :])[:output_signal.shape[0]]

    return output_signal
