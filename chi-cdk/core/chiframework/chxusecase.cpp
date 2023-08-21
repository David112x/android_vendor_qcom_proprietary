////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxusecase.cpp
/// @brief Usecases class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chxdefs.h"
#include "chxusecase.h"
#include "chxutils.h"

#include "chistatspropertydefines.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

static const UINT32 FLUSH_ALL_THREADS_TIMEOUT = 1000;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT32 DefaultNumMetadataBuffers = 16; ///< Default number of metadata buffers

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::~Usecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Usecase::~Usecase()
{
    // work around cmake warning
    (VOID)PerNumTargetUsecases;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::DestroyObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::DestroyObject(
    BOOL isForced)
{
    m_isUsecaseDestroy = TRUE;
    // Flush everything
    if (TRUE == isForced)
    {
        Flush();
    }
    else if (TRUE == m_useParallelFlush)
    {
        Session* pFakeSessions[MaxSessions] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        m_shouldFlushThreadExecute          = FALSE; // Terminate the flush threads
        FlushAllSessionsInParallel(pFakeSessions, MaxSessions);
    }

    if (TRUE == m_useParallelFlush)
    {
        for (UINT session = 0; session < MaxSessions; session++)
        {

            CHX_LOG_ERROR("Usecase:: m_pFlushstartCondition m_isUsecaseDestroy = %d, hThreadHandle = %p",
                          m_isUsecaseDestroy,
                          reinterpret_cast<VOID*>(m_FlushProcessThreadData[session].hThreadHandle));

            ChxUtils::ThreadTerminate(m_FlushProcessThreadData[session].hThreadHandle);

            CHX_LOG_ERROR("Usecase::m_pFlushstartCondition ThreadTerminate from Destroy %d ", session);

            if (NULL != m_pFlushThreadMutex[session])
            {
                m_pFlushThreadMutex[session]->Destroy();
                m_pFlushThreadMutex[session] = NULL;
            }

            if (NULL != m_pFlushstartCondition[session])
            {
                m_pFlushstartCondition[session]->Destroy();
                m_pFlushstartCondition[session] = NULL;
            }
        }
    }

    // Call the derived usecase object to destroy itself
    Destroy(isForced);

    if (NULL != m_pAppResultMutex)
    {
        m_pAppResultMutex->Destroy();
        m_pAppResultMutex = NULL;
    }

    if (NULL != m_pAllResultsMutex)
    {
        m_pAllResultsMutex->Destroy();
        m_pAllResultsMutex = NULL;
    }

    if (NULL != m_pAppResultAvailable)
    {
        m_pAppResultAvailable->Destroy();
        m_pAppResultAvailable = NULL;
    }

    if (NULL != m_pAllResultsAvailable)
    {
        m_pAllResultsAvailable->Destroy();
        m_pAllResultsAvailable = NULL;
    }

    if (NULL != m_pParallelFlushDone)
    {
        m_pParallelFlushDone->Destroy();
        m_pParallelFlushDone = NULL;
    }

    for (UINT i = 0; i < MaxOutstandingRequests; i++)
    {
        if (NULL != m_captureResult[i].output_buffers)
        {
            CHX_FREE(const_cast<camera3_stream_buffer_t*>(m_captureResult[i].output_buffers));
            m_captureResult[i].output_buffers = NULL;
        }
        if (NULL != m_captureResult[i].input_buffer)
        {
            CHX_FREE(const_cast<camera3_stream_buffer_t*>(m_captureResult[i].input_buffer));
            m_captureResult[i].input_buffer = NULL;
        }
        if (NULL != m_pMapLock)
        {
            m_pMapLock->Lock();
        }
        if (NULL != m_pendingPCRs[i].output_buffers)
        {
            CHX_FREE(const_cast<camera3_stream_buffer_t*>(m_pendingPCRs[i].output_buffers));
            m_pendingPCRs[i].output_buffers     = NULL;
            m_pendingPCRs[i].num_output_buffers = 0;
        }
        if (NULL != m_pMapLock)
        {
            m_pMapLock->Unlock();
        }
    }

    // Free Empty MetaData
    if (NULL != m_pEmptyMetaData)
    {
        ChxUtils::AndroidMetadata::FreeMetaData(m_pEmptyMetaData);
        m_pEmptyMetaData = NULL;
    }

    // Free the replaced metadata
    if (NULL != m_pReplacedMetadata)
    {
        free_camera_metadata(m_pReplacedMetadata);
        m_pReplacedMetadata = NULL;
    }

    if (m_pMapLock)
    {
        m_pMapLock->Destroy();
        m_pMapLock = NULL;
    }

    if (NULL != m_pFlushDone)
    {
        m_pFlushDone->Destroy();
        m_pFlushDone = NULL;
    }

    if (NULL != m_pFlushMutex)
    {
        m_pFlushMutex->Destroy();
        m_pFlushMutex = NULL;
    }

    if (NULL != m_pMetadataManager)
    {
        m_pMetadataManager->Destroy();
        m_pMetadataManager = NULL;
    }

    if (NULL != m_pParallelFlushLock)
    {
        m_pParallelFlushLock->Destroy();
        m_pParallelFlushLock = NULL;
    }

    m_flushStatus = FlushStatus::HasFlushed;

    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::Usecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Usecase::Usecase()
    : m_pMetadataManager(NULL)
    , m_metadataClientId(ChiMetadataManager::InvalidClientId)
    , m_genericMetadataClientId(ChiMetadataManager::InvalidClientId)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::Initialize(
    bool initializeMetadataManager)
{
    CDKResult result = CDKResultSuccess;

    m_useParallelFlush           = ShouldUseParallelFlush();
    m_nextAppResultFrame         = InvalidFrameNumber;
    m_nextAppMessageFrame        = InvalidFrameNumber;
    m_lastAppRequestFrame        = InvalidFrameNumber;
    m_lastResultMetadataFrameNum = -1;
    m_isUsecaseDestroy           = FALSE;

    // This is the first valid frame number in the override module and is incremented sequentially for every new request
    m_chiOverrideFrameNum = 0;

    m_resultProcessingThread.pPrivateData = this;

    m_pAppResultMutex      = Mutex::Create();
    m_pMapLock             = Mutex::Create();
    m_pAllResultsMutex     = Mutex::Create();
    m_pFlushMutex          = Mutex::Create();
    m_pParallelFlushLock   = Mutex::Create();
    m_pAppResultAvailable  = Condition::Create();
    m_pAllResultsAvailable = Condition::Create();
    m_pFlushDone           = Condition::Create();
    m_pParallelFlushDone   = Condition::Create();
    m_isUsecaseInBadState  = FALSE;

    m_flushStatus          = FlushStatus::NotFlushing;

    // Allocate Empty metadata
    m_pEmptyMetaData       = ChxUtils::AndroidMetadata::AllocateMetaData(0, 0);
    CHX_ASSERT(NULL != m_pEmptyMetaData);

    // Allocate replaced metadata
    m_pReplacedMetadata    = allocate_camera_metadata(ReplacedMetadataEntryCapacity, ReplacedMetadataDataCapacity);
    CHX_ASSERT(NULL != m_pReplacedMetadata);
    m_replacedMetadataSize = calculate_camera_metadata_size(ReplacedMetadataEntryCapacity, ReplacedMetadataDataCapacity);

    if ((NULL == m_pAppResultMutex) || (NULL == m_pMapLock) || (NULL == m_pAllResultsMutex) || (NULL == m_pFlushMutex) ||
        (NULL == m_pParallelFlushLock) || (NULL == m_pAppResultAvailable) || (NULL == m_pAllResultsAvailable) ||
        (NULL == m_pFlushDone) || (NULL == m_pEmptyMetaData) || (NULL == m_pReplacedMetadata))
    {
        CHX_LOG_ERROR("Mutex or Condition or Camera MetaData Create failed");
        result = CDKResultENoMemory;
    }

    if (CDKResultSuccess == result)
    {
        m_pMapLock->Lock();
        for (UINT i = 0; i < MaxOutstandingRequests; i++)
        {
            m_pendingPCRs[i].num_output_buffers = 0;
            m_pendingPCRs[i].output_buffers     =
                static_cast<camera3_stream_buffer_t*>(CHX_CALLOC(sizeof(camera3_stream_buffer_t) * MaxExternalBuffers));
        }

        ChxUtils::Memset(m_chiMetadataArray, 0x0, sizeof(m_chiMetadataArray));
        m_pMapLock->Unlock();
    }

    for (UINT i = 0; i < MaxOutstandingRequests; i++)
    {
        m_captureResult[i].output_buffers = static_cast<camera3_stream_buffer_t*>(
            CHX_CALLOC(sizeof(camera3_stream_buffer_t) * MaxExternalBuffers));

        m_captureResult[i].input_buffer   = static_cast<camera3_stream_buffer_t*>(
            CHX_CALLOC(sizeof(camera3_stream_buffer_t)));
    }

    if (TRUE == initializeMetadataManager)
    {
        if (NULL == m_pMetadataManager)
        {
            CreateMetadataManager();
        }
        else
        {
            CHX_LOG_ERROR("Metadata Manager already initialized");
        }
    }
    InitializeRequestFlags();

    m_triggeredFlushedCnt = 0;
    if ((TRUE == m_useParallelFlush) && (CDKResultSuccess == result))
    {
        m_shouldFlushThreadExecute = TRUE;
        m_pFlushMutex->Lock();
        for (UINT session = 0; session < MaxSessions; session++)
        {
            m_FlushProcessThreadData[session].pPrivateData = this;
            m_pFlushstartCondition[session]                = Condition::Create();
            m_pFlushThreadMutex[session]                   = Mutex::Create();

            result = ChxUtils::ThreadCreate(Usecase::FlushThread,
                                            &m_FlushProcessThreadData[session],
                                            &m_FlushProcessThreadData[session].hThreadHandle);
        }
        m_pFlushMutex->Unlock();
    }

    /// @todo Enable Multithreading: We need to enable threading so keeping it and will enable as a follow on
    // result = ChxUtils::ThreadCreate(Usecase::ResultThreadMain,
    //                                 &m_resultProcessingThread,
    //                                 &m_resultProcessingThread.hThreadHandle);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Usecase::ResultThreadMain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* Usecase::ResultThreadMain(
    VOID* pThreadData)
{
    PerThreadData* pPerThreadData = reinterpret_cast<PerThreadData*>(pThreadData);

    Usecase* pUsecase = reinterpret_cast<Usecase*>(pPerThreadData->pPrivateData);

    pUsecase->ProcessResults();

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::ProcessCaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::ProcessCaptureRequest(
    camera3_capture_request_t* pRequest)
{
    CDKResult                result               = CDKResultSuccess;
    UINT32                   chiOverrideFrameNum  = GetChiOverrideFrameNum();
    UINT32                   resultFrameIndexChi  = chiOverrideFrameNum % MaxOutstandingRequests;
    UINT32                   frameworkFrameNum    = pRequest->frame_number;
    camera3_capture_request* pPendingPCRSlot      = &m_pendingPCRs[resultFrameIndexChi];

    /// Chi override frame number is what the rest of the override module knows. The original application frame number is only
    /// known to this class and no one else. Hence any result communication to application needs to go thru this class strictly

    m_pMapLock->Lock();

    if (FALSE == m_requestFlags[resultFrameIndexChi].isInErrorState)
    {
        // Old request not returned yet. Flush its result result first
        if (0 != m_numberOfPendingOutputBuffers[resultFrameIndexChi])
        {
            CHX_LOG_ERROR("Chi Frame: %d hasn't returned a result and will be canceled in favor of Chi Frame: %d. Index: %d",
                          pPendingPCRSlot->frame_number,
                          pRequest->frame_number,
                          resultFrameIndexChi);
            HandleProcessRequestError(pPendingPCRSlot);
        }
        else if (FALSE == m_requestFlags[resultFrameIndexChi].isOutputMetaDataSent)
        {
            CHX_LOG_ERROR("Pending metadata in PCR. ChiOverrideFrame: %d, Last Request Frame: %" PRIu64,
                          chiOverrideFrameNum - MaxOutstandingRequests,
                          m_lastAppRequestFrame);

            // We reached max number of outstanding requests but metadata is not sent. Most probably errored out.
            // Send Metadata error for this frame.

            // release metadata also.
            if (NULL != m_captureResult[resultFrameIndexChi].result)
            {
                m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(
                    m_captureResult[resultFrameIndexChi].result);
            }

            HandleResultError(pPendingPCRSlot);
        }
    }

    AssignChiOverrideFrameNum(pRequest->frame_number);
    pRequest->frame_number = chiOverrideFrameNum;

    CHX_LOG("Saving buffer for CHI Frame: %d, requestFrame: %d, NumBuff: %d resultFrameIndexChi: %d",
            chiOverrideFrameNum,
            frameworkFrameNum,
            pRequest->num_output_buffers,
            resultFrameIndexChi);

    // Set pending output buffers after clearing the previous one
    m_numAppPendingOutputBuffers[resultFrameIndexChi]   = pRequest->num_output_buffers;
    m_numberOfPendingOutputBuffers[resultFrameIndexChi] = pRequest->num_output_buffers;
    m_numBufferErrorMessages[resultFrameIndexChi]       = 0;

    m_requestFlags[resultFrameIndexChi].value               = 0; // Reset all flags
    m_requestFlags[resultFrameIndexChi].isMessagePending    = TRUE;
    pPendingPCRSlot->frame_number       = chiOverrideFrameNum;
    pPendingPCRSlot->num_output_buffers = pRequest->num_output_buffers;
    pPendingPCRSlot->input_buffer       = pRequest->input_buffer;

    if (InvalidFrameNumber == m_nextAppMessageFrame)
    {
        m_nextAppMessageFrame           = chiOverrideFrameNum;
        m_lastAppMessageFrameReceived   = chiOverrideFrameNum;
    }

    if (InvalidFrameNumber == m_nextAppResultFrame)
    {
        CHX_SET_AND_LOG_UINT64(m_nextAppResultFrame, chiOverrideFrameNum);
        m_nextAppMessageFrame        = chiOverrideFrameNum;
        m_lastAppRequestFrame        = chiOverrideFrameNum;
        m_lastResultMetadataFrameNum = m_nextAppMessageFrame - 1;
    }

    BOOL isSnapshotStream = FALSE;

    for (UINT i = 0; i < pRequest->num_output_buffers; i++)
    {
        if (&pRequest->output_buffers[i] == NULL)
        {
            continue;
        }
        CHX_LOG("SAVING frame: %d resultFrameIndexChi: %d", frameworkFrameNum, resultFrameIndexChi);
        ChxUtils::Memcpy(const_cast<camera3_stream_buffer_t*>(&pPendingPCRSlot->output_buffers[i]),
                         &pRequest->output_buffers[i],
                         sizeof(camera3_stream_buffer_t));
        if ((UsecaseSelector::IsJPEGSnapshotStream(pRequest->output_buffers[i].stream)) ||
            (UsecaseSelector::IsHEIFStream(pRequest->output_buffers[i].stream)))
        {
            isSnapshotStream = TRUE;
        }
    }

    UINT32 isZSLMode = 0;
    isZSLMode        = ChxUtils::AndroidMetadata::GetZSLMode(const_cast<camera_metadata_t*>(pRequest->settings));

    if ((TRUE == isSnapshotStream) && (1 == isZSLMode))
    {
        CHX_LOG_INFO("Frame: %u(idx: %u) is a Snapshot, Setting isSnapshotStream TRUE",
            chiOverrideFrameNum, resultFrameIndexChi);
        m_requestFlags[resultFrameIndexChi].isZSLMessageAvailable = TRUE;
    }

    m_pMapLock->Unlock();
    ResetMetadataStatus(pRequest);
    ChxUtils::AtomicStore64(&m_lastAppRequestFrame, chiOverrideFrameNum);

    camera3_capture_result_t* pUsecaseResult = GetCaptureResult(resultFrameIndexChi);
    pUsecaseResult->result                   = NULL;
    pUsecaseResult->frame_number             = pRequest->frame_number;
    pUsecaseResult->num_output_buffers       = 0;

    if (FlushStatus::NotFlushing != GetFlushStatus())
    {
        CHX_LOG_INFO("Usecase is flushing. No requests will be generated for Framework Frame: %d", frameworkFrameNum);
        HandleProcessRequestError(pPendingPCRSlot);
    }
    else
    {
        if (NULL != pRequest->settings)
        {
            // Replace the metadata by appending vendor tag for cropRegions
            result = ReplaceRequestMetadata(pRequest->settings);
            if (CDKResultSuccess == result)
            {
                pRequest->settings = m_pReplacedMetadata;
            }

            // The translation must be done in base class as it is also required for default usecase,
            // which is used for most CTS/ITS cases.
            if ((NULL  != m_pLogicalCameraInfo) &&
                (TRUE  == UsecaseSelector::IsQuadCFASensor(m_pLogicalCameraInfo, NULL)) &&
                (FALSE == ExtensionModule::GetInstance()->ExposeFullsizeForQuadCFA()))
            {
                // map ROIs (aec/af/crop region) from binning active array size based to full active arrsy size based
                result = OverrideInputMetaForQCFA(const_cast<camera_metadata_t*>(pRequest->settings));
            }

            if (CamxResultSuccess != result)
            {
                CHX_LOG_ERROR("OverrideInputMetaForQCFA Errored Out! Usecase:%d cameraId:%d in state: %s",
                    GetUsecaseId(), GetCameraId(), CamxResultStrings[result]);
            }
        }

        result = ExecuteCaptureRequest(pRequest);

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("ECR Errored Out! Usecase:%d cameraId:%d in state: %s",
                GetUsecaseId(), GetCameraId(), CamxResultStrings[result]);

            if (CDKResultETimeout == result)
            {
                CHX_LOG_ERROR("Usecase:%d cameraId:%d timed out - returning success to trigger recovery",
                              GetUsecaseId(), GetCameraId());
                result = CDKResultSuccess;
            }
        }

        // Restore the original metadata
        RestoreRequestMetadata(pRequest);
    }

    // reset the frame number
    pRequest->frame_number = frameworkFrameNum;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::ReplaceRequestMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::ReplaceRequestMetadata(
    const VOID* pMetadata)
{
    CDKResult result = CDKResultSuccess;

    // Save the the original metadata
    m_pOriginalMetadata = pMetadata;

    m_pReplacedMetadata = place_camera_metadata(m_pReplacedMetadata,
                            m_replacedMetadataSize,
                            ReplacedMetadataEntryCapacity,
                            ReplacedMetadataDataCapacity);

    // Add the existing metadata first before appending the new tags
    result = append_camera_metadata(m_pReplacedMetadata, static_cast<const camera_metadata_t*>(pMetadata));

    if (CDKResultSuccess == result)
    {
        // Read the android crop region
        camera_metadata_entry_t entry = { 0 };
        if (0 == find_camera_metadata_entry(m_pReplacedMetadata, ANDROID_SCALER_CROP_REGION, &entry))
        {
            CaptureRequestCropRegions cropRegions;
            cropRegions.userCropRegion.left   = entry.data.i32[0];
            cropRegions.userCropRegion.top    = entry.data.i32[1];
            cropRegions.userCropRegion.width  = entry.data.i32[2];
            cropRegions.userCropRegion.height = entry.data.i32[3];

            CHIRECT* pUserZoom = reinterpret_cast<CHIRECT*>(&cropRegions.userCropRegion);
            cropRegions.pipelineCropRegion = *pUserZoom;
            cropRegions.ifeLimitCropRegion = *pUserZoom;

            // Set the cropRegions vendor tag data
            ChxUtils::AndroidMetadata::SetVendorTagValue(m_pReplacedMetadata, VendorTag::CropRegions,
                            sizeof(CaptureRequestCropRegions), &cropRegions);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::RestoreRequestMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::RestoreRequestMetadata(
    camera3_capture_request_t* pRequest)
{
    if (m_pOriginalMetadata != NULL)
    {
        pRequest->settings = static_cast<const camera_metadata_t*>(m_pOriginalMetadata);
        m_pOriginalMetadata = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::ProcessCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::ProcessCaptureResult(
    const CHICAPTURERESULT* pResult)
{
    CDKResult result = CDKResultSuccess;

    ///@ todo Fix preview only case
    (VOID)pResult;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::ProcessMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::ProcessMessage(
    const CHIMESSAGEDESCRIPTOR* pMessageDescriptor)
{
    CDKResult result = CDKResultSuccess;

    ///@ todo Fix preview only case
    (VOID)pMessageDescriptor;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::ProcessErrorMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::ProcessErrorMessage(
    const CHIMESSAGEDESCRIPTOR* pMessageDescriptor)
{
    CHX_ASSERT(ChiMessageTypeError == pMessageDescriptor->messageType);

    UINT64 resultFrame = pMessageDescriptor->message.errorMessage.frameworkFrameNum;
    UINT64 frameIndex = resultFrame % MaxOutstandingRequests;

    switch (pMessageDescriptor->message.errorMessage.errorMessageCode)
    {
        case MessageCodeTriggerRecovery:
        {
            ExtensionModule::GetInstance()->SignalRecoveryCondition(m_cameraId);
            break;
        }
        default:
            CHX_LOG_ERROR("Unhandled error");
            break;
        }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Usecase::ReturnPendingPCR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
camera3_capture_request_t* Usecase::ReturnPendingPCR(
    UINT frameNumber)
{
    return &m_pendingPCRs[frameNumber % MaxOutstandingRequests];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Usecase::InvalidateResultMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::InvalidateResultMetadata(
    camera3_capture_request_t* pRequest)
{
    const UINT& frameIndex                                 = pRequest->frame_number % MaxOutstandingRequests;
    m_requestFlags[frameIndex].isOutputMetaDataSent        = TRUE;
    m_requestFlags[frameIndex].isDriverPartialMetaDataSent = TRUE;
    m_requestFlags[frameIndex].isOutputPartialMetaDataSent = TRUE;

    if (NULL != m_driverPartialCaptureResult[frameIndex].result)
    {
        m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(
            m_driverPartialCaptureResult[frameIndex].result);
    }

    if (NULL != m_chiPartialCaptureResult[frameIndex].result)
    {
        m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(
            m_chiPartialCaptureResult[frameIndex].result);
    }

    if (NULL != m_captureResult[frameIndex].result)
    {
        m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(
            m_captureResult[frameIndex].result);
    }

    ChxUtils::Memset(&m_driverPartialCaptureResult[frameIndex], 0, sizeof(camera3_capture_result_t));
    ChxUtils::Memset(&m_chiPartialCaptureResult[frameIndex], 0, sizeof(camera3_capture_result_t));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Usecase::OverrideInputMetaForQCFA
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::OverrideInputMetaForQCFA(
    camera_metadata_t* pInputAndroidMeta)
{
    CDKResult result          = CDKResultSuccess;
    CHIRECT   activeArraySize = { 0 };

    if (NULL == m_pLogicalCameraInfo)
    {
        result = CDKResultEFailed;
        return result;
    }

    activeArraySize = m_pLogicalCameraInfo->m_cameraCaps.sensorCaps.activeArray;

    CHX_LOG_VERBOSE("input meta:%p, real active array size:%dx%d",
        pInputAndroidMeta, activeArraySize.width, activeArraySize.height);

    camera_metadata_entry_t entry = { 0 };
    if (0 == find_camera_metadata_entry(pInputAndroidMeta, ANDROID_SCALER_CROP_REGION, &entry))
    {
        CHIRECT cropRegions = { 0 };
        cropRegions.left    = entry.data.i32[0] << 1;
        cropRegions.top     = entry.data.i32[1] << 1;
        cropRegions.width   = entry.data.i32[2] << 1;
        cropRegions.height  = entry.data.i32[3] << 1;

        CHX_LOG_VERBOSE("crop region, origin (%d, %d, %d, %d) -> new (%d, %d, %d, %d)",
            entry.data.i32[0], entry.data.i32[1], entry.data.i32[2], entry.data.i32[3],
            cropRegions.left, cropRegions.top, cropRegions.width, cropRegions.height);

        entry.data.i32[0] = cropRegions.left;
        entry.data.i32[1] = cropRegions.top;
        entry.data.i32[2] = cropRegions.width;
        entry.data.i32[3] = cropRegions.height;
    }

    if (0 == find_camera_metadata_entry(pInputAndroidMeta, ANDROID_CONTROL_AE_REGIONS, &entry))
    {
        CHIRECT AERegions = { 0 };
        UINT32  tmpW      = entry.data.i32[2] - entry.data.i32[0];
        UINT32  tmpH      = entry.data.i32[3] - entry.data.i32[1];
        AERegions.left    = entry.data.i32[0] << 1;
        AERegions.top     = entry.data.i32[1] << 1;
        tmpW              <<= 1;
        tmpH              <<= 1;
        AERegions.width   = AERegions.left + tmpW;
        AERegions.height  = AERegions.top  + tmpH;

        CHX_LOG_VERBOSE("AE region, origin (%d, %d, %d, %d) -> new (%d, %d, %d, %d)",
            entry.data.i32[0], entry.data.i32[1], entry.data.i32[2], entry.data.i32[3],
            AERegions.left, AERegions.top, AERegions.width, AERegions.height);

        entry.data.i32[0] = AERegions.left;
        entry.data.i32[1] = AERegions.top;
        entry.data.i32[2] = AERegions.width;
        entry.data.i32[3] = AERegions.height;
    }

    if (0 == find_camera_metadata_entry(pInputAndroidMeta, ANDROID_CONTROL_AF_REGIONS, &entry))
    {
        CHIRECT AFRegions = { 0 };
        UINT32  tmpW      = entry.data.i32[2] - entry.data.i32[0];
        UINT32  tmpH      = entry.data.i32[3] - entry.data.i32[1];
        AFRegions.left    = entry.data.i32[0] << 1;
        AFRegions.top     = entry.data.i32[1] << 1;
        tmpW              <<= 1;
        tmpH              <<= 1;
        AFRegions.width   = AFRegions.left + tmpW;
        AFRegions.height  = AFRegions.top  + tmpH;

        CHX_LOG_VERBOSE("AF region, origin (%d, %d, %d, %d) -> new (%d, %d, %d, %d)",
            entry.data.i32[0], entry.data.i32[1], entry.data.i32[2], entry.data.i32[3],
            AFRegions.left, AFRegions.top, AFRegions.width, AFRegions.height);

        entry.data.i32[0] = AFRegions.left;
        entry.data.i32[1] = AFRegions.top;
        entry.data.i32[2] = AFRegions.width;
        entry.data.i32[3] = AFRegions.height;
    }

    if (TRUE == ChxUtils::AndroidMetadata::IsVendorTagPresent(pInputAndroidMeta, VendorTag::T2TConfigRegROI))
    {
        INT32 *pData            = NULL;
        INT32 trackerRegROI[4]  = { 0,0,0,0 } ;
        ChxUtils::AndroidMetadata::GetVendorTagValue(pInputAndroidMeta, VendorTag::T2TConfigRegROI, (void**)(&pData));

        if( NULL != pData)
        {
            trackerRegROI[0] = *pData       << 1;
            trackerRegROI[1] = *(pData+1)   << 1;
            trackerRegROI[2] = *(pData+2)   << 1;
            trackerRegROI[3] = *(pData+3)   << 1;

            CHX_LOG_VERBOSE("Tracker registration ROI (%d %d %d %d) -> new (%d %d %d %d)",
                *pData, *(pData + 1), *(pData + 2), *(pData + 3),
                trackerRegROI[0], trackerRegROI[1], trackerRegROI[2], trackerRegROI[3]);

            ChxUtils::AndroidMetadata::SetVendorTagValue(pInputAndroidMeta, VendorTag::T2TConfigRegROI, 4, &trackerRegROI[0]);
        }
        else
        {
            CHX_LOG_WARN("Failed to get vendor tag value for T2TConfigRegROI");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Usecase::OverrideResultMetaForQCFA
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::OverrideResultMetaForQCFA(
    camera_metadata_t* pResultAndroidMeta)
{
    CDKResult result          = CDKResultSuccess;
    CHIRECT   activeArraySize = { 0 };

    if (NULL == m_pLogicalCameraInfo)
    {
        result = CDKResultEFailed;
        return result;
    }

    camera_metadata_entry_t entry = { 0 };
    if (0 == find_camera_metadata_entry(pResultAndroidMeta, ANDROID_SCALER_CROP_REGION, &entry))
    {
        CHIRECT cropRegions = { 0 };
        cropRegions.left    = entry.data.i32[0] >> 1;
        cropRegions.top     = entry.data.i32[1] >> 1;
        cropRegions.width   = entry.data.i32[2] >> 1;
        cropRegions.height  = entry.data.i32[3] >> 1;

        CHX_LOG_VERBOSE("crop region, origin (%d, %d, %d, %d) -> new (%d, %d, %d, %d)",
            entry.data.i32[0], entry.data.i32[1], entry.data.i32[2], entry.data.i32[3],
            cropRegions.left, cropRegions.top, cropRegions.width, cropRegions.height);

        entry.data.i32[0] = cropRegions.left;
        entry.data.i32[1] = cropRegions.top;
        entry.data.i32[2] = cropRegions.width;
        entry.data.i32[3] = cropRegions.height;
    }

    if (0 == find_camera_metadata_entry(pResultAndroidMeta, ANDROID_CONTROL_AE_REGIONS, &entry))
    {
        CHIRECT AERegions = { 0 };
        UINT32  tmpW      = entry.data.i32[2] - entry.data.i32[0];
        UINT32  tmpH      = entry.data.i32[3] - entry.data.i32[1];
        AERegions.left    = entry.data.i32[0] >> 1;
        AERegions.top     = entry.data.i32[1] >> 1;
        tmpW              >>= 1;
        tmpH              >>= 1;
        AERegions.width   = AERegions.left + tmpW;
        AERegions.height  = AERegions.top  + tmpH;

        CHX_LOG_VERBOSE("AE region, origin (%d, %d, %d, %d) -> new (%d, %d, %d, %d)",
            entry.data.i32[0], entry.data.i32[1], entry.data.i32[2], entry.data.i32[3],
            AERegions.left, AERegions.top, AERegions.width, AERegions.height);

        entry.data.i32[0] = AERegions.left;
        entry.data.i32[1] = AERegions.top;
        entry.data.i32[2] = AERegions.width;
        entry.data.i32[3] = AERegions.height;
    }

    if (0 == find_camera_metadata_entry(pResultAndroidMeta, ANDROID_CONTROL_AF_REGIONS, &entry))
    {
        CHIRECT AFRegions = { 0 };
        UINT32  tmpW      = entry.data.i32[2] - entry.data.i32[0];
        UINT32  tmpH      = entry.data.i32[3] - entry.data.i32[1];
        AFRegions.left    = entry.data.i32[0] >> 1;
        AFRegions.top     = entry.data.i32[1] >> 1;
        tmpW              >>= 1;
        tmpH              >>= 1;
        AFRegions.width   = AFRegions.left + tmpW;
        AFRegions.height  = AFRegions.top  + tmpH;

        CHX_LOG_VERBOSE("AF region, origin (%d, %d, %d, %d) -> new (%d, %d, %d, %d)",
            entry.data.i32[0], entry.data.i32[1], entry.data.i32[2], entry.data.i32[3],
            AFRegions.left, AFRegions.top, AFRegions.width, AFRegions.height);

        entry.data.i32[0] = AFRegions.left;
        entry.data.i32[1] = AFRegions.top;
        entry.data.i32[2] = AFRegions.width;
        entry.data.i32[3] = AFRegions.height;
    }

    if (0 == find_camera_metadata_entry(pResultAndroidMeta, ANDROID_STATISTICS_FACE_RECTANGLES, &entry))
    {
        UINT32 numElemsRect = sizeof(CHIRECT) / sizeof(UINT32);
        UINT32 numFaces     = entry.count / numElemsRect;

        for (UINT32 i = 0; i < numFaces; ++i)
        {
            INT32 xMin = entry.data.i32[(i << 2) + 0] >> 1;
            INT32 yMin = entry.data.i32[(i << 2) + 1] >> 1;
            INT32 xMax = entry.data.i32[(i << 2) + 2] >> 1;
            INT32 yMax = entry.data.i32[(i << 2) + 3] >> 1;

            CHX_LOG_VERBOSE("FD rect[%d], origin (%d, %d, %d, %d) -> new (%d, %d, %d, %d)",
                i,
                entry.data.i32[(i << 2) + 0],
                entry.data.i32[(i << 2) + 1],
                entry.data.i32[(i << 2) + 2],
                entry.data.i32[(i << 2) + 3],
                xMin, yMin, xMax, yMax);

            entry.data.i32[(i << 2) + 0] = xMin;
            entry.data.i32[(i << 2) + 1] = yMin;
            entry.data.i32[(i << 2) + 2] = xMax;
            entry.data.i32[(i << 2) + 3] = yMax;
        }
    }

    if (TRUE == ChxUtils::AndroidMetadata::IsVendorTagPresent(pResultAndroidMeta, VendorTag::T2TResultROI))
    {
        INT32 *pData                = NULL;
        INT32 trackerResultROI[4]   = { 0, 0, 0, 0 };
        ChxUtils::AndroidMetadata::GetVendorTagValue(pResultAndroidMeta, VendorTag::T2TResultROI, (void**)(&pData));

        if (NULL != pData)
        {
            trackerResultROI[0] = *pData >> 1;
            trackerResultROI[1] = *(pData + 1) >> 1;
            trackerResultROI[2] = *(pData + 2) >> 1;
            trackerResultROI[3] = *(pData + 3) >> 1;

            CHX_LOG_VERBOSE("Tracker result ROI (%d %d %d %d) -> new (%d %d %d %d)",
                *pData, *(pData + 1), *(pData + 2), *(pData + 3),
                trackerResultROI[0], trackerResultROI[1], trackerResultROI[2], trackerResultROI[3]);

            ChxUtils::AndroidMetadata::SetVendorTagValue(pResultAndroidMeta, VendorTag::T2TResultROI, 4, &trackerResultROI[0]);
        }
        else
        {
            CHX_LOG_WARN("Failed to get vendor tag value for T2TResultROI");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Usecase::HandleResultError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::HandleResultError(
    camera3_capture_request_t* pRequest)
{
    ChiMessageDescriptor messageDescriptor;
    CHIERRORMESSAGE*     pErrorMessage = &messageDescriptor.message.errorMessage;

    pErrorMessage->frameworkFrameNum   = pRequest->frame_number;
    pErrorMessage->errorMessageCode    = MessageCodeResult;
    pErrorMessage->pErrorStream        = NULL;

    InvalidateResultMetadata(pRequest);

    ReturnFrameworkErrorMessage((camera3_notify_msg_t*)&messageDescriptor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Usecase::HandleProcessRequestError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::HandleProcessRequestError(
    camera3_capture_request_t* pRequest)
{
    UINT32 frameIndex        = pRequest->frame_number % MaxOutstandingRequests;
    UINT32 numPendingBuffers = m_numberOfPendingOutputBuffers[frameIndex];

    m_pMapLock->Lock();

    if (FALSE == m_requestFlags[frameIndex].isInErrorState)
    {
        ChiMessageDescriptor     messageDescriptor;
        camera3_capture_result_t result;
        camera3_stream_buffer_t* pInputBuffer        = pRequest->input_buffer;
        CHIERRORMESSAGE*         pErrorMessage       = &messageDescriptor.message.errorMessage;
        const BOOL               hasSentMetadata     = CheckIfAnyPartialMetaDataHasBeenSent(frameIndex);
        const BOOL               canSendRequestError = ((FALSE == hasSentMetadata) &&
                                                        (numPendingBuffers == pRequest->num_output_buffers));

        pErrorMessage->frameworkFrameNum = pRequest->frame_number;

        if (TRUE == canSendRequestError)
        {
            // Send the ERROR_REQUEST message to the framework first
            pErrorMessage->errorMessageCode = MessageCodeRequest;
            pErrorMessage->pErrorStream     = NULL;
            ReturnFrameworkErrorMessage((camera3_notify_msg_t*)&messageDescriptor);
        }
        else
        {
            CHX_LOG_INFO("Cannot send ERROR_REQUEST for chi frame: %d hasSentMetaData: %d Buffers Sent: (%d/%d)",
                         pRequest->frame_number,
                         hasSentMetadata,
                         pRequest->num_output_buffers - numPendingBuffers,
                         pRequest->num_output_buffers);
            if (FALSE == m_requestFlags[frameIndex].isOutputMetaDataSent)
            {
                m_requestFlags[frameIndex].isMetadataErrorSent = TRUE;
                HandleResultError(pRequest);
            }
        }

        // Set the Input + Output buffers to BUFFER_STATUS_ERROR
        if (NULL != pInputBuffer)
        {
            pInputBuffer->release_fence = -1; // Return buffer ownership to the framework immediately
            pInputBuffer->status        = CAMERA3_BUFFER_STATUS_ERROR;
        }

        // Return the invalidated buffers
        const BOOL& canSendBufferError = (FALSE == canSendRequestError);
        InvalidateResultBuffers(pRequest, canSendBufferError, TRUE);
        InvalidateResultMetadata(pRequest);

        // At this point, the Google Framework will no longer treat pRequest as inflight.
        // Setting isInErrorState to TRUE will ensure that we will never notify the framework about this request again.
        // Invalidate request flags
        m_requestFlags[frameIndex].isInErrorState     = TRUE;
        m_requestFlags[frameIndex].isMessagePending   = FALSE;
        m_requestFlags[frameIndex].isMessageAvailable = FALSE;
    }
    else
    {
        CHX_LOG_INFO("A request error has already been sent for Chi Frame: %d",
                     m_pendingPCRs[frameIndex].frame_number);
        CHX_ASSERT(0 == m_numberOfPendingOutputBuffers[frameIndex]);
    }

    m_pMapLock->Unlock();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Usecase::InvalidateResultBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::InvalidateResultBuffers(
    camera3_capture_request_t* pRequest,
    BOOL                       shouldSendBufferError,
    BOOL                       shouldReturnInputBuffer)
{
    camera3_capture_result_t  result;
    ChiMessageDescriptor      messageDescriptor;
    camera3_stream_buffer_t   pendingStreamBuffers[16];
    UINT                      numOutputBuffersPending = 0;

    if (TRUE == shouldSendBufferError)
    {   // Only copy fields when needed
        messageDescriptor.messageType                            = ChiMessageTypeError;
        messageDescriptor.message.errorMessage.frameworkFrameNum = pRequest->frame_number;
        messageDescriptor.message.errorMessage.errorMessageCode  = MessageCodeBuffer;
    }

    for (UINT i = 0; i < pRequest->num_output_buffers; i++)
    {
        camera3_stream_buffer_t* pStreamBuffer = const_cast<camera3_stream_buffer_t*>(&pRequest->output_buffers[i]);
        if (pStreamBuffer->buffer != NULL)
        {   // Send buffers which are non NULL as NULL means, already returned to frameworks in regular call
            pStreamBuffer->release_fence = -1;
            pStreamBuffer->status        = CAMERA3_BUFFER_STATUS_ERROR;
            // Copy to pendingStreamBuffers
            pendingStreamBuffers[numOutputBuffersPending] = pRequest->output_buffers[i];
            numOutputBuffersPending++;
            CHX_LOG("Flush buffer %p num_output_buffers %d numOutputBuffersPending %d",
                    pRequest->output_buffers[i].buffer,
                    pRequest->num_output_buffers, numOutputBuffersPending);
            if (TRUE == shouldSendBufferError)
            {
                messageDescriptor.message.errorMessage.pErrorStream = reinterpret_cast<CHISTREAM*>(pStreamBuffer->stream);
                ReturnFrameworkErrorMessage((camera3_notify_msg_t*)&messageDescriptor);
            }
        }

        // invalidate the FDs
        for (UINT i = 0; i < pRequest->num_output_buffers; i++)
        {
            if (InvalidNativeFence != pRequest->output_buffers[i].acquire_fence)
            {
                CDKResult result = ChxUtils::NativeFenceWait(pRequest->output_buffers[i].acquire_fence,
                                                             FLUSH_ALL_THREADS_TIMEOUT);
                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("NativeFenceWait failed for buffer %d, request ID %d ", i, pRequest->frame_number);
                    continue;
                }
                ChxUtils::Close(pRequest->output_buffers[i].acquire_fence);
                camera3_stream_buffer_t* pOutputBuffer = const_cast<camera3_stream_buffer_t*>(&pRequest->output_buffers[i]);
                pOutputBuffer->acquire_fence = InvalidNativeFence;
            }
        }
    }

    UINT64 frameIndex                           = pRequest->frame_number % MaxOutstandingRequests;
    m_numAppPendingOutputBuffers[frameIndex]    = 0;

    result.frame_number         = pRequest->frame_number;
    result.result               = NULL;
    result.num_output_buffers   = numOutputBuffersPending;
    result.output_buffers       = reinterpret_cast<const camera3_stream_buffer_t *>(pendingStreamBuffers);
    result.input_buffer         = (TRUE == shouldReturnInputBuffer) ? pRequest->input_buffer : NULL;
    result.partial_result       = 0;
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) //Android-P or better
    result.num_physcam_metadata = 0;
#endif //Android-P or better
    ReturnFrameworkResult(reinterpret_cast<const camera3_capture_result_t*>(&result), m_cameraId);

    for (UINT32 lastErrorRequest = pRequest->frame_number; lastErrorRequest < m_lastAppRequestFrame; lastErrorRequest++)
    {
        UINT frameIndex = lastErrorRequest % MaxOutstandingRequests;
        if ((lastErrorRequest > pRequest->frame_number) && (FALSE == m_requestFlags[frameIndex].isInErrorState))
        {   // Request Errors may come in any order.
            // Continue to increment the nextAppResultFrame until we reach a request with no errors.
            break;
        }
        if (m_nextAppResultFrame == lastErrorRequest)
        {
            CHX_INC_AND_LOG_UINT64(m_nextAppResultFrame);
        }
        UINT64 secondLastErrorRequest = (lastErrorRequest >= 1) ? (lastErrorRequest - 1) : 0;
        if (m_lastResultMetadataFrameNum == static_cast<INT64>(secondLastErrorRequest))
        {
            CHX_SET_AND_LOG_UINT64(m_lastResultMetadataFrameNum, lastErrorRequest);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Usecase::ReturnFrameworkErrorMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::ReturnFrameworkErrorMessage(
    const camera3_notify_msg_t* pMessage)
{
    camera3_notify_msg_t* pOverrideMessage       = const_cast<camera3_notify_msg_t*>(pMessage);
    const UINT32          chiFrameNumber         = pMessage->message.error.frame_number;
    const UINT32          frameIndex             = chiFrameNumber % MaxOutstandingRequests;
    pOverrideMessage->type                       = ChiMessageTypeError;
    pOverrideMessage->message.error.frame_number = GetAppFrameNum(chiFrameNumber);

    // Don't send the error message to the framework twice
    if (FALSE == m_requestFlags[frameIndex].isInErrorState)
    {
        CHX_LOG("Sending message: %s (%d) for frame %d | CAMERAID: %d",
                ChxUtils::ErrorMessageCodeToString(pOverrideMessage->message.error.error_code),
                pOverrideMessage->message.error.error_code,
                pOverrideMessage->message.error.frame_number,
                this->m_cameraId);
        ExtensionModule::GetInstance()->ReturnFrameworkMessage(pOverrideMessage, this->m_cameraId);
    }
    else
    {
        CHX_LOG_INFO("A request error has already been sent for this request. Will not send error message: %d",
                     pOverrideMessage->message.error.error_code);
    }
    pOverrideMessage->message.error.frame_number = chiFrameNumber;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::ReturnFrameworkResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::ReturnFrameworkResult(
    const camera3_capture_result_t* pResult,
    UINT32                          cameraID)
{
    camera3_capture_result_t* pOverrideResult               = const_cast<camera3_capture_result_t*>(pResult);
    UINT32                    chiOriginalOverrideFrameNum   = pResult->frame_number;
    UINT32                    resultFrameIndexChi           = chiOriginalOverrideFrameNum % MaxOutstandingRequests;
    BOOL                      metadataResult                = TRUE;
    BOOL                      resultCanBeSent               = TRUE;
    BOOL                      allBuffersReturned            = FALSE;

    pOverrideResult->frame_number = GetAppFrameNum(pResult->frame_number);

    m_pMapLock->Lock();
    CHX_LOG_INFO("chiOriginalOverrideFrameNum: %d frame_number: %d resultFrameIndexF: %d FW: %d, Buffer Count: %d RESULT: %p",
                 chiOriginalOverrideFrameNum,
                 pOverrideResult->frame_number,
                 resultFrameIndexChi,
                 m_captureResult[resultFrameIndexChi].frame_number,
                 m_numberOfPendingOutputBuffers[resultFrameIndexChi],
                 pResult->result);

    if ((NULL  != pResult->result) &&
        (NULL  != m_pLogicalCameraInfo) &&
        (TRUE  == UsecaseSelector::IsQuadCFASensor(m_pLogicalCameraInfo, NULL)) &&
        (FALSE == ExtensionModule::GetInstance()->ExposeFullsizeForQuadCFA()))
    {
        // map ROIs (aec/af/crop region) from full active array size based to binning active arrsy size based
        OverrideResultMetaForQCFA(const_cast<camera_metadata_t*>(pResult->result));
    }

    if (chiOriginalOverrideFrameNum != m_captureResult[resultFrameIndexChi].frame_number)
    {
        CHX_LOG_ERROR("Unexpected Frame Number %u", chiOriginalOverrideFrameNum);
        resultCanBeSent = FALSE;
    }

    camera3_capture_request_t* pRequest = &m_pendingPCRs[resultFrameIndexChi];
    if (0 != m_numberOfPendingOutputBuffers[resultFrameIndexChi])
    {
        // Set NULL to returned buffer so that it won't be returned again in flush call
        CHX_LOG("pResult->num_output_buffers %d pending buffers %d",
            pResult->num_output_buffers,
            m_numberOfPendingOutputBuffers[resultFrameIndexChi]);
        for (UINT resultIdx = 0; resultIdx < pResult->num_output_buffers; resultIdx++)
        {
            camera3_stream_buffer_t* pStreamBuffer = NULL;
            for (UINT requestIdx = 0; requestIdx < pRequest->num_output_buffers; requestIdx++)
            {
                pStreamBuffer = const_cast<camera3_stream_buffer_t*>(&pRequest->output_buffers[requestIdx]);
                if (pResult->output_buffers[resultIdx].stream == pRequest->output_buffers[requestIdx].stream)
                {
                    CHX_LOG("pStreamBuffer %p, i %d, j %d buffer %p",
                        pStreamBuffer,
                        resultIdx,
                        requestIdx,
                        pStreamBuffer->buffer);
                    pStreamBuffer->buffer = NULL;
                    break;
                }
            }
        }
    }
    else
    {
        allBuffersReturned = TRUE;
    }

    if (NULL != pResult->input_buffer)
    {
        pRequest->input_buffer = NULL;
        allBuffersReturned     = FALSE;
    }

    // Decrement the number of pending output buffers given the desired result to return
    if (m_numberOfPendingOutputBuffers[resultFrameIndexChi] >= pResult->num_output_buffers)
    {
        m_numberOfPendingOutputBuffers[resultFrameIndexChi] -= pResult->num_output_buffers;
    }
    else
    {
        resultCanBeSent = FALSE;

        CHX_LOG_ERROR("ChiFrame: %d App Frame: %d - "
                      "pResult contains more buffers (%d) than the expected number of buffers (%d) to return to the framework!",
                      chiOriginalOverrideFrameNum,
                      pOverrideResult->frame_number,
                      pResult->num_output_buffers,
                      m_numberOfPendingOutputBuffers[resultFrameIndexChi]);
    }

    CHX_LOG("m_numberOfPendingOutputBuffers = %d", m_numberOfPendingOutputBuffers[resultFrameIndexChi]);

    BOOL metadataAvailable  = ((NULL != pOverrideResult->result) &&
        (0 != pOverrideResult->partial_result) && (pOverrideResult->partial_result < ExtensionModule::GetInstance()->GetNumMetadataResults()) ) ? TRUE : FALSE;


    // Block AFRegions in CHI level for fixed-focus lens. OEM can customize this based on need.
    // AFRegion is always published in CamX driver
    const LogicalCameraInfo          *pLogicalCameraInfo = NULL;
    pLogicalCameraInfo = ExtensionModule::GetInstance()->GetPhysicalCameraInfo(cameraID);
    if ((NULL != pLogicalCameraInfo) && (NULL != pOverrideResult->result))
    {
        const CHICAMERAINFO* pChiCameraInfo = &(pLogicalCameraInfo->m_cameraCaps);
        if (TRUE == pChiCameraInfo->lensCaps.isFixedFocus)
        {
            camera_metadata_entry_t metadata_entry;
            INT findResult = find_camera_metadata_entry(
                const_cast<camera_metadata_t*>(pOverrideResult->result), ANDROID_CONTROL_AF_REGIONS, &metadata_entry);
            if (0 == findResult) //OK
            {
                delete_camera_metadata_entry(const_cast<camera_metadata_t*>(pOverrideResult->result), metadata_entry.index);
            }
        }
    }


    PartialResultCount partialResultCount =
        static_cast<PartialResultCount>(pOverrideResult->partial_result);
    MetaDataResultCount totalMetaDataCount =
        static_cast<MetaDataResultCount>(ExtensionModule::GetInstance()->GetNumMetadataResults());

    // check if this result is only for partial metadata
    if ((0 < static_cast<UINT8>(partialResultCount)) &&
        (static_cast<UINT8>(partialResultCount) < static_cast<UINT8>(totalMetaDataCount)))
    {
        // Check if final metadata has already been sent
        if (TRUE == m_requestFlags[resultFrameIndexChi].isOutputMetaDataSent)
        {
            resultCanBeSent = FALSE;
            CHX_LOG_WARN("Attempting to Send Partial Metadata after Final Metadata has been sent for Chi Frame: %u FW Frame: %u",
                         chiOriginalOverrideFrameNum,
                         pResult->frame_number);

            if (pOverrideResult->num_output_buffers > 0)
            {
                CHX_LOG_ERROR("Partial Metadata sent with buffers after Metadata is sent for Chi Frame: %u FW Frame: %u",
                              chiOriginalOverrideFrameNum,
                              pResult->frame_number);
            }
        }
    }


    BOOL metadataErrorSent   = m_requestFlags[resultFrameIndexChi].isMetadataErrorSent;
    BOOL allMetadataReturned = m_requestFlags[resultFrameIndexChi].isOutputMetaDataSent;

    if ((TRUE == allBuffersReturned) &&
        ((TRUE == metadataErrorSent) ||
        (TRUE == allMetadataReturned)))
    {
        CHX_LOG_WARN("Result not returned - framework does not need more results for this request: "
                     "allBuffersReturned %d, metadataErrorSent: %d, allMetadataReturned: %d ",
                     allBuffersReturned, metadataErrorSent, allMetadataReturned);
        resultCanBeSent = FALSE;
    }

    if ((FALSE == m_requestFlags[resultFrameIndexChi].isInErrorState) && (resultCanBeSent == TRUE))
    {
        metadataResult = HandleMetadataResultReturn(pOverrideResult, pResult->frame_number, resultFrameIndexChi, cameraID);
        ExtensionModule::GetInstance()->ReturnFrameworkResult(pResult, cameraID);
    }
    else
    {
        CHX_LOG_WARN("Cannot return results for Chi Frame: %u FW Frame: %u",
                     chiOriginalOverrideFrameNum,
                     pResult->frame_number);
    }
    m_pMapLock->Unlock();
    pOverrideResult->frame_number = chiOriginalOverrideFrameNum;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::ReturnFrameworkMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::ReturnFrameworkMessage(
    const camera3_notify_msg_t* pMessage,
    UINT32                      cameraID)
{
    camera3_notify_msg_t* pOverrideMessage = const_cast<camera3_notify_msg_t*>(pMessage);
    UINT64                resultFrame      = pMessage->message.shutter.frame_number;
    UINT                  frameIndex       = (resultFrame % MaxOutstandingRequests);

    m_pMapLock->Lock(); // Maintaining request state and sending a message must occur in a serialized order
    CHX_LOG("ResultFrame = %" PRIu64 " FrameIdx = %d type = %d", resultFrame, frameIndex, pOverrideMessage->type);
    if (InvalidFrameNumber == m_nextAppMessageFrame)
    {
        // We have sent all the messages. Drop this which has come most probably after flush
        CHX_LOG_WARN("[%s] will not send message type: %d for frame %" PRIu64 " because m_nextAppMessageFrame is invalid",
                     ((NULL != m_pChiUsecase) ? m_pChiUsecase->pUsecaseName : "Unknown Usecase"),
                     pOverrideMessage->type,
                     resultFrame);
    }
    else if (ChiMessageTypeShutter == pOverrideMessage->type)
    {
        if (TRUE == m_requestFlags[frameIndex].isMessagePending)
        {
            if (pMessage->message.shutter.frame_number > m_lastAppMessageFrameReceived)
            {
                m_lastAppMessageFrameReceived = pMessage->message.shutter.frame_number;
            }

            pOverrideMessage->message.shutter.frame_number = GetAppFrameNum(pMessage->message.shutter.frame_number);

            CHX_LOG("Override ResultFrame = %d m_lastAppMessageFrameReceived = %" PRIu64 " m_nextAppMessageFrame=%" PRIu64,
                    pOverrideMessage->message.shutter.frame_number,
                    m_lastAppMessageFrameReceived,
                    m_nextAppMessageFrame);
            ChxUtils::Memcpy(&m_notifyMessage[frameIndex], pOverrideMessage, sizeof(camera3_notify_msg_t));
            m_requestFlags[frameIndex].isMessageAvailable = TRUE;
            ReturnPendingAvailableFrameworkMessages(cameraID);
        }
    }
    else if (ChiMessageTypeError == pOverrideMessage->type)
    {
        camera3_capture_request_t* pRequest  = &m_pendingPCRs[frameIndex];
        ChiErrorMessageCode        errorCode = static_cast<ChiErrorMessageCode>(pMessage->message.error.error_code);

        if (MessageCodeBuffer == errorCode)
        {
            BOOL                          isFrameworkBuffer = FALSE;
            const camera3_stream_t* const pErrorStream      = pOverrideMessage->message.error.error_stream;
            for (UINT i = 0; i < pRequest->num_output_buffers; i++)
            {
                if (pRequest->output_buffers[i].stream == pErrorStream)
                {
                    isFrameworkBuffer = TRUE;
                    break;
                }
            }
            if (FALSE == isFrameworkBuffer)
            {
                errorCode = MessageCodeRequest;
                CHX_LOG_INFO("Chi Frame %u cannot send a buffer error for stream: %p",
                             pRequest->frame_number,
                             pErrorStream);
            }
        }

        if ((pRequest->frame_number != resultFrame) && (errorCode != MessageCodeDevice))
        {
            CHX_LOG_ERROR("Will not sent Error Message: %s to framework\n"
                          "Result Frame Number: %" PRIu64 " does not match Request Chi Frame Number: %u",
                          ChxUtils::ErrorMessageCodeToString(errorCode),
                          resultFrame,
                          pRequest->frame_number);
        }
        else
        {
            switch (errorCode)
            {
                case MessageCodeRequest:
                    HandleProcessRequestError(pRequest);
                    break;
                case MessageCodeResult:
                    HandleResultError(pRequest);
                    break;
                case MessageCodeDevice:
                    DumpDebugInfo();
                    ReturnFrameworkErrorMessage(pMessage);
                    break;
                default:
                    ReturnFrameworkErrorMessage(pMessage);
                    break;
            }
            ReturnPendingAvailableFrameworkMessages(cameraID);
        }
    }
    else
    {
        CHX_LOG_ERROR("Unsupported Framework Message - ChiMessage Type: %d", pOverrideMessage->type);
    }

    m_pMapLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::ReturnPendingAvailableFrameworkMessages
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::ReturnPendingAvailableFrameworkMessages(
        UINT32 cameraID)
{
    for (INT64 i = m_nextAppMessageFrame; i <= m_lastAppMessageFrameReceived; i++)
    {
        UINT frameIndex                     = (i % MaxOutstandingRequests);
        UsecaseRequestFlags* pRequestFlags  = &m_requestFlags[frameIndex];
        UINT32  messageAvailableToSend      = ((TRUE == pRequestFlags->isMessageAvailable) &&
                                               (TRUE == pRequestFlags->isMessagePending));
        UINT32  outputBuffersInRequest      = m_pendingPCRs[frameIndex].num_output_buffers;
        UINT32  noMoreValidBuffers          = ((TRUE == pRequestFlags->isBufferErrorSent) &&
                                               ((0 == m_numberOfPendingOutputBuffers[frameIndex]) ||
                                                (outputBuffersInRequest == m_numBufferErrorMessages[frameIndex])));
        UINT32  entireRequestHasErrored     = ((TRUE == pRequestFlags->isInErrorState) ||
                                               ((TRUE == noMoreValidBuffers) && (TRUE == pRequestFlags->isMetadataErrorSent)));
        CHX_LOG("frameIndex = %d messageAvailable = %d messagePending = %d isInErrorState = %d, isBufferErrorSent = %d, "
                "isMetadataErrorSent = %d",
                frameIndex, m_requestFlags[frameIndex].isMessageAvailable,
                pRequestFlags->isMessagePending,
                pRequestFlags->isInErrorState,
                pRequestFlags->isBufferErrorSent,
                pRequestFlags->isMetadataErrorSent);

        // Feature2 now translates request errors to buffer/metadata errors so need an additional check if
        // we have a buffer error as a means of blocking shutter. If there is a buffer error and no pending
        // buffers, means framework does not expect a shutter; do not send but do increment update request flags
        if ((TRUE == messageAvailableToSend) || (TRUE == entireRequestHasErrored))
        {
            if ((TRUE == messageAvailableToSend) && (FALSE == entireRequestHasErrored))
            {
                ExtensionModule::GetInstance()->ReturnFrameworkMessage(&m_notifyMessage[frameIndex], cameraID);
            }

            m_requestFlags[frameIndex].isMessageAvailable = FALSE;
            m_requestFlags[frameIndex].isMessagePending   = FALSE;
            // This index must be incremented even on error
            m_nextAppMessageFrame++;
        }
        else
        {
            // Check if next index has already been sent as Snapshot, if TRUE, increment again
            if (FALSE == m_requestFlags[frameIndex].isMessagePending)
            {
                m_nextAppMessageFrame++;
            }
            else
            {
                break; // Wait until the nextAppMessageFrame is ready
            }
        }
    }

    for (INT64 i = m_nextAppMessageFrame; i <= m_lastAppMessageFrameReceived; i++)
    {
        UINT frameIndex = (i % MaxOutstandingRequests);
        UsecaseRequestFlags* pRequestFlags          = &m_requestFlags[frameIndex];
        UINT32               messageAvailableToSend = ((TRUE == pRequestFlags->isMessageAvailable) &&
                                                       (TRUE == pRequestFlags->isMessagePending));
        UINT32               outputBuffersInRequest = m_pendingPCRs[frameIndex].num_output_buffers;
        UINT32               noMoreValidBuffers     = ((TRUE == pRequestFlags->isBufferErrorSent) &&
                                                       ((0 == m_numberOfPendingOutputBuffers[frameIndex]) ||
                                                       (outputBuffersInRequest == m_numBufferErrorMessages[frameIndex])));

        if (TRUE == m_requestFlags[frameIndex].isZSLMessageAvailable)
        {
            if ((TRUE == messageAvailableToSend) || (TRUE == pRequestFlags->isInErrorState) ||
                (TRUE == noMoreValidBuffers))
            {
                if ((TRUE == messageAvailableToSend) && (FALSE == noMoreValidBuffers))
                {
                    CHX_LOG_INFO("Sending Snapshot frame_number: %" PRIu64", frame_index: %u", i, frameIndex);
                    ExtensionModule::GetInstance()->ReturnFrameworkMessage(&m_notifyMessage[frameIndex], cameraID);
                }

                m_requestFlags[frameIndex].isMessageAvailable = FALSE;
                m_requestFlags[frameIndex].isMessagePending = FALSE;
            }
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::UpdateSensorModeIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::UpdateSensorModeIndex(
    camera_metadata_t* pMetaData)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL != pMetaData))
    {
        if (FALSE == ChxUtils::AndroidMetadata::IsVendorTagPresent(reinterpret_cast<const VOID*>(pMetaData),
            VendorTag::SensorModeIndex))
        {
            result = ChxUtils::AndroidMetadata::SetVendorTagValue(pMetaData, VendorTag::SensorModeIndex, 1,
                &m_selectedSensorModeIndex);
        }
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Cant set sensor mode index vendor tag");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::UpdateFeatureModeIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::UpdateFeatureModeIndex(
    camera_metadata_t* pMetaData)
{
    CDKResult result = CDKResultSuccess;

    CHX_LOG("%s E", __func__);
    if ((NULL != pMetaData))
    {
        if (FALSE == ChxUtils::AndroidMetadata::IsVendorTagPresent(reinterpret_cast<const VOID*>(pMetaData),
            VendorTag::Feature1Mode))
        {
            ChiModeFeature1SubModeType featureMode = ChiModeFeature1SubModeType::None;
            result = ChxUtils::AndroidMetadata::SetVendorTagValue(pMetaData, VendorTag::Feature1Mode, 1, &featureMode);
        }

        if (ExtensionModule::GetInstance()->GetVideoHDRMode())
        {
            ChiModeFeature1SubModeType featureMode = ChiModeFeature1SubModeType::ISPHDR;
            result = ChxUtils::AndroidMetadata::SetVendorTagValue(pMetaData, VendorTag::Feature1Mode, 1, &featureMode);
        }

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Cant set Feature1Mode mode index vendor tag");
        }

        if (FALSE == ChxUtils::AndroidMetadata::IsVendorTagPresent(reinterpret_cast<const VOID*>(pMetaData),
            VendorTag::Feature2Mode))
        {
            ChiModeFeature2SubModeType featureMode = ChiModeFeature2SubModeType::None;
            result = ChxUtils::AndroidMetadata::SetVendorTagValue(pMetaData, VendorTag::Feature2Mode, 1, &featureMode);
        }
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Cant set Feature1Mode mode index vendor tag");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::FlushThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* Usecase::FlushThread(
    VOID* pThreadData)
{
    PerThreadData* pPerThreadData = reinterpret_cast<PerThreadData*>(pThreadData);
    Usecase*       pUsecase       = reinterpret_cast<Usecase*>(pPerThreadData->pPrivateData);

    pUsecase->FlushThreadProcessing(reinterpret_cast<VOID*>(pPerThreadData->hThreadHandle));

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::FlushThreadProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::FlushThreadProcessing(VOID* pThreadHandle)
{
    UINT32 sessionIndex = MaxSessions;

    m_pFlushMutex->Lock();
    for (UINT session = 0; session < MaxSessions; session++)
    {
        if (pThreadHandle == reinterpret_cast<VOID*>(m_FlushProcessThreadData[session].hThreadHandle))
        {
            sessionIndex = session;
        }
    }
    m_pFlushMutex->Unlock();

    // return from if sessionIndex exceeds MaxSessions
    if (sessionIndex >= MaxSessions)
    {
        return;
    }

    BOOL shouldThreadContinue = TRUE;
    m_pFlushThreadMutex[sessionIndex]->Lock();
    while (TRUE == shouldThreadContinue)
    {
        CHX_LOG_INFO("Wait for Session IndeX: %d - Handle: %p", sessionIndex, pThreadHandle);

        m_pFlushstartCondition[sessionIndex]->Wait(m_pFlushThreadMutex[sessionIndex]->GetNativeHandle());
        m_pFlushThreadMutex[sessionIndex]->Unlock();

        shouldThreadContinue = m_shouldFlushThreadExecute;
        Session* pSession    = m_pSessionsToFlush[sessionIndex];

        if (NULL != pSession)
        {
            ExtensionModule::GetInstance()->Flush(pSession->GetSessionHandle());
            m_pSessionsToFlush[sessionIndex] = NULL;
        }

        m_pFlushThreadMutex[sessionIndex]->Lock(); // Acquire thread lock to prevent us from being signaled before wait
        if(NULL != pSession)
        {
            m_pParallelFlushLock->Lock();
            if (0 == ChxUtils::AtomicDecU32(&m_numOfSessionsToBeFlushed))
            {
                m_pParallelFlushDone->Signal();
            }
            m_pParallelFlushLock->Unlock();
        }
        else if(FALSE == m_isUsecaseDestroy)
        {
            CHX_LOG_ERROR("Session Index: %d has a null entry to flush. BAD STATE", sessionIndex);
        }
    }
    m_pFlushThreadMutex[sessionIndex]->Unlock();
    CHX_LOG_INFO("Exit Session Index: %d", sessionIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::FlushAllSessions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::FlushAllSessions(
    Session* const *    ppSession,
    const UINT32        size)
{
    CDKResult result = CDKResultSuccess;
    if (m_useParallelFlush)
    {
        result = FlushAllSessionsInParallel(ppSession, size);
    }
    else
    {
        CHX_LOG_INFO("Flushing Sessions serially");
        for (UINT i = 0; i < size; i++)
        {
            Session* pSession = ppSession[i];
            CHX_LOG_INFO("Flushing session:%d, %p", i, pSession);
            if (NULL != pSession)
            {
                ExtensionModule::GetInstance()->Flush(pSession->GetSessionHandle());
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::FlushAllSessionsInParallel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::FlushAllSessionsInParallel(
    Session* const *    ppSession,
    const UINT32        size)
{
    CHX_LOG_INFO("[E]");

    CDKResult result            = CDKResultSuccess;
    m_numOfSessionsToBeFlushed  = 0;
    for (UINT sessionIndex = 0; sessionIndex < size; sessionIndex++)
    {
        if (NULL != ppSession[sessionIndex] || (FALSE == m_shouldFlushThreadExecute))
        {
            if (NULL != ppSession[sessionIndex])

            {
                ChxUtils::AtomicIncU32(&m_numOfSessionsToBeFlushed);
            }
            m_pSessionsToFlush[sessionIndex] = ppSession[sessionIndex];
            m_pFlushThreadMutex[sessionIndex]->Lock();
            m_pFlushstartCondition[sessionIndex]->Signal();
            m_pFlushThreadMutex[sessionIndex]->Unlock();
        }
        else
        {
            CHX_LOG("Skipping Session Index: %d Handle: NULL", sessionIndex);
        }
    }

    if (ChxUtils::AtomicLoadU32(&m_numOfSessionsToBeFlushed) > 0)
    {
        m_pParallelFlushLock->Lock();
        const UINT flushWaitTime = FLUSH_ALL_THREADS_TIMEOUT / 2;
        m_pParallelFlushDone->TimedWait(m_pParallelFlushLock->GetNativeHandle(), flushWaitTime);
        if (ChxUtils::AtomicLoadU32(&m_numOfSessionsToBeFlushed) > 0)
        {
            CHX_LOG_ERROR("CamX flush is taking too long! - Flush Status: ");
            for (UINT i = 0; i < size; i++)
            {
                CHX_LOG_ERROR("\tSession[%d] Status: %s", i, (NULL == m_pSessionsToFlush[i]) ? "Done" : "Flushing");
            }
            result = m_pParallelFlushDone->TimedWait(m_pParallelFlushLock->GetNativeHandle(), flushWaitTime);
        }
        m_pParallelFlushLock->Unlock();
        CHX_LOG_INFO("Usecase::Flush timed wait done");
    }
    else
    {
        CHX_LOG_INFO("No sessions waiting to flush!");
    }
    CHX_LOG_INFO("[X]");

    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LogFeatureRequestMappings
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::LogFeatureRequestMappings(UINT32 inFrameNum, UINT32 reqFrameNum, const CHAR* identifyingData)
{
    CDK_UNUSED_PARAM(inFrameNum);
    CDK_UNUSED_PARAM(reqFrameNum);
    CDK_UNUSED_PARAM(identifyingData);

    CHX_LOG_REQMAP("Derieved classes (MultiCam, Advanced, or Base Usecase) should override this method.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::Flush()
{
    CDKResult result        = CDKResultSuccess;
    CDKResult derivedResult = CDKResultSuccess;
    BOOL      executeFlush  = FALSE;

    // Determine whether or not a flush should occur
    m_pFlushMutex->Lock();

    if (FlushStatus::NotFlushing == m_flushStatus)
    {
        executeFlush  = TRUE;
        m_flushStatus = FlushStatus::IsFlushing;
    }

    m_pFlushMutex->Unlock();

    if (TRUE == executeFlush)
    {
        const CHAR* pName = (m_pChiUsecase != NULL ? m_pChiUsecase->pUsecaseName : "UnknownUsecase");
        CHX_LOG_INFO("[%s] Flush start", pName);
        // Flush sessions
        FlushAllSessions(m_pSession, MaxSessions);

        m_shouldFlushThreadExecute = (FALSE == m_isUsecaseDestroy);
        derivedResult              = ExecuteFlush();
        CHX_LOG_INFO("ExecuteFlush - END");
        if (CDKResultSuccess != derivedResult)
        {
            CHX_LOG_ERROR("[%s] ExecuteFlush failed Error Code: %d", pName, derivedResult);
            result = derivedResult;
        }

        DeleteAllPendingResults();

        // Reinitialize Request information
        m_lastAppMessageFrameReceived  = 0;
        m_nextAppResultFrame           = InvalidFrameNumber;
        m_lastAppRequestFrame          = 0;
        m_lastResultMetadataFrameNum   = 0;

        InitializeRequestFlags();
        ChxUtils::Memset(&m_notifyMessage[0], 0, sizeof(m_notifyMessage));
        CHX_LOG_INFO("[%s] Flush End", pName);

        m_pFlushMutex->Lock();

        m_flushStatus = FlushStatus::NotFlushing;
        m_pFlushDone->Broadcast();

        m_pFlushMutex->Unlock();
    }
    else if (FlushStatus::HasFlushed == m_flushStatus)
    {
        CHX_LOG_ERROR("[%s] Cannot Flush because a flush has already occured!",
            (m_pChiUsecase != NULL ? m_pChiUsecase->pUsecaseName : "UnknownUsecase"));
    }
    else
    {
        CHX_LOG("[%s] Flush Already in progress!", (m_pChiUsecase != NULL ? m_pChiUsecase->pUsecaseName : "UnknownUsecase"));
        // In the case an internal flush is occuring and a framework flush is issued,
        // block the caller until the internal flush completes
        WaitUntilFlushFinishes();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::FlushRequestsByFrameNumber
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::FlushRequestsByFrameNumber(
    BOOL   bLock,
    UINT32 lastChiFrameNum)
{
    // Only flush requests for whose framenumber is <= lastChiFrameNum

    CHX_LOG("FlushUnLock start, chiFramenumber:%d, appFramenumber:%d",
        lastChiFrameNum, GetAppFrameNum(lastChiFrameNum));

    if (TRUE == bLock)
    {
        CHX_LOG_ERROR("bLock true is not supported!");
        return;
    }

    m_pMapLock->Lock();

    for (UINT i = 0; i < MaxOutstandingRequests; i++)
    {
        camera3_capture_request_t request = m_pendingPCRs[i];
        if ((lastChiFrameNum > 0) && (request.frame_number > lastChiFrameNum))
        {
            continue;
        }

        CHX_LOG("Pending result IN MAP0 =%d", request.frame_number);
        if (0 != request.num_output_buffers)
        {
            CHX_LOG("deleting Pending result IN MAP0 =%d", request.frame_number);
            HandleProcessRequestError(&request);
        }

        ChxUtils::Memset(&m_notifyMessage[i], 0, sizeof(camera3_notify_msg_t));
    }

    for (UINT i = 0; i < MaxOutstandingRequests; i++)
    {
        camera3_capture_request_t request = m_pendingPCRs[i];
        if (lastChiFrameNum > 0 && (request.frame_number > lastChiFrameNum))
        {
            continue;
        }

        if (0 != m_pendingPCRs[i].num_output_buffers)
        {
            m_pendingPCRs[i].num_output_buffers = 0;
        }
    }

    m_nextAppMessageFrame           = lastChiFrameNum + 1;
    m_nextAppResultFrame            = lastChiFrameNum + 1;

    m_lastAppMessageFrameReceived   = lastChiFrameNum;
    m_lastResultMetadataFrameNum    = lastChiFrameNum;

    m_pMapLock->Unlock();

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::Dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::Dump(
    int     fd)
{
    CDKResult result        = CDKResultSuccess;

    CHX_LOG_TO_FILE(fd, 2, "+------------------------------------------------------------------+\n  "
                           "+         ChiUsecase: %d Frame Dump                                 +\n  "
                           "+------------------------------------------------------------------+",
                           m_usecaseId);

    result = m_pMapLock->TryLock();

    if ((CDKResultSuccess == result) && (MaxOutstandingRequests > m_lastAppRequestFrame))
    {
        // DumpAllPendingResults
        UINT32 latestFrameworkRequestIndex  = m_lastAppRequestFrame;
        UINT32 count                        = MaxOutstandingRequests;

        while (count)
        {
            count--;
            camera3_capture_request_t* pRequest = &m_pendingPCRs[latestFrameworkRequestIndex];
            camera3_capture_result_t*  pResult  = &m_captureResult[latestFrameworkRequestIndex];

            const BOOL& hasSentMetadata = m_requestFlags[latestFrameworkRequestIndex].isOutputMetaDataSent;

            if ((m_requestFlags[latestFrameworkRequestIndex].isInErrorState == FALSE) &&
                   ((0 != m_numberOfPendingOutputBuffers[latestFrameworkRequestIndex]) || (FALSE == hasSentMetadata)))
            {
                CHX_LOG_TO_FILE(fd, 4, "Dumping result CHI frame: %u, Framework Frame: %u, Num Buffers: %u, "
                              "m_numberOfPendingOutputBuffers: %u, isMessageAvailable: %u, "
                              "isMessagePending: %u, isInErrorState: %u, isOutputMetaDataSent: %u, "
                              "isOutputPartialMetaDataSent: %u, isDriverPartialMetaDataSent: %u, isMetadataErrorSent: %u",
                              pRequest->frame_number,
                              pResult->frame_number,
                              pRequest->num_output_buffers,
                              m_numberOfPendingOutputBuffers[latestFrameworkRequestIndex],
                              m_requestFlags[latestFrameworkRequestIndex].isMessageAvailable,
                              m_requestFlags[latestFrameworkRequestIndex].isMessagePending,
                              m_requestFlags[latestFrameworkRequestIndex].isInErrorState,
                              m_requestFlags[latestFrameworkRequestIndex].isOutputMetaDataSent,
                              m_requestFlags[latestFrameworkRequestIndex].isOutputPartialMetaDataSent,
                              m_requestFlags[latestFrameworkRequestIndex].isDriverPartialMetaDataSent,
                              m_requestFlags[latestFrameworkRequestIndex].isMetadataErrorSent);
            }
            if (0 == latestFrameworkRequestIndex)
            {
                latestFrameworkRequestIndex = MaxOutstandingRequests - 1;
            }
            else
            {
                latestFrameworkRequestIndex--;
            }
        }
    }
    else
    {
        CHX_LOG("m_frameworkRequests is empty latestFrameworkRequestIndex: %" PRIu64 ", lock result %d",
                m_lastAppRequestFrame, result);
    }
    if (CDKResultSuccess == result)
    {
        m_pMapLock->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::HandleMetadataResultReturn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Usecase::HandleMetadataResultReturn(
    camera3_capture_result_t* pOverrideResult,
    UINT32                    frame_Number,
    UINT32                    resultFrameIndexChi,
    UINT32                    cameraID)
{
    BOOL bValid = TRUE;
    CDK_UNUSED_PARAM(cameraID);

    m_pMapLock->Lock();
    if (NULL != pOverrideResult->result)
    {
        PartialResultCount partialResultCount   =
            static_cast<PartialResultCount>(pOverrideResult->partial_result);
        MetaDataResultCount totalMetaDataCount  =
            static_cast<MetaDataResultCount>(ExtensionModule::GetInstance()->GetNumMetadataResults());

        CHX_LOG("Sending metadata for [FrameNum=%d and override FrameNum=%d] with totalexpectedCount=%d partialResultCount=%d",
            frame_Number,
            pOverrideResult->frame_number,
            totalMetaDataCount,
            partialResultCount);

        switch (totalMetaDataCount)
        {
        // When the TotalMetaDatCount is equal to OneMetaDataCount then there is no partial
        // meta data supported. Meta Data will be sent by the core driver. To avoid multiple times
        // sending the same metadata, we mark the metadata as sent here.
        case MetaDataResultCount::OneMetaDataCount:
            if (PartialResultCount::FirstPartialResult == partialResultCount)
            {
                SetMetaDataHasBeenSent(PartialResultSender::DriverMetaData, resultFrameIndexChi, TRUE);
                CHX_LOG("Partial Result Completed for framenumber=%d", frame_Number);
            }
            else
            {
                CHX_LOG_ERROR("Unexpected Partial Result for framenumber=%d", frame_Number);
                bValid = FALSE;
            }
            break;
        // When the TotalMetaDatCount is equal to TwoMetaDataCount then there is partial
        // meta data supported. Partial Data support can be either by
        // 1) CHI Partial Data (no Driver Partial MetaData is supported)
        // 2) Driver Partial Data (no User Partial MetaData is supported)
        // 3) Driver and CHI Partial Data is being sent as merged
        case MetaDataResultCount::TwoMetaDataCount:
        {
            // This is Handling of 1) CHI Partial Data(no Driver Partial MetaData is supported)
            if (PartialMetaSupport::SeperatePartialMeta == ExtensionModule::GetInstance()->EnableCHIPartialData())
            {
                if (PartialResultCount::FirstPartialResult == partialResultCount)
                {
                    // Once CHI Partial Result is sent we need to mark it as sent and thereby avoid duplicates
                    SetMetaDataHasBeenSent(PartialResultSender::CHIPartialData, resultFrameIndexChi, TRUE);
                }
                else if (PartialResultCount::SecondPartialResult == partialResultCount)
                {
                    // When the final Partial result is being sent, we need to check if all previous partial
                    // result has been sent.
                    if (FALSE == CheckIfMetaDataHasBeenSent(PartialResultSender::CHIPartialData, resultFrameIndexChi))
                    {
                        // Since CHI Partial result has not been sent, still mark it as sent
                        SetMetaDataHasBeenSent(PartialResultSender::CHIPartialData, resultFrameIndexChi, TRUE);
                    }
                    SetMetaDataHasBeenSent(PartialResultSender::DriverMetaData, resultFrameIndexChi, TRUE);
                    CHX_LOG("Partial Result Completed for framenumber=%d", frame_Number);
                }
                else
                {
                    CHX_LOG_ERROR("Unexpected Partial Result for framenumber=%d", frame_Number);
                    bValid = FALSE;
                }
            }
            // This is Handling of either
            // 2) Driver Partial Data (no User Partial MetaData is supported)
            // 3) Driver and CHI Partial Data is being sent as merged
            else
            {
                if (PartialResultCount::FirstPartialResult == partialResultCount)
                {
                    if (PartialMetaSupport::CombinedPartialMeta == ExtensionModule::GetInstance()->EnableCHIPartialData())
                    {
                        // CHI Partial result has not been sent, still mark it as sent
                        SetMetaDataHasBeenSent(PartialResultSender::CHIPartialData, resultFrameIndexChi, TRUE);
                    }
                    SetMetaDataHasBeenSent(PartialResultSender::DriverPartialData, resultFrameIndexChi, TRUE);
                }
                else if (PartialResultCount::SecondPartialResult == partialResultCount)
                {
                    // Check if Driver Partial Result has been sent or not
                    if (FALSE == CheckIfMetaDataHasBeenSent(PartialResultSender::DriverPartialData, resultFrameIndexChi))
                    {
                        if (PartialMetaSupport::CombinedPartialMeta == ExtensionModule::GetInstance()->EnableCHIPartialData())
                        {
                            // CHI Partial result has not been sent, still mark it as sent
                            SetMetaDataHasBeenSent(PartialResultSender::CHIPartialData, resultFrameIndexChi, TRUE);
                        }
                        SetMetaDataHasBeenSent(PartialResultSender::DriverPartialData, resultFrameIndexChi, TRUE);
                    }
                    SetMetaDataHasBeenSent(PartialResultSender::DriverMetaData, resultFrameIndexChi, TRUE);
                    CHX_LOG("Partial Result Completed for framenumber=%d", frame_Number);
                }
                else
                {
                    CHX_LOG_ERROR("Unexpected Partial Result");
                    bValid = FALSE;
                }
            }
        }
            break;
        case MetaDataResultCount::ThreeMetaDataCount:
        {
            if (PartialResultCount::FirstPartialResult == partialResultCount)
            {
                SetMetaDataHasBeenSent(PartialResultSender::CHIPartialData, resultFrameIndexChi, TRUE);
            }
            else if (PartialResultCount::SecondPartialResult == partialResultCount)
            {
                SetMetaDataHasBeenSent(PartialResultSender::DriverPartialData, resultFrameIndexChi, TRUE);
            }
            else if (PartialResultCount::ThirdPartialResult == partialResultCount)
            {
                // Step1: Check if Driver Partial Result has been sent or not
                if (FALSE == CheckIfMetaDataHasBeenSent(PartialResultSender::DriverPartialData, resultFrameIndexChi))
                {
                    // Driver Partial result has not been sent, still mark it as sent
                    SetMetaDataHasBeenSent(PartialResultSender::DriverPartialData, resultFrameIndexChi, TRUE);
                }
                // Step2: Check if CHIPartial Result Needs to be sent or not
                if (FALSE == CheckIfMetaDataHasBeenSent(PartialResultSender::CHIPartialData, resultFrameIndexChi))
                {
                    // CHI Partial result has not been sent, still mark it as sent
                    SetMetaDataHasBeenSent(PartialResultSender::CHIPartialData, resultFrameIndexChi, TRUE);
                }
                // Step3: Set the Core driver meta Data has been sent
                SetMetaDataHasBeenSent(PartialResultSender::DriverMetaData, resultFrameIndexChi, TRUE);
                CHX_LOG("Partial Result Completed for framenumber=%d", frame_Number);
            }
            else
            {
                CHX_LOG_ERROR("Unexpected Partial Result");
                bValid = FALSE;
            }
        }
            break;
        case MetaDataResultCount::NoMetaDataCount:
        default:
            break;
        }
    }

    m_pMapLock->Unlock();
    return bValid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::CheckIfPartialDataCanBeSent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Usecase::CheckIfPartialDataCanBeSent(PartialResultSender sender, UINT32 resultFrameIndexChi)
{
    BOOL    canBeSent           = FALSE;

    m_pMapLock->Lock();

    switch (sender)
    {
    case PartialResultSender::CHIPartialData:
        // Only send partial if final has not been sent
        if ((ExtensionModule::GetInstance()->EnableCHIPartialData() != PartialMetaSupport::NoSupport) &&
            (FALSE == CheckIfMetaDataHasBeenSent(sender, resultFrameIndexChi)) &&
            (FALSE == CheckIfMetaDataHasBeenSent(PartialResultSender::DriverMetaData, resultFrameIndexChi)))
        {
            canBeSent = TRUE;
        }
        break;
    case PartialResultSender::DriverPartialData:
        // Only send partial if final has not been sent
        if (FALSE == CheckIfMetaDataHasBeenSent(PartialResultSender::DriverMetaData, resultFrameIndexChi))
        {
            canBeSent = !CheckIfMetaDataHasBeenSent(sender, resultFrameIndexChi);
        }
        break;
    case PartialResultSender::DriverMetaData:
        canBeSent = !CheckIfMetaDataHasBeenSent(sender, resultFrameIndexChi);
        break;
    }

    if (TRUE == m_requestFlags[resultFrameIndexChi].isMetadataErrorSent)
    {
        canBeSent = FALSE;
    }

    m_pMapLock->Unlock();

    CHX_LOG("CheckIfMetadata for resultFrameIndexChi=%d Sent=%d", resultFrameIndexChi, canBeSent);
    return canBeSent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::CheckIfMetaDataHasBeenSent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Usecase::CheckIfMetaDataHasBeenSent(
    PartialResultSender sender,
    UINT32              resultFrameIndexChi)
{
    BOOL    isMetadataSent      = FALSE;

    m_pMapLock->Lock();

    switch (sender)
    {
    case PartialResultSender::CHIPartialData:
        isMetadataSent = m_requestFlags[resultFrameIndexChi].isOutputPartialMetaDataSent;
        break;
    case PartialResultSender::DriverPartialData:
        isMetadataSent = m_requestFlags[resultFrameIndexChi].isDriverPartialMetaDataSent;
        break;
    case PartialResultSender::DriverMetaData:
        isMetadataSent = m_requestFlags[resultFrameIndexChi].isOutputMetaDataSent;
        break;
    }

    m_pMapLock->Unlock();

    CHX_LOG("CheckIfMetadata for resultFrameIndexChi=%d Sent=%d", resultFrameIndexChi, isMetadataSent);
    return isMetadataSent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::CheckIfAnyPartialMetaDataHasBeenSent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Usecase::CheckIfAnyPartialMetaDataHasBeenSent(
    UINT32              resultFrameIndexChi)
{
    BOOL    isMetadataSent = FALSE;

    m_pMapLock->Lock();

    if ((TRUE == m_requestFlags[resultFrameIndexChi].isOutputPartialMetaDataSent) ||
        (TRUE == m_requestFlags[resultFrameIndexChi].isDriverPartialMetaDataSent) ||
        (TRUE == m_requestFlags[resultFrameIndexChi].isOutputMetaDataSent))
    {
        isMetadataSent = TRUE;
    }

    m_pMapLock->Unlock();

    CHX_LOG("CheckIfAnyPartialMetaDataHasBeenSent for resultFrameIndexChi=%d Sent=%d", resultFrameIndexChi, isMetadataSent);
    return isMetadataSent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::SetMetaDataHasBeenSent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::SetMetaDataHasBeenSent(
    PartialResultSender sender,
    UINT32              resultFrameIndexChi,
    BOOL                hasSent)
{
    m_pMapLock->Lock();

    switch (sender)
    {
    case PartialResultSender::CHIPartialData:
        m_requestFlags[resultFrameIndexChi].isOutputPartialMetaDataSent = hasSent;
        break;
    case PartialResultSender::DriverPartialData:
        m_requestFlags[resultFrameIndexChi].isDriverPartialMetaDataSent = hasSent;
        break;
    case PartialResultSender::DriverMetaData:
        m_requestFlags[resultFrameIndexChi].isOutputMetaDataSent = hasSent;
        break;
    }

    m_pMapLock->Unlock();

    CHX_LOG("Metadata for sender %d with resultFrameIndexChi=%d Sent=%d",
        sender, resultFrameIndexChi, hasSent);
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::InjectFrameworkResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::InjectFrameworkResult(
    UINT32              frameNumber,
    PartialResultCount  partialResultCount,
    UINT32              cameraID)
{
    if (ExtensionModule::GetInstance()->EnableCHIPartialDataRecovery() == TRUE)
    {
        camera3_capture_result_t injectResult;

        injectResult.frame_number         = frameNumber;
        injectResult.input_buffer         = NULL;
        injectResult.output_buffers       = NULL;
        injectResult.partial_result       = static_cast<int>(partialResultCount);
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) //Android-P or better
        injectResult.num_physcam_metadata = 0;
#endif//Android-P or better
        injectResult.num_output_buffers   = 0;
        injectResult.result               = static_cast<camera_metadata*>(m_pEmptyMetaData);
        ExtensionModule::GetInstance()->ReturnFrameworkResult(
            reinterpret_cast<const camera3_capture_result_t*>(&injectResult),
            cameraID);
    }
    else
    {
        CHX_LOG_ERROR("Partial Meta data has not been sent for frame number:%d",frameNumber);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::DumpDebugInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::DumpDebugInfo()
{
    CHX_LOG_INFO("''''''''''DUMPING USEFUL DEBUG DATA'''''''''''''''''");
    CHX_LOG_INFO("Usecase %s selected", (m_pChiUsecase != NULL ? m_pChiUsecase->pUsecaseName : "UnknownUsecase"));
    if (NULL != m_pChiUsecase)
    {
        for (UINT i = 0; i < m_pChiUsecase->numPipelines; i++)
        {
            CHX_LOG_INFO("[%d/%d], pipeline name:%s",
                         i,
                         m_pChiUsecase->numPipelines,
                         m_pChiUsecase->pPipelineTargetCreateDesc[i].pPipelineName);
        }
    }
    CHX_LOG_INFO("SelectedSensorModeIndex =  %d", m_selectedSensorModeIndex);
    CHX_LOG_INFO("CAMERAID =  %d", m_cameraId);
    ExtensionModule::GetInstance()->DumpDebugData(m_cameraId);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::CreateMetadataManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::CreateMetadataManager(
    INT32     cameraId,
    bool      initFrameworkMetadata,
    Pipeline* pDefaultPipeline,
    bool      initGenericMetadata)
{
    CDKResult result = CDKResultSuccess;

    CAMX_UNREFERENCED_PARAM(cameraId);

    if (NULL == m_pMetadataManager)
    {
        m_pMetadataManager = ChiMetadataManager::Create();

        if (NULL != m_pMetadataManager)
        {
            if (initFrameworkMetadata)
            {
                result = m_pMetadataManager->InitializeFrameworkInputClient(DefaultNumMetadataBuffers);
            }

            if (initGenericMetadata)
            {
                m_genericMetadataClientId = m_pMetadataManager->RegisterClient(
                    TRUE,
                    NULL,
                    0,
                    0,
                    DefaultNumMetadataBuffers,
                    ChiMetadataUsage::Generic);
            }

            if ((CDKResultSuccess == result) && (NULL != pDefaultPipeline))
            {
                m_metadataClientId = m_pMetadataManager->RegisterClient(
                    pDefaultPipeline->IsRealTime(),
                    pDefaultPipeline->GetTagList(),
                    pDefaultPipeline->GetTagCount(),
                    pDefaultPipeline->GetPartialTagCount(),
                    DefaultNumMetadataBuffers,
                    pDefaultPipeline->IsRealTime() ?
                    ChiMetadataUsage::RealtimeOutput : ChiMetadataUsage::OfflineOutput);

                if (ChiMetadataManager::InvalidClientId == m_metadataClientId)
                {
                    result = CDKResultEFailed;
                }
            }
        }
        else
        {
            result = CDKResultENoMemory;
        }
    }
    else
    {
        result = CDKResultEFailed;

        CHX_LOG_ERROR("[CMB_ERROR] ERROR metadata manager already initialized");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::MetadataCaptureRequestUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::MetadataCaptureRequestUpdate(
    CHICAPTUREREQUEST& captureRequest,
    UINT32             sessionId,
    bool               isInputSticky)
{
    CDKResult result = CDKResultSuccess;

    CHX_ASSERT(MaxSessions > sessionId);

    std::lock_guard<std::mutex> guard(m_MetadataLock);

    if ((MaxSessions > sessionId) &&
        (NULL == captureRequest.pInputMetadata) &&
        (NULL == captureRequest.pOutputMetadata) &&
        (ChiMetadataManager::InvalidClientId != m_genericMetadataClientId))
    {
        ChiMetadataBundle bundle  = { 0 };

        bundle.isInputSticky      = isInputSticky;
        UINT32 frameNumber        = static_cast<UINT32>(captureRequest.frameNumber);

        camera_metadata_t* pAndroidMetadata = static_cast<camera_metadata_t*>(
            const_cast<VOID*>(captureRequest.pMetadata));

        if (isInputSticky)
        {
            bundle.pInputMetadata = m_pMetadataManager->GetInput(pAndroidMetadata, frameNumber);
        }
        else
        {
            bundle.pInputMetadata = m_pMetadataManager->Get(m_genericMetadataClientId, frameNumber);

            if (NULL != bundle.pInputMetadata)
            {
                bundle.pInputMetadata->SetAndroidMetadata(pAndroidMetadata);
            }
            else
            {
                result = CDKResultEFailed;
                CHX_LOG_ERROR("ERROR: Cannot get input metadata for %u", frameNumber);
            }
        }

        if (CDKResultSuccess == result)
        {
            bundle.pOutputMetadata = m_pMetadataManager->Get(m_genericMetadataClientId, frameNumber);

            if (NULL == bundle.pOutputMetadata)
            {
                result = CDKResultEFailed;

                CHX_LOG_ERROR("ERROR: Cannot get output metadata for %u", frameNumber);
            }
        }

        if ((NULL != bundle.pInputMetadata) && (NULL != bundle.pOutputMetadata))
        {
            m_pMetadataHandleMap[sessionId].insert({ captureRequest.frameNumber, bundle });

            captureRequest.pInputMetadata  = bundle.pInputMetadata->GetHandle();
            captureRequest.pOutputMetadata = bundle.pOutputMetadata->GetHandle();
        }
        else
        {
            result = CDKResultEFailed;

            CHX_LOG_ERROR("[CMB_ERROR] ERROR invalid input %p %p sessionId %d clientId %x",
                bundle.pInputMetadata,
                bundle.pOutputMetadata,
                sessionId,
                m_genericMetadataClientId);
        }
    }
    else
    {
        result = CDKResultEInvalidArg;

        CHX_LOG_ERROR("[CMB_ERROR] ERROR invalid input %p %p sessionId %d clientId %x",
                      captureRequest.pInputMetadata,
                      captureRequest.pOutputMetadata,
                      sessionId,
                      m_genericMetadataClientId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::MetadataCaptureResultGet
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::MetadataCaptureResultGet(
    ChiCaptureResult& captureResult,
    UINT32            sessionId)
{
    CDKResult result           = CDKResultSuccess;
    UINT32    resultFrameIndex = ChxUtils::GetResultFrameIndexChi(captureResult.frameworkFrameNum);
    CHX_ASSERT(MaxSessions > sessionId);

    std::lock_guard<std::mutex> guard(m_MetadataLock);

    if (MaxSessions > sessionId)
    {
        auto bundleIterator = m_pMetadataHandleMap[sessionId].find(captureResult.frameworkFrameNum);

        if (bundleIterator != m_pMetadataHandleMap[sessionId].end())
        {
            ChiMetadataBundle& bundle = bundleIterator->second;

            if (bundle.pOutputMetadata->GetHandle() == captureResult.pOutputMetadata)
            {
                camera_metadata_t* pResultAndMetadata = m_pMetadataManager->GetAndroidFrameworkOutputMetadata();

                if (NULL != pResultAndMetadata)
                {
                    UINT32  partialTagCount = 0;
                    UINT32* pPartialTagList = NULL;
                    // If Partial Meta data has been sent already, then get the list
                    if (FALSE == CheckIfPartialDataCanBeSent(PartialResultSender::DriverPartialData, resultFrameIndex))
                    {
                        partialTagCount = m_pMetadataManager->RetrievePartialTagCount(sessionId);
                        pPartialTagList = m_pMetadataManager->RetrievePartialTags(sessionId);
                        CHX_LOG("Partial Data has been sent and meta data needs to be filtered for tags=%d",
                            partialTagCount);
                    }
                    bundle.pOutputMetadata->TranslateToCameraMetadata(pResultAndMetadata,
                        TRUE,
                        TRUE,
                        partialTagCount,
                        pPartialTagList);
                    captureResult.pResultMetadata = static_cast<CHIMETADATAHANDLE>(pResultAndMetadata);
                }
                else
                {
                    result = CDKResultEFailed;

                    CHX_LOG_ERROR("[CMB_ERROR] ERROR cannot get android meta input %p frameNum%d sessionId %d",
                                  captureResult.pOutputMetadata,
                                  captureResult.frameworkFrameNum,
                                  sessionId);
                }

                m_pMetadataManager->Release(bundle.pInputMetadata);
                m_pMetadataManager->Release(bundle.pOutputMetadata);
            }
            else
            {
                result = CDKResultEFailed;

                CHX_LOG_ERROR("[CMB_ERROR] ERROR metadata handle mismatch input %p %p frameNum%d sessionId %d",
                              captureResult.pOutputMetadata,
                              bundle.pOutputMetadata->GetHandle(),
                              captureResult.frameworkFrameNum,
                              sessionId);
            }
            m_pMetadataHandleMap[sessionId].erase(bundleIterator);
        }
        else
        {
            result = CDKResultEFailed;

            CHX_LOG_ERROR("[CMB_ERROR] ERROR cannot find bundle input %p frameNum%d sessionId %d",
                          captureResult.pOutputMetadata,
                          captureResult.frameworkFrameNum,
                          sessionId);
        }
    }
    else
    {
        result = CDKResultEInvalidArg;

        CHX_LOG_ERROR("[CMB_ERROR] ERROR invalid input %p frameNum %d sessionId %d",
                      captureResult.pResultMetadata,
                      captureResult.frameworkFrameNum,
                      sessionId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::MetadataCaptureResultRelease
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::MetadataCaptureResultRelease(
   const VOID* pAndroidMetadata)
{
    camera_metadata_t* pMetadata = static_cast<camera_metadata_t*>(const_cast<VOID*>(pAndroidMetadata));

    m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(pMetadata);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::GetMetadataBundle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::GetMetadataBundle(
   const camera_metadata_t* pFrameworkInput,
   UINT32                   frameNumber,
   ChiMetadataBundle&       rBundle,
   UINT32                   metadataClientId)
{
    CDKResult          result  = CDKResultEInvalidArg;
    ChiMetadataBundle& bundle  = m_chiMetadataArray[frameNumber%MaxOutstandingRequests];

    CHX_ASSERT(NULL != m_pMetadataManager);

    // check if metadata is provided by the caller
    if (ChiMetadataManager::InvalidClientId == metadataClientId)
    {
        metadataClientId = m_metadataClientId;
    }

    // check if metadata client is registered during initialization
    if (ChiMetadataManager::InvalidClientId != metadataClientId)
    {
        bundle.pOutputMetadata  = m_pMetadataManager->Get(metadataClientId, frameNumber);
    }
    else
    {
        bundle.pOutputMetadata  = ChiMetadata::Create();
    }

    bundle.pInputMetadata   = m_pMetadataManager->GetInput(pFrameworkInput, frameNumber);

    if (bundle.pInputMetadata && bundle.pOutputMetadata)
    {
        rBundle = bundle;
        result  = CDKResultSuccess;
    }
    else
    {
        CHX_LOG_ERROR("[CMB_ERROR] Cannot get metadata %p %p for frame %d",
                      bundle.pInputMetadata,
                      bundle.pOutputMetadata,
                      frameNumber);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::ReleaseMetadataBundle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::ReleaseMetadataBundle(
    INT32 frameNumber)
{
    CDKResult result = CDKResultEInvalidArg;

    CHX_ASSERT(NULL != m_pMetadataManager);

    ChiMetadataBundle& bundle = m_chiMetadataArray[frameNumber%MaxOutstandingRequests];

    if ((NULL != bundle.pInputMetadata) &&
        (NULL != bundle.pOutputMetadata))
    {
        m_pMetadataManager->Release(bundle.pInputMetadata);
        m_pMetadataManager->Release(bundle.pOutputMetadata);
        bundle.pInputMetadata   = NULL;
        bundle.pOutputMetadata  = NULL;
    }
    else
    {
        CHX_LOG_ERROR("[CMB_ERROR] Cannot release metadata %p %p for frame %d",
                      bundle.pInputMetadata,
                      bundle.pOutputMetadata,
                      frameNumber);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::UpdateAppPartialResultMetadataFromDriver
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::UpdateAppPartialResultMetadataFromDriver(
    ChiMetadata* pChiMetadata,
    UINT         resultId,
    UINT32       resultFrameNumber,
    UINT32       clientId)
{
    CDKResult          result = CDKResultSuccess;

    if (TRUE == m_requestFlags[resultId].isInErrorState)
    {
        result = CDKResultEInvalidState;
    }
    else if (NULL != pChiMetadata)
    {
        CHX_LOG("PMD is being created for resultID=%d resultFrameNumber:%d",
            resultId,
            resultFrameNumber);
        camera_metadata_t* pFrameworkOutput = m_pMetadataManager->GetAndroidFrameworkOutputMetadata();

        if (NULL != pFrameworkOutput)
        {
            pChiMetadata->TranslateToCameraPartialMetadata(pFrameworkOutput,
                m_pMetadataManager->RetrievePartialTags(clientId),
                m_pMetadataManager->RetrievePartialTagCount(clientId));
            m_driverPartialCaptureResult[resultId].result               = pFrameworkOutput;
            m_driverPartialCaptureResult[resultId].frame_number         = resultFrameNumber;
            m_driverPartialCaptureResult[resultId].input_buffer         = NULL;
            m_driverPartialCaptureResult[resultId].output_buffers       = NULL;
            m_driverPartialCaptureResult[resultId].num_output_buffers   = 0;
            m_driverPartialCaptureResult[resultId].partial_result       =
                static_cast<int>(ChxUtils::GetPartialResultCount(
                    PartialResultSender::DriverPartialData));
        }
        else
        {
            result = CDKResultEFailed;
            CHX_LOG_ERROR("Failed to get framework partial metadata %p for result: %u",
                pChiMetadata, resultId);
        }
    }
    else
    {
        result = CDKResultENoMemory;
        CHX_LOG_ERROR("Invalid chi metadata %p for result: %u",
            pChiMetadata, resultId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::UpdateAppResultMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::UpdateAppResultMetadata(
    ChiMetadata* pChiMetadata,
    UINT         resultId,
    UINT32       clientId)
{
    CDKResult          result = CDKResultSuccess;

    if (NULL != pChiMetadata)
    {
        camera_metadata_t* pFrameworkOutput = m_pMetadataManager->GetAndroidFrameworkOutputMetadata();

        if (NULL != pFrameworkOutput)
        {
            UINT32  partialTagCount = 0;
            UINT32* pPartialTagList = NULL;
            // If Partial Meta data has been sent already, then get the list
            if (FALSE == CheckIfPartialDataCanBeSent(PartialResultSender::DriverPartialData, resultId))
            {
                partialTagCount = m_pMetadataManager->RetrievePartialTagCount(clientId);
                pPartialTagList = m_pMetadataManager->RetrievePartialTags(clientId);
                CHX_LOG("Partial Data has been sent and meta data needs to be filtered for tags=%d",
                    partialTagCount);
            }
            pChiMetadata->TranslateToCameraMetadata(pFrameworkOutput,
                TRUE,
                !m_isReprocessUsecase,
                partialTagCount,
                pPartialTagList);
            m_captureResult[resultId].result = pFrameworkOutput;
        }
        else
        {
            result = CDKResultEFailed;

            CHX_LOG_ERROR("Failed to get framework metadata %p for result: %u",
                pChiMetadata, resultId);
        }
    }
    else
    {
        result = CDKResultENoMemory;

        CHX_LOG_ERROR("Invalid chi metadata %p for result: %u", pChiMetadata, resultId);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::DeleteAllPendingResults
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::DeleteAllPendingResults()
{
    m_pAppResultMutex->Lock();
    // Cancel all outstanding requests
    for (UINT i = 0; i < MaxOutstandingRequests; i++)
    {
        m_pMapLock->Lock();
        camera3_capture_request_t* pRequest = &m_pendingPCRs[i];
        camera3_capture_result_t*  pResult = &m_captureResult[i];
        m_pMapLock->Unlock();

        CHX_LOG("Pending result CHI frame: %d, Framework Frame: %d, Num Buffers: %d, IsInErrorState: %d",
                pRequest->frame_number,
                pResult->frame_number,
                pRequest->num_output_buffers,
                m_requestFlags[i].isInErrorState);

        const BOOL& hasSentMetadata = m_requestFlags[i].isOutputMetaDataSent;
        if ((m_requestFlags[i].isInErrorState == FALSE) &&
            ((0 != m_numberOfPendingOutputBuffers[i]) || (FALSE == hasSentMetadata)))
        {
            CHX_LOG_INFO("Deleting pending result - ChiOverrideFrameNum: %d", pRequest->frame_number);
            HandleProcessRequestError(pRequest);
        }
    }
    m_pAppResultMutex->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::SubmitRequestMC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::SubmitRequestMC(CHIPIPELINEREQUEST* pSubmitRequestData)
{
    CDKResult result          = CDKResultSuccess;

    if (TRUE == IsUsecaseInBadState())
    {
       CHX_LOG_ERROR("Request timed out returning");
       return CDKResultECancelledRequest;
    }

    if (IsFlushing != GetFlushStatus())
    {
        result = ExtensionModule::GetInstance()->SubmitRequest(pSubmitRequestData);
    }
    else
    {
        CHX_LOG_WARN("Request: %" PRIu64 " has been dropped as flush is in progress",
            pSubmitRequestData->pCaptureRequests->frameNumber);
        result = CDKResultECancelledRequest;
    }

    if (result == CDKResultETimeout)
    {
        CHX_LOG_ERROR("Usecase:%d cameraId:%d is in bad state.", GetUsecaseId(), GetCameraId());
        SetUsecaseInBadState(TRUE);
        //signal recovery.
        ExtensionModule::GetInstance()->SignalRecoveryCondition(m_cameraId);
        result = CDKResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::SubmitRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Usecase::SubmitRequest(CHIPIPELINEREQUEST* pSubmitRequestData)
{
    CDKResult result          = CDKResultSuccess;

    if (TRUE == IsUsecaseInBadState())
    {
       CHX_LOG_ERROR("Request timed out returning");
       return CDKResultETimeout;
    }

    if (IsFlushing != GetFlushStatus())
    {
        result = ExtensionModule::GetInstance()->SubmitRequest(pSubmitRequestData);

        if (CDKResultECancelledRequest == result)
        {
            CHX_LOG_WARN("Session returned that flush was in progress. Rewriting result as success.");
            result = CDKResultSuccess;
        }
    }
    else
    {
        CHX_LOG_WARN("Request: %" PRIu64 " has been dropped as flush is in progress",
            pSubmitRequestData->pCaptureRequests->frameNumber);
        result = CDKResultECancelledRequest;
    }

    if (CDKResultETimeout == result)
    {
        CHX_LOG_ERROR("Usecase:%d cameraId:%d is in bad state.", GetUsecaseId(), GetCameraId());
        SetUsecaseInBadState(TRUE);
        //signal recovery.
        ExtensionModule::GetInstance()->SignalRecoveryCondition(m_cameraId);
        result = CDKResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase::PrepareForRecovery
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usecase::PrepareForRecovery()
{
    CHX_LOG_ERROR("Usecase:%d cameraId:%d is in bad state.", GetUsecaseId(), GetCameraId());
    SetUsecaseInBadState(TRUE);
    if (FALSE == m_isUsecaseDestroy)
    {
        DeleteAllPendingResults();
    }
}
