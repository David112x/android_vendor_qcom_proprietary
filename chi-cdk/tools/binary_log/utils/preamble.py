#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################
from ._context import binary_log
from binary_log.core.preamble_types import Preamble, EnumDescriptor, TypeDescriptor, VariableDescriptor, LogEncoding, log_id_ignored
from binary_log.core.preamble_parser import PreambleParser

def join_preambles(preamble_a, preamble_b):
    """ Combines two preamble structures into one. Will assert on an incompatibl.

    Args:
        preamble_a (Preamble): The first preamble to be joined.
        preamble_b (Preamble): The second preamble to be joined.

    Returns:
        Preamble: The preamble joined by the two passed in.
    """
    joint_preamble = Preamble("")

    # Join type descriptors, collision chances are low here
    for cxx_type in preamble_a.type_descriptors:
        joint_preamble.add_type_descriptor(cxx_type)

    for cxx_type in preamble_b.type_descriptors:
        # if a type is already in the preamble, just ignore it
        if not(cxx_type in joint_preamble.type_descriptors):
            identifiers = [type_desc.identifier for type_desc in joint_preamble.type_descriptors]

            # We have an id collision but type difference, we can assume symbols coming from a single .o will NOT have multiple IDs
            # So hashing those will be ok
            if cxx_type.identifier in identifiers:
                cxx_type.identifier = '{}_{}'.format(cxx_type.identifier, hash(preamble_b.filename)).replace('_-', '__')
                cxx_type.metadata.append('src_preamble({})'.format(preamble_b.filename.replace('\\', '/')))
                joint_preamble.add_type_descriptor(cxx_type)
                continue
            else:
                joint_preamble.add_type_descriptor(cxx_type)

    # Join Encodings
    for key in preamble_a.encodings.keys():
        joint_preamble.add_event_encoding(preamble_a.encodings[key])

    for key in preamble_b.encodings.keys():
        try:
            joint_preamble.add_event_encoding(preamble_b.encodings[key])
        except:
            print('\nError: Incompatible log encodings found for event "{}"'.format(key))
            print('\t File: {}'.format(preamble_a.filename))
            print('\t\t Encoding: {}'.format([(x.descriptor.identifier, x.variable_id) for x in preamble_a.encodings[key].encodings]))
            print('\t File: {}'.format(preamble_b.filename))
            print('\t\t Encoding: {}\n'.format([(x.descriptor.identifier, x.variable_id) for x in preamble_b.encodings[key].encodings]))
            raise

    return joint_preamble

def parse_json_data(json_data):
    import json
    parsed_json = json.loads(json_data)
    preamble = Preamble("Preamble to parse")

    get_metadata = lambda x : x["metadata"] if "metadata" in x else []

    if "log_events" not in parsed_json:
        return PreambleParser("Run from ParseJsonData").parse_json("Preamble to parse", parsed_json)

    for log_event in parsed_json["log_events"]:
        if log_id_ignored != log_event["id"]:
            event = EnumDescriptor(log_event["id"], int(log_event["value"]), int(log_event["size"]))
            preamble.add_log_event(event)

    type_ids = [type_desc["id"] for type_desc in parsed_json["type_descriptors"]]
    def get_type(identifier):
        idx = type_ids.index(identifier)
        return parsed_json["type_descriptors"][idx]

    types_map = {}
    def add_type_desc(type_desc):
        if (type_desc["id"]) in types_map:
            return

        cxx_type = TypeDescriptor(type_desc["id"], int(type_desc["size"]), get_metadata(type_desc))

        for member in type_desc["members"]:
            if not(member["type"] in types_map):
                add_type_desc(get_type(member["type"]))

            var_type = VariableDescriptor(member["id"], int(member["offset"]), types_map[member["type"]], get_metadata(member))
            cxx_type.add_member_type(var_type)

        types_map[cxx_type.identifier] = cxx_type
        preamble.add_type_descriptor(cxx_type)

    [add_type_desc(x) for x in parsed_json["type_descriptors"]]

    for log_encoding in parsed_json["log_encodings"]:
        encoding_vars = [VariableDescriptor(var["id"], 0, types_map[var["type"]], get_metadata(var)) for var in log_encoding["encoding"]]
        total_size = encoding_vars[0].descriptor.size

        for idx in range(1, len(encoding_vars)):
            var_desc = encoding_vars[idx]
            prev_var_desc = encoding_vars[idx-1]
            encoding_vars[idx].offset = prev_var_desc.offset + prev_var_desc.descriptor.size
            total_size = total_size + var_desc.descriptor.size

        preamble.add_event_encoding(LogEncoding(preamble.get_log_event(log_encoding["log_event"]), total_size, encoding_vars))

    return preamble
