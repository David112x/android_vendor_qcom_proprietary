#!/usr/bin/python
###############################################################################################################################
#
# Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################
import glob
import argparse
import json
import traceback
from .composerutils import *
import xml.etree.ElementTree as ET

MTIME            = "mtime"
TARGET           = "last_target"
COMMON_TARGET    = "common"

fmt_string_segment   = os.path.join("%s", "segments", "*", "*.xml")
fmt_string_pipelines = os.path.join("%s", "usecases", "*", "pipelines", "*.xml")
g_pipelines_path     = os.path.join("%s", "pipelines")


class Segment:
    def __init__(self, dirname, base_name):
        self.dirname = dirname
        self.name    = base_name
        logging.debug("In dir: %s Segment: %s", (dirname, base_name))


def get_config():
    """
    get_config

    Load the configuration/cache file of this script. This file contains the last built target and the timestamps of when files
    were last modified.

    Returns:
        dict -- Returns the current configuration/cache dictionary
    """

    stat_info = {}
    if os.path.exists(CONFIG_FILE_NAME):
        with open(CONFIG_FILE_NAME) as fd:
            try:
                stat_info = json.load(fd)
            except ValueError:
                pass
    if TARGET not in stat_info:
        stat_info[TARGET] = -1
    return stat_info


def glob_files(glob_str, forced=False):
    """
    glob_files

    Globs a list of files given a glob pattern. This function filters the glob based on the date modfied returned by stat

    Arguments:
        glob_str {str} -- a pattern to glob

    Keyword Arguments:
        forced {bool} -- if true, ignore time modified and always add to glob (default: {False})

    Returns:
        list -- A list of strings matching glob_str
    """

    file_glob = glob.glob(glob_str)
    result    = []
    stat_info = get_config()

    for i, filename in enumerate(file_glob):
        info = os.stat(filename)
        if filename not in stat_info:
            stat_info[filename] = {}
        saved_info = stat_info[filename]
        recently_modified = MTIME not in saved_info or saved_info[MTIME] < info.st_mtime
        if recently_modified:
            saved_info[MTIME] = info.st_mtime
            # print("%02d: %s has been recently modified" % (i, filename))
        if recently_modified or forced:
            result.append((filename, recently_modified))
    with open(CONFIG_FILE_NAME, "w+") as fd:
        json.dump(stat_info, fd)
    return result


def get_segments(target, cwd, header_str=""):
    """
    get_segments

    Globs the list of segment files for the current target

    Arguments:
        target {str} -- The hardware target
        cwd    {str} -- The current working directory

    Returns:
        SegmentInfo -- A SegmentInfo object constructed from the target
    """

    globbed_segment_files = glob_files(fmt_string_segment % cwd, True)
    segment_files         = {}
    any_changes           = False
    build_target_name     = ""

    for full_filename, has_changes in globbed_segment_files:
        dirname, filename = os.path.split(full_filename)
        build_target_name = os.path.split(dirname)[-1]
        if build_target_name != "common" and build_target_name != target:
            continue  # Skip targets we're not looking for
        segment_name = filename.split(".")[0]
        if segment_name not in segment_files:
            segment_files[segment_name] = {}
        segment_files[segment_name][build_target_name] = full_filename
        any_changes = (any_changes or has_changes)

    segment_info    = SegmentInfo(any_changes, header_str=header_str)
    use_composition = False
    for segment_type in segment_files:
        files = None
        segment_file_map = segment_files[segment_type]
        if True == use_composition:
            files = segment_file_map.values()
        else:  # Override common with target
            if len(segment_file_map.keys()) == 1:
                logging.info("%s: Using [%s] implementation" % (segment_type, COMMON_TARGET))
                files = [segment_file_map[COMMON_TARGET]]
            else:
                logging.info("%s: Overriding [%s] with [%s] implementation" % (segment_type, COMMON_TARGET, target))
                files = [segment_file_map[target]]
        segment_tree = parse_segment_files(files)
        segment_info.add(segment_tree)
    return segment_info


def include_preprocess(input_filename, cwd=None):
    target_info = TargetInfo()
    root        = camx_include(input_filename, cwd=cwd, target_info=target_info)
    return ET.ElementTree(root)


def segment_preprocess(target, cwd, forced=False, header_str=""):
    """
    preprocess

    This function is the entry point for including segments into pipelines.

    It replaces all instances of <PipelineSegment> tags within a Pipeline and amends the Segment information into a
    Pipeline's <NodesList> and <PortLinkages>

    It generates the g_pipeline xml files.

    Arguments:
        target {str} -- The hardware target
        cwd {str}    -- The current working directory

    Keyword Arguments:
        forced {bool} -- if true, then ignore the last modified timestamp (default: {False})
    """

    node_include.root_topology_directory = cwd
    segment_info                         = get_segments(target, cwd, header_str=header_str)
    stat_info                            = get_config()
    g_dir                                = g_pipelines_path % cwd
    forced                               = (forced or segment_info.has_changes)
    forced                               = (forced or stat_info[TARGET] != target)

    if not os.path.exists(g_dir):
        os.mkdir(g_dir)
        forced = True

    pipelines                            = glob_files(fmt_string_pipelines % cwd, forced)
    stat_info                            = get_config()
    stat_info[TARGET]                    = target

    with open(CONFIG_FILE_NAME, "w+") as fd:
        json.dump(stat_info, fd)

    for pipeline_filename, has_changes in pipelines:
        dirname, base_file = os.path.split(pipeline_filename)
        g_file = os.path.join(g_dir, "g_%s" % base_file)
        g_file = textutils.process_path(g_file)
        logging.info("Updating: %s", g_file)
        try:
            segment_info.merge_info_into_pipeline(pipeline_filename, g_file)
        except Exception as e:
            print("\nEncountered a fatal exception while generating: [%s]" % g_file)
            if type(e) == CompositionException:
                print("\t%s" % str(e))
            else:
                traceback.print_exc()
            with open(CONFIG_FILE_NAME, "w+") as fd:
                json.dump({}, fd)
            exit(1)
    return len(pipelines)


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description="Preprocess segment files into generated pipeline files")
    arg_parser.add_argument('-f', '--forceGenerate',
                            action='store_true',
                            dest='forced',
                            default=False,
                            required=False)

    arg_parser.add_argument('-t', '--target',
                            action='store',
                            type=str,
                            dest='target',
                            default="sdm845",
                            required=False)

    arg_parser.add_argument('-d', '--workingDirectory',
                            action='store',
                            type=str,
                            dest='cwd',
                            default=".",
                            required=False)

    args = arg_parser.parse_args()
    logging.basicConfig(level=logging.INFO)
    segment_preprocess(args.target, args.cwd, args.forced)
