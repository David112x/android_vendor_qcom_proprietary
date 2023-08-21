////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxusecasesuperslowmotionfrc.cpp
/// @brief Super Slow Motion FRC Usecase Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chi.h"
#include "chioverride.h"
#include "camxcdktypes.h"
#include "chxusecaseutils.h"

#include "chxusecasesuperslowmotionfrc.h"
#include "chxadvancedcamerausecase.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Class UsecaseSuperSlowMotionFRC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT64 GetTvUs(
    struct timeval* pstTv)
{
    return (pstTv->tv_sec * 1000000ULL) + pstTv->tv_usec;
}

static UINT64 GetTvDiff(
    struct timeval* pstTvEnd,
    struct timeval* pstTvStart)
{
    return GetTvUs(pstTvEnd) - GetTvUs(pstTvStart);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::FRCRequest::PopulateVppBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::FRCRequest::PopulateVppBuffer()
{
    UINT32           pixelSize      = CHIBufferManager::GetBufferSize(&imageBuffer);
    buffer_handle_t* phBufferHandle = CHIBufferManager::GetGrallocHandle(&imageBuffer);
    native_handle_t* handle         = NULL;

    if (NULL != phBufferHandle)
    {
        ChxUtils::Memset(&vppBuffer, 0, sizeof(V1_1::VppBuffer));

        handle                          = const_cast<native_handle*>(*phBufferHandle);

        // Common fields
        vppBuffer.bufferId              = id;
        vppBuffer.flags                 = 0;
        vppBuffer.pvGralloc.setTo(handle);

        // Extra data
        vppBuffer.extradata.handleFd.setTo(nullptr);

        // Pixel
        vppBuffer.pixel.handleFd.setTo(handle);
        vppBuffer.pixel.offset          = 0;
        vppBuffer.pixel.allocLen        = pixelSize;
        vppBuffer.pixel.validDataLen    = vppBuffer.pixel.allocLen - vppBuffer.pixel.offset;

        // Input specific
        if (TRUE == isFRCInput)
        {
            vppBuffer.pixel.filledLen   = pixelSize;
            vppBuffer.timestamp         = timestamp / 1000;
            vppBuffer.flags             |= static_cast<UINT32>(V1_1::VppBufferFlag::VPP_BUFFER_FLAG_READONLY);

            if (TRUE == isEos)
            {
                CHX_LOG("setting EOS on frame_number=%u", srcFrameNumber);
                vppBuffer.flags |= static_cast<UINT32>(V1_1::VppBufferFlag::VPP_BUFFER_FLAG_EOS);
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("unable to get gralloc handle");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UsecaseSuperSlowMotionFRC* UsecaseSuperSlowMotionFRC::Create(
    LogicalCameraInfo*              pCameraInfo,   ///< Camera info
    camera3_stream_configuration_t* pStreamConfig) ///< Stream configuration
{
    CDKResult               result                          = CDKResultSuccess;
    UsecaseSuperSlowMotionFRC* pUsecaseSuperSlowMotionFRC   = CHX_NEW UsecaseSuperSlowMotionFRC;

    if (NULL != pUsecaseSuperSlowMotionFRC)
    {
        result = pUsecaseSuperSlowMotionFRC->Initialize(pCameraInfo, pStreamConfig);

        if (CDKResultSuccess != result)
        {
            pUsecaseSuperSlowMotionFRC->Destroy(FALSE);
            pUsecaseSuperSlowMotionFRC = NULL;
        }
    }

    return pUsecaseSuperSlowMotionFRC;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::Flush()
{
    CHX_LOG("SSM-EVENT: UsecaseSuperSlowMotionFRC::Flush");
    //Pause features to take any actions on flushed buffers

    m_pMutex->Lock();
    while (m_flushingOrSendingBatch == TRUE)
    {
        CHX_LOG("Sending batches in progress, waiting...");
        m_pCond->Wait(m_pMutex->GetNativeHandle());
    }
    m_flushingOrSendingBatch = TRUE;
    m_pMutex->Unlock();

    CameraUsecaseBase::Flush();

    m_pMutex->Lock();
    m_flushingOrSendingBatch = FALSE;
    m_pCond->Broadcast();
    m_pMutex->Unlock();

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::FlushRequestQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::FlushRequestQueue(
    LDLLQueue* pQueue)
{
    FRCRequest* pRequest;
    m_pMutex->Lock();
    while (NULL != (pRequest = static_cast<FRCRequest*>(pQueue->RemoveFromHead())))
    {
        DoneWithFRCRequest_l(pRequest);
        pRequest = NULL;
    }
    m_pMutex->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::Destroy(
    BOOL isForced)
{
    CHX_LOG("SSM-EVENT: UsecaseSuperSlowMotionFRC::Destroy(%u) this=%p, state=%u",
            isForced, this, m_State);

    m_State = SSMState::SSM_DESTROY;

    if (FALSE == isForced)
    {
        WaitForBuffersInCamxStream(TRUE);
        WaitForBuffersInCamxStream(FALSE);
    }

    JoinWorkerThreads();

    DestroyVPP(TRUE);

    FlushRequestQueue(&m_frcPendingInQ);
    FlushRequestQueue(&m_frcProcInQ);
    FlushRequestQueue(&m_frcProcOutQ);
    FlushRequestQueue(&m_frcDoneInQ);
    FlushRequestQueue(&m_frcDoneOutQ);
    FlushRequestQueue(&m_preCapQ);
    FlushRequestQueue(&m_postCapQ);

    while (NULL != m_frcFreeQ.RemoveFromHead())
    {
        // just cleaning up the queue structure
    }

    DumpQueueStates();

    m_pMetadataManager->Release(m_pCommonInputMetadata);

    m_pMetadataManager->UnregisterClient(m_clientId);
    m_clientId = ChiMetadataManager::InvalidClientId;

    if (NULL != m_bufferManager)
    {
        m_bufferManager->Destroy();
        m_bufferManager = NULL;
    }

    if (NULL != m_pMutex)
    {
        m_pMutex->Destroy();
        m_pMutex = NULL;
    }

    if (NULL != m_pCond)
    {
        m_pCond->Destroy();
        m_pCond = NULL;
    }

    CameraUsecaseBase::Destroy(isForced);

    m_pPreviewStream    = NULL;
    m_pVideoStream      = NULL;

    if (NULL != m_pCallbacks)
    {
        CHX_FREE(m_pCallbacks);
        m_pCallbacks = NULL;
    }

    CHX_LOG("UsecaseSuperSlowMotionFRC::Destroy(%u) finished", isForced);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::WaitForBuffersInCamxStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::WaitForBuffersInCamxStream(
    BOOL isPreview)
{
    CDKResult   result      = CDKResultSuccess;
    UINT32*     pQueued     = NULL;
    UINT32*     pReturned   = NULL;
    const CHAR* pStreamName = "";

    if (isPreview)
    {
        pQueued     = &m_camxStats.previewQueued;
        pReturned   = &m_camxStats.previewReturned;
        pStreamName = "preview";
    }
    else
    {
        pQueued     = &m_camxStats.videoQueued;
        pReturned   = &m_camxStats.videoReturned;
        pStreamName = "video";
    }

    m_pMutex->Lock();
    while ((*pQueued) != (*pReturned))
    {
        m_camxStats.DumpState();
        CHX_LOG("Waiting for %s buffers to return from CAMX", pStreamName);
        result = m_pCond->TimedWait(m_pMutex->GetNativeHandle(), CamxWaitForBufferDuration);
        if (CDKResultETimeout == result)
        {
            CHX_LOG_ERROR("CAMX timed out returning %s buffers", pStreamName);
            m_camxStats.DumpState();
            break;
        }
    }
    m_pMutex->Unlock();

    CHX_LOG("Finished waiting for %s buffers to return from CAMX", pStreamName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::IssueGeneratedPartialMeta_l
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::IssueGeneratedPartialMeta_l(
    FRCRequest* pRequest)
{
    if (NULL != pRequest)
    {
        CHX_LOG("frame_number=%u, issuing driver partial result", pRequest->resFrameNumber);

        if (NULL == pRequest->pPartialMetadata)
        {
            pRequest->pPartialMetadata =
                m_pMetadataManager->Get(m_clientId, pRequest->resFrameNumber);
        }

        if (NULL != pRequest->pPartialMetadata)
        {
            ChxUtils::UpdateTimeStamp(pRequest->pPartialMetadata,
                                      pRequest->timestamp,
                                      pRequest->resFrameNumber);

            CHIPARTIALCAPTURERESULT result  = { };
            result.frameworkFrameNum        = pRequest->resFrameNumber;
            result.pPartialResultMetadata   = pRequest->pPartialMetadata->GetHandle();
            result.pPrivData                = GetChiPrivateData(result.frameworkFrameNum);

            SessionPrivateData privData     = { };
            GetSessionPrivateData(&privData);
            CameraUsecaseBase::SessionCbPartialCaptureResult(&result, &privData);

            pRequest->pPartialMetadata->ReleaseReference();
            pRequest->pPartialMetadata      = NULL;
        }
        else
        {
            CHX_LOG_ERROR("frame_number=%u, failed to acquire partial metadata", pRequest->resFrameNumber);
        }
    }
    else
    {
        CHX_LOG_ERROR("can't issue partial for null request!");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::IssueShutterMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::IssueShutterMessage(
    UINT32 frameNum,
    UINT64 timestamp)
{
    CHX_LOG("frame_number=%u: (VIDEO) issuing shutter message, timestamp=%" PRId64,
            frameNum, timestamp);

    CHIMESSAGEDESCRIPTOR descriptor                     = { };

    descriptor.messageType                              = ChiMessageTypeShutter;
    descriptor.pPrivData                                = GetChiPrivateData(frameNum);
    descriptor.message.shutterMessage.frameworkFrameNum = frameNum;
    descriptor.message.shutterMessage.timestamp         = timestamp;

    SessionPrivateData privData                         = { };
    GetSessionPrivateData(&privData);
    CameraUsecaseBase::SessionCbNotifyMessage(&descriptor, &privData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::IssueErrorMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::IssueErrorMessage(
    CHIERRORMESSAGECODE error,
    CHISTREAM*          pStream,
    UINT32              frameNum)
{
    CHX_LOG("frame_number=%u: issuing error message, error=%s",
            frameNum,
            error == MessageCodeDevice ? "MessageCodeDevice" :
            error == MessageCodeRequest ? "MessageCodeRequest" :
            error == MessageCodeResult ? "MessageCodeResult" :
            error == MessageCodeBuffer ? "MessageCodeBuffer" :
            "Unknown");

    CHIMESSAGEDESCRIPTOR descriptor                    = { };

    descriptor.messageType                             = ChiMessageTypeError;

    descriptor.message.errorMessage.frameworkFrameNum  = frameNum;
    descriptor.message.errorMessage.pErrorStream       = pStream;
    descriptor.message.errorMessage.errorMessageCode   = error;

    descriptor.pPrivData                               = GetChiPrivateData(frameNum);

    CameraUsecaseBase::ProcessErrorMessage(&descriptor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::DiscoverVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::DiscoverVendorTag(
    const CHAR* pTagName,
    UINT32*     pTagLoc)
{
    CDKResult   result      = CDKResultSuccess;
    const CHAR* pTagGroup   = "com.qti.chi.superslowmotionfrc";

    if ((NULL == pTagLoc) || (NULL == pTagName))
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        result = m_vendorTagOps.pQueryVendorTagLocation(pTagGroup, pTagName, pTagLoc);
        if (CDKResultSuccess == result)
        {
            CHX_LOG("discovered tag=%s at location=%u", pTagName, *pTagLoc);
        }
        else
        {
            CHX_LOG_ERROR("failed to discover tag: %s, result=%u", pTagName, result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::DiscoverVendorTagLocations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::DiscoverVendorTagLocations()
{
    CDKResult result = CDKResultSuccess;

    result = DiscoverVendorTag("CaptureStart", &m_vendorTags.CaptureStart);

    if (CDKResultSuccess == result)
        result = DiscoverVendorTag("CaptureComplete", &m_vendorTags.CaptureComplete);

    if (CDKResultSuccess == result)
        result = DiscoverVendorTag("ProcessingComplete", &m_vendorTags.ProcessingComplete);

    if (CDKResultSuccess == result)
        result = DiscoverVendorTag("InterpolationFactor", &m_vendorTags.InterpFactor);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::WriteVendorTagInt32
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::WriteVendorTagInt32(
    ChiMetadata*    pMetadata,
    UINT32          tag,
    INT32           val)
{
    CDKResult   result  = CDKResultSuccess;
    INT32       tagVal  = TRUE;

    if (NULL == pMetadata)
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        CHX_LOG("writing vendor tag=%u, val=0x%x", tag, val);
        result = pMetadata->SetTag(tag, &val, 1);
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("failed to write vendor tag=%u, val=0x%x", tag, val);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::FindRequiredStreams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::FindRequiredStreams(
    camera3_stream_configuration_t* pStreamConfig)
{
    CDKResult result = CDKResultSuccess;
    if (m_numExpectedStreams != pStreamConfig->num_streams)
    {
        CHX_LOG_ERROR("unexpected number of streams, got %u", pStreamConfig->num_streams);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < pStreamConfig->num_streams; i++)
        {
            camera3_stream_t* pStream = pStreamConfig->streams[i];
            if (TRUE == IsVideoStream(pStream))
            {
                CHX_LOG("stream[%u]=%p is video stream", i, pStream);
                m_pVideoStream = reinterpret_cast<ChiStream*>(pStream);
            }
            else
            {
                // Assume that it is preview...
                CHX_LOG("stream[%u]=%p is preview stream", i, pStream);
                m_pPreviewStream = reinterpret_cast<ChiStream*>(pStream);
            }
        }

        if ((NULL == m_pVideoStream) || (NULL == m_pPreviewStream))
        {
            CHX_LOG_ERROR("failed to find stream, preview=%p, video=%p", m_pPreviewStream, m_pVideoStream);
            result = CDKResultEInvalidArg;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::ClientOutputThreadShouldSleep_l
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSuperSlowMotionFRC::ClientOutputThreadShouldSleep_l()
{
    BOOL shouldSleep = TRUE;

    if ((TRUE == ShouldExitClientThread_l()) ||
        (TRUE == ShouldErrorClientVideo()) ||
        (TRUE == ShouldCopyInterpolatedOutput()))
    {
        shouldSleep = FALSE;
    }

    return shouldSleep;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::ClientBufferReturnThreadShouldSleep_l
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSuperSlowMotionFRC::ClientBufferReturnThreadShouldSleep_l()
{
    BOOL shouldSleep = TRUE;

    if ((TRUE == ShouldExitClientThread_l()) ||
        (TRUE == ShouldIssueErrorBuffers_l()) ||
        (TRUE == ShouldSendCopiedBuffer_l()))
    {
        shouldSleep = FALSE;
    }

    return shouldSleep;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::HandleErrorClientVideo_l
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::HandleErrorClientVideo_l()
{
    // CaptureRequests should always be sent to this module with two capture
    // targets. There are two main cases where we will error out the video
    // target:
    // (a) always on preview requests
    // (b) on video requests where we do not have an FRC output frame
    //     available. This typically would occur when we're at the beginning
    //     of the stream and FRC has not yet output anything yet.
    //
    // In both cases, we intercept the NOTIFY message from the driver, so
    // we need to update the result here so that the frames are properly
    // returned.

    CHX_LOG("should error client video buffer");
    SSMCaptureRequest* pCr  = reinterpret_cast<SSMCaptureRequest*>(m_errorVideoQ.RemoveFromHead());
    pCr->issueVideoError    = TRUE;

    m_pAppResultMutex->Lock();
    camera3_capture_result_t* pUsecaseResult = GetCaptureResult(pCr->frameIndex);

    camera3_stream_buffer_t* pResBuf =
        const_cast<camera3_stream_buffer_t*>(
            &pUsecaseResult->output_buffers[pUsecaseResult->num_output_buffers]);

    // some basic sanity...
    if (TRUE == pCr->hasClientVidBuf)
    {
        CHX_ASSERT(NULL != pCr->clientVidBuf.pStream);
        CHX_ASSERT(NULL != pCr->clientVidBuf.bufferInfo.phBuffer);
    }

    m_clientStats.videoReturned++;
    m_clientStats.videoErrors++;
    m_clientStats.DumpState();

    CHX_LOG("frame_number=%u: capture_request=%u, numToRet=%u, (VIDEO -> ERROR) [%s], req/ret=(%u/%u/%u/%u)",
            pCr->frameNumber, pCr->captureRequestNum, pUsecaseResult->num_output_buffers,
            pCr->isPreviewRequest ? "preview request" : "frc out not available",
            m_SSMFramesCaptured, m_clientStats.videoReturned,
            m_clientVideoRetSuccess, m_clientStats.videoErrors);

    if (FALSE == pCr->isPreviewRequest)
    {
        // For non-preview requests, SHUTTER message is intercepted and either
        // dropped or deferred. In this case, we need to signal an error to
        // the framework so that it doesn't expect to get these back and so
        // that the buffers can be returned to the surfaces.
        IssueErrorMessage(MessageCodeRequest, NULL, pCr->frameNumber);
    }
    else
    {
        ChxUtils::PopulateChiToHALStreamBuffer(&pCr->clientVidBuf, pResBuf);
        pResBuf->status         = CAMERA3_BUFFER_STATUS_ERROR;
        pResBuf->release_fence  = pResBuf->acquire_fence;
        pResBuf->acquire_fence  = -1;

        pUsecaseResult->num_output_buffers++;

        IssueErrorMessage(MessageCodeBuffer, m_pVideoStream, pCr->frameNumber);
    }

    m_pAppResultMutex->Unlock();

    pCr->clientVidBuf.pStream               = NULL;
    pCr->clientVidBuf.bufferInfo.phBuffer   = NULL;

    if (NULL != pCr->pInputMetadata)
    {
        m_pMetadataManager->Release(pCr->pInputMetadata);
        pCr->pInputMetadata = NULL;
    }

    if (NULL != pCr->pOutputMetadata)
    {
        m_pMetadataManager->Release(pCr->pOutputMetadata);
        pCr->pOutputMetadata = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::HandleCopyInterpolatedOutput_l
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::HandleCopyInterpolatedOutput_l()
{
    CDKResult           result      = CDKResultSuccess;
    FRCRequest*         pRequest    = static_cast<FRCRequest*>(m_frcDoneOutQ.RemoveFromHead());
    SSMCaptureRequest*  pCr         = reinterpret_cast<SSMCaptureRequest*>(m_clientVideoQ.RemoveFromHead());

    CHX_ASSERT(NULL == pCr->imageBuffer.phBuffer);
    CHX_ASSERT(NULL != pCr->clientVidBuf.pStream);
    CHX_ASSERT(NULL != pCr->clientVidBuf.bufferInfo.phBuffer);
    CHX_ASSERT(NULL != pRequest->imageBuffer.phBuffer);

    BOOL isLastFrame            = IsFinishedAllProcessing_l();
    BOOL shouldIssueCapComplete = m_sendCapComplete;
    m_sendCapComplete           = FALSE;

    m_pMutex->Unlock();

    if (TRUE == pCr->isPreviewRequest)
    {
        // This should be impossible
        IssueErrorMessage(MessageCodeBuffer, m_pPreviewStream, pCr->frameNumber);
        CHX_LOG_ERROR("ERROR: copying output into preview request, frame_number=%u",
                      pCr->frameNumber);
    }

    // Adjust timestamps to always be fps distance apart.
    pRequest->timestamp = m_firstFRCTimestamp + (m_buffersReturned * m_videoTsAdjustPeriod);
    m_buffersReturned++;

    pRequest->resFrameNumber = pCr->frameNumber;
    IssueShutterMessage(pRequest->resFrameNumber, pRequest->timestamp);

    IssueGeneratedPartialMeta_l(pRequest);

    if (NULL == pRequest->pOutputMetadata)
    {
        pRequest->pOutputMetadata =
            m_pMetadataManager->Get(m_clientId, pRequest->resFrameNumber);
        if (NULL == pRequest->pOutputMetadata)
        {
            CHX_LOG_ERROR("failed to get new output metadata");
            result = CDKResultENoMemory;
        }
    }

    if (CDKResultSuccess == result)
    {
        ChxUtils::UpdateTimeStamp(pRequest->pOutputMetadata,
                                  pRequest->timestamp,
                                  pRequest->resFrameNumber);
        CHIBufferManager::SetPerfMode(&pCr->clientVidBuf.bufferInfo);

        struct timeval start;
        struct timeval end;
        gettimeofday(&start, NULL);
        result = CHIBufferManager::CopyBuffer(&pRequest->imageBuffer,
                                              &pCr->clientVidBuf.bufferInfo);
        gettimeofday(&end, NULL);
        UINT64 diff             = GetTvDiff(&end, &start);
        UINT64 tsDelta          = pRequest->timestamp - m_lastCopiedTimestamp;
        m_lastCopiedTimestamp   = pRequest->timestamp;
        CHX_LOG("copying frc output from frame_number=%u to frame_number=%u, ts=%" PRId64 ", delta=%" PRId64 ", duration=%" PRIu64,
                pRequest->srcFrameNumber, pRequest->resFrameNumber, pRequest->timestamp, tsDelta, diff);

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("failed to copy frc output to client buffer, result=%u", result);
        }
    }

    if (CDKResultSuccess == result)
    {
        // The buffer has been successfully copied, so no more need for
        // this internal buffer. Free it up to the buffer pool.
        ReleaseChiVideoBuffer(&pRequest->imageBuffer);

        if (TRUE == shouldIssueCapComplete)
        {
            CHX_LOG("issuing cap complete: pRequest=%p, pMetadata=%p",
                    pRequest, pRequest->pOutputMetadata);
            WriteVendorTagInt32(pRequest->pOutputMetadata,
                                m_vendorTags.CaptureComplete, TRUE);
        }

        if (TRUE == isLastFrame)
        {
            CHX_LOG("SSM-EVENT: processing last frc output buffer");
            m_pMutex->Lock();
            DumpQueueStates();
            m_pMutex->Unlock();
            WriteVendorTagInt32(pRequest->pOutputMetadata,
                                m_vendorTags.ProcessingComplete, TRUE);
        }

        m_pMutex->Lock();
        // Transfer ownership of metadata to SSMCaptureRequest.
        pCr->pOutputMetadata      = pRequest->pOutputMetadata;
        pRequest->pOutputMetadata = NULL;

        CHX_LOG("frame_number=%u: done with request", pRequest->resFrameNumber);
        DoneWithFRCRequest_l(pRequest);
        m_copiedClientVideoQ.InsertToTail(pCr);
    }

    if (CDKResultSuccess != result)
    {
        m_pMutex->Lock();
        DoneWithFRCRequest_l(pRequest);
        m_errorVideoQ.InsertToTail(pCr);
        m_pCond->Broadcast();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::HandleReleaseDeferredBuffer_l
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::HandleReleaseDeferredBuffer_l()
{
    SSMCaptureRequest* pCr =
        reinterpret_cast<SSMCaptureRequest*>(m_copiedClientVideoQ.RemoveFromHead());

    m_clientStats.videoReturned++;
    m_clientStats.DumpState();
    m_clientVideoRetSuccess++;
    CHX_LOG("issuing result for frame_number=%u, req/ret=(%u/%u/%u/%u)",
            pCr->frameNumber, m_SSMFramesCaptured, m_clientStats.videoReturned,
            m_clientVideoRetSuccess, m_clientStats.videoErrors);
    m_pMutex->Unlock();

    // Issue metadata (if available) and output frame together
    CHICAPTURERESULT result     = { };
    result.frameworkFrameNum    = pCr->frameNumber;
    result.pInputBuffer         = NULL;
    result.pPrivData            = NULL;
    result.numOutputBuffers     = 1;
    result.pOutputBuffers       = &pCr->clientVidBuf;
    result.pInputMetadata       = pCr->pInputMetadata->GetHandle();
    result.pOutputMetadata      = pCr->pOutputMetadata->GetHandle();
    result.numPartialMetadata   =
        static_cast<int>(ChxUtils::GetPartialResultCount(
            PartialResultSender::DriverMetaData));

    SessionPrivateData privData = { };
    GetSessionPrivateData(&privData);

    CameraUsecaseBase::SessionCbCaptureResult(&result, &privData);

    m_pMutex->Lock();

    if ((TRUE == IsFinishedAllProcessing_l()) && (0 == m_copiedClientVideoQ.NumNodes()))
    {
        CHX_LOG("SSM-EVENT: finished sending all buffers back to FW, resetting...");
        ResetForNewRecording_l();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::ResetForNewRecording_l
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::ResetForNewRecording_l()
{
    CDKResult result = CDKResultSuccess;

    if (CDKResultSuccess == result)
    {
        CHX_LOG("Doing VPP Destroy");
        // FALSE because already locked
        result = DestroyVPP(FALSE);
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("DestroyVPP() failed, result=%u", result);
        }
    }

    if (CDKResultSuccess == result)
    {
        CHX_LOG("Resetting VPP Manager");
        m_VppManager.Reset();
        m_VppManager.outCountMax = MaxBufsToFRCOut;
    }

    if (CDKResultSuccess == result)
    {
        CHX_LOG("Doing VPP Init");
        result = InitializeVPP();
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("InitializeVPP() failed, result=%u", result);
        }
    }

    if (CDKResultSuccess == result)
    {
        CHX_LOG("Queueing buffers to VPP after VPP Init");
        result = QueueBuffersToVpp();
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("failed to queue buffers to vpp, result=%u", result);
        }
    }

    m_State                     = SSMState::SSM_BYPASS;
    m_SSMFramesCaptured         = 0;
    m_buffersReturned           = 0;
    m_postCapCfg.captureReqCnt  = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::RunClientOutputThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* UsecaseSuperSlowMotionFRC::RunClientOutputThread()
{
    while (TRUE)
    {
        m_pMutex->Lock();
        if (TRUE == ClientOutputThreadShouldSleep_l())
        {
            CHX_LOG("sleeping worker thread");
            m_pCond->Wait(m_pMutex->GetNativeHandle());
        }

        DumpQueueStates();

        if (TRUE == ShouldExitClientThread_l())
        {
            CHX_LOG("SSM-EVENT: RunClientOutputThread() breaking loop");
            m_pMutex->Unlock();
            break;
        }

        if (TRUE == ShouldErrorClientVideo())
        {
            SSMCaptureRequest* pCr =
                reinterpret_cast<SSMCaptureRequest*>(m_clientVideoQ.RemoveFromHead());
            m_errorVideoQ.InsertToTail(pCr);
            m_pCond->Broadcast();
        }
        else if (TRUE == ShouldCopyInterpolatedOutput())
        {
            HandleCopyInterpolatedOutput_l();
        }

        m_pMutex->Unlock();
    }

    CHX_LOG("SSM-EVENT: RunClientOutputThread() exiting");
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::RunClientBufferReturnThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* UsecaseSuperSlowMotionFRC::RunClientBufferReturnThread()
{
    ATRACE_BEGIN("UsecaseSuperSlowMotionFRC::RunClientBufferReturnThread");
    while (TRUE)
    {
        m_pMutex->Lock();
        if (TRUE == ClientBufferReturnThreadShouldSleep_l())
        {
            CHX_LOG("sleeping worker thread");
            m_pCond->Wait(m_pMutex->GetNativeHandle());
        }

        DumpQueueStates();

        if (TRUE == ShouldExitClientThread_l())
        {
            CHX_LOG("SSM-EVENT: RunClientBufferReturnThread() breaking loop");
            m_pMutex->Unlock();
            break;
        }

        if (SSMState::SSM_PROCESS == m_State)
        {
            m_pMutex->Unlock();
            QueueBuffersToVpp();
            m_pMutex->Lock();
        }

        if (TRUE == ShouldIssueErrorBuffers_l())
        {
            HandleErrorClientVideo_l();
        }

        while (TRUE == ShouldSendCopiedBuffer_l())
        {
            HandleReleaseDeferredBuffer_l();
        }

        m_pMutex->Unlock();

        // Once client stops sending buffers, it is possible that the last
        // batch will get stuck waiting to be released since typically either
        // PREVIEW's ProcessResult(), or a copied video buffer will trigger
        // ProcessAndReturnFinishedResults().
        ProcessAndReturnFinishedResults();
    }

    ATRACE_END();

    CHX_LOG("SSM-EVENT: RunClientBufferReturnThread() exiting");
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::LaunchWorkerThreads
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::LaunchWorkerThreads()
{
    m_exitThreads                           = FALSE;
    m_clientOutputThread.pPrivateData       = this;
    m_clientBufferRetThread.pPrivateData    = this;

    ChxUtils::ThreadCreate(UsecaseSuperSlowMotionFRC::ClientOutputThread,
                           &m_clientOutputThread,
                           &m_clientOutputThread.hThreadHandle);

    ChxUtils::ThreadCreate(UsecaseSuperSlowMotionFRC::ClientBufferReturnThread,
                           &m_clientBufferRetThread,
                           &m_clientBufferRetThread.hThreadHandle);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::JoinWorkerThreads
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::JoinWorkerThreads()
{
    m_pMutex->Lock();
    m_exitThreads = TRUE;
    m_pCond->Broadcast();
    m_pMutex->Unlock();
    ChxUtils::ThreadTerminate(m_clientOutputThread.hThreadHandle);
    ChxUtils::ThreadTerminate(m_clientBufferRetThread.hThreadHandle);
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::ClientOutputThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* UsecaseSuperSlowMotionFRC::ClientOutputThread(
    VOID* pThreadData)
{
    PerThreadData*              pThread = reinterpret_cast<PerThreadData*>(pThreadData);
    UsecaseSuperSlowMotionFRC*  pSSM    = reinterpret_cast<UsecaseSuperSlowMotionFRC*>(pThread->pPrivateData);
    return pSSM->RunClientOutputThread();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::ClientBufferReturnThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* UsecaseSuperSlowMotionFRC::ClientBufferReturnThread(
    VOID* pThreadData)
{
    PerThreadData*              pThread = reinterpret_cast<PerThreadData*>(pThreadData);
    UsecaseSuperSlowMotionFRC*  pSSM    = reinterpret_cast<UsecaseSuperSlowMotionFRC*>(pThread->pPrivateData);
    return pSSM->RunClientBufferReturnThread();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::Initialize(
    LogicalCameraInfo*              pCameraInfo,   ///< Camera info
    camera3_stream_configuration_t* pStreamConfig) ///< Stream configuration
{
    ATRACE_BEGIN("UsecaseSuperSlowMotionFRC::Initialize");
    CDKResult result                = CDKResultSuccess;

    // this isn't really useful since if we're here, we must be SSM
    m_usecaseId                     = UsecaseId::SuperSlowMotionFRC;
    m_cameraId                      = pCameraInfo->cameraId;

    m_debugLastResultFrameNumber    = static_cast<UINT32>(-1);

    m_State                         = SSMState::SSM_BYPASS;
    m_SSMFramesCaptured             = 0;
    m_buffersReturned               = 0;
    m_flushingOrSendingBatch        = FALSE;

    ExtensionModule::GetInstance()->GetVendorTagOps(&m_vendorTagOps);
    CHX_LOG("pGetMetaData:%p, pSetMetaData:%p", m_vendorTagOps.pGetMetaData, m_vendorTagOps.pSetMetaData);

    result = DiscoverVendorTagLocations();
    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("failed to discover vendor tag locations, result=%u", result);
    }

    if (CDKResultSuccess == result)
    {
        result = FindRequiredStreams(pStreamConfig);
        if (CDKResultSuccess != result)
        {
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        m_pMutex = Mutex::Create();
        m_pCond  = Condition::Create();
        if ((NULL == m_pMutex) || (NULL == m_pCond))
        {
            CHX_LOG_ERROR("Failed to initialize mutex=%p or cond=%p", m_pMutex, m_pCond);
            result = CDKResultENoMemory;
        }
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < MaxOutstandingFRCRequests; i++)
        {
            m_FRCRequests[i].pData = NULL;
            m_FRCRequests[i].pPrev = NULL;
            m_FRCRequests[i].pNext = NULL;
            m_FRCRequests[i].id    = GetNewFRCRequestId();
            m_frcFreeQ.InsertToTail(&m_FRCRequests[i]);
        }
    }

    if (CDKResultSuccess == result)
    {
        result = LaunchWorkerThreads();
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Failed to launch worker threads");
        }
    }

    if (CDKResultSuccess == result)
    {
        mWidth          = m_pVideoStream->width;
        mHeight         = m_pVideoStream->height;
        mVppColorFormat = GetVppColorFormatFromGrallocUsage(m_pVideoStream->grallocUsage);

        CHIBufferManagerCreateData createData = { };

        createData.width                = mWidth;
        createData.height               = mHeight;
        createData.format               = GetChiStreamFormatFromGrallocUsage(m_pVideoStream->grallocUsage);
        createData.producerFlags        = GRALLOC_USAGE_HW_VIDEO_ENCODER;
        createData.consumerFlags        = GRALLOC_USAGE_HW_VIDEO_ENCODER;
        createData.maxBufferCount       = MaxOutstandingFRCRequests;
        createData.immediateBufferCount = MaxOutstandingFRCRequests;
        createData.bufferHeap           = BufferHeapEGL;
        createData.pChiStream           = m_pVideoStream;

        CHX_LOG("allocating buffer pool: w=%u, h=%u, fmt=0x%x",
                createData.width, createData.height, createData.format);
        m_bufferManager = CHIBufferManager::Create("SlowMotion Pool", &createData);
        if (NULL == m_bufferManager)
        {
            CHX_LOG_ERROR("Failed to initialize buffer pool");
            result = CDKResultENoMemory;
        }
    }

    if (CDKResultSuccess == result)
    {
        m_VppManager.Reset();
        m_VppManager.outCountMax = MaxBufsToFRCOut;

        m_Fps                    = ExtensionModule::GetInstance()->GetUsecaseMaxFPS();
        m_videoTsAdjustPeriod    = 1e9 / DefaultVideoFPS;

        m_preCapCfg.Configure(m_Fps, DefaultVideoFPS, DefaultPreCapDuration);
        m_postCapCfg.Configure(m_Fps, DefaultVideoFPS, DefaultPostCapDuration);

        CHX_LOG("UsecaseSuperSlowMotionFRC::Initialize, usecaseId:%d, maxFps=%u",
                m_usecaseId, m_Fps);
        CHX_LOG("CHI Input Stream Configs:");

        for (UINT stream = 0; stream < pStreamConfig->num_streams; stream++)
        {
            CHX_LOG("\tstream = %p streamType = %d streamFormat = %d streamWidth = %d streamHeight = %d",
                pStreamConfig->streams[stream],
                pStreamConfig->streams[stream]->stream_type,
                pStreamConfig->streams[stream]->format,
                pStreamConfig->streams[stream]->width,
                pStreamConfig->streams[stream]->height);
        }
    }

    if (CDKResultSuccess == result)
    {
        // This must be done before queuing buffers to VPP since metadata buffers
        // are associated with an FRC request before buffers are queued to VPP.
        result = CreateMetadataManager(m_cameraId, false, NULL, false);
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("failed to create metadata manager");
        }
    }

    if (CDKResultSuccess == result)
    {
        result = SelectUsecaseConfig(pCameraInfo, pStreamConfig);

        if ((NULL != m_pChiUsecase) && (CDKResultSuccess == result))
        {
            CHX_LOG("Usecase %s selected, pipelines=%u",
                    m_pChiUsecase->pUsecaseName, m_pChiUsecase->numPipelines);

            m_pCallbacks = static_cast<ChiCallBacks*>(
                CHX_CALLOC(sizeof(ChiCallBacks) * m_pChiUsecase->numPipelines));

            if (NULL != m_pCallbacks)
            {
                for (UINT i = 0; i < m_pChiUsecase->numPipelines; i++)
                {
                    m_pCallbacks[i].ChiNotify                      = UsecaseSuperSlowMotionFRC::ProcessMessageCb;
                    m_pCallbacks[i].ChiProcessCaptureResult        = UsecaseSuperSlowMotionFRC::ProcessResultCb;
                    m_pCallbacks[i].ChiProcessPartialCaptureResult =
                        UsecaseSuperSlowMotionFRC::ProcessDriverPartialCaptureResultCb;
                }

                result = CameraUsecaseBase::Initialize(m_pCallbacks);
            }
            PostUsecaseCreation(pStreamConfig);
        }
        else
        {
            CHX_LOG_ERROR("failed to select usecase");
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        // Allocate *2 for additional partial metadata, +1 for the 1 input metadata we have
        const PipelineData* pPipelineData = GetPipelineData(m_sessionId, m_pipelineId);

        if (NULL != pPipelineData)
        {
            m_clientId = m_pMetadataManager->RegisterClient(true,
                                                            pPipelineData->pPipeline->GetTagList(),
                                                            pPipelineData->pPipeline->GetTagCount(),
                                                            pPipelineData->pPipeline->GetPartialTagCount(),
                                                            (MaxOutstandingFRCRequests * 2) + 1,
                                                            ChiMetadataUsage::RealtimeOutput);
            if (ChiMetadataManager::InvalidClientId == m_clientId)
            {
                CHX_LOG_ERROR("failed to register client");
                result = CDKResultEFailed;
            }
        }
        else
        {
            CHX_LOG_ERROR("Invalid Pipeline data");
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        m_pCommonInputMetadata = m_pMetadataManager->Get(m_clientId, 0);
        if (NULL == m_pCommonInputMetadata)
        {
            CHX_LOG_ERROR("failed to get metadata");
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        result = InitializeVPP();
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("failed to initialize vpp, result=%u", result);
        }
    }

    if (CDKResultSuccess == result)
    {
        result = QueueBuffersToVpp();
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("failed to queue initial buffers to vpp, result=%u", result);
        }
    }

    if (CDKResultSuccess == result)
    {
        CHX_LOG("maxNumBuffers for video=%u, preview=%u",
                m_pVideoStream->maxNumBuffers, m_pPreviewStream->maxNumBuffers);
    }

    m_numBatchedFrames = ExtensionModule::GetInstance()->GetNumBatchedFrames();
    m_captureRequestTracker.Initialize(m_numBatchedFrames);

    ATRACE_END();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::SelectUsecaseConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::SelectUsecaseConfig(
    LogicalCameraInfo*              pCameraInfo,   ///< Camera info
    camera3_stream_configuration_t* pStreamConfig)  ///< Stream configuration
{
    CDKResult result = CDKResultSuccess;

    // We want CAMX to think that this is just normal high speed video
    UINT32 originalMode             = pStreamConfig->operation_mode;
    pStreamConfig->operation_mode   = StreamConfigModeConstrainedHighSpeed;

    {
        CHX_LOG("Initializing using default usecase matching");
        m_cameraId          = pCameraInfo->ppDeviceInfo[0]->cameraId;
        m_pChiUsecase       = UsecaseSelector::DefaultMatchingUsecase(pStreamConfig);
        UINT* cameraIds     = NULL;
        UINT  pipelineNum;
        if (NULL != m_pChiUsecase)
        {
            pipelineNum = m_pChiUsecase->numPipelines;
            cameraIds   = static_cast<UINT*>(CHX_CALLOC(sizeof(UINT) * pipelineNum));
            if (NULL != cameraIds)
            {
                for (UINT i = 0; i < pipelineNum; i++)
                {
                    cameraIds[i] = m_cameraId;
                }
                result      = SetPipelineToCameraMapping(pipelineNum, cameraIds);
                CHX_FREE(cameraIds);
                cameraIds   = NULL;
            }
            else
            {
                CHX_LOG("No memory allocated for cameraIds");
                result = CDKResultENoMemory;
            }
        }
        else
        {
            CHX_LOG("No proper usecase selected");
            result = CDKResultEFailed;
        }
    }

    pStreamConfig->operation_mode = originalMode;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::PipelineCreated
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::PipelineCreated(
    UINT sessionId,    ///< Id of session created
    UINT pipelineId)   ///< Pipeline of session create
{
    CHX_LOG("PipelineCreated: sessionId=%u, pipelineId=%u", sessionId, pipelineId);
    m_sessionId     = sessionId;
    m_pipelineId    = pipelineId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::PostUsecaseCreation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::PostUsecaseCreation(
    camera3_stream_configuration_t* pStreamConfig)  ///< Stream configuration
{
    (void)pStreamConfig;
    CDKResult result = CDKResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::SetInterpFactorIfAllowed
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::SetInterpFactorIfAllowed(
    camera3_capture_request_t* pRequest)
{
    CDKResult   result              = CDKResultSuccess;
    UINT32      clientInterpFactor;

    if ((TRUE == m_VppManager.clientSetInterp) ||
        (NULL == pRequest->settings) ||
        (SSMState::SSM_BYPASS != m_State))
    {
        // Invalid to change interp factor, so just return success without
        // actually doing anything.
        result = CDKResultSuccess;
    }
    else
    {
        result = m_vendorTagOps.pGetMetaData(
            reinterpret_cast<VOID*>(const_cast<camera_metadata_t*>(pRequest->settings)),
            m_vendorTags.InterpFactor, &clientInterpFactor, sizeof(UINT32));
        if (CDKResultSuccess != result)
        {
            // client didn't send a request
            result = CDKResultSuccess;
        }
        else
        {
            result = ConfigureVPP(clientInterpFactor);
            if (CDKResultSuccess == result)
            {
                m_VppManager.clientSetInterp = TRUE;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::ShouldStartRecording
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL UsecaseSuperSlowMotionFRC::ShouldStartRecording(
    camera3_capture_request_t* pRequest)
{
    BOOL    shouldStartRecording    = FALSE;
    UINT32  captureStart            = 0;

    if ((NULL != pRequest) && (SSMState::SSM_BYPASS == m_State))
    {
        CDKResult result = m_vendorTagOps.pGetMetaData(
            reinterpret_cast<VOID*>(const_cast<camera_metadata_t*>(pRequest->settings)),
            m_vendorTags.CaptureStart, &captureStart, sizeof(UINT32));
        if (CDKResultSuccess != result)
        {
            // client didn't send this metadata
            captureStart = 0;
        }

        if (0 != captureStart)
        {
            shouldStartRecording = TRUE;
        }
    }

    return shouldStartRecording;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::UpdateHDRModes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::UpdateHDRModes(
    camera3_capture_request_t* pRequest)
{
    CDKResult result = UpdateFeatureModeIndex(const_cast<camera_metadata_t*>(pRequest->settings));
    if (TRUE == ChxUtils::AndroidMetadata::IsVendorTagPresent(reinterpret_cast<const VOID*>(pRequest->settings),
            VendorTag::VideoHDR10Mode))
    {
        VOID* pData = NULL;
        StreamHDRMode HDRMode = StreamHDRMode::HDRModeNone;
        ChxUtils::AndroidMetadata::GetVendorTagValue(reinterpret_cast<const VOID*>(pRequest->settings),
            VendorTag::VideoHDR10Mode, reinterpret_cast<VOID**>(&pData));
        if (NULL != pData)
        {
            HDRMode = *(static_cast<StreamHDRMode*>(pData));
            if (StreamHDRMode::HDRModeHDR10 == HDRMode)
            {
                m_tuningFeature2Value = static_cast<UINT32>(ChiModeFeature2SubModeType::HDR10);
            }
            else if (StreamHDRMode::HDRModeHLG == HDRMode)
            {
                m_tuningFeature2Value = static_cast<UINT32>(ChiModeFeature2SubModeType::HLG);
            }
            else
            {
                m_tuningFeature2Value = 0;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::ExecuteCaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::ExecuteCaptureRequest(
    camera3_capture_request_t* pRequest)
{
    CDKResult           result          = CDKResultSuccess;
    UINT                frameIndex      = pRequest->frame_number % MaxOutstandingRequests;
    BOOL                bIsFirstRequest = FALSE;
    SSMCaptureRequest*  pCr;

    CHX_LOG("UsecaseSuperSlowMotionFRC::ExecuteCaptureRequest %u %u %u",
            pRequest->frame_number, frameIndex, pRequest->num_output_buffers);

    if (SSMState::SSM_DESTROY == m_State)
    {
        CHX_LOG_ERROR("Can't execute capture request during usecase destroy");
        result = CDKResultEInvalidState;
    }

    result          = m_captureRequestTracker.ValidateAndUpdate(pRequest);
    bIsFirstRequest = m_captureRequestTracker.IsFirstRequestInBatch(pRequest);
    if ((CDKResultSuccess == result) && (TRUE == bIsFirstRequest))
    {
        result = SetInterpFactorIfAllowed(pRequest);
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("failed to set client interp factor, res=%u", result);
        }

        if (CDKResultSuccess == result)
        {
            result = UpdateHDRModes(pRequest);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("failed to update HDR modes, res=%u", result);
            }
        }

        if ((CDKResultSuccess == result) && (TRUE == ShouldStartRecording(pRequest)))
        {
            m_State = SSMState::SSM_RECORD;
            CHX_LOG("SSM-EVENT: changing state to RECORD");
        }
    }

    // Extract the relevant information from the original request first
    CHISTREAMBUFFER                 clientVidBuf    = {0};
    const camera3_stream_buffer_t*  pClientPreview  = NULL;
    const camera3_stream_buffer_t*  pClientVideo    = NULL;

    FindBuffersInRequest(pRequest, &pClientPreview, &pClientVideo);

    if (CDKResultSuccess == result)
    {
        if (NULL != pClientVideo)
        {
            CHX_LOG("frame_number=%u, pClientVideo=%p, populating clientVidBuf structure",
                    pRequest->frame_number, pClientVideo);
            ChxUtils::PopulateHALToChiStreamBuffer(pClientVideo, &clientVidBuf);
        }

        m_pMutex->Lock();
        m_clientStats.videoQueued   += (NULL != pClientVideo) ? 1 : 0;
        m_clientStats.previewQueued += (NULL != pClientPreview) ? 1 : 0;
        m_pMutex->Unlock();
    }

    // Handle request based on its position within a batch
    if ((CDKResultSuccess == result) && (FALSE == bIsFirstRequest))
    {
        pCr                     = GetSSMCaptureRequest(pRequest->frame_number);
        pCr->hasClientVidBuf    = TRUE;
        pCr->clientVidBuf       = clientVidBuf;
        pCr->pInputMetadata     = m_pCommonInputMetadata;
        pCr->pInputMetadata->AddReference();

        CHX_LOG("frame_number=%u, received client video buffer", pRequest->frame_number);

        m_pMutex->Lock();
        m_clientVideoQ.InsertToTail(pCr);
        m_pCond->Broadcast();
        m_pMutex->Unlock();
    }
    else if (CDKResultSuccess == result)
    {
        m_pMutex->Lock();
        while (m_flushingOrSendingBatch == TRUE)
        {
            CHX_LOG("frame_number=%u, flush in progress, so waiting...", pRequest->frame_number);
            m_pCond->Wait(m_pMutex->GetNativeHandle());
        }
        m_flushingOrSendingBatch = TRUE;
        m_pMutex->Unlock();

        // First request in the batch from the client. Generate a complete
        // batch to CAMX.
        UINT32 baseFrameNumber  = pRequest->frame_number;

        for (UINT32 currentFrameNum = baseFrameNumber;
             ((currentFrameNum < baseFrameNumber + m_numBatchedFrames) &&
              (CDKResultSuccess == result));
              currentFrameNum++)
        {
            BOOL bIsFirstRequestInBatch = (currentFrameNum == baseFrameNumber);
            frameIndex                  = currentFrameNum % MaxOutstandingRequests;

            camera3_capture_result_t* pUsecaseResult    = GetCaptureResult(frameIndex);
            pUsecaseResult->result                      = NULL;
            pUsecaseResult->frame_number                = currentFrameNum;
            pUsecaseResult->num_output_buffers          = 0;

            // We always replace the client video buffer with our own.
            pCr                     = GetSSMCaptureRequest(currentFrameNum);
            pCr->Reset();
            pCr->frameNumber        = currentFrameNum;
            pCr->frameIndex         = frameIndex;
            pCr->captureRequestNum  = m_camxStats.captureRequestCount;
            pCr->isPreviewRequest   = FALSE;
            pCr->imageBuffer        = m_bufferManager->GetImageBufferInfo();
            pCr->stateRequested     = m_State;
            pCr->destination        = SSMDestination::DROP;
            pCr->pInputMetadata     = NULL;
            pCr->camxOwnsVidBuf     = TRUE;

            if (NULL == pCr->imageBuffer.phBuffer)
            {
                CHX_LOG_ERROR("ERROR: base_number=%u, frame_number=%u, null handle for pImageBuffer=%p",
                              baseFrameNumber, pCr->frameNumber, pCr->imageBuffer.phBuffer);
                result = CDKResultENoMemory;
                break;
            }

            if (TRUE == bIsFirstRequestInBatch && NULL != pClientVideo)
            {
                CHX_LOG("frame_number=%u, first request in the batch!", currentFrameNum);
                pCr->hasClientVidBuf    = TRUE;
                pCr->clientVidBuf       = clientVidBuf;
                pCr->pInputMetadata     = m_pCommonInputMetadata;
                pCr->pInputMetadata->AddReference();
            }

            CHX_LOG("SSMCaptureRequest, base_number=%u, generating frame_number=%u",
                    baseFrameNumber, pCr->frameNumber);

            // Every request that is sent to the driver contains a video
            // buffer. Determine what to do with that video buffer once we get
            // it back from the driver.
            if ((SSMState::SSM_BYPASS == m_State) &&
                (TRUE == m_preCapCfg.IsEnabled()) &&
                (0 == (m_camxStats.captureRequestCount % m_preCapCfg.sampleRate)))
            {
                pCr->destination    = SSMDestination::PRE_CAP;
            }

            if (SSMState::SSM_RECORD == m_State)
            {
                if (m_SSMFramesCaptured < MaxSSMFramesToCapture)
                {
                    m_SSMFramesCaptured++;
                    pCr->destination = SSMDestination::FRC_IN;
                    if (m_SSMFramesCaptured == MaxSSMFramesToCapture)
                    {
                        m_lastSSMCapReq = m_camxStats.captureRequestCount;
                        pCr->isEos      = TRUE;
                        if (FALSE == m_postCapCfg.IsEnabled())
                        {
                            m_State     = SSMState::SSM_RECORD_TO_PROCESS;
                            CHX_LOG("SSM-EVENT: post-cap disabled, state to RECORD_TO_PROCESS");
                        }
                        else
                        {
                            CHX_LOG("SSM-EVENT: %u SSM rq's, starting post-cap", m_SSMFramesCaptured);
                        }
                    }
                }
                else if ((TRUE == m_postCapCfg.IsEnabled()) &&
                         (0 == ((m_camxStats.captureRequestCount - m_lastSSMCapReq) % m_postCapCfg.sampleRate)))
                {
                    pCr->destination = SSMDestination::POST_CAP;
                    m_postCapCfg.captureReqCnt++;
                    if (m_postCapCfg.captureReqCnt == m_postCapCfg.numFrames)
                    {
                        pCr->isEos  = TRUE;
                        m_State     = SSMState::SSM_RECORD_TO_PROCESS;
                        CHX_LOG("SSM-EVENT: %u post-cap rq's, state to RECORD_TO_PROCESS", m_postCapCfg.numFrames);
                    }
                }
            }

            // For every capture request, we override the FW video buffer with
            // our own that we will queue to FRC.
            UINT32                  outIdx                          = 0;
            camera3_stream_buffer_t outBufs[m_numExpectedStreams]   = { { 0 } };

            camera3_stream_buffer_t* pVideoBuf = &outBufs[outIdx++];
            pVideoBuf->stream                  = reinterpret_cast<camera3_stream_t*>(m_pVideoStream);
            pVideoBuf->buffer                  = CHIBufferManager::GetGrallocHandle(&pCr->imageBuffer);
            pVideoBuf->status                  = CAMERA3_BUFFER_STATUS_OK;
            pVideoBuf->acquire_fence           = -1;
            pVideoBuf->release_fence           = -1;

            if (NULL != pClientPreview)
            {
                if (TRUE == bIsFirstRequestInBatch)
                {
                    pCr->isPreviewRequest   = TRUE;
                    outBufs[outIdx++]       = *pClientPreview;
                }

                m_pMutex->Lock();
                m_camxStats.videoQueued++;
                m_camxStats.previewQueued += (pCr->isPreviewRequest) ? 1 : 0;
                m_pMutex->Unlock();

                // Submit the new request
                pRequest->num_output_buffers    = outIdx;
                pRequest->output_buffers        = outBufs;
                pRequest->frame_number          = currentFrameNum;

                CHX_LOG("base_number=%u, frame_number=%u: capture_request=%u, numToRet=%u, (%s) (VIDEO -> %s)",
                        baseFrameNumber, pCr->frameNumber, pCr->captureRequestNum, pRequest->num_output_buffers,
                        pCr->isPreviewRequest ? "PREVIEW -> HAL" : "NO PREVIEW",
                        GetDestinationString(pCr->destination));

                m_camxStats.captureRequestCount++;
                result = CameraUsecaseBase::ExecuteCaptureRequest(pRequest);

                // Send the client buffer to be copied if it's ready. The copy thread
                // will handle returning the buffer to the client (or erroring it out).
                // This needs to be handled after ExecuteCaptureRequest() has been run
                // on the base class since it sets m_isMetadataSent and
                // m_isMetadataAvailable, which is also set by the worker thread if
                // it needs to bypass the buffer.
                m_pMutex->Lock();
                if (TRUE == pCr->isPreviewRequest)
                {
                    // Always true in the first request of a batch, since it is a
                    // requirement that the client sends P in the leading capture
                    // request in a batch.
                    AppendPendingPreviewQueue_l(pRequest->frame_number);
                }
                if (TRUE == pCr->hasClientVidBuf)
                {
                    // True only if the client sent the video buffer in the leading
                    // request. This is not always the case since P and V don't
                    // need to come together in the first request.
                    m_clientVideoQ.InsertToTail(pCr);
                }
                m_pCond->Broadcast();
                m_pMutex->Unlock();
            }
            else
            {
                result = CDKResultEInvalidArg;
                CHX_LOG_ERROR("Invalid arguments: NO PREVIEW");
            }
        }

        m_pMutex->Lock();
        m_flushingOrSendingBatch = FALSE;
        m_pCond->Broadcast();
        m_pMutex->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::DumpCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::DumpCaptureResult(
    CHICAPTURERESULT*   pResult,
    const char*         pcStr)
{
    if (NULL != pResult)
    {
        if (NULL == pcStr)
            pcStr = "";

        CHX_LOG("DBG: %s: fnum=%u, meta=%p, numOut=%u, out=%p, inp=%p, partial=%u, priv=%p, inmeta=%p, outmeta=%p",
                pcStr,
                pResult->frameworkFrameNum,
                pResult->pResultMetadata,
                pResult->numOutputBuffers,
                pResult->pOutputBuffers,
                pResult->pInputBuffer,
                pResult->numPartialMetadata,
                pResult->pPrivData,
                pResult->pInputMetadata,
                pResult->pOutputMetadata);

        for (UINT32 i = 0; i < pResult->numOutputBuffers; i++)
        {
            const CHISTREAMBUFFER* pBuf = &pResult->pOutputBuffers[i];
            CHX_LOG("\tout[%u] stream=%p, buffer=%p, status=%d, acFen={v=%u, t=%u, fd=%d}, relFen={v=%u, t=%u, fd=%d}",
                    i, pBuf->pStream, pBuf->bufferInfo.phBuffer, pBuf->bufferStatus,
                    pBuf->acquireFence.valid, pBuf->acquireFence.type, pBuf->acquireFence.nativeFenceFD,
                    pBuf->releaseFence.valid, pBuf->releaseFence.type, pBuf->releaseFence.nativeFenceFD);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::ProcessResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::ProcessResult(
    CHICAPTURERESULT*   pResult,
    VOID*               pPrivateCallbackData)
{
    CHX_ASSERT(pResult != NULL);

    CHX_LOG("frame_number=%u, outputs=%u, pInputMetadata=%p, pOutputMetadata=%p, numPartialMetadata=%u",
            pResult->frameworkFrameNum, pResult->numOutputBuffers, pResult->pInputMetadata,
            pResult->pOutputMetadata, pResult->numPartialMetadata);

    m_pMutex->Lock();

    BOOL                    issueCb             = FALSE;
    UINT32                  idx                 = 0;
    CHISTREAMBUFFER         resultBuffers[2]    = { { 0 } };
    const CHISTREAMBUFFER*  pBuf                = NULL;
    SSMCaptureRequest*      pCr                 = GetSSMCaptureRequest(pResult->frameworkFrameNum);
    CHICAPTURERESULT        captureResult       = *pResult;
    CHIMETAHANDLE           pInputMetadata      = NULL;
    CHIMETAHANDLE           pOutputMetadata     = NULL;

    // DumpCaptureResult(pResult, "before processing result");

    if ((NULL != pResult->pInputMetadata) && (NULL != pResult->pOutputMetadata))
    {
        CHX_LOG("frame_number=%u, pInputMetadata=%p, pOutputMetadata=%p, numPartialMetadata=%u",
                pCr->frameNumber, pResult->pInputMetadata, pResult->pOutputMetadata, pResult->numPartialMetadata);

        pInputMetadata  = pResult->pInputMetadata;
        pOutputMetadata = pResult->pOutputMetadata;

        if (TRUE == pCr->isPreviewRequest)
        {
            issueCb = TRUE;
        }

        if (TRUE == IsDestinedForProcessing(pCr->destination))
        {
            if (NULL == pCr->pResultOutputMetadata)
            {
                pCr->pResultOutputMetadata = m_pMetadataManager->Get(m_clientId, pCr->frameNumber);
                CHX_LOG("frame_number=%u, new pResultOutputMetadata=%p",
                        pCr->frameNumber, pCr->pResultOutputMetadata);
            }

            if (NULL != pCr->pResultOutputMetadata)
            {
                ChiMetadata* pOutputChiMetadata = m_pMetadataManager->GetMetadataFromHandle(pOutputMetadata);
                pCr->pResultOutputMetadata->Copy(*pOutputChiMetadata);
            }
            else
            {
                CHX_LOG_ERROR("frame_number=%u, failed to get output metadata", pCr->frameNumber);
            }

            pCr->metaReceived               = TRUE;
        }
    }

    // Extract the buffers
    for (UINT32 i = 0; i < captureResult.numOutputBuffers; i++)
    {
        pBuf = &captureResult.pOutputBuffers[i];
        if (TRUE == IsChiStreamVideo(pBuf->pStream))
        {
            pCr->camxOwnsVidBuf = FALSE;
            if (TRUE == IsDestinedForProcessing(pCr->destination))
            {
                FRCRequest* pRequest = static_cast<FRCRequest*>(m_frcFreeQ.RemoveFromHead());
                if (NULL != pRequest)
                {
                    CHX_LOG("dequeued free node from frc free queue, new size: %u",
                            m_frcFreeQ.NumNodes());

                    pRequest->imageBuffer       = pCr->imageBuffer;
                    pRequest->timestamp         = pCr->tsNotify;
                    pRequest->srcFrameNumber    = pCr->frameNumber;
                    pRequest->frameIndex        = pCr->frameIndex;
                    pRequest->captureRequestNum = pCr->captureRequestNum;
                    pRequest->isFRCInput        = TRUE;
                    pRequest->isEos             = pCr->isEos;
                    pRequest->destination       = pCr->destination;

                    // Ownership of the metadata is transferred to FRC. FRC needs to
                    // free it when finished.
                    pRequest->pOutputMetadata   = pCr->pResultOutputMetadata;
                    pCr->pResultOutputMetadata  = NULL;
                    pRequest->pPartialMetadata  = pCr->pDriverPartialMetadata;
                    pCr->pDriverPartialMetadata = NULL;

                    if (SSMDestination::PRE_CAP == pCr->destination)
                    {
                        CHX_LOG("frame_number=%u: capture_request=%u, (received VIDEO from HAL) [pre-cap]",
                                pCr->frameNumber, pCr->captureRequestNum);

                        m_preCapQ.InsertToTail(pRequest);
                        pRequest                = NULL;

                        if (m_preCapQ.NumNodes() > m_preCapCfg.numFrames)
                        {
                            pRequest            = static_cast<FRCRequest*>(m_preCapQ.RemoveFromHead());
                            DoneWithFRCRequest_l(pRequest);
                            pRequest            = NULL;
                        }
                    }
                    else if (SSMDestination::FRC_IN == pCr->destination)
                    {
                        CHX_LOG("frame_number=%u: capture_request=%u, (received VIDEO from HAL) [to frc]",
                                pCr->frameNumber, pCr->captureRequestNum);
                        m_frcPendingInQ.InsertToTail(pRequest);
                        if ((TRUE == pRequest->isEos) &&
                            (FALSE == m_postCapCfg.IsEnabled()))
                        {
                            m_sendCapComplete   = TRUE;
                        }
                    }
                    else // POST_CAP
                    {
                        CHX_LOG("frame_number=%u: capture_request=%u, (received VIDEO from HAL) [post-cap]",
                                pCr->frameNumber, pCr->captureRequestNum);
                        if (TRUE == pRequest->isEos)
                        {
                            // The next capture result that is sent to the client
                            // should indicate that all frames have been captured for
                            // SSM and that we are now in the processing stage.
                            m_sendCapComplete   = TRUE;
                        }

                        m_postCapQ.InsertToTail(pRequest);
                    }

                    DumpQueueStates();

                    if (TRUE == m_sendCapComplete)
                    {
                        CHX_LOG("SSM-EVENT: queueing pre-cap buffers to output");

                        // Get the timestamp of the first buffer that will be
                        // issued in the video stream. Use the timestamp of the
                        // first buffer in m_frcPendingInQ so that the first
                        // buffer sent in the video stream has the same
                        // timestamp as when the user starts recording.
                        m_firstFRCTimestamp = 0;
                        pRequest            = static_cast<FRCRequest*>(m_frcPendingInQ.Head());
                        if (NULL != pRequest)
                        {
                            // This statement should _always_ be true,
                            // otherwise the use case would not be able to
                            // enter this m_sentCapComplete block.
                            m_firstFRCTimestamp = pRequest->timestamp;
                        }
                        CHX_LOG("first video ts=%" PRId64, m_firstFRCTimestamp);

                        while (0 < m_preCapQ.NumNodes())
                        {
                            pRequest = static_cast<FRCRequest*>(m_preCapQ.RemoveFromHead());
                            m_frcDoneOutQ.InsertToTail(pRequest);
                        }

                        // Depending on what the binder overhead is to queue this to VPP,
                        // this work might need to be pushed to the worker thread to
                        // handle.
                        CHX_LOG("SSM-EVENT: queueing input buffers to FRC");
                        m_pMutex->Unlock();
                        QueueBuffersToVppIn();
                        m_pMutex->Lock();
                        m_State = SSMState::SSM_PROCESS;
                        CHX_LOG("SSM-EVENT: changing state to PROCESS");
                    }
                }
                else
                {
                    CHX_LOG_ERROR("CRITICAL ERROR: empty frcFreeQ! size=%u", m_frcFreeQ.NumNodes());
                }
            }
            else
            {
                // Drop the video frame
                CHX_LOG("frame_number=%u: capture_request=%u, image=%p, (received VIDEO from HAL) [dropping]",
                        pCr->frameNumber, pCr->captureRequestNum, pCr->imageBuffer.phBuffer);
                ReleaseChiVideoBuffer(&pCr->imageBuffer);
            }

            m_camxStats.videoReturned++;
        }
        else
        {
            // Preview stream
            issueCb                 = TRUE;
            resultBuffers[idx++]    = captureResult.pOutputBuffers[i];
            CHX_LOG("frame_number=%u: capture_request=%u, (received PREVIEW from HAL)",
                    pCr->frameNumber, pCr->captureRequestNum);
            m_camxStats.previewReturned++;
            m_clientStats.previewReturned++;
            m_clientStats.DumpState();
        }
    }

    m_camxStats.DumpState();

    captureResult.numOutputBuffers  = idx;
    captureResult.pOutputBuffers    = resultBuffers;

    // This m_pCond here is used to wake up Destroy() thread when waiting for
    // all buffers to come back from CAMX.
    m_pCond->Broadcast();
    m_pMutex->Unlock();

    // DumpCaptureResult(&captureResult, "after processing result");

    if (TRUE == issueCb)
    {
        CHX_LOG("frame_number=%u, issuing result with %u outputs, metaDeprecated=%p, metaIn=%p, metaOut=%p",
                pCr->frameNumber, captureResult.numOutputBuffers, captureResult.pResultMetadata,
                captureResult.pInputMetadata, captureResult.pOutputMetadata);
        CameraUsecaseBase::SessionCbCaptureResult(&captureResult, pPrivateCallbackData);

        m_pMutex->Lock();
        PreviewCaptureRequest* pPreviewCr   = GetPreviewCaptureRequest(pResult->frameworkFrameNum);
        if (0 < captureResult.numOutputBuffers)
        {
            pPreviewCr->bufferReceived      = TRUE;
        }
        if ((NULL != captureResult.pInputMetadata) && (NULL != captureResult.pOutputMetadata))
        {
            pPreviewCr->metaReceived        = TRUE;
        }
        RemovePendingPreviewQueue_l();
        m_pMutex->Unlock();
    }
    else
    {
        CHX_LOG("frame_number=%u, not issuing result with %u outputs, metaDeprecated=%p, metaIn=%p, metaOut=%p",
                pCr->frameNumber, captureResult.numOutputBuffers, captureResult.pResultMetadata,
                captureResult.pInputMetadata, captureResult.pOutputMetadata);

        CHX_LOG("frame_number=%u, manually decrementing base meta ref count", pCr->frameNumber);

        // The following metadata that we are releasing are the input/output metadata that
        // was assigned by the base class at ExecuteCaptureRequest. We are releasing these
        // references here because we are going to be sending our own input/output metadata
        // to the base class in place of this. The base class will never decrement the
        // references otherwise, so we must do so here.
        ChiMetadata* pMeta = NULL;
        if (NULL != pInputMetadata)
        {
            pMeta = m_pMetadataManager->GetMetadataFromHandle(pInputMetadata);
            m_pMetadataManager->Release(pMeta);
            pInputMetadata = NULL;
        }

        if (NULL != pOutputMetadata)
        {
            pMeta = m_pMetadataManager->GetMetadataFromHandle(pOutputMetadata);
            m_pMetadataManager->Release(pMeta);
            pOutputMetadata = NULL;
        }
    }

    if (0 != ExtensionModule::GetInstance()->EnableDumpDebugData())
    {
        // Process debug-data
        ProcessDebugData(&captureResult, pPrivateCallbackData, captureResult.frameworkFrameNum);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::ProcessDriverPartialCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::ProcessDriverPartialCaptureResult(
    CHIPARTIALCAPTURERESULT* pResult,
    VOID*                    pPrivateCallbackData)
{
    // Some rules:
    // Preview: need to always send the partial metadata immediately since the
    // preview stream is always kept alive.
    //
    // Video: if a video buffer is destined for output at some point in time (i.e.
    // it is going into pre-cap, post-cap, or frc queues), the partial metadata
    // needs to be deferred in the same manner that normal metadata is. Otherwise
    // it should be safe to drop it. The logic here should be much the same as
    // in ProcessResult().

    m_pMutex->Lock();

    UINT32              frameNumber = pResult->frameworkFrameNum;
    SSMCaptureRequest*  pCr         = GetSSMCaptureRequest(frameNumber);
    BOOL                issueCb     = FALSE;

    CHX_LOG("frame_number=%u, driverPartial, isPreview=%u",
            frameNumber, pCr->isPreviewRequest);

    if (TRUE == pCr->isPreviewRequest)
    {
        issueCb = TRUE;
    }

    if (TRUE == IsDestinedForProcessing(pCr->destination))
    {
        ChiMetadata* pPartialMeta   =
            m_pMetadataManager->GetMetadataFromHandle(pResult->pPartialResultMetadata);
        pCr->pDriverPartialMetadata = m_pMetadataManager->Get(m_clientId, frameNumber);
        if (NULL != pCr->pDriverPartialMetadata)
        {
            pCr->pDriverPartialMetadata->Copy(*pPartialMeta);
        }
        else
        {
            CHX_LOG_ERROR("frame_number=%u, failed to get partial meta", frameNumber);
        }
    }

    m_pMutex->Unlock();

    if (TRUE == issueCb)
    {
        CameraUsecaseBase::SessionCbPartialCaptureResult(pResult, pPrivateCallbackData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::ProcessMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::ProcessMessage(
    const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
    VOID*                       pPrivateCallbackData)
{
    LogMessage(pMessageDescriptor);

    if (ChiMessageTypeShutter == pMessageDescriptor->messageType)
    {
        UINT32              frameNum    = pMessageDescriptor->message.shutterMessage.frameworkFrameNum;
        SSMCaptureRequest*  pCr         = GetSSMCaptureRequest(frameNum);

        pCr->tsNotify       = pMessageDescriptor->message.shutterMessage.timestamp;
        pCr->notifyReceived = TRUE;

        if (TRUE == pCr->isPreviewRequest)
        {
            CHX_LOG("frame_number=%u: (PREVIEW) issuing shutter message, "
                    "timestamp=%" PRId64, pCr->frameNumber, pCr->tsNotify);
            CameraUsecaseBase::SessionCbNotifyMessage(pMessageDescriptor,
                                                      pPrivateCallbackData);
        }
        else
        {
            CHX_LOG("frame_number=%u: (NOT PREVIEW) storing timestamp=%" PRId64,
                    pCr->frameNumber, pCr->tsNotify);
        }
    }
    else if (ChiMessageTypeError == pMessageDescriptor->messageType)
    {
        UINT32              frameNum    = pMessageDescriptor->message.errorMessage.frameworkFrameNum;
        SSMCaptureRequest*  pCr         = GetSSMCaptureRequest(frameNum);

        switch (pMessageDescriptor->message.errorMessage.errorMessageCode)
        {
            case MessageCodeBuffer:
                if ((m_pVideoStream == pMessageDescriptor->message.errorMessage.pErrorStream) &&
                    (TRUE == pCr->isPreviewRequest) &&
                    (FALSE == pCr->hasClientVidBuf))    // Can only ignore if P comes without a V
                {
                    CHX_LOG("frame_number=%u: received ERROR_BUFFER message for injected video buffer, ignoring...",
                            frameNum);
                }
                else
                {
                    CameraUsecaseBase::SessionCbNotifyMessage(pMessageDescriptor, pPrivateCallbackData);
                }
                break;
            default:
                CameraUsecaseBase::SessionCbNotifyMessage(pMessageDescriptor, pPrivateCallbackData);
                break;
        }
    }
    else
    {
        CameraUsecaseBase::SessionCbNotifyMessage(pMessageDescriptor, pPrivateCallbackData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::LogMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::LogMessage(
    const CHIMESSAGEDESCRIPTOR* pMessageDescriptor)
{
    switch (pMessageDescriptor->messageType)
    {
        case ChiMessageTypeError:
            CHX_LOG("frame_number=%u, type=%u, pErrorStream=%p (%s), errorMessageCode=%u",
                    pMessageDescriptor->message.errorMessage.frameworkFrameNum,
                    pMessageDescriptor->messageType,
                    pMessageDescriptor->message.errorMessage.pErrorStream,
                    pMessageDescriptor->message.errorMessage.pErrorStream == m_pVideoStream ? "VIDEO" :
                    pMessageDescriptor->message.errorMessage.pErrorStream == m_pPreviewStream ? "PREVIEW" : "UNKNOWN",
                    pMessageDescriptor->message.errorMessage.errorMessageCode);
            break;
        case ChiMessageTypeShutter:
            CHX_LOG("frame_number=%u, type=%u, timestamp=%" PRIu64,
                    pMessageDescriptor->message.shutterMessage.frameworkFrameNum,
                    pMessageDescriptor->messageType,
                    pMessageDescriptor->message.shutterMessage.timestamp);
            break;
        case ChiMessageTypeSof:
            CHX_LOG("frame_number=%u, type=%u, timestamp=%" PRIu64 ", "
                    "sofId=%u, bIsFrameworkFrameNumValid=%u",
                    pMessageDescriptor->message.sofMessage.frameworkFrameNum,
                    pMessageDescriptor->messageType,
                    pMessageDescriptor->message.sofMessage.timestamp,
                    pMessageDescriptor->message.sofMessage.sofId,
                    pMessageDescriptor->message.sofMessage.bIsFrameworkFrameNumValid);
            break;
        case ChiMessageTypeMetaBufferDone:
            CHX_LOG("frame_number=%u, type=%u, inputMetabuffer=%p, "
                    "outputMetabuffer=%p, numMetaBuffers=%u",
                    pMessageDescriptor->message.metaBufferDoneMessage.frameworkFrameNum,
                    pMessageDescriptor->messageType,
                    pMessageDescriptor->message.metaBufferDoneMessage.inputMetabuffer,
                    pMessageDescriptor->message.metaBufferDoneMessage.outputMetabuffer,
                    pMessageDescriptor->message.metaBufferDoneMessage.numMetaBuffers);
            break;
        case ChiMessageTypeTriggerRecovery:
            CHX_LOG("type=%u", pMessageDescriptor->messageType);
            break;
        default:
            CHX_LOG_ERROR("received invalid message type=%u",
                          pMessageDescriptor->messageType);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::DumpDebugData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::DumpDebugData(
    const VOID*     pDebugData,
    const SIZE_T    sizeDebugData,
    const UINT32    sessionId,
    const UINT32    cameraId,
    const UINT32    resultFrameNumber)
{
    CHAR dumpFilename[256];
    CHAR suffix[] = "bin";
    const CHAR parserString[19] = "QTI Debug Metadata";
    DebugDataStartHeader metadataHeader;
    char timeBuf[128]= { 0 };
    time_t currentTime;
    time (&currentTime);
    struct tm* timeInfo = localtime (&currentTime);

    if (NULL != timeInfo) {
        strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", timeInfo);
    }

    if (resultFrameNumber == m_debugLastResultFrameNumber)
    {
        m_debugMultiPassCount++;
    }
    else
    {
        m_debugMultiPassCount = 0;
    }

    CdkUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
#if defined (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // NOWHINE PR002 <- Win32 definition
                       "/data/vendor/camera/%s_debug-data_session[%u]_cameraId[%u]_req[%u]_cnt[%u].%s",
#else
                       "/data/misc/camera/%s_debug-data_session[%u]_cameraId[%u]_req[%u]_cnt[%u].%s",
#endif // Android-P or later
                       timeBuf, sessionId, cameraId, resultFrameNumber, m_debugMultiPassCount, suffix);

    metadataHeader.dataSize                = sizeof(parserString) + sizeof(metadataHeader) + sizeDebugData;
    metadataHeader.majorRevision           = 1;
    metadataHeader.minorRevision           = 1;
    metadataHeader.patchRevision           = 0;
    metadataHeader.SWMajorRevision         = 1;
    metadataHeader.SWMinorRevision         = 0;
    metadataHeader.SWPatchRevision         = 0;
    metadataHeader.featureDesignator[0]    = 'R';
    metadataHeader.featureDesignator[1]    = 'C';

    CHX_LOG("DebugDataAll: dumpFilename: %s, pDebugData: %p, sizeDebugData: %zu, sizeMeta: %u [0x%x]",
        dumpFilename, pDebugData, sizeDebugData, metadataHeader.dataSize, metadataHeader.dataSize);

    FILE* pFile = CdkUtils::FOpen(dumpFilename, "wb");
    if (NULL != pFile)
    {
        CdkUtils::FWrite(parserString, sizeof(parserString), 1, pFile);
        CdkUtils::FWrite(&metadataHeader, sizeof(metadataHeader), 1, pFile);
        CdkUtils::FWrite(pDebugData, sizeDebugData, 1, pFile);
        CdkUtils::FClose(pFile);
    }
    else
    {
        CHX_LOG("DebugDataAll: Debug data failed to open for writing: %s", dumpFilename);
    }

    m_debugLastResultFrameNumber = resultFrameNumber;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::HandleBatchModeResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::HandleBatchModeResult(
   ChiCaptureResult* pResult,
   ChiMetadata*      pChiOutputMetadata,
   UINT32            resultId,
   UINT32            clientId)
{
    CDKResult           result           = CDKResultSuccess;
    camera_metadata_t*  pFrameworkOutput = NULL;

    pFrameworkOutput = m_pMetadataManager->GetAndroidFrameworkOutputMetadata(false);
    if (NULL != pFrameworkOutput)
    {
        UINT32  partialTagCount = 0;
        UINT32* pPartialTagList = NULL;

        // Get the Partial Tag ID list if Partial Data has already been sent
        if (FALSE == CheckIfPartialDataCanBeSent(PartialResultSender::DriverPartialData, resultId))
        {
            partialTagCount = m_pMetadataManager->RetrievePartialTagCount(clientId);
            pPartialTagList = m_pMetadataManager->RetrievePartialTags(clientId);
            CHX_LOG("Partial Data has been sent and meta data needs to be filtered for tags=%d", partialTagCount);
        }

        pChiOutputMetadata->TranslateToCameraMetadata(pFrameworkOutput,
                                                      TRUE,
                                                      TRUE,
                                                      partialTagCount,
                                                      pPartialTagList);
    }
    else
    {
        CHX_LOG_ERROR("frame_number=%u, failed to get framework metadata",
                      pResult->frameworkFrameNum);
        result = CDKResultEFailed;
    }

    if ((CDKResultSuccess == result) && (NULL != pFrameworkOutput))
    {
        m_captureResult[resultId].result = pFrameworkOutput;

        CHX_LOG("frame_number=%u, updating final metadata with ts=%" PRId64,
                pResult->frameworkFrameNum,
                m_shutterTimestamp[pResult->frameworkFrameNum % MaxOutstandingRequests]);

        ChxUtils::AndroidMetadata::UpdateTimeStamp(
            pFrameworkOutput,
            m_shutterTimestamp[pResult->frameworkFrameNum % MaxOutstandingRequests],
            pResult->frameworkFrameNum);

        SetMetadataAvailable(resultId);
    }

    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("ERROR Cannot get buffer for frame %u interval (%u %u)",
            pResult->frameworkFrameNum,
            m_batchRequestStartIndex,
            m_batchRequestEndIndex);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::SSMHidleVppCallbacks::inputBufferDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
android::hardware::Return<uint32_t> UsecaseSuperSlowMotionFRC::SSMHidlVppCallbacks::inputBufferDone(
    const V1_1::VppBuffer& vppBuf)
{
    m_pUsecase->OnInputBufferDone(&vppBuf);
    return static_cast<uint32_t>(V1_1::VppError::VPP_OK);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::SSMHidlVppCallbacks::outputBufferDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
android::hardware::Return<uint32_t> UsecaseSuperSlowMotionFRC::SSMHidlVppCallbacks::outputBufferDone(
    const V1_1::VppBuffer& vppBuf)
{
    m_pUsecase->OnOutputBufferDone(&vppBuf);
    return static_cast<uint32_t>(V1_1::VppError::VPP_OK);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::SSMHidlVppCallbacks::vppEvent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
android::hardware::Return<uint32_t> UsecaseSuperSlowMotionFRC::SSMHidlVppCallbacks::vppEvent(
    const V1_1::VppEvent& e)
{
    m_pUsecase->OnEvent(e);
    return static_cast<uint32_t>(V1_1::VppError::VPP_OK);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::DoneWithFRCRequest_l
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::DoneWithFRCRequest_l(
    FRCRequest* pRequest)
{
    if (NULL != pRequest->imageBuffer.phBuffer)
    {
        ReleaseChiVideoBuffer(&pRequest->imageBuffer);
    }

    if (NULL != pRequest->pPartialMetadata)
    {
        m_pMetadataManager->Release(pRequest->pPartialMetadata);
        pRequest->pPartialMetadata = NULL;
    }

    if (NULL != pRequest->pOutputMetadata)
    {
        m_pMetadataManager->Release(pRequest->pOutputMetadata);
        pRequest->pOutputMetadata = NULL;
    }
    m_frcFreeQ.InsertToTail(pRequest);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::OnInputBufferDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::OnInputBufferDone(
    const V1_1::VppBuffer* pVppBuf)
{
    CHX_ASSERT(NULL != pVppBuf);
    FRCRequest* pRequest = GetFRCRequestFromVppBuffer(pVppBuf);

    m_pMutex->Lock();

    CHX_LOG("IBD: ");

    m_VppManager.ibdCount++;
    m_VppManager.inCount--;

    CHX_ASSERT(TRUE == pRequest->isFRCInput);

    m_frcProcInQ.RemoveNode(pRequest);

    if (TRUE == m_VppManager.isInFlushing)
    {
        DoneWithFRCRequest_l(pRequest);
    }
    else
    {
#ifdef SYNCHRONIZE_IBD_BUFFERS
        m_frcDoneInQ.InsertToTail(pRequest);
#else
        DoneWithFRCRequest_l(pRequest);
#endif
    }

    DumpQueueStates();
    m_VppManager.DumpState();

    m_pCond->Broadcast();
    m_pMutex->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::OnOutputBufferDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::OnOutputBufferDone(
    const V1_1::VppBuffer* pVppBuf)
{
    BOOL isEos = FALSE;

    CHX_ASSERT(NULL != pVppBuf);
    FRCRequest* pRequest = GetFRCRequestFromVppBuffer(pVppBuf);

    isEos = (pVppBuf->flags & static_cast<UINT32>(V1_1::VppBufferFlag::VPP_BUFFER_FLAG_EOS)) ? TRUE : FALSE;

    m_pMutex->Lock();

    m_VppManager.obdCount++;

    if (TRUE == pRequest->isFRCInput)
    {
        m_VppManager.inCount--;
        m_frcProcInQ.RemoveNode(pRequest);
    }
    else
    {
        m_VppManager.outCount--;
        m_frcProcOutQ.RemoveNode(pRequest);
    }

    if ((TRUE == m_VppManager.isOutFlushing) || (0 == pVppBuf->pixel.filledLen))
    {
        CHX_LOG("OBD: frameNum=%u, isOutFlushing=%u, filled_len=%u",
                pRequest->srcFrameNumber, m_VppManager.isOutFlushing,
                pVppBuf->pixel.filledLen);
        DoneWithFRCRequest_l(pRequest);
    }
    else
    {
        pRequest->timestamp = pVppBuf->timestamp * 1000;
        CHX_LOG("OBD: frameNum=%u, ts=%" PRId64,
                pRequest->srcFrameNumber, pRequest->timestamp);
        m_frcDoneOutQ.InsertToTail(pRequest);
    }

    if (TRUE == isEos)
    {
        CHX_LOG("SSM-EVENT: received EOS from FRC, queue post-cap buffers to output");
        while (0 < m_postCapQ.NumNodes())
        {
            pRequest = static_cast<FRCRequest*>(m_postCapQ.RemoveFromHead());
            m_frcDoneOutQ.InsertToTail(pRequest);
        }
    }

    DumpQueueStates();
    m_VppManager.DumpState();

    m_pCond->Broadcast();
    m_pMutex->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::OnEvent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::OnEvent(
    V1_1::VppEvent e)
{
    if (V1_1::VppEventType::VPP_EVENT_FLUSH_DONE == e.type)
    {
        CHX_LOG("received vpp event: type=%u, port=%u", e.type, e.u.flushDone.port);
        m_pMutex->Lock();
        if (V1_1::VppPort::VPP_PORT_INPUT == e.u.flushDone.port)
        {
            if (FALSE == m_VppManager.isInFlushing)
            {
                CHX_LOG_ERROR("unexpected vpp flush done (input)");
            }
            m_VppManager.isInFlushing = FALSE;
        }
        else if (V1_1::VppPort::VPP_PORT_OUTPUT == e.u.flushDone.port)
        {
            if (FALSE == m_VppManager.isOutFlushing)
            {
                CHX_LOG_ERROR("unexpected vpp flush done (output)");
            }
            m_VppManager.isOutFlushing = FALSE;
        }

        m_pCond->Broadcast();
        m_pMutex->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::GetFRCRequestFromVppBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UsecaseSuperSlowMotionFRC::FRCRequest* UsecaseSuperSlowMotionFRC::GetFRCRequestFromVppBuffer(
    const V1_1::VppBuffer* pVppBuf)
{
    BOOL        isInput;
    LDLLQueue*  pQueueToSearch;
    FRCRequest* pRequest;

    CHX_ASSERT(NULL != pVppBuf);

    if (0 != (pVppBuf->flags & static_cast<UINT32>(V1_1::VppBufferFlag::VPP_BUFFER_FLAG_READONLY)))
    {
        pQueueToSearch  = &m_frcProcInQ;
        isInput         = TRUE;
    }
    else
    {
        pQueueToSearch  = &m_frcProcOutQ;
        isInput         = FALSE;
    }

    pRequest = static_cast<FRCRequest*>(pQueueToSearch->Head());
    while (NULL != pRequest)
    {
        if (pRequest->id == pVppBuf->bufferId)
        {
            break;
        }
        pRequest = static_cast<FRCRequest*>(pRequest->pNext);
    }

    if (NULL == pRequest)
    {
        CHX_LOG_ERROR("Failed to find buffer id=%u in %s procQ", pVppBuf->bufferId,
                      isInput ? "input" : "output");
    }
    return pRequest;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::SSMDeathRecipient::serviceDied
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseSuperSlowMotionFRC::SSMDeathRecipient::serviceDied(
    uint64_t                                                cookie,
    const android::wp<android::hidl::base::V1_0::IBase>&    who)
{
    FRCRequest* pRequest;

    CHX_LOG_ERROR("vpp service died, cookie=%" PRIu64 ", who=%p", cookie, &who);

    // Recycle vpp-queued output buffers.
    m_pUsecase->FlushRequestQueue(&m_pUsecase->m_frcProcOutQ);

    // Bypass input buffers as output buffers.
    m_pUsecase->m_pMutex->Lock();
    pRequest = static_cast<FRCRequest*>(m_pUsecase->m_frcProcInQ.RemoveFromHead());
    m_pUsecase->m_pMutex->Unlock();
    while (NULL != pRequest)
    {
        m_pUsecase->OnOutputBufferDone(&pRequest->vppBuffer);

        m_pUsecase->m_pMutex->Lock();
        pRequest = static_cast<FRCRequest*>(m_pUsecase->m_frcProcInQ.RemoveFromHead());
        m_pUsecase->m_pMutex->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::InitializeVPP
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::InitializeVPP()
{
    CDKResult   result      = CDKResultSuccess;
    UINT32      vppResult;
    UINT32      attempts    = 0;

    while (NULL == m_VppService.get() && attempts < MaxGetServiceRetries)
    {
        m_VppService = V1_2::IHidlVppService::tryGetService("vppService");
        if (NULL == m_VppService.get())
        {
            ChxUtils::SleepMicroseconds(VppServiceRetryIntervalUS);
            attempts++;
            CHX_LOG_INFO("getting vppservice, attempt %u of %u",
                         attempts, MaxGetServiceRetries);
        }
    }

    if (NULL == m_VppService.get())
    {
        CHX_LOG_ERROR("Failed to access vppservice");
        result = CDKResultENoSuch;
    }

    if (CDKResultSuccess == result)
    {
        m_DeathRecipient = CHX_NEW SSMDeathRecipient(this);
        m_VppService->linkToDeath(m_DeathRecipient, 0);

        m_VppSession = m_VppService->getNewVppSession_1_2(0);
        if (NULL == m_VppSession.get())
        {
            CHX_LOG_ERROR("Failed to construct vpp session");
            result = CDKResultENoSuch;
        }
    }

    if (CDKResultSuccess == result)
    {
        vppResult = m_VppSession->vppInit(0, CHX_NEW SSMHidlVppCallbacks(this));
        if (static_cast<UINT32>(V1_1::VppError::VPP_OK) != vppResult)
        {
            CHX_LOG_ERROR("vpp_init() failed, res=%u", vppResult);
            result = CDKResultENoSuch;
        }
    }

    if (CDKResultSuccess == result)
    {
        m_VppInitialized            = TRUE;
        ConfigureVPP(2);
    }

    if (CDKResultSuccess == result)
    {
        // Assuming a UBWC format is being used
        V1_1::VppPortParam param    = { 0 };
        param.width                 = mWidth;
        param.height                = mHeight;
        param.stride                = mWidth;
        param.scanlines             = mHeight;
        param.fmt                   = mVppColorFormat;

        vppResult = m_VppSession->vppSetParameter(V1_1::VppPort::VPP_PORT_INPUT, param);
        if (static_cast<UINT32>(V1_1::VppError::VPP_OK) != vppResult)
        {
            CHX_LOG_ERROR("vpp_set_param(input) failed, result=%u", vppResult);
            result = CDKResultEFailed;
        }

        if (CDKResultSuccess == result)
        {
            vppResult = m_VppSession->vppSetParameter(V1_1::VppPort::VPP_PORT_OUTPUT, param);
            if (static_cast<UINT32>(V1_1::VppError::VPP_OK) != vppResult)
            {
                CHX_LOG_ERROR("vpp_set_param(output) failed, result=%u", vppResult);
                result = CDKResultEFailed;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::ConfigureVPP
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::ConfigureVPP(
    UINT32 interpFactor)
{
    CDKResult               result              = CDKResultSuccess;
    BOOL                    shouldSendControls  = TRUE;
    UINT32                  vppResult           = static_cast<UINT32>(V1_1::VppError::VPP_OK);
    V1_1::VppRequirements   req;
    V1_2::VppControl_1_2    control;
    V1_1::VideoProperty     vidProp;
    V1_1::HqvFrcInterp      interp;

    ChxUtils::Memset(&req, 0, sizeof(req));
    ChxUtils::Memset(&control, 0, sizeof(control));
    ChxUtils::Memset(&vidProp, 0, sizeof(vidProp));

    if (FALSE == m_VppInitialized)
    {
        CHX_LOG_ERROR("ERROR: can not configure, vpp not initialized");
        result = CDKResultEInvalidState;
    }

    if (CDKResultSuccess == result)
    {
        if (interpFactor == m_VppManager.interpFactor)
        {
            CHX_LOG("interpFactor already set to %u, no need to set again", interpFactor);
            shouldSendControls = FALSE;
        }
    }

    if (TRUE == shouldSendControls)
    {
        if (CDKResultSuccess == result)
        {
            if (1 == interpFactor)
            {
                interp = V1_1::HqvFrcInterp::HQV_FRC_INTERP_1X;
            }
            else if (2 == interpFactor)
            {
                interp = V1_1::HqvFrcInterp::HQV_FRC_INTERP_2X;
            }
            else if (3 == interpFactor)
            {
                interp = V1_1::HqvFrcInterp::HQV_FRC_INTERP_3X;
            }
            else if (4 == interpFactor)
            {
                interp = V1_1::HqvFrcInterp::HQV_FRC_INTERP_4X;
            }
            else
            {
                CHX_LOG_ERROR("vpp_set_ctrl(frc) failed, invalid interpFactor=%u", interpFactor);
                result = CDKResultEInvalidArg;
            }
        }

        if (CDKResultSuccess == result)
        {
            m_VppManager.interpFactor                       = interpFactor;

            control.mode                                    = V1_1::HqvMode::HQV_MODE_MANUAL;
            control.ctrlType                                = V1_1::HqvControlType::HQV_CONTROL_FRC;
            control.frc.segments.resize(1);
            control.frc.segments[0].mode                    = V1_1::HqvFrcMode::HQV_FRC_MODE_SLOMO;
            control.frc.segments[0].level                   = V1_1::HqvFrcLevel::HQV_FRC_LEVEL_HIGH;
            control.frc.segments[0].interp                  = interp;
            control.frc.segments[0].ts_start                = 0;
            control.frc.segments[0].frame_copy_on_fallback  = 1;
            control.frc.segments[0].frame_copy_input        = 0;

            m_VppSession->vppSetCtrl_1_2(control, [&vppResult, &req](V1_1::VppRequirements reqs) {
                                                        req         = reqs;
                                                        vppResult   = reqs.retStatus;
                                                    });
            if (static_cast<UINT32>(V1_1::VppError::VPP_OK) != vppResult)
            {
                m_VppManager.interpFactor   = static_cast<UINT32>(-1);
                CHX_LOG_ERROR("vpp_set_ctrl(frc) failed, result=%u", vppResult);
                result = CDKResultEFailed;
            }
        }

        if (CDKResultSuccess == result)
        {
            vidProp.propertyType                = V1_1::VidPropType::VID_PROP_NON_REALTIME;
            vidProp.u.nonRealtime.bNonRealtime  = 1;

            vppResult = m_VppSession->vppSetVidProp(vidProp);
            if (static_cast<UINT32>(V1_1::VppError::VPP_OK) != vppResult)
            {
                CHX_LOG_ERROR("vpp_set_vid_prop() failed, result=%u", vppResult);
                result = CDKResultEFailed;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::DestroyVPP
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::DestroyVPP(UINT32 bDoLock)
{
    UINT32  vppResult;

    if (TRUE == m_VppInitialized)
    {
        if (TRUE == bDoLock)
        {
            m_pMutex->Lock();
        }

        if (NULL != m_VppSession.get())
        {
            m_VppManager.isInFlushing       = TRUE;
            m_VppManager.isOutFlushing      = TRUE;

            vppResult = m_VppSession->vppFlush(V1_1::VppPort::VPP_PORT_INPUT);
            if (static_cast<UINT32>(V1_1::VppError::VPP_OK) != vppResult)
            {
                CHX_LOG_ERROR("vpp_flush(input) failed, result=%u", vppResult);
                m_VppManager.isInFlushing   = FALSE;
            }
            vppResult = m_VppSession->vppFlush(V1_1::VppPort::VPP_PORT_OUTPUT);
            if (static_cast<UINT32>(V1_1::VppError::VPP_OK) != vppResult)
            {
                CHX_LOG_ERROR("vpp_flush(output) failed, result=%u", vppResult);
                m_VppManager.isOutFlushing  = FALSE;
            }
        }

        while ((TRUE == m_VppManager.isInFlushing) || (TRUE == m_VppManager.isOutFlushing))
        {
            CHX_LOG("waiting for vpp flush to complete, in=%u, out=%u",
                    m_VppManager.isInFlushing, m_VppManager.isOutFlushing);
            m_pCond->Wait(m_pMutex->GetNativeHandle());
        }

        CHX_LOG("vpp flush done");
        if (TRUE == bDoLock)
        {
            m_pMutex->Unlock();
        }

        if (NULL != m_VppSession.get())
        {
            m_VppSession->vppTerm();
        }
        m_VppSession = NULL;
    }

    if (NULL != m_VppSession.get())
    {
        m_VppSession->unlinkToDeath(m_DeathRecipient);
        m_DeathRecipient    = NULL;
        m_VppSession        = NULL;
    }
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::QueueBuffersToVpp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::QueueBuffersToVpp()
{
    CDKResult result = QueueBuffersToVppOut();

    if (CDKResultSuccess == result)
    {
        result = QueueBuffersToVppIn();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::QueueBuffersToVppIn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::QueueBuffersToVppIn()
{
    CDKResult   result      = CDKResultSuccess;
    FRCRequest* pRequest    = NULL;

    m_pMutex->Lock();
    while (0 < m_frcPendingInQ.NumNodes())
    {
        pRequest = static_cast<FRCRequest*>(m_frcPendingInQ.RemoveFromHead());

        // Put this in the queue so that in the unlikely event that vpp sends
        // the buffer back before we enqueue it, it exists.
        pRequest->ownedByVPP = TRUE;
        m_VppManager.inCount++;
        m_VppManager.inQueued++;
        m_frcProcInQ.InsertToTail(pRequest);

        m_pMutex->Unlock();
        result = QueueBufferToVppIn(pRequest);
        m_pMutex->Lock();

        if (CDKResultSuccess != result)
        {
            // This probably failed because we tried to queue too many
            // buffers to VPP. Put the buffer back at the front of
            // the queue and try again later.
            CHX_LOG_ERROR("failed to queue buffer to vpp, try again later, err=%u", result);

            m_frcProcInQ.RemoveNode(pRequest);

            pRequest->ownedByVPP = FALSE;
            m_VppManager.inCount--;
            m_VppManager.inQueued--;

            m_frcPendingInQ.InsertToHead(pRequest);
            break;
        }
        m_VppManager.DumpState();
    }

    m_pMutex->Unlock();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::QueueBuffersToVppOut
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::QueueBuffersToVppOut()
{
    CDKResult   result = CDKResultSuccess;
    UINT32      buffersToQueue;

    m_pMutex->Lock();

    if (SSMState::SSM_BYPASS == m_State)
    {
        buffersToQueue = FRCOutputPortReq;
    }
    else
    {
        buffersToQueue = m_VppManager.outCountMax - m_VppManager.outCount;

        // This logic is necessary to bring the VPP Output port count back to
        // what it was initialized to in Initialize(), in the situation after an
        // entire record+process cycle is over. Because our buffer allocation
        // and management is very tight, we need the VPP Output port to hold the
        // same amount of buffers at the end of a record+process cycle as the
        // beginning of a cycle, so that at the end it is ready to do another
        // recording.
        UINT32 preferredVPPOutputPortCount = m_VppManager.inCount + FRCOutputPortReq;
        UINT32 diffBetweenPreferredAndActual = preferredVPPOutputPortCount > m_VppManager.outCount ?
                                               preferredVPPOutputPortCount - m_VppManager.outCount : 0;
        if (diffBetweenPreferredAndActual < buffersToQueue)
        {
            buffersToQueue = diffBetweenPreferredAndActual;
        }
    }

    UINT32 busyBuffers      = (m_preCapQ.NumNodes() +
                               m_postCapQ.NumNodes() +
                               m_frcProcInQ.NumNodes() +
                               m_frcProcOutQ.NumNodes() +
                               m_frcDoneInQ.NumNodes() +
                               m_frcDoneOutQ.NumNodes());
    UINT32 availBuffers     = m_frcFreeQ.NumNodes() > ExtraBuffersNeededForProc ?
                              m_frcFreeQ.NumNodes() - ExtraBuffersNeededForProc : 0;

    if (availBuffers < buffersToQueue)
    {
        buffersToQueue = availBuffers;
    }

    m_pMutex->Unlock();

    CHX_LOG("queueing %u buffers to vpp out (available=%u, busy=%u)",
            buffersToQueue, availBuffers, busyBuffers);

    for (UINT32 i = 0; i < buffersToQueue; i++)
    {
        result = QueueOneBufferToVppOut();
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("failed to queue initial output to vpp, i=%u, res=%u", i, result);
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::QueueOneBufferToVppOut
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::QueueOneBufferToVppOut()
{
    CDKResult   result      = CDKResultSuccess;
    UINT32      vppResult;

    m_pMutex->Lock();

    FRCRequest* pRequest = static_cast<FRCRequest*>(m_frcFreeQ.RemoveFromHead());
    if (NULL == pRequest)
    {
        CHX_LOG_ERROR("unable to get frc request, size=%u", m_frcFreeQ.NumNodes());
        m_pMutex->Unlock();
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        // Invalid
        pRequest->srcFrameNumber    = static_cast<UINT32>(-1);
        pRequest->frameIndex        = static_cast<UINT32>(-1);
        pRequest->captureRequestNum = static_cast<UINT32>(-1);

        pRequest->timestamp         = 0;
        pRequest->isFRCInput        = FALSE;
        pRequest->imageBuffer       = m_bufferManager->GetImageBufferInfo();
        pRequest->destination       = SSMDestination::DROP;

        if (NULL == pRequest->imageBuffer.phBuffer)
        {
            m_frcFreeQ.InsertToTail(pRequest);
            m_pMutex->Unlock();
            CHX_LOG_ERROR("failed to get image buffer for output");
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        // Put it in the out queue before queueing it to VPP in case VPP returns it
        // before this is done.
        pRequest->ownedByVPP = TRUE;
        m_VppManager.outCount++;
        m_VppManager.outQueued++;
        m_frcProcOutQ.InsertToTail(pRequest);

        m_pMutex->Unlock();

        pRequest->PopulateVppBuffer();
        vppResult = m_VppSession->vppQueueBuf(V1_1::VppPort::VPP_PORT_OUTPUT, pRequest->vppBuffer);

        m_pMutex->Lock();

        if (static_cast<UINT32>(V1_1::VppError::VPP_OK) != vppResult)
        {
            pRequest->ownedByVPP = FALSE;
            m_VppManager.outCount--;
            m_VppManager.outQueued--;
            m_frcProcOutQ.RemoveNode(pRequest);

            DoneWithFRCRequest_l(pRequest);
            CHX_LOG_ERROR("failed to queue buffer to vpp output, result=%u", vppResult);
        }

        m_VppManager.DumpState();

        m_pMutex->Unlock();
        result = static_cast<UINT32>(V1_1::VppError::VPP_OK) == vppResult ? CDKResultSuccess :
                                                                            CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseSuperSlowMotionFRC::QueueBufferToVppIn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseSuperSlowMotionFRC::QueueBufferToVppIn(
    FRCRequest* pRequest)
{
    UINT32 vppResult;
    CHX_ASSERT(NULL != pRequest);

    pRequest->PopulateVppBuffer();

    vppResult = m_VppSession->vppQueueBuf(V1_1::VppPort::VPP_PORT_INPUT, pRequest->vppBuffer);
    if (static_cast<UINT32>(V1_1::VppError::VPP_OK) != vppResult)
    {
        CHX_LOG_ERROR("failed to queue buffer to vpp input, result=%u", vppResult);
    }

    return static_cast<UINT32>(V1_1::VppError::VPP_OK) == vppResult ? CDKResultSuccess : CDKResultEFailed;
}
