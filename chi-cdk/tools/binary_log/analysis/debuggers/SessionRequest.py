#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################

from NodeRequest import NodeRequest
import Utils


class SessionRequest:

    event_ids = ['Pipeline_PartialMetadataDone',
                 'Pipeline_MetadataDone',
                 'Pipeline_RequestIdDone']

    def __init__(self, session_request, pipeline_handle):
        self.pipeline_handle     = pipeline_handle
        self.stage_id            = session_request['stageInfo']['stageId']
        self.stage_sequence_id   = session_request['stageInfo']['stageSequenceId']
        self.num_buffers         = session_request['numOutputs']
        self.received_buffers    = 0
        self.received_metadata   = False
        self.complete            = False
        self.node_requests       = {}       # Node requests from this session request
        self.delta_node_requests = {}       # Node requests from other session requests expecting a fence from this frame num
        self.req_map             = None
        self.pipeline_name = 'Unknown Pipeline'
        self.pipeline_request_id_done = False
        self.pipeline_metadata_done = False
        self.pipeline_partial_meta_done = False

    def print_summary(self, scope=None):
        pipeline_id  = '{} {}'.format(self.pipeline_name.rjust(48, ' '),self.pipeline_handle)
        print('        stageId: {} stageSequenceId: {}'.format(self.stage_id, self.stage_sequence_id))
        if scope != 'CHI':
            print('        ------------------------------------------------------------------------')
            print('        Pipeline   {}'.format(pipeline_id))
            print('        ------------------------------------------------------------------------')
            if self.req_map is not None:
                chi_frame_num = str(self.req_map.chi_frame_number).center(11)
                request_id    = str(self.req_map.request_id).center(9)
                sequence_id   = str(self.req_map.sequence_id).center(10)
                csl_sync_id   = str(self.req_map.csl_sync_id).center(9)
                print('          ChiFrameNum   <=>   RequestId   <=>   SequenceId   <=>   CSLSyncId')
                print('          {}         {}         {}         {}'.format(chi_frame_num, request_id, sequence_id, csl_sync_id))
            request_id_done_status   = '  Complete' if self.pipeline_request_id_done else 'Incomplete'
            meta_done_status         = '  Complete' if self.pipeline_metadata_done else 'Incomplete'
            partial_meta_done_status = '  Complete' if self.pipeline_partial_meta_done else 'Incomplete'
            print('          RequestIdDone                                               {}'.format(request_id_done_status))
            print('          MetadataDone                                                {}'.format(meta_done_status))
            print('          PartialMetaDone                                             {}'.format(partial_meta_done_status))
            if scope != 'PIPELINE':
                print('          ----------------------------------------------------------------------')
                print('          Nodes')
                print('          ----------------------------------------------------------------------')
                print('                                  RequestIdDone | MetadataDone | PartialMetaDone')
                for node_request in self.node_requests.values():
                    node_request.print_summary()
                if scope != 'NODE':
                    print('          ----------------------------------------------------------------------')
                    print('          Fences')
                    print('          ----------------------------------------------------------------------')
                    for node_request in self.node_requests.values():
                        node_request.print_fence_table()

    def add_buffer(self, buffer_res):
        # Update buffer status
        self.received_buffers += buffer_res['numOutputBuffers']
        if self.received_buffers > self.num_buffers:
            print('Received more buffers than requested for frame {}'.format(buffer_res['chiFrameNumber']))
            self.received_buffers = self.num_buffers

    def add_metadata(self, metadata_res):
        # Update metadata status
        self.received_metadata = True

    def update_status(self):
        # Update completion status
        if (self.received_buffers == self.num_buffers) and self.received_metadata:
            self.complete = True

    def process_result(self, process_result):
        # Update session request status with with incoming session results
        if process_result['hasBuffers']:
            self.add_buffer(process_result)
        if process_result['hasMetadata']:
            self.add_metadata(process_result)
        self.update_status()

    def add_node_request(self, event_id, node_port_request):
        # Since we don't have PipelineRequest class, we'll handle node requests in the SessionRequest
        node_id          = node_port_request['nodeId']
        node_instance_id = node_port_request['nodeInstanceId']
        node_request_id  = '{}_{}'.format(node_id, node_instance_id)
        if node_request_id not in self.node_requests:
            self.node_requests[node_request_id] = NodeRequest(node_id, node_instance_id)

    def add_delta_node_request(self, node_request):
        # Delta node requests are used when node from a future request depends on an output from this request
        node_request_id = '{}_{}'.format(node_request.node_id, node_request.node_instance_id)
        self.delta_node_requests[node_request_id] = node_request

    def add_req_map(self, req_map):
        raise Exception('Is anybody calling this anymore')
        self.req_map = req_map

    def handle_event(self, event_id, log):
        # Handle pipeline status events (since we don't have a PipelineRequest class)
        if event_id == 'Pipeline_PartialMetadataDone':
            self.pipeline_partial_meta_done = True
        elif event_id == 'Pipeline_MetadataDone':
            self.pipeline_metadata_done = True
        elif event_id == 'Pipeline_RequestIdDone':
            self.pipeline_request_id_done = True