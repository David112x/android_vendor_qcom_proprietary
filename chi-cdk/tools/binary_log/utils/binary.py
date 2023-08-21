#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################

from struct import unpack
from functools import reduce

from binary_log.core.preamble_types import metadata_id_bitfield

# This is a map from a type to the list of ways that type might appear in our code.
_type_mappings_base = [
    (('char', "c"),                 ['CHAR']),
    (('signed char', "b"),          ['int8_t', 'INT8']),
    (('unsigned char', "B"),        ['SBYTE', 'std::byte', 'UCHAR', 'UINT8', 'uint8_t']),
    (('bool', "?"),                 ['_Bool']),
    (('short', "h"),                ['INT16', 'int16_t']),
    (('unsigned short', "H"),       ['UINT16', 'uint16_t']),
    (('int', "i"),                  ['BOOL', 'INT', 'INT32','int32_t', 'signed', 'signed int', 'CamxResult', 'CSLFence', 'CSLFenceResult']),
    (('unsigned int', "I"),         ['UINT', 'UINT32', 'CamX::PropertyID', 'uint32_t', 'unsigned']),
    (('long', "q"),                 []),
    (('unsigned long', "Q"),        []),
    (('long long', "q"),            ['INT64', 'int64_t']),
    (('unsigned long long', "Q"),   ['UINT64', 'SIZE_T', 'size_t', 'uint64_t']),
    (('float', "f"),                ['FLOAT']),
    (('double', "d"),               ['DOUBLE']),
    (('void *', "Q"),               ['CHIFEATUREREQUESTOBJECTHANDLE'])
]

result_mapping = [
    "Success",
    "EFailed",
    "EUnsupported",
    "EInvalidState",
    "EInvalidArg",
    "EInvalidPointer",
    "ENoSuch",
    "EOutOfBounds",
    "ENoMemory",
    "ETimeout",
    "ENoMore",
    "ENeedMore",
    "EExists",
    "EPrivLevel",
    "EResource",
    "EUnableToLoad",
    "EInProgress",
    "ETryAgain",
    "EBusy",
    "EReentered",
    "EReadOnly",
    "EOverflow",
    "EOutOfDomain",
    "EInterrupted",
    "EWouldBlock",
    "ETooManyUsers",
    "ENotImplemented",
    "EDisabled",
    "ECancelledRequest",
]

len_result_mapping = len(result_mapping)

_fundamental_types = reduce(lambda x, y: x + y, [[t1] + t2 for (t1, _), t2  in _type_mappings_base])

_type_mappings = {}
for target, maps in _type_mappings_base:
    key, _ = target
    _type_mappings[key] = target
    for elem in maps:
        _type_mappings[elem] = target

def regularize_type(t):
    return _type_mappings[t] if t in _type_mappings else (t, None)

def read_encoding(type_id, raw_data):
    if type_id.endswith("*"):
        type_id = "void *"

    if "enum" in type_id:
        type_id = "int"

    type_id = type_id.replace('const', '').replace('volatile', '').strip()

    unit_type, unpack_key = regularize_type(type_id)

    parse_result = None
    if unpack_key is not None:
        parse_result = unpack(unpack_key, raw_data)[0]

    if type == "CamxResult":
        parse_result = result_mapping[parse_result] if parse_result < len_result_mapping else "MemDump {}".format(parse_result)

    # We failed to convert it to any known types, create a hex representation of it in memory
    if parse_result == None:
        parse_result = reduce(lambda x, y: x + y, ['{}'.format(hex(c))[2:] for c in raw_data])
        parse_result = 'MemDump: 0x' + parse_result
    else:
        # Camx Bools are 32 bit ints in reality, convert them to useful True or False values
        if type_id == 'BOOL':
            if (parse_result != 0) and (parse_result != 1):
                parse_result = reduce(lambda x, y: x + y, ['{}'.format(hex(c))[2:] for c in raw_data])
                parse_result = 'Corrupted: 0x' + parse_result
            else:
                parse_result = (parse_result == 1)

        try:
            if (type(parse_result) == type(b'')) and (unit_type == 'char'):
                parse_result = parse_result.decode('utf8')
        except: pass

    # Parse pointers as hex values
    if type_id == "void *":
        parse_result = hex(parse_result).replace('L', '') # python may add a trailing L

    return parse_result

def get_bit(data, bit_pos):
    return (data & (1 << bit_pos - 1)) != 0

def clear_bit(data, bit_pos, value):
    return data & (~(1 << bit_pos - 1))

def set_bit(data, bit_pos, value):
    return (data & (~(1 << bit_pos - 1))) | (value << bit_pos)

