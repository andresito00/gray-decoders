from enum import Enum
import numpy as np
import time
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
        scaling_factor: Optional[float] = None,
        rate_func: Optional[Callable[[Stimuli], np.ndarray]] = None,
        preferred_stimulus: Optional[Stimuli] = None,
        group_name: str = "unk",
    ):
        if distribution == SpikeDistribution.EXP:
            rand_func = lambda beta: np.random.exponential(beta)
        elif distribution == SpikeDistribution.GAMMA:
            if scaling_factor is None:
                raise NeuronError("Gamma distribution type requires a scaling factor.")
            shape = scaling_factor + 1
            rand_func = lambda beta: np.random.gamma(shape, beta)
        elif distribution == SpikeDistribution.POISSON:
            self.rng = np.random.default_rng()
            rand_func = lambda beta: self.rng.poisson(beta)
        else:
            raise NeuronError(f"Distribution {distribution} not implemented!")

        self.rand_func = rand_func
        self.distribution = distribution
        self.id = uuid.uuid1()
        self.group = group_name
        # These two are provisioned by assigning the rate function either by
        # tuning or providing a given one.
        self.rate_func = rate_func
        self.preferred_stimulus = preferred_stimulus

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

    def tune_cosine_model(
        self,
        dirs: np.array,
        rates: np.array,
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
        if self.preferred_stimulus or self.rate_func:
            raise NeuronError("Rate function parameters already set!")
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

    def generate_rasters(
        self,
        spike_rates: np.ndarray, # spikes/s
        intervals_ms: np.ndarray,   # ms
        num_trials: int,
        start_time: int, # ms
    ) -> Generator[np.ndarray, None, None]:
        """
        The "workhorse" of the NeuronSimulator class. This one should be optimized to produce.
        rasters.

        Note a real neuron's refractory period is on the order of ms.

        The following generates and returns spike rasters by using
        delta-t from a random exponential distribution characterized by...

        :param spike_rates: the firing rate of a neuron
        :param intervals_ms: the duration for each spike rate (1:1 mapping)
        :param num_trials: number of trials (rasters to generate)
        :param start_time: "grounds" the spike events in absolutely. "spike n happens at time t."
        """
        with np.errstate(divide='ignore'):
            betas = 1000/spike_rates # convert s to ms
        duration = np.sum(intervals_ms)
        for i in range(0, num_trials):
            spike_train = []
            prev_t_spike = start_time
            dt = start_time
            for rate, beta, interval in zip(spike_rates, betas, intervals_ms):
                if rate > 0:
                    while dt <= duration and (dt - prev_t_spike) <= interval:
                        spike_delta = self.rand_func(beta)
                        dt += spike_delta
                        spike_train.append(int(dt))
                else:
                    dt += interval

                prev_t_spike = dt

            yield np.asarray(spike_train)
