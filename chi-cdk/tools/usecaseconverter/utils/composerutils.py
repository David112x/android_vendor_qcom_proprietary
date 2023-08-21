###############################################################################################################################
#
# Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################
import sys
import xml.etree.ElementTree as ET
import os
import logging
import copy
from . import textutils
from . import assertrules

COMMON_TARGET        = "common_target_buffers.xml"
COMMON_TARGET_FOLDER = "target_buffers"
CONFIG_FILE_NAME     = ".usecase_cache" if os.name != "nt" else "build.cache"
SEGMENT_NAME_TAG     = "PipelineSegmentName"
SEGMENT_LIST_TAG     = "PipelineSegments"
INCLUDE_TAG          = "CamxInclude"
CAMX_SEGMENT_TAG     = "CamxSegment"
NODES_LIST_TAG       = "NodesList"
PORTS_LIST_TAG       = "PortLinkages"


class CompositionException(Exception):
    def __init__(self, *args):
        Exception.__init__(self, *args)


class ElementInfo:
    def __init__(self, root_element, element, child_ids):
        self.original     = element
        self.element      = copy.deepcopy(element)
        self.child_ids    = list(child_ids)
        self.root_element = root_element

        root_element.remove(element)
        root_element.append(self.element)

    def update_info(self, new_element, combine_element_id):
        self.element.append(new_element)
        self.child_ids.append(combine_element_id)


class TargetInfo:
    def __init__(self):
        self.target = {}
        target_buffer_file = os.path.join(node_include.root_topology_directory, COMMON_TARGET_FOLDER, COMMON_TARGET)
        if os.path.isfile(target_buffer_file):
            self.get_targets(target_buffer_file)

    def get_targets(self, target_buffer_file):
        target_tree = ET.parse(target_buffer_file)
        targets     = target_tree.getroot()
        for target_segment in targets.findall('TargetSegment'):
            attributes                       = target_segment.attrib
            target_segment_name              = attributes['name']
            self.target[target_segment_name] = target_segment


class SegmentInfo:
    def __init__(self, has_changes=False, header_str=""):
        logging.debug("Creating SegmentInfo")
        self.has_changes       = has_changes
        self.segments          = {}
        self.included_segments = {}
        self.header_str        = header_str

    def add(self, segments):
        """
        add

        Add segments to the container

        Arguments:
            segments {container} -- A container of <PipelineSegment> elements to add
        """

        for segment in segments:
            name                = segment.attrib["name"]
            self.segments[name] = segment

    def merge_info_into_pipeline(self, input_filename, output_filename):
        """
        merge_info_into_pipeline

        Include the <PipelineSegment> elements defined within this container into a Pipeline XML file

        Arguments:
            input_filename  {str} -- A path to a pipeline xml file
            output_filename {str} -- The destination path for the generated pipeline file
        """

        tree = ET.parse(input_filename)
        root = tree.getroot()
        logging.info("Composing %s into %s", input_filename, output_filename)
        self.included_segments = {}
        logging.debug("Including segments into pipelines")
        cwd = os.path.join(os.path.split(input_filename)[0])
        node_include(root, cwd, 0, segment_info=self)
        merge_pipeline_elements(root)
        # Write to file
        with open(output_filename, 'w+') as fd:
            fd.write(self.header_str)
            fd.write(ET.tostring(root).decode())

    def __str__(self):
        result = "SegmentInfo: (Number of Segments: %d | Segments: [%s])"
        segment_str = ", ".join([str(key) for key in self.segments.keys()])
        result = result % (len(self.segments.keys()), segment_str)
        return result


