#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################

"""Generates a camx.preamble file for the given out file path."""
from __future__ import print_function

from argparse import ArgumentParser
from os import path, getcwd
from subprocess import Popen, check_output, PIPE
from time import sleep

def parse_args(curr_dir):
    """Parses and returns command line arguments."""
    parser = ArgumentParser()

    parser.add_argument(
        '-d', '--dir', type=path.realpath, help='Path to where we save the log files', default=curr_dir)

    parser.add_argument(
        '-o', '--output_dir', type=path.realpath, help='Path to where we save the log files', default=curr_dir)

    parser.add_argument(
        '-f', '--frequency', type=float, help='How many often to poll for a file', default=float(0.5))

    parser.add_argument(
        '-l', '--logs_path', type=str, help='Where the logs are on the device', default='/data/vendor/camera/bin_log')

    return parser.parse_args()

def find_device():
    try:
        return [dev.split('\t')[0] for dev in check_output("adb devices").split('\n')[1:] if len(dev) > 1]
    except:
        return []

def root_and_remount():
    check_output(['adb', 'wait-for-device', 'root'])
    check_output(['adb', 'wait-for-device', 'remount'])

def pull_log_files(logs_path):
    return check_output(['adb', 'shell', 'ls', logs_path]).split()

def main():
    """Program entry point."""
    args = parse_args(path.realpath(getcwd()))

    devices = find_device()

    if len(devices) == 0:
        print("No devices found to connect to.")
        return

    print('Found {} device(s): {}'.format(len(devices), devices))
    print('Rooting {}...'.format(devices[0]))
    root_and_remount()

    if path.sep != args.output_dir[-1]:
        args.output_dir = args.output_dir + path.sep

    while True:
        devices = find_device()

        if len(devices) == 0:
            print("Lost connection with device.")
            return

        log_files = pull_log_files(args.logs_path)
        for file in log_files:
            print('Pulling {} to {}'.format(file, args.output_dir))
            pull_cmd = 'adb pull {}/{} {}'.format(args.logs_path, file, args.output_dir)
            rm_cmd   = 'adb shell rm {}/{}'.format(args.logs_path, file)
            check_output(pull_cmd)
            check_output(rm_cmd)

        sleep(args.frequency)

if __name__ == '__main__':
    main()
