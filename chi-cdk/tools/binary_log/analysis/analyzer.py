#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################

import json
from debuggers.StreamSession import *
import sys
import os
import argparse

def main():

    parser = argparse.ArgumentParser(description='Chi Feature2 Analysis Tool')
    parser.add_argument('-i', '--input_file',
                        dest='input_file',
                        type=str,
                        required=True,
                        help='JSON output from parsed binary logs')
    parser.add_argument('--scope',
                        type=str,
                        choices=['HAL', 'CHI', 'PIPELINE', 'NODE', 'FENCE'],
                        default=None,
                        help='Limit the max scope of generated reports. Scope level chosen will be the last level included in the report')
    args = parser.parse_args()

    # Verify input file
    input_file = args.input_file
    if not os.path.isfile(input_file):
        print('Invalid input file: {}'.format(input_file))
        exit()

    # Decide how we will load in the file
    iterative_parse = False
    if os.path.getsize(input_file) > 2000000000:   # 2GB threshold
        print('Large file detected, attempting iterative parse. Every event must be on one line.')
        iterative_parse = True

    # Run analysis
    stream_sessions = []
    current_session = 0
    stream_sessions.insert(current_session, StreamSession())
    with open(input_file, 'r') as input_file:
        if iterative_parse:
            # For iterative parse, we read the file line by line and extract one JSON object at a time
            # This is much slower, but allows us to parse large files without reading the entire file into memory
            for line in input_file:
                if line.rstrip() == '[' or line.rstrip() == ']':
                    continue
                try:
                    event_id, log = json.loads(line.rstrip(',\n'))
                except:
                    print ('Failed to parse {}'.format(line))
                    exit()

                if event_id == 'HAL3_ConfigSetup':
                    if stream_sessions[current_session].is_modified():
                        # Start new session
                        current_session += 1
                        stream_sessions.insert(current_session, StreamSession(log))
                    else:
                        # Keep same session, update with this configure streams
                        stream_sessions[current_session].add_configure_stream(log)
                else:
                    stream_sessions[current_session].handle_event(event_id, log)
        else:
            # For more manageable file sizes, we load the entire file into memory
            json_obj = json.load(input_file)
            print ('Log loaded.')
            for event_id, log in json_obj:
                if event_id == 'HAL3_ConfigSetup':
                    if stream_sessions[current_session].is_modified():
                        # Start new session
                        current_session += 1
                        stream_sessions.insert(current_session, StreamSession(log))
                    else:
                        # Keep same session, update with this configure streams
                        stream_sessions[current_session].add_configure_stream(log)
                else:
                    stream_sessions[current_session].handle_event(event_id, log)

    # Generate reports
    for session_index in range(0, len(stream_sessions)):
        # Reset stdout
        sys.stdout = sys.__stdout__
        session = stream_sessions[session_index]

        # Print full reports of every session with desired scope
        filename = 'session_{}.full.txt'.format(session_index)
        with open(filename, 'w') as f:
            sys.stdout = f
            session.print_summary(scope=args.scope)

        # Print only incomplete reports of every session with desired scope
        filename = 'session_{}.incomplete.txt'.format(session_index)
        with open(filename, 'w') as f:
            sys.stdout = f
            session.print_summary(scope=args.scope, only_incomplete=True)

    return 0


if __name__ == '__main__':
    main()