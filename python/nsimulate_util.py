import argparse
import numpy as np
import scipy.io as io
import matplotlib as plt

def build_args():
    parser = argparse.ArgumentParser()

    # main parameters
    parser.add_argument(
      '--mode', '-m',
      type=str,
      default='synthetic',
      choices=['file', 'tune', 'synthetic', 'sim3_1', 'simulate'],
      help='synthetic spikes generated by prob. distribution or file input'
    )

    parser.add_argument(
      '--show', '-s',
      action='store_true',
      default=False,
      help='whether or not to show the plots'
    )

    parser.add_argument(
      '--lat', '-l',
      action='store_true',
      default=False,
      help='whether or not to measure script latencies'
    )

    # file input parameters
    parser.add_argument(
      '--mat-file', '-mf',
      type=str,
      help='filename argument for a .matfile (hardcoded to 100x8 array of'
          'structs containing 2-D arrays)'
    )

    # tuning curve parameters
    parser.add_argument(
      '--dirs', '-d',
      nargs='+',
      default=[10, 20, 30],
      type=int,
      help='list of the potential reach directions in units of degrees'
    )

    parser.add_argument(
      '--rates', '-r',
      nargs='+',
      type=int,
      help='rate in spikes/s'
    )

    # probability distribution parameters
    parser.add_argument(
      '--type', '-t',
      type=str,
      default='Poisson',
      choices=['Poisson'],
      help='type of synthetic data'
    )

    parser.add_argument(
      '--shift', '-sh',
      type=int,
      default=20,
      help='dc shift or offset of the probability distribution'
    )

    parser.add_argument(
      '--dists', '-td',
      type=int,
      default=10,
      help='target distance in units of cm'
    )

    parser.add_argument(
      '--rand', '-rn',
      type=str,
      choices=[x.name for x in SpikeDistribution],
      help='type of random variable'
    )

    parser.add_argument(
      '--scale-factor', '-sf',
      type=int,
      help='GAMMA distribution scale factor'
    )

    parser.add_argument(
      '--pref-dir', '-pd',
      type=int,
      default=45,
      help='preferred target direction in units of degrees'
    )

    parser.add_argument(
      '--num-trials', '-n',
      type=int,
      default=10,
      help='the number of trials we\'ll generate for each target'
    )

    parser.add_argument(
      '--start-time', '-st',
      type=int,
      help='the number of trials we\'ll generate for each target'
    )

    # raster generation parameters
    parser.add_argument(
      '--intervals', '-dt',
      nargs='+',
      type=int,
      help='the intervals of the stimuli in units of milliseconds'
    )

    parser.add_argument(
      '--bin-size', '-b',
      type=int,
      default=10,
      help='the binning resolution at which to capture the millisecond data'
    )

    return parser.parse_args()


@staticmethod
def plot_cosine_model(k: np.ndarray, figure_number: int) -> None:
    theta = np.linspace(0, 2*np.pi, 80)
    exp_firing_rates = k[0][0] + k[1][0]*np.sin(theta) + \
                        k[2][0]*np.cos(theta)
    plt.figure(figure_number)
    plt.plot(dirs, rates, 'r*')
    plt.plot(np.rad2deg(theta), exp_firing_rates)
    plt.xlabel('angle (degrees)')
    plt.ylabel('firing rate (spikes/s)')
    plt.draw()


@staticmethod
def plot_rasters(spike_trains: List[np.ndarray], figure_number: Optional[int]) -> None:
    plt.figure(figure_number)
    plt.eventplot(spike_trains)
    plt.xlabel("time (ms)")
    plt.ylabel("sensor")
    plt.draw()


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
            masked_intervals = _mask_pad(intervals,
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

def parse_mat_file(mat_file: str):
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
    trials = np.asarray(trials)

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

