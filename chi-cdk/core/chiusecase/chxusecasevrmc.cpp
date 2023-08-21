////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxusecasevrmc.cpp
/// @brief Usecases class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
#include "chxusecase.h"
#include "chxusecasevrmc.h"
#include "chistatspropertydefines.h"
#include "chxutils.h"
#include "chxmulticamcontroller.h"
#include "chxsensorselectmode.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::~UsecaseMultiVRCamera
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UsecaseMultiVRCamera::~UsecaseMultiVRCamera()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UsecaseMultiVRCamera* UsecaseMultiVRCamera::Create(
    LogicalCameraInfo*              cameraInfo,    ///< Camera info
    camera3_stream_configuration_t* pStreamConfig) ///< Stream configuration
{
    CDKResult             result                = CDKResultSuccess;
    UsecaseMultiVRCamera* pUsecaseMultiVRCamera = CHX_NEW UsecaseMultiVRCamera;

    if (NULL != pUsecaseMultiVRCamera)
    {
        result = pUsecaseMultiVRCamera->Initialize(cameraInfo, pStreamConfig);

        if (CDKResultSuccess != result)
        {
            pUsecaseMultiVRCamera->Destroy(FALSE);
            pUsecaseMultiVRCamera = NULL;
        }
    }
    else
    {
        result = CDKResultEFailed;
    }

    return pUsecaseMultiVRCamera;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UsecaseMultiVRCamera::RemapPipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 UsecaseMultiVRCamera::RemapPipelineIndex(
    UINT32              pipelineIndexFromUsecase,
    DualCameraUsecase   usecase)
{
    UINT32 pipelineIndex=0;

    if (usecase == UseCaseVR)
    {
        switch (pipelineIndexFromUsecase)
        {
            case RealtimeVRPreview0:
                pipelineIndex = REALTIMEPIPELINEMAIN;
                break;
            case RealtimeVRPreview1:
                pipelineIndex = REALTIMEPIPELINEAUX;
                break;
            case VROfflinePreview:
                pipelineIndex = OFFLINEPREVIEWPIPELINE;
                break;
            case VRSnapshotYUV0:
                pipelineIndex = OFFLINEYUVPIPELINE;
                break;
            case VRSnapshotYUV1:
                pipelineIndex = OFFLINEYUVPIPELINEAUX;
                break;
            case VROfflineSnapshot:
                pipelineIndex = OFFLINEJPEGPIPELINE;
                break;
            case VROfflineRaw:
                pipelineIndex = OFFLINERAWPIPELINE;
                break;
            default:
                CHX_LOG_ERROR("ERROR: Pipeline index is more than expected");
        }
    }
    else if (usecase == UseCaseVRVideo)
    {
         switch (pipelineIndexFromUsecase)
         {
              case RealtimeVideo0:
                  pipelineIndex = REALTIMEPIPELINEMAIN;
                  break;
              case RealtimeVideo1:
                  pipelineIndex = REALTIMEPIPELINEAUX;
                  break;
              case VRfflineVideo:
                  pipelineIndex = OFFLINEPREVIEWPIPELINE;
                  break;
              case VROfflineVamVideo:
                  pipelineIndex = OFFLINEVAMPIPELINE;
                  break;
              default:
                  CHX_LOG_ERROR("ERROR: Pipeline index is more than expected");
         }

    }
    else
    {
        CHX_LOG_ERROR("ERROR: Cannot come here undefined usecase");
    }
    return pipelineIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseMultiVRCamera::Destroy(BOOL isForced)
{
    for (UINT i = 0 ; i < MaxSessions; i++)
    {
        if (NULL != m_pSession[i])
        {
            m_pSession[i]->Destroy(isForced);
            m_pSession[i] = NULL;
        }
    }

    for (UINT i = 0 ; i < MaxPipelinesPerSession; i++)
    {
        if (NULL != m_pPipeline[i])
        {
            m_pPipeline[i]->Destroy();
            m_pPipeline[i] = NULL;
        }
    }

    for (UINT i = 0 ; i < MAX_REALTIME_PIPELINE ; i++)
    {
        if (NULL != m_pRTBPreviewStream[i])
        {
            ChxUtils::Free(m_pRTBPreviewStream[i]);
            m_pRTBPreviewStream[i] = NULL;
        }
        if (NULL != m_pRTBVamStream[i])
        {
            ChxUtils::Free(m_pRTBVamStream[i]);
            m_pRTBVamStream[i] = NULL;
        }
        if (NULL != m_pRTPreviewStream[i])
        {
            ChxUtils::Free(m_pRTPreviewStream[i]);
            m_pRTPreviewStream[i] = NULL;
        }
        // Check if all is NULL or not
        if (NULL != m_pRTVamStream[i])
        {
            ChxUtils::Free(m_pRTVamStream[i]);
            m_pRTVamStream[i] = NULL;
        }
        if (NULL != m_pPreviewBufferManager[i])
        {
            m_pPreviewBufferManager[i]->Destroy();
            m_pPreviewBufferManager[i] = NULL;
        }
        if (NULL != m_pRDIBufferManager[i])
        {
            m_pRDIBufferManager[i]->Destroy();
            m_pRDIBufferManager[i] = NULL;
        }
        if (NULL != m_pRTBSnapshotStream[i])
        {
            ChxUtils::Free(m_pRTBSnapshotStream[i]);
            m_pRTBSnapshotStream[i] = NULL;
        }
        if (NULL != m_pRTYUVStream[i])
        {
            ChxUtils::Free(m_pRTYUVStream[i]);
            m_pRTYUVStream[i] = NULL;
        }
        if (NULL != m_pRTRDIStream[i])
        {
            ChxUtils::Free(m_pRTRDIStream[i]);
            m_pRTRDIStream[i] = NULL;
        }
        if (NULL != m_pRTRDISnapshotStream[i])
        {
            ChxUtils::Free(m_pRTRDISnapshotStream[i]);
            m_pRTRDISnapshotStream[i] = NULL;
        }
        if (NULL != m_pJPEGStream[i])
        {
            ChxUtils::Free(m_pJPEGStream[i]);
            m_pJPEGStream[i] = NULL;
        }
        if (NULL != m_pVamBufferManager[i])
        {
            m_pVamBufferManager[i]->Destroy();
            m_pVamBufferManager[i] = NULL;
        }
        if (NULL != m_pYUVBufferManager[i])
        {
            m_pYUVBufferManager[i]->Destroy();
            m_pYUVBufferManager[i] = NULL;
        }
    }

    // Free the input metadata for the offline pipeline
    if (NULL != m_pOfflinePipelineInputMetadata)
    {
        free_camera_metadata(m_pOfflinePipelineInputMetadata);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::processRDIJpegFrame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseMultiVRCamera::processRDIJpegFrame(
    const ChiCaptureResult* pResult,
    CHISTREAMBUFFER*        RDIBuffer)
{
    CHISTREAMBUFFER     outputBuffers[2];
    CHICAPTUREREQUEST   request             = {0};
    UINT32              applicationFrameNum = 0;
    UINT32              numOutBuffer        = 0;
    UINT32              pipelineIndex;
    UINT32              sessionIndex;

    pipelineIndex = pResult->pPrivData->streamIndex;

    numOutBuffer = 1;
    if (pipelineIndex == REALTIMEPIPELINEMAIN)
    {
        if (TRUE == m_isPostViewNeeded)
        {
            numOutBuffer = 2;
        }
        sessionIndex = OFFLINERDISESSION;
    }
    else
    {
        sessionIndex = OFFLINERDISESSIONAUX;
    }

    applicationFrameNum              = pResult->frameworkFrameNum;
    request.frameNumber              = pResult->frameworkFrameNum;
    request.hPipelineHandle          = reinterpret_cast<CHIPIPELINEHANDLE>(
        m_pSession[sessionIndex]->GetPipelineHandle(0));
    request.numInputs                = 1;
    request.pInputBuffers            = RDIBuffer;
    request.pInputBuffers[0].pStream = m_pRTBSnapshotStream[pipelineIndex];
    request.numOutputs               = numOutBuffer;

    outputBuffers[0].size               = sizeof(CHISTREAMBUFFER);
    outputBuffers[0].acquireFence.valid = FALSE;
    outputBuffers[0].bufferInfo         = m_pYUVBufferManager[pipelineIndex]->GetImageBufferInfo();
    outputBuffers[0].pStream            = m_pRTYUVStream[pipelineIndex];

    if ((REALTIMEPIPELINEMAIN == pipelineIndex) && (TRUE == m_isPostViewNeeded))
    {
        ChxUtils::PopulateHALToChiStreamBuffer(&m_appPostviewBuffer[applicationFrameNum % MaxOutstandingRequests],
                                               &outputBuffers[1]);
    }

    request.pOutputBuffers = outputBuffers;

    for (UINT i = 0; i < request.numOutputs; i++)
    {
        CHX_LOG("[VRMC RDI] Request : frameNum :%" PRIu64
                " acquireFence: (valid=%d type=%d nativeFence=%d chiFence=%p) , "
                "ReleaseFence: (valid=%d type=%d nativeFence=%d chiFence=%p) Buffer: type %d handle %p, status: %d",
                request.frameNumber,
                request.pOutputBuffers[i].acquireFence.valid,
                request.pOutputBuffers[i].acquireFence.type,
                request.pOutputBuffers[i].acquireFence.nativeFenceFD,
                request.pOutputBuffers[i].acquireFence.hChiFence,
                request.pOutputBuffers[i].releaseFence.valid,
                request.pOutputBuffers[i].releaseFence.type,
                request.pOutputBuffers[i].releaseFence.nativeFenceFD,
                request.pOutputBuffers[i].releaseFence.hChiFence,
                request.pOutputBuffers[i].bufferInfo.bufferType,
                request.pOutputBuffers[i].bufferInfo.phBuffer,
                request.pOutputBuffers[i].bufferStatus);
    }

    request.pMetadata = rtBuffer[applicationFrameNum % MaxOutstandingRequests].pMetadata[pipelineIndex];
    if (pipelineIndex == REALTIMEPIPELINEMAIN)
    {
        request.pPrivData = &privRDIOffline1[pResult->frameworkFrameNum % MaxOutstandingRequests];
    }
    else
    {
        request.pPrivData = &privRDIOffline2[pResult->frameworkFrameNum % MaxOutstandingRequests];
    }
    request.pPrivData->streamIndex         = pipelineIndex;

    // Save input RDI buffer info, the reference of it will be released once yuv result is received in OfflineSnapshotReulst.
    request.pPrivData->numInputBuffers      = 1;
    request.pPrivData->inputBuffers[0]      = RDIBuffer->bufferInfo;
    request.pPrivData->bufferManagers[0]    = m_pRDIBufferManager[pipelineIndex];

    CHX_LOG("%s: [VRMC RDI] pipelineindex:%d, sessionIndex %d, pMetadata %p",
            __func__, pipelineIndex, sessionIndex, request.pMetadata);

    GenerateInternalRequest(sessionIndex, 1, &request);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::processRDIRawsnapshotFrame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseMultiVRCamera::processRDIRawSnapshotFrame(
    const ChiCaptureResult* pResult,
    CHISTREAMBUFFER*        RDIBuffer)
{
    CHICAPTUREREQUEST   request                 = {0};
    UINT32              pipelineIndex           = 0;
    UINT32              applicationFrameNum     = pResult->frameworkFrameNum;
    UINT32              applicationFrameIndex   = applicationFrameNum %  MaxOutstandingRequests;
    static UINT         frameCount              = 0;

    pipelineIndex = pResult->pPrivData->streamIndex;

    // Only take metadata from Main
    if (REALTIMEPIPELINEMAIN == pipelineIndex)
    {
        if (NULL != pResult->pResultMetadata)
        {
            rtRawSnapshotBuffer[applicationFrameIndex].pMetadata[0]           =
                static_cast<VOID*>(clone_camera_metadata((camera_metadata_t *)pResult->pResultMetadata));
            rtRawSnapshotBuffer[applicationFrameIndex].valid[pipelineIndex]  |= META_READY_FLAG;
            CHX_LOG("Offline Raw SnapshotResult pipeline idx:%d meta is ready,value:%x",
                pipelineIndex, rtRawSnapshotBuffer[applicationFrameIndex].valid[pipelineIndex]);
        }
    }

    ///< Receive Realtime pipeline result
    if ((REALTIMEPIPELINEMAIN == pipelineIndex) && (NULL != RDIBuffer))
    {
        rtRawSnapshotBuffer[applicationFrameIndex].frameNumber = applicationFrameNum;
        if (pResult->numOutputBuffers > 0)
        {
            ChxUtils::Memcpy(&rtRawSnapshotBuffer[applicationFrameIndex].buffer[pipelineIndex],
                 const_cast<CHISTREAMBUFFER*>(RDIBuffer),
                 sizeof(CHISTREAMBUFFER));
            rtRawSnapshotBuffer[applicationFrameIndex].buffer[pipelineIndex].pStream = m_pRTBRDISnapshotStream[REALTIMEPIPELINEMAIN];
            rtRawSnapshotBuffer[applicationFrameIndex].valid[pipelineIndex]         |= BUF_READY_FLAG;;
            CHX_LOG("Offline Raw SnapshotResult pipeline idx:%d buffer is ready,stream:%p,valid:%x", pipelineIndex,
                rtRawSnapshotBuffer[applicationFrameIndex].buffer[pipelineIndex].pStream,
                rtRawSnapshotBuffer[applicationFrameIndex].valid[pipelineIndex]);
        }
        frameCount++;
    }
    else if ((REALTIMEPIPELINEAUX == pipelineIndex) && (NULL != RDIBuffer))
    {
        rtRawSnapshotBuffer[applicationFrameIndex].frameNumber = applicationFrameNum;

        if (pResult->numOutputBuffers > 0)
        {
            ChxUtils::Memcpy(&rtRawSnapshotBuffer[applicationFrameIndex].buffer[pipelineIndex],
                 const_cast<CHISTREAMBUFFER*>(RDIBuffer),
                 sizeof(CHISTREAMBUFFER));
            rtRawSnapshotBuffer[applicationFrameIndex].buffer[pipelineIndex].pStream = m_pRTBRDISnapshotStream[REALTIMEPIPELINEAUX];
            rtRawSnapshotBuffer[applicationFrameIndex].valid[pipelineIndex]         |= BUF_READY_FLAG;
            CHX_LOG("Offline Raw SnapshotResult pipeline idx:%d buffer is ready,stream:%p, valid:%d", pipelineIndex,
                rtRawSnapshotBuffer[applicationFrameIndex].buffer[pipelineIndex].pStream,
                rtRawSnapshotBuffer[applicationFrameIndex].valid[pipelineIndex]);
        }
    }

    ///< if two result are ready, send offline request
    if ((rtRawSnapshotBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEMAIN] & META_READY_FLAG)  &&
            (rtRawSnapshotBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEMAIN] & BUF_READY_FLAG) &&
            (rtRawSnapshotBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEAUX] & BUF_READY_FLAG))
    {
        CHISTREAMBUFFER outputBuffer = { };

        ChxUtils::PopulateHALToChiStreamBuffer(&m_appRawSnapshotBuffer[applicationFrameIndex], &outputBuffer);

        // trigger reprocessing
        request.frameNumber                    = rtRawSnapshotBuffer[applicationFrameIndex].frameNumber;
        request.hPipelineHandle                = reinterpret_cast<CHIPIPELINEHANDLE>(
                                                   m_pSession[OFFLINERAWSESSION]->GetPipelineHandle(0));
        request.numInputs                      = 2;
        request.numOutputs                     = 1;
        request.pInputBuffers                  = rtRawSnapshotBuffer[applicationFrameIndex].buffer;
        request.pOutputBuffers                 = &outputBuffer;
        request.pMetadata                      = rtRawSnapshotBuffer[applicationFrameIndex].pMetadata[0];
        request.pPrivData                      = &rawSnapOffline[applicationFrameIndex];
        request.pPrivData->streamIndex         = OFFLINERAWPIPELINE;

        // Save input RDI Raw buffers info, reference will be released once snapshot result is received OfflineRTBRawResult
        request.pPrivData->numInputBuffers   = 2;
        request.pPrivData->bufferManagers[0] = m_pRDISnapshotBufferManager[REALTIMEPIPELINEMAIN];
        request.pPrivData->inputBuffers[0]   =
            rtRawSnapshotBuffer[applicationFrameIndex].buffer[REALTIMEPIPELINEMAIN].bufferInfo;
        request.pPrivData->bufferManagers[1] = m_pRDISnapshotBufferManager[REALTIMEPIPELINEAUX];
        request.pPrivData->inputBuffers[1]   =
            rtRawSnapshotBuffer[applicationFrameIndex].buffer[REALTIMEPIPELINEAUX].bufferInfo;

        CHX_LOG("Offline Raw SnapshotResult send capture fusion request!");
        GenerateInternalRequest(OFFLINERAWSESSION, 1, &request);

        rtRawSnapshotBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEMAIN] = 0;
        rtRawSnapshotBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEAUX]  = 0;
        frameCount = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::processRDIFrame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseMultiVRCamera::processRDIFrame(
    const ChiCaptureResult* pResult)
{
    BOOL                hasRDIJpegStream    = FALSE;
    BOOL                hasRDIRawStream     = FALSE;
    CHISTREAMBUFFER*    RDIBuffer           = NULL;

    if (NULL == pResult->pPrivData)
    {
        CHX_LOG_ERROR("Result: Cannot Accept NULL private data here for = %d", pResult->frameworkFrameNum);
        // Early return
        return;
    }

    for (UINT32 i = 0; i < pResult->numOutputBuffers; i++)
    {
        hasRDIJpegStream = FALSE;
        hasRDIRawStream  = FALSE;
        RDIBuffer        = NULL;
        if (IsRdiStream(pResult->pOutputBuffers[i].pStream))
        {
            hasRDIJpegStream = TRUE;
            RDIBuffer        = const_cast<CHISTREAMBUFFER*>(&pResult->pOutputBuffers[i]);
        }
        else if (IsRdiRawStream(pResult->pOutputBuffers[i].pStream))
        {
            hasRDIRawStream = TRUE;
            RDIBuffer       = const_cast<CHISTREAMBUFFER*>(&pResult->pOutputBuffers[i]);
        }
        CHX_LOG("hasRDIJpegStream = %d hasRDIRawStream = %d", hasRDIJpegStream, hasRDIRawStream);
        if (NULL != RDIBuffer)
        {
            if (TRUE == hasRDIJpegStream)
            {
                processRDIJpegFrame(pResult, RDIBuffer);
            }
            else if (TRUE == hasRDIRawStream)
            {
                processRDIRawSnapshotFrame(pResult, RDIBuffer);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::RealtimeCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseMultiVRCamera::RealtimeCaptureResult(
    const ChiCaptureResult* pResult)
{
    UINT32 pipelineIndex;

    if (NULL == pResult->pPrivData)
    {
        CHX_LOG_ERROR("Result: Cannot Accept NULL private data here for = %d", pResult->frameworkFrameNum);
        // Early return
        return;
    }
    else
    {
        pipelineIndex = pResult->pPrivData->streamIndex;
    }

    /// Real time data. Reprocess
    if ((pipelineIndex == REALTIMEPIPELINEMAIN) && (NULL != pResult->pResultMetadata))
    {
        camera3_capture_result_t result;
        result.frame_number       = pResult->frameworkFrameNum;
        result.result             = const_cast<camera_metadata_t *>(static_cast<const camera_metadata_t *>(pResult->pResultMetadata));
        result.num_output_buffers = 0;
        result.output_buffers     = const_cast<camera3_stream_buffer_t *>(static_cast<camera3_stream_buffer_t *>(NULL));
        result.input_buffer       = const_cast<camera3_stream_buffer_t *>(static_cast<camera3_stream_buffer_t *>(NULL));
        result.partial_result     = 1;
        Usecase::ReturnFrameworkResult(reinterpret_cast<const camera3_capture_result_t*>(&result), m_cameraId);
    }

    // process meta and realtime preview buffer
    if (pResult->numOutputBuffers != 0)
    {
        for (UINT i = 0; i < pResult->numOutputBuffers; i++)
        {
            NotifyResultsAvailable(pResult, i);
        }
    }
    else
    {
        NotifyResultsAvailable(pResult, -1);
    }
    if (pResult->numOutputBuffers != 0)
    {
        // process RDI stream buffer
        processRDIFrame(pResult);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::OfflineSnapshotResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseMultiVRCamera::OfflineSnapshotResult(
    const ChiCaptureResult* pResult)
{
    UINT32 pipelineIndex;
    static UINT frameCount           = 0;
    const CHISTREAMBUFFER* YUVBuffer = NULL;

    if (NULL == pResult->pPrivData)
    {
        CHX_LOG_ERROR("Result: Cannot Accept NULL private data here for = %d", pResult->frameworkFrameNum);
        // Early return
        return;
    }
    else
    {
        pipelineIndex = pResult->pPrivData->streamIndex;
    }

    // Release reference to the input RDI buffers
    if (0 != pResult->numOutputBuffers)
    {
        ReleaseReferenceToInputBuffers(pResult->pPrivData);
    }

    for (UINT i = 0; i < pResult->numOutputBuffers; i++)
    {
        if ((TRUE                               == m_isPostViewNeeded) &&
            (pResult->pOutputBuffers[i].pStream == m_pTargetPostviewStream))
        {
            camera3_capture_result_t result;
            camera3_stream_buffer_t  outputBuffer = { };

            ///< @todo (CAMX-4114) Do not assume incoming CHI buffer is always Native Handle
            ChxUtils::PopulateChiToHALStreamBuffer(&pResult->pOutputBuffers[i], &outputBuffer);

            result.frame_number       = pResult->frameworkFrameNum;
            result.result             = NULL;
            result.num_output_buffers = 1;
            result.output_buffers     = &outputBuffer;
            result.input_buffer       = NULL;
            result.partial_result     = 0;
            Usecase::ReturnFrameworkResult(reinterpret_cast<const camera3_capture_result_t*>(&result), m_cameraId);
            CHX_LOG("Thumbnail Callback: Sent to application i =%d", i);
        }
        else
        {
           YUVBuffer = const_cast <CHISTREAMBUFFER *>((CHISTREAMBUFFER *)&pResult->pOutputBuffers[i]);
        }
    }

    UINT32 applicationFrameNum    = pResult->frameworkFrameNum;
    UINT32 applicationFrameIndex  = applicationFrameNum %  MaxOutstandingRequests;

    // Only take metadata from Main
    if (REALTIMEPIPELINEMAIN == pipelineIndex)
    {
        if (NULL != pResult->pResultMetadata)
        {
            rtSnapshotBuffer[applicationFrameIndex].pMetadata[0]           =
                static_cast<VOID*>(clone_camera_metadata((camera_metadata_t *)pResult->pResultMetadata));
            rtSnapshotBuffer[applicationFrameIndex].valid[pipelineIndex]  |= META_READY_FLAG;
            CHX_LOG("OfflineSnapshotResult pipeline idx:%d meta is ready,value:%x",
                pipelineIndex, rtSnapshotBuffer[applicationFrameIndex].valid[pipelineIndex]);
        }
    }

    ///< Receive Realtime pipeline result
    if ((REALTIMEPIPELINEMAIN == pipelineIndex) && (NULL != YUVBuffer))
    {
        rtSnapshotBuffer[applicationFrameIndex].frameNumber = applicationFrameNum;

        if (pResult->numOutputBuffers > 0)
        {
            ChxUtils::Memcpy(&rtSnapshotBuffer[applicationFrameIndex].buffer[pipelineIndex],
                 const_cast<CHISTREAMBUFFER*>(YUVBuffer),
                 sizeof(CHISTREAMBUFFER));
            rtSnapshotBuffer[applicationFrameIndex].buffer[pipelineIndex].pStream = m_pJPEGStream[REALTIMEPIPELINEMAIN];
            rtSnapshotBuffer[applicationFrameIndex].valid[pipelineIndex]         |= BUF_READY_FLAG;;
            CHX_LOG("OfflineSnapshotResult pipeline idx:%d buffer is ready,stream:%p,valid:%x", pipelineIndex,
                rtSnapshotBuffer[applicationFrameIndex].buffer[pipelineIndex].pStream,
                rtSnapshotBuffer[applicationFrameIndex].valid[pipelineIndex]);
        }
        frameCount++;
    }
    else if ((REALTIMEPIPELINEAUX == pipelineIndex) && (NULL != YUVBuffer))
    {
        rtSnapshotBuffer[applicationFrameIndex].frameNumber = applicationFrameNum;

        if (pResult->numOutputBuffers > 0)
        {
            ChxUtils::Memcpy(&rtSnapshotBuffer[applicationFrameIndex].buffer[pipelineIndex],
                 const_cast<CHISTREAMBUFFER*>(YUVBuffer),
                 sizeof(CHISTREAMBUFFER));
            rtSnapshotBuffer[applicationFrameIndex].buffer[pipelineIndex].pStream = m_pJPEGStream[REALTIMEPIPELINEAUX];
            rtSnapshotBuffer[applicationFrameIndex].valid[pipelineIndex]         |= BUF_READY_FLAG;
            CHX_LOG("OfflineSnapshotResult pipeline idx:%d buffer is ready,stream:%p, valid:%d", pipelineIndex,
                rtSnapshotBuffer[applicationFrameIndex].buffer[pipelineIndex].pStream,
                rtSnapshotBuffer[applicationFrameIndex].valid[pipelineIndex]);
        }
    }

    ///< if two result are ready, send offline request
    if ((rtSnapshotBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEMAIN] & META_READY_FLAG)  &&
            (rtSnapshotBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEMAIN] & BUF_READY_FLAG) &&
            (rtSnapshotBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEAUX] & BUF_READY_FLAG))
    {
        // trigger reprocessing
        CHICAPTUREREQUEST request       = { 0 };
        CHISTREAMBUFFER   ouputBuffer   = { 0 };

        ChxUtils::PopulateHALToChiStreamBuffer(&m_appSnapshotBuffer[applicationFrameIndex], &ouputBuffer);

        request.frameNumber             = rtSnapshotBuffer[applicationFrameIndex].frameNumber;
        request.hPipelineHandle         = reinterpret_cast<CHIPIPELINEHANDLE>(
                                          m_pSession[OFFLINERTBJPEGSESSION]->GetPipelineHandle(0));
        request.numInputs               = 2;
        request.numOutputs              = 1;
        request.pInputBuffers           = rtSnapshotBuffer[applicationFrameIndex].buffer;
        request.pOutputBuffers          = &ouputBuffer;

        request.pMetadata               = rtSnapshotBuffer[applicationFrameIndex].pMetadata[0];
        request.pPrivData               = &privSnapshotOffline[applicationFrameIndex];
        request.pPrivData->streamIndex  = OFFLINEJPEGPIPELINE;

        // Save input YUV buffers info, the reference will be released once snapshot result is received OfflineRTBJPEGResult
        request.pPrivData->numInputBuffers   = 2;
        request.pPrivData->bufferManagers[0] = m_pYUVBufferManager[REALTIMEPIPELINEMAIN];
        request.pPrivData->inputBuffers[0]   = rtSnapshotBuffer[applicationFrameIndex].buffer[REALTIMEPIPELINEMAIN].bufferInfo;
        request.pPrivData->bufferManagers[1] = m_pYUVBufferManager[REALTIMEPIPELINEAUX];
        request.pPrivData->inputBuffers[1]   = rtSnapshotBuffer[applicationFrameIndex].buffer[REALTIMEPIPELINEAUX].bufferInfo;

        CHX_LOG("OfflineSnapshotResult send capture fusion request!");

        GenerateInternalRequest(OFFLINERTBJPEGSESSION, 1, &request);

        rtSnapshotBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEMAIN] = 0;
        rtSnapshotBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEAUX]  = 0;

        frameCount = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::OfflineRTBJPEGResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseMultiVRCamera::OfflineRTBJPEGResult(
    const ChiCaptureResult* pResult)
{
    UINT32 pipelineIndex;

    if (NULL == pResult->pPrivData)
    {
        CHX_LOG_ERROR("Result: Cannot Accept NULL private data here for = %d", pResult->frameworkFrameNum);
        // Early return
        return;
    }
    else
    {
        pipelineIndex = pResult->pPrivData->streamIndex;
    }

    if (NULL != rtSnapshotBuffer[pResult->frameworkFrameNum % MaxOutstandingRequests].pMetadata[0])
    {
        free_camera_metadata(static_cast<camera_metadata_t*>(
            rtSnapshotBuffer[pResult->frameworkFrameNum % MaxOutstandingRequests].pMetadata[0]));
        rtSnapshotBuffer[pResult->frameworkFrameNum % MaxOutstandingRequests].pMetadata[0] = NULL;
    }

    /// Real time data. Reprocess
    if (0 != pResult->numOutputBuffers)
    {
        // Release reference to the input YUV buffers
        ReleaseReferenceToInputBuffers(pResult->pPrivData);

        camera3_capture_result_t result        = { };
        camera3_stream_buffer_t* pOutputBuffer = static_cast<camera3_stream_buffer_t*>(
            CHX_CALLOC(sizeof(camera3_stream_buffer_t) * pResult->numOutputBuffers));

        if (NULL != pOutputBuffer)
        {
            for (UINT32 i = 0; i < pResult->numOutputBuffers; i++)
            {
                ///< @todo (CAMX-4114) Do not assume incoming CHI buffer is always Native Handle
                ChxUtils::PopulateChiToHALStreamBuffer(&pResult->pOutputBuffers[i], &pOutputBuffer[i]);
            }

            result.frame_number       = pResult->frameworkFrameNum;
            result.result             = NULL;
            result.num_output_buffers = pResult->numOutputBuffers;
            result.output_buffers     = pOutputBuffer;
            result.input_buffer       = NULL;
            result.partial_result     = 0;
            Usecase::ReturnFrameworkResult(reinterpret_cast<const camera3_capture_result_t*>(&result), m_cameraId);
            CHX_LOG("Snapshot Callback: Send to application");

            CHX_FREE(pOutputBuffer);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::OfflineRTBRawResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseMultiVRCamera::OfflineRTBRawResult(
    const ChiCaptureResult* pResult)
{
    UINT32 pipelineIndex;

    if (NULL == pResult->pPrivData)
    {
        CHX_LOG_ERROR("Result: Cannot Accept NULL private data here for = %d", pResult->frameworkFrameNum);
        // Early return
        return;
    }
    else
    {
        pipelineIndex = pResult->pPrivData->streamIndex;
    }



    if (NULL != rtRawSnapshotBuffer[pResult->frameworkFrameNum % MaxOutstandingRequests].pMetadata[0])
    {
        free_camera_metadata(static_cast<camera_metadata_t*>(
            rtRawSnapshotBuffer[pResult->frameworkFrameNum % MaxOutstandingRequests].pMetadata[0]));
        rtRawSnapshotBuffer[pResult->frameworkFrameNum % MaxOutstandingRequests].pMetadata[0] = NULL;
    }

    /// Real time data. Reprocess
    if (0 != pResult->numOutputBuffers)
    {
        // Release reference to the input RDI Raw buffers
        ReleaseReferenceToInputBuffers(pResult->pPrivData);

        camera3_capture_result_t result;
        camera3_stream_buffer_t* pOutputBuffer = static_cast<camera3_stream_buffer_t*>(
            CHX_CALLOC(sizeof(camera3_stream_buffer_t) * pResult->numOutputBuffers));

        if (NULL != pOutputBuffer)
        {
            for (UINT32 i = 0; i < pResult->numOutputBuffers; i++)
            {
                ChxUtils::PopulateChiToHALStreamBuffer(&pResult->pOutputBuffers[i], &pOutputBuffer[i]);
            }

            result.frame_number       = pResult->frameworkFrameNum;
            result.result             = NULL;
            result.num_output_buffers = pResult->numOutputBuffers;
            result.output_buffers     = pOutputBuffer;
            result.input_buffer       = NULL;
            result.partial_result     = 0;
            Usecase::ReturnFrameworkResult(reinterpret_cast<const camera3_capture_result_t*>(&result), m_cameraId);
            CHX_LOG("Raw Snapshot Callback: Send to application");

            CHX_FREE(pOutputBuffer);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::OfflinePreviewResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseMultiVRCamera::OfflinePreviewResult(
    const ChiCaptureResult* pResult)
{
    UINT32 pipelineIndex;

    if (NULL == pResult->pPrivData)
    {
        CHX_LOG_ERROR("Result: Cannot Accept NULL private data here for = %d", pResult->frameworkFrameNum);
        // Early return
        return;
    }
    else
    {
        pipelineIndex = pResult->pPrivData->streamIndex;
    }

    if (pResult->numOutputBuffers != 0)
    {
        // Release reference to the input Vam buffers
        ReleaseReferenceToInputBuffers(pResult->pPrivData);

        camera3_capture_result_t result        = { };
        camera3_stream_buffer_t* pOutputBuffer = static_cast<camera3_stream_buffer_t*>(
            CHX_CALLOC(sizeof(camera3_stream_buffer_t) * pResult->numOutputBuffers));

        if (NULL != pOutputBuffer)
        {
            for (UINT32 i = 0; i < pResult->numOutputBuffers; i++)
            {
                ChxUtils::PopulateChiToHALStreamBuffer(&pResult->pOutputBuffers[i], &pOutputBuffer[i]);
            }

            result.frame_number       = pResult->frameworkFrameNum;
            result.result             = NULL;
            result.num_output_buffers = pResult->numOutputBuffers;
            result.output_buffers     = pOutputBuffer;
            result.input_buffer       = NULL;
            result.partial_result     = 0;
            Usecase::ReturnFrameworkResult(reinterpret_cast<const camera3_capture_result_t*>(&result), m_cameraId);

            CHX_FREE(pOutputBuffer);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::SessionCbNotifyMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseMultiVRCamera::SessionCbNotifyMessage(
    const ChiMessageDescriptor* pMessageDescriptor,
    VOID*                       pPrivateCallbackData)
{
    SessionPrivateData* pSessionPrivateData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
    UsecaseMultiVRCamera* pUsecase            = static_cast<UsecaseMultiVRCamera*>(pSessionPrivateData->pUsecase);

    if ((NULL == pMessageDescriptor) ||
        (ChiMessageTypeSof == pMessageDescriptor->messageType) ||
        (NULL == pMessageDescriptor->pPrivData))
    {
        // Sof notifications are not sent to the HAL3 application and so ignore
        if ((NULL == pMessageDescriptor) ||
            ((ChiMessageTypeSof != pMessageDescriptor->messageType) &&
             (NULL == pMessageDescriptor->pPrivData)))
        {
            CHX_LOG_ERROR("Result: Cannot Accept NULL private data for Notify");
        }
        else if (ChiMessageTypeSof == pMessageDescriptor->messageType)
        {
            CHX_LOG("Chi Notify SOF frameNum %u framework frameNum %u, timestamp %" PRIu64,
                pMessageDescriptor->message.sofMessage.sofId,
                pMessageDescriptor->message.sofMessage.frameworkFrameNum,
                pMessageDescriptor->message.sofMessage.timestamp);
        }

        return;
    }

    CHX_LOG("Notify Session: %d Pipeline: %d frameNum: %d Type: %d Timestamp: %" PRIu64,
        pSessionPrivateData->sessionId,
        pMessageDescriptor->pPrivData->streamIndex,
        pMessageDescriptor->message.shutterMessage.frameworkFrameNum,
        pMessageDescriptor->messageType, pMessageDescriptor->message.shutterMessage.timestamp);

    // Send only the realtime pipeline 0 shutter message
    if (pSessionPrivateData->sessionId == REALTIMESESSIONIDX)
    {
        if (pMessageDescriptor->pPrivData->streamIndex == REALTIMEPIPELINEMAIN)
        {
            UINT32* shutterFrameNum = const_cast<UINT32*>(&(pMessageDescriptor->message.shutterMessage.frameworkFrameNum));
            camera3_notify_msg_t message;
            message.type                         = pMessageDescriptor->messageType;
            message.message.shutter.frame_number = pMessageDescriptor->message.shutterMessage.frameworkFrameNum;
            message.message.shutter.timestamp    = pMessageDescriptor->message.shutterMessage.timestamp;
            pUsecase->ReturnFrameworkMessage((camera3_notify_msg_t*)pMessageDescriptor, pUsecase->m_cameraId);

        }
    }
    else
    {
        if ((pSessionPrivateData->sessionId == OFFLINESESSIONIDX) ||
            (pSessionPrivateData->sessionId == OFFLINERTBJPEGSESSION))
        {
            pUsecase->m_shutterFrameNum = pMessageDescriptor->message.shutterMessage.frameworkFrameNum;
        }
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::SessionCbCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseMultiVRCamera::SessionCbCaptureResult(
    ChiCaptureResult* pResult,
    VOID*             pPrivateCallbackData)
{
    SessionPrivateData* pSessionPrivateData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
    UsecaseMultiVRCamera* pUsecase            = static_cast<UsecaseMultiVRCamera*>(pSessionPrivateData->pUsecase);

    UINT32 pipelineIndex;

    if (NULL == pResult->pPrivData)
    {
        CHX_LOG_ERROR("Result: Cannot Accept NULL private data here for = %d", pResult->frameworkFrameNum);
        // Early return
        return;
    }
    else
    {
        pipelineIndex = pResult->pPrivData->streamIndex;
    }

    CHX_LOG("Result: Session = %d Pipeline = %d FrameNum: %d buffer - type %d handle %p, "
            "status - %d numbuf = %d partial = %d input = %p",
            pSessionPrivateData->sessionId,
            pipelineIndex,
            pResult->frameworkFrameNum,
            pResult->pOutputBuffers[0].bufferInfo.bufferType,
            pResult->pOutputBuffers[0].bufferInfo.phBuffer,
            pResult->pOutputBuffers[0].bufferStatus,
            pResult->numOutputBuffers,
            pResult->numPartialMetadata,
            pResult->pInputBuffer);

    pUsecase->MetadataCaptureResultGet(*pResult, pSessionPrivateData->sessionId);

    if (NULL != pResult->pResultMetadata)
    {
        // Process the result metadata with controller
        pUsecase->m_pMultiCamController->ProcessResultMetadata(const_cast<VOID*>(pResult->pResultMetadata));
    }

    if (REALTIMESESSIONIDX == pSessionPrivateData->sessionId)
    {
        pUsecase->RealtimeCaptureResult(pResult);
    }
    else if (OFFLINESESSIONIDX == pSessionPrivateData->sessionId)
    {
        pUsecase->OfflinePreviewResult(pResult);
    }
    else if (OFFLINERDISESSION == pSessionPrivateData->sessionId ||
             OFFLINERDISESSIONAUX == pSessionPrivateData->sessionId)
    {
        pUsecase->OfflineSnapshotResult(pResult);
    }
    else if (OFFLINERTBJPEGSESSION == pSessionPrivateData->sessionId)
    {
        pUsecase->OfflineRTBJPEGResult(pResult);
    }
    else if (OFFLINERAWSESSION == pSessionPrivateData->sessionId)
    {
        pUsecase->OfflineRTBRawResult(pResult);
    }
    else if (OFFLINEVAMSESSION == pSessionPrivateData->sessionId)
    {
        pUsecase->OfflinePreviewResult(pResult);
    }
    else
    {
        CHX_LOG_ERROR("Unknown session ID %d ", pSessionPrivateData->sessionId);
    }
    pUsecase->MetadataCaptureResultRelease(pResult->pResultMetadata);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseMultiVRCamera::Initialize(
    LogicalCameraInfo*              pCameraInfo,   ///< Camera info
    camera3_stream_configuration_t* pStreamConfig) ///< Stream configuration
{
    CDKResult result             = CDKResultSuccess;
    ChiUsecase* pChiUsecase      = NULL;
    m_isPostViewNeeded           = FALSE;
    m_streamDSFactor             = 2;
    DualCameraUsecase dcUsecase  = UsecaseMax;
    UINT32 remappedPipelineIndex = 0;
    m_pTargetRawSnapshotStream   = NULL;
    m_pTargetSnapshotStream      = NULL;
    m_pTargetVamStream           = NULL;
    m_pTargetPreviewStream       = NULL;
    UINT32 RDIWidth[MAX_REALTIME_PIPELINE];
    UINT32 RDIHeight[MAX_REALTIME_PIPELINE];

    ChiPortBufferDescriptor pipelineOutputBuffer[MaxChiStreams];
    ChiPortBufferDescriptor pipelineInputBuffer[MaxChiStreams];
    DesiredSensorMode desiredSensorMode = { 0 };
    desiredSensorMode.frameRate = ExtensionModule::GetInstance()->GetUsecaseMaxFPS();

    ChxUtils::Memset(pipelineOutputBuffer, 0, sizeof(pipelineOutputBuffer));
    ChxUtils::Memset(pipelineInputBuffer, 0, sizeof(pipelineInputBuffer));
    m_kernelFrameSyncEnable = ExtensionModule::GetInstance()->GetDCFrameSyncMode();

    m_usecaseId = UsecaseId::MultiCamera;
    m_cameraId = pCameraInfo->cameraId;
    CHX_ASSERT(m_streamDSFactor > 0);

    result      = Usecase::Initialize();

    /// Work around. Accepting only preview
    for (UINT stream = 0; stream < pStreamConfig->num_streams; stream++)
    {
        pStreamConfig->streams[stream]->max_buffers = 8;
    }

    m_shutterFrameNum = -1;

    for (UINT stream = 0; stream < pStreamConfig->num_streams; stream++)
    {
        CHX_LOG("stream = %p streamType = %d streamFormat = %d streamWidth = %d streamHeight = %d",
            pStreamConfig->streams[stream],
            pStreamConfig->streams[stream]->stream_type,
            pStreamConfig->streams[stream]->format,
            pStreamConfig->streams[stream]->width,
            pStreamConfig->streams[stream]->height);
        if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == pStreamConfig->streams[stream]->format ||
            HAL_PIXEL_FORMAT_YCbCr_420_888 == pStreamConfig->streams[stream]->format)
        {
            if (m_pTargetPreviewStream == NULL)
            {
                m_pTargetPreviewStream = reinterpret_cast<CHISTREAM*>(pStreamConfig->streams[stream]);
            }
            else
            {
                // @todo use usage flag to identify vam stream
                m_pTargetVamStream = reinterpret_cast<CHISTREAM*>(pStreamConfig->streams[stream]);
            }
        }
        if (ChiStreamFormatBlob == (ChiStreamFormat)pStreamConfig->streams[stream]->format)
        {
            CHX_ASSERT(pCameraInfo->numPhysicalCameras == 2);
            m_pTargetSnapshotStream = reinterpret_cast<CHISTREAM*>(pStreamConfig->streams[stream]);

                UsecaseSelector::getSensorDimension(pCameraInfo->ppDeviceInfo[0]->cameraId,
                    pStreamConfig,
                    &RDIWidth[0],
                    &RDIHeight[0],
                    m_streamDSFactor);

               UsecaseSelector::getSensorDimension(pCameraInfo->ppDeviceInfo[1]->cameraId,
                   pStreamConfig,
                   &RDIWidth[1],
                   &RDIHeight[1],
                   m_streamDSFactor);
        }
        if (TRUE == UsecaseSelector::IsRawStream(pStreamConfig->streams[stream]))
        {
            m_pTargetRawSnapshotStream = reinterpret_cast<CHISTREAM*>(pStreamConfig->streams[stream]);
        }

        // Add Thumbnail YUV size into list if have.
        if ((pStreamConfig->streams[stream]->width * pStreamConfig->streams[stream]->height)
            == (320 * 240))
        {
            m_pTargetPostviewStream = reinterpret_cast<CHISTREAM*>(pStreamConfig->streams[stream]);
            m_isPostViewNeeded = TRUE;
        }
    }

    if (NULL == m_pTargetSnapshotStream)
    {
        pChiUsecase = g_pUsecaseVRVideo;
        dcUsecase = UseCaseVRVideo;
    }
    else
    {
        pChiUsecase = g_pUsecaseVR;
        dcUsecase = UseCaseVR;
    }
    CHX_ASSERT(dcUsecase != UsecaseMax); // it should not come

    if (NULL != pChiUsecase)
    {
        CHX_LOG("[VRMC] usecase name = %s, numPipelines = %d numTargets = %d",
            pChiUsecase->pUsecaseName, pChiUsecase->numPipelines, pChiUsecase->numTargets);

        for (UINT i = 0; i < pChiUsecase->numPipelines; i++)
        {
            CHX_LOG("Pipeline[%d] name = %s, Source Target num = %d, Sink Target num = %d",
                i, pChiUsecase->pPipelineTargetCreateDesc[i].pPipelineName,
                pChiUsecase->pPipelineTargetCreateDesc[i].sourceTarget.numTargets,
                pChiUsecase->pPipelineTargetCreateDesc[i].sinkTarget.numTargets);
        }

        ///< allocate stream for realtimepreview1 and realtimepreview2
        CHIBufferManagerCreateData previewData;
        previewData.format                      = getImplDefinedFormat(m_pTargetPreviewStream->format);
        previewData.width                       = m_pTargetPreviewStream->width/m_streamDSFactor;
        previewData.height                      = m_pTargetPreviewStream->height;
        previewData.consumerFlags               = GRALLOC1_CONSUMER_USAGE_CAMERA | GRALLOC1_CONSUMER_USAGE_CPU_READ;
        previewData.producerFlags               = GRALLOC1_CONSUMER_USAGE_CAMERA;

        if (FPS30 >= desiredSensorMode.frameRate)
        {
            previewData.maxBufferCount          = VRVIDEODEPTH;
        }
        else
        {
           previewData.maxBufferCount           = BufferQueueDepth;
        }
        previewData.immediateBufferCount        = CHIImmediateBufferCountDefault;
        previewData.bEnableLateBinding          = ExtensionModule::GetInstance()->EnableCHILateBinding();
        previewData.bufferHeap                  = BufferHeapDefault;

        CHIBufferManagerCreateData YUVsnapshotData;
        if ( UseCaseVR == dcUsecase)
        {
            YUVsnapshotData.format                  = ChiStreamFormatYCbCr420_888;
            YUVsnapshotData.width                   = m_pTargetSnapshotStream->width/m_streamDSFactor;
            YUVsnapshotData.height                  = m_pTargetSnapshotStream->height;
            YUVsnapshotData.consumerFlags           = GRALLOC1_CONSUMER_USAGE_CAMERA | GRALLOC1_CONSUMER_USAGE_CPU_READ;
            YUVsnapshotData.producerFlags           = GRALLOC1_PRODUCER_USAGE_CAMERA;
            YUVsnapshotData.maxBufferCount          = 2;
            YUVsnapshotData.immediateBufferCount    = 0;
            YUVsnapshotData.bEnableLateBinding      = ExtensionModule::GetInstance()->EnableCHILateBinding();
            YUVsnapshotData.bufferHeap              = BufferHeapDefault;
        }

        CHAR bufferManagerName[MaxStringLength64];

        for(UINT i=0 ; i < MAX_REALTIME_PIPELINE ; i++)
        {
           if (UseCaseVR == dcUsecase)
           {
                m_pRTBPreviewStream[i]  = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                m_pRTPreviewStream[i]   = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                m_pRTBSnapshotStream[i] = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                m_pRTYUVStream[i]       = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                m_pJPEGStream[i]        = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));

                ChxUtils::Memcpy(m_pRTBPreviewStream[i], m_pTargetPreviewStream, sizeof(CHISTREAM));
                ChxUtils::Memcpy(m_pRTPreviewStream[i], m_pTargetPreviewStream, sizeof(CHISTREAM));
                m_pRTBPreviewStream[i]->width = m_pRTBPreviewStream[i]->width/m_streamDSFactor;
                m_pRTBPreviewStream[i]->height = m_pRTBPreviewStream[i]->height;
                m_pRTPreviewStream[i]->width   = m_pRTPreviewStream[i]->width/m_streamDSFactor;
                m_pRTPreviewStream[i]->height  = m_pRTPreviewStream[i]->height;
                m_pRTBPreviewStream[i]->grallocUsage  = GRALLOC1_CONSUMER_USAGE_CAMERA;
                m_pRTPreviewStream[i]->grallocUsage   = GRALLOC1_CONSUMER_USAGE_CAMERA;
                ChxUtils::Memcpy(m_pRTYUVStream[i], m_pTargetSnapshotStream, sizeof(CHISTREAM));
                m_pRTYUVStream[i]->width = m_pRTYUVStream[i]->width/m_streamDSFactor;
                m_pRTYUVStream[i]->height = m_pRTYUVStream[i]->height;
                m_pRTYUVStream[i]->format = ChiStreamFormatYCbCr420_888;
                m_pRTYUVStream[i]->grallocUsage = GRALLOC1_CONSUMER_USAGE_CAMERA;
                ChxUtils::Memcpy(m_pJPEGStream[i], m_pRTYUVStream[i], sizeof(CHISTREAM));

                m_pRTRDIStream[i]                = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                m_pRTRDIStream[i]->format        = ChiStreamFormatRawOpaque;
                m_pRTRDIStream[i]->width         = RDIWidth[i];
                m_pRTRDIStream[i]->height        = RDIHeight[i];
                m_pRTRDIStream[i]->maxNumBuffers = 0;
                m_pRTRDIStream[i]->rotation      = StreamRotationCCW0;
                m_pRTRDIStream[i]->streamType    = ChiStreamTypeBidirectional;
                m_pRTRDIStream[i]->grallocUsage  = 0;
                ChxUtils::Memcpy(m_pRTBSnapshotStream[i], m_pRTRDIStream[i], sizeof(CHISTREAM));

                CHIBufferManagerCreateData snapshotData;
                snapshotData.format                     = HAL_PIXEL_FORMAT_RAW10;
                snapshotData.width                      = RDIWidth[i];
                snapshotData.height                     = RDIHeight[i];
                snapshotData.consumerFlags              = GRALLOC1_CONSUMER_USAGE_CAMERA;
                snapshotData.producerFlags              = GRALLOC1_PRODUCER_USAGE_CAMERA;
                snapshotData.maxBufferCount             = 2;
                snapshotData.immediateBufferCount       = 0;
                snapshotData.bEnableLateBinding         = ExtensionModule::GetInstance()->EnableCHILateBinding();
                snapshotData.bufferHeap                 = BufferHeapDefault;
                snapshotData.pChiStream                 = m_pRTRDIStream[i];

                CdkUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "PreviewBufferManager_%d", i);
                previewData.pChiStream               = m_pRTPreviewStream[i];
                m_pPreviewBufferManager[i]           = CHIBufferManager::Create(bufferManagerName, &previewData);

                CdkUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "RDIBufferManager_%d", i);
                m_pRDIBufferManager[i]               = CHIBufferManager::Create(bufferManagerName, &snapshotData);

                CdkUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "YUVBufferManager_%d", i);
                YUVsnapshotData.pChiStream           = m_pRTYUVStream[i];
                m_pYUVBufferManager[i]               = CHIBufferManager::Create(bufferManagerName, &YUVsnapshotData);

                if (TRUE == IsRawSnapshotSupported())
                {
                    CHIBufferManagerCreateData RawsnapshotData;
                    RawsnapshotData.format                       = HAL_PIXEL_FORMAT_RAW10;
                    RawsnapshotData.width                        = RDIWidth[i];
                    RawsnapshotData.height                       = RDIHeight[i];
                    RawsnapshotData.consumerFlags                = GRALLOC1_CONSUMER_USAGE_CAMERA;
                    RawsnapshotData.producerFlags                = GRALLOC1_PRODUCER_USAGE_CAMERA;
                    RawsnapshotData.maxBufferCount               = BufferQueueDepth;
                    RawsnapshotData.immediateBufferCount         = CHIImmediateBufferCountDefault;
                    RawsnapshotData.bEnableLateBinding           = ExtensionModule::GetInstance()->EnableCHILateBinding();
                    RawsnapshotData.bufferHeap                   = BufferHeapDefault;

                    m_pRTRDISnapshotStream[i]                    = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                    m_pRTBRDISnapshotStream[i]                   = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                    ChxUtils::Memcpy(m_pRTRDISnapshotStream[i], m_pTargetRawSnapshotStream, sizeof(CHISTREAM));
                    ChxUtils::Memcpy(m_pRTBRDISnapshotStream[i], m_pTargetRawSnapshotStream, sizeof(CHISTREAM));

                    m_pRTRDISnapshotStream[i]->width             = RDIWidth[i];
                    m_pRTRDISnapshotStream[i]->height            = RDIHeight[i];
                    m_pRTRDISnapshotStream[i]->grallocUsage      = GRALLOC1_CONSUMER_USAGE_CAMERA;

                    m_pRTBRDISnapshotStream[i]->width            = RDIWidth[i];
                    m_pRTBRDISnapshotStream[i]->height           = RDIHeight[i];
                    m_pRTBRDISnapshotStream[i]->grallocUsage     = GRALLOC1_CONSUMER_USAGE_CAMERA;

                    RawsnapshotData.pChiStream                   = m_pRTBRDISnapshotStream[i];

                    CdkUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "RDIRawBufferManager_%d", i);
                    m_pRDISnapshotBufferManager[i]               = CHIBufferManager::Create(bufferManagerName,
                                                                                            &RawsnapshotData);
                }
                else
                {
                    m_pRTRDISnapshotStream[i]                    = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                    m_pRTRDISnapshotStream[i]->format            = ChiStreamFormatRawOpaque;
                    m_pRTRDISnapshotStream[i]->width             = RDIWidth[i];
                    m_pRTRDISnapshotStream[i]->height            = RDIHeight[i];
                    m_pRTRDISnapshotStream[i]->maxNumBuffers     = 0;
                    m_pRTRDISnapshotStream[i]->rotation          = StreamRotationCCW0;
                    m_pRTRDISnapshotStream[i]->streamType        = ChiStreamTypeBidirectional;
                    m_pRTRDISnapshotStream[i]->grallocUsage      = 0;
                }
           }
           else if (UseCaseVRVideo == dcUsecase)
           {
                m_pRTBPreviewStream[i]                = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                m_pRTPreviewStream[i]                 = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                ChxUtils::Memcpy(m_pRTBPreviewStream[i], m_pTargetPreviewStream, sizeof(CHISTREAM));
                ChxUtils::Memcpy(m_pRTPreviewStream[i], m_pTargetPreviewStream, sizeof(CHISTREAM));
                m_pRTBPreviewStream[i]->width         = m_pRTBPreviewStream[i]->width/m_streamDSFactor;
                m_pRTBPreviewStream[i]->height        = m_pRTBPreviewStream[i]->height;
                m_pRTPreviewStream[i]->width          = m_pRTPreviewStream[i]->width/m_streamDSFactor;
                m_pRTPreviewStream[i]->height         = m_pRTPreviewStream[i]->height;
                m_pRTBPreviewStream[i]->grallocUsage  = GRALLOC1_CONSUMER_USAGE_CAMERA;
                m_pRTPreviewStream[i]->grallocUsage   = GRALLOC1_CONSUMER_USAGE_CAMERA;

                CdkUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "PreviewBufferManager_%d", i);
                previewData.pChiStream                = m_pRTPreviewStream[i];
                m_pPreviewBufferManager[i]            = CHIBufferManager::Create(bufferManagerName, &previewData);

                if (IsVamSupported())
                {
                    ///< allocate stream for realtimepreview1 and realtimepreview2
                    CHIBufferManagerCreateData vamData;
                    vamData.format                      = getImplDefinedFormat(m_pTargetVamStream->format);
                    vamData.width                       = m_pTargetVamStream->width/m_streamDSFactor;
                    vamData.height                      = m_pTargetVamStream->height;
                    vamData.consumerFlags               = GRALLOC1_CONSUMER_USAGE_CAMERA | GRALLOC1_CONSUMER_USAGE_CPU_READ;
                    vamData.producerFlags               = GRALLOC1_PRODUCER_USAGE_CAMERA;

                    if (FPS30 >= desiredSensorMode.frameRate)
                    {
                        vamData.maxBufferCount          = VRVIDEODEPTH;
                    }
                    else
                    {
                        vamData.maxBufferCount          = BufferQueueDepth;
                    }
                    vamData.immediateBufferCount        = CHIImmediateBufferCountDefault;
                    vamData.bEnableLateBinding          = ExtensionModule::GetInstance()->EnableCHILateBinding();
                    vamData.bufferHeap                  = BufferHeapDefault;

                    m_pRTVamStream[i]                   = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                    m_pRTBVamStream[i]                  = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                    ChxUtils::Memcpy(m_pRTVamStream[i], m_pTargetVamStream, sizeof(CHISTREAM));
                    ChxUtils::Memcpy(m_pRTBVamStream[i], m_pTargetVamStream, sizeof(CHISTREAM));
                    m_pRTVamStream[i]->width            = m_pRTVamStream[i]->width/m_streamDSFactor;
                    m_pRTVamStream[i]->height           = m_pRTVamStream[i]->height;
                    m_pRTBVamStream[i]->width           = m_pRTBVamStream[i]->width/m_streamDSFactor;
                    m_pRTBVamStream[i]->height          = m_pRTBVamStream[i]->height;
                    m_pRTVamStream[i]->grallocUsage     = GRALLOC1_CONSUMER_USAGE_CAMERA;
                    m_pRTBVamStream[i]->grallocUsage    = GRALLOC1_CONSUMER_USAGE_CAMERA;

                    CdkUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "VamBufferManager_%d", i);
                    vamData.pChiStream                  = m_pRTVamStream[i];
                    m_pVamBufferManager[i]              = CHIBufferManager::Create(bufferManagerName, &vamData);
                }
                else
                {
                    m_pRTVamStream[i]                    = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                    m_pRTVamStream[i]->format            = ChiStreamFormatYCrCb420_SP;
                    m_pRTVamStream[i]->width             = DUMMYVAMSTREAMWIDTH;
                    m_pRTVamStream[i]->height            = DUMMYVAMSTREAMHEIGHT;
                    m_pRTVamStream[i]->maxNumBuffers     = 0;
                    m_pRTVamStream[i]->rotation          = StreamRotationCCW0;
                    m_pRTVamStream[i]->streamType        = ChiStreamTypeBidirectional;
                    m_pRTVamStream[i]->grallocUsage      = GRALLOC1_CONSUMER_USAGE_CAMERA;
                }
            }
        }

        CHX_LOG("pChiUsecase->numPipelines:%d", pChiUsecase->numPipelines);
        for (UINT i = 0; i < pChiUsecase->numPipelines; i++)
        {
            ChiTargetPortDescriptorInfo* pSinkTarget = &pChiUsecase->pPipelineTargetCreateDesc[i].sinkTarget;
            ChiTargetPortDescriptorInfo* pSrcTarget  = &pChiUsecase->pPipelineTargetCreateDesc[i].sourceTarget;

            CHX_LOG("pipeline Index:%d name = %s",i, pChiUsecase->pPipelineTargetCreateDesc[i].pPipelineName);
            if (pSinkTarget != NULL || pSrcTarget != NULL) {
                CHX_LOG("pipeline Index:%d,targetNum:%d,sourceNums:%d",i,pSinkTarget->numTargets,
                    pSrcTarget->numTargets);
            }
        }

        UINT cameraID = pCameraInfo->ppDeviceInfo[0]->cameraId;
        UINT32 pipelineIndex;
        for (UINT i = 0; i < pChiUsecase->numPipelines; i++)
        {
            ChiTargetPortDescriptorInfo* pSinkTarget = &pChiUsecase->pPipelineTargetCreateDesc[i].sinkTarget;
            ChiTargetPortDescriptorInfo* pSrcTarget  = &pChiUsecase->pPipelineTargetCreateDesc[i].sourceTarget;
            ChiPipelineCreateDescriptor* pPipeline   = &pChiUsecase->pPipelineTargetCreateDesc[i].pipelineCreateDesc;
            CHX_LOG("pipeline Index:%d,targetNum:%d,sourceNums:%d",i,pSinkTarget->numTargets,
                pSrcTarget->numTargets);
            UINT sinkIdx = 0, sourceIdx = 0;
            ChiTargetPortDescriptor* pSinkTargetDesc = NULL, *pSrcTargetDesc = NULL;
            remappedPipelineIndex = RemapPipelineIndex(i, dcUsecase);
            CHX_LOG("input Pipeline Index: %d, Remapped Index: %d", i, remappedPipelineIndex);
            pipelineIndex = INVALIDPIPELINE;
            switch (remappedPipelineIndex)
            {
                case REALTIMEPIPELINEMAIN :
                case REALTIMEPIPELINEAUX  :
                    pipelineIndex = remappedPipelineIndex;
                    cameraID      = pCameraInfo->ppDeviceInfo[pipelineIndex]->cameraId;

                    //Real time Preview
                    pSinkTargetDesc                                  = &pSinkTarget->pTargetPortDesc[sinkIdx];
                    pSinkTargetDesc->pTarget->pChiStream             = m_pRTPreviewStream[pipelineIndex];
                    pipelineOutputBuffer[sinkIdx].pStream            = m_pRTPreviewStream[pipelineIndex];
                    pipelineOutputBuffer[sinkIdx].nodePort           = pSinkTargetDesc->nodeport;
                    pSinkTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeBidirectional;
                    if ( UseCaseVR == dcUsecase)
                    {
                         //Real time RDI
                         sinkIdx++;
                         pSinkTargetDesc                                  = &pSinkTarget->pTargetPortDesc[sinkIdx];
                         pSinkTargetDesc->pTarget->pChiStream             = m_pRTRDIStream[pipelineIndex];
                         pipelineOutputBuffer[sinkIdx].pStream            = m_pRTRDIStream[pipelineIndex];
                         pipelineOutputBuffer[sinkIdx].nodePort           = pSinkTargetDesc->nodeport;
                         pSinkTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeBidirectional;

                         //Realtime/Dummy RDI stream for RAW snapshot
                         sinkIdx++;
                         pSinkTargetDesc                                  = &pSinkTarget->pTargetPortDesc[sinkIdx];
                         pSinkTargetDesc->pTarget->pChiStream             = m_pRTRDISnapshotStream[pipelineIndex];
                         pipelineOutputBuffer[sinkIdx].pStream            = m_pRTRDISnapshotStream[pipelineIndex];
                         pipelineOutputBuffer[sinkIdx].nodePort           = pSinkTargetDesc->nodeport;
                         pSinkTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeBidirectional;
                    }
                    if (IsVamSupported())
                    {
                        //Real time Vam
                        sinkIdx++;
                        pSinkTargetDesc                                  = &pSinkTarget->pTargetPortDesc[sinkIdx];
                        pSinkTargetDesc->pTarget->pChiStream             = m_pRTVamStream[pipelineIndex];
                        pipelineOutputBuffer[sinkIdx].pStream            = m_pRTVamStream[pipelineIndex];
                        pipelineOutputBuffer[sinkIdx].nodePort           = pSinkTargetDesc->nodeport;
                        pSinkTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeBidirectional;
                    }
                    else if (UseCaseVRVideo == dcUsecase)
                    {
                        //Real time dummy vam
                        sinkIdx++;
                        pSinkTargetDesc                                  = &pSinkTarget->pTargetPortDesc[sinkIdx];
                        pSinkTargetDesc->pTarget->pChiStream             = m_pRTVamStream[pipelineIndex];
                        pipelineOutputBuffer[sinkIdx].pStream            = m_pRTVamStream[pipelineIndex];
                        pipelineOutputBuffer[sinkIdx].nodePort           = pSinkTargetDesc->nodeport;
                        pSinkTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeBidirectional;
                    }

                    break;

                case OFFLINEPREVIEWPIPELINE :
                    //Offline Preview
                    pipelineIndex = OFFLINEPREVIEWPIPELINE;
                    CHX_LOG_ERROR("setting OFFLINEPREVIEWPIPELINE ");
                    pSinkTargetDesc                                  = &pSinkTarget->pTargetPortDesc[sinkIdx];
                    pSinkTargetDesc->pTarget->pChiStream             = m_pTargetPreviewStream;
                    pipelineOutputBuffer[sinkIdx].pStream            = m_pTargetPreviewStream;
                    pipelineOutputBuffer[sinkIdx].nodePort           = pSinkTargetDesc->nodeport;
                    pSinkTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeOutput;

                    pSrcTargetDesc                                  = &pSrcTarget->pTargetPortDesc[sourceIdx];
                    pSrcTargetDesc->pTarget->pChiStream             = m_pRTBPreviewStream[sourceIdx];
                    pSrcTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeInput;
                    pipelineInputBuffer[sourceIdx].pStream          = pSrcTargetDesc->pTarget->pChiStream;
                    pipelineInputBuffer[sourceIdx].nodePort         = pSrcTargetDesc->nodeport;

                    sourceIdx++;
                    pSrcTargetDesc                                  = &pSrcTarget->pTargetPortDesc[sourceIdx];
                    pSrcTargetDesc->pTarget->pChiStream             = m_pRTBPreviewStream[sourceIdx];
                    pSrcTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeInput;
                    pipelineInputBuffer[sourceIdx].pStream          = pSrcTargetDesc->pTarget->pChiStream;
                    pipelineInputBuffer[sourceIdx].nodePort         = pSrcTargetDesc->nodeport;
                    break;

                case OFFLINEYUVPIPELINE :
                    //Offline YUV Snapshot
                    pipelineIndex                                    = OFFLINEYUVPIPELINE;
                    pSinkTargetDesc                                  = &pSinkTarget->pTargetPortDesc[sinkIdx];
                    pSinkTargetDesc->pTarget->pChiStream             = m_pRTYUVStream[REALTIMEPIPELINEMAIN];
                    pipelineOutputBuffer[sinkIdx].pStream            = m_pRTYUVStream[REALTIMEPIPELINEMAIN];
                    pipelineOutputBuffer[sinkIdx].nodePort           = pSinkTargetDesc->nodeport;
                    pSinkTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeOutput;
                    if (TRUE == m_isPostViewNeeded)
                    {
                        sinkIdx++;
                        pSinkTargetDesc                                  = &pSinkTarget->pTargetPortDesc[sinkIdx];
                        pSinkTargetDesc->pTarget->pChiStream             = m_pTargetPostviewStream;
                        pipelineOutputBuffer[sinkIdx].pStream            = m_pTargetPostviewStream;
                        pipelineOutputBuffer[sinkIdx].nodePort           = pSinkTargetDesc->nodeport;
                        pSinkTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeOutput;
                    }
                    else
                    {
                        // if no thumbnail post preview stream configure, remove it
                        pSinkTarget->numTargets = 1;
                        pPipeline->numLinks     = 2;
                        for (UINT i = 0; i< pPipeline->numNodes; i++)
                        {
                            if (pPipeline->pNodes[i].nodeId == 65538)
                            {
                                pPipeline->pNodes[i].nodeAllPorts.numOutputPorts = 1;
                            }
                        }

                    }
                    pSrcTargetDesc                                  = &pSrcTarget->pTargetPortDesc[sourceIdx];
                    pSrcTargetDesc->pTarget->pChiStream             = m_pRTBSnapshotStream[REALTIMEPIPELINEMAIN];
                    pSrcTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeInput;
                    pipelineInputBuffer[sourceIdx].pStream          = pSrcTargetDesc->pTarget->pChiStream;
                    pipelineInputBuffer[sourceIdx].nodePort         = pSrcTargetDesc->nodeport;
                    break;

                case OFFLINEYUVPIPELINEAUX :
                    //Offline YUV Snapshot
                    pipelineIndex                                   = OFFLINEYUVPIPELINEAUX;
                    pSinkTargetDesc                                 = &pSinkTarget->pTargetPortDesc[sinkIdx];
                    pSinkTargetDesc->pTarget->pChiStream            = m_pRTYUVStream[REALTIMEPIPELINEAUX];
                    pipelineOutputBuffer[sinkIdx].pStream           = m_pRTYUVStream[REALTIMEPIPELINEAUX];
                    pipelineOutputBuffer[sinkIdx].nodePort          = pSinkTargetDesc->nodeport;
                    pSinkTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeOutput;

                    pSrcTargetDesc                                  = &pSrcTarget->pTargetPortDesc[sourceIdx];
                    pSrcTargetDesc->pTarget->pChiStream             = m_pRTBSnapshotStream[REALTIMEPIPELINEAUX];
                    pSrcTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeInput;
                    pipelineInputBuffer[sourceIdx].pStream          = pSrcTargetDesc->pTarget->pChiStream;
                    pipelineInputBuffer[sourceIdx].nodePort         = pSrcTargetDesc->nodeport;
                    break;

                case OFFLINEJPEGPIPELINE :
                    //Offline SNAPSHOT
                    pipelineIndex                                    = OFFLINEJPEGPIPELINE;
                    pSinkTargetDesc                                  = &pSinkTarget->pTargetPortDesc[sinkIdx];
                    pSinkTargetDesc->pTarget->pChiStream             = m_pTargetSnapshotStream;
                    pipelineOutputBuffer[sinkIdx].pStream            = m_pTargetSnapshotStream;
                    pipelineOutputBuffer[sinkIdx].nodePort           = pSinkTargetDesc->nodeport;
                    pSinkTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeOutput;

                    pSrcTargetDesc                                   = &pSrcTarget->pTargetPortDesc[sourceIdx];
                    pSrcTargetDesc->pTarget->pChiStream              = m_pJPEGStream[sourceIdx];
                    pSrcTargetDesc->pTarget->pChiStream->streamType  = ChiStreamTypeInput;
                    pipelineInputBuffer[sourceIdx].pStream           = pSrcTargetDesc->pTarget->pChiStream;
                    pipelineInputBuffer[sourceIdx].nodePort          = pSrcTargetDesc->nodeport;

                    sourceIdx++;
                    pSrcTargetDesc                                   = &pSrcTarget->pTargetPortDesc[sourceIdx];
                    pSrcTargetDesc->pTarget->pChiStream              = m_pJPEGStream[sourceIdx];
                    pSrcTargetDesc->pTarget->pChiStream->streamType  = ChiStreamTypeInput;
                    pipelineInputBuffer[sourceIdx].pStream           = pSrcTargetDesc->pTarget->pChiStream;
                    pipelineInputBuffer[sourceIdx].nodePort          = pSrcTargetDesc->nodeport;
                    break;

                case OFFLINERAWPIPELINE:
                    if (TRUE == IsRawSnapshotSupported())
                    {
                        CHX_LOG_ERROR("setting OFFLINERAWPIPELINE ");
                        pipelineIndex                                    = OFFLINERAWPIPELINE;
                        pSinkTargetDesc                                  = &pSinkTarget->pTargetPortDesc[sinkIdx];
                        pSinkTargetDesc->pTarget->pChiStream             = m_pTargetRawSnapshotStream;
                        pipelineOutputBuffer[sinkIdx].pStream            = m_pTargetRawSnapshotStream;
                        pipelineOutputBuffer[sinkIdx].nodePort           = pSinkTargetDesc->nodeport;
                        pSinkTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeOutput;

                        pSrcTargetDesc                                   = &pSrcTarget->pTargetPortDesc[sourceIdx];
                        pSrcTargetDesc->pTarget->pChiStream              = m_pRTBRDISnapshotStream[sourceIdx];
                        pSrcTargetDesc->pTarget->pChiStream->streamType  = ChiStreamTypeInput;
                        pipelineInputBuffer[sourceIdx].pStream           = pSrcTargetDesc->pTarget->pChiStream;
                        pipelineInputBuffer[sourceIdx].nodePort          = pSrcTargetDesc->nodeport;

                        sourceIdx++;
                        pSrcTargetDesc                                   = &pSrcTarget->pTargetPortDesc[sourceIdx];
                        pSrcTargetDesc->pTarget->pChiStream              = m_pRTBRDISnapshotStream[sourceIdx];
                        pSrcTargetDesc->pTarget->pChiStream->streamType  = ChiStreamTypeInput;
                        pipelineInputBuffer[sourceIdx].pStream           = pSrcTargetDesc->pTarget->pChiStream;
                        pipelineInputBuffer[sourceIdx].nodePort          = pSrcTargetDesc->nodeport;
                    }
                    break;

                case OFFLINEVAMPIPELINE :
                    // Offline Vam
                    if (IsVamSupported())
                    {
                        pipelineIndex = OFFLINEVAMPIPELINE;
                        CHX_LOG_ERROR("setting OFFLINEVAMPIPELINE ");
                        pSinkTargetDesc                                  = &pSinkTarget->pTargetPortDesc[sinkIdx];
                        pSinkTargetDesc->pTarget->pChiStream             = m_pTargetVamStream;
                        pipelineOutputBuffer[sinkIdx].pStream            = m_pTargetVamStream;
                        pipelineOutputBuffer[sinkIdx].nodePort           = pSinkTargetDesc->nodeport;
                        pSinkTargetDesc->pTarget->pChiStream->streamType = ChiStreamTypeOutput;

                        pSrcTargetDesc                                   = &pSrcTarget->pTargetPortDesc[sourceIdx];
                        pSrcTargetDesc->pTarget->pChiStream              = m_pRTBVamStream[sourceIdx];
                        pSrcTargetDesc->pTarget->pChiStream->streamType  = ChiStreamTypeInput;
                        pipelineInputBuffer[sourceIdx].pStream           = pSrcTargetDesc->pTarget->pChiStream;
                        pipelineInputBuffer[sourceIdx].nodePort          = pSrcTargetDesc->nodeport;

                        sourceIdx++;
                        pSrcTargetDesc                                   = &pSrcTarget->pTargetPortDesc[sourceIdx];
                        pSrcTargetDesc->pTarget->pChiStream              = m_pRTBVamStream[sourceIdx];
                        pSrcTargetDesc->pTarget->pChiStream->streamType  = ChiStreamTypeInput;
                        pipelineInputBuffer[sourceIdx].pStream           = pSrcTargetDesc->pTarget->pChiStream;
                        pipelineInputBuffer[sourceIdx].nodePort          = pSrcTargetDesc->nodeport;
                    }
                    break;

                default :
                    CHX_LOG_ERROR("ERROR: Pipeline index is more than expected");
                    break;
            }
            if (pipelineIndex != INVALIDPIPELINE)
            {
                m_pPipeline[pipelineIndex] = Pipeline::Create(cameraID, PipelineType::Default);

                if (NULL != m_pPipeline[pipelineIndex])
                {
                    m_pPipeline[pipelineIndex]->SetOutputBuffers(pSinkTarget->numTargets, &pipelineOutputBuffer[0]);
                    m_pPipeline[pipelineIndex]->SetInputBuffers(pSrcTarget->numTargets, &pipelineInputBuffer[0]);
                    m_pPipeline[pipelineIndex]->SetPipelineNodePorts(&pChiUsecase->pPipelineTargetCreateDesc[i].pipelineCreateDesc);
                    m_pPipeline[pipelineIndex]->SetPipelineName(pChiUsecase->pPipelineTargetCreateDesc[i].pPipelineName);

                    result = m_pPipeline[pipelineIndex]->CreateDescriptor();
                }
                else
                {
                    CHX_LOG_ERROR("Failed to creat pipeline");
                    result = CDKResultEFailed;
                    break;
                }
            }
        }
    }
    else
    {
        result = CDKResultEFailed;
    }


    if (CDKResultSuccess == result)
    {
        ChiCallBacks callbacks = { 0 };
        ///< create realtime session
        callbacks.ChiNotify               = SessionCbNotifyMessage;
        callbacks.ChiProcessCaptureResult = SessionCbCaptureResult;

        m_perSessionPvtData[REALTIMESESSIONIDX].sessionId  = REALTIMESESSIONIDX;
        m_perSessionPvtData[REALTIMESESSIONIDX].pUsecase   = this;
        m_pSession[REALTIMESESSIONIDX] = Session::Create(&m_pPipeline[REALTIMEPIPELINEMAIN], 2, &callbacks, &m_perSessionPvtData[REALTIMESESSIONIDX]);
        if (NULL == m_pSession[REALTIMESESSIONIDX])
        {
           CHX_LOG_ERROR("Failed to creat realtime Session with multiple pipeline");
           result = CDKResultEFailed;
        }

        ///< create preview offline session
        callbacks.ChiNotify                               = SessionCbNotifyMessage;
        callbacks.ChiProcessCaptureResult                 = SessionCbCaptureResult;
        m_perSessionPvtData[OFFLINESESSIONIDX].sessionId  = OFFLINESESSIONIDX;
        m_perSessionPvtData[OFFLINESESSIONIDX].pUsecase   = this;
        m_pSession[OFFLINESESSIONIDX]                     =
            Session::Create(&m_pPipeline[OFFLINEPREVIEWPIPELINE], 1, &callbacks, &m_perSessionPvtData[OFFLINESESSIONIDX]);
        if (NULL == m_pSession[OFFLINESESSIONIDX])
        {
           CHX_LOG_ERROR("Failed to creat offline Session failed");
           result = CDKResultEFailed;
        }
        if (IsVamSupported())
        {
            m_perSessionPvtData[OFFLINEVAMSESSION].sessionId  = OFFLINEVAMSESSION;
            m_perSessionPvtData[OFFLINEVAMSESSION].pUsecase   = this;
            m_pSession[OFFLINEVAMSESSION]                     =
            Session::Create(&m_pPipeline[OFFLINEVAMPIPELINE], 1, &callbacks, &m_perSessionPvtData[OFFLINEVAMSESSION]);
            if (NULL == m_pSession[OFFLINEVAMSESSION])
            {
               CHX_LOG_ERROR("Failed to create offline vam Session failed");
               result = CDKResultEFailed;
            }
        }
        if ( UseCaseVR == dcUsecase)
        {
            ///< create RDI offline session
            callbacks.ChiNotify                               = SessionCbNotifyMessage;
            callbacks.ChiProcessCaptureResult                 = SessionCbCaptureResult;
            m_perSessionPvtData[OFFLINERDISESSION].sessionId  = OFFLINERDISESSION;
            m_perSessionPvtData[OFFLINERDISESSION].pUsecase   = this;
            m_pSession[OFFLINERDISESSION]                     =
            Session::Create(&m_pPipeline[OFFLINEYUVPIPELINE], 1, &callbacks, &m_perSessionPvtData[OFFLINERDISESSION]);
            if (NULL == m_pSession[OFFLINERDISESSION])
            {
               CHX_LOG_ERROR("Failed to creat offline Session failed");
               result = CDKResultEFailed;
            }

            ///< create RDI offline session for AUX sensor
            callbacks.ChiNotify                                  = SessionCbNotifyMessage;
            callbacks.ChiProcessCaptureResult                    = SessionCbCaptureResult;
            m_perSessionPvtData[OFFLINERDISESSIONAUX].sessionId  = OFFLINERDISESSIONAUX;
            m_perSessionPvtData[OFFLINERDISESSIONAUX].pUsecase   = this;
            m_pSession[OFFLINERDISESSIONAUX]                     =
            Session::Create(&m_pPipeline[OFFLINEYUVPIPELINEAUX], 1, &callbacks, &m_perSessionPvtData[OFFLINERDISESSIONAUX]);
            if (NULL == m_pSession[OFFLINERDISESSIONAUX])
            {
                CHX_LOG_ERROR("Failed to creat offline Session failed");
                result = CDKResultEFailed;
            }

            ///< create JPEG offline session
            callbacks.ChiNotify                                   = SessionCbNotifyMessage;
            callbacks.ChiProcessCaptureResult                     = SessionCbCaptureResult;
            m_perSessionPvtData[OFFLINERTBJPEGSESSION].sessionId  = OFFLINERTBJPEGSESSION;
            m_perSessionPvtData[OFFLINERTBJPEGSESSION].pUsecase   = this;
            m_pSession[OFFLINERTBJPEGSESSION]                     =
            Session::Create(&m_pPipeline[OFFLINEJPEGPIPELINE], 1, &callbacks, &m_perSessionPvtData[OFFLINERTBJPEGSESSION]);
            if (NULL == m_pSession[OFFLINERTBJPEGSESSION])
            {
                CHX_LOG_ERROR("Failed to creat offline Session failed");
                result = CDKResultEFailed;
            }

            if (TRUE == IsRawSnapshotSupported())
            {
                ///< create JPEG offline session
                callbacks.ChiNotify                                   = SessionCbNotifyMessage;
                callbacks.ChiProcessCaptureResult                     = SessionCbCaptureResult;
                m_perSessionPvtData[OFFLINERAWSESSION].sessionId      = OFFLINERAWSESSION;
                m_perSessionPvtData[OFFLINERAWSESSION].pUsecase       = this;
                m_pSession[OFFLINERAWSESSION]                         =
                Session::Create(&m_pPipeline[OFFLINERAWPIPELINE], 1, &callbacks, &m_perSessionPvtData[OFFLINERAWSESSION]);
                if (NULL == m_pSession[OFFLINERAWSESSION])
                {
                    CHX_LOG_ERROR("Failed to creat offline Session failed");
                    result = CDKResultEFailed;
                }
            }
        }
        if (CDKResultSuccess == result)
        {
            // Get multi-cam controller object
            MccCreateData mccCreateData = { 0 };
            mccCreateData.numBundledCameras = pCameraInfo->numPhysicalCameras;
            mccCreateData.primaryCameraId   = pCameraInfo->primaryCameraId;
            CHX_ASSERT(DualCamCount == pCameraInfo->numPhysicalCameras);
            CamInfo camInfo[DualCamCount];

            for (UINT32 i = 0; i < mccCreateData.numBundledCameras; ++i)
            {
                camInfo[i].camId         = pCameraInfo->ppDeviceInfo[i]->cameraId;
                camInfo[i].pChiCamInfo   = pCameraInfo->ppDeviceInfo[i]->m_pDeviceCaps;
                camInfo[i].pChiCamConfig = pCameraInfo->ppDeviceInfo[i]->pDeviceConfig;

                CHISENSORMODEINFO* selectedSensorMode = UsecaseSelector::GetSensorModeInfo(camInfo[i].camId, pStreamConfig, 1);

                camInfo[i].sensorOutDimension.width  = selectedSensorMode->frameDimension.width;
                camInfo[i].sensorOutDimension.height = selectedSensorMode->frameDimension.height;

                UsecaseSelector::CalculateFovRectIFE(&camInfo[i].fovRectIFE,
                                                     selectedSensorMode->frameDimension,
                                                     camInfo[i].pChiCamInfo->sensorCaps.activeArray);

                mccCreateData.pBundledCamInfo[i] = &camInfo[i];
            }

            StreamConfig streamConfig = { 0 };
            streamConfig.numStreams      = pStreamConfig->num_streams;
            streamConfig.pStreamInfo     =
                static_cast<StreamInfo*>(CHX_CALLOC(pStreamConfig->num_streams * sizeof(StreamInfo)));
            if (NULL != streamConfig.pStreamInfo)
            {
                for (UINT32 i = 0; i < streamConfig.numStreams; ++i)
                {
                    streamConfig.pStreamInfo[i].streamType             = pStreamConfig->streams[i]->stream_type;
                    streamConfig.pStreamInfo[i].usage                  = pStreamConfig->streams[i]->usage;
                    streamConfig.pStreamInfo[i].streamDimension.width  = pStreamConfig->streams[i]->width;
                    streamConfig.pStreamInfo[i].streamDimension.height = pStreamConfig->streams[i]->height;
                }
            }
            mccCreateData.pStreamConfig         = &streamConfig;
            mccCreateData.logicalCameraId       = 0;
            mccCreateData.logicalCameraType     = LogicalCameraType_VR;

            m_pMultiCamController = MultiCamControllerManager::GetInstance()->GetController(&mccCreateData);

            if (NULL != m_pMultiCamController)
            {
                // Allocate input metadata buffer for the offline pipeline
                m_pOfflinePipelineInputMetadata = allocate_camera_metadata(ReplacedMetadataEntryCapacity,
                                                        ReplacedMetadataDataCapacity);
                CHX_ASSERT(NULL != m_pOfflinePipelineInputMetadata);
            }
            else
            {
                result = CDKResultEFailed;
            }

            CHX_FREE(streamConfig.pStreamInfo);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::getImplDefinedFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHISTREAMFORMAT UsecaseMultiVRCamera::getImplDefinedFormat(CHISTREAMFORMAT bufFormat)
{
    UINT32 overrideFormat;
    CHISTREAMFORMAT overrideImplDefinedFormat = ChiStreamFormatImplDefined;
    if (bufFormat == ChiStreamFormatImplDefined)
    {
        overrideFormat = ExtensionModule::GetInstance()->GetOutputFormat();
        if (overrideFormat == 1)
        {
            overrideImplDefinedFormat = ChiStreamFormatUBWCNV12;
        }
    }
    else
    {
        overrideImplDefinedFormat = bufFormat;
    }
    return overrideImplDefinedFormat;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::ExecuteCaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseMultiVRCamera::ExecuteCaptureRequest(
    camera3_capture_request_t* pRequest)
{
    CDKResult result = CDKResultSuccess;

    // Request to generate preview for now
    for (UINT i = 0; i < pRequest->num_output_buffers; i++)
    {
        CHX_LOG("Request : frameNum :%d acquireFence: %d , ReleaseFence: %d Buffer: %p, status: %d",
            pRequest->frame_number, pRequest->output_buffers[i].acquire_fence,
            pRequest->output_buffers[i].release_fence, pRequest->output_buffers[i].buffer,
            pRequest->output_buffers[i].status);
    }
    result = GenerateRealtimeRequest(pRequest);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::NotifyResultsAvailable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseMultiVRCamera::NotifyResultsAvailable(
    const CHICAPTURERESULT* pResult, INT index)
{
    CDKResult result                 = CDKResultSuccess;
    UINT32 pipelineIndex             = pResult->pPrivData->streamIndex;
    UINT32 applicationFrameNum       = pResult->frameworkFrameNum;
    UINT32 applicationFrameIndex     = applicationFrameNum %  MaxOutstandingRequests;
    BOOL   hasPreviewStream          = FALSE;
    BOOL   hasVamStream              = FALSE;
    BOOL   vidRequest                = FALSE;
    BOOL   vamRequest                = FALSE;
    BOOL   metaAvailable             = FALSE;
    ChiStreamBuffer* rtPreviewBuffer = NULL;
    ChiStreamBuffer* rtBufferVam     = NULL;

    if (index != -1)
    {
        if (IsRTPreviewStream(pResult->pOutputBuffers[index].pStream))
        {
            hasPreviewStream = TRUE;
            rtPreviewBuffer  = const_cast<CHISTREAMBUFFER*>(&pResult->pOutputBuffers[index]);
        }
        if ((IsVamSupported()) && IsRTVamStream(pResult->pOutputBuffers[index].pStream))
        {
            hasVamStream = TRUE;
            rtBufferVam  = const_cast<CHISTREAMBUFFER*>(&pResult->pOutputBuffers[index]);
        }
    }

    if ((TRUE == hasPreviewStream) && (NULL != rtPreviewBuffer))
    {
        // Receive Realtime pipeline result
        if (REALTIMEPIPELINEMAIN == pipelineIndex)
        {
            rtBuffer[applicationFrameIndex].frameNumber = applicationFrameNum;
            if (pResult->numOutputBuffers > 0)
            {
                ChxUtils::Memcpy(&rtBuffer[applicationFrameIndex].buffer[pipelineIndex],
                                 rtPreviewBuffer,
                                 sizeof(CHISTREAMBUFFER));

                rtBuffer[applicationFrameIndex].buffer[pipelineIndex].pStream = m_pRTBPreviewStream[REALTIMEPIPELINEMAIN];
                rtBuffer[applicationFrameIndex].valid[pipelineIndex]         |= BUF_READY_FLAG;;
            }
        }
        else if (REALTIMEPIPELINEAUX == pipelineIndex)
        {
            rtBuffer[applicationFrameIndex].frameNumber = applicationFrameNum;
            if (pResult->numOutputBuffers > 0)
            {
                ChxUtils::Memcpy(&rtBuffer[applicationFrameIndex].buffer[pipelineIndex],
                                 rtPreviewBuffer,
                                 sizeof(CHISTREAMBUFFER));

                rtBuffer[applicationFrameIndex].buffer[pipelineIndex].pStream = m_pRTBPreviewStream[REALTIMEPIPELINEAUX];
                rtBuffer[applicationFrameIndex].valid[pipelineIndex]         |= BUF_READY_FLAG;
            }
        }
    }

    if ((TRUE == hasVamStream) && (NULL != rtBufferVam))
    {
        // Receive Realtime pipeline result
        rtVamBuffer[applicationFrameIndex].frameNumber = applicationFrameNum;
        if (pResult->numOutputBuffers > 0)
        {
            ChxUtils::Memcpy(&rtVamBuffer[applicationFrameIndex].buffer[pipelineIndex],
                             rtBufferVam,
                             sizeof(CHISTREAMBUFFER));
            rtVamBuffer[applicationFrameIndex].buffer[pipelineIndex].pStream = m_pRTBVamStream[pipelineIndex];
            rtVamBuffer[applicationFrameIndex].valid[pipelineIndex]         |= BUF_READY_FLAG;
        }
    }

    ///< if video requests
    if ((rtBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEMAIN] & BUF_READY_FLAG) &&
            (rtBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEAUX] & BUF_READY_FLAG))
    {
        vidRequest = TRUE;
    }

    ///< if vam requests
    if ((rtVamBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEMAIN] & BUF_READY_FLAG) &&
        (rtVamBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEAUX] & BUF_READY_FLAG))
    {
        vamRequest = TRUE;
    }
    if (NULL != pResult->pResultMetadata)
    {
        // Receive Realtime pipeline result
        rtBuffer[applicationFrameIndex].pMetadata[pipelineIndex]   = const_cast<VOID*>(pResult->pResultMetadata);
        rtBuffer[applicationFrameIndex].valid[pipelineIndex]      |= META_READY_FLAG;
        if (TRUE == IsVamSupported())
        {
            rtVamBuffer[applicationFrameIndex].valid[pipelineIndex]   |= META_READY_FLAG;
        }
    }
    if ((rtBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEMAIN] & META_READY_FLAG) ||
        (rtVamBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEMAIN] & META_READY_FLAG))
    {
        metaAvailable = TRUE;
    }
    ///< if two result are ready, send offline request
    if ((TRUE == metaAvailable) && ((TRUE == vidRequest) || (TRUE == vamRequest)))
    {
        // trigger reprocessing
        CHISTREAMBUFFER     outputBuffers[2]    = { };
        CHICAPTUREREQUEST   request             = { 0 };
        UINT32              buffercount         = 0;

        request.pPrivData   = &privOffline[applicationFrameIndex];

        if (TRUE == vidRequest)
        {
            // TARGET_PREVIEW
            ChxUtils::PopulateHALToChiStreamBuffer(&m_appPreviewBuffer[applicationFrameIndex], &outputBuffers[buffercount]);

            buffercount++;
            request.frameNumber     = rtBuffer[applicationFrameIndex].frameNumber;
            request.hPipelineHandle = reinterpret_cast<CHIPIPELINEHANDLE>(
                                      m_pSession[OFFLINESESSIONIDX]->GetPipelineHandle(0));
            request.numInputs       = 2;
            request.numOutputs      = buffercount;
            request.pInputBuffers   = rtBuffer[applicationFrameIndex].buffer;
            request.pOutputBuffers  = outputBuffers;

            // Save input Preview buffers info, reference will be released once snapshot result is received OfflinePreviewResult
            request.pPrivData->numInputBuffers   = 2;
            request.pPrivData->bufferManagers[0] = m_pPreviewBufferManager[REALTIMEPIPELINEMAIN];
            request.pPrivData->inputBuffers[0]   = rtBuffer[applicationFrameIndex].buffer[REALTIMEPIPELINEMAIN].bufferInfo;
            request.pPrivData->bufferManagers[1] = m_pPreviewBufferManager[REALTIMEPIPELINEAUX];
            request.pPrivData->inputBuffers[1]   = rtBuffer[applicationFrameIndex].buffer[REALTIMEPIPELINEAUX].bufferInfo;
        }

        if (TRUE == vamRequest)
        {
            // TARGET_PREVIEW
            ChxUtils::PopulateHALToChiStreamBuffer(&m_appVamBuffer[applicationFrameIndex], &outputBuffers[buffercount]);

            buffercount++;
            request.frameNumber     = rtVamBuffer[applicationFrameIndex].frameNumber;
            request.hPipelineHandle = reinterpret_cast<CHIPIPELINEHANDLE>(
                                      m_pSession[OFFLINEVAMSESSION]->GetPipelineHandle(0));
            request.numInputs       = 2;
            request.numOutputs      = buffercount;

            request.pInputBuffers   = rtVamBuffer[applicationFrameIndex].buffer;
            request.pOutputBuffers  = outputBuffers;

            // Save input Vam buffers info, the reference will be released once snapshot result is received OfflinePreviewResult
            request.pPrivData->numInputBuffers   = 2;
            request.pPrivData->bufferManagers[0] = m_pVamBufferManager[REALTIMEPIPELINEMAIN];
            request.pPrivData->inputBuffers[0]   = rtVamBuffer[applicationFrameIndex].buffer[REALTIMEPIPELINEMAIN].bufferInfo;
            request.pPrivData->bufferManagers[1] = m_pVamBufferManager[REALTIMEPIPELINEAUX];
            request.pPrivData->inputBuffers[1]   = rtVamBuffer[applicationFrameIndex].buffer[REALTIMEPIPELINEAUX].bufferInfo;
        }

        if (TRUE == videoRequest[applicationFrameIndex])
        {
            if (buffercount >= 2)
            {
                CHX_LOG_ERROR("Array output index of size 2 is using index %d, returning result as failed", buffercount);
                result = CDKResultEFailed;
                return result;
            }

            // TARGET_VIDEO
            ChxUtils::PopulateHALToChiStreamBuffer(&m_appVideoBuffer[applicationFrameIndex], &outputBuffers[buffercount]);
            buffercount++;
        }

        MulticamResultMetadata multiCamResultMetadata;

        multiCamResultMetadata.frameNum            = pResult->frameworkFrameNum;
        /// @note: Change the num results when LPM is enabled
        multiCamResultMetadata.numResults          = DualCamCount;
        multiCamResultMetadata.ppResultMetadata    =
            static_cast<VOID**>(CHX_CALLOC(multiCamResultMetadata.numResults * sizeof(VOID*)));

        if (NULL != multiCamResultMetadata.ppResultMetadata)
        {
            multiCamResultMetadata.ppResultMetadata[0]  = rtBuffer[applicationFrameIndex].pMetadata[0];
            multiCamResultMetadata.ppResultMetadata[1]  = rtBuffer[applicationFrameIndex].pMetadata[1];

            camera_metadata_t* pCameraInfoConsolidated1 = static_cast<camera_metadata_t*>(
                                                                              multiCamResultMetadata.ppResultMetadata[1]);
            camera_metadata_t* pCameraInfoConsolidated0 = static_cast<camera_metadata_t*>(
                                                                              multiCamResultMetadata.ppResultMetadata[0]);
            UINT64 sensorTimestamp0        = 0;
            UINT64 sensorTimestamp1        = 0;
            camera_metadata_entry_t entry2 = { 0 };

            if (0 == find_camera_metadata_entry(pCameraInfoConsolidated1, ANDROID_SENSOR_TIMESTAMP, &entry2))
            {
                sensorTimestamp1 = entry2.data.i64[0];
            }
            entry2 = { 0 };
            if (0 == find_camera_metadata_entry(pCameraInfoConsolidated0, ANDROID_SENSOR_TIMESTAMP, &entry2))
            {
                sensorTimestamp0 = entry2.data.i64[0];
            }
            if (sensorTimestamp1 > sensorTimestamp0)
            {
                CHX_LOG("Sensor time diff for frame:%d and Timestamp : %" PRIi64,
                        applicationFrameNum, (sensorTimestamp1 - sensorTimestamp0));
            }
            else
            {
                CHX_LOG("Sensor time diff for frame:%d and Timestamp : %" PRIi64,
                        applicationFrameNum, (sensorTimestamp0 - sensorTimestamp1));
            }
            m_pMultiCamController->FillOfflinePipelineInputMetadata(&multiCamResultMetadata,
                                                                    m_pOfflinePipelineInputMetadata,
                                                                    FALSE);
            request.pMetadata = m_pOfflinePipelineInputMetadata;

            if (TRUE == vidRequest)
            {
                request.pPrivData->streamIndex = OFFLINEPREVIEWPIPELINE;
            }

            if (TRUE == vamRequest)
            {
                request.pPrivData->streamIndex = OFFLINEVAMPIPELINE;
            }

            if (FALSE == m_pSession[REALTIMESESSIONIDX]->IsPipelineActive(0))
            {
                result = ExtensionModule::GetInstance()->ActivatePipeline(m_pSession[REALTIMESESSIONIDX]->GetSessionHandle(),
                                                                          m_pSession[REALTIMESESSIONIDX]->GetPipelineHandle(0));
                if (CDKResultSuccess == result)
                {
                    m_pSession[REALTIMESESSIONIDX]->SetPipelineActivateFlag(0);
                }
            }

            CHX_LOG("Request: Buffer for reprocessing frameNum = %" PRIu64 " buffercount %d",
                    request.frameNumber, buffercount);
            if (TRUE == vidRequest)
            {
                GenerateInternalRequest(OFFLINESESSIONIDX, 1, &request);

                rtBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEMAIN] = 0;
                rtBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEAUX] = 0;
            }
            if (TRUE == vamRequest)
            {
                GenerateInternalRequest(OFFLINEVAMSESSION, 1, &request);

                rtVamBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEMAIN] = 0;
                rtVamBuffer[applicationFrameIndex].valid[REALTIMEPIPELINEAUX] = 0;
            }

            CHX_FREE(multiCamResultMetadata.ppResultMetadata);
        }
        else
        {
            CHX_LOG_ERROR("Calloc for ppResultMetadata failed!");
            result = CDKResultEFailed;
        }
    }

     return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::ProcessResults
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseMultiVRCamera::ProcessResults()
{

}

CDKResult UsecaseMultiVRCamera::GenerateInternalRequest(
    UINT32 sessionId, UINT numRequest, CHICAPTUREREQUEST* pRequest)
{
    CDKResult         result = CDKResultSuccess;

    CHIPIPELINEREQUEST submitRequest;

    for (UINT32 i = 0; i < numRequest; i++)
    {
        CHX_LOG("Request SessionId %d, i %d, PipelineIndex %d, frame %" PRIu64 ", numInputs %d, numOutputs %d",
            sessionId, i, pRequest[i].pPrivData->streamIndex,
            pRequest[i].frameNumber, pRequest[i].numInputs, pRequest[i].numOutputs);

        MetadataCaptureRequestUpdate(pRequest[i], sessionId, (REALTIMESESSIONIDX == sessionId) && (0 == i));
    }
    submitRequest.pSessionHandle   = reinterpret_cast<CHIHANDLE>(m_pSession[sessionId]->GetSessionHandle());
    submitRequest.numRequests      = numRequest;
    submitRequest.pCaptureRequests = pRequest;

    result = SubmitRequest(&submitRequest);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::GeneratePreviewRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseMultiVRCamera::GenerateRealtimeRequest(
    camera3_capture_request_t* pRequest)
{
    CDKResult           result              = CDKResultSuccess;
    CHISTREAMBUFFER     mainBuffer[3];
    CHISTREAMBUFFER     auxBuffer[3];
    CHISTREAMBUFFER*    pBuffer             = NULL;
    UINT32              internalFrameNum    = 0;
    UINT32              camID               = 0;
    UINT32              numBuffers          = 0;
    CHISTREAM*          pStream             = NULL;
    CHICAPTUREREQUEST   chiRequest[2]       = { {0}, {0} };
    CHICAPTUREREQUEST*  request             = NULL;

    // first we need to store request output;
    UINT32 applicationFrameNum   = pRequest->frame_number;
    UINT32 applicationFrameIndex = pRequest->frame_number % MaxOutstandingRequests;

    MulticamReqSettings multiCamSettings;
    const camera_metadata_t* pMetaTranslated = pRequest->settings;

    if (NULL != pRequest->settings)
    {
        // Translate request settings
        multiCamSettings.numSettingsBuffers = DualCamCount;
        multiCamSettings.ppSettings         = static_cast<VOID**>(CHX_CALLOC(multiCamSettings.numSettingsBuffers * sizeof(VOID*)));
        if (NULL != multiCamSettings.ppSettings)
        {
            multiCamSettings.ppSettings[0]      = static_cast<VOID*>(const_cast<camera_metadata_t*>(pRequest->settings));
            multiCamSettings.ppSettings[1]      =  allocate_copy_camera_metadata_checked(pRequest->settings,
                                                   get_camera_metadata_size(pRequest->settings));
        }
        multiCamSettings.frameNum              =  pRequest->frame_number;
        multiCamSettings.kernelFrameSyncEnable = m_kernelFrameSyncEnable;

        if (CDKResultSuccess != m_pMultiCamController->TranslateRequestSettings(&multiCamSettings))
        {
            CHX_LOG_ERROR("Error in translating request settings");
        }
        else
        {
            pMetaTranslated = static_cast<const camera_metadata_t*>(multiCamSettings.ppSettings[1]);
        }
    }

    /// Initialize the postview and videoRequest
    postviewRequest[applicationFrameIndex] = FALSE;
    videoRequest[applicationFrameIndex]    = FALSE;
    camID                      = REALTIMEPIPELINEMAIN;

    for( UINT32 stream = 0 ; stream < pRequest->num_output_buffers; stream++){
        CHX_LOG("pRequest->output_buffers[stream].stream = %p", pRequest->output_buffers[stream].stream);
        pStream = (CHISTREAM *)pRequest->output_buffers[stream].stream;
        if (pStream == m_pTargetPreviewStream)
        {
            ChxUtils::Memcpy(&m_appPreviewBuffer[applicationFrameIndex],
                &pRequest->output_buffers[stream], sizeof(camera3_stream_buffer_t));

            CHX_LOG("Preview real time request = %p", m_pRTPreviewStream[camID]);

            pBuffer = &mainBuffer[numBuffers];

            pBuffer->size               = sizeof(CHISTREAMBUFFER);
            pBuffer->acquireFence.valid = FALSE;
            pBuffer->bufferInfo         = m_pPreviewBufferManager[camID]->GetImageBufferInfo();
            pBuffer->pStream            = m_pRTPreviewStream[camID];

            numBuffers++;
        }
        else if (pStream == m_pTargetSnapshotStream)
        {
            ChxUtils::Memcpy(&m_appSnapshotBuffer[applicationFrameIndex],
                &pRequest->output_buffers[stream], sizeof(camera3_stream_buffer_t));

            CHX_LOG("RDI Real time request = %p", m_pRTRDIStream[camID]);
            CHX_LOG("Snapshot acquire = %d release = %d", m_appSnapshotBuffer[applicationFrameIndex].acquire_fence,
                m_appSnapshotBuffer[applicationFrameIndex].release_fence);

            pBuffer = &mainBuffer[numBuffers];

            pBuffer->size               = sizeof(CHISTREAMBUFFER);
            pBuffer->acquireFence.valid = FALSE;
            pBuffer->bufferInfo         = m_pRDIBufferManager[camID]->GetImageBufferInfo();
            pBuffer->pStream            = m_pRTRDIStream[camID];

            numBuffers++;
        }
        else if (pStream == m_pTargetPostviewStream)
        {
            CHX_LOG("Thumbnail Requested = %p", m_pTargetPostviewStream);
            ChxUtils::Memcpy(&m_appPostviewBuffer[applicationFrameIndex],
                &pRequest->output_buffers[stream], sizeof(camera3_stream_buffer_t));
            CHX_LOG("Thumbnail acquire = %d release = %d", m_appPostviewBuffer[applicationFrameIndex].acquire_fence,
                m_appPostviewBuffer[applicationFrameIndex].release_fence);
            postviewRequest[applicationFrameIndex] = 1;
        }
        else if (pStream == m_pTargetVideoStream)
        {
            CHX_LOG("Video Requested = %p", m_pTargetVideoStream);
            ChxUtils::Memcpy(&m_appVideoBuffer[applicationFrameIndex],
                &pRequest->output_buffers[stream], sizeof(camera3_stream_buffer_t));
            CHX_LOG("Video acquire = %d release = %d", m_appVideoBuffer[applicationFrameIndex].acquire_fence,
                m_appVideoBuffer[applicationFrameIndex].release_fence);
            videoRequest[applicationFrameIndex] = 1;
        }
        else if (pStream == m_pTargetRawSnapshotStream)
        {
            ChxUtils::Memcpy(&m_appRawSnapshotBuffer[applicationFrameIndex],
                &pRequest->output_buffers[stream], sizeof(camera3_stream_buffer_t));

            CHX_LOG("Raw Snapshot Real time request = %p", m_pRTRDISnapshotStream[camID]);
            CHX_LOG("Snapshot acquire = %d release = %d", m_appRawSnapshotBuffer[applicationFrameIndex].acquire_fence,
                m_appRawSnapshotBuffer[applicationFrameIndex].release_fence);

            pBuffer = &mainBuffer[numBuffers];

            pBuffer->size           = sizeof(CHISTREAMBUFFER);
            pBuffer->acquireFence.valid = FALSE;
            pBuffer->bufferInfo     = m_pRDISnapshotBufferManager[camID]->GetImageBufferInfo();
            pBuffer->pStream        = m_pRTRDISnapshotStream[camID];

            numBuffers++;
        }
        else if (pStream == m_pTargetVamStream)
        {
            CHX_LOG("VAM Requested = %p", m_pTargetVamStream);
            ChxUtils::Memcpy(&m_appVamBuffer[applicationFrameIndex],
                &pRequest->output_buffers[stream], sizeof(camera3_stream_buffer_t));

            CHX_LOG("Video acquire = %d release = %d", m_appVamBuffer[applicationFrameIndex].acquire_fence,
                m_appVamBuffer[applicationFrameIndex].release_fence);
            vamRequest[applicationFrameIndex] = 1;

            pBuffer = &mainBuffer[numBuffers];

            pBuffer->size               = sizeof(CHISTREAMBUFFER);
            pBuffer->acquireFence.valid = FALSE;
            pBuffer->bufferInfo         = m_pVamBufferManager[camID]->GetImageBufferInfo();
            pBuffer->pStream            = m_pRTVamStream[camID];

            numBuffers++;
        }
        else
        {
            CHX_LOG("New stream???");
        }
    }

    if (NULL != pRequest->settings)
    {
         UINT32 sensorModeIndex = m_pSession[REALTIMESESSIONIDX]->GetSensorModeInfo()->modeIndex;

         ChxUtils::AndroidMetadata::FillTuningModeData(
                                     const_cast<VOID*>(reinterpret_cast<const VOID*>(pRequest->settings)),
                                     pRequest,
                                     sensorModeIndex,
                                     &m_effectModeValue[camID],
                                     &m_sceneModeValue[camID],
                                     &m_tuningFeature1Value[camID],
                                     &m_tuningFeature2Value[camID]);
    }

    internalFrameNum                 = applicationFrameNum;
    request                          = &chiRequest[camID];
    request->frameNumber             = internalFrameNum;
    request->hPipelineHandle         = reinterpret_cast<CHIPIPELINEHANDLE>(
                                       m_pSession[REALTIMESESSIONIDX]->GetPipelineHandle(camID));
    request->numOutputs              = numBuffers;
    request->pOutputBuffers          = mainBuffer;
    request->pMetadata               = pRequest->settings;
    request->pPrivData               = &privRealTime1[applicationFrameNum % MaxOutstandingRequests];
    request->pPrivData->streamIndex  = REALTIMEPIPELINEMAIN;

    ///< generate request for realtime1
    numBuffers = 0;
    camID      = REALTIMEPIPELINEAUX;

    for( UINT32 stream = 0 ; stream < pRequest->num_output_buffers ; stream++){
        pStream = (CHISTREAM *)pRequest->output_buffers[stream].stream;
        if (pStream == m_pTargetPreviewStream)
        {
            CHX_LOG("Preview real time request = %p", m_pRTPreviewStream[camID]);
            pBuffer = &auxBuffer[numBuffers];

            pBuffer->size               = sizeof(CHISTREAMBUFFER);
            pBuffer->acquireFence.valid = FALSE;
            pBuffer->bufferInfo         = m_pPreviewBufferManager[camID]->GetImageBufferInfo();
            pBuffer->pStream            = m_pRTPreviewStream[camID];

            numBuffers++;
        }
        else if (pStream == m_pTargetSnapshotStream)
        {
            CHX_LOG("RDI Real time request = %p", m_pRTRDIStream[camID]);
            pBuffer = &auxBuffer[numBuffers];

            pBuffer->size               = sizeof(CHISTREAMBUFFER);
            pBuffer->acquireFence.valid = FALSE;
            pBuffer->bufferInfo         = m_pRDIBufferManager[camID]->GetImageBufferInfo();
            pBuffer->pStream            = m_pRTRDIStream[camID];

            numBuffers++;
        }
        else if (pStream == m_pTargetPostviewStream)
        {
            CHX_LOG("Thumbnail Requested = %p", m_pTargetPostviewStream);
            postviewRequest[applicationFrameIndex] = 1;
        }
        else if (pStream == m_pTargetRawSnapshotStream)
        {
            CHX_LOG("Raw Snapshot Real time request = %p", m_pRTRDISnapshotStream[camID]);
            pBuffer = &auxBuffer[numBuffers];

            pBuffer->size               = sizeof(CHISTREAMBUFFER);
            pBuffer->acquireFence.valid = FALSE;
            pBuffer->bufferInfo         = m_pRDISnapshotBufferManager[camID]->GetImageBufferInfo();
            pBuffer->pStream            = m_pRTRDISnapshotStream[camID];

            numBuffers++;
        }
        else if (pStream == m_pTargetVamStream)
        {
            CHX_LOG("Aux VAM request = %p", m_pRTVamStream[camID]);
            pBuffer = &auxBuffer[numBuffers];

            pBuffer->size               = sizeof(CHISTREAMBUFFER);
            pBuffer->acquireFence.valid = FALSE;
            pBuffer->bufferInfo         = m_pVamBufferManager[camID]->GetImageBufferInfo();
            pBuffer->pStream            = m_pRTVamStream[camID];

            numBuffers++;
        }
        else
        {
            CHX_LOG("New stream???");
        }
    }

    if (NULL != pMetaTranslated)
    {
         UINT32 sensorModeIndex = m_pSession[REALTIMESESSIONIDX]->GetSensorModeInfo()->modeIndex;

        ChxUtils::AndroidMetadata::FillTuningModeData(const_cast<VOID*>(reinterpret_cast<const VOID*>(pMetaTranslated)),
                                     pRequest,
                                     sensorModeIndex,
                                     &m_effectModeValue[camID],
                                     &m_sceneModeValue[camID],
                                     &m_tuningFeature1Value[camID],
                                     &m_tuningFeature2Value[camID]);
    }
    request                         = &chiRequest[camID];
    request->frameNumber            = internalFrameNum;
    request->hPipelineHandle        = reinterpret_cast<CHIPIPELINEHANDLE>(
                                      m_pSession[REALTIMESESSIONIDX]->GetPipelineHandle(camID));
    request->numOutputs             = numBuffers;
    request->pOutputBuffers         = auxBuffer;
    request->pMetadata              = pMetaTranslated;
    request->pPrivData              =  &privRealTime2[applicationFrameNum % MaxOutstandingRequests];
    request->pPrivData->streamIndex = REALTIMEPIPELINEAUX;

    for (UINT32 i = 0; i <= REALTIMEPIPELINEAUX; i++)
    {
        if (FALSE == m_pSession[REALTIMESESSIONIDX]->IsPipelineActive(i))
        {
            result =
                ExtensionModule::GetInstance()->ActivatePipeline(m_pSession[REALTIMESESSIONIDX]->GetSessionHandle(),
                                                                 m_pSession[REALTIMESESSIONIDX]->GetPipelineHandle(i));
            if (CDKResultSuccess == result)
            {
                m_pSession[REALTIMESESSIONIDX]->SetPipelineActivateFlag(i);
            }
        }
    }

    // send two request together
    result = GenerateInternalRequest(REALTIMESESSIONIDX, MAX_REALTIME_PIPELINE, &chiRequest[0]);

    if (NULL != pRequest->settings)
    {
        free_camera_metadata(static_cast<camera_metadata_t*>(multiCamSettings.ppSettings[1]));
        CHX_FREE(multiCamSettings.ppSettings);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::IsRdiStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseMultiVRCamera::IsRdiStream(
        CHISTREAM* pStream) const                ///< Stream to check
{
    BOOL isRDI = FALSE;
    for (UINT32 i = 0; i < MAX_REALTIME_PIPELINE; i++)
    {
        if (m_pRTRDIStream[i] == pStream)
        {
            isRDI = TRUE;
            break;
        }
    }
    return isRDI;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::IsRdiRawStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseMultiVRCamera::IsRdiRawStream(
        CHISTREAM* pStream) const                ///< Stream to check
{
    BOOL isRDI = FALSE;
    for (UINT32 i = 0; i < MAX_REALTIME_PIPELINE; i++)
    {
        if (m_pRTRDISnapshotStream[i] == pStream)
        {
            isRDI = TRUE;
            break;
        }
    }
    return isRDI;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::IsRTPreviewStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseMultiVRCamera::IsRTPreviewStream(
        CHISTREAM* pStream) const                ///< Stream to check
{
    BOOL isRTPreview = FALSE;
    for (UINT32 i = 0; i < MAX_REALTIME_PIPELINE; i++)
    {
        if (m_pRTPreviewStream[i] == pStream)
        {
            isRTPreview = TRUE;
            break;
        }
    }
    return isRTPreview;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::IsRTVamStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseMultiVRCamera::IsRTVamStream(
        CHISTREAM* pStream) const                ///< Stream to check
{
    BOOL isRTVam = FALSE;
    for (UINT32 i = 0; i < MAX_REALTIME_PIPELINE; i++)
    {
        if (m_pRTVamStream[i] == pStream)
        {
            isRTVam = TRUE;
            break;
        }
    }
    return isRTVam;
}

/// UsecaseMultiVRCamera::IsRawSnapshotSupported
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseMultiVRCamera::IsRawSnapshotSupported()
{
    if(NULL != m_pTargetRawSnapshotStream)
    {
        return TRUE;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::IsVamSupported
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseMultiVRCamera::IsVamSupported()
{
    if (m_pTargetVamStream != NULL)
    {
        return TRUE;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseMultiVRCamera::ReleaseReferenceToInputBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseMultiVRCamera::ReleaseReferenceToInputBuffers(
    CHIPRIVDATA* pPrivData)
{
    // A offline request stores input buffers info in privData.
    // When result is received, it should call this API to release reference to those inputs.

    CDKResult result = CDKResultSuccess;

    if (NULL == pPrivData)
    {
        CHX_LOG_ERROR("pPrivData is NULL");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        for(UINT32 i = 0; i < pPrivData->numInputBuffers; i++)
        {
            if ((NULL != pPrivData->bufferManagers[i]) && (NULL != pPrivData->inputBuffers[i].phBuffer))
            {
                reinterpret_cast<CHIBufferManager*>(pPrivData->bufferManagers[i])->ReleaseReference(
                    &(pPrivData->inputBuffers[i]));
                pPrivData->bufferManagers[i]        = NULL;
                pPrivData->inputBuffers[i].phBuffer = NULL;
            }
            else
            {
                CHX_LOG_ERROR("numInputBuffers=%d, pPrivData->bufferManagers[%d]=%p, pPrivData->inputBuffers[%d].phBuffer=%p",
                              pPrivData->numInputBuffers,
                              i, pPrivData->bufferManagers[i],
                              i, pPrivData->inputBuffers[i].phBuffer);
                result = CDKResultEFailed;
            }
        }
    }

    return result;
}
#endif


