#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################

from HALRequest import HALRequest
from FT2Request import URO
from SessionRequest import SessionRequest
from NodeRequest import NodeRequest, InputPortRequest, OutputPortRequest
from collections import defaultdict
import json
import Utils
from collections import defaultdict


class ReqMapEntry:

    def __init__(self, log):
        self.frame_number = log['chiFrameNumber']
        self.chi_frame_number = log['chiFrameNumber']
        self.request_id = log['requestId']
        self.sequence_id = log['sequenceId']
        self.csl_sync_id = log['CSLSyncID']
        self.hSession = log['(literal)']

class ReqMap:

    def __init__(self):
        self.frame_number_map = {}
        self.chi_frame_number_map = {}
        self.request_id_map = {}

    def add_mapping(self, log):
        req_map_entry = ReqMapEntry(log)
        self.frame_number_map[req_map_entry.frame_number] = req_map_entry
        self.chi_frame_number_map[req_map_entry.chi_frame_number] = req_map_entry
        self.request_id_map[req_map_entry.request_id] = req_map_entry

class FenceCB:

    def __init__(self, fence_cb):
        self.pipeline_handle  = fence_cb['hPipeline']
        self.fence_handle     = fence_cb['rFenceHandlerData']['hFence']
        self.node_id          = fence_cb['nodeId']
        self.node_instance_id = fence_cb['nodeInstanceId']
        self.output_port      = fence_cb['outputPortId']
        self.request_id       = fence_cb['rFenceHandlerData']['requestId']
        self.is_error         = (fence_cb['rFenceHandlerData']['fenceResult'] != 0)

class Pipeline:

    def __init__(self, pipeline_handle):
        self.pipeline_handle = pipeline_handle
        self.requests = {} # session requests
        self.req_map  = ReqMap()

class Session:

    def __init__(self, session_handle):
        self.session_handle = session_handle
        self.pipeline_map = {}

class StreamInfo:

    def __init__(self, stream_info):
        stream_info = stream_info['rConfigStream']
        self.format = stream_info['format']
        self.physical_camera_id = stream_info['physical_camera_id']
        self.stream_type = stream_info['stream_type']
        self.max_buffers = stream_info['max_buffers']
        self.width = stream_info['width']
        self.height = stream_info['height']
        self.usage = stream_info['usage']
        self.rotation = stream_info['rotation']

    def print_summary(self):
        print('    Format:                {}'.format(self.format))
        print('    Width:                 {}'.format(self.width))
        print('    Height:                {}'.format(self.height))
        print('    Stream Type:           {}'.format(self.stream_type))
        print('    Usage:                 {}'.format(self.usage))
        print('    Max Buffers            {}'.format(self.max_buffers))
        print('    Rotation               {}'.format(self.rotation))
        print('    Physical Camera Id:    {}'.format(self.physical_camera_id))

class ConfigureStream:

    def __init__(self, stream_config):
        self.camera_id = stream_config['cameraId']
        self.logical_camera_id = stream_config['logicalCameraId']
        self.num_streams = stream_config['numStreams']
        self.streams = []

    def print_summary(self):
        print('  Logical Camera Id {}'.format(self.logical_camera_id))
        print('  Camera Id         {}'.format(self.camera_id))
        for stream_id in range(0, len(self.streams)):
            print('  stream[{}] - info'.format(stream_id))
            self.streams[stream_id].print_summary()

    def add_stream(self, stream_info):
        self.streams.append(StreamInfo(stream_info))

