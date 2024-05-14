import numpy as np
from numpy.typing import ArrayLike, NDArray
from typing import List
import matplotlib.pyplot as plt
from utils import ms_to_samps

_HIGH_EPS = 1e9


class OnsetDetector():
    """
    Onset detector with a leaky integrator"""

    def __init__(self,
                 fs: float,
                 attack_time_ms: float = 5.0,
                 release_time_ms: float = 50.0,
                 min_onset_hold_ms: float = 20.0,
                 min_onset_sep_ms: float = 50.0):
        """
        Args:
            fs (float): sampling rate in Hz
            attack_time_ms (float): leaky integrator attack time
            release_time_ms (float): leaky integrator release time
            min_onset_hold_ms (float): minimum time to wait
                                       before an onset becomes an offset
            min_onset_sep_ms (float): minimum separation between two
                                      onsets in ms
        """
        self.fs = fs
        self.leaky = LeakyIntegrator(fs, attack_time_ms, release_time_ms)
        self._onset_flag = []
        self.min_onset_hold_samps = int(ms_to_samps(min_onset_hold_ms,
                                                    self.fs))
        self.min_onset_sep_samps = int(ms_to_samps(min_onset_sep_ms, self.fs))
        self._threshold = None
        self._signal_env = None

    @property
    def signal_env(self) -> NDArray:
        return self._signal_env

    @property
    def threshold(self) -> NDArray:
        return self._threshold

    @property
    def running_sum_thres(self) -> float:
        return self._running_sum_thres

    @property
    def onset_flag(self) -> List[bool]:
        return self._onset_flag

    @staticmethod
    def check_local_peak(cur_samp: float, prev_samp: float, next_samp: float):
        """
        Given the current, previous and next samples, check if the current sample
        is a local peak
        """
        if cur_samp > prev_samp and cur_samp > next_samp:
            return True
        else:
            return False

    @staticmethod
    def check_direction(cur_samp: float,
                        prev_samp: float,
                        next_samp: float,
                        is_rising: bool = True) -> bool:
        """
        Check whether the signal envelope is rising or falling. 
        The flag `is_rising` is used to check for rising envelopes
        """
        if is_rising:
            return True if cur_samp > prev_samp and cur_samp < next_samp else False
        else:
            return True if cur_samp < prev_samp and cur_samp > next_samp else False

    def process(self, input_signal: NDArray, to_plot: bool = False):
        """Given an input signal, find the location of onsets"""
        if (input_signal.ndim == 2 and input_signal.shape[1] > 1):
            input_signal = input_signal[:, 0]
        num_samp = len(input_signal)
        self._signal_env = self.leaky.process(input_signal)
        # onet flag is a list of bools
        self._onset_flag = [False for k in range(num_samp)]
        # threshold for onset calculation, calculated dynamically
        self._threshold = np.ones(num_samp) * _HIGH_EPS
        # running sum to calculate mean of the signal envelope
        self._running_sum_thres = 0.0
        hold_counter = 0
        inhibit_counter = 0

        for k in range(1, num_samp - 1):
            cur_samp = self._signal_env[k]
            prev_samp = self._signal_env[k - 1]
            next_samp = self._signal_env[k + 1]

            is_local_peak = self.check_local_peak(cur_samp, prev_samp,
                                                  next_samp)

            # running sum of the signal envelope
            self._running_sum_thres += self.signal_env[k]

            # threshold is 1.4 * mean of envelope if there is a local peak
            self._threshold[k] = 2 * (self._running_sum_thres /
                                      k) if is_local_peak else self._threshold[
                                          k - 1]

            # if an onset is detected, the flag will be true for a minimum number
            # of frames to prevent false offset detection
            if 0 < hold_counter < self.min_onset_hold_samps:
                hold_counter += 1
                self.onset_flag[k] = True
                continue
            # if an offset is detected, the flag will be false for a minimum number
            # of frames to prevent false onset detection
            elif 0 < inhibit_counter < self.min_onset_sep_samps:
                inhibit_counter += 1
                self.onset_flag[k] = False
                continue
            else:
                hold_counter = 0
                inhibit_counter = 0
                # if the signal is rising and the value is greater than the
                # mean of the thresholds so far
                if self.check_direction(
                        cur_samp, prev_samp, next_samp,
                        is_rising=True) and cur_samp > (self._threshold[k]):
                    self._onset_flag[k] = True
                    hold_counter += 1
                # if the signal is fallng and the value is lesser than the
                # mean of the thresholds so far
                elif self.check_direction(
                        cur_samp, prev_samp, next_samp,
                        is_rising=False) and cur_samp < (self._threshold[k]):
                    self._onset_flag[k] = False
                    inhibit_counter += 1

        if to_plot:
            ax = self.plot(input_signal)

    def plot(self, input_signal: NDArray):
        """Plot the input signal and the detected signal, threshold and onsets"""
        num_samp = len(input_signal)
        time_vector = np.arange(0, num_samp / self.fs, 1.0 / self.fs)
        onset_pos = np.zeros_like(time_vector)
        onset_idx = np.where(self._onset_flag)[0]
        onset_pos[onset_idx] = 1.0
        print(onset_idx)

        fig, ax = plt.subplots(figsize=(6, 4))
        ax.plot(time_vector, input_signal, label='input signal')
        ax.plot(time_vector, self._signal_env, label='envelope')
        ax.plot(time_vector, self._threshold, label='threshold')
        ax.plot(time_vector, onset_pos, 'k--', label='onsets')
        ax.legend(loc='lower left')
        ax.set_ylim([-1.0, 1.0])
        ax.set_xlim([0, 5.0])
        plt.show()

        return ax


class LeakyIntegrator():
    """Leaky integrator for signal envelope detection"""

    def __init__(self, fs: float, attack_time_ms: float,
                 release_time_ms: float):
        self.fs = fs
        self.attack_time_ms = attack_time_ms
        self.release_time_ms = release_time_ms

    def process(self, input_signal: NDArray) -> NDArray:
        """Estimate the signal amplitude envelope with a leaky integrator.

        Leaky integrator = a first-order IIR low-pass filter.

        Args:
            input_signal (npt.NDArray): The impulse response (should be 1-dimensional array or only the 1st column is taken)
            fs (float): Sample rate
            attack_time_ms (float): Integrator attack time in milliseconds, by default 5
            release_time_ms (float): Integrator release time in milliseconds, by default 50

        Returns:
            npt.NDArray: The envelope of the impulse response

        """
        # find envelope with a leaky integrator
        if (input_signal.ndim == 2 and input_signal.shape[1] > 1):
            input_signal = input_signal[:, 0]

        tau_a = self.attack_time_ms * self.fs * 1e-3
        tau_r = self.release_time_ms * self.fs * 1e-3
        signal_length = len(input_signal)
        signal_env = np.zeros_like(input_signal)
        for n in range(1, signal_length):
            if input_signal[n] > signal_env[n - 1]:
                signal_env[n] = signal_env[n - 1] + (
                    1 - np.exp(-1 / tau_a)) * (np.abs(input_signal[n]) -
                                               signal_env[n - 1])
            else:
                signal_env[n] = signal_env[n - 1] + (
                    1 - np.exp(-1 / tau_r)) * (np.abs(input_signal[n]) -
                                               signal_env[n - 1])

        return signal_env
