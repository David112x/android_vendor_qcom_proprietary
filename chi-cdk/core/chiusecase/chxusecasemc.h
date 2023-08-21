////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxusecasemc.h
/// @brief CHX MultiCamera usecase base class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXUSECASEMC_H
#define CHXUSECASEMC_H

#include <assert.h>
#include <map>

#include "chxadvancedcamerausecase.h"
#include "chxincs.h"
#include "chxmulticamcontroller.h"
#include "chxpipeline.h"
#include "chxsession.h"
#include "chxusecaseutils.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

const UINT32 NumOfInSensorHDR3ExpPreviewDelay   = 2;
const UINT32 DebugDataMaxOfflineFramesMC        = 16;

/// Meta-struct that describes the full/downscale buffer information of a ChiTargetPortDescriptorInfo struct
typedef struct
{
    UINT numFull;                            ///< The number of full buffer targets and the length of mapFull
    UINT numDS4;                             ///< The number of ds4 buffer targets and the length of mapDS4
    UINT numDS16;                            ///< The number of ds16 buffer targets and the length of mapDS16
    UINT mapFull[MaxDevicePerLogicalCamera]; ///< A list of indexes to full target buffers
    UINT mapDS4[MaxDevicePerLogicalCamera];  ///< A list of indexes to ds4 target buffers
    UINT mapDS16[MaxDevicePerLogicalCamera]; ///< A list of indexes to ds16 target buffers
} DownscaleTargetBufferInfo;

/// Given a ChiTargetPortDescriptor, find the Full/DS buffer properties of a sink/source target buffer list
CDKResult GetDownscaleTargetBufferInfo(
    ChiTargetPortDescriptorInfo* pTargetDescInfo,   ///< Input to the target description info
    DownscaleTargetBufferInfo&   targetBufferInfo); ///< Output reference to buffer info that will be populated

/// Usecase enum required for remapping the pipeline ID's based on respective UsecaseID
enum MultiCameraUsecase
{
    UsecaseRTB,              ///< RTB Preview/Snapshot
    UsecaseSAT,              ///< SAT Preview/Snapshot
    UsecaseMax               ///< Reserved
};

/// multi camera session type
enum MultiCameraSession
{
    REALTIME_SESSION,                 ///< RAW and YUV session.
    OFFLINE_YUV_SESSION,              ///< Preview output session
    OFFLINE_RDI_SESSION0,             ///< Raw to YUV session for Cam0
    OFFLINE_RDI_SESSION1,             ///< Raw to YUV session for Cam1
    OFFLINE_RDI_SESSION2,             ///< Raw to YUV session for Cam2
    OFFLINE_RDI_SESSION3,             ///< Raw to YUV session for Cam3
    OFFLINE_FUSION_SESSION,           ///< Snapshot fusion session
    OFFLINE_JPEG_SESSION,             ///< Jpeg Session
    OFFLINE_RAW16_SESSION,            ///< RAW16 Session
    MAX_MULTI_CAMERA_SESSION,
};

enum MultiCameraRawCBType
{
    RawCBTypeNone = 0,
    RawCBTypeIdeal,
    RawCBTypeSensor
};


static const UINT32 g_maxPipelines = 16;

struct SessionPipelineMap
{
    UINT32 sessionIndex;                    ///< Session this pipeline belongs to
    UINT32 numPipelines;
    UINT32 xmlPipeline[g_maxPipelines];       ///< Pipeline index from xml
};

