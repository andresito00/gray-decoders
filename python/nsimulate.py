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
import asyncio
import struct
import binascii
import sys
import argparse
import numpy as np
import matplotlib.pyplot as plt
from nsimulate_util import build_args, parse_mat_file
from neuron import NeuronSimulator, SpikeDistribution
from stimuli import ReachStimuli

def sim3_1_rate_func(reaches: ReachStimuli, preferred: ReachStimuli):
    return 20 + 2*reaches.distances*np.cos(reaches.angles - preferred.angles[0])


def sim3_1():
    # Create a neuron responsive to reach stimuli, whose preferred stimulus
    # is 0 degrees and 10 cm to the right.

    radians = np.deg2rad(np.linspace(0, 315, 8))
    milliseconds = np.ones(radians.shape)*500
    centimeters = np.ones(radians.shape)*10
    reaches = ReachStimuli(milliseconds, radians, centimeters)

    pref_stimuli = [
        ReachStimuli(np.array([500]), np.deg2rad(np.array([0])), np.array([10])),
        ReachStimuli(np.array([500]), np.deg2rad(np.array([45])), np.array([10])),
        ReachStimuli(np.array([500]), np.deg2rad(np.array([90])), np.array([10])),
        ReachStimuli(np.array([500]), np.deg2rad(np.array([135])), np.array([10])),
        ReachStimuli(np.array([500]), np.deg2rad(np.array([180])), np.array([10])),
        ReachStimuli(np.array([500]), np.deg2rad(np.array([225])), np.array([10])),
        ReachStimuli(np.array([500]), np.deg2rad(np.array([270])), np.array([10])),
        ReachStimuli(np.array([500]), np.deg2rad(np.array([315])), np.array([10])),
    ]

    neurons = [
        NeuronSimulator(SpikeDistribution.GAMMA, scaling_factor=2),
        NeuronSimulator(SpikeDistribution.GAMMA, scaling_factor=2),
        NeuronSimulator(SpikeDistribution.GAMMA, scaling_factor=2),
        NeuronSimulator(SpikeDistribution.GAMMA, scaling_factor=2),
        NeuronSimulator(SpikeDistribution.GAMMA, scaling_factor=2),
        NeuronSimulator(SpikeDistribution.GAMMA, scaling_factor=2),
        NeuronSimulator(SpikeDistribution.GAMMA, scaling_factor=2),
        NeuronSimulator(SpikeDistribution.GAMMA, scaling_factor=2),
    ]

    fig_num = 0
    for neuron, pref_stimulus in zip(neurons, pref_stimuli):
        neuron.assign_rate_func(sim3_1_rate_func, pref_stimulus)
        rates = neuron.get_rates(reaches)
        rasters = list(neuron.generate_rasters(rates, milliseconds, 100, 0))
        print(rasters[0])
        print(rasters[1])
        exit()
        neuron.plot_rasters(fig_num, rasters)
        fig_num += 1

    # reusing reach stimuli from pref list
    reach_0 = pref_stimuli[0]
    reach_180 = pref_stimuli[4]
    rates_0 = []
    rates_180 = []
    for neuron, pref_stimulus in zip(neurons, pref_stimuli):
        rates_0.append(neuron.get_rates(reach_0))
        rates_180.append(neuron.get_rates(reach_180))

    plt.figure(99)
    plt.plot(np.arange(len(neurons)), rates_0, label='0 deg.')
    plt.plot(np.arange(len(neurons)), rates_180, label='180 deg.')
    plt.xlabel("neuron #")
    plt.ylabel("spikes/s")
    plt.draw()

async def simulate():
    radians = np.deg2rad(np.linspace(0, 315, 8))
    milliseconds = np.ones(radians.shape)*500
    centimeters = np.ones(radians.shape)*10
    reaches = ReachStimuli(milliseconds, radians, centimeters)
    pref_stimulus = ReachStimuli(
        np.array([500]), np.deg2rad(np.array([0])), np.array([10]))

    neuron = NeuronSimulator(SpikeDistribution.GAMMA, scaling_factor=2)
    neuron.assign_rate_func(sim3_1_rate_func, pref_stimulus)
    rates = neuron.get_rates(reaches)

    print('Opening the connection...')
    _, writer = await asyncio.open_connection('127.0.0.1', 8808)
    print('Connection open...')
    for raster in neuron.generate_rasters(rates, milliseconds, 100, 0):
        raster_bytes = struct.pack(
            f'Nd{len(raster)}dI', len(raster), raster[-1], *raster, 0xdeadbeef)
        print(binascii.hexlify(raster_bytes))
        writer.write(raster_bytes)
        break

    print('Closing the connection...')
    writer.close()
    await writer.wait_closed()

def main(args):
    mode = args.mode
    show_plots = args.show

    if mode == 'file':
        parse_mat_file(args.mat_file)

    elif mode == 'tune':
        neuron = NeuronSimulator(SpikeDistribution[args.rand])
        assert len(args.rates) == len(args.dirs)
        rates = np.asarray(args.rates).astype(np.float)
        dirs = np.asarray(args.dirs).astype(np.float)
        rates = np.reshape(rates, (np.shape(rates)[0], 1))
        dirs = np.reshape(dirs, (np.shape(dirs)[0], 1))

        neuron.tune_cosine_model(dirs=dirs, rates=rates, show_plots=show_plots)

    elif mode == 'synthetic':
        neuron = NeuronSimulator(SpikeDistribution[args.rand])
        assert len(args.intervals) == len(args.rates)
        spike_trains = [neuron.generate_rasters(
            spike_rates=args.rates,
            intervals=args.intervals,
            num_trials=args.num_trials,
        )]

        neuron.plot_rasters(1, spike_trains)

        neuron.generate_spike_time_hist(
            spike_trains=spike_trains,
            duration=sum(args.intervals),
            bin_size=args.bin_size,
            show_plots=show_plots
        )
        neuron.generate_inter_spike_interval_hist(
            spike_trains=spike_trains,
            duration=sum(args.intervals),
            show_plots=show_plots
        )

    elif mode == 'sim3_1':
        sim3_1()

    elif mode == 'simulate':
        asyncio.run(simulate())

    else:
        raise ValueError("Must pick a mode!")

    if show_plots:
        plt.show()

if __name__ == "__main__":
    """
    Usage Examples:

        python3 nsimulate --mode synthetic --rates 50 --intervals 2000 --bin-size 10 --num-trials 10 --rand gamma --show True
        python3 nsimulate --mode synthetic --rates 50 --intervals 2000 --bin-size 10 --num-trials 10 --rand exp --show True
    """
    args = build_args()
    main(args)




