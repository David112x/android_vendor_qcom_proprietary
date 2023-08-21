###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################
from functools import reduce

_identity_map = lambda x: x

###############################################################################################################################
# Interesting log identifiers
###############################################################################################################################
log_id_ignored = 'BinaryLog__IGNORE'
"""str: Indicates that this log_event is to be ignored. Used currently to get array types into the log as well"""

###############################################################################################################################
# Metadata IDs
###############################################################################################################################
metadata_id_baseclass = 'base'
"""str: Associated with a variable, indicates this member is a parent (or higher on the inheritence hiearchy) of its parent"""

metadata_id_bitfield = 'bitfield'
"""str: Associated with a type or variable, indicates this object represents a bitfield"""

def list_to_str_map(list, f = _identity_map):
    result = '['
    # Print Var Descriptors
    if len(list) > 0:
        for member in list[0:-1]:
            result = result + " " + f(member) +  ", "
        result = result + " " + f(list[-1])
    result = result + ']'
    return result

###############################################################################################################################
# Errors
###############################################################################################################################
class EncodingCollisionError(Exception):
    """ Occurs when trying to add another encoding to an event that already has a different encoding"""
    pass

###############################################################################################################################
# Descriptor Types
###############################################################################################################################

class EnumDescriptor:
    """Enumerations representation in the preamble

    Attributes:
        identifier (str): The identifier of the enumerator
        value      (int): The value that backs this enumerator, this is what we will find in the binary log files.
        size       (int): The size of this enum
    """

    def __init__(self, identifier, value, size):
        """ Constructor for the log event type
        Args:
            identifier (str):   The name of the enumerator
            value (int):        The value that backs this enumerator, this is what we will find in the binary log files.
            size (int):         The size of this enum
        """
        self.value      = int(value)
        self.identifier = identifier
        self.size       = int(size)

    def __eq__(self, other):
        """Override the default equality behavior"""
        return isinstance(other, EnumDescriptor) and (self.value == other.value) and (self.identifier == other.identifier)

    def __ne__(self, other):
        """Override the default inequality behavior"""
        return not(self == other)

    def __hash__(self):
        """Override the default hash behavior"""
        return hash(self.value) ^ hash(self.identifier)

    def to_str(self):
        return '{{ "id": "{}", "size": "{}", "value": "{}"}}'.format(self.identifier, self.size, self.value)

class TypeDescriptor:
    """Type represents a description of a c++ object layout in memory.

    Attributes:
        identifier:                     The type's identifier
        size:                           The size of the type
        members ([VariableDescriptor]): The data members of a type (identifiers, offsets, and layouts)
        metadata ([str]):               Metadata related to this type (is this a bitfield)
    """

    def __init__(self, identifier, size, metadata):
        """ Constructor
        Args:
            identifier(str):    The type's identifier
            size(int):          The size of the type
            metadata([str]):    Metadata related to the type (eg. is it a bitfield)
        """
        self.identifier = identifier
        self.size       = int(size)
        self.members    = []
        self.metadata   = metadata
        self.metadata.sort()

    def add_member_type(self, type_encoding):
        """ Add a data member to this type
        Args:
            type_encoding (VariableDescriptor): The encoding of a type (identifiers, offsets, and layouts)
        """
        self.members.append(type_encoding)

    def to_str(self):
        members_str = list_to_str_map(self.members, lambda x: x.to_str(True))
        metadata_str = list_to_str_map(self.metadata, lambda x: '{}'.format(x))
        return '{{ "id": "{}", "size": "{}", "metadata": {}, "members" : {} }}'.format(self.identifier, self.size, metadata_str, members_str)

    def __eq__(self, other):
        """Override the default equality behavior"""
        return isinstance(other,TypeDescriptor) and \
                self.identifier == other.identifier and \
                self.size == other.size and \
                self.members == other.members and \
                self.metadata == other.metadata

    def __ne__(self, other):
        """Override the default inequality behavior"""
        return not(self == other)

    def __hash__(self):
        """Override the default hash behavior"""

        member_hash = 0
        if len(self.members) > 0:
            member_hash = reduce((lambda x, y: hash(x) ^ hash(y)), self.members)

        metadata_hash = 0
        if len(self.metadata) > 0:
            metadata_hash = reduce((lambda x, y: hash(x) ^ hash(y)), self.metadata)

        return hash(self.identifier) ^ hash(self.size) ^ hash(metadata_hash) ^ hash(member_hash)

