////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxpipeline.cpp
/// @brief Pipeline class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxatomic.h"
#include "camxdebug.h"
#include "camxexternalsensor.h"
#include "camxhwcontext.h"
#include "camxhwdefs.h"
#include "camxhwfactory.h"
#include "camximagesensormoduledatamanager.h"
#include "camximagesensormoduledata.h"
#include "camximagesensordata.h"
#include "camxmem.h"
#include "camxmetadatapool.h"
#include "camxncsservice.h"
#include "camxnode.h"
#include "camxpipeline.h"
#include "camxsession.h"
#include "camxthreadmanager.h"
#include "camxtrace.h"
#include "camxvendortags.h"
#include "camxchicontext.h"
#include "camxhal3metadatautil.h"
#include "camxstatsdebugdatatypes.h"
#include "camxtypes.h"
#include "chibinarylog.h"

CAMX_NAMESPACE_BEGIN
using namespace std;

// NOWHINE FILE CP006: used standard libraries for performance improvements

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constant Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// @brief list of tags published
static const UINT32 PipelineMetadataTags[] =
{
    SensorTimestamp,
    PropertyIDSensorCurrentMode,
};

// @brief list of vendor tags published
static const struct NodeVendorTag PipelineVendorTags[] =
{
    { "com.qti.chi.multicamerainfo"      , "SyncMode"             },
    { "org.codeaurora.qcamera3.av_timer" , "use_av_timer"         },
    { "org.quic.camera.qtimer"           , "timestamp"            },
    { "org.quic.camera.streamTypePresent", "preview"              },
};

static const UINT SingleMetadataResult           = 1;
static const UINT DefaultFPS                     = 30;
static const UINT DefaultMaxIFEPipelineDelay     = 3;
static const UINT DefaultMaxBPSPipelineDelay     = DefaultMaxIFEPipelineDelay + 2;
static const UINT DefaultNodesRequestDoneTimeout = 300;   ///< Default nodes request done time out in ms
static const UINT DefaultStreamOnTimeout         = (LivePendingRequestTimeoutDefault - 100);

