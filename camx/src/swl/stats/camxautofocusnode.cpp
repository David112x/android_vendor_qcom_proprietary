////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxautofocusnode.cpp
/// @brief AutoFocus node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxautofocusnode.h"
#include "camxatomic.h"
#include "camxcafstatsprocessor.h"
#include "camxpipeline.h"
#include "camxstatscommon.h"
#include "camxtrace.h"
#include "parametertuningtypes.h"
#include "camximagesensormoduledata.h"
#include "camximagesensordata.h"
#include "camxvendortags.h"
#include "camxtitan17xcontext.h"

CAMX_NAMESPACE_BEGIN

static const INT MaxAfPipelineDelay = 3;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AutoFocusNode* AutoFocusNode::Create(
    const NodeCreateInputData*  pCreateInputData,
    NodeCreateOutputData*       pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    CamxResult      result          = CamxResultSuccess;
    AutoFocusNode*  pAutoFocusNode  = CAMX_NEW AutoFocusNode();
    UINT32          propertyCount   = pCreateInputData->pNodeInfo->nodePropertyCount;

    if (NULL != pAutoFocusNode)
    {
        pAutoFocusNode->m_skipPattern = 1;

        for (UINT32 count = 0; count < propertyCount; count++)
        {
            if (NodePropertyStatsSkipPattern == pCreateInputData->pNodeInfo->pNodeProperties[count].id)
            {
                pAutoFocusNode->m_skipPattern = *static_cast<UINT*>(pCreateInputData->pNodeInfo->pNodeProperties[count].pValue);
            }
            if (NodePropertyEnableFOVC == pCreateInputData->pNodeInfo->pNodeProperties[count].id)
            {
                pAutoFocusNode->m_enableFOVC = *static_cast<BOOL*>(pCreateInputData->pNodeInfo->pNodeProperties[count].pValue);
            }
        }

        result = CAFStatsProcessor::Create(&(pAutoFocusNode->m_pAFStatsProcessor));
        if (CamxResultSuccess != result)
        {
            if (NULL != pAutoFocusNode->m_pAFStatsProcessor)
            {
                pAutoFocusNode->m_pAFStatsProcessor->Destroy();
                pAutoFocusNode->m_pAFStatsProcessor = NULL;
            }

            CAMX_DELETE pAutoFocusNode;
            pAutoFocusNode = NULL;
        }
        else
        {
            if (NULL != pCreateInputData->pAFAlgoCallbacks)
            {
                pAutoFocusNode->m_pStatscallback.pAFCCallback = pCreateInputData->pAFAlgoCallbacks;
            }
            else
            {
                pAutoFocusNode->m_pStatscallback.pAFCCallback = NULL;
            }

            if (NULL != pCreateInputData->pPDLibCallbacks)
            {
                pAutoFocusNode->m_pStatscallback.pPDCallback = pCreateInputData->pPDLibCallbacks;
            }
            else
            {
                pAutoFocusNode->m_pStatscallback.pPDCallback = NULL;
            }

            pAutoFocusNode->m_inputSkipFrameTag = 0;
            if (CDKResultSuccess !=
                VendorTagManager::QueryVendorTagLocation("com.qti.chi.statsSkip", "skipFrame",
                                                         &pAutoFocusNode->m_inputSkipFrameTag))
            {
                CAMX_LOG_ERROR(CamxLogGroupAF, "Failed to query stats skip vendor tag");
            }

            pAutoFocusNode->m_FOVCFrameControlTag = 0;
            if (CDKResultSuccess !=
                VendorTagManager::QueryVendorTagLocation("org.quic.camera2.statsconfigs", "FOVCFrameControl",
                                                         &pAutoFocusNode->m_FOVCFrameControlTag))
            {
                CAMX_LOG_ERROR(CamxLogGroupStats, "Failed to query FOVC control vendor tag");
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAF, "Create failed - out of memory");
        result = CamxResultENoMemory;
    }

    return pAutoFocusNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AutoFocusNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::AutoFocusNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AutoFocusNode::AutoFocusNode()
    : m_pAFStatsProcessor(NULL)
    , m_pAFAlgorithmHandler(NULL)
    , m_pAFIOUtil(NULL)
    , m_pMultiStatsOperator(NULL)
    , m_pPDAFEnablementConditions(NULL)
{
    m_pNodeName                  = "AutoFocus";
    m_derivedNodeHandlesMetaDone = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::~AutoFocusNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AutoFocusNode::~AutoFocusNode()
{
    // Destroy all the created objects.
    CAMX_LOG_INFO(CamxLogGroupAF, "AutoFocusNode::~AutoFocusNode()");

    if (NULL != m_pAFStatsProcessor)
    {
        m_pAFStatsProcessor->Destroy();
        m_pAFStatsProcessor = NULL;
    }

    if (NULL != m_pAFIOUtil)
    {
        CAMX_DELETE m_pAFIOUtil;
        m_pAFIOUtil = NULL;
    }

    if (NULL != m_pAFAlgorithmHandler)
    {
        CAMX_DELETE m_pAFAlgorithmHandler;
        m_pAFAlgorithmHandler = NULL;
    }

    if (NULL != m_pMultiStatsOperator)
    {
        CAMX_DELETE m_pMultiStatsOperator;
        m_pMultiStatsOperator = NULL;
    }

    if (NULL != m_pPDAFEnablementConditions)
    {
        CAMX_DELETE m_pPDAFEnablementConditions;
        m_pPDAFEnablementConditions = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AutoFocusNode::ProcessingNodeInitialize(
    const NodeCreateInputData*  pCreateInputData,
    NodeCreateOutputData*       pCreateOutputData)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupAF, SCOPEEventAutoFocusNodeProcessingNodeInitialize);

    CamxResult result = CamxResultSuccess;

    m_pChiContext                = pCreateInputData->pChiContext;
    pCreateOutputData->pNodeName = m_pNodeName;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::ProcessingNodeFinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AutoFocusNode::ProcessingNodeFinalizeInitialization(
    FinalizeInitializationData* pFinalizeInitializationData)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupAF, SCOPEEventAutoFocusNodeProcessingNodeInitialize);

    CamxResult result = CamxResultSuccess;

    // Store the required input information.
    m_autoFocusInitializeData.pStaticPool        = m_pChiContext->GetStaticMetadataPool(GetPipeline()->GetCameraId());
    m_autoFocusInitializeData.pDebugDataPool     = pFinalizeInitializationData->pDebugDataPool;
    m_autoFocusInitializeData.pThreadManager     = pFinalizeInitializationData->pThreadManager;
    m_autoFocusInitializeData.pHwContext         = pFinalizeInitializationData->pHwContext;
    m_autoFocusInitializeData.pPipeline          = GetPipeline();
    m_autoFocusInitializeData.pTuningDataManager = GetPipeline()->GetTuningDataManager();
    m_autoFocusInitializeData.initializecallback = m_pStatscallback;
    m_bufferOffset                               = GetMaximumPipelineDelay() - 1;

    if (m_bufferOffset < MaxAfPipelineDelay)
    {
        m_bufferOffset = MaxAfPipelineDelay;
    }

    UINT inputPortId[StatsInputPortMaxCount];
    UINT numInputPort = 0;

    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
    {
        // need request - 3 buffer
        SetInputPortBufferDelta(inputIndex, m_bufferOffset);
    }

    m_pPDAFEnablementConditions = CAMX_NEW PDAFEnablementConditions();

    if (NULL == m_pPDAFEnablementConditions)
    {
        CAMX_LOG_ERROR(CamxLogGroupAF, "m_pPDAFEnablementConditions Create failed - out of memory");
        result = CamxResultENoMemory;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AutoFocusNode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    CAMX_ASSERT(NULL != pBufferNegotiationData);
    UINT32                       cameraID           = GetPipeline()->GetCameraId();
    const ImageSensorModuleData* pSensorModuleData  = GetHwContext()->GetImageSensorModuleData(cameraID);
    UINT                         currentMode        = 0;

    m_isFixedFocus = (pSensorModuleData->GetActuatorDataObject() == NULL) ? TRUE : FALSE;

    static const UINT GetProps[] =
    {
        PropertyIDUsecaseSensorCurrentMode,
        PropertyIDUsecaseIFEPDHWInfo
    };

    static const UINT GetPropsLength            = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GetPropsLength]     = { 0 };
    UINT64            offsets[GetPropsLength]   = { 0 };

    GetDataList(GetProps, pData, offsets, GetPropsLength);

    if (NULL != pData[0])
    {
        currentMode = *reinterpret_cast<UINT*>(pData[0]);
        BOOL isCurrentSensorModeSupportedPDAF   = FALSE;
        PDLibSensorType pdafType                = PDLibSensorInvalid;
        pSensorModuleData->GetPDAFInformation(currentMode, &isCurrentSensorModeSupportedPDAF,
            reinterpret_cast<PDAFType*>(&pdafType));
        m_pPDAFEnablementConditions->SetCurrentSensorModeSupportedPDAF(isCurrentSensorModeSupportedPDAF);
        m_pPDAFEnablementConditions->SetPDAFType(pdafType);
    }

    if (NULL != pData[1])
    {
        PDHwAvailablity* pPDHwAvailablity = reinterpret_cast<PDHwAvailablity*>(pData[1]);
        m_pPDAFEnablementConditions->SetIFESupportedPDHW(pPDHwAvailablity);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAF, "PropertyIDUsecaseIFEPDHWInfo hasn't been published by IFE");
        PDHwAvailablity PDHwAvailablityInfo = { FALSE, FALSE, FALSE };
        m_pPDAFEnablementConditions->SetIFESupportedPDHW(&PDHwAvailablityInfo);
    }

    // Disable unwanted pdaf ports
    // fetch other pdaf conditions
    m_pPDAFEnablementConditions->SetPdafEnableFromSetting(GetStaticSettings()->disablePDAF);
    m_pPDAFEnablementConditions->SetLCREnableFromSetting(GetStaticSettings()->disablePDLibLCR);
    m_pPDAFEnablementConditions->SetPDAFHWEnableMask(GetStaticSettings()->pdafHWEnable);
    m_pPDAFEnablementConditions->SetPDCallBack(m_pStatscallback.pPDCallback);

    // if PDAF is enabled from camxsettings
    UINT inputPortId[StatsInputPortMaxCount];
    UINT numInputPort = 0;

    // Get the list of enabled input port IDs for AF node
    GetAllInputPortIds(&numInputPort, &inputPortId[0]);
    PDLibSensorType sensorPDType = m_pPDAFEnablementConditions->GetPDAFType();

    if (m_pPDAFEnablementConditions->IsPDAFEnabledFromSetting())
    {
        // Check if PDAF ports are enabled for current usecase
        for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
        {
            // Check if appropriate PDAF port for current sensor type is defined for current usecase
            if (TRUE == m_pPDAFEnablementConditions->IsPDAFPortDefinedForSensorType(sensorPDType, inputPortId[inputIndex]))
            {
                m_pPDAFEnablementConditions->SetPDAFPortEnabled(TRUE);

                if (inputPortId[inputIndex] == StatsInputPortDualPDHWPDAF)
                {
                    m_pPDAFEnablementConditions->SetDualPDHWPortEnabled(TRUE);
                    m_pPDAFEnablementConditions->SetSparsePDHWPortEnabled(TRUE);
                }
            }

            if (StatsInputPortLCRHW == inputPortId[inputIndex])
            {
                m_pPDAFEnablementConditions->SetLCRHWPortEnabled(TRUE);
            }
        }

        // check if PDAF is enabled, disable unneeded ports/links from current usecase
        if (TRUE == m_pPDAFEnablementConditions->IsPDAFEnabled())
        {
            // Tuning is not available here
            DisableUnwantedPDHWLCRLink(&numInputPort, &inputPortId[0]);

            if (TRUE == m_pPDAFEnablementConditions->IsSparsePDHWAvailableInTarget())
            {
                // If sparse PD HW is available in target, PDAF out from CAMIF(T3 port) is not Supported in target.
                // This means irrespective of the sensor type, Type3 port needs to be disabled
                DisableUnwantedPDAFLink(&numInputPort, &inputPortId[0], DisableType3);
            }
            else // Sparse PD HW is not available on the target
            {
                if ((PDLibSensorDualPD == sensorPDType) || (PDLibSensorType2 == sensorPDType) ||
                    (PDLibSensorType1 == sensorPDType))
                {
                    // If sensor is T1, T2 or dual PD, T3 port is disabled.
                    DisableUnwantedPDAFLink(&numInputPort, &inputPortId[0], DisableType3);
                }
                else if (PDLibSensorType3 == sensorPDType)
                {
                    // Since the sensor is Type3, disable Type2 port
                    DisableUnwantedPDAFLink(&numInputPort, &inputPortId[0], DisableType2);
                }
            }
        }
    }

    if (FALSE == m_pPDAFEnablementConditions->IsPDAFEnabled())
    {
        // PDAF is disabled in override setting or sensor does not support PDAF
        // Disable any PDAF links present in the use case
        DisableUnwantedPDAFLink(&numInputPort, &inputPortId[0], DisableALL);
    }

    // Publish PDHWEnableConditions information for other nodes
    PDHWEnableConditions pdHWEnableConditions;
    m_pPDAFEnablementConditions->GetPDHWEnabled(&pdHWEnableConditions);
    static const UINT WriteProps[] =
    {
        PropertyIDUsecasePDLibInputPDHWEnableConditions
    };

    UINT pDataCount[CAMX_ARRAY_SIZE(WriteProps)] =
    {
        sizeof(PDHWEnableConditions)
    };

    const VOID* pPDHWEnableConditionsData[CAMX_ARRAY_SIZE(WriteProps)] = { &pdHWEnableConditions };

    WriteDataList(WriteProps, pPDHWEnableConditionsData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::IsControlAFTriggerPresent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AutoFocusNode::IsControlAFTriggerPresent()
{
    static const UINT       GetProps[]              = { InputControlAFTrigger };
    static const UINT       GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
    VOID*                   pData[GetPropsLength]   = { 0 };
    UINT64                  offsets[GetPropsLength] = { 0 };
    ControlAFTriggerValues  controlAFTriggerValues;
    BOOL                    isAFTriggerPresent      = FALSE;

    m_autoFocusInitializeData.pNode->GetDataList(GetProps, pData, offsets, GetPropsLength);

    controlAFTriggerValues = *reinterpret_cast<ControlAFTriggerValues*>(pData[0]);
    if (ControlAFTriggerStart == controlAFTriggerValues ||
        ControlAFTriggerCancel == controlAFTriggerValues)
    {
        isAFTriggerPresent = TRUE;
    }

    return isAFTriggerPresent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AutoFocusNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    UINT64 requestId                    = pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId;
    UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);

    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupAF,
                            SCOPEEventAutoFocusNodeExecuteProcessRequest, requestId);

    CAMX_ASSERT(NULL != pExecuteProcessRequestData);

    CAMX_LOG_VERBOSE(CamxLogGroupAF,
        "AF Node execute request for req %llu pipeline %d seqId %d requestIdOffsetFromLastFlush %llu",
        requestId, GetPipeline()->GetPipelineId(),
        pExecuteProcessRequestData->pNodeProcessRequestData->processSequenceId, requestIdOffsetFromLastFlush);

    if (PerRequestNodeStatus::Success == GetRequestStatus(requestId))
    {
        CAMX_LOG_INFO(CamxLogGroupAF, "Node: %s, enter again after processDone, Skip this reqid %llu",
                       NodeIdentifierString(), requestId);
        return CamxResultSuccess;
    }

    CamxResult              result                      = CamxResultSuccess;
    StatsProcessRequestData autoFocusProcessRequestData = { 0 };
    BOOL                    FOVCEnabled                 = FALSE;

    // Prepare AF process request data.
    PrepareAFProcessRequestData(pExecuteProcessRequestData, &autoFocusProcessRequestData);

    if (pExecuteProcessRequestData->pNodeProcessRequestData->processSequenceId == AddDependencies)
    {
        // Initialize number of dependency lists to 0
        pExecuteProcessRequestData->pNodeProcessRequestData->numDependencyLists = 0;

        result = GetPropertyDependencies(pExecuteProcessRequestData,
            &autoFocusProcessRequestData);

        // Process all property dependencies before buffers so that the PDAF dep can be adjusted
        if (CamxResultSuccess == result)
        {
            result = GetMultiStatsDependencies(pExecuteProcessRequestData, &autoFocusProcessRequestData);
        }

        if ((CamxResultSuccess == result) && (FALSE == autoFocusProcessRequestData.skipProcessing))
        {
            result = GetBufferDependencies(pExecuteProcessRequestData);
        }

        NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;

        // If PDAF is the only port without property dependencies then move its dependency down
        if (0 != pNodeRequestData->dependencyInfo[1].dependencyFlags.dependencyFlagsMask)
        {
            if (0 == pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask)
            {
                Utils::Memcpy(&pNodeRequestData->dependencyInfo[0],
                            &pNodeRequestData->dependencyInfo[1],
                            sizeof(pNodeRequestData->dependencyInfo[1]));
                pNodeRequestData->numDependencyLists = 1;
            }
            else
            {
                pNodeRequestData->numDependencyLists = 2;
            }
        }
        else if (0 != pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask)
        {
            pNodeRequestData->numDependencyLists = 1;
        }
    }

    // This check takes care of overridesettings & operation mode together for FOVC.
    if (TRUE == m_enableFOVC)
    {
        FOVCEnabled = TRUE;
    }

    if (FALSE == Node::HasAnyDependency(pExecuteProcessRequestData->pNodeProcessRequestData->dependencyInfo))
    {
        BOOL processingDone = FALSE;
        INT  sequenceId     = pExecuteProcessRequestData->pNodeProcessRequestData->processSequenceId;
        if ((TRUE == FOVCEnabled) && ((FOVCCopyDependency == sequenceId) || (-1 == sequenceId)))
        {
            // Copy data for FOVC. If preempted before future requests arrive, use current data, NULL checked
            UINT64            offset                  = (-1 == sequenceId) ? 0 : m_bufferOffset - 1;
            UINT              GetProps[]              = { PropertyIDAFFrameInfo };
            static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
            VOID*             pData[GetPropsLength]   = { 0 };
            UINT64            offsets[GetPropsLength] = { offset };
            BOOL              negate[GetPropsLength]  = { TRUE };

            GetDataListFromPipeline(GetProps, pData, offsets, GetPropsLength, negate, GetPipelineId());

            UINT        WriteProps[]                             = { PropertyIDFOVCFrameInfo };
            UINT        pDataCount[CAMX_ARRAY_SIZE(WriteProps)]  = { sizeof(FOVCOutput) };
            FOVCOutput  output                                   = { 0 };
            const VOID* pOutputData[CAMX_ARRAY_SIZE(WriteProps)] = { &output };

            if (NULL != pData[0])
            {
                output = reinterpret_cast<AFFrameInformation*>(pData[0])->fovcOutput;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupAF, "Failed to find valid AFFrameInfo from frame offset %llu", offset);
            }

            WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
            if (0 != m_FOVCFrameControlTag)
            {
                WriteProps[0] = m_FOVCFrameControlTag;
                WriteDataList(WriteProps, pOutputData, pDataCount, CAMX_ARRAY_SIZE(WriteProps));
            }

            processingDone = TRUE;
        }
        else
        {
            if ((TRUE == m_autoFocusInitializeData.pPipeline->IsMultiCamera()) && (NULL != m_pMultiStatsOperator) &&
                (MultiCamera3ASyncDisabled != GetStaticSettings()->multiCamera3ASync))
            {
                autoFocusProcessRequestData.algoAction = m_pMultiStatsOperator->GetStatsAlgoAction();
            }

            // Execute process Request of the job dispatcher
            result = m_pAFStatsProcessor->ExecuteProcessRequest(&autoFocusProcessRequestData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupAF, "Execute Process Request failed: %s", Utils::CamxResultToString(result));
            }

            /// @note the first m_bufferOffset requests have no dependencies
            if (m_bufferOffset >= requestIdOffsetFromLastFlush)
            {
                processingDone = TRUE;
            }
            // First requests don't populate AFFrameInfo for us to wait on
            else if (BAFStatsDependencyMet == sequenceId)
            {
                // As soon as AF Node receives sequenceId 1, AF node Publishes PropertyIDAFBAFDependencyMet.
                m_pAFIOUtil->PublishBAFStatDependencyMet(&autoFocusProcessRequestData);

                // This assumes we'll always have BAF. If only PDAF is used this code needs populated in the PDAF path mutually
                // exclusively with the BAF path (only one dependency for this node with FOVCCopyDependency should be set up)
                if (TRUE == FOVCEnabled)
                {
                    // Add dependency in future to copy FOVC data
                    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;

                    pNodeRequestData->dependencyInfo[0].propertyDependency.properties[
                        pNodeRequestData->dependencyInfo[0].propertyDependency.count]   = PropertyIDAFFrameInfo;
                    pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[
                        pNodeRequestData->dependencyInfo[0].propertyDependency.count]   = m_bufferOffset - 1;
                    pNodeRequestData->dependencyInfo[0].propertyDependency.negate[
                        pNodeRequestData->dependencyInfo[0].propertyDependency.count++] = TRUE;

                    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency             = TRUE;
                    pNodeRequestData->dependencyInfo[0].dependencyFlags.isPreemptable                     = TRUE;
                    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency = TRUE;
                    pNodeRequestData->dependencyInfo[0].processSequenceId                                 = FOVCCopyDependency;
                    pNodeRequestData->numDependencyLists                                                  = 1;
                }
                // BAF completion marks request completion if PDAF is disabled
                else if (FALSE == m_pPDAFEnablementConditions->IsPDAFEnabled())
                {
                    processingDone = TRUE;
                }
            }
            // If PDAF is enabled, PDAF will mark completion, and may race against BAF completion (needs attention)
            else if ((TRUE  == m_pPDAFEnablementConditions->IsPDAFEnabled()) &&
                    (sequenceId == PDStatDependencyMet) &&
                    (FALSE == FOVCEnabled))
            {
                processingDone = TRUE;
            }
        }

        if (TRUE == processingDone || (TRUE == autoFocusProcessRequestData.skipProcessing))
        {
            NotifyJobProcessRequestDone(&autoFocusProcessRequestData);

            CAMX_LOG_VERBOSE(CamxLogGroupAF, "requestId(%llu) Process Seq: %d", requestId,
                pExecuteProcessRequestData->pNodeProcessRequestData->processSequenceId);

            CAMX_LOG_VERBOSE(CamxLogGroupAF, "ProcessRequestDone %llu", requestId - 1);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AutoFocusNode::PostPipelineCreate()
{
    CamxResult            result            = CamxResultSuccess;
    CAFStatsProcessor*    pAFStatsProcessor = static_cast<CAFStatsProcessor*>(m_pAFStatsProcessor);

    if (NULL == pAFStatsProcessor)
    {
        CAMX_LOG_ERROR(CamxLogGroupAF, "pAFStatsProcessor is NULL");
        result = CamxResultEInvalidPointer;
    }

    if (FALSE == GetPipeline()->HasStatsNode())
    {
        m_autoFocusInitializeData.isStatsNodeAvailable = FALSE;
    }
    else
    {
        m_autoFocusInitializeData.isStatsNodeAvailable = TRUE;
    }

    m_autoFocusInitializeData.pNode = this;
    m_autoFocusInitializeData.pChiContext = m_pChiContext;

    if (CamxResultSuccess == result)
    {
        pAFStatsProcessor->SetAFPDAFInformation(m_pPDAFEnablementConditions);

        pAFStatsProcessor->SetFixedFocus(m_isFixedFocus);

        m_pAFIOUtil = CAMX_NEW CAFIOUtil();

        if (NULL == m_pAFIOUtil)
        {
            CAMX_LOG_ERROR(CamxLogGroupAF, "AutoFocusIOUtil Create failed - out of memory");
            result = CamxResultENoMemory;
        }
        else
        {
            result = m_pAFIOUtil->Initialize(&m_autoFocusInitializeData);

            m_pAFIOUtil->SetAFPDAFInformation(m_pPDAFEnablementConditions);

            pAFStatsProcessor->SetAFIOUtil(m_pAFIOUtil);
        }
    }

    if (CamxResultSuccess == result)
    {
        m_pAFAlgorithmHandler = CAMX_NEW CAFAlgorithmHandler();

        if (NULL == m_pAFAlgorithmHandler)
        {
            CAMX_LOG_ERROR(CamxLogGroupAF, "Algorithm Handler create Failed - out of memory");
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        pAFStatsProcessor->SetAFAlgorithmHandler(m_pAFAlgorithmHandler);

        if ((NULL != m_pStatscallback.pAFCCallback) &&
            (NULL != m_pStatscallback.pAFCCallback->pfnSetAlgoInterface))
        {
            CHIALGORITHMINTERFACE chiAlgoInterface;

            chiAlgoInterface.pGetVendorTagBase       = ChiStatsSession::GetVendorTagBase;
            chiAlgoInterface.pGetMetadata            = ChiStatsSession::FNGetMetadata;
            chiAlgoInterface.pSetMetaData            = ChiStatsSession::FNSetMetadata;
            chiAlgoInterface.pQueryVendorTagLocation = ChiStatsSession::QueryVendorTagLocation;
            chiAlgoInterface.size                    = sizeof(CHIALGORITHMINTERFACE);

            m_pStatscallback.pAFCCallback->pfnSetAlgoInterface(&chiAlgoInterface);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupAF,
                          "Could not set the Algo Interface. pAFCCallback: %p",
                          m_pStatscallback.pAFCCallback);
        }

        if ((NULL != m_pStatscallback.pPDCallback) &&
            (NULL != m_pStatscallback.pPDCallback->pfnSetAlgoInterface))
        {
            CHIALGORITHMINTERFACE chiAlgoInterface;

            chiAlgoInterface.pGetVendorTagBase = ChiStatsSession::GetVendorTagBase;
            chiAlgoInterface.pGetMetadata = ChiStatsSession::FNGetMetadata;
            chiAlgoInterface.pSetMetaData = ChiStatsSession::FNSetMetadata;
            chiAlgoInterface.pQueryVendorTagLocation = ChiStatsSession::QueryVendorTagLocation;
            chiAlgoInterface.size = sizeof(CHIALGORITHMINTERFACE);

            m_pStatscallback.pPDCallback->pfnSetAlgoInterface(&chiAlgoInterface);
        }

        // Initialize the autofocus processor.
        result = m_pAFStatsProcessor->Initialize(&m_autoFocusInitializeData);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupAF, "AF stats intialize failed");
        }
    }

    if (CamxResultSuccess == result)
    {
        result = InitializeMultiStats();
    }

    if (CamxResultSuccess == result && NULL != m_pPDAFEnablementConditions)
    {
        // Check if LCRHW is enabled based on all required conditions + tuning
        UINT inputPortId[StatsInputPortMaxCount];
        UINT numInputPort = 0;

        // Get the list of enabled input port IDs for AF node
        GetAllInputPortIds(&numInputPort, &inputPortId[0]);

        // At this stage tuning values are available
        DisableUnwantedPDHWLCRLink(&numInputPort, &inputPortId[0]);
        if (FALSE == m_pPDAFEnablementConditions->IsLCRSWEnabled())
        {
            // Disable LCR SW port after we get result From PDlib
            DisableUnwantedPDAFLink(&numInputPort, &inputPortId[0], DisableTypeRDIRaw);
        }

        m_pPDAFEnablementConditions->PrintPDHWEnableConditions();
        m_pPDAFEnablementConditions->PrintLCREnableConditions();

        m_pAFIOUtil->PublishPDHWEnableConditions();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::NotifyJobProcessRequestDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AutoFocusNode::NotifyJobProcessRequestDone(
    StatsProcessRequestData* pAutoFocusProcessRequestInfo)
{
    ProcessPartialMetadataDone(pAutoFocusProcessRequestInfo->requestId);
    ProcessMetadataDone(pAutoFocusProcessRequestInfo->requestId);
    ProcessRequestIdDone(pAutoFocusProcessRequestInfo->requestId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::GetPropertyDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AutoFocusNode::GetPropertyDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pAutoFocusProcessRequestDataInfo
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);
    CAMX_ASSERT_MESSAGE(NULL != pExecuteProcessRequestData, "pExecuteProcessRequestData NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pAutoFocusProcessRequestDataInfo, "pAutoFocusProcessRequestDataInfo NULL pointer");

    CamxResult              result              = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData    = pExecuteProcessRequestData->pNodeProcessRequestData;
    StatsDependency         statsDependency     = { 0 };

    CAMX_ASSERT_MESSAGE(NULL != pNodeRequestData, "Node capture request data NULL pointer");

    pAutoFocusProcessRequestDataInfo->pDependencyUnit   = &pNodeRequestData->dependencyInfo[0];

    // Check property dependencies
    result = m_pAFStatsProcessor->GetDependencies(pAutoFocusProcessRequestDataInfo, &statsDependency);

    // For First request (and first request after flush), we should not set any dependency and it should just be bypassed
    UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(pNodeRequestData->pCaptureRequest->requestId);

    if ((CamxResultSuccess == result) && (requestIdOffsetFromLastFlush > 1))
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

        if (TRUE == IsTagPresentInPublishList(PropertyIDAECInternal))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count]   = PropertyIDAECInternal;
            pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count++] = 1;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMeta, "PropertyIDAECInternal %08x is not published in the pipeline",
                PropertyIDAECInternal);
        }

        if (TRUE == IsTagPresentInPublishList(PropertyIDAECFrameInfo))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count] = PropertyIDAECFrameInfo;
            pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count++] = 1;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMeta, "PropertyIDAECFrameInfo %08x is not published in the pipeline",
                PropertyIDAECFrameInfo);
        }

        if (TRUE == IsTagPresentInPublishList(PropertyIDAFFrameControl))
        {
            // We set dependecy on previous AF frame because if we get AF request in wrong order AF state machine may go wrong
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count]   = PropertyIDAFFrameControl;
            pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count++] = 1;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMeta, "PropertyIDAFFrameControl %08x is not published in the pipeline",
                PropertyIDAFFrameControl);
        }

        if (TRUE == IsTagPresentInPublishList(PropertyIDParsedHDRBEStatsOutput))
        {
            // We set dependecy on previous frame ParsedHDRBEStats which is a full frame HDRBEStats
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count] = PropertyIDParsedHDRBEStatsOutput;
            pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[
                pNodeRequestData->dependencyInfo[0].propertyDependency.count++] = 1;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMeta, "PropertyIDParsedHDRBEStatsOutput %08x is not published in the pipeline",
                PropertyIDParsedHDRBEStatsOutput);
        }

        if (pNodeRequestData->dependencyInfo[0].propertyDependency.count > 1)
        {
            // Update dependency request data for topology to consume
            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency             = TRUE;
            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency = TRUE;
            pNodeRequestData->dependencyInfo[0].processSequenceId                                 = 1;
        }

        // If pdaf is enabled, add dependency on receiving baf stat(sequenceId1) first so pd stat(sequenceId2) will be
        // received after baf stat
        if (TRUE == m_pPDAFEnablementConditions->IsPDAFEnabled())
        {
            result = GetPDPropertyDependencies(pExecuteProcessRequestData, pAutoFocusProcessRequestDataInfo);
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAF, "AF node does not have any property dependency for Request ID %llu",
                         pNodeRequestData->pCaptureRequest->requestId);
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::GetPDPropertyDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AutoFocusNode::GetPDPropertyDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pAutoFocusProcessRequestDataInfo
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);
    CAMX_ASSERT_MESSAGE(NULL != pExecuteProcessRequestData, "pExecuteProcessRequestData NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pAutoFocusProcessRequestDataInfo, "pAutoFocusProcessRequestDataInfo NULL pointer");

    CamxResult              result              = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData    = pExecuteProcessRequestData->pNodeProcessRequestData;

    CAMX_ASSERT_MESSAGE(NULL != pNodeRequestData, "Node capture request data NULL pointer");

    PerRequestActivePorts* pEnabledPorts = pExecuteProcessRequestData->pEnabledPortsInfo;

    UINT64 requestId                    = pNodeRequestData->pCaptureRequest->requestId;
    UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);

    // Use default values for the first requests
    if (m_bufferOffset < requestIdOffsetFromLastFlush)
    {
        for (UINT i = 0; i < pEnabledPorts->numInputPorts; i++)
        {
            PerRequestInputPortInfo* pPerRequestInputPort = &pEnabledPorts->pInputPorts[i];
            CAMX_ASSERT_MESSAGE(NULL != pPerRequestInputPort, "Per Request Input PortNULL pointer");

            CAMX_ASSERT_MESSAGE(NULL != pPerRequestInputPort, "Per Request Port info a Null pointer for index: %d", i);

            switch (pPerRequestInputPort->portId)
            {
                // By adding dependency on PropertyIDAFBAFDependencyMet, all below stats
                // (PDAF Type2, Type3, SW/HW LCR, dual/sparse PD HW) will add their buffer dependencies after recivieng
                // BAF stat/when PropertyIDAFBAFDependencyMet gets published.
                case StatsInputPortPDAFType3:
                case StatsInputPortRDIPDAF:
                case StatsInputPortRDIRaw:
                case StatsInputPortDualPDHWPDAF:
                case StatsInputPortLCRHW:
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupAF, "Add dependency on PropertyIDAFBAFDependencyMet request : %llu",
                        requestId);
                    if (TRUE == IsTagPresentInPublishList(PropertyIDAFBAFDependencyMet))
                    {
                        // Add property dependencies
                        pNodeRequestData->dependencyInfo[1].propertyDependency.count        = 0;
                        pNodeRequestData->dependencyInfo[1].propertyDependency.properties[
                            pNodeRequestData->dependencyInfo[1].propertyDependency.count]   = PropertyIDAFBAFDependencyMet;
                        pNodeRequestData->dependencyInfo[1].propertyDependency.offsets[
                            pNodeRequestData->dependencyInfo[1].propertyDependency.count++] = 0;

                        // Update dependency request data for dependencyInfo[1]
                        pNodeRequestData->dependencyInfo[1].dependencyFlags.hasPropertyDependency = TRUE;
                        pNodeRequestData->dependencyInfo[1].dependencyFlags.isPreemptable         =
                            (m_enableFOVC == TRUE) ? TRUE : FALSE;
                        pNodeRequestData->dependencyInfo[1].processSequenceId                     = ADDPDStatDependency;
                    }
                    else
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "property: %08x is not published in the pipeline",
                            PropertyIDAFBAFDependencyMet);
                    }
                    break;
                }
                default:
                    CAMX_LOG_VERBOSE(CamxLogGroupAF, " Unhandled port %d", pPerRequestInputPort->portId);
                    break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::GetBufferDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AutoFocusNode::GetBufferDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData
    ) const
                {
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CAMX_ASSERT_MESSAGE(NULL != pExecuteProcessRequestData, "Execute Process Request Data NULL pointer");

    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    CamxResult result = CamxResultSuccess;
    PerRequestActivePorts* pEnabledPorts = pExecuteProcessRequestData->pEnabledPortsInfo;

    UINT64 requestId                    = pNodeRequestData->pCaptureRequest->requestId;
    UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);

    CAMX_ASSERT_MESSAGE(NULL != pNodeRequestData, "Node capture request data NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pEnabledPorts, "Per Request Active Ports NULL pointer");

    // Use default values for the first requests
    if (m_bufferOffset < requestIdOffsetFromLastFlush)
    {
        for (UINT i = 0; i < pEnabledPorts->numInputPorts; i++)
        {
            PerRequestInputPortInfo* pPerRequestInputPort = &pEnabledPorts->pInputPorts[i];
            CAMX_ASSERT_MESSAGE(NULL != pPerRequestInputPort, "Per Request Input PortNULL pointer");

            CAMX_ASSERT_MESSAGE(NULL != pPerRequestInputPort, "Per Request Port info a Null pointer for index: %d", i);

            switch (pPerRequestInputPort->portId)
            {
                case StatsInputPortBF:
                {
                    pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[0] =
                        pPerRequestInputPort->phFence;
                    pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[0] =
                        pPerRequestInputPort->pIsFenceSignaled;
                    pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount = 1;

                    // BF Stats
                    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasInputBuffersReadyDependency      = TRUE;
                    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;
                    pNodeRequestData->dependencyInfo[0].processSequenceId                                   =
                        AddBAFStatsDependency;

                    break;
                }
                // Add buffer dependency for PDAF Type2, Type3, SW/HW LCR and dual/sparse PD HW
                case StatsInputPortPDAFType3:
                case StatsInputPortRDIPDAF:
                case StatsInputPortRDIRaw:
                case StatsInputPortDualPDHWPDAF:
                case StatsInputPortLCRHW:
                {
                    if (TRUE == m_pPDAFEnablementConditions->IsPDAFEnabled())
                    {
                        UINT32 count = pNodeRequestData->dependencyInfo[1].bufferDependency.fenceCount;
                        pNodeRequestData->dependencyInfo[1].bufferDependency.phFences[count] =
                            pPerRequestInputPort->phFence;
                        pNodeRequestData->dependencyInfo[1].bufferDependency.pIsFenceSignaled[count] =
                            pPerRequestInputPort->pIsFenceSignaled;
                        pNodeRequestData->dependencyInfo[1].bufferDependency.fenceCount++;

                        pNodeRequestData->dependencyInfo[1].dependencyFlags.hasInputBuffersReadyDependency      = TRUE;
                        pNodeRequestData->dependencyInfo[1].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;
                        pNodeRequestData->dependencyInfo[1].dependencyFlags.isPreemptable         =
                            (m_enableFOVC == TRUE) ? TRUE : FALSE;
                        pNodeRequestData->dependencyInfo[1].processSequenceId                                   =
                            ADDPDStatDependency;
                    }
                    break;
                }
                case StatsInputPortBPSRegYUV:
                {
                    pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[0] =
                        pPerRequestInputPort->phFence;
                    pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[0] =
                        pPerRequestInputPort->pIsFenceSignaled;
                    pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount = 1;

                    // YUV Buffer
                    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasInputBuffersReadyDependency      = TRUE;
                    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;
                    pNodeRequestData->dependencyInfo[0].processSequenceId                                   =
                        AddBAFStatsDependency;

                    break;
                }

                default:
                    CAMX_LOG_ERROR(CamxLogGroupAF, " Unhandled port %d", pPerRequestInputPort->portId);
                    break;
            }

            CAMX_LOG_INFO(CamxLogGroupAF,
                "AF buffer dependencies requestId %llu processSequenceId %d needBuffers %d Fence (%08x)",
                          requestId,
                          pNodeRequestData->dependencyInfo[i].processSequenceId,
                          pNodeRequestData->dependencyInfo[i].dependencyFlags.hasIOBufferAvailabilityDependency,
                          *pPerRequestInputPort->phFence);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::GetMultiStatsDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AutoFocusNode::GetMultiStatsDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pAutoFocusProcessRequestDataInfo
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);
    CAMX_ASSERT_MESSAGE(NULL != pExecuteProcessRequestData, "Execute Process Request Data NULL pointer");

    CamxResult              result              = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData    = pExecuteProcessRequestData->pNodeProcessRequestData;
    DependencyUnit*         pDependencyUnit     = &(pNodeRequestData->dependencyInfo[0]);

    if ( TRUE == m_autoFocusInitializeData.pPipeline->IsMultiCamera() &&
        (MultiCamera3ASyncDisabled != GetStaticSettings()->multiCamera3ASync))
    {
        if (NULL != m_pMultiStatsOperator)
        {
            UINT64 requestIdOffsetFromLastFlush =
                GetRequestIdOffsetFromLastFlush(pNodeRequestData->pCaptureRequest->requestId);

            result = m_pMultiStatsOperator->UpdateStatsDependencies(
                pExecuteProcessRequestData,
                pAutoFocusProcessRequestDataInfo,
                requestIdOffsetFromLastFlush);

            if (CamxResultSuccess == result && TRUE == pDependencyUnit->dependencyFlags.hasPropertyDependency)
            {
                if (0 == pDependencyUnit->processSequenceId)
                {
                    pDependencyUnit->processSequenceId                                  = 1;
                    pDependencyUnit->dependencyFlags.hasIOBufferAvailabilityDependency  = TRUE;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::PrepareAFProcessRequestData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AutoFocusNode::PrepareAFProcessRequestData(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pAutoFocusProcessRequestData)
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CamxResult              result = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*  pEnabledPorts = pExecuteProcessRequestData->pEnabledPortsInfo;
    UINT32                  bufferCount = 0;

    CAMX_ASSERT_MESSAGE(NULL != pNodeRequestData, "Node request data NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pEnabledPorts, "Per Request Active Ports NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pNodeRequestData->pCaptureRequest, "Node capture request data NULL pointer");

    pAutoFocusProcessRequestData->skipProcessing    = CanSkipAlgoProcessing(pNodeRequestData->pCaptureRequest->requestId);
    pAutoFocusProcessRequestData->requestId         = pNodeRequestData->pCaptureRequest->requestId;
    pAutoFocusProcessRequestData->processSequenceId = pNodeRequestData->processSequenceId;
    pAutoFocusProcessRequestData->pMultiRequestSync = pNodeRequestData->pCaptureRequest->pMultiRequestData;
    pAutoFocusProcessRequestData->peerSyncInfo      = pNodeRequestData->pCaptureRequest->peerSyncInfo;
    pAutoFocusProcessRequestData->pNode             = this;
    pAutoFocusProcessRequestData->algoAction        = StatsAlgoProcessRequest;

    if (TRUE == pAutoFocusProcessRequestData->skipProcessing &&
        ControlAFTriggerIdle != IsControlAFTriggerPresent())
    {
        pAutoFocusProcessRequestData->skipProcessing = FALSE;
    }
    if (FALSE == pAutoFocusProcessRequestData->skipProcessing)
    {
        for (UINT i = 0; i < pEnabledPorts->numInputPorts; i++)
        {
            PerRequestInputPortInfo* pPerRequestInputPort = &pEnabledPorts->pInputPorts[i];

            CAMX_ASSERT_MESSAGE(NULL != pPerRequestInputPort, "Per Request port info a Null pointer for index: %d", i);

            pAutoFocusProcessRequestData->bufferInfo[bufferCount].statsType =
                StatsUtil::GetStatsType(pPerRequestInputPort->portId);
            pAutoFocusProcessRequestData->bufferInfo[bufferCount].pBuffer   = pPerRequestInputPort->pImageBuffer;
            pAutoFocusProcessRequestData->bufferInfo[bufferCount].phFences  = pPerRequestInputPort->phFence;
            bufferCount++;
        }

        pAutoFocusProcessRequestData->bufferCount = bufferCount;

        if (NULL != pExecuteProcessRequestData->pTuningModeData)
        {
            pAutoFocusProcessRequestData->pTuningModeData = pExecuteProcessRequestData->pTuningModeData;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::CanSkipAlgoProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AutoFocusNode::CanSkipAlgoProcessing(
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

    CAMX_LOG_VERBOSE(CamxLogGroupAF, "skipFactor %d skipRequired %d iSForceSkip %d requestIdOffsetFromLastFlush %llu",
        skipFactor, skipRequired, iSForceSkip, requestIdOffsetFromLastFlush);

    return skipRequired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::InitializeMultiStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AutoFocusNode::InitializeMultiStats()
{
    CamxResult            result                = CamxResultSuccess;
    const StaticSettings* pStaticSettings       = GetStaticSettings();
    BOOL                  isMultiCameraRunning  = FALSE;

    if (TRUE == m_autoFocusInitializeData.pPipeline->IsMultiCamera())
    {
        isMultiCameraRunning = TRUE;
    }

    if (TRUE == isMultiCameraRunning)
    {
        if (MultiCamera3ASyncQTI == pStaticSettings->multiCamera3ASync)
        {
            CAMX_LOG_INFO(CamxLogGroupAF, "3A sync scheme selected: QTI");
            m_pMultiStatsOperator = CAMX_NEW QTIMultiStatsOperator;
        }
        else if (MultiCamera3ASyncSingleton == pStaticSettings->multiCamera3ASync)
        {
            CAMX_LOG_INFO(CamxLogGroupAF, "3A sync scheme selected: Singleton Algo");
            m_pMultiStatsOperator = CAMX_NEW SingletonStatsOperator;
        }
        else if (MultiCamera3ASyncDisabled == pStaticSettings->multiCamera3ASync)
        {
            CAMX_LOG_INFO(CamxLogGroupAF, "3A sync scheme selected: No 3A sync needed");
            m_pMultiStatsOperator = CAMX_NEW NoSyncStatsOperator;
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupAF, "3A sync scheme selected: Invalid scheme");
            m_pMultiStatsOperator = CAMX_NEW NoSyncStatsOperator;
        }

        if (NULL != m_pMultiStatsOperator)
        {
            MultiStatsData multiStatsData;

            multiStatsData.algoSyncType     = StatsAlgoSyncType::StatsAlgoSyncTypeAF;
            multiStatsData.algoRole         = StatsAlgoRole::StatsAlgoRoleDefault;
            multiStatsData.pipelineId       = m_autoFocusInitializeData.pPipeline->GetPipelineId();
            multiStatsData.pHwContext       = m_autoFocusInitializeData.pHwContext;
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
// AutoFocusNode::DisableUnwantedPDHWLCRLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AutoFocusNode::DisableUnwantedPDHWLCRLink(
    UINT*           pNumPorts,
    UINT*           pPortIds)
{
    // If both dual/sparse PD HW are disabled
    if (FALSE == m_pPDAFEnablementConditions->IsPDHWEnabled())
    {
        DisableUnwantedPDAFLink(pNumPorts, pPortIds, DisableTypePDHW);
    }

    if (FALSE == m_pPDAFEnablementConditions->IsLCRHWEnabled())
    {
        DisableUnwantedPDAFLink(pNumPorts, pPortIds, DisableTypeLCRHW);

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::DisableUnwantedPDAFLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AutoFocusNode::DisableUnwantedPDAFLink(
    UINT* pNumPorts,
    UINT* pPortIds,
    PDAFDisableType PDAFLinkDisableType)
{
    for (UINT inputIndex = 0; inputIndex < *pNumPorts; inputIndex++)
    {
        UINT inputPortIndex = InputPortIndex(pPortIds[inputIndex]);
        switch(PDAFLinkDisableType)
        {
            case DisableType2:
                if (pPortIds[inputIndex] == StatsInputPortRDIPDAF)
                {
                    CAMX_LOG_INFO(CamxLogGroupAF, "Disabling Type2 link %d inputPortIndex:%d",
                                  pPortIds[inputIndex],
                                  inputPortIndex);
                    DisableInputOutputLink(inputPortIndex, TRUE);
                }
                break;
            case DisableType3:
                if (pPortIds[inputIndex] == StatsInputPortPDAFType3)
                {
                    CAMX_LOG_INFO(CamxLogGroupAF, "Disabling PDAF Type3 link %d inputPortIndex:%d",
                                  pPortIds[inputIndex],
                                  inputPortIndex);
                    DisableInputOutputLink(inputPortIndex, TRUE);
                }
                break;
            case DisableTypePDHW:
                if (pPortIds[inputIndex] == StatsInputPortDualPDHWPDAF)
                {
                    CAMX_LOG_INFO(CamxLogGroupAF, "Disabling PDAF HW link (StatsInputPortDualPDHWPDAF) %d inputPortIndex:%d",
                                  pPortIds[inputIndex],
                                  inputPortIndex);
                    DisableInputOutputLink(inputPortIndex, TRUE);
                }
                break;
            case DisableTypeLCRHW:
                if (pPortIds[inputIndex] == StatsInputPortLCRHW)
                {
                    CAMX_LOG_INFO(CamxLogGroupAF, "Disabling LCR HW link (StatsInputPortLCRHW) %d inputPortIndex:%d",
                                   pPortIds[inputIndex],
                                   inputPortIndex);
                    DisableInputOutputLink(inputPortIndex, TRUE);
                }
                break;
            case DisableTypeRDIRaw:
                if (pPortIds[inputIndex] == StatsInputPortRDIRaw)
                {
                    CAMX_LOG_INFO(CamxLogGroupAF, "Disabling LCR SW link (StatsInputPortRDIRaw) %d inputPortIndex:%d",
                        pPortIds[inputIndex],
                        inputPortIndex);
                    DisableInputOutputLink(inputPortIndex, TRUE);
                }
                break;
            case DisableALL:
                if ((pPortIds[inputIndex] == StatsInputPortPDAFType3)    ||
                    (pPortIds[inputIndex] == StatsInputPortRDIPDAF)      ||
                    (pPortIds[inputIndex] == StatsInputPortDualPDHWPDAF) ||
                    (pPortIds[inputIndex] == StatsInputPortRDIRaw)       ||
                    (pPortIds[inputIndex] == StatsInputPortLCRHW))
                {
                    CAMX_LOG_INFO(CamxLogGroupAF, "Disabling All, PDAF link %d inputPortIndex:%d",
                                  pPortIds[inputIndex],
                                  inputPortIndex);
                    DisableInputOutputLink(inputPortIndex, TRUE);
                }
                break;
            default:
                CAMX_LOG_INFO(CamxLogGroupAF, "The link to be disabled for pdaf is wrong %d", PDAFLinkDisableType);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AutoFocusNode::IsForceSkipRequested
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AutoFocusNode::IsForceSkipRequested()
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
/// AutoFocusNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult AutoFocusNode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_pAFStatsProcessor)
    {
        result = m_pAFStatsProcessor->GetPublishList(CAMX_ARRAY_SIZE(pPublistTagList->tagArray),
                                                     pPublistTagList->tagArray,
                                                     &pPublistTagList->tagCount,
                                                     &pPublistTagList->partialTagCount);
    }
    else
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupMeta, "AF processor not initialized");
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%d tags will be published", pPublistTagList->tagCount);
    return result;
}

CAMX_NAMESPACE_END