def node_include(root, cwd, depth, segment_info=None, target_info=None):
    replacement_list = []
    href_attr        = "href"
    path_attr        = "include_path"
    segment_attr     = "segment"
    segment_src_attr = "segment_folder"
    target_attr      = "target_segment"

    for child_element in root:
        replacement_element  = None
        replacement_elements = []
        if INCLUDE_TAG == child_element.tag:
            fmt_str    = "Include tag found - Type: %s [%s]"
            type       = "UNKNOWN"
            location   = "???"
            attributes = child_element.attrib
            if href_attr in attributes:
                type     = href_attr
                location = attributes[href_attr]
                # Set location to be relative from correct segment folder
                if segment_src_attr in attributes:
                    build_target = attributes[segment_src_attr]
                    location = textutils.process_path(os.path.join(node_include.root_topology_directory, "segments",
                                                                   build_target, location))
                else:
                    location = textutils.process_path(os.path.join(cwd, location))
                replacement_element = camx_include(location, depth=(depth + 1), target_info=target_info)
                replacement_element.attrib[path_attr] = textutils.process_path(location)
                replacement_elements = [replacement_element]
            elif segment_attr in attributes:
                type     = segment_attr
                segment  = attributes[segment_attr]
                location = segment
                if segment_info is not None:
                    if segment in segment_info.segments:
                        base_replacement_element = segment_info.segments[segment]
                    else:
                        raise CompositionException("No segment definition found for [%s]" % segment)
                    # Ensure that segment is not included recursively
                    if base_replacement_element not in segment_info.included_segments:
                        segment_info.included_segments[base_replacement_element] = True
                        replacement_element = node_include(base_replacement_element, cwd, depth + 1,
                                                           segment_info=segment_info, target_info=target_info)
                        # Add all child elements
                        replacement_elements = replacement_element.find(".")
                    else:
                        raise CompositionException(
                            "Segment [%s] is already included! Perhaps there is unbounded recursion?" % segment)
                else:
                    raise CompositionException("Segment encountered at invalid location: [%s]" % segment)
            elif target_attr in attributes:
                type = target_attr
                target_segment = attributes[target_attr]
                if target_info is not None:
                    if target_segment in target_info.target:
                        replacement_element = target_info.target[target_segment]
                        replacement_elements += replacement_element.findall("TargetFormat")
                        replacement_elements += replacement_element.findall("Range")
                    else:
                        raise CompositionException("No target buffer definition found for [%s]" % target_segment)
                else:
                    raise CompositionException("Target segment encountered at invalid location [%s]" % target_segment)

            logging.debug(fmt_str % (type, location))
            replacement_list.append((child_element, replacement_elements))
        else:
            node_include(child_element, cwd, depth + 1, segment_info=segment_info, target_info=target_info)

    # Remove CamxInclude tag and replace with correct content
    for old_element, new_elements in replacement_list:
        root.remove(old_element)
        for new_element in new_elements:
            root.append(new_element)

    return root


def camx_include(input_filename, cwd=None, depth=0, target_info=None):
    if os.name == "nt":
        input_filename = textutils.process_path(input_filename)
    tree = ET.parse(input_filename)
    root = tree.getroot()
    if cwd is None:
        cwd = os.path.split(input_filename)[0]
    return node_include(root, cwd, depth, target_info=target_info)


def merge_elements(root_element, elements, assert_equal=False):
    """
    merge_elements

    Given a list of elments that are children of the root_element, combine the list of elements so that:

    1) Only one element of that type remains in the root_element
    2) The remaining element has all child elements of the other elements as its own children

    Arguments:
        root_element {container} -- A container where elements is a valid a subset
        elements     {container} -- A container of xml.etree.ElementTree.Element

    Keyword Arguments:
        assert_equal {bool} -- if true, then assert that all elements in elements have the same text value (default: {False})

    Raises:
        Exception -- Raised if assert_equal is true and its constraint is violated
    """

    if len(elements) > 1:
        base_element = elements[0]
        for element in elements[1:]:
            root_element.remove(element)
            if assert_equal and element.text != base_element.text:
                raise Exception("Mismatching value for: %s (%s != %s)" % (element, base_element.text, element.text))
            for child_element in element:
                base_element.append(child_element)
    elif len(elements) == 1:
        logging.debug("No elements to merge")
    else:
        logging.debug("Elements was empty!")


def match_and_combine(search_root, element_type, match_types, combine_types, remove_existing=True, match_child_tags=None):
    """
    match_and_combine

    foo bar

    Arguments:
        search_root   {[Container]}   -- [List-like object of Element Tree Nodes where elements should be combined]
        element_type  {[str]}         -- [Tag of elements to combine]
        match_types   {[list of str]} -- [List of tags that must match in seperate elements to combine]
        combine_types {[list of str]} -- [List of tags that are children to element_type that should be combined]

    Keyword Arguments:
        remove_exisiting {bool}          -- [description] (default: {True})
        match_child_tags {[list of str]} -- [List of tags that further specifies how to create match_types UID] (default: {None})
    """

    unique_elements = {}
    for child_element in list(search_root):
        if child_element.tag == element_type:
            logging.debug("Match found: %s", child_element)
            match_id            = child_element.tag
            elements_to_combine = {}
            for match_element in child_element:
                if match_element.tag in match_types:
                    # match_id used to determine whether this element is unique
                    match_id += "M_(%s:%s)" % (match_element.tag, match_element.text)
                    logging.debug("Found required match element: %s", match_element.tag)
                    for match_child in match_element:
                        if match_child_tags is None or match_child.tag in match_child_tags:
                            match_id += "(%s:%s)" % (match_child.tag, match_child.text)
                    match_id = match_id.replace("\n", "")
                    match_id = match_id.replace(" ", "")
                elif match_element.tag in combine_types:
                    # combine_element_id used to determine if this combine element is unique
                    combine_element_id = match_element.tag
                    for child_match in match_element:
                        combine_element_id += "C_(%s:%s)" % (child_match.tag, child_match.text)
                    logging.debug("Found combine element: %s", match_element.tag)
                    combine_element_id = combine_element_id.replace("\n", "")
                    combine_element_id = combine_element_id.replace(" ", "")
                    if combine_element_id not in elements_to_combine:
                        # Only append unique combine elements
                        elements_to_combine[combine_element_id] = match_element
                    elif remove_existing:
                        child_element.remove(match_element)
            if match_id not in unique_elements:
                logging.debug("Found new UID: %s", match_id)
                unique_elements[match_id] = ElementInfo(search_root, child_element, elements_to_combine.keys())
            else:
                logging.debug("Found exisiting UID: %s", match_id)
                existing = unique_elements[match_id]
                for combine_element_id, new_element in elements_to_combine.items():
                    if combine_element_id not in existing.child_ids:
                        # Update existing element
                        logging.debug("Merging %s into %s [%s]", child_element, existing.element, match_id)
                        existing.update_info(new_element, combine_element_id)
                # Remove duplicate
                if remove_existing:
                    search_root.remove(child_element)
        else:
            logging.debug("Skipping: %s", child_element.tag)


