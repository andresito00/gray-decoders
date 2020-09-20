import argparse

def build_args():
    def str2bool(v):
        if v.lower() in ('yes', 'true', 't', 'y', '1'):
            return True
        elif v.lower() in ('no', 'false', 'f', 'n', '0'):
            return False
        else:
            raise argparse.ArgumentTypeError('Boolean value expected.')

    parser = argparse.ArgumentParser()

    # main parameters
    parser.add_argument(
      '--mode', '-m',
      type=str,
      default='synthetic',
      choices=['file', 'tune', 'synthetic'],
      help='synthetic spikes generated by prob. distribution or file input'
    )

    parser.add_argument(
      '--show', '-s',
      type=str2bool,
      default=False,
      help='whether or not to show the plots'
    )

    parser.add_argument(
      '--lat', '-l',
      type=str2bool,
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
      default='EXP',
      choices=['EXP', 'GAMMA', 'POISSON'],
      help='type of random distribution'
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

    # raster generation parameters
    parser.add_argument(
      '--duration', '-dt',
      nargs='+',
      type=int,
      help='the duration of the stimulus in units of milliseconds'
    )

    parser.add_argument(
      '--bin-size', '-b',
      type=int,
      default=10,
      help='the binning resolution at which to capture the millisecond data'
    )

    return parser.parse_args()