struct UsecaseMapInfo
{
    UINT32             numPhyCameras;                 ///< Number of physical cameras
    UINT32             usecaseMode;                   ///< Usecase mode like RTB/SAT
    SessionPipelineMap map[MAX_MULTI_CAMERA_SESSION]; ///< Pipeline to Session mapping
    ChiUsecase*        pUsecaseXML;                   ///< pointer to usecase
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Multi-camera usecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UsecaseMultiCamera final : public AdvancedCameraUsecase
{
public:

    /// Static create function to create an instance of the object
    static UsecaseMultiCamera* Create(
        LogicalCameraInfo*              cameraInfo,       ///< Camera Info
        camera3_stream_configuration_t* pStreamConfig);  ///< Stream configuration;

    CHX_INLINE const Session* GetRealtimeSession()
    {
        return m_sessionInfo[REALTIME_SESSION].m_pSession;
    }

    static const UINT32  DUAL_CAMERA_COUNT           = 2;
    static const UINT32  META_READY_FLAG             = 1 << 0;   ///< Flag to indicate meta arrived or not
    static const UINT32  BUF_READY_FLAG              = 1 << 1;   ///< Flag to indicate buffer arrived or not
    static const UINT32  DS4_BUF_READY_FLAG          = 1 << 2;   ///< Flag to indicate DS4 buffer arrived
    static const UINT32  DS16_BUF_READY_FLAG         = 1 << 3;   ///< Flag to indicate DS16 buffer arrived

    static const UINT32  DUMMYWIDTH                  = 640;      ///< Dummy stream width for pipeline create to work in video
    static const UINT32  DUMMYHEIGHT                 = 480;      ///< Dummy stream height for pipeline create to work in video
    static const UINT    INVALID_CAMERAID            = 0xFFFF;   ///< Invalid camera ID

    const static INT     MaxFeatureSupported         = 10;

    static const UINT32  MAX_NUMBER_TARGETS          = 32;       ///< Max numbers of target
    static const UINT32  MAX_NUMBER_PIPELINES        = 32;       ///< Max numbers of pipeline
    static const UINT32  MAX_EMPTY_RTMETA_COUNT      = 32;       ///< Max realtime meta holders
    static const UINT32  MAX_EMPTY_OFFLINEMETA_COUNT = 8;        ///< Max offline meta holders
    static const UINT32  MaxTargetsPerPipelines      = 4;
    static const UINT32  MAX_RESOURCE_AVAILABLE      = 2;

    enum AdvanceFeatureId
    {
        AdvanceFeatureMFNR = 0,
        AdvanceFeatureHDR,
        AdvanceFeatureSWMF,
        AdvanceFeatureMFSR,
        AdvanceFeatureMax,
    };

    virtual VOID ProcessFeatureDataNotify(
        UINT32   appFrameNum,
        Feature* pCurrentFeature,
        VOID*    pData);

    virtual VOID ProcessFeatureDone(
        UINT32 appFrameNum,
        Feature* pCurrentFeature,
        CHICAPTURERESULT* pResult);

    CHIMETAHANDLE GetAvialableResultMetadata()
    {
        return m_hResultMetadataHandle;
    }

    virtual VOID LogFeatureRequestMappings(
        UINT32          inFrameNum,
        UINT32          reqFrameNum,
        const CHAR*     identifyingData) override;

protected:
    /// Destroy/Cleanup the object
    virtual VOID Destroy(BOOL isForced);

    /// Flush
    virtual CDKResult ExecuteFlush();
private:

    UsecaseMultiCamera() = default;
    virtual ~UsecaseMultiCamera();

    // Do not allow the copy constructor or assignment operator
    UsecaseMultiCamera(const UsecaseMultiCamera&) = delete;
    UsecaseMultiCamera& operator= (const UsecaseMultiCamera&) = delete;

    static VOID SessionCbCaptureResult(
        ChiCaptureResult* pCaptureResult,                       ///< Capture result
        VOID*             pPrivateCallbackData);                ///< Private callback data

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseBuffersOfActivePipelines
    ///
    /// @brief  Function to release all active pipelines buffers
    ///
    /// @param  pCaptureResult        Capture result object pointed
    /// @param  pPrivateCallbackData  Private Callback Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ReleaseBuffersOfActivePipelines(
        ChiCaptureResult* pCaptureResult,                       ///< Capture result
        VOID*             pPrivateCallbackData);                ///< Private callback data


    static VOID SessionCbPartialCaptureResult(
        ChiPartialCaptureResult* pPartialCaptureResult,         ///< Partial capture result
        VOID*                    pPrivateCallbackData);         ///< Private callback data

    static VOID SessionCbNotifyMessage(
        const ChiMessageDescriptor* pMessageDescriptor,         ///< Message Descriptor
        VOID*                       pPrivateCallbackData);      ///< Private callback data

    /// Execute capture request
    CDKResult ExecuteCaptureRequest(
        camera3_capture_request_t* pRequest);                   ///< Request parameters

    /// Used to decouple the metadata for the pipeline being flushed during pipeline flush
    CDKResult DecoupleMetadata(
        ChiMetadata* pMetadata,
        UINT32       pipelineIndex);

    /// Process the available results
    virtual CDKResult NotifyResultsAvailable(
        const CHICAPTURERESULT* pResult);                       ///< Capture results

    /// This will be called by the usecase when Partial Result is available from the driver
    VOID ProcessDriverPartialCaptureResult(
        CHIPARTIALCAPTURERESULT*    pResult,
        UINT32                      sessionId);

    /// This will be called by the usecase when Partial Result is available from CHI
    VOID ProcessCHIPartialData(
        UINT32    frameNum,
        UINT32    sessionId);

    /// Process result from realtime pipelines
    VOID RealtimeCaptureResult(
        ChiCaptureResult* pResult);                             ///< Request parameters

    /// Process result from offline preview pipeline
    VOID OfflinePreviewResult(
        ChiCaptureResult* pResult);                             ///< Request parameters

    /// Process result from offline RAW16 pipeline
    VOID OfflineRAW16Result(
        ChiCaptureResult* pResult);                             ///< Request parameters

    /// Process result from offline jpeg pipeline
    VOID OfflineJPEGResult(
        ChiCaptureResult* pResult);                             ///< Request parameters

    /// Process result from offline snapshot pipeline
    VOID OfflineSnapshotResult(
        const ChiCaptureResult* pResult, UINT32 sessionIdx);    ///< Request parameters

    /// Process result from offline snapshot fusion pipeline
    VOID OfflineFusionSnapshotResult(
        const ChiCaptureResult* pResult);                       ///< Request parameters

    /// Process result from offline snapshot active pipeline
    VOID OfflineYUVCallbackResult(
        ChiCaptureResult* pResult, UINT32 sessionIdx);          ///< Request parameters

    /// Process result from offline snapshot deactive pipeline
    VOID OfflineYUVErrorResult(
        UINT32 applicationFrameNum, UINT32 sessionIdx);         ///< application FrameNum

    /// Process notify messages from real time session
    VOID ProcessRealTimeNotifyMessage(
        const ChiMessageDescriptor* pMessageDescriptor);

    /// Dispatch a capture request to realtime pipelines
    CDKResult GenerateRealtimeRequest(
        camera3_capture_request_t* pRequest,                    ///< Request parameters
        ChiMetadata*               pFeatureInputMetadata);      ///< Feature input meta

