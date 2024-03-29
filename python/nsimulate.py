# raster.py

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

import asyncio
import struct
import binascii
import sys
import argparse
import numpy as np
import time
from nsimulate_util import (
    build_args,
    parse_mat_file,
    generate_inter_spike_interval_hist,
    generate_spike_time_hist,
    plot_rasters,
    show,
)
from neuron import NeuronSimulator, SpikeDistribution
from stimuli import ReachStimuli


def sim3_1_rate_func(reaches: ReachStimuli, preferred: ReachStimuli):
    result = 20 + 2 * reaches.distances * np.cos(reaches.angles - preferred.angles[0])
    return result


def sim3_1():
    # Create a neuron responsive to reach stimuli, whose preferred stimulus
    # is 0 degrees and 10 cm to the right.
    idle_ms_val = 200
    ms_val = 500
    cm_val = 10
    # radians = np.deg2rad(np.linspace(0, 315, 8))
    radians = np.deg2rad([0, 270])
    # milliseconds = np.ones(radians.shape)*ms_val
    stimulus_durations = np.array([idle_ms_val, ms_val])
    # centimeters = np.ones(radians.shape)*cm_val
    stimulus_param_dist = np.array([0, cm_val])
    reaches = ReachStimuli(stimulus_durations, radians, stimulus_param_dist)

    pref_stimuli = [
        ReachStimuli(np.array([ms_val]), np.array([0]), np.array([cm_val]))
        # for angdir in radians
    ]

    neurons = [
        NeuronSimulator(SpikeDistribution.GAMMA, scaling_factor=2)
        # for i in range(len(radians))
    ]

    fig_num = 0
    for neuron, pref_stimulus in zip(neurons, pref_stimuli):
        # Still not 100% committed to a neuron owning its rate function.
        # Conceptually makes sense. Computationally, adds overhead...
        # For now, a neuron is pretty much just a random distribution...
        neuron.assign_rate_func(sim3_1_rate_func, pref_stimulus)
        rates: np.ndarray = neuron.get_rates(reaches)
        rasters = list(
            neuron.generate_rasters(
                spike_rate_hz=rates,
                intervals_ms=stimulus_durations,
                num_trials=100,
                start_time=-200,
            )
        )
        # generate_inter_spike_interval_hist(
        #     spike_trains=rasters,
        #     figure_number=(fig_num+1)*9,
        #     show_plots=True
        # )
        generate_spike_time_hist(
            spike_trains=rasters,
            start_time_ms=-200,
            duration_ms=stimulus_durations,
            bin_size_ms=10,
            figure_number=(fig_num + 1) * 9,
            show_plots=True,
        )
        plot_rasters(rasters, figure_number=fig_num)
        fig_num += 1

    # reusing reach stimuli from pref list
    # reach_0 = pref_stimuli[0]
    # reach_180 = pref_stimuli[4]
    # rates_0 = []
    # rates_180 = []
    # for neuron, pref_stimulus in zip(neurons, pref_stimuli):
    #     rates_0.append(neuron.get_rates(reach_0))
    #     rates_180.append(neuron.get_rates(reach_180))

    # if args.show:
    #     import matplotlib.pyplot as plt
    #     plt.figure(99)
    #     plt.plot(np.arange(len(neurons)), rates_0, label='0 deg.')
    #     plt.plot(np.arange(len(neurons)), rates_180, label='180 deg.')
    #     plt.xlabel("neuron #")
    #     plt.ylabel("spikes/s")
    #     plt.draw()


async def simulate_reaches(num_trials: int, ip: str, port: int):
    radians = np.deg2rad(np.linspace(0, 315, 8))
    milliseconds = np.ones(radians.shape) * 500
    centimeters = np.ones(radians.shape) * 10
    pref_stimulus = ReachStimuli(
        np.array([500]), np.deg2rad(np.array([0])), np.array([10])
    )
    neuron = NeuronSimulator(
        SpikeDistribution.GAMMA,
        scaling_factor=2,
        rate_func=sim3_1_rate_func,
        preferred_stimulus=pref_stimulus,
    )
    reaches = ReachStimuli(milliseconds, radians, centimeters)
    rates = neuron.get_rates(reaches)

    _, writer = await asyncio.open_connection(ip, port)
    print("Connection open...")
    count = 0
    raster_id = 0
    for r, raster in enumerate(
        neuron.generate_rasters(
            spike_rate_hz=rates,
            intervals_ms=milliseconds,
            num_trials=num_trials,
            start_time=0,
        )
    ):
        print("\t\tID: " + str(raster_id))
        for i in raster:
            print(i)
        raster_bytes = struct.pack(f"Q{len(raster)}QI", raster_id, *raster, 0xDEADBEEF)
        time.sleep(0.001)
        writer.write(raster_bytes)
        raster_id += 1

    writer.close()
    await writer.wait_closed()
    print("Connection closed...")


def main(args):
    mode = args.mode
    show_plots = args.show

    if mode == "file":
        parse_mat_file(args.mat_file)

    elif mode == "tune_ex":
        neuron = NeuronSimulator(SpikeDistribution[args.rand])
        assert len(args.rates) == len(args.dirs)
        rates = np.asarray(args.rates).astype(np.float)
        dirs = np.asarray(args.dirs).astype(np.float)
        rates = np.reshape(rates, (np.shape(rates)[0], 1))
        dirs = np.reshape(dirs, (np.shape(dirs)[0], 1))

        neuron.tune_cosine_model(dirs=dirs, rates=rates)

    elif mode == "synthetic":
        neuron = NeuronSimulator(SpikeDistribution[args.rand])
        assert len(args.intervals) == len(args.rates)
        spike_trains = list(
            neuron.generate_rasters(
                spike_rate_hz=np.asarray(args.rates),
                intervals_ms=np.asarray(args.intervals),
                num_trials=args.num_trials,
                start_time=args.start_time or 0,
            )
        )
        if args.show:
            plot_rasters(spike_trains, figure_number=0)

        generate_spike_time_hist(
            spike_trains=spike_trains,
            duration_ms=sum(args.intervals),
            bin_size_ms=args.bin_size,
            show_plots=show_plots,
            figure_number=1,
        )
        generate_inter_spike_interval_hist(
            spike_trains=spike_trains,
            duration_ms=sum(args.intervals),
            show_plots=show_plots,
            figure_number=2,
        )

    elif mode == "sim3_1":
        sim3_1()

    elif mode == "simulate":
        asyncio.run(
            simulate_reaches(
                num_trials=args.num_trials, ip=args.dest_ip, port=args.dest_port
            )
        )

    else:
        raise ValueError("Must pick a mode!")

    if show_plots:
        show()


if __name__ == "__main__":
    """
    Run with --help/-h for a usage example.
    """
    args = build_args()
    main(args)
