import numpy as np
from numpy.typing import NDArray, ArrayLike
from typing import Tuple
from scipy.fft import rfftfreq, rfft, irfft, fftshift
from scipy.signal import fftconvolve, oaconvolve
from interaural_cues import HRTFParams, get_hrtf_from_spherical_head_model, convert_ipd_to_itd
from utils import calculate_interchannel_cross_correlation_matrix, calculate_interchannel_coherence
import matplotlib.pyplot as plt


class HRTFStereoWidener():
    """
	Stereo widening with ITD modification. The distance between the two speakers
	is a controllable parameter. A spherical head model with nominal head radius is used
	"""

    def __init__(self,
                 sample_rate: float,
                 azimuth_range: Tuple[float, float],
                 angular_res_deg: float = 2,
                 num_freq_points: int = 2**9,
                 num_time_samples: int = 2**10,
                 head_radius: float = 0.075):
        """
		Args:
			sample_rate (float):
			azimuth_range (ArrayLike): the range of azimuth angles in ascending order
									   in which the stereo pair can lie.
			angular_res_deg (float): angular reslution of the azimuth values in degrees
			num_freq_points (int): number of frequency bins in HRTF
			num_time_samples (int): length of HRIRs in samples
			head_radius (float): head radius in metre for HRTF calculation
		"""
        self.sample_rate = sample_rate
        assert azimuth_range[0] <= azimuth_range[
            1], "Azimuth range should be ascending"
        self.azimuth_range = np.arange(azimuth_range[0],
                                       azimuth_range[1] + angular_res_deg,
                                       angular_res_deg)
        self.num_freq_points = num_freq_points
        self.num_time_samples = num_time_samples
        # angle between the head axis and the left speaker
        # (angle between head and right speaker is negative of this)
        self.speaker_angle_deg = 30.0
        # HRTF set derived from spherical head model
        self._hrtf_set = get_hrtf_from_spherical_head_model(
            self.azimuth_range, self.frequency_axis, self.num_time_samples,
            head_radius)

    @property
    def num_orientations(self):
        return len(self.azimuth_range)

    @property
    def frequency_axis(self):
        return rfftfreq(self.num_freq_points, d=1.0 / self.sample_rate)

    @property
    def left_ear_axis(self):
        return 0

    @property
    def right_ear_axis(self):
        return 1

    @property
    def num_inputs(self):
        return 2

    @property
    def num_outputs(self):
        return 2

    @property
    def hrtf_set(self):
        itd = convert_ipd_to_itd(self._hrtf_set.ipd,
                                 self.sample_rate,
                                 self.frequency_axis / (self.sample_rate / 2) *
                                 np.pi,
                                 wrapped_phase=False)
        self._hrtf_set.itd = itd
        return self._hrtf_set

    def update_speaker_angle(self, new_speaker_angle: float):
        assert self.azimuth_range[0] <= new_speaker_angle <= self.azimuth_range[
            -1], f"Speaker angle cannot exceed {azimuth_range[-1]} degrees"
        self.speaker_angle_deg = new_speaker_angle

    def find_closest_doa(self) -> Tuple[int, int]:
        all_doas = self._hrtf_set.doa
        left_closest_idx = np.argmin(np.abs(all_doas - self.speaker_angle_deg))
        right_closest_idx = np.argmin(
            np.abs(all_doas - (-self.speaker_angle_deg)))
        return (left_closest_idx, right_closest_idx)

    def process(self, input_signal: NDArray) -> NDArray:
        """Apply the right HRIRs to the direct and cross-talk path of the input signal"""

        # ensure input signal is of the right length
        _, num_channels = input_signal.shape
        if num_channels > self.num_inputs:
            input_signal = input_signal.T
            num_channels = self.num_inputs
        if num_channels != self.num_inputs:
            raise RuntimeError("Input signal must be stereo!")
        output_signal = np.zeros((input_signal.shape[0], self.num_outputs))

        # find closest DOA
        closest_left_idx, closest_right_idx = self.find_closest_doa()
        # HRIRs for both ears corresponding to left speaker
        left_spk_hrir = self._hrtf_set.hrirs[closest_left_idx, ...]
        # HRIR for both ears corresponding to right speaker
        right_spk_hrir = self._hrtf_set.hrirs[closest_right_idx, ...]
        # this array is num_time_samples x num_ears x num_speakers
        spk_hrirs = np.dstack([left_spk_hrir, right_spk_hrir])

        for ear in range(self.num_outputs):
            for spk in range(self.num_inputs):
                # ear axis stays constant
                output_signal[:, ear] += oaconvolve(
                    np.squeeze(input_signal[:, spk]),
                    np.squeeze(spk_hrirs[:, ear, spk]),
                    mode='same') * 0.5

        return output_signal

    def calculate_correlation_function(self,
                                       output_signal: np.ndarray) -> float:
        X = rfft(output_signal[:, 0], n=self.num_freq_points)
        Y = rfft(output_signal[:, 1], n=self.num_freq_points)
        corr_freq = (X * np.conj(Y)) / (
            np.sqrt(np.sum(np.abs(X)) + np.sum(np.abs(Y))))
        corr_func = fftshift(irfft(corr_freq, n=self.num_time_samples))
        return corr_func

    @staticmethod
    def calculate_correlation(output_signal: np.ndarray) -> float:
        return calculate_interchannel_coherence(output_signal[:, 0],
                                                output_signal[:, 1],
                                                time_axis=0)

    def calculate_interchannel_coherence(self, output_signal: NDArray):
        icc_matrix, icc_freqs = calculate_interchannel_cross_correlation_matrix(
            output_signal,
            fs=self.sample_rate,
            num_channels=self.num_outputs,
            time_axis=0,
            channel_axis=-1,
            bands_per_octave=3,
            freq_range=(20, self.sample_rate / 2.0))
        icc_vector = np.squeeze(icc_matrix[..., 0, 1])
        return icc_vector, icc_freqs
