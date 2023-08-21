#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################
""" Generates a camx.preamble.json file for the given out file path. """
from __future__ import print_function

from utils import io
from binary_log.core.preamble_types import EncodingCollisionError
from binary_log.core.preamble_parser import InvalidJsonErr
from utils.preamble import join_preambles
from functools import reduce

import argparse
import os
from sys import stderr

_data_arr_symbol = 'CXX_BINARY_LOG_PREAMLBE_DATA'
_data_arr_len_symbol = 'CXX_BINARY_LOG_PREAMLBE_DATA_LEN'

_collision_err  = ('-------------\nEncodingCollisionError: '
                  'An error occured. Found two different encodings for the same log event. Exiting now.'
                  '\n  Log Event:\t "{}"\n  File:\t\t {}\n  Encoding 1:\t {}\n  Encoding 2:\t {}')

_file_exist_err = ('-------------\nFileNotFoundError: '
                  'Failed to find the file "{}". Exiting Now.')

_json_parse_err = ('-------------\nImproperFileError: '
                  'The file is not a valid compiler generated preamble json file. Exiting Now.'
                  '\n  File:\t "{}"')


def printe(*args, **kwargs):
    """Print a message to the error stream
    """
    print(*args, file=stderr, **kwargs)

def parse_args():
    """Parses and returns command line arguments."""
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '-o', '--output', type=os.path.realpath, required=True,
        help=R'Path to generated preamble file')

    parser.add_argument(
        '-d', '--directory', action='store_true',
        help=R'Path to build directory for this project (eg. <camx_src>\camx\camx\build\built\android64\Release)')

    parser.add_argument(
        'preamble_files', type=os.path.realpath, nargs='+',
        help=R'List of files to be processed, if -d is given, this is treated a list of directories instead')

    return parser.parse_args()

def main():
    """Program entry point."""
    args = parse_args()

    preamble_files = args.preamble_files

    if args.directory:
        # If we were given a directory, recurse down a direcotory and join paths
        try:
            preamble_files = reduce(lambda x, y: x + y,
                                    [io.get_files_from_dir(s_dir, ".preamble.json") for s_dir in args.preamble_files])
        except:
            print("Failed to find preamble files in the given directory")
            exit(1)

    if len(preamble_files) == 0:
        print("Failed to find preamble files")
        exit(1)

    # Generate the preambles
    try:
        gen_preambles = [(file, io.get_preamble(file)) for file in preamble_files]
    except EncodingCollisionError as err:
        enc_info = lambda enc: ['{}: {}'.format(str(var.descriptor.identifier), str(var.variable_id)) for var in enc]
        msg =_collision_err.format(err.args[1], err.args[0], enc_info(err.args[2].encodings), enc_info(err.args[3].encodings))
        printe(msg)
        exit(1)
    except FileNotFoundError as err:
        printe(_file_exist_err.format(err.filename))
        exit(1)
    except InvalidJsonErr as err:
        printe(_json_parse_err.format(err.args[0]))
        exit(1)

    with open(args.output, 'w') as out_file:
        if len(gen_preambles) > 1:
            joint_preamble = reduce(lambda x,y: join_preambles(x, y), [preamble for _, preamble in gen_preambles])
            output = joint_preamble.to_str()
            out_file.write(output)
        elif len(gen_preambles) == 1:
            output = gen_preambles[0][1].to_str()
            out_file.write(output)

if __name__ == '__main__':
    main()