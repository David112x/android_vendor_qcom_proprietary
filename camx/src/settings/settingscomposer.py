#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################
import argparse
import os.path
import logging
import xml.etree.ElementTree as ET

class SettingsException(Exception):
    def __init__(self, *args):
        Exception.__init__(self, *args)

def override_setting(base_settings, subgroup_name, variable_name, new_setting):
    current_setting = base_settings.find(".//settingsSubGroup[@Name='%s']//setting[VariableName='%s']" % (subgroup_name,
                                         variable_name))
    if current_setting is not None:
        for property in current_setting:
            property.text = new_setting.find(property.tag).text

def generate_settings(common_file, override_file, output_file, header):
    common_file_ts = os.path.getmtime(common_file)
    override_file_ts = os.path.getmtime(override_file)
    if os.path.exists(output_file) == True:
        output_file_ts = os.path.getmtime(output_file)
    else:
        output_file_ts = 0
    if common_file_ts < output_file_ts and override_file_ts < output_file_ts:
        print "No update in common settings(",common_file,") and target settings(",override_file,"): Skip generating composed settings(",output_file,")."
        return

    common_file_tree = ET.parse(common_file)
    base_settings = common_file_tree.getroot()
    if common_file == override_file:
        with open(output_file, 'w+') as fd:
            fd.write(header)
            fd.write(ET.tostring(common_file_tree.getroot()))
        return
    override_file_tree = ET.parse(override_file)
    override_settings = override_file_tree.getroot()
    for subgroup in list(override_settings.findall('settingsSubGroup')):
        subgroup_name = subgroup.get('Name')
        for setting in list(subgroup.findall('setting')):
            variable_name = setting.find('VariableName')
            if variable_name is not None:
                variable_name = variable_name.text
            else:
                raise SettingsException('Cannot override setting when VariableName not defined')
            override_setting(base_settings, subgroup_name, variable_name, setting)
    # Generate file and append header
    logging.info("Generating %s" % output_file)
    with open(output_file, 'w+') as fd:
        fd.write(header)
        fd.write(ET.tostring(common_file_tree.getroot()))

if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description="Generate target based camxsettings.xml")
    arg_parser.add_argument('-c', '--commonSettings',
                            action='store',
                            type=str,
                            dest='commonSettings',
                            required=True)
    arg_parser.add_argument('-t', '--targetSettings',
                            action='store',
                            type=str,
                            dest='targetSettings',
                            required=True)
    arg_parser.add_argument('-o', '--outputSettings',
                            action='store',
                            type=str,
                            dest='outputSettings',
                            required=True)
    arg_parser.add_argument('-H', '--headerFile',
                            action='store',
                            type=str,
                            dest='header_filename',
                            required=False)
    args = arg_parser.parse_args()
    common_settings = args.commonSettings
    if os.path.isfile(args.targetSettings):
        target_settings = args.targetSettings
    else:
        logging.info("Warning: Override settings file is not found. XML file is generated using common settings")
        target_settings = args.commonSettings
    output_settings = args.outputSettings
    # Set header
    if args.header_filename:
        logging.info("Appending contents of %s to the generated files" % args.header_filename)
        with open(args.header_filename) as fd:
            header = fd.read()
    else:
        header = ""
    generate_settings(common_settings, target_settings, output_settings, header)
