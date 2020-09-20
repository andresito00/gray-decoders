from enum import Enum
import numpy as np
import time
import matplotlib.pyplot as plt
from typing import List

class SpikeDistribution(Enum):
    EXP = 0
    GAMMA = 1
    POISSON = 2

class NeuronSimulator:

    def __init__(self, distribution: SpikeDistribution, scaling_factor=None):
        if distribution == SpikeDistribution.EXP:
            rand_func = lambda beta: int(np.random.exponential(beta))
        elif distribution == SpikeDistribution.GAMMA:
            if scaling_factor is None:
                raise ValueError(
                    "Gamma distribution type requires a scaling factor.")
            shape = k + 1
            rand_func = lambda beta: int(np.random.exponential(shape, beta))
        elif distribution == SpikeDistribution.POISSON:
            self.rng = np.random.default_rng()
            rand_func = lambda beta: int(self.rng.poisson(beta))
        else:
            raise ValueError(f"Distribution {distribution} not implemented!")

        self.rand_func = rand_func
        self.distribution = distribution
        self.k = None # cosine model tuning coefficients

    def tune_cosine_model(
        self,
        dirs: np.array,
        rates: np.array,
        show_plots=False
    ) -> np.ndarray:
        """
        Takes an array of average firing rates of a neuron
        during a reach to dirs[0] degrees, dirs[1] degrees, and so on.
        It computes the tuning model, f(theta) = c0 + c1 cos(theta - theta0).

        The outputs, c0, c1, and theta0 are the parameters of the tuning model.

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
        rads = np.radians(dirs)
        A = np.hstack((
            np.ones((3, 1), order='F'), np.sin(rads), np.cos(rads)
        ))
        self.k = np.linalg.lstsq(A, rates, rcond=None)[0]

        if show_plots:
            theta = np.linspace(0, 2*np.pi, 80)
            exp_firing_rates = self.k[0][0] + self.k[1][0]*np.sin(theta) + \
                                self.k[2][0]*np.cos(theta)
            plt.figure(0)
            plt.plot(dirs, rates, 'r*')
            plt.plot(np.rad2deg(theta), exp_firing_rates)
            plt.xlabel('angle (degrees)')
            plt.ylabel('firing rate (spikes/s)')
            plt.draw()

        return self.k

    def get_tuned_spike_rates_rads(self, theta: np.ndarray) -> np.ndarray:
        """
        Using previously tuned cosine model coefficients, return the predicted
        spike rate (Hz) for this neuron for the given reach direction.
        """
        assert k is not None
        return self.k[0][0] + self.k[1][0]*np.sin(theta) + \
                self.k[2][0]*np.cos(theta)


    def get_tuned_spike_rates_deg(self, theta: np.ndarray) -> np.ndarray:
        """
        Using previously tuned cosine model coefficients, return the predicted
        spike rate (Hz) for this neuron for the given reach direction
        """
        assert k is not None
        theta = np.rad2deg(theta)
        return self.k[0][0] + self.k[1][0]*np.sin(theta) + \
                self.k[2][0]*np.cos(theta)


    def generate_rasters(
        self,
        spike_rates: List[float], # spikes/s
        intervals: List[int],     # ms
        num_trials: int,
        scaling_factor=None,      # (1 / k!) of the gamma random distribution
        show_plots=False
    ) -> List[np.ndarray]:
        """
        The following generates and returns spike rasters by using
        delta-t from a random exponential distribution characterized by...
        """
        betas = [1000 / r for r in spike_rates] # beta = 1  / lambda
        start_time = time.time()
        spike_trains = []
        for i in range(0, num_trials):
            spike_train = []
            prev_t_spike = 0
            dt = 0
            duration = sum(intervals)
            for beta, interval in zip(betas, intervals):
                while dt <= duration and (dt - prev_t_spike) <= interval:
                    dt += self.rand_func(beta)
                    spike_train.append(dt)
                prev_t_spike = dt
            spike_trains.append(np.asfortranarray(spike_train))
        elapsed_time = time.time() - start_time
        print(f"Generated in {elapsed_time} seconds")

        if show_plots:
            plt.figure(1)
            plt.eventplot(spike_trains)
            plt.xlabel("time (ms)")
            plt.ylabel("sensor")
            plt.draw()

        return spike_trains

    @staticmethod
    def _mask_pad(array: np.ndarray, max_length: int) -> np.ndarray:
        """
        Utility to create a masked array that fills to max_length
        """
        ogLen = len(array)
        assert ogLen <= max_length
        masked_array = np.asfortranarray(np.zeros(max_length))
        for i in range(0, max_length):
            masked_array[i] = array[i] if i < ogLen else np.ma.masked
        return masked_array

    @staticmethod
    def generate_inter_spike_interval_hist(
        spike_trains: np.ndarray,
        duration: int, # ms
        show_plots=False
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
            plt.figure(3)
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
        show_plots=False
    ) -> np.ndarray:
        """
        Creates a histogram of the binned spike counts
        from neurons modeled by the Poisson process for a given bin size.
        """
        num_trials = len(spike_trains)
        start = 0
        sentinel = start + bin_size
        idx = 0;
        spike_counts = np.zeros(int(duration/bin_size), dtype=int)

        while (sentinel < duration):
            for trial in range(0, num_trials):
                # find all values between start and stop for this trial
                spike_counts[idx] += \
                    np.where(
                        np.logical_and(
                            spike_trains[trial] > start,
                            spike_trains[trial] <= sentinel)
                        )[0].size

            # now we'll take the average for this bin across all trials
            # and increment our counters
            spike_counts[idx] /= (bin_size * (10**(-3)) * num_trials)
            start = start + bin_size
            sentinel = start + bin_size
            idx = idx + 1

        if show_plots:
            plt.figure(2)
            plt.bar(
                range(0, int(duration/bin_size)), spike_counts, align='edge')
            plt.xlabel("bin number")
            plt.ylabel("avg spike count")
            plt.draw()

        return spike_counts
