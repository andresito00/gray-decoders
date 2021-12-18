from enum import Enum
import numpy as np
import time
import matplotlib.pyplot as plt
from typing import List, Callable, Generator, Optional
import uuid
from stimuli import Stimuli

class SpikeDistribution(Enum):
    EXP = 0
    GAMMA = 1
    POISSON = 2

class NeuronError(Exception):
    pass

class NeuronSimulator:

    def __init__(
        self,
        distribution: SpikeDistribution,
        scaling_factor: float = None,
        group_name="unk"
    ):
        if distribution == SpikeDistribution.EXP:
            rand_func = lambda beta: int(np.random.exponential(beta))
        elif distribution == SpikeDistribution.GAMMA:
            if scaling_factor is None:
                raise NeuronError(
                    "Gamma distribution type requires a scaling factor.")
            shape = scaling_factor + 1
            rand_func = lambda beta: int(np.random.gamma(shape, beta))
        elif distribution == SpikeDistribution.POISSON:
            self.rng = np.random.default_rng()
            rand_func = lambda beta: int(self.rng.poisson(beta))
        else:
            raise NeuronError(f"Distribution {distribution} not implemented!")

        self.rand_func = rand_func
        self.distribution = distribution
        self.id = uuid.uuid1()
        self.group = group_name
        # These two are provisioned by assigning the rate function either by
        # tuning or providing a given one.
        self.rate_func = None
        self.preferred_stimulus = None

        # Only needed when tuning.
        self.k = None # cosine model tuning coefficients

    def _get_tuned_rates(self, theta: np.ndarray) -> np.ndarray:
        """
        Using previously tuned cosine model coefficients, return the predicted
        spike rate (Hz) for this neuron for the given reach direction
        """
        assert k is not None
        theta = np.rad2deg(theta)
        return self.k[0][0] + self.k[1][0]*np.sin(theta) + \
                self.k[2][0]*np.cos(theta)

    def plot_cosine_model(self, figure_number: int) -> None:
        theta = np.linspace(0, 2*np.pi, 80)
        exp_firing_rates = self.k[0][0] + self.k[1][0]*np.sin(theta) + \
                            self.k[2][0]*np.cos(theta)
        plt.figure(figure_number)
        plt.plot(dirs, rates, 'r*')
        plt.plot(np.rad2deg(theta), exp_firing_rates)
        plt.xlabel('angle (degrees)')
        plt.ylabel('firing rate (spikes/s)')
        plt.draw()


    def tune_cosine_model(
        self,
        dirs: np.array,
        rates: np.array,
        show_plots: bool = False
    ) -> np.ndarray:
        """
        Takes an array of average firing rates of a neuron
        during a reach to dirs[0] degrees, dirs[1] degrees, and so on.
        It computes the tuning model, f(theta) = c0 + c1 cos(theta - theta0).

        big picture: the cosine tuning model asserts
        this needs to be computed for every sensor:

        f(theta) = c0 + c1*cos(theta - theta0)

        Considering:
        --> cos(theta - theat0) = cos(theta)*cos(theta0) + sin(theta)*sin(theta0)

        To simplify parameter estimation:
        --> f(theta) = k0 + k1*sin(theta) + k2*cos(theta)

        Given a column vector of size M for measured rate:

        [              [                                   [
         rate1          1  sin(theta1)  cos(theta1)         k0
         rate2          1  sin(theta2)  cos(theta2)         k1
         ...      =                ...                 .    k2
         rateM          1  sin(thetaM)  cos(thetaM)        ]
        ]              ]

        Solving for vector k with least error
        """
        # This shouldn't be known unless we already have the rate function.
        assert self.preferred_direction is None
        assert self.rate_func is None

        rads = np.radians(dirs)
        A = np.hstack((
            np.ones((3, 1), order='F'), np.sin(rads), np.cos(rads)
        ))
        self.k = np.linalg.lstsq(A, rates, rcond=None)[0]
        self.preferred_stimulus = np.atan(k[1][0] / k[2][0])
        self.rate_func = self._get_tuned_rates

        return self.k

    def assign_rate_func(
        self,
        rate_func: Callable[[Stimuli], np.ndarray],
        preferred_stimulus: Stimuli
    ):
        """
        Assigns the rate function for this neuron if tuning isn't used.
        At this time rate functions recevie an array of action directions
        and return an array of rates (1:1).
        """
        self.preferred_stimulus = preferred_stimulus
        self.rate_func = rate_func


    def get_rates(self, stimuli: Stimuli) -> np.ndarray:
        """
        Wrapper function for the assigned rate func
        """
        if self.rate_func is None:
            raise NeuronError(
                f"No rate function assigned to neuron {self.group}-{self.id}")

        return self.rate_func(stimuli, self.preferred_stimulus)

    @staticmethod
    def plot_rasters(spike_trains: List[np.ndarray], figure_number: Optional[int]) -> None:
        plt.figure(figure_number)
        plt.eventplot(spike_trains)
        plt.xlabel("time (ms)")
        plt.ylabel("sensor")
        plt.draw()

    def generate_rasters(
        self,
        spike_rates: np.ndarray, # spikes/s
        intervals: np.ndarray,     # ms
        num_trials: int,
        start_time: int, # ms
    ) -> Generator[np.ndarray, None, None]:
        """
        The following generates and returns spike rasters by using
        delta-t from a random exponential distribution characterized by...
        """
        betas = 1000/spike_rates
        for i in range(0, num_trials):
            spike_train = []
            prev_t_spike = start_time
            dt = start_time
            duration = np.sum(intervals)

            for rate, beta, interval in zip(spike_rates, betas, intervals):
                if rate > 0:
                    while dt <= duration and (dt - prev_t_spike) <= interval:
                        dt += self.rand_func(beta)
                        spike_train.append(int(dt))

                else:
                    dt += interval

                prev_t_spike = dt

            yield np.asarray(spike_train)

    @staticmethod
    def _mask_pad(array: np.ndarray, max_length: int) -> np.ndarray:
        """
        Utility to create a masked array that fills to max_length
        """
        ogLen = len(array)
        assert ogLen <= max_length
        masked_array = np.asarray(np.zeros(max_length))
        for i in range(0, max_length):
            masked_array[i] = array[i] if i < ogLen else np.ma.masked
        return masked_array

    @staticmethod
    def generate_inter_spike_interval_hist(
        spike_trains: np.ndarray,
        duration: int, # ms
        figure_number: Optional[int],
        show_plots: bool = False,
    ) -> List[np.ndarray]:
        """
        Creates a histogram of the inter-spike intervals in the provided rasters
        """
        isi = []
        # max diff intervals length of the provided spike_trains
        max_length = len(max(spike_trains, key=len)) - 1
        for trial in spike_trains:
            intervals = np.diff(trial)
            current_length = len(intervals)
            masked_intervals = intervals
            if current_length < max_length:
                masked_intervals = NeuronSimulator._mask_pad(intervals,
                    max_length)
            isi.append(masked_intervals)

        if show_plots:
            plt.figure(figure_number)
            plt.hist(isi)
            plt.xlabel('inter-spike interval (ms)')
            plt.ylabel('# spikes')
            plt.draw()

        return isi

    @staticmethod
    def generate_spike_time_hist(
        spike_trains: np.ndarray,
        duration: int, # ms
        bin_size: int, # ms
        figure_number: Optional[int],
        show_plots: bool = False,
    ) -> np.ndarray:
        """
        Creates a histogram of the binned spike counts
        from neurons modeled by the Poisson process for a given bin size.
        """
        num_trials = len(spike_trains)
        start = 0
        sentinel = start + bin_size
        idx = 0;
        num_bins = int(duration/bin_size)
        spike_counts = np.zeros(num_bins, dtype=int)

        while (sentinel < duration):
            for trial in range(0, num_trials):
                # find all values between start and stop for this trial
                spike_counts[idx] += \
                    np.where(
                        np.logical_and(
                            spike_trains[trial] >= start,
                            spike_trains[trial] < sentinel)
                        )[0].size

            # now we'll take the average for this bin across all trials
            # and increment our counters
            spike_counts[idx] /= (bin_size * (10**(-3)) * num_trials)
            start += bin_size
            sentinel = start + bin_size
            idx += 1

        if show_plots:
            plt.figure(figure_number)
            plt.bar(
                range(0, num_bins), spike_counts, align='center')
            plt.xlabel(f"bins ({bin_size}s of ms)")
            plt.ylabel("avg spike count")
            plt.draw()

        return spike_counts
