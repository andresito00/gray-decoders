#raster.py

# /**************************************************************************
# **
# ** Copyright (C) 2020 Andres Hernandez, all rights reserved.
# **
# **               gray-decoders
# **
# ** gray-decoders source and binaries are provided "as is" without any express
# ** or implied warranty of any kind with respect to this software.
# ** In particular the authors shall not be liable for any direct,
# ** indirect, special, incidental or consequential damages arising
# ** in any way from use of the software.
# **
# ** Everyone is granted permission to copy, modify and redistribute this
# ** source, provided:
# **  1.  All copies contain this copyright notice.
# **  2.  All modified copies shall carry a notice stating who
# **      made the last modification and the date of such modification.
# **  3.  No charge is made for this software or works derived from it.
# **      This clause shall not be construed as constraining other software
# **      distributed on the same medium as this software, nor is a
# **      distribution fee considered a charge.
# **
# ***************************************************************************/

# the meschach library has existing functions for some of this,
# but they can't parse compressed matlab data or sparse matrices...

import sys
import argparse
from enum import Enum
import numpy as np
import scipy.io as io
import time
import matplotlib.pyplot as plt
from raster_util import build_args
from typing import List

#REFACTORY_PERIOD_MS = 2 # time that must pass before a neuron fires again in milliseconds

class SpikeDistribution(Enum):
    EXP = 0
    GAMMA = 1
    POISSON = 2


def maskPad(array=None, max_len=0):
    """
    Utility to create a masked array that fills to max_len

    array: np.array() of shape (numSpikeEvents,)
    max_len: length to pad to
    """
    ogLen = len(array)
    assert ogLen <= max_len
    maskedArray = np.asfortranarray(np.zeros(max_len))
    for i in range(0, max_len):
        maskedArray[i] = array[i] if i < ogLen else np.ma.masked
    return maskedArray

def plotISIH(spike_trains, duration, show_plots):
    """
    Plot ISIH creates a histogram of the inter-spike intervals in the provided rasters

    spike_trains: list of np.array()s of shape (numSpikeEvents,)
    duration: units of milliseconds
    """

    isi=[]
    maxLen = len(max(spike_trains, key=len)) - 1 # max diff intervals length of the provided spike_trains
    for trial in spike_trains:
        # print(trial)
        intervals = np.diff(trial)
        # print(intervals)
        currLength = len(intervals)
        maskedIntervals = intervals
        if currLength < maxLen:
            maskedIntervals = maskPad(intervals, maxLen)
        isi.append(maskedIntervals)

    if show_plots:
        plt.figure(3)
        plt.hist(isi)
        plt.xlabel('inter-spike interval (ms)')
        plt.ylabel('# spikes')
        plt.draw()

def plotPSTH(
    spike_trains: np.ndarray,
    duration: int,
    bin_size: int,
    show_plots=False
) -> None:
    """
    PlotPSTH creates a histogram of the binned spike counts
    from neurons modeled by the Poisson process for a given bin size.

    spike_trains: list of np.arrays() of shape (1, numSpikeEvents)
    duration: units of milliseconds
    bin_size: units of milliseconds
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
                        spike_trains[trial] > start, spike_trains[trial] <= sentinel)
                    )[0].size

        # now we'll take the average for this bin across all trials and increment our counters
        spike_counts[idx] = spike_counts[idx] / (bin_size * (10**(-3)) * num_trials)
        start = start + bin_size
        sentinel = start + bin_size
        idx = idx + 1

    if show_plots:
        plt.figure(2)
        plt.bar(range(0, int(duration/bin_size)), spike_counts, align='edge')
        plt.xlabel("bin number")
        plt.ylabel("avg spike count")
        plt.draw()

def generate_rasters(
    spike_rates: List[float], # spikes/s
    intervals: List[int],     # ms
    num_trains: int,
    distribution: SpikeDistribution,
    k=None,              # the scaling factor (1 / k!) of the gamma random distribution
    show_plots=False
) -> np.ndarray:
    """
    The following generates and returns spike rasters by using
    delta-t from a random exponential distribution characterized by...
    """
    # TODO: Provide rand_func as a parameter
    betas = [1000 / r for r in spike_rates] # beta = 1  / lambda
    if distribution == SpikeDistribution.EXP:
        rand_func = lambda beta: int(np.random.exponential(beta))
    elif distribution == SpikeDistribution.GAMMA:
        assert k is not None
        shape = k + 1
        rand_func = lambda beta: int(np.random.exponential(shape, beta))
    elif distribution == SpikeDistribution.POISSON:
        rng = np.random.default_rng()
        rand_func = lambda beta: int(rng.poisson(beta))
    else:
        raise ValueError(f"Distribution {distribution} not implemented!")

    start_time = time.time()
    spike_trains = []
    for i in range(0, num_trains):
        spike_train = []
        prev_t_spike = 0
        dt = 0
        duration = sum(intervals)
        for beta, interval in zip(betas, intervals):
            while dt <= duration and (dt - prev_t_spike) <= interval:
                dt += rand_func(beta)
                spike_train.append(dt)
            # because we always go over by one sample
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

def plan_tuning_curve(dirs: np.array, rates: np.array, show_plots=False) -> List[float]:
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


    Given a column vector of size M for measured rates.

    [              [                                   [
     rate1          1  sin(theta1)  cos(theta1)         k0
     rate2          1  sin(theta2)  cos(theta2)         k1
     ...      =     ...                            .    k2
     rateM          1  sin(thetaM)  cos(thetaM)        ]
    ]              ]

    Solving for vector k with least error

    """


    rads = np.radians(dirs)
    A = np.hstack((
        np.ones((3, 1), order='F'), np.sin(rads), np.cos(rads)
    ))
    k = np.linalg.lstsq(A, rates, rcond=None)[0]

    theta = np.linspace(0, 2*np.pi, 80)
    exp_firing_rates = k[0][0] + k[1][0]*np.sin(theta) + k[2][0]*np.cos(theta)


    if show_plots:
        plt.figure(0)
        plt.plot(dirs, rates, 'r*')
        plt.plot(np.rad2deg(theta), exp_firing_rates)
        plt.xlabel('angle (degrees)')
        plt.ylabel('firing rate (spikes/s)')
        plt.draw()

