#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################

from .preamble_types import TypeDescriptor, VariableDescriptor, EnumDescriptor, LogEncoding, Preamble, log_id_ignored, metadata_id_baseclass
from .preamble_types import metadata_id_bitfield, metadata_id_baseclass
from functools import reduce

import json

###############################################################################################################################
# Errors
###############################################################################################################################
class InvalidJsonErr(Exception):
    """ Occurs when trying to add another encoding to an event that already has a different encoding"""
    pass

def is_structured_type(type_id):
    """Whether this type may have members or not.

    Attributes:
        type_id (str): The identifier for this type.

    Returns:
        True if we this type has members, False otherwise.
    """
    structured_nodes = ['struct', 'class', 'union']
    res = False
    for struct_type in structured_nodes:
        res = res or (struct_type in type_id)
    return res

###############################################################################################################################
# Preamble Parser
###############################################################################################################################

class PreambleParser:
    """A class to help parse a preamble file to a preamble object.

    Attributes:
        filename (str):          The file to parse.
    """

    def __init__(self, filename):
        """Constructor

        Args:
            filename (str):          The name of the file this preamble belongs to.
        """
        self.filename = filename

    def parse_log_enum(self, json_obj):
        return EnumDescriptor(json_obj['var'], int(json_obj['enum val']), int(json_obj['size']))

    def parse_var_encoding(self, json_obj):
        """ Given a textual node, parse it for the variable descriptors and types that it may include.

        Args:
            node (Node): A representation of an entity in the textual dump with the hierarchy info needed for parsing members.

        Returns:
            (set([TypeDescriptor]), VariableDescriptor) : A tuple with the variable descriptor we parsed and types it included.
        """
        types_set = set([])
        var_metadata = []
        type_metadata = []

        # calculate the offset for the types
        offset    = json_obj['offset'] if ('offset' in json_obj) else -1

        if '(base)' in json_obj['type']:
            json_obj['type'] = json_obj['type'].replace(' (base)', '')
            var_metadata.append(metadata_id_baseclass)

        size = 1
        if 'size' in json_obj:
            size = int(json_obj['size'])
        else:
            size = int(json_obj['size (bits)'])
            # encode it in the type name because really, int and int:3 are NOT the same type.
            json_obj['type'] = json_obj['type'] + ' ({}_{})'.format(metadata_id_bitfield, size)
            var_metadata.append(metadata_id_bitfield)
            type_metadata.append(metadata_id_bitfield)

        type_desc = TypeDescriptor(json_obj['type'], size, type_metadata)
        var_desc  = VariableDescriptor(json_obj['var'], offset, type_desc, var_metadata)

        if ('members' in json_obj):
            for child_obj in json_obj['members']:
                child_type_set, child_var = self.parse_var_encoding(child_obj)
                if child_var.variable_id != '':
                    type_desc.members.append(child_var)
                    types_set = types_set.union(child_type_set)

        types_set.add(type_desc)

        return types_set, var_desc

    def parse_json(self, name, raw_json):
        """ Parse the json obj given and generate a ``Preamble`` structure representing it.

        Args:
            name: (str)         Name of the preamble currently being generated
            raw_json (JSONObj): A json object representing the preamble

        Returns:
            A Preamble object representing the file given to the parser.
        """
        p = Preamble(name)
        for log_json_obj in raw_json:

            # Add the event enum to the preamble
            event_enum = self.parse_log_enum(log_json_obj['log'])
            if event_enum.identifier != log_id_ignored:
                p.add_log_event(event_enum)

            # parse the data members associated with the var encoding
            event_data = log_json_obj['data']
            log_encoding_data = [self.parse_var_encoding(json_elem) for json_elem in event_data]

            # add the encoding and fix the offsets
            encoding = [var_desc for (_, var_desc) in log_encoding_data]
            size = reduce(lambda x, y: x + y, [var_desc.descriptor.size for var_desc in encoding])

            # Recalculate offsets
            encoding[0].offset = 0
            if len(encoding) > 1:
                for idx in range(1, len(encoding)):
                    encoding[idx].offset = encoding[idx-1].offset + encoding[idx-1].descriptor.size

            for types, _ in log_encoding_data:
                for type in types:
                    p.add_type_descriptor(type)

            if event_enum.identifier != log_id_ignored:
                log_encoding = LogEncoding(event_enum, size, encoding)
                p.add_event_encoding(log_encoding)

        return p

    def parse_str(self, str):
        """ Parse the string given and generate a ``Preamble`` structure representing it.

        Args:
            str (str):  The str to parse

        Returns:
            A Preamble object representing the file given to the parser.

        Throws:
            InvalidJsonErr: The json string passed is invalid.
        """
        try:
            return self.parse_json(self.filename, json.loads(str))
        except:
            raise InvalidJsonErr(self.filename)

    def parse(self, json_file):
        """ Parse the preamble file given and generate a ``Preamble`` structure representing it.

        Args:
            json_file (Node): A json object representing the

        Returns:
            A Preamble object representing the file given to the parser.

        Throws:
            InvalidJsonErr: The file passed is not a valid preamble file.
        """
        try:
            self.filename = json_file.name
            return self.parse_json(json_file.name, json.load(json_file))
        except:
            raise InvalidJsonErr(json_file.name)