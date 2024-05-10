import numpy as np
import numpy.typing as npt
import pyfar as pf
from typing import Tuple
from abc import ABC
from enum import Enum
from pathlib import Path
from velvet import process_velvet
from allpass import process_allpass
from utils import calculate_interchannel_coherence, calculate_interchannel_cross_correlation_matrix

VN_PATH = Path('../../../Resources/init_vn_filters.txt')
OPT_VN_PATH = Path('../../../Resources/opt_vn_filters.txt')


class FilterbankType(Enum):
    AMP_PRESERVE = "amplitude-preserve"
    ENERGY_PRESERVE = "energy-preserve"


class DecorrelationType(Enum):
    ALLPASS = "allpass"
    VELVET = "velvet"
    OPT_VELVET = "opt_velvet"


class StereoWidener(ABC):
    """Parent stereo widener class"""

    def __init__(self, input_stereo: np.ndarray, fs: float,
                 decorr_type: DecorrelationType, beta: float):
        """Args: 
		   input_stereo (ndarray) : input stereo signal
		   fs (float) : sampling frequency
		   decorr_type (Decorrelation type) : decorrelation type (allpass, velvet or opt_velvet)
		   beta_init (between 0 and pi/2): crossfading factor (initial)
		 """

        self.input_signal = input_stereo
        self.fs = fs
        self.decorr_type = decorr_type
        self.beta = beta

        _, self.num_channels = input_stereo.shape
        if self.num_channels > 2:
            self.input_signal = self.input_signal.T
            self.num_channels = 2
        if self.num_channels != 2:
            raise RuntimeError("Input signal must be stereo!")

        self.decorrelated_signal = self.decorrelate_input()

    def decorrelate_input(self) -> np.ndarray:
        if self.decorr_type == DecorrelationType.ALLPASS:
            decorrelated_signal = process_allpass(self.input_signal,
                                                  self.fs,
                                                  num_biquads=200)
        elif self.decorr_type == DecorrelationType.VELVET:
            decorrelated_signal = process_velvet(self.input_signal, self.fs,
                                                 VN_PATH)
        elif self.decorr_type == DecorrelationType.OPT_VELVET:
            decorrelated_signal = process_velvet(self.input_signal, self.fs,
                                                 OPT_VN_PATH)
        else:
            raise NotImplementedError("Other decorrelators are not available")
        return decorrelated_signal

    def process(self):
        pass


class StereoWidenerBroadband(StereoWidener):
    """Broadband stereo widener class"""

    def __init__(self, input_stereo: np.ndarray, fs: float,
                 decorr_type: DecorrelationType, beta: float):
        super().__init__(input_stereo, fs, decorr_type, beta)

    def update_beta(self, new_beta: float):
        self.beta = new_beta

    def process(self) -> np.ndarray:
        stereo_output = np.zeros_like(self.input_signal)
        for chan in range(self.num_channels):
            stereo_output[:, chan] = np.cos(
                self.beta) * self.input_signal[:, chan] + np.sin(
                    self.beta) * self.decorrelated_signal[:, chan]
        return stereo_output

    @staticmethod
    def calculate_correlation(output_signal: np.ndarray) -> float:
        return calculate_interchannel_coherence(output_signal[:, 0],
                                                output_signal[:, 1],
                                                time_axis=0)


class StereoWidenerFrequencyBased(StereoWidener):

    def __init__(self, input_stereo: np.ndarray, fs: float,
                 filterbank_type: FilterbankType,
                 decorr_type: DecorrelationType, beta: Tuple[float, float],
                 cutoff_freq: float):
        """Frequency based stereo widener
		Args:
			input_stereo (ndarray): input stereo signal
			fs (float): sampling rate
			filterbank_type (Filterbank type): amplitude or energy preserving
			decorr_type (Decorrelation type): allpass, velvet or opt-velvet
			beta (Tuple(float, float)): cross-fading gain for low and high frequencies
			cutoff_freq (float): cutoff frequency of filterbank (Hz)
		"""

        super().__init__(input_stereo, fs, decorr_type, beta)
        self.filterbank_type = filterbank_type
        self.cutoff_freq = cutoff_freq
        self.get_filter_coefficients()

    def get_filter_coefficients(self):
        if self.filterbank_type == FilterbankType.AMP_PRESERVE:
            # Linkwitz Riley crossover filterbank
            filters = pf.dsp.filter.crossover(signal=None,
                                              N=4,
                                              frequency=self.cutoff_freq,
                                              sampling_rate=self.fs)
            self.lowpass_filter_coeffs = pf.classes.filter.FilterSOS(
                filters.coefficients[0, ...], self.fs)
            self.highpass_filter_coeffs = pf.classes.filter.FilterSOS(
                filters.coefficients[1, ...], self.fs)

        elif self.filterbank_Type == FilterbankType.ENERGY_PRESERVE:

            self.lowpass_filter_coeffs = pf.dsp.filter.butterworth(
                signal=None,
                N=16,
                frequency=self.cutoff_freq,
                btype='lowpass',
                sampling_rate=self.fs)

            self.highpass_filter_coeffs = pf.dsp.filter.butterworth(
                signal=None,
                N=16,
                frequency=self.cutoff_freq,
                btype='highpass',
                sampling_rate=self.fs)

        else:
            raise NotImplementedError(
                "Only Butterworth and LR crossover filters are available")

    def filter_in_subbands(self, signal: np.ndarray) -> np.ndarray:
        """Filter signal into two frequency bands"""
        pf_signal = pf.classes.audio.Signal(signal, self.fs)
        lowpass_signal = self.lowpass_filter_coeffs.process(pf_signal).time
        highpass_signal = self.highpass_filter_coeffs.process(pf_signal).time
        return np.vstack((lowpass_signal, highpass_signal))

    def update_beta(self, new_beta: Tuple[float, float]):
        self.beta = new_beta

    def update_cutoff_frequency(self, new_cutoff_freq: float):
        self.cutoff_freq = new_cutoff_freq
        self.get_filter_coefficients()

    def process(self):
        stereo_output = np.zeros_like(self.input_signal)
        filtered_input = np.zeros(
            (2, self.input_signal.shape[0], self.num_channels))
        filtered_decorr = np.zeros_like(filtered_input)

        for chan in range(self.num_channels):
            filtered_input[..., chan] = self.filter_in_subbands(
                self.input_signal[:, chan])
            filtered_decorr[..., chan] = self.filter_in_subbands(
                self.decorrelated_signal[:, chan])

            for k in range(self.num_channels):
                stereo_output[:, chan] += np.cos(self.beta[k]) * np.squeeze(
                    filtered_input[k, :, chan]) + np.sin(
                        self.beta[k]) * np.squeeze(filtered_decorr[k, :, chan])

        return stereo_output

    def calculate_interchannel_coherence(self, output_signal: np.ndarray):
        icc_matrix, icc_freqs = calculate_interchannel_cross_correlation_matrix(
            output_signal,
            fs=self.fs,
            num_channels=self.num_channels,
            time_axis=0,
            channel_axis=-1,
            bands_per_octave=3,
            freq_range=(20, self.fs / 2.0))
        icc_vector = np.squeeze(icc_matrix[..., 0, 1])
        return icc_vector, icc_freqs
