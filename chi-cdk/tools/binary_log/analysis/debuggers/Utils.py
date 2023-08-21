#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################


def get_app_frame_num(event_id, log, app_frame_delta=0):
    # app_frame_delta is used when HAL request number and the CHI app frame number diverge
    frame_number = None
    if event_id == 'HAL3_ProcessCaptureRequest':
        frame_number = log['rCaptureRequest']['frame_number']
        frame_number -= app_frame_delta
    elif event_id == 'HAL3_BufferInfo':
        frame_number = log['frame_number']
        frame_number -= app_frame_delta
    elif event_id == 'HAL3_ProcessCaptureResult':
        frame_number = log['rCaptureResult']['frame_number']
        frame_number -= app_frame_delta
    elif event_id == 'HAL3_Notify':
        if log['rNotifyMessage']['type'] == 2: # Use shutter frame_num
            frame_number = log['rNotifyMessage']['message']['shutter']['frame_number']
        else: # Use error frame_num
            frame_number = log['rNotifyMessage']['message']['error']['frame_number']
        frame_number -= app_frame_delta
    elif event_id == 'FT2_Base_SubmitSessionRequest':
        frame_number = log['appFrameNum']
    elif event_id == 'FT2_Base_ProcessResult':
        frame_number = log['appFrameNumber']
    elif event_id == 'FT2_URO_Init':
        frame_number = log['frameNumber']
    elif event_id == 'FT2_Graph_Init':
        frame_number = log['frameNumber']
    elif event_id == 'FT2_Graph_FeatureInit':
        frame_number = log['frameNumber']
    elif event_id == 'FT2_FRO_Init':
        frame_number = log['frameNumber']
    elif event_id == 'FT2_FRO_StateInfo':
        frame_number = log['frameNumber']

    if frame_number is None:
        raise NotImplementedError('No app frame number available for {}'.format(event_id))
    return frame_number


def get_chi_frame_num(event_id, log):
    chi_frame_num = None
    if event_id == 'FT2_Base_SubmitSessionRequest':
        return log['chiFrameNum']
    elif event_id == 'FT2_Base_ProcessResult':
        return log['chiFrameNumber']
    elif event_id == 'ReqMap_CamXInfo':
        return log['chiFrameNumber']

    if chi_frame_num is None:
        raise NotImplementedError('No chi frame number available for {}'.format(event_id))
    return chi_frame_num


def get_session_handle(event_id, log):
    session_handle = None
    if event_id == 'FT2_Base_SubmitSessionRequest':
        session_handle = log['hSession']
    elif event_id == 'FT2_Base_ProcessResult':
        session_handle = log['hSession']
    elif event_id == 'ReqMap_CamXInfo':
        session_handle = log['(literal)']

    if session_handle is None:
        raise NotImplementedError('No session handle available for {}'.format(event_id))
    return session_handle


def get_pipeline_handle(event_id, log):
    pipeline_handle = None
    if event_id == 'FT2_Base_SubmitSessionRequest':
        pipeline_handle = log['hPipeline']
    elif event_id == 'FT2_Base_ProcessResult':
        pipeline_handle = log['hPipeline']
    elif event_id == 'FenceCB_Processed':
        pipeline_handle = log['hPipeline']
    elif event_id == 'ReqMap_CamXInfo':
        pipeline_handle = log['hPipeline']
    elif event_id == 'Node_SetupRequestOutputPort':
        pipeline_handle = log['hPipeline']
    elif event_id == 'Node_SetupRequestInputPort':
        pipeline_handle = log['hPipeline']
    elif event_id == 'Pipeline_Initialize':
        pipeline_handle = log['hPipeline']
    elif event_id == 'Pipeline_PartialMetadataDone':
        pipeline_handle = log['hPipeline']
    elif event_id == 'Pipeline_MetadataDone':
        pipeline_handle = log['hPipeline']
    elif event_id == 'Pipeline_RequestIdDone':
        pipeline_handle = log['hPipeline']
    elif event_id == 'Node_PartialMetadataDone':
        pipeline_handle = log['hPipeline']
    elif event_id == 'Node_MetadataDone':
        pipeline_handle = log['hPipeline']
    elif event_id == 'Node_RequestIdDone':
        pipeline_handle = log['hPipeline']

    if pipeline_handle is None:
        raise NotImplementedError('No pipeline handle available for {}'.format(event_id))
    return pipeline_handle


def get_fro_handle(event_id, log):
    fro_handle = None
    if event_id == 'FT2_Base_SubmitSessionRequest':
        fro_handle = log['hFroHandle']
    elif event_id == 'FT2_Base_ProcessResult':
        fro_handle = log['hFroHandle']
    elif event_id == 'FT2_FRO_Init':
        fro_handle = log['hFroHandle']
    elif event_id == 'FT2_FRO_StateInfo':
        fro_handle = log['hFroHandle']

    if fro_handle is None:
        raise NotImplementedError('No FRO handle available for {}'.format(event_id))
    return fro_handle


