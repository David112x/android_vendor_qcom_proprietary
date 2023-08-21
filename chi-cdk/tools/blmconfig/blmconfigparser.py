#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2020 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################

import argparse
import datetime
import itertools
import logging
import os.path
import sys

import argparse
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import ParseError

ORIGINAL_DATE       = 2020
COMMENT_STR_LEN     = 129
COMMENT_LINE        = "/" * (COMMENT_STR_LEN - 1)
CONFIG_OUTPUT_FILE  = "g_blmclientconfig.h"

def get_copyright_notice():
    """ Generate a proper copyright notice for the generated file
    """
    curr_year = datetime.datetime.now().year
    date_stamp = "2020" if curr_year == ORIGINAL_DATE else "2020-{}".format(curr_year)
    str_data = "\n".join([
        COMMENT_LINE,
        "// Copyright (c) {} Qualcomm Technologies, Inc.".format(date_stamp),
        "// All Rights Reserved.",
        "// Confidential and Proprietary - Qualcomm Technologies, Inc.",
        COMMENT_LINE
    ])
    return str_data

def validate_attributes(passed, needed, optional=[]):
    """Checks if the set of passed attributes is what is needed

    Throws:
    ParseError -- The passed in element does not have the right set of attributes

    Keyword arguments:
    elem -- The xml element to parse
    """
    filtered_passed = [p for p in passed if (p not in optional)]
    needed_but_not_passed = list(set(needed).difference(set(passed)))
    passed_but_not_needed = list(set(filtered_passed).difference(set(needed)))

    if (len(needed_but_not_passed) > 0) and (len(passed_but_not_needed) > 0):
        err_msg = "Missing Attribute(s): {}. Invalid passed attribute(s): {}."
        raise_error(ParseError, err_msg.format(needed_but_not_passed, passed_but_not_needed))
    elif len(needed_but_not_passed) > 0:
        err_msg = "Missing Attribute(s): {}, Passed attributes are: {}."
        raise_error(ParseError, err_msg.format(needed_but_not_passed, passed))
    elif len(passed_but_not_needed) > 0:
        err_msg = "Invalid attribute(s) passed: {}. Valid attributes are {}."
        raise_error(ParseError, err_msg.format(passed_but_not_needed, (needed + optional)))

def raise_error(error, msg):
    """ Raises an error and prints a nice backtrace for it
    """
    err_msg = [msg]
    format_msg = "\n".join(err_msg)
    raise error(format_msg)

def generate_blmconfigheader(maxBlmHintCount, maxTargetIdx):
    """ Generate the blmconfig file header
    """
    generated_file_data = "\n".join([
        COMMENT_LINE,
        "// @file  g_blmclientconfig.h",
        "// @brief BLMClient Config generated header",
        COMMENT_LINE,
        "",
        "",
        "#ifndef BLMCLIENTCONFIG_H",
        "#define BLMCLIENTCONFIG_H",
        "",
        "#define MAX_BLMCONFIG  " + str(maxBlmHintCount),
        "#define MAX_TARGETIDX  " + str(maxTargetIdx),
        "#define MAX_ResourceParams 1",
        ""
    ])

    file_contents = "\n".join([get_copyright_notice(), "", generated_file_data, ""])

    return file_contents

def parseBLMConfig(printDebug, inputXMLname):
    """Validate and Parse Config XML
    """

    tree = ET.parse(inputXMLname)
    root = tree.getroot()

    blmHintCount = 0
    generated_blmStruct = "\n".join([
        "// @brief Enum for resource Type",
        "enum ResourceType{",
        "ResourceTypeBW,",
        "};",
        "",
        "// @brief Structure for resourceType Value pairs",
        "struct ResourceParam{",
        "ResourceType resourceType;",
        "INT          value;",
        "};\n",
        "",
        "// @brief Structure to BLM usecaseID and ResourceParams",
        "struct ChiBLMTarget{",
        "INT            blmUsecaseID;",
        "ResourceParam  resourceParam[MAX_ResourceParams];",
        "};\n"
        "// @brief Structure to BLM Target info",
        "struct ChiBLMConfig{",
        "INT            socID;",
        "ChiBLMTarget   chiBLMTarget[MAX_BLMCONFIG];",
        "};\n"
    ])

    generated_blmXMLdata = "\n".join([
        "const ChiBLMConfig g_blmClientInfo[] =",
        "{"
    ])

    maxTargetIdx = 0
    for target in root.findall("Target"):
        if printDebug:
            print target.tag, target.attrib
        validate_attributes(target.attrib.keys(), ["socID"])
        generated_blmXMLdata += "\n".join([
                "    {" + str(target.attrib['socID']) + ",",
                "        {"
                "\n"
            ])
        maxTargetIdx += 1

        for child in target:
            if printDebug:
                print child.tag, child.attrib
            validate_attributes(child.attrib.keys(), ["usecaseID"])

            generated_blmXMLdata += "\n".join([
                "        {" + str(child.attrib['usecaseID']) + ",",
                "            {"
                ""
            ])
            for resourceParam in child.findall("ResourceParam"):
                validate_attributes(resourceParam.attrib.keys(), ["type","value"])
                if printDebug:
                    print resourceParam.attrib
                generated_blmXMLdata += "\n".join([
                    "",
                    "                {"+str(resourceParam.attrib['type']) + "," + str(resourceParam.attrib['value']) + "},"
                ])

                generated_blmXMLdata += "\n".join([
                "",
                "            },"
                "",
                "        },"
                "",
                ""
            ])
            blmHintCount += 1

        generated_blmXMLdata = generated_blmXMLdata + "    },\n    },\n"

    generated_blmXMLdata = generated_blmXMLdata + "};\n"
    if printDebug:
        print generated_blmXMLdata
        print "blm Hint Count " + str(blmHintCount)


    return generated_blmStruct + "\n" + generated_blmXMLdata, blmHintCount, maxTargetIdx



if __name__ == '__main__':
    arg_parser = argparse.ArgumentParser(description="Generates BLM config header from the given xml file")
    arg_parser.add_argument('-i', '--inputFile', action='store', type=str, dest='input_filename', required=True)
    arg_parser.add_argument('-l', '--log_level', action='store', type=str, dest='log_level', default="none", required=False)
    arg_parser.add_argument('-g', '--debug', action='store_true', default=False, dest='debug')
    arg_parser.add_argument('-o', '--output_dir', action='store', type=str, dest='output_dir', required=True)

    args           = arg_parser.parse_args()
    inputXMLname   = args.input_filename
    printDebug     = args.debug

    if printDebug:
        print "Input args: " + str(args)

    outputFilename = os.path.join(args.output_dir, CONFIG_OUTPUT_FILE)
    oldest_output = 0

    outputFile = open(outputFilename, "w")

    g_parseXMLInfo, g_blmConfigHint, g_maxTargetIdx = parseBLMConfig(printDebug, inputXMLname)
    g_headerInfo                                    = generate_blmconfigheader(g_blmConfigHint, g_maxTargetIdx)

    #writing into Output Header
    outputFile.write(g_headerInfo)
    outputFile.write(g_parseXMLInfo)
    outputFile.write("\n")
    outputFile.write("#endif // BLMCLIENTCONFIG_H_H")
    outputFile.close()