class VariableDescriptor:
    """Represents a variable in the dump of a LogEvent

    Attributes:
        variable_id (str):          The name for this variable within the log call
        offset (int):               The offset of the type within the log call data
        descriptor(TypeDescriptor): The descriptor for the type (its layout, members...etc)
        metadata([str]):            The metadata related to this variable
    """
    def __init__(self, variable_id, offset, descriptor, metadata):
        """ Constructor
        Args:
            identifier (str):           The name for this variable within the log call
            offset (int):               The offset of the type within the log call data
            descriptor(TypeDescriptor): The descriptor for the type (its layout, members...etc)
            metadata([str]):            The metadata related to this variable
        """
        self.variable_id = variable_id
        self.offset      = int(offset)
        self.descriptor  = descriptor
        self.metadata    = metadata
        self.metadata.sort()

    def __eq__(self, other):
        """Override the default equality behavior, variable names don't matter """
        return isinstance(other, VariableDescriptor) and (self.offset == other.offset) and (self.descriptor == other.descriptor)

    def __ne__(self, other):
        """Override the default inequality behavior, variable names don't matter """
        return (self.offset != other.offset) or (self.descriptor != other.descriptor)

    def __hash__(self):
        """Override the default hash behavior"""
        metadata_hash = 0
        if len(self.metadata) > 0:
            metadata_hash = reduce((lambda x, y: hash(x) ^ hash(y)), self.metadata, 0)

        return hash(self.variable_id) ^ hash(self.descriptor.identifier) ^ hash(self.offset) ^ metadata_hash

    def to_str(self, show_offset):
        if show_offset:
            return '{{ "id": "{}", "offset": "{}", "type": "{}", "metadata": {}}}'.format(self.variable_id, self.offset, self.descriptor.identifier, self.metadata)
        else:
            return '{{ "id": "{}", "type": "{}"}}'.format(self.variable_id, self.descriptor.identifier)

class LogEncoding:
    """The event dumped as well as the types dumped with it.

    Attributes:
        log_event (EnumDescriptor):       The encoding dump will belong to this log event.
        encodings ([VariableDescriptor]): List of type encodings that will help decode this call.
        size (int):                       The size of the dump in binary.
    """

    def __init__(self, log_event, size, encoding = []):
        """ Constructor.
        Args:
            log_event (str):           The encoding dump will belong to this log event.
            size (int):                The size of the dump in binary.
        """
        self.log_event  = log_event
        self.encodings  = encoding
        self.size       = int(size)

    def __eq__(self, other):
        """Override the default equality behavior, variable names don't matter. """
        return isinstance(other, LogEncoding) and \
               self.log_event == other.log_event and \
               self.encodings == other.encodings and \
               self.size == other.size

    def __ne__(self, other):
        """Override the default inequality behavior, variable names don't matter. """
        return not (self.__eq__(other))

    def __hash__(self):
        """Override the default hash behavior. """
        return hash(self.log_event) ^ hash(self.encodings) ^ hash(self.size)

    def append_type_encoding(self, type_encoding):
        """ Add a type encoding to this log.
        Args:
            type_encoding (VariableDescriptor): The encoding of a type to be added (identifiers, offset, and layout).
        """
        if len(self.encodings) > 0:
            type_encoding.offset = self.encodings[-1].offset + self.encodings[-1].descriptor.size
        self.encodings.append(type_encoding)

    def to_str(self):
        encodings_str = list_to_str_map(self.encodings, lambda x: x.to_str(False))
        return '{{ "log_event": "{}", "encoding": {} }}'.format(self.log_event.identifier, encodings_str)