def get_fro_request_id(event_id, log):
    fro_request_id = None
    if event_id == 'FT2_Base_SubmitSessionRequest':
        fro_request_id = log['requestId']
    elif event_id == 'FT2_Base_ProcessResult':
        fro_request_id = log['requestId']
    elif event_id == 'FT2_FRO_StateInfo':
        fro_request_id = log['requestIndex']

    if fro_request_id is None:
        raise NotImplementedError('No FRO requestId available for {}'.format(event_id))
    return fro_request_id


def get_fence_handle(event_id, log):
    fence_handle = None
    if event_id == 'FenceCB_Processed':
        fence_handle = log['rFenceHandlerData']['hFence']
    elif event_id == 'Node_SetupRequestOutputPort':
        fence_handle = log['rFenceHandlerData']['hFence']
    elif event_id == 'Node_SetupRequestInputPort':
        fence_handle = log['hFence']

    if fence_handle is None:
        raise NotImplementedError('No fence handle available for {}'.format(event_id))
    return fence_handle


def get_request_id(event_id, log):
    request_id = None
    if event_id == 'FenceCB_Processed':
        request_id = log['rFenceHandlerData']['requestId']
    elif event_id == 'Node_SetupRequestOutputPort':
        request_id = log['requestId']
    elif event_id == 'Node_SetupRequestInputPort':
        request_id = log['requestId']
    elif event_id == 'Pipeline_PartialMetadataDone':
        request_id = log['requestId']
    elif event_id == 'Pipeline_MetadataDone':
        request_id = log['requestId']
    elif event_id == 'Pipeline_RequestIdDone':
        request_id = log['requestId']
    elif event_id == 'Node_PartialMetadataDone':
        request_id = log['requestId']
    elif event_id == 'Node_MetadataDone':
        request_id = log['requestId']
    elif event_id == 'Node_RequestIdDone':
        request_id = log['requestId']

    if request_id is None:
        raise NotImplementedError('No request id available for {}'.format(event_id))
    return request_id


def get_request_id_buffer_delta(event_id, log):
    request_id_buffer_delta = None
    if event_id == 'Node_SetupRequestInputPort':
        request_id_buffer_delta = log['requestIdWithBufferDelta']

    if request_id_buffer_delta is None:
        raise NotImplementedError('No request id buffer delta available for {}'.format(event_id))
    return request_id_buffer_delta


def get_node_id(event_id, log):
    node_id = None
    if event_id == 'Node_PartialMetadataDone':
        node_id = log['nodeType']
    elif event_id == 'Node_MetadataDone':
        node_id = log['nodeId']
    elif event_id == 'Node_RequestIdDone':
        node_id = log['nodeId']

    if node_id is None:
        raise NotImplementedError('No node id available for {}'.format(event_id))
    return node_id


def node_id_to_node_name(node_id):
    node_name = None
    if node_id == 0:
        node_name = 'Sensor'
    elif node_id == 1:
        node_name = 'StatsProcessing'
    elif node_id == 2:
        node_name = 'SinkBuffer'
    elif node_id == 3:
        node_name = 'SinkNoBuffer'
    elif node_id == 4:
        node_name = 'SourceBuffer'
    elif node_id == 5:
        node_name = 'AutoFocus'
    elif node_id == 6:
        node_name = 'JPEGAggregator'
    elif node_id == 7:
        node_name = 'FDSoftware'
    elif node_id == 8:
        node_name = 'FDManager'
    elif node_id == 9:
        node_name = 'StatsParse'
    elif node_id == 10:
        node_name = 'OfflineStats'
    elif node_id == 11:
        node_name = 'Torch'
    elif node_id == 12:
        node_name = 'AutoWhiteBalance'
    elif node_id == 13:
        node_name = 'HistogramProcess'
    elif node_id == 14:
        node_name = 'Tracker'
    elif node_id == 255:
        node_name = 'ChiNode'
    elif node_id == 65536:
        node_name = 'IFE'
    elif node_id == 65537:
        node_name = 'JPEG'
    elif node_id == 65538:
        node_name = 'IPE'
    elif node_id == 65539:
        node_name = 'BPS'
    elif node_id == 65540:
        node_name = 'FDHardware'
    elif node_id == 65541:
        node_name = 'LRME'
    elif node_id == 65542:
        node_name = 'RANSAC'
    elif node_id == 65543:
        node_name = 'CVP'
    else:
        node_name = str(node_id)
    return node_name
