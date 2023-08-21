////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxawbnode.cpp
/// @brief Stats Auto White Balance node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxatomic.h"
#include "camxawbnode.h"
#include "camxcawbstatsprocessor.h"
#include "camxhwdefs.h"
#include "camxpipeline.h"
#include "camxpropertyblob.h"
#include "camxstatscommon.h"
#include "camxtrace.h"
#include "camxvendortags.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::AWBNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AWBNode::AWBNode()
    : m_pMultiStatsOperator(NULL)
{
    CAMX_LOG_INFO(CamxLogGroupAWB, "AWBNode()");

    m_pNodeName    = "AWB";
    m_derivedNodeHandlesMetaDone = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::~AWBNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AWBNode::~AWBNode()
{
    if (NULL != m_pAWBStatsProcessor)
    {
        m_pAWBStatsProcessor->Destroy();
        m_pAWBStatsProcessor = NULL;
    }

    if (NULL != m_pMultiStatsOperator)
    {
        CAMX_DELETE m_pMultiStatsOperator;
        m_pMultiStatsOperator = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AWBNode* AWBNode::Create(
    const NodeCreateInputData*  pCreateInputData,
    NodeCreateOutputData*       pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    CamxResult               result              = CamxResultSuccess;
    UINT32                   propertyCount       = pCreateInputData->pNodeInfo->nodePropertyCount;
    AWBNode*                 pStatsNode          = NULL;
    StatsNodeCreateData      createData          = {0};
    StatsInitializeCallback* pInitializecallback = NULL;

    pStatsNode = CAMX_NEW AWBNode();
    if (NULL != pStatsNode)
    {
        createData.pNode            = pStatsNode;
        createData.instanceId       = pCreateInputData->pNodeInfo->instanceId;

        pStatsNode->m_skipPattern   = 1;

        for (UINT32 count = 0; count < propertyCount; count++)
        {
            if (NodePropertyStatsSkipPattern == pCreateInputData->pNodeInfo->pNodeProperties[count].id)
            {
                pStatsNode->m_skipPattern = *static_cast<UINT*>(pCreateInputData->pNodeInfo->pNodeProperties[count].pValue);
            }
        }

        // Create stats AWB processor
        result = CAWBStatsProcessor::Create(&pStatsNode->m_pAWBStatsProcessor);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupAWB, "CAWBStatsProcessor::Create failed = %d", result);
            CAMX_DELETE pStatsNode;

            pStatsNode = NULL;
        }
        else
        {
            pInitializecallback                 = &pStatsNode->m_statsInitializeData.initializecallback;
            pInitializecallback->pAWBCallback   = pCreateInputData->pAWBAlgoCallbacks;
        }

        pStatsNode->m_inputSkipFrameTag = 0;
        if (CDKResultSuccess !=
            VendorTagManager::QueryVendorTagLocation("com.qti.chi.statsSkip", "skipFrame", &pStatsNode->m_inputSkipFrameTag))
        {
            CAMX_LOG_ERROR(CamxLogGroupAWB, "Failed to query stats skip vendor tag");
        }

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "AWBNode::Create failed - out of memory");
        result = CamxResultENoMemory;
    }

    CAMX_LOG_INFO(CamxLogGroupAWB, "AWBNode::Create result = %d", result);

    return pStatsNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AWBNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AWBNode::ProcessingNodeInitialize(
    const NodeCreateInputData*  pCreateInputData,
    NodeCreateOutputData*       pCreateOutputData)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupAWB, SCOPEEventAWBNodeNodeInitialize);
    CamxResult result = CamxResultSuccess;

    m_pChiContext                   = pCreateInputData->pChiContext;
    pCreateOutputData->pNodeName    = m_pNodeName;

    m_bufferOffset = GetMaximumPipelineDelay();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::ProcessingNodeFinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AWBNode::ProcessingNodeFinalizeInitialization(
    FinalizeInitializationData* pFinalizeInitializationData)
{
    CamxResult result = CamxResultSuccess;

    // Store the required input information.
    m_statsInitializeData.pDebugDataPool = pFinalizeInitializationData->pDebugDataPool;
    m_statsInitializeData.pThreadManager = pFinalizeInitializationData->pThreadManager;
    m_statsInitializeData.pHwContext     = pFinalizeInitializationData->pHwContext;

    InitializePreviousSessionData();

    UINT inputPortId[StatsInputPortMaxCount];
    UINT numInputPort = 0;

    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
    {
        // need request - 4 buffer
        SetInputPortBufferDelta(inputIndex, m_bufferOffset);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::GetPropertyDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AWBNode::GetPropertyDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsProcessRequestDataInfo
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);

    if (NULL == pExecuteProcessRequestData || NULL == pStatsProcessRequestDataInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pExecuteProcessRequestData: %p or pStatsProcessRequestDataInfo: %p pointer is NULL",
            pExecuteProcessRequestData, pStatsProcessRequestDataInfo);
        return CamxResultEInvalidPointer;
    }

    CamxResult              result = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    StatsDependency         statsDependency = { 0 };

    if (NULL == pNodeRequestData || NULL == m_pAWBStatsProcessor)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pNodeRequestData: %p or m_pAWBStatsProcessor: %p pointer is NULL",
            pNodeRequestData, m_pAWBStatsProcessor);
        return CamxResultEInvalidPointer;
    }

    pStatsProcessRequestDataInfo->pDependencyUnit = &pNodeRequestData->dependencyInfo[0];

    // Check property dependencies
    result = m_pAWBStatsProcessor->GetDependencies(pStatsProcessRequestDataInfo, &statsDependency);

    if (CamxResultSuccess == result)
    {
        // Add property dependencies
        pNodeRequestData->dependencyInfo[0].propertyDependency.count = statsDependency.propertyCount;
        for (INT32 i = 0; i < statsDependency.propertyCount; i++)
        {
            if (TRUE == IsTagPresentInPublishList(statsDependency.properties[i].property))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[i] =
                    statsDependency.properties[i].property;
                pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[i]    =
                    statsDependency.properties[i].offset;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupMeta, "property: %08x is not published in the pipeline count %d ",
                    statsDependency.properties[i].property, pNodeRequestData->dependencyInfo[0].propertyDependency.count);
                pNodeRequestData->dependencyInfo[0].propertyDependency.count--;
            }
        }
    }

    // For First request (and first request after flush), we should not set any dependency and it should just be bypassed
    UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(pNodeRequestData->pCaptureRequest->requestId);

    if ((CamxResultSuccess == result) && (requestIdOffsetFromLastFlush > 1))
    {
        if (TRUE == IsTagPresentInPublishList(PropertyIDAECFrameControl))
        {
            // Adding dependency on AEC(N-1) so that AWB and AEC can run in parallel
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count]   = PropertyIDAECFrameControl;
            pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count++] = 1;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMeta, "property: %08x is not published in the pipeline", PropertyIDAECFrameControl);
        }

        if (TRUE == IsTagPresentInPublishList(PropertyIDAECFrameInfo))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count]   = PropertyIDAECFrameInfo;
            pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count++] = 1;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMeta, "property: %08x is not published in the pipeline", PropertyIDAECFrameInfo);
        }

        if (TRUE == IsTagPresentInPublishList(PropertyIDAWBFrameControl))
        {
            // Adding dependency on AWB(N-1) to make sure AWB runs sequentially
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count]   = PropertyIDAWBFrameControl;
            pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count++] = 1;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMeta, "property: %08x is not published in the pipeline", PropertyIDAWBFrameControl);
        }
    }

    if (pNodeRequestData->dependencyInfo[0].propertyDependency.count > 1)
    {
        // Update dependency request data for topology to consume
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency               = TRUE;
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;
        pNodeRequestData->dependencyInfo[0].processSequenceId                                   = 1;

        pNodeRequestData->numDependencyLists = 1;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::GetBufferDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AWBNode::GetBufferDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);

    if (NULL == pExecuteProcessRequestData)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pExecuteProcessRequestData pointer is NULL");
        return CamxResultEInvalidPointer;
    }

    PerRequestActivePorts*  pEnabledPorts       = pExecuteProcessRequestData->pEnabledPortsInfo;
    NodeProcessRequestData* pNodeRequestData    = pExecuteProcessRequestData->pNodeProcessRequestData;
    CamxResult              result              = CamxResultSuccess;

    if (NULL == pNodeRequestData || NULL == pEnabledPorts)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pNodeRequestData: %p or pEnabledPorts: %p pointer is NULL",
            pNodeRequestData, pEnabledPorts);
        return CamxResultEInvalidPointer;
    }

    PropertyDependency* pDeps = &pNodeRequestData->dependencyInfo[0].propertyDependency;

    for (UINT i = 0; i < pEnabledPorts->numInputPorts; i++)
    {
        PerRequestInputPortInfo* pPerRequestInputPort = &pEnabledPorts->pInputPorts[i];

        if (NULL == pPerRequestInputPort)
        {
            CAMX_LOG_ERROR(CamxLogGroupAWB, "Per Request Port info a Null pointer for index: %d", i);
            return CamxResultEInvalidPointer;
        }

        switch (pPerRequestInputPort->portId)
        {
            case StatsInputPortAWBBG:
                pDeps->properties[pDeps->count++] = PropertyIDParsedAWBBGStatsOutput;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupAWB, "Need to add new stats support");
                break;
        }
    }

    if (pDeps->count > 0)
    {
        pNodeRequestData->dependencyInfo[0].processSequenceId                                   = 1;
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency               = TRUE;
        pNodeRequestData->numDependencyLists                                                    = 1;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::GetMultiStatsDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AWBNode::GetMultiStatsDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsProcessRequestDataInfo
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);

    if (NULL == pExecuteProcessRequestData)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "Execute Process Request Data NULL pointer");
        return CamxResultEInvalidPointer;
    }

    CamxResult              result              = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData    = pExecuteProcessRequestData->pNodeProcessRequestData;
    DependencyUnit*         pDependencyUnit     = &(pNodeRequestData->dependencyInfo[0]);

    if (TRUE == GetStaticSettings()->multiCameraEnable)
    {
        if (NULL != m_pMultiStatsOperator)
        {
            UINT64 requestIdOffsetFromLastFlush =
                GetRequestIdOffsetFromLastFlush(pNodeRequestData->pCaptureRequest->requestId);

            result = m_pMultiStatsOperator->UpdateStatsDependencies(
                pExecuteProcessRequestData,
                pStatsProcessRequestDataInfo,
                requestIdOffsetFromLastFlush);

            if (CamxResultSuccess == result && TRUE == pDependencyUnit->dependencyFlags.hasPropertyDependency)
            {
                if (0 == pDependencyUnit->processSequenceId)
                {
                    pDependencyUnit->processSequenceId                                  = 1;
                    pDependencyUnit->dependencyFlags.hasIOBufferAvailabilityDependency  = TRUE;
                }
                if (0 == pNodeRequestData->numDependencyLists)
                {
                    pNodeRequestData->numDependencyLists = 1;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::PrepareStatsProcessRequestData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AWBNode::PrepareStatsProcessRequestData(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsProcessRequestData)
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);

    if (NULL == pExecuteProcessRequestData)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "Execute Process Request Data NULL pointer");
        return CamxResultEInvalidPointer;
    }

    CamxResult              result              = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData    = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*  pEnabledPorts       = pExecuteProcessRequestData->pEnabledPortsInfo;

    if (NULL == pNodeRequestData || NULL == pEnabledPorts || NULL == pNodeRequestData->pCaptureRequest)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "NULL pointer! pNodeRequestData: %p pEnabledPorts: %p",
            pNodeRequestData, pEnabledPorts);
        return CamxResultEInvalidPointer;
    }

    pStatsProcessRequestData->skipProcessing    = CanSkipAlgoProcessing(pNodeRequestData->pCaptureRequest->requestId);
    pStatsProcessRequestData->requestId         = pNodeRequestData->pCaptureRequest->requestId;
    pStatsProcessRequestData->pMultiRequestSync = pNodeRequestData->pCaptureRequest->pMultiRequestData;
    pStatsProcessRequestData->peerSyncInfo      = pNodeRequestData->pCaptureRequest->peerSyncInfo;
    pStatsProcessRequestData->pNode             = this;
    pStatsProcessRequestData->algoAction        = StatsAlgoProcessRequest;
    pStatsProcessRequestData->operationMode     = m_statsInitializeData.statsStreamInitConfig.operationMode;

    if (FALSE == pStatsProcessRequestData->skipProcessing)
    {
        for (UINT i = 0; i < pEnabledPorts->numInputPorts; i++)
        {
            PerRequestInputPortInfo* pPerRequestInputPort = &pEnabledPorts->pInputPorts[i];

            if (NULL == pPerRequestInputPort)
            {
                CAMX_LOG_ERROR(CamxLogGroupAWB, "Per Request Port info a Null pointer for index: %d", i);
                return CamxResultEInvalidPointer;
            }

            pStatsProcessRequestData->bufferInfo[i].statsType = StatsUtil::GetStatsType(pPerRequestInputPort->portId);
            // Removed fence and image buffer as they do not apply when coming from StatsParse and should not be used
        }

        pStatsProcessRequestData->bufferCount = pEnabledPorts->numInputPorts;

        if (NULL != pExecuteProcessRequestData->pTuningModeData)
        {
            pStatsProcessRequestData->pTuningModeData = pExecuteProcessRequestData->pTuningModeData;
        }
    }

    if (TRUE == m_statsInitializeData.pPipeline->IsMultiCamera())
    {
        UINT        tag[1]              = { 0 };
        VOID*       pData[1]            = { 0 };
        UINT        length              = CAMX_ARRAY_SIZE(tag);
        UINT64      configDataOffset[1] = { 0 };

        // Query information sent by multi camera controller
        VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicamerainfo", "MasterCamera", &tag[0]);
        tag[0] |= InputMetadataSectionMask;
        GetDataList(tag, pData, configDataOffset, length);

        if (NULL != pData[0])
        {
            BOOL*              pIsMaster = reinterpret_cast<BOOL*>(pData[0]);

            pStatsProcessRequestData->cameraInfo.cameraId = GetPipeline()->GetCameraId();
            pStatsProcessRequestData->cameraInfo.algoRole =
                (*pIsMaster) ? StatsAlgoRoleMaster : StatsAlgoRoleSlave;

            CAMX_LOG_VERBOSE(CamxLogGroupAWB, "AWBNode CameraInfo ReqId:%llu CamId:%d Role:%-7s",
                pStatsProcessRequestData->requestId,
                pStatsProcessRequestData->cameraInfo.cameraId,
                StatsUtil::GetRoleName(pStatsProcessRequestData->cameraInfo.algoRole));
            m_cameraInfo = pStatsProcessRequestData->cameraInfo;
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupAWB, "Multi camera metadata not published for request %llu",
                          pStatsProcessRequestData->requestId);
            pStatsProcessRequestData->cameraInfo = m_cameraInfo;
        }
    }
    else
    {
        pStatsProcessRequestData->cameraInfo.algoRole   = StatsAlgoRoleDefault;
        pStatsProcessRequestData->cameraInfo.cameraId   = GetPipeline()->GetCameraId();
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AWBNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupAWB, SCOPEEventAWBNodeExecuteProcessRequest,
        pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId);

    CAMX_LOG_VERBOSE(CamxLogGroupAWB,
                     "AWBNode execute request for id %llu seqId %d",
                     pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId,
                     pExecuteProcessRequestData->pNodeProcessRequestData->processSequenceId);

    CamxResult              result                          = CamxResultSuccess;
    StatsProcessRequestData statsProcessRequestData         = {0};
    NodeProcessRequestData* pNodeRequestData                = pExecuteProcessRequestData->pNodeProcessRequestData;

    if (pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask != 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pending dependencies! dependencyFlagsMask = %d",
            pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask);
        result = CamxResultEFailed;
    }

    PrepareStatsProcessRequestData(pExecuteProcessRequestData, &statsProcessRequestData);

    CAMX_LOG_VERBOSE(CamxLogGroupAWB, "AWBNode execute request for requsetId %llu pipeline %d seqId %d skipProcessing: %d",
        pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId,
        GetPipeline()->GetPipelineId(),
        pExecuteProcessRequestData->pNodeProcessRequestData->processSequenceId,
        statsProcessRequestData.skipProcessing);

    if (CamxResultSuccess == result && 0 == pExecuteProcessRequestData->pNodeProcessRequestData->processSequenceId)
    {
        // Initialize number of dependency lists to 0
        pExecuteProcessRequestData->pNodeProcessRequestData->numDependencyLists = 0;

        result = GetPropertyDependencies(pExecuteProcessRequestData, &statsProcessRequestData);

        if ((CamxResultSuccess == result) && (FALSE == statsProcessRequestData.skipProcessing))
        {
            result = GetBufferDependencies(pExecuteProcessRequestData);
        }

        if (CamxResultSuccess == result)
        {
            result = GetMultiStatsDependencies(pExecuteProcessRequestData, &statsProcessRequestData);
        }
    }

    if ((CamxResultSuccess  == result) &&
        (FALSE              == Node::HasAnyDependency(pExecuteProcessRequestData->pNodeProcessRequestData->dependencyInfo)))
    {
        if (TRUE == GetStaticSettings()->multiCameraEnable && NULL != m_pMultiStatsOperator)
        {
            statsProcessRequestData.algoAction = m_pMultiStatsOperator->GetStatsAlgoAction();
        }

        result = m_pAWBStatsProcessor->ExecuteProcessRequest(&statsProcessRequestData);

        NotifyJobProcessRequestDone(pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId);

        CAMX_LOG_VERBOSE(CamxLogGroupAWB,
                         "ProcessRequestDone %llu",
                         pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AWBNode::PostPipelineCreate()
{
    CamxResult              result              = CamxResultSuccess;
    CHIALGORITHMINTERFACE   chiAlgoInterface    = { 0 };
    INT32                   cameraID            = GetPipeline()->GetCameraId();

    // Store the required input information.
    m_statsInitializeData.pNode              = this;
    m_statsInitializeData.pStaticPool        = m_pChiContext->GetStaticMetadataPool(GetPipeline()->GetCameraId());
    m_statsInitializeData.pDebugDataPool     = GetPerFramePool(PoolType::PerFrameDebugData);
    m_statsInitializeData.pThreadManager     = GetThreadManager();
    m_statsInitializeData.pHwContext         = GetHwContext();
    m_statsInitializeData.pTuningDataManager = GetTuningDataManager();
    m_statsInitializeData.pPipeline          = GetPipeline();
    m_statsInitializeData.pNode              = this;
    m_statsInitializeData.pChiContext        = m_pChiContext;

    // Remove the below hard-coding after testing
    m_statsInitializeData.statsStreamInitConfig.operationMode = StatsOperationModeNormal;
    StatsUtil::GetStatsStreamInitConfig(m_statsInitializeData.pPipeline->GetPerFramePool(PoolType::PerUsecase),
        &m_statsInitializeData.statsStreamInitConfig);

    // Initialize algo role as default
    // Multicamera vendor tags will be read later in PrepareStatsProcessRequestData
    m_statsInitializeData.cameraInfo.cameraId   = static_cast<UINT32>(cameraID);
    m_statsInitializeData.cameraInfo.algoRole   = StatsAlgoRoleDefault;

    chiAlgoInterface.pGetVendorTagBase       = ChiStatsSession::GetVendorTagBase;
    chiAlgoInterface.pGetMetadata            = ChiStatsSession::FNGetMetadata;
    chiAlgoInterface.pSetMetaData            = ChiStatsSession::FNSetMetadata;
    chiAlgoInterface.pQueryVendorTagLocation = ChiStatsSession::QueryVendorTagLocation;
    chiAlgoInterface.size                    = sizeof(CHIALGORITHMINTERFACE);

    if ((NULL != m_statsInitializeData.initializecallback.pAWBCallback) &&
        (NULL != m_statsInitializeData.initializecallback.pAWBCallback->pfnSetAlgoInterface))
    {
        m_statsInitializeData.initializecallback.pAWBCallback->pfnSetAlgoInterface(&chiAlgoInterface);
    }

    InitializePreviousSessionData();

    result = m_pAWBStatsProcessor->Initialize(&m_statsInitializeData);


    // Handle multi camera usecase
    if (CamxResultSuccess == result)
    {
        result = InitializeMultiStats();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::InitializePreviousSessionData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AWBNode::InitializePreviousSessionData()
{


    /// @todo  (CAMX-523): Get the previous session data.


}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::NotifyJobProcessRequestDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AWBNode::NotifyJobProcessRequestDone(
    UINT64 requestId)
{
    CamxResult  result      = CamxResultSuccess;

    ProcessPartialMetadataDone(requestId);
    ProcessMetadataDone(requestId);
    ProcessRequestIdDone(requestId);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::CanSkipAlgoProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AWBNode::CanSkipAlgoProcessing(
    UINT64 requestId)
{
    UINT skipFactor                     = m_skipPattern;
    UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);
    BOOL iSForceSkip                    = FALSE;

    // Stats node will publish default initialized output for maxPipelineDelay number of frames.
    // We should start skipping 1 frame after that. This way we will have all the result output from previous frame
    // to publish for the first skipped frame
    skipFactor =
        (requestIdOffsetFromLastFlush <= (FirstValidRequestId + GetMaximumPipelineDelay())) ? 1 : skipFactor;

    // Force skip can be supported any frame after FirstValidRequestId + GetMaximumPipelineDelay()
    iSForceSkip =
        (requestIdOffsetFromLastFlush < ( FirstValidRequestId + GetMaximumPipelineDelay() )) ? FALSE : IsForceSkipRequested();

    BOOL skipRequired =
        (((requestIdOffsetFromLastFlush % skipFactor) == 0) && (iSForceSkip == FALSE) ? FALSE : TRUE);

    CAMX_LOG_VERBOSE(CamxLogGroupAWB, "skipFactor %d skipRequired %d iSForceSkip %d requestIdOffsetFromLastFlush %llu",
        skipFactor, skipRequired, iSForceSkip, requestIdOffsetFromLastFlush);

    return skipRequired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::InitializeMultiStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AWBNode::InitializeMultiStats()
{
    CamxResult            result                = CamxResultSuccess;
    const StaticSettings* pStaticSettings       = GetStaticSettings();
    BOOL                  isMultiCameraRunning  = FALSE;

    if (TRUE == m_statsInitializeData.pPipeline->IsMultiCamera())
    {
        isMultiCameraRunning = TRUE;
    }

    if (TRUE == isMultiCameraRunning)
    {
        if (MultiCamera3ASyncQTI == pStaticSettings->multiCamera3ASync)
        {
            CAMX_LOG_INFO(CamxLogGroupAWB, "3A sync scheme selected: QTI");
            m_pMultiStatsOperator = CAMX_NEW QTIMultiStatsOperator;
        }
        else if (MultiCamera3ASyncSingleton == pStaticSettings->multiCamera3ASync)
        {
            CAMX_LOG_INFO(CamxLogGroupAWB, "3A sync scheme selected: Singleton Algo");
            m_pMultiStatsOperator = CAMX_NEW SingletonStatsOperator;
        }
        else if (MultiCamera3ASyncDisabled == pStaticSettings->multiCamera3ASync)
        {
            CAMX_LOG_INFO(CamxLogGroupAWB, "3A sync scheme selected: No 3A sync needed");
            m_pMultiStatsOperator = CAMX_NEW NoSyncStatsOperator;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupAWB, "3A sync scheme selected: Invalid scheme");
            m_pMultiStatsOperator = CAMX_NEW NoSyncStatsOperator;
        }

        if (NULL != m_pMultiStatsOperator)
        {
            MultiStatsData multiStatsData;

            multiStatsData.algoSyncType     = StatsAlgoSyncType::StatsAlgoSyncTypeAWB;
            multiStatsData.algoRole         = StatsAlgoRole::StatsAlgoRoleDefault;
            multiStatsData.pipelineId       = m_statsInitializeData.pPipeline->GetPipelineId();
            multiStatsData.pHwContext       = m_statsInitializeData.pHwContext;
            multiStatsData.pNode            = this;

            m_pMultiStatsOperator->Initialize(&multiStatsData);
        }
        else
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AWBNode::IsForceSkipRequested
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AWBNode::IsForceSkipRequested()
{
    UINT32   isForceSkip = 0;

    if (0 != m_inputSkipFrameTag)
    {
        UINT tagReadInput[]    = { m_inputSkipFrameTag | InputMetadataSectionMask };
        VOID* pDataFrameSkip[] = { 0 };
        UINT64 dataOffset[1]   = { 0 };
        GetDataList(tagReadInput, pDataFrameSkip, dataOffset, 1);

        if (NULL != pDataFrameSkip[0])
        {
            isForceSkip = *static_cast<UINT32*>(pDataFrameSkip[0]);
        }
    }

    return ((isForceSkip) == 0) ? FALSE : TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AWBNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AWBNode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_pAWBStatsProcessor)
    {
        result = m_pAWBStatsProcessor->GetPublishList(CAMX_ARRAY_SIZE(pPublistTagList->tagArray),
            pPublistTagList->tagArray,
            &pPublistTagList->tagCount,
            &pPublistTagList->partialTagCount);
    }
    else
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupMeta, "AWB processor not initialized");
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%d tags will be published", pPublistTagList->tagCount);
    return result;
}

CAMX_NAMESPACE_END