def unnest_pipeline(pipeline_root, nested_pipeline):
    # Migrate any nodeslists
    for nodeslist in nested_pipeline.findall('NodesList'):
        pipeline_root.append(nodeslist)
    # Migrate any port linkages
    for linkages in nested_pipeline.findall('PortLinkages'):
        pipeline_root.append(linkages)
    pipeline_root.remove(nested_pipeline)


def merge_pipeline_elements(pipeline_root):
    try:
        # Unnest any included pipelines
        if (len(pipeline_root.findall('Pipeline')) > 0):
            for nested_pipeline in pipeline_root.findall('Pipeline'):
                unnest_pipeline(pipeline_root, nested_pipeline)
        # Ensure only one PipelineName exists and it is the same value
        merge_elements(pipeline_root, pipeline_root.findall('PipelineName'), assert_equal=True)
        # Ensure only one NodesList
        merge_elements(pipeline_root, pipeline_root.findall('NodesList'), assert_equal=False)
        # Ensure only one port linkage exists
        merge_elements(pipeline_root, pipeline_root.findall('PortLinkages'), assert_equal=False)
        # Combine node subelements
        main_nodeslist = pipeline_root.find('NodesList')
        if main_nodeslist is not None:
            match_and_combine(main_nodeslist, "Node", ["NodeId", "NodeInstanceId"], ["NodeProperty"])
            for node in main_nodeslist:
                assertrules.assert_element_rules(node)
        #  Combine Link subelements
        main_linkage = pipeline_root.find('PortLinkages')
        if main_linkage is not None:
            match_and_combine(main_linkage, "Link", ["SrcPort"], ["DstPort", "BufferProperties", "LinkProperties"], match_child_tags=["NodeId", "NodeInstanceId", "PortId"])
            for link in main_linkage:
                assertrules.assert_element_rules(link)
    except CompositionException as ce:
        source_name = ""
        if pipeline_root.find('PipelineName') is not None:
            source_name = pipeline_root.find('PipelineName').text
        elif pipeline_root.attrib and pipeline_root.attrib['name'] is not None:
            source_name = (pipeline_root.attrib)['name']
        error_msg = "[%s] %s" % (source_name, ce.message)
        terminate(error_msg)


def tag_include_elements(segment, build_target):
    # Mark CamxInclude tags with the segment folder they originate from
    for include_element in list(segment.iter('CamxInclude')):
        include_element.set('segment_folder', build_target)


def parse_segment_files(files):
    logging.info("Parsing Segment Files: %s", ", ".join(files))
    isolated_pipeline_segment = ET.Element(SEGMENT_LIST_TAG)
    for segment_file in files:
        other_tree = ET.parse(segment_file)
        other_root = other_tree.getroot()
        # Determine segment source and tag includes
        dirname, filename = os.path.split(segment_file)
        build_target_name = os.path.split(dirname)[-1]
        tag_include_elements(other_root, build_target_name)
        isolated_pipeline_segment.append(other_root)
    match_and_combine(isolated_pipeline_segment, CAMX_SEGMENT_TAG, [SEGMENT_NAME_TAG], ["NodesList", "PortLinkages"])
    for segment in isolated_pipeline_segment:
        merge_pipeline_elements(segment)
    return isolated_pipeline_segment


def terminate(error_message):
    print(error_message)
    # Invalidate cache
    if os.path.exists(CONFIG_FILE_NAME):
        os.remove(CONFIG_FILE_NAME)
    # Terminate program
    sys.exit(1)
