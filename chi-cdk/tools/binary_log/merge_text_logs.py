#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################

"""ASCII Logs merger for text files in a given path(s)"""
from datetime import datetime
import threading as th
import itertools
import argparse
import os
import re


class Merger(object):
    def __init__(self):
        self.file_data = dict()


    def readf(self, fs):
        with open(fs, 'r') as fh:
            self.file_data[os.path.splitext(os.path.basename(fs))[0]] = fh.readlines()


if __name__ == '__main__':
    startTime = datetime.now()

    parser = argparse.ArgumentParser()

    parser.add_argument(
        '-o', '--output_file', required=True, type=os.path.realpath, help='Path to the output log file')

    parser.add_argument(
        '-d', '--directory', action='store_true', help=R'Path to a directory containing the ascii logs')

    parser.add_argument('input_files', type=os.path.realpath, nargs='+',
                        help=R'List of files to be processed, if -d is given, this is treated a list of directories instead')

    args = parser.parse_args()


    def read_files_in_input_files(top_dir, thread_list):
        """
        @summary: reading all text files in a directory in multiple threads
        """
        for d, _, f in os.walk(top_dir):
            for fil in f:
                if fil.endswith('.txt'):
                    thread_list.append(th.Thread(target=mg.readf, args=(os.path.join(d, fil),)))

        return thread_list


    # read files in multiple threads
    for each in args.input_files:
        mg = Merger()
        threadList = read_files_in_input_files(each, [])

        # starting all threads
        for thr in threadList:
            thr.start()

        # joining all threads
        for thr in threadList:
            thr.join()

        # merging list of lists
        all_in_one = list(itertools.chain(*mg.file_data.values()))

        # sorting
        all_in_one.sort()

        all_in_one = filter(lambda line: re.match(r'\d+', line), all_in_one)

        # writing sorted data to one file
        with open(os.path.join(each, args.output_file), 'w') as mfh:
            mfh.writelines(all_in_one)

    print('Time Elapsed: {0}'.format(str(datetime.now() - startTime)))
