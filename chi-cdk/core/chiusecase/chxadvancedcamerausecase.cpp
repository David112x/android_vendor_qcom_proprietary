////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxadvancedcamerausecase.cpp
/// @brief Usecases class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <assert.h>

#include "chistatspropertydefines.h"
#include "chifeature2wrapper.h"

#include "chxadvancedcamerausecase.h"
#include "chxdefs.h"
#include "chxincs.h"
#include "chxpipeline.h"
#include "chxsession.h"
#include "chxusecase.h"

#include "g_pipelines.h"

#include "system/camera_metadata.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

extern CHICONTEXTOPS g_chiContextOps;

static const UINT32 TIMEOUT_BUFFER_WAIT          =  600;
/// @todo (CAMX-4193) Handle error and thread termination.
static const UINT32 DEFER_OFFLINE_THREAD_TIMEOUT = 1500;
static const CHAR*  ZSL_USECASE_NAME             = "UsecaseZSL";

inline BOOL IsThisClonedStream(ChiStream** ppClonedStream, ChiStream* pActualStream, UINT *pIndex)
{
    BOOL bIsCloned = FALSE;
    for (UINT i = 0; i < MaxChiStreams; i++)
    {
        if (ppClonedStream[i] == pActualStream)
        {
            bIsCloned = TRUE;
            *pIndex   = i;
            break;
        }
    }
    return bIsCloned;
}

ChiUsecase* pAdvancedUsecase  = NULL;
ChiUsecase* pZslTuningUsecase = g_pUsecaseZSLTuning;

const INT32 AdvancedPipelineIdMapping[] =
{
    UsecaseZSLPipelineIds::ZSLSnapshotJpeg,             // ZSLSnapshotJpegType = 0
    UsecaseZSLPipelineIds::ZSLSnapshotYUV,              // ZSLSnapshotYUVType  = 1
    UsecaseZSLPipelineIds::InternalZSLYuv2Jpeg,         // InternalZSLYuv2JpegType         // YUV to JPEG
    UsecaseZSLPipelineIds::InternalZSLYuv2JpegMFNR,     // InternalZSLYuv2JpegMFNRType     // YUV to JPEG for MFNR
    UsecaseZSLPipelineIds::Merge3YuvCustomTo1Yuv,       // Merge3YuvCustomTo1YuvType       // merge 3 YUV to 1 YUV
    UsecaseZSLPipelineIds::ZSLPreviewRaw,               // ZSLPreviewRawType
    UsecaseZSLPipelineIds::ZSLPreviewRawFS,             // ZSLPreviewRawFSType
    UsecaseZSLPipelineIds::ZSLPreviewRawYUV,            // ZSLPreviewRawYUVType
    UsecaseZSLPipelineIds::MfnrPrefilter,               // MFNRPrefilterType               // MFNR prefilter
    UsecaseZSLPipelineIds::MfnrBlend,                   // MFNRBlendType                   // MFNR blend
    UsecaseZSLPipelineIds::MfnrPostFilter,              // MFNRPostFilterType              // MFNR post filter
    UsecaseZSLPipelineIds::SWMFMergeYuv,                // SWMFMergeYuvType                // SWMF merge pipeline
    UsecaseZSLPipelineIds::SWMFMergeRaw,                // SWMFMergeRawType                // SWMF merge pipeline
    UsecaseZSLPipelineIds::ZSLSnapshotYUVAux,           // ZSLSnapshotYUVAuxType           // BPS + IPE for Aux
    UsecaseZSLPipelineIds::InternalZSLYuv2JpegMFNRAux,  // InternalZSLYuv2JpegMFNRTypeAux  // YUV to JPEG for MFNR
    UsecaseZSLPipelineIds::MfnrPrefilterAux,            // MFNRPrefilterAuxType            // MFNR prefilter Aux
    UsecaseZSLPipelineIds::MfnrBlendAux,                // MFNRBlendAuxType                // MFNR blend Aux
    UsecaseZSLPipelineIds::MfnrPostFilterAux,           // MFNRPostFilterAuxType           // MFNR post filter Aux
    UsecaseZSLPipelineIds::ZSLYuv2Yuv,                  // YUV to YUV reprocess
    UsecaseZSLPipelineIds::ZSLSnapshotJpegGPU,          // ZSLSnapshotJpegGPUType
    UsecaseZSLPipelineIds::InternalZSLYuv2JpegMFSR,     // InternalZSLYuv2JpegMFSRType     // YUV to JPEG for MFSR
    UsecaseZSLPipelineIds::MfsrPrefilter,               // MFSRPrefilterType               // MFSR prefilter
    UsecaseZSLPipelineIds::MfsrBlend,                   // MFSRBlendType                   // MFSR blend
    UsecaseZSLPipelineIds::MfsrPostFilter,              // MFSRPostFilterType              // MFSR post filter
    UsecaseZSLPipelineIds::InternalZSLYuv2JpegMFSRAux,  // InternalZSLYuv2JpegMFSRTypeAux  // YUV to JPEG for MFSR
    UsecaseZSLPipelineIds::MfsrPrefilterAux,            // MFSRPrefilterAuxType            // MFSR prefilter Aux
    UsecaseZSLPipelineIds::MfsrBlendAux,                // MFSRBlendAuxType                // MFSR blend Aux
    UsecaseZSLPipelineIds::MfsrPostFilterAux,           // MFSRPostFilterAuxType           // MFSR post filter Aux
    UsecaseZSLPipelineIds::QuadCFAFullSizeRaw,          // QuadCFAFullSizeRawType          // QuadCFA full size raw
    UsecaseZSLPipelineIds::QuadCFARemosaic,             // QuadCFARemosaicType             // QuadCFA sw remosaic
    UsecaseZSLPipelineIds::QuadCFASnapshotYuv,          // QuadCFASnapshotYuvType,        // QuadCFA Yuv snapshot
    UsecaseZSLPipelineIds::OfflineNoiseReprocess,       // OfflineNoiseReprocess           // Offline Noise Reprocess
    UsecaseZSLPipelineIds::OfflineNoiseReprocessAux,    // OfflineNoiseReprocess           // Offline Noise Reprocess Aux
    UsecaseZSLPipelineIds::ZSLSnapshotYUVHAL            // ZSLSnapshotYUVHALType           // SnapshotYUV pipeline for HAL
};

const INT32 AdvancedPipelineIdTuningMapping[] =
{
    UsecaseZSLTuningPipelineIds::ZSLSnapshotJpegTuning, // ZSLSnapshotJpegType = 0
    UsecaseZSLTuningPipelineIds::ZSLSnapshotYUVTuning,  // ZSLSnapshotYUVType  = 1
    -1,                                                 // InternalZSLYuv2JpegType         // YUV to JPEG
    -1,                                                 // Merge3YuvCustomTo1YuvType       // merge 3 YUV to 1 YUV
    UsecaseZSLTuningPipelineIds::ZSLPreviewRawTuning    // ZSLPreviewRawType
};

