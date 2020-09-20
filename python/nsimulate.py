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
import matplotlib.pyplot as plt
from nsimulate_util import build_args, parse_mat_file
from neuron import NeuronSimulator, SpikeDistribution

def main(args):
    mode = args.mode
    show_plots = args.show

    neuron = NeuronSimulator(SpikeDistribution[args.rand])

    if(mode == 'file'):
        parse_mat_file(args.mat_file)

    elif(mode == 'tune'):
        assert len(args.rates) == len(args.dirs)
        rates = np.asfortranarray(args.rates).astype(np.float)
        dirs = np.asfortranarray(args.dirs).astype(np.float)
        rates = np.reshape(rates, (np.shape(rates)[0], 1))
        dirs = np.reshape(dirs, (np.shape(dirs)[0], 1))

        neuron.tune_cosine_model(dirs=dirs, rates=rates, show_plots=show_plots)

    elif(mode == 'synthetic'):
        assert len(args.intervals) == len(args.rates)
        spike_trains = neuron.generate_rasters(
            spike_rates=args.rates,
            intervals=args.intervals,
            num_trials=args.num_trials,
            show_plots=show_plots,
        )
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