    CDKResult processSessionRequest(
        UINT32             sessionId,                           ///< Session index
        UINT               numRequest,                          ///< Number of CHI request
        CHICAPTUREREQUEST* pRequest);                           ///< Request parameters

    VOID processRDIFrame(
        const ChiCaptureResult* pResult);                       ///< Request parameters

    VOID processRAWCallback(
        const ChiCaptureResult* pResult);                       ///< Request parameters

    /// we can only deep copy between chi buffer here,so need to cast them to chi buffer and do the copy
    VOID deepCopyFromChiToHal(
        camera3_stream_buffer_t*       dstHalBufferSnap,        ///< for snapshot
        const CHISTREAMBUFFER*         srcChiBuffer);           ///< the result callback buffer

    CDKResult processRAW16Frame(
        const ChiCaptureResult* pResult);                       ///< Request parameters

    UsecaseMapInfo* GetUsecaseInfo();

    UINT32 GetSessionIndex(
        UINT32              pipelineIndexFromUsecase,
        MultiCameraUsecase  usecase);

    /// Does one time initialization of the created object
    CDKResult Initialize(
        LogicalCameraInfo*              cameraInfo,             ///< Camera Info
        camera3_stream_configuration_t* pStreamConfig);         ///< Stream configuration

    CDKResult ClassifyTargetStream(camera3_stream_configuration_t* pStreamConfig);

    ChiUsecase* SelectUsecaseXML();

    /// Does one time initialization of the created object
    CDKResult InitializeAdvanceFeatures(
        LogicalCameraInfo*              pCameraInfo,             ///< Camera Info
        camera3_stream_configuration_t* pStreamConfig);         ///< Stream configuration


    /// Create Multi Controller
    CDKResult CreateMultiController(
        LogicalCameraInfo*              pCameraInfo,            ///< Camera Info
        camera3_stream_configuration_t* pStreamConfig);         ///< Stream configuration

    /// Create Offline Sessions
    CDKResult CreateOfflineSession();

    CDKResult CreateSession(
        UINT32 sessionId);

    /// Setup Offline Pipeline mapping
    CDKResult SetupOfflinePipelineMapping(
        UINT                         sessionIdx,
        ChiTargetPortDescriptorInfo* pSrcTarget,
        ChiStream**                  ppSourceChiStream,
        UINT&                        sourceIndex,
        ChiPhysicalCameraConfig&     physicalCameraConfiguration,
        DownscaleTargetBufferInfo&   targetBufferInfo);

    /// Create Pipeline
    CDKResult CreatePipelines(
        ChiUsecase*                     pChiUsecase,
        camera3_stream_configuration_t* pStreamConfig,
        BOOL                            doCrossPipelineBufferNegotiations);

    /// Process the saved results
    VOID ProcessResults();

    /// Function to determine if the stream is a Rdi stream
    BOOL IsRdiStream(
        CHISTREAM* pStream) const;

    /// Function to determine if the stream is a FD stream
    BOOL IsFDStream(
        CHISTREAM* pStream) const;

    BOOL IsRdiRAW16Stream(
        CHISTREAM* pStream) const;

    /// Function to determine if the stream is a ideal raw stream
    BOOL IsIdealRAWStream(
        CHISTREAM* pStream) const;

    /// Function to determine if the stream is a realtime preview stream
    BOOL IsRTPreviewStream(
        CHISTREAM* pStream) const;

    /// Function to determine if the stream is a realtime DS4 preview stream
    BOOL IsRTPreviewDS4Stream(
        CHISTREAM* pStream) const;

    /// Function to determine if the stream is a realtime DS16 preview stream
    BOOL IsRTPreviewDS16Stream(
        CHISTREAM* pStream) const;

    BOOL canEnableAdvanceFeature() const
    {
        return ((NULL == m_pTargetVideoStream) && (NULL != m_pTargetSnapshotStream));
    }

    /// Function to determine if offline pipeline requires DS4 source buffer
    CHX_INLINE BOOL IsDS4RequiredOffline()
    {
        return ((m_offlinePipelineExpectationMask & DS4_BUF_READY_FLAG) == DS4_BUF_READY_FLAG) ? TRUE : FALSE;
    }

    // Function to determine if offline pipeline requires DS16 source buffer
    CHX_INLINE BOOL IsDS16RequiredOffline()
    {
        return ((m_offlinePipelineExpectationMask & DS16_BUF_READY_FLAG) == DS16_BUF_READY_FLAG) ? TRUE : FALSE;
    }

    /// dual camera start up optimization function
    CDKResult ActivateDeactivateRealtimePipeline(
        ControllerResult*           pMccResult,
        UINT64                      requestId,
        camera3_capture_request_t*  pRequest);

    /// Defer snapshot related session thread function
    static VOID* DeferSnapSessionThread(
        VOID* pThreadData);

    /// defer snapshot session create function
    VOID DeferSnapSession();

    CDKResult CreateOfflineYUVSession();

    CDKResult WaitForDeferSnapThread();

    /// start defer thread
    VOID StartDeferThread();

    /// intialize defer related resource
    VOID InitializeDeferResource();

    /// destroy defer related resource
    VOID DestroyDeferResource();

