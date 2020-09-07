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
import numpy as np
import scipy.io as io
import time
import matplotlib.pyplot as plt
from raster_util import build_args

#REFACTORY_PERIOD_MS = 2 # time that must pass before a neuron fires again in milliseconds

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

def plotPSTH(spike_trains, duration, bin_size, show_plots):
    """
    PlotPSTH creates a histogram of the binned spike counts
    from neurons modeled by the Poisson process for a given bin size.

    spike_trains: list of np.arrays() of shape (1, numSpikeEvents)
    duration: units of milliseconds
    bin_size: units of milliseconds
    """
    numTrials = len(spike_trains)
    start = 0
    sentinel = start + bin_size
    idx = 0;
    spike_counts = np.zeros(int(duration/bin_size), dtype=int)

    while (sentinel < duration):
        for trial in range(0, numTrials):
            spike_counts[idx] = spike_counts[idx] +  np.where(np.logical_and(spike_trains[trial] >= start, spike_trains[trial] < sentinel))[0].size # find all values between start and stop for this trial

        # now we'll take the average for this bin across all trials and increment our counters
        spike_counts[idx] = spike_counts[idx] / (bin_size * (10**(-3)) * numTrials)
        start = start + bin_size
        sentinel = start + bin_size
        idx = idx + 1

    if show_plots:
        plt.figure(2)
        plt.bar(range(0, int(duration/bin_size)), spike_counts, align='edge')
        plt.xlabel("bin number")
        plt.ylabel("avg spike count")
        plt.draw()

def generateRaster(
    spike_rate=50, 
    duration=2000, 
    num_trains=10, 
    distribution=None, 
    k=2,
    show_plots=False
):
    """
    The following generates and returns spike rasters by using
    delta-t from a random exponential distribution characterized by...

    spike_rate: rate in spikes/s
    duration: duration of the spike raster in milliseconds
    num_trains: the number of spike rasters to generate
    distribution: random distribution choice, for now either exponential or gamma
    k: for the scaling factor (1 / k!) of the gamma random distribution
    """
    spike_trains = []
    scale = 1000 / spike_rate # beta = 1  / lambda
    shape = k + 1

    start_time = time.time()

    for i in range(0, num_trains):
        spikeTime = 0
        spike_train = []
        dTime = 0

        while(1):
            if distribution == "exp":
                dTime = int(np.random.exponential(scale))
            elif distribution == "gamma":
                dTime = int(np.random.gamma(shape, scale))

            spikeTime = spikeTime + dTime
            if spikeTime < duration:                        # append if we're still under the duration, otherwise break out
                spike_train.append(spikeTime)
            else:
                break

        spike_trains.append(np.asfortranarray(spike_train))

    elapsed_time = time.time() - start_time
    print(str(elapsed_time) + " seconds")

    if show_plots:
        plt.figure(1)
        plt.eventplot(spike_trains)
        plt.xlabel("time (ms)")
        plt.ylabel("sensor")
        plt.draw()

    return spike_trains

def planTuningCurve(dirs=None, rates=None):
    """
    planTuningCurve takes an array of average firing rates of a neuron
    during a reach to dirs[0] degrees, dirs[1] degrees, and so on.
    It computes the tuning model, f(theta) = c0 + c1 cos(theta - theta0).
    (restricted to 3 reach directions for now)

    the outputs, c0, c1, and theta0 are the parameters of the tuning model.

    big picture: this needs to be computed for every sensor.

    f(theta) = c0 + c1*cos(theta - theta0)

    considering:
    --> cos(theta - theat0) = cos(theta)*cos(theta0) + sin(theta)*sin(theta0)

    to simplify parameter estimation:
    --> f(theta) = k0 + k1*sin(theta) + k2*cos(theta)

    TODO: when there are 3+ reach angles, the system is overdetermined.
    in other words there are more equations than k's to solve for.
    in that situation we'd have to solve for k's that minimize the
    mean square error. will probably end up implementing this in C when the time comes.
    """
    # find the values of the k's
    k0 = np.mean(rates)
    k1 = 1/np.sqrt(3) * (rates[1] - rates[2])
    k2 = 2*rates[0]/3 - (rates[1] - rates[2])/3

    # find the values of the c's
    theta0 = np.arctan(k1 / k2)
    c0 = k0
    c1 = k1 / np.sin(theta0)

    # x and y values...
    theta = np.linspace(0 ,2*np.pi, 80)
    firing_rates = c0 + c1*np.cos(theta - theta0)

    if showPlots:
        plt.figure(0)
        plt.plot(dirs, rates, 'r*')
        plt.plot(np.rad2deg(theta), firing_rates)
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
        rates = np.asfortranarray(args.rates).astype(np.float)
        dirs = np.asfortranarray(args.dirs).astype(np.int)
        planTuningCurve(dirs=dirs, rates=rates)

    elif(mode == 'synthetic'):
        spike_trains = generateRaster(
            spike_rate=int(args.rates[0]), 
            duration=args.duration, 
            num_trains=args.num_trials, 
            distribution=args.rand,
            show_plots=show_plots,
        )
        plotPSTH(spike_trains, args.duration, args.bin_size, show_plots)
        plotISIH(spike_trains, args.duration, show_plots)

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




