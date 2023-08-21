#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################

from ._context import binary_log

from binary_log.core.preamble_parser import PreambleParser

from os import path, walk

def get_files_from_dir(directory, extension):
    """Get the preamble files that follow from a specific directory.

    Args:
        directory (str): The directory to look for preamble from.

    Returns:
    ``[str]``: A list of preamble files from this dierctory.
    """
    found_files = []
    for fileDir, _, fileList in walk(directory):
        for fname in [file for file in fileList if file.endswith(extension)]:
            found_files.append(fileDir + path.sep + fname)
    return found_files

def get_preamble(preamble_file_path):
    """Open a preamble file and read it

    Args:
        preamble_file_path (str): The path of the file you want to get a preamble from.

    Returns:
        A ``PreambleTree`` object from the file passed.

    Raises:
        IOError: Failed to open a file
        EncodingCollisionError: Encountered a log event with two different encodings.
    """
    with open(preamble_file_path) as preamble_file:
        parser = PreambleParser(preamble_file_path)
        return parser.parse(preamble_file)

    raise IOError('Failed to process "{}"'.format(preamble_file_path))