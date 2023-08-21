#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################
from functools import reduce

class LogInfo:
    def __init__(self, generator, extract_timestamp, compare_timestamps):
        """ Constructs a LogInfo of a file that allows you to read all the lines that have the same timestamp at once.

        Arguments:
            generator (f :() -> bytes)                -- function to read a single line and return it from the file
            extract_timestamp (f:bytes -> int)        -- function to extract timestamp from the the output of the generator
            compare_timestamps (f:[int, int] -> bool) -- Check if two timestmaps (output of extract_timestamp) are the same
        """
        self.generator = generator
        self.extract_timestamp = extract_timestamp
        self.compare_timestamps = compare_timestamps
        self.timestamp = -1
        self.eof       = True
        self.buffer    = {}
        # Read 1 value into the buffer and set the initial settings
        try:
            self.buffer     = next(self.generator)
            self.timestamp  = self.extract_timestamp(self.buffer[1])
            self.eof        = False
        except GeneratorExit:
            self.generator = {}
        except StopIteration:
            # The file is empty
            self.eof = True

    def read_timestamp_chunk(self):
        res = []

        curr_timestamp = self.timestamp
        same_timestamp = True

        while not(self.eof) and same_timestamp:
            res.append(self.buffer)

            try:
                self.buffer     = next(self.generator)
                self.timestamp  = self.extract_timestamp(self.buffer[1])
            except StopIteration:
                self.eof = True
            except GeneratorExit:
                self.eof = True

            same_timestamp = self.compare_timestamps(curr_timestamp, self.timestamp)

        return curr_timestamp, res

class LogMerger:

    def __init__(self, logInfos):
        """Construct a merger that takes log info files and merges them according to their timestmap polic

        Arguments:
            logInfos ([LogInfo]) -- A list of log info objects to poll for logs with matching timestamps
        """
        self.log_infos = logInfos
        self.curr_log_chunks  = [[] for _ in range(len(self.log_infos))]
        self.curr_log_timestmap = [-1 for _ in range(len(self.log_infos))]

    def get_log_chunks(self):
        res = []

        # Make suer all the logInfo objects gave us their earliest chunks
        for idx, chunk in enumerate(self.curr_log_chunks):
            if chunk == []:
                # The chunk for this file is empty, time to ask for more logs
                self.curr_log_timestmap[idx], self.curr_log_chunks[idx] = self.log_infos[idx].read_timestamp_chunk()

        # Failed to read any data, we are done
        chunks_available = len([chunk for chunk in self.curr_log_chunks if chunk != []])
        if not(chunks_available):
            return None

        min_timestamp = min([self.curr_log_timestmap[idx] for idx, chunk in enumerate(self.curr_log_chunks) if chunk != []])

        # Merge the chunks that share the earliest timestamp
        for idx, chunk in enumerate(self.curr_log_chunks):
            if (chunk != []) and (min_timestamp == self.curr_log_timestmap[idx]):
                res.append(chunk)
                self.curr_log_chunks[idx] = []

        merged_res = reduce(lambda x, y: x + y, res)

        # Give back the result
        return merged_res