def two_complement(value, length):
    if get_bit(value, length) != 0:
        value = value - (1 << length)
    return value

def get_mask(start_bit = 0, end_bit = 7):
    return ((1 << (end_bit - start_bit + 1)) - 1) << start_bit

def bytearray_to_int(hex_byte, start_bit = 0, end_bit = 7, signed = False):
    res = (hex_byte & get_mask(start_bit, end_bit)) >> start_bit

    if signed and get_bit(hex_byte, end_bit):
        return two_complement(res | get_mask(end_bit - start_bit, 7), 8)

    return res

def parse_bitfield(raw_msg, type_size, type_id, var_offset):
    start_bit  = var_offset % 8
    first_byte = raw_msg[0] if type(raw_msg[0]) == int else ord(raw_msg[0])
    unit_type  = regularize_type(type_id)

    signed = get_bit(first_byte, start_bit + type_size)
    if ('unsigned' in type_id) or ('unsigned' in unit_type) or type_size == 1 or ("bool" in unit_type):
        signed = False

    first_byte_end_bit = (7 if (type_size - 1) > 8 else (type_size - 1) % 8) + 1
    result = bytearray_to_int(first_byte, start_bit, first_byte_end_bit, signed)

    return result

def parse_variable(type_desc, var_offset, var_meta, raw_msg, known_types, true_char_array):
    result = None

    if metadata_id_bitfield in var_meta:
        # Convert the offset to the right byte to start reading from
        result = parse_bitfield(raw_msg, type_desc.size, type_desc.identifier, var_offset)
    elif len(type_desc.members) > 0:
        result = parse_encoding(type_desc.members, raw_msg, var_meta, known_types, true_char_array)
    else:
        # Check if the type is an array
        type_data = [t_data.replace(']', '').strip() for t_data in type_desc.identifier.split('[')]
        type_data_len = len(type_data)

        # Not an array or composite type, read its encoding directly
        if type_data_len == 1:
            parse_result = read_encoding(type_desc.identifier, raw_msg)

        # Parse an array
        elif type_data_len > 1:
            sub_array_sizes = [int(a) for a in type_data[1:]]
            total_count = reduce(lambda x, y: x * y, sub_array_sizes, 1)
            type_size = len(raw_msg) // total_count
            sub_raw_msg = [raw_msg[type_size*(i):type_size*(i+1)] for i in range(total_count)]
            cleaned_type = type_data[0].replace('const', '').replace('volatile','').strip()
            if (cleaned_type in _fundamental_types) or (type_data[0] not in known_types):
                try:
                    parse_result = [read_encoding(type_data[0], subset) for subset in sub_raw_msg]
                    if not(true_char_array) and ((cleaned_type == "char") or (cleaned_type == "CHAR")):
                        parse_result = parse_result if chr(0) not in parse_result else parse_result[:parse_result.index(chr(0))]
                        parse_result = "".join(parse_result)
                except:
#                    print("{} -> {}".format(type_data[0], cleaned_type))
                    parse_result = [hex(read_encoding('UINT8', subset)) for subset in sub_raw_msg]
            else:
                # we can safely ignore the var_offset as we are passing subsets for the binary data to be parsed
                # we can also ignore the metadata here as bitfields dont apply (cant have arrays of incomplete objects) or
                # if the type is a base type (wont make a difference)
                type_desc = known_types[type_data[0]]
                parse_result = [parse_variable(type_desc, 0, [], subset, known_types, true_char_array) for subset in sub_raw_msg ]

            # if its a multidimensional array, reconstruct the proper grouping
            if type_data_len > 2:
                for count in reversed(sub_array_sizes[1:]):
                    parse_result = [parse_result[idx*count:(idx+1)*count] for idx in range(int(len(parse_result)/count))]

        result = parse_result


    return result

def parse_encoding(encodings, raw_msg, metadata, known_types, true_char_array):
    raw_msg = bytearray(raw_msg)
    res = {}

    for member in encodings:
        size_in_bytes = member.descriptor.size
        offset_in_bytes = member.offset

        if metadata_id_bitfield in member.metadata:
            size_in_bytes = int(member.descriptor.size / 8)
            offset_in_bytes = int(member.offset / 8)

            if size_in_bytes == 0 or member.descriptor.size % 8 != 0:
                size_in_bytes = size_in_bytes + 1

        res[member.variable_id] = parse_variable(member.descriptor, member.offset, member.metadata,\
                                                 raw_msg[offset_in_bytes:offset_in_bytes + size_in_bytes], known_types, true_char_array)

    return res