    /// check if there is preview stream for this request
    BOOL hasPreviewStreamRequest(
        camera3_capture_request_t* pRequest) const
    {
        BOOL hasPreviewRequest = FALSE;
        for (UINT i = 0; i < pRequest->num_output_buffers; i++)
        {
            if (((CHISTREAM *)pRequest->output_buffers[i].stream == m_pTargetPreviewStream) ||
                ((NULL != m_pTargetVideoStream) &&
                (CHISTREAM *)pRequest->output_buffers[i].stream == m_pTargetVideoStream))
            {
                hasPreviewRequest = TRUE;
                break;
            }
        }
        return hasPreviewRequest;
    }

    /// check if there is snapshot stream for this request
    BOOL hasSnapshotStreamRequest(
        camera3_capture_request_t* pRequest) const
    {
        BOOL hasSnapshotRequest = FALSE;
        for (UINT i = 0; i < pRequest->num_output_buffers; i++)
        {
            if ((NULL != m_pTargetSnapshotStream) &&
                (CHISTREAM *)pRequest->output_buffers[i].stream == m_pTargetSnapshotStream)
            {
                hasSnapshotRequest = TRUE;
                break;
            }
        }
        return hasSnapshotRequest;
    }

    UINT32 GetActivePipelineIndex(
        UINT32 masterCameraId) const
    {
        UINT32 activePipelineIndex = 0;

        for (UINT32 i = 0; i < m_numOfPhysicalDevices; i++)
        {
            if (masterCameraId == m_sessionInfo[REALTIME_SESSION].m_pSession->GetCameraId(i))
            {
                activePipelineIndex = i;
                break;
            }
        }

        return activePipelineIndex;
    }

    BOOL getFeatureDependency(Feature* pFeature)
    {
        BOOL ret = FALSE;
        if ((NULL != pFeature) &&
            ((FeatureType::MFNR == pFeature->GetFeatureType()) || (FeatureType::SWMF == pFeature->GetFeatureType())))
        {
            ret = TRUE;
        }
        return ret;
    }

    /// offline request
    CDKResult CreateOfflineProcessResource();

    VOID DestroyOfflineProcessResource();

    // Pick input framenumber from RDI queue to do snapshot
    CDKResult PickInputFrameForSnapshot(
        UINT32 snapshotReqId);

    // Create buffer and metadata array for offline process
    CDKResult CreateInputResourceForOffline(
        UINT32 snapshotReqId);

    // Send offline process request
    CDKResult SendOfflineProcessRequest(
        UINT32 snapshotReqId);

    // Update Snapshot metadata
    VOID UpdateSnapshotMetadataForFeature(
        UINT32 requestFrameNum,
        UINT32 pipelineIndex);

    VOID OfflineRequestProcess();

    static VOID* OfflineRequestThread(
        VOID* pArg);

    VOID TriggerOfflineRequest(
        UINT32                     frameNumber,
        camera3_capture_request_t* pRequest,
        ChiMetadata*               pFeatureMetadata);

    VOID GenerateSnapshotRequest(
        UINT32 frameNumber,
        UINT32 rtIndex,
        UINT32 bufferIndex = 0);

    BOOL isOfflineProcessRequired(
        camera3_capture_request_t* pRequest,
        RequestMapInfo*            pRequestMapInfo);

    VOID UpdateRequestMapInfo(camera3_capture_request_t* pRequest,
            RequestMapInfo*                              pRequestMapInfo,
            FeatureRequestInfo*                          pRequestInfo,
            Feature*                                     pFeature,
            UINT32                                       featureReqIdx);

