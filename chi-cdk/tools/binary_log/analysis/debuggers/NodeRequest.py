#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################

import Utils
from collections import defaultdict


class BufferDependency:

    def __init__(self, port_request):
        self.output_port    = None
        self.input_ports    = []
        if port_request.is_output:
            self.output_port = port_request
        else:
            self.input_ports.append(port_request)
        self.fence_handle   = port_request.fence_handle     # Fence tracking
        self.fence_signaled = False
        self.fence_error    = False

    def add_output_port(self, output_port_req):
        """ Add output port to buffer dependency """
        if self.output_port is None:
            self.output_port = output_port_req

    def add_input_port(self, input_port_req):
        """ Add input port to buffer dependency """
        self.input_ports.append(input_port_req)

    def fence_status(self):
        if self.fence_signaled: # Fence status is known, OK or ERR
            return 'OK' if not self.fence_error else 'ERR'
        else:
            return '--'         # Fence status is unknown

    def signal_fence(self, is_error):
        """ Update fence status """
        self.fence_signaled = True
        self.fence_error    = is_error

    def get_input_port(self, node_id, node_instance_id):
        for input_port in self.input_ports:
            if input_port.node_id == node_id and input_port.node_instance_id == node_instance_id:
                return input_port.port_id
        return -1

    def get_output_port(self):
        if self.output_port is not None:
            return self.output_port.get_identifier()
        return -1


class OutputPortRequest:

    def __init__(self, output_port_request):
        self.is_output        = True
        self.node_id          = output_port_request['nodeId']
        self.node_instance_id = output_port_request['nodeInstanceId']
        self.port_id          = output_port_request['portId']
        self.fence_handle     = output_port_request['rFenceHandlerData']['hFence']

    def get_identifier(self):
        return self.port_id


class InputPortRequest:

    def __init__(self, input_port_request):
        self.is_output        = False
        self.node_id          = input_port_request['nodeId']
        self.node_instance_id = input_port_request['nodeInstanceId']
        self.port_id          = input_port_request['rPerRequestInputPort']['portId']
        self.fence_handle     = input_port_request['hFence']

    def get_identifier(self):
        return '{}_{}_{}'.format(self.node_id, self.node_instance_id, self.port_id)


class NodeRequest:

    # Events to be handled by NodeRequest class
    event_ids = ['Node_PartialMetadataDone',
                 'Node_MetadataDone',
                 'Node_RequestIdDone']

    # Initialize
    def __init__(self, node_id, node_instance_id):
        self.node_id               = node_id
        self.node_instance_id      = node_instance_id
        self.fence_to_dependency_map = defaultdict(list)    # Buffer tracking
        self.input_dependencies    = {}
        self.output_dependencies   = {}
        self.partial_metadata_done = False                  # Node status tracking
        self.metadata_done         = False
        self.request_id_done       = False

    def print_summary(self):
        """ Prints row indicating RequestId, MetadataDone, PartialMetaDone status """
        node_id = ('{}_{}'.format(Utils.node_id_to_node_name(self.node_id), self.node_instance_id)).ljust(22)
        request_id_status   = ('Complete' if self.request_id_done else 'Incomplete').center(14)
        metadata_status     = ('Complete' if self.metadata_done else 'Incomplete').center(14)
        partial_meta_status = ('Complete' if self.partial_metadata_done else 'Incomplete').center(16)
        print('            {}{}|{}|{}'.format(node_id, request_id_status, metadata_status, partial_meta_status))

    def print_fence_table(self):
        """ Prints table indicating input/output buffer requests their associated fence status"""
        sorted_inputs  = sorted(self.input_dependencies.values(), key=lambda x: x.get_input_port(self.node_id, self.node_instance_id))
        sorted_outputs = sorted(self.output_dependencies.values(), key=lambda x: x.get_output_port())
        num_rows = max(len(sorted_inputs), len(sorted_outputs))
        node_id = ('{}_{}'.format(Utils.node_id_to_node_name(self.node_id), self.node_instance_id)).ljust(27)
        print('            {}Fence|In Port|Status|Status|OutPort|Fence'.format(node_id))
        for row in range(0, num_rows):
            in_fence  = '  '
            in_port   = '  '
            in_status = '  '
            in_sep    = ' '
            if row < len(sorted_inputs):
                in_fence  = sorted_inputs[row].fence_handle
                in_port   = sorted_inputs[row].get_input_port(self.node_id, self.node_instance_id)
                in_status = sorted_inputs[row].fence_status()
                in_sep    = '|'
            out_port   = '  '
            out_fence  = '  '
            out_status = '  '
            out_sep    = ' '
            if row < len(sorted_outputs):
                out_fence  = sorted_outputs[row].fence_handle
                out_port   = sorted_outputs[row].get_output_port()
                out_status = sorted_outputs[row].fence_status()
                out_sep    = '|'
            print('                                        {:3} {}  {:2}   {} {:3}  | {:3}  {}  {:2}   {} {:3}'.format(
                in_fence, in_sep, in_port, in_sep, in_status, out_status, out_sep, out_port, out_sep, out_fence))

    def input_port_request(self, input_port_request):
        """ Setup node input port request """
        if input_port_request.fence_handle != 0:
            request_id = input_port_request.get_identifier()
            if request_id not in self.input_dependencies:
                self.input_dependencies[request_id] = BufferDependency(input_port_request)
                self.fence_to_dependency_map[input_port_request.fence_handle].append(self.input_dependencies[request_id])

    def output_port_request(self, output_port_request):
        """ Setup node output port request """
        if output_port_request.fence_handle != 0:
            request_id = output_port_request.get_identifier()
            if request_id not in self.output_dependencies:
                self.output_dependencies[request_id] = BufferDependency(output_port_request)
                self.fence_to_dependency_map[output_port_request.fence_handle].append(self.output_dependencies[request_id])

    def signal_fence(self, fence_handle, is_error):
        """ Update buffer dependency with fence status """
        for dependency in self.fence_to_dependency_map[fence_handle]:
            dependency.signal_fence(is_error)

    def handle_event(self, event_id, log):
        """ Handle node status event """
        if event_id == 'Node_PartialMetadataDone':
            self.partial_metadata_done = True
        elif event_id == 'Node_MetadataDone':
            self.metadata_done = True
        elif event_id == 'Node_RequestIdDone':
            self.request_id_done = True
