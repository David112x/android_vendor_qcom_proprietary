###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################

""" Parses binary files to json and merges them to one output file """
from __future__ import print_function
from utils.preamble import parse_json_data
from utils.binary import read_encoding, parse_encoding
import glob
import argparse
import os
import sys
import json
import time
import multiprocessing
import datetime as dt
from heapq import merge

_msg_size_type = 'unsigned long long'
timestamp_identifier = 'timestamp_nanoseconds'
BYTES_TO_READ = 8

class ParseMsgException(Exception):
    pass


class BinaryParse(object):
    """
    @summary: Binary Parsing
    """
    def __init__(self):
        """
        @summary: constructor class for binary parse
        """
        self.parsedMessages = list()


    def parse_message(self, preamble, raw_msg, event_ids, true_char_array):
        event_val = read_encoding('unsigned short', raw_msg[0:2])
        event_idx = event_ids.index(event_val)
        log_event = preamble.log_events[event_idx]
        encoding = parse_encoding(preamble.encodings[log_event.identifier].encodings, raw_msg[2:], [], preamble.type_descriptors, true_char_array)
        self.parsedMessages.append((log_event.identifier, encoding))


def parse_args():
    """Parses and returns command line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--preamble',
                        required=True,
                        type=os.path.realpath,
                        help='Path to the json preamble file used to decode the binary files')
    parser.add_argument('-d', '--directory',
                        required=True,
                        type=os.path.realpath,
                        help='Path to directory holding binary files')
    parser.add_argument('-o', '--output',
                        required=True,
                        type=os.path.realpath,
                        dest='output_file',
                        help='Path to directory holding binary files')
    parser.add_argument('-j', '--jobs',
                        required=False,
                        type=int,
                        dest='num_jobs',
                        help='Number of concurrent jobs')
    return parser.parse_args()


def get_preamble_from_json(preamble_file_path):
    try:
        with open(preamble_file_path, 'r') as preamble_file:
            txt_data = preamble_file.read()
            return parse_json_data(txt_data)
    except:
        return None


def parse_message(preamble, raw_msg, event_ids, true_char_array):
    event_val = read_encoding('unsigned short', raw_msg[0:2])
    event_idx = event_ids.index(event_val)
    log_event = preamble.log_events[event_idx]
    encoding  = parse_encoding(preamble.encodings[log_event.identifier].encodings, raw_msg[2:], [], preamble.type_descriptors, true_char_array)
    return (log_event.identifier, encoding)


def read_msg(file, event_ids, preamble, true_char_array):
    bp = BinaryParse()
    total_read = 0
    with open(file, "rb") as bin_file:
        while True:
            # Try to read the message, if we fail to read 8 bytes (and read no bytes, we were able to read the file properly)
            msg_size_raw = bin_file.read(BYTES_TO_READ)
            if len(msg_size_raw) < BYTES_TO_READ:
                # If there are bytes left over, we failed to read properly
                if len(msg_size_raw) > 0:
                    raise ParseMsgException("{} Is not a proper binary log file.".format(file))
                break
            else:
                msg_size = read_encoding(_msg_size_type, msg_size_raw)
                raw_msg = bin_file.read(msg_size)
                if len(raw_msg) != msg_size or len(raw_msg) <= 2:
                    raise ParseMsgException("{} is corrupt. At {} bytes, expected to read {} bytes, found only {}.".format(file, total_read, msg_size, len(raw_msg)))
                bp.parse_message(preamble, raw_msg, event_ids, true_char_array)
                total_read = total_read + len(raw_msg) + 8
    return bp.parsedMessages


def parse_log_file(input_params):
    job_start = time.time()
    input_file, event_ids, preamble = input_params
    output_file_name = input_file.replace('.bin', '.json')
    with open(output_file_name, 'w') as output_fd:
        gtor = lambda x, y, z: read_msg(x, y, z, False)
        for log in gtor(input_file, event_ids, preamble):
            timestamp = dt.datetime.utcfromtimestamp(int(log[1]['timestamp_nanoseconds']) / 1e9).strftime('%m-%d %H:%S:%M.%f000')
            output_fd.write('{} ["{}", {}],\n'.format(timestamp, log[0], json.dumps(log[1])))
    print ('Finished %s in %s seconds' % (output_file_name, time.time() - job_start))


def json_sort_key(log):
    try:
        return json.loads(log.rstrip(',\n'))[1]['timestamp_nanoseconds']
    except:
        print('Error: Could not parse timestamp_nanoseconds from JSON: %s' % (log))
        exit()


def json_log_iter(log_file, key_func):
    for log in log_file:
        json_log = log[25:] # Skip over datetime stamp and only read JSON encoding
        yield (key_func(json_log), json_log)

def iter_wrapper2(iter):
    # Python2 style iterator, returns True when last element is reached
    it = iter.__iter__()
    c_elem = it.next()
    while True:
        try:
            n_elem = it.next()
            yield (False, c_elem)
            c_elem = n_elem
        except StopIteration:
            yield (True, c_elem)
            break

def iter_wrapper3(iter):
    # Python3 style iterator, returns True when last element is reached
    it = iter.__iter__()
    c_elem = next(it)
    while True:
        try:
            n_elem = next(it)
            yield (False, c_elem)
            c_elem = n_elem
        except StopIteration:
            yield (True, c_elem)
            break

def main():
    args = parse_args()

    # Load preamble
    preamble = get_preamble_from_json(args.preamble)
    if preamble is None:
        print('Failed to parse file "{}" to a preamble. Make sure it is a valid compact json preamble'.format(args.preamble))
        print('Note: If this is a compiler generated preamble, you will need to run it by "gen_preamble.py" script first to generate a compact json preamble.')
        sys.exit(1)
    event_ids = [event.value for event in preamble.log_events]

    # Gather log files
    log_files = list(glob.iglob(os.path.join(args.directory, '*.bin')))
    data = [(log_file, event_ids, preamble) for log_file in log_files]

    # Determine number of concurrent jobs
    try:
        if args.num_jobs:
            num_jobs = args.num_jobs
        else:
            num_jobs = multiprocessing.cpu_count()
    except NotImplementedError:
        num_jobs = 1

    # Parse individual binary files
    print('Using %s worker processes' % num_jobs)
    start_time = time.time()
    pool = multiprocessing.Pool(num_jobs)
    pool.map(parse_log_file, data)

    # Merge individual json files
    json_files = list(glob.iglob(os.path.join(args.directory, '*.json')))
    json_log_list = map(open, json_files)
    with open(args.output_file, 'w+') as merged_file:
        merged_file.write('[\n')
        iter_wrapper = iter_wrapper3 if sys.version_info >= (3, 0) else iter_wrapper2
        for is_last, merged_log in iter_wrapper(merge(*[json_log_iter(log, json_sort_key) for log in json_log_list])):
            log_to_write = merged_log[1]
            if is_last:
                # Remove trailing comma from last element
                log_to_write = log_to_write.rstrip(',\n') + '\n'
            merged_file.write(log_to_write)
        merged_file.write(']')
    
    # Close individual json files
    for json_log in json_log_list:
        json_log.close()

    print ('Time elapsed in seconds: %s' % (time.time() - start_time))


if __name__ == '__main__':
    main()