    VOID UpdateFeatureType(
            camera3_capture_request_t* pRequest,
            RequestMapInfo*            pRequestMapInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessAppSettings
    ///
    /// @brief  Function to get and process App settings value
    ///
    /// @param  pRequest Capture request info from hal3
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessAppSettings    (
        camera3_capture_request_t* pRequest);

    /// Function to set session parameter per pipeline
    CDKResult SetSessionSettings(
        Pipeline*                       m_pPipeline,
        camera3_stream_configuration_t* pStreamConfig);

    /// Function to update session parameter per pipeline
    CDKResult UpdateSessionSettings(
        UINT32  sessionId);

    /// Function to signal threads waiting for results
    CDKResult SignalResult(
        UINT32 sessionIdx,
        UINT32 pipelineIndex);

    /// Function to wait for all results
    CDKResult WaitUntilResultDrain(
        UINT64 requestId,
        UINT32 sessionIdx,
        UINT32 pipelineIndex);

    /// Function to select Pipeline Index to Flush
    UINT32 SelectPipelineToFlush(
        ControllerResult * pMccResult,
        UINT64 requestId);

    /// Function to Flush and deactivate give pipeline
    CDKResult FlushPipeline(
        UINT flushPipelineIndex);

    /// Function to get Physical device info
    DeviceInfo *getPhysicalDeviceInfo(
        UINT32 cameraId);

    CDKResult ActivatePendingPipeline();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseInputMetadata
    ///
    /// @brief  Release input metadata if necessary
    ///
    /// @param pResult     Capture result object pointed
    /// @param sessionId     Session Identifier
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseInputMetadata(
        ChiCaptureResult* pResult,
        UINT              sessionId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseReferenceToRTOutputBuffers
    ///
    /// @brief  Release output buffers from realtime pipeline result
    ///
    /// @param pResult     Capture result object pointed
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseReferenceToRTOutputBuffers(
        ChiCaptureResult* pResult);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseRequestBuffers
    ///
    /// @brief  Release Request Buffer if requst is cancelled
    ///
    /// @param sessionId            Session Id
    /// @param numRequest           Number of requests
    /// @param CHICAPTUREREQUEST    Request pointer
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseRequestBuffers(
        UINT32             sessionId,
        UINT               numRequest,
        CHICAPTUREREQUEST* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleErrorMessage
    ///
    /// @brief  Determines how to deal with certain error messages
    ///
    /// @param  pMessageDescriptor     Message Descriptor
    /// @param  pPrivateCallbackData   Private Callback Data
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleErrorMessage(
        const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
        VOID*                       pPrivateCallbackData);

    struct BufferQueue
    {
        UINT32              frameNumber;                              ///< Frame number
        ChiMetadata*        pMetadata[MaxRealTimePipelines];          ///< Array of metadata
        CHISTREAMBUFFER     buffer[MaxRealTimePipelines];             ///< RT buffer. Assuming one sink per pipeline
        CHISTREAMBUFFER     ds4Buffer[MaxRealTimePipelines];          ///< DS4 RT buffer
        CHISTREAMBUFFER     ds16Buffer[MaxRealTimePipelines];         ///< DS16 RT buffer
        BOOL                valid[MaxRealTimePipelines];              ///< valid flag
        UINT32              errMask[MaxRealTimePipelines];            ///< Mask of err types in Msg Cb
        UINT32              flushPipelineIndex;                       ///< Index non-zero val if flushed
        UINT32              m_resultMask;                             ///< Flag to check if all result of this
                                                                      ///  request is ready
    };

    struct RequestInfo
    {
        UINT32                masterPipelineIndex;                    ///< Tells the master pipeline index
        UINT32                m_activeMask;                           ///< lpm info per pipeline
        UINT32                m_snapshotMask;                         ///< Mask to find snapshot stream in request.
        UINT32                m_videoMask;                            ///< Mask to find video stream in request.
        UINT32                m_postviewMask;                         ///< Mask to find postview stream in request.
        UINT32                m_previewMask;                          ///< Mask to find preview stream in request.
        UINT32                m_rawMask;                              ///< Mask to find raw stream in request.
        UINT32                m_fdMask;                               ///< Mask to find fd stream in request
        UINT32                m_yuvPreviewMask;                       ///< Mask to find YUV stream in request.
        UINT32                m_rawcbMask;                            ///< Mask to find RAW stream in request.
        MultiCameraRawCBType  m_rawCbType;                            ///< flag to differentiate between ideal/sensor raw cb
        ChiMetadata*          m_pOutputMetaData[MaxRealTimePipelines];///< Metadata buffer associated with the request
    };

    struct jpegRequestInfo
    {
        UINT64     requestId;
        CHISTREAM* stream;
    };

    VOID ReturnFrameworkMetadata(
        camera3_capture_result_t *pResult);

    VOID SkipPreviousFrame(
        UINT32 internalFrameNumber,
        UINT32 nSkipFrames);

    VOID SendNopRequest(
        UINT32           nFrames,
        ControllerResult mccResult);

    CDKResult SubmitJpegRequest(
            UINT32              frameNum,
            CHISTREAMBUFFER*    inputBuffer,
            UINT                numOutputBuffers,
            CHISTREAMBUFFER*    JpegBuffer,
            ChiMetadata*        pMetadata,
            CHIBufferManager*   pInputBufferManager);

    /// Merges the provided input metadatas into the result
    CDKResult AppendOfflineMetadata(
            UINT32       frameNum,
            ChiMetadata* pMetadata1,
            ChiMetadata* pMetadata2,
            ChiMetadata* pResultMeta,
            BOOL         isSnapshotMeta,
            BOOL         bReleaseInputMeta); ///< flag to indicate whether the caller needs to keep
                                             ///  input metadata refererences

    VOID UpdateAdvanceFeatureStatus(
        Feature* pFeature);

    /// Function to check whether AEC trigger the in-sensor HDR 3 exposure
    CHX_INLINE BOOL IsAECTriggerInSensorHDR3Exp(
        AECFrameControl* pAECFrameControl)
    {
        return ((NULL != pAECFrameControl)                                                  &&
                (TRUE == pAECFrameControl->inSensorHDR3ExpTriggerOutput.isTriggerInfoValid) &&
                (TRUE == pAECFrameControl->isInSensorHDR3ExpSnapshot))? TRUE: FALSE;
    }

    /// Function to check whether it is manual mode snapshot
    CHX_INLINE BOOL IsManualModeSnapshot(
        camera_metadata_t*      settings,
        ChiMetadata*            pFeatureMetadata
        )
    {
        return ((TRUE == m_isFlashRequired)                                                         ||
                (TRUE == m_isInSensorHDR3ExpCapture)                                                ||
                (0    == ChxUtils::AndroidMetadata::GetZSLMode(reinterpret_cast<VOID*>(settings)))  ||
                ((NULL != pFeatureMetadata) && (0 == ChxUtils::GetZSLMode(pFeatureMetadata))))? TRUE: FALSE;
    }

    /// Function to check whether it is in-sensor HDR 3 exposure snapshot
    CHX_INLINE BOOL IsInSensorHDR3ExpEnabled()
    {
        return ((SeamlessInSensorState::InSensorHDR3ExpStart   == m_seamlessInSensorState) ||
                (SeamlessInSensorState::InSensorHDR3ExpEnabled == m_seamlessInSensorState) ||
                (SeamlessInSensorState::InSensorHDR3ExpStop    == m_seamlessInSensorState))? TRUE: FALSE;
    }

    /// Offline Request thread info
    PerThreadData               m_offlineRequestProcessThread;                 ///< Thread to process the results
    Mutex*                      m_pOfflineRequestMutex;                        ///< App Result mutex
    Condition*                  m_pOfflineRequestAvailable;                    ///< Wait till SensorListener results
                                                                               ///  are available
    volatile BOOL               m_offlineRequestProcessTerminate;              ///< Indication to SensorListener result
                                                                               ///  thread to terminate itself
    PerThreadData               m_deferSnapSessionThreadData;                  ///< Thread data for defer

    volatile BOOL               m_deferSnapshotSessionDone;                    ///< Flag indicate if snapshot session
                                                                               ///  create done
    volatile BOOL               m_deferSnapshotThreadCreateDone;               ///< Flag indicate if snapshot session
    volatile BOOL               m_offlinePreviewSessionCreateDone;             ///< flag indicate if offline preview session
                                                                               ///  create done
    Condition*                  m_pDeferSnapDoneCondition;                     ///< Condition for defer snapshot done
    Mutex*                      m_pDeferSnapDoneMutex;                         ///< Mutex for defer snapshot done
    Condition*                  m_pOfflinePreviewSessionCreateDoneCondition;   ///< Offline preview create done condition
    Mutex*                      m_pOfflinePreviewSessionCreateDoneMutex;       ///< Mutex for offline preview session creation
    UINT                        m_deferedCameraID;                             ///< deferred camera id
    Mutex*                      m_pDeferSyncMutex;                             ///< Mutex for defer sync
    Mutex*                      m_pDeferSnapMetadataMutex;                     ///< Mutex for early snapshot metadata

    UINT32                      m_snapshotReqId;                               ///< Next ID to provide snapshot request
    UINT32                      m_maxSnapshotReqId;                            ///< Last valid ID to move snapshotReqId to
                                                                               ///  Owned by the main thread
    Mutex*                      m_pFeatureDataMutex;                           ///< Mutex for feature data availability.
    Condition*                  m_pFeatureDataAvailable;                       ///< Wait till feature data available
    UINT32                      m_waitForFeatureData;                          ///< Wait for feature data
    UINT32                      m_featureAnchorIndex;                          ///< Feature data, it is anchorframe Index.

    typedef struct syncInfo
    {
        Condition*             m_pSyncCondition;                                ///< Condition for syncronization
        Mutex*                 m_pSyncMutex;                                    ///< Mutex for syncronization
        volatile INT32         m_syncCounter;                                   ///< Counter
        BOOL                   m_waitForSignal;                                 ///< Waiting for signal
    } SyncInfo;

    typedef struct sessionInfo
    {
        UINT32              m_sessionId;                                   ///< Session Index
        UINT32              m_numPipelines;                                ///< Number of pipelines for this session.
        Pipeline*           m_pPipeline[MaxRealTimePipelines];             ///< Pipeline pointers
        Session*            m_pSession;                                    ///< Session pointer.
        SessionPrivateData  m_privateData;                                 ///< Session cb private data
        CHIPRIVDATA         m_chiPrivData[MaxRealTimePipelines][MaxOutstandingRequests]; ///< Result callback priv data
        BufferQueue         m_bufferQueue[MaxOutstandingRequests];         ///< Hold pipeline buffer output
        SyncInfo            m_waitUntilDrain[MaxRealTimePipelines];        ///< Wait till all request processed per pipeline
    } SessionInfo;

    SessionInfo             m_sessionInfo[MAX_MULTI_CAMERA_SESSION];       ///< Session info holder.

    RequestInfo             m_requestInfo[MaxOutstandingRequests];          ///< Realtime request info

    CHISTREAM*              m_pTargetPreviewStream;                         ///< Preview Stream configured from framework
    CHISTREAM*              m_pTargetSnapshotStream;                        ///< Snapshot Stream configured from framework
    CHISTREAM*              m_pTargetPrimaryJpegStream;                     ///< Wide Snapshot Stream configured from framework
    CHISTREAM*              m_pTargetPostviewStream;                        ///< Postview Stream configured from framework
    CHISTREAM*              m_pTargetRAW16Stream;                           ///< RAW Stream configured from framework
    CHISTREAM*              m_pTargetVideoStream;                           ///< Video Stream configured from framework
    CHISTREAM*              m_pTargetRDIStream;                             ///< Target RDI stream
    CHISTREAM*              m_pTargetYUVPreviewStream;                      ///< Target YUV stream
    CHISTREAM*              m_pTargetRawCBStream[MaxRealTimePipelines];     ///< Use for sensor raw 10/16 callback per phy camera

    camera3_stream_buffer_t m_appSnapshotBuffer[MaxOutstandingRequests];    ///< buffer application request buffer handle for snapshot
    camera3_stream_buffer_t m_appPreviewBuffer[MaxOutstandingRequests];     ///< buffer application request buffer handle for preview
    camera3_stream_buffer_t m_appPostviewBuffer[MaxOutstandingRequests];    ///< buffer application request buffer handle for postview
    camera3_stream_buffer_t m_appVideoBuffer[MaxOutstandingRequests];       ///< buffer application request buffer handle for video
    camera3_stream_buffer_t m_appRAW16Buffer[MaxOutstandingRequests];       ///< buffer application request buffer handle for RAW16
    camera3_stream_buffer_t m_appWideJpegBuffer[MaxOutstandingRequests];    ///< buffer application request buffer handle for snapshot
    camera3_stream_buffer_t m_appThumbnailBuffer[MaxOutstandingRequests];   ///< buffer application request buffer handle for thumbnail
    camera3_stream_buffer_t m_appIdealRawBuffer[MaxRealTimePipelines][MaxOutstandingRequests];	  ///< buffer application request buffer handle for snapshot
    camera3_stream_buffer_t m_appYUVBuffer[MaxRealTimePipelines][MaxOutstandingRequests];   ///< buffer application request buffer handle for YUV callback
    camera3_stream_buffer_t m_appPreviewYUVBuffer[MaxOutstandingRequests];                  ///< buffer application request buffer handle for YUV preview callback

    ChiMetadata**           m_ppOfflinePipelineInputMetadataPreview; ///< Input metadata for offline preview

    MultiCamController*     m_pMultiCamController;                   ///< Multicam controller handle

    ChiMetadata*            m_pOfflinePipelineInputMetadataSnapshot[MAX_EMPTY_OFFLINEMETA_COUNT]; ///< Input metadata for offline snapshot pipeline
    ChiMetadata*            m_pOfflinePipelineInputMetadataRAW16;    ///< Input metadata for offline RAW pipeline
    ChiMetadata*            m_pYUVCallbackMetadata[MaxDevicePerLogicalCamera];                    ///< YUV callback output meta for each camera
    UINT32                  m_numYUVOutputmetadata[MaxOutstandingRequests];                       ///< Count for YUV output metadata

    BOOL                    m_isPostViewNeeded;                      ///< Is postview needed
    BOOL                    m_isVideoNeeded;                         ///< Is Video stream needed
    BOOL                    m_isRaw16Needed;                         ///< Is Video stream needed
    BOOL                    m_isSnapshotYUVNeeded;                   ///< Is Snapshot YUV needed from app
    BOOL                    m_isRecordingRdiHaltEnabled;             ///< Is RDI halt enabled
    MultiCameraUsecase      m_usecaseMode;                           ///< usecase mode
    UINT32                  m_offlinePipelineExpectationMask;        ///< So OfflinePipeline knows what buffers RT is sending
    UINT32                  m_realtimeRequestID;
    ControllerResult        m_MCCResult;                             ///< Holds multi camera controller result
    ControllerResult        m_prevMCCResult;                         ///< Holds Previous multi camera controller result
    UINT32                  m_kernelFrameSyncEnable;                 ///< This tells whether frame sync enabled or not
    BOOL                    m_isLLSSnapshot;                         ///< Flag to indicate LLS snapshot or not
    BOOL                    m_isInSensorHDR3ExpSnapshotUsecase;      ///< If in-sensor HDR 3 exp enabled in override settings
    BOOL                    m_isInSensorHDR3ExpAECTrigger;           ///< If AEC triggered in-sensor HDR 3 exposure Snapshot
    BOOL                    m_isInSensorHDR3ExpCapture;              ///< IF in-sensor HDR 3 exposure snapshot triggered
    SeamlessInSensorState   m_seamlessInSensorState;                 ///< State of seamless in-sensor control state
    UINT32                  m_numInSensorHDR3ExpPreviewDelay;        ///< number of delay after applying in-sensor HDR 3 exp
    BOOL                    m_flushLock;                             ///< Flag to indicate flush locked.

    SnapshotStreamConfig    m_snapshotConfig;
    BOOL                    m_bokehPrimaryJpeg;                      ///< capture wide jpeg as well in bokeh mode
    UINT64                  m_jpegRequestCount;                      ///< RequestId for jpeg pipeline
    jpegRequestInfo         m_jpegRequestId[MaxOutstandingRequests]; ///< store jpeg requestId
    UINT32                  m_curActiveMap;                          ///< the realtime pipeline active map of the realtime session
    BOOL                    m_isOfflineRequired;                     ///< Flag to check if snapshot related pipeline is required
    BOOL                    m_isOfflinePreviewSessionDeferred;       ///< Flag to check if offline preview session is deferred
    BOOL                    m_isB2YSnapshot;                         ///< TRUE if b2y snapshot
    UINT32                  m_pendingActiveMask;                     ///< Pending pipelines to active in case of EPCR
    CHIMETAHANDLE           m_hResultMetadataHandle;                 ///< latest Result Metadata handle
    BOOL                    m_isFusionSnapshotModeEnabled;           ///< TRUE if fusion snapshot enabled.
    UINT32                  m_snapshotActiveMask;                    ///< Active mask of realtime needed for snapshot input frames.
    /// Get debug-data from corresponding real-time processing and merge for snapshot process
    VOID MergeDebugData(
        ChiMetadata*    pRDIMetadata,
        UINT32          currentRDIBuffer);

    /// Clear debug-data pointers from metadata. Helper function to avoid the use of real-time memory by offline processing
    VOID ClearOfflineDebugData(
        ChiMetadata*    pRDIMetadata);

    DebugData               m_debugDataOffline[DebugDataMaxOfflineFramesMC];  ///< Place holder for debug-data use in
                                                                              ///  offline process

    UINT32                  m_primaryCameraIndex;                           ///< index of primary camera from app perspective

    CDKResult CreateMultiCameraResource(
        camera3_stream_configuration_t* pStreamConfig);                     /// Create multi camera usecase resources

    /// Create multi camera usecase resources
    CDKResult CreateRDIResources(
        CHIBUFFEROPTIONS*               pFullOptions,
        camera3_stream_configuration_t* pStreamConfig);

    CDKResult CreateBufferManagers();                     /// Create multi camera usecase resources

    VOID DestroyMultiCameraResource();                                      /// Destroy  multi camera usecase resources

    /// Create abd Destroy function for sync lock and condition
    CDKResult InitializeSyncInfo(
        SyncInfo *syncInfo);
    CDKResult DestroySyncInfo(
        SyncInfo *syncInfo);

    /// Function to determine snapshot frame based on feature and available frames.
    UINT32 GetZSLInputFrameNumber(
        UINT32 requestFrameNumber,
        UINT32 requiredBufferCnt,
        UINT32 activeMask);

    typedef struct multicameraresource
    {
        //Multi Camera internal Streams
        CHISTREAM*          m_pRTOutputYUVStream;                         ///< realtime YUV output stream
        CHISTREAM*          m_pRTOutputDS4Stream;                         ///< realtime DS4 output stream
        CHISTREAM*          m_pRTOutputDS16Stream;                        ///< realtime DS16 output stream
        CHISTREAM*          m_pRTOutputRDIStream;                         ///< realtime RDI output stream
        CHISTREAM*          m_pRTOutputFDStream;                          ///< realtime FD output stream
        CHISTREAM*          m_pRTOutputRAW16Stream;                       ///< realtime RAW16 output stream
        CHISTREAM*          m_pYUVInputStream;                            ///< offline input stream for YUV session
        CHISTREAM*          m_pDS4InputStream;                            ///< offline input stream for DS4
        CHISTREAM*          m_pDS16InputStream;                           ///< offline input stream for DS16
        CHISTREAM*          m_pRAW16InputStream;                          ///< realtime RAW16 input stream
        CHISTREAM*          m_pYUVOutputSnapshotStream;                   ///< offline Snapshot pipeline output stream
        CHISTREAM*          m_pJPEGInputStream;                           ///< offline JPEG pipeline Input stream
        CHISTREAM*          m_pJPEGOutputStream;                          ///< offline JPEG pipeline Output stream
        CHISTREAM*          m_pDummyStream;                               ///< Dummy stream
        CHISTREAM*          m_pTargetYUVStream;                           ///< YUV callback output stream
        CHISTREAM*          m_pFusionOutputStream;                        ///< Snapshot fusion output stream
        CHISTREAM*          m_pFusionInputStream;                         ///< Snapshot fusion output stream
        CHISTREAM*          m_pScreenGrabYUVOutSnapshotStream;            ///< Screen Grab Snapshot output stream
        CHISTREAM*          m_pRTOutputIdealRawStream;                    ///< realtime ideal raw cb stream

        //Multi Camera internal Buffer Managers
        CHIBufferManager*   m_pRTYUVBufferManager;                        ///< buffer manager for realtime yuv output
        CHIBufferManager*   m_pRTDS4BufferManager;                        ///< buffer manager for realtime DS4 output
        CHIBufferManager*   m_pRTDS16BufferManager;                       ///< buffer manager for realtime DS16 output
        CHIBufferManager*   m_pRAW16BufferManager;                        ///< Buffer manager for RAW16 realtime output
        CHIBufferManager*   m_pYUVSnapshotBufferManager;                  ///< buffer manager for offline pipeline YUV output
        CHIBufferManager*   m_pFusionSnapshotBufferManager;               ///< buffer manager for snapshot fusion pipeline
        CHIBufferManager*   m_pScreenGrabYUVSnapshotBufferManager;        ///< buffer manager for offline pipeline Screen Grab YUV output

       //Multi Camera tuning mode params
        ChiMetadata*        m_pStickyMetadata;                             ///< Sticky meta data for both the realtime pipelines
        BOOL                m_stickyMetaNeeded;                            ///< This indicates whether sticky meta needed or not
        UINT32              m_effectModeValue;                             ///< effect value
        UINT32              m_sceneModeValue;                              ///< scenemode value
        UINT32              m_tuningFeature1Value;                         ///< tuning Feature1Value value
        UINT32              m_tuningFeature2Value;                         ///< tuning Feature2Value value

        //Multi Camera flags
        CHISTREAMFORMAT     m_RTOutputYUVFormat;                           ///< Real time output YUV format.
        CHISTREAMFORMAT     m_RTOutputDS4Format;                           ///< Real time output YUV DS4 format.
        CHISTREAMFORMAT     m_RTOutputDS16Format;                          ///< Real time output YUV DS16 format.
        UINT32              m_RAWWidth;                                    ///< RAW Width per camera
        UINT32              m_RAWHeight;                                   ///< Raw Hight per Camera.
        UINT32              m_baseFrameNumber;                             ///< Initial frame number from the time
                                                                           ///  we start sending request to the pipeline
    } MultiCameraResource;

    UINT32                  m_maxJpegCameraId;                             ///< Camera ID mapping to max JPEG size
    MultiCameraResource     m_multiCamResource[MaxDevicePerLogicalCamera]; ///< Multi camera resource holder
    std::map<UINT32, CHAR*> phyicalCameraIdToStringMap;                    ///< map of phyical camera Id to Id string

    UINT32                  m_appToInternalFrameNumMap[MaxOutstandingRequests]; ///< App frame number to internal frame number map
    UINT32                  m_maxEmptyRTMetaCount;                              ///< Max empty real time meta for dynamic alloc

    volatile UINT32         m_aisFlushInProgress;                           ///< A flush is in progress
};

#endif // CHXUSECASEMC_H