class Preamble:
    """Represents a variable in the dump of a LogEvent.

    Attributes:
        filename         (str):                 The name of the file this preamble belongs to.
        log_events       ([EnumDescriptor]):    List of the events found within this preamble.
        type_descriptors ([TypeDescriptor]):    The descriptor for the type (its layout, members...etc).
        encodings        ({(str,LogEncoding)}): The mapping between a log_event and its encoding.
    """

    def __init__(self, filename):
        """Constructor
        Args:
            filename (str): The name of the file this preamble belongs to.
        """
        self.filename = filename
        self.log_events = []
        self.type_descriptors = []
        self.encodings  = {}

    def get_log_event(self, identifier):
        """ Get a log event by its identifier.

        Args:
            identifier (str): The log event's identifier.

        Returns:
            EnumDescriptor: describing the log event if found

            If the element is not found, ``None`` is returned.
        """
        for log_event in self.log_events:
            if log_event.identifier == identifier:
                return log_event

        return None

    def get_log_event_by_value(self, value):
        """ Get a log event by its value.

        Args:
            value (int): The log event's value.

        Returns:
            EnumDescriptor: describing the log event if found

            If the element is not found, ``None`` is returned.
        """
        for log_event in self.log_events:
            if log_event.value == value:
                return log_event

        return None

    def get_type_descriptor(self, identifier):
        """ Get a type descriptor by its identifier.

        Args:
            identifier (str): The type descriptor's identifier

        Returns:
            EnumDescriptor: describing the log event if found

            If the element is not found, ``None`` is returned.
        """
        type_identifiers = [cxx.identifier for cxx in self.type_descriptors]
        if identifier in type_identifiers:
            return self.type_descriptors[type_identifiers.index(identifier)]
        return None

    def add_log_event(self, enum_desc):
        """Adds an enum descriptor to the list of log events.

        Attributes:
            enum_desc (EnumDescriptor):   The enumerator to add
        """
        if enum_desc not in self.log_events:
            self.log_events.append(enum_desc)

    def add_type_descriptor(self, cxx_type):
        """Adds a type descriptor preamble.

        Attributes:
            cxx_type (TypeDescriptor): The name of the enumerator.
        """
        if not(cxx_type in self.type_descriptors):
            self.type_descriptors.append(cxx_type)

    def add_event_encoding(self, log_encoding):
        """Associate a log event with an encoding.

        Attributes:
            log_encoding (LogEncoding): The encoding of the event.
        """
        if log_encoding.log_event.identifier in self.encodings.keys():
            if (log_encoding != self.encodings[log_encoding.log_event.identifier]):
                if (log_id_ignored != log_encoding.log_event.identifier):
                    raise EncodingCollisionError(self.filename, log_encoding.log_event.identifier, log_encoding, self.encodings[log_encoding.log_event.identifier])
                else:
                    return

        self.encodings[log_encoding.log_event.identifier] = log_encoding
        if not(log_encoding.log_event in self.log_events):
            self.log_events.append(log_encoding.log_event)

    def to_str(self):
        """ Creates a json-string representation of the preamble type

        Returns:
            A json encoding of the dump as a ``str``
        """
        log_events_str = list_to_str_map(self.log_events, lambda x: x.to_str())
        type_descriptors_str = list_to_str_map(self.type_descriptors, lambda x: x.to_str())

        # Print Encodings
        log_encoding_str = '"log_encodings" : ['
        encoding_keys = list(self.encodings.keys())
        for encoding_key in encoding_keys[0:-1]:
            log_encoding_str = log_encoding_str + "\t" + self.encodings[encoding_key].to_str() +  ", "
        log_encoding_str = log_encoding_str + "\t" + self.encodings[encoding_keys[-1]].to_str()
        log_encoding_str = log_encoding_str + "\t]"

        return '{{ "log_events": {}, "type_descriptors": {}, {} }}'.format(log_events_str, type_descriptors_str, log_encoding_str)