////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxusecasesuperslowmotionfrc.h
/// @brief CHX Super Slow Motion usecase declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXSUPERSLOWMOTIONFRCUSECASE_H
#define CHXSUPERSLOWMOTIONFRCUSECASE_H

#include "chistatspropertydefines.h"
#include "cdkutils.h"
#include "chxdefs.h"
#include "chxextensionmodule.h"
#include "chxpipeline.h"
#include "chxusecase.h"
#include "chxusecaseutils.h"
#include "chxadvancedcamerausecase.h"

// Slow Motion specific requirements
#include <utils/StrongPointer.h>
#include <vendor/qti/hardware/vpp/1.1/IHidlVppCallbacks.h>
#include <vendor/qti/hardware/vpp/1.2/IHidlVppService.h>
#include <vendor/qti/hardware/vpp/1.2/IHidlVpp.h>

using namespace ::vendor::qti::hardware::vpp;

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Super Slow Motion use case
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UsecaseSuperSlowMotionFRC : public CameraUsecaseBase
{
public:

    /// Static create function to create an instance of the object
    static UsecaseSuperSlowMotionFRC* Create(
        LogicalCameraInfo*              pCameraInfo,    ///< Camera info
        camera3_stream_configuration_t* pStreamConfig); ///< Stream configuration;

protected:

    /// Destroy/Cleanup the object
    virtual VOID Destroy(
        BOOL isForced);

    // Flush
    virtual CDKResult Flush();

    /// Callback to inform of a pipeline create
    virtual VOID PipelineCreated(
        UINT sessionId,     ///< Id of session created
        UINT pipelineId);   ///< Pipeline of session created

    /// Execute capture request
    virtual CDKResult ExecuteCaptureRequest(
        camera3_capture_request_t* pRequest);   ///< Request parameters

    // Function to handle batch mode result
    virtual CDKResult HandleBatchModeResult(
        ChiCaptureResult*   pResult,
        ChiMetadata*        pChiOutputMetadata,
        UINT32              resultId,
        UINT32              clientId);

    UsecaseSuperSlowMotionFRC()     = default;
    ~UsecaseSuperSlowMotionFRC()    = default;

protected:

    /// Callback for a result from the driver
    static VOID ProcessResultCb(
        CHICAPTURERESULT*   pResult,
        VOID*               pPrivateCallbackData)
    {
        SessionPrivateData* pCbData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
        static_cast<UsecaseSuperSlowMotionFRC*>(pCbData->pUsecase)->ProcessResult(pResult,
                                                                                  pPrivateCallbackData);
    }

    /// Callback for Partial Capture result from the driver
    static VOID ProcessDriverPartialCaptureResultCb(
         CHIPARTIALCAPTURERESULT*   pCaptureResult,
         VOID*                      pPrivateCallbackData)
    {
        SessionPrivateData* pCbData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
        static_cast<UsecaseSuperSlowMotionFRC*>(pCbData->pUsecase)->ProcessDriverPartialCaptureResult(
            pCaptureResult, pPrivateCallbackData);
    }

    /// Callback for a message from the driver
    static VOID ProcessMessageCb(
        const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
        VOID*                       pPrivateCallbackData)
    {
        SessionPrivateData* pCbData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
        static_cast<UsecaseSuperSlowMotionFRC*>(pCbData->pUsecase)->ProcessMessage(pMessageDescriptor,
                                                                                   pPrivateCallbackData);
    }

    // Do not allow the copy constructor or assignment operator
    UsecaseSuperSlowMotionFRC(const UsecaseSuperSlowMotionFRC&)             = delete;
    UsecaseSuperSlowMotionFRC& operator= (const UsecaseSuperSlowMotionFRC&) = delete;

    /// Does one time initialization of the created object
    CDKResult Initialize(
        LogicalCameraInfo*              pCameraInfo,     ///< Camera info
        camera3_stream_configuration_t* pStreamConfig);  ///< Stream configuration

    /// Performs usecase struct selection and stream matching
    CDKResult SelectUsecaseConfig(
        LogicalCameraInfo*              pCameraInfo,     ///< Camera info
        camera3_stream_configuration_t* pStreamConfig);  ///< Stream configuration

    /// Perform actions after [Camera]Usecase has been created and initialized
    CDKResult PostUsecaseCreation(
        camera3_stream_configuration_t* pStreamConfig);  ///< Stream configuration

    /// Processor called by callback
    VOID ProcessResult(
        CHICAPTURERESULT*   pResult,
        VOID*               pPrivateCallbackData);

    /// Processor called by callback
    VOID ProcessDriverPartialCaptureResult(
        CHIPARTIALCAPTURERESULT* pResult,
        VOID*                    pPrivateCallbackData);

    /// Processor called by callback
    VOID ProcessMessage(
        const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
        VOID*                       pPrivateCallbackData);

    VOID LogMessage(
        const CHIMESSAGEDESCRIPTOR* pMessageDescriptor);

    /// Dump into a file debug data from nodes
    VOID DumpDebugData(
        const VOID*     pDebugData,
        const SIZE_T    sizeDebugData,
        const UINT32    sessionId,
        const UINT32    cameraId,
        const UINT32    resultFrameNumber);

    // Dump Debug/Tuning data
    UINT32  m_debugLastResultFrameNumber;
    UINT32  m_debugMultiPassCount;

    // New Slow Motion APIs

    enum SSMTags
    {
        CAPTURE_START,
        CAPTURE_COMPLETE,
        PROCESSING_COMPLETE,
    };

    enum SSMState
    {
        SSM_BYPASS,
        SSM_RECORD,
        SSM_RECORD_TO_PROCESS,
        SSM_PROCESS,
        SSM_DESTROY,
    };

    struct SSMVendorTags
    {
        UINT32  CaptureStart;
        UINT32  CaptureComplete;
        UINT32  ProcessingComplete;
        UINT32  InterpFactor;
    } m_vendorTags;

    enum SSMDestination
    {
        DROP,
        PRE_CAP,
        FRC_IN,
        POST_CAP,
    };

    BOOL IsDestinedForProcessing(SSMDestination dest)
    {
        return ((SSMDestination::FRC_IN   == dest) ||
                (SSMDestination::PRE_CAP  == dest) ||
                (SSMDestination::POST_CAP == dest)) ? TRUE : FALSE;
    }

    const CHAR* GetDestinationString(SSMDestination dest)
    {
        const CHAR* destination;
        switch (dest)
        {
            case SSMDestination::DROP:      destination = "DROP";       break;
            case SSMDestination::PRE_CAP:   destination = "PRE_CAP";    break;
            case SSMDestination::FRC_IN:    destination = "FRC_IN";     break;
            case SSMDestination::POST_CAP:  destination = "POST_CAP";   break;
            default:                        destination = "UNKNOWN";    break;
        }
        return destination;
    }

    typedef LightweightDoublyLinkedList LDLLQueue;
    struct FRCRequest : public LightweightDoublyLinkedListNode
    {
        UINT32          id;                 ///< Needed to identify buffers returned from VPP
        CHIBUFFERINFO   imageBuffer;
        UINT64          timestamp;
        BOOL            isFRCInput;
        ChiMetadata*    pOutputMetadata;
        ChiMetadata*    pPartialMetadata;
        BOOL            isEos;

        SSMDestination  destination;

        // For debugging
        UINT32          srcFrameNumber;     ///< The capture request number received from the framework
        UINT32          resFrameNumber;     ///< The frame number used to return this frame to the client
        UINT32          frameIndex;         ///< The frameIndex corresponding to the captureRequestNum
        UINT32          captureRequestNum;  ///< The internal request number we assigned. We start counting
                                            ///< from 0 when we receive the first SSM request.

        // VPP specific
        V1_1::VppBuffer vppBuffer;
        BOOL            ownedByVPP;

        VOID PopulateVppBuffer();
    };

    struct VPPBufferManager
    {
        UINT32  inCount;
        UINT32  inQueued;
        UINT32  ibdCount;

        UINT32  outCount;
        UINT32  outQueued;
        UINT32  obdCount;
        UINT32  outCountMax;

        BOOL    isInFlushing;
        BOOL    isOutFlushing;

        UINT32  interpFactor;
        BOOL    clientSetInterp;

        VOID Reset()
        {
            memset(this, 0, sizeof(*this));
        }

        VOID DumpState()
        {
            CHX_LOG("VPP: in={cnt=%u, q=%u, ibd=%u} out={cnt=%u, q=%u, obd=%u, max=%u}",
                    inCount, inQueued, ibdCount, outCount, outQueued, obdCount, outCountMax);
        }
    } m_VppManager;

    struct ClientStats
    {
        UINT32  previewQueued;
        UINT32  previewReturned;
        UINT32  previewErrors;
        UINT32  videoQueued;
        UINT32  videoReturned;
        UINT32  videoErrors;

        void DumpState()
        {
            CHX_LOG("CLIENT: preview={q=%u, ret=%u, err=%u}, video={q=%u, ret=%u, err=%u}",
                    previewQueued, previewReturned, previewErrors,
                    videoQueued, videoReturned, videoErrors);
        }
    } m_clientStats;

    struct CamxStats
    {
        UINT32  previewQueued;
        UINT32  previewReturned;
        UINT32  videoQueued;
        UINT32  videoReturned;
        UINT32  captureRequestCount;

        void DumpState()
        {
            UINT32 videoHeld = videoQueued - videoReturned;
            UINT32 previewHeld = previewQueued - previewReturned;
            CHX_LOG("CAMX: preview={q=%u, ret=%u, held=%u}, "
                    "video={q=%u, ret=%u, held=%u}, "
                    "captureRequestCount=%u",
                    previewQueued, previewReturned, previewHeld,
                    videoQueued, videoReturned, videoHeld,
                    captureRequestCount);
        }
    } m_camxStats;

    struct SSMCaptureRequest : public LightweightDoublyLinkedListNode
    {
        UINT32          frameNumber;
        UINT32          frameIndex;
        UINT32          captureRequestNum;
        BOOL            isPreviewRequest;
        CHIBUFFERINFO   imageBuffer;
        SSMState        stateRequested;

        BOOL            notifyReceived;
        UINT64          tsNotify;

        BOOL            metaReceived;
        UINT64          tsMeta;

        // pInputMetadata is assigned at ExecuteCaptureRequest(). Depending on
        // the scenario, it is free'd:
        // (1) if the client video buffer for the request gets errored, then
        //     the reference is released when we error the buffer
        // (2) if we copy a frame into the client video buffer, then we send
        //     this input metadata to the base class and let it release the
        //     reference for us.
        ChiMetadata*    pInputMetadata;

        // pOutputMetadata is used to send a metadata handle back to the base class
        // when issuing pixel buffer results.
        ChiMetadata*    pOutputMetadata;

        // pResultOutputMetadata is used to defer metadata from the driver for a given
        // video buffer until that video buffer is released. It is allocated in ProcessResult
        // when metadata is received from the driver. The driver's metadata is merged
        // with this metadata. When we receive pixel buffers (whether in the same call to
        // ProcessResult or not) this metadata is "transferred" to an FRCRequest.
        // At some later point in time, that FRCRequest will show up in the "done" queue
        // to be picked up by the worker thread. After copying the pixel buffer from
        // the CHI video buffer into the client video buffer, an FRCRequest
        // is no longer required. However, since the pixel buffer is not immediately
        // returned to the base class, it is stored in pOutputMetadata.
        ChiMetadata*    pResultOutputMetadata;

        ChiMetadata*    pDriverPartialMetadata;

        BOOL            isEos;

        BOOL            hasClientVidBuf;
        CHISTREAMBUFFER clientVidBuf;
        BOOL            issueVideoError;
        BOOL            camxOwnsVidBuf;

        SSMDestination  destination;

        void Reset()
        {
            memset(this, 0, sizeof(*this));
        }
    };

    struct PreviewCaptureRequest : public LightweightDoublyLinkedListNode
    {
        UINT32  frameNumber;
        BOOL    metaReceived;
        BOOL    bufferReceived;

        void Reset()
        {
            memset(this, 0, sizeof(*this));
        }
    };

    struct CaptureRequestValidator {
        UINT32 m_captureRequestCount;   ///< number of valid capture requests received
        UINT32 m_numBatchedFrames;      ///< number of frames per batch
        UINT32 m_firstRequestMod;       ///< Frame number of the first request modulo batch size.
                                        ///< Used to determine whether an arbitrary frame
                                        ///< number is the leading request within a batch
        BOOL m_firstRequestSeen;        ///< whether we've seen the first request

        VOID Initialize(UINT32 numBatchedFrames)
        {
            m_numBatchedFrames = numBatchedFrames;
            m_firstRequestSeen = FALSE;
        }

        BOOL IsFirstRequestInBatch(
            camera3_capture_request_t* pRequest) const
        {
            BOOL                            isFirst     = FALSE;
            const camera3_stream_buffer_t*  pPreviewBuf = NULL;

            FindBuffersInRequest(pRequest, &pPreviewBuf, NULL);
            if (NULL != pPreviewBuf)
            {
                if (TRUE == m_firstRequestSeen)
                {
                    if (m_firstRequestMod == pRequest->frame_number % m_numBatchedFrames)
                    {
                        isFirst = TRUE;
                    }
                }
                else
                {
                    isFirst = TRUE;
                }
            }

            return isFirst;
        }

        CDKResult ValidateAndUpdate(
            camera3_capture_request_t* pRequest)
        {
            const camera3_stream_buffer_t*  pVideoBuf   = NULL;
            const camera3_stream_buffer_t*  pPreviewBuf = NULL;
            CDKResult                       result      = CDKResultSuccess;
            UINT32                          frameNumber = pRequest->frame_number;

            FindBuffersInRequest(pRequest, &pPreviewBuf, &pVideoBuf);

            if (m_numExpectedStreams < pRequest->num_output_buffers)
            {
                CHX_LOG_ERROR("frame_number=%u, unexpected number of streams, got %d",
                              frameNumber, pRequest->num_output_buffers);
                result = CDKResultEInvalidArg;
            }

            if (0 == (m_captureRequestCount % m_numBatchedFrames))
            {
                // First batch in frame, must contain PREVIEW
                if (NULL == pPreviewBuf)
                {
                    result = CDKResultEInvalidArg;
                    CHX_LOG_ERROR("frame_number=%u, leading client request lacks preview, "
                                  "pPreview=%p, pVideo=%p", frameNumber,
                                  pPreviewBuf, pVideoBuf);
                }
            }
            else
            {
                // Every other batch in the frame must contain VIDEO
                if (NULL == pVideoBuf || NULL != pPreviewBuf)
                {
                    result = CDKResultEInvalidArg;
                    CHX_LOG_ERROR("frame_number=%u, non-leading request, invalid"
                                  "buffers, pPreview=%p, pVideo=%p",
                                  frameNumber, pPreviewBuf, pVideoBuf);
                }
            }

            if (result == CDKResultSuccess)
            {
                m_captureRequestCount++;
                if (FALSE == m_firstRequestSeen)
                {
                    m_firstRequestSeen = TRUE;
                    m_firstRequestMod  = frameNumber % m_numBatchedFrames;
                }
            }

            return result;
        }
    } m_captureRequestTracker;

    UINT32                  m_numBatchedFrames;
    UINT32                  mWidth;
    UINT32                  mHeight;
    V1_1::VppColorFormat    mVppColorFormat;

    CDKResult DiscoverVendorTag(const CHAR* pTagName, UINT32* pTagLoc);
    CDKResult DiscoverVendorTagLocations();
    CDKResult WriteVendorTagInt32(ChiMetadata* pMetadata, UINT32 tag, INT32 val);

    CDKResult SetInterpFactorIfAllowed(
        camera3_capture_request_t* pRequest);

    BOOL ShouldStartRecording(
        camera3_capture_request_t* pRequest);

    CDKResult UpdateHDRModes(
        camera3_capture_request_t* pRequest);

    VOID FlushRequestQueue(LDLLQueue* pQueue);

    // Worker thread related
    CDKResult       LaunchWorkerThreads();
    CDKResult       JoinWorkerThreads();

    static VOID*    ClientOutputThread(VOID* pThreadData);
    VOID            HandleErrorClientVideo_l();
    VOID            HandleCopyInterpolatedOutput_l();
    VOID            HandleReleaseDeferredBuffer_l();
    VOID*           RunClientOutputThread();
    BOOL            ClientOutputThreadShouldSleep_l();
    VOID            ResetForNewRecording_l();

    static VOID*    ClientBufferReturnThread(VOID* pThreadData);
    VOID*           RunClientBufferReturnThread();
    BOOL            ClientBufferReturnThreadShouldSleep_l();

    VOID IssueGeneratedPartialMeta_l(FRCRequest* pRequest);

    VOID IssueShutterMessage(UINT32 frameNum, UINT64 timestamp);

    VOID IssueErrorMessage(
        CHIERRORMESSAGECODE error, CHISTREAM* pStream, UINT32 frameNum);

    VOID WaitForBuffersInCamxStream(BOOL isPreview);

    VOID GetSessionPrivateData(SessionPrivateData* pData)
    {
        if (NULL != pData)
        {
            pData->pUsecase     = m_perSessionPvtData[m_sessionId].pUsecase;
            pData->sessionId    = m_perSessionPvtData[m_sessionId].sessionId;
        }
    }

    CHIPRIVDATA* GetChiPrivateData(UINT32 frameNum)
    {
        return &m_privData[frameNum % MaxOutstandingRequests];
    }

    VOID DumpQueueStates()
    {
        CHX_LOG("QueueState: pre=%u, post=%u, pending=%u, proc={in=%u, out=%u}, done={in=%u, out=%u}, "
                "previewPending=%u, copiedClient=%u, client=%u, error=%u, freeReq=%u",
                m_preCapQ.NumNodes(), m_postCapQ.NumNodes(),
                m_frcPendingInQ.NumNodes(),
                m_frcProcInQ.NumNodes(), m_frcProcOutQ.NumNodes(),
                m_frcDoneInQ.NumNodes(), m_frcDoneOutQ.NumNodes(),
                m_pendingPreviewQ.NumNodes(), m_copiedClientVideoQ.NumNodes(),
                m_clientVideoQ.NumNodes(), m_errorVideoQ.NumNodes(), m_frcFreeQ.NumNodes());
    }

    VOID DumpCaptureResult(
        CHICAPTURERESULT*   pResult,
        const char*         pcStr       = NULL);

    static BOOL IsVideoStream(camera3_stream_t* pStream)
    {
        BOOL isVideoStream = FALSE;
        if (NULL != pStream)
        {
            isVideoStream = (pStream->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) ? TRUE : FALSE;
        }
        return isVideoStream;
    }

    static BOOL IsChiStreamVideo(CHISTREAM* pStream)
    {
        BOOL isChiStreamVideo = FALSE;
        if (NULL != pStream)
        {
            isChiStreamVideo = (pStream->grallocUsage & GrallocUsageHwVideoEncoder) ? TRUE : FALSE;
        }
        return isChiStreamVideo;
    }

    CHISTREAMFORMAT GetChiStreamFormatFromGrallocUsage(GrallocUsage usage)
    {
        CHISTREAMFORMAT fmt = ChiStreamFormatUBWCNV12;
        if (usage & ChiGralloc1ConsumerUsagePrivate_0)
        {
            fmt = ChiStreamFormatUBWCNV12;
        }
        else
        {
            CHX_LOG_ERROR("Unknown chi format from usage=%u, using default value", usage);
        }
        CHX_LOG("Converted usage=0x%x to chi format=%u", usage, fmt);
        return fmt;
    }

    V1_1::VppColorFormat GetVppColorFormatFromGrallocUsage(GrallocUsage usage)
    {
        V1_1::VppColorFormat fmt = V1_1::VppColorFormat::VPP_COLOR_FORMAT_UBWC_NV12;
        if (usage & ChiGralloc1ConsumerUsagePrivate_0)
        {
            fmt = V1_1::VppColorFormat::VPP_COLOR_FORMAT_UBWC_NV12;
        }
        else
        {
            CHX_LOG_ERROR("Unknown vpp format from usage=%u, using default value", usage);
        }
        CHX_LOG("Converted usage=0x%x to vpp format=%u", usage, fmt);
        return fmt;
    }

    BOOL ShouldExitClientThread_l()
    {
        return m_exitThreads;
    }

    BOOL ShouldErrorClientVideo()
    {
        BOOL                shouldError = FALSE;
        SSMCaptureRequest*  pCr         =
            reinterpret_cast<SSMCaptureRequest*>(m_clientVideoQ.Head());

        if (0 < m_clientVideoQ.NumNodes())
        {
            if (0 == m_frcDoneOutQ.NumNodes())
            {
                shouldError = TRUE;
            }

            else if (TRUE == pCr->isPreviewRequest)
            {
                shouldError = TRUE;
            }

            else
            {
                // Check whether video is behind preview meta, which could cause
                // preview to stall
                PreviewCaptureRequest* pPreviewCr =
                    reinterpret_cast<PreviewCaptureRequest*>(m_pendingPreviewQ.Head());
                if (NULL != pPreviewCr)
                {
                    UINT32 numBatchFrames       = ExtensionModule::GetInstance()->GetNumBatchedFrames();
                    UINT32 previewHeadFrameNum  = pPreviewCr->frameNumber;

                    while ((NULL != pPreviewCr) && (TRUE == pPreviewCr->metaReceived))
                    {
                        CHX_LOG("-- vid=%u | preview=%u, pxRx=%u, metaRx=%u",
                                pCr->frameNumber, pPreviewCr->frameNumber,
                                pPreviewCr->bufferReceived, pPreviewCr->metaReceived);
                        pPreviewCr = reinterpret_cast<PreviewCaptureRequest*>(pPreviewCr->pNext);
                    }

                    if ((NULL != pPreviewCr) &&
                        (pCr->frameNumber < (pPreviewCr->frameNumber + numBatchFrames)))
                    {
                        CHX_LOG("---> behind! jumping: video=%u, preview: head=%u, next=%u, target=%u",
                                pCr->frameNumber, previewHeadFrameNum,
                                pPreviewCr->frameNumber,
                                (pPreviewCr->frameNumber + numBatchFrames));
                        shouldError = TRUE;
                    }
                }
            }
        }

        return shouldError;
    }

    BOOL ShouldCopyInterpolatedOutput()
    {
        BOOL shouldCopy = FALSE;
        // There is a requirement from Google framework that SHUTTER messages
        // are returned in order.
        if ((0 < m_frcDoneOutQ.NumNodes()) && (0 < m_clientVideoQ.NumNodes()))
        {
            shouldCopy = TRUE;
        }
        return shouldCopy;
    }

    BOOL ShouldIssueErrorBuffers_l()
    {
        BOOL                shouldIssue = FALSE;
        SSMCaptureRequest*  pCr         =
            reinterpret_cast<SSMCaptureRequest*>(m_errorVideoQ.Head());
        if (NULL != pCr)
        {
            if (FALSE == pCr->camxOwnsVidBuf)
            {
                shouldIssue = TRUE;
                CHX_LOG("frame_number=%u, errQSz=%u to issue error",
                        pCr->frameNumber, m_errorVideoQ.NumNodes());
            }
            else
            {
                CHX_LOG("frame_number=%u, errQSz=%u, camx still owns video buffer, deferring",
                        pCr->frameNumber, m_errorVideoQ.NumNodes());
            }
        }
        return shouldIssue;
    }

    BOOL ShouldSendCopiedBuffer_l()
    {
        BOOL                    shouldSend  = FALSE;
        SSMCaptureRequest*      pSSMCr      =
            reinterpret_cast<SSMCaptureRequest*>(m_copiedClientVideoQ.Head());
        PreviewCaptureRequest*  pPreviewCr  =
            reinterpret_cast<PreviewCaptureRequest*>(m_pendingPreviewQ.Head());

        if (0 < m_copiedClientVideoQ.NumNodes())
        {
            if (0 < m_pendingPreviewQ.NumNodes())
            {
                if (pPreviewCr->frameNumber > pSSMCr->frameNumber)
                {
                    shouldSend = TRUE;
                }
            }
            else
            {
                // If there's no pending preview buffers but we have some copied
                // video buffers, then that means the preview buffer before
                // these video buffers must have already been sent, and
                // therefore these copied video buffers can be sent as well.
                shouldSend = TRUE;
            }
        }

        if (TRUE == shouldSend && FALSE == pSSMCr->camxOwnsVidBuf)
        {
            CHX_LOG("frame_number=%u, copiedClientVideoQ=%u, camx still owns buf",
                    pSSMCr->frameNumber, m_copiedClientVideoQ.NumNodes());
        }
        return shouldSend;
    }

    CDKResult FindRequiredStreams(
        camera3_stream_configuration_t* pStreamConfig);

    static VOID FindBuffersInRequest(
        camera3_capture_request_t* pRequest,
        const camera3_stream_buffer_t** ppPreviewBuf,
        const camera3_stream_buffer_t** ppVideoBuf)
    {
        for (UINT32 i = 0; i < pRequest->num_output_buffers; i++)
        {
            const camera3_stream_buffer_t* pBuf = &pRequest->output_buffers[i];
            if (TRUE == IsVideoStream(pBuf->stream))
            {
                if (NULL != ppVideoBuf)
                {
                    *ppVideoBuf = pBuf;
                }
            }
            else
            {
                if (NULL != ppPreviewBuf)
                {
                    *ppPreviewBuf = pBuf;
                }
            }
        }
    }

    SSMCaptureRequest* GetSSMCaptureRequest(UINT32 frameNum)
    {
        return &m_requestTracker[frameNum % MaxOutstandingRequests];
    }

    void ReleaseChiVideoBuffer(CHIBUFFERINFO* pBufInfo)
    {
        m_bufferManager->ReleaseReference(pBufInfo);
        pBufInfo->phBuffer = NULL;
    }

    BOOL IsFinishedAllProcessing_l()
    {
        return ((SSMState::SSM_PROCESS == m_State) &&
                (0 == m_frcDoneOutQ.NumNodes()) &&
                (0 == m_frcProcInQ.NumNodes())) ? TRUE : FALSE;
    }

    /* VPP APIs */
    VOID OnInputBufferDone(const V1_1::VppBuffer* pVppBuf);
    VOID OnOutputBufferDone(const V1_1::VppBuffer* pVppBuf);
    VOID OnEvent(V1_1::VppEvent e);

    CDKResult InitializeVPP();
    CDKResult DestroyVPP(UINT32 bDoLock);
    CDKResult ConfigureVPP(UINT32 interpFactor);

    CDKResult QueueBuffersToVpp();
    CDKResult QueueBuffersToVppIn();
    CDKResult QueueBuffersToVppOut();
    CDKResult QueueOneBufferToVppOut();
    CDKResult QueueBufferToVppIn(FRCRequest* pRequest);

    FRCRequest* GetFRCRequestFromVppBuffer(const V1_1::VppBuffer* pVppBuf);
    VOID DoneWithFRCRequest_l(FRCRequest* pRequest);

    struct SSMHidlVppCallbacks : public V1_1::IHidlVppCallbacks
    {
        SSMHidlVppCallbacks(UsecaseSuperSlowMotionFRC* usecase)
        {
            m_pUsecase = usecase;
        }
        android::hardware::Return<uint32_t> inputBufferDone(const V1_1::VppBuffer& buf) override;
        android::hardware::Return<uint32_t> outputBufferDone(const V1_1::VppBuffer& buf) override;
        android::hardware::Return<uint32_t> vppEvent(const V1_1::VppEvent& e) override;

        UsecaseSuperSlowMotionFRC* m_pUsecase;
    };

    struct SSMDeathRecipient : public android::hardware::hidl_death_recipient
    {
        public:
            SSMDeathRecipient(UsecaseSuperSlowMotionFRC* usecase)
            {
                m_pUsecase = usecase;
            }
            // Callback
            virtual void serviceDied(uint64_t cookie,
                                     const android::wp<android::hidl::base::V1_0::IBase>& who);

        private:
            UsecaseSuperSlowMotionFRC* m_pUsecase;
    };

    static const UINT32 MaxGetServiceRetries        = 100;
    static const UINT32 VppServiceRetryIntervalUS   = 1000;

    android::sp<V1_2::IHidlVpp>         m_VppSession;
    android::sp<V1_2::IHidlVppService>  m_VppService;
    android::sp<SSMDeathRecipient>      m_DeathRecipient;
    BOOL                                m_VppInitialized;

    /* Slow Motion */
    UINT32                  m_clientId = ChiMetadataManager::InvalidClientId;

    UINT32                  m_clientVideoRetSuccess;
    CHIBufferManager*       m_bufferManager;            ///< allocates buffers for FRC input/output
    ChiMetadata*            m_pCommonInputMetadata;     ///< The 1 input meta that is sent to UCB with all buffers

    SSMState                m_State;
    SSMCaptureRequest       m_requestTracker[MaxOutstandingRequests];
    LDLLQueue               m_clientVideoQ;
    LDLLQueue               m_errorVideoQ;
    BOOL                    m_sendCapComplete;

    LDLLQueue               m_copiedClientVideoQ;
    PreviewCaptureRequest   m_previewFrameNumTracker[MaxOutstandingRequests];
    LDLLQueue               m_pendingPreviewQ;

    UINT64                  m_buffersReturned;
    UINT64                  m_videoTsAdjustPeriod;

    PreviewCaptureRequest* GetPreviewCaptureRequest(UINT32 frameNum)
    {
        return &m_previewFrameNumTracker[frameNum % MaxOutstandingRequests];
    }

    VOID AppendPendingPreviewQueue_l(UINT32 frameNumber)
    {
        PreviewCaptureRequest* pPreviewCr = GetPreviewCaptureRequest(frameNumber);
        pPreviewCr->Reset();
        pPreviewCr->frameNumber     = frameNumber;
        pPreviewCr->metaReceived    = FALSE;
        pPreviewCr->bufferReceived  = FALSE;
        CHX_LOG("frame_number=%u, appending pending preview=%p", frameNumber, pPreviewCr);
        m_pendingPreviewQ.InsertToTail(pPreviewCr);
    }

    VOID RemovePendingPreviewQueue_l()
    {
        BOOL                    removedFromQueue    = FALSE;
        PreviewCaptureRequest*  pPendingPreviewCr   =
            reinterpret_cast<PreviewCaptureRequest*>(m_pendingPreviewQ.Head());

        while ((NULL != pPendingPreviewCr) &&
               pPendingPreviewCr->metaReceived && pPendingPreviewCr->bufferReceived)
        {
            CHX_LOG("frame_number=%u, removing this pending preview=%p",
                    pPendingPreviewCr->frameNumber, pPendingPreviewCr);

            // No need to do anything with this node since we always just index
            // into the array
            m_pendingPreviewQ.RemoveFromHead();

            removedFromQueue    = TRUE;

            pPendingPreviewCr   =
                reinterpret_cast<PreviewCaptureRequest*>(m_pendingPreviewQ.Head());
        }

        if (TRUE == removedFromQueue)
        {
            m_pCond->Broadcast();
        }
    }

    static const UINT32 DefaultVideoFPS             = 30;
    static const UINT32 DefaultPreCapDuration       = 1;
    static const UINT32 DefaultPostCapDuration      = 1;

    static const UINT32 MaxSSMFramesToCapture       = 120;
    static const UINT32 NumFramesPreCap             = DefaultVideoFPS * DefaultPreCapDuration;
    static const UINT32 NumFramesPostCap            = DefaultVideoFPS * DefaultPostCapDuration;
    static const UINT32 ExtraBuffersNeededForProc   = 64; // to keep HFR pipeline running
    static const UINT32 FRCOutputPortReq            = 5;
    static const UINT32 MaxOutstandingFRCRequests   = (MaxSSMFramesToCapture +
                                                       NumFramesPreCap +
                                                       NumFramesPostCap +
                                                       ExtraBuffersNeededForProc +
                                                       FRCOutputPortReq);
    static const UINT32 MaxBufsToFRCOut             = 30;

    static const UINT32 CamxWaitForBufferDuration   = 1000;

    // Flushing and sending a batch must be mutually exclusive because if you
    // flush in the middle of a batch, camxsession rejects the remaining capture
    // requests that came after the flush, meaning that the session will have
    // a partial batch and time out when asked to flush. The HFR usecase does
    // not explicitly have to do this because it is done on the framework-level
    // via ConstrainedHighSpeed (which SSMC does not use).
    UINT32              m_flushingOrSendingBatch;

    UINT32              m_SSMFramesCaptured;
    UINT32              m_Fps;                  // FPS of video stream

    UINT64              m_lastCopiedTimestamp;
    UINT64              m_firstFRCTimestamp;
    UINT32              m_lastSSMCapReq;

    UINT32              m_nextFRCRequestId;
    UINT32 GetNewFRCRequestId()
    {
        return m_nextFRCRequestId++;
    }

    struct PrePostCapConfig
    {
        UINT32  duration;       // Duration of pre/post capture segments (in seconds)
        UINT32  fps;            // FPS of pre/post capture segments
        UINT32  numFrames;      // fps x duration
        UINT32  sampleRate;     // fps / prepostCapFps

        UINT32  captureReqCnt;

        VOID Configure(UINT32 nativeFps, UINT32 samplingFps, UINT32 samplingDuration)
        {
            fps         = samplingFps;
            duration    = samplingDuration;
            numFrames   = samplingFps * samplingDuration;
            sampleRate  = nativeFps / samplingFps;
        }

        VOID Debug()
        {
            CHX_LOG("duration=%u, fps=%u, numFrames=%u, sampleRate=%u",
                    duration, fps, numFrames, sampleRate);
        }

        BOOL IsEnabled()
        {
            return numFrames > 0 ? TRUE : FALSE;
        }
    };

    PrePostCapConfig    m_preCapCfg;
    PrePostCapConfig    m_postCapCfg;

    FRCRequest          m_FRCRequests[MaxOutstandingFRCRequests];
    LDLLQueue           m_frcFreeQ;
    LDLLQueue           m_preCapQ;      // Normal speed buffers
    LDLLQueue           m_postCapQ;     // Normal speed buffers
    LDLLQueue           m_frcPendingInQ;// Requests that are waiting to be queued to FRC
    LDLLQueue           m_frcProcInQ;   // Requests that are currently sent to FRC input
    LDLLQueue           m_frcProcOutQ;  // Requests that are currently sent to FRC output
    LDLLQueue           m_frcDoneInQ;   // Requests returned as IBD
    LDLLQueue           m_frcDoneOutQ;  // Requests returned as OBD

    BOOL                m_exitThreads;
    PerThreadData       m_clientOutputThread;
    PerThreadData       m_clientBufferRetThread;
    Mutex*              m_pMutex;
    Condition*          m_pCond;

    ChiStream*          m_pPreviewStream;
    ChiStream*          m_pVideoStream;
    static const UINT32 m_numExpectedStreams = 2;

    UINT                m_sessionId;
    UINT                m_pipelineId;

    ChiCallBacks*       m_pCallbacks;          ///< Notify and captureResult callbacks for each pipeline
};

#endif // CHXSUPERSLOWMOTIONFRCUSECASE_H
