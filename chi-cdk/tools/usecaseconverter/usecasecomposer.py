#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################
import argparse
import logging
import os.path
import time
import glob
import traceback
import xml.etree.ElementTree as ET
from utils.preprocessor import segment_preprocess, include_preprocess
from utils.textutils import process_path
from utils.composerutils import terminate

if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description="Process CamxInclude statements to generate a Usecase.xml file")
    arg_parser.add_argument('-i', '--inputFile',
                            action='store',
                            type=str,
                            dest='input_filename',
                            required=True)

    arg_parser.add_argument('-o', '--outputFile',
                            action='store',
                            type=str,
                            dest='output_filename',
                            required=True)

    arg_parser.add_argument('-H', '--headerFile',
                            action='store',
                            type=str,
                            dest='header_filename',
                            required=False)

    arg_parser.add_argument('-t', '--target',
                            action='store',
                            type=str,
                            dest='target',
                            default="none",
                            required=False)

    arg_parser.add_argument('-l', '--log_level',
                            action='store',
                            type=str,
                            dest='log_level',
                            default="none",
                            required=False)

    arg_parser.add_argument('-d', '--root_topology_directory',
                            action='store',
                            type=str,
                            dest='root_topology_directory',
                            default=".",
                            required=False)

    arg_parser.add_argument('-g', '--debug',
                            action='store_true',
                            default=False,
                            dest='debug')

    args                    = arg_parser.parse_args()
    input_filename          = process_path(args.input_filename)
    output_filename         = process_path(args.output_filename)
    log_level               = args.log_level
    root_topology_directory = process_path(args.root_topology_directory)

    # Find the newest input usecase file that can be included
    input_file_ts    = os.path.getmtime(input_filename)
    usecase_glob_str = os.path.join(root_topology_directory, "usecases", "*", "*.xml")
    streams_glob_str = os.path.join(root_topology_directory, "target_buffers", "*.xml")
    for path in glob.glob(usecase_glob_str) + glob.glob(streams_glob_str):
        if "g_" not in path: # Ignore autogenerated files
            input_file_ts = max(input_file_ts, os.path.getmtime(path))

    if os.path.exists(output_filename) == True:
        output_file_ts          = os.path.getmtime(output_filename)
    else:
        output_file_ts          = 0

    is_input_old = (input_file_ts < output_file_ts)

    if "info" == log_level:
        logging.basicConfig(level=logging.INFO)
    elif "debug" == log_level:
        logging.basicConfig(level=logging.DEBUG)
    elif "warning" == log_level or "warn" == log_level:
        logging.basicConfig(level=logging.WARNING)

    forced_modes = [True] if args.debug else [False, True]
    for forced in forced_modes:
        try:
            # Set header
            if args.header_filename:
                header_filename = process_path(args.header_filename)
                logging.debug("Appending contents of %s to the generated files" % header_filename)
                with open(header_filename) as fd:
                    header = fd.read()
            else:
                header = ""

            # Do preprocessing
            num_modified_pipelines = segment_preprocess(args.target, root_topology_directory, forced=forced, header_str=header)

            if 0 == num_modified_pipelines and is_input_old:
                logging.debug("No input file update: %s Skip generating outputfile: %s", input_filename, output_filename)
                os._exit(0)

            tree = include_preprocess(input_filename)
            end  = time.time()

            # Generate usecase file and append header
            logging.debug("Generating %s" % output_filename)
            with open(output_filename, 'w+') as fd:
                fd.write(header)
                fd.write(ET.tostring(tree.getroot()).decode())

            break # On sucessful generation, break out of loop
        except Exception as e:
            traceback.print_exc()
            if forced: # Force build failed, terminate
                terminate(str(e))
            else:
                logging.error(str(e))
                logging.warning("Composition failed - trying again with forced build")
