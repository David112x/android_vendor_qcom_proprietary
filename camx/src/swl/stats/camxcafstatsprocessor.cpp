////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcafstatsprocessor.cpp
/// @brief The class that implements IStatsProcessor for AF.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chistatsdebug.h"

#include "camxcafioutil.h"
#include "camxcafstatsprocessor.h"
#include "camxhal3defs.h"
#include "camxhal3module.h"
#include "camxmem.h"
#include "camxstatsdebuginternal.h"
#include "camxtrace.h"
#include "camxtuningdatamanager.h"

CAMX_NAMESPACE_BEGIN

static const CHAR* pHAFCreateFunctionName            = "CreateHAFAlgorithm";
static const CHAR* pDefaultHAFOverrideLibraryName    = "com.qti.stats.hafoverride";

// @brief Number of Partial Meta Data Tag from the Publish Tag list
static const UINT NumberOfPartialMetadataTags = 4;

// @brief list of tags published
static const UINT32 AFOutputMetadataTags[] =
{
    ControlAFState,
    ControlAFMode,
    ControlAFRegions,
    ControlAFTrigger,
    PropertyIDAFFrameControl,
    PropertyIDAFStatsControl,
    PropertyIDAFFrameInfo,
    PropertyIDAFPDFrameInfo,
    PropertyIDUsecaseAFFrameControl,
    PropertyIDUsecaseAFStatsControl,
    PropertyIDPDHwConfig,
    PropertyIDBasePDInternal,
    PropertyIDAFPeerInfo,
    PropertyIDAFBAFDependencyMet,
    PropertyIDFOVCFrameInfo,
};