// Update the AdvancedPipelineIdMapping if the AdvancedPipelineType is changed
CHX_STATIC_ASSERT((sizeof(AdvancedPipelineIdMapping) / sizeof(AdvancedPipelineIdMapping[0]) ==
    AdvancedPipelineType::PipelineCount));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Class CameraUsecaseBaseBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::~CameraUsecaseBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CameraUsecaseBase::~CameraUsecaseBase()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::DestroyDeferResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::DestroyDeferResource()
{
    CDKResult result = CDKResultSuccess;

    WaitForDeferThread();

    if (NULL != m_pDeferOfflineDoneCondition)
    {
        m_pDeferOfflineDoneCondition->Destroy();
        m_pDeferOfflineDoneCondition = NULL;
    }

    if (NULL != m_pDeferOfflineDoneMutex)
    {
        m_pDeferOfflineDoneMutex->Destroy();
        m_pDeferOfflineDoneMutex = NULL;
    }

    m_deferOfflineSessionDone      = FALSE;
    m_deferOfflineThreadCreateDone = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::Destroy(
    BOOL isForced)
{
    ATRACE_BEGIN("CameraUsecaseBase::Destroy");

    DestroyDeferResource();

    for (UINT session = 0; session < MaxSessions; session++)
    {
        SessionData* pData = &m_sessions[session];

        if (NULL != pData->pSession)
        {
            pData->pSession->Destroy(isForced);
            pData->pSession = NULL;
        }

        for (UINT pipeline = 0; pipeline < pData->numPipelines; pipeline++)
        {
            for (UINT stream = 0; stream < pData->pipelines[pipeline].numStreams; stream++)
            {
                // Anything we need to delete with streams?
                pData->pipelines[pipeline].pStreams[stream] = NULL;
            }

            if (NULL != pData->pipelines[pipeline].pPipeline)
            {
                pData->pipelines[pipeline].pPipeline->Destroy();
                pData->pipelines[pipeline].pPipeline = NULL;
            }

            m_pMetadataManager->UnregisterClient(m_metadataClients[pData->pipelines[pipeline].id]);
            m_metadataClients[pData->pipelines[pipeline].id] = ChiMetadataManager::InvalidClientId;
        }
    }

    for (UINT i = 0; i < MaxChiStreams; i++)
    {
        if (NULL != m_pClonedStream[i])
        {
            CHX_FREE(m_pClonedStream[i]);
        }
    }

    if (NULL != m_pPipelineToCamera)
    {
        CHX_FREE(m_pPipelineToCamera);
        m_pPipelineToCamera = NULL;
    }

    if (NULL != m_pPipelineToSession)
    {
        CHX_FREE(m_pPipelineToSession);
        m_pPipelineToSession = NULL;
    }

    if (NULL != m_pEmptyMetaData)
    {
        ChxUtils::AndroidMetadata::FreeMetaData(m_pEmptyMetaData);
        m_pEmptyMetaData = NULL;
    }

    if (NULL != m_pChiUsecase)
    {
        // For Multicamera Usecase, m_pChiUsecase->numTargets is greater than the MaxNumOfTargets allowed
        UINT numTargets = (TRUE == IsMultiCameraUsecase() ? MaxNumOfTargets : m_pChiUsecase->numTargets);

        for (UINT target = 0; target < numTargets; target++)
        {
            for (UINT queueIndex = 0; queueIndex < RDIBufferQueueDepth; queueIndex++)
            {
                if (NULL != m_targetBuffers[target].bufferQueue[queueIndex].pRdiOutputBuffer)
                {
                    if (NULL != m_targetBuffers[target].bufferQueue[queueIndex].pRdiOutputBuffer->bufferInfo.phBuffer)
                    {
                        m_targetBuffers[target].pBufferManager->ReleaseReference(
                            &(m_targetBuffers[target].bufferQueue[queueIndex].pRdiOutputBuffer->bufferInfo));
                    }

                    CHX_FREE(m_targetBuffers[target].bufferQueue[queueIndex].pRdiOutputBuffer);
                    m_targetBuffers[target].bufferQueue[queueIndex].pRdiOutputBuffer = NULL;
                }
                if (NULL != m_targetBuffers[target].bufferQueue[queueIndex].pMetadata)
                {
                    m_targetBuffers[target].bufferQueue[queueIndex].pMetadata = NULL;
                }
            }

            if (NULL != m_targetBuffers[target].pBufferManager)
            {
                m_targetBuffers[target].pBufferManager->Destroy();
                m_targetBuffers[target].pBufferManager = NULL;
            }

            if (NULL != m_targetBuffers[target].pMutex)
            {
                m_targetBuffers[target].pMutex->Destroy();
                m_targetBuffers[target].pMutex = NULL;
            }

            if (NULL != m_targetBuffers[target].pCondition)
            {
                m_targetBuffers[target].pCondition->Destroy();
                m_targetBuffers[target].pCondition = NULL;
            }
        }
    }

    ATRACE_END();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::DeferOfflineSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::DeferOfflineSession()
{
    ATRACE_BEGIN("DeferOfflineSession");
    CDKResult result            = CDKResultSuccess;

    m_pDeferOfflineDoneMutex->Lock();
    m_deferOfflineSessionDone = FALSE;
    m_pDeferOfflineDoneMutex->Unlock();

    result = CreateOfflineSessions(m_pCallBacks);

    m_pDeferOfflineDoneMutex->Lock();

    if(CDKResultSuccess == result)
    {
        m_deferOfflineSessionDone = TRUE;
    }
    else
    {
        CHX_LOG_ERROR("Failed to create session");
    }
    m_pDeferOfflineDoneCondition->Signal();

    m_pDeferOfflineDoneMutex->Unlock();

    ATRACE_END();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::DeferOfflineSessionThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CameraUsecaseBase::DeferOfflineSessionThread(VOID * pThreadData)
{
    CDKResult          result             = CDKResultSuccess;
    PerThreadData*     pPerThreadData     = reinterpret_cast<PerThreadData*>(pThreadData);
    CameraUsecaseBase* pCameraUsecaseBase = reinterpret_cast<CameraUsecaseBase*>(pPerThreadData->pPrivateData);

    result = pCameraUsecaseBase->DeferOfflineSession();
    CHX_ASSERT(CDKResultSuccess == result);

    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("Destroyed defer thread");
        pthread_exit(NULL);
    }

    return NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::StartDeferThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::StartDeferThread()
{
    CDKResult result       = CDKResultSuccess;
    INT       threadStatus = 0;

    if (FALSE == m_deferOfflineThreadCreateDone)
    {
        m_deferOfflineSessionThreadData.pPrivateData = this;
        CHX_LOG("%s", __FUNCTION__);

        result = ChxUtils::ThreadCreate(CameraUsecaseBase::DeferOfflineSessionThread,
                                        &m_deferOfflineSessionThreadData,
                                        &m_deferOfflineSessionThreadData.hThreadHandle);
        CHX_ASSERT(CDKResultSuccess == result);

        if (CDKResultSuccess == result)
        {
            threadStatus = pthread_detach(m_deferOfflineSessionThreadData.hThreadHandle);
            CHX_ASSERT(0 == threadStatus);

            if (0 == threadStatus)
            {
                m_pDeferOfflineDoneMutex->Lock();
                m_deferOfflineThreadCreateDone = TRUE;
                m_pDeferOfflineDoneMutex->Unlock();

                CHX_LOG_INFO("Create defer thread successful");
            }
            else
            {
                ChxUtils::ThreadTerminate(m_deferOfflineSessionThreadData.hThreadHandle);

                CHX_LOG_ERROR("Detach defer thread failed!");
                result = CDKResultEFailed;
            }
        }
        else
        {
            m_deferOfflineSessionThreadData.pPrivateData  = NULL;
            m_deferOfflineSessionThreadData.hThreadHandle = INVALID_THREAD_HANDLE;
            CHX_LOG_ERROR("Create defer thread failed!");
        }
    }
    else
    {
        CHX_LOG_WARN("Warning: Advanced offline related session has already been created!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::WaitForDeferThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::WaitForDeferThread()
{
    CDKResult result = CDKResultSuccess;

    if (TRUE == m_deferOfflineThreadCreateDone)
    {
        m_pDeferOfflineDoneMutex->Lock();

        if (FALSE == m_deferOfflineSessionDone)
        {
            result = m_pDeferOfflineDoneCondition->TimedWait(m_pDeferOfflineDoneMutex->GetNativeHandle(),
                                                             DEFER_OFFLINE_THREAD_TIMEOUT);
            if (CDKResultSuccess == result)
            {
                if (FALSE == m_deferOfflineSessionDone)
                {
                    result = CDKResultEFailed;
                }
            }
            else
            {
                CHX_LOG_ERROR("Timed Out!");
            }
        }

        m_pDeferOfflineDoneMutex->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::GetInternalTargetBufferIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CameraUsecaseBase::GetInternalTargetBufferIndex(
    ChiStream* pChiStream)
{
    UINT32 targetIndex = InvalidId;

    if ((NULL != pChiStream) && (m_numOfInternalTargetBuffers <= MaxNumOfTargets))
    {
        for (UINT32 i = 0 ; i < m_numOfInternalTargetBuffers; i++)
        {
            if (pChiStream == m_targetBuffers[i].pChiStream)
            {
                targetIndex = i;
                break;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid stream pointer!");
    }
    return targetIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::GenerateInternalBufferIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::GenerateInternalBufferIndex()
{
    ChiPipelineTargetCreateDescriptor* pDesc    = NULL;
    ChiTarget*                         pTarget  = NULL;
    UINT32                             targetId = 0;

    for (UINT32 descIndex = 0; descIndex < m_pChiUsecase->numPipelines; descIndex++)
    {
        pDesc = &(m_pChiUsecase->pPipelineTargetCreateDesc[descIndex]);

        for (UINT32 targetIndex = 0; targetIndex < pDesc->sinkTarget.numTargets; targetIndex++)
        {
            pTarget = (pDesc->sinkTarget.pTargetPortDesc[targetIndex].pTarget);

            if (NULL != pTarget)
            {
                if (TRUE == StreamIsInternal(pTarget->pChiStream))
                {
                    m_targetBuffers[targetId].pChiStream          = pTarget->pChiStream;

                    // Mark the internal stream index to easy identify which one is RDI or FD
                    if ((ChiFormatRawMIPI == pTarget->pBufferFormats[0]) &&
                        (InvalidId == m_rdiStreamIndex))
                    {
                        m_rdiStreamIndex = targetId;
                        CHX_LOG_CONFIG("m_rdiStreamIndex:%d", m_rdiStreamIndex);
                    }
                    else if ((ChiFormatYUV420NV12 == pTarget->pBufferFormats[0]) &&
                        (InvalidId == m_rdiStreamIndex))
                    {
                        m_fdStreamIndex  = targetId;
                        CHX_LOG_CONFIG("m_fdStreamIndex:%d", m_fdStreamIndex);
                    }

                    targetId++;
                }
            }
            else
            {
                CHX_LOG_ERROR("There is something wrong with target port descriptor:%s:%s!",
                    pDesc->pPipelineName,
                    pDesc->sinkTarget.pTargetPortDesc[targetIndex].pTargetName);
            }
        }
    }
    m_numOfInternalTargetBuffers = targetId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::CreateInternalBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::CreateInternalBufferManager()
{
    ChiPipelineTargetCreateDescriptor* pDesc    = NULL;
    ChiTarget*                         pTarget  = NULL;
    UINT32                             targetId = 0;

    for (UINT32 descIndex = 0; descIndex < m_pChiUsecase->numPipelines; descIndex++)
    {
        pDesc = &(m_pChiUsecase->pPipelineTargetCreateDesc[descIndex]);

        for (UINT32 targetIndex = 0; targetIndex < pDesc->sinkTarget.numTargets; targetIndex++)
        {

            pTarget = (pDesc->sinkTarget.pTargetPortDesc[targetIndex].pTarget);

            if (NULL != pTarget)
            {
                if (TRUE == StreamIsInternal(pTarget->pChiStream))
                {
                    CHX_LOG("%s, %s,Create CHI buffers for targets [%d/%d], chistream:%p, format:%d, %dx%d",
                        pDesc->pPipelineName,
                        pDesc->sinkTarget.pTargetPortDesc[targetIndex].pTargetName,
                        targetIndex,
                        pDesc->sinkTarget.numTargets,
                        pTarget->pChiStream,
                        pTarget->pChiStream->format,
                        pTarget->pChiStream->width,
                        pTarget->pChiStream->height);

                    CHIBufferManagerCreateData createData = { 0 };

                    createData.width                = pTarget->pChiStream->width;
                    createData.height               = pTarget->pChiStream->height;
                    createData.format               = pTarget->pChiStream->format;
                    createData.producerFlags        = GRALLOC1_PRODUCER_USAGE_CAMERA;
                    createData.consumerFlags        = GRALLOC1_CONSUMER_USAGE_CAMERA;
                    createData.maxBufferCount       = BufferQueueDepth;
                    createData.immediateBufferCount = CHIImmediateBufferCountZSL;
                    createData.bEnableLateBinding   = ExtensionModule::GetInstance()->EnableCHILateBinding();
                    createData.bufferHeap           = BufferHeapDefault;
                    createData.pChiStream           = pTarget->pChiStream;

                    if (TRUE == IsFdStream(pTarget->pChiStream))
                    {
                        // set CPU access flags as CHI override access the buffer through CPU for Anchor picking algorithm
                        createData.producerFlags |= GRALLOC1_PRODUCER_USAGE_CPU_WRITE;
                        // Do not set CONSUMER CAMERA Flag for EGL Buffers. Default Gralloc Format NV12 is used.
                        // NV12 has different alignments in different targets. By not setting CONSUMER CAMERA flag, sufficient
                        // memory will be allocated
                        createData.consumerFlags  = GRALLOC1_PRODUCER_USAGE_CPU_READ;

                        // Lets use Gralloc heap for this. Even though we dont really need gralloc, set this to make sure
                        // Gralloc heap path is being executed.
                        createData.bufferHeap     = BufferHeapEGL;
                    }

                    if (TRUE == IsRDIStream(pTarget->pChiStream))
                    {
                        // set CPU access flags as AF access the buffer through CPU for LCR algorithm
                        createData.consumerFlags |= GRALLOC1_PRODUCER_USAGE_CPU_READ;
                    }

                    CHAR bufferManagerName[MaxStringLength64];
                    CdkUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "%s_%s",
                        pDesc->pPipelineName,
                        pDesc->sinkTarget.pTargetPortDesc[targetIndex].pTargetName);
                    m_targetBuffers[targetId].pBufferManager      = CHIBufferManager::Create(bufferManagerName, &createData);
                    m_targetBuffers[targetId].pMutex              = Mutex::Create();
                    m_targetBuffers[targetId].pCondition          = Condition::Create();
                    m_targetBuffers[targetId].lastReadySequenceID = INVALIDSEQUENCEID;
                    m_targetBuffers[targetId].validBufferLength   = 0;
                    m_targetBuffers[targetId].pChiStream          = pTarget->pChiStream;
                    for (UINT queueIndex = 0; queueIndex < RDIBufferQueueDepth; queueIndex++)
                    {
                        CHISTREAMBUFFER* pRdiOutputBuffer =
                            static_cast<CHISTREAMBUFFER*>(CHX_CALLOC(sizeof(CHISTREAMBUFFER)));

                        m_targetBuffers[targetId].bufferQueue[queueIndex].pRdiOutputBuffer = pRdiOutputBuffer;
                        m_targetBuffers[targetId].bufferQueue[queueIndex].frameNumber      = INVALIDSEQUENCEID;

                        if (NULL != pRdiOutputBuffer)
                        {
                            pRdiOutputBuffer->size                  = sizeof(CHISTREAMBUFFER);
                            pRdiOutputBuffer->acquireFence.valid    = FALSE;
                            pRdiOutputBuffer->bufferInfo.phBuffer   = NULL;
                            pRdiOutputBuffer->pStream               = pTarget->pChiStream;
                        }
                    }

                    targetId++;
                }
            }
            else
            {
                CHX_LOG_ERROR("There is something wrong with target port descriptor:%s:%s!",
                    pDesc->pPipelineName,
                    pDesc->sinkTarget.pTargetPortDesc[targetIndex].pTargetName);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::Initialize(
    ChiCallBacks*                   pCallbacks,
    camera3_stream_configuration_t* pStreamConfig)
{
    ATRACE_BEGIN("CameraUsecaseBase::Initialize");

    CDKResult result               = Usecase::Initialize(false);
    BOOL      bReprocessUsecase    = FALSE;

    m_lastResultMetadataFrameNum   = -1;
    m_effectModeValue              = ANDROID_CONTROL_EFFECT_MODE_OFF;
    m_sceneModeValue               = ANDROID_CONTROL_SCENE_MODE_DISABLED;
    m_rtSessionIndex               = InvalidId;

    m_finalPipelineIDForPartialMetaData = InvalidId;

    m_deferOfflineThreadCreateDone = FALSE;
    m_pDeferOfflineDoneMutex       = Mutex::Create();
    m_pDeferOfflineDoneCondition   = Condition::Create();
    m_deferOfflineSessionDone      = FALSE;
    m_pCallBacks                   = pCallbacks;
    m_GpuNodePresence              = FALSE;
    m_debugLastResultFrameNumber   = static_cast<UINT32>(-1);
    m_pEmptyMetaData               = ChxUtils::AndroidMetadata::AllocateMetaData(0,0);
    m_rdiStreamIndex               = InvalidId;
    m_fdStreamIndex                = InvalidId;
    m_isRequestBatchingOn          = false;
    m_batchRequestStartIndex       = UINT32_MAX;
    m_batchRequestEndIndex         = UINT32_MAX;

    ChxUtils::Memset(&m_sessions[0], 0, sizeof(m_sessions));

    // Default to 1-1 mapping of sessions and pipelines
    if (0 == m_numSessions)
    {
        m_numSessions = m_pChiUsecase->numPipelines;
    }

    CHX_ASSERT(0 != m_numSessions);

    if (CDKResultSuccess == result)
    {
        ChxUtils::Memset(m_pClonedStream, 0, (sizeof(ChiStream*)*MaxChiStreams));
        ChxUtils::Memset(m_pFrameworkOutStreams, 0, (sizeof(ChiStream*)*MaxChiStreams));
        m_bCloningNeeded         = FALSE;
        m_numberOfOfflineStreams = 0;

        for (UINT i = 0; i < m_pChiUsecase->numPipelines; i++)
        {
            if (m_pChiUsecase->pPipelineTargetCreateDesc[i].sourceTarget.numTargets > 0)
            {
                bReprocessUsecase = TRUE;
                break;
            }
        }

        for (UINT i = 0; i < m_pChiUsecase->numPipelines; i++)
        {
            if (TRUE == m_pChiUsecase->pPipelineTargetCreateDesc[i].pipelineCreateDesc.isRealTime)
            {
                // Cloning of streams needs when source target stream is enabled and
                // all the streams are connected in both real time and offline pipelines
                // excluding the input stream count
                m_bCloningNeeded = bReprocessUsecase && (UsecaseId::PreviewZSL != m_usecaseId) &&
                    (m_pChiUsecase->pPipelineTargetCreateDesc[i].sinkTarget.numTargets == (m_pChiUsecase->numTargets - 1));
                if (TRUE == m_bCloningNeeded)
                {
                    break;
                }
            }
        }
        CHX_LOG("m_bCloningNeeded = %d", m_bCloningNeeded);
        // here just generate internal buffer index which will be used for feature to related target buffer
        GenerateInternalBufferIndex() ;

        for (UINT i = 0; i < m_pChiUsecase->numPipelines; i++)
        {
            // use mapping if available, otherwise default to 1-1 mapping
            UINT sessionId  = (NULL != m_pPipelineToSession) ? m_pPipelineToSession[i] : i;
            UINT pipelineId = m_sessions[sessionId].numPipelines++;

            // Assign the ID to pipelineID
            m_sessions[sessionId].pipelines[pipelineId].id = i;

            CHX_LOG("Creating Pipeline %s at index %u for session %u, session's pipeline %u, camera id:%d",
                m_pChiUsecase->pPipelineTargetCreateDesc[i].pPipelineName, i, sessionId, pipelineId, m_pPipelineToCamera[i]);

            result = CreatePipeline(m_pPipelineToCamera[i],
                                    &m_pChiUsecase->pPipelineTargetCreateDesc[i],
                                    &m_sessions[sessionId].pipelines[pipelineId],
                                    pStreamConfig);

            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Failed to Create Pipeline %s at index %u for session %u, session's pipeline %u, camera id:%d",
                    m_pChiUsecase->pPipelineTargetCreateDesc[i].pPipelineName, i, sessionId, pipelineId, m_pPipelineToCamera[i]);
                break;
            }

            m_sessions[sessionId].pipelines[pipelineId].isHALInputStream = PipelineHasHALInputStream(&m_pChiUsecase->pPipelineTargetCreateDesc[i]);

            if (FALSE == m_GpuNodePresence)
            {
                for (UINT nodeIndex = 0;
                        nodeIndex < m_pChiUsecase->pPipelineTargetCreateDesc[i].pipelineCreateDesc.numNodes; nodeIndex++)
                {
                    UINT32 nodeIndexId =
                                m_pChiUsecase->pPipelineTargetCreateDesc[i].pipelineCreateDesc.pNodes->nodeId;
                    if (255 == nodeIndexId)
                    {
                        if (NULL != m_pChiUsecase->pPipelineTargetCreateDesc[i].pipelineCreateDesc.pNodes->pNodeProperties)
                        {
                            const CHAR* gpuNodePropertyValue = "com.qti.node.gpu";
                            const CHAR* nodePropertyValue = (const CHAR*)
                                m_pChiUsecase->pPipelineTargetCreateDesc[i].pipelineCreateDesc.pNodes->pNodeProperties->pValue;
                            if (!strcmp(gpuNodePropertyValue, nodePropertyValue))
                            {
                                m_GpuNodePresence = TRUE;
                                break;
                            }
                        }
                    }
                }
            }

            PipelineCreated(sessionId, pipelineId);

        }
        if (CDKResultSuccess == result)
        {
            //create internal buffer
            CreateInternalBufferManager();

            //If Session's Pipeline has HAL input stream port,
            //create it on main thread to return important Stream
            //information during configure_stream call.
            result = CreateSessionsWithInputHALStream(pCallbacks);
        }

        if (CDKResultSuccess == result)
        {
            result = StartDeferThread();
        }

        if (CDKResultSuccess == result)
        {
            result = CreateRTSessions(pCallbacks);
        }

        if (CDKResultSuccess == result)
        {
            INT32 frameworkBufferCount = BufferQueueDepth;

            for (UINT32 sessionIndex = 0; sessionIndex < m_numSessions; ++sessionIndex)
            {
                PipelineData* pPipelineData = m_sessions[sessionIndex].pipelines;

                for (UINT32 pipelineIndex = 0; pipelineIndex < m_sessions[sessionIndex].numPipelines; pipelineIndex++)
                {
                    Pipeline* pPipeline = pPipelineData[pipelineIndex].pPipeline;
                    if (TRUE == pPipeline->IsRealTime())
                    {
                        m_metadataClients[pPipelineData[pipelineIndex].id] =
                             m_pMetadataManager->RegisterClient(
                                pPipeline->IsRealTime(),
                                pPipeline->GetTagList(),
                                pPipeline->GetTagCount(),
                                pPipeline->GetPartialTagCount(),
                                pPipeline->GetMetadataBufferCount() + BufferQueueDepth,
                                ChiMetadataUsage::RealtimeOutput);

                        pPipelineData[pipelineIndex].pPipeline->SetMetadataClientId(
                            m_metadataClients[pPipelineData[pipelineIndex].id]);

                        // update tag filters
                        PrepareHFRTagFilterList(pPipelineData[pipelineIndex].pPipeline->GetMetadataClientId());
                        frameworkBufferCount += pPipeline->GetMetadataBufferCount();
                    }
                    ChiMetadata* pMetadata = pPipeline->GetDescriptorMetadata();
                    result = pMetadata->SetTag("com.qti.chi.logicalcamerainfo", "NumPhysicalCameras", &m_numOfPhysicalDevices,
                        sizeof(m_numOfPhysicalDevices));
                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_ERROR("Failed to set metadata tag NumPhysicalCameras");
                    }
                }
            }

            m_pMetadataManager->InitializeFrameworkInputClient(frameworkBufferCount);
        }
    }

    ATRACE_END();
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::PipelineHasHALInputStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CameraUsecaseBase::PipelineHasHALInputStream(ChiPipelineTargetCreateDescriptor*  pPipelineDesc)
{
    BOOL isHALInputStream = FALSE;
    if (NULL != m_pYuvInStream)
    {
        ChiTargetPortDescriptorInfo* pSrcTarget = &pPipelineDesc->sourceTarget;

        for (UINT sourceIdx = 0; sourceIdx < pSrcTarget->numTargets; sourceIdx++)
        {
            ChiTargetPortDescriptor* pSrcTargetDesc = &pSrcTarget->pTargetPortDesc[sourceIdx];
            if (m_pYuvInStream == pSrcTargetDesc->pTarget->pChiStream)
            {
                isHALInputStream = TRUE;
                break;
            }
        }
    }
    return isHALInputStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::CreateSessions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::CreateRTSessions(
    ChiCallBacks* pCallbacks)
{
    CDKResult result = CDKResultSuccess;
    BOOL RealtimeSession = FALSE;

    for (UINT sessionId = 0; sessionId < m_numSessions; sessionId++)
    {
        RealtimeSession = FALSE;

        Pipeline* pPipelines[MaxPipelinesPerSession] = {0};

        m_sessions[sessionId].rtPipelineIndex = InvalidId;

        // Accumulate the pipeline pointers to an array to pass the session creation
        for (UINT pipelineId = 0; pipelineId < m_sessions[sessionId].numPipelines; pipelineId++)
        {
            // CHX_LOG("MFNR CreateSessions pPipelineData->pPipeline %p sessionId %d pipelineId %d",
                // m_sessions[sessionId].pipelines[pipelineId].pPipeline, sessionId, pipelineId);

            if (TRUE == m_sessions[sessionId].pipelines[pipelineId].pPipeline->IsRealTime())
            {
                CHX_LOG("pipeline name:%s", m_sessions[sessionId].pipelines[pipelineId].pPipeline->GetPipelineName());

                // Only set the rtPipelineIndex for the first realtime pipeline
                if (InvalidId == m_sessions[sessionId].rtPipelineIndex)
                {
                    m_pDeferOfflineDoneMutex->Lock();
                    m_sessions[sessionId].rtPipelineIndex = pipelineId;
                    m_pDeferOfflineDoneMutex->Unlock();
                }
                // Only set m_rtSessionIdex for the first realtime session
                if (InvalidId == m_rtSessionIndex)
                {
                    m_pDeferOfflineDoneMutex->Lock();
                    m_rtSessionIndex = sessionId;
                    /* Partial Meta data can come from multiple pipeline.
                       We are just marking that the real time pipeline will be final one sending the partial meta data*/
                    CHX_LOG_INFO("Partial Data will be sent from Session:%d", sessionId);
                    SetFinalPipelineForPartialMetaData(sessionId);
                    m_pDeferOfflineDoneMutex->Unlock();
                }
                RealtimeSession = TRUE;
            }

            pPipelines[pipelineId] = m_sessions[sessionId].pipelines[pipelineId].pPipeline;
        }

        if (TRUE == RealtimeSession)
        {
            result = CreateSession(sessionId,
                    pPipelines,
                    pCallbacks);

            // For quadra cfa sensor, there might be more than one realtime session,
            // one for preview, and the other for snapshot RDI,
            // only create first realtime session.
            break;
        }
    }

    if (result != CDKResultSuccess)
    {
        for (INT sessionId = m_numSessions - 1; sessionId >= 0; sessionId--)
        {
            if (NULL != m_sessions[sessionId].pSession)
            {
                m_sessions[sessionId].pSession->Destroy(TRUE);
                m_sessions[sessionId].pSession = NULL;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::CreateOfflineSessions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::CreateOfflineSessions(
    ChiCallBacks* pCallbacks)
{
    CDKResult result = CDKResultSuccess;
    BOOL RealtimeSession = FALSE;
    UINT numSessionsMinusOne = (m_numSessions >= 1) ? (m_numSessions - 1) : 0;

    for (INT sessionId = numSessionsMinusOne; sessionId >= 0; sessionId--)
    {
        if (0 == m_sessions[sessionId].numPipelines)
        {
            continue;
        }

        RealtimeSession = FALSE;

        Pipeline* pPipelines[MaxPipelinesPerSession] = { 0 };

        m_sessions[sessionId].rtPipelineIndex = InvalidId;

        // Accumulate the pipeline pointers to an array to pass the session creation
        for (UINT pipelineId = 0; pipelineId < m_sessions[sessionId].numPipelines; pipelineId++)
        {
            if (TRUE == m_sessions[sessionId].pipelines[pipelineId].pPipeline->IsRealTime())
            {
                // Only set the rtPipelineIndex for the first realtime pipeline
                if (InvalidId == m_sessions[sessionId].rtPipelineIndex)
                {
                    m_pDeferOfflineDoneMutex->Lock();
                    m_sessions[sessionId].rtPipelineIndex = pipelineId;
                    m_pDeferOfflineDoneMutex->Unlock();
                }
                // Only set m_rtSessionIdex for the first realtime session
                if (InvalidId == m_rtSessionIndex)
                {
                    m_pDeferOfflineDoneMutex->Lock();
                    m_rtSessionIndex = sessionId;
                    m_pDeferOfflineDoneMutex->Unlock();
                }
                RealtimeSession = TRUE;
            }

            pPipelines[pipelineId] = m_sessions[sessionId].pipelines[pipelineId].pPipeline;
        }

        if (FALSE == RealtimeSession)
        {
            CHX_LOG("Creating offline session %d in defer thread", sessionId);
            m_perSessionPvtData[sessionId].sessionId = sessionId;
            m_perSessionPvtData[sessionId].pUsecase = this;

            if (NULL == m_sessions[sessionId].pSession)
            {
                m_sessions[sessionId].pSession = Session::Create(pPipelines,
                                                                 m_sessions[sessionId].numPipelines,
                                                                 &pCallbacks[sessionId],
                                                                 &m_perSessionPvtData[sessionId]);
            }

            if (NULL == m_sessions[sessionId].pSession)
            {
                CHX_LOG_ERROR("Failed to create offline session, sessionId: %d", sessionId);
                result = CDKResultEFailed;
                break;
            }
            else
            {
                CHX_LOG("success Creating Session %d", sessionId);

                // activate offline pipelines for quadcfa usecase
                if (UsecaseId::QuadCFA == m_usecaseId)
                {
                    for (UINT index = 0; index < m_sessions[sessionId].numPipelines; index++)
                    {
                        if (FALSE == m_sessions[sessionId].pSession->IsPipelineActive(index))
                        {
                            CHX_LOG_INFO("activate pipeline, session:%d, pipeline index in session:%d", sessionId, index);

                            result = ExtensionModule::GetInstance()->ActivatePipeline(
                                m_sessions[sessionId].pSession->GetSessionHandle(),
                                m_sessions[sessionId].pSession->GetPipelineHandle(index));

                            if (CDKResultSuccess == result)
                            {
                                m_sessions[sessionId].pSession->SetPipelineActivateFlag(index);
                                CHX_LOG_CONFIG("Setting pipeline activate flag");
                            }
                            else
                            {
                                CHX_LOG_ERROR("Fail to activate pipeline.");
                                result = CDKResultEFailed;
                                break;
                            }
                        }
                    }
                }

                if (CDKResultSuccess == result)
                {
                    for (UINT32 pipelineIndex = 0; pipelineIndex < m_sessions[sessionId].numPipelines; pipelineIndex++)
                    {
                        PipelineData* pPipelineData = &(m_sessions[sessionId].pipelines[pipelineIndex]);

                        CHX_ASSERT(TRUE == pPipelineData->pPipeline->IsOffline());

                        if (TRUE == pPipelineData->pPipeline->IsOffline())
                        {
                            m_metadataClients[pPipelineData->id] =
                                   m_pMetadataManager->RegisterClient(
                                   FALSE,
                                   pPipelineData->pPipeline->GetTagList(),
                                   pPipelineData->pPipeline->GetTagCount(),
                                   pPipelineData->pPipeline->GetPartialTagCount(),
                                   pPipelineData->pPipeline->GetMetadataBufferCount(),
                                   ChiMetadataUsage::OfflineOutput);

                            pPipelineData->pPipeline->SetMetadataClientId(m_metadataClients[pPipelineData->id]);
                        }
                    }
                }
            }
        }
    }

    if (result != CDKResultSuccess)
    {
        for (INT sessionId = m_numSessions - 1; sessionId >= 0; sessionId--)
        {
            if (NULL != m_sessions[sessionId].pSession)
            {
                m_sessions[sessionId].pSession->Destroy(TRUE);
                m_sessions[sessionId].pSession = NULL;
            }
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::CreateSessionsWithInputHALStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::CreateSessionsWithInputHALStream(
    ChiCallBacks* pCallbacks)
{
    CDKResult result    = CDKResultSuccess;
    BOOL inputHALStream = FALSE;
    UINT numSessionsMinusOne = (m_numSessions >= 1) ? (m_numSessions - 1) : 0;

    for (INT sessionId = numSessionsMinusOne; sessionId >= 0; sessionId--)
    {
        Pipeline* pPipelines[MaxPipelinesPerSession] = { 0 };

        // Accumulate the pipeline pointers to an array to pass the session creation
        for (UINT pipelineId = 0; pipelineId < m_sessions[sessionId].numPipelines; pipelineId++)
        {
            if (TRUE == m_sessions[sessionId].pipelines[pipelineId].isHALInputStream)
            {
                inputHALStream = TRUE;
            }

            pPipelines[pipelineId] = m_sessions[sessionId].pipelines[pipelineId].pPipeline;
        }

        if (TRUE == inputHALStream)
        {
            result = CreateSession(sessionId,
                    pPipelines,
                    pCallbacks);
        }
    }

    if (result != CDKResultSuccess)
    {
        for (INT sessionId = m_numSessions - 1; sessionId >= 0; sessionId--)
        {
            if (NULL != m_sessions[sessionId].pSession)
            {
                m_sessions[sessionId].pSession->Destroy(TRUE);
                m_sessions[sessionId].pSession = NULL;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::CreateSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::CreateSession(
        INT sessionId,
        Pipeline** ppPipelines,
        ChiCallBacks* pCallbacks)
{
    CDKResult result = CDKResultSuccess;

    CHX_LOG("Creating session %d ", sessionId);

    m_perSessionPvtData[sessionId].sessionId = sessionId;
    m_perSessionPvtData[sessionId].pUsecase  = this;

    if (NULL == m_sessions[sessionId].pSession)
    {
        m_sessions[sessionId].pSession = Session::Create(ppPipelines,
                m_sessions[sessionId].numPipelines,
                &pCallbacks[sessionId],
                &m_perSessionPvtData[sessionId]);
    }

    if (NULL == m_sessions[sessionId].pSession)
    {
        CHX_LOG_ERROR("Failed to create offline session, sessionId: %d", sessionId);
        result = CDKResultEFailed;
    }
    else
    {
        CHX_LOG("success Creating Session %d", sessionId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::DestroyOfflineSessions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::DestroyOfflineSessions()
{
    CDKResult result = CDKResultSuccess;

    CHX_LOG("E.");

    for (UINT session = 0; session < m_numSessions; session++)
    {
        SessionData* pData = &m_sessions[session];

        if (NULL == pData->pSession)
        {
            continue;
        }

        CHX_LOG("sessionidx:%d, session:%p, pipeline:%p",
            session, pData->pSession, pData->pipelines[0].pPipeline);

        if (FALSE == pData->pipelines[0].pPipeline->IsRealTime())
        {
            CHX_LOG("destroying session:%d, num_pipelines:%d", session, pData->numPipelines);
            result = DestroySessionByID(session, FALSE);

            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Fail to destroy offline session. session id:%d", session);
                break;
            }

            for (UINT32 index = 0; index < m_sessions[session].numPipelines; index++)
            {
                PipelineData* pPipelineData = &(m_sessions[session].pipelines[index]);

                if (ChiMetadataManager::InvalidClientId != m_metadataClients[pPipelineData->id])
                {
                    CHX_LOG("UnregisterClient pipeline id:%d, name:%s meta client:%x",
                        pPipelineData->id, pPipelineData->pPipeline->GetPipelineName(), m_metadataClients[pPipelineData->id]);

                    m_pMetadataManager->UnregisterClient(m_metadataClients[pPipelineData->id]);
                    m_metadataClients[pPipelineData->id] = ChiMetadataManager::InvalidClientId;
                }
            }
        }
    }

    CHX_LOG("X");
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::CameraCSIDTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::CameraCSIDTrigger(
    CSIDBinningInfo*                    pBinninginfo,
    ChiPipelineTargetCreateDescriptor*  pPipelineDesc)
{
    BOOL    isPreviewStream = FALSE;
    BOOL    isRawStream     = FALSE;
    UINT32  preview_width   = 0;
    UINT32  preview_height  = 0;
    UINT32  raw_width       = 0;
    UINT32  raw_height      = 0;

    ChiTargetPortDescriptorInfo*    pSinkTarget     = &pPipelineDesc->sinkTarget;
    ChiTargetPortDescriptor*        pSinkTargetDesc = NULL;
    const camera3_stream_t*         pStream         = NULL;

    // Sample code, keep it disable
    return CDKResultSuccess;

    for (UINT sinkIdx = 0; sinkIdx < pSinkTarget->numTargets; sinkIdx++)
    {
        pSinkTargetDesc = &pSinkTarget->pTargetPortDesc[sinkIdx];
        pStream         = reinterpret_cast<const camera3_stream_t *>(pSinkTargetDesc->pTarget->pChiStream);
        if (pStream != NULL)
        {
            if (UsecaseSelector::IsPreviewStream(pStream))
            {
                isPreviewStream = TRUE;
                preview_width   = pStream->width;
                preview_height  = pStream->height;
            }

            if((pStream->stream_type == ChiStreamTypeBidirectional) &&
                ((pStream->format == ChiStreamFormatRaw16)      ||
                 (pStream->format == ChiStreamFormatRaw10)))
            {
                isRawStream = TRUE;
                raw_width = pStream->width;
                raw_height = pStream->height;
            }
        }
    }

    if (isPreviewStream)
    {
        // OEM to add desired condition here
        if (isRawStream)
        {
            if ((raw_width > 2*preview_width) && (raw_height > 2*preview_height))
            {
                pBinninginfo->isBinningEnabled  = TRUE;
                pBinninginfo->binningMode       = CSIDBinningMode::HorizontalBinning;
                CHX_LOG_CONFIG("[CSID]: binning enable,the raw (%d, %d) the preview (%d, %d)",
                                raw_width, raw_height, preview_width, preview_height);
            }
        }
    }

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::CreatePipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::CreatePipeline(
    UINT32                              cameraId,
    ChiPipelineTargetCreateDescriptor*  pPipelineDesc,
    PipelineData*                       pPipelineData,
    camera3_stream_configuration_t*     pStreamConfig)
{
    CDKResult result = CDKResultSuccess;

    pPipelineData->pPipeline = Pipeline::Create(cameraId, PipelineType::Default, pPipelineDesc->pPipelineName);

    if (NULL != pPipelineData->pPipeline)
    {
        UINT                         numStreams  = 0;
        ChiTargetPortDescriptorInfo* pSinkTarget = &pPipelineDesc->sinkTarget;
        ChiTargetPortDescriptorInfo* pSrcTarget  = &pPipelineDesc->sourceTarget;

        ChiPortBufferDescriptor pipelineOutputBuffer[MaxChiStreams];
        ChiPortBufferDescriptor pipelineInputBuffer[MaxChiStreams];

        ChxUtils::Memset(pipelineOutputBuffer, 0, sizeof(pipelineOutputBuffer));
        ChxUtils::Memset(pipelineInputBuffer, 0, sizeof(pipelineInputBuffer));

        UINT32 tagId = ExtensionModule::GetInstance()->GetVendorTagId(VendorTag::FastShutterMode);
        UINT8 isFSMode = 0;
        if (StreamConfigModeFastShutter == ExtensionModule::GetInstance()->GetOpMode(m_cameraId))
        {
            isFSMode = 1;
        }

        if (TRUE == pPipelineData->pPipeline->HasSensorNode(&pPipelineDesc->pipelineCreateDesc))
        {
            ChiMetadata* pMetadata = pPipelineData->pPipeline->GetDescriptorMetadata();
            if (NULL != pMetadata)
            {
                CSIDBinningInfo binningInfo ={ 0 };
                CameraCSIDTrigger(&binningInfo, pPipelineDesc);

                result = pMetadata->SetTag("org.quic.camera.ifecsidconfig",
                                           "csidbinninginfo",
                                           &binningInfo,
                                           sizeof(binningInfo));
                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("Failed to set metadata ifecsidconfig");
                    result = CDKResultSuccess;
                }
            }
        }

        result = pPipelineData->pPipeline->SetVendorTag(tagId, static_cast<VOID*>(&isFSMode), 1);
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Failed to set metadata FSMode");
            result = CDKResultSuccess;
        }

        if (NULL != pStreamConfig)
        {
            pPipelineData->pPipeline->SetAndroidMetadata(pStreamConfig);
        }

        for (UINT sinkIdx = 0; sinkIdx < pSinkTarget->numTargets; sinkIdx++)
        {
            ChiTargetPortDescriptor* pSinkTargetDesc = &pSinkTarget->pTargetPortDesc[sinkIdx];


            UINT previewFPS  = ExtensionModule::GetInstance()->GetPreviewFPS();
            UINT videoFPS    = ExtensionModule::GetInstance()->GetVideoFPS();
            UINT pipelineFPS = ExtensionModule::GetInstance()->GetUsecaseMaxFPS();

            pSinkTargetDesc->pTarget->pChiStream->streamParams.streamFPS = pipelineFPS;

            // override ChiStream FPS value for Preview/Video streams with stream-specific values only IF
            // APP has set valid stream-specific fps
            if (UsecaseSelector::IsPreviewStream(reinterpret_cast<camera3_stream_t*>(pSinkTargetDesc->pTarget->pChiStream)))
            {
                pSinkTargetDesc->pTarget->pChiStream->streamParams.streamFPS = (previewFPS == 0) ? pipelineFPS : previewFPS;
            }
            else if (UsecaseSelector::IsVideoStream(reinterpret_cast<camera3_stream_t*>(pSinkTargetDesc->pTarget->pChiStream)))
            {
                pSinkTargetDesc->pTarget->pChiStream->streamParams.streamFPS = (videoFPS == 0) ? pipelineFPS : videoFPS;
            }

            if ((pSrcTarget->numTargets > 0) && (TRUE == m_bCloningNeeded))
            {
                m_pFrameworkOutStreams[m_numberOfOfflineStreams] = pSinkTargetDesc->pTarget->pChiStream;
                m_pClonedStream[m_numberOfOfflineStreams]        = static_cast<CHISTREAM*>(CHX_CALLOC(sizeof(CHISTREAM)));

                ChxUtils::Memcpy(m_pClonedStream[m_numberOfOfflineStreams], pSinkTargetDesc->pTarget->pChiStream, sizeof(CHISTREAM));

                pipelineOutputBuffer[sinkIdx].pStream     = m_pClonedStream[m_numberOfOfflineStreams];
                pipelineOutputBuffer[sinkIdx].pNodePort   = pSinkTargetDesc->pNodePort;
                pipelineOutputBuffer[sinkIdx].numNodePorts= pSinkTargetDesc->numNodePorts;
                pPipelineData->pStreams[numStreams++]     = pipelineOutputBuffer[sinkIdx].pStream;
                m_numberOfOfflineStreams++;

                CHX_LOG("CloningNeeded sinkIdx %d numStreams %d pStream %p nodePortId %d",
                        sinkIdx,
                        numStreams-1,
                        pipelineOutputBuffer[sinkIdx].pStream,
                        pipelineOutputBuffer[sinkIdx].pNodePort[0].nodePortId);
            }
            else
            {
                pipelineOutputBuffer[sinkIdx].pStream      = pSinkTargetDesc->pTarget->pChiStream;
                pipelineOutputBuffer[sinkIdx].pNodePort    = pSinkTargetDesc->pNodePort;
                pipelineOutputBuffer[sinkIdx].numNodePorts = pSinkTargetDesc->numNodePorts;
                pPipelineData->pStreams[numStreams++]   = pipelineOutputBuffer[sinkIdx].pStream;
                CHX_LOG("sinkIdx %d numStreams %d pStream %p format %u %d:%d nodePortID %d",
                        sinkIdx,
                        numStreams - 1,
                        pipelineOutputBuffer[sinkIdx].pStream,
                        pipelineOutputBuffer[sinkIdx].pStream->format,
                        pipelineOutputBuffer[sinkIdx].pNodePort[0].nodeId,
                        pipelineOutputBuffer[sinkIdx].pNodePort[0].nodeInstanceId,
                        pipelineOutputBuffer[sinkIdx].pNodePort[0].nodePortId);
            }
        }

        for (UINT sourceIdx = 0; sourceIdx < pSrcTarget->numTargets; sourceIdx++)
        {
            UINT                     i              = 0;
            ChiTargetPortDescriptor* pSrcTargetDesc = &pSrcTarget->pTargetPortDesc[sourceIdx];

            pipelineInputBuffer[sourceIdx].pStream = pSrcTargetDesc->pTarget->pChiStream;

            pipelineInputBuffer[sourceIdx].pNodePort    = pSrcTargetDesc->pNodePort;
            pipelineInputBuffer[sourceIdx].numNodePorts = pSrcTargetDesc->numNodePorts;

            for (i = 0; i < numStreams; i++)
            {
                if (pPipelineData->pStreams[i] == pipelineInputBuffer[sourceIdx].pStream)
                {
                    break;
                }
            }
            if (numStreams == i)
            {
                pPipelineData->pStreams[numStreams++] = pipelineInputBuffer[sourceIdx].pStream;
            }

            for (UINT portIndex = 0; portIndex < pipelineInputBuffer[sourceIdx].numNodePorts; portIndex++)
            {
                CHX_LOG("sourceIdx %d portIndex %d numStreams %d pStream %p format %u %d:%d nodePortID %d",
                        sourceIdx,
                        portIndex,
                        numStreams - 1,
                        pipelineInputBuffer[sourceIdx].pStream,
                        pipelineInputBuffer[sourceIdx].pStream->format,
                        pipelineInputBuffer[sourceIdx].pNodePort[portIndex].nodeId,
                        pipelineInputBuffer[sourceIdx].pNodePort[portIndex].nodeInstanceId,
                        pipelineInputBuffer[sourceIdx].pNodePort[portIndex].nodePortId);
            }
        }
        pPipelineData->pPipeline->SetOutputBuffers(pSinkTarget->numTargets, &pipelineOutputBuffer[0]);
        pPipelineData->pPipeline->SetInputBuffers(pSrcTarget->numTargets, &pipelineInputBuffer[0]);
        pPipelineData->pPipeline->SetPipelineNodePorts(&pPipelineDesc->pipelineCreateDesc);
        pPipelineData->pPipeline->SetPipelineName(pPipelineDesc->pPipelineName);

        CHX_LOG("set sensor mode pick hint: %p", GetSensorModePickHint(pPipelineData->id));
        pPipelineData->pPipeline->SetSensorModePickHint(GetSensorModePickHint(pPipelineData->id));

        pPipelineData->numStreams       = numStreams;

        result = pPipelineData->pPipeline->CreateDescriptor();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::CreatePipelineDescriptorsPerSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::CreatePipelineDescriptorsPerSession(
    UINT32        sessionId)
{
    CDKResult    result = CDKResultSuccess;
    SessionData* pData  = &m_sessions[sessionId];

    CHX_LOG("sessionId:%d", sessionId);

    if (NULL != pData->pSession)
    {
        CHX_LOG_ERROR("Can't create pipeline destriptors when session is there");
        result = CDKResultEFailed;
    }

    if ((CDKResultSuccess == result) && (pData->numPipelines > 0))
    {
        CHX_LOG_ERROR("Already has piplines associated with session, numPipelines:%d", pData->numPipelines);
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT i = 0; i < m_pChiUsecase->numPipelines; i++)
        {
            UINT sessionIdOfPipeline  = (NULL != m_pPipelineToSession) ? m_pPipelineToSession[i] : i;
            UINT pipelineIdxInSession = InvalidId;

            if (sessionIdOfPipeline != sessionId)
            {
                continue;
            }

            pipelineIdxInSession = m_sessions[sessionId].numPipelines++;

            m_sessions[sessionId].pipelines[pipelineIdxInSession].id = i;// Assign the ID to pipelineID

            CHX_LOG("Creating Pipeline %s at index %u for session %u, session's pipeline %u, camera id:%d",
                m_pChiUsecase->pPipelineTargetCreateDesc[i].pPipelineName, i, sessionId, pipelineIdxInSession, m_pPipelineToCamera[i]);

            result = CreatePipeline(m_pPipelineToCamera[i],
                                    &m_pChiUsecase->pPipelineTargetCreateDesc[i],
                                    &m_sessions[sessionId].pipelines[pipelineIdxInSession]);

            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Failed to create pipeline!");
                break;
            }

            PipelineCreated(sessionId, pipelineIdxInSession);

            m_sessions[sessionId].pipelines[pipelineIdxInSession].pPipeline->SetMetadataClientId(m_metadataClients[i]);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::DestroyPipelineDescriptorsPerSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::DestroyPipelineDescriptorsPerSession(
    UINT32        sessionId)
{
    CDKResult    result = CDKResultSuccess;
    SessionData* pData  = &m_sessions[sessionId];

    CHX_LOG("sessionId:%d", sessionId);

    if (NULL != pData->pSession)
    {
        CHX_LOG_ERROR("Can't destroy pipeline destriptors when session is there");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT pipeline = 0; pipeline < pData->numPipelines; pipeline++)
        {
            for (UINT stream = 0; stream < pData->pipelines[pipeline].numStreams; stream++)
            {
                pData->pipelines[pipeline].pStreams[stream] = NULL;
            }
            pData->pipelines[pipeline].numStreams = 0;

            if (NULL != pData->pipelines[pipeline].pPipeline)
            {
                CHX_LOG("Destroying pipeline: %s", pData->pipelines[pipeline].pPipeline->GetPipelineName());

                pData->pipelines[pipeline].pPipeline->Destroy();
                pData->pipelines[pipeline].pPipeline = NULL;
            }

            PipelineDestroyed(sessionId, pipeline);

            pData->pipelines[pipeline].id = InvalidId;
        }

        pData->numPipelines = 0;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::CreateSessionByID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::CreateSessionByID(
    UINT32        sessionId,
    ChiCallBacks* pCallbacks)
{
    CDKResult    result                             = CDKResultSuccess;
    SessionData* pSessionData                       = &m_sessions[sessionId];
    Pipeline*    pPipelines[MaxPipelinesPerSession] = {0};

    CHX_LOG("E. sessionid:%d", sessionId);

    if (NULL != pSessionData->pSession)
    {
        CHX_LOG_ERROR("Session is already created, pSession:%p", pSessionData->pSession);
        result = CDKResultEExists;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT index = 0; index < pSessionData->numPipelines; index++)
        {
            if (NULL != pSessionData->pipelines[index].pPipeline)
            {
                pPipelines[index] = pSessionData->pipelines[index].pPipeline;

                CHX_LOG("[%d/%d] global pipeline index:%d, pPipeline:%p",
                    index, pSessionData->numPipelines,
                    pSessionData->pipelines[index].id,
                    pSessionData->pipelines[index].pPipeline);
            }
            else
            {
                CHX_LOG_ERROR("Invalid Pipeline! session:%d, pipeline index:%d", sessionId, index);
                result = CDKResultEFailed;
                break;
            }
        }
    }

    if (CDKResultSuccess == result)
    {
        result = CreateSession(sessionId, pPipelines, pCallbacks);

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Fail to create chx session.");
        }
    }

    CHX_LOG("X.");
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::DestroySessionByID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::DestroySessionByID(
    UINT32 sessionId,
    BOOL   isForced)
{
    CDKResult    result       = CDKResultSuccess;
    SessionData* pSessionData = &m_sessions[sessionId];

    CHX_LOG("E. sessionid:%d, pSession:%p, isForced:%d", sessionId, pSessionData->pSession, isForced);

    if (NULL != pSessionData->pSession)
    {
        CHX_LOG("set pipeline to deactivated state");
        pSessionData->pSession->SetPipelineDeactivate();

        pSessionData->pSession->Destroy(isForced);
        pSessionData->pSession = NULL;
    }
    else
    {
        result = CDKResultEInvalidArg;
    }

    CHX_LOG("X.");
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::DestroyInternalBufferQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::DestroyInternalBufferQueue(
    UINT32 bufferID)
{
    if (MaxChiStreams > bufferID)
    {
        TargetBuffer* pTargetBuffer = &m_MCTargetBuffer[bufferID];

        for (UINT32 i = 0; i< RDIBufferQueueDepth; i++)
        {
            if (NULL != pTargetBuffer->bufferQueue[i].pRdiOutputBuffer)
            {
                if (NULL != pTargetBuffer->bufferQueue[i].pRdiOutputBuffer->bufferInfo.phBuffer)
                {
                    pTargetBuffer->pBufferManager->ReleaseReference(
                        &(pTargetBuffer->bufferQueue[i].pRdiOutputBuffer->bufferInfo));
                }
                ChxUtils::Free(pTargetBuffer->bufferQueue[i].pRdiOutputBuffer);
                pTargetBuffer->bufferQueue[i].pRdiOutputBuffer = NULL;
            }

            if (NULL != pTargetBuffer->bufferQueue[i].pMetadata)
            {
                pTargetBuffer->bufferQueue[i].pMetadata = NULL;
            }

            pTargetBuffer->bufferQueue[i].isBufferReady   = FALSE;
            pTargetBuffer->bufferQueue[i].isMetadataReady = FALSE;
        }

        if (NULL != pTargetBuffer->pBufferManager)
        {
            pTargetBuffer->pBufferManager->Destroy();
            pTargetBuffer->pBufferManager = NULL;
        }

        if( NULL != pTargetBuffer->pMutex)
        {
            pTargetBuffer->pMutex->Destroy();
            pTargetBuffer->pMutex = NULL;
        }

        if (NULL != pTargetBuffer->pCondition)
        {
            pTargetBuffer->pCondition->Destroy();
            pTargetBuffer->pCondition = NULL;
        }
    }
    else
    {
        CHX_LOG("invalid bufferID:%d", bufferID);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::DestroyAllInternalBufferQueues
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::DestroyAllInternalBufferQueues()
{
    for (UINT32 i = 0; i < m_targetBuffersCount; i++)
    {
        DestroyInternalBufferQueue(i);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::AddNewInternalBufferQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::AddNewInternalBufferQueue(
    ChiStream*                  pChiStream,
    CHIBufferManagerCreateData* pBufferCreateData,
    UINT32*                     pBufferID,
    const CHAR*                 pBufferManagerName)
{
    CHX_ASSERT(NULL != pChiStream);
    CHX_ASSERT(NULL != pBufferCreateData);
    CHX_ASSERT(NULL != pBufferID);
    CHX_ASSERT(NULL != pBufferManagerName);

    CDKResult result = CDKResultSuccess;

    if (MaxChiStreams <= m_targetBuffersCount)
    {
        CHX_LOG_ERROR("Can't add new internal buffer queue!");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        m_MCTargetBuffer[m_targetBuffersCount].pBufferManager       = CHIBufferManager::Create(pBufferManagerName, pBufferCreateData);
        m_MCTargetBuffer[m_targetBuffersCount].pMutex               = Mutex::Create();
        m_MCTargetBuffer[m_targetBuffersCount].pCondition           = Condition::Create();
        m_MCTargetBuffer[m_targetBuffersCount].lastReadySequenceID  = INVALIDSEQUENCEID;
        m_MCTargetBuffer[m_targetBuffersCount].firstReadySequenceID = INVALIDSEQUENCEID;
        m_MCTargetBuffer[m_targetBuffersCount].validBufferLength    = 0;

        for (UINT32 i = 0; i < RDIBufferQueueDepth; i++)
        {
            TargetBufferInfo* pBufferQueue = &m_MCTargetBuffer[m_targetBuffersCount].bufferQueue[i];

            pBufferQueue->pRdiOutputBuffer = static_cast<CHISTREAMBUFFER*>(CHX_CALLOC(sizeof(CHISTREAMBUFFER)));
            pBufferQueue->frameNumber      = INVALIDSEQUENCEID;

            if (NULL != pBufferQueue->pRdiOutputBuffer)
            {
                pBufferQueue->pRdiOutputBuffer->size                = sizeof(CHISTREAMBUFFER);
                pBufferQueue->pRdiOutputBuffer->acquireFence.valid  = FALSE;
                pBufferQueue->pRdiOutputBuffer->bufferInfo.phBuffer = NULL;
                pBufferQueue->pRdiOutputBuffer->pStream             = pChiStream;
                pBufferQueue->pMetadata                             = NULL;
                pBufferQueue->isBufferReady                         = FALSE;
                pBufferQueue->isMetadataReady                       = FALSE;

                CHX_LOG("create target buffer:%d,%p,%p type %d", i,
                    pBufferQueue->pRdiOutputBuffer,
                    pBufferQueue->pRdiOutputBuffer->bufferInfo.phBuffer,
                    pBufferQueue->pRdiOutputBuffer->bufferInfo.bufferType);
            }
        }
    }

    *pBufferID = m_targetBuffersCount;
    m_targetBuffersCount++;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::IsRDIBufferMetaReadyForInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CameraUsecaseBase::IsRDIBufferMetaReadyForInput(
        UINT32 frameNumber,
        UINT32 pipelineIndex)
{
    BOOL isBufferMetaReady          = TRUE;
    UINT32          bufferIndex     = frameNumber % RDIBufferQueueDepth;
    TargetBuffer*   pTargetBuffer   = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_RDIBufferID[pipelineIndex]);
        }
    }

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();
        if ((FALSE == pTargetBuffer->bufferQueue[bufferIndex].isBufferReady) ||
            (FALSE == pTargetBuffer->bufferQueue[bufferIndex].isMetadataReady))
        {
            isBufferMetaReady = FALSE;
        }
        pTargetBuffer->pMutex->Unlock();
    }
    else
    {
        CHX_LOG_WARN("PTargetBuffer is NULL");
    }

    return isBufferMetaReady;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ReleaseRDIBufferReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::ReleaseRDIBufferReference(
    UINT32          pipelineIndex,
    CHIBUFFERINFO*  pBufferInfo)
{
    CHX_ASSERT(MaxChiStreams  > pipelineIndex);
    CHX_ASSERT(NULL          != pBufferInfo);

    CDKResult result              = CDKResultSuccess;
    TargetBuffer*   pTargetBuffer = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_RDIBufferID[pipelineIndex]);
        }
    }

    if (NULL != pTargetBuffer)
    {
        result = pTargetBuffer->pBufferManager->ReleaseReference(pBufferInfo);
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ReleaseFDBufferReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::ReleaseFDBufferReference(
    UINT32              pipelineIndex,
    CHIBUFFERINFO*      pBufferInfo)
{
    CHX_ASSERT(MaxChiStreams  > pipelineIndex);
    CHX_ASSERT(NULL           != pBufferInfo);

    CDKResult result              = CDKResultSuccess;
    TargetBuffer*   pTargetBuffer = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_FDBufferID[pipelineIndex]);
        }
    }

    if (NULL != pTargetBuffer)
    {
        result = pTargetBuffer->pBufferManager->ReleaseReference(pBufferInfo);
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::GetBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIBufferManager* CameraUsecaseBase::GetBufferManager(
    UINT32 pipelineIndex)
{
    TargetBuffer*     pTargetBuffer        = NULL;
    CHIBufferManager* pTargetBufferManager = NULL;

    if (pipelineIndex < MaxChiStreams)
    {
        if (FALSE == IsMultiCameraUsecase())
        {
            pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
        }
        else
        {
            pTargetBuffer = GetTargetBufferPointer(m_RDIBufferID[pipelineIndex]);
        }
    }

    if (NULL != pTargetBuffer)
    {
        pTargetBufferManager = pTargetBuffer->pBufferManager;
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
    }

    return pTargetBufferManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ResetBufferQueueSlot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::ReserveBufferQueueSlot(
    UINT32          frameNumber,
    UINT32          pipelineIndex,
    CHIBUFFERHANDLE phBuffer)
{
    // Reserve the queue slot for a request.
    // If framework provides a buffer, then we copy the buffer handle to the slot,
    // Otherwise GetOutputBufferFromRDIQueue API should setup the buffer handle

    TargetBuffer*   pTargetBuffer   = GetTargetBufferPointer(pipelineIndex);
    UINT32          queueIndex      = frameNumber % RDIBufferQueueDepth;

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();

        pTargetBuffer->bufferQueue[queueIndex].frameNumber                              = frameNumber;
        pTargetBuffer->bufferQueue[queueIndex].pRdiOutputBuffer->bufferInfo.phBuffer    = phBuffer;
        pTargetBuffer->bufferQueue[queueIndex].isBufferReady                            = FALSE;
        pTargetBuffer->bufferQueue[queueIndex].isMetadataReady                          = FALSE;

        pTargetBuffer->pMutex->Unlock();
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ClearBufferQueueSlot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::ClearBufferQueueSlot(
    UINT32 frameNumber,
    UINT32 pipelineIndex)
{
    TargetBuffer*   pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    UINT32          queueIndex    = frameNumber % RDIBufferQueueDepth;

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();

        pTargetBuffer->bufferQueue[queueIndex].frameNumber                           = INVALIDSEQUENCEID;
        pTargetBuffer->bufferQueue[queueIndex].pRdiOutputBuffer->bufferInfo.phBuffer = NULL;
        pTargetBuffer->bufferQueue[queueIndex].isBufferReady                         = FALSE;
        pTargetBuffer->bufferQueue[queueIndex].isMetadataReady                       = FALSE;

        pTargetBuffer->pMutex->Unlock();
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::HasRDIbuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CameraUsecaseBase::HasRDIBuffer(
    UINT32 frameNumber,
    UINT32 pipelineIndex)
{
    BOOL            hasRDI          = FALSE;
    UINT32          queueIndex      = frameNumber % RDIBufferQueueDepth;
    TargetBuffer*   pTargetBuffer   = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_RDIBufferID[pipelineIndex]);
        }
    }

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();
        if ((frameNumber == pTargetBuffer->bufferQueue[queueIndex].frameNumber) &&
             (NULL != pTargetBuffer->bufferQueue[queueIndex].pRdiOutputBuffer->bufferInfo.phBuffer))
        {
            hasRDI = TRUE;
        }
        pTargetBuffer->pMutex->Unlock();
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
    }

    return hasRDI;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::FlushRDIQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::FlushRDIQueue(
    UINT32 frameNumber,
    UINT32 pipelineIndex,
    BOOL   flushMetadata)
{
    CDKResult       result          = CDKResultSuccess;
    UINT32          bufferIndex = frameNumber % RDIBufferQueueDepth;
    TargetBuffer*   pTargetBuffer = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_RDIBufferID[pipelineIndex]);
        }
    }

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();

        // Flush out all the RDI buffers with frameNumber less than the input frameNumber.
        // Use frameNumber 0xFFFFFFFF if want to flush the entire queue.
        for (UINT32 i = 0; i < RDIBufferQueueDepth; i++)
        {
            if (pTargetBuffer->bufferQueue[i].frameNumber != INVALIDSEQUENCEID &&
                pTargetBuffer->bufferQueue[i].frameNumber <= frameNumber &&
                pTargetBuffer->bufferQueue[i].pRdiOutputBuffer->bufferInfo.phBuffer != NULL)
            {
                pTargetBuffer->pBufferManager->ReleaseReference(
                    &(pTargetBuffer->bufferQueue[i].pRdiOutputBuffer->bufferInfo));
                pTargetBuffer->bufferQueue[i].frameNumber = INVALIDSEQUENCEID;
                pTargetBuffer->bufferQueue[i].pRdiOutputBuffer->bufferInfo.phBuffer = NULL;
                pTargetBuffer->bufferQueue[i].isBufferReady = FALSE;

                if (TRUE == flushMetadata)
                {
                    ReleaseMetadataBuffer(pTargetBuffer, i);
                }
            }
        }

        if (frameNumber < pTargetBuffer->lastReadySequenceID)
        {
            pTargetBuffer->firstReadySequenceID = frameNumber + 1;
        }
        else
        {
            pTargetBuffer->lastReadySequenceID  = INVALIDSEQUENCEID;
            pTargetBuffer->firstReadySequenceID = INVALIDSEQUENCEID;
        }

        UINT32 lastReadyBufferIndex  = pTargetBuffer->lastReadySequenceID % BufferQueueDepth;
        UINT32 firstReadyBufferIndex = pTargetBuffer->firstReadySequenceID % BufferQueueDepth;
        // sanity check
        if ((FALSE == pTargetBuffer->bufferQueue[lastReadyBufferIndex].isBufferReady) ||
            (FALSE == pTargetBuffer->bufferQueue[lastReadyBufferIndex].isMetadataReady))
        {
            pTargetBuffer->lastReadySequenceID  = INVALIDSEQUENCEID;
        }

        if ((FALSE == pTargetBuffer->bufferQueue[firstReadyBufferIndex].isBufferReady) ||
            (FALSE == pTargetBuffer->bufferQueue[firstReadyBufferIndex].isMetadataReady) ||
            (pTargetBuffer->lastReadySequenceID == INVALIDSEQUENCEID))
        {
            pTargetBuffer->firstReadySequenceID = INVALIDSEQUENCEID;
        }

        pTargetBuffer->pMutex->Unlock();
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::HasFDBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CameraUsecaseBase::HasFDBuffer(
    UINT32 frameNumber,
    UINT32 pipelineIndex)
{
    BOOL            hasRDI          = FALSE;
    UINT32          queueIndex      = frameNumber % RDIBufferQueueDepth;
    UINT32          bufferIndex     = frameNumber % RDIBufferQueueDepth;
    TargetBuffer*   pTargetBuffer   = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_FDBufferID[pipelineIndex]);
        }
    }

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();
        if ((frameNumber == pTargetBuffer->bufferQueue[queueIndex].frameNumber) &&
            (NULL != pTargetBuffer->bufferQueue[queueIndex].pRdiOutputBuffer->bufferInfo.phBuffer))
        {
            hasRDI = TRUE;
        }
        pTargetBuffer->pMutex->Unlock();
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
    }

    return hasRDI;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::FlushFDQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::FlushFDQueue(
    UINT32 frameNumber,
    UINT32 pipelineIndex)
{
    CDKResult       result          = CDKResultSuccess;
    UINT32          queueIndex      = frameNumber % RDIBufferQueueDepth;
    TargetBuffer*   pTargetBuffer   = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_FDBufferID[pipelineIndex]);
        }
    }

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();

        // Flush out all the FD buffers with frameNumber less than the input frameNumber.
        // Use frameNumber 0xFFFFFFFF if want to flush the entire queue.
        for (UINT32 i = 0; i < RDIBufferQueueDepth; i++)
        {
            if (pTargetBuffer->bufferQueue[i].frameNumber != INVALIDSEQUENCEID &&
                pTargetBuffer->bufferQueue[i].frameNumber <= frameNumber &&
                pTargetBuffer->bufferQueue[i].pRdiOutputBuffer->bufferInfo.phBuffer != NULL)
            {
                pTargetBuffer->pBufferManager->ReleaseReference(
                    &(pTargetBuffer->bufferQueue[i].pRdiOutputBuffer->bufferInfo));
                pTargetBuffer->bufferQueue[i].frameNumber                           = INVALIDSEQUENCEID;
                pTargetBuffer->bufferQueue[i].pRdiOutputBuffer->bufferInfo.phBuffer = NULL;
                pTargetBuffer->bufferQueue[i].isBufferReady                         = FALSE;
            }
        }

        pTargetBuffer->pMutex->Unlock();
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::GetInputBufferFromRDIQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::GetInputBufferFromRDIQueue(
    UINT32                      frameNumber,
    UINT32                      pipelineIndex,
    UINT32                      bufferIndex,
    CHISTREAMBUFFER*            pInputBuffer,
    ChiMetadata**               ppChiMetadata,
    BOOL                        releaseBufHdlRef)
{
    CDKResult result                    = CDKResultSuccess;
    const RequestMapInfo* pRequestInfo  = &(m_requestMapInfo[frameNumber % MaxOutstandingRequests]);
    UINT32 snapshotRequestID            = pRequestInfo->snapshotRequestID;


    if (INVALIDSEQUENCEID != snapshotRequestID)
    {
        OfflineInputInfo* pInputInfo    = &m_snapshotInputInfo[snapshotRequestID % MaxOutstandingRequests];

        if (bufferIndex < pInputInfo->numOfBuffers)
        {
            *ppChiMetadata = pInputInfo->pRDIMetadata[pipelineIndex][bufferIndex];
            ChxUtils::Memcpy(pInputBuffer,
                             &(pInputInfo->RDIBufferArray[pipelineIndex][bufferIndex]),
                             sizeof(CHISTREAMBUFFER));

            if (NULL != pInputInfo->RDIBufferArray[pipelineIndex][bufferIndex].pStream)
            {
                pInputBuffer->pParentSinkStreamPrivateInfo =
                    pInputInfo->RDIBufferArray[pipelineIndex][bufferIndex].pStream->pPrivateInfo;
            }
            // if release buffer handle ref is True, the buffer reference will be released by itself.
            if (TRUE == releaseBufHdlRef)
            {
                pInputInfo->RDIBufferArray[pipelineIndex][bufferIndex].bufferInfo.phBuffer = NULL;
            }
        }
        else
        {
            CHX_LOG_ERROR("Invalid buffer index:%d, framenumber:%d,pipeline:%d",
                          bufferIndex, frameNumber, pipelineIndex);
            result = CDKResultEInvalidArg;
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid snapshotReqID! buffer index:%d, framenumber:%d,pipeline:%d",
                      bufferIndex, frameNumber, pipelineIndex);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::GetInputBufferFromFDQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::GetInputBufferFromFDQueue(
    UINT32                      frameNumber,
    UINT32                      pipelineIndex,
    UINT32                      bufferIndex,
    CHISTREAMBUFFER*            pInputBuffer,
    BOOL                        releaseBufHdlRef)
{
    CDKResult result = CDKResultSuccess;
    const RequestMapInfo* pRequestInfo = &(m_requestMapInfo[frameNumber % MaxOutstandingRequests]);
    UINT32 snapshotRequestID = pRequestInfo->snapshotRequestID;
    if (INVALIDSEQUENCEID != snapshotRequestID)
    {
        OfflineInputInfo* pInputInfo = &m_snapshotInputInfo[snapshotRequestID % MaxOutstandingRequests];
        if (bufferIndex < pInputInfo->numOfBuffers)
        {
            ChxUtils::Memcpy(pInputBuffer, &(pInputInfo->FDBufferArray[pipelineIndex][bufferIndex]),
                sizeof(CHISTREAMBUFFER));

            // if release buffer handle ref is True, the buffer reference will be released by itself.
            if (TRUE == releaseBufHdlRef)
            {
                pInputInfo->FDBufferArray[pipelineIndex][bufferIndex].bufferInfo.phBuffer = NULL;
            }
        }
        else
        {
            CHX_LOG_ERROR("Invalid buffer index:%d, framenumber:%d,pipeline:%d",
                bufferIndex, frameNumber, pipelineIndex);
            result = CDKResultEInvalidArg;
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid snapshotReqID! buffer index:%d, framenumber:%d,pipeline:%d",
            bufferIndex, frameNumber, pipelineIndex);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ReleaseMetadataBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::ReleaseMetadataBuffer(
    TargetBuffer*   pTargetBuffer,
    UINT32          releasedBufferIdx)
{
    CDKResult         result            = CDKResultSuccess;
    TargetBufferInfo* pTargetBufferInfo = &(pTargetBuffer->bufferQueue[releasedBufferIdx]);

    if (NULL != pTargetBufferInfo->pMetadata)
    {
        result = m_pMetadataManager->Release(
            pTargetBufferInfo->pMetadata);

        if (CDKResultSuccess == result)
        {
            pTargetBufferInfo->pMetadata       = NULL;
            pTargetBufferInfo->isMetadataReady = FALSE;
        }
        else
        {
            CHX_LOG_ERROR("ERROR cannot release metadata for clientId %x frameNum %u",
                          pTargetBufferInfo->pMetadata->GetClientId(),
                          pTargetBufferInfo->frameNumber);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::UpdateBufferReadyForRDIQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::UpdateBufferReadyForRDIQueue(
    UINT32              frameNumber,
    UINT32              pipelineIndex,
    BOOL                isBufferReady)
{
    CHX_ASSERT(MaxChiStreams > pipelineIndex);

    CDKResult           result          = CDKResultSuccess;
    UINT32          bufferIndex         = frameNumber % RDIBufferQueueDepth;
    TargetBuffer*   pTargetBuffer       = NULL;

    if (pipelineIndex < MaxChiStreams)
    {
        if (FALSE == IsMultiCameraUsecase())
        {
            pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
        }
        else
        {
            pTargetBuffer = GetTargetBufferPointer(m_RDIBufferID[pipelineIndex]);
        }
    }

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();

        if (frameNumber != pTargetBuffer->bufferQueue[bufferIndex].frameNumber)
        {
            CHX_LOG_ERROR("FrameNumber mismatch! bufferQueue[%d].frameNumber=%d, input frameNumber=%d",
                          bufferIndex, pTargetBuffer->bufferQueue[bufferIndex].frameNumber, frameNumber);
            result = CDKResultEFailed;
        }

        if (CDKResultSuccess == result)
        {
            // always reserve valid buffer length for offline capture.
            // for example, if both MFNR and HDR feature are enabled at the same time,
            // the valid buffer length is always max(MFNR_required, HDR required) input frames
            // use valid buffer length, it will save buffer consuming.

            if (frameNumber >= pTargetBuffer->validBufferLength)
            {
                UINT32 releasedBufferIdx = (frameNumber - pTargetBuffer->validBufferLength) % RDIBufferQueueDepth;
                //release the invalid buffer. The valid buffer is  always at the range
                // [current frame number - valid buffer length + 1, current frame number]
                if (NULL != pTargetBuffer->bufferQueue[releasedBufferIdx].pRdiOutputBuffer->bufferInfo.phBuffer)
                {
                    pTargetBuffer->pBufferManager->ReleaseReference(
                        &(pTargetBuffer->bufferQueue[releasedBufferIdx].pRdiOutputBuffer->bufferInfo));
                    pTargetBuffer->bufferQueue[releasedBufferIdx].pRdiOutputBuffer->bufferInfo.phBuffer = NULL;
                    pTargetBuffer->bufferQueue[releasedBufferIdx].pRdiOutputBuffer->acquireFence.valid  = FALSE;
                    pTargetBuffer->bufferQueue[releasedBufferIdx].pRdiOutputBuffer->releaseFence.valid  = FALSE;
                    pTargetBuffer->bufferQueue[releasedBufferIdx].isBufferReady = FALSE;
                    CHX_LOG_WARN("released framenumber :%d,framenumber:%d, releasedBufferIdx:%d buffer handle is released",
                                 (frameNumber - pTargetBuffer->validBufferLength),
                                 frameNumber,
                                 releasedBufferIdx);

                }
                else
                {
                    CHX_LOG_INFO("released framenumber :%d,framenumber:%d, buffer handle is used or released",
                                 (frameNumber - pTargetBuffer->validBufferLength),
                                 frameNumber);
                }

                // release metadata
                ReleaseMetadataBuffer(pTargetBuffer, releasedBufferIdx);
            }

            pTargetBuffer->bufferQueue[bufferIndex].isBufferReady = isBufferReady;

            if ((TRUE == isBufferReady) && (TRUE == pTargetBuffer->bufferQueue[bufferIndex].isMetadataReady))
            {
                // Normally the lastReadySequenceID should equal with frameNumber - 1
                // Otherwise the RDI buffer will NOT be continouse
                if (((pTargetBuffer->lastReadySequenceID) == INVALIDSEQUENCEID) ||
                    ((pTargetBuffer->firstReadySequenceID) == INVALIDSEQUENCEID) ||
                    ((frameNumber - pTargetBuffer->lastReadySequenceID) > 1))
                {
                    if ((TRUE == IsMultiCameraUsecase())   &&
                        (INVALIDSEQUENCEID != frameNumber) &&
                        (0 < frameNumber))
                    {
                        FlushRDIQueue(frameNumber-1, pipelineIndex);
                    }

                    pTargetBuffer->firstReadySequenceID = frameNumber;
                }
                else
                {
                    if ((frameNumber - pTargetBuffer->firstReadySequenceID + 1) >= pTargetBuffer->validBufferLength)
                    {
                        pTargetBuffer->firstReadySequenceID = frameNumber - pTargetBuffer->validBufferLength + 1;
                    }
                }

                pTargetBuffer->lastReadySequenceID = frameNumber;
                pTargetBuffer->pCondition->Signal();
            }
        }

        CHX_LOG_INFO("validBufferLength:%d, framenumber:%d, pipeline:%d, lastReady:%d,firstValid:%d",
                     pTargetBuffer->validBufferLength, frameNumber, pipelineIndex,
                     pTargetBuffer->lastReadySequenceID,
                     pTargetBuffer->firstReadySequenceID);

        pTargetBuffer->pMutex->Unlock();
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::UpdateBufferReadyForFDQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::UpdateBufferReadyForFDQueue(
    UINT32              frameNumber,
    UINT32              pipelineIndex,
    BOOL                isBufferReady)
{
    CHX_ASSERT(MaxChiStreams > pipelineIndex);

    CDKResult       result            = CDKResultSuccess;
    UINT32          bufferIndex       = frameNumber % RDIBufferQueueDepth;
    TargetBuffer*   pTargetBuffer     = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        pTargetBuffer = GetTargetBufferPointer(m_FDBufferID[pipelineIndex]);
    }

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();

        if (frameNumber != pTargetBuffer->bufferQueue[bufferIndex].frameNumber)
        {
            CHX_LOG_ERROR("FrameNumber mismatch! bufferQueue[%d].frameNumber=%d, input frameNumber=%d",
                bufferIndex, pTargetBuffer->bufferQueue[bufferIndex].frameNumber, frameNumber);
            result = CDKResultEFailed;
        }

        if (CDKResultSuccess == result)
        {
            // always reserve valid buffer length for offline capture.
            // for example, if both MFNR and HDR feature are enabled at the same time,
            // the valid buffer length is always max(MFNR_required, HDR required) input frames
            // use valid buffer length, it will save buffer consuming.
            CHX_LOG_INFO("validBufferLength:%d, framenumber:%d, pipeline:%d",
                pTargetBuffer->validBufferLength, frameNumber, pipelineIndex);

            if (frameNumber >= pTargetBuffer->validBufferLength)
            {
                UINT32 releasedBufferIdx = (frameNumber - pTargetBuffer->validBufferLength) % RDIBufferQueueDepth;
                //release the invalid buffer. The valid buffer is  always at the range
                // [current frame number - valid buffer length + 1, current frame number]
                if (NULL != pTargetBuffer->bufferQueue[releasedBufferIdx].pRdiOutputBuffer->bufferInfo.phBuffer)
                {
                    pTargetBuffer->pBufferManager->ReleaseReference(
                        &(pTargetBuffer->bufferQueue[releasedBufferIdx].pRdiOutputBuffer->bufferInfo));
                    pTargetBuffer->bufferQueue[releasedBufferIdx].pRdiOutputBuffer->bufferInfo.phBuffer = NULL;
                    pTargetBuffer->bufferQueue[releasedBufferIdx].pRdiOutputBuffer->acquireFence.valid  = FALSE;
                    pTargetBuffer->bufferQueue[releasedBufferIdx].pRdiOutputBuffer->releaseFence.valid  = FALSE;
                    pTargetBuffer->bufferQueue[releasedBufferIdx].isBufferReady = FALSE;
                }
                else
                {
                    CHX_LOG_WARN("released framenumber :%d,framenumber:%d, buffer handle is used or released",
                        (frameNumber - pTargetBuffer->validBufferLength),
                        frameNumber);
                }

            }

            pTargetBuffer->bufferQueue[bufferIndex].isBufferReady = isBufferReady;

            if (TRUE == isBufferReady)
            {
                pTargetBuffer->lastReadySequenceID = frameNumber;
                pTargetBuffer->pCondition->Signal();
            }
        }
        pTargetBuffer->pMutex->Unlock();
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::FillMetadataForRDIQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadata* CameraUsecaseBase::FillMetadataForRDIQueue(
    UINT32                      frameNumber,
    UINT32                      pipelineIndex,
    ChiMetadata*                pMetadata)
{
    CHX_ASSERT(NULL != pMetadata);
    CHX_ASSERT(MaxChiStreams > pipelineIndex);

    CDKResult       result          = CDKResultSuccess;
    ChiMetadata*    pReturnMetadata = NULL;
    UINT32          bufferIndex     = frameNumber % RDIBufferQueueDepth;
    TargetBuffer*   pTargetBuffer   = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        pTargetBuffer = GetTargetBufferPointer(m_RDIBufferID[pipelineIndex]);
    }

    if (NULL == pMetadata)
    {
        CHX_LOG_ERROR("pMetadata is NULL");
        result = CDKResultEFailed;
    }

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();

        if (frameNumber != pTargetBuffer->bufferQueue[bufferIndex].frameNumber)
        {
            CHX_LOG_ERROR("FrameNumber mismatch! bufferQueue[%d].frameNumber=%d, input frameNumber=%d",
                          bufferIndex, pTargetBuffer->bufferQueue[bufferIndex].frameNumber, frameNumber);
            result = CDKResultEFailed;
        }

        if (CDKResultSuccess == result)
        {
            TargetBufferInfo* pTargetBufferinfo = &(pTargetBuffer->bufferQueue[bufferIndex]);
            if (NULL != pTargetBufferinfo->pMetadata)
            {
                CHX_LOG_ERROR("Output metadata not synced with RDI frameNumber=%d",
                              frameNumber);
                ReleaseMetadataBuffer(pTargetBuffer, bufferIndex);
            }
            pTargetBufferinfo->pMetadata       = pMetadata;
            pTargetBufferinfo->isMetadataReady = TRUE;

            if (TRUE == pTargetBufferinfo->isBufferReady)
            {
                // Normally the lastReadySequenceID should equal with frameNumber - 1
                // Otherwise the RDI buffer will NOT be continouse
                if (((pTargetBuffer->lastReadySequenceID) == INVALIDSEQUENCEID) ||
                    ((pTargetBuffer->firstReadySequenceID) == INVALIDSEQUENCEID) ||
                    ((frameNumber - pTargetBuffer->lastReadySequenceID) > 1))
                {
                    pTargetBuffer->firstReadySequenceID = frameNumber;
                }
                else
                {
                    if ((pTargetBuffer->lastReadySequenceID - pTargetBuffer->firstReadySequenceID + 1) >=
                        pTargetBuffer->validBufferLength)
                    {
                        pTargetBuffer->firstReadySequenceID = pTargetBuffer->firstReadySequenceID + 1;
                    }
                }

                pTargetBuffer->lastReadySequenceID = frameNumber;
                pTargetBuffer->pCondition->Signal();
            }
            pReturnMetadata = pMetadata;
        }

        pTargetBuffer->pMutex->Unlock();
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
        result = CDKResultEFailed;
    }

    return pReturnMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::GetOutputBufferFromRDIQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::GetOutputBufferFromRDIQueue(
    UINT32              frameNumber,
    UINT32              pipelineIndex,
    CHISTREAMBUFFER*    pOutputbuffer)
{
    CDKResult       result        = CDKResultSuccess;
    UINT32          bufferIndex   = frameNumber % RDIBufferQueueDepth;
    TargetBuffer*   pTargetBuffer = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_RDIBufferID[pipelineIndex]);
        }
    }

    if (NULL == pOutputbuffer)
    {
        CHX_LOG_ERROR("pOutputBuffer is NULL");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        if (NULL != pTargetBuffer)
        {
            pTargetBuffer->pMutex->Lock();

            // If the buffer handle is not released, release it as this slot will be occupied for incoming request.
            // However, be careful with this situation. This is an indication that BufferQueueDepth is too small and
            // the slot is being overwritten due to some reasons (e.g. maxHalRequests is increased or previous
            // RDI results didn't come back).
            if (NULL != pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo.phBuffer)
            {
                CHX_LOG_ERROR("bufferQueue[%d] (phBuffer=%p, frameNumber=%d) is not empty and buffer is being released!" \
                    "current frameNumber=%d, pipelineIndex=%d",
                    bufferIndex,
                    pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo.phBuffer,
                    pTargetBuffer->bufferQueue[bufferIndex].frameNumber,
                    frameNumber,
                    pipelineIndex);

                pTargetBuffer->pBufferManager->ReleaseReference(
                    &(pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo));
                pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo.phBuffer = NULL;
            }

            // Reset frame number, buffer and metadata flag of the slot
            pTargetBuffer->bufferQueue[bufferIndex].frameNumber     = frameNumber;
            pTargetBuffer->bufferQueue[bufferIndex].isBufferReady   = FALSE;
            pTargetBuffer->bufferQueue[bufferIndex].isMetadataReady = FALSE;

            // Setup buffer handle for this slot
            pTargetBuffer->pMutex->Unlock();
            CHIBUFFERINFO bufferInfo = {};
            bufferInfo               = pTargetBuffer->pBufferManager->GetImageBufferInfo();
            pTargetBuffer->pMutex->Lock();

            pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo            = bufferInfo;
            pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->acquireFence.valid    = FALSE;
            CHX_LOG_INFO("bufferIndex:%d, bufferhandle:%p", bufferIndex, bufferInfo.phBuffer);
            ChxUtils::Memcpy(pOutputbuffer,
                             pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer,
                             sizeof(CHISTREAMBUFFER));
            pOutputbuffer->acquireFence.valid = FALSE;

            pTargetBuffer->pMutex->Unlock();
        }
        else
        {
            CHX_LOG_WARN("pTargetBuffer is NULL");
            result = CDKResultEFailed;
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::GetOutputBufferFromFDQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::GetOutputBufferFromFDQueue(
    UINT32                      frameNumber,
    UINT32                      pipelineIndex,
    CHISTREAMBUFFER*            pOutputbuffer)
{
    CDKResult       result = CDKResultSuccess;
    UINT32          bufferIndex = frameNumber % RDIBufferQueueDepth;
    TargetBuffer*   pTargetBuffer = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_FDBufferID[pipelineIndex]);
        }
    }

    if (NULL == pOutputbuffer)
    {
        CHX_LOG_ERROR("pOutputBuffer is NULL");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        if (NULL != pTargetBuffer)
        {
            pTargetBuffer->pMutex->Lock();

            // If the buffer handle is not released, release it as this slot will be occupied for incoming request.
            // However, be careful with this situation. This is an indication that BufferQueueDepth is too small and
            // the slot is being overwritten due to some reasons (e.g. maxHalRequests is increased or previous
            // RDI results didn't come back).
            if (NULL != pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo.phBuffer)
            {
                CHX_LOG_WARN("bufferQueue[%d] (phBuffer=%p, frameNumber=%d) is not empty and buffer is being released!" \
                    "current frameNumber=%d, pipelineIndex=%d",
                    bufferIndex,
                    pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo.phBuffer,
                    pTargetBuffer->bufferQueue[bufferIndex].frameNumber,
                    frameNumber,
                    pipelineIndex);

                pTargetBuffer->pBufferManager->ReleaseReference(
                    &(pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo));
                pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo.phBuffer = NULL;
            }

            // Reset frame number, buffer and metadata flag of the slot
            pTargetBuffer->bufferQueue[bufferIndex].frameNumber = frameNumber;
            pTargetBuffer->bufferQueue[bufferIndex].isBufferReady = FALSE;
            pTargetBuffer->bufferQueue[bufferIndex].isMetadataReady = FALSE;

            // Setup buffer handle for this slot
            pTargetBuffer->pMutex->Unlock();
            CHIBUFFERINFO bufferInfo = {};
            bufferInfo               = pTargetBuffer->pBufferManager->GetImageBufferInfo();
            pTargetBuffer->pMutex->Lock();

            pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo            = bufferInfo;
            pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->acquireFence.valid    = FALSE;

            ChxUtils::Memcpy(pOutputbuffer,
                             pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer,
                             sizeof(CHISTREAMBUFFER));
            pOutputbuffer->acquireFence.valid = FALSE;

            pTargetBuffer->pMutex->Unlock();
        }
        else
        {
            CHX_LOG_WARN("pTargetBuffer is NULL");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::WaitForBufferMetaReady
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::WaitForBufferMetaReady(
    UINT32 frameNumber,
    UINT32 pipelineIndex)
{
    CDKResult     result        = CDKResultSuccess;
    UINT32          bufferIndex = frameNumber % RDIBufferQueueDepth;
    TargetBuffer*   pTargetBuffer = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_RDIBufferID[pipelineIndex]);
        }
    }

    if (FlushStatus::NotFlushing != GetFlushStatus())
    {
        CHX_LOG_INFO("Flush is in progress, no need to wait for rdi in timed wait");
        result = CDKResultEFailed;
        return result;
    }
    else
    {
        if (NULL != pTargetBuffer)
        {
            pTargetBuffer->pMutex->Lock();
            while ((frameNumber > pTargetBuffer->bufferQueue[bufferIndex].frameNumber) ||
                   (FALSE == pTargetBuffer->bufferQueue[bufferIndex].isBufferReady) ||
                   (FALSE == pTargetBuffer->bufferQueue[bufferIndex].isMetadataReady))
            {
                result = pTargetBuffer->pCondition->TimedWait(pTargetBuffer->pMutex->GetNativeHandle(), TIMEOUT_BUFFER_WAIT);
                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("Wait for buffer and meta timedout! frameNumber:%d, bufferIndex:%d, lastReadySequenceID:%d, " \
                                  "(frameNumber: %d isBufferReady:%d isMetadataReady:%d)",
                                  frameNumber,
                                  bufferIndex,
                                  pTargetBuffer->lastReadySequenceID,
                                  pTargetBuffer->bufferQueue[bufferIndex].frameNumber,
                                  pTargetBuffer->bufferQueue[bufferIndex].isBufferReady,
                                  pTargetBuffer->bufferQueue[bufferIndex].isMetadataReady);
                    break;
                }
            }
            pTargetBuffer->pMutex->Unlock();
        }
        else
        {
            CHX_LOG_WARN("pTargetBuffer is NULL");
            result = CDKResultEFailed;
        }

        return result;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::CreateOfflineInputResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::CreateOfflineInputResource(
    UINT32 requestFrameNum,
    UINT32 pipelineIndex,
    BOOL   bUpdateValidLength)
{
    CDKResult     result                = CDKResultSuccess;
    const RequestMapInfo* pRequestInfo  = &(m_requestMapInfo[requestFrameNum % MaxOutstandingRequests]);
    UINT32 snapshotRequestID            = pRequestInfo->snapshotRequestID;
    UINT32 bufferIndex                  = 0;
    UINT32 inputFrameNumber             = 0;

    if ((INVALIDSEQUENCEID != snapshotRequestID) && (pipelineIndex < MaxRealTimePipelines))
    {
        OfflineInputInfo*  pInputInfo       = &m_snapshotInputInfo[snapshotRequestID % MaxOutstandingRequests];

        TargetBuffer*      pTargetRDIBuffer = GetTargetBufferPointer(m_RDIBufferID[pipelineIndex]);
        TargetBuffer*      pTargetFDBuffer  = GetTargetBufferPointer(m_FDBufferID[pipelineIndex]);

        if ((NULL != pTargetRDIBuffer) && (NULL != pTargetFDBuffer))
        {
            pTargetRDIBuffer->pMutex->Lock();

            for (UINT32 i = 0; i < pInputInfo->numOfBuffers; i++)
            {
                inputFrameNumber = pInputInfo->inputFrameNumber - i;
                bufferIndex      = inputFrameNumber % RDIBufferQueueDepth;
                if ((FALSE == pTargetRDIBuffer->bufferQueue[bufferIndex].isBufferReady) ||
                    (FALSE == pTargetRDIBuffer->bufferQueue[bufferIndex].isMetadataReady))
                {
                    CHX_LOG_ERROR("buffer or metadata (framenumber:%d,buffervalid:%d, metdatavalid:%d) is not ready!",
                                  requestFrameNum,
                                  pTargetRDIBuffer->bufferQueue[bufferIndex].isBufferReady,
                                  pTargetRDIBuffer->bufferQueue[bufferIndex].isMetadataReady);
                    result = CDKResultEFailed;
                    break;
                }

                if (CDKResultSuccess == result)
                {
                    if (NULL != pTargetRDIBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo.phBuffer)
                    {
                        ChxUtils::Memcpy(&(pInputInfo->RDIBufferArray[pipelineIndex][i]),
                                           pTargetRDIBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer,
                                           sizeof(CHISTREAMBUFFER));
                        pTargetRDIBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo.phBuffer = NULL;

                        if (NULL != pTargetRDIBuffer->pChiStream)
                        {
                            pInputInfo->RDIBufferArray[pipelineIndex][i].pParentSinkStreamPrivateInfo =
                                pTargetRDIBuffer->pChiStream->pPrivateInfo;
                        }

                        pInputInfo->RDIBufferArray[pipelineIndex][i].acquireFence.valid = FALSE;

                        if ((NULL != pRequestInfo->pFeature) &&
                            ((FeatureType::MFNR == pRequestInfo->pFeature->GetFeatureType()) ||
                             (FeatureType::MFSR == pRequestInfo->pFeature->GetFeatureType()) ||
                             (TRUE == pInputInfo->isFDStreamRequired)))
                        {
                            ChxUtils::Memcpy(&(pInputInfo->FDBufferArray[pipelineIndex][i]),
                                               pTargetFDBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer,
                                               sizeof(CHISTREAMBUFFER));
                            pTargetFDBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo.phBuffer = NULL;
                            pInputInfo->FDBufferArray[pipelineIndex][i].acquireFence.valid = FALSE;
                        }
                    }
                    else
                    {
                        result = CDKResultEFailed;
                        CHX_LOG_ERROR("the buffer handle (framenumber:%d,pipeline:%d) is released!!!!",
                                     (pInputInfo->inputFrameNumber - i), pipelineIndex);
                        break;
                    }
                }

                if (CDKResultSuccess == result)
                {
                    ChiMetadata*& pSnapshotInputMeta = pInputInfo->pRDIMetadata[pipelineIndex][i];

                    pSnapshotInputMeta = pTargetRDIBuffer->bufferQueue[bufferIndex].pMetadata;

                    // Set RDI metabuffer null to avoid being released when subsequent request is coming
                    pTargetRDIBuffer->bufferQueue[bufferIndex].pMetadata = NULL;

                    if ((NULL != pSnapshotInputMeta) && (NULL != pInputInfo->pAppMetadata))
                    {
                        pSnapshotInputMeta->Copy(*pInputInfo->pAppMetadata, TRUE);
                        ChxUtils::UpdateMetadataWithInputSettings(*pInputInfo->pAppMetadata, *pSnapshotInputMeta);

                        // Realtime pipeline output metadata will be used for OfflinePreview pipeline and
                        // Snapshot bayer2yuv pipeline input. Add reference to avoid bayer2yuv process
                        // null pointer issue
                        pSnapshotInputMeta->AddReference();
                    }

                    CHX_LOG_INFO("numOfBuffer:%d,index:%d,rdimetadata:%p,AppMetadata:%p,inputframenumber:%d, "
                                 "requestframenumber:%d,snapIdx:%d pipelineIndex = %d",
                                 pInputInfo->numOfBuffers, i,
                                 pSnapshotInputMeta, pInputInfo->pAppMetadata,
                                 (pInputInfo->inputFrameNumber - i),
                                 requestFrameNum, snapshotRequestID,
                                 pipelineIndex);
                }
            }

            // After pick RDI buffer from the queue, update first and last buffer framenumber
            if (CDKResultSuccess == result)
            {
                if (inputFrameNumber < pTargetRDIBuffer->lastReadySequenceID)
                {
                    pTargetRDIBuffer->firstReadySequenceID = inputFrameNumber + 1;
                }
                else
                {
                    pTargetRDIBuffer->lastReadySequenceID  = INVALIDSEQUENCEID;
                    pTargetRDIBuffer->firstReadySequenceID = INVALIDSEQUENCEID;
                }
            }

            pTargetRDIBuffer->pMutex->Unlock();

            if (CDKResultSuccess == result)
            {
                if (TRUE == bUpdateValidLength)
                {
                    // Make sure request buffer number is less than reserved buffer
                    if (pInputInfo->numOfBuffers <= pTargetRDIBuffer->validBufferLength)
                    {
                        pTargetRDIBuffer->pMutex->Lock();
                        pTargetRDIBuffer->validBufferLength -= pInputInfo->numOfBuffers;
                        pTargetRDIBuffer->pMutex->Unlock();

                        if ((NULL != pRequestInfo->pFeature) &&
                            ((FeatureType::MFNR == pRequestInfo->pFeature->GetFeatureType()) ||
                             (TRUE == pInputInfo->isFDStreamRequired)))
                        {
                            pTargetFDBuffer->pMutex->Lock();
                            pTargetFDBuffer->validBufferLength -= pInputInfo->numOfBuffers;
                            pTargetFDBuffer->pMutex->Unlock();
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("Reserved buffer length(%d) is less than required buffer(%d)!",
                            pTargetRDIBuffer->validBufferLength, pInputInfo->numOfBuffers);
                        result = CDKResultENoMore;
                    }
                }

                FlushRDIQueue(inputFrameNumber, pipelineIndex);
                FlushFDQueue(inputFrameNumber, pipelineIndex);
            }
            else
            {
                //if create buffer failed, release related resource.
                for (UINT32 i = 0; i < pInputInfo->numOfBuffers; i++)
                {
                    if (NULL != pInputInfo->pRDIMetadata[pipelineIndex][i])
                    {
                        pInputInfo->pRDIMetadata[pipelineIndex][i] = NULL;
                    }
                }
                CHX_LOG_ERROR("Create offline input resource faild!!!!!!");
            }
        }
        else
        {
            CHX_LOG_ERROR("Unable to get buffer pointer for both TargetRDIBuffer and TargetFDBuffer!");
            result = CDKResultEFailed;

            //if create buffer failed, release related resource.
            for (UINT32 i = 0 ; i < pInputInfo->numOfBuffers ; i++)
            {
                if (NULL != pInputInfo->pRDIMetadata[pipelineIndex][i])
                {
                    m_pMetadataManager->Release(pInputInfo->pRDIMetadata[pipelineIndex][i]);
                    pInputInfo->pRDIMetadata[pipelineIndex][i] = NULL;
                }
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid request framenumber:%d,pipeline:%d! There is no snapshot request information!",
                      requestFrameNum, pipelineIndex);
        result = CDKResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ReleaseReferenceToInputBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::ReleaseReferenceToInputBuffers(
    CHIPRIVDATA* pPrivData)
{
    // A offline request stores input buffers info in privData.
    // When result is received, it should call this API to release reference to those inputs.
    // TODO: consider merging ReleaseReferenceToInputBuffers and ReleaseSingleOffineInputResource API

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
            CHIBufferManager* const pBufferManager = reinterpret_cast<CHIBufferManager*>(pPrivData->bufferManagers[i]);
            if ((NULL != pBufferManager) && (NULL != pPrivData->inputBuffers[i].phBuffer))
            {
                pBufferManager->ReleaseReference(&(pPrivData->inputBuffers[i]));
                pPrivData->bufferManagers[i]        = NULL;
                pPrivData->inputBuffers[i].phBuffer = NULL;
            }
            else
            {
                CHX_LOG_WARN("numInputBuffers=%d, pPrivData->bufferManagers[%d]=%p, pPrivData->inputBuffers[%d].phBuffer=%p",
                              pPrivData->numInputBuffers,
                              i, pPrivData->bufferManagers[i],
                              i, pPrivData->inputBuffers[i].phBuffer);
                result = CDKResultEFailed;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ReleaseOffineInputResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::ReleaseSingleOffineInputResource(
    UINT32 requestFrameNum,
    UINT32 pipelineIndex,
    UINT32 bufferIndex)
{
    CDKResult     result         = CDKResultSuccess;
    RequestMapInfo* pRequestInfo = &(m_requestMapInfo[requestFrameNum % MaxOutstandingRequests]);
    UINT32 snapshotRequestID     = pRequestInfo->snapshotRequestID;

    if ((INVALIDSEQUENCEID != snapshotRequestID) && (pipelineIndex < MaxRealTimePipelines))
    {
        OfflineInputInfo* pInputInfo = &m_snapshotInputInfo[snapshotRequestID % MaxOutstandingRequests];

        if (bufferIndex < pInputInfo->numOfBuffers)
        {
            if (NULL != pInputInfo->pRDIMetadata[pipelineIndex][bufferIndex])
            {
                m_pMetadataManager->Release(pInputInfo->pRDIMetadata[pipelineIndex][bufferIndex]);
                pInputInfo->pRDIMetadata[pipelineIndex][bufferIndex] = NULL;
            }

            if (NULL != pInputInfo->RDIBufferArray[pipelineIndex][bufferIndex].bufferInfo.phBuffer)
            {
                m_MCTargetBuffer[m_RDIBufferID[pipelineIndex]].pBufferManager->ReleaseReference(
                    &pInputInfo->RDIBufferArray[pipelineIndex][bufferIndex].bufferInfo);
                pInputInfo->RDIBufferArray[pipelineIndex][bufferIndex].bufferInfo.phBuffer = NULL;
            }

            UpdateValidRDIBufferLength(pipelineIndex, 1);
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid request framenumber! There is no snapshot request information");
        result = CDKResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ReleaseOffineFDInputResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::ReleaseSingleOffineFDInputResource(
    UINT32 requestFrameNum,
    UINT32 pipelineIndex,
    UINT32 bufferIndex)
{
    CDKResult     result = CDKResultSuccess;
    RequestMapInfo* pRequestInfo = &(m_requestMapInfo[requestFrameNum % MaxOutstandingRequests]);
    UINT32 snapshotRequestID = pRequestInfo->snapshotRequestID;

    if ((INVALIDSEQUENCEID != snapshotRequestID) && (pipelineIndex < MaxRealTimePipelines))
    {
        OfflineInputInfo* pInputInfo = &m_snapshotInputInfo[snapshotRequestID % MaxOutstandingRequests];

        if (bufferIndex < pInputInfo->numOfBuffers)
        {
            if (NULL != pInputInfo->FDBufferArray[pipelineIndex][bufferIndex].bufferInfo.phBuffer)
            {
                m_MCTargetBuffer[m_FDBufferID[pipelineIndex]].pBufferManager->ReleaseReference(
                    &pInputInfo->FDBufferArray[pipelineIndex][bufferIndex].bufferInfo);
                pInputInfo->FDBufferArray[pipelineIndex][bufferIndex].bufferInfo.phBuffer = NULL;
            }

            UpdateValidFDBufferLength(pipelineIndex, 1);
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid request framenumber! There is no snapshot request information");
        result = CDKResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::GetValidBufferLength
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CameraUsecaseBase::GetValidBufferLength(
    UINT32 pipelineIndex)
{
    UINT32          validBufferLength   = 0;
    TargetBuffer*   pTargetBuffer = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_RDIBufferID[pipelineIndex]);
        }
    }

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();
        validBufferLength = pTargetBuffer->validBufferLength;
        pTargetBuffer->pMutex->Unlock();
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
    }

    return validBufferLength;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::GetPipelineIdFromCamId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CameraUsecaseBase::GetPipelineIdFromCamId(UINT32 physicalCameraId)
{
    UINT32 pipelineId = INVALID_INDEX;

    for (UINT32 i = 0; i < m_numOfPhysicalDevices; ++i)
    {
        if (physicalCameraId == m_cameraIdMap[i])
        {
            pipelineId = i;
            break;
        }
    }

    CHX_LOG_INFO("pipeline index %d for camera Id %d", pipelineId, physicalCameraId);

    return pipelineId;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::UpdateValidRDIBufferLength
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::UpdateValidRDIBufferLength(
    UINT32 pipelineIndex,
    INT32  delta)
{
    CDKResult       result          = CDKResultSuccess;
    TargetBuffer*   pTargetBuffer = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_RDIBufferID[pipelineIndex]);
        }
    }
    INT32           resultVal;

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();
        resultVal = pTargetBuffer->validBufferLength + delta;

        if ((resultVal < 0) || ((UINT32)resultVal > RDIBufferQueueDepth))
        {
            CHX_LOG_ERROR("Current validBufferLength = %d, delta = %d",
                pTargetBuffer->validBufferLength, delta);
            result = CDKResultEOutOfBounds;
        }

        if (CDKResultSuccess == result)
        {
            pTargetBuffer->validBufferLength = resultVal;
        }
        pTargetBuffer->pMutex->Unlock();
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::UpdateValidFDBufferLength
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::UpdateValidFDBufferLength(
    UINT32 pipelineIndex,
    INT32  delta)
{
    CDKResult       result = CDKResultSuccess;
    TargetBuffer*   pTargetBuffer = NULL;
    if (FALSE == IsMultiCameraUsecase())
    {
        pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
    }
    else
    {
        if (pipelineIndex < MaxChiStreams)
        {
            pTargetBuffer = GetTargetBufferPointer(m_FDBufferID[pipelineIndex]);
        }
    }
    INT32           resultVal;

    if (NULL != pTargetBuffer)
    {
        pTargetBuffer->pMutex->Lock();
        resultVal = pTargetBuffer->validBufferLength + delta;

        if ((resultVal < 0) || ((UINT32)resultVal > RDIBufferQueueDepth))
        {
            CHX_LOG_ERROR("Current validBufferLength = %d, delta = %d",
                pTargetBuffer->validBufferLength, delta);
            result = CDKResultEOutOfBounds;
        }

        if (CDKResultSuccess == result)
        {
            pTargetBuffer->validBufferLength = resultVal;
        }
        pTargetBuffer->pMutex->Unlock();
     }
     else
     {
         CHX_LOG_WARN("pTargetBuffer is NULL");
         result = CDKResultEFailed;
     }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::GetTargetBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::GetTargetBuffer(
    UINT32              frameNumber,
    TargetBuffer*       pTargetBuffer,
    ChiMetadata*        pSrcMetadata,
    CHISTREAMBUFFER*    pInputBuffer,
    ChiMetadata**       ppDstMetadata)
{
    CDKResult result = CDKResultSuccess;

    UINT32 bufferIndex = frameNumber % RDIBufferQueueDepth;

    pTargetBuffer->pMutex->Lock();

    ChxUtils::Memcpy(pInputBuffer,
                     pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer,
                     sizeof(CHISTREAMBUFFER));

    if (NULL != pTargetBuffer->pChiStream)
    {
        // Update the parent stream
        pInputBuffer->pParentSinkStreamPrivateInfo = pTargetBuffer->pChiStream->pPrivateInfo;
    }
    // Clear out this slot and the caller now has the ownership of this buffer
    pTargetBuffer->bufferQueue[bufferIndex].frameNumber                              = INVALIDSEQUENCEID;
    pTargetBuffer->bufferQueue[bufferIndex].pRdiOutputBuffer->bufferInfo.phBuffer    = NULL;
    pTargetBuffer->bufferQueue[bufferIndex].isBufferReady                            = FALSE;
    pTargetBuffer->bufferQueue[bufferIndex].isMetadataReady                          = FALSE;

    // update metadata
    if (NULL != ppDstMetadata)
    {
        ChiMetadata* pDstMetadata = pTargetBuffer->bufferQueue[bufferIndex].pMetadata;

#ifdef __ACU_DUMP_META__
        CHAR metaFileName[MaxFileLen];
        CdkUtils::SNPrintF(metaFileName, sizeof(metaFileName), "off_src_%d.txt", frameNumber);
        pSrcMetadata->DumpDetailsToFile(metaFileName);
        CdkUtils::SNPrintF(metaFileName, sizeof(metaFileName), "off_rdi_%d.txt", frameNumber);
        pDstMetadata->DumpDetailsToFile(metaFileName);
#endif

        if (NULL != pDstMetadata)
        {
            if (NULL != pSrcMetadata)
            {
                pDstMetadata->Merge(*pSrcMetadata, TRUE);
                ChxUtils::UpdateMetadataWithInputSettings(*pSrcMetadata, *pDstMetadata);
            }

            ChxUtils::UpdateMetadataWithSnapshotSettings(*pDstMetadata);

            CHX_LOG_INFO("Metadata dst %p into src %p to use for Snapshot frame %u",
                         pDstMetadata, pSrcMetadata, frameNumber);

#ifdef __ACU_DUMP_META__
            CdkUtils::SNPrintF(metaFileName, sizeof(metaFileName), "off_merge_%d.txt", frameNumber);
            pDstMetadata->DumpDetailsToFile(metaFileName);
#endif

            *ppDstMetadata = pDstMetadata;
        }
        else
        {
            *ppDstMetadata = NULL;
            CHX_LOG_ERROR("Fatal error: Taking snapshot without waiting for Metadata to be available for %u",
                frameNumber);
            result = CDKResultEFailed;
        }
    }

    pTargetBuffer->pMutex->Unlock();

    pInputBuffer->size = sizeof(CHISTREAMBUFFER);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::GetZSLMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::GetZSLMode(
    UINT32*                  zslMode,
    ChiMetadata*             pMetadata)
{

    *zslMode = ChxUtils::GetZSLMode(pMetadata);
    CHX_LOG("ZslMode value %d", *zslMode);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::IsFlashRequired
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CameraUsecaseBase::IsFlashRequired(
    ChiMetadata&  metadata)
{
    // If the snapshot is captured with Flash or Manual 3A gains
    BOOL   isFlashRequired = FALSE;

    UINT32 tagId = ExtensionModule::GetInstance()->GetVendorTagId(VendorTag::IsFlashRequiredTag);

    UINT8* pFlashRequired = static_cast<UINT8*>(metadata.GetTag(tagId));

    if (NULL != pFlashRequired)
    {
        isFlashRequired = (1 == *pFlashRequired) ? TRUE : FALSE;
        CHX_LOG("isFlashRequired value %d", isFlashRequired);
    }

    return isFlashRequired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::NeedMetadataUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CameraUsecaseBase::NeedMetadataUpdate(
    camera3_capture_request_t* pRequest)
{
    bool needUpdate = true;

    if (TRUE == IsBatchModeEnabled())
    {
        INT32  minFps;
        INT32  maxFps;
        UINT32 numBatchedFrames = ExtensionModule::GetInstance()->GetNumBatchedFrames();

        if ((NULL != pRequest->settings) &&
            (CDKResultSuccess == ChxUtils::AndroidMetadata::GetFpsRange(pRequest->settings, &minFps, &maxFps)))
        {
            if (minFps == maxFps)
            {
                if (!m_isRequestBatchingOn)
                {
                    m_batchRequestStartIndex = pRequest->frame_number;
                    m_batchRequestEndIndex   = UINT32_MAX;
                    m_isRequestBatchingOn    = true;
                }
            }
            else
            {
                if (m_isRequestBatchingOn)
                {
                    m_batchRequestEndIndex = pRequest->frame_number;
                    m_isRequestBatchingOn  = false;
                }
            }
        }

        needUpdate = (0 == (pRequest->frame_number - m_batchRequestStartIndex) % numBatchedFrames);
        CHX_LOG_INFO("Batching enabled %d num_batched_frames %d frameNum %u needUpdate %d",
                     m_isRequestBatchingOn,
                     numBatchedFrames,
                     pRequest->frame_number,
                     needUpdate);
    }

    return needUpdate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::UpdateMetadataBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::UpdateMetadataBuffers(
    camera3_capture_request_t* pSrcRequest,
    UINT32                     pipelineId,
    CHICAPTUREREQUEST*         pDstRequest,
    UINT32                     sessionIndex,
    UINT32                     pipelineIndex,
    BOOL                       bIsSticky)
{
    CDKResult result = CDKResultEFailed;

    if (NeedMetadataUpdate(pSrcRequest))
    {
        ChiMetadata* pChiInputMetadata = m_pMetadataManager->GetInput(
            pSrcRequest->settings,
            pSrcRequest->frame_number,
            bIsSticky);

        if (NULL != pChiInputMetadata)
        {
            ChiMetadata* pChiOutputMetadata = m_pMetadataManager->Get(
                m_metadataClients[pipelineId],
                pSrcRequest->frame_number);

            if (NULL != pChiOutputMetadata)
            {
                pDstRequest->pInputMetadata                   = pChiInputMetadata->GetHandle();
                pDstRequest->pOutputMetadata                  = pChiOutputMetadata->GetHandle();

                if (NULL == m_sessions[sessionIndex].pSession->GetSensorModeInfo(pipelineIndex))
                {
                    CHX_LOG_ERROR("Sensor mode info is Null");
                }
                else
                {
                    ChxUtils::FillTuningModeData(pChiInputMetadata,
                                                 pSrcRequest,
                                                 m_sessions[sessionIndex].pSession->GetSensorModeInfo(pipelineIndex)->modeIndex,
                                                 &m_effectModeValue,
                                                 &m_sceneModeValue,
                                                 &m_tuningFeature1Value,
                                                 &m_tuningFeature2Value);

                    ChxUtils::FillCameraId(pChiInputMetadata, GetCameraId());

                    m_pBatchMetadataPair[0] = pChiInputMetadata;
                    m_pBatchMetadataPair[1] = pChiOutputMetadata;

                    result = CDKResultSuccess;
                }
            }
            else
            {
                CHX_LOG_ERROR("[CM_ERROR] Unable to get output metadata buffer for framenumber: %u",
                              pSrcRequest->frame_number);
                m_pMetadataManager->PrintAllBuffers(m_metadataClients[pipelineId]);
            }
        }
        else
        {
            CHX_LOG_ERROR("[CMB_ERROR] Unable to get input metadata buffer for framenumber: %u",
                          pSrcRequest->frame_number);
            m_pMetadataManager->PrintAllBuffers();
        }
    }
    else if ((NULL != m_pBatchMetadataPair[0]) && (NULL != m_pBatchMetadataPair[1]))
    {
        // batchmode
        pDstRequest->pInputMetadata                   = m_pBatchMetadataPair[0]->GetHandle();
        pDstRequest->pOutputMetadata                  = m_pBatchMetadataPair[1]->GetHandle();
        m_pBatchMetadataPair[0]->AddReference();
        m_pBatchMetadataPair[1]->AddReference();
        CHX_LOG_INFO("[CMB_DEBUG] Skip metadata for framenumber: %u %p %p",
                     pSrcRequest->frame_number,
                     m_pBatchMetadataPair[0],
                     m_pBatchMetadataPair[1]);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ResetMetadataStatus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::ResetMetadataStatus(
    camera3_capture_request_t* pRequest)
{
    UINT frameIndex                   = pRequest->frame_number % MaxOutstandingRequests;
    m_isMetadataSent[frameIndex]      = FALSE;
    m_isMetadataAvailable[frameIndex] = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ExecuteCaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::ExecuteCaptureRequest(
    camera3_capture_request_t* pRequest)
{
    // Base implementation finds the buffers that go to each output and invokes SubmitRequest for each pipeline with outputs
    // If the advanced class wishes to use this function, but not invoke all the pipelines, the output produced from the disired
    // inactive pipeline should be removed from the pRequest->output_buffers
    CDKResult result = CDKResultSuccess;

    CHX_LOG("CameraUsecaseBase::ExecuteCaptureRequest for frame %d with %d output buffers",
        pRequest->frame_number, pRequest->num_output_buffers);

    static const UINT32 NumOutputBuffers = 5;

    UINT frameIndex = pRequest->frame_number % MaxOutstandingRequests;

    if (InvalidId != m_rtSessionIndex)
    {
        UINT32 rtPipeline = m_sessions[m_rtSessionIndex].rtPipelineIndex;

        if (InvalidId != rtPipeline)
        {
            m_selectedSensorModeIndex =
                m_sessions[m_rtSessionIndex].pipelines[rtPipeline].pPipeline->GetSensorModeInfo()->modeIndex;
            result = UpdateSensorModeIndex(const_cast<camera_metadata_t*>(pRequest->settings));
        }
    }

    for (UINT session = 0; session < MaxSessions; session++)
    {
        BOOL bIsOffline = FALSE;

        for (UINT pipeline = 0; pipeline < m_sessions[session].numPipelines; pipeline++)
        {
            if (NULL != pRequest->input_buffer)
            {
                bIsOffline = TRUE;

                result = WaitForDeferThread();

                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("Defer thread failure");
                    break;
                }

                // Skip submitting to realtime pipelines when an input buffer is provided
                if (TRUE == m_sessions[session].pipelines[pipeline].pPipeline->IsRealTime())
                {
                    continue;
                }
            }
            else
            {
                // Skip submitting to offline pipelines when an input buffer is not provided
                if (FALSE == m_sessions[session].pipelines[pipeline].pPipeline->IsRealTime())
                {
                    continue;
                }
            }

            CHISTREAMBUFFER outputBuffers[NumOutputBuffers] = { { 0 } };
            UINT32          outputCount                     = 0;
            PipelineData*   pPipelineData                   = &m_sessions[session].pipelines[pipeline];

            for (UINT32 buffer = 0; buffer < pRequest->num_output_buffers; buffer++)
            {
                for (UINT stream = 0; stream < pPipelineData->numStreams; stream++)
                {
                    if ( (TRUE == bIsOffline) &&
                        (FALSE == m_sessions[session].pipelines[pipeline].pPipeline->IsRealTime()) &&
                        (TRUE  == m_bCloningNeeded) )
                    {
                        UINT index = 0;
                        if (TRUE == IsThisClonedStream(m_pClonedStream, pPipelineData->pStreams[stream], &index))
                        {
                            if ((reinterpret_cast<CHISTREAM*>(pRequest->output_buffers[buffer].stream) ==
                                m_pFrameworkOutStreams[index]))
                            {
                                ChxUtils::PopulateHALToChiStreamBuffer(&pRequest->output_buffers[buffer],
                                                                       &outputBuffers[outputCount]);
                                outputBuffers[outputCount].pStream = pPipelineData->pStreams[stream];
                                outputCount++;
                            }
                        }
                    }
                    else
                    {
                        if (reinterpret_cast<CHISTREAM*>(pRequest->output_buffers[buffer].stream) ==
                            pPipelineData->pStreams[stream])
                        {
                            ChxUtils::PopulateHALToChiStreamBuffer(&pRequest->output_buffers[buffer],
                                                                   &outputBuffers[outputCount]);
                            outputCount++;
                        }
                    }
                }
            }

            if (0 < outputCount)
            {
                CHICAPTUREREQUEST request       = { 0 };
                CHISTREAMBUFFER   inputBuffer   = { 0 };
                UINT32            sensorModeIndex;

                if (NULL != pRequest->input_buffer)
                {
                    request.numInputs     = 1;
                    ChxUtils::PopulateHALToChiStreamBuffer(pRequest->input_buffer, &inputBuffer);
                    request.pInputBuffers = &inputBuffer;
                }

                request.frameNumber       = pRequest->frame_number;
                request.hPipelineHandle   = reinterpret_cast<CHIPIPELINEHANDLE>(
                                                m_sessions[session].pSession->GetPipelineHandle());
                request.numOutputs        = outputCount;
                request.pOutputBuffers    = outputBuffers;
                request.pPrivData         = &m_privData[frameIndex];

                UpdateMetadataBuffers(pRequest, pPipelineData->id, &request, session, pipeline, !bIsOffline);

                CHIPIPELINEREQUEST submitRequest = { 0 };
                submitRequest.pSessionHandle     = reinterpret_cast<CHIHANDLE>(
                                                        m_sessions[session].pSession->GetSessionHandle());
                submitRequest.numRequests        = 1;
                submitRequest.pCaptureRequests   = &request;

                UINT32 numPCRsBeforeStreamOn = ExtensionModule::GetInstance()->GetNumPCRsBeforeStreamOn();

                if (1 > numPCRsBeforeStreamOn)
                {
                    // Activate pipeline before submitting request when EarlyPCR disabled
                    CheckAndActivatePipeline(m_sessions[session].pSession);
                }

                CHX_LOG("Submitting request to Session %d Pipeline %d outputCount=%d", session, pipeline, outputCount);

                CHX_LOG_REQMAP("frame: %u  <==>  (chiFrameNum) chiOverrideFrameNum: %" PRIu64,
                               GetAppFrameNum(request.frameNumber),
                               request.frameNumber);

                result = SubmitRequest(&submitRequest);

                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("Submit request failure for session:%d", session);
                    break;
                }

                if (0 < numPCRsBeforeStreamOn)
                {
                    // Activate pipeline after submitting request when EarlyPCR enabled
                    CheckAndActivatePipeline(m_sessions[session].pSession);
                }

            }
        }

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Defer thread or submit request failure for session:%d", session);
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::UpdateSnapshotMetadataWithRDITags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::UpdateSnapshotMetadataWithRDITags(
    ChiMetadata& rInputMetadata,
    ChiMetadata& rOutputMetadata)
{
    UINT32 rdiTags[] = {
        ANDROID_SENSOR_EXPOSURE_TIME,
        ANDROID_LENS_FOCAL_LENGTH,
        ANDROID_SENSOR_SENSITIVITY,
        ANDROID_CONTROL_POST_RAW_SENSITIVITY_BOOST,
        ANDROID_CONTROL_AWB_MODE,
        ANDROID_FLASH_STATE,
        ANDROID_FLASH_MODE,
        ANDROID_CONTROL_AE_MODE,
        ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
        ANDROID_CONTROL_AE_COMPENSATION_STEP
    };

    UINT32 rdiTagCount[] = {
        1, /*ANDROID_SENSOR_EXPOSURE_TIME */
        1, /*ANDROID_LENS_FOCAL_LENGTH*/
        1, /*ANDROID_SENSOR_SENSITIVITY*/
        1, /*ANDROID_CONTROL_POST_RAW_SENSITIVITY_BOOST*/
        1, /*ANDROID_CONTROL_AWB_MODE*/
        1, /*ANDROID_FLASH_STATE*/
        1, /*ANDROID_FLASH_MODE*/
        1, /*ANDROID_CONTROL_AE_MODE*/
        1, /*ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION*/
        1, /*ANDROID_CONTROL_AE_COMPENSATION_STEP*/
    };

    for (UINT32 index = 0; index < CHX_ARRAY_SIZE(rdiTags); ++index)
    {
        VOID* pTagData = static_cast<VOID*>(rInputMetadata.GetTag(rdiTags[index]));

        if (NULL != pTagData)
        {
            rOutputMetadata.SetTag(rdiTags[index], pTagData, rdiTagCount[index]);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::GetXMLUsecaseByName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiUsecase* CameraUsecaseBase::GetXMLUsecaseByName(const CHAR* usecaseName)
{
    ChiUsecase* pUsecase   = NULL;
    UINT32      numTargets = 0;

    CHX_LOG("E. usecaseName:%s", usecaseName);

    numTargets = sizeof(PerNumTargetUsecases) / sizeof(PerNumTargetUsecases[0]);

    for (UINT32 i = 0; i < numTargets; i++)
    {
        if (0 < PerNumTargetUsecases[i].numUsecases)
        {
            ChiUsecase* pUsecasePerTarget = PerNumTargetUsecases[i].pChiUsecases;

            for (UINT32 index = 0; index < PerNumTargetUsecases[i].numUsecases; index++)
            {
                if (0 == strcmp(usecaseName, pUsecasePerTarget[index].pUsecaseName))
                {
                    pUsecase = &pUsecasePerTarget[index];
                    break;
                }
            }
        }
    }

    CHX_LOG("pUsecase:%p", pUsecase);
    return pUsecase;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::SessionCbNotifyMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::SessionCbNotifyMessage(
    const ChiMessageDescriptor* pMessageDescriptor,
    VOID*                       pPrivateCallbackData)
{
    /// @todo Provide common Get functions to do the below two casts
    SessionPrivateData* pSessionPrivateData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
    CameraUsecaseBase*  pCameraUsecase      = static_cast<CameraUsecaseBase*>(pSessionPrivateData->pUsecase);

    pCameraUsecase->SessionProcessMessage(pMessageDescriptor, pSessionPrivateData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::SessionCbCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::SessionCbCaptureResult(
    ChiCaptureResult* pCaptureResult,
    VOID*             pPrivateCallbackData)
{
    SessionPrivateData* pSessionPrivateData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
    CameraUsecaseBase*  pCameraUsecase      = static_cast<CameraUsecaseBase*>(pSessionPrivateData->pUsecase);

    for (UINT stream = 0; stream < pCaptureResult->numOutputBuffers; stream++)
    {
        UINT index = 0;
        if (TRUE == IsThisClonedStream(pCameraUsecase->m_pClonedStream, pCaptureResult->pOutputBuffers[stream].pStream, &index))
        {
            CHISTREAMBUFFER* pTempStreamBuffer  = const_cast<CHISTREAMBUFFER*>(&pCaptureResult->pOutputBuffers[stream]);
            pTempStreamBuffer->pStream          = pCameraUsecase->m_pFrameworkOutStreams[index];
        }
    }

    pCameraUsecase->SessionProcessResult(pCaptureResult, pSessionPrivateData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ProcessCHIPartialData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::ProcessCHIPartialData(UINT32 frameNum)
{
    camera3_capture_result_t*   pCHIPartialResult    = NULL;
    UINT32                      resultFrameIndex     = ChxUtils::GetResultFrameIndexChi(frameNum);
    PartialResultSender         sender               =
        PartialResultSender::CHIPartialData;

    if (TRUE == CheckIfPartialDataCanBeSent(sender, resultFrameIndex))
    {
        pCHIPartialResult = GetCHIPartialCaptureResult(resultFrameIndex);

        pCHIPartialResult->frame_number          = frameNum;
        pCHIPartialResult->input_buffer          = NULL;
        pCHIPartialResult->output_buffers        = NULL;
        pCHIPartialResult->partial_result        = static_cast<int>(ChxUtils::GetPartialResultCount(
            PartialResultSender::CHIPartialData));
        pCHIPartialResult->num_output_buffers    = 0;
        // This needs to be populated as per requirements by the feature. This is kept intentionally
        // as empty for reference.
        pCHIPartialResult->result                = static_cast<camera_metadata*>(m_pEmptyMetaData);

        ProcessAndReturnPartialMetadataFinishedResults(sender);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::SessionCbPartialCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::SessionCbPartialCaptureResult(
    CHIPARTIALCAPTURERESULT* pPartialCaptureResult, ///< Partial Capture result
    VOID*                    pPrivateCallbackData)
{
    SessionPrivateData* pSessionPrivateData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
    CameraUsecaseBase*  pCameraUsecase      = static_cast<CameraUsecaseBase*>(pSessionPrivateData->pUsecase);

    pCameraUsecase->SessionPartialCaptureProcessResult(pPartialCaptureResult, pSessionPrivateData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::SessionProcessMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::SessionProcessMessage(
    const ChiMessageDescriptor* pMessageDescriptor,
    const SessionPrivateData*   pSessionPrivateData)
{
    (VOID*)pSessionPrivateData;

    if (ChiMessageTypeSof == pMessageDescriptor->messageType)
    {
        const CHISOFMESSAGE& sofMessage = pMessageDescriptor->message.sofMessage;
        // SOF notifications are not sent to the HAL3 application
        CHX_LOG("Chi Notify SOF frameNum %u framework frameNum %u, timestamp %" PRIu64,
            sofMessage.sofId,
            sofMessage.frameworkFrameNum,
            sofMessage.timestamp);
    }
    else if (ChiMessageTypeShutter == pMessageDescriptor->messageType)
    {
        const CHISHUTTERMESSAGE& shutterMessage = pMessageDescriptor->message.shutterMessage;
        // SOF notifications are not sent to the HAL3 application
        CHX_LOG("Chi Notify Shutter framework frameNum %u, timestamp %" PRIu64,
            shutterMessage.frameworkFrameNum,
            shutterMessage.timestamp);
        m_shutterTimestamp[shutterMessage.frameworkFrameNum%MaxOutstandingRequests] = shutterMessage.timestamp;
        ReturnFrameworkMessage((camera3_notify_msg_t*)pMessageDescriptor, m_cameraId);
    }
    else if (ChiMessageTypeError == pMessageDescriptor->messageType)
    {
        ProcessErrorMessage(pMessageDescriptor);
    }
    else if (ChiMessageTypeMetaBufferDone == pMessageDescriptor->messageType)
    {
        CHX_LOG("CameraUsecaseBase Chi Notify metadata done for framework frameNum %u i/p metadata %p o/p metadata %p",
            pMessageDescriptor->message.metaBufferDoneMessage.frameworkFrameNum,
            pMessageDescriptor->message.metaBufferDoneMessage.inputMetabuffer,
            pMessageDescriptor->message.metaBufferDoneMessage.outputMetabuffer);
    }
    else
    {
        ReturnFrameworkMessage((camera3_notify_msg_t*)pMessageDescriptor, m_cameraId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::SessionPartialCaptureProcessResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::SessionPartialCaptureProcessResult(
    ChiPartialCaptureResult*    pPartialCaptureResult,     ///< Message Descriptor
    const SessionPrivateData*   pSessionPrivateData)       ///< Private callback data
{
    camera3_capture_result_t*   pDriverPartialResult    = NULL;
    ChiMetadata*                pChiOutputMetadata      = NULL;
    UINT32                      resultFrameNum          = pPartialCaptureResult->frameworkFrameNum;
    UINT32                      resultFrameIndex        = ChxUtils::GetResultFrameIndexChi(resultFrameNum);
    PartialResultSender         sender                  =
        PartialResultSender::DriverPartialData;

    if ((GetFinalPipelineForPartialMetaData() == pSessionPrivateData->sessionId) &&
        (TRUE == CheckIfPartialDataCanBeSent(sender, resultFrameIndex)))
    {
        pChiOutputMetadata = m_pMetadataManager->GetMetadataFromHandle(pPartialCaptureResult->pPartialResultMetadata);

        if (PartialMetaSupport::CombinedPartialMeta == ExtensionModule::GetInstance()->EnableCHIPartialData())
        {
            ProcessCHIPartialData(pPartialCaptureResult->frameworkFrameNum);
        }

        UpdateAppPartialResultMetadataFromDriver(pChiOutputMetadata,
                                                 resultFrameIndex,
                                                 resultFrameNum,
                                                 GetMetadataClientIdFromPipeline(pSessionPrivateData->sessionId, 0));
        ProcessAndReturnPartialMetadataFinishedResults(sender);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::PrepareHFRTagFilterList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::PrepareHFRTagFilterList(UINT32 clientId)
{
    static const struct ChiVendorTagNames HFRFilterVendorTagArray[] =
    {
        { "org.codeaurora.qcamera3.bayer_exposure",   "r_stats"                },
        { "org.codeaurora.qcamera3.bayer_exposure",   "g_stats"                },
        { "org.codeaurora.qcamera3.bayer_exposure",   "b_stats"                },
        { "org.codeaurora.qcamera3.bayer_grid",       "r_stats"                },
        { "org.codeaurora.qcamera3.bayer_grid",       "g_stats"                },
        { "org.codeaurora.qcamera3.bayer_grid",       "b_stats"                },
        { "org.codeaurora.qcamera3.histogram",        "stats"                  },
        { "com.qti.stats.af",                         "PDMapTag"               },
        { "com.qti.stats.af",                         "ConfidenceMapTag"       },
        { "com.qti.stats.af",                         "DefocusRangeNearMapTag" },
        { "com.qti.stats.af",                         "DefocusRangeFarMapTag"  },
        { "com.qti.stats.af",                         "DistanceMapTag"         },
        { "com.qti.stats.af",                         "AFSelectionMapTag"      },
        { "com.qti.stats.af",                         "DefocusMapTag"          },
        { "org.quic.camera2.jpegquantizationtables",  "OEMJPEGLumaQuantizationTable"},
        { "org.quic.camera2.jpegquantizationtables",  "OEMJPEGChromaQuantizationTable"},
        { "org.quic.camera.cvpMetaData",              "CVPMetaData"            },
    };

    m_hfrTagFilter.reserve(MaximumChiPipelineTagsPublished);
    m_hfrTagFilterWithPMD.reserve(MaximumChiPipelineTagsPublished);

    for (UINT index = 0; index < CHX_ARRAY_SIZE(HFRFilterVendorTagArray); ++index)
    {
        UINT32                   tagID;
        const ChiVendorTagNames* pNames = &HFRFilterVendorTagArray[index];

        if (CDKResultSuccess == m_vendorTagOps.pQueryVendorTagLocation(pNames->pComponentName, pNames->pTagName, &tagID))
        {
            m_hfrTagFilterWithPMD.push_back(tagID);
        }
    }

    UINT32* partialTagArray = m_pMetadataManager->RetrievePartialTags(clientId);
    UINT32  partialTagCount = m_pMetadataManager->RetrievePartialTagCount(clientId);

    if ((0 < partialTagCount) && (NULL != partialTagArray))
    {
        std::copy(m_hfrTagFilter.begin(), m_hfrTagFilter.end(), std::back_inserter(m_hfrTagFilterWithPMD));
        for (UINT index = 0; index < partialTagCount; ++index)
        {
            if (std::find(m_hfrTagFilter.begin(), m_hfrTagFilter.end(), partialTagArray[index]) == m_hfrTagFilter.end())
            {
                m_hfrTagFilterWithPMD.push_back(partialTagArray[index]);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::HandleBatchModeResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::HandleBatchModeResult(
   ChiCaptureResult* pResult,
   ChiMetadata*      pChiOutputMetadata,
   UINT32            resultId,
   UINT32            clientId)
{
    CDKResult result           = CDKResultSuccess;
    UINT32    numBatchedFrames = ExtensionModule::GetInstance()->GetNumBatchedFrames();
    UINT32    batchFrameIndex  = (pResult->frameworkFrameNum - m_batchRequestStartIndex) % numBatchedFrames;
    bool      isStartOfBatch   = (0 == batchFrameIndex);
    bool      isEndOfBatch     = ((numBatchedFrames - 1) == batchFrameIndex);

    camera_metadata_t* pFrameworkOutput = NULL;

    if (isStartOfBatch)
    {
        m_batchFrameworkOutput = m_pMetadataManager->GetAndroidFrameworkOutputMetadata(false);

        if (NULL != m_batchFrameworkOutput)
        {
            UINT32  partialTagCount = 0;
            UINT32* pPartialTagList = NULL;

            // Get the Partial Tag ID list if Partial Data has already been sent
            if (FALSE == CheckIfPartialDataCanBeSent(PartialResultSender::DriverPartialData, resultId))
            {
                partialTagCount = m_pMetadataManager->RetrievePartialTagCount(clientId);
                pPartialTagList = m_pMetadataManager->RetrievePartialTags(clientId);

                CHX_LOG_VERBOSE("Partial Data has been sent and meta data needs to be filtered for tags=%d, %d",
				partialTagCount, clientId);
            }
            else
            {
                partialTagCount = m_hfrTagFilter.size();
                pPartialTagList = m_hfrTagFilter.data();
                CHX_LOG_VERBOSE("Number of meta data needs to be filtered = %d", partialTagCount);
            }

            pChiOutputMetadata->TranslateToCameraMetadata(m_batchFrameworkOutput,
                TRUE,
                TRUE,
                partialTagCount,
                pPartialTagList);
        }
        else
        {
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        if (isEndOfBatch)
        {
            pFrameworkOutput       = m_batchFrameworkOutput;
            m_batchFrameworkOutput = NULL;
        }
        else
        {
            // Get sparse metadata to generate output except for the last frame in the batch
            pFrameworkOutput = m_pMetadataManager->GetAndroidFrameworkOutputMetadata(true);
        }
    }

    if ((CDKResultSuccess == result) && (NULL != pFrameworkOutput))
    {
        m_captureResult[resultId].result = pFrameworkOutput;

        ChxUtils::AndroidMetadata::UpdateTimeStamp(
            pFrameworkOutput,
            m_shutterTimestamp[pResult->frameworkFrameNum%MaxOutstandingRequests],
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
/// CameraUsecaseBase::SessionProcessResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::SessionProcessResult(
    ChiCaptureResult*         pResult,
    const SessionPrivateData* pSessionPrivateData)
{
    CHX_LOG("CameraUsecaseBase::SessionProcessResult for frame: %u", pResult->frameworkFrameNum);

    CDKResult           result                  = CDKResultSuccess;
    UINT32              resultFrameNum          = pResult->frameworkFrameNum;
    UINT32              resultFrameIndex        = resultFrameNum % MaxOutstandingRequests;
    BOOL                isAppResultsAvailable   = FALSE;

    camera3_capture_result_t* pUsecaseResult     = GetCaptureResult(resultFrameIndex);

    pUsecaseResult->frame_number = resultFrameNum;

    // Fill all the info in m_captureResult and call ProcessAndReturnFinishedResults to send the meta
    // callback in sequence
    m_pAppResultMutex->Lock();
    for (UINT i = 0; i < pResult->numOutputBuffers; i++)
    {
        camera3_stream_buffer_t* pResultBuffer =
            const_cast<camera3_stream_buffer_t*>(&pUsecaseResult->output_buffers[i + pUsecaseResult->num_output_buffers]);

        ChxUtils::PopulateChiToHALStreamBuffer(&pResult->pOutputBuffers[i], pResultBuffer);
        isAppResultsAvailable = TRUE;
    }
    pUsecaseResult->num_output_buffers += pResult->numOutputBuffers;
    m_pAppResultMutex->Unlock();

    if (NULL != &pResult->pInputBuffer[0])
    {
        camera3_stream_buffer_t* pResultInBuffer =
            const_cast<camera3_stream_buffer_t*>(pUsecaseResult->input_buffer);

        ChxUtils::PopulateChiToHALStreamBuffer(pResult->pInputBuffer, pResultInBuffer);
        isAppResultsAvailable = TRUE;
    }

    if ((NULL != pResult->pInputMetadata) && (NULL != pResult->pOutputMetadata))
    {

        ChiMetadata* pChiInputMetadata  = m_pMetadataManager->GetMetadataFromHandle(pResult->pInputMetadata);
        ChiMetadata* pChiOutputMetadata = m_pMetadataManager->GetMetadataFromHandle(pResult->pOutputMetadata);

        if ((pResult->frameworkFrameNum >= m_batchRequestStartIndex) &&
            (pResult->frameworkFrameNum <= m_batchRequestEndIndex))
        {
            result = HandleBatchModeResult(pResult,
                                           pChiOutputMetadata,
                                           resultFrameIndex,
                                           GetMetadataClientIdFromPipeline(pSessionPrivateData->sessionId, 0));
        }
        else
        {
            if ((CDKResultSuccess == result) && (FALSE == m_isMetadataAvailable[resultFrameIndex]) &&
                (FALSE == m_isMetadataSent[resultFrameIndex]))
            {
                result = Usecase::UpdateAppResultMetadata(pChiOutputMetadata,
                                                          resultFrameIndex,
                                                          GetMetadataClientIdFromPipeline(pSessionPrivateData->sessionId, 0));
                if (CDKResultSuccess == result)
                {
                    SetMetadataAvailable(resultFrameIndex);
                }
            }
        }

        if (CDKResultSuccess == result)
        {
            // release the buffers
            m_pMetadataManager->Release(pChiOutputMetadata);

            m_pMetadataManager->Release(pChiInputMetadata);

            CHX_LOG("Released output metadata buffer for session id: %d: %d",
                pSessionPrivateData->sessionId, result);

            pUsecaseResult->partial_result = pResult->numPartialMetadata;
            isAppResultsAvailable = TRUE;
        }
    }

    if (TRUE == isAppResultsAvailable)
    {
        ProcessAndReturnFinishedResults();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ProcessAndReturnFinishedResults
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::ProcessAndReturnFinishedResults()
{
    m_pAppResultMutex->Lock();
    for (INT64 resultFrame = m_nextAppResultFrame; resultFrame <= m_lastAppRequestFrame; resultFrame++)
    {
        UINT frameIndex        = (resultFrame % MaxOutstandingRequests);
        BOOL metadataAvailable = ((NULL != m_captureResult[frameIndex].result)         &&
                                  (0    != m_captureResult[frameIndex].partial_result) &&
                                  (TRUE == m_isMetadataAvailable[frameIndex])) ? TRUE : FALSE;
        BOOL metadataReturn    = ((TRUE                         == metadataAvailable) &&
                                  (m_lastResultMetadataFrameNum == (resultFrame - 1))) ? TRUE : FALSE;
        BOOL bufferReturn      = (0 < m_captureResult[frameIndex].num_output_buffers &&
                                  0 < m_numAppPendingOutputBuffers[frameIndex]) ? TRUE : FALSE;
        BOOL inBufferReturn    = FALSE;

        if (NULL != m_captureResult[frameIndex].input_buffer)
        {
            inBufferReturn    = (NULL == m_captureResult[frameIndex].input_buffer->stream) ? FALSE : TRUE;
        }

        CHX_LOG("Frame %" PRIu64 ": metadataAvailable: %d, metadataReturn: %d, bufferReturn %d, inBufferReturn %d",
                resultFrame, metadataAvailable, metadataReturn, bufferReturn, inBufferReturn);

        if ((TRUE == bufferReturn) || (TRUE == metadataReturn))
        {
            CHX_ASSERT(0 < m_numAppPendingOutputBuffers[frameIndex]);

            camera3_capture_result_t result = { 0 };

            if (TRUE == metadataReturn)
            {
                result.frame_number             = m_captureResult[frameIndex].frame_number;
                result.partial_result           =
                    static_cast<int>(ChxUtils::GetPartialResultCount(PartialResultSender::DriverMetaData));
                result.result                   = m_captureResult[frameIndex].result;

                CHX_LOG("Frame %" PRIu64 ": Returning metadata result %d "
                    "for app frame %d m_nextAppResultFrame=%" PRIu64 " m_lastAppRequestFrame=%" PRIu64,
                    resultFrame,
                    result.partial_result,
                    result.frame_number,
                    m_nextAppResultFrame,
                    m_lastAppRequestFrame);

                Usecase::ReturnFrameworkResult(&result, m_cameraId);

                if (NULL != m_driverPartialCaptureResult[frameIndex].result)
                {
                    m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(
                        m_driverPartialCaptureResult[frameIndex].result);
                }

                if (NULL != m_captureResult[frameIndex].result)
                {
                    m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(
                        m_captureResult[frameIndex].result);
                }

                if (result.partial_result == ExtensionModule::GetInstance()->GetNumMetadataResults())
                {
                    m_isMetadataSent[frameIndex]        = TRUE;
                    // Reset the meta related info in corresponding capture result
                    m_captureResult[frameIndex].result  = NULL;
                    m_isMetadataAvailable[frameIndex]   = FALSE;
                }
            }

            if ((TRUE == bufferReturn) && (TRUE == CheckIfAnyPartialMetaDataHasBeenSent(frameIndex)))
            {
                CHX_LOG("Frame %" PRIu64 ": Returning buffer result for %d output buffers for app frame %d",
                    resultFrame,
                    m_captureResult[frameIndex].num_output_buffers,
                    m_captureResult[frameIndex].frame_number);

                for (UINT i = 0; i < m_captureResult[frameIndex].num_output_buffers; i++)
                {
                    CHX_LOG("\tStream %p, W: %d, H: %d",
                            m_captureResult[frameIndex].output_buffers[i].stream,
                            m_captureResult[frameIndex].output_buffers[i].stream->width,
                            m_captureResult[frameIndex].output_buffers[i].stream->height);
                }

                camera3_capture_result_t result = { 0 };

                result.frame_number       = m_captureResult[frameIndex].frame_number;
                result.num_output_buffers = m_captureResult[frameIndex].num_output_buffers;
                result.output_buffers     = m_captureResult[frameIndex].output_buffers;

                ReturnFrameworkResult(&result, m_cameraId);

                // Decrease the pending output buffers number by the number of output buffer sent now.
                // Reset the num_output_buffers in capture result too.
                if (m_numAppPendingOutputBuffers[frameIndex] >= m_captureResult[frameIndex].num_output_buffers)
                {
                    m_numAppPendingOutputBuffers[frameIndex] -= m_captureResult[frameIndex].num_output_buffers;
                }
                else
                {
                    CHX_LOG_ERROR("PendingOutputBuffers number decrease to below zero.");
                    m_numAppPendingOutputBuffers[frameIndex] = 0;
                }

                // send the input buffer seperately once all corresponding outut buffers of that
                // request are sent
                if ((TRUE == inBufferReturn) && (0 == m_numAppPendingOutputBuffers[frameIndex]))
                {
                    ChxUtils::Memset(&result, 0, sizeof(camera3_capture_result_t));

                    result.frame_number   = m_captureResult[frameIndex].frame_number;
                    result.input_buffer   = m_captureResult[frameIndex].input_buffer;

                    CHX_LOG("Frame %d Returning input buffer as all input buffers are done", result.frame_number);

                    ReturnFrameworkResult(&result, m_cameraId);

                    ChxUtils::Memset(const_cast<camera3_stream_buffer_t*>(m_captureResult[frameIndex].input_buffer),
                        0, sizeof(camera3_stream_buffer_t));

                }
                m_captureResult[frameIndex].num_output_buffers = 0;
            }
        }

        // Handles the error cases which can set m_isMetadataSent, but may do it out of order. This function
        // steps through the requests, including ones in error, and will move m_lastResultMetadataFrameNum here
        if ((TRUE == m_isMetadataSent[frameIndex]) && (m_lastResultMetadataFrameNum == (resultFrame - 1)))
        {
            m_lastResultMetadataFrameNum++;
        }

        // Moving forward if there is no pending output buffer and metadata
        if ((0 == m_numAppPendingOutputBuffers[frameIndex]) && (TRUE == m_isMetadataSent[frameIndex]))
        {
            CHX_LOG("Frame %" PRIu64 " m_nextAppResultFrame: %" PRIu64 " has returned all results",
                resultFrame, m_nextAppResultFrame);

            // Don't increment m_nextAppResultFrame if we are not done with that
            // frame yet
            if (resultFrame == m_nextAppResultFrame)
            {
                m_nextAppResultFrame++;
                CHX_LOG("Advanced next result frame to %" PRIu64, m_nextAppResultFrame);
                if (TRUE == m_rejectedSnapshotRequestList[m_nextAppResultFrame % MaxOutstandingRequests])
                {
                    UINT featureIndex                          = m_nextAppResultFrame % MaxOutstandingRequests;
                    camera3_capture_request_t* pRequest        = ReturnPendingPCR(featureIndex);
                    CHX_LOG_INFO("Reject snapshot Buffer as error for rejected request frame  %" PRIu64" index = %d",
                        m_nextAppResultFrame, featureIndex);
                    HandleProcessRequestError(pRequest);
                    m_rejectedSnapshotRequestList[featureIndex] = FALSE;
                }
            }
        }
    }
    m_pAppResultMutex->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ProcessAndReturnPartialMetadataFinishedResults
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::ProcessAndReturnPartialMetadataFinishedResults(PartialResultSender partialResultSender)
{
    camera3_capture_result_t *pPartialResult        = NULL;
    camera3_capture_result_t *pPartialResultTest    = NULL;

    if (PartialResultSender::DriverMetaData == partialResultSender)
    {
        return;
    }

    if ((PartialResultSender::CHIPartialData == partialResultSender) &&
        (PartialMetaSupport::CombinedPartialMeta == ExtensionModule::GetInstance()->EnableCHIPartialData()))
    {
        return;
    }

    m_pAppResultMutex->Lock();

    for (INT64 resultFrame = m_nextAppResultFrame; resultFrame <= m_lastAppRequestFrame; resultFrame++)
    {
        UINT frameIndex = ChxUtils::GetResultFrameIndexChi(resultFrame);

        pPartialResult  = (PartialResultSender::DriverPartialData == partialResultSender) ?
            &m_driverPartialCaptureResult[frameIndex] : &m_chiPartialCaptureResult[frameIndex];

        BOOL metadataAvailable  = ((NULL != pPartialResult->result) &&
            (0 != pPartialResult->partial_result)) ? TRUE : FALSE;

        BOOL metadataErrorSent = m_requestFlags[frameIndex].isMetadataErrorSent;

        if ((TRUE == metadataAvailable) && (FALSE == metadataErrorSent))
        {
            if ((PartialResultSender::DriverPartialData == partialResultSender) &&
                (PartialMetaSupport::CombinedPartialMeta == ExtensionModule::GetInstance()->EnableCHIPartialData()))
            {
                // We need to merge the meta data
                camera3_capture_result_t *pCHIPartialResult     = &m_chiPartialCaptureResult[frameIndex];
                camera_metadata_t        *pCHIResultMetadata    = const_cast<camera_metadata_t*>(pCHIPartialResult->result);
                camera_metadata_t        *pResultMetadata       = const_cast<camera_metadata_t*>(pPartialResult->result);

                ChxUtils::AndroidMetadata::MergeMetadata(static_cast<VOID*>(pCHIResultMetadata),
                                        static_cast<VOID*>(pResultMetadata));

                CHX_LOG("Partial Result is merged");
            }

            camera3_capture_result_t result     = { 0 };
            result.frame_number                 = pPartialResult->frame_number;
            result.result                       = pPartialResult->result;
            result.partial_result               =
                static_cast<int>(ChxUtils::GetPartialResultCount(partialResultSender));

            CHX_LOG("Frame %" PRIu64 ": Returning Partial metadata result %d "
                "for app frame %d m_nextAppResultFrame=%" PRIu64 " m_lastAppRequestFrame=%" PRIu64 ,
                resultFrame,
                result.partial_result,
                result.frame_number,
                m_nextAppResultFrame,
                m_lastAppRequestFrame);

            Usecase::ReturnFrameworkResult(&result, m_cameraId);

            if (PartialResultSender::DriverPartialData == partialResultSender)
            {
                if (NULL != m_driverPartialCaptureResult[frameIndex].result)
                {
                    m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(
                        m_driverPartialCaptureResult[frameIndex].result);
                }
            }
            else if (PartialResultSender::CHIPartialData == partialResultSender)
            {
                if (NULL != m_chiPartialCaptureResult[frameIndex].result)
                {
                    m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(
                        m_chiPartialCaptureResult[frameIndex].result);
                }
            }

            // Reset the partial meta related info in corresponding partial capture result
            pPartialResult->result           = NULL;
            pPartialResult->partial_result   = 0;

        }
    }
    m_pAppResultMutex->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ProcessErrorMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::ProcessErrorMessage(
    const ChiMessageDescriptor* pMessageDescriptor)
{
    CHX_ASSERT(ChiMessageTypeError == pMessageDescriptor->messageType);

    INT64 resultFrame  = pMessageDescriptor->message.errorMessage.frameworkFrameNum;
    UINT64 frameIndex  = resultFrame % MaxOutstandingRequests;

    switch (pMessageDescriptor->message.errorMessage.errorMessageCode)
    {
        case MessageCodeRequest:
            CHX_LOG_INFO("Frame %" PRIu64 ": Request error. Only this error will be reported", resultFrame);
            m_isMetadataSent[frameIndex] = TRUE;
            break;
        case MessageCodeResult:
            CHX_LOG_INFO("Frame %" PRIu64 ": Metadata error", resultFrame);
            m_isMetadataSent[frameIndex]                            = TRUE;
            m_requestFlags[frameIndex].isMetadataErrorSent          = TRUE;
            break;
        case MessageCodeBuffer:
            CHX_LOG_INFO("Frame %" PRIu64 ": Buffer error for stream %p",
                    resultFrame, pMessageDescriptor->message.errorMessage.pErrorStream);
            CHX_ASSERT(0 < m_numAppPendingOutputBuffers[frameIndex]);
            m_requestFlags[frameIndex].isBufferErrorSent = TRUE;
            m_numBufferErrorMessages[frameIndex]++;
            break;
        case MessageCodeTriggerRecovery:
            CHX_LOG_INFO("Frame %" PRIu64 ": Detected recovery error message; propagating message to Usecase", resultFrame);
            Usecase::ProcessErrorMessage(pMessageDescriptor);
            return;
            break;
        case MessageCodeDevice:
            CHX_LOG_ERROR("Not handling device errors");
            break;
        default:
            CHX_LOG_ERROR("Unhandled error for frame:%" PRIu64 ": %d", resultFrame,
                          pMessageDescriptor->message.errorMessage.errorMessageCode);
            break;
    }

    if ((0 == m_numAppPendingOutputBuffers[frameIndex]) && (TRUE == m_isMetadataSent[frameIndex]))
    {
        CHX_LOG("Frame %" PRIu64 " has returned all results (with some errors)", resultFrame);

        // Only increment m_nextAppResultFrame if we are done it
        if (resultFrame == m_nextAppResultFrame)
        {
            m_nextAppResultFrame++;
            CHX_LOG("Advanced next result frame to %" PRIu64, m_nextAppResultFrame);
        }
    }

    // If we move m_nextAppResultFrame without moving this, the loop in ProcessAndReturnFinishedResults won't catch it
    if ((m_lastResultMetadataFrameNum == (resultFrame - 1)) && (TRUE == m_isMetadataSent[frameIndex]))
    {
        m_lastResultMetadataFrameNum++;
    }
    ReturnFrameworkMessage((camera3_notify_msg_t*)pMessageDescriptor, m_cameraId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ExecuteFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::ExecuteFlush()
{
    Session* ppSessionsToFlush[MaxSessions];

    for (UINT sessionIndex = 0; sessionIndex < MaxSessions; sessionIndex++)
    {
        ppSessionsToFlush[sessionIndex] = m_sessions[sessionIndex].pSession;
    }
    FlushAllSessions(ppSessionsToFlush, MaxSessions);

    return CDKResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::MergeDebugData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CameraUsecaseBase::MergeDebugData(
    ChiMetadata*    pMetadata,
    DebugData*      pOfflineDebugData,
    const CHAR*     sProcessingStage)
{
    CDKResult result = CDKResultSuccess;

    if (TRUE == ChxUtils::IsVendorTagPresent(pMetadata, VendorTag::DebugDataTag))
    {
        CHAR* pData = NULL;
        ChxUtils::GetVendorTagValue(pMetadata, VendorTag::DebugDataTag, (VOID**)&pData);
        if (NULL != pData)
        {
            DebugData* pDebug = reinterpret_cast<DebugData*>(pData);

            if ((NULL != pDebug->pData) && (0 < pDebug->size))
            {
                if ((NULL != pOfflineDebugData->pData) &&
                    (pDebug->size == pOfflineDebugData->size))
                {
                    CHX_LOG("%s: Replace DebugData: %p (%zu), new: %p (%zu)",
                            sProcessingStage,
                            pDebug->pData, pDebug->size,
                            pOfflineDebugData->pData, pOfflineDebugData->size);
                    // Use offline snapshot data
                    result = ChxUtils::SetVendorTagValue(pMetadata,
                                                         VendorTag::DebugDataTag,
                                                         sizeof(DebugData),
                                                         pOfflineDebugData);
                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_ERROR("DebugDataAll: Fail to set debugdata tag in offline input metadata");
                    }
                }
                else
                {
                    CHX_LOG_ERROR("DebugDataAll: Fail to set offline debug-data");
                    result = CDKResultEFailed;
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("DebugDataAll: Unable to get vendor tag data!");
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::DumpDebugData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::DumpDebugData(
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
                       "%s/%s_debug-data_session[%u]_cameraId[%u]_req[%u]_cnt[%u].%s",
                       FileDumpPath,
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

    CHX_LOG_INFO("DebugDataAll: dumpFilename: %s, pDebugData: %p, sizeDebugData: %zu, sizeMeta: %u [0x%x]",
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
        CHX_LOG_ERROR("DebugDataAll: Debug data failed to open for writing: %s", dumpFilename);
    }

    m_debugLastResultFrameNumber = resultFrameNumber;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraUsecaseBase::ProcessDebugData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CameraUsecaseBase::ProcessDebugData(
    const CHICAPTURERESULT* pResult,
    const VOID*             pPrivateCallbackData,
    const UINT32            resultFrameNum)
{
    const SessionPrivateData* pCbData = static_cast<const SessionPrivateData*>(pPrivateCallbackData);
    if ((NULL != pResult->pOutputMetadata))
    {
        ChiMetadata* pChiOutputMetadata = m_pMetadataManager->GetMetadataFromHandle(pResult->pOutputMetadata);
        if (TRUE == ChxUtils::IsVendorTagPresent(pChiOutputMetadata, VendorTag::DebugDataTag))
        {
            CHAR* pData = NULL;
            ChxUtils::GetVendorTagValue(pChiOutputMetadata, VendorTag::DebugDataTag, (VOID**)&pData);

            if (NULL != pData)
            {
                DebugData* pDebug = reinterpret_cast<DebugData*>(pData);
                if ((NULL != pDebug->pData) && (0 < pDebug->size))
                {
                    DumpDebugData(pDebug->pData,
                                  pDebug->size,
                                  pCbData->sessionId,
                                  pCbData->pUsecase->GetCameraId(),
                                  resultFrameNum);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Class AdvancedCameraUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::~AdvancedCameraUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AdvancedCameraUsecase::~AdvancedCameraUsecase()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AdvancedCameraUsecase* AdvancedCameraUsecase::Create(
    LogicalCameraInfo*              pCameraInfo,   ///< Camera info
    camera3_stream_configuration_t* pStreamConfig, ///< Stream configuration
    UsecaseId                       usecaseId)     ///< Identifier for usecase function
{
    CDKResult              result                 = CDKResultSuccess;
    AdvancedCameraUsecase* pAdvancedCameraUsecase = CHX_NEW AdvancedCameraUsecase;

    if ((NULL != pAdvancedCameraUsecase) && (NULL != pStreamConfig))
    {
        result = pAdvancedCameraUsecase->Initialize(pCameraInfo, pStreamConfig, usecaseId);

        if (CDKResultSuccess != result)
        {
            pAdvancedCameraUsecase->Destroy(FALSE);
            pAdvancedCameraUsecase = NULL;
        }
    }
    else
    {
        result = CDKResultEFailed;
    }

    return pAdvancedCameraUsecase;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::ExecuteFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AdvancedCameraUsecase::ExecuteFlush()
{
    CDKResult result = CDKResultSuccess;

    // wait if session is being reconfigured, such as snapshot in remosaic case
    AcquireRTConfigLock();

    result = CameraUsecaseBase::ExecuteFlush();

    // Pause features to take any actions on flushed buffers
    BOOL isMCFeature2Flushed = FALSE;
    for (UINT32 index = 0; index < m_numOfPhysicalDevices; index++)
    {
        for (UINT32 i = 0; i < m_enabledFeaturesCount[index]; i++)
        {
            CHX_LOG_INFO("AdvancedCameraUsecase FLUSH started FEATURE TYPE: %d and device index =%d",
                m_enabledFeatures[index][i]->GetFeatureType(),index);

            if ((TRUE == IsMultiCameraUsecase()) && (FeatureType::Feature2 == m_enabledFeatures[index][i]->GetFeatureType()))
            {    //For feature 2 mulitcamera is using single instance of Feature2Wrapper, so flushing for multiple device
                 // not needed as the same feature will be flushed again and also creating timing issues in flushing
                 // resulting flushtimeout
                if (FALSE == isMCFeature2Flushed)
                {
                    m_enabledFeatures[index][i]->Flush();
                    isMCFeature2Flushed = TRUE;
                }
            }
            else
            {
                m_enabledFeatures[index][i]->Flush();
            }
        }
    }

    ReleaseRTConfigLock();
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AdvancedCameraUsecase::Destroy(BOOL isForced)
{
    //Pause features to take any actions on flushed buffers
    for (UINT32 index = 0 ; index < m_numOfPhysicalDevices ; index++)
    {
        if (0 != m_enabledFeaturesCount[index])
        {
            for (UINT32 i = 0; i < m_enabledFeaturesCount[index]; i++)
            {
                CHX_LOG("AdvancedCameraUsecase Pause FEATURE TYPE=%d",
                    m_enabledFeatures[index][i]->GetFeatureType());
                if (m_enabledFeatures[index][i]->GetFeatureType() == FeatureType::Feature2 && index == 0)
                {
                    m_pFeature2Wrapper->Pause(FALSE);
                }
                else
                {
                    m_enabledFeatures[index][i]->Pause(FALSE);
                }

            }
        }
    }

    CameraUsecaseBase::Destroy(isForced);

    if (NULL != m_pSetFeatureMutex)
    {
        m_pSetFeatureMutex->Destroy();
        m_pSetFeatureMutex = NULL;
    }

    if (NULL != m_pResultMutex)
    {
        m_pResultMutex->Destroy();
        m_pResultMutex = NULL;
    }

    if (NULL != m_pRealtimeReconfigDoneMutex)
    {
        m_pRealtimeReconfigDoneMutex->Destroy();
        m_pRealtimeReconfigDoneMutex = NULL;
    }

    for (UINT32 i = 0 ; i < m_numOfPhysicalDevices; i++)
    {
        if ((FALSE == m_isRdiStreamImported) && (NULL != m_pRdiStream[i]))
        {
            CHX_FREE(m_pRdiStream[i]);
            m_pRdiStream[i] = NULL;
        }
        else
        {
            CHX_LOG("m_isRdiStreamImported=%d m_pRdiStream[%d]=%p RDI stream will be freed in usecaseMC if it is imported",
                    m_isRdiStreamImported, i, m_pRdiStream[i]);
        }

        if ((FALSE == m_isFdStreamImported) && (NULL != m_pFdStream[i]))
        {
            CHX_FREE(m_pFdStream[i]);
            m_pFdStream[i] = NULL;
        }
        else
        {
            CHX_LOG("m_isFdStreamImported=%d m_pFdStream[%d]=%p FD stream will be freed in usecaseMC if it is imported",
                    m_isFdStreamImported, i, m_pFdStream[i]);
        }

        if (NULL != m_pBayer2YuvStream[i])
        {
            CHX_FREE(m_pBayer2YuvStream[i]);
            m_pBayer2YuvStream[i] = NULL;
        }

        if (NULL != m_pJPEGInputStream[i])
        {
            CHX_FREE(m_pJPEGInputStream[i]);
            m_pJPEGInputStream[i] = NULL;
        }
    }

    m_pSnapshotStream   = NULL;
    m_pPreviewStream    = NULL;
    m_pYuvInStream      = NULL;
    m_pYuvCBStream      = NULL;
    m_pVideoStream      = NULL;

    for (UINT target = 0; NULL != m_pChiUsecase && target < m_pChiUsecase->numTargets; target++)
    {
        m_pChiUsecase->ppChiTargets[target]->pChiStream = NULL;
    }

    for(UINT32 physicalCameraIndex = 0 ; physicalCameraIndex < m_numOfPhysicalDevices; physicalCameraIndex++)
    {
        if (0 != m_enabledFeaturesCount[physicalCameraIndex])
        {
            for (UINT32 i = 0; i < m_enabledFeaturesCount[physicalCameraIndex]; i++)
            {
                if (NULL != m_enabledFeatures[physicalCameraIndex][i])
                {
                    FeatureType type = m_enabledFeatures[physicalCameraIndex][i]->GetFeatureType();
                    if (type != FeatureType::Feature2)
                    {
                        m_enabledFeatures[physicalCameraIndex][i]->Destroy(isForced);
                    }
                    m_enabledFeatures[physicalCameraIndex][i] = NULL;
                }
            }
            m_enabledFeaturesCount[physicalCameraIndex] = 0;
        }
    }

    if (NULL != m_pFeature2Wrapper)
    {
        m_pFeature2Wrapper->Destroy(isForced);
        m_pFeature2Wrapper = NULL;
    }

    if (NULL != m_pCallbacks)
    {
        CHX_FREE(m_pCallbacks);
    }

    if ((NULL != m_pChiUsecase) && (FALSE == m_pChiUsecase->isOriginalDescriptor))
    {
        UsecaseSelector::FreeUsecaseDescriptor(m_pChiUsecase);
        m_pChiUsecase = NULL;
    }
    if (NULL != m_pClonedUsecase)
    {
        UsecaseSelector::DestroyUsecase(m_pClonedUsecase);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AdvancedCameraUsecase::Initialize(
    LogicalCameraInfo*              pCameraInfo,   ///< Camera info
    camera3_stream_configuration_t* pStreamConfig, ///< Stream configuration
    UsecaseId                       usecaseId)     ///< Identifier for the usecase function
{
    ATRACE_BEGIN("AdvancedCameraUsecase::Initialize");
    CDKResult result = CDKResultSuccess;

    m_usecaseId                     = usecaseId;
    m_cameraId                      = pCameraInfo->cameraId;
    m_pLogicalCameraInfo            = pCameraInfo;

    m_pResultMutex                  = Mutex::Create();
    m_pSetFeatureMutex              = Mutex::Create();
    m_pRealtimeReconfigDoneMutex    = Mutex::Create();
    m_isReprocessUsecase            = FALSE;
    m_numOfPhysicalDevices          = pCameraInfo->numPhysicalCameras;
    m_isUsecaseCloned               = FALSE;

    for (UINT32 i = 0 ; i < m_numOfPhysicalDevices; i++)
    {
        m_cameraIdMap[i] = pCameraInfo->ppDeviceInfo[i]->cameraId;
    }

    ExtensionModule::GetInstance()->GetVendorTagOps(&m_vendorTagOps);
    CHX_LOG("pGetMetaData:%p, pSetMetaData:%p", m_vendorTagOps.pGetMetaData, m_vendorTagOps.pSetMetaData);

    pAdvancedUsecase = GetXMLUsecaseByName(ZSL_USECASE_NAME);

    if (NULL == pAdvancedUsecase)
    {
        CHX_LOG_ERROR("Fail to get ZSL usecase from XML!");
        result = CDKResultEFailed;
    }

    ChxUtils::Memset(m_enabledFeatures, 0, sizeof(m_enabledFeatures));
    ChxUtils::Memset(m_rejectedSnapshotRequestList, 0, sizeof(m_rejectedSnapshotRequestList));

    if (TRUE == IsMultiCameraUsecase())
    {
        m_isRdiStreamImported   = TRUE;
        m_isFdStreamImported    = TRUE;
    }
    else
    {
        m_isRdiStreamImported   = FALSE;
        m_isFdStreamImported    = FALSE;
        m_inputOutputType       = static_cast<UINT32>(InputOutputType::NO_SPECIAL);
    }

    for (UINT32 i = 0; i < m_numOfPhysicalDevices; i++)
    {
        if (FALSE == m_isRdiStreamImported)
        {
            m_pRdiStream[i] = static_cast<CHISTREAM*>(CHX_CALLOC(sizeof(CHISTREAM)));
        }

        if (FALSE == m_isFdStreamImported)
        {
            m_pFdStream[i]  = static_cast<CHISTREAM*>(CHX_CALLOC(sizeof(CHISTREAM)));
        }

        m_pBayer2YuvStream[i] = static_cast<CHISTREAM*>(CHX_CALLOC(sizeof(CHISTREAM)));
        m_pJPEGInputStream[i] = static_cast<CHISTREAM*>(CHX_CALLOC(sizeof(CHISTREAM)));
    }

    for (UINT32 i = 0; i < MaxPipelines; i++)
    {
        m_pipelineToSession[i] = InvalidSessionId;
    }

    m_realtimeSessionId = static_cast<UINT32>(InvalidSessionId);

    if (NULL == pStreamConfig)
    {
        CHX_LOG_ERROR("pStreamConfig is NULL");
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        CHX_LOG_INFO("AdvancedCameraUsecase::Initialize usecaseId:%d num_streams:%d", m_usecaseId, pStreamConfig->num_streams);
        CHX_LOG_INFO("CHI Input Stream Configs:");
        for (UINT stream = 0; stream < pStreamConfig->num_streams; stream++)
        {
             CHX_LOG_INFO("\tstream = %p streamType = %d streamFormat = %d streamWidth = %d streamHeight = %d",
                          pStreamConfig->streams[stream],
                          pStreamConfig->streams[stream]->stream_type,
                          pStreamConfig->streams[stream]->format,
                          pStreamConfig->streams[stream]->width,
                          pStreamConfig->streams[stream]->height);

             if (CAMERA3_STREAM_INPUT == pStreamConfig->streams[stream]->stream_type)
             {
                 CHX_LOG_INFO("Reprocess usecase");
                 m_isReprocessUsecase = TRUE;
             }
        }
        result = CreateMetadataManager(m_cameraId, false, NULL, true);
    }

    // Default sensor mode pick hint
    m_defaultSensorModePickHint.sensorModeCaps.value    = 0;
    m_defaultSensorModePickHint.postSensorUpscale       = FALSE;
    m_defaultSensorModePickHint.sensorModeCaps.u.Normal = TRUE;

    if (TRUE == IsQuadCFAUsecase() && (CDKResultSuccess == result))
    {
        CHIDIMENSION binningSize = { 0 };

        // get binning mode sensor output size,
        // if more than one binning mode, choose the largest one
        for (UINT i = 0; i < pCameraInfo->m_cameraCaps.numSensorModes; i++)
        {
            CHX_LOG("i:%d, sensor mode:%d, size:%dx%d",
                i, pCameraInfo->pSensorModeInfo[i].sensorModeCaps.value,
                pCameraInfo->pSensorModeInfo[i].frameDimension.width,
                pCameraInfo->pSensorModeInfo[i].frameDimension.height);

            if (1 == pCameraInfo->pSensorModeInfo[i].sensorModeCaps.u.Normal)
            {
                if ((pCameraInfo->pSensorModeInfo[i].frameDimension.width  > binningSize.width) ||
                    (pCameraInfo->pSensorModeInfo[i].frameDimension.height > binningSize.height))
                {
                    binningSize.width  = pCameraInfo->pSensorModeInfo[i].frameDimension.width;
                    binningSize.height = pCameraInfo->pSensorModeInfo[i].frameDimension.height;
                }
            }
        }

        CHX_LOG("sensor binning mode size:%dx%d", binningSize.width, binningSize.height);

        // For Quad CFA sensor, should use binning mode for preview.
        // So set postSensorUpscale flag here to allow sensor pick binning sensor mode.
        m_QuadCFASensorInfo.sensorModePickHint.sensorModeCaps.value    = 0;
        m_QuadCFASensorInfo.sensorModePickHint.postSensorUpscale       = TRUE;
        m_QuadCFASensorInfo.sensorModePickHint.sensorModeCaps.u.Normal = TRUE;
        m_QuadCFASensorInfo.sensorModePickHint.sensorOutputSize.width  = binningSize.width;
        m_QuadCFASensorInfo.sensorModePickHint.sensorOutputSize.height = binningSize.height;

        // For Quad CFA usecase, should use full size mode for snapshot.
        m_defaultSensorModePickHint.sensorModeCaps.value               = 0;
        m_defaultSensorModePickHint.postSensorUpscale                  = FALSE;
        m_defaultSensorModePickHint.sensorModeCaps.u.QuadCFA           = TRUE;
    }

    if (CDKResultSuccess == result)
    {
        FeatureSetup(pStreamConfig);
        result = SelectUsecaseConfig(pCameraInfo, pStreamConfig);
    }

    if ((NULL != m_pChiUsecase) && (CDKResultSuccess == result) && (NULL != m_pPipelineToCamera))
    {
        CHX_LOG_INFO("Usecase %s selected", m_pChiUsecase->pUsecaseName);

        m_pCallbacks = static_cast<ChiCallBacks*>(CHX_CALLOC(sizeof(ChiCallBacks) * m_pChiUsecase->numPipelines));

        CHX_LOG_INFO("Pipelines need to create in advance usecase:%d", m_pChiUsecase->numPipelines);
        for (UINT i = 0; i < m_pChiUsecase->numPipelines; i++)
        {
            CHX_LOG_INFO("[%d/%d], pipeline name:%s, pipeline type:%d, session id:%d, camera id:%d",
                          i,
                          m_pChiUsecase->numPipelines,
                          m_pChiUsecase->pPipelineTargetCreateDesc[i].pPipelineName,
                          GetAdvancedPipelineTypeByPipelineId(i),
                          (NULL != m_pPipelineToSession) ? m_pPipelineToSession[i] : i,
                          m_pPipelineToCamera[i]);
        }

        if (NULL != m_pCallbacks)
        {
            for (UINT i = 0; i < m_pChiUsecase->numPipelines; i++)
            {
                m_pCallbacks[i].ChiNotify                      = AdvancedCameraUsecase::ProcessMessageCb;
                m_pCallbacks[i].ChiProcessCaptureResult        = AdvancedCameraUsecase::ProcessResultCb;
                m_pCallbacks[i].ChiProcessPartialCaptureResult = AdvancedCameraUsecase::ProcessDriverPartialCaptureResultCb;
            }

            result = CameraUsecaseBase::Initialize(m_pCallbacks, pStreamConfig);

            for (UINT index = 0; index < m_pChiUsecase->numPipelines; ++index)
            {
                INT32  pipelineType = GET_PIPELINE_TYPE_BY_ID(m_pipelineStatus[index].pipelineId);
                UINT32 rtIndex      = GET_FEATURE_INSTANCE_BY_ID(m_pipelineStatus[index].pipelineId);

                if (CDKInvalidId == m_metadataClients[index])
                {
                    result = CDKResultEFailed;
                    break;
                }

                if ((rtIndex < MaxRealTimePipelines) && (pipelineType < AdvancedPipelineType::PipelineCount))
                {
                    m_pipelineToClient[rtIndex][pipelineType] = m_metadataClients[index];
                    m_pMetadataManager->SetPipelineId(m_metadataClients[index], m_pipelineStatus[index].pipelineId);
                }
            }
        }

        PostUsecaseCreation(pStreamConfig);

        UINT32 maxRequiredFrameCnt = GetMaxRequiredFrameCntForOfflineInput(0);
        if (TRUE == IsMultiCameraUsecase())
        {
            //todo: it is better to calculate max required frame count according to pipeline,
            // for example,some customer just want to enable MFNR feature for wide sensor,
            // some customer just want to enable SWMF feature for tele sensor.
            // here suppose both sensor enable same feature simply.
            for (UINT i = 0; i < m_numOfPhysicalDevices; i++)
            {
                maxRequiredFrameCnt = GetMaxRequiredFrameCntForOfflineInput(i);
                UpdateValidRDIBufferLength(i, maxRequiredFrameCnt + 1);
                UpdateValidFDBufferLength(i, maxRequiredFrameCnt + 1);
                CHX_LOG_CONFIG("physicalCameraIndex:%d,validBufferLength:%d",
                    i, GetValidBufferLength(i));
            }

        }
        else
        {
            if (m_rdiStreamIndex != InvalidId)
            {
                UpdateValidRDIBufferLength(m_rdiStreamIndex, maxRequiredFrameCnt + 1);
                CHX_LOG_INFO("m_rdiStreamIndex:%d validBufferLength:%d",
                             m_rdiStreamIndex, GetValidBufferLength(m_rdiStreamIndex));
            }
            else
            {
                CHX_LOG_INFO("No RDI stream");
            }

            if (m_fdStreamIndex != InvalidId)
            {
                UpdateValidFDBufferLength(m_fdStreamIndex, maxRequiredFrameCnt + 1);
                CHX_LOG_INFO("m_fdStreamIndex:%d validBufferLength:%d",
                             m_fdStreamIndex, GetValidBufferLength(m_fdStreamIndex));
            }
            else
            {
                CHX_LOG_INFO("No FD stream");
            }
        }
    }
    else
    {
        result = CDKResultEFailed;
    }

    ATRACE_END();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::PreUsecaseSelection
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AdvancedCameraUsecase::FeatureSetup(
    camera3_stream_configuration_t* pStreamConfig)
{
    CDKResult result = CDKResultSuccess;

    if ((UsecaseId::PreviewZSL    == m_usecaseId) ||
        (UsecaseId::YUVInBlobOut  == m_usecaseId) ||
        (UsecaseId::VideoLiveShot == m_usecaseId) ||
        (UsecaseId::QuadCFA       == m_usecaseId) ||
        (UsecaseId::RawJPEG       == m_usecaseId))
    {
        SelectFeatures(pStreamConfig);
    }
    else if (UsecaseId::MultiCamera == m_usecaseId)
    {
        SelectFeatures(pStreamConfig);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::SelectUsecaseConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AdvancedCameraUsecase::SelectUsecaseConfig(
    LogicalCameraInfo*              pCameraInfo,   ///< Camera info
    camera3_stream_configuration_t* pStreamConfig)  ///< Stream configuration
{
    CDKResult result = CDKResultSuccess;

    if ((UsecaseId::PreviewZSL    == m_usecaseId) ||
        (UsecaseId::YUVInBlobOut  == m_usecaseId) ||
        (UsecaseId::VideoLiveShot == m_usecaseId) ||
        (UsecaseId::MultiCamera   == m_usecaseId) ||
        (UsecaseId::QuadCFA       == m_usecaseId) ||
        (UsecaseId::RawJPEG       == m_usecaseId))
    {
        ConfigureStream(pCameraInfo, pStreamConfig);
        BuildUsecase(pCameraInfo, pStreamConfig);

        if (NULL != m_pChiUsecase)
        {
            if (MaxNumOfTargets < m_pChiUsecase->numTargets)
            {
                if (UsecaseId::MultiCamera == m_usecaseId)
                {
                    CHX_LOG_WARN("Multicamera usecase:%d numTargets:%d expected to be greater than max allowed:%d",
                                 m_usecaseId,
                                 m_pChiUsecase->numTargets,
                                 MaxNumOfTargets);
                }
                else
                {
                    CHX_LOG_ERROR("Usecase:%d numTargets:%d is greater than max allowed:%d",
                                  m_usecaseId,
                                  m_pChiUsecase->numTargets,
                                  MaxNumOfTargets);

                    result = CamxResultEOutOfBounds;
                }
            }
        }
    }
    else
    {
        CHX_LOG("Initializing using default usecase matching");
        m_pChiUsecase = UsecaseSelector::DefaultMatchingUsecase(pStreamConfig);
        UINT* cameraIds     = NULL;
        UINT  pipelineNum;
        if (NULL != m_pChiUsecase)
        {
            pipelineNum = m_pChiUsecase->numPipelines;
            cameraIds = static_cast<UINT*>(CHX_CALLOC(sizeof(UINT) * pipelineNum));
            if (NULL != cameraIds)
            {
                for (UINT i = 0; i < pipelineNum; i++)
                {
                    cameraIds[i] = pCameraInfo->ppDeviceInfo[0]->cameraId;
                }
                result = SetPipelineToCameraMapping(pipelineNum, cameraIds);
                CHX_FREE(cameraIds);
                cameraIds = NULL;
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
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::PipelineCreated
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AdvancedCameraUsecase::PipelineCreated(
    UINT sessionId,          ///< Id of session created
    UINT pipelineIndex)      ///< Index of the pipeline created (within the context of the session)
{
    CHX_LOG_INFO("AdvancedCameraUsecase pipelineIndex: %d", pipelineIndex);

    if (TRUE == m_sessions[sessionId].pipelines[pipelineIndex].pPipeline->IsRealTime())
    {
        CHISENSORMODEINFO* pSensorInfo = m_sessions[sessionId].pipelines[pipelineIndex].pPipeline->GetSensorModeInfo();

        CHX_LOG_INFO("Final seleted sensor mode:%d, dimension: %dx%d",
            pSensorInfo->modeIndex,
            pSensorInfo->frameDimension.width,
            pSensorInfo->frameDimension.height);
    }

    if ((UsecaseId::PreviewZSL    == m_usecaseId) ||
        (UsecaseId::YUVInBlobOut  == m_usecaseId) ||
        (UsecaseId::VideoLiveShot == m_usecaseId) ||
        (UsecaseId::MultiCamera   == m_usecaseId) ||
        (UsecaseId::QuadCFA       == m_usecaseId) ||
        (UsecaseId::RawJPEG       == m_usecaseId))
    {

        for (UINT32 index = 0; index < m_numOfPhysicalDevices; index++)
        {
            for (UINT32 i = 0; i < m_enabledFeaturesCount[index]; i++)
            {
                m_enabledFeatures[index][i]->PipelineCreated(sessionId, pipelineIndex);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::PipelineDestroyed
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AdvancedCameraUsecase::PipelineDestroyed(
    UINT32 sessionId,          ///< Id of session created
    UINT32 pipelineIndex)      ///< Index of the pipeline created (within the context of the session)
{
    CHX_LOG_INFO("AdvancedCameraUsecase pipelineIndex: %d", pipelineIndex);

    for (UINT32 index = 0; index < m_numOfPhysicalDevices; index++)
    {
        for (UINT32 i = 0; i < m_enabledFeaturesCount[index]; i++)
        {
            m_enabledFeatures[index][i]->PipelineDestroyed(sessionId, pipelineIndex);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::PostUsecaseCreation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AdvancedCameraUsecase::PostUsecaseCreation(
    camera3_stream_configuration_t* pStreamConfig)  ///< Stream configuration
{
    (void)pStreamConfig;

    CDKResult result = CDKResultSuccess;

    for (UINT32 physicalCameraIndex = 0 ; physicalCameraIndex < m_numOfPhysicalDevices; physicalCameraIndex++)
    {
        for (UINT32 index = 0; index < m_enabledFeaturesCount[physicalCameraIndex]; ++index)
        {
            if (NULL != m_enabledFeatures[physicalCameraIndex][index])
            {
                m_enabledFeatures[physicalCameraIndex][index]->PostUsecaseCreated();
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::StreamIsInternal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AdvancedCameraUsecase::StreamIsInternal(
    ChiStream* pStream)
{
    BOOL isRDI = FALSE;

    for (UINT32 index = 0 ; index < m_numOfPhysicalDevices; index++)
    {
        for (UINT32 i = 0; i < m_enabledFeaturesCount[index]; i++)
        {
            if (TRUE == m_enabledFeatures[index][i]->StreamIsInternal(pStream))
            {
                isRDI = TRUE;
                break;
            }
        }
        if (TRUE == isRDI)
        {
            break;
        }
    }

    if ((TRUE == IsMultiCameraUsecase()) &&
        (TRUE == m_isRdiStreamImported))
    {
        CHX_LOG("RDI buffer Queue is from child class (usecasemc), override is internal to false here.");
        for (UINT32 i = 0 ; i < m_numOfPhysicalDevices; i++)
        {
            if (pStream == m_pRdiStream[i])
            {
                isRDI = FALSE;
                break;
            }
        }
    }

    return isRDI;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::IsFdStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AdvancedCameraUsecase::IsFdStream(
    ChiStream* pStream)
{
    BOOL bIsFdStream = FALSE;

    for(UINT32 i = 0 ; i < m_numOfPhysicalDevices; i++)
    {
        if (pStream == m_pFdStream[i])
        {
            bIsFdStream = TRUE;
            break;
        }
    }

    return bIsFdStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::IsRDIStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AdvancedCameraUsecase::IsRDIStream(
    ChiStream* pStream)
{
    BOOL bIsRdiStream = FALSE;

    for(UINT32 physicalCameraIndex = 0 ; physicalCameraIndex < m_numOfPhysicalDevices ; physicalCameraIndex++)
    {
        if (pStream == m_pRdiStream[physicalCameraIndex])
        {
            bIsRdiStream = TRUE;
            break;
        }
    }

    return bIsRdiStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::GetRealtimeSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const Session* AdvancedCameraUsecase::GetRealtimeSession()
{
    return  m_sessions[m_realtimeSessionId].pSession;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::ExecuteCaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AdvancedCameraUsecase::ExecuteCaptureRequest(
    camera3_capture_request_t* pRequest)
{
    CDKResult result     = CDKResultSuccess;
    UINT      frameIndex = pRequest->frame_number % MaxOutstandingRequests;
    Feature*  pFeature   = m_pActiveFeature;

    CHX_LOG("AdvancedCameraUsecase::ExecuteCaptureRequest %u %u", pRequest->frame_number, frameIndex);

    // swapping JPEG thumbnail size params
    const ExtensionModule*      pExtModule = ExtensionModule::GetInstance();
    BOOL isGpuOverrideSetting = pExtModule->UseGPUDownscaleUsecase() || pExtModule->UseGPURotationUsecase();
    if( (TRUE == m_GpuNodePresence) && (TRUE == isGpuOverrideSetting))
    {
        if (NULL != pRequest->settings)
        {
            // ANDROID_JPEG_ORIENTATION
            INT32 JpegOrientation    = 0;
            CDKResult result         = m_vendorTagOps.pGetMetaData(
                                        const_cast<VOID*>
                                        (reinterpret_cast<const VOID*>(pRequest->settings)),
                                        ANDROID_JPEG_ORIENTATION,
                                        &JpegOrientation,
                                        sizeof(INT32));

            if (CDKResultSuccess == result)
            {
                INT32*                  pIntentJpegSize = NULL;

                if (JpegOrientation % 180)
                {
                    JPEGThumbnailSize thumbnailSizeGet, thumbnailSizeSet;
                    CDKResult result = m_vendorTagOps.pGetMetaData(
                                        const_cast<VOID*>
                                        (reinterpret_cast<const VOID*>(pRequest->settings)),
                                        ANDROID_JPEG_THUMBNAIL_SIZE,
                                        &thumbnailSizeGet,
                                        sizeof(JPEGThumbnailSize));

                    if (CDKResultSuccess == result)
                    {
                        thumbnailSizeSet.JpegThumbnailSize_0 = thumbnailSizeGet.JpegThumbnailSize_1;
                        thumbnailSizeSet.JpegThumbnailSize_1 = thumbnailSizeGet.JpegThumbnailSize_0;

                        CDKResult result = m_vendorTagOps.pSetMetaData(
                                            const_cast<VOID*>
                                            (reinterpret_cast<const VOID*>(pRequest->settings)),
                                            ANDROID_JPEG_THUMBNAIL_SIZE,
                                            &thumbnailSizeSet,
                                            sizeof(JPEGThumbnailSize));

                        if (CDKResultSuccess == result)
                        {
                            CDKResult result = m_vendorTagOps.pGetMetaData(
                                                const_cast<VOID*>
                                                (reinterpret_cast<const VOID*>(pRequest->settings)),
                                                ANDROID_JPEG_THUMBNAIL_SIZE,
                                                &thumbnailSizeGet,
                                                sizeof(JPEGThumbnailSize));
                        }
                    }
                }
            }
        }
    }
    // exchange JPEG thumbnail

    m_shutterTimestamp[frameIndex]              = 0;

    result = UpdateFeatureModeIndex(const_cast<camera_metadata_t*>(pRequest->settings));
    if (TRUE ==
        ChxUtils::AndroidMetadata::IsVendorTagPresent(reinterpret_cast<const VOID*>(pRequest->settings),
            VendorTag::VideoHDR10Mode))
    {
        VOID* pData = NULL;
        StreamHDRMode  HDRMode = StreamHDRMode::HDRModeNone;
        ChxUtils::AndroidMetadata::GetVendorTagValue(reinterpret_cast<const VOID*>(pRequest->settings),
            VendorTag::VideoHDR10Mode,
            reinterpret_cast<VOID**>(&pData));
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

    if (StreamConfigModeFastShutter == ExtensionModule::GetInstance()->GetOpMode(m_cameraId) && NULL != pRequest->settings)
    {
        CHX_LOG("SetMetaData: StreamConfigModeFastShutter ");

        UINT8    isFSModeVendorTag = 1;
        UINT32   FSModeTagId       = ExtensionModule::GetInstance()->GetVendorTagId(VendorTag::FastShutterMode);
        result = m_vendorTagOps.pSetMetaData(
            const_cast<VOID*>
            (reinterpret_cast<const VOID*>(pRequest->settings)),
            FSModeTagId,
            &isFSModeVendorTag,
            sizeof(isFSModeVendorTag));

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("pSetMetaData failed result %d", result);
        }
    }

    if (TRUE == hasSnapshotStreamRequest(pRequest))
    {
        WaitForDeferThread();
    }

    ChxUtils::Memset(&m_snapshotFeatures[frameIndex], 0, sizeof(SnapshotFeatureList));

    if (TRUE == AdvancedFeatureEnabled())
    {
        for (UINT32 i = 0; i < pRequest->num_output_buffers; i++)
        {
            if (m_pSnapshotStream == reinterpret_cast<CHISTREAM*>(pRequest->output_buffers[i].stream))
            {
                pFeature = SelectFeatureToExecuteCaptureRequest(pRequest, 0);
            }
        }

        if (NULL != pFeature)
        {
            m_shutterTimestamp[frameIndex] = 0;
            result = pFeature->ExecuteProcessRequest(pRequest);
        }
    }
    else
    {
        CHX_LOG_INFO("CameraUsecaseBase::ExecuteCaptureRequest()");
        result = CameraUsecaseBase::ExecuteCaptureRequest(pRequest);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::ParseResultMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult AdvancedCameraUsecase::ParseResultMetadata(
    ChiMetadata* pResultMetadata)
{
    // check preview result meta, like asd output, aec/awb result.
    UINT32    tag         = 0;
    CDKResult result      = CDKResultSuccess;
    INT32     isLLSNeeded = 0;

    if (NULL == pResultMetadata)
    {
        CHX_LOG_ERROR("pResultMetadata is NULL");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        result = m_vendorTagOps.pQueryVendorTagLocation(
                 "org.quic.camera2.asdresults",
                 "ASDResults",
                 &tag);

        ASDOutput* pASDOutput = static_cast<ASDOutput*>(pResultMetadata->GetTag(
            "org.quic.camera2.asdresults", "ASDResults"));

        if (NULL != pASDOutput)
        {
            m_asdResult = *pASDOutput;
        }

        INT32* pLLSNeeded = static_cast<INT32*>(pResultMetadata->GetTag(
            "com.qti.stats_control", "is_lls_needed"));

        m_isLLSNeeded = pLLSNeeded ? *pLLSNeeded : FALSE;

        AECFrameControl* pAECFrameControl = static_cast<AECFrameControl*>(pResultMetadata->GetTag(
                                                    "org.quic.camera2.statsconfigs", "AECFrameControl"));

        if (NULL != pAECFrameControl)
        {
            CHX_LOG_VERBOSE("ParseResultMetadata: isTriggerInfoValid=%u, isInSensorHDR3ExpSnapshot=%u",
                            pAECFrameControl->inSensorHDR3ExpTriggerOutput.isTriggerInfoValid,
                            pAECFrameControl->isInSensorHDR3ExpSnapshot);
            if (TRUE == pAECFrameControl->inSensorHDR3ExpTriggerOutput.isTriggerInfoValid)
            {
                m_isInSensorHDR3ExpCapture = pAECFrameControl->isInSensorHDR3ExpSnapshot;
            }
        }

        m_isFlashRequired = IsFlashRequired(*pResultMetadata);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::ProcessResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AdvancedCameraUsecase::ProcessResult(
    CHICAPTURERESULT*           pResult,
    VOID*                       pPrivateCallbackData)
{
    if (TRUE == AdvancedFeatureEnabled())
    {
        SessionPrivateData* pSessionPrivateData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
        UINT32              sessionId           = pSessionPrivateData->sessionId;

        if ((NULL != pResult->pOutputMetadata) && (sessionId == m_realtimeSessionId))
        {
            ParseResultMetadata(m_pMetadataManager->GetMetadataFromHandle(pResult->pOutputMetadata));
        }

        m_pResultMutex->Lock();

        Feature* pFeature = FindFeatureToProcessResult(static_cast<CHIPRIVDATA*>(pResult->pPrivData),
                                                       pResult->frameworkFrameNum,
                                                       pPrivateCallbackData);
        if (NULL != pFeature)
        {
            pFeature->ProcessResult(pResult, pPrivateCallbackData);
        }
        else
        {
            CHX_LOG_ERROR("pFeature is NULL.");
        }

        m_pResultMutex->Unlock();
    }
    else
    {
        m_pResultMutex->Lock();
        CameraUsecaseBase::SessionCbCaptureResult(pResult, pPrivateCallbackData);
        m_pResultMutex->Unlock();
    }

    if (2 <= ExtensionModule::GetInstance()->EnableDumpDebugData())
    {
        // Process debug-data
        ProcessDebugData(pResult, pPrivateCallbackData, pResult->frameworkFrameNum);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::ProcessDriverPartialCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AdvancedCameraUsecase::ProcessDriverPartialCaptureResult(
    CHIPARTIALCAPTURERESULT* pResult,
    VOID*                    pPrivateCallbackData)
{
    if (TRUE == AdvancedFeatureEnabled())
    {
        SessionPrivateData* pSessionPrivateData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
        UINT32              sessionId = pSessionPrivateData->sessionId;

        if ((NULL != pResult->pPartialResultMetadata) && (sessionId == m_realtimeSessionId))
        {
            ParseResultMetadata(m_pMetadataManager->GetMetadataFromHandle(pResult->pPartialResultMetadata));
        }

        m_pResultMutex->Lock();

        Feature* pFeature = FindFeatureToProcessResult(static_cast<CHIPRIVDATA*>(pResult->pPrivData),
                                                       pResult->frameworkFrameNum,
                                                       pPrivateCallbackData);
        if (NULL != pFeature)
        {
            if (PartialMetaSupport::CombinedPartialMeta ==ExtensionModule::GetInstance()->EnableCHIPartialData())
            {
                pFeature->ProcessCHIPartialData(pResult->frameworkFrameNum, sessionId);
            }
            pFeature->ProcessDriverPartialCaptureResult(pResult, pPrivateCallbackData);
        }
        else
        {
            CHX_LOG_ERROR("pFeature is NULL.");
        }

        m_pResultMutex->Unlock();
    }
    else
    {
        CameraUsecaseBase::SessionCbPartialCaptureResult(pResult, pPrivateCallbackData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::IsRealtimeSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AdvancedCameraUsecase::IsRealtimeSession(
    INT32 sessionId)
{
    BOOL result = FALSE;
    for (UINT32 i = 0 ; i < m_numOfPhysicalDevices; i++)
    {
        UINT32 rtOffset = i << InstanceOffset;
        if ((m_pipelineToSession[rtOffset + AdvancedPipelineType::ZSLPreviewRawType] == sessionId) ||
            (m_pipelineToSession[rtOffset + AdvancedPipelineType::ZSLPreviewRawFSType] == sessionId) ||
            (m_pipelineToSession[rtOffset + AdvancedPipelineType::ZSLPreviewRawYUVType] == sessionId) ||
            (m_pipelineToSession[rtOffset + AdvancedPipelineType::QuadCFAFullSizeRawType] == sessionId))
        {
            result = TRUE;
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::IsOfflineSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AdvancedCameraUsecase::IsOfflineSession(
    INT32 sessionId)
{
    BOOL result = FALSE;

    for (UINT32 i = 0 ; i < m_numOfPhysicalDevices; i++)
    {
        UINT32 rtOffset = i << InstanceOffset;
        if ((m_pipelineToSession[rtOffset + AdvancedPipelineType::ZSLYuv2YuvType] == sessionId) ||
            (m_pipelineToSession[rtOffset + AdvancedPipelineType::InternalZSLYuv2JpegType] == sessionId))
        {
            result = TRUE;
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::ProcessMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AdvancedCameraUsecase::ProcessMessage(
    const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
    VOID*                       pPrivateCallbackData)
{
    Feature*              pFeature                   = NULL;
    SessionPrivateData*   pCbData                    = static_cast<SessionPrivateData*>(pPrivateCallbackData);
    const UINT32&         requestId                  = pMessageDescriptor->message.shutterMessage.frameworkFrameNum;
    ChiMessageDescriptor* pOverrideMessageDescriptor = const_cast<ChiMessageDescriptor*>(pMessageDescriptor);
    INT32                 sessionId                  = 0;

    if (ChiMessageTypeError == pMessageDescriptor->messageType)
    {
        // Convert feature mapping requestid to framework frame number
        if (TRUE == AdvancedFeatureEnabled())
        {
            pFeature = GetFeatureFromRequestMapping(pCbData->sessionId, requestId);
        }
        if (NULL != pFeature)
        {
            UINT32 resultFrameNum = GetChiFrameNumFromReqId(pCbData->sessionId, 0, requestId);

            if (INVALIDFRAMENUMBER == resultFrameNum)
            {
                CHX_LOG_ERROR("Invalid frame number");
            }
            else
            {
                CHX_LOG_INFO("Error notify ReqId to ChiOverrideFrameNum: %d <--> %d", requestId, resultFrameNum);
                pOverrideMessageDescriptor->message.errorMessage.frameworkFrameNum = resultFrameNum;
            }
        }
        ProcessErrorMessage(pMessageDescriptor);
    }
    else if (TRUE == AdvancedFeatureEnabled())
    {
        m_pResultMutex->Lock();

        pFeature = GetFeatureFromRequestMapping(pCbData->sessionId, requestId);
        if (ChiMessageTypeSof == pMessageDescriptor->messageType)
        {
            // SOF notifications are not sent to the HAL3 application
            CHX_LOG("ZSL Chi Notify SOF frameNum %u framework frameNum %u, timestamp %" PRIu64,
                    pMessageDescriptor->message.sofMessage.sofId,
                    pMessageDescriptor->message.sofMessage.frameworkFrameNum,
                    pMessageDescriptor->message.sofMessage.timestamp);

            if (NULL != pFeature)
            {
                pFeature->ProcessMessage(pMessageDescriptor, pPrivateCallbackData);
            }
        }
        else if (ChiMessageTypeShutter == pMessageDescriptor->messageType)
        {
            if (NULL != pFeature && FeatureType::HDR == pFeature->GetFeatureType())
            {
                CHX_LOG("FeatureHDR to handle shutter message it self.");
                pFeature->ProcessMessage(pMessageDescriptor, pPrivateCallbackData);
            }
            else
            {
                BOOL shouldReturnMessage = FALSE;
                sessionId = static_cast<INT32>(pCbData->sessionId);
                if (TRUE == IsRealtimeSession(sessionId))
                {
                    UINT32 resultFrameNum = GetChiFrameNumFromReqId(pCbData->sessionId, 0, requestId);

                    if (INVALIDFRAMENUMBER == resultFrameNum)
                    {
                        CHX_LOG_ERROR("Invalid frame number");
                    }
                    else
                    {
                        UINT32 frameNumIndex  = (resultFrameNum % MaxOutstandingRequests);
                        CHX_LOG("Shutter notify Realtime ReqId to AppFrameNum: %d <--> %d", requestId, resultFrameNum);

                        if (0 == m_shutterTimestamp[frameNumIndex])
                        {
                            m_shutterTimestamp[frameNumIndex] = pMessageDescriptor->message.shutterMessage.timestamp;
                            pOverrideMessageDescriptor->message.shutterMessage.frameworkFrameNum = resultFrameNum;
                            shouldReturnMessage = TRUE;
                            camera3_capture_result_t *pAppResult = GetCaptureResult(frameNumIndex);

                            // For features using past RDI frames, timestamp may not be available when snapshot meta is available,
                            // in that case meta is dispatched from here

                            if ((FALSE == IsMetadataSent(frameNumIndex)) && (NULL != pAppResult->result))
                            {
                                UINT32                  SensorTimestampTag = 0x000E0010;
                                camera_metadata_entry_t entry = { 0 };

                                camera_metadata_t* pMetadata = const_cast<camera_metadata_t*>(pAppResult->result);
                                UINT64             timestamp = m_shutterTimestamp[frameNumIndex];

                                INT32 status = find_camera_metadata_entry(pMetadata, SensorTimestampTag, &entry);

                                if (-ENOENT == status)
                                {
                                    status = add_camera_metadata_entry(pMetadata, SensorTimestampTag, &timestamp, 1);
                                }
                                else if (0 == status)
                                {
                                    status = update_camera_metadata_entry(pMetadata, entry.index, &timestamp, 1, NULL);
                                }

                                SetMetadataAvailable(frameNumIndex);
                                ProcessAndReturnFinishedResults();
                            }
                        }
                        else
                        {
                            CHX_LOG_WARN("Already have same shutter for appFrameNum:%d", resultFrameNum);
                        }
                    }
                }
                else if ((TRUE == IsOfflineSession(sessionId)) && (FALSE == IsQuadCFAUsecase()))
                {
                    UINT32 frameNum = pMessageDescriptor->message.shutterMessage.frameworkFrameNum;
                    CHX_LOG("Shutter notify Offline ReqId to AppFrameNum: %d ",frameNum);

                    UINT32 frameNumIndex = (frameNum % MaxOutstandingRequests);
                    if (0 == m_shutterTimestamp[frameNumIndex])
                    {
                        m_shutterTimestamp[frameNumIndex] = pMessageDescriptor->message.shutterMessage.timestamp;
                    }
                    shouldReturnMessage = TRUE;
                }

                if (TRUE == shouldReturnMessage)
                {
                    ReturnFrameworkMessage((camera3_notify_msg_t*)pMessageDescriptor, GetCameraId());
                }
            }
        }
        else if (ChiMessageTypeMetaBufferDone == pMessageDescriptor->messageType)
        {
            // MB done notifications are not sent to the HAL3 application
            pFeature = GetFeatureFromRequestMapping(pCbData->sessionId, requestId);

            if (NULL != pFeature)
            {
                pFeature->ProcessMessage(pMessageDescriptor, pPrivateCallbackData);
            }
            else
            {
                CHX_LOG_ERROR("Invalid feature for MetaBuffer done frameNum %u", requestId);
            }
        }
        else
        {
            ReturnFrameworkMessage((camera3_notify_msg_t*)pMessageDescriptor, GetCameraId());
        }

        m_pResultMutex->Unlock();
    }
    else
    {
        CameraUsecaseBase::SessionCbNotifyMessage(pMessageDescriptor, pPrivateCallbackData);
    }
}

UINT32 AdvancedCameraUsecase::GetMaxRequiredFrameCntForOfflineInput(
    UINT32 pipelineIndex)
{
    UINT32 maxRequiredFrameCnt = 1;
    for (UINT32 i = 0 ; i < m_enabledFeaturesCount[pipelineIndex] ; i++)
    {
        CHX_LOG_INFO("enabled feature %d, RequiredFramesForSnapshot = %d",
                     i, m_enabledFeatures[pipelineIndex][i]->GetMaxRequiredFramesForSnapshot(NULL));

        if (m_enabledFeatures[pipelineIndex][i]->GetMaxRequiredFramesForSnapshot(NULL) > maxRequiredFrameCnt)
        {
            maxRequiredFrameCnt = m_enabledFeatures[pipelineIndex][i]->GetMaxRequiredFramesForSnapshot(NULL);
        }
    }
    return maxRequiredFrameCnt;
}

// START of OEM to change section
VOID AdvancedCameraUsecase::SelectFeatures(camera3_stream_configuration_t* pStreamConfig)
{
    // OEM to change
    // this function to decide which features to run per the current pStreamConfig and static settings
    INT32  index                  = 0;
    UINT32 enabledAdvanceFeatures = 0;

    enabledAdvanceFeatures = ExtensionModule::GetInstance()->GetAdvanceFeatureMask();
    CHX_LOG("SelectFeatures(), enabled feature mask:%x", enabledAdvanceFeatures);

    // FastShutter support is there for SWMF and MFNR
    if (StreamConfigModeFastShutter == ExtensionModule::GetInstance()->GetOpMode(m_cameraId))
    {
        enabledAdvanceFeatures = AdvanceFeatureSWMF|AdvanceFeatureMFNR;
    }
    CHX_LOG("SelectFeatures(), enabled feature mask:%x", enabledAdvanceFeatures);

    for (UINT32 physicalCameraIndex = 0 ; physicalCameraIndex < m_numOfPhysicalDevices ; physicalCameraIndex++)
    {
        index = 0;
        if ((UsecaseId::PreviewZSL      == m_usecaseId)   ||
            (UsecaseId::MultiCamera     == m_usecaseId)   ||
            (UsecaseId::QuadCFA         == m_usecaseId)   ||
            (UsecaseId::VideoLiveShot   == m_usecaseId)   ||
            (UsecaseId::RawJPEG         == m_usecaseId))
        {
            if (AdvanceFeatureMFNR == (enabledAdvanceFeatures & AdvanceFeatureMFNR))
            {
                m_isOfflineNoiseReprocessEnabled = ExtensionModule::GetInstance()->EnableOfflineNoiseReprocessing();
                m_isFDstreamBuffersNeeded = TRUE;
            }

            if ((AdvanceFeatureSWMF         == (enabledAdvanceFeatures & AdvanceFeatureSWMF))   ||
                (AdvanceFeatureHDR          == (enabledAdvanceFeatures & AdvanceFeatureHDR))    ||
                ((AdvanceFeature2Wrapper    == (enabledAdvanceFeatures & AdvanceFeature2Wrapper))))
            {
                Feature2WrapperCreateInputInfo feature2WrapperCreateInputInfo;
                feature2WrapperCreateInputInfo.pUsecaseBase             = this;
                feature2WrapperCreateInputInfo.pMetadataManager         = m_pMetadataManager;
                feature2WrapperCreateInputInfo.pFrameworkStreamConfig   =
                    reinterpret_cast<ChiStreamConfigInfo*>(pStreamConfig);

                for (UINT32 i = 0; i < feature2WrapperCreateInputInfo.pFrameworkStreamConfig->numStreams; i++)
                {
                    feature2WrapperCreateInputInfo.pFrameworkStreamConfig->pChiStreams[i]->pHalStream = NULL;
                }

                if (NULL == m_pFeature2Wrapper)
                {
                    if (TRUE == IsMultiCameraUsecase())
                    {
                        if (FALSE == IsFusionStreamIncluded(pStreamConfig))
                        {
                            feature2WrapperCreateInputInfo.inputOutputType =
                                static_cast<UINT32>(InputOutputType::YUV_OUT);
                        }

                        for (UINT8 streamIndex = 0; streamIndex < m_numOfPhysicalDevices; streamIndex++)
                        {
                            feature2WrapperCreateInputInfo.internalInputStreams.push_back(m_pRdiStream[streamIndex]);
                            feature2WrapperCreateInputInfo.internalInputStreams.push_back(m_pFdStream[streamIndex]);
                        }

                        m_isFDstreamBuffersNeeded = TRUE;
                    }

                    m_pFeature2Wrapper = Feature2Wrapper::Create(&feature2WrapperCreateInputInfo, physicalCameraIndex);
                }

                m_enabledFeatures[physicalCameraIndex][index] = m_pFeature2Wrapper;
                index++;
            }
        }

        m_enabledFeaturesCount[physicalCameraIndex] = index;
    }

    if (m_enabledFeaturesCount[0] > 0)
    {
        if (NULL == m_pActiveFeature)
        {
            m_pActiveFeature = m_enabledFeatures[0][0];
        }

        CHX_LOG_INFO("num features selected:%d, FeatureType for preview:%d",
            m_enabledFeaturesCount[0], m_pActiveFeature->GetFeatureType());
    }
    else
    {
        CHX_LOG_INFO("No features selected");
    }

    m_pLastSnapshotFeature = m_pActiveFeature;

}

Feature* AdvancedCameraUsecase::SelectFeatureToExecuteCaptureRequest(
    camera3_capture_request_t* pRequest,
    UINT32                     physicalCameraIndex)
{
    // OEM to change
    // @todo add logic to select the feature to run the request
    Feature*    pFeature                = NULL;
    UINT32      enabledAdvanceFeatures  = ExtensionModule::GetInstance()->GetAdvanceFeatureMask();

    if (m_enabledFeaturesCount[physicalCameraIndex] == 0)
    {
        return NULL;
    }

    BOOL isMultiframefeature = FALSE;
    BOOL isHDRSceneMode = FALSE;
    UINT32 burstShotFps = 0;
    UINT32 customNoiseReduction = 0;
    camera_metadata_t *metadata = const_cast<camera_metadata_t*>(pRequest->settings);

    camera_metadata_entry_t entry = { 0 };

    entry.tag = ExtensionModule::GetInstance()->GetVendorTagId(VendorTag::BurstFps);
    INT32 status = find_camera_metadata_entry(metadata, entry.tag, &entry);
    if (0 == status)
    {
        burstShotFps = static_cast<UINT32>(*(entry.data.u8));
        CHX_LOG_INFO("Burst mode selected %d", burstShotFps);
    }

    entry.tag = ExtensionModule::GetInstance()->GetVendorTagId(VendorTag::CustomNoiseReduction);
    status = find_camera_metadata_entry(metadata, entry.tag, &entry);
    if (0 == status)
    {
        customNoiseReduction = static_cast<UINT32>(*(entry.data.u8));
        CHX_LOG_INFO("Custom Noise Reduction %d", customNoiseReduction);
    }


    entry.tag = ANDROID_NOISE_REDUCTION_MODE;
    status = find_camera_metadata_entry(metadata, entry.tag, &entry);

    if (0 == status)
    {
        INT32 controlNoiseReductionMode = static_cast<INT32>(*(entry.data.i32));
        CHX_LOG_INFO("Noise Reduction Mode %d", controlNoiseReductionMode);
        isMultiframefeature =
            (ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY == controlNoiseReductionMode) ? TRUE : FALSE;
    }

    entry.tag = ANDROID_CONTROL_SCENE_MODE;
    status = find_camera_metadata_entry(metadata, entry.tag, &entry);

    if (0 == status)
    {
        INT32 controlSceneMode = static_cast<INT32>(*(entry.data.i32));
        CHX_LOG_INFO("Scene Mode %d", controlSceneMode);
        isHDRSceneMode =
            (ANDROID_CONTROL_SCENE_MODE_HDR == controlSceneMode) ? TRUE : FALSE;
    }

    if ((TRUE == isHDRSceneMode) && (TRUE == isMultiframefeature))
    {
        // If both HDR and Noise Reduction High quality enabled, give priority to HDR
        isMultiframefeature = FALSE;
    }

    if ((NULL != m_pLastSnapshotFeature) &&
        (FeatureStatus::BUSY == m_pLastSnapshotFeature->GetFeatureStatus()))
    {
        CHX_LOG_INFO("Mark to reject snapshot request for frame number=%d", pRequest->frame_number);
        m_rejectedSnapshotRequestList[pRequest->frame_number % MaxOutstandingRequests] = TRUE;
    }
    else
    {
        for (UINT32 i = 0; i < m_enabledFeaturesCount[physicalCameraIndex]; i++)
        {
            if (AdvanceFeature2Wrapper == (enabledAdvanceFeatures & AdvanceFeature2Wrapper))
            {
                if (FeatureType::Feature2 == m_enabledFeatures[physicalCameraIndex][i]->GetFeatureType())
                {
                    pFeature = m_enabledFeatures[physicalCameraIndex][i];
                    break;
                }
            }
            else if ((0 == burstShotFps) &&
                     (NULL == pRequest->input_buffer) &&
                     (1 == customNoiseReduction))
            {
                if (TRUE == isMultiframefeature)
                {
                    // Select enabled multiframe feature
                    if ((FeatureType::MFNR == m_enabledFeatures[physicalCameraIndex][i]->GetFeatureType()) ||
                        (FeatureType::SWMF == m_enabledFeatures[physicalCameraIndex][i]->GetFeatureType()))
                    {
                        pFeature = m_enabledFeatures[physicalCameraIndex][i];
                        break;
                    }
                }

                if (TRUE == isHDRSceneMode)
                {
                    // Select HDR  feature
                    if (FeatureType::HDR == m_enabledFeatures[physicalCameraIndex][i]->GetFeatureType())
                    {
                        pFeature = m_enabledFeatures[physicalCameraIndex][i];
                        break;
                    }
                }
            }
        }

        if (NULL == pFeature)
        {
            if (FALSE == IsMultiCameraUsecase())
            {
                // By default choose ZSL/yuvcb feature for Single camera
                // By default SAT will be choosen for Dual Camera
                pFeature = m_enabledFeatures[physicalCameraIndex][0];
            }
        }

        m_pLastSnapshotFeature = pFeature;
    }

    if ((UsecaseId::QuadCFA == m_usecaseId) &&
        (0 == (enabledAdvanceFeatures & AdvanceFeature2Wrapper)) &&
        (0 == (enabledAdvanceFeatures & AdvanceFeatureSWMF)))
    {
        // Feature QuadCFA is selected when iso gain is below pre-defined threshold,
        // in order to trigger remosaic snapshot (non-zsl).
        //
        // If a multi-frame feature is selected according to above rules,
        // it will be added as a sub feature after qcfa remosaic snapshot,
        // other wise single remosaic snapshot will be triggered
        Feature* pFeaturebyGain = PickAdvanceFeatureByGain(m_rdiStreamIndex);

        if ((NULL != pFeaturebyGain) && (FeatureType::QuadCFA == pFeaturebyGain->GetFeatureType()))
        {
            UINT32 frameIndex = pRequest->frame_number % MaxOutstandingRequests;

            m_snapshotFeatures[frameIndex].appFrameNum = pRequest->frame_number;
            m_snapshotFeatures[frameIndex].featureInfo[m_snapshotFeatures[frameIndex].numOfFeatures].pFeature = pFeaturebyGain;
            m_snapshotFeatures[frameIndex].numOfFeatures++;

            // currently only support comination of qcfa + mfnr
            if ((NULL != pFeature) && (FeatureType::MFNR == pFeature->GetFeatureType()))
            {
                UINT32 index = m_snapshotFeatures[frameIndex].numOfFeatures;
                m_snapshotFeatures[frameIndex].featureInfo[index].pFeature = pFeature;
                m_snapshotFeatures[frameIndex].numOfFeatures++;

                CHX_LOG("Multi frame remosaic snapshot, sub Feature: pFeature:%p, type:%d",
                    pFeature, pFeature->GetFeatureType());
            }
            else
            {
                CHX_LOG_INFO("Single remosaic snapshot");
            }

            // set FeatureQuadCFA as first Feature
            pFeature = pFeaturebyGain;
        }
    }

    // Uncomment following code to enable MFNR/MFSR testing
    // pFeature = PickAdvanceFeature(pRequest);

    if (NULL != pFeature)
    {
        CHX_LOG("Select Feature %p type %d for request:%d", pFeature, pFeature->GetFeatureType(),
            pRequest->frame_number);
    }
    else
    {
        CHX_LOG_ERROR("no feature selected for request=%d", pRequest->frame_number);
    }

    // return the first Feature
    return pFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::PickAdvanceFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Feature*  AdvancedCameraUsecase::PickAdvanceFeature(camera3_capture_request_t* pRequest)
{
    Feature* pFeature = NULL;

    // Need APP to send vendor tag information for selection of feature
    // hardcoded these flags for now
    BOOL  bMultiFrame = TRUE;
    BOOL  bSWMF = FALSE;
    BOOL  bZSL = FALSE;
    BOOL  bHDR = FALSE;

    if (FALSE == bMultiFrame)
    {
        if (TRUE == bZSL)
        {
            pFeature = FindFeaturePerType(FeatureType::ZSL);
        }
        else if (TRUE == bHDR)
        {
            pFeature = FindFeaturePerType(FeatureType::HDR);
        }
    }
    else
    {
        UINT32 metaTag1 = ANDROID_SCALER_CROP_REGION;
        UINT32 metaTag2 = 0;

        camera_metadata_entry_t cropRegion;
        RefCropWindowSize       refCrop;

        find_camera_metadata_entry(const_cast<camera_metadata_t *>(pRequest->settings), metaTag1, &cropRegion);

        m_vendorTagOps.pQueryVendorTagLocation("org.quic.camera2.ref.cropsize", "RefCropSize", &metaTag2);
        m_vendorTagOps.pGetMetaData(const_cast<VOID *>(reinterpret_cast<const VOID*>(pRequest->settings)),
            metaTag2, &refCrop, sizeof(RefCropWindowSize));

        CHX_LOG_INFO("cropRegion[0]=%d, cropRegion[1]=%d, cropRegion[2]=%d, cropRegion[3]=%d",
            cropRegion.data.i32[0], cropRegion.data.i32[1],
            cropRegion.data.i32[2], cropRegion.data.i32[3]);
        CHX_LOG_INFO("refCrop.width=%d, refCrop.height=%d", refCrop.width, refCrop.height);

        if (FALSE == bSWMF)
        {
            if ((cropRegion.data.i32[2] < refCrop.width) &&
                (cropRegion.data.i32[3] < refCrop.height))
            {
                pFeature = FindFeaturePerType(FeatureType::MFSR);
            }
            else
            {
                pFeature = FindFeaturePerType(FeatureType::MFNR);
            }
        }
        else
        {
            pFeature = FindFeaturePerType(FeatureType::SWMF);
        }
    }

    return pFeature;
}

Feature*  AdvancedCameraUsecase::PickAdvanceFeatureByGain(UINT32 rdiBufferIndex)
{
    CDKResult          result                  = CDKResultSuccess;
    TargetBuffer*      pTargetBuffer           = GetTargetBufferPointer(rdiBufferIndex);
    Feature*           pFeature                = NULL;
    AECFrameControl*   pFrameCtrl              = NULL;

    if (NULL != pTargetBuffer)
    {
        UINT32             reqId                   = pTargetBuffer->lastReadySequenceID;
        ChiMetadata*       pMeta                   = pTargetBuffer->bufferQueue[reqId % BufferQueueDepth].pMetadata;

        if (NULL != pMeta)
        {
            pFrameCtrl = static_cast<AECFrameControl*>(pMeta->GetTag(
                "org.quic.camera2.statsconfigs",
                "AECFrameControl"));
        }

        if (NULL != pFrameCtrl)
        {
            FLOAT realGain = pFrameCtrl->exposureInfo[ExposureIndexSafe].linearGain;
            FLOAT AECGainThreshold = ExtensionModule::GetInstance()->AECGainThresholdForQCFA();
            CHX_LOG_VERBOSE("AEC Gain received = %f, QCFA AEC Gain Threshold = %f ", realGain, AECGainThreshold);

            if (realGain <= AECGainThreshold)
            {
                //always assuming just one realtime pipeline enabled for QuadCFA case
                pFeature = FindFeaturePerType(FeatureType::QuadCFA, 0);
            }
            else
            {
                // Since the SelectFeatureToExecuteCaptureRequest select feature at first,
                // then call PickAdvanceFeatureByGain, if QuadCFA Feature is not selected,
                // set feature to NULL here, SelectFeatureToExecuteCaptureRequest will use
                // the feature which selected at first.
                pFeature = NULL;
            }
        }
        else
        {
            CHX_LOG_WARN("Can't get AECFrameControl from metadata");
        }
    }
    else
    {
        CHX_LOG_WARN("pTargetBuffer is NULL");
    }

    return pFeature;
}

// END of OEM to change section

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AdvancedCameraUsecase::GetTargetIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT AdvancedCameraUsecase::GetTargetIndex(ChiTargetPortDescriptorInfo* pTargets, const char* pTargetName)
{
    INT index = -1;

    for (UINT i = 0; i < pTargets->numTargets; i++)
    {
        if (0 == strcmp(pTargetName, pTargets->pTargetPortDesc[i].pTargetName))
        {
            index = i;
            break;
        }
    }

    if (-1 == index)
    {
        CHX_LOG_ERROR("Could not find %s in %p", pTargetName, pTargets);
    }

    return index;
}

VOID AdvancedCameraUsecase::SetupSharedPipeline(
    ChiPipelineTargetCreateDescriptor* pPipeline,
    AdvancedPipelineType pipelineType)
{
    UINT                               previewIndex;
    UINT                               rdiIndex;
    UINT                               fdIndex;
    UINT                               videoIndex;
    ChiTargetPortDescriptorInfo*       pSinkTarget = NULL;

    CHX_LOG_INFO("SetupSharedPipeline for %p type %d", pPipeline, pipelineType);

    /// @todo: For ZSLSnapshotYUVType/InternalZSLYuv2JpegType,
    /// in each feature, it will set input/output stream for required pipeline,
    /// but better to move it to here, as they're shared pipelines.

    switch (pipelineType)
    {
    case AdvancedPipelineType::ZSLPreviewRawType:
    case AdvancedPipelineType::ZSLPreviewRawFSType:
    {
        // Setup the raw target with the RDI stream. Raw has only one buffer format, it will either match here, or
        // not matter in the output format because none of its will match (another assumption)
        previewIndex = GetTargetIndex(&pPipeline->sinkTarget, "TARGET_BUFFER_DISPLAY");
        rdiIndex     = GetTargetIndex(&pPipeline->sinkTarget, "TARGET_BUFFER_RAW");
        fdIndex      = GetTargetIndex(&pPipeline->sinkTarget, "TARGET_BUFFER_FD");

        if (CDKInvalidId != previewIndex)
        {
            pPipeline->sinkTarget.pTargetPortDesc[previewIndex].pTarget->pChiStream = m_pPreviewStream;
        }
        if (CDKInvalidId != rdiIndex)
        {
            pPipeline->sinkTarget.pTargetPortDesc[rdiIndex].pTarget->pChiStream = m_pRdiStream[0];
        }
        if (CDKInvalidId != fdIndex)
        {
            pPipeline->sinkTarget.pTargetPortDesc[fdIndex].pTarget->pChiStream = m_pFdStream[0];
            m_isFdStreamSupported = TRUE;
        }
    }
        break;
    case AdvancedPipelineType::ZSLPreviewRawYUVType:
    {
        previewIndex = GetTargetIndex(&pPipeline->sinkTarget, "TARGET_BUFFER_DISPLAY");
        rdiIndex     = GetTargetIndex(&pPipeline->sinkTarget, "TARGET_BUFFER_RAW");
        videoIndex      = GetTargetIndex(&pPipeline->sinkTarget, "TARGET_BUFFER_VIDEO");

        if (CDKInvalidId != previewIndex)
        {
            pPipeline->sinkTarget.pTargetPortDesc[previewIndex].pTarget->pChiStream = m_pPreviewStream;
        }
        if (CDKInvalidId != rdiIndex)
        {
            pPipeline->sinkTarget.pTargetPortDesc[rdiIndex].pTarget->pChiStream = m_pRdiStream[0];
        }
        if (CDKInvalidId != videoIndex)
        {
            pPipeline->sinkTarget.pTargetPortDesc[videoIndex].pTarget->pChiStream = m_pVideoStream;
        }
    }
        break;

    default:
        break;
    }
}

ChiStream*  AdvancedCameraUsecase::GetSharedStream(
    SharedStreamType streamType,
    UINT32 pipelineIndex)
{
    ChiStream* pStream = NULL;
    switch (streamType)
    {
        case AdvancedCameraUsecase::SnapshotStream:
        {
            pStream = m_pSnapshotStream;
            break;
        }
        case AdvancedCameraUsecase::PreviewStream:
        {
            pStream = m_pPreviewStream;
            break;
        }
        case AdvancedCameraUsecase::RdiStream:
        {
            pStream = m_pRdiStream[pipelineIndex];
            break;
        }
        case AdvancedCameraUsecase::FdStream:
        {
            pStream = m_pFdStream[pipelineIndex];
            break;
        }
        case AdvancedCameraUsecase::Bayer2YuvStream:
        {
            pStream = m_pBayer2YuvStream[pipelineIndex];
            break;
        }
        case AdvancedCameraUsecase::JPEGInputStream:
        {
            pStream = m_pJPEGInputStream[pipelineIndex];
            break;
        }
        case AdvancedCameraUsecase::YuvCBStream:
        {
            pStream = m_pYuvCBStream;
            break;
        }
        case AdvancedCameraUsecase::VideoStream:
        {
            pStream = m_pVideoStream;
            break;
        }
        case AdvancedCameraUsecase::YuvInputStream:
        {
            pStream = m_pYuvInStream;
            break;
        }
        default:
            CHX_LOG_ERROR("Unhandled stream type %d", streamType);
            break;
    }

    return pStream;
}

VOID AdvancedCameraUsecase::ConfigureStream(
    LogicalCameraInfo*              pCameraInfo,
    camera3_stream_configuration_t* pStreamConfig)
{
    ChiStream*  pTempStream;
    if (UsecaseId::VideoLiveShot == m_usecaseId)
    {
        for (UINT32 stream = 0; stream < pStreamConfig->num_streams; stream++)
        {
            pTempStream = reinterpret_cast<ChiStream*>(pStreamConfig->streams[stream]);

            if ((ChiStreamTypeOutput == pTempStream->streamType) &&
                (ChiStreamFormatBlob == pTempStream->format))
            {
                m_pSnapshotStream = pTempStream;
            }
            else if ((ChiStreamTypeOutput   == pTempStream->streamType) &&
                     (GRALLOC_USAGE_HW_VIDEO_ENCODER & pTempStream->grallocUsage))
            {
                m_pVideoStream = pTempStream;
            }
            else
            {
                m_pPreviewStream = pTempStream;
            }
        }

        UsecaseSelector::getSensorDimension(pCameraInfo->ppDeviceInfo[0]->cameraId,
            pStreamConfig,
            &m_pRdiStream[0]->width,
            &m_pRdiStream[0]->height,
            1);
    }
    else if (UsecaseId::YUVInBlobOut == m_usecaseId)
    {
        UINT32  inputStreamWidth  = 0;
        UINT32  inputStreamHeight = 0;
        for (UINT32 stream = 0; stream < pStreamConfig->num_streams; stream++)
        {
            pTempStream = reinterpret_cast<ChiStream*>(pStreamConfig->streams[stream]);
            if ((ChiStreamTypeInput == pTempStream->streamType) &&
                (ChiStreamFormatYCbCr420_888 == pTempStream->format))
            {
                m_pYuvInStream      = pTempStream;
                inputStreamWidth    = m_pYuvInStream->width;
                inputStreamHeight   = m_pYuvInStream->height;
                break;
            }
        }
        for (UINT32 stream = 0; stream < pStreamConfig->num_streams; stream++)
        {
            pTempStream = reinterpret_cast<ChiStream*>(pStreamConfig->streams[stream]);
            if ((ChiStreamTypeOutput         == pTempStream->streamType) &&
                (ChiStreamFormatYCbCr420_888 == pTempStream->format) &&
                (inputStreamWidth            == pTempStream->width) &&
                (inputStreamHeight           == pTempStream->height))
            {
                m_pYuvCBStream = pTempStream;
            }
            else if ((ChiStreamTypeOutput == pTempStream->streamType) &&
                     (ChiStreamFormatBlob == pTempStream->format))
            {
                m_pSnapshotStream = pTempStream;
            }
            else if ((ChiStreamTypeInput          == pTempStream->streamType) &&
                     (ChiStreamFormatYCbCr420_888 == pTempStream->format))
            {
                m_pYuvInStream = pTempStream;
            }
            else
            {
                if ((ChiStreamTypeOutput == pTempStream->streamType) &&
                        ((ChiStreamFormatImplDefined == pTempStream->format) ||
                         (ChiStreamFormatYCbCr420_888 == pTempStream->format)))
                {
                    m_pPreviewStream = pTempStream;
                }
                else
                {
                    CHX_LOG_ERROR("Unknown stream type for YUVInBlobOut usecase");
                }
            }
        }
    }
    else
    {
        // For multi camera, preview stream will NOT come into advance
        if (FALSE == IsMultiCameraUsecase())
        {
            m_pPreviewStream  = reinterpret_cast<ChiStream*>(pStreamConfig->streams[0]);
            m_pSnapshotStream = reinterpret_cast<ChiStream*>(pStreamConfig->streams[1]);

            // Preview output is up to the SOC, camera directly feeds display, so we match the preview to the stream with format
            if ((ChiStreamFormatImplDefined == m_pSnapshotStream->format) &&
                (FALSE == UsecaseSelector::IsHEIFStream(reinterpret_cast<camera3_stream_t*>(m_pSnapshotStream))))
            {
                m_pPreviewStream  = reinterpret_cast<ChiStream*>(pStreamConfig->streams[1]);
                m_pSnapshotStream = reinterpret_cast<ChiStream*>(pStreamConfig->streams[0]);
            }

            if (pStreamConfig->num_streams > 2 && UsecaseSelector::HasHeicSnapshotStream(pStreamConfig))
            {
                m_pYuvCBStream = reinterpret_cast<ChiStream*>(pStreamConfig->streams[pStreamConfig->num_streams - 1]);
            }
        }
        else
        {
            m_pPreviewStream  = NULL;
            m_pSnapshotStream = reinterpret_cast<ChiStream*>(pStreamConfig->streams[0]);
        }

        if (UsecaseId::QuadCFA == m_usecaseId)
        {
            UsecaseSelector::getSensorDimension(pCameraInfo->ppDeviceInfo[0]->cameraId,
                pStreamConfig,
                &m_QuadCFASensorInfo.fullSizeSensorOutput.width,
                &m_QuadCFASensorInfo.fullSizeSensorOutput.height,
                1);

            CHX_LOG("QCFA sensor full size output: %dx%d",
                m_QuadCFASensorInfo.fullSizeSensorOutput.width,
                m_QuadCFASensorInfo.fullSizeSensorOutput.height);

            //alwasy assuming there is only one realtime pipeline for QuadraCfa
            m_pRdiStream[0]->width  = m_QuadCFASensorInfo.sensorModePickHint.sensorOutputSize.width;
            m_pRdiStream[0]->height = m_QuadCFASensorInfo.sensorModePickHint.sensorOutputSize.height;

            CHX_LOG("QCFA sensor set rdi stream to %dx%d",
                m_pRdiStream[0]->width, m_pRdiStream[0]->height);
        }
        else if (FALSE == m_isRdiStreamImported)
        {
            for (UINT32 i = 0 ; i < m_numOfPhysicalDevices; i++)
            {
                UsecaseSelector::getSensorDimension(pCameraInfo->ppDeviceInfo[i]->cameraId,
                                                    pStreamConfig,
                                                    &m_pRdiStream[i]->width,
                                                    &m_pRdiStream[i]->height,
                                                    1);
            }
        }

    }

    if ((NULL != m_pPreviewStream) && (NULL != m_pSnapshotStream))
    {
        CHX_LOG("UsecaseId:%u preview stream:%p, wrapper:%p, snapshot stream:%p, wrapper:%p",
                m_usecaseId, m_pPreviewStream, m_pPreviewStream->pPrivateInfo,
                m_pSnapshotStream, m_pSnapshotStream->pPrivateInfo);

        m_previewAspectRatio = static_cast<FLOAT>(m_pPreviewStream->width) / m_pPreviewStream->height;
    }

    for (UINT32 i = 0; i < m_numOfPhysicalDevices; i++)
    {
        m_pRdiStream[i]->format = ChiStreamFormatRaw10;
        if (UsecaseZSLTuningId == static_cast<UsecaseId3Target>(ExtensionModule::GetInstance()->OverrideUseCase()))
        {
            m_pRdiStream[i]->format = ChiStreamFormatRaw16;
        }

        // Create the RDI stream output based on the input buffer requirements to generate the snapshot stream buffer
        m_pRdiStream[i]->maxNumBuffers = 0;
        m_pRdiStream[i]->rotation = StreamRotationCCW90;
        m_pRdiStream[i]->streamType = ChiStreamTypeBidirectional;
        m_pRdiStream[i]->grallocUsage = 0;
    }

    ConfigFdStream();
}

VOID AdvancedCameraUsecase::BuildUsecase(
    LogicalCameraInfo*              pCameraInfo,
    camera3_stream_configuration_t* pStreamConfig)
{
    AdvancedPipelineType    pipelines[MaxRealTimePipelines][AdvancedPipelineType::PipelineCount];
    AdvancedPipelineType    featurePipelines[AdvancedPipelineType::PipelineCount];
    UINT32                  pipelineCounts[MaxRealTimePipelines] = { 0 };
    UINT32                  totalPipelineCount = 0;
    UINT32                  featurePipelineCount = 0;
    UINT32                  pipelineTypes[MaxPipelines];
    UINT                    cameraIds[MaxPipelines]          = {0};
    UINT32                  pipelineToSession[MaxPipelines]  = {0};
    UINT32                  pipelineIDMap[MaxPipelines]      = {0};
    UINT32                  descIndex = 0;

    // Get the pipeline requirments for feature
    for (UINT32 physicalCameraIndex = 0 ; physicalCameraIndex < m_numOfPhysicalDevices; physicalCameraIndex++)
    {
        for (UINT32 i = 0; i < m_enabledFeaturesCount[physicalCameraIndex]; i++)
        {
            featurePipelineCount = m_enabledFeatures[physicalCameraIndex][i]->GetRequiredPipelines(
                &featurePipelines[0], AdvancedPipelineType::PipelineCount);

            // insert to the unique pipeline array
            for (UINT32 n = 0; n < featurePipelineCount; n++)
            {
                UINT32 m = 0;
                for (m = 0; m < pipelineCounts[physicalCameraIndex]; m++)
                {
                    if (pipelines[physicalCameraIndex][m] == featurePipelines[n])
                        break;
                }
                if (m == pipelineCounts[physicalCameraIndex])
                {
                    // not found, add it to the end
                    pipelines[physicalCameraIndex][m] = featurePipelines[n];
                    pipelineTypes[totalPipelineCount] = featurePipelines[n];
                    pipelineCounts[physicalCameraIndex]++;
                    totalPipelineCount++;
                }
            }
        }
    }

    m_pipelineCount             = totalPipelineCount;

    // rebuild m_pChiUsecase based on feature requirements
    for(UINT32 i = 0 ; i < totalPipelineCount ; i++)
    {
        pipelineIDMap[i] = AdvancedPipelineIdMapping[pipelineTypes[i]];
    }

    if (UsecaseZSLTuningId == static_cast<UsecaseId3Target>(ExtensionModule::GetInstance()->OverrideUseCase()))
    {
        m_pClonedUsecase = UsecaseSelector::CloneUsecase(pZslTuningUsecase, totalPipelineCount, pipelineIDMap);
    }
    else
    {
        m_pClonedUsecase = UsecaseSelector::CloneUsecase(pAdvancedUsecase, totalPipelineCount, pipelineIDMap);
    }

    if (NULL != m_pClonedUsecase)
    {
        CHX_LOG_INFO("Clone usecase successfully!");
        m_isUsecaseCloned = TRUE;
        UsecaseSelector::PruneUsecaseByStreamConfig(pStreamConfig, m_pClonedUsecase, &m_pChiUsecase);
    }
    // Map camera ID and session ID for the pipeline and prepare for pipeline/session creation
    for (UINT32 physicalCameraIndex = 0 ; physicalCameraIndex < m_numOfPhysicalDevices; physicalCameraIndex++)
    {
        UINT32 rtOffset = physicalCameraIndex << InstanceOffset;
        for(UINT32 i = 0 ; i < pipelineCounts[physicalCameraIndex]; i++)
        {
            m_pipelineStatus[descIndex].pipelineId      = pipelines[physicalCameraIndex][i] + rtOffset;
            m_pipelineStatus[descIndex].used            = TRUE;
            cameraIds[descIndex]                        = m_cameraIdMap[physicalCameraIndex];
            pipelineToSession[descIndex]                = m_pipelineToSession[pipelines[physicalCameraIndex][i] + rtOffset];

            if (FALSE == IsMultiCameraUsecase())
            {
                if ((NULL != m_pChiUsecase) && (NULL != m_pChiUsecase->pPipelineTargetCreateDesc))
                {
                    SetupSharedPipeline(&m_pChiUsecase->pPipelineTargetCreateDesc[descIndex], pipelines[physicalCameraIndex][i]);
                }
            }
            else
            {
                CHX_LOG("Don't create realtime pipeline in multi camera usecase.");
            }

            if ((AdvancedPipelineType::ZSLPreviewRawType    == pipelines[physicalCameraIndex][i]) ||
                (AdvancedPipelineType::ZSLPreviewRawFSType  == pipelines[physicalCameraIndex][i]) ||
                (AdvancedPipelineType::ZSLPreviewRawYUVType == pipelines[physicalCameraIndex][i]))
            {
                m_realtimeSessionId = m_pipelineToSession[pipelines[physicalCameraIndex][i] + rtOffset];
                CHX_LOG("realtime session id:%d", m_realtimeSessionId);
            }
            descIndex ++;
        }
    }

    m_numSessions = m_uniqueSessionId;
    SetPipelineToSessionMapping(m_numSessions, totalPipelineCount, pipelineToSession);
    SetPipelineToCameraMapping(totalPipelineCount, cameraIds);

    //override stream for features
    for (UINT32 physicalCameraIndex = 0 ; physicalCameraIndex < m_numOfPhysicalDevices; physicalCameraIndex++)
    {
        for (UINT32 i = 0; i < m_enabledFeaturesCount[physicalCameraIndex]; i++)
        {
            m_enabledFeatures[physicalCameraIndex][i]->OverrideUsecase(pCameraInfo, pStreamConfig);
        }
    }
}

Feature* AdvancedCameraUsecase::FindFeatureToProcessResult(
    const ChiPrivateData*             pPrivData,
    UINT32                      frameworkFrameNum,
    VOID*                       pPrivateCallbackData)
{
    SessionPrivateData* pCbData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
    Feature* pFeature = NULL;

    if ((NULL != pPrivData) && (FeatureType::UNINIT != pPrivData->featureType))
    {
        pFeature = FindFeaturePerType(static_cast<FeatureType>(pPrivData->featureType), pPrivData->streamIndex);
    }
    else
    {
        pFeature = GetFeatureFromRequestMapping(pCbData->sessionId, frameworkFrameNum);
    }

    if (NULL != pFeature)
    {
        CHX_LOG("ProcessResult SessionId %d frameNum %d feature type: %d",
            pCbData->sessionId, frameworkFrameNum, pFeature->GetFeatureType());
    }

    return pFeature;
}

Feature*  AdvancedCameraUsecase::FindFeaturePerType(
    FeatureType type,
    UINT32      physicalCameraIndex)
{
    Feature* pFeature = NULL;
    for (UINT32 i = 0; i < m_enabledFeaturesCount[physicalCameraIndex]; i++)
    {
        pFeature = m_enabledFeatures[physicalCameraIndex][i];
        if (pFeature->GetFeatureType() == type)
            break;
        else
            pFeature = NULL;
    }

    return pFeature;
}

ChiPipelineTargetCreateDescriptor AdvancedCameraUsecase::GetPipelineDescriptorFromUsecase(
    ChiUsecase* pChiUsecase,
    AdvancedPipelineType id)
{
    if (UsecaseZSLTuningId == static_cast<UsecaseId3Target>(ExtensionModule::GetInstance()->OverrideUseCase()))
        return pChiUsecase->pPipelineTargetCreateDesc[AdvancedPipelineIdTuningMapping[id]];
    else
        return pChiUsecase->pPipelineTargetCreateDesc[AdvancedPipelineIdMapping[id]];
}

INT32 AdvancedCameraUsecase::GetPipelineIdByAdvancedPipelineType(
    AdvancedPipelineType type,
    UINT32               physicalCameraIndex)
{
    INT32 index = -1;
    UINT32 pipelineId = (physicalCameraIndex << InstanceOffset) + type;
    for (UINT32 i = 0; i < m_pipelineCount; i++)
    {
        if (m_pipelineStatus[i].pipelineId == pipelineId)
        {
            index = i;
            break;
        }
    }

    return index;
}

INT32  AdvancedCameraUsecase::GetAdvancedPipelineTypeByPipelineId(INT32 descriptorIndex)
{
    return m_pipelineStatus[descriptorIndex].pipelineId & PipelineTypeMask;
}

UINT64 AdvancedCameraUsecase::GetRequestShutterTimestamp(UINT64 frameNumber)
{
    UINT32 resultFrameIndex = frameNumber % MaxOutstandingRequests;
    return m_shutterTimestamp[resultFrameIndex];
}

VOID     AdvancedCameraUsecase::SetRequestToFeatureMapping(UINT sessionId, UINT64 requestId, Feature* pFeature)
{
    m_pSetFeatureMutex->Lock();

    const UINT size = sizeof(m_sessionRequestIdFeatureMapping) / sizeof(SessionRequestIdFeatureMapping);
    UINT       index = m_sessionRequestIdFeatureMappingCount % size;

    m_sessionRequestIdFeatureMapping[index].sessionId = sessionId;
    m_sessionRequestIdFeatureMapping[index].requestId = requestId;
    m_sessionRequestIdFeatureMapping[index].pFeature  = pFeature;

    CHX_LOG("Current index %d RequestCount %d sessionId %d requestId %" PRIu64 " pFeature %p type %d",
                  index, m_sessionRequestIdFeatureMappingCount,
                  sessionId, requestId, pFeature, pFeature->GetFeatureType());

    m_sessionRequestIdFeatureMappingCount++;
    m_pSetFeatureMutex->Unlock();
}

Feature* AdvancedCameraUsecase::GetFeatureFromRequestMapping(UINT sessionId, UINT64 requestId)
{
    m_pSetFeatureMutex->Lock();

    const UINT size = sizeof(m_sessionRequestIdFeatureMapping) / sizeof(SessionRequestIdFeatureMapping);
    Feature*   pFeature = NULL;
    UINT       index = 0;
    for (index = 0; index < size; index++)
    {
        if (m_sessionRequestIdFeatureMapping[index].sessionId == sessionId &&
            m_sessionRequestIdFeatureMapping[index].requestId == requestId)
        {
            pFeature = m_sessionRequestIdFeatureMapping[index].pFeature;
            break;
        }
    }
    if (NULL != pFeature)
    {
        CHX_LOG("Look for feature with current index %d sessionId %d requestId %" PRIu64 " pFeature %p type %d",
                  index, sessionId, requestId, pFeature, pFeature->GetFeatureType());
    }

    m_pSetFeatureMutex->Unlock();

    return pFeature;
}

/*
    * return a unique session id for the given pipeline group
    * if any id in the pipelineGroup already has the session id, then
    * all of the pipeline will share this session id
    *
    * return a new session id if none of the pipeline id was associated with any session
    * return the existing if one of the pipeline id is already associated with a session
    */
UINT32 AdvancedCameraUsecase::GetUniqueSessionId(
    AdvancedPipelineType*   pipelineGroup,
    UINT32                  pipelineCount,
    UINT32                  physicalCameraIndex)
{
    UINT32 sessionId     = InvalidSessionId;
    UINT32 uniqueId      = 0;
    UINT32 rtOffset      = physicalCameraIndex << InstanceOffset;
    UINT32 pipelineIndex = 0;

    for (UINT32 i = 0; i < pipelineCount; i++)
    {
        pipelineIndex = rtOffset + pipelineGroup[i];

        if (m_pipelineToSession[pipelineIndex] != (INT) InvalidSessionId)
        {
            sessionId = m_pipelineToSession[pipelineIndex];
        }
    }

    if (sessionId != InvalidSessionId)
    {
        uniqueId = sessionId;
    }
    else
    {
        uniqueId = m_uniqueSessionId++;
    }

    for (UINT32 i = 0; i < pipelineCount; i++)
    {
        pipelineIndex = rtOffset + pipelineGroup[i];
        m_pipelineToSession[pipelineIndex] = uniqueId;
    }

    CHX_LOG_INFO("return uniqueSessionId: %d", uniqueId);
    return uniqueId;
}

/*
 * Set the cameraId for each pipeline
 */
VOID  AdvancedCameraUsecase::SetPipelineCameraId(
    AdvancedPipelineType*   pipelineGroup,
    UINT32*                 cameraId,
    UINT32                  pipelineCount)
{
    for (UINT32 i = 0; i < pipelineCount; i++)
    {
        m_pipelineToCameraId[pipelineGroup[i]] = cameraId[i];
    }

    return;
}

UINT AdvancedCameraUsecase::GetPhysicalCameraId(UINT32          physicalDeviceIndex)
{
    UINT cameraId = InvalidPhysicalCameraId;

    if (physicalDeviceIndex < m_numOfPhysicalDevices)
    {
        cameraId = m_cameraIdMap[physicalDeviceIndex];
    }
    else
    {
        CHX_LOG_ERROR("Invalid physical camera index:%d", physicalDeviceIndex);
    }

    CHX_LOG_INFO("Physical Camera index %d camera Id %d", physicalDeviceIndex, cameraId);

    return cameraId;
}

const CHISENSORMODEPICKHINT* AdvancedCameraUsecase::GetSensorModePickHint(UINT pipelineIndex)
{
    const CHISENSORMODEPICKHINT* pSensorModePickHint = NULL;
    AdvancedPipelineType         pipelineType        = AdvancedPipelineType::PipelineCount;

    pipelineType = static_cast<AdvancedPipelineType>(GetAdvancedPipelineTypeByPipelineId(pipelineIndex));
    CHX_LOG("pipelineIndex:%d, AdvancedPipelineType:%d", pipelineIndex, pipelineType);

    if (TRUE == IsQuadCFAUsecase())
    {
        switch (GetAdvancedPipelineTypeByPipelineId(pipelineIndex))
        {
            // For pipeline QuadCFARemosaicSnapshotType, need to override to select QCFA capability sensor mode.
            // So we can choose the "Full size" sensor mode.
            case AdvancedPipelineType::QuadCFASnapshotYuvType:
            case AdvancedPipelineType::QuadCFARemosaicType:
            case AdvancedPipelineType::QuadCFAFullSizeRawType:
            {
                pSensorModePickHint = &m_defaultSensorModePickHint;
                break;
            }

            // Don't override sensor mode pick option for realtime pipeline.
            case AdvancedPipelineType::ZSLPreviewRawType:
            case AdvancedPipelineType::ZSLPreviewRawFSType:
            {
                pSensorModePickHint = NULL;
                break;
            }

            // If needed, query sensor mode pick hint from each Feature based on pipeline type and enabled Features
            default:
            {
                pSensorModePickHint = &(m_QuadCFASensorInfo.sensorModePickHint);
                break;
            }
        }
    }
    else
    {
        BOOL isInSensorHDR3ExpPreview = (SelectInSensorHDR3ExpUsecase::InSensorHDR3ExpPreview ==
                                         ExtensionModule::GetInstance()->SelectInSensorHDR3ExpUsecase())? TRUE: FALSE;

        pSensorModePickHint = &m_defaultSensorModePickHint;
        if (TRUE == isInSensorHDR3ExpPreview)
        {
            m_defaultSensorModePickHint.sensorModeCaps.value  = 0;
            m_defaultSensorModePickHint.sensorModeCaps.u.IHDR = 1;
        }
    }

    return pSensorModePickHint;
}

VOID    AdvancedCameraUsecase::ConfigRdiStream(
    ChiSensorModeInfo* pSensorInfo,
    UINT32             physicalCameraIndex)
{
    if (NULL == pSensorInfo || NULL == m_pRdiStream[physicalCameraIndex])
    {
        CHX_LOG_ERROR("Buffer NULL pSensorInfo:%p m_pRdiStream:%p",
            pSensorInfo, m_pRdiStream[physicalCameraIndex]);
        return;
    }

    // configure the shared Rdi stream
    if ((pSensorInfo->frameDimension.width > m_pRdiStream[physicalCameraIndex]->width) ||
        (pSensorInfo->frameDimension.height > m_pRdiStream[physicalCameraIndex]->height))
    {
        m_pRdiStream[physicalCameraIndex]->format        = ChiStreamFormatRaw10;
        if (UsecaseZSLTuningId == static_cast<UsecaseId3Target>(ExtensionModule::GetInstance()->OverrideUseCase()))
            m_pRdiStream[physicalCameraIndex]->format    = ChiStreamFormatRaw16;

        // Create the RDI stream output based on the input buffer requirements to generate the snapshot stream buffer
        m_pRdiStream[physicalCameraIndex]->width         = pSensorInfo->frameDimension.width;
        m_pRdiStream[physicalCameraIndex]->height        = pSensorInfo->frameDimension.height;
        m_pRdiStream[physicalCameraIndex]->maxNumBuffers = 0;
        m_pRdiStream[physicalCameraIndex]->rotation      = StreamRotationCCW90;
        m_pRdiStream[physicalCameraIndex]->streamType    = ChiStreamTypeBidirectional;
        m_pRdiStream[physicalCameraIndex]->grallocUsage  = 0;

        CHX_LOG("ConfigRdiStream, wxh:%dx%d", m_pRdiStream[physicalCameraIndex]->width,
            m_pRdiStream[physicalCameraIndex]->height);
    }
    else
    {
        CHX_LOG_WARN("RdiStream already configured, m_pRdiStream %p wxh:%dx%d",
            m_pRdiStream[physicalCameraIndex],
            m_pRdiStream[physicalCameraIndex]->width,
            m_pRdiStream[physicalCameraIndex]->height);
    }
}

VOID    AdvancedCameraUsecase::ConfigFdStream()
{
    // Default FD stream dimensions
    UINT32            width  = 640;
    UINT32            height = 480;

    FLOAT currentAspectRatio = static_cast<FLOAT>(width) / static_cast<FLOAT>(height);

    if (currentAspectRatio < m_previewAspectRatio)
    {
        height = static_cast<UINT32>(static_cast<FLOAT>(width) / m_previewAspectRatio);
    }
    else if (currentAspectRatio > m_previewAspectRatio)
    {
        width = static_cast<UINT32>(static_cast<FLOAT>(height) * m_previewAspectRatio);
    }

    width  = ChxUtils::EvenCeilingUINT32(width);
    height = ChxUtils::EvenCeilingUINT32(height);

    for(UINT32 i = 0; i < m_numOfPhysicalDevices; i++)
    {
        if(FALSE == IsMultiCameraUsecase())
        {
            m_pFdStream[i]->width      = width;
            m_pFdStream[i]->height     = height;
        }
        m_pFdStream[i]->format         = ChiStreamFormatYCbCr420_888;
        m_pFdStream[i]->maxNumBuffers  = 0;
        m_pFdStream[i]->rotation       = StreamRotationCCW90;
        m_pFdStream[i]->streamType     = ChiStreamTypeOutput;
        m_pFdStream[i]->grallocUsage   = 0;
    }
}

CDKResult AdvancedCameraUsecase::DestroyOfflinePipelineDescriptors()
{
    CDKResult result = CDKResultSuccess;

    for (UINT session = 0; session < m_numSessions; session++)
    {
        SessionData* pData = &m_sessions[session];

        if (0 == pData->numPipelines)
        {
            continue;
        }

        CHX_LOG("sessionidx:%d, session:%p, numPipelines:%d, isRealTime:%d",
            session, pData->pSession, pData->numPipelines, pData->pipelines[0].pPipeline->IsRealTime());

        if (TRUE == pData->pipelines[0].pPipeline->IsRealTime())
        {
            continue;
        }

        for (UINT pipeline = 0; pipeline < pData->numPipelines; pipeline++)
        {
            for (UINT stream = 0; stream < pData->pipelines[pipeline].numStreams; stream++)
            {
                // Anything we need to delete with streams?
                pData->pipelines[pipeline].pStreams[stream] = NULL;
            }

            if (NULL != pData->pipelines[pipeline].pPipeline)
            {
                CHX_LOG("Destroying pipeline:%p", pData->pipelines[pipeline].pPipeline);
                pData->pipelines[pipeline].pPipeline->Destroy();
                pData->pipelines[pipeline].pPipeline = NULL;
            }

            PipelineDestroyed(session, pipeline);

            pData->pipelines[pipeline].id = InvalidId;
        }

        pData->numPipelines = 0;
    }

    return result;
}

CDKResult AdvancedCameraUsecase::CreateOfflinePipelineDescriptors()
{
    CDKResult result = CDKResultSuccess;

    for (UINT i = 0; i < m_pChiUsecase->numPipelines; i++)
    {
        CHX_LOG("pipelineindex:%d, isrealtime:%d",
            i, m_pChiUsecase->pPipelineTargetCreateDesc[i].pipelineCreateDesc.isRealTime);

        // make sure the initial SessionData::numPipelines is 0 for all offline sessions!!

        if (FALSE == m_pChiUsecase->pPipelineTargetCreateDesc[i].pipelineCreateDesc.isRealTime)
        {
            // use mapping if available, otherwise default to 1-1 mapping
            UINT sessionId  = (NULL != m_pPipelineToSession) ? m_pPipelineToSession[i] : i;
            UINT pipelineId = m_sessions[sessionId].numPipelines++;

            m_sessions[sessionId].pipelines[pipelineId].id = i;// Assign the ID to pipelineID

            CHX_LOG("Creating Pipeline %s at index %u for session %u, session's pipeline %u, camera id:%d",
                m_pChiUsecase->pPipelineTargetCreateDesc[i].pPipelineName, i, sessionId, pipelineId, m_pPipelineToCamera[i]);

            result = CreatePipeline(m_pPipelineToCamera[i],
                                    &m_pChiUsecase->pPipelineTargetCreateDesc[i],
                                    &m_sessions[sessionId].pipelines[pipelineId]);

            PipelineCreated(sessionId, pipelineId);
        }
    }

    return result;
}

CDKResult AdvancedCameraUsecase::RestartOfflineSessions(
    BOOL reCreatePipeline)
{
    CDKResult result = CDKResultSuccess;

    CHX_LOG("E. reCreatePipeline:%d", reCreatePipeline);

    DestroyOfflineSessions();

    if (TRUE == reCreatePipeline)
    {
        result = DestroyOfflinePipelineDescriptors();

        if (CDKResultSuccess == result)
        {
            result = CreateOfflinePipelineDescriptors();
        }
    }

    if (CDKResultSuccess == result)
    {
        // @todo: move this to thread
        result = CreateOfflineSessions(m_pCallbacks);
    }

    CHX_LOG("X");
    return result;
}

CDKResult AdvancedCameraUsecase::ReconfigureRealtimeSession(
    UINT oldSessionId,
    UINT newSessionId,
    BOOL restartOfflineSessions)
{
    CDKResult result          = CDKResultSuccess;
    UINT      rtPipelineIndex = 0;
    BOOL      isForced        = FALSE;

    CHX_LOG("oldSessionId:%d, newSessionId:%d, restartOfflineSessions:%d", oldSessionId, newSessionId, restartOfflineSessions);

    // For Preview session we must call destroy session with isForced == TRUE,
    // otherwise WaitTillAllResultsAvailable() will timeout,
    // because AF node doesn't release buffer reference of last three (buffer_delta == 3) IFE RDI buffers.
    if (m_realtimeSessionId == oldSessionId)
    {
        isForced = TRUE;
    }

    // wait if flush is in progress
    AcquireRTConfigLock();

    CHX_LOG("destroy current RT session. sessionid: %d", oldSessionId);

    result = DestroySessionByID(oldSessionId, isForced);

    if (CDKResultSuccess == result)
    {
        /// When session is destroyed, also destroy the pipeline descriptor and recreate it.
        /// Because the pipeline obj will pass to session,
        /// if the pipeline descriptor is not recreated, it will fail during next session initialize
        result = DestroyPipelineDescriptorsPerSession(oldSessionId);

        if (CDKResultSuccess == result)
        {
            result = CreatePipelineDescriptorsPerSession(oldSessionId);
        }
    }

    if ((CDKResultSuccess == result) && (TRUE == restartOfflineSessions))
    {
        // during re create pipeline destriptor, will trigger buffer negotiation again
        // and re configure RDI stream size, reset to zero here first.
        m_pRdiStream[0]->width  = 0;
        m_pRdiStream[0]->height = 0;

        if (newSessionId == m_realtimeSessionId)
        {
            m_pRdiStream[0]->width  = m_QuadCFASensorInfo.sensorModePickHint.sensorOutputSize.width;
            m_pRdiStream[0]->height = m_QuadCFASensorInfo.sensorModePickHint.sensorOutputSize.height;

            CHX_LOG("QCFA sensor set rdi stream to %dx%d", m_pRdiStream[0]->width, m_pRdiStream[0]->height);

            m_QuadCFASensorInfo.sensorModePickHint.postSensorUpscale = TRUE;
        }
        else
        {
            m_pRdiStream[0]->width  = m_QuadCFASensorInfo.fullSizeSensorOutput.width;
            m_pRdiStream[0]->height = m_QuadCFASensorInfo.fullSizeSensorOutput.height;

            CHX_LOG("QCFA sensor set rdi stream to %dx%d", m_pRdiStream[0]->width, m_pRdiStream[0]->height);

            m_QuadCFASensorInfo.sensorModePickHint.postSensorUpscale = FALSE;
        }

        CHX_LOG("Update postSensorUpscale flag:%d", m_QuadCFASensorInfo.sensorModePickHint.postSensorUpscale);

        result = RestartOfflineSessions(TRUE);
    }

    // create new pipeline and session
    CHX_LOG("create new RT session. sessionid: %d", newSessionId);

    result = CreateSessionByID(newSessionId, m_pCallbacks);

    ReleaseRTConfigLock();

    CHX_LOG("X");

    return result;
}

VOID AdvancedCameraUsecase::ProcessFeatureDataNotify(
    UINT32   internalFrameNum,
    Feature* pCurrentFeature,
    VOID*    pData)
{
    CAMX_UNREFERENCED_PARAM(internalFrameNum);
    CAMX_UNREFERENCED_PARAM(pCurrentFeature);
    CAMX_UNREFERENCED_PARAM(pData);

    CHX_LOG("In advanced class, feature type:%d", pCurrentFeature->GetFeatureType());

    return;
}

VOID AdvancedCameraUsecase::ProcessFeatureDone(
    UINT32            internalFrameNum,
    Feature*          pCurrentFeature,
    CHICAPTURERESULT* pResult)
{
    CAMX_UNREFERENCED_PARAM(internalFrameNum);
    CAMX_UNREFERENCED_PARAM(pCurrentFeature);
    CAMX_UNREFERENCED_PARAM(pResult);

    CHX_LOG("In advanced class, feature type:%d", pCurrentFeature->GetFeatureType());

    return;
}

VOID AdvancedCameraUsecase::LogFeatureRequestMappings(
    UINT32          inFrameNum,
    UINT32          reqFrameNum,
    const CHAR*     identifyingData)
{
    CHX_LOG_REQMAP("frame: %u  <==>  chiOverrideFrameNum: %u  <==>  chiFrameNum: %u -- %s",
        GetAppFrameNum(inFrameNum),
        inFrameNum,
        reqFrameNum,
        identifyingData);
}

UINT32 AdvancedCameraUsecase::GetTotalNumOfFeaturesPerRequest(UINT32 appFrameNum)
{
    UINT32 numOfFeatures = 0;
    UINT32 index         = appFrameNum % MaxOutstandingRequests;

    CHX_LOG("appFrameNum:%d, index:%d, numOfFeatures:%d", appFrameNum, index, m_snapshotFeatures[index].numOfFeatures);

    return (m_snapshotFeatures[index].numOfFeatures);
}

Feature* AdvancedCameraUsecase::GetNextFeatureForSnapshot(
    UINT32 appFrameNum,
    const Feature* pCurrentFeature)
{
    UINT32   numOfFeatures     = 0;
    UINT32   index             = appFrameNum % MaxOutstandingRequests;

    CHX_LOG("appFrameNum:%d, index:%d, numOfFeatures:%d", appFrameNum, index, m_snapshotFeatures[index].numOfFeatures);

    Feature* pNextFeature = NULL;

    if ((appFrameNum == m_snapshotFeatures[index].appFrameNum) &&
        (1 < m_snapshotFeatures[index].numOfFeatures))
    {
        for (UINT i = 0; i < m_snapshotFeatures[index].numOfFeatures - 1; i++)
        {
            if (pCurrentFeature == m_snapshotFeatures[index].featureInfo[i].pFeature)
            {
                pNextFeature = m_snapshotFeatures[index].featureInfo[i + 1].pFeature;
                break;
            }
        }
    }

    CHX_LOG("pCurrentFeature:%p, pNextFeature:%p", pCurrentFeature, pNextFeature);

    return pNextFeature;
}

Feature* AdvancedCameraUsecase::GetPreviousFeatureForSnapshot(
    UINT32 appFrameNum,
    const Feature* pCurrentFeature)
{
    UINT32   numOfFeatures     = 0;
    UINT32   index             = appFrameNum % MaxOutstandingRequests;

    CHX_LOG("appFrameNum:%d, index:%d, numOfFeatures:%d", appFrameNum, index, m_snapshotFeatures[index].numOfFeatures);

    Feature* pPreviousFeature = NULL;

    if ((appFrameNum == m_snapshotFeatures[index].appFrameNum) &&
        (1 < m_snapshotFeatures[index].numOfFeatures))
    {
        for (UINT i = 0; i < m_snapshotFeatures[index].numOfFeatures; i++)
        {
            if (pCurrentFeature == m_snapshotFeatures[index].featureInfo[i].pFeature)
            {
                if (i > 0)
                {
                    pPreviousFeature = m_snapshotFeatures[index].featureInfo[i - 1].pFeature;
                }

                break;
            }
        }
    }

    CHX_LOG("pCurrentFeature:%p, pPreviousFeature:%p", pCurrentFeature, pPreviousFeature);

    return pPreviousFeature;
}

SnapshotFeatureInfo* AdvancedCameraUsecase::GetSnapshotFeatureInfo(
    UINT32 appFrameNum,
    const Feature* pFeature)
{
    UINT32               index        = appFrameNum % MaxOutstandingRequests;
    SnapshotFeatureInfo* pFeatureInfo = NULL;

    CHX_LOG("appFrameNum:%d, index:%d, numOfFeatures:%d", appFrameNum, index, m_snapshotFeatures[index].numOfFeatures);

    for (UINT i = 0; i < m_snapshotFeatures[index].numOfFeatures; i++)
    {
        if ((appFrameNum == m_snapshotFeatures[index].appFrameNum) &&
            (pFeature    == m_snapshotFeatures[index].featureInfo[i].pFeature))
        {
            pFeatureInfo = &(m_snapshotFeatures[index].featureInfo[i]);
            break;
        }
    }

    CHX_LOG("pFeatureInfo:%p", pFeatureInfo);

    return pFeatureInfo;
}

VOID AdvancedCameraUsecase::NotifyFeatureSnapshotDone(
    UINT32 appFrameNum,
    Feature* pCurrentFeature,
    camera3_capture_request_t* pRequest)
{
    Feature* pNextFeature = NULL;

    CHX_LOG("appFrameNum:%d, currentFeature:%p, type:%d",
        appFrameNum, pCurrentFeature, pCurrentFeature->GetFeatureType());

    pNextFeature = GetNextFeatureForSnapshot(appFrameNum, pCurrentFeature);

    if (NULL != pNextFeature)
    {
        CHX_LOG("next feature:%p, type:%d", pNextFeature, pNextFeature->GetFeatureType());
        pNextFeature->ExecuteProcessRequest(pRequest);
    }

    return;
}