def parseMatFile(mat_file: str):
    """
    think of matContents as a 2-D matrix where
    the rows are each trial
    the columns are each motor function target
    each cell in the matrix contains a structure with another 2-D matrix where
    the rows are each each neuron sensor
    the columns are millisecond, where 1 implies an action potential firing has been detected by that sensor
    """

    varName = io.whosmat(mat_file)[0][0]
    matContents = io.loadmat(mat_file)

    plan_training_data = matContents[varName]

    numTrials = plan_training_data.shape[0]  # m = rows
    numTargets = plan_training_data.shape[1] # n  = columns

    # dimensions will always be referred to as "m x n"

    trials = []
    for i in range(0, numTrials):

        targets = []
        for j in range(0, numTargets):

            # the 1st dimension of this internal structure contains the actual spike train data (a 2-D matrix)
            targets.append(plan_training_data[i,j][1])

        trials.append(targets)

    # convert the list of lists (of 2-D arrays) to a 4-dimensional numpy array such that
    # the dimensions are, in order, [trial, target, sensor, neural spike/ms]
    trials = np.asfortranarray(trials)

    outputDir = "parser_output/"
    filename, extension = mat_file.split('.')

    # the following loops save all trial data to a .txt file for each target
    for n in range(0, numTargets):
        print("Now parsing target " + str(n) + "...")
        with open(outputDir + filename + "_target" + str(n) + ".txt",'w') as f:

            for m in range(0, numTrials-1):
                curr_trial = trials[m,n,:,:]
                next_trial = trials[m+1,n,:,:]
                curr_trial = np.concatenate((curr_trial, next_trial), axis=1)

            # the following is the current format that the meschach matrix file input function expects to receive.
            f.write("Matrix: " + str(curr_trial.shape[0]) +  " by " + str(curr_trial.shape[1]) + "\n")

            # faster now that we've concatenated our training trials...
            np.savetxt(f, curr_trial, delimiter=' ', fmt='%i')

def main(args):
    mode = args.mode
    show_plots = args.show

    if(mode == 'file'):
        parseMatFile(args.mat_file)

    elif(mode == 'tune'):
        assert len(args.rates) == len(args.dirs)
        rates = np.asfortranarray(args.rates).astype(np.float)
        dirs = np.asfortranarray(args.dirs).astype(np.float)
        rates = np.reshape(rates, (np.shape(rates)[0], 1))
        dirs = np.reshape(dirs, (np.shape(dirs)[0], 1))
        plan_tuning_curve(dirs=dirs, rates=rates, show_plots=show_plots)

    elif(mode == 'synthetic'):
        assert len(args.duration) == len(args.rates)
        spike_trains = generate_rasters(
            spike_rates=args.rates,
            intervals=args.duration,
            num_trains=args.num_trials,
            distribution=SpikeDistribution[args.rand],
            show_plots=show_plots,
        )
        plotPSTH(spike_trains, sum(args.duration), args.bin_size, show_plots)
        plotISIH(spike_trains, sum(args.duration), show_plots)

    else:
        print("need to pick a mode")

    if show_plots:
        plt.show()

if __name__ == "__main__":
    """
    Usage Examples:

        python3 raster.py --mode synthetic --rates 50 --duration 2000 --binSize 10 --numTrials 10 --rand gamma --show True
        python3 raster.py --mode synthetic --rates 50 --duration 2000 --binSize 10 --numTrials 10 --rand exp --show True
    """
    args = build_args()
    main(args)




