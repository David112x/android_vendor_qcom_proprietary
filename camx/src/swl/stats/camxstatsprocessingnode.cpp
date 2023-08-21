////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxstatsprocessingnode.cpp
/// @brief Stats processing node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxatomic.h"
#include "camxhwdefs.h"
#include "camxpipeline.h"
#include "camxpropertyblob.h"
#include "camxstatscommon.h"
#include "camxstatsprocessingnode.h"
#include "camxtrace.h"
#include "camxvendortags.h"
#include "camxhal3module.h"
#include "camximagesensormoduledata.h"
#include "camxsensordriver.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::StatsProcessingNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsProcessingNode::StatsProcessingNode()
    : m_pMultiStatsOperator(NULL)
{
    CAMX_LOG_INFO(CamxLogGroupStats, "StatsProcessingNodeStatsProcessingNode()");

    m_pNodeName    = "Stats";
    m_derivedNodeHandlesMetaDone = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::~StatsProcessingNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsProcessingNode::~StatsProcessingNode()
{
    if (NULL != m_pStatsProcessorManager)
    {
        // Destroy the job dispatcher.
        m_pStatsProcessorManager->Destroy();
        m_pStatsProcessorManager = NULL;
    }

    if (NULL != m_pMultiStatsOperator)
    {
        CAMX_DELETE m_pMultiStatsOperator;
        m_pMultiStatsOperator = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsProcessingNode* StatsProcessingNode::Create(
    const NodeCreateInputData*  pCreateInputData,
    NodeCreateOutputData*       pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    CamxResult               result              = CamxResultSuccess;
    UINT32                   propertyCount       = pCreateInputData->pNodeInfo->nodePropertyCount;
    StatsProcessingNode*     pStatsNode          = NULL;
    StatsNodeCreateData      createData          = {0};
    StatsInitializeCallback* pInitializecallback = NULL;

    pStatsNode = CAMX_NEW StatsProcessingNode();
    if (NULL != pStatsNode)
    {
        createData.pNode = pStatsNode;
        createData.instanceId = pCreateInputData->pNodeInfo->instanceId;
        createData.pStatsNotifier = static_cast<IStatsNotifier*>(pStatsNode);

        pStatsNode->m_skipPattern = 1;

        for (UINT32 count = 0; count < propertyCount; count++)
        {
            if (NodePropertyStatsSkipPattern == pCreateInputData->pNodeInfo->pNodeProperties[count].id)
            {
                pStatsNode->m_skipPattern = *static_cast<UINT*>(pCreateInputData->pNodeInfo->pNodeProperties[count].pValue);
            }
        }
        // Create stats job dispatcher.
        result = StatsProcessorManager::Create(&createData, &pStatsNode->m_pStatsProcessorManager);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupStats, "StatsProcessorManager::Create failed = %d", result);
            CAMX_DELETE pStatsNode;

            pStatsNode = NULL;
        }
        else
        {
            pInitializecallback = &pStatsNode->m_statsInitializeData.initializecallback;
            pInitializecallback->pAECCallback = pCreateInputData->pAECAlgoCallbacks;
            pInitializecallback->pAWBCallback = pCreateInputData->pAWBAlgoCallbacks;
            pInitializecallback->pASDCallback = pCreateInputData->pASDAlgoCallbacks;
            pInitializecallback->pAFDCallback = pCreateInputData->pAFDAlgoCallbacks;


            pStatsNode->m_inputSkipFrameTag = 0;
            if (CDKResultSuccess !=
                VendorTagManager::QueryVendorTagLocation("com.qti.chi.statsSkip",
                    "skipFrame", &pStatsNode->m_inputSkipFrameTag))
            {
                CAMX_LOG_ERROR(CamxLogGroupStats, "Failed to query stats skip vendor tag");
            }
        }

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "StatsProcessingNode::Create failed - out of memory");
        result = CamxResultENoMemory;
    }

    CAMX_LOG_INFO(CamxLogGroupStats, "StatsProcessingNode::Create result = %d", result);

    return pStatsNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsProcessingNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessingNode::ProcessingNodeInitialize(
    const NodeCreateInputData*  pCreateInputData,
    NodeCreateOutputData*       pCreateOutputData)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupStats, SCOPEEventStatsProcessingNodeProcessingNodeInitialize);
    CamxResult result = CamxResultSuccess;

    m_pChiContext                   = pCreateInputData->pChiContext;
    pCreateOutputData->pNodeName    = m_pNodeName;

    m_bufferOffset = GetMaximumPipelineDelay();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::ProcessingNodeFinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessingNode::ProcessingNodeFinalizeInitialization(
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
// StatsProcessingNode::GetPropertyDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessingNode::GetPropertyDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsProcessRequestDataInfo
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);
    CamxResult result = CamxResultSuccess;

    if ((NULL != pExecuteProcessRequestData) && (NULL != pStatsProcessRequestDataInfo))
    {
        NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
        StatsDependency         statsDependency  = { 0 };

        if ((NULL != pNodeRequestData) && (NULL != m_pStatsProcessorManager))
        {
            pStatsProcessRequestDataInfo->pDependencyUnit = &pNodeRequestData->dependencyInfo[0];

            // Check property dependencies
            result = m_pStatsProcessorManager->GetDependencies(pStatsProcessRequestDataInfo, &statsDependency);

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
                        CAMX_LOG_VERBOSE(CamxLogGroupStats, "property: %08x is not published in the pipeline count %d ",
                            statsDependency.properties[i].property,
                            pNodeRequestData->dependencyInfo[0].propertyDependency.count);
                        pNodeRequestData->dependencyInfo[0].propertyDependency.count--;
                    }
                }
                if (pNodeRequestData->dependencyInfo[0].propertyDependency.count > 1)
                {
                    // Update dependency request data for topology to consume
                    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency             = TRUE;
                    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency = TRUE;
                    pNodeRequestData->dependencyInfo[0].processSequenceId                                 = 1;
                    pNodeRequestData->numDependencyLists                                                  = 1;
                }
            }
        }
        else
        {

            CAMX_LOG_ERROR(CamxLogGroupStats, "NULL pointer Access: pNodeRequestData:%p m_pStatsProcessorManager:%p",
                pNodeRequestData, m_pStatsProcessorManager);
            result = CamxResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "NULL pointer Access: pExecuteProcessRequestData:%p "
            "pStatsProcessRequestDataInfo:%p",
            pExecuteProcessRequestData, pStatsProcessRequestDataInfo);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::GetBufferDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessingNode::GetBufferDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);
    CamxResult result = CamxResultSuccess;

    if (NULL != pExecuteProcessRequestData)
    {
        PerRequestActivePorts*  pEnabledPorts    = pExecuteProcessRequestData->pEnabledPortsInfo;
        NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;

        if ((NULL != pNodeRequestData) && (NULL != pEnabledPorts))
        {
            PropertyDependency* pDeps = &pNodeRequestData->dependencyInfo[0].propertyDependency;

            for (UINT portIndex = 0; portIndex < pEnabledPorts->numInputPorts; portIndex++)
            {
                PerRequestInputPortInfo* pPerRequestInputPort = &pEnabledPorts->pInputPorts[portIndex];

                if (NULL == pPerRequestInputPort)
                {
                    CAMX_LOG_ERROR(CamxLogGroupStats, "pPerRequestInputPort is NULL for portIndex: %d",
                        portIndex);
                    result = CamxResultEFailed;
                    break;
                }

                switch (pPerRequestInputPort->portId)
                {
                    case StatsInputPortBHist:
                        pDeps->properties[pDeps->count++] = PropertyIDParsedBHistStatsOutput;
                        break;
                    case StatsInputPortIHist:
                        pDeps->properties[pDeps->count++] = PropertyIDParsedIHistStatsOutput;
                        break;
                    case StatsInputPortHDRBE:
                        pDeps->properties[pDeps->count++] = PropertyIDParsedHDRBEStatsOutput;
                        break;
                    case StatsInputPortHDRBHist:
                        pDeps->properties[pDeps->count++] = PropertyIDParsedHDRBHistStatsOutput;
                        break;
                    case StatsInputPortBPSRegYUV:
                    case StatsInputPortRS:
                        pDeps->properties[pDeps->count++] = PropertyIDParsedRSStatsOutput;
                        break;
                    case StatsInputPortRDIStats:
                        if (TRUE == m_isVideoHDREnabled)
                        {
                            pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[0]         =
                            pEnabledPorts->pInputPorts[portIndex].phFence;
                            pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[0] =
                            pEnabledPorts->pInputPorts[portIndex].pIsFenceSignaled;
                            pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount          = 1;

                            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasInputBuffersReadyDependency    = TRUE;
                            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency = TRUE;
                            pNodeRequestData->dependencyInfo[0].processSequenceId                                 = 1;
                        }
                        break;
                    default:
                        CAMX_LOG_ERROR(CamxLogGroupStats, "Need to add new stats support");
                        break;
                }
            }

            if (CamxResultSuccess == result)
            {
                if (pDeps->count > 0)
                {
                    pNodeRequestData->dependencyInfo[0].processSequenceId                                 = 1;
                    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency = TRUE;
                    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency             = TRUE;
                    pNodeRequestData->numDependencyLists                                                  = 1;
                }
            }
        }
        else
        {

            CAMX_LOG_ERROR(CamxLogGroupStats, "NULL pointer Access: pNodeRequestData:%p pEnabledPorts:%p",
                pNodeRequestData, pEnabledPorts);
            result = CamxResultEFailed;
        }
    }
    else
    {

        CAMX_LOG_ERROR(CamxLogGroupStats, "pExecuteProcessRequestData is NULL");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::GetMultiStatsDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessingNode::GetMultiStatsDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsProcessRequestDataInfo
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);
    CamxResult result = CamxResultSuccess;

    if ((NULL != pExecuteProcessRequestData) && (NULL != pStatsProcessRequestDataInfo))
    {
        NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
        DependencyUnit*         pDependencyUnit  = &(pNodeRequestData->dependencyInfo[0]);

        if ( TRUE == m_statsInitializeData.pPipeline->IsMultiCamera() &&
            (MultiCamera3ASyncDisabled != GetStaticSettings()->multiCamera3ASync))
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
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "pExecuteProcessRequestData/pStatsProcessRequestDataInfo is NULL");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::PrepareStatsProcessRequestData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessingNode::PrepareStatsProcessRequestData(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsProcessRequestData)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    CamxResult              result           = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*  pEnabledPorts    = pExecuteProcessRequestData->pEnabledPortsInfo;

    if ((NULL != pNodeRequestData) && (NULL != pEnabledPorts) && (NULL != pNodeRequestData->pCaptureRequest))
    {
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
                    CAMX_LOG_ERROR(CamxLogGroupStats, "pPerRequestInputPort is NULL");
                    result = CamxResultEFailed;
                    break;
                }

                pStatsProcessRequestData->bufferInfo[i].statsType = StatsUtil::GetStatsType(pPerRequestInputPort->portId);

                if (StatsInputPortRDIStats == pPerRequestInputPort->portId)
                {
                    pStatsProcessRequestData->bufferInfo[i].pBuffer  = pPerRequestInputPort->pImageBuffer;
                    pStatsProcessRequestData->bufferInfo[i].phFences = pPerRequestInputPort->phFence;
                }
            }

            if (CamxResultSuccess == result)
            {
                pStatsProcessRequestData->bufferCount = pEnabledPorts->numInputPorts;

                if (NULL != pExecuteProcessRequestData->pTuningModeData)
                {
                    pStatsProcessRequestData->pTuningModeData = pExecuteProcessRequestData->pTuningModeData;
                }
            }
        }

        if (TRUE == m_statsInitializeData.pPipeline->IsMultiCamera() &&
            (MultiCamera3ASyncDisabled != GetStaticSettings()->multiCamera3ASync))
        {
            UINT   tag[1]              = { 0 };
            VOID*  pData[1]            = { 0 };
            UINT   length              = CAMX_ARRAY_SIZE(tag);
            UINT64 configDataOffset[1] = { 0 };

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

                CAMX_LOG_VERBOSE(CamxLogGroupStats, "StatsNode CameraInfo ReqId:%llu CamId:%d Role:%-7s",
                    pStatsProcessRequestData->requestId,
                    pStatsProcessRequestData->cameraInfo.cameraId,
                    StatsUtil::GetRoleName(pStatsProcessRequestData->cameraInfo.algoRole));

                m_cameraInfo = pStatsProcessRequestData->cameraInfo;
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupStats, "Multi camera metadata not published for request %llu",
                    pStatsProcessRequestData->requestId);
                pStatsProcessRequestData->cameraInfo = m_cameraInfo;
            }
        }
        else
        {
            pStatsProcessRequestData->cameraInfo.algoRole   = StatsAlgoRoleDefault;
            pStatsProcessRequestData->cameraInfo.cameraId   = GetPipeline()->GetCameraId();
        }
    }
    else
    {

        CAMX_LOG_ERROR(CamxLogGroupStats, "NULL pointer Access: pNodeRequestData:%p pEnabledPorts:%p",
            pNodeRequestData, pEnabledPorts);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessingNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupStats, SCOPEEventStatsProcessingNodeExecuteProcessRequest,
        pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId);

    CamxResult result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupStats,
                     "StatsProcessingNode execute request for id %llu seqId %d",
                     pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId,
                     pExecuteProcessRequestData->pNodeProcessRequestData->processSequenceId);

    StatsProcessRequestData statsProcessRequestData         = {0};
    NodeProcessRequestData* pNodeRequestData                = pExecuteProcessRequestData->pNodeProcessRequestData;

    CAMX_ASSERT(pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask == 0);

    result = PrepareStatsProcessRequestData(pExecuteProcessRequestData, &statsProcessRequestData);

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupStats, "Stats Node execute request for req %llu pipeline %d seqId %d",
            pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId,
            GetPipeline()->GetPipelineId(),
            pExecuteProcessRequestData->pNodeProcessRequestData->processSequenceId);

        if (0 == pExecuteProcessRequestData->pNodeProcessRequestData->processSequenceId)
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

        if (FALSE == Node::HasAnyDependency(pExecuteProcessRequestData->pNodeProcessRequestData->dependencyInfo))
        {
            if (TRUE == m_statsInitializeData.pPipeline->IsMultiCamera() &&
                (MultiCamera3ASyncDisabled != GetStaticSettings()->multiCamera3ASync) &&
                (NULL != m_pMultiStatsOperator))
            {
                statsProcessRequestData.algoAction = m_pMultiStatsOperator->GetStatsAlgoAction();
            }

            // Execute process Request of the job dispatcher
            result = m_pStatsProcessorManager->OnProcessRequest(&statsProcessRequestData);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Failed to PrepareStatsProcessRequestData");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessingNode::PostPipelineCreate()
{
    CamxResult              result                                   = CamxResultSuccess;
    CHIALGORITHMINTERFACE   chiAlgoInterface                         = { 0 };
    INT32                   cameraID                                 = GetPipeline()->GetCameraId();
    const                   ImageSensorModuleData* pSensorModuleData = GetHwContext()->GetImageSensorModuleData(cameraID);
    VOID*                   pStatsParseFuncPtr                       = NULL;

    // Store the required input information.
    m_statsInitializeData.pNode              = this;
    m_statsInitializeData.pStaticPool        = m_pChiContext->GetStaticMetadataPool(GetPipeline()->GetCameraId());
    m_statsInitializeData.pDebugDataPool     = GetPerFramePool(PoolType::PerFrameDebugData);
    m_statsInitializeData.pThreadManager     = GetThreadManager();
    m_statsInitializeData.pHwContext         = GetHwContext();
    m_statsInitializeData.pTuningDataManager = GetTuningDataManager();
    m_statsInitializeData.pPipeline          = GetPipeline();
    m_statsInitializeData.pNode              = this;

    // Remove the below hardcoding after testing
    m_statsInitializeData.statsStreamInitConfig.operationMode = StatsOperationModeNormal;
    StatsUtil::GetStatsStreamInitConfig(m_statsInitializeData.pPipeline->GetPerFramePool(PoolType::PerUsecase),
        &m_statsInitializeData.statsStreamInitConfig);

    // Initialize algo role as default
    // Multicamera vendor tags will be read later in PrepareStatsProcessRequestData
    m_statsInitializeData.cameraInfo.cameraId   = static_cast<UINT32>(cameraID);
    m_statsInitializeData.cameraInfo.algoRole   = StatsAlgoRoleDefault;

    m_pStatsProcessorManager->SetVideoHDRInformation(m_isVideoHDREnabled, m_HDR3ExposureType);

    if (TRUE == m_isVideoHDREnabled)
    {
        pStatsParseFuncPtr = pSensorModuleData->GetStatsParseFuncPtr();
        CAMX_LOG_INFO(CamxLogGroupStats, "sensor HDR Enabled: %d, StatsParseFuncPtr=%p",
            m_isVideoHDREnabled, pStatsParseFuncPtr);
        if (NULL != pStatsParseFuncPtr)
        {
            m_pStatsProcessorManager->SetStatsParseFuncPtr(pStatsParseFuncPtr);
        }
    }

    chiAlgoInterface.pGetVendorTagBase       = ChiStatsSession::GetVendorTagBase;
    chiAlgoInterface.pGetMetadata            = ChiStatsSession::FNGetMetadata;
    chiAlgoInterface.pSetMetaData            = ChiStatsSession::FNSetMetadata;
    chiAlgoInterface.pQueryVendorTagLocation = ChiStatsSession::QueryVendorTagLocation;
    chiAlgoInterface.size                    = sizeof(CHIALGORITHMINTERFACE);

    if ((NULL != m_statsInitializeData.initializecallback.pAECCallback) &&
        (NULL != m_statsInitializeData.initializecallback.pAECCallback->pfnSetAlgoInterface))
    {
        m_statsInitializeData.initializecallback.pAECCallback->pfnSetAlgoInterface(&chiAlgoInterface);
    }

    InitializePreviousSessionData();

    if (CamxResultSuccess == result)
    {
        // Initialize the stats process manager.
        result = m_pStatsProcessorManager->Initialize(&m_statsInitializeData);
    }

    // Handle multi camera usecase
    if (CamxResultSuccess == result)
    {
        result = InitializeMultiStats();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::InitializePreviousSessionData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsProcessingNode::InitializePreviousSessionData()
{
    /// @todo  (CAMX-523): Get the previous session data.
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::NotifyJobProcessRequestDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessingNode::NotifyJobProcessRequestDone(
    UINT64 requestId)
{
    CamxResult  result      = CamxResultSuccess;

    ProcessPartialMetadataDone(requestId);
    ProcessMetadataDone(requestId);
    ProcessRequestIdDone(requestId);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::CanSkipAlgoProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StatsProcessingNode::CanSkipAlgoProcessing(
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

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "skipFactor %d skipRequired %d iSForceSkip %d requestIdOffsetFromLastFlush %llu",
        skipFactor, skipRequired, iSForceSkip, requestIdOffsetFromLastFlush);

    return skipRequired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::InitializeMultiStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessingNode::InitializeMultiStats()
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
            CAMX_LOG_INFO(CamxLogGroupStats, "3A sync scheme selected: QTI");
            m_pMultiStatsOperator = CAMX_NEW QTIMultiStatsOperator;
        }
        else if (MultiCamera3ASyncSingleton == pStaticSettings->multiCamera3ASync)
        {
            CAMX_LOG_INFO(CamxLogGroupStats, "3A sync scheme selected: Singleton Algo");
            m_pMultiStatsOperator = CAMX_NEW SingletonStatsOperator;
        }
        else if (MultiCamera3ASyncDisabled == pStaticSettings->multiCamera3ASync)
        {
            CAMX_LOG_INFO(CamxLogGroupStats, "3A sync scheme selected: No 3A sync needed");
            m_pMultiStatsOperator = CAMX_NEW NoSyncStatsOperator;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupStats, "3A sync scheme selected: Invalid scheme");
            m_pMultiStatsOperator = CAMX_NEW NoSyncStatsOperator;
        }

        if (NULL != m_pMultiStatsOperator)
        {
            MultiStatsData multiStatsData;

            multiStatsData.algoSyncType     = StatsAlgoSyncType::StatsAlgoSyncTypeAEC;
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
// StatsProcessingNode::IsForceSkipRequested
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StatsProcessingNode::IsForceSkipRequested()
{
    UINT32   isForceSkip  = 0;

    if (0 != m_inputSkipFrameTag)
    {
        UINT tagReadInput[1]    = { m_inputSkipFrameTag | InputMetadataSectionMask };
        VOID* pDataFrameSkip[1] = { 0 };
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
/// StatsProcessingNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessingNode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_pStatsProcessorManager)
    {
        result = m_pStatsProcessorManager->GetPublishList(CAMX_ARRAY_SIZE(pPublistTagList->tagArray),
            pPublistTagList->tagArray,
            &pPublistTagList->tagCount,
            &pPublistTagList->partialTagCount);
    }
    else
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Stats processing manager not initialized");
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%d tags will be published", pPublistTagList->tagCount);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessingNode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsProcessingNode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    if (NULL == pBufferNegotiationData)
    {
        return;
    }

    UINT32    cameraID    = GetPipeline()->GetCameraId();
    const ImageSensorModuleData* pSensorModuleData      = GetHwContext()->GetImageSensorModuleData(cameraID);
    UINT                         currentMode            = 0;
    BOOL                         isSensorModeSupportHDR = FALSE;
    UINT                         inputIndex             = 0;

    m_isVideoHDREnabled     = FALSE;
    m_HDR3ExposureType      = HDR3ExposureTypeUnknown;

    static const UINT GetProps[] =
    {
        PropertyIDUsecaseSensorCurrentMode
    };

    static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GetPropsLength]   = { 0 };
    UINT64            offsets[GetPropsLength] = { 0 };

    GetDataList(GetProps, pData, offsets, GetPropsLength);

    if (NULL != pData[0])
    {
        currentMode = *reinterpret_cast<UINT*>(pData[0]);
        pSensorModuleData->Get3ExposureHDRInformation(currentMode,
                                                      &isSensorModeSupportHDR,
                                                      reinterpret_cast<HDR3ExposureTypeInfo*>(&m_HDR3ExposureType));
    }

    CAMX_LOG_INFO(CamxLogGroupStats, "currentMode = %d, isSensorModeSupportHDR = %d, m_HDR3ExposureType = %d",
        currentMode, isSensorModeSupportHDR, m_HDR3ExposureType);

    BOOL HDRPortEnabled = FALSE;
    UINT inputPortId[StatsInputPortMaxCount];
    UINT numInputPort   = 0;
    UINT inputPortIndex = 0;

    // Get the list of enabled input port IDs for stats processing node
    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    for (inputIndex = 0; inputIndex < numInputPort; inputIndex++)
    {
        // Check if HDR input port is enabled in the usecase
        if (StatsInputPortRDIStats == inputPortId[inputIndex])
        {
            HDRPortEnabled = TRUE;
            inputPortIndex = InputPortIndex(inputPortId[inputIndex]);
            CAMX_LOG_INFO(CamxLogGroupStats, "HDR port is enabled. inputPortIndex %d", inputPortIndex);
            break;
        }
    }

    if ((TRUE == isSensorModeSupportHDR) && (HDR3ExposureTypeUnknown != m_HDR3ExposureType) && (TRUE == HDRPortEnabled))
    {
        m_isVideoHDREnabled = TRUE;
    }

    if ((TRUE == HDRPortEnabled) && (FALSE == m_isVideoHDREnabled))
    {
        DisableInputOutputLink(inputPortIndex, TRUE);
        CAMX_LOG_INFO(CamxLogGroupStats, "Disable RDI Stats link");
    }

}



CAMX_NAMESPACE_END