class StreamSession:

    def __init__(self, configure_stream = None):
        if configure_stream is not None:
            self.configure_stream = ConfigureStream(configure_stream)
        self.is_flushing = False
        self.session_map         = {} # Maps Camx Sessions
        self.hal_request_mapping = {} # Maps HAL requests
        self.fence_mapping       = defaultdict(dict) # Maps Fence callbacks, [pipeline_handle][fence_handle]
        self.app_frame_delta     = 0

        # Name mappings,
        self.pipeline_handle_to_pipeline_name = {}
        self.feature_id_to_feature_name       = {}

    def add_configure_stream(self, configure_stream):
        self.configure_stream = ConfigureStream(configure_stream)

    def is_modified(self):
        return len(self.hal_request_mapping) != 0

    def print_summary(self, scope=None, only_incomplete=False):
        if self.configure_stream is not None:
            print('################################################################################')
            self.configure_stream.print_summary()
            print('################################################################################')
        for hal_request in self.hal_request_mapping.values():
            if only_incomplete and hal_request.is_complete():
                    continue
            hal_request.print_summary(scope=scope)


    def get_session_request(self, pipeline_handle, request_id):
        pipeline = None
        for session in self.session_map.values():
            if pipeline_handle in session.pipeline_map:
                pipeline = session.pipeline_map[pipeline_handle]
                if request_id in pipeline.req_map.request_id_map:
                    chi_frame_num = pipeline.req_map.request_id_map[request_id].chi_frame_number
                    if chi_frame_num in pipeline.requests:
                        return pipeline.requests[chi_frame_num]
                    else:
                        pass
                        #print('No session request found for chiframenum {} in pipeline {}'.format(chi_frame_num, pipeline_handle))
                else:
                    pass
                    #print('No reqmap found for requestId {} pipeline {}'.format(request_id, pipeline_handle))
        if pipeline is None:
            pass
            #print('Could not find session containing pipeline {}'.format(pipeline_handle))
        return None

    def handle_event(self, event_id, log):

        # Handle events here if they're needed for high-level mappings
        if event_id == 'HAL3_ProcessCaptureRequest':
            frame_number = log['rCaptureRequest']['frame_number']

            if not self.is_modified() and frame_number > 0:
                self.app_frame_delta = frame_number

            self.hal_request_mapping[frame_number - self.app_frame_delta] = HALRequest(log)

        elif event_id == 'ReqMap_CamXInfo':
            session_handle = Utils.get_session_handle(event_id, log)
            pipeline_handle = Utils.get_pipeline_handle(event_id, log)
            chi_frame_num = Utils.get_chi_frame_num(event_id, log)
            if session_handle in self.session_map:
                session = self.session_map[session_handle]
                if pipeline_handle in session.pipeline_map:
                    pipeline = session.pipeline_map[pipeline_handle]
                    pipeline.req_map.add_mapping(log)
                    pipeline.requests[chi_frame_num].req_map = pipeline.req_map.chi_frame_number_map[chi_frame_num]

        elif event_id == 'HAL3_FlushInfo':
            if not log['isDone']:
                self.is_flushing = True
            else:
                self.is_flushing = False

        elif event_id == 'FT2_Base_SubmitSessionRequest':
            session_handle  = Utils.get_session_handle(event_id, log)
            pipeline_handle = Utils.get_pipeline_handle(event_id, log)
            chi_frame_num   = Utils.get_chi_frame_num(event_id, log)

            # Setup high level session
            if session_handle not in self.session_map:
                self.session_map[session_handle] = Session(session_handle)
            session = self.session_map[session_handle]

            if pipeline_handle not in session.pipeline_map:
                session.pipeline_map[pipeline_handle] = Pipeline(pipeline_handle)
            pipeline = session.pipeline_map[pipeline_handle]

            # Map session request to FRO
            session_request = SessionRequest(log, pipeline_handle)
            if pipeline_handle in self.pipeline_handle_to_pipeline_name:
                session_request.pipeline_name = self.pipeline_handle_to_pipeline_name[pipeline_handle]
            pipeline.requests[chi_frame_num] = session_request
            frame_number = Utils.get_app_frame_num(event_id, log, self.app_frame_delta)
            if frame_number in self.hal_request_mapping:
                hal_request = self.hal_request_mapping[frame_number]
                if hal_request.usecase_request_obj is not None:
                    fro_handle = Utils.get_fro_handle(event_id, log)
                    if fro_handle in hal_request.usecase_request_obj.fro_map:
                        fro = hal_request.usecase_request_obj.fro_map[fro_handle]
                        if fro.feature_id in self.feature_id_to_feature_name:
                            fro.feature_name = self.feature_id_to_feature_name[fro.feature_id]
                        request_id = Utils.get_fro_request_id(event_id, log)
                        session_req_id = '{}_{}'.format(pipeline_handle, chi_frame_num)
                        fro_request = fro.requests[request_id]
                        fro_request.submit_session_request(log)
                        fro_request.session_requests[session_req_id] = session_request

        elif event_id == 'FT2_Base_ProcessResult':
            app_frame_num   = Utils.get_app_frame_num(event_id, log, self.app_frame_delta)
            chi_frame_num   = Utils.get_chi_frame_num(event_id, log)
            pipeline_handle = Utils.get_pipeline_handle(event_id, log)
            fro_handle      = Utils.get_fro_handle(event_id, log)
            request_id      = Utils.get_fro_request_id(event_id, log)
            session_req_id  = '{}_{}'.format(pipeline_handle, chi_frame_num)
            hal_request     = self.hal_request_mapping[app_frame_num]
            fro             = hal_request.usecase_request_obj.fro_map[fro_handle]
            fro.requests[request_id].update_session_request(log, session_req_id)

        elif event_id == 'FenceCB_Processed':
            fence_handle = Utils.get_fence_handle(event_id, log)
            if fence_handle != 0:
                pipeline_handle = Utils.get_pipeline_handle(event_id, log)
                request_id      = Utils.get_request_id(event_id, log)
                self.fence_mapping[pipeline_handle][fence_handle] = FenceCB(log)
                session_request = self.get_session_request(pipeline_handle, request_id)
                is_error = (log['rFenceHandlerData']['fenceResult'] != 0)
                if session_request is not None:
                    for node_request in session_request.node_requests.values():
                        node_request.signal_fence(fence_handle, is_error)
                    for node_request in session_request.delta_node_requests.values():
                        node_request.signal_fence(fence_handle, is_error)
                else:
                    pass
                    #print('Could not find session request from pipeline {} request_id {}'.format(pipeline_handle, request_id))

        elif event_id == 'Node_SetupRequestOutputPort':
            pipeline_handle = Utils.get_pipeline_handle(event_id, log)
            request_id      = Utils.get_request_id(event_id, log)
            session_request = self.get_session_request(pipeline_handle, request_id)
            if session_request is not None:
                node_request_id = '{}_{}'.format(log['nodeId'], log['nodeInstanceId'])
                if node_request_id not in session_request.node_requests:
                    session_request.add_node_request(event_id, log)
                node_request = session_request.node_requests[node_request_id]
                node_request.output_port_request(OutputPortRequest(log))
            else:
                pass
                #print('Could not find session request from pipeline {} request_id {}'.format(pipeline_handle, request_id))

        elif event_id == 'Node_SetupRequestInputPort':
            pipeline_handle = Utils.get_pipeline_handle(event_id, log)
            request_id = Utils.get_request_id(event_id, log)
            request_id_buffer_delta = Utils.get_request_id_buffer_delta(event_id, log)
            session_request = self.get_session_request(pipeline_handle, request_id)
            fence_handle = Utils.get_fence_handle(event_id, log)
            if not log['isSourceBufferInputPort'] and fence_handle != 0:
                if session_request is not None:
                    node_request_id = '{}_{}'.format(log['nodeId'], log['nodeInstanceId'])
                    if node_request_id not in session_request.node_requests:
                        session_request.add_node_request(event_id, log)
                    node_request = session_request.node_requests[node_request_id]
                    node_request.input_port_request(InputPortRequest(log))
                    if log['requestIdWithBufferDelta'] != request_id:
                        previous_session = self.get_session_request(pipeline_handle, request_id_buffer_delta)
                        if previous_session is not None:
                            previous_session.add_delta_node_request(node_request)

                    # Case: FenceCB occurs before setup input port request
                    if pipeline_handle in self.fence_mapping and fence_handle in self.fence_mapping[pipeline_handle] and \
                        self.fence_mapping[pipeline_handle][fence_handle].request_id == log['requestIdWithBufferDelta']:
                            node_request.signal_fence(fence_handle, self.fence_mapping[pipeline_handle][fence_handle].is_error)
                else:
                    pass
                    #print('Could not find session request from pipeline {} request_id {}'.format(pipeline_handle, request_id))

        elif event_id == 'Pipeline_Initialize':
            pipeline_handle = Utils.get_pipeline_handle(event_id, log)
            self.pipeline_handle_to_pipeline_name[pipeline_handle] = log['rPipelineName']

        elif event_id == 'FT2_Pool_CreateInstance':
            feature_id = log['featureId']
            self.feature_id_to_feature_name[feature_id] = log['rFeatureName']

        if event_id in HALRequest.event_ids:
            frame_number = Utils.get_app_frame_num(event_id, log, self.app_frame_delta)
            if frame_number in self.hal_request_mapping:
                hal_request = self.hal_request_mapping[frame_number]
                hal_request.handle_event(event_id, log, self.is_flushing)
            else:
                pass
                #print('Skipping {}, Could not find hal request for frame_number {}'.format(event_id, frame_number))

        elif event_id == 'FT2_URO_Init':
            frame_number = Utils.get_app_frame_num(event_id, log, self.app_frame_delta)
            if frame_number in self.hal_request_mapping:
                self.hal_request_mapping[frame_number].usecase_request_obj = URO(log)
            else:
                pass
                #print('Skipping {}, Could not find hal request for frame_number {}'.format(event_id, frame_number))

        elif event_id in URO.event_ids:
            frame_number = Utils.get_app_frame_num(event_id, log, self.app_frame_delta)
            if frame_number in self.hal_request_mapping:
                hal_request = self.hal_request_mapping[frame_number]
                hal_request.usecase_request_obj.handle_event(event_id, log)
            else:
                if event_id == 'FT2_FRO_StateInfo' and log['requestState'] == 9:
                    pass
                else:
                    pass
                    #print('Skipping {}, Could not find hal request for frame_number {}'.format(event_id, frame_number))

        elif event_id in SessionRequest.event_ids:
            pipeline_handle = Utils.get_pipeline_handle(event_id, log)
            request_id = Utils.get_request_id(event_id, log)
            session_request = self.get_session_request(pipeline_handle, request_id)
            if session_request is not None:
                session_request.handle_event(event_id, log)
            else:
                pass
                #print('Pipeline - Could not find session from pipeline {} request_id {}'.format(pipeline_handle, request_id))

        elif event_id in NodeRequest.event_ids:
            pipeline_handle = Utils.get_pipeline_handle(event_id, log)
            request_id = Utils.get_request_id(event_id, log)
            session_request = self.get_session_request(pipeline_handle, request_id)
            if session_request is not None:
                node_id = '{}_{}'.format(Utils.get_node_id(event_id, log), log['nodeInstanceId'])
                if node_id in session_request.node_requests:
                    session_request.node_requests[node_id].handle_event(event_id, log)
                else:
                    pass
                    #print('Node - Could not find node id {} instance id {} in session request request_id {}'.format(Utils.get_node_id(event_id, log), log['nodeInstanceId'], request_id))
            else:
                pass
                #print('Node - Could not find session from pipeline {} request_id {}'.format(pipeline_handle, request_id))

        elif event_id == 'HAL3_StreamInfo':
            if self.configure_stream is not None:
                self.configure_stream.add_stream(log)