// @brief list of vendor tags published
static const struct NodeVendorTag AFOutputVendorTags[] =
{
    { "org.quic.camera2.statsconfigs" , "AFFrameControl" },
    { "org.quic.camera.focusvalue"    , "FocusValue" },
    { "org.quic.camera2.statsconfigs" , "FOVCFrameControl" },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::Create(
    IStatsProcessor** ppAFStatsProcessor)
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CamxResult          result              = CamxResultSuccess;
    CAFStatsProcessor*  pAFStatsProcessor   = NULL;

    CAMX_ASSERT_MESSAGE(NULL != ppAFStatsProcessor, "Invalid arguments");

    pAFStatsProcessor = CAMX_NEW CAFStatsProcessor();

    if (NULL == pAFStatsProcessor)
    {
        CAMX_LOG_ERROR(CamxLogGroupAF, "Failed to create AFStatsProcessor");
        result = CamxResultENoMemory;
    }

    *ppAFStatsProcessor = pAFStatsProcessor;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::CAFStatsProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAFStatsProcessor::CAFStatsProcessor()
    : m_pAFAlgorithm(NULL)
    , m_pAFAlgorithmHandler(NULL)
    , m_pAFIOUtil(NULL)
    , m_hHandle(NULL)
    , m_isPDAFEnabled(FALSE)
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::VendorTagListAllocation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::VendorTagListAllocation()
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CamxResult      result          = CamxResultSuccess;
    AFAlgoGetParam  algoGetParam    = { { 0 } , { 0 } };

    // fetch vendor tag list
    m_pAFIOUtil->PopulateVendorTagGetParam(AFGetParamVendorTagList, &algoGetParam);
    CDKResult cdkResult = m_pAFAlgorithm->AFGetParam(m_pAFAlgorithm, &algoGetParam);
    result              = StatsUtil::CdkResultToCamXResult(cdkResult);

    if (CamxResultSuccess == result)
    {
        // allocate memory to hold each vendor tag's data
        result = m_pAFIOUtil->AllocateMemoryVendorTag(&algoGetParam);

        m_pOutputVendorTagInfoList = NULL;

        if (CamxResultSuccess == result)
        {
            for (UINT32 index = 0; index < algoGetParam.outputInfo.getParamOutputCount; ++index)
            {
                if (0 == algoGetParam.outputInfo.pGetParamOutputs[index].sizeOfWrittenGetParamOutput)
                {
                    continue;
                }
                // going through the list of vendor tag dependency
                if (AFGetParamVendorTagList == algoGetParam.outputInfo.pGetParamOutputs[index].getParamOutputType)
                {
                    m_pOutputVendorTagInfoList =
                        static_cast<StatsVendorTagInfoList*>(algoGetParam.outputInfo.pGetParamOutputs[index].pGetParamOutput);
                    break;
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupAF, "Cannot get publishlist");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::SetHardwareCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::SetHardwareCapability()
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CAMX_ASSERT(NULL != m_pAFIOUtil);
    CAMX_ASSERT(NULL != m_pAFAlgorithm);

    CamxResult result                                   = CamxResultSuccess;
    AFAlgoSetParam      setParam[AFSetParamLastIndex]   = { {0} };
    AFAlgoSetParamList  setParamList;
    StatsCameraInfo     cameraInfo;

    setParamList.numberOfSetParam   = 0;
    setParamList.pAFSetParams       = setParam;
    m_pAFIOUtil->GetCameraInfo(&cameraInfo);

    AFHardwareCapability* pHWCapability = m_pAFIOUtil->ReadHardwareCapability();
    setParam[setParamList.numberOfSetParam].pAFSetParam     = static_cast<VOID*>(pHWCapability);
    setParam[setParamList.numberOfSetParam].setParamType    = AFSetParamTypeHardwareCapabilities;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam  = sizeof(AFHardwareCapability);
    setParamList.numberOfSetParam                           = setParamList.numberOfSetParam + 1;
    setParam[setParamList.numberOfSetParam].pAFSetParam     = static_cast<VOID*>(&cameraInfo);
    setParam[setParamList.numberOfSetParam].setParamType    = AFSetParamTypeCameraInfo;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam  = sizeof(StatsCameraInfo);
    setParamList.numberOfSetParam                           = setParamList.numberOfSetParam + 1;


    CDKResult cdkResult = m_pAFAlgorithm->AFSetParam(m_pAFAlgorithm, &setParamList);
    result              = StatsUtil::CdkResultToCamXResult(cdkResult);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::SetDefaultSensorResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::SetDefaultSensorResolution()
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CAMX_ASSERT(NULL != m_pAFIOUtil);
    CAMX_ASSERT(NULL != m_pAFAlgorithm);

    CamxResult      result      = CamxResultSuccess;
    AFSensorInfo*   pSensorInfo = NULL;
    StatsCameraInfo cameraInfo;

    m_pAFIOUtil->GetCameraInfo(&cameraInfo);
    result = m_pAFIOUtil->GetDefaultSensorResolution(&pSensorInfo);
    if (CamxResultSuccess == result)
    {
        AFAlgoSetParam      setParam[AFSetParamLastIndex] = { {0} };
        AFAlgoSetParamList  setParamList;

        setParamList.numberOfSetParam                          = 0;
        setParamList.pAFSetParams                              = setParam;
        setParam[setParamList.numberOfSetParam].pAFSetParam    = static_cast<VOID*>(pSensorInfo);
        setParam[setParamList.numberOfSetParam].setParamType   = AFSetParamTypeSensorInfo;
        setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(AFSensorInfo);
        setParamList.numberOfSetParam                          = setParamList.numberOfSetParam + 1;
        setParam[setParamList.numberOfSetParam].pAFSetParam     = static_cast<VOID*>(&cameraInfo);
        setParam[setParamList.numberOfSetParam].setParamType    = AFSetParamTypeCameraInfo;
        setParam[setParamList.numberOfSetParam].sizeOfSetParam  = sizeof(StatsCameraInfo);
        setParamList.numberOfSetParam                           = setParamList.numberOfSetParam + 1;

        // Assuming the params are copied, so the pointer to local
        CDKResult cdkResult = m_pAFAlgorithm->AFSetParam(m_pAFAlgorithm, &setParamList);
        result = StatsUtil::CdkResultToCamXResult(cdkResult);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAF, "Failed to get the default sensor resolution");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::SetDefaultConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::SetDefaultConfig()
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CamxResult      result       = CamxResultSuccess;

    CAMX_ASSERT(NULL != m_pAFIOUtil);
    CAMX_ASSERT(NULL != m_pAFAlgorithm);

    result = SetDefaultSensorResolution();
    if (CamxResultSuccess == result)
    {
        result = SetHardwareCapability();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupAF, "SetHardwareCapability() failed!");
        }
    }

    if (( CamxResultSuccess == result) && (m_isPDAFEnabled == TRUE))
    {
        m_pAFIOUtil->LoadPDLibHandleFromUsecasePool();
    }

    // Get configuration based on default tuning.
    GetDefaultConfigFromAlgo();


    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::GetDefaultConfigFromAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::GetDefaultConfigFromAlgo()
{

    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CamxResult      result       = CamxResultSuccess;
    AFAlgoGetParam  algoGetParam = {};

    // Get default BAF config and lens position from algorithm and publish them for ISP & sensor
    m_pAFIOUtil->PopulateAFDefaultGetParamBuffers(&algoGetParam);

    CDKResult cdkResult = m_pAFAlgorithm->AFGetParam(m_pAFAlgorithm, &algoGetParam);
    result = StatsUtil::CdkResultToCamXResult(cdkResult);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAF, "AFGetParam() failed for the default parameters!");
    }

    if (CamxResultSuccess == result)
    {
        result = m_pAFIOUtil->PublishPreRequestOutput(&algoGetParam);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupAF, "Failed to publish the pre-request AF output");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::LoadHAFOverrideCreateFunction
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFStatsProcessor::LoadHAFOverrideCreateFunction()
{
    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    VOID* pAddr                           = NULL;
    const CHAR* pLibraryName              = NULL;
    const CHAR* pLibraryPath              = NULL;

    if (FALSE == pStaticSettings->enableCustomHAFAlgo)
    {
        pLibraryName = pDefaultHAFOverrideLibraryName;
        pLibraryPath = DefaultAlgorithmPath;
    }
    else
    {
        pLibraryName = pStaticSettings->customHAFAlgoName;
        pLibraryPath = pStaticSettings->customHAFAlgoPath;
    }

    pAddr = StatsUtil::LoadAlgorithmLib(&m_hHAFHandle, pLibraryPath, pLibraryName, pHAFCreateFunctionName);
    if (NULL == pAddr)
    {
        m_pHAFCreate = NULL;
    }
    else
    {
        m_pHAFCreate = reinterpret_cast<CREATEHAFALGORITHM>(pAddr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::Initialize(
    const StatsInitializeData* pInitializeData)
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CamxResult result = CamxResultSuccess;
    StatsCameraInfo       cameraInfo;

    CAMX_ASSERT(pInitializeData != NULL);
    CAMX_ASSERT(pInitializeData->pHwContext != NULL);
    CAMX_ASSERT(pInitializeData->pTuningDataManager);

    m_isStatsNodeAvailable = pInitializeData->isStatsNodeAvailable;
    m_pTuningDataManager   = pInitializeData->pTuningDataManager->GetChromatix();
    m_pNode                = pInitializeData->pNode;

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    InitializeCameraInfo(pInitializeData, &cameraInfo);
    m_cameraInfo = cameraInfo;

    result = UpdateAFSettings(TRUE);

    if (CamxResultSuccess == result)
    {
        AFAlgoCreateParamList createParamList                      = { 0 };
        StatsTuningData       statsTuningData                      = { 0 };
        AFAlgoCreateParam     createParams[AFCreateParamTypeCount] = {};
        AFCalibrationOTPData* pAfCalibrationOTPData                 = NULL;
        ChiTuningMode tuningSelectors[1];
        tuningSelectors[0].mode                                     = ChiModeType::Default;
        tuningSelectors[0].subMode                                  = { 0 };

        statsTuningData.numSelectors            = 1;
        statsTuningData.pTuningModeSelectors    = &tuningSelectors[0];
        statsTuningData.pTuningSetManager = pInitializeData->pTuningDataManager->GetChromatix();

        createParams[AFCreateParamTypeTuningParams].createParamType               = AFCreateParamTypeTuningParams;
        createParams[AFCreateParamTypeTuningParams].pParam                        = &statsTuningData;
        createParams[AFCreateParamTypeTuningParams].sizeOfParam                   = sizeof(statsTuningData);

        createParams[AFCreateParamTypeLogFunctionPtr].createParamType             = AFCreateParamTypeLogFunctionPtr;
        createParams[AFCreateParamTypeLogFunctionPtr].pParam                      =
            reinterpret_cast<VOID*>(StatsLoggerFunction);
        createParams[AFCreateParamTypeLogFunctionPtr].sizeOfParam                 = sizeof(VOID*);

        createParams[AFCreateParamTypeSettingsPtr].createParamType                = AFCreateParamTypeSettingsPtr;
        createParams[AFCreateParamTypeSettingsPtr].pParam                         = static_cast<VOID*>(&m_settingsInfo);
        createParams[AFCreateParamTypeSettingsPtr].sizeOfParam                    = sizeof(AFAlgoSettingInfo);

        LoadHAFOverrideCreateFunction();
        createParams[AFCreateParamTypeCreateCustomHAFFunctionPtr].createParamType =
            AFCreateParamTypeCreateCustomHAFFunctionPtr;
        createParams[AFCreateParamTypeCreateCustomHAFFunctionPtr].pParam          = reinterpret_cast<VOID*>(m_pHAFCreate);
        createParams[AFCreateParamTypeCreateCustomHAFFunctionPtr].sizeOfParam     = sizeof(VOID*);

        ChiStatsSession* pStatsSession                               = m_pAFIOUtil->GetChiStatsSession();
        createParams[AFCreateParamTypeSessionHandle].createParamType = AFCreateParamTypeSessionHandle;
        createParams[AFCreateParamTypeSessionHandle].pParam          = static_cast<VOID*>(pStatsSession);
        createParams[AFCreateParamTypeSessionHandle].sizeOfParam     = sizeof(ChiStatsSession);

        UINT* pOverrideCameraOpen                                          = (UINT *)&pStaticSettings->overrideCameraOpen;
        createParams[AFCreateParamTypeCameraOpenIndicator].createParamType = AFCreateParamTypeCameraOpenIndicator;
        createParams[AFCreateParamTypeCameraOpenIndicator].pParam          = static_cast<VOID*>(pOverrideCameraOpen);
        createParams[AFCreateParamTypeCameraOpenIndicator].sizeOfParam     = sizeof(UINT);

        createParams[AFCreateParamTypeCameraInfo].createParamType = AFCreateParamTypeCameraInfo;
        createParams[AFCreateParamTypeCameraInfo].pParam          = static_cast<VOID*>(&cameraInfo);
        createParams[AFCreateParamTypeCameraInfo].sizeOfParam     = sizeof(StatsCameraInfo);

        pAfCalibrationOTPData = m_pAFIOUtil->GetOTPData();
        createParams[AFCreateParamAFCalibrationOTPData].createParamType = AFCreateParamAFCalibrationOTPData;
        createParams[AFCreateParamAFCalibrationOTPData].pParam          = static_cast<VOID*>(pAfCalibrationOTPData);
        createParams[AFCreateParamAFCalibrationOTPData].sizeOfParam     = sizeof(AFCalibrationOTPData);

        createParamList.paramCount = AFCreateParamTypeCount;
        createParamList.pParamList = &createParams[0];

        // Create an instance of the core algorithm
        if (NULL != pInitializeData->initializecallback.pAFCCallback)
        {
            m_pCreate = pInitializeData->initializecallback.pAFCCallback->pfnCreate;
        }
        result = m_pAFAlgorithmHandler->CreateAlgorithm(&createParamList, &m_pAFAlgorithm, m_pCreate);
        CAMX_ASSERT(m_pAFAlgorithm != NULL);
    }

    if (CamxResultSuccess == result)
    {
        result = SetSingleParamToAlgorithm(
            AFSetParamTypeSettings, static_cast<VOID*>(&m_settingsInfo), sizeof(AFAlgoSettingInfo));
    }

    if (CamxResultSuccess == result)
    {
        result = SetDefaultConfig();
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagListAllocation();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupAF, "Failed to allocate the vendor tag list");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::InitializeCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::InitializeCameraInfo(
    const StatsInitializeData* pInitializeData,
    StatsCameraInfo* pStatsCameraInfo)
{
    CAMX_ASSERT(NULL != pInitializeData);
    CAMX_ASSERT(NULL != pInitializeData->pChiContext);
    UINT32 cameraId = static_cast <UINT32>(pInitializeData->pPipeline->GetCameraId());

    // During initialization, set algo role as default
    // Selected usecase will decide correct algo role later on
    pStatsCameraInfo->algoRole = StatsAlgoRoleDefault;
    pStatsCameraInfo->cameraId = cameraId;

    m_pAFIOUtil->SetCameraInfo(pStatsCameraInfo);
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::UpdateControlAFState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAFStatsProcessor::UpdateControlAFState(
    const AFAlgoOutputList* pOutput,
    AFHALOutputList*        pHALOutput)
{
    CAMX_ASSERT_MESSAGE(NULL != pOutput, "Output NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pHALOutput, "HAL output NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pOutput->pAFOutputs, "Output array NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pHALOutput->pAFOutputs, "HAL output array NULL pointer");

    AFAlgoStatusType status  = static_cast<AFAlgoStatusParam*>(pOutput->pAFOutputs[AFOutputTypeStatusParam].pAFOutput)->status;

    m_AFStateMachine.ProcessAFStatusUpdate(status);
    ControlAFStateValues* pState    =
        static_cast<ControlAFStateValues*>(pHALOutput->pAFOutputs[AFHALOutputTypeControlAFState].pAFOutput);
    *pState                         = m_AFStateMachine.GetControlAFState();

    CAMX_LOG_VERBOSE(CamxLogGroupAF, "AFOutput->status %d pState %d", status, *pState);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::UpdateControlLensDistance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAFStatsProcessor::UpdateControlLensDistance(
    const AFAlgoOutputList* pOutput,
    AFHALOutputList*        pHALOutput)
{
    CAMX_ASSERT_MESSAGE(NULL != pOutput, "Output NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pHALOutput, "HAL output NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pOutput->pAFOutputs, "Output array NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pHALOutput->pAFOutputs, "HAL output array NULL pointer");

    AFAlgoStatusParam* pAlgostatus = static_cast<AFAlgoStatusParam*>(pOutput->pAFOutputs[AFOutputTypeStatusParam].pAFOutput);

    AFFocusDistanceInfo* pLensDistance    =
        static_cast<AFFocusDistanceInfo*>(pHALOutput->pAFOutputs[AFHALOutputTypeLensFocusDistance].pAFOutput);
    pLensDistance->lensFocusDistance    = pAlgostatus->focusDistanceOptimal;

    CAMX_LOG_VERBOSE(CamxLogGroupAF, "LensFocusDistance %f", pLensDistance->lensFocusDistance);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::UpdateFocusMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFStatsProcessor::UpdateFocusMode(
    const AFAlgoOutputList* pOutput,
    AFHALOutputList*        pHALOutput)
{
    AFFocusMode* pMode = static_cast<AFFocusMode*>(pOutput->pAFOutputs[AFOutputTypeFocusMode].pAFOutput);
    ControlAFModeValues* pFocusMode =
                    static_cast<ControlAFModeValues*>(pHALOutput->pAFOutputs[AFHALOutputTypeControlFocusMode].pAFOutput);
    m_pAFIOUtil->MapFocusModeToHALType(pMode, pFocusMode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::UpdateFocusValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFStatsProcessor::UpdateFocusValue(
    const AFAlgoOutputList* pOutput,
    AFHALOutputList*        pHALOutput)
{
    *(static_cast<FLOAT *>(pHALOutput->pAFOutputs[AFHALOutputTypeFocusValue].pAFOutput)) =
                            *(static_cast<FLOAT *>(pOutput->pAFOutputs[AFOutputTypeFocusValue].pAFOutput));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::BuildHALOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::BuildHALOutput(
    const AFAlgoOutputList* pOutput,
    AFHALOutputList*        pHALOutput)
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT_MESSAGE(NULL != pOutput, "Output NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pHALOutput, "HAL output NULL pointer");

    UpdateControlAFState(pOutput, pHALOutput);
    UpdateFocusMode(pOutput, pHALOutput);
    UpdateControlLensDistance(pOutput, pHALOutput);
    UpdateFocusValue(pOutput, pHALOutput);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::ExecuteProcessRequest(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo)
{
    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupAF, SCOPEEventCAFStatsProcessorExecuteProcessRequest,
        pStatsProcessRequestDataInfo->requestId);

    CamxResult            result          = CamxResultSuccess;
    AFAlgoInputList       input           = { 0 };
    AFAlgoOutputList*     pOutput         = NULL;
    AFHALOutputList*      pHALOutput      = NULL;
    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    VOID*                 pPeer_info      = NULL;
    AFBAFStatistics       BFStats         = { 0 };
    StatsBayerGrid        BGStats;
    StatsCameraInfo       cameraInfo      = { StatsAlgoRoleDefault, 0 };
    UINT64 requestIdOffsetFromLastFlush   =
        m_pNode->GetRequestIdOffsetFromLastFlush(pStatsProcessRequestDataInfo->requestId);

    m_requestId                           = requestIdOffsetFromLastFlush;

    CAMX_LOG_INFO(CamxLogGroupAF, "AF Processor execute request for id %llu seqId %d requestIdOffsetFromLastFlush %llu",
                  pStatsProcessRequestDataInfo->requestId, pStatsProcessRequestDataInfo->processSequenceId,
                  requestIdOffsetFromLastFlush);

    CAMX_ASSERT_MESSAGE(NULL != m_pAFIOUtil, "AF IO Util NULL pointer");

    if (TRUE == pStatsProcessRequestDataInfo->skipProcessing )
    {
        CAMX_LOG_INFO(CamxLogGroupAF,
                         "Skipping AF Algo processing for RequestId: %llu",
                         pStatsProcessRequestDataInfo->requestId);

        result = m_pAFIOUtil->PublishSkippedFrameOutput();
        m_pAFIOUtil->PublishPeerFocusInfo(pPeer_info);
        m_pAFIOUtil->PublishCrossProperty(pStatsProcessRequestDataInfo);
    }
    else
    {
        if (TRUE == pStaticSettings->enableNCSService)
        {
            /// Get gyro data from NCS interface
            result = m_pAFIOUtil->PopulateGyroData();
            if (result == CamxResultEFailed)
            {
                /// In case of error, just print log and proceed
                CAMX_LOG_VERBOSE(CamxLogGroupAF, "Failed to populate Gyro data");
            }

            /// Get gravity data from NCS interface
            result = m_pAFIOUtil->PopulateGravityData();
            if (result == CamxResultEFailed)
            {
                /// In case of error, just print log and proceed
                CAMX_LOG_VERBOSE(CamxLogGroupAF, "Failed to populate Gravity data");
            }
            /// since we don't want to stop execution even if above fails, set result to CamxResultSuccess
            result = CamxResultSuccess;
        }

        if (TRUE == pStaticSettings->enableTOFInterface)
        {
            /// Get TOF data from TOF interface
            result = m_pAFIOUtil->PopulateTOFData(m_requestId);
            if (CamxResultEFailed == result)
            {
                /// In case of error, just print log and proceed
                CAMX_LOG_WARN(CamxLogGroupAF, "Failed to populate TOF data");
            }

            /// since we don't want to stop execution even if above fails, set result to CamxResultSuccess
            result = CamxResultSuccess;
        }

        /// Get an output buffer to hold AF algorithm output
        m_pAFIOUtil->GetAFOutputBuffers(&pOutput, &pHALOutput);
        if ((NULL == pOutput) || (NULL == pHALOutput))

        {
            CAMX_LOG_ERROR(CamxLogGroupAF, "NULL pointer Output: %p, HALOutput: %p", pOutput, pHALOutput);
            result = CamxResultEInvalidPointer;
        }

        // For the first req ID, update static settings so that correct primary algo is selected
        if ((CamxResultSuccess == result) && (FirstValidRequestId == requestIdOffsetFromLastFlush))
        {
            UpdateAFSettings(FALSE);
            result = SetSingleParamToAlgorithm(
                AFSetParamTypeSettings, static_cast<VOID*>(&m_settingsInfo), sizeof(AFAlgoSettingInfo));
        }

        if (CamxResultSuccess == result)
        {
            if (TRUE == pStaticSettings->disableAFStatsProcessing || TRUE == m_isFixedFocus)
            {
                m_pAFIOUtil->GetAFReadProperties(requestIdOffsetFromLastFlush);
                ProcessSetParams(
                                pStatsProcessRequestDataInfo->pTuningModeData,
                                pStatsProcessRequestDataInfo);
                m_pAFIOUtil->CopyInputSettings();

                OverwriteAutoFocusOutput(pOutput);
                m_pAFIOUtil->PublishCrossProperty(pStatsProcessRequestDataInfo);
                if (TRUE == m_isFixedFocus)
                {
                    ProcessUpdatePeerFocusInfo();
                }
                else
                {
                    m_pAFIOUtil->PublishPeerFocusInfo(pPeer_info);
                }
                result = m_pAFIOUtil->PublishOutput(pOutput, pHALOutput);
                if (CamxResultSuccess == result)
                {
                    result = m_pAFIOUtil->PublishFOVC();
                }
            }
            else if (m_pNode->GetMaximumPipelineDelay() >= requestIdOffsetFromLastFlush)
            {
                BOOL isEarlyPCREnabled = FALSE;
                m_pNode->IsEarlyPCREnabled(&isEarlyPCREnabled);
                m_pAFIOUtil->ReadMultiCameraInfo(&cameraInfo);

                if ((FirstValidRequestId == requestIdOffsetFromLastFlush) &&
                    (TRUE == isEarlyPCREnabled))
                {
                    // Early PCR is enabled, pass proper tuning parameter to algo and get and publish initial AF config
                    if (NULL != pStatsProcessRequestDataInfo->pTuningModeData)
                    {
                        StatsTuningData statsTuningData = { 0 };
                        SetAlgoChromatix(pStatsProcessRequestDataInfo->pTuningModeData, &statsTuningData);
                        SetSingleParamToAlgorithm(
                                        AFSetParamTypeTuningData,
                                        static_cast<VOID*>(&statsTuningData),
                                        sizeof(StatsTuningData));
                        GetDefaultConfigFromAlgo();
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupAF, "Failed to set tuning data. pTuningModeData is NULL");
                        result = CamxResultEInvalidPointer;
                    }
                }

                m_pAFIOUtil->GetAFReadProperties(requestIdOffsetFromLastFlush);

                BFStats.startupMode = StatisticsStartUpInvalid;
                BGStats.startupMode = StatisticsStartUpInvalid;

                PrepareInputParams(&BFStats, &BGStats, &cameraInfo, &input);

                if ((CamxResultSuccess == result) && (NULL != pStatsProcessRequestDataInfo->pTuningModeData))
                {
                    result = ProcessSetParams(
                                        pStatsProcessRequestDataInfo->pTuningModeData,
                                        pStatsProcessRequestDataInfo);
                }
                else
                {
                    result = CamxResultEInvalidPointer;
                }
                m_pAFIOUtil->ResetOutputs(pOutput);

                CAMX_ASSERT_MESSAGE(NULL != m_pAFAlgorithm, "AF Algorithm NULL pointer");

                m_pAFIOUtil->UpdateChiStatsSession(pStatsProcessRequestDataInfo);

                if (CamxResultSuccess == result)
                {
                    result = m_pAFAlgorithm->AFProcess(m_pAFAlgorithm, &input, pOutput);
                }

                if (CamxResultSuccess == result)
                {
                    m_pAFIOUtil->CopyInputSettings();

                    // result = m_pAFIOUtil->PublishDefaultOutput();
                    m_pAFIOUtil->GetAFReadProperties(requestIdOffsetFromLastFlush);
                    AFROIInfo AFROIInfo;
                    m_pAFIOUtil->ReadFocusRegions(&AFROIInfo);

                    AFFocusMode AFMode;
                    m_pAFIOUtil->ReadFocusMode(&AFMode);


                    result = BuildHALOutput(pOutput, pHALOutput);
                    if (CamxResultSuccess == result)
                    {
                        result = m_pAFIOUtil->PublishOutput(pOutput, pHALOutput);
                    }
                    // Publish cross pipeline property for multi camera even in pre request
                    if (CamxResultSuccess == result)
                    {
                        result = m_pAFIOUtil->PublishCrossProperty(pStatsProcessRequestDataInfo);
                        m_pAFIOUtil->PublishPeerFocusInfo(pPeer_info);
                    }

                    // Publish FOVC data for request id < maxPipelineDelay
                    if (CamxResultSuccess == result)
                    {
                        result = m_pAFIOUtil->PublishFOVC();
                    }
                }

            }
            else
            {

                UpdateAFSettings(FALSE);
                m_pAFIOUtil->ReadMultiCameraInfo(&cameraInfo);

                // PD processing holds good when
                // 1. PDAF is enabled and
                // 2. Early processing if sensor is not PDAF Type1 or
                // 3. During end of frame with full frame PDAF data for all PDAF sensors
                // 4. Disable PD processing when camera role is slave for performance
                if ((TRUE == m_isPDAFEnabled) && (StatsAlgoRoleSlave != cameraInfo.algoRole) &&
                    ((BAFStatsDependencyMet == pStatsProcessRequestDataInfo->processSequenceId &&
                      PDLibSensorType1 != m_PDAFType) ||
                    (PDStatDependencyMet == pStatsProcessRequestDataInfo->processSequenceId)))
                {
                    result = m_pAFIOUtil->ParsePDStats(pStatsProcessRequestDataInfo);
                }

                if (BAFStatsDependencyMet == pStatsProcessRequestDataInfo->processSequenceId)
                {
                    if (CamxResultSuccess == result)
                    {
                        result = m_pAFIOUtil->ParseISPStats(pStatsProcessRequestDataInfo);
                    }

                    if (CamxResultSuccess == result)
                    {
                        m_pAFIOUtil->GetAFReadProperties(requestIdOffsetFromLastFlush);
                    }

                    if (CamxResultSuccess == result)
                    {
                        PrepareInputParams(&BFStats, &BGStats, &cameraInfo, &input);
                    }

                    if ((CamxResultSuccess == result) && (NULL != pStatsProcessRequestDataInfo->pTuningModeData))
                    {
                        result = ProcessSetParams(
                                pStatsProcessRequestDataInfo->pTuningModeData,
                                pStatsProcessRequestDataInfo);
                    }

                    // Process the input
                    if (CamxResultSuccess == result)
                    {
                        m_pAFIOUtil->ResetOutputs(pOutput);

                        m_pAFIOUtil->UpdateChiStatsSession(pStatsProcessRequestDataInfo);

                        result = m_pAFAlgorithm->AFProcess(m_pAFAlgorithm, &input, pOutput);

                        // Publish cross pipeline property for multi camera
                        m_pAFIOUtil->PublishCrossProperty(pStatsProcessRequestDataInfo);

                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupAF, "AFProcess() failed!");
                        }
                        else
                        {
                            // Generate HAL output
                            result = BuildHALOutput(pOutput, pHALOutput);

                            if (CamxResultSuccess == result)
                            {
                                result = m_pAFIOUtil->PublishOutput(pOutput, pHALOutput);
                                if (CamxResultSuccess == result)
                                {
                                    /* In Multi camera case publish info require for peer*/
                                    if ((TRUE == pStaticSettings->multiCameraEnable) &&
                                            (MultiCamera3ASyncDisabled != pStaticSettings->multiCamera3ASync))
                                    {
                                        ProcessUpdatePeerFocusInfo();
                                    }
                                }

                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::GetOEMDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::GetOEMDependencies(
    AFAlgoGetParam* pAlgoGetParam)
{
    CamxResult result = CamxResultSuccess;

    // Get OEM properties dependencies list. Get vendor tag
    m_pAFIOUtil->PopulateVendorTagGetParam(AFGetParamDependentVendorTags, pAlgoGetParam);

    CDKResult cdkResult = m_pAFAlgorithm->AFGetParam(m_pAFAlgorithm, pAlgoGetParam);
    result = StatsUtil::CdkResultToCamXResult(cdkResult);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::GetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::GetDependencies(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    StatsDependency*                pStatsDependency)
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CamxResult                result                  = CamxResultSuccess;
    StatsDependency           statsDependency         = { 0 };
    AFAlgoGetParam            algoGetParam            = { { 0 } , { 0 } };
    AFPropertyDependencyList  propertyDependencyList  = { 0 };
    StatsCameraInfo           cameraInfo              = { StatsAlgoRoleDefault, 0 };

    if (TRUE == pStatsProcessRequestDataInfo->skipProcessing)
    {
        // We need to publish previous frame data to property pool when skipProcessing flag is set.
        // Add previous frame(offset = 1) AF Frame/Stats Control properties as dependency.
        pStatsDependency->properties[0] = { PropertyIDAFFrameControl, 1 };
        pStatsDependency->properties[1] = { PropertyIDAFStatsControl, 1 };

        pStatsDependency->propertyCount = 2;
        m_pAFIOUtil->ReadMultiCameraInfo(&cameraInfo);

        // In case of Type1 sensor, early PD processing does not hold good. In such cases, SeqID
        // is not guaranteed. I.e SeqID 1 of ReqId 9 can come before SeqID 2 of ReqId 8
        // So, in this case, if the skip pattern is 2, we would like to skip ReqID 9 processing
        // and copy the output from ReqId 8, but here ReqID 8 PD processing is yet to be done and
        // the result is not available. So, add dependency on previous frame PD output to
        // serialize the execution

        // Not adding dependency when camera role is slave
        if ((TRUE == m_isPDAFEnabled) && (PDLibSensorType1 == m_PDAFType) &&
            (StatsAlgoRoleSlave != cameraInfo.algoRole))
        {
            pStatsDependency->properties[2] = { PropertyIDAFPDFrameInfo, 1 };
            pStatsDependency->propertyCount = 3;
        }
    }
    else
    {
        /// @todo  (CAMX-523): get dependencies from the algorithm

        // Get Vendor tag
        result = GetOEMDependencies(&algoGetParam);

        if (CamxResultSuccess == result)
        {
            CombinePropertyDependencLists(&propertyDependencyList,
                &statsDependency,
                &algoGetParam,
                pStatsProcessRequestDataInfo->requestId);

            AddPropertyDependencies(&propertyDependencyList, pStatsDependency);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::CombinePropertyDependencLists
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFStatsProcessor::CombinePropertyDependencLists(
    AFPropertyDependencyList*   pPropertyDependencyList,
    StatsDependency*            pStatsDependency,
    AFAlgoGetParam*             pAlgoGetParam,
    UINT64                      requestId)
{
    CAMX_ASSERT_MESSAGE(NULL != pStatsDependency, "pStatsDependency NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pAlgoGetParam, "pAlgoGetParam NULL pointer");

    UINT32 propertyIndex = 0;

    // Adding stat dependencies
    for (INT32 i = 0; (i < pStatsDependency->propertyCount && propertyIndex < MaxStatsProperties); i++, propertyIndex++)
    {
        pPropertyDependencyList->properties[propertyIndex].propertyID   = pStatsDependency->properties[i].property;
        pPropertyDependencyList->properties[propertyIndex].slotNum      = requestId;
        pPropertyDependencyList->propertyCount++;
    }

    AFAlgoGetParamOutputList* pGetParamOutput = &pAlgoGetParam->outputInfo;
    for (UINT32 i = 0; i < pGetParamOutput->getParamOutputCount; i++)
    {
        if (0 == pGetParamOutput->pGetParamOutputs[i].sizeOfWrittenGetParamOutput)
        {
            continue;
        }
        // going through the list of vendor tag dependency
        if (AFGetParamDependentVendorTags == pGetParamOutput->pGetParamOutputs[i].getParamOutputType)
        {
            StatsVendorTagInfoList* pVendorTagInfoList =
                static_cast<StatsVendorTagInfoList*>(pAlgoGetParam->outputInfo.pGetParamOutputs[i].pGetParamOutput);
            // Adding vendor tag dependencies
            for (UINT32 j = 0;
                 ((j < pVendorTagInfoList->vendorTagCount) && (propertyIndex < MaxStatsProperties));
                 j++, propertyIndex++)
            {
                pPropertyDependencyList->properties[propertyIndex].propertyID   = pVendorTagInfoList->vendorTag[j].vendorTagId;
                pPropertyDependencyList->properties[propertyIndex].slotNum      =
                    requestId + pVendorTagInfoList->vendorTag[j].appliedDelay;
                pPropertyDependencyList->propertyCount++;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::AddPropertyDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFStatsProcessor::AddPropertyDependencies(
    AFPropertyDependencyList*   pUnpublishedPropertyDependencyList,
    StatsDependency*            pStatsDependency)
{
    INT32 propertyCount;

    // Add unpublished dependencies
    for (propertyCount = 0; propertyCount < pUnpublishedPropertyDependencyList->propertyCount; propertyCount++)
    {
        pStatsDependency->properties[propertyCount].property =
            pUnpublishedPropertyDependencyList->properties[propertyCount].propertyID;
    }

    pStatsDependency->propertyCount = propertyCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::IsDependenciesSatisfied
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::IsDependenciesSatisfied(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    BOOL*                           pIsSatisfied)
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CamxResult      result          = CamxResultSuccess;
    BOOL            isStatisfied    = FALSE;
    StatsDependency statsDependency = { 0 };

    /// @todo  (CAMX-523): query algorithm to check if dependencies are satisfied.

    // Get stats property dependency list
    result = GetDependencies(pStatsProcessRequestDataInfo, &statsDependency);

    isStatisfied = (statsDependency.propertyCount == 0) ? TRUE : FALSE;

    *pIsSatisfied &= isStatisfied;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::~CAFStatsProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAFStatsProcessor::~CAFStatsProcessor()
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);
    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    CAMX_ASSERT_MESSAGE(NULL != pStaticSettings, "pStaticSettings NULL pointer");

    StatsCameraInfo cameraInfo = m_cameraInfo;

    // Destroy all the created objects.
    if (NULL != m_pAFAlgorithm)
    {
        AFAlgoDestroyParamList destroyParamList                       = { 0 };
        AFAlgoDestroyParam     destroyParams[AFDestroyParamTypeCount] = {};

        UINT  overrideCameraClose                                              = pStaticSettings->overrideCameraClose;
        destroyParams[AFDestroyParamTypeCameraCloseIndicator].destroyParamType = AFDestroyParamTypeCameraCloseIndicator;
        destroyParams[AFDestroyParamTypeCameraCloseIndicator].pParam           = &overrideCameraClose;
        destroyParams[AFDestroyParamTypeCameraCloseIndicator].sizeOfParam      = sizeof(UINT);

        destroyParams[AFDestroyParamTypeCameraInfo].destroyParamType = AFDestroyParamTypeCameraInfo;
        destroyParams[AFDestroyParamTypeCameraInfo].pParam = static_cast<VOID*>(&cameraInfo);
        destroyParams[AFDestroyParamTypeCameraInfo].sizeOfParam = sizeof(StatsCameraInfo);

        destroyParamList.paramCount = AFDestroyParamTypeCount;
        destroyParamList.pParamList = &destroyParams[0];

        m_pAFAlgorithm->AFDestroy(m_pAFAlgorithm, &destroyParamList);
        m_pAFAlgorithm = NULL;
    }

    if (NULL != m_hHandle)
    {
        OsUtils::LibUnmap(m_hHandle);
        m_hHandle = NULL;
    }

    if (NULL != m_hHAFHandle)
    {
        OsUtils::LibUnmap(m_hHAFHandle);
        m_hHAFHandle = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::MapControlAFTriggerToAFStateTransition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAFStatsProcessor::MapControlAFTriggerToAFStateTransition(
    ControlAFTriggerValues* pAFTriggerValue,
    AFStateTransitionType*  pAFStateTransitionType)
{
    CAMX_ASSERT_MESSAGE(NULL != pAFTriggerValue, "pAFTriggerValue NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pAFStateTransitionType, "pAFStateTransitionType NULL pointer");

    switch (*pAFTriggerValue)
    {
        case ControlAFTriggerIdle:
            *pAFStateTransitionType = AFStateTransitionType::EventIdle;
            break;
        case ControlAFTriggerStart:
            *pAFStateTransitionType = AFStateTransitionType::EventTrigger;
            break;
        case ControlAFTriggerCancel:
        case ControlAFTriggerEnd:
            *pAFStateTransitionType = AFStateTransitionType::EventCancel;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("ControlAFTriggerValues has been passed by HAL and is not right value: %d",
                                       *pAFTriggerValue);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::GenerateFocusControlCommand
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AFControlCommand  CAFStatsProcessor::GenerateFocusControlCommand(
    AFFocusMode*    pFocusMode,
    AFRunMode*      pCameraOperationalMode)
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CAMX_ASSERT_MESSAGE(NULL != pFocusMode, "pFocusMode NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pCameraOperationalMode, "pCameraOperationalMode NULL pointer");

    ControlAFTriggerValues AFTriggerValue    = m_pAFIOUtil->ReadControlAFTrigger();
    AFStateTransitionType  AFStateTransition = AFStateTransitionType::EventIdle;
    MapControlAFTriggerToAFStateTransition(&AFTriggerValue, &AFStateTransition);

    m_AFStateMachine.SetAFFocusAndRunMode(*pFocusMode, *pCameraOperationalMode);
    m_AFStateMachine.HandleAFStateTransition(AFStateTransition);

    return m_AFStateMachine.GetControlCommand();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::ProcessSetParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::ProcessSetParams(
    ChiTuningModeParameter*        pInputTuningModeData,
    const StatsProcessRequestData* pStatsProcessRequestDataInfo)
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);

    CamxResult                     result                        = CamxResultSuccess;
    AFAlgoSetParam                 setParam[AFSetParamLastIndex] = { {0} };
    AFAlgoSetParamList             setParamList                  = {};

    // AFSetParam must deep copy these if it needs them after the call, so we use locals to save state temporarily
    AFSensorInfo               sensorInfo              = { 0 };
    AFAECInfo                  AECInfo                 = { 0 };
    AFFocusDistanceInfo        focusDistanceInfo       = { 0 };
    StatsBayerGrid             AFBayerGridInfo         = {};
    AFFocusMode                AFMode                  = AFFocusModeAuto;
    AFRunMode                  cameraOperationalMode   = AFRunModePreview;
    AFControlCommand           controlCommand          = AFStartCommand;
    AFFaceROIInfo              faceROIInfo             = {};
    UnstabilizedROIInformation unstabilizedFaceROIInfo = {};
    UnstabilizedROIInformation trackROIInfo = {};
    AFROIInfo                  AFROIInfo               = {};
    StatsCameraInfo            cameraInfo              = {};
    AFGyroData                 AFGyroInfo              = {};
    AFGravityData              AFGravityInfo           = {};
    AFTOFData                  AFTOFInfo               = {};
    VOID*                      pPeerInfo               = NULL;
    StatsRectangle             cropWindow              = {};
    const StaticSettings*      pStaticSettings         = HwEnvironment::GetInstance()->GetStaticSettings();

    setParamList.numberOfSetParam = 0;
    setParamList.pAFSetParams = setParam;
    CAMX_ASSERT_MESSAGE(NULL != m_pAFIOUtil, "AF IO Util NULL pointer");

    // Primary algo changes via AFSetParamTypeTuningData
    // Send it before others to ensure setparams go to the right mixer/algo
    StatsTuningData statsTuningData                        = { 0 };
    SetAlgoChromatix(pInputTuningModeData, &statsTuningData);
    setParam[setParamList.numberOfSetParam].pAFSetParam    = static_cast<VOID*>(&statsTuningData);
    setParam[setParamList.numberOfSetParam].setParamType   = AFSetParamTypeTuningData;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(StatsTuningData);
    setParamList.numberOfSetParam                          = setParamList.numberOfSetParam + 1;

    // This need to be called before AFSetParamTypeROI beacuse setROI needs sensor resolution/ camif info
    m_pAFIOUtil->ReadSensorInput(&sensorInfo);
    sensorInfo.isFixedFocus = m_isFixedFocus;

    setParam[setParamList.numberOfSetParam].pAFSetParam    = static_cast<VOID*>(&sensorInfo);
    setParam[setParamList.numberOfSetParam].setParamType   = AFSetParamTypeSensorInfo;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(AFSensorInfo);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    AFHardwareCapability* pHWCapability = m_pAFIOUtil->ReadHardwareCapability();

    setParam[setParamList.numberOfSetParam].pAFSetParam    = static_cast<VOID*>(pHWCapability);
    setParam[setParamList.numberOfSetParam].setParamType   = AFSetParamTypeHardwareCapabilities;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(AFHardwareCapability);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    m_pAFIOUtil->ReadFocusMode(&AFMode);
    setParam[setParamList.numberOfSetParam].pAFSetParam    = static_cast<VOID*>(&AFMode);
    setParam[setParamList.numberOfSetParam].setParamType   = AFSetParamTypeFocusMode;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(AFFocusMode);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    m_pAFIOUtil->ReadFocusDistanceInfo(&focusDistanceInfo);
    setParam[setParamList.numberOfSetParam].pAFSetParam    = static_cast<VOID*>(&focusDistanceInfo);
    setParam[setParamList.numberOfSetParam].setParamType   = AFSetParamTypeManualLensMoveInfo;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(focusDistanceInfo);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    m_pAFIOUtil->ReadFocusRegions(&AFROIInfo);
    setParam[setParamList.numberOfSetParam].pAFSetParam    = static_cast<VOID*>(&AFROIInfo);
    setParam[setParamList.numberOfSetParam].setParamType   = AFSetParamTypeROI;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(AFROIInfo);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    if (CamxResultSuccess == m_pAFIOUtil->ReadFaceROIInformation(&faceROIInfo, &unstabilizedFaceROIInfo))
    {
        setParam[setParamList.numberOfSetParam].pAFSetParam    = static_cast<VOID*>(&faceROIInfo);
        setParam[setParamList.numberOfSetParam].setParamType   = AFSetParamTypeFaceROI;
        setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(AFFaceROIInfo);
        setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

        setParam[setParamList.numberOfSetParam].pAFSetParam    = static_cast<VOID*>(&unstabilizedFaceROIInfo);
        setParam[setParamList.numberOfSetParam].setParamType   = AFSetParamTypeUnstabilizedFaceROI;
        setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(UnstabilizedROIInformation);
        setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;
    }
    if (CamxResultSuccess == m_pAFIOUtil->ReadTrackerROIInformation(&trackROIInfo))
    {
        setParam[setParamList.numberOfSetParam].pAFSetParam    = static_cast<VOID*>(&trackROIInfo);
        setParam[setParamList.numberOfSetParam].setParamType   = AFSetParamTypeTrackROI;
        setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(UnstabilizedROIInformation);
        setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;
    }
    if (CamxResultSuccess == m_pAFIOUtil->ReadBGStat(&AFBayerGridInfo))
    {
        setParam[setParamList.numberOfSetParam].pAFSetParam = static_cast<VOID*>(&AFBayerGridInfo);
        setParam[setParamList.numberOfSetParam].setParamType = AFSetParamTypeBGStats;
        setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(StatsBayerGrid);
        setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;
    }

    AFGyroInfo = m_pAFIOUtil->GetGyroData();
    setParam[setParamList.numberOfSetParam].pAFSetParam = static_cast<VOID*>(&AFGyroInfo);
    setParam[setParamList.numberOfSetParam].setParamType = AFSetParamTypeGyroData;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(AFGyroData);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    AFGravityInfo = m_pAFIOUtil->GetGravityData();
    setParam[setParamList.numberOfSetParam].pAFSetParam = static_cast<VOID*>(&AFGravityInfo);
    setParam[setParamList.numberOfSetParam].setParamType = AFSetParamTypeGravityData;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(AFGravityData);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    cameraOperationalMode = m_pAFIOUtil->ReadCameraOperationalMode();
    setParam[setParamList.numberOfSetParam].pAFSetParam = static_cast<VOID*>(&cameraOperationalMode);
    setParam[setParamList.numberOfSetParam].setParamType = AFSetParamTypeRunMode;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(AFRunMode);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    // Added to support AF Lock feature in Multicamera zoom usecase
    BOOL isAFLock = m_pAFIOUtil->ReadVendorTagIsAFLock();
    if (TRUE == isAFLock)
    {
        controlCommand = AFLockCommand;
    }
    else
    {
        controlCommand = GenerateFocusControlCommand(&AFMode, &cameraOperationalMode);
    }
    setParam[setParamList.numberOfSetParam].pAFSetParam = static_cast<VOID*>(&controlCommand);
    setParam[setParamList.numberOfSetParam].setParamType = AFSetParamTypeFocusControlCommand;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(AFControlCommand);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    AFTOFInfo = m_pAFIOUtil->GetTOFData();
    if (TRUE == AFTOFInfo.isValid)
    {
        setParam[setParamList.numberOfSetParam].pAFSetParam     = static_cast<VOID*>(&AFTOFInfo);
        setParam[setParamList.numberOfSetParam].setParamType    = AFSetParamTypeTOFData;
        setParam[setParamList.numberOfSetParam].sizeOfSetParam  = sizeof(AFTOFData);
        setParamList.numberOfSetParam                           = setParamList.numberOfSetParam + 1;
    }

    if (TRUE == m_isStatsNodeAvailable)
    {
        if (CamxResultSuccess == m_pAFIOUtil->ReadAECInput(&AECInfo))
        {
            setParam[setParamList.numberOfSetParam].pAFSetParam    = static_cast<VOID*>(&AECInfo);
            setParam[setParamList.numberOfSetParam].setParamType   = AFSetParamTypeAECInfo;
            setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(AFAECInfo);
            setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;
        }
    }

    if (TRUE == m_isPDAFEnabled)
    {
        AFPDAFData* pPDAFData = m_pAFIOUtil->ReadPDAFDataInput();
        setParam[setParamList.numberOfSetParam].pAFSetParam = static_cast<VOID*>(pPDAFData);
        setParam[setParamList.numberOfSetParam].setParamType = AFSetParamTypePDAFData;
        setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(AFPDAFData);
        setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    }

    if ((TRUE == m_pNode->IsMultiCamera()) && (MultiCamera3ASyncDisabled != pStaticSettings->multiCamera3ASync) &&
        pStatsProcessRequestDataInfo->pMultiRequestSync->currReq.isMultiRequest)
    {
        // Be sure keep set peer info before camera info, algorithm need to use this for sync judgement.
        pPeerInfo = m_pAFIOUtil->GetPeerInfo(pStatsProcessRequestDataInfo);
    }
    else
    {
        pPeerInfo = NULL;
    }
    setParam[setParamList.numberOfSetParam].pAFSetParam =  pPeerInfo;
    setParam[setParamList.numberOfSetParam].setParamType = AFSetParamPeerInfo;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(VOID*);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    m_pAFIOUtil->GetCameraInfo(&cameraInfo);
    setParam[setParamList.numberOfSetParam].pAFSetParam = static_cast<VOID*>(&cameraInfo);
    setParam[setParamList.numberOfSetParam].setParamType = AFSetParamTypeCameraInfo;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam = sizeof(StatsCameraInfo);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    CAMX_LOG_VERBOSE(CamxLogGroupAF, "cameraInfo cameraID %d role %d!",
                    cameraInfo.cameraId, cameraInfo.algoRole);

    m_pAFIOUtil->GetCropWindowInfo(&cropWindow);
    setParam[setParamList.numberOfSetParam].pAFSetParam     = static_cast<VOID*>(&cropWindow);
    setParam[setParamList.numberOfSetParam].setParamType    = AFSetParamCropWindow;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam  = sizeof(StatsRectangle);
    setParamList.numberOfSetParam                           = setParamList.numberOfSetParam + 1;


    // Set whether realtime BPS camera usecase is selected
    BOOL isBPSCamera = m_pNode->IsCameraRunningOnBPS();
    setParam[setParamList.numberOfSetParam].pAFSetParam     = static_cast<VOID*>(&isBPSCamera);
    setParam[setParamList.numberOfSetParam].setParamType    = AFSetParamTypeBPSCamera;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam  = sizeof(BOOL);
    setParamList.numberOfSetParam                           = setParamList.numberOfSetParam + 1;


    CDKResult cdkResult = m_pAFAlgorithm->AFSetParam(m_pAFAlgorithm, &setParamList);
    if (CDKResultENotImplemented == cdkResult)
    {
        result = CamxResultSuccess;
    }
    else
    {
        result = StatsUtil::CdkResultToCamXResult(cdkResult);
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAF, "AFSetParam() failed!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::UpdateInputParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE VOID CAFStatsProcessor::UpdateInputParam(
    AFAlgoInputType inputType,
    UINT32          inputSize,
    VOID*           pValue,
    SIZE_T          inputIndex)
{
    m_inputArray[inputIndex].inputType      = inputType;
    m_inputArray[inputIndex].sizeOfInput    = static_cast<UINT32>(inputSize);
    m_inputArray[inputIndex].pAFInput       = pValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::PrepareInputParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFStatsProcessor::PrepareInputParams(
    AFBAFStatistics* pBFStats,
    StatsBayerGrid*  pBGStats,
    StatsCameraInfo* pCameraInfo,
    AFAlgoInputList* pInputList)
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);


    SIZE_T      inputCount  = 0;

    pInputList->pAFInputs           = m_inputArray;
    pInputList->numberOfInputParam  = 0;
    BOOL haveBFStats = (CamxResultSuccess == m_pAFIOUtil->ReadBFStat(pBFStats)) ? TRUE : FALSE;
    const StaticSettings*   pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    if ((FALSE == haveBFStats) && (NULL != pBFStats))
    {
        pBFStats->startupMode = StatisticsStartUpInvalid;
    }
    else
    {
        if (NULL != pBFStats)
        {
            pBFStats->startupMode = StatisticsStartUpValid;
        }
    }

    CAMX_ASSERT(pStaticSettings != NULL);

    // Even if we don't have valid BG stat, we still execute ProcessRequest
    UpdateInputParam(AFInputTypeBAFStats,
                     sizeof(AFBAFStatistics),
                     static_cast<VOID*>(pBFStats),
                     inputCount++);

    if (FALSE == pStaticSettings->disableBGStatsForAF)
    {
        if (TRUE == m_isStatsNodeAvailable)
        {
            if (CamxResultSuccess == m_pAFIOUtil->ReadBGStat(pBGStats))
            {
                pBGStats->startupMode = StatisticsStartUpValid;
            }
            else if (StatisticsStartUpInvalid == pBGStats->startupMode)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupAF, "BG Stats startup mode is invalid!");
            }
            UpdateInputParam(AFInputTypeBGStats, sizeof(StatsBayerGrid), static_cast<VOID*>(pBGStats), inputCount++);
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupAF, "AF is not required to read BG Stats!");
    }

    // Fill vendortag information into input list
    m_pAFIOUtil->ReadVendorTag(&m_pInputVendorTagList);

    if (m_pInputVendorTagList.vendorTagCount > 0)
    {
        UpdateInputParam(AFInputTypeVendorTag,
                         sizeof(StatsBayerGrid),
                         static_cast<VOID*>(&m_pInputVendorTagList),
                         inputCount++);
    }

    // Update request ID
    UpdateInputParam(AFInputTypeRequestId, sizeof(UINT64), static_cast<VOID*>(&m_requestId), inputCount++);
    // Update Debug Data
    DebugData* pDebugData = NULL;
    m_pAFIOUtil->GetDebugData(m_requestId, &pDebugData);
    if (NULL != pDebugData &&
        NULL != pDebugData->pData &&
        0 < pDebugData->size &&
        (StatsAlgoRoleDefault == pCameraInfo->algoRole || StatsAlgoRoleMaster == pCameraInfo->algoRole))
    {
        UpdateInputParam(AFInputTypeDebugData, sizeof(StatsDataPointer), static_cast<VOID*>(pDebugData), inputCount++);
    }
    else
    {
        UpdateInputParam(AFInputTypeDebugData, 0, 0, inputCount++);
    }

    ChiStatsSession* pStatsSession = m_pAFIOUtil->GetChiStatsSession();
    UpdateInputParam(AFInputTypeStatsChiHandle, sizeof(ChiStatsSession), static_cast<VOID*>(pStatsSession), inputCount++);

    // Update camera information
    UpdateInputParam(AFInputTypeCameraInfo, sizeof(StatsCameraInfo), static_cast<VOID*>(pCameraInfo), inputCount++);

    pInputList->pAFInputs           = m_inputArray;
    pInputList->numberOfInputParam  = inputCount;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::OverwriteAutoFocusOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFStatsProcessor::OverwriteAutoFocusOutput(
    AFAlgoOutputList* pOutputList)
{
    CAMX_ASSERT_MESSAGE(NULL != pOutputList, "AF Algo OutputList NULL pointer");

    AFLensPositionInfo* pLensPositionInfo                               =
        static_cast<AFLensPositionInfo *>(pOutputList->pAFOutputs[AFOutputTypeMoveLensParam].pAFOutput);

    CAMX_ASSERT_MESSAGE(NULL != pLensPositionInfo, "Lens Position Info NULL pointer");
    pLensPositionInfo->moveType                                         = AFLensMoveTypeLogical;
    pLensPositionInfo->logicalLensMoveInfo.lensPositionInLogicalUnits   =
        HwEnvironment::GetInstance()->GetStaticSettings()->lensPos;
    pOutputList->pAFOutputs[AFOutputTypeMoveLensParam].sizeOfWrittenAFOutput = sizeof(AFLensPositionInfo);

    AFAlgoStatusParam* pStatusInfo  =
        static_cast<AFAlgoStatusParam *>(pOutputList->pAFOutputs[AFOutputTypeStatusParam].pAFOutput);

    CAMX_ASSERT_MESSAGE(NULL != pStatusInfo, "Status Info NULL pointer");
    pStatusInfo->focusDone          = TRUE;
    pStatusInfo->status             = AFAlgoStatusTypeFocused;
    pOutputList->pAFOutputs[AFOutputTypeStatusParam].sizeOfWrittenAFOutput = sizeof(AFAlgoStatusParam);
    pOutputList->outputCount = AFOutputTypeMoveLensParam;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::UpdateAFSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::UpdateAFSettings(
    BOOL initialUpdate)
{
    CamxResult result = CamxResultSuccess;
    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    if (TRUE == initialUpdate)
    {
        m_settingsInfo.disableFocusIndication   = pStaticSettings->disableFocusIndication;
        m_settingsInfo.enableAFAlgo             = pStaticSettings->enableAFAlgo;
        m_settingsInfo.fovcEnable               = pStaticSettings->fovcEnable;
        m_settingsInfo.kpiDebug                 = pStaticSettings->kpiDebug;
        // Copy VM debug mask from static settings
        Utils::Memcpy(&m_settingsInfo.vmDebugMask, pStaticSettings->vmDebugMask,
          sizeof(m_settingsInfo.vmDebugMask));
        Utils::Memcpy(&m_settingsInfo.activeNodeMask, pStaticSettings->activeNodeMask,
            sizeof(m_settingsInfo.activeNodeMask));
        m_settingsInfo.apiMask = pStaticSettings->apiMask;
    }

    m_settingsInfo.afFullsweep                  = pStaticSettings->afFullsweep;
    m_settingsInfo.lensPos                      = pStaticSettings->lensPos;
    m_settingsInfo.manualAf                     = pStaticSettings->manualAf;
    m_settingsInfo.isSpdEnable                  = pStaticSettings->enableSPD;
    m_settingsInfo.spdStatsType                 = pStaticSettings->spdStatsType;
    m_settingsInfo.enableHJAF                   = pStaticSettings->enableHJAF;

    m_settingsInfo.mwEnable                     = pStaticSettings->mwEnable;
    m_settingsInfo.recordMode                   = pStaticSettings->AFRecorderMode;
    m_settingsInfo.overrideHAFAlgoMask          = pStaticSettings->overrideHAFAlgoMask;
    m_settingsInfo.spotlightFallback            = pStaticSettings->spotlightFallbackToCAF;
    // Enables PDAF only if PDAF is enabled from override settings and sensor is streaming PDAF data
    m_settingsInfo.disablePDAF                  = !m_isPDAFEnabled;
    m_settingsInfo.enableLensSagComp            = pStaticSettings->enableLensSagComp;
    m_settingsInfo.profile3A                    = pStaticSettings->profile3A;
    m_settingsInfo.enable3ADebugData            = pStaticSettings->enable3ADebugData;
    m_settingsInfo.enableConcise3ADebugData     = pStaticSettings->enableConcise3ADebugData;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::SetAFAlgorithmHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFStatsProcessor::SetAFAlgorithmHandler(
    CAFAlgorithmHandler* pAlgoHandler)
{
    m_pAFAlgorithmHandler = pAlgoHandler;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::SetAFIOUtil
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFStatsProcessor::SetAFIOUtil(
    CAFIOUtil * pAFIOUtil)
{
    m_pAFIOUtil = pAFIOUtil;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::SetAFPDAFInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFStatsProcessor::SetAFPDAFInformation(
    PDAFEnablementConditions* pPDAFEnablementConditions)
{
    m_isPDAFEnabled = pPDAFEnablementConditions->IsPDAFEnabled();
    m_PDAFType = pPDAFEnablementConditions->GetPDAFType();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::SetAlgoChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::SetAlgoChromatix(
    ChiTuningModeParameter* pInputTuningModeData,
    StatsTuningData*        pTuningData)
{
    CamxResult              result = CamxResultSuccess;

    if (NULL != pInputTuningModeData)
    {
        pTuningData->pTuningSetManager    = m_pTuningDataManager;
        pTuningData->pTuningModeSelectors = reinterpret_cast<TuningMode*>(&pInputTuningModeData->TuningMode[0]);
        pTuningData->numSelectors         = pInputTuningModeData->noOfSelectionParameter;
        CAMX_LOG_VERBOSE(CamxLogGroupAF,
                         "Tuning data as mode: %d usecase %d  feature1 %d feature2 %d scene %d, effect %d,",
                         pInputTuningModeData->TuningMode[0].mode,
                         pInputTuningModeData->TuningMode[2].subMode.usecase,
                         pInputTuningModeData->TuningMode[3].subMode.feature1,
                         pInputTuningModeData->TuningMode[4].subMode.feature2,
                         pInputTuningModeData->TuningMode[5].subMode.scene,
                         pInputTuningModeData->TuningMode[6].subMode.effect);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAF, "Input tuning data is NULL pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::ProcessUpdatePeerFocusInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID  CAFStatsProcessor::ProcessUpdatePeerFocusInfo()
{
    CamxResult      result                  = CamxResultSuccess;
    UINT32          getParamOutArrayIndex   = 0;
    UINT32          getParamInputArrayIndex = 0;
    AFAlgoGetParam  algoGetParam            = {};

    AFAlgoGetParamOutput getoutput[AFGetParamLastIndex];
    AFAlgoGetParamInput  getParamInputArray[AFGetParamLastIndex];

    getParamInputArray[getParamInputArrayIndex].type = AFGetParamInfoForPeer;
    getParamInputArrayIndex += 1;
    getoutput[getParamOutArrayIndex].getParamOutputType          = AFGetParamInfoForPeer;
    getoutput[getParamOutArrayIndex].pGetParamOutput             = NULL;
    getoutput[getParamOutArrayIndex].sizeOfGetParamOutput        = sizeof(VOID*);
    getoutput[getParamOutArrayIndex].sizeOfWrittenGetParamOutput = 0;
    getParamOutArrayIndex += 1;

    algoGetParam.outputInfo.pGetParamOutputs    = getoutput;
    algoGetParam.outputInfo.getParamOutputCount = getParamOutArrayIndex;
    algoGetParam.inputInfo.pGetParamInputs      = getParamInputArray;
    algoGetParam.inputInfo.getParamInputCount   = getParamInputArrayIndex;

    CDKResult cdkResult = m_pAFAlgorithm->AFGetParam(m_pAFAlgorithm, &algoGetParam);
    result = StatsUtil::CdkResultToCamXResult(cdkResult);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAF, "AFGetParam Failed");
    }
    m_pAFIOUtil->PublishPeerFocusInfo(algoGetParam.outputInfo.pGetParamOutputs->pGetParamOutput);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFStatsProcessor::SetSingleParamToAlgorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::SetSingleParamToAlgorithm(
    AFAlgoSetParamType  paramType,
    VOID*               pParam,
    UINT32              paramSize)
{
    CAMX_ENTRYEXIT(CamxLogGroupAF);
    CAMX_ASSERT(NULL != m_pAFAlgorithm);

    CamxResult          result                        = CamxResultSuccess;
    AFAlgoSetParam      setParam[AFSetParamLastIndex] = { { 0 } };
    AFAlgoSetParamList  setParamList;
    StatsCameraInfo     cameraInfo;
    VOID*               pPeerInfo = NULL;

    m_pAFIOUtil->GetCameraInfo(&cameraInfo);

    setParamList.numberOfSetParam                           = 0;
    setParamList.pAFSetParams                               = setParam;
    setParam[setParamList.numberOfSetParam].setParamType    = paramType;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam  = paramSize;
    setParam[setParamList.numberOfSetParam].pAFSetParam     = pParam;
    setParamList.numberOfSetParam                           = setParamList.numberOfSetParam + 1;

    setParam[setParamList.numberOfSetParam].pAFSetParam     = pPeerInfo;
    setParam[setParamList.numberOfSetParam].setParamType    = AFSetParamPeerInfo;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam  = sizeof(VOID*);
    setParamList.numberOfSetParam                           = setParamList.numberOfSetParam + 1;

    setParam[setParamList.numberOfSetParam].pAFSetParam     = static_cast<VOID*>(&cameraInfo);
    setParam[setParamList.numberOfSetParam].setParamType    = AFSetParamTypeCameraInfo;
    setParam[setParamList.numberOfSetParam].sizeOfSetParam  = sizeof(StatsCameraInfo);
    setParamList.numberOfSetParam                           = setParamList.numberOfSetParam + 1;

    CDKResult cdkResult = m_pAFAlgorithm->AFSetParam(m_pAFAlgorithm, &setParamList);
    result              = StatsUtil::CdkResultToCamXResult(cdkResult);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFStatsProcessor::GetPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFStatsProcessor::GetPublishList(
    const UINT32    maxTagArraySize,
    UINT32*         pTagArray,
    UINT32*         pTagCount,
    UINT32*         pPartialTagCount)
{
    CamxResult  result               = CamxResultSuccess;
    UINT32      tagCount             = 0;
    UINT32      numMetadataTags      = CAMX_ARRAY_SIZE(AFOutputMetadataTags);
    UINT32      numVendorTags        = CAMX_ARRAY_SIZE(AFOutputVendorTags);
    UINT32      algoVendortagCount   = 0;
    UINT32      tagID;

    if (NULL != m_pOutputVendorTagInfoList)
    {
        algoVendortagCount = m_pOutputVendorTagInfoList->vendorTagCount;
    }

    if (numMetadataTags + numVendorTags + algoVendortagCount <= maxTagArraySize)
    {
        for (UINT32 tagIndex = 0; tagIndex < numMetadataTags; ++tagIndex)
        {
            pTagArray[tagCount++] = AFOutputMetadataTags[tagIndex];
        }

        for (UINT32 tagIndex = 0; tagIndex < algoVendortagCount; ++tagIndex)
        {
            pTagArray[tagCount++] = m_pOutputVendorTagInfoList->vendorTag[tagIndex].vendorTagId;
        }

        for (UINT32 tagIndex = 0; tagIndex < numVendorTags; ++tagIndex)
        {
            result = VendorTagManager::QueryVendorTagLocation(
                AFOutputVendorTags[tagIndex].pSectionName,
                AFOutputVendorTags[tagIndex].pTagName,
                &tagID);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot query vendor tag %s %s",
                    AFOutputVendorTags[tagIndex].pSectionName,
                    AFOutputVendorTags[tagIndex].pTagName);
                break;
            }
            pTagArray[tagCount++] = tagID;
        }
    }
    else
    {
        result = CamxResultEOutOfBounds;
        CAMX_LOG_ERROR(CamxLogGroupMeta, "ERROR %d %d %d tags cannot be published.",
                       numMetadataTags,
                       numVendorTags,
                       algoVendortagCount);
    }

    if (CamxResultSuccess == result)
    {
        *pTagCount        = tagCount;
        *pPartialTagCount = NumberOfPartialMetadataTags;
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%d tags will be published (partialtags=%d)",
            tagCount,
            NumberOfPartialMetadataTags);
    }
    return result;
}

CAMX_NAMESPACE_END
