from dataclasses import dataclass
import numpy as np


class StimuliError(Exception):
    pass


class Stimuli:
    """
    Should be the parent class of any neuronal stimulus.

    Child classes can define kinematic behavior and optical, auditory, or even
    olfactory stimulus.
    """

    def __init__(self, durations: np.ndarray):
        self.durations = durations


class ReachStimuli(Stimuli):
    def __init__(
        self, durations: np.ndarray, angles: np.ndarray, distances: np.ndarray
    ):
        super().__init__(durations)
        self.angles = angles
        self.distances = distances
