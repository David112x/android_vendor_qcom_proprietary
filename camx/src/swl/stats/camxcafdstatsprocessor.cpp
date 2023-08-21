////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcafdstatsprocessor.cpp
/// @brief The class that implements IStatsProcessor for AFD.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhal3module.h"
#include "camxmem.h"
#include "camxstatsdebuginternal.h"
#include "camxtrace.h"
#include "camxtuningdatamanager.h"
#include "camxcafdstatsprocessor.h"

CAMX_NAMESPACE_BEGIN

// @brief list of tags published
static const UINT32 AFDOutputMetadataTags[] =
{
    ControlAEAntibandingMode,
    PropertyIDAFDStatsControl,
    PropertyIDAFDFrameInfo
};

// Global AFD algo pointer
static CHIAFDAlgorithm* g_pCHIAFDAlgorithm = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::Create
///
/// @brief  Create the object for CAFDStatsProcessor.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDStatsProcessor::Create(
    IStatsProcessor** ppAFDStatsProcessor)
{
    CAMX_ENTRYEXIT(CamxLogGroupAFD);

    CamxResult          result             = CamxResultSuccess;
    CAFDStatsProcessor* pAFDStatsProcessor = NULL;

    if (NULL != ppAFDStatsProcessor)
    {
        pAFDStatsProcessor = CAMX_NEW CAFDStatsProcessor;

        if (NULL != pAFDStatsProcessor)
        {
            *ppAFDStatsProcessor = pAFDStatsProcessor;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupAFD, "CAFDStatsProcessor create failed");
            result = CamxResultENoMemory;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "CAFDStatsProcessor::Create Invalid arguments");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::CAFDStatsProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAFDStatsProcessor::CAFDStatsProcessor()
    : m_pAFDAlgorithmHandler(NULL)
    , m_pAFDIOHandler(NULL)
{
    CAMX_ENTRYEXIT(CamxLogGroupAFD);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::SetAFDAlgorithmHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFDStatsProcessor::SetAFDAlgorithmHandler(
    CAFDAlgorithmHandler* pAlgoHandler)
{
    m_pAFDAlgorithmHandler = pAlgoHandler;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::SetAFDIOHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFDStatsProcessor::SetAFDIOHandler(
    CAFDIOHandler* pAFDIOHandler)
{
    m_pAFDIOHandler = pAFDIOHandler;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDStatsProcessor::Initialize(
    const StatsInitializeData* pInitializeData)
{
    CAMX_ENTRYEXIT(CamxLogGroupAFD);

    CamxResult  result = CamxResultSuccess;

    if (NULL != pInitializeData &&
        NULL != pInitializeData->pHwContext &&
        NULL != pInitializeData->pTuningDataManager)
    {
        m_pStaticSettings  = pInitializeData->pHwContext->GetStaticSettings();
        m_pNode            = pInitializeData->pNode;

        // Create an instance of the core agorithm
        if (NULL == g_pCHIAFDAlgorithm)
        {
            // create and load algorithm
            AFDAlgoCreateParamList createParamList = {};
            AFDAlgoCreateParam     createParams[AFDCreateParamTypeCount] = {};

            createParams[AFDAlgoCreateParamsLoggerFunctionPtr].createParamType = AFDAlgoCreateParamsLoggerFunctionPtr;
            createParams[AFDAlgoCreateParamsLoggerFunctionPtr].pCreateParam = reinterpret_cast<VOID*>(StatsLoggerFunction);
            createParams[AFDAlgoCreateParamsLoggerFunctionPtr].sizeOfCreateParam = sizeof(StatsLoggingFunction);

            createParamList.paramCount       = AFDCreateParamTypeCount;
            createParamList.pCreateParamList = &createParams[0];

            if (NULL != pInitializeData->initializecallback.pAFDCallback)
            {
                CREATEAFD pCreate = pInitializeData->initializecallback.pAFDCallback->pfnCreate;
                result            = pCreate(&createParamList, &g_pCHIAFDAlgorithm);
            }
        }

        m_pTuningDataManager = pInitializeData->pTuningDataManager->GetChromatix();
    }
    else
    {
        if (NULL == pInitializeData)
        {
            CAMX_LOG_ERROR(CamxLogGroupAFD, "StatsInitializeData is Invalid(NULL)");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupAFD, "pHwContext: %p or pTuningDataManager is NULL",
                pInitializeData->pHwContext,
                pInitializeData->pTuningDataManager);
        }
    }

    m_pAFDIOHandler = CAMX_NEW CAFDIOHandler();

    if (NULL == m_pAFDIOHandler)
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "Auto Flicker Detection IO Handler Allocation failed");
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        result = m_pAFDIOHandler->Initialize(pInitializeData);

    }

    if (CamxResultSuccess == result)
    {
        result = SetOverrideAlgoSetting();
    }

    if (CamxResultSuccess == result)
    {
        result = SetCameraInfo(pInitializeData->cameraInfo);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDStatsProcessor::ExecuteProcessRequest(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo)
{
    CAMX_ENTRYEXIT(CamxLogGroupAFD);

    CamxResult         result           = CamxResultSuccess;
    AFDAlgoInputList   input            = { 0 };
    AFDAlgoOutputList* pOutput          = NULL;

    /// Get an output buffer to hold AFD algorithm output
    if (NULL == m_pAFDIOHandler)
    {
        return CamxResultEFailed;
    }
    else
    {
        pOutput = m_pAFDIOHandler->GetAFDOutputBuffers();
        m_pAFDIOHandler->GetAlgoOutputBuffers(pOutput);
    }

    UINT64             requestId        = pStatsProcessRequestDataInfo->requestId;
    UINT64 requestIdOffsetFromLastFlush =
        m_pNode->GetRequestIdOffsetFromLastFlush(pStatsProcessRequestDataInfo->requestId);

    GetPrevConfigFromAlgo(); // Always call this function before Read and Set HAL param
    result = ReadHALAFDParam(&m_HALParam);



    if (TRUE == pStatsProcessRequestDataInfo->skipProcessing ||
        StatsOperationModeFastConvergence == pStatsProcessRequestDataInfo->operationMode ||
        TRUE == m_pStaticSettings->disableAFDStatsProcessing ||
        StatsAlgoRoleSlave == pStatsProcessRequestDataInfo->cameraInfo.algoRole)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAFD,
                         "Skip algo processing, RequestId=%llu requestIdOffsetFromLastFlush = %llu operationMode:%d "
                         "skipProcessing:%d disableAFDStatsProcessing:%d"
                          "maximumPipelineDelay:%d Role:%s",
                         pStatsProcessRequestDataInfo->requestId,
                         requestIdOffsetFromLastFlush,
                         pStatsProcessRequestDataInfo->operationMode,
                         pStatsProcessRequestDataInfo->skipProcessing,
                         m_pStaticSettings->disableAFDStatsProcessing,
                         m_pNode->GetMaximumPipelineDelay(),
                         StatsUtil::GetRoleName(pStatsProcessRequestDataInfo->cameraInfo.algoRole));
    }
    else if (m_pNode->GetMaximumPipelineDelay() >= requestIdOffsetFromLastFlush)
    {
        // For First few requests, set Sensor Info
        AFDAlgoSetParam     setParam[1] = { { 0 } };
        AFDAlgoSetParamList setParamList;

        setParamList.numberOfSetParam = 0;
        setParamList.pAFDSetParamList = setParam;

        AFDSensorInfo*      pAFDSensorInfo = m_pAFDIOHandler->ReadSensorInput();
        setParam[0].pAFDSetParam         = static_cast<VOID*>(pAFDSensorInfo);
        setParam[0].setParamType = AFDAlgoSetParamSensorInfo;
        setParam[0].sizeOfAFDSetParam = sizeof(AFDSensorInfo);
        setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

        CDKResult cdkResult = g_pCHIAFDAlgorithm->AFDSetParam(g_pCHIAFDAlgorithm, &setParamList);
        result = StatsUtil::CdkResultToCamXResult(cdkResult);
    }
    else
    {
        if (CamxResultSuccess == result)
        {
            result = PrepareInputParams(&input);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupStats,
                               "RequestId=%llu Failed to PrepareInputParams",
                               pStatsProcessRequestDataInfo->requestId);
            }
        }

        if (CamxResultSuccess == result)
        {
            result = ProcessSetParams(&requestId, pStatsProcessRequestDataInfo->pTuningModeData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupAFD,
                    "RequestId=%llu Failed to ProcessSetParams",
                    pStatsProcessRequestDataInfo->requestId);
            }
        }

        if (CamxResultSuccess == result)
        {
            result = g_pCHIAFDAlgorithm->AFDProcess(g_pCHIAFDAlgorithm, &input, pOutput);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupAFD,
                    "RequestId=%llu Failed AFDProcess",
                    pStatsProcessRequestDataInfo->requestId);
            }
        }
    }

    // Publish Output even in error case
    if (NULL != pOutput)
    {
        m_pAFDIOHandler->PublishOutput(pOutput, requestId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::GetPrevConfigFromAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAFDStatsProcessor::GetPrevConfigFromAlgo()
{
    AFDAlgoGetParam         getParam[2];
    AFDAlgoGetParamList     getParamList;
    UINT32                  index = 0;

    // Get RS Config data
    getParam[index].type                    = AFDAlgoGetParamStatsConfig;
    getParam[index].input.pInputData        = NULL;
    getParam[index].input.sizeOfInputData   = 0;
    getParam[index].output.pOutputData      = &m_pAFDIOHandler->m_rowSumConfig;
    getParam[index].output.sizeOfOutputData = static_cast<UINT32>(sizeof(m_pAFDIOHandler->m_rowSumConfig));
    index++;

    // Get last detect AFD mode
    getParam[index].type                    = AFDAlgoGetParamLastDetectedMode;
    getParam[index].input.pInputData        = NULL;
    getParam[index].input.sizeOfInputData   = 0;
    getParam[index].output.pOutputData      = &m_pAFDIOHandler->m_lastAFDMode;
    getParam[index].output.sizeOfOutputData = static_cast<UINT32>(sizeof(m_pAFDIOHandler->m_lastAFDMode));
    index++;

    getParamList.numberOfAFDGetParams = index;
    getParamList.pAFDGetParamList     = &getParam[0];

    if (NULL != g_pCHIAFDAlgorithm)
    {
        g_pCHIAFDAlgorithm->AFDGetParam(g_pCHIAFDAlgorithm, &getParamList);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::PrepareInputParams()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDStatsProcessor::PrepareInputParams(
    AFDAlgoInputList* pInputList)
{
    CAMX_ENTRYEXIT(CamxLogGroupAFD);
    CamxResult result = CamxResultSuccess;

    pInputList->pAFDInputList = m_inputArray;
    pInputList->inputCount    = 0;

    if (NULL == m_pAFDIOHandler)
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "AFD IO Handler NULL pointer");
        return CamxResultEInvalidPointer;
    }

    StatsBayerGrid* pBGStat   = m_pAFDIOHandler->ReadBGStat();
    if (NULL == pBGStat)
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "BG Stat NULL pointer");
        return CamxResultEInvalidPointer;
    }
    else
    {
        UpdateInputParam(pInputList, AFDAlgoInputType::AFDAlgoInputBayerGrid,
            sizeof(StatsBayerGrid), static_cast<VOID*>(pBGStat));
    }

    StatsRowSum* pRStat    = m_pAFDIOHandler->ReadRowSumStat();
    if (NULL == pRStat)
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "pRStat NULL pointer");
        return CamxResultEInvalidPointer;
    }
    else
    {
        UpdateInputParam(pInputList, AFDAlgoInputType::AFDAlgoInputRowSum,
            sizeof(StatsRowSum), static_cast<VOID*>(pRStat));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::UpdateInputParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE VOID CAFDStatsProcessor::UpdateInputParam(
    AFDAlgoInputList* pInputList,
    AFDAlgoInputType  inputType,
    UINT32            inputSize,
    VOID*             pValue)
{
    CAMX_ENTRYEXIT(CamxLogGroupAFD);
    pInputList->pAFDInputList[pInputList->inputCount].inputType      = inputType;
    pInputList->pAFDInputList[pInputList->inputCount].sizeOfAFDInput = static_cast<UINT32>(inputSize);
    pInputList->pAFDInputList[pInputList->inputCount].pAFDInput      = pValue;
    pInputList->inputCount++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::SetOverrideAlgoSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDStatsProcessor::SetOverrideAlgoSetting()
{
    CAMX_ENTRYEXIT(CamxLogGroupAFD);

    CamxResult                  result = CamxResultSuccess;

    AFDAlgoSetParam             setParam[1] = { { 0 } };
    AFDAlgoSetParamList         setParamList;
    AFDAlgoOverrideSettingParam overridesettings;

    setParamList.numberOfSetParam = 0;
    setParamList.pAFDSetParamList = setParam;

    overridesettings.profile3A    = m_pStaticSettings->profile3A;

    setParam[0].pAFDSetParam      = static_cast<VOID*>(&overridesettings);
    setParam[0].setParamType      = AFDAlgoSetParamOverrideSettings;
    setParam[0].sizeOfAFDSetParam = sizeof(overridesettings);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    CDKResult cdkResult = g_pCHIAFDAlgorithm->AFDSetParam(g_pCHIAFDAlgorithm, &setParamList);
    result = StatsUtil::CdkResultToCamXResult(cdkResult);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::SetCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDStatsProcessor::SetCameraInfo(
    StatsCameraInfo cameraInfo)
{
    CAMX_ENTRYEXIT(CamxLogGroupAFD);

    CamxResult                  result = CamxResultSuccess;

    AFDAlgoSetParam             setParam[1] = { { 0 } };
    AFDAlgoSetParamList         setParamList;

    setParamList.numberOfSetParam = 0;
    setParamList.pAFDSetParamList = setParam;

    setParam[0].pAFDSetParam      = static_cast<VOID*>(&cameraInfo);
    setParam[0].setParamType      = AFDAlgoSetParamCameraInfo;
    setParam[0].sizeOfAFDSetParam = sizeof(cameraInfo);
    setParamList.numberOfSetParam = setParamList.numberOfSetParam + 1;

    CDKResult cdkResult = g_pCHIAFDAlgorithm->AFDSetParam(g_pCHIAFDAlgorithm, &setParamList);
    result = StatsUtil::CdkResultToCamXResult(cdkResult);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::ProcessSetParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDStatsProcessor::ProcessSetParams(
    UINT64*                 pRequestId,
    ChiTuningModeParameter* pInputTuningModeData)
{
    CAMX_ENTRYEXIT(CamxLogGroupAFD);

    CamxResult          result                         = CamxResultSuccess;
    AFDAlgoSetParam     setParam[AFDSetParamLastIndex] = { { 0 } };
    AFDAECInfo*         pAFDAECInfo                    = NULL;
    AFDAFInfo*          pAFDAFInfo                     = NULL;
    AFDSensorInfo*      pAFDSensorInfo                 = NULL;
    AFDAlgoSetParamList setParamList;
    UINT                binningTypeVertical;
    FLOAT               maxNumOfSensorLinesPerSecond;
    UINT32              numOfLinesCoveredByRowStats;
    UINT32              setParamCount = 0;

    setParamList.pAFDSetParamList = setParam;

    if (NULL == m_pAFDIOHandler)
    {
        CAMX_LOG_ERROR(CamxLogGroupAFD, "AFD IO Handler NULL pointer");
        result = CamxResultEInvalidPointer;
        return result;
    }

    setParam[setParamCount].pAFDSetParam       = static_cast<VOID*>(pRequestId);
    setParam[setParamCount].setParamType       = AFDAlgoSetParamFrameId;
    setParam[setParamCount].sizeOfAFDSetParam  = sizeof(UINT64);
    setParamCount++;

    pAFDAECInfo                                         = m_pAFDIOHandler->ReadAECInput();
    setParam[setParamCount].pAFDSetParam       = static_cast<VOID*>(pAFDAECInfo);
    setParam[setParamCount].setParamType       = AFDAlgoSetParamAECInfo;
    setParam[setParamCount].sizeOfAFDSetParam  = sizeof(AFDAECInfo);
    setParamCount++;

    pAFDAFInfo                                          = m_pAFDIOHandler->ReadAFInput();
    setParam[setParamCount].pAFDSetParam        = static_cast<VOID*>(pAFDAFInfo);
    setParam[setParamCount].setParamType        = AFDAlgoSetParamAFInfo;
    setParam[setParamCount].sizeOfAFDSetParam   = sizeof(AFDAFInfo);
    setParamCount++;

    pAFDSensorInfo = m_pAFDIOHandler->ReadSensorInput();
    if (pAFDSensorInfo->binningTypeV == 0)
    {
        binningTypeVertical = 1;
    }
    else
    {
        binningTypeVertical = pAFDSensorInfo->binningTypeV;
    }

    /* compute timing settings: compute number of sensor lines in one second (sensor lines are configured based
    on max FPS and not changing with respect to cur FPS, hence use max FPS in computing), then multiple by number
    of lines covered row stats array */
    maxNumOfSensorLinesPerSecond = pAFDSensorInfo->maxFPS * static_cast<FLOAT>(pAFDSensorInfo->numLinesPerFrame);
    if (maxNumOfSensorLinesPerSecond <= 0)
    {
        maxNumOfSensorLinesPerSecond = 1;
    }

    m_pAFDIOHandler->m_rowSumInfo.rowSumCount =
        m_pAFDIOHandler->m_rowSumInfo.horizontalRegionCount * m_pAFDIOHandler->m_rowSumInfo.verticalRegionCount;
    numOfLinesCoveredByRowStats = m_pAFDIOHandler->m_rowSumInfo.verticalRegionCount
                                  * m_pAFDIOHandler->m_rowSumInfo.regionHeight;

    // In BPS scenario, m_rowSumInfo does not contain accurate information as
    // the SW generator is not aware of the actual frame dimensions. However,
    // the SW generator will operate on the full downscaled image array, so
    // we can assume that the full pixel array is covered by SW RS stats.
    if (TRUE == m_pNode->IsCameraRunningOnBPS())
    {
        numOfLinesCoveredByRowStats = pAFDSensorInfo->numLinesPerFrame - pAFDSensorInfo->minVerticalBlanking;
    }

    pAFDSensorInfo->rowSumTime = (numOfLinesCoveredByRowStats / maxNumOfSensorLinesPerSecond);
    CAMX_LOG_VERBOSE(CamxLogGroupAFD, "maxNumOfSensorLinesPerSecond:%f rowSumCount:%d rowSumTime:%lf "
        "numOfLinesCoveredByRowStats %u horizontalRegionCount %d, verticalRegionCount %d, regionHeight %d, regionWidth %d",
        maxNumOfSensorLinesPerSecond,
        m_pAFDIOHandler->m_rowSumInfo.rowSumCount,
        pAFDSensorInfo->rowSumTime,
        numOfLinesCoveredByRowStats,
        m_pAFDIOHandler->m_rowSumInfo.horizontalRegionCount,
        m_pAFDIOHandler->m_rowSumInfo.verticalRegionCount,
        m_pAFDIOHandler->m_rowSumInfo.regionHeight,
        m_pAFDIOHandler->m_rowSumInfo.regionWidth);

    setParam[setParamCount].pAFDSetParam            = static_cast<VOID*>(&m_HALParam.antiBandingMode);
    setParam[setParamCount].setParamType            = AFDAlgoSetParamAFDMode;
    setParam[setParamCount].sizeOfAFDSetParam       = sizeof(m_HALParam.antiBandingMode);
    setParamCount++;

    setParam[setParamCount].pAFDSetParam         = static_cast<VOID*>(pAFDSensorInfo);
    setParam[setParamCount].setParamType         = AFDAlgoSetParamSensorInfo;
    setParam[setParamCount].sizeOfAFDSetParam    = sizeof(AFDSensorInfo);
    setParamCount++;

    StatsTuningData statsTuningData = { 0 };
    SetAlgoChromatix(pInputTuningModeData, &statsTuningData);
    setParam[setParamCount].pAFDSetParam      = static_cast<VOID*>(&statsTuningData);
    setParam[setParamCount].setParamType      = AFDAlgoSetParamChromatixData;
    setParam[setParamCount].sizeOfAFDSetParam = sizeof(StatsTuningData);
    setParamCount++;

    // Set whether realtime BPS camera usecase is selected
    BOOL isBPSCamera                            = m_pNode->IsCameraRunningOnBPS();
    setParam[setParamCount].pAFDSetParam        = static_cast<VOID*>(&isBPSCamera);
    setParam[setParamCount].setParamType        = AFDAlgoSetParamBPSCamera;
    setParam[setParamCount].sizeOfAFDSetParam   = sizeof(BOOL);
    setParamCount++;

    BOOL dumpRowSumStats                                     = FALSE;
    if (ControlCaptureIntentStillCapture == m_HALParam.captureIntent &&
        TRUE                             == m_pStaticSettings->dumpAFDRowsum)
    {
        dumpRowSumStats                                       = TRUE;
        setParam[setParamCount].pAFDSetParam      = static_cast<VOID*>(&dumpRowSumStats);
        setParam[setParamCount].setParamType      = AFDAlgoSetParamDumpRowSum;
        setParam[setParamCount].sizeOfAFDSetParam = sizeof(dumpRowSumStats);
        setParamCount++;
    }
    setParamList.numberOfSetParam = setParamCount;
    CDKResult cdkResult = g_pCHIAFDAlgorithm->AFDSetParam(g_pCHIAFDAlgorithm, &setParamList);
    result              = StatsUtil::CdkResultToCamXResult(cdkResult);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::GetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDStatsProcessor::GetDependencies(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    StatsDependency*               pStatsDependency)
{
    CAMX_UNREFERENCED_PARAM(pStatsDependency);
    CAMX_ENTRYEXIT(CamxLogGroupAFD);
    CamxResult result                       = CamxResultSuccess;
    UINT       maxPipelineDelay             = m_pNode->GetMaximumPipelineDelay();
    UINT64     requestIdOffsetFromLastFlush =
        m_pNode->GetRequestIdOffsetFromLastFlush(pStatsProcessRequestDataInfo->requestId);

    if (FALSE == m_pStaticSettings->disableAFDStatsProcessing)
    {
        if (TRUE == pStatsProcessRequestDataInfo->skipProcessing)
        {
            // We need to publish previous frame data to property pool when skipProcessing flag is set.
            // Add previous frame(offset = 1) AFD Frame/Stats Control properties as dependency.
            pStatsDependency->properties[pStatsDependency->propertyCount++] = { PropertyIDAFDFrameInfo, 1 };

            // For AF Frame Info and AEC Frame info dependency for offset = pipeline delay
            pStatsDependency->properties[pStatsDependency->propertyCount++] = { PropertyIDAFFrameInfo, maxPipelineDelay };

            pStatsDependency->properties[pStatsDependency->propertyCount++] = { PropertyIDAECFrameInfo, maxPipelineDelay };
        }

        if (FirstValidRequestId < requestIdOffsetFromLastFlush)
        {
            pStatsDependency->properties[pStatsDependency->propertyCount++] = { PropertyIDAFDFrameInfo, 1 };
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::IsDependenciesSatisfied
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDStatsProcessor::IsDependenciesSatisfied(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    BOOL*                          pIsSatisfied)
{
    CAMX_ENTRYEXIT(CamxLogGroupAFD);

    CamxResult   result           = CamxResultSuccess;
    BOOL         isSatisfiedHDRBE = FALSE;
    BOOL         isSatisfiedRS    = FALSE;
    ISPStatsType statsType;

    for (INT32 i = 0; i < pStatsProcessRequestDataInfo->bufferCount; i++)
    {
        statsType = pStatsProcessRequestDataInfo->bufferInfo[i].statsType;
        switch (statsType)
        {
            case ISPStatsTypeHDRBE:
                isSatisfiedHDRBE = TRUE;
                break;
            case ISPStatsTypeRS:
                isSatisfiedRS = TRUE;
                break;
            default:
                break;
        }
    }

    *pIsSatisfied = (isSatisfiedHDRBE && isSatisfiedRS);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::~CAFDStatsProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAFDStatsProcessor::~CAFDStatsProcessor()
{
    CAMX_ENTRYEXIT(CamxLogGroupAFD);

    if (NULL != m_pAFDAlgorithmHandler)
    {
        CAMX_DELETE m_pAFDAlgorithmHandler;
        m_pAFDAlgorithmHandler = NULL;
    }

    if (NULL != m_pAFDIOHandler)
    {
        CAMX_DELETE m_pAFDIOHandler;
        m_pAFDIOHandler = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFDStatsProcessor::ReadHALAFDParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDStatsProcessor::ReadHALAFDParam(
    AFDHALParam* pHALParam
    ) const
{
    CamxResult result       = CamxResultSuccess;
    UINT       HALAFDData[] =
    {
        InputControlAEAntibandingMode,
        InputControlCaptureIntent
    };
    static const UINT HALAFDDataLength        = CAMX_ARRAY_SIZE(HALAFDData);
    VOID*  pData[HALAFDDataLength]            = { 0 };
    UINT64 HALAFDDataOffset[HALAFDDataLength] = { 0 };

    m_pAFDIOHandler->m_pNode->GetDataList(HALAFDData, pData, HALAFDDataOffset, HALAFDDataLength);

    if ((NULL != pData[0]) && (NULL != pData[1]))
    {
        pHALParam->antiBandingMode = *(reinterpret_cast<ControlAEAntibandingModeValues*>(pData[0]));
        pHALParam->captureIntent = *(reinterpret_cast<ControlCaptureIntentValues*>(pData[1]));

        m_pAFDIOHandler->SetHALParam(pHALParam);
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupAFD,
            "InputControlAEAntibandingMode:%p InputControlCaptureIntent:%p not published",
            pData[0],
            pData[1]);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAFDStatsProcessor::SetAlgoChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDStatsProcessor::SetAlgoChromatix(
    ChiTuningModeParameter* pInputTuningModeData,
    StatsTuningData*        pTuningData)
{
    CamxResult              result = CamxResultSuccess;

    if (NULL != pInputTuningModeData)
    {
        pTuningData->pTuningSetManager    = m_pTuningDataManager;
        pTuningData->pTuningModeSelectors = reinterpret_cast<TuningMode*>(&pInputTuningModeData->TuningMode[0]);
        pTuningData->numSelectors         = pInputTuningModeData->noOfSelectionParameter;
        CAMX_LOG_VERBOSE(CamxLogGroupAFD,
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
        CAMX_LOG_ERROR(CamxLogGroupAFD, "Input tuning data is NULL pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAFDStatsProcessor::GetPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAFDStatsProcessor::GetPublishList(
    const UINT32    maxTagArraySize,
    UINT32*         pTagArray,
    UINT32*         pTagCount,
    UINT32*         pPartialTagCount)
{
    CamxResult result          = CamxResultSuccess;
    UINT32     numMetadataTags = CAMX_ARRAY_SIZE(AFDOutputMetadataTags);

    if (numMetadataTags <= maxTagArraySize)
    {
        for (UINT32 tagIndex = 0; tagIndex < numMetadataTags; ++tagIndex)
        {
            pTagArray[tagIndex] = AFDOutputMetadataTags[tagIndex];
        }
    }
    else
    {
        result = CamxResultEOutOfBounds;
        CAMX_LOG_ERROR(CamxLogGroupMeta, "ERROR %d tags cannot be published.", numMetadataTags);
    }

    if (CamxResultSuccess == result)
    {
        *pTagCount        = numMetadataTags;
        *pPartialTagCount = 0;
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%d tags will be published.", numMetadataTags);
    }
    return result;
}

CAMX_NAMESPACE_END
