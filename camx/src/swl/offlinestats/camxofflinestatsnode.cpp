////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxofflinestatsnode.cpp
/// @brief Offline stats node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxmem.h"
#include "camxnode.h"
#include "camxistatsprocessor.h"
#include "camxofflinestatsnode.h"

CAMX_NAMESPACE_BEGIN

// @brief list of tags published
static const UINT OfflineStatsOutputTags[] =
{
    PropertyIDAWBFrameControl
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OfflineStatsNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OfflineStatsNode* OfflineStatsNode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    return CAMX_NEW OfflineStatsNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OfflineStatsNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineStatsNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OfflineStatsNode::OfflineStatsNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OfflineStatsNode::OfflineStatsNode()
{
    m_pNodeName = "OfflineStats";
    m_derivedNodeHandlesMetaDone = TRUE;

    /// @todo (CAMX-4346) Check this and change it to FALSE or even remove this conditional using of requests.
    //  For offline nodes, by the time we process current request, all stats buffers are available for this request,
    //  Doesn't really need to use Buffers from previous request. For now, Set to TRUE to maintain existing behavior.
    m_bUsePreviousRequestBuffers = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OfflineStatsNode::~OfflineStatsNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OfflineStatsNode::~OfflineStatsNode()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OfflineStatsNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CamxResult  result = CamxResultSuccess;

    if ((NULL == pCreateInputData) || (NULL == pCreateOutputData))
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "NULL  params pCreateInputData %p pCreateOutputData %p",
            pCreateInputData,
            pCreateOutputData);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        m_pStatsParser               = GetHwContext()->GetStatsParser();
        m_pChiContext                = pCreateInputData->pChiContext;
        pCreateOutputData->pNodeName = m_pNodeName;
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::CreateAWBAlgorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::CreateAWBAlgorithm()
{
    CamxResult  result = CamxResultSuccess;

    if (CamxResultSuccess == result)
    {
        const StaticSettings*   pStaticSettings  = GetHwContext()->GetStaticSettings();
        AWBAlgoCreateParamList  createParamList  = { 0 };
        AWBAlgoCreateParam      createParams[2];

        UINT* pOverrideCameraOpen           = (UINT *)&pStaticSettings->overrideCameraOpen;
        createParams[0].createParamType     = AWBCreateParamTypeCameraOpenIndicator;
        createParams[0].pCreateParam        = static_cast<VOID*>(pOverrideCameraOpen);
        createParams[0].sizeOfCreateParam   = sizeof(UINT);

        createParams[1].createParamType   = AWBCreateParamTypeSessionHandle;
        createParams[1].pCreateParam      = static_cast<VOID*>(&m_chiStatsSession);
        createParams[1].sizeOfCreateParam = sizeof(ChiStatsSession);

        createParamList.createParamsCount = sizeof(createParams) / sizeof(AWBAlgoCreateParam);
        createParamList.pCreateParams     = &createParams[0];

        if (NULL != m_statsInitializeData.initializecallback.pAWBCallback)
        {
            m_pfnCreate = m_statsInitializeData.initializecallback.pAWBCallback->pfnCreate;
        }

        CREATEAWB pFNCreateAWB = reinterpret_cast<CREATEAWB>(m_pfnCreate);
        result = (*pFNCreateAWB)(&createParamList, &m_pAWBAlgorithm);

        if ((CamxResultSuccess != result) || (NULL == m_pAWBAlgorithm))
        {
            result = CamxResultENoMemory;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupStats, "AWB algorithm created %d ", result);
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::PostPipelineCreate()
{
    CamxResult result = CamxResultSuccess;
    CHIALGORITHMINTERFACE   chiAlgoInterface    = { 0 };

    // Store the required input information.
    m_statsInitializeData.pNode       = this;
    m_statsInitializeData.pStaticPool = m_pChiContext->GetStaticMetadataPool(GetPipeline()->GetCameraId());
    m_statsInitializeData.pHwContext  = GetHwContext();
    m_statsInitializeData.pPipeline   = GetPipeline();


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

    m_chiStatsSession.Initialize(&m_statsInitializeData);

    result = CreateAWBAlgorithm();
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Failed to create AWB algorithm in OfflineStats");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
    CamxResult  result                     = CamxResultSuccess;

    if (NULL == pBufferNegotiationData)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "NULL params pBufferNegotiationData");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OfflineStatsNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CamxResult                result                  = CamxResultSuccess;
    NodeProcessRequestData*   pNodeRequestData        = NULL;
    PerRequestActivePorts*    pPerRequestPorts        = NULL;
    PerRequestOutputPortInfo* pOutputPort             = NULL;
    ImageBuffer*              pOutputImageBuffer      = NULL;
    CaptureRequest*           pCaptureRequest         = NULL;
    StatsProcessRequestData   statsProcessRequestData = { 0 };
    AWBAlgoInputList          algoInputList           = { 0 };
    AWBAlgoOutputList         algoOutputList          = { 0 };
    AWBAlgoSetParamList       algoSetParamList        = { 0 };
    INT                       sequenceNumber          = -1;
    AWBAlgoGetParam           algoGetParam            = {};
    UINT64                    frameNumber             = 0;

    CAMX_ENTRYEXIT(CamxLogGroupStats);
    CAMX_UNREFERENCED_PARAM(pOutputPort);
    CAMX_UNREFERENCED_PARAM(pOutputImageBuffer);

    if (NULL == pExecuteProcessRequestData)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "NULL params pExecuteProcessRequestData %p", pExecuteProcessRequestData);
        return CamxResultEInvalidPointer;
    }

    result = PrepareStatsProcessRequestData(pExecuteProcessRequestData, &statsProcessRequestData);

    if (CamxResultSuccess == result)
    {
        pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
        pPerRequestPorts = pExecuteProcessRequestData->pEnabledPortsInfo;

        if ((NULL == pNodeRequestData) || (NULL == pPerRequestPorts))
        {
            CAMX_LOG_ERROR(CamxLogGroupStats, "NULL params pNodeRequestData %p pPerRequestPorts %p",
                           pNodeRequestData,
                           pPerRequestPorts);
            return CamxResultEInvalidPointer;
        }
    }

    sequenceNumber = pNodeRequestData->processSequenceId;

    if (CamxResultSuccess == result)
    {
        pCaptureRequest = pNodeRequestData->pCaptureRequest;

        if (NULL == pCaptureRequest)
        {
            CAMX_LOG_ERROR(CamxLogGroupStats, "NULL params pCaptureRequest %p", pCaptureRequest);
            return CamxResultEInvalidPointer;
        }
        else
        {
            frameNumber = pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId;
        }
    }

    // Set buffer dep
    if ((CamxResultSuccess == result) && (0 == sequenceNumber))
    {
        SetDependencies(pNodeRequestData, pPerRequestPorts);
    }

    // Actually process data
    if (1 == sequenceNumber)
    {
        CAMX_ASSERT(NULL != m_pAWBAlgorithm);

        if (CamxResultSuccess == result)
        {
            result = GetAWBAlgoSetParamInput(&algoSetParamList);
        }

        if (CamxResultSuccess == result)
        {
            result = m_pAWBAlgorithm->AWBSetParam(m_pAWBAlgorithm, &algoSetParamList);
        }

        if (CamxResultSuccess == result)
        {
            result = GetAWBAlgoProcessInput(&statsProcessRequestData, &algoInputList);
        }

        if (CamxResultSuccess == result)
        {
            GetAWBAlgoExpectedOutputList(&algoOutputList);
        }

        // Process stats
        if (CamxResultSuccess == result)
        {
            result = m_pAWBAlgorithm->AWBProcess(m_pAWBAlgorithm, &algoInputList, &algoOutputList);
        }

        if (CamxResultSuccess == result)
        {
            PostMetadata();
            ProcessPartialMetadataDone(frameNumber);
            ProcessMetadataDone(frameNumber);

            if (TRUE == m_bUsePreviousRequestBuffers)
            {
                if (GetRequestIdOffsetFromLastFlush(frameNumber) > 1)
                {
                    ProcessRequestIdDone(frameNumber - 1);
                }
            }
            else
            {
                ProcessRequestIdDone(frameNumber);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::PostMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineStatsNode::PostMetadata()
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    AWBFrameControl   frameControl = { { 0 } };
    static const UINT OfflineStatsOutputData[] =
    {
        PropertyIDAWBFrameControl
    };
    static const UINT Length = CAMX_ARRAY_SIZE(OfflineStatsOutputData);
    const VOID* pData[Length]      = { 0 };
    UINT        pDataCount[Length] = { 0 };

    GetAWBFrameControl(&frameControl);

    pData[0] = &frameControl;

    pDataCount[0] = sizeof(frameControl);

    WriteDataList(OfflineStatsOutputData, pData, pDataCount, Length);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::ProcessingNodeFinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::ProcessingNodeFinalizeInitialization(
    FinalizeInitializationData* pFinalizeInitializationData)
{
    CAMX_UNREFERENCED_PARAM(pFinalizeInitializationData);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::AddChiStatsSessionHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::AddChiStatsSessionHandle(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoInputList*              pInputList)
{
    m_chiStatsSession.SetStatsProcessorRequestData(pStatsProcessRequestDataInfo);

    pInputList->pAWBInputs[AWBInputTypeStatsChiHandle].inputType      = AWBInputTypeStatsChiHandle;
    pInputList->pAWBInputs[AWBInputTypeStatsChiHandle].pAWBInput      = &m_chiStatsSession;
    pInputList->pAWBInputs[AWBInputTypeStatsChiHandle].sizeOfAWBInput = sizeof(ChiStatsSession);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::SetBufferDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineStatsNode::SetBufferDependency(
    NodeProcessRequestData* pNodeRequestData,
    PerRequestActivePorts*  pEnabledPorts)
{

    UINT fencesNotSignalled = 0;

    for (UINT i = 0; i < pEnabledPorts->numInputPorts; i++)
    {
        PerRequestInputPortInfo* pPort = &pEnabledPorts->pInputPorts[i];

        if (NULL != pPort)
        {
            UINT fenceCount = pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount;

            pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[fenceCount] = pPort->phFence;
            pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[fenceCount] = pPort->pIsFenceSignaled;
            pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount++;

            fencesNotSignalled++;
        }
    }

    if (pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount > 0)
    {
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasInputBuffersReadyDependency      = TRUE;
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;
        pNodeRequestData->dependencyInfo[0].processSequenceId                                   = 1;
        pNodeRequestData->numDependencyLists                                                    = 1;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineStatsNode::SetDependencies(
    NodeProcessRequestData*   pNodeRequestData,
    PerRequestActivePorts*    pEnabledPorts)
{
    SetBufferDependency(pNodeRequestData, pEnabledPorts);

    if (0 != pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask)
    {
        pNodeRequestData->numDependencyLists = 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::PrepareStatsProcessRequestData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::PrepareStatsProcessRequestData(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsProcessRequestData)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    CamxResult              result = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*  pEnabledPorts = pExecuteProcessRequestData->pEnabledPortsInfo;

    CAMX_ASSERT_MESSAGE(NULL != pNodeRequestData, "Node request data NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pEnabledPorts, "Per Request Active Ports NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pNodeRequestData->pCaptureRequest, "Node capture request data NULL pointer");

    UINT64 requestId = pNodeRequestData->pCaptureRequest->requestId;

    UINT64 slotId;

    if (TRUE == m_bUsePreviousRequestBuffers)
    {
        slotId = (2 > GetRequestIdOffsetFromLastFlush(requestId)) ? requestId : ((requestId - 1));
    }
    else
    {
        slotId = requestId;
    }

    if (0 == pNodeRequestData->processSequenceId)
    {
        m_procReqData[requestId].pEnabledPortsInfo = pExecuteProcessRequestData->pEnabledPortsInfo;
        m_procReqData[requestId].pNodeProcessRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    }

    if (NULL != m_procReqData[slotId].pEnabledPortsInfo)
    {
        // override the ExecuteProcessRequestData data to match the request providing the buffers for AF stats.  The rest
        // of the request data does not change.  This allows the dependencies to be set up with the existing code
        pEnabledPorts = m_procReqData[slotId].pEnabledPortsInfo;
    }

    for (UINT i = 0; i < pEnabledPorts->numInputPorts; i++)
    {
        PerRequestInputPortInfo* pPerRequestInputPort = &pEnabledPorts->pInputPorts[i];

        CAMX_ASSERT_MESSAGE(NULL != pPerRequestInputPort, "Per Request Input PortNULL pointer");

        pStatsProcessRequestData->bufferInfo[i].statsType = StatsUtil::GetStatsType(pPerRequestInputPort->portId);
        pStatsProcessRequestData->bufferInfo[i].pBuffer   = pPerRequestInputPort->pImageBuffer;
        pStatsProcessRequestData->bufferInfo[i].phFences  = pPerRequestInputPort->phFence;
    }

    pStatsProcessRequestData->requestId   = pNodeRequestData->pCaptureRequest->requestId;
    pStatsProcessRequestData->bufferCount = pEnabledPorts->numInputPorts;


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::GetAWBAlgoSetParamInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::GetAWBAlgoSetParamInput(
    AWBAlgoSetParamList* pInputList)
{
    CamxResult result = CamxResultSuccess;

    pInputList->pAWBSetParams    = m_setParamInputArray;
    pInputList->inputParamsCount = AWBSetParamTypeLastIndex;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::GetAWBAlgoProcessInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::GetAWBAlgoProcessInput(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoInputList*              pInputList)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    CamxResult result      = CamxResultSuccess;
    pInputList->pAWBInputs = m_processInputArray;
    pInputList->inputCount = AWBInputTypeLastIndex;

    result = GetAWBStatistics(pStatsProcessRequestDataInfo, pInputList);

    if (CamxResultSuccess == result)
    {
        result = AddChiStatsSessionHandle(pStatsProcessRequestDataInfo, pInputList);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::GetAWBAlgoExpectedOutputList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineStatsNode::GetAWBAlgoExpectedOutputList(
    AWBAlgoOutputList* pOutputList)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);

    pOutputList->pAWBOutputs                                                = m_processOutputArray;
    pOutputList->outputCount                                                = AWBOutputTypeLastIndex;

    pOutputList->pAWBOutputs[AWBOutputTypeGains].outputType                 = AWBOutputTypeGains;
    pOutputList->pAWBOutputs[AWBOutputTypeGains].pAWBOutput                 = &m_gains;
    pOutputList->pAWBOutputs[AWBOutputTypeGains].sizeOfAWBOutput            = sizeof(m_gains);

    pOutputList->pAWBOutputs[AWBOutputTypeColorTemperature].outputType      = AWBOutputTypeColorTemperature;
    pOutputList->pAWBOutputs[AWBOutputTypeColorTemperature].pAWBOutput      = &m_cct;
    pOutputList->pAWBOutputs[AWBOutputTypeColorTemperature].sizeOfAWBOutput = sizeof(m_cct);

    pOutputList->pAWBOutputs[AWBOutputTypeSampleDecision].outputType        = AWBOutputTypeSampleDecision;
    pOutputList->pAWBOutputs[AWBOutputTypeSampleDecision].pAWBOutput        = &m_sampleDecision;
    pOutputList->pAWBOutputs[AWBOutputTypeSampleDecision].sizeOfAWBOutput   = sizeof(m_sampleDecision);

    pOutputList->pAWBOutputs[AWBOutputTypeBGConfig].outputType              = AWBOutputTypeBGConfig;
    pOutputList->pAWBOutputs[AWBOutputTypeBGConfig].pAWBOutput              = &m_BGConfig;
    pOutputList->pAWBOutputs[AWBOutputTypeBGConfig].sizeOfAWBOutput         = sizeof(m_BGConfig);

    pOutputList->pAWBOutputs[AWBOutputTypeState].outputType                 = AWBOutputTypeState;
    pOutputList->pAWBOutputs[AWBOutputTypeState].pAWBOutput                 = &m_state;
    pOutputList->pAWBOutputs[AWBOutputTypeState].sizeOfAWBOutput            = sizeof(m_state);

    pOutputList->pAWBOutputs[AWBOutputTypeMode].outputType                  = AWBOutputTypeMode;
    pOutputList->pAWBOutputs[AWBOutputTypeMode].pAWBOutput                  = &m_mode;
    pOutputList->pAWBOutputs[AWBOutputTypeMode].sizeOfAWBOutput             = sizeof(m_mode);

    pOutputList->pAWBOutputs[AWBOutputTypeLock].outputType                  = AWBOutputTypeLock;
    pOutputList->pAWBOutputs[AWBOutputTypeLock].pAWBOutput                  = &m_lock;
    pOutputList->pAWBOutputs[AWBOutputTypeLock].sizeOfAWBOutput             = sizeof(m_lock);

    pOutputList->pAWBOutputs[AWBOutputTypeCCM].outputType                   = AWBOutputTypeCCM;
    pOutputList->pAWBOutputs[AWBOutputTypeCCM].pAWBOutput                   = &m_CCMList;
    pOutputList->pAWBOutputs[AWBOutputTypeCCM].sizeOfAWBOutput              = sizeof(m_CCMList);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::GetAWBStatistics
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::GetAWBStatistics(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoInputList*              pInputList)
{
    CamxResult result = ParseAWBStats(pStatsProcessRequestDataInfo);

    static const UINT GetProps[]              = { PropertyIDParsedAWBBGStatsOutput, PropertyIDISPAWBBGConfig };
    static const UINT GerPropsLength          = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GerPropsLength]   = { 0 };
    UINT64            offsets[GerPropsLength] = { 0, 0 };

    GetDataList(GetProps, pData, offsets, GerPropsLength);

    CAMX_ASSERT_MESSAGE(NULL != pData[0], "PropertyIDParsedAWBBGStatsOutput not published!");
    CAMX_ASSERT_MESSAGE(NULL != pData[1], "PropertyIDISPAWBBGConfig not published!");

    if ((NULL != pData[0]) && (NULL != pData[1]))
    {
        // Save the BG stats inside core's input structure
        SetAWBBayerGrid(reinterpret_cast<ParsedAWBBGStatsOutput*>(*static_cast<VOID**>(pData[0])),
                        reinterpret_cast<ISPAWBBGStatsConfig*>(pData[1]),
                        &m_statsBayerGrid);
    }
    pInputList->pAWBInputs[AWBInputTypeBGStats].inputType      = AWBInputTypeBGStats;
    pInputList->pAWBInputs[AWBInputTypeBGStats].pAWBInput      = &m_statsBayerGrid;
    pInputList->pAWBInputs[AWBInputTypeBGStats].sizeOfAWBInput = sizeof(StatsBayerGrid);

    // Indicating parsed stats are related to offline BPSAWBBG stats
    m_isOffline = TRUE;
    pInputList->pAWBInputs[AWBInputTypeIsOffline].inputType      = AWBInputTypeIsOffline;
    pInputList->pAWBInputs[AWBInputTypeIsOffline].pAWBInput      = &m_isOffline;
    pInputList->pAWBInputs[AWBInputTypeIsOffline].sizeOfAWBInput = sizeof(BOOL);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::ParseAWBStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::ParseAWBStats(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo)
{
    CamxResult result = CamxResultSuccess;

    for (INT32 i = 0; i < pStatsProcessRequestDataInfo->bufferCount; i++)
    {
        if (ISPStatsTypeAWBBG == pStatsProcessRequestDataInfo->bufferInfo[i].statsType)
        {
            const ImageFormat* pFormat    = pStatsProcessRequestDataInfo->bufferInfo[i].pBuffer->GetFormat();
            VOID*              pRawBuffer = pStatsProcessRequestDataInfo->bufferInfo[i].pBuffer->GetPlaneVirtualAddr(0, 0);

            result = pStatsProcessRequestDataInfo->bufferInfo[i].pBuffer->CacheOps(TRUE, FALSE);

            if (CamxResultSuccess == result)
            {
                ParseData parseData = {this, FALSE, pRawBuffer, NULL, pFormat};
                // gbergsch - Need to determine if this can truly depend on StatsParse or whether this class holds data
                result              = m_pStatsParser->Parse(ISPStatsTypeBPSAWBBG, &parseData);
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupStats, "Failed to parse ISP BG-stats result: %s",
                    Utils::CamxResultToString(result));
            }

            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::SetAWBBayerGrid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineStatsNode::SetAWBBayerGrid(
    ParsedAWBBGStatsOutput* pBayerGridOutput,
    ISPAWBBGStatsConfig*    pStatsConfig,
    StatsBayerGrid*         pBayerGrid)
{
    BGBEConfig* pBGConfig             = &pStatsConfig->AWBBGConfig;

    pBayerGrid->horizontalRegionCount = pBGConfig->horizontalNum;
    pBayerGrid->verticalRegionCount   = pBGConfig->verticalNum;
    pBayerGrid->totalRegionCount      = pBGConfig->horizontalNum * pBGConfig->verticalNum;
    pBayerGrid->regionWidth           = pStatsConfig->regionWidth;
    pBayerGrid->regionHeight          = pStatsConfig->regionHeight;
    pBayerGrid->bitDepth              = static_cast<UINT16>(pBGConfig->outputBitDepth);


    pBayerGrid->flags.hasSatInfo      = pBayerGridOutput->flags.hasSatInfo;
    pBayerGrid->flags.usesY           = pBayerGridOutput->flags.usesY;
    pBayerGrid->numValidRegions       = pBayerGridOutput->numROIs;
    pBayerGrid->SetChannelDataArray(reinterpret_cast<StatsBayerGridChannelInfo*>(pBayerGridOutput->GetChannelDataArray()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::GetAWBFrameControl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineStatsNode::GetAWBFrameControl(
    AWBFrameControl*   pFrameControl)
{
    // Set control gains
    pFrameControl->AWBGains.rGain = m_gains.red;
    pFrameControl->AWBGains.gGain = m_gains.green;
    pFrameControl->AWBGains.bGain = m_gains.blue;

    // Set Color temperature
    pFrameControl->colorTemperature = m_cct;

    // set CCM values
    pFrameControl->numValidCCMs = m_CCMList.numValidCCMs;

    for (UINT32 ccmIndex = 0; ccmIndex < pFrameControl->numValidCCMs; ccmIndex++)
    {
        pFrameControl->AWBCCM[ccmIndex].isCCMOverrideEnabled = m_CCMList.CCM[ccmIndex].isCCMOverrideEnabled;
        if (TRUE == pFrameControl->AWBCCM[ccmIndex].isCCMOverrideEnabled)
        {
            for (UINT i = 0; i < AWBNumCCMRows; i++)
            {
                for (UINT j = 0; j < AWBNumCCMCols; j++)
                {
                    pFrameControl->AWBCCM[ccmIndex].CCM[i][j] = m_CCMList.CCM[ccmIndex].CCM[i][j];
                }
                pFrameControl->AWBCCM[ccmIndex].CCMOffset[i] = m_CCMList.CCM[ccmIndex].CCMOffset[i];
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineStatsNode::FillBGConfigurationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID OfflineStatsNode::FillBGConfigurationData(
    AWBConfig*                                  pAWBConfigData,
    const StatsBayerGridBayerExposureConfig*    pAWBAlgoConfig)
{
    CAMX_ASSERT(NULL != pAWBConfigData);
    CAMX_ASSERT(NULL != pAWBAlgoConfig);

    pAWBConfigData->BGConfig.horizontalNum                        = pAWBAlgoConfig->horizontalRegionCount;
    pAWBConfigData->BGConfig.verticalNum                          = pAWBAlgoConfig->verticalRegionCount;
    pAWBConfigData->BGConfig.ROI.left                             = pAWBAlgoConfig->ROI.left;
    pAWBConfigData->BGConfig.ROI.top                              = pAWBAlgoConfig->ROI.top;
    pAWBConfigData->BGConfig.ROI.width                            = pAWBAlgoConfig->ROI.width;
    pAWBConfigData->BGConfig.ROI.height                           = pAWBAlgoConfig->ROI.height;
    pAWBConfigData->BGConfig.channelGainThreshold[ChannelIndexR]  = pAWBAlgoConfig->channelGainThreshold[StatsColorChannelR];
    pAWBConfigData->BGConfig.channelGainThreshold[ChannelIndexGR] = pAWBAlgoConfig->channelGainThreshold[StatsColorChannelGR];
    pAWBConfigData->BGConfig.channelGainThreshold[ChannelIndexGB] = pAWBAlgoConfig->channelGainThreshold[StatsColorChannelGB];
    pAWBConfigData->BGConfig.channelGainThreshold[ChannelIndexB]  = pAWBAlgoConfig->channelGainThreshold[StatsColorChannelB];
    pAWBConfigData->BGConfig.outputBitDepth                       = pAWBAlgoConfig->outputBitDepth;
    pAWBConfigData->BGConfig.outputMode                           = BGBERegular;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OfflineStatsNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OfflineStatsNode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    UINT32 numTags = CAMX_ARRAY_SIZE(OfflineStatsOutputTags);

    for (UINT32 tagIndex = 0; tagIndex < numTags; ++tagIndex)
    {
        pPublistTagList->tagArray[tagIndex] = OfflineStatsOutputTags[tagIndex];
    }

    pPublistTagList->tagCount = numTags;
    CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%u tags will be published", numTags);
    return CamxResultSuccess;
}

CAMX_NAMESPACE_END