CAMX_STATIC_ASSERT(0 < (LivePendingRequestTimeoutDefault - 100));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::Pipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Pipeline::Pipeline()
    : m_hCSLSession(CSLInvalidHandle)
    , m_referenceCount(1)
{
    SetPipelineStatus(PipelineStatus::UNINITIALIZED);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::~Pipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Pipeline::~Pipeline()
{

    DestroyNodes();

    if (CSLInvalidHandle != GetCSLSession())
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "%s_%u: CSLClose(0x%x)", GetPipelineName(), GetPipelineId(), GetCSLSession());
        CSLClose(GetCSLSession());
    }

    SetPipelineStatus(PipelineStatus::UNINITIALIZED);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::Create(
    PipelineCreateInputData*  pCreateInputData,
    PipelineCreateOutputData* pCreateOutputData)
{
    /// @todo (CAMX-1512) Add pipeline event
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCore, SCOPEEventTopologyCreate);

    CAMX_ASSERT((NULL != pCreateInputData) && (NULL != pCreateOutputData));

    CamxResult result    = CamxResultSuccess;
    Pipeline*  pPipeline = CAMX_NEW Pipeline;

    if (NULL != pPipeline)
    {
        result = pPipeline->Initialize(pCreateInputData, pCreateOutputData);

        if (CamxResultSuccess == result)
        {
            pCreateOutputData->pPipeline = pPipeline;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline[%s] Initialize failed!", pPipeline->GetPipelineIdentifierString());

            pCreateOutputData->pPipeline = NULL;
            pPipeline->Destroy();
            pPipeline = NULL;
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Out of memory");
        result = CamxResultENoMemory;
    }

    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::Destroy()
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupCore, SCOPEEventTopologyDestroy);

    INT refCount = CamxAtomicDec(&m_referenceCount);

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "%s_%u: Updated m_referenceCount=%u", GetPipelineName(), GetPipelineId(), refCount);

    if (0 > refCount)
    {
        CAMX_ASSERT_ALWAYS();
    }

    if (0 == m_referenceCount)
    {
        // Can no longer rely on the Session being around since Session::Destroy() also calls Pipeline::Destroy() on this
        // object and the final Destroy() call on this pipeline may come after the Session is destroyed.
        m_pSession = NULL;

        CAMX_LOG_CONFIG(CamxLogGroupCore, "Destroying %s_%u", GetPipelineName(), GetPipelineId());

        if (NULL != m_pHwContext)
        {
            if (NULL != m_pExternalSensor)
            {
                m_pExternalSensor->Destroy();
                m_pExternalSensor = NULL;
            }
        }

        if (NULL != m_pResourceAcquireReleaseLock)
        {
            m_pResourceAcquireReleaseLock->Destroy();
            m_pResourceAcquireReleaseLock = NULL;
        }

        if (NULL != m_pWaitForStreamOnDone)
        {
            m_pWaitForStreamOnDone->Destroy();
            m_pWaitForStreamOnDone = NULL;
        }

        if (NULL != m_pStreamOnDoneLock)
        {
            m_pStreamOnDoneLock->Destroy();
            m_pStreamOnDoneLock = NULL;
        }

        if (NULL != m_pConfigDoneLock)
        {
            m_pConfigDoneLock->Destroy();
            m_pConfigDoneLock = NULL;
        }

        if (NULL != m_pWaitForConfigDone)
        {
            m_pWaitForConfigDone->Destroy();
            m_pWaitForConfigDone = NULL;
        }

        if (NULL != m_pCSLSyncIDToRequestId)
        {
            CAMX_FREE(m_pCSLSyncIDToRequestId);
            m_pCSLSyncIDToRequestId = NULL;
        }

        for (UINT i = 0; i < MaxPerRequestInfo; i++)
        {
            if (NULL != m_perRequestInfo[i].pSequenceId)
            {
                CAMX_FREE(m_perRequestInfo[i].pSequenceId);
                m_perRequestInfo[i].pSequenceId = NULL;
            }
        }

        if (NULL != m_pInputPool)
        {
            m_pInputPool->Destroy();
            m_pInputPool = NULL;
        }

        if (NULL != m_pMainPool)
        {
            m_pMainPool->Destroy();
            m_pMainPool = NULL;
        }

        if (NULL != m_pInternalPool)
        {
            m_pInternalPool->Destroy();
            m_pInternalPool = NULL;
        }

        if (NULL != m_pUsecasePool)
        {
            m_pUsecasePool->Destroy();
            m_pUsecasePool = NULL;
        }

        if (NULL != m_pPerRequestInfoLock)
        {
            m_pPerRequestInfoLock->Destroy();
            m_pPerRequestInfoLock = NULL;
        }

        if (NULL != m_pStreamBufferBlob)
        {
            CAMX_FREE(m_pStreamBufferBlob);
            m_pStreamBufferBlob = NULL;
        }

        if (NULL != m_pNodesRequestDoneLock)
        {
            m_pNodesRequestDoneLock->Destroy();
            m_pNodesRequestDoneLock = NULL;
        }

        if (NULL != m_pWaitAllNodesRequestDone)
        {
            m_pWaitAllNodesRequestDone->Destroy();
            m_pWaitAllNodesRequestDone = NULL;
        }

        // if we destroy and recreate session without destroy pipeline descriptor,
        // the stream wrapper is not destroyed either, and the same stream wrapper will be used  in the new session,
        // so reset frame enabled info to avoid impacting the new created session.
        for (UINT i = 0; i < m_pPipelineDescriptor->numOutputs; i++)
        {
            const PipelineOutputData* pPipelineOutputData = &m_pPipelineDescriptor->outputData[i];
            if (NULL != pPipelineOutputData->pOutputStreamWrapper)
            {
                pPipelineOutputData->pOutputStreamWrapper->ResetEnabledFrameInfo();
            }
        }

        CAMX_DELETE this;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::Initialize(
    PipelineCreateInputData*  pPipelineCreateInputData,
    PipelineCreateOutputData* pPipelineCreateOutputData)
{
    CamxResult result = CamxResultEFailed;

    m_pChiContext                   = pPipelineCreateInputData->pChiContext;
    m_flags.isSecureMode            = pPipelineCreateInputData->isSecureMode;
    m_flags.isHFRMode               = pPipelineCreateInputData->pPipelineDescriptor->flags.isHFRMode;
    m_flags.isInitialConfigPending  = TRUE;
    m_pThreadManager                = pPipelineCreateInputData->pChiContext->GetThreadManager();
    m_pPipelineDescriptor           = pPipelineCreateInputData->pPipelineDescriptor;
    m_pipelineIndex                 = pPipelineCreateInputData->pipelineIndex;
    m_cameraId                      = m_pPipelineDescriptor->cameraId;
    m_hCSLLinkHandle                = CSLInvalidHandle;
    m_numConfigDoneNodes            = 0;
    m_lastRequestId                 = 0;
    m_configDoneCount               = 0;
    m_hCSLLinkHandle                = 0;
    m_HALOutputBufferCombined       = m_pPipelineDescriptor->HALOutputBufferCombined;
    m_lastSubmittedShutterRequestId = 0;
    m_pTuningManager                = HwEnvironment::GetInstance()->GetTuningDataManager(m_cameraId);
    m_sensorSyncMode                = NoSync;

    // Create lock and condition for config done
    m_pConfigDoneLock       = Mutex::Create("PipelineConfigDoneLock");
    m_pWaitForConfigDone    = Condition::Create("PipelineWaitForConfigDone");

    // Resource lock, used to syncronize acquire resources and release resources
    m_pResourceAcquireReleaseLock = Mutex::Create("PipelineResourceAcquireReleaseLock");
    if (NULL == m_pResourceAcquireReleaseLock)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!!");
        return CamxResultENoMemory;
    }

    m_pWaitForStreamOnDone = Condition::Create("PipelineWaitForStreamOnDone");
    if (NULL == m_pWaitForStreamOnDone)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!!");
        return CamxResultENoMemory;
    }

    m_pStreamOnDoneLock = Mutex::Create("PipelineStreamOnDoneLock");
    if (NULL == m_pStreamOnDoneLock)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!!");
        return CamxResultENoMemory;
    }

    m_pNodesRequestDoneLock = Mutex::Create("PipelineAllNodesRequestDone");
    if (NULL == m_pNodesRequestDoneLock)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!!");
        return CamxResultENoMemory;
    }

    m_pWaitAllNodesRequestDone = Condition::Create("PipelineWaitAllNodesRequestDone");
    if (NULL == m_pWaitAllNodesRequestDone)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!!");
        return CamxResultENoMemory;
    }

    // Create external Sensor when sensor module is enabled
    // External Sensor Module is created so as to test CAMX ability to work with OEMs
    // who has external sensor (ie they do all sensor configuration outside of driver
    // and there is no sensor node in the pipeline )
    HwContext* pHwcontext = pPipelineCreateInputData->pChiContext->GetHwContext();
    if (TRUE == pHwcontext->GetStaticSettings()->enableExternalSensorModule)
    {
        m_pExternalSensor = ExternalSensor::Create();
        CAMX_ASSERT(NULL != m_pExternalSensor);
    }

    CAMX_ASSERT(NULL != m_pConfigDoneLock);
    CAMX_ASSERT(NULL != m_pWaitForConfigDone);
    CAMX_ASSERT(NULL != m_pResourceAcquireReleaseLock);

    OsUtils::SNPrintF(m_pipelineIdentifierString, sizeof(m_pipelineIdentifierString), "%s_%d",
        GetPipelineName(), GetPipelineId());

    // We can't defer UsecasePool since we are publishing preview dimension to it.
    m_pUsecasePool  = MetadataPool::Create(PoolType::PerUsecase, m_pipelineIndex, NULL, 1, GetPipelineIdentifierString(), 0);

    if (NULL != m_pUsecasePool)
    {
        m_pUsecasePool->UpdateRequestId(0); // Usecase pool created, mark the slot as valid
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!!");
        return CamxResultENoMemory;
    }

    SetLCRRawformatPorts();

    SetNumBatchedFrames(m_pPipelineDescriptor->numBatchedFrames, m_pPipelineDescriptor->maxFPSValue);

    m_pCSLSyncIDToRequestId = static_cast<UINT64*>(CAMX_CALLOC(sizeof(UINT64) * MaxPerRequestInfo * GetBatchedHALOutputNum()));

    if (NULL == m_pCSLSyncIDToRequestId)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!!");
        return CamxResultENoMemory;
    }

    m_pStreamBufferBlob = static_cast<StreamBufferInfo*>(CAMX_CALLOC(sizeof(StreamBufferInfo) * GetBatchedHALOutputNum() *
                                                                     MaxPerRequestInfo));
    if (NULL == m_pStreamBufferBlob)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!!");
        return CamxResultENoMemory;
    }

    for (UINT i = 0; i < MaxPerRequestInfo; i++)
    {
        m_perRequestInfo[i].pSequenceId = static_cast<UINT32*>(CAMX_CALLOC(sizeof(UINT32) * GetBatchedHALOutputNum()));

        if (NULL == m_perRequestInfo[i].pSequenceId)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!!");
            return CamxResultENoMemory;
        }
        m_perRequestInfo[i].request.pStreamBuffers = &m_pStreamBufferBlob[i * GetBatchedHALOutputNum()];
    }

    MetadataSlot*    pMetadataSlot                = m_pUsecasePool->GetSlot(0);
    MetaBuffer*      pInitializationMetaBuffer    = m_pPipelineDescriptor->pSessionMetadata;
    MetaBuffer*      pMetadataSlotDstBuffer       = NULL;

    // Copy metadata published by the Chi Usecase to this pipeline's UsecasePool
    if (NULL != pInitializationMetaBuffer)
    {
        result = pMetadataSlot->GetMetabuffer(&pMetadataSlotDstBuffer);

        if (CamxResultSuccess == result)
        {
            pMetadataSlotDstBuffer->Copy(pInitializationMetaBuffer, TRUE);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot copy! Error Code: %u", result);
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupMeta, "No init metadata found!");
    }

    if (CamxResultSuccess == result)
    {
        UINT32      metaTag             = 0;
        UINT        sleepStaticSetting  = HwEnvironment::GetInstance()->GetStaticSettings()->induceSleepInChiNode;

        result                          = VendorTagManager::QueryVendorTagLocation(
                                                "org.quic.camera.induceSleepInChiNode",
                                                "InduceSleep",
                                                &metaTag);

        if (CamxResultSuccess == result)
        {
            result  = pMetadataSlot->SetMetadataByTag(metaTag, &sleepStaticSetting, 1, "camx_session");

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to set Induce sleep result %d", result);
            }
        }
    }

    GetCameraRunningOnBPS(pMetadataSlot);

    ConfigureMaxPipelineDelay(m_pPipelineDescriptor->maxFPSValue,
        (FALSE == m_flags.isCameraRunningOnBPS) ? DefaultMaxIFEPipelineDelay : DefaultMaxBPSPipelineDelay);

    QueryEISCaps();

    PublishOutputDimensions();
    PublishTargetFPS();

    if (CamxResultSuccess == result)
    {
        result = PopulatePSMetadataSet();
    }

    result = CreateNodes(pPipelineCreateInputData, pPipelineCreateOutputData);

    // set frame delay in session metadata
    if (CamxResultSuccess == result)
    {
        UINT32 metaTag    = 0;
        UINT32 frameDelay = DetermineFrameDelay();
        result            = VendorTagManager::QueryVendorTagLocation(
                            "org.quic.camera.eislookahead", "FrameDelay", &metaTag);
        if (CamxResultSuccess == result)
        {
            MetaBuffer* pSessionMetaBuffer = m_pPipelineDescriptor->pSessionMetadata;
            if (NULL != pSessionMetaBuffer)
            {
                result = pSessionMetaBuffer->SetTag(metaTag, &frameDelay, 1, sizeof(UINT32));
            }
            else
            {
                result = CamxResultEInvalidPointer;
                CAMX_LOG_ERROR(CamxLogGroupCore, "Session metadata pointer null");
            }
        }
    }

    // set EIS enabled flag in session metadata
    if (CamxResultSuccess == result)
    {
        UINT32  metaTag     = 0;
        BOOL    bEnabled    = IsEISEnabled();
        result              = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime", "Enabled", &metaTag);

        // write the enabled flag only if it's set to TRUE. IsEISEnabled may return FALSE when vendor tag is not published too
        if ((TRUE == bEnabled) && (CamxResultSuccess == result))
        {
            MetaBuffer* pSessionMetaBuffer = m_pPipelineDescriptor->pSessionMetadata;
            if (NULL != pSessionMetaBuffer)
            {
                result = pSessionMetaBuffer->SetTag(metaTag, &bEnabled, 1, sizeof(BYTE));
            }
            else
            {
                result = CamxResultEInvalidPointer;
                CAMX_LOG_ERROR(CamxLogGroupCore, "Session metadata pointer null");
            }
        }
    }

    // set EIS minimal total margin in session metadata
    if (CamxResultSuccess == result)
    {
        UINT32 metaTag = 0;
        MarginRequest margin = { 0 };

        result = DetermineEISMiniamalTotalMargin(&margin);

        if (CamxResultSuccess == result)
        {
            result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime", "MinimalTotalMargins", &metaTag);
        }

        if (CamxResultSuccess == result)
        {
            MetaBuffer* pSessionMetaBuffer = m_pPipelineDescriptor->pSessionMetadata;
            if (NULL != pSessionMetaBuffer)
            {
                result = pSessionMetaBuffer->SetTag(metaTag, &margin, 1, sizeof(MarginRequest));
            }
            else
            {
                result = CamxResultEInvalidPointer;
                CAMX_LOG_ERROR(CamxLogGroupCore, "Session metadata pointer null");
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        for (UINT i = 0; i < m_nodeCount; i++)
        {
            result = FilterAndUpdatePublishSet(m_ppNodes[i]);
        }
    }

    if (HwEnvironment::GetInstance()->GetStaticSettings()->numMetadataResults > SingleMetadataResult)
    {
        m_bPartialMetadataEnabled = TRUE;
    }

    if ((TRUE == m_bPartialMetadataEnabled) && (TRUE == m_flags.isHFRMode))
    {
        CAMX_LOG_CONFIG(CamxLogGroupCore, "Disable partial metadata in HFR mode");
        m_bPartialMetadataEnabled = FALSE;
    }

    if (CamxResultSuccess == result)
    {
        m_pPerRequestInfoLock = Mutex::Create("PipelineRequestInfo");
        if (NULL != m_pPerRequestInfoLock)
        {
            if (IsRealTime())
            {
                m_metaBufferDelay = Utils::MaxUINT32(
                    GetMaxPipelineDelay(),
                    DetermineFrameDelay());
            }
            else
            {
                m_metaBufferDelay = 0;
            }
        }
        else
        {
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (IsRealTime())
        {
            m_metaBufferDelay = Utils::MaxUINT32(
                GetMaxPipelineDelay(),
                DetermineFrameDelay());
        }
        else
        {
            m_metaBufferDelay = 0;
        }

        UpdatePublishTags();
    }

    if (CamxResultSuccess == result)
    {
        pPipelineCreateOutputData->pPipeline = this;
        SetPipelineStatus(PipelineStatus::INITIALIZED);
        auto& rPipelineName = m_pipelineIdentifierString;
        UINT  pipelineId    = GetPipelineId();
        BOOL  isRealtime    = IsRealTime();
        auto  hPipeline     = m_pPipelineDescriptor;
        BINARY_LOG(LogEvent::Pipeline_Initialize, rPipelineName, pipelineId, hPipeline);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::InitializeMetadataPools
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::InitializeMetadataPools(
    UINT numSlots)
{
    // Create perFrame MetadataPools for pipeline
    m_pMainPool      = MetadataPool::Create(PoolType::PerFrameResult,
                          m_pipelineIndex,
                          NULL,
                          numSlots,
                          GetPipelineIdentifierString(),
                          GetMaxPipelineDelay());
    m_pInputPool     = MetadataPool::Create(PoolType::PerFrameInput,
                          m_pipelineIndex,
                          NULL,
                          numSlots,
                          GetPipelineIdentifierString(),
                          0);
    m_pInternalPool  = MetadataPool::Create(PoolType::PerFrameInternal,
                          m_pipelineIndex,
                          m_pThreadManager,
                          numSlots,
                          GetPipelineIdentifierString(),
                          0);
    m_pEarlyMainPool = m_pMainPool;

    CAMX_ASSERT(NULL != m_pMainPool);
    CAMX_ASSERT(NULL != m_pInputPool);
    CAMX_ASSERT(NULL != m_pInternalPool);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::PrepareStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::PrepareStreamOn()
{
    CamxResult result = CamxResultSuccess;

    UINT devicecount = 0;

    for (UINT i = 0; i < m_nodeCount; i++)
    {
        if (NULL != m_ppNodes[i])
        {
            result = m_ppNodes[i]->PrepareNodeStreamOn();

            if (CamxResultSuccess != result)
            {
                break;
            }

            for (UINT j = 0; j < m_ppNodes[i]->CSLDeviceHandleCount(); j++)
            {
                m_hDevices[devicecount++] =  m_ppNodes[i]->GetCSLDeviceHandle(j);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::StreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::StreamOn()
{
    CamxResult result        = CamxResultSuccess;
    BOOL       skipStreamOn  = FALSE;

    if ((CSLInvalidHandle == m_hCSLLinkHandle) && (TRUE == IsRealTime()))
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "Calling Link pipeline");
        result = Link();

        if (CamxResultSuccess == result)
        {
            result = CSLRegisterMessageHandler(m_hCSLSession, m_hCSLLinkHandle,
                CSLMessageHandler, static_cast<VOID*>(this));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to register message handler, error code %d", result);
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        // Acquire resources before stream on
        result = CallNodeAcquireResources();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline::%s Acquire resources failed and calling Unlink",
                    GetPipelineIdentifierString());
            Unlink();
        }
    }

    if ((CamxResultSuccess == result) && (PipelineStatus::STREAM_ON != GetPipelineStatus()))
    {
        m_lastMonoTimestamp   = 0;
        m_lastQTimerTimestamp = 0;

        if (TRUE == IsRealTime())
        {
            // Wait for realtime pipeline config done before calling stream on
            m_pConfigDoneLock->Lock();
            while (m_configDoneCount != (m_pSession->GetCurrentPipelineRequestId(m_pipelineIndex) - m_lastRequestId))
            {
                CAMX_LOG_INFO(CamxLogGroupCore,
                      "Wait before Stream on for %p m_configDoneCount=%d current pipeline reqId %llu, last reqId %llu",
                      this,
                      m_configDoneCount,
                      m_pSession->GetCurrentPipelineRequestId(m_pipelineIndex),
                      m_lastRequestId);
                m_isWaitForConfigDone = TRUE;
                result = m_pWaitForConfigDone->TimedWait(m_pConfigDoneLock->GetNativeHandle(), DefaultStreamOnTimeout);
                m_isWaitForConfigDone = FALSE;
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Stream On for pipeline[%s] timed out(%d)!! result %s, Lets do a Reset",
                        GetPipelineIdentifierString(), DefaultStreamOnTimeout, CamxResultStrings[result]);
                    break;
                }

                if (TRUE == m_isConfigDoneAborted)
                {
                    m_isConfigDoneAborted = FALSE;
                    skipStreamOn          = TRUE;
                    CAMX_LOG_INFO(CamxLogGroupCore, "Init config done is aborted by flush ",
                                   "exiting from stream on for pipeline %s", GetPipelineIdentifierString());
                    break;
                }
            }
            m_pConfigDoneLock->Unlock();
        }

        // if destroy is in progress exit
        if (FALSE == skipStreamOn)
        {
            if (CamxResultSuccess == result)
            {
                CAMX_LOG_CONFIG(CamxLogGroupCore, "[%s] StreamingOn for pipeline: %p", GetPipelineIdentifierString(), this);

                if ((TRUE == m_pHwContext->GetStaticSettings()->enableExternalSensorModule) &&
                    (TRUE == IsRealTime()))
                {
                    m_pExternalSensor->Initialize(m_cameraId,
                        m_hCSLSession,
                        m_pHwContext,
                        m_currentSensorMode);
                    m_pExternalSensor->StreamOn();
                }

                result = m_pChiContext->GetHwContext()->StreamOn(GetCSLSession(), GetCSLLink(), GetCSLDevices());
            }

            if (CamxResultSuccess == result)
            {
                SetPipelineStatus(PipelineStatus::STREAM_ON);

                m_pStreamOnDoneLock->Lock();
                m_isStreamOnDone = TRUE;
                m_pWaitForStreamOnDone->Signal();
                m_pStreamOnDoneLock->Unlock();
            }

            if ((CamxResultSuccess == result) && (m_ppNodes != NULL))
            {
                // StreamOn each node in pipeline
                for (UINT i = 0; i < m_nodeCount; i++)
                {
                    if (NULL != m_ppNodes[i])
                    {
                        result = m_ppNodes[i]->OnNodeStreamOn();
                    }

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "%s OnNodeStreamOn failed, result=%d",
                                       m_ppNodes[i]->NodeIdentifierString(), result);
                        break;
                    }
                }

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline::%s Failed to OnNodeStreamOn error: %d",
                        GetPipelineIdentifierString(),
                        result);
                    result = StreamOff(CHIDeactivateModeDefault);
                }
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline::%s Failed to CSL StreamOn error: %d",
                    GetPipelineIdentifierString(),
                    result);
                m_flags.isInitialConfigPending = TRUE;
                Unlink();
                CallNodeReleaseResources(CHIDeactivateModeDefault);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::StreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::StreamOff(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CamxResult result = CamxResultSuccess;

    // All resources should be released even if the release fails on some of them, to minimize leaked resources

    if ((PipelineStatus::STREAM_ON          == GetPipelineStatus()) ||
        (PipelineStatus::PARTIAL_STREAM_ON  == GetPipelineStatus()))
    {
        if (m_ppNodes != NULL)
        {
            // StreamOff each node in pipeline
            for (UINT i = 0; i < m_nodeCount; i++)
            {
                if (NULL != m_ppNodes[i])
                {
                    result = m_ppNodes[i]->OnNodeStreamOff(modeBitmask);
                }
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "[%s][%s] OnNodeStreamOff failed.",
                        GetPipelineIdentifierString(), m_ppNodes[i]->NodeIdentifierString());
                }
            }
        }

        // ChiContext StreamOff is only called if defaultDeactivateMode or sensorStandbyMode bit is set
        if ((modeBitmask & CHIDeactivateModeDefault) ||
            (modeBitmask & CHIDeactivateModeSensorStandby) ||
            (modeBitmask & CHIDeactivateModeRealTimeDevices) ||
            (modeBitmask & CHIDeactivateModeUnlinkPipeline))
        {

            result = m_pChiContext->GetHwContext()->StreamOff(GetCSLSession(), GetCSLLink(), GetCSLDevices(), modeBitmask);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore,
                               "[%s] StreamingOff for pipeline: %p status: %s real time %d cam Id: %d mode : %d",
                               GetPipelineIdentifierString(),
                               this,
                               Utils::CamxResultToString(result),
                               IsRealTime(),
                               m_cameraId,
                               modeBitmask);
                // Force release to be done
                modeBitmask = CHIDeactivateModeDefault;
            }

            // We should still go ahead and release all the resources even if the CSL streamoff failed

            if ((TRUE == m_pHwContext->GetStaticSettings()->enableExternalSensorModule) &&
                (TRUE == IsRealTime()))
            {
                m_pExternalSensor->StreamOff();
                // releases sensor device then csiphy device
                m_pExternalSensor->Uninitialize();
            }

            m_configDoneCount               = 0;
            m_flags.isInitialConfigPending  = TRUE;
            m_lastRequestId                 = m_pSession->GetCurrentPipelineRequestId(m_pipelineIndex);
            m_flushInfo.hasFlushOccurred    = FALSE;

            if (CamxResultSuccess == result)
            {
                CAMX_LOG_CONFIG(CamxLogGroupCore,
                                "%s Streaming Off successful:"
                                "%p, last request id %llu is real time %d cam Id: %d mode : %d",
                                GetPipelineIdentifierString(),
                                this,
                                m_lastRequestId,
                                IsRealTime(),
                                m_cameraId,
                                modeBitmask);
            }

            UpdateLastFlushedRequestId(m_lastRequestId);

            if ((modeBitmask & CHIDeactivateModeSensorStandby) ||
                (modeBitmask & CHIDeactivateModeRealTimeDevices))
            {
                SetPipelineStatus(PipelineStatus::PARTIAL_STREAM_ON);
            }
            else
            {
                SetPipelineStatus(PipelineStatus::STREAM_OFF);
            }

            m_pStreamOnDoneLock->Lock();
            m_isStreamOnDone = FALSE;
            m_pStreamOnDoneLock->Unlock();

            // Release resources after stream off
            CallNodeReleaseResources(modeBitmask);

            if (modeBitmask & CHIDeactivateModeUnlinkPipeline)
            {
                CAMX_LOG_CONFIG(CamxLogGroupCore, "Unlink pipeline for mode: CHIDeactivateModeUnlinkPipeline");
                Unlink();
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::Link
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::Link()
{
    CamxResult result = CamxResultSuccess;

    if (TRUE == IsRealTime())
    {
        result = m_pChiContext->GetHwContext()->Link(GetCSLSession(), GetCSLLink(), GetCSLDevices());
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "[%s] Link for pipeline: %p Failed", GetPipelineIdentifierString(), this);
        }
        else
        {
            CAMX_LOG_CONFIG(CamxLogGroupCore, "[%s] Link for pipeline: %p Success. Link = %p",
                GetPipelineIdentifierString(), this, GetCSLLink());
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::Unlink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::Unlink()
{
    CamxResult result = CamxResultSuccess;

    if ((TRUE == IsRealTime()) && (NULL != GetCSLLink()) && (0 != *GetCSLLink()))
    {
        result = m_pChiContext->GetHwContext()->Unlink(GetCSLSession(), GetCSLLink(), GetCSLDevices());
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "[%s] Unlink for pipeline: %p Failed. Link = %p CSL Session %u",
                           GetPipelineIdentifierString(), this, GetCSLLink(), GetCSLSession());
        }
        else
        {
            CAMX_LOG_CONFIG(CamxLogGroupCore, "[%s] Unlink for pipeline: %p Success. Link = %p",
                GetPipelineIdentifierString(), this, GetCSLLink());
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::ReleaseResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::ReleaseResources()
{
    CamxResult result = CamxResultSuccess;

    result = CallNodeReleaseResources(CHIDeactivateModeDefault);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "[%s] Release Resources for pipeline: %p Failed",
                       GetPipelineIdentifierString(), this);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::GetDualCameraSyncEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pipeline::GetDualCameraSyncEnabled(
    UINT64 requestId)
{
    UINT32  tag                 = 0;
    BOOL    isSyncModeEnabled   = FALSE;

    // Query the kernel frame sync mode to sync both the real time requests
    if (CDKResultSuccess == VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicamerainfo", "SyncMode", &tag))
    {
        MetadataSlot* pMetadataSlot  = m_pInputPool->GetSlot(requestId);
        SyncModeInfo* pInputMetadata = static_cast<SyncModeInfo*>(pMetadataSlot->GetMetadataByTag(tag));

        if (NULL != pInputMetadata)
        {
            isSyncModeEnabled = pInputMetadata->isSyncModeEnabled;
        }
        CAMX_LOG_INFO(CamxLogGroupCore, "VendorTag syncmode enabled %d", isSyncModeEnabled);
    }
    return isSyncModeEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::GetCameraRunningOnBPS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::GetCameraRunningOnBPS(
    MetadataSlot* pMetadataSlot)
{
    CamxResult result                = CamxResultSuccess;
    UINT32     tag                   = 0;
    BOOL*      pIsCameraRunningOnBPS = NULL;

    m_flags.isCameraRunningOnBPS = FALSE;

    if (CDKResultSuccess == VendorTagManager::QueryVendorTagLocation(
            "com.qti.chi.bpsrealtimecam", "cameraRunningOnBPS", &tag))
    {
        pIsCameraRunningOnBPS = static_cast<BOOL*>(pMetadataSlot->GetMetadataByTag(tag));
        if (NULL != pIsCameraRunningOnBPS)
        {
            m_flags.isCameraRunningOnBPS = *pIsCameraRunningOnBPS;
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupStats, "Couldn't retrieve cameraRunningOnBPS tag location.");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::PrepareResultMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::PrepareResultMetadata(
    MetaBuffer* pInputMetadata,
    MetaBuffer* pOutputMetadata,
    UINT64      requestId)
{
    CamxResult result           = CamxResultSuccess;
    BOOL       hasMasterChanged = FALSE;

    InputMetadataOpticalZoom* pInputMultiCamMetadata =
        static_cast<InputMetadataOpticalZoom*>(pInputMetadata->GetTag(m_vendorTagMultiCamInput));

    OutputMetadataOpticalZoom* pOutputMultiCamMetadata =
        static_cast<OutputMetadataOpticalZoom*>(pOutputMetadata->GetTag(m_vendorTagMultiCamOutput));

    if ((NULL != pInputMultiCamMetadata) && (NULL != pOutputMultiCamMetadata) &&
        (1 < pInputMultiCamMetadata->numInputs))
    {
        UINT32 previousMasterId = pInputMultiCamMetadata->cameraMetadata[0].masterCameraId;
        UINT32 newMasterId      = pOutputMultiCamMetadata->masterCameraId;

        if (previousMasterId != newMasterId)
        {
            result = pOutputMetadata->Merge(pInputMetadata, previousMasterId, newMasterId, TRUE);

            CAMX_LOG_INFO(CamxLogGroupCore, "Update metadata for pipeline %s old master %u new master %u for request %llu",
                GetPipelineName(), previousMasterId, newMasterId, requestId);

            if (CamxResultSuccess == result)
            {
                hasMasterChanged = TRUE;
            }
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Retain metadata for pipeline %s  master %u for request %llu",
                GetPipelineName(), previousMasterId, requestId);
        }
    }

    if (FALSE == hasMasterChanged)
    {
        result = pOutputMetadata->Merge(pInputMetadata, TRUE, TRUE);
    }

    if (CamxResultSuccess == result)
    {
        PrepareResultPSM(requestId, pInputMetadata, pOutputMetadata);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::OpenRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::OpenRequest(
    UINT64 requestId,
    UINT64 CSLSyncID,
    BOOL   isSyncMode,
    UINT32 expectedExposureTimeInMs)
{
    CamxResult result = CamxResultSuccess;

    if (TRUE == IsRealTime())
    {
        // First PCR before stream on will be an init op, do not call CSLOpenRequest on this request.
        // isInitialConfigPending flag is set to true on pipeline init and stream off
        if ((TRUE == m_flags.isInitialConfigPending) && (FALSE == IsStreamedOn()))
        {
            m_flags.isInitialConfigPending = FALSE;
        }
        else
        {
            if (FALSE == GetFlushStatus())
            {
                if ((TRUE == isSyncMode) && (TRUE == GetDualCameraSyncEnabled(requestId)))
                {
                    CAMX_LOG_CONFIG(CamxLogGroupCore, "pipeline[%d] CSLOpenRequest for CSLSyncID: %llu, requestId: %llu with"
                        " syncmode:TRUE  -- request exposureTimeout is %u milliseconds",
                        m_pipelineIndex, CSLSyncID, requestId, expectedExposureTimeInMs);
                    result = CSLOpenRequest(m_hCSLSession, m_hCSLLinkHandle, CSLSyncID, TRUE, CSLSyncLinkModeSync,
                        expectedExposureTimeInMs);
                }
                else
                {
                    CAMX_LOG_CONFIG(CamxLogGroupCore, "pipeline[%d] CSLOpenRequest for CSLSyncID: %llu, requestId: %llu with"
                        " syncmode:FALSE  -- request exposureTimeout is %u milliseconds",
                        m_pipelineIndex, CSLSyncID, requestId, expectedExposureTimeInMs);
                    result = CSLOpenRequest(m_hCSLSession, m_hCSLLinkHandle, CSLSyncID, TRUE, CSLSyncLinkModeNoSync,
                        expectedExposureTimeInMs);
                }
            }
            else
            {
                CAMX_LOG_CONFIG(CamxLogGroupCore, "pipeline[%d] CSLOpenRequest is Cancelled for CSLSyncID: %llu,"
                    "requestId: %llu with request exposureTimeout is %u milliseconds",
                    m_pipelineIndex, CSLSyncID, requestId, expectedExposureTimeInMs);
                result = CamxResultECancelledRequest;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::ProcessRequest(
    PipelineProcessRequestData* pPipelineRequestData)
{
    CAMX_ASSERT(NULL != pPipelineRequestData);
    CAMX_ASSERT(NULL != pPipelineRequestData->pCaptureRequest);
    CAMX_ASSERT(NULL != pPipelineRequestData->pPerBatchedFrameInfo);

    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupCore,
                            SCOPEEventTopologyProcessRequest,
                            pPipelineRequestData->pCaptureRequest->requestId);

    CamxResult      result                    = CamxResultSuccess;
    CaptureRequest* pCaptureRequest           = pPipelineRequestData->pCaptureRequest;
    UINT            currentActiveStreamIdMask = 0;
    UINT64          requestId                 = pCaptureRequest->requestId;
    UINT            perRequestIdIndex         = (requestId % MaxPerRequestInfo);

    // Must have recieved all fences created back and freed their node in the LDLL
    CAMX_ASSERT(0 == m_perRequestInfo[perRequestIdIndex].fences.NumNodes());
    UINT32* pSequenceId = m_perRequestInfo[perRequestIdIndex].pSequenceId;
    Utils::Memset(pSequenceId, 0, sizeof(UINT32) * GetBatchedHALOutputNum());

    CAMX_ASSERT(GetBatchedHALOutputNum() >= pCaptureRequest->GetBatchedHALOutputNum(pCaptureRequest));

    m_perRequestInfo[perRequestIdIndex].pSequenceId = pSequenceId;
    m_perRequestInfo[perRequestIdIndex].aMetadataReady = 0;
    m_perRequestInfo[perRequestIdIndex].isSofDispatched = FALSE;
    m_perRequestInfo[perRequestIdIndex].numNodesRequestIdDone = 0;
    m_perRequestInfo[perRequestIdIndex].numNodesMetadataDone = 0;
    m_perRequestInfo[perRequestIdIndex].numNodesPartialMetadataDone = 0;
    m_perRequestInfo[perRequestIdIndex].numNodesConfigDone = 0;
    m_perRequestInfo[perRequestIdIndex].batchFrameIntervalNanoSeconds = 0;
    m_perRequestInfo[perRequestIdIndex].bufferDone = 0;
    m_perRequestInfo[perRequestIdIndex].fences.FreeAllNodesAndTheirClientData();
    m_perRequestInfo[perRequestIdIndex].isSlowdownPresent = FALSE;

    CAMX_ASSERT(m_perRequestInfo[perRequestIdIndex].request.pStreamBuffers ==
        &m_pStreamBufferBlob[perRequestIdIndex * GetBatchedHALOutputNum()]);

    result = CaptureRequest::PartialDeepCopy(&m_perRequestInfo[perRequestIdIndex].request, pCaptureRequest);

    UINT*                pCurrentActiveStreams = NULL;
    BOOL                 differentStreams      = FALSE;
    PerBatchedFrameInfo* pPerBatchedFrameInfo  = NULL;

    if (CamxResultSuccess == result)
    {
        for (UINT batchIndex = 0; batchIndex < pCaptureRequest->GetBatchedHALOutputNum(pCaptureRequest); batchIndex++)
        {
            // Create mapping between request id and framework frame number.
            m_perRequestInfo[perRequestIdIndex].pSequenceId[batchIndex] =
                pPipelineRequestData->pPerBatchedFrameInfo[batchIndex].sequenceId;

            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                             "%s In perRequestInfo[%d], map request id %lld with sequence id %d",
                             GetPipelineIdentifierString(),
                             perRequestIdIndex,
                             requestId,
                             pPipelineRequestData->pPerBatchedFrameInfo[batchIndex].sequenceId);
        }

        /// @note It is assumed the first batchInfo will have all the streams that can possibly be enabled in other batches
        pPerBatchedFrameInfo = &pPipelineRequestData->pPerBatchedFrameInfo[0];
#if ASSERTS_ENABLED
        /// Validate assumption that the first batchInfo will have all the streams that can possibly be enabled
        for (UINT batchIndex = 1; batchIndex < pCaptureRequest->GetBatchedHALOutputNum(pCaptureRequest); batchIndex++)
        {
            if ((pPerBatchedFrameInfo[batchIndex].activeStreamIdMask | pPerBatchedFrameInfo[0].activeStreamIdMask) !=
                pPerBatchedFrameInfo[0].activeStreamIdMask)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Pipeline-ERROR: First batch index does not have all the streams possible");
            }
        }
#endif // ASSERTS_ENABLED

        currentActiveStreamIdMask = pPerBatchedFrameInfo->activeStreamIdMask;
        differentStreams          = FALSE;

        if (m_lastRequestActiveStreamIdMask != currentActiveStreamIdMask)
        {
            differentStreams = TRUE;
            m_lastRequestActiveStreamIdMask = currentActiveStreamIdMask;
        }
        pCurrentActiveStreams = &currentActiveStreamIdMask;

        if (TRUE == m_pChiContext->GetHwContext()->GetImageSensorModuleData(m_cameraId)->IsExternalSensor())
        {
            PublishSensorModeInformation(pCaptureRequest->requestId);
        }

        // Init debug/tuning data buffer
        if (TRUE == HAL3MetadataUtil::IsDebugDataEnable())
        {
            InitializeDebugDataBuffer(requestId);
        }

        // Use this table to find the correct request ID from CSL sync ID when CSL callback comes
        m_pCSLSyncIDToRequestId[pCaptureRequest->CSLSyncID % (MaxPerRequestInfo * GetBatchedHALOutputNum())] =
            pCaptureRequest->requestId;

        if (FALSE == IsRealTime())
        {
            UINT64        requestIdInner = pCaptureRequest->requestId;
            MetadataSlot* pInputSlot     = m_pInputPool->GetSlot(requestIdInner);
            MetadataSlot* pMainSlot      = m_pMainPool->GetSlot(requestIdInner);
            UINT32        tag            = SensorTimestamp;

            if (NULL != pInputSlot)
            {
                UINT64* pTimestamp = static_cast<UINT64*>(pInputSlot->GetMetadataByTag(tag));

                if (NULL != pTimestamp)
                {
                    pMainSlot->SetMetadataByTag(tag, pTimestamp, 1, GetPipelineIdentifierString());
                }
                else
                {
                    UINT64 timeStamp = 0;
                    pMainSlot->SetMetadataByTag(tag, &timeStamp, 1, GetPipelineIdentifierString());
                    CAMX_LOG_WARN(CamxLogGroupCore, "Timestamp tag found! %p", pTimestamp);
                }

                pMainSlot->PublishMetadataList(&tag, 1);

                SendOfflineShutterNotification(requestIdInner, pTimestamp);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline::%s Metadata slots not found input %p main %p",
                               GetPipelineIdentifierString(), pInputSlot, pMainSlot);
            }
        }

        // Publish vendor tag to indicate if request has video buffer or not
        PublishRequestHasVideoBufferTag(pPipelineRequestData);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "PartialDeepCopy() failed perRequestIdIndex=%u", perRequestIdIndex);
    }

    m_lastSubmittedRequestId = CamX::Utils::MaxUINT64(pCaptureRequest->requestId, m_lastSubmittedRequestId);
    CAMX_LOG_VERBOSE(CamxLogGroupCore, "Pipeline::%s last submitted request updated to %llu",
                    GetPipelineIdentifierString(), m_lastSubmittedRequestId);

    // Check if node recources acquired before process request, Acquire node recources if not yet acquired
    // This can happen in early PCR usecase
    if (CamxResultSuccess == result)
    {
        result = CallNodeAcquireResources();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline::%s Acquire resources failed",
                    GetPipelineIdentifierString());
        }
    }

    // For PCRs with input buffer(s), shall we assume that the HAL have successfully waited on (all) the input
    // acquire fence(s) before forwarding such a request to the topology? If YES, the topology need not wait on
    // any fences on the SourceBuffer nodes. If NO, then the fence will have to be waited on.

    // Since CSLCreateNativeFence is not implemented, it is expected that a CSLFence is created and passed on
    // with each Input Buffer of an Offline/Reprocess PCR

    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT_MESSAGE(32 >= m_nodeCount, "Using a 32 bit bitmask to track node enable, and there are too many nodes");

        // If this is too costly, try to check for flush within the loop and partially setup requests.
        UINT32 nodesEnabled = 0;

        for (UINT nodeIndex = 0; nodeIndex  < m_nodeCount; nodeIndex++)
        {
            m_ppNodes[nodeIndex]->InvalidateRequest();
        }

        for (UINT nodeIndex = 0; nodeIndex < m_orderedNodeCount; nodeIndex++)
        {
            BOOL isNodeEnabled = FALSE;

            m_ppOrderedNodes[nodeIndex]->SetupRequest(pPerBatchedFrameInfo,
                                                pCurrentActiveStreams,
                                                differentStreams,
                                                requestId,
                                                pCaptureRequest->CSLSyncID,
                                                &isNodeEnabled);

            if (TRUE == isNodeEnabled)
            {
                nodesEnabled = Utils::BitSet(nodesEnabled, nodeIndex);
            }
            else
            {
                CAMX_LOG_DRQ("Failed to setup Node %s for Pipeline %u RequestId %llu Session %p",
                        m_ppOrderedNodes[nodeIndex]->NodeIdentifierString(),
                        m_pipelineIndex, requestId, m_pSession);
            }
        }

        // Queueing the nodes is deferred to ensure all nodes have completed setup before any are invoked, which could happen
        // in the event of a previous request kicking the queue
        UINT32 nodesSentToDRQ = 0;
        for (UINT nodeIndex = 0; nodeIndex  < m_orderedNodeCount ; nodeIndex++)
        {
            if (TRUE == Utils::IsBitSet(nodesEnabled, nodeIndex))
            {
                if (FALSE == GetFlushStatus())
                {
                    CAMX_LOG_DRQ("Queueing Node: %s on pipeline: %d for new requestId: %llu",
                            m_ppOrderedNodes[nodeIndex]->NodeIdentifierString(),
                            m_pipelineIndex, requestId);

                    result = m_pDeferredRequestQueue->AddDeferredNode(requestId,
                            m_ppOrderedNodes[nodeIndex], NULL);

                    if (CamxResultSuccess == result)
                    {
                        nodesSentToDRQ = Utils::BitSet(nodesSentToDRQ, nodeIndex);
                    }
                }
                else
                {
                    CAMX_LOG_DRQ("Skipping Node: %s on pipeline: %d for new requestId: %llu as Session %p in Flush state",
                            m_ppOrderedNodes[nodeIndex]->NodeIdentifierString(),
                            m_pipelineIndex, requestId, m_pSession);
                    m_ppOrderedNodes[nodeIndex]->Flush(requestId);
                }
            }
        }

        if (0 != nodesSentToDRQ)
        {

            // Consider any nodes now ready
            m_pDeferredRequestQueue->DispatchReadyNodes();

        }

        // If no nodes got setup, a flush was called right before the setup, no nodes were sent to DRQ. Notify the session.
        if (0 == nodesEnabled)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Returning Cancelled. Equal to flush");
            result = CamxResultECancelledRequest;
        }
    }

    if (CamxResultSuccess != result)
    {
        // If the request is cancelled/flushed, the next request stream setup should happen again.
        CAMX_LOG_ERROR(CamxLogGroupCore, "Cancelling request. Reset the streamMask. res = %d", result);
        m_lastRequestActiveStreamIdMask = 0;
    }
    if ((TRUE == GetFlushStatus()) && (result == CamxResultECancelledRequest))
    {
        CAMX_LOG_CONFIG(CamxLogGroupCore,
            "Flush status is set for %s with cancel request, trigger request error for reqid %d",
                       GetPipelineIdentifierString(), requestId);
        TriggerRequestError(requestId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::CreateNodes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::CreateNodes(
    PipelineCreateInputData*  pCreateInputData,
    PipelineCreateOutputData* pCreateOutputData)
{
    /// @todo (CAMX-423) Break it into smaller functions

    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    CamxResult                result                    = CamxResultSuccess;
    const PipelineDescriptor* pPipelineDescriptor       = pCreateInputData->pPipelineDescriptor;
    const PerPipelineInfo*    pPipelineInfo             = &pPipelineDescriptor->pipelineInfo;
    UINT                      numInPlaceSinkBufferNodes = 0;
    Node*                     pInplaceSinkBufferNode[MaxNodeType];
    UINT                      numBypassableNodes        = 0;
    Node*                     pBypassableNodes[MaxNodeType];
    ExternalComponentInfo*    pExternalComponentInfo    = HwEnvironment::GetInstance()->GetExternalComponent();
    UINT                      numExternalComponents     = HwEnvironment::GetInstance()->GetNumExternalComponent();

    CAMX_ASSERT(NULL == m_ppNodes);

    m_nodeCount                        = pPipelineInfo->numNodes;
    m_ppNodes                          = static_cast<Node**>(CAMX_CALLOC(sizeof(Node*) * m_nodeCount));
    m_ppOrderedNodes = static_cast<Node**>(CAMX_CALLOC(sizeof(Node*) * m_nodeCount));

    CAMX_ASSERT(NULL != m_ppOrderedNodes);

    if ((NULL != m_ppNodes) &&
        (NULL != m_ppOrderedNodes))
    {
        NodeCreateInputData createInputData  = { 0 };

        createInputData.pPipeline    = this;
        createInputData.pChiContext  = pCreateInputData->pChiContext;

        UINT nodeIndex = 0;

        CAMX_LOG_CONFIG(CamxLogGroupCore,
                      "Topology: Creating Pipeline %s, numNodes %d isSensorInput %d isRealTime %d",
                      GetPipelineIdentifierString(),
                      m_nodeCount,
                      IsSensorInput(),
                      IsRealTime());

        for (UINT numNodes = 0; numNodes < m_nodeCount; numNodes++)
        {
            NodeCreateOutputData createOutputData = { 0 };
            createInputData.pNodeInfo         = &(pPipelineInfo->pNodeInfo[numNodes]);
            createInputData.pipelineNodeIndex = numNodes;

            for (UINT propertyIndex = 0; propertyIndex < createInputData.pNodeInfo->nodePropertyCount; propertyIndex++)
            {
                for (UINT index = 0; index < numExternalComponents; index++)
                {
                    if ((pExternalComponentInfo[index].nodeAlgoType == ExternalComponentNodeAlgo::COMPONENTALGORITHM) &&
                        (NodePropertyCustomLib == createInputData.pNodeInfo->pNodeProperties[propertyIndex].id))
                    {
                        CHAR matchString[FILENAME_MAX] = {0};
                        OsUtils::SNPrintF(matchString, FILENAME_MAX, "%s.%s",
                            static_cast<CHAR*>(createInputData.pNodeInfo->pNodeProperties[propertyIndex].pValue),
                            SharedLibraryExtension);

                        if (OsUtils::StrNICmp(pExternalComponentInfo[index].pComponentName,
                            matchString,
                            OsUtils::StrLen(pExternalComponentInfo[index].pComponentName)) == 0)
                        {
                            if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOAF)
                            {
                                createInputData.pAFAlgoCallbacks = &pExternalComponentInfo[index].AFAlgoCallbacks;
                            }
                            else if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOAEC)
                            {
                                createInputData.pAECAlgoCallbacks = &pExternalComponentInfo[index].AECAlgoCallbacks;
                            }
                            else if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOAWB)
                            {
                                createInputData.pAWBAlgoCallbacks = &pExternalComponentInfo[index].AWBAlgoCallbacks;
                            }
                            else if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOAFD)
                            {
                                createInputData.pAFDAlgoCallbacks = &pExternalComponentInfo[index].AFDAlgoCallbacks;
                            }
                            else if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOASD)
                            {
                                createInputData.pASDAlgoCallbacks = &pExternalComponentInfo[index].ASDAlgoCallbacks;
                            }
                            else if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOPD)
                            {
                                createInputData.pPDLibCallbacks = &pExternalComponentInfo[index].PDLibCallbacks;
                            }
                            else if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOHIST)
                            {
                                createInputData.pHistAlgoCallbacks = &pExternalComponentInfo[index].histAlgoCallbacks;
                            }
                            else if (pExternalComponentInfo[index].statsAlgo == ExternalComponentStatsAlgo::ALGOTRACK)
                            {
                                createInputData.pTrackerAlgoCallbacks = &pExternalComponentInfo[index].trackerAlgoCallbacks;
                            }
                        }
                    }
                    else if ((pExternalComponentInfo[index].nodeAlgoType == ExternalComponentNodeAlgo::COMPONENTHVX) &&
                        (NodePropertyCustomLib == createInputData.pNodeInfo->pNodeProperties[propertyIndex].id) &&
                        (OsUtils::StrStr(pExternalComponentInfo[index].pComponentName,
                        static_cast<CHAR*>(createInputData.pNodeInfo->pNodeProperties[propertyIndex].pValue)) != NULL))
                    {
                        createInputData.pHVXAlgoCallbacks = &pExternalComponentInfo[index].HVXAlgoCallbacks;
                    }
                }
            }

            result = Node::Create(&createInputData, &createOutputData);

            if (CamxResultSuccess == result)
            {
                CAMX_LOG_CONFIG(CamxLogGroupCore,
                              "Topology::%s Node::%s Type %d numInputPorts %d numOutputPorts %d",
                              GetPipelineIdentifierString(),
                              createOutputData.pNode->NodeIdentifierString(),
                              createOutputData.pNode->Type(),
                              createInputData.pNodeInfo->inputPorts.numPorts,
                              createInputData.pNodeInfo->outputPorts.numPorts);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_WARN(CamxLogGroupCore, "[%s] Cannot get publish list for %s",
                                  GetPipelineIdentifierString(), createOutputData.pNode->NodeIdentifierString());
                }

                if (StatsProcessing == createOutputData.pNode->Type())
                {
                    m_flags.hasStatsNode = TRUE;
                }

                if (0x10000 == createOutputData.pNode->Type())
                {
                    m_flags.hasIFENode = TRUE;
                }

                if ((JPEGAggregator == createOutputData.pNode->Type()) || (0x10001 == createOutputData.pNode->Type()))
                {
                    m_flags.hasJPEGNode = TRUE;
                }

                m_ppNodes[nodeIndex] = createOutputData.pNode;

                if ((TRUE == createOutputData.createFlags.isSinkBuffer) ||
                    (TRUE == createOutputData.createFlags.isSinkNoBuffer))
                {
                    m_nodesSinkOutPorts.nodeIndices[m_nodesSinkOutPorts.numNodes] = nodeIndex;
                    m_nodesSinkOutPorts.numNodes++;
                }

                if ((TRUE == createOutputData.createFlags.isSinkBuffer) && (TRUE == createOutputData.createFlags.isInPlace))
                {
                    pInplaceSinkBufferNode[numInPlaceSinkBufferNodes] = createOutputData.pNode;
                    numInPlaceSinkBufferNodes++;
                }

                if (TRUE == createOutputData.createFlags.isBypassable)
                {
                    pBypassableNodes[numBypassableNodes] = createOutputData.pNode;
                    numBypassableNodes++;
                }

                if ((TRUE == createOutputData.createFlags.isSourceBuffer) || (Sensor == m_ppNodes[nodeIndex]->Type()))
                {
                    m_nodesSourceInPorts.nodeIndices[m_nodesSourceInPorts.numNodes] = nodeIndex;
                    m_nodesSourceInPorts.numNodes++;
                }

                if (TRUE == createOutputData.createFlags.willNotifyConfigDone)
                {
                    m_numConfigDoneNodes++;
                }

                if (TRUE == createOutputData.createFlags.hasDelayedNotification)
                {
                    m_isDelayedPipeline = TRUE;
                }

                nodeIndex++;
            }
            else
            {
                break;
            }
        }

        if (CamxResultSuccess == result)
        {
            // Set the input link of the nodes - basically connects output port of one node to input port of another
            for (UINT nodeIndexInner = 0; nodeIndexInner < m_nodeCount; nodeIndexInner++)
            {
                const PerNodeInfo* pXMLNode = &pPipelineInfo->pNodeInfo[nodeIndexInner];

                for (UINT inputPortIndex = 0; inputPortIndex < pXMLNode->inputPorts.numPorts; inputPortIndex++)
                {
                    const InputPortInfo* pInputPortInfo = &pXMLNode->inputPorts.pPortInfo[inputPortIndex];

                    if (FALSE == m_ppNodes[nodeIndexInner]->IsSourceBufferInputPort(inputPortIndex))
                    {
                        m_ppNodes[nodeIndexInner]->SetInputLink(inputPortIndex,
                                                           pInputPortInfo->portId,
                                                           m_ppNodes[pInputPortInfo->parentNodeIndex],
                                                           pInputPortInfo->parentOutputPortId);

                        m_ppNodes[nodeIndexInner]->SetUpLoopBackPorts(inputPortIndex);

                        /// In the parent node's output port, Save this node as one of the output node connected to it.
                        m_ppNodes[pInputPortInfo->parentNodeIndex]->AddOutputNodes(pInputPortInfo->parentOutputPortId,
                                                                                   m_ppNodes[nodeIndexInner]);

                        /// Update access device index list for the source port based on current nodes device index list
                        /// At this point the source node which maintains the output buffer manager have the access information
                        /// required for buffer manager creation.
                        m_ppNodes[pInputPortInfo->parentNodeIndex]->AddOutputDeviceIndices(
                            pInputPortInfo->parentOutputPortId,
                            m_ppNodes[nodeIndexInner]->DeviceIndices(),
                            m_ppNodes[nodeIndexInner]->DeviceIndexCount());

                        const ImageFormat* pImageFormat = m_ppNodes[nodeIndexInner]->GetInputPortImageFormat(inputPortIndex);
                        if (NULL != pImageFormat)
                        {
                            CAMX_LOG_CONFIG(CamxLogGroupCore,
                                          "Topology: Pipeline[%s] "
                                          "Link: Node::%s(outPort %d) --> (inPort %d) Node::%s using format %d",
                                          GetPipelineIdentifierString(),
                                          m_ppNodes[pInputPortInfo->parentNodeIndex]->NodeIdentifierString(),
                                          pInputPortInfo->parentOutputPortId,
                                          pInputPortInfo->portId,
                                          m_ppNodes[nodeIndexInner]->NodeIdentifierString(),
                                          pImageFormat->format);
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Invalid pImageFormat",
                                           m_ppNodes[nodeIndexInner]->NodeIdentifierString());
                        }
                    }
                    else
                    {
                        m_ppNodes[nodeIndexInner]->SetupSourcePort(inputPortIndex, pInputPortInfo->portId);
                    }
                }
                if (TRUE == m_ppNodes[nodeIndexInner]->IsLoopBackNode())
                {
                    m_ppNodes[nodeIndexInner]->EnableParentOutputPorts();
                }
            }
        }

        /// @todo (CAMX-1015) Look into non recursive implementation
        if (CamxResultSuccess == result)
        {
            for (UINT index = 0; index < m_nodesSinkOutPorts.numNodes; index++)
            {
                if (NULL != m_ppNodes[m_nodesSinkOutPorts.nodeIndices[index]])
                {
                    m_ppNodes[m_nodesSinkOutPorts.nodeIndices[index]]->TriggerOutputPortStreamIdSetup();
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "m_ppNodes or m_ppOrderedNodes is Null");
        result = CamxResultENoMemory;
    }

    // Bypass node processing
    if (CamxResultSuccess == result)
    {
        for (UINT index = 0; index < numBypassableNodes; index++)
        {
            pBypassableNodes[index]->BypassNodeProcessing();
        }
    }

    if (CamxResultSuccess == result)
    {
        for (UINT index = 0; index < numInPlaceSinkBufferNodes; index++)
        {
            pInplaceSinkBufferNode[index]->TriggerInplaceProcessing();
        }
    }

    if (CamxResultSuccess == result)
    {
        for (UINT index = 0; index < m_nodesSinkOutPorts.numNodes; index++)
        {
            CAMX_ASSERT((NULL != m_ppNodes) && (NULL != m_ppNodes[m_nodesSinkOutPorts.nodeIndices[index]]));

            Node* pNode = m_ppNodes[m_nodesSinkOutPorts.nodeIndices[index]];

            result = pNode->TriggerBufferNegotiation();

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupCore, "Unable to satisfy node input buffer requirements, retrying with NV12");
                break;
            }
        }
        if (CamxResultSuccess != result)
        {
            result = RenegotiateInputBufferRequirement(pCreateInputData, pCreateOutputData);
        }
    }

    if (CamxResultSuccess != result)
    {
        CAMX_ASSERT_ALWAYS();
        CAMX_LOG_ERROR(CamxLogGroupCore, "%s Creating Nodes Failed. Going to Destroy sequence", GetPipelineIdentifierString());
        DestroyNodes();
    }
    else
    {
        UINT numInputs = 0;

        for (UINT index = 0; index < m_nodesSourceInPorts.numNodes; index++)
        {
            Node*                    pNode         = m_ppNodes[m_nodesSourceInPorts.nodeIndices[index]];
            ChiPipelineInputOptions* pInputOptions = &pCreateOutputData->pPipelineInputOptions[numInputs];

            numInputs += pNode->FillPipelineInputOptions(pInputOptions);
        }

        pCreateOutputData->numInputs = numInputs;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  Pipeline::QueryMetadataInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::QueryMetadataInfo(
    const UINT32    maxPublishTagArraySize,
    UINT32*         pPublishTagArray,
    UINT32*         pPublishTagCount,
    UINT32*         pPartialPublishTagCount,
    UINT32*         pMaxNumMetaBuffers)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(0 < maxPublishTagArraySize);
    CAMX_ASSERT(NULL != pPublishTagArray);
    CAMX_ASSERT(NULL != pPublishTagCount);
    CAMX_ASSERT(NULL != pPartialPublishTagCount);
    CAMX_ASSERT(NULL != pMaxNumMetaBuffers);

    if ((maxPublishTagArraySize >= m_nodePublishSet.size()) &&
        (maxPublishTagArraySize >= m_nodePartialPublishSet.size()))
    {
        UINT32                               tagIndex       = 0;
        std::unordered_set<UINT32>::iterator setIterator;

        *pPartialPublishTagCount = 0;

        if (TRUE == m_bPartialMetadataEnabled)
        {
            if (0 < m_nodePartialPublishSet.size())
            {
                setIterator = m_nodePartialPublishSet.begin();
                for (; setIterator != m_nodePartialPublishSet.end(); ++setIterator)
                {
                    pPublishTagArray[tagIndex++] = *setIterator;
                }
                *pPartialPublishTagCount = static_cast<UINT32>(m_nodePartialPublishSet.size());
            }
        }

        setIterator = m_nodePublishSet.begin();

        for (; setIterator != m_nodePublishSet.end(); ++setIterator)
        {
            pPublishTagArray[tagIndex++] = *setIterator;
        }
        *pPublishTagCount = tagIndex;
    }
    else
    {
        result = CamxResultEOutOfBounds;
    }

    *pMaxNumMetaBuffers = HwEnvironment::GetInstance()->GetStaticSettings()->maxHalRequests;
    *pMaxNumMetaBuffers = *pMaxNumMetaBuffers + m_metaBufferDelay;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  Pipeline::CheckOfflinePipelineInputBufferRequirements
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::CheckOfflinePipelineInputBufferRequirements()
{
    CamxResult result = CamxResultSuccess;

    if (FALSE == IsRealTime())
    {
        CAMX_ASSERT(NULL != m_ppNodes);
        // Re check the buffer requirements only for offline pipelines as input streams are not there during
        // pipeline creation.
        for (UINT index = 0; index < m_nodesSourceInPorts.numNodes; index++)
        {
            CAMX_ASSERT(NULL != m_ppNodes[m_nodesSourceInPorts.nodeIndices[index]]);

            Node* pNode = m_ppNodes[m_nodesSourceInPorts.nodeIndices[index]];

            result = pNode->CheckSourcePortBufferRequirements();

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Unable to satisfy node input buffer requirements, retrying with NV12");
                break;
            }
        }

        if (CamxResultSuccess != result)
        {
            PipelineCreateInputData  pipelineCreateInputData   = { 0 };
            PipelineCreateOutputData pipelineCreateOutputData  = { 0 };

            result = RenegotiateInputBufferRequirement(&pipelineCreateInputData, &pipelineCreateOutputData);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Renegotiate Failed for offline pipeline(%s)", GetPipelineIdentifierString());
            }
        }

    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  Pipeline::FinalizePipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::FinalizePipeline(
    FinalizeInitializationData* pFinalizeInitializationData)
{
    CamxResult                    result              = CamxResultSuccess;
    const ImageSensorModuleData*  pSensorModuleData;

    CAMX_ASSERT(NULL != pFinalizeInitializationData);

    m_pHwContext            = pFinalizeInitializationData->pHwContext;
    m_pDeferredRequestQueue = pFinalizeInitializationData->pDeferredRequestQueue;
    m_pSession              = pFinalizeInitializationData->pSession;
    m_hCSLSession           = m_pSession->GetCSLSession();
    m_numSessionPipelines   = pFinalizeInitializationData->numSessionPipelines;
    m_flags.enableQTimer    = pFinalizeInitializationData->enableQTimer;

    if (CSLInvalidHandle != m_hCSLSession)
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "%s_%u: CSLAddReference(0x%x)", GetPipelineName(), GetPipelineId(), GetCSLSession());
        result = CSLAddReference(GetCSLSession());
    }

    if (CamxResultSuccess == result)
    {
        pSensorModuleData    = m_pChiContext->GetHwContext()->GetImageSensorModuleData(m_cameraId);
        m_currentSensorMode  = pFinalizeInitializationData->pSensorModeInfo->modeIndex;
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Current Sensor mode is %d", m_currentSensorMode);

        // Make sure metadata pools are all initialied before nodes FinalizeInitialization
        m_pInputPool->WaitForMetadataPoolCreation();
        CAMX_ASSERT(PoolStatus::Initialized == m_pInputPool->GetPoolStatus());

        m_pInternalPool->WaitForMetadataPoolCreation();
        CAMX_ASSERT(PoolStatus::Initialized == m_pInternalPool->GetPoolStatus());

        m_pMainPool->WaitForMetadataPoolCreation();
        CAMX_ASSERT(PoolStatus::Initialized == m_pMainPool->GetPoolStatus());

        m_pEarlyMainPool->WaitForMetadataPoolCreation();
        CAMX_ASSERT(PoolStatus::Initialized == m_pEarlyMainPool->GetPoolStatus());

        pFinalizeInitializationData->pDebugDataPool->WaitForMetadataPoolCreation();
        CAMX_ASSERT(PoolStatus::Initialized == pFinalizeInitializationData->pDebugDataPool->GetPoolStatus());

        /// @todo (CAMX-1512) Metadata pools needs to be per pipeline
        m_pDebugDataPool = pFinalizeInitializationData->pDebugDataPool;

        // If not realtime pipeline, need publish all necessary properties into usecase pool
        if ((FALSE == IsRealTime()) ||
            (TRUE == pSensorModuleData->IsExternalSensor()) ||
            ((TRUE == m_pHwContext->GetStaticSettings()->enableExternalSensorModule) && ((TRUE == IsRealTime()))))
        {
            PublishSensorUsecaseProperties(pSensorModuleData);
        }
    }

    if (CamxResultSuccess == result)
    {
        if ((0 == m_nodeCount) || (NULL == m_ppNodes))
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline::%s Failed as nodes are not created ", GetPipelineIdentifierString());
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        MetadataSlot*    pMetadataSlot          = m_pUsecasePool->GetSlot(0);
        MetaBuffer*      pSessionMetaBuffer     = m_pPipelineDescriptor->pSessionMetadata;
        MetaBuffer*      pMetadataSlotDstBuffer = NULL;

        // Metadata might have been updated after buffer negotiation backward walk
        // Merge updated metadata published by the Chi Usecase to this pipeline's UsecasePool
        if ((NULL != pSessionMetaBuffer) && (NULL != pMetadataSlot))
        {
            result = pMetadataSlot->GetMetabuffer(&pMetadataSlotDstBuffer);
            if (CamxResultSuccess == result)
            {
                result = pMetadataSlotDstBuffer->Merge(pSessionMetaBuffer, FALSE);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMeta, "Usecase pool metadata merge failed: %u", result);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot get meta buffer! Error Code: %u", result);
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupMeta, "Pointer to session metadata %p or metadata slot %p",
                          pSessionMetaBuffer, pMetadataSlot);
        }
    }

    if (CamxResultSuccess == result)
    {
        for (UINT i = 0; i < m_nodeCount; i++)
        {
            result = m_ppNodes[i]->FinalizeInitialization(pFinalizeInitializationData);

            if (CamxResultSuccess != result)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Failed to finalize init of node: %d", i);
                break;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        /// @todo (CAMX-1797) Simplify the logic involving m_ppOrderedNodes
        // Reverse the ordered array as its nodes are added in reverse during the walk-back
        for (UINT index = 0; index < (m_orderedNodeCount / 2); index++)
        {
            Node* pNode = m_ppOrderedNodes[index];
            m_ppOrderedNodes[index] = m_ppOrderedNodes[m_orderedNodeCount - index - 1];
            m_ppOrderedNodes[m_orderedNodeCount - index - 1] = pNode;
        }

        // The source node will be added as the last element in the array and the sink nodes will be at the start of the array.
        // And since we need to walk-forward from the source-to-the-sink we go in reverse order here
        for (UINT index = 0; index < m_orderedNodeCount; index++)
        {
            if (NULL != m_ppOrderedNodes[index])
            {
                m_ppOrderedNodes[index]->DetermineBufferProperties();
            }
        }

        // Check if the ordered array was properly filled in - it is possible that buffer negotiation
        // does not traverse certain nodes and we are left with mismatched size between ordered and unordered.
        // In principle, this usually indicates an error in the XML description of the pipeline (for instance,
        // connecting a port both to a NonBufferSink and BufferSink Targets).
        // However, it is possible to end up in this state with some legacy pipelines.
        // Print a warning message and add any untracked nodes to the end of the ordered array for now.
        if (m_nodeCount > m_orderedNodeCount)
        {
            CAMX_LOG_WARN(CamxLogGroupCore,
                "Unordered node array has %d nodes, ordered has %d",
                m_nodeCount,
                m_orderedNodeCount);

            for (UINT i = 0; i < m_nodeCount; i++)
            {
                BOOL nodeIsOrdered = FALSE;
                for (UINT j = 0; j < m_orderedNodeCount; j++)
                {
                    if (m_ppOrderedNodes[j] == m_ppNodes[i])
                    {
                        nodeIsOrdered = TRUE;
                        break;
                    }
                }

                if (FALSE == nodeIsOrdered)
                {
                    m_ppOrderedNodes[m_orderedNodeCount] = m_ppNodes[i];
                    m_orderedNodeCount++;
                }
            }
        }

        // Check if we are still mismatched in terms of node count.
        if (m_nodeCount != m_orderedNodeCount)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore,
                "Unordered and ordered arrays have mismatched number of nodes (%d to %d)",
                m_nodeCount,
                m_orderedNodeCount);
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            for (UINT i = 0; i < m_nodeCount; i++)
            {
                result = m_ppNodes[i]->CreateBufferManagers();

                if (CamxResultSuccess != result)
                {
                    break;
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            // Let the Nodes that need to defer call NotifyPipelineCreated firstly.
            // Make sure that PostPipelineCreate are called in separate threads.
            for (UINT i = 0; i < m_nodeCount; i++)
            {
                BOOL isDefer = m_ppNodes[i]->IsDeferPipelineCreate();
                if (TRUE == isDefer)
                {
                    result = m_ppNodes[i]->NotifyPipelineCreated(isDefer);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline[%s] Failed to initialize %s: ",
                                       GetPipelineIdentifierString(), m_ppNodes[i]->NodeIdentifierString());
                        break;
                    }
                }
            }
            // If Node that need to defer have been notified in seperate threads,
            // then call NotifyPipelineCreated for the rest of node in the main thread serially.
            for (UINT i = 0; i < m_nodeCount; i++)
            {
                BOOL isDefer = m_ppNodes[i]->IsDeferPipelineCreate();
                if (FALSE == isDefer)
                {
                    result = m_ppNodes[i]->NotifyPipelineCreated(isDefer);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline[%s] Failed to initialize %s: ",
                                       GetPipelineIdentifierString(), m_ppNodes[i]->NodeIdentifierString());
                        break;
                    }
                }
            }

            if (CamxResultSuccess == result)
            {
                for (UINT i = 0; i < m_nodeCount; i++)
                {
                    m_ppNodes[i]->WaitForDeferPipelineCreate();
                    result = FilterAndUpdatePublishSet(m_ppNodes[i]);

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline[%s] Failed to UpdatePublishSet %s: ",
                                       GetPipelineIdentifierString(), m_ppNodes[i]->NodeIdentifierString());
                        break;
                    }
                }
            }
        }
        if (CamxResultSuccess != result)
        {
            CAMX_ASSERT_ALWAYS();
            DestroyNodes();
        }
        else
        {
            result = PrepareStreamOn();
        }
    }

    if (CamxResultSuccess == result)
    {
        result = Link();
        SetPipelineStatus(PipelineStatus::FINALIZED);
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "pipeline:%d finalize done!", GetPipelineId());
    }

    if ((CamxResultSuccess == result) && (TRUE == IsRealTime()))
    {
        result = CSLRegisterMessageHandler(m_hCSLSession, m_hCSLLinkHandle,
            CSLMessageHandler, static_cast<VOID*>(this));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to register message handler, error code %d", result);
        }
    }

    if (CamxResultSuccess == result)
    {
        result =  VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.av_timer", "use_av_timer",
                                                           &m_vendorTagIndexAVTimer);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL,
                "Failed to find org.codeaurora.qcamera3.av_timer, resultCode=%s",
                Utils::CamxResultToString(result));
        }
    }

    if (CamxResultSuccess == result)
    {
        result =
            VendorTagManager::QueryVendorTagLocation("org.quic.camera.qtimer", "timestamp", &m_vendorTagIndexTimestamp);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL,
                "Failed to find org.quic.camera.qtimer.timestamp, resultCode=%s",
                Utils::CamxResultToString(result));
        }
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicameraoutputmetadata",
            "OutputMetadataOpticalZoom", &m_vendorTagMultiCamOutput);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL,
                "Failed to find com.qti.chi.multicameraoutputmetadata.OutputMetadataOpticalZoom, resultCode=%s",
                Utils::CamxResultToString(result));
        }
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicamerainputmetadata",
            "InputMetadataOpticalZoom", &m_vendorTagMultiCamInput);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL,
                "Failed to find com.qti.chi.multicamerainputmetadata.InputMetadataOpticalZoom, resultCode=%s",
                Utils::CamxResultToString(result));
        }
    }

    // For real time pipelines, update the number of IFEs used for this pipeline.
    // Update this info in m_pPipelineDescriptor's metadata, this info will be used to update the resource cost.
    if ((CamxResultSuccess == result) && (TRUE == IsRealTime()))
    {
        UINT32 numIFEsUsedMetaTag = 0;
        result = VendorTagManager::QueryVendorTagLocation(
            "org.quic.camera.ISPConfigData", "numIFEsUsed", &numIFEsUsedMetaTag);

        if (CamxResultSuccess == result)
        {
            MetadataSlot* pUsecaseMetadataSlot = m_pUsecasePool->GetSlot(0);
            MetaBuffer*   pSessionMetaBuffer   = m_pPipelineDescriptor->pSessionMetadata;

            INT32* pNumIFEsUsed = static_cast<INT32*>(pUsecaseMetadataSlot->GetMetadataByTag(numIFEsUsedMetaTag));

            if (NULL != pNumIFEsUsed)
            {
                pSessionMetaBuffer->SetTag(numIFEsUsedMetaTag, pNumIFEsUsed, 1, sizeof(INT32));
            }
            else
            {
                INT32 numIFEsUsed = 1;
                pSessionMetaBuffer->SetTag(numIFEsUsedMetaTag, &numIFEsUsed, 1, sizeof(INT32));
                CAMX_LOG_WARN(CamxLogGroupCore, "numIFEsUsed tag not found! updating numIFEsUsed value to 1");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore,
                           "Pipeline[%s] Failed to find org.quic.camera.ISPConfigData.numIFEsUsed, result=%s",
                           Utils::CamxResultToString(result));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  Pipeline::DestroyNodes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::DestroyNodes()
{

    if (NULL != m_ppOrderedNodes)
    {
        CAMX_FREE(m_ppOrderedNodes);
        m_ppOrderedNodes = NULL;
    }

    if (NULL != m_ppNodes)
    {
        for (UINT i = 0; i < m_nodeCount; i++)
        {
            if (NULL != m_ppNodes[i])
            {
                m_ppNodes[i]->Destroy();
                m_ppNodes[i] = NULL;
            }
        }

        CAMX_FREE(m_ppNodes);
        m_ppNodes   = NULL;
        m_nodeCount = 0;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  Pipeline::SetIPERTPipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 VOID Pipeline::SetIPERTPipeline(BOOL isRealTime)
{
    m_isIPERealtime = isRealTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  Pipeline::GetIPERTPipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pipeline::GetIPERTPipeline()
{
    return m_isIPERealtime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  Pipeline::CheckIPERTPipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pipeline::CheckIPERTPipeline()
{
    for (UINT i = 0; i < m_nodeCount; i++)
    {
        if (TRUE == m_ppNodes[i]->GetIPERTPipeline())
        {
            m_isIPERealtime = TRUE;
            return TRUE;
        }
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::GetNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Node* Pipeline::GetNode(
    UINT nodeType,
    UINT instanceId)
{
    Node* pNode = NULL;

    for (UINT i = 0; i < m_nodeCount; i++)
    {
        if ((NULL != m_ppNodes[i]) && (m_ppNodes[i]->Type() == nodeType) && (m_ppNodes[i]->InstanceID() == instanceId))
        {
            pNode = m_ppNodes[i];
            break;
        }
    }

    return pNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::IsNodeExist
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pipeline::IsNodeExist(
    UINT nodeType)
{
    BOOL found = FALSE;

    for (UINT i = 0; i < m_nodeCount; i++)
    {
        if ((NULL != m_ppNodes[i]) && (m_ppNodes[i]->Type() == nodeType))
        {
            found = TRUE;
            break;
        }
    }

    return found;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::IsNodeExistByName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pipeline::IsNodeExistByName(
    CHAR* pNodeName)
{
    BOOL found = FALSE;

    for (UINT i = 0; i < m_nodeCount; i++)
    {
        if ((NULL != m_ppNodes[i]) && !(strcmp(m_ppNodes[i]->Name(), pNodeName)))
        {
            found = TRUE;
            break;
        }
    }

    return found;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::IsNodeExistByNodePropertyValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pipeline::IsNodeExistByNodePropertyValue(
    CHAR* pNodePropertyValue)
{
    BOOL found = FALSE;

    if (NULL != pNodePropertyValue)
    {
        for (UINT i = 0; i < m_pPipelineDescriptor->pipelineInfo.numNodes; i++)
        {
            for (UINT j = 0; j < m_pPipelineDescriptor->pipelineInfo.pNodeInfo[i].nodePropertyCount; j++)
            {
                if (NULL != m_pPipelineDescriptor->pipelineInfo.pNodeInfo[i].pNodeProperties)
                {
                    const CHAR* pPropertyValue = (const CHAR*)m_pPipelineDescriptor->pipelineInfo.pNodeInfo[i].
                                                pNodeProperties[j].pValue;
                    if (0 == OsUtils::StrCmp(pNodePropertyValue, pPropertyValue))
                    {
                        found = TRUE;
                        break;
                    }
                }
            }
        }
    }

    return found;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::HasSnapshotJPEGStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pipeline::HasSnapshotJPEGStream()
{
    BOOL found = FALSE;

    for (UINT i = 0; i < m_pPipelineDescriptor->numOutputs; i++)
    {
        ChiStreamWrapper* pStreamWrapper = m_pPipelineDescriptor->outputData[i].pOutputStreamWrapper;
        Camera3Stream* pStream = pStreamWrapper->GetNativeStream();
        CAMX_ASSERT(NULL != pStream);

        if (HALPixelFormatBlob == pStream->format)
        {
            found = TRUE;
            break;
        }
    }

    return found;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::GetIntraPipelinePerFramePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MetadataPool* Pipeline::GetIntraPipelinePerFramePool(
    PoolType poolType,
    UINT     pipelineId)
{
    return m_pSession->GetIntraPipelinePerFramePool(poolType, pipelineId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::GetPeerRealtimePipelineId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::GetPeerRealtimePipelineId(
    MultiRequestSyncData* pMultiReqSync,
    UINT                  ownPipelineId,
    UINT*                 pPeerPipelineId)
{
    CamxResult  result               = CamxResultSuccess;
    UINT        index                = 0;
    UINT        numOfActivePipeline  = 0;
    BOOL        foundOwnPipeline     = FALSE;
    BOOL        foundPeerPipeline    = FALSE;

    if (NULL == pPeerPipelineId)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "NULL pointer to pass peer pipeline ID");
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        for (index = 0; index < MaxPipelinesPerSession; index++)
        {
            if (TRUE == pMultiReqSync->currReq.isActive[index])
            {
                numOfActivePipeline++;
                if (ownPipelineId == index)
                {
                    foundOwnPipeline = TRUE;
                }
                else
                {
                    *pPeerPipelineId = index;
                    foundPeerPipeline = TRUE;
                }
            }
        }

        if ((numOfActivePipeline <= MaxActiveRealtimePipelines) && (TRUE == foundPeerPipeline) && (TRUE == foundOwnPipeline))
        {
            result = CamxResultSuccess;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Error parsing multi request info: numOfActivePipeline:%d foundOwnPipeline:%d",
                           numOfActivePipeline, foundOwnPipeline);
            result = CamxResultEFailed;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "IsMultiReq:%d NumActive:%d Own_PipelineId:%d Peer_PipelineId:%d",
                     pMultiReqSync->currReq.isMultiRequest,
                     numOfActivePipeline,
                     ownPipelineId,
                     foundPeerPipeline ? *pPeerPipelineId : -1);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::ProcessPartialMetadataRequestIdDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::ProcessPartialMetadataRequestIdDone(
    UINT64 requestId)
{
    CamxResult        result                        = CamxResultSuccess;
    UINT              perRequestIdIndex             = requestId % MaxPerRequestInfo;
    PerRequestInfo*   pPerRequestInfo               = &m_perRequestInfo[perRequestIdIndex];
    ResultsData       resultsData                   = {};
    MetadataSlot*     pMetadataSlot                 = NULL;
    MetaBuffer*       pOutputMetaBuffer             = NULL;

    resultsData.type = CbType::PartialMetadata;
    pMetadataSlot    = m_pMainPool->GetSlot(requestId);

    if (NULL != pMetadataSlot)
    {
        result = pMetadataSlot->GetMetabuffer(&pOutputMetaBuffer);
    }

    if ((CamxResultSuccess == result) && (NULL != pOutputMetaBuffer))
    {
        for (UINT batchIndex = 0;
             batchIndex < pPerRequestInfo->request.GetBatchedHALOutputNum(&pPerRequestInfo->request);
             batchIndex++)
        {
            resultsData.cbPayload.partialMetadata.sequenceId                     =
                pPerRequestInfo->pSequenceId[batchIndex];
            resultsData.cbPayload.partialMetadata.partialMetaPayload.pMetaBuffer = pOutputMetaBuffer;
            resultsData.cbPayload.partialMetadata.partialMetaPayload.requestId   = static_cast<UINT32>(requestId);

            resultsData.pipelineIndex = m_pipelineIndex;
            resultsData.pPrivData     = pPerRequestInfo->request.pPrivData;

            auto      hPipeline  = m_pPipelineDescriptor;
            UINT32    sequenceId = pPerRequestInfo->pSequenceId[batchIndex];
            BINARY_LOG(LogEvent::Pipeline_PartialMetadataDone, hPipeline, requestId, sequenceId);
            CAMX_LOG_INFO(CamxLogGroupMeta,
                "%s Notify partial metadata for request id %lld and sequenceId %d",
                GetPipelineIdentifierString(),
                requestId,
                sequenceId);
            if (TRUE == m_bPartialMetadataEnabled)
            {
                m_pSession->NotifyResult(&resultsData);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::ProcessMetadataBufferDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::ProcessMetadataBufferDone(
    UINT64 publishRequestId)
{
    UINT            perRequestIdIndex = publishRequestId % MaxPerRequestInfo;
    PerRequestInfo* pPerRequestInfo   = &m_perRequestInfo[perRequestIdIndex];
    CamxResult      result            = CamxResultSuccess;
    ResultsData     resultsData       = {};
    MetaBuffer*     pInputMetaBuffer  = NULL;
    MetaBuffer*     pResultMetaBuffer = NULL;

    resultsData.type = CbType::MetaBufferDone;

    MetadataSlot* pMainSlot  = m_pMainPool->GetSlot(publishRequestId);
    MetadataSlot* pInputSlot = m_pInputPool->GetSlot(publishRequestId);

    result = pInputSlot->DetachMetabuffer(&pInputMetaBuffer);

    if (CamxResultSuccess == result)
    {
        result = pMainSlot->DetachMetabuffer(&pResultMetaBuffer);
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_INFO(CamxLogGroupMeta,
                      "DetachMetabuffer for pipeline %s %p %p reqId %d res %d", GetPipelineIdentifierString(),
                      pInputMetaBuffer, pResultMetaBuffer, publishRequestId, result);

        // One buffer per batch
        resultsData.cbPayload.metabufferDone.sequenceId = pPerRequestInfo->pSequenceId[0];

        resultsData.cbPayload.metabufferDone.pInputMetabuffer  = pInputMetaBuffer;
        resultsData.cbPayload.metabufferDone.pOutputMetabuffer = pResultMetaBuffer;
        m_pSession->NotifyResult(&resultsData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::ProcessMetadataRequestIdDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::ProcessMetadataRequestIdDone(
    UINT64 requestId,
    BOOL   earlyMetadata)
{
    CamxResult      result                        = CamxResultSuccess;
    UINT            perRequestIdIndex             = requestId % MaxPerRequestInfo;
    PerRequestInfo* pPerRequestInfo               = &m_perRequestInfo[perRequestIdIndex];
    UINT64*         pFrameTimestamp               = NULL;
    UINT64          batchFrameIntervalNanoSeconds = pPerRequestInfo->batchFrameIntervalNanoSeconds;
    UINT64          captureTime                   = OsUtils::GetNanoSeconds();
    ResultsData     resultsData                   = {};
    MetadataSlot*   pMetadataSlot                 = NULL;
    MetaBuffer*     pInputMetaBuffer              = NULL;
    MetaBuffer*     pOutputMetaBuffer             = NULL;

    if (TRUE == earlyMetadata)
    {
        pMetadataSlot    = m_pEarlyMainPool->GetSlot(requestId);
        resultsData.type = CbType::EarlyMetadata;
        m_pInputPool->GetSlot(requestId)->GetMetabuffer(&pInputMetaBuffer);
        m_pMainPool->GetSlot(requestId)->GetMetabuffer(&pOutputMetaBuffer);
    }
    else
    {
        pMetadataSlot    = m_pMainPool->GetSlot(requestId);
        resultsData.type = CbType::Metadata;

        result = m_pInputPool->GetSlot(requestId)->GetMetabuffer(&pInputMetaBuffer);

        if (CamxResultSuccess == result)
        {
            result = pMetadataSlot->GetMetabuffer(&pOutputMetaBuffer);
        }

        if (CamxResultSuccess == result)
        {
            result = PrepareResultMetadata(pInputMetaBuffer, pOutputMetaBuffer, requestId);

            CAMX_LOG_VERBOSE(CamxLogGroupMeta, "Metadata count after merge %u %u result %d",
                             pOutputMetaBuffer->Count(), pInputMetaBuffer->Count(), result);

            UINT dumpMetadata = HwEnvironment::GetInstance()->GetStaticSettings()->dumpMetadata;

            CHAR metadataFileName[FILENAME_MAX];

            dumpMetadata &= (TRUE == IsRealTime()) ?  RealTimeMetadataDumpMask : OfflineMetadataDumpMask;

            if (0 != (dumpMetadata & 0x3))
            {
                OsUtils::SNPrintF(metadataFileName, FILENAME_MAX, "outputMetadata_%s_%d.txt",
                    GetPipelineIdentifierString(),
                    requestId);

                pOutputMetaBuffer->DumpDetailsToFile(metadataFileName);
            }
            else if (0 != ((dumpMetadata>>2) & 0x3))
            {
                OsUtils::SNPrintF(metadataFileName, FILENAME_MAX, "outputMetadata_%s_%d.bin",
                    GetPipelineIdentifierString(),
                    requestId);

                pOutputMetaBuffer->BinaryDump(metadataFileName);
            }
        }

        CAMX_ASSERT(CamxResultSuccess == result);

        if (CamxResultSuccess == result)
        {
            // Fill undefined default values
            if (0 == pMetadataSlot->GetMetadataCountByTag(ControlVideoStabilizationMode, FALSE))
            {
                ControlVideoStabilizationModeValues videoStabilizationMode =
                    (0 == m_videoStabilizationCaps) ? ControlVideoStabilizationModeOff : ControlVideoStabilizationModeOn;

                pMetadataSlot->SetMetadataByTag(ControlVideoStabilizationMode,
                                                static_cast<VOID*>(&videoStabilizationMode),
                                                1,
                                                GetPipelineIdentifierString());
            }
        }
    }

    CAMX_ASSERT(NULL != pMetadataSlot);

    if (TRUE == pMetadataSlot->IsPublished(UnitType::Metadata, SensorTimestamp))
    {
        pFrameTimestamp = static_cast<UINT64*>(m_pMainPool->GetSlot(requestId)->GetMetadataByTag(SensorTimestamp));

        if (NULL != pFrameTimestamp)
        {
            captureTime = static_cast<UINT64>(*pFrameTimestamp);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "pFrameTimestamp is NULL");
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "SensorTimestamp not yet published for %llu", requestId);
    }

    for (UINT batchIndex = 0;
         batchIndex < pPerRequestInfo->request.GetBatchedHALOutputNum(&pPerRequestInfo->request);
         batchIndex++)
    {
        UINT64 frameTimestamp       = captureTime + (batchFrameIntervalNanoSeconds * batchIndex);

        resultsData.cbPayload.metadata.sequenceId                 = pPerRequestInfo->pSequenceId[batchIndex];
        resultsData.cbPayload.metadata.metaPayload.pMetaBuffer[0] = pInputMetaBuffer;
        resultsData.cbPayload.metadata.metaPayload.pMetaBuffer[1] = pOutputMetaBuffer;
        resultsData.cbPayload.metadata.metaPayload.requestId      = static_cast<UINT32>(requestId);

        resultsData.pipelineIndex = m_pipelineIndex;
        resultsData.pPrivData     = pPerRequestInfo->request.pPrivData;

        auto      hPipeline  = m_pPipelineDescriptor;
        UINT32    sequenceId = pPerRequestInfo->pSequenceId[batchIndex];
        BINARY_LOG(LogEvent::Pipeline_MetadataDone, hPipeline, requestId, sequenceId);
        CAMX_LOG_INFO(CamxLogGroupCore,
                      "%s Notify %s metadata for request id %lld and sequenceId %d timestamp = %llu",
                      GetPipelineIdentifierString(),
                      earlyMetadata == TRUE ? "early" : "",
                      requestId,
                      sequenceId,
                      frameTimestamp);

        // Update metadata SensorTimestamp
        if (resultsData.type == CbType::Metadata)
        {
            result = pMetadataSlot->SetMetadataByTag(SensorTimestamp, reinterpret_cast<VOID*>(&frameTimestamp), 1,
                                                     GetPipelineIdentifierString());

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "SetMetadataByTag for SensorTimestamp failure %d", result);
            }
            else
            {
                result = pMetadataSlot->PublishMetadata(SensorTimestamp);

                CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to publish SensorTimestamp");
            }
        }
        m_pSession->NotifyResult(&resultsData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::ProcessMetadataRequestIdError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::ProcessMetadataRequestIdError(
    UINT64        requestId)
{
    ResultsData     resultsData                   = {};
    UINT            perRequestIdIndex             = requestId % MaxPerRequestInfo;
    PerRequestInfo* pPerRequestInfo               = &m_perRequestInfo[perRequestIdIndex];

    resultsData.type                               = CbType::Error;
    resultsData.cbPayload.error.code               = MessageCodeResult;
    resultsData.pipelineIndex                      = m_pipelineIndex;
    resultsData.pPrivData                          = pPerRequestInfo->request.pPrivData;

    CAMX_LOG_INFO(CamxLogGroupCore, "%s Reporting metadata error for request %llu", GetPipelineIdentifierString(), requestId);

    for (UINT batchIndex = 0;
         batchIndex < pPerRequestInfo->request.GetBatchedHALOutputNum(&pPerRequestInfo->request);
         batchIndex++)
    {
        resultsData.cbPayload.error.sequenceId = pPerRequestInfo->pSequenceId[batchIndex];
        m_pSession->NotifyResult(&resultsData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::SinkPortFenceErrorSignaled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SinkPortFenceErrorSignaled(
    UINT            sinkPortStreamId,
    UINT32          sequenceId,
    UINT64          requestId,
    ChiBufferInfo*  pChiBufferInfo)
{
    ResultsData     resultsData       = {};
    UINT            perRequestIdIndex = requestId % MaxPerRequestInfo;
    PerRequestInfo* pPerRequestInfo   = &m_perRequestInfo[perRequestIdIndex];

    CAMX_ASSERT(NULL != pChiBufferInfo);

    resultsData.type                       = CbType::Error;
    resultsData.cbPayload.error.code       = MessageCodeBuffer;
    resultsData.cbPayload.error.sequenceId = sequenceId;
    resultsData.cbPayload.error.streamId   = sinkPortStreamId;
    resultsData.cbPayload.error.bufferInfo = *pChiBufferInfo;
    resultsData.pipelineIndex              = m_pipelineIndex;
    resultsData.pPrivData                  = pPerRequestInfo->request.pPrivData;

    if (TRUE == GetFlushStatus())
    {
        CAMX_LOG_INFO(CamxLogGroupCore,
            "%s Reporting stream done for stream=%d, sequenceId=%d Buffer type=%d handle=%p",
            GetPipelineIdentifierString(),
            resultsData.cbPayload.error.streamId,
            sequenceId,
            pChiBufferInfo->bufferType,
            pChiBufferInfo->phBuffer);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore,
            "%s Reporting stream done for stream=%d, sequenceId=%d Buffer type=%d handle=%p",
            GetPipelineIdentifierString(),
            resultsData.cbPayload.error.streamId,
            sequenceId,
            pChiBufferInfo->bufferType,
            pChiBufferInfo->phBuffer);
    }

    m_pSession->NotifyResult(&resultsData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::SinkPortFenceSignaled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SinkPortFenceSignaled(
    UINT           sinkPortStreamId,
    UINT32         sequenceId,
    UINT64         requestId,
    ChiBufferInfo* pChiBufferInfo)
{
    ResultsData     resultsData       = {};
    UINT            perRequestIdIndex = requestId % MaxPerRequestInfo;
    PerRequestInfo* pPerRequestInfo   = &m_perRequestInfo[perRequestIdIndex];

    CAMX_ASSERT(NULL != pChiBufferInfo);

    resultsData.pipelineIndex = m_pipelineIndex;
    resultsData.pPrivData     = pPerRequestInfo->request.pPrivData;

    resultsData.type                        = CbType::Buffer;
    resultsData.cbPayload.buffer.sequenceId = sequenceId;
    resultsData.cbPayload.buffer.streamId   = sinkPortStreamId;
    resultsData.cbPayload.buffer.bufferInfo = *pChiBufferInfo;

    CAMX_LOG_INFO(CamxLogGroupCore,
                  "%s Reporting stream done for stream=%d, sequenceId=%d, Buffer type=%d handle=%p",
                  GetPipelineIdentifierString(),
                  resultsData.cbPayload.buffer.streamId,
                  sequenceId,
                  pChiBufferInfo->bufferType,
                  pChiBufferInfo->phBuffer);
    m_pSession->NotifyResult(&resultsData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::SealDebugDataBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SealDebugDataBuffer(
    DebugDataType   dataType,
    DebugData*      pDebugData)
{
    DebugDataTag*   pTag            = NULL;
    VOID*           pDataStart      = NULL;
    SIZE_T          dataSize        = HAL3MetadataUtil::DebugDataSize(dataType);

    if (0 != dataSize)
    {
        if (sizeof(DebugDataTag) <= dataSize &&
            (HAL3MetadataUtil::DebugDataOffset(dataType) <= (pDebugData->size + sizeof(DebugDataTag))))
        {
            pDataStart = Utils::VoidPtrInc(pDebugData->pData, HAL3MetadataUtil::DebugDataOffset(dataType));

            if (DebugDataType::IPETuning == dataType)
            {
                dataSize /= DebugDataPartitionsIPE;
                for (UINT partition = 0; DebugDataPartitionsIPE > partition; partition++)
                {
                    pTag        = reinterpret_cast<DebugDataTag*>(pDataStart);
                    pTag->id    = DebugDataTagID::UnusedSpace;
                    pTag->type  = DebugDataTagType::UInt8;
                    pTag->count = static_cast<TagCount>(dataSize - sizeof(DebugDataTag));

                    pDataStart  = Utils::VoidPtrInc(pDataStart, dataSize);
                }
            }
            if (DebugDataType::BPSTuning == dataType)
            {
                dataSize /= DebugDataPartitionsBPS;
                for (UINT partition = 0; DebugDataPartitionsBPS > partition; partition++)
                {
                    pTag        = reinterpret_cast<DebugDataTag*>(pDataStart);
                    pTag->id    = DebugDataTagID::UnusedSpace;
                    pTag->type  = DebugDataTagType::UInt8;
                    pTag->count = static_cast<TagCount>(dataSize - sizeof(DebugDataTag));

                    pDataStart  = Utils::VoidPtrInc(pDataStart, dataSize);
                }
            }
            else
            {
                pTag        = reinterpret_cast<DebugDataTag*>(pDataStart);
                pTag->id    = DebugDataTagID::UnusedSpace;
                pTag->type  = DebugDataTagType::UInt8;
                pTag->count = static_cast<TagCount>(dataSize - sizeof(DebugDataTag));
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupDebugData, "Size not enough for type: %u", dataType);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::InitializeDebugDataBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::InitializeDebugDataBuffer(
    UINT64 requestId)
{
    VOID*           pBlob           = NULL;
    DebugData*      pDebugData      = NULL;
    MetadataSlot*   pMetadataSlot   = m_pDebugDataPool->GetSlot(requestId);

    pMetadataSlot->GetPropertyBlob(reinterpret_cast<VOID**>(&pBlob));

    // Clear older values from the pool, do this only for DebugData wich is not using isPublish() and direct access data
    pDebugData = reinterpret_cast<DebugData*>(
                    Utils::VoidPtrInc(pBlob,
                                      DebugDataPropertyOffsets[PropertyIDDebugDataAll & ~DriverInternalGroupMask]));

    if ((NULL != pDebugData) && (NULL != pDebugData->pData))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupDebugData, "Init/Reset debug-data blob reqID: %llu pointer: %p size: %zu",
                         requestId, pDebugData->pData, pDebugData->size);

        // Use non-zero value, zero is a very common number.
        Utils::Memset(pDebugData->pData, 0xFF, pDebugData->size);

        // Seal every data block
        SealDebugDataBuffer(DebugDataType::AEC,         pDebugData);
        SealDebugDataBuffer(DebugDataType::AWB,         pDebugData);
        SealDebugDataBuffer(DebugDataType::AF,          pDebugData);
        SealDebugDataBuffer(DebugDataType::IFETuning,   pDebugData);
        SealDebugDataBuffer(DebugDataType::IPETuning,   pDebugData);
        SealDebugDataBuffer(DebugDataType::BPSTuning,   pDebugData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::NonSinkPortFenceErrorSignaled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::NonSinkPortFenceErrorSignaled(
    CSLFence* phFence,
    UINT64    requestId)
{
    m_pDeferredRequestQueue->FenceErrorSignaledCallback(m_pipelineIndex, phFence, requestId, GetFlushStatus());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::NonSinkPortFenceSignaled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::NonSinkPortFenceSignaled(
    CSLFence* phFence,
    UINT64    requestId)
{
    m_pDeferredRequestQueue->FenceSignaledCallback(phFence, requestId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::SendOfflineShutterNotification
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SendOfflineShutterNotification(
    UINT64  requestId,
    UINT64* pTimestamp)
{
    CSLFrameMessage message;
    message.requestID = requestId;

    if (NULL != pTimestamp)
    {
        message.timestamp = *pTimestamp;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "timestamp should not be ZERO!");
    }

    SendShutterNotification(&message);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::SendShutterNotification
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SendShutterNotification(
    const CSLFrameMessage* pMessage)
{
    CamxResult result = CamxResultSuccess;

    if (CamxResultSuccess != CanProcessCSLMessage())
    {
        return;
    }

    if (CamxInvalidRequestId != pMessage->requestID)
    {
        ResultsData resultsData = {};

        resultsData.type          = CbType::Async;
        resultsData.pipelineIndex = m_pipelineIndex;

        UINT8*          pUseAVTimer                   = NULL;
        UINT            perRequestIdIndex             = pMessage->requestID % MaxPerRequestInfo;
        PerRequestInfo* pPerRequestInfo               = &m_perRequestInfo[perRequestIdIndex];
        UINT64          captureTime                   = 0;
        UINT64          frameTimestamp                = 0;
        UINT64          batchFrameIntervalNanoSeconds = 0;

        if ((1 != pMessage->requestID - m_lastSubmittedShutterRequestId) &&
            (m_lastSubmittedShutterRequestId > m_flushInfo.lastFlushRequestId))
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "nonincrementing shutter request id: %d m_lastSubmittedShutterRequestId: %d; "
                           "lastValidRequestIdbeforeFlush: %llu",
                           pMessage->requestID,
                           m_lastSubmittedShutterRequestId,
                           m_flushInfo.lastValidRequestIdbeforeFlush);
            if (FALSE == GetPipelineTriggeringRecovery())
            {
                OsUtils::RaiseSignalAbort();
            }
        }

        m_lastSubmittedShutterRequestId = pMessage->requestID;

        CAMX_ASSERT(pMessage->requestID == m_perRequestInfo[perRequestIdIndex].request.requestId);
        if (0 == pMessage->timestamp)
        {
            CAMX_LOG_WARN(CamxLogGroupCore, "pMessage->timestamp is 0, falling back to system time");
        }

        resultsData.cbPayload.async.requestID = m_perRequestInfo[perRequestIdIndex].request.requestId;

        if (m_vendorTagIndexAVTimer > 0)
        {
            MetadataSlot* pMetadataSlot = m_pInputPool->GetSlot(pMessage->requestID);
            pUseAVTimer = static_cast<UINT8 *>(pMetadataSlot->GetMetadataByTag(m_vendorTagIndexAVTimer));
        }

        if (((NULL != pUseAVTimer) && (1 == *pUseAVTimer)) ||
            (1 == m_flags.enableQTimer))
        {
            captureTime = pMessage->timestamp;
        }

        // Fallback option
        if (captureTime == 0)
        {
            captureTime = OsUtils::GetNanoSeconds();
        }

        // There is one CSL SOF notification per batch, so we need to update batchFrameInterval for HFR base on the FPS,
        // and append the interval for each frames.
        if (pPerRequestInfo->request.numBatchedFrames > 1)
        {
            /// @todo (CAMX-1015) - Check if we don't need to query fps every single time
            MetadataSlot*   pUsecaseMetadataSlot = m_pUsecasePool->GetSlot(0);

            UINT* pFps = static_cast<UINT*>(pUsecaseMetadataSlot->GetMetadataByTag(PropertyIDUsecaseFPS));

            if ((NULL != pFps) && (0 != *pFps))
            {
                batchFrameIntervalNanoSeconds = NanoSecondsPerSecond / *pFps;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "pFps is invalid %p", pFps);
            }
        }

        pPerRequestInfo->batchFrameIntervalNanoSeconds = batchFrameIntervalNanoSeconds;

        // Update metadata SensorTimestamp
        MetadataSlot*   pMainMetadataSlot  = m_pMainPool->GetSlot(pMessage->requestID);
        MetadataSlot*   pEarlyMetadataSlot = m_pEarlyMainPool->GetSlot(pMessage->requestID);
        CAMX_ASSERT(NULL != pMainMetadataSlot);

        result =  pMainMetadataSlot->SetMetadataByTag(SensorTimestamp, reinterpret_cast<VOID*>(&captureTime), 1,
                                                      GetPipelineIdentifierString());

        if (CamxResultSuccess == result)
        {
            result = pEarlyMetadataSlot->SetMetadataByTag(SensorTimestamp, reinterpret_cast<VOID*>(&captureTime), 1,
                                                          GetPipelineIdentifierString());
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "SetMetadataByTag for SensorTimestamp failure %d", result);
        }
        else
        {
            UINT32 tag = SensorTimestamp;
            result = pMainMetadataSlot->PublishMetadataList(&tag, 1);
            CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to publish SensorTimestamp");
        }

        for (UINT batchIndex = 0;
             batchIndex < pPerRequestInfo->request.GetBatchedHALOutputNum(&pPerRequestInfo->request);
             batchIndex++)
        {
            frameTimestamp                         = (captureTime + (batchFrameIntervalNanoSeconds * batchIndex));
            resultsData.cbPayload.async.sequenceId = pPerRequestInfo->pSequenceId[batchIndex];
            resultsData.cbPayload.async.timestamp  = frameTimestamp;
            resultsData.pPrivData                  = pPerRequestInfo->request.pPrivData;

            // If metadata is ready, we already tried to notify result and shutter was not yet sent, this means
            // there was a slowdown in the system for this request; take note for recovery purposes
            if (1 == m_perRequestInfo[pMessage->requestID % MaxPerRequestInfo].aMetadataReady)
            {
                m_perRequestInfo[pMessage->requestID % MaxPerRequestInfo].isSlowdownPresent = TRUE;
            }

            if (FALSE == GetPipelineTriggeringRecovery())
            {
                m_pSession->NotifyResult(&resultsData);
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupCore, "Not sending Shutter Notification as recovery has been triggered");
            }
            m_perRequestInfo[pMessage->requestID % MaxPerRequestInfo].isSofDispatched = TRUE;
            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Shutter for sequence %u", resultsData.cbPayload.async.sequenceId);
        }

        BOOL nodesOutstanding =
            CamxAtomicCompareExchangeU(&m_perRequestInfo[pMessage->requestID % MaxPerRequestInfo].aMetadataReady, 0, 1);

        if (FALSE == nodesOutstanding)
        {
            ProcessMetadataRequestIdDone(pMessage->requestID, FALSE);
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Shutter not sent, result = %d", result);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::CanProcessCSLMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::CanProcessCSLMessage()
{
    CamxResult result = CamxResultSuccess;

    if (m_pCSLSyncIDToRequestId == NULL ||
        (PipelineStatus::RESOURCES_RELEASED == GetPipelineStatus()))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "pipeline is destroy");
        result = CamxResultEFailed;
    }

    if (0 == GetLivePendingRequest())
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "No pending requests in pipeline to process!");
        result = CamxResultEFailed;
    }

    if (TRUE == GetFlushStatus())
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Flush is in progress, do not process this CSLMessage");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::SendErrorNotification
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SendErrorNotification(
    const CSLErrorMessage* pMessage)
{
    ResultsData resultsData = {};

    resultsData.type          = CbType::Error;
    resultsData.pipelineIndex = m_pipelineIndex;
    m_isFullRecovery          = FALSE;

    switch (pMessage->errorType)
    {
        case CSLErrorMessageCodeDevice:
            resultsData.cbPayload.error.code = MessageCodeDevice;
            break;
        case CSLErrorMessageCodeRequest:
            resultsData.cbPayload.error.code = MessageCodeRequest;
            break;
        case CSLErrorMessageCodeBuffer:
            resultsData.cbPayload.error.code = MessageCodeBuffer;
            break;
        // This is intentional, both events should be handled the same way
        case CSLErrorMessageCodeSOFFreeze:
        case CSLErrorMessageCodeRecovery:
        {
            if (FALSE == GetFlushStatus())
            {
                resultsData.cbPayload.error.code = MessageCodeRecovery;
            }
            break;
        }
        case CSLErrorMessageCodeFullRecovery:
        {
            if (FALSE == GetFlushStatus())
            {
                resultsData.cbPayload.error.code = MessageCodeRecovery;
                m_isFullRecovery                 = TRUE;
            }
            break;
        }
        default:
            CAMX_LOG_ERROR(CamxLogGroupCore, "Unexpected error type:%d", pMessage->errorType);
            break;
    }

    UINT            perRequestIdIndex = pMessage->requestID % MaxPerRequestInfo;
    PerRequestInfo* pPerRequestInfo   = &m_perRequestInfo[perRequestIdIndex];

    // No request frame is applicable to the error
    if (CamxInvalidRequestId == pMessage->requestID)
    {
        // CSLErrorMessageCodeDevice, CSLErrorMessageCodeRecovery and CSLErrorMessageCodeSOFFreeze are not specific to a
        // requestId; only allow an error to continue with invalid request id in these cases
        if ((CSLErrorMessageCodeDevice == pMessage->errorType) || (CSLErrorMessageCodeRecovery == pMessage->errorType) ||
            (CSLErrorMessageCodeSOFFreeze == pMessage->errorType) ||
            (CSLErrorMessageCodeFullRecovery == pMessage->errorType))
        {
            resultsData.cbPayload.error.sequenceId = 0;
            resultsData.pPrivData = pPerRequestInfo->request.pPrivData;
            m_pSession->NotifyResult(&resultsData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Cannot handle error from KMD for RequestId 0. Expected error type 0, 3 4 or 5"
                           "(CSLErrorMessageCodeDevice), got error type:%d", pMessage->errorType);
        }
    }
    else
    {
        CAMX_ASSERT(pMessage->requestID == pPerRequestInfo->request.requestId);

        for (UINT batchIndex = 0;
             batchIndex < pPerRequestInfo->request.GetBatchedHALOutputNum(&pPerRequestInfo->request);
             batchIndex++)
        {
            resultsData.cbPayload.error.sequenceId = pPerRequestInfo->pSequenceId[batchIndex];
            resultsData.pPrivData                  = pPerRequestInfo->request.pPrivData;

            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                "SendErrorNotification message request id %lld, sequence ID %d",
                pMessage->requestID,
                resultsData.cbPayload.error.sequenceId);

            m_pSession->NotifyResult(&resultsData);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::SendRecoveryError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SendRecoveryError(
    UINT64 requestId)
{
    ResultsData resultsData = {};

    resultsData.type                    = CbType::Error;
    resultsData.pipelineIndex           = m_pipelineIndex;
    resultsData.cbPayload.error.code    = MessageCodeRecovery;

    UINT            perRequestIdIndex   = requestId % MaxPerRequestInfo;
    PerRequestInfo* pPerRequestInfo     = &m_perRequestInfo[perRequestIdIndex];

    if (CamxInvalidRequestId != requestId)
    {
        CAMX_ASSERT(requestId == pPerRequestInfo->request.requestId);

        for (UINT batchIndex = 0;
             batchIndex < pPerRequestInfo->request.GetBatchedHALOutputNum(&pPerRequestInfo->request);
             batchIndex++)
        {
            resultsData.cbPayload.error.sequenceId  = pPerRequestInfo->pSequenceId[batchIndex];
            resultsData.pPrivData                   = pPerRequestInfo->request.pPrivData;

            CAMX_LOG_CONFIG(CamxLogGroupCore,
                          "Sending recovery error notification message request id %lld, sequence ID %d",
                          requestId,
                          resultsData.cbPayload.error.sequenceId);

            m_pSession->NotifyResult(&resultsData);
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupCore, "Failed to send recovery notification, requestId invalid:%llu", requestId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::SendSOFNotification
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SendSOFNotification(
    const CSLFrameMessage* pMessage)
{
    ResultsData resultsData         = {};

    if (CamxResultSuccess != CanProcessCSLMessage())
    {
        return;
    }

    resultsData.type                                = CbType::SOF;
    resultsData.pipelineIndex                       = m_pipelineIndex;
    resultsData.cbPayload.sof.frameNum              = static_cast<UINT32>(pMessage->frameCount);
    resultsData.cbPayload.sof.timestamp             = pMessage->timestamp;
    resultsData.cbPayload.sof.bIsSequenceIdValid    = (CamxInvalidRequestId != pMessage->requestID);

    if (resultsData.cbPayload.sof.bIsSequenceIdValid)
    {
        UINT            perRequestIdIndex       = pMessage->requestID % MaxPerRequestInfo;
        PerRequestInfo* pPerRequestInfo         = &m_perRequestInfo[perRequestIdIndex];
        resultsData.cbPayload.sof.sequenceId    = pPerRequestInfo->pSequenceId[0];
    }

    if (FALSE == GetPipelineTriggeringRecovery())
    {
        m_pSession->NotifyResult(&resultsData);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupCore, "Not sending SOF as recovery has been triggered");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::UpdateCurrExpTimeUseBySensor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::UpdateCurrExpTimeUseBySensor(
    UINT64 currExposureTimeUseBySensor)
{
    if (NULL != m_pSession)
    {
        m_pSession->UpdateCurrExpTimeUseBySensor(currExposureTimeUseBySensor);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "m_pSession is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::CheckForRecovery
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::CheckForRecovery(
    const CSLFrameMessage* pMessage)
{
    BOOL        isSequenceValid     = (CamxInvalidRequestId != pMessage->requestID);
    ResultsData errorData           = {};
    UINT32      requestQueueDepth   = m_pSession->GetCurrentRequestQueueDepth();
    BOOL        sendRecovery        = FALSE;
    BOOL        delayRecovery       = FALSE;

    const StaticSettings* pStaticSettings = m_pChiContext->GetStaticSettings();

    // Reset the SOF watchdog counter if (1) SOF is serviced with valid request or
    // (2) there are no active request in pipeline
    if ((TRUE == isSequenceValid) || (m_lastCompletedRequestId == m_lastSubmittedRequestId))
    {
        m_invalidSOFCounter = 0;
    }
    else
    {
        // We only want to trigger recovery if there has been activity and we are not flushing
        if ((FALSE == GetFlushStatus()) && (FALSE == m_pSession->IsResultHolderEmpty()))
        {
            // If invalidSOF counter has hit twice the request queue depth, we know that we are stuck
            if (m_invalidSOFCounter > (requestQueueDepth * 2))
            {
                for (UINT i = 0; i < MaxPerRequestInfo; i++)
                {
                    if (TRUE == m_perRequestInfo[i].isSlowdownPresent)
                    {
                        delayRecovery = TRUE;
                        CAMX_LOG_INFO(CamxLogGroupCore, "Detected a slowdown for request:%llu, don't trigger recovery",
                                      m_perRequestInfo[i].request.requestId);
                        m_perRequestInfo[i].isSlowdownPresent = FALSE;
                    }
                }

                if ((m_lastCompletedRequestId != m_lastSubmittedRequestId) && (FALSE == delayRecovery))
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Hit SOF threshold of [%d] consecutive frames with invalid"
                                   "requestId; triggering watchdog recovery for pipeline %s",
                                   m_invalidSOFCounter, GetPipelineIdentifierString());

                    // Since we need to do recovery and we don't have way of knowing which handle to choose
                    // because we have gotten continuous SOF, we are attempting with an arbitrary handle
                    PerRequestInfo* pPerRequestInfo = &m_perRequestInfo[0];

                    errorData.type                          = CbType::Error;
                    errorData.pipelineIndex                 = m_pipelineIndex;
                    errorData.cbPayload.error.code          = MessageCodeRecovery;
                    errorData.cbPayload.error.sequenceId    = 0;
                    errorData.cbPayload.error.streamId      = 0;
                    errorData.pPrivData                     = NULL;

                    sendRecovery = TRUE;
                }
                m_invalidSOFCounter = 0;
            }
            else
            {
                m_invalidSOFCounter++;
            }
        }
        else
        {
            m_invalidSOFCounter = 0;
        }
    }

    if ((TRUE == pStaticSettings->enableWatchdogRecovery) && (TRUE == sendRecovery))
    {
        m_pSession->NotifyResult(&errorData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::CSLMessageHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::CSLMessageHandler(
    VOID*        pUserData,
    CSLMessage*  pMessage)
{
    CAMX_ASSERT((NULL != pMessage) && (NULL != pUserData));

    Pipeline*         pPipeline         = static_cast<Pipeline*>(pUserData);
    CHITIMESTAMPINFO  timestampInfo     = { 0 };
    UINT16            requestIdIndex    = 0;
    MetadataPool*     pMainPool         = NULL;
    MetadataSlot*     pMainMetadataSlot = NULL;
    UINT64*           pTimestamp;

    switch (pMessage->type)
    {
        case CSLMessageTypeFrame:

            timestampInfo.timestamp = pMessage->message.frameMessage.timestamp;
            timestampInfo.frameId   = pMessage->message.frameMessage.frameCount;

            if (CamxResultSuccess != pPipeline->CanProcessCSLMessage())
            {
                break;
            }

            // The requestID in CSL message is CSL sync ID and so need to get the real pipeline request ID
            requestIdIndex = static_cast<UINT16>(pMessage->message.frameMessage.requestID %
                                                 (MaxPerRequestInfo * pPipeline->GetBatchedHALOutputNum()));
            if (CamxInvalidRequestId != pMessage->message.frameMessage.requestID)
            {
                pMessage->message.frameMessage.requestID = pPipeline->m_pCSLSyncIDToRequestId[requestIdIndex];
            }

            if (CamxInvalidRequestId != pMessage->message.frameMessage.requestID)
            {
                pMainPool = pPipeline->GetPerFramePool(PoolType::PerFrameResult);
                if (NULL != pMainPool)
                {
                    pMainMetadataSlot = pMainPool->GetSlot(pMessage->message.frameMessage.requestID);
                    if (NULL == pMainMetadataSlot)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to obtain main pool matadata slot");
                    }
                    else
                    {
                        UINT64* pSensorExposureTime      = static_cast<UINT64*>(pMainMetadataSlot->GetMetadataByTag(
                                                                      SensorExposureTime));
                        if (NULL != pSensorExposureTime)
                        {
                            pPipeline->UpdateCurrExpTimeUseBySensor(*pSensorExposureTime);
                        }
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to obtain matadata pool");
                }
            }

            if (pMessage->message.frameMessage.timestampType == CSLTimestampTypeQtimer)
            {
                // We publish the QTimer timestamp through vendor tag and Monotonic system timestamp through
                // shutter notification
                if ((CamxInvalidRequestId != pMessage->message.frameMessage.requestID) && (NULL != pMainMetadataSlot))
                {
                    pMainMetadataSlot->SetMetadataByTag(pPipeline->m_vendorTagIndexTimestamp,
                                                        &timestampInfo,
                                                        sizeof(CHITIMESTAMPINFO),
                                                        pPipeline->GetPipelineIdentifierString());
                    pMainMetadataSlot->WriteLock();
                    pMainMetadataSlot->PublishMetadataList(&(pPipeline->m_vendorTagIndexTimestamp), 1);
                    pMainMetadataSlot->Unlock();
                }
            }

            CAMX_LOG_INFO(CamxLogGroupCore,
                "frameMessage:requestID=%lld, linkhdl:%p, frameCount=%lld, timestamp=%lld, monoTimestamp=%lld",
                pMessage->message.frameMessage.requestID,
                pMessage->message.frameMessage.link_hdl,
                pMessage->message.frameMessage.frameCount,
                timestampInfo.timestamp,
                pMessage->message.frameMessage.timestamp);

            if (CamxResultSuccess != pPipeline->CanProcessCSLMessage())
            {
                break;
            }

            if (TRUE == pMessage->message.frameMessage.bNotify)
            {
                pPipeline->SendSOFNotification(&(pMessage->message.frameMessage));

                for (UINT i = 0; i < pPipeline->m_nodeCount; i++)
                {
                    if (NULL != pPipeline->m_ppNodes[i])
                    {
                        pPipeline->m_ppNodes[i]->NotifyNodeCSLMessage(pMessage);
                    }
                }

                if ((CamxInvalidRequestId != pMessage->message.frameMessage.requestID) && (NULL != pMainMetadataSlot))
                {
                    pTimestamp = static_cast<UINT64*>(pMainMetadataSlot->GetMetadataByTag(PropertyIDSensorExposureStartTime));

                    if (NULL != pTimestamp)
                    {
                        pMessage->message.frameMessage.timestamp = *pTimestamp;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to get sensor exposure start time");
                    }
                }

                pPipeline->SendShutterNotification(&(pMessage->message.frameMessage));
            }
            else
            {
                // bNotify only true for BOOT_SOF; BOOT_SOF only exist for valid requests, therefore we need to check
                // for recovery here
                pPipeline->CheckForRecovery(&(pMessage->message.frameMessage));
            }
            break;
        case CSLMessageTypeError:

            if (FALSE == pPipeline->GetPipelineTriggeringRecovery())
            {
                // The requestID in CSL message is CSL sync ID and so need to get the real pipeline request ID
                requestIdIndex = static_cast<UINT16>(pMessage->message.errorMessage.requestID %
                                                     (MaxPerRequestInfo * pPipeline->GetBatchedHALOutputNum()));
                if (CamxInvalidRequestId != pMessage->message.errorMessage.requestID)
                {
                    pMessage->message.errorMessage.requestID = pPipeline->m_pCSLSyncIDToRequestId[requestIdIndex];
                }

                CAMX_LOG_ERROR(CamxLogGroupCore,
                    "request id %lld, error type = %d, device handle = %d, resource index = %d",
                    pMessage->message.errorMessage.requestID,
                    pMessage->message.errorMessage.errorType,
                    pMessage->message.errorMessage.hDevice,
                    pMessage->message.errorMessage.resourceIndex);
                for (UINT i = 0; i < pPipeline->m_nodeCount; i++)
                {
                    pPipeline->m_ppNodes[i]->DumpDebugNodeInfo(pMessage->message.errorMessage.requestID);
                }

                pPipeline->SendErrorNotification(&(pMessage->message.errorMessage));
            }
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupCore, "Unexpected CSL message type");
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::RenegotiateInputBufferRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::RenegotiateInputBufferRequirement(
    PipelineCreateInputData*  pCreateInputData,
    PipelineCreateOutputData* pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    /// @todo (CAMX-1797) Upcomming CHI override changes will change this
    ///                   logic as it should loop through valid formats
    ///                   defined in the topology xml and re-walk
    CamxResult result = CamxResultSuccess;

    for (UINT i = 0; i < m_nodeCount; i++)
    {
        if (NULL != m_ppNodes[i])
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                             "ResetBufferNegotiationData node %d instance %d i %d",
                             m_ppNodes[i]->Type(),
                             m_ppNodes[i]->InstanceID(),
                             i);

            m_ppNodes[i]->ResetBufferNegotiationData();
        }
    }

    for (UINT nodeIdx = 0; nodeIdx < m_nodesSinkOutPorts.numNodes; nodeIdx++)
    {
        CAMX_ASSERT((NULL != m_ppNodes) && (NULL != m_ppNodes[m_nodesSinkOutPorts.nodeIndices[nodeIdx]]));

        Node* pNode = m_ppNodes[m_nodesSinkOutPorts.nodeIndices[nodeIdx]];
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node::%S calling Switch OP Format to NV12",
                         pNode->NodeIdentifierString());
        pNode->SwitchNodeOutputFormat(Format::YUV420NV12);
    }

    for (UINT index = 0; index < m_orderedNodeCount; index++)
    {
        m_ppOrderedNodes[index] = NULL;
    }

    m_orderedNodeCount = 0;

    for (UINT nodeIndex = 0; nodeIndex < m_nodesSinkOutPorts.numNodes; nodeIndex++)
    {
        CAMX_ASSERT((NULL != m_ppNodes) && (NULL != m_ppNodes[m_nodesSinkOutPorts.nodeIndices[nodeIndex]]));

        Node* pNode = m_ppNodes[m_nodesSinkOutPorts.nodeIndices[nodeIndex]];

        result = pNode->TriggerBufferNegotiation();

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline::%s Unable to satisfy node input buffer requirements",
                           GetPipelineIdentifierString());
            break;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::SetNumBatchedFrames
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SetNumBatchedFrames(
    UINT usecaseNumBatchedFrames,
    UINT usecaseFPSValue)
{
    // NumBatchedFrames of 1 essentially means batching is disabled for the usecase
    UINT fpsValue           = usecaseFPSValue;
    m_numMaxBatchedFrames   = usecaseNumBatchedFrames;

    // set it to default values to handle if m_usecaseNumBatchedFrames is 0
    if (m_numMaxBatchedFrames == 0)
    {
        m_numMaxBatchedFrames = 1;
    }

    // set it to default values to handle if m_usecaseNumBatchedFrames is 0
    if (fpsValue == 0)
    {
        fpsValue = 30;
    }

    MetadataSlot*        pSlot = m_pUsecasePool->GetSlot(0);

    pSlot->SetMetadataByTag(PropertyIDUsecaseBatch, &m_numMaxBatchedFrames, 1, GetPipelineIdentifierString());
    pSlot->PublishMetadata(PropertyIDUsecaseBatch);

    pSlot->SetMetadataByTag(PropertyIDUsecaseFPS, &fpsValue, 1, GetPipelineIdentifierString());
    pSlot->PublishMetadata(PropertyIDUsecaseFPS);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::SetLCRRawformatPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SetLCRRawformatPorts()
{
    BOOL                  rawFound           = FALSE;
    MetadataSlot*         pPerUsecaseSlot    = m_pUsecasePool->GetSlot(0);
    CHIRAWFORMATPORT      pPublishrawport;
    UINT                  numRawPorts        = 0;

    const PerPipelineInfo pPipelineInfo      = m_pPipelineDescriptor->pipelineInfo;

    // Find the IFE sinkport RAW format shared with AF node which is required for sensor node Pdlib initilization
    for (UINT numNodes = 0; numNodes < pPipelineInfo.numNodes; numNodes++)
    {
        if (AutoFocus == pPipelineInfo.pNodeInfo[numNodes].nodeId)
        {
            PerNodeInfo pChildNodeInfo = pPipelineInfo.pNodeInfo[numNodes];
            for (UINT portid = 0 ; portid < pChildNodeInfo.inputPorts.numPorts; portid++)
            {
                if (IFE == pPipelineInfo.pNodeInfo[pChildNodeInfo.inputPorts.pPortInfo[portid].parentNodeIndex].nodeId)
                {
                    PerNodeInfo pNodeInfo      =
                        pPipelineInfo.pNodeInfo[pChildNodeInfo.inputPorts.pPortInfo[portid].parentNodeIndex];
                    UINT parentPort            = pChildNodeInfo.inputPorts.pPortInfo[portid].parentOutputPortId;
                    for (UINT portnum = 0 ; portnum < pNodeInfo.outputPorts.numPorts; portnum++)
                    {
                        if ((parentPort == pNodeInfo.outputPorts.pPortInfo[portnum].portId) &&
                            (TRUE == pNodeInfo.outputPorts.pPortInfo[portnum].flags.isSinkBuffer))
                        {
                            ChiStreamWrapper* pChiStreamWrapper =
                            GetOutputStreamWrapper(pNodeInfo.nodeId,
                            pNodeInfo.instanceId, pNodeInfo.outputPorts.pPortInfo[portnum].portId);

                            if (NULL != pChiStreamWrapper)
                            {
                                switch (pChiStreamWrapper->GetInternalFormat())
                                {
                                    case Format::RawYUV8BIT:
                                    case Format::RawPrivate:
                                    case Format::RawMIPI:
                                    case Format::RawPlain16:
                                    case Format::RawMeta8BIT:
                                    case Format::RawMIPI8:
                                    case Format::RawPlain64:
                                    {
                                        pPublishrawport.format =
                                        static_cast<UINT>(pChiStreamWrapper->GetInternalFormat());
                                        pPublishrawport.sinkPortId= pNodeInfo.outputPorts.pPortInfo[portnum].portId;
                                        pPublishrawport.inputPortId = pChildNodeInfo.inputPorts.pPortInfo[portid].portId;
                                        CAMX_LOG_INFO(CamxLogGroupCore, "%s Raw format saved as %d AF port %d IFEport %d",
                                            GetPipelineName(), pPublishrawport.format,
                                            pPublishrawport.inputPortId,
                                            pPublishrawport.sinkPortId);
                                        rawFound = TRUE;
                                    }
                                    break;
                                    default:
                                        CAMX_LOG_INFO(CamxLogGroupCore, "FORMAT %d IFEport %d",
                                            static_cast<UINT>(pChiStreamWrapper->GetInternalFormat()),
                                            pNodeInfo.outputPorts.pPortInfo[portid].portId);
                                        break;
                                }

                                if (TRUE == rawFound)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (TRUE == rawFound)
    {
        static UINT32 m_vendorTagpipelineType;
        VendorTagManager::QueryVendorTagLocation("org.quic.camera.LCRRawformat",
            "LCRRawformat", &m_vendorTagpipelineType);
        pPerUsecaseSlot->SetMetadataByTag(m_vendorTagpipelineType, static_cast<VOID*>(&pPublishrawport),
            sizeof(CHIRAWFORMATPORT), GetPipelineIdentifierString());
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::ConfigureMaxPipelineDelay
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::ConfigureMaxPipelineDelay(
    UINT usecaseFPSValue,
    UINT maxPipelineDelay)
{
    m_maxPipelineDelay = maxPipelineDelay;
    if (usecaseFPSValue > DefaultFPS)
    {
        m_maxPipelineDelay++;
    }

    CAMX_LOG_INFO(CamxLogGroupCore,
        "FPS:%d StaticSettingPipelinedelay:%d FinalPipelineDelay:%d",
        usecaseFPSValue,
        maxPipelineDelay,
        m_maxPipelineDelay);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::GetFPSValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Pipeline::GetFPSValue()
{
    MetadataSlot*        pSlot           = m_pUsecasePool->GetSlot(0);
    const UINT           defaultFpsValue = 30;
    UINT*                pFpsValue;

    pFpsValue = static_cast<UINT*>(pSlot->GetMetadataByTag(PropertyIDUsecaseFPS));

    return pFpsValue ? *pFpsValue : defaultFpsValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::FinalizeSensorModeInitalization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::FinalizeSensorModeInitalization(
    const ImageSensorModuleData* pSensorModuleData)
{
    MetadataSlot*           pPerUsecaseSlot = m_pUsecasePool->GetSlot(0);
    UINT16                  cameraId        = 0;
    UINT32                  CSIPHYSlotInfo  = 0;
    UINT32                  laneAssign      = 0;
    BOOL                    comboMode       = FALSE;
    UsecaseSensorModes      sensorModeData;

    // If the pool creation failed we will *never* come here
    CAMX_ASSERT(NULL != pPerUsecaseSlot);

    if (CamxResultSuccess == pSensorModuleData->GetCameraId(&cameraId))
    {
        // For external sensor, upper 8 bit store the CSIPHY slot info.
        CSIPHYSlotInfo = (cameraId & 0xFF00) >> 8;
        comboMode      = pSensorModuleData->GetCSIInfo()->isComboMode;
        laneAssign     = pSensorModuleData->GetCSIInfo()->laneAssign;

        CAMX_LOG_INFO(CamxLogGroupCore,
                      "Sensor Module data cameraId:%d CSIPHYSlotInfo:%d comboMode:%d",
                      cameraId,
                      CSIPHYSlotInfo,
                      comboMode);

        pSensorModuleData->GetSensorDataObject()->PopulateSensorModeData(&sensorModeData,
                                                                         CSIPHYSlotInfo,
                                                                         laneAssign,
                                                                         comboMode);

        pPerUsecaseSlot->SetMetadataByTag(PropertyIDUsecaseSensorModes, &sensorModeData, 1, GetPipelineIdentifierString());
        pPerUsecaseSlot->PublishMetadata(PropertyIDUsecaseSensorModes);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to get CSIPHYSlotInfo from sensor driver binary");
    }

    pPerUsecaseSlot->SetMetadataByTag(PropertyIDUsecaseSensorCurrentMode,
                                      &m_currentSensorMode,
                                      1,
                                      GetPipelineIdentifierString());

    PublishSensorModeInformation(FirstValidRequestId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::PublishOutputDimensions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::PublishOutputDimensions()
{
    MetadataSlot*       pPerUsecaseSlot     = m_pUsecasePool->GetSlot(0);
    Camera3Stream*      pStream             = NULL;
    UINT32              previewMetaTag      = 0;
    UINT32              videoMetaTag        = 0;
    UINT32              snapshotMetaTag     = 0;
    UINT32              depthMetaTag        = 0;
    CamxResult          result              = CamxResultSuccess;
    ChiBufferDimension  previewDimension    = {0};

    PropertyPipelineOutputDimensions pipelineOutputInfo;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.streamDimension",
                                                      "preview",
                                                      &previewMetaTag);
    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag: org.quic.camera.streamDimension.preview");

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.streamDimension",
                                                      "video",
                                                      &videoMetaTag);
    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag: org.quic.camera.streamDimension.video");

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.streamDimension",
                                                      "snapshot",
                                                      &snapshotMetaTag);
    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag: org.quic.camera.streamDimension.snapshot");

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.streamDimension",
                                                      "depth",
                                                      &depthMetaTag);
    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag: org.quic.camera.streamDimension.depth");

    // If the pool creation failed we will *never* come here
    CAMX_ASSERT(NULL != m_pPipelineDescriptor);

    pipelineOutputInfo.numberOutputs = m_pPipelineDescriptor->numOutputs;

    for (UINT i = 0; i < m_pPipelineDescriptor->numOutputs; i++)
    {
        ChiStreamWrapper* pStreamWrapper = m_pPipelineDescriptor->outputData[i].pOutputStreamWrapper;
        pStream = pStreamWrapper->GetNativeStream();
        CAMX_ASSERT(NULL != pStream);

        pipelineOutputInfo.dimensions[i].outputDimension.width      = pStream->width;
        pipelineOutputInfo.dimensions[i].outputDimension.height     = pStream->height;

        if (pStreamWrapper->IsPreviewStream() &&
            ((pStream->width > previewDimension.width)|| (pStream->height > previewDimension.height)))
        {
            pipelineOutputInfo.dimensions[i].outputType = ChiPipelineOutputPreview;
            pPerUsecaseSlot->SetMetadataByTag(previewMetaTag,
                                              static_cast<VOID*>(&pipelineOutputInfo.dimensions[i].outputDimension),
                                              sizeof(ChiBufferDimension),
                                              GetPipelineIdentifierString());
            pPerUsecaseSlot->PublishMetadataList(&previewMetaTag, 1);
            previewDimension.width = pStream->width;
            previewDimension.height= pStream->height;
        }
        else if (pStreamWrapper->IsVideoStream())
        {
            pipelineOutputInfo.dimensions[i].outputType = ChiPipelineOutputVideo;
            pPerUsecaseSlot->SetMetadataByTag(videoMetaTag,
                                              static_cast<VOID*>(&pipelineOutputInfo.dimensions[i].outputDimension),
                                              sizeof(ChiBufferDimension),
                                              GetPipelineIdentifierString());
            pPerUsecaseSlot->PublishMetadataList(&videoMetaTag, 1);
        }
        else if (pStreamWrapper->IsSnapshotStream())
        {
            pipelineOutputInfo.dimensions[i].outputType = ChiPipelineOutputSnapshot;
            pPerUsecaseSlot->SetMetadataByTag(snapshotMetaTag,
                                              static_cast<VOID*>(&pipelineOutputInfo.dimensions[i].outputDimension),
                                              sizeof(ChiBufferDimension),
                                              GetPipelineIdentifierString());
            pPerUsecaseSlot->PublishMetadataList(&snapshotMetaTag, 1);
        }
        else if (pStreamWrapper->IsDepthStream())
        {
            pipelineOutputInfo.dimensions[i].outputType = ChiPipelineOutputDepth;
            pPerUsecaseSlot->SetMetadataByTag(depthMetaTag,
                                              static_cast<VOID*>(&pipelineOutputInfo.dimensions[i].outputDimension),
                                              sizeof(ChiBufferDimension),
                                              GetPipelineIdentifierString());
            pPerUsecaseSlot->PublishMetadataList(&depthMetaTag, 1);
        }
        else
        {
            pipelineOutputInfo.dimensions[i].outputType = ChiPipelineOutputDefault;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "%s: Publish w %d h %d type %d, stream type %d gralloc usage 0x%x",
                         GetPipelineIdentifierString(),
                         pipelineOutputInfo.dimensions[i].outputDimension.width,
                         pipelineOutputInfo.dimensions[i].outputDimension.height,
                         pipelineOutputInfo.dimensions[i].outputType,
                         pStream->streamType,
                         pStream->grallocUsage);
    }

    pPerUsecaseSlot->SetMetadataByTag(PropertyIDUsecasePipelineOutputDimensions,
        &pipelineOutputInfo,
        1,
        GetPipelineIdentifierString());
    pPerUsecaseSlot->PublishMetadata(PropertyIDUsecasePipelineOutputDimensions);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::PublishTargetFPS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::PublishTargetFPS()
{
    MetadataPool*           pPerUsecasePool  = m_pUsecasePool;
    MetadataSlot*           pPerUsecaseSlot  = NULL;
    UsecasePropertyBlob*    pPerUsecaseBlob  = NULL;
    UINT32                  targetFPSMetaTag = 0;
    CamxResult              result           = CamxResultSuccess;
    UINT32                  targetFPS        = 0;
    BOOL                    publishTargetFPS = FALSE;

    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sensor_meta_data",
                                                      "targetFPS",
                                                      &targetFPSMetaTag);
    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result,
                        "Failed to get vendor tag: org.codeaurora.qcamera3.sensor_meta_data.targetFPS");

    // If the pool creation failed we will *never* come here
    CAMX_ASSERT(NULL != pPerUsecasePool);
    CAMX_ASSERT(NULL != m_pPipelineDescriptor);

    pPerUsecaseSlot = pPerUsecasePool->GetSlot(0);
    CAMX_ASSERT(NULL != pPerUsecaseSlot);

    if (FALSE == IsRealTime())
    {
        for (UINT i = 0; i < m_pPipelineDescriptor->numOutputs; i++)
        {
            ChiStreamWrapper* pStreamWrapper = m_pPipelineDescriptor->outputData[i].pOutputStreamWrapper;
            if ((TRUE == pStreamWrapper->IsPreviewStream()) || (TRUE == pStreamWrapper->IsVideoStream()))
            {
                publishTargetFPS = TRUE;
                break;
            }
        }
    }

    if ((TRUE == IsRealTime()) || (TRUE == publishTargetFPS))
    {
        targetFPS = m_pPipelineDescriptor->maxFPSValue;

        pPerUsecaseSlot->SetMetadataByTag(targetFPSMetaTag, static_cast<VOID*>(&targetFPS), 1, GetPipelineIdentifierString());
        pPerUsecaseSlot->PublishMetadataList(&targetFPSMetaTag, 1);

        CAMX_LOG_INFO(CamxLogGroupCore, "%s: Publishing target FPS %u", GetPipelineIdentifierString(), targetFPS);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::PublishSensorModeInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::PublishSensorModeInformation(
    UINT64 requestId)
{
    MainPropertyBlob*  pMainPropertyBlob = NULL;
    MetadataSlot*      pMetadataSlot     = m_pMainPool->GetSlot(requestId);

    pMetadataSlot->SetMetadataByTag(PropertyIDSensorCurrentMode, &m_currentSensorMode, 1, GetPipelineIdentifierString());
    pMetadataSlot->PublishMetadata(PropertyIDSensorCurrentMode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::PublishSensorUsecaseProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::PublishSensorUsecaseProperties(
    const ImageSensorModuleData* pSensorModuleData)
{
    // publish PropertyIDUsecaseSensorModes
    FinalizeSensorModeInitalization(pSensorModuleData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::DumpState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::DumpState(
    INT     fd,
    UINT32  indent)
{
    static const CHAR* SensorSyncModStrings[] = { "NoSync", "MasterMode", "SlaveMode" };
    const CHAR*  piplineStatusString          = PipelineStatusStrings[static_cast<UINT>(m_currentPipelineStatus)];

    CAMX_LOG_TO_FILE(fd, indent, "+ Pipeline Name: %s, Pipeline ID: %d, %p, CurrentRequestId: %llu",
                  GetPipelineIdentifierString(), GetPipelineId(), this,
                  ((NULL == m_pSession) ? 0 : m_pSession->GetCurrentPipelineRequestId(m_pipelineIndex)));
    CAMX_LOG_TO_FILE(fd, indent, "  -------------------------------------------------------------------------------------");
    CAMX_LOG_TO_FILE(fd, indent, "+ index                         = %u",     m_pipelineIndex);
    CAMX_LOG_TO_FILE(fd, indent, "+ status                        = \"%s\"", piplineStatusString);
    CAMX_LOG_TO_FILE(fd, indent, "+ currentSensorMode             = %d",     m_currentSensorMode);
    CAMX_LOG_TO_FILE(fd, indent, "+ cameraId                      = %d",     m_cameraId);
    CAMX_LOG_TO_FILE(fd, indent, "+ sensorSyncMode                = \"%s\"", SensorSyncModStrings[m_sensorSyncMode]);
    CAMX_LOG_TO_FILE(fd, indent, "+ pipelineFlags                 = 0x%08X", m_flags.allFlagsValue);
    CAMX_LOG_TO_FILE(fd, indent, "+ hCSLLinkHandle                = %d",     m_hCSLLinkHandle);
    CAMX_LOG_TO_FILE(fd, indent, "+ hCSLSyncLinkHandle            = %d",     m_hCSLSyncLinkHandle);
    CAMX_LOG_TO_FILE(fd, indent, "+ nodeCount                     = %u",     m_nodeCount);
    CAMX_LOG_TO_FILE(fd, indent, "+ maxPipelineDelay              = %u",     GetMaxPipelineDelay());
    CAMX_LOG_TO_FILE(fd, indent, "+ lastRequestActiveStreamIdMask = 0x%08X", m_lastRequestActiveStreamIdMask);
    CAMX_LOG_TO_FILE(fd, indent, "+ lastInOrderCompletedRequestId = %llu",   m_lastInOrderCompletedRequestId);
    CAMX_LOG_TO_FILE(fd, indent, "----------------------------------------------------------------------");
    CAMX_LOG_TO_FILE(fd, indent, "+  Pending Nodes:");
    CAMX_LOG_TO_FILE(fd, indent, "----------------------------------------------------------------------");
    CAMX_LOG_TO_FILE(fd, indent + 2, "+ Requests Range - lastInOrderCompletedRequestId: %llu currentRequest: %llu",
                                  m_lastInOrderCompletedRequestId,
                                  m_lastSubmittedRequestId);
    CAMX_LOG_TO_FILE(fd, indent, "  ---------------");

    CHAR pendingNodeString[MaxStringLength256] = { 0 };
    CHAR nodeString[MaxStringLength64]         = { 0 };

    for (UINT64 requestId = (m_lastInOrderCompletedRequestId);
        requestId <= m_lastSubmittedRequestId;
        requestId++)
    {
        Utils::Memset(pendingNodeString, 0, sizeof(CHAR) * MaxStringLength256);
        Utils::Memset(nodeString, 0, sizeof(CHAR) * MaxStringLength64);

        for (UINT index = 0; index < m_nodeCount; index++)
        {
            Utils::Memset(nodeString, 0, MaxStringLength64);

            PerRequestNodeStatus requestStatus = m_ppNodes[index]->GetRequestStatus(requestId);
            if (PerRequestNodeStatus::Setup == requestStatus || PerRequestNodeStatus::Deferred == requestStatus ||
                PerRequestNodeStatus::Running == requestStatus)
            {
                OsUtils::SNPrintF(nodeString, MaxStringLength64, "%s%u ", m_ppNodes[index]->Name(),
                    m_ppNodes[index]->InstanceID());

                OsUtils::StrLCat(pendingNodeString, nodeString, sizeof(pendingNodeString));
            }
        }

        CAMX_LOG_TO_FILE(fd, indent + 4, "RequestId = %llu, Nodes: %s", requestId, pendingNodeString);
    }


    PerRequestInfo* pMinRequestInfo    = &(m_perRequestInfo[m_lastInOrderCompletedRequestId % MaxPerRequestInfo]);
    UINT64          minLoggedRequestId = (GetMaxPipelineDelay() < pMinRequestInfo->request.requestId)?
                                                   pMinRequestInfo->request.requestId - GetMaxPipelineDelay(): 0;

    CAMX_LOG_TO_FILE(fd, indent, "----------------------------------------------------------------------");
    CAMX_LOG_TO_FILE(fd, indent, "+ Request info for the pending nodes:");
    CAMX_LOG_TO_FILE(fd, indent, "----------------------------------------------------------------------");
    for (UINT i = 0; i < MaxPerRequestInfo; i++)
    {
        if ((CamxInvalidRequestId != m_perRequestInfo[i].request.requestId)     &&
            (minLoggedRequestId <= m_perRequestInfo[i].request.requestId)       &&
            (m_lastSubmittedRequestId >= m_perRequestInfo[i].request.requestId) &&
            (m_perRequestInfo[i].numNodesRequestIdDone != m_nodeCount))
        {
            CAMX_LOG_TO_FILE(fd, indent, "");
            CAMX_LOG_TO_FILE(fd, indent, "=========================");
            CAMX_LOG_TO_FILE(fd, indent, "||   REQUEST ID = %llu   ||", m_perRequestInfo[i].request.requestId);
            CAMX_LOG_TO_FILE(fd, indent, "=========================");
            for (UINT j = 0; j < m_perRequestInfo[i].request.numBatchedFrames; j++)
            {
                CAMX_LOG_TO_FILE(fd, indent + 4, "SequenceId[%u]                 = %u", j, m_perRequestInfo[i].pSequenceId[j]);
            }
            CAMX_LOG_TO_FILE(fd, indent + 4, "CSLSyncId                     = %u", m_perRequestInfo[i].request.CSLSyncID);
            CAMX_LOG_TO_FILE(fd, indent + 4, "numNodes                      = %u", m_nodeCount);
            CAMX_LOG_TO_FILE(fd, indent + 4, "numNodesRequestIdDone         = %u", m_perRequestInfo[i].numNodesRequestIdDone);
            CAMX_LOG_TO_FILE(fd, indent + 4, "numNodesPartialMetadataDone   = %u",
                             m_perRequestInfo[i].numNodesPartialMetadataDone);
            CAMX_LOG_TO_FILE(fd, indent + 4, "numNodesMetadataDone          = %u", m_perRequestInfo[i].numNodesMetadataDone);
            CAMX_LOG_TO_FILE(fd, indent + 4, "numNodesConfigDone            = %u", m_perRequestInfo[i].numNodesConfigDone);
            CAMX_LOG_TO_FILE(fd, indent + 4, "batchFrameIntervalNanoSeconds = %llu",
                             m_perRequestInfo[i].batchFrameIntervalNanoSeconds);
            CAMX_LOG_TO_FILE(fd, indent + 4, "numBatchedFrames              = %u",
                             m_perRequestInfo[i].request.numBatchedFrames);
            CAMX_LOG_TO_FILE(fd, indent + 4, "isSofDispatched               = %u", m_perRequestInfo[i].isSofDispatched);

            for (UINT j = 0; j < m_nodeCount; j++)
            {
                m_ppNodes[j]->DumpState(fd, indent + 4, m_perRequestInfo[i].request.requestId);
            }
        }
    }

    CAMX_LOG_TO_FILE(fd, indent, "+------------------------------------------------------------------+");
    CAMX_LOG_TO_FILE(fd, indent, "+ Node Info:");
    CAMX_LOG_TO_FILE(fd, indent, "+------------------------------------------------------------------+");
    for (UINT i = 0; i < m_nodeCount; i++)
    {
        m_ppNodes[i]->DoDumpNodeInfo(fd, indent + 2);
    }

    CAMX_LOG_TO_FILE(fd, indent, "+------------------------------------------------------------------+");
    CAMX_LOG_TO_FILE(fd, indent, "+ Fence Error Info:");
    CAMX_LOG_TO_FILE(fd, indent, "+------------------------------------------------------------------+");
    for (UINT j = 0; j < m_nodeCount; j++)
    {
        m_ppNodes[j]->DumpFenceErrors(fd, indent + 2);
    }

    CAMX_LOG_TO_FILE(fd, indent, "+------------------------------------------------------------------+");
    CAMX_LOG_TO_FILE(fd, indent, "+ Graph:");
    CAMX_LOG_TO_FILE(fd, indent, "+------------------------------------------------------------------+");

    CAMX_LOG_TO_FILE(fd, indent+ 2, "Nodes + ports:");
    for (UINT i = 0; i < m_nodeCount; i++)
    {
        m_ppNodes[i]->DumpNodeInfo(fd, indent + 4);
    }

    CAMX_LOG_TO_FILE(fd, indent, "+------------------------------------------------------------------+");
    CAMX_LOG_TO_FILE(fd, indent, "Links:");
    CAMX_LOG_TO_FILE(fd, indent, "+------------------------------------------------------------------+");
    for (UINT i = 0; i < m_nodeCount; i++)
    {
        m_ppNodes[i]->DumpLinkInfo(fd, indent + 2);
    }

    CAMX_LOG_TO_FILE(fd, indent, "+------------------------------------------------------------------+");

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::DumpDebugInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::DumpDebugInfo()
{
    CAMX_LOG_DUMP(CamxLogGroupCore, "+----------------------------------------------------------+");
    CAMX_LOG_DUMP(CamxLogGroupCore, "+    PIPELINE DEBUG INFO     +");
    CAMX_LOG_DUMP(CamxLogGroupCore, "+  Pipeline Name: %s, Pipeline ID: %d, %p, CurrentRequestId: %llu",
                    GetPipelineIdentifierString(),
                    GetPipelineId(),
                    this,
                    ((NULL == m_pSession) ? 0 : m_pSession->GetCurrentPipelineRequestId(m_pipelineIndex)));

    if (NULL == m_pSession)
    {
        CAMX_LOG_DUMP(CamxLogGroupCore, "+   WARNING: could not access CurrentRequestId for pipeline, CurrentRequestId"
                       " dumped as 0");
    }
    if (GetPipelineStatus() == PipelineStatus::STREAM_ON)
    {
        LogPendingNodes();
    }

    CAMX_LOG_DUMP(CamxLogGroupCore, "+------------------------------------------------------------------+");
    CAMX_LOG_DUMP(CamxLogGroupCore, "+ Request info for the pending nodes");

    UINT    index;
    for (UINT64 requestId = (m_lastInOrderCompletedRequestId + 1);
        requestId <= m_lastSubmittedRequestId;
        requestId++)
    {
        index    = (requestId % MaxPerRequestInfo);
        if (0 != m_perRequestInfo[index].request.requestId)
        {
            CAMX_LOG_DUMP(CamxLogGroupCore, "+   RequestId:       %lld", m_perRequestInfo[index].request.requestId);

            for (UINT j = 0; j < m_perRequestInfo[index].request.numBatchedFrames; j++)
            {
                CAMX_LOG_DUMP(CamxLogGroupCore, "+    SequenceId[%d]:  %d", j, m_perRequestInfo[index].pSequenceId[j]);
            }
            CAMX_LOG_DUMP(CamxLogGroupCore, "+     CSLSyncId:                       %u",
                          m_perRequestInfo[index].request.CSLSyncID);
            CAMX_LOG_DUMP(CamxLogGroupCore, "+     numNodes:                        %d",
                          m_nodeCount);
            CAMX_LOG_DUMP(CamxLogGroupCore, "+     numNodesRequestIdDone:           %d",
                          m_perRequestInfo[index].numNodesRequestIdDone);
            CAMX_LOG_DUMP(CamxLogGroupCore, "+     numNodesPartialMetadataDone:     %d",
                             m_perRequestInfo[index].numNodesPartialMetadataDone);
            CAMX_LOG_DUMP(CamxLogGroupCore, "+     numNodesMetadataDone:            %d",
                          m_perRequestInfo[index].numNodesMetadataDone);
            CAMX_LOG_DUMP(CamxLogGroupCore, "+     numNodesConfigDone:              %d",
                          m_perRequestInfo[index].numNodesConfigDone);
            CAMX_LOG_DUMP(CamxLogGroupCore, "+     batchFrameIntervalNanoSeconds:   %lld",
                m_perRequestInfo[index].batchFrameIntervalNanoSeconds);
            CAMX_LOG_DUMP(CamxLogGroupCore, "+     isSofdispatched:                 %d",
                          m_perRequestInfo[index].isSofDispatched);
        }
    }

    CAMX_LOG_DUMP(CamxLogGroupCore, "+------------------------------------------------------------------+");
    CAMX_LOG_DUMP(CamxLogGroupCore, "+      NODE DEBUG INFO       +");
    // print the pending nodes
    for (UINT i = 0; i < m_nodeCount; i++)
    {
        if (NULL != m_ppNodes[i])
        {
            m_ppNodes[i]->DumpDebugInfo();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::DetermineExtrabuffersNeeded
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Pipeline::DetermineExtrabuffersNeeded()
{
    UINT32        metaTag                = 0;
    CamxResult    result                 = CamxResultSuccess;
    UINT32*       pExtraFrameworkBuffers = NULL;
    MetadataPool* pUsecasePool           = NULL;
    MetadataSlot* pSlot                  = NULL;
    UINT32        extraFrameworkBuffers  = 0;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs", "ExtraFrameworkBuffers", &metaTag);
    if (CamxResultSuccess == result)
    {
        pUsecasePool = GetPerFramePool(PoolType::PerUsecase);
        CAMX_ASSERT(NULL != pUsecasePool);

        pSlot = pUsecasePool->GetSlot(0);
        CAMX_ASSERT(NULL != pSlot);

        pExtraFrameworkBuffers = static_cast<UINT32*>(pSlot->GetMetadataByTag(metaTag));
        if (NULL != pExtraFrameworkBuffers)
        {
            extraFrameworkBuffers = *pExtraFrameworkBuffers;
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Extra framework buffers required %u for EIS", extraFrameworkBuffers);
        }
    }

    return extraFrameworkBuffers;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::DetermineFrameDelay
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Pipeline::DetermineFrameDelay()
{
    UINT32        metaTag      = 0;
    CamxResult    result       = CamxResultSuccess;
    UINT32*       pFrameDelay  = NULL;
    MetadataPool* pUsecasePool = NULL;
    MetadataSlot* pSlot        = NULL;
    UINT32        frameDelay   = 0;
    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eislookahead", "FrameDelay", &metaTag);
    if (CamxResultSuccess == result)
    {
        pUsecasePool = GetPerFramePool(PoolType::PerUsecase);
        CAMX_ASSERT(NULL != pUsecasePool);

        pSlot = pUsecasePool->GetSlot(0);
        CAMX_ASSERT(NULL != pSlot);

        pFrameDelay = static_cast<UINT32*>(pSlot->GetMetadataByTag(metaTag));
        if (NULL != pFrameDelay)
        {
            frameDelay = *pFrameDelay;
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "%s Pipeline frame delay is %u for EIS set",
                             GetPipelineIdentifierString(), frameDelay);
        }
    }

    return frameDelay;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::IsEISEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pipeline::IsEISEnabled()
{
    UINT32        metaTag      = 0;
    CamxResult    result       = CamxResultSuccess;
    BOOL*         pEISEnabled  = NULL;
    MetadataPool* pUsecasePool = NULL;
    MetadataSlot* pSlot        = NULL;
    BOOL          bEISEnabled  = FALSE;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime", "Enabled", &metaTag);
    if (CamxResultSuccess == result)
    {
        pUsecasePool = GetPerFramePool(PoolType::PerUsecase);
        CAMX_ASSERT(NULL != pUsecasePool);

        pSlot = pUsecasePool->GetSlot(0);
        CAMX_ASSERT(NULL != pSlot);

        pEISEnabled = static_cast<BOOL*>(pSlot->GetMetadataByTag(metaTag));
        if (NULL != pEISEnabled)
        {
            bEISEnabled = *pEISEnabled;
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "%s EIS enabled flag is %d for EIS set",
                GetPipelineIdentifierString(), bEISEnabled);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to get tag location of eisrealtime.Enabled (%d)", result);
    }

    return bEISEnabled;
}

 ;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::DetermineEISMiniamalTotalMargin
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::DetermineEISMiniamalTotalMargin(
    MarginRequest* pMargin)
{
    UINT32          metaTag             = 0;
    CamxResult      result              = CamxResultSuccess;
    MetadataPool*   pUsecasePool        = NULL;
    MetadataSlot*   pSlot               = NULL;
    MarginRequest*  pMinialTotalMargin  = NULL;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime", "MinimalTotalMargins", &metaTag);
    if (CamxResultSuccess == result)
    {
        pUsecasePool = GetPerFramePool(PoolType::PerUsecase);
        CAMX_ASSERT(NULL != pUsecasePool);

        pSlot = pUsecasePool->GetSlot(0);
        CAMX_ASSERT(NULL != pSlot);

        pMinialTotalMargin = static_cast<MarginRequest*>(pSlot->GetMetadataByTag(metaTag));
        if (NULL != pMinialTotalMargin)
        {
            *pMargin = *pMinialTotalMargin;
            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                            "%s EIS minimal total margin(%fx%f)",
                            GetPipelineIdentifierString(),
                            pMinialTotalMargin->widthMargin,
                            pMinialTotalMargin->heightMargin);
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::GetRequestQueueDepth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Pipeline::GetRequestQueueDepth()
{
    UINT32 requestQueueDepth = DefaultRequestQueueDepth + DetermineFrameDelay();

    if (RequestQueueDepth < requestQueueDepth)
    {
        CAMX_LOG_WARN(CamxLogGroupCore,
                      "Current Request Queue Depth %d is larger than our max %d, capping depth to the max",
                      requestQueueDepth,
                      RequestQueueDepth);
        requestQueueDepth = RequestQueueDepth;
    }

    return requestQueueDepth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::FlushMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::FlushMetadata()
{
    m_pInputPool->Flush(m_flushInfo.lastValidRequestIdbeforeFlush, m_lastCompletedRequestId);
    m_pMainPool->Flush(m_flushInfo.lastValidRequestIdbeforeFlush, m_lastCompletedRequestId);
    m_pInternalPool->Flush(m_flushInfo.lastValidRequestIdbeforeFlush, m_lastCompletedRequestId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::FilterAndUpdatePublishSet
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::FilterAndUpdatePublishSet(
    Node* pNode)
{
    const UINT32* pPublishedTagArray = NULL;
    UINT32        publishedTagCount  = 0;
    UINT32        partialPublishedTagCount  = 0;

    CamxResult result = pNode->GetMetadataPublishList(&pPublishedTagArray, &publishedTagCount, &partialPublishedTagCount);

    if ((CamxResultSuccess == result) && (NULL != pPublishedTagArray) && (0 < publishedTagCount))
    {
        for (UINT32 index = 0; index < publishedTagCount; ++index)
        {
            switch (pPublishedTagArray[index] & PropertyGroupMask)
            {
                case UsecaseMetadataSectionMask:
                case PropertyIDPerFrameInternalBegin:
                case PropertyIDUsecaseBegin:
                    // skip usecase and internal tags
                    continue;
                default:
                    break;
            }

            // skip PSM tag entry initialization if it is already publishing
            if (TRUE == pNode->IsPSMEnabled(pPublishedTagArray[index]))
            {
                continue;
            }

            if (partialPublishedTagCount > index)
            {
                m_nodePartialPublishSet.insert(pPublishedTagArray[index]);
            }

            m_nodePublishSet.insert(pPublishedTagArray[index]);

            // update port specific metadata map
            auto plmVector = m_psMetaMap.find(pPublishedTagArray[index]);

            if (plmVector != m_psMetaMap.end())
            {
                if (LinkMetadataCount > m_portLinkPropertyCount)
                {
                    result = pNode->InitializePSMetadata(pPublishedTagArray[index]);

                    if (CamxResultSuccess == result)
                    {
                        UINT propertyId = PropertyIDLinkMetadata0 + m_portLinkPropertyCount;
                        plmVector->second.push_back({ propertyId, pNode });

                        CAMX_LOG_VERBOSE(CamxLogGroupMeta,
                            "[PSM_INFO] Assigned property %x to vendor tag %x",
                            propertyId, pPublishedTagArray[index]);

                        m_portLinkPropertyCount++;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::UpdatePublishTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::UpdatePublishTags()
{
    CamxResult result             = CamxResultSuccess;
    UINT32     tagCount           = 0;
    UINT32     numMetadataTags    = CAMX_ARRAY_SIZE(PipelineMetadataTags);
    UINT32     numVendorTags      = CAMX_ARRAY_SIZE(PipelineVendorTags);
    UINT32     tagID;

    if (numMetadataTags + numVendorTags < MaxTagsPublished)
    {
        for (UINT32 tagIndex = 0; tagIndex < numMetadataTags; ++tagIndex)
        {
            m_nodePublishSet.insert(PipelineMetadataTags[tagIndex]);
        }

        for (UINT32 tagIndex = 0; tagIndex < numVendorTags; ++tagIndex)
        {
            result = VendorTagManager::QueryVendorTagLocation(
                PipelineVendorTags[tagIndex].pSectionName,
                PipelineVendorTags[tagIndex].pTagName,
                &tagID);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot query vendor tag %s %s",
                    PipelineVendorTags[tagIndex].pSectionName,
                    PipelineVendorTags[tagIndex].pTagName);
                break;
            }

            m_nodePublishSet.insert(tagID);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::CallNodeAcquireResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::CallNodeAcquireResources()
{
    CamxResult result = CamxResultSuccess;

    m_pResourceAcquireReleaseLock->Lock();

    // Acquire resources only when current status is either pipeline finalized or resources released state
    if ((PipelineStatus::FINALIZED          == GetPipelineStatus()) ||
        (PipelineStatus::RESOURCES_RELEASED == GetPipelineStatus()) ||
        (PipelineStatus::PARTIAL_STREAM_ON  == GetPipelineStatus()))
    {
        UINT loop;
        for (loop = 0; loop < m_nodeCount; loop++)
        {
            if (NULL != m_ppNodes[loop])
            {
                result = m_ppNodes[loop]->NodeAcquireResources();

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline[%s] Node[%s] Acquire resources fail",
                                   GetPipelineIdentifierString(), m_ppNodes[loop]->NodeIdentifierString());
                    break;
                }
            }
        }

        if (CamxResultSuccess != result)
        {
            while (0 < loop)
            {
                if (NULL != m_ppNodes[loop])
                {
                    m_ppNodes[loop]->NodeReleaseResources(CHIDeactivateModeDefault);
                }
                loop--;
            }
        }
        else
        {
            if (PipelineStatus::PARTIAL_STREAM_ON != GetPipelineStatus())
            {
                SetPipelineStatus(PipelineStatus::RESOURCES_ACQUIRED);
            }
        }
    }

    m_pResourceAcquireReleaseLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::CallNodeReleaseResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::CallNodeReleaseResources(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CamxResult result = CamxResultSuccess;

    m_pResourceAcquireReleaseLock->Lock();

    // Release resources only when current status is either pipeline stream off or resources acquired state
    if ((PipelineStatus::RESOURCES_ACQUIRED == GetPipelineStatus()) ||
        (PipelineStatus::INITIALIZED        == GetPipelineStatus()) ||
        (PipelineStatus::STREAM_OFF         == GetPipelineStatus()) ||
        (PipelineStatus::PARTIAL_STREAM_ON  == GetPipelineStatus()))
    {
        UINT loop;
        for (loop = 0; loop < m_nodeCount; loop++)
        {
            if (NULL != m_ppNodes[loop])
            {
                result = m_ppNodes[loop]->NodeReleaseResources(modeBitmask);

                if (CamxResultSuccess != result)
                {
                    break;
                }
            }
        }

        if ((CamxResultSuccess == result) && !(modeBitmask & CHIDeactivateModeSensorStandby))
        {
            SetPipelineStatus(PipelineStatus::RESOURCES_RELEASED);
        }
    }

    m_pResourceAcquireReleaseLock->Unlock();
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::NotifyNodeRequestIdDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::NotifyNodeRequestIdDone(
    UINT64 requestId)
{
    CamxResult      result            = CamxResultSuccess;
    UINT            perRequestIdIndex = (requestId % MaxPerRequestInfo);
    PerRequestInfo* pPerRequestInfo   = &m_perRequestInfo[perRequestIdIndex];

    if (m_nodeCount == CamxAtomicIncU(&(pPerRequestInfo->numNodesRequestIdDone)))
    {
        auto      hPipeline = m_pPipelineDescriptor;
        UINT      nodeCount = m_nodeCount;
        BINARY_LOG(LogEvent::Pipeline_RequestIdDone, hPipeline, nodeCount, requestId);
        CAMX_LOG_INFO(CamxLogGroupCore, "All nodes in pipeline %s complete with requestId: %llu.",
            GetPipelineName(),
            requestId);

        if (requestId > m_lastCompletedRequestId)
        {
            m_lastCompletedRequestId = requestId;
        }

        // check only if it is in flush state
        if (TRUE == GetFlushStatus())
        {
            BOOL  allNodesDone = TRUE;

            // scan for all pending requests and notify the flush session/pipeline when all the nodes are done
            for (UINT64 reqId = (m_lastInOrderCompletedRequestId + 1);
                reqId <= m_lastSubmittedRequestId;
                reqId++)
            {
                UINT            reqIdIndex  = (reqId % MaxPerRequestInfo);
                PerRequestInfo* pPerReqInfo = &m_perRequestInfo[reqIdIndex];

                if (m_nodeCount > CamxAtomicLoadU(&(pPerReqInfo->numNodesRequestIdDone)))
                {
                    allNodesDone = FALSE;
                    break;
                }
                else
                {
                    m_lastInOrderCompletedRequestId = reqId;
                }
            }

            if (TRUE == allNodesDone)
            {
                m_pSession->NotifyProcessingDone();
            }
        }
        else
        {
            // requests come back in order when not in flush state
            m_lastInOrderCompletedRequestId = m_lastCompletedRequestId;
            m_pSession->NotifyProcessingDone();
        }

        m_pNodesRequestDoneLock->Lock();
        // signal the thread waiting for all node requests done
        if ((TRUE == CamxAtomicLoadU(&m_isRequestDoneSignalNeeded)) && (m_lastCompletedRequestId == m_lastSubmittedRequestId))
        {
            m_pWaitAllNodesRequestDone->Signal();
        }
        m_pNodesRequestDoneLock->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::TriggerRequestError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::TriggerRequestError(
    UINT64 requestId)
{
    ResultsData resultsData = {};

    resultsData.type          = CbType::Error;
    resultsData.pipelineIndex = m_pipelineIndex;

    resultsData.cbPayload.error.code = MessageCodeRequest;

    UINT            perRequestIdIndex = requestId % MaxPerRequestInfo;
    PerRequestInfo* pPerRequestInfo   = &m_perRequestInfo[perRequestIdIndex];

    // No request frame is applicable to the error
    if (CamxInvalidRequestId == requestId)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Cannot handle error for invalid requestId");
    }
    else
    {
        for (UINT batchIndex = 0;
             batchIndex < pPerRequestInfo->request.GetBatchedHALOutputNum(&pPerRequestInfo->request);
             batchIndex++)
        {
            resultsData.cbPayload.error.sequenceId = pPerRequestInfo->pSequenceId[batchIndex];
            resultsData.pPrivData                  = pPerRequestInfo->request.pPrivData;

            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                "SendErrorNotification message request id %lld, sequence ID %d",
                requestId,
                resultsData.cbPayload.error.sequenceId);

            m_pSession->NotifyResult(&resultsData);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::LogPendingNodes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::LogPendingNodes()
{

    CAMX_LOG_INFO(CamxLogGroupCore, "+    Session %p - Pipeline %s Pending Nodes\n",
                  m_pSession,
                  GetPipelineIdentifierString());
    CAMX_LOG_INFO(CamxLogGroupCore, "+------------------------------------------------------------------+\n");
    CAMX_LOG_INFO(CamxLogGroupCore, "|      Requests Range - lastInOrderCompletedRequestId: %llu currentRequest : %llu",
                  m_lastInOrderCompletedRequestId,
                  m_lastSubmittedRequestId);

    CHAR pendingNodeString[MaxStringLength256] = { 0 };
    CHAR nodeString[MaxStringLength64]         = { 0 };

    for (UINT64 requestId = (m_lastInOrderCompletedRequestId + 1);
        requestId <= m_lastSubmittedRequestId;
        requestId++)
    {
        Utils::Memset(pendingNodeString, 0, sizeof(CHAR) * MaxStringLength256);
        Utils::Memset(nodeString, 0, sizeof(CHAR) * MaxStringLength64);

        for (UINT index = 0; index < m_nodeCount; index++)
        {
            Utils::Memset(nodeString, 0, MaxStringLength64);

            PerRequestNodeStatus requestStatus = m_ppNodes[index]->GetRequestStatus(requestId);
            if (PerRequestNodeStatus::Setup == requestStatus || PerRequestNodeStatus::Deferred == requestStatus ||
                PerRequestNodeStatus::Running == requestStatus)
            {
                OsUtils::SNPrintF(nodeString, MaxStringLength64, "%s%u ", m_ppNodes[index]->Name(),
                                  m_ppNodes[index]->InstanceID());

                OsUtils::StrLCat(pendingNodeString, nodeString, sizeof(pendingNodeString));
            }
        }

        CAMX_LOG_INFO(CamxLogGroupCore, "+        Request %llu Nodes: %s", requestId, pendingNodeString);
    }
 }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::FlushPendingNodes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::FlushPendingNodes()
{
    for (UINT64 requestId = (m_lastInOrderCompletedRequestId + 1);
        requestId <= m_lastSubmittedRequestId;
        requestId++)
    {
        for (UINT index = 0; index < m_nodeCount; index++)
        {
            PerRequestNodeStatus requestStatus = m_ppNodes[index]->GetRequestStatus(requestId);

            if (PerRequestNodeStatus::Deferred == requestStatus)
            {
                NotifyDRQofRequestError(requestId);
            }
            else if (PerRequestNodeStatus::Setup == requestStatus || PerRequestNodeStatus::Running == requestStatus)
            {
                m_ppNodes[index]->Flush(requestId);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::GetFlushResponseTimeInMs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 Pipeline::GetFlushResponseTimeInMs()
{
    UINT64 responseTimeInMs = 0;

    for (UINT index = 0; index < m_nodeCount; index++)
    {
        if (NULL != m_ppNodes[index] && m_ppNodes[index]->IsCurrentlyProcessing())
        {
            responseTimeInMs = CamX::Utils::MaxUINT64(responseTimeInMs, m_ppNodes[index]->GetFlushResponseTimeInMs());
        }
    }

    return  responseTimeInMs;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::ClearPendingResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::ClearPendingResources()
{
    if (GetPipelineStatus() == PipelineStatus::STREAM_ON)
    {
        for (UINT32 nodeIndex = 0; nodeIndex < m_nodeCount; nodeIndex++)
        {
            m_ppNodes[nodeIndex]->ClearDependencies(m_lastCompletedRequestId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::WaitForAllNodesRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::WaitForAllNodesRequest()
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_INFO(CamxLogGroupCore, "Waiting for all node request to complete");

    m_pNodesRequestDoneLock->Lock();
    if (FALSE == AreAllNodesDone())
    {
        CamxAtomicStoreU(&m_isRequestDoneSignalNeeded, 1);
        result = m_pWaitAllNodesRequestDone->TimedWait(m_pNodesRequestDoneLock->GetNativeHandle(),
            DefaultNodesRequestDoneTimeout);
        CamxAtomicStoreU(&m_isRequestDoneSignalNeeded, 0);
    }
    m_pNodesRequestDoneLock->Unlock();

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "m_pWaitAllNodeRequestsDone timed out(%d)!!", DefaultNodesRequestDoneTimeout);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::PopulatePSMetadataSet
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::PopulatePSMetadataSet()
{
    CamxResult result        = CamxResultSuccess;
    UINT32     numVendorTags = CAMX_ARRAY_SIZE(PSVendorTags);
    UINT32&    rCount         = m_nodePSMetadataArray.tagCount;
    UINT32     tagID;

    rCount = 0;

    for (UINT32 tagIndex = 0; tagIndex < numVendorTags; ++tagIndex)
    {
        result = VendorTagManager::QueryVendorTagLocation(
            PSVendorTags[tagIndex].pSectionName,
            PSVendorTags[tagIndex].pTagName,
            &tagID);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMeta, "Pipeline Error: Cannot query vendor tag %s %s",
                PSVendorTags[tagIndex].pSectionName,
                PSVendorTags[tagIndex].pTagName);
            break;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "[PLM_INFO] Populated vendor tag %s.%s %x",
            PSVendorTags[tagIndex].pSectionName,
            PSVendorTags[tagIndex].pTagName,
            tagID);

        m_psMetaMap.insert({ tagID, vector<NodeLinkPropertyInfo>() });
        m_nodePSMetadataArray.tagIdArray[rCount].isVendorTag = TRUE;
        m_nodePSMetadataArray.tagIdArray[rCount].u.vendorTag.pComponentName = PSVendorTags[tagIndex].pSectionName;
        m_nodePSMetadataArray.tagIdArray[rCount].u.vendorTag.pTagName       = PSVendorTags[tagIndex].pTagName;
        m_nodePSMetadataArray.tagIdArray[rCount].u.vendorTag.tagId          = tagID;
        rCount++;
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("com.qti.camera.streamMapMeta",
                                                          "StreamMap",
                                                          &m_streamMapVendorTag);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("com.qti.camera.streamCropInfo",
                                                          "RequestCropInfo",
                                                          &m_requestCropInfoVendorTag);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("com.qti.camera.streamCropInfo",
                                                          "EnableAllCropInfo",
                                                          &m_enableAllCropFlagVendorTag);
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::DumpPSMetadataInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::DumpPSMetadataInfo()
{
    CHAR          logString[MaxStringLength256];

    OsUtils::SNPrintF(logString, sizeof(logString), "Link metadata count %u ", m_psMetaMap.size());

    for (auto mapIterator = m_psMetaMap.begin(); mapIterator != m_psMetaMap.end(); ++mapIterator)
    {
        OsUtils::SNPrintF(logString, sizeof(logString), "--VendorTag %x count %d ",
            mapIterator->first,
            mapIterator->second.size());

        for (auto plmIterator = mapIterator->second.begin(); plmIterator != mapIterator->second.end(); ++plmIterator)
        {
            OsUtils::SNPrintF(logString, sizeof(logString), "--Property %u Node %p name %s ",
                plmIterator->propertyId,
                plmIterator->pNode,
                plmIterator->pNode->Name());
        }
    }
    CAMX_LOG_INFO(CamxLogGroupMeta, "%s", logString);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::GetLinkPropertyId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::GetLinkPropertyId(
    UINT32  metadataId,
    Node*   pNode,
    UINT32* pLinkPropertyId)
{
    CamxResult result           = CamxResultENoSuch;
    auto       linkPropertyList = m_psMetaMap.find(metadataId);

    if (linkPropertyList != m_psMetaMap.end())
    {
        for (auto plmIterator = linkPropertyList->second.begin(); plmIterator != linkPropertyList->second.end(); ++plmIterator)
        {
            if (plmIterator->pNode == pNode)
            {
                *pLinkPropertyId = plmIterator->propertyId;
                result           = CamxResultSuccess;
                break;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::PrepareResultPSM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::PrepareResultPSM(
    UINT64          requestId,
    MetaBuffer*     pInputMetadata,
    MetaBuffer*     pOutputMetadata)
{
    UINT              perRequestIdIndex = (requestId % MaxPerRequestInfo);
    PerRequestInfo*   pPerRequestInfo   = &m_perRequestInfo[perRequestIdIndex];
    StreamBufferInfo* pStreamBufferInfo = &pPerRequestInfo->request.pStreamBuffers[0];
    StreamMapMeta     streamMap         = {};
    UINT              streamMapOffset   = 0;
    RequestCropInfo   requestCropInfo;

    UINT              currentOffset[MaxPSMetaType]                     = {};
    OutputPort*       pOutputPortPerStream[MaxOutputStreamsPerRequest] = {};
    Node*             pNodePerStream[MaxOutputStreamsPerRequest]       = {};

    // update the older stream map
    StreamMapMeta* pInputStreamMap = static_cast<StreamMapMeta*>(pInputMetadata->GetTag(m_streamMapVendorTag));
    requestCropInfo.streamCropCount = 0;

    if (NULL != pInputStreamMap)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "[PSM_INFO] Previous stream count %u request %llu",
            pInputStreamMap->streamMapEntryCount, requestId);
        streamMap       = *pInputStreamMap;
        streamMapOffset = pInputStreamMap->streamMapEntryCount;

        for (UINT index = 0; index < pInputStreamMap->streamMapEntryCount; ++index)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMeta, "[PSM_INFO] Stream[%d] dataIndex %u stream %p parentStream %p request %llu ",
                index,
                pInputStreamMap->streamMapEntry[index].index,
                pInputStreamMap->streamMapEntry[index].pSinkStreamPrivateInfo,
                pInputStreamMap->streamMapEntry[index].pParentSinkStreamPrivateInfo,
                requestId);
        }
    }

    if (pStreamBufferInfo->numOutputBuffers + streamMapOffset < MaxOutputStreamsPerRequest)
    {
        for (UINT streamIndex = 0; streamIndex < pStreamBufferInfo->numOutputBuffers; ++streamIndex)
        {
            streamMap.streamMapEntry[streamIndex + streamMapOffset].pSinkStreamPrivateInfo =
                pStreamBufferInfo->outputBuffers[streamIndex].pStream->pPrivateInfo;
        }
    }

    // Fill output port and node info for each stream
    for (UINT nodeIndex = 0; nodeIndex < m_nodesSinkOutPorts.numNodes; ++nodeIndex)
    {
        std::vector<OutputPort*> outputPorts;

        Node* pNode = m_ppNodes[m_nodesSinkOutPorts.nodeIndices[nodeIndex]];

        pNode->GetOutputPortsWithSinkBuffer(outputPorts);

        // get the list of all ports that can produce HAL buffer
        for (UINT portIndex = 0; portIndex < outputPorts.size(); ++portIndex)
        {
            OutputPort* pOutputPort = outputPorts[portIndex];
            for (UINT bufIndex = 0; bufIndex < pStreamBufferInfo->numOutputBuffers; ++bufIndex)
            {
                ChiStreamWrapper* pStreamWrapper = static_cast<ChiStreamWrapper*>(
                    pStreamBufferInfo->outputBuffers[bufIndex].pStream->pPrivateInfo);

                if (NULL != pStreamWrapper)
                {
                    if (pStreamWrapper->GetStreamIndex() == pOutputPort->sinkTargetStreamId)
                    {
                        pOutputPortPerStream[bufIndex] = pOutputPort;
                        pNodePerStream[bufIndex]       = pNode;
                        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "[PSM_INFO] Node %s port %u request %llu streamId %u %u",
                            pNode->Name(), pOutputPort->portId, requestId,
                            pStreamWrapper->GetStreamIndex(), pOutputPort->sinkTargetStreamId);
                        break;
                    }
                }
            }
        }
    }

    // Fill map structure and populate each PSMeta data
    for (UINT bufIndex = 0; bufIndex < pStreamBufferInfo->numOutputBuffers; ++bufIndex)
    {
        if ((NULL != pOutputPortPerStream[bufIndex]) && (NULL != pNodePerStream[bufIndex]))
        {
            // update parent sink stream now
            {
                vector<PerRequestInputPortInfo*> sinkInputPorts;
                pNodePerStream[bufIndex]->GetParentSinkPortWithHALBuffer(
                    pNodePerStream[bufIndex]->OutputPortIndex(pOutputPortPerStream[bufIndex]->portId),
                    requestId, sinkInputPorts);

                if (0 < sinkInputPorts.size())
                {
                    // must select only one source port
                    streamMap.streamMapEntry[bufIndex + streamMapOffset].pParentSinkStreamPrivateInfo =
                        sinkInputPorts[0]->pParentSinkStream;
                }
            }

            // Update for each tag
            for (UINT tagIndex = 0; tagIndex < MaxPSMetaType; ++tagIndex)
            {
                // Only if multi-instance is supported
                if ((TRUE == PSVendorTags[tagIndex].multiInstance) || (0 == currentOffset[tagIndex]))
                {
                    UINT tagId  = m_nodePSMetadataArray.tagIdArray[tagIndex].u.vendorTag.tagId;

                    auto linkPropertyList = m_psMetaMap.find(tagId);

                    // atleast one publisher should be there
                    if ((linkPropertyList != m_psMetaMap.end()) &&
                        (1 <= linkPropertyList->second.size()))
                    {
                        // get the active ancestor that publishes the tags
                        vector<UniqueNodePortInfo> ancestorList;

                        pNodePerStream[bufIndex]->GetActiveAncestorNodePortInfo(
                            pNodePerStream[bufIndex]->OutputPortIndex(pOutputPortPerStream[bufIndex]->portId),
                            tagId, requestId, ancestorList);

                        // Pick one ancestor
                        if (0 < ancestorList.size())
                        {
                            UniqueNodePortInfo& rNodePortInfo = ancestorList[0];
                            VOID*               pData        = NULL;

                            rNodePortInfo.pNode->GetPSMetaByRequestId(rNodePortInfo.portId, tagId, requestId, &pData);

                            if (NULL != pData)
                            {
                                CamxResult resultLocal = pOutputMetadata->SetTag(tagId, pData,
                                    PSVendorTags[tagIndex].unitSize,
                                    PSVendorTags[tagIndex].unitSize,
                                    FALSE, NULL,
                                    currentOffset[tagIndex]*PSVendorTags[tagIndex].unitSize);

                                if (CamxResultSuccess == resultLocal)
                                {
                                    streamMap.streamMapEntry[bufIndex + streamMapOffset].index[tagIndex] =
                                        currentOffset[tagIndex];
                                    currentOffset[tagIndex]++;
                                }

                                if ((StreamCrop == tagIndex) && (TRUE == IsPSMPublished(tagId)))
                                {
                                    UINT requestCropIndex = 0;

                                    resultLocal = pNodePerStream[bufIndex]->PopulateRequestCropInfo(
                                        pNodePerStream[bufIndex]->OutputPortIndex(pOutputPortPerStream[bufIndex]->portId),
                                        tagId,
                                        requestId,
                                        requestCropInfo,
                                        requestCropIndex);

                                    if (CamxResultSuccess == resultLocal)
                                    {
                                        streamMap.streamMapEntry[bufIndex + streamMapOffset].requestCropIndex =
                                            requestCropIndex;
                                    }
                                    else
                                    {
                                        CAMX_LOG_ERROR(CamxLogGroupMeta, "PopulateRequestCropInfo failed for tagId: %x",
                                            tagId);
                                    }

                                }
                            }

                            CAMX_LOG_VERBOSE(CamxLogGroupMeta, "Publisher for vendor tag %s %s Sink Node %s publisher %s"
                                " nextoffset %u",
                                PSVendorTags[tagIndex].pSectionName,
                                PSVendorTags[tagIndex].pTagName,
                                pNodePerStream[bufIndex]->Name(),
                                rNodePortInfo.pNode->Name(),
                                currentOffset[tagIndex]);
                        }
                    }
                }
            }
        }
    }

    streamMap.streamMapEntryCount += pStreamBufferInfo->numOutputBuffers;

    BOOL* pEnableAllCrop = NULL;
    if (0 < m_enableAllCropFlagVendorTag)
    {
        pEnableAllCrop = static_cast<BOOL*>(pInputMetadata->GetTag(m_enableAllCropFlagVendorTag));
    }

    // set request crop information
    if ((0 < m_requestCropInfoVendorTag) && (NULL != pEnableAllCrop) && (TRUE == *pEnableAllCrop) )
    {
        pOutputMetadata->SetTag(m_requestCropInfoVendorTag,
                                &requestCropInfo,
                                1,
                                sizeof(requestCropInfo));
    }

    // set the map information
    if (0 < m_streamMapVendorTag)
    {
        pOutputMetadata->SetTag(m_streamMapVendorTag, &streamMap, sizeof(streamMap), sizeof(streamMap));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::WaitUntilStreamOnDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Pipeline::WaitUntilStreamOnDone()
{
    CamxResult result = CamxResultSuccess;

    m_pStreamOnDoneLock->Lock();
    if (FALSE == m_isStreamOnDone)
    {
        result = m_pWaitForStreamOnDone->TimedWait(m_pStreamOnDoneLock->GetNativeHandle(), DefaultStreamOnTimeout);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Timed out!! waiting for pipeline: %s streamon for %u",
                GetPipelineIdentifierString(), DefaultStreamOnTimeout);
        }
    }
    m_pStreamOnDoneLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::NotifyStreamOnWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::NotifyStreamOnWait()
{
    m_pStreamOnDoneLock->Lock();
    CAMX_LOG_INFO(CamxLogGroupCore, "Wakeup streamon waiting thread for pipeline: %s",
        GetPipelineIdentifierString());
    m_pWaitForStreamOnDone->Signal();
    m_pStreamOnDoneLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::CalculateFirstPendingRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::CalculateFirstPendingRequest()
{
    BOOL firstPendingRequestSet = FALSE;

    for (UINT64 requestId = (m_lastInOrderCompletedRequestId + 1);
        requestId <= m_lastSubmittedRequestId;
        requestId++)
    {
        for (UINT index = 0; index < m_nodeCount; index++)
        {
            PerRequestNodeStatus requestStatus = m_ppNodes[index]->GetRequestStatus(requestId);
            if (PerRequestNodeStatus::Submit == requestStatus)
            {
                m_firstPendingRequestId = requestId;
                m_firstPendingCSLSyncId = m_ppNodes[index]->GetCSLSyncId(m_firstPendingRequestId);
                firstPendingRequestSet  = TRUE;
                break;
            }
        }

        if (TRUE == firstPendingRequestSet)
        {
            break;
        }
    }
}

CAMX_NAMESPACE_END
