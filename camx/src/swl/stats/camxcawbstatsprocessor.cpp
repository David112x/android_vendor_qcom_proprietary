////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcawbstatsprocessor.cpp
/// @brief The class that implements IStatsProcessor for AWB.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcawbioutil.h"
#include "camxcawbstatsprocessor.h"
#include "camxhal3module.h" /// @todo (CAMX-1796) Remove all references of HAL3 from core driver
#include "camxmem.h"
#include "camxstatsdebuginternal.h"
#include "camxtrace.h"

CAMX_NAMESPACE_BEGIN

const CHAR* pDefaultAlgorithmAWBLibraryName = "com.qti.stats.awb";
static const CHAR* pFunctionName            = "CreateAWBAlgorithm";

// Forward declaration of implementation classes
class  TunedParameterManager;

// @brief Number of Partial Meta Data Tag from the Publish Tag list
static const UINT NumberOfPartialMetadataTags = 3;

// @brief list of tags published
static const UINT32 AWBOutputMetadataTags[] =
{
    ControlAWBLock,
    ControlAWBMode,
    ControlAWBState,
    PropertyIDCrossAWBStats,
    PropertyIDAWBPeerInfo,
    PropertyIDUsecaseAWBFrameControl,
    PropertyIDUsecaseAWBStatsControl,
    PropertyIDAWBFrameControl,
    PropertyIDAWBFrameInfo,
    PropertyIDAWBInternal,
    PropertyIDAWBStatsControl,
    PropertyIDAWBFrameInfo
};

// @brief list of vendor tags published
static const struct NodeVendorTag AWBOutputVendorTags[] =
{
    { "org.quic.camera2.statsconfigs" , "AWBFrameControl" },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::Create(
    IStatsProcessor** ppAWBStatsProcessor)
{
    CamxResult          result              = CamxResultSuccess;
    CAWBStatsProcessor* pAWBStatsProcessor  = NULL;

    if (NULL != ppAWBStatsProcessor)
    {
        pAWBStatsProcessor = CAMX_NEW CAWBStatsProcessor;
        if (NULL != ppAWBStatsProcessor)
        {
            *ppAWBStatsProcessor = pAWBStatsProcessor;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupAWB, "AWB stats processor create failed - out of memory");
            result = CamxResultENoMemory;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "AWB stats processor create failed - Invalid arguments");
        result = CamxResultEInvalidArg;
    }

    CAMX_LOG_INFO(CamxLogGroupAWB, "CAWBStatsProcessor::Create result = %s", Utils::CamxResultToString(result));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::CAWBStatsProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAWBStatsProcessor::CAWBStatsProcessor()
    : m_hHandle(NULL)
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::VendorTagListAllocation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::VendorTagListAllocation()
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);

    CamxResult      result       = CamxResultSuccess;
    AWBAlgoGetParam algoGetParam = {};

    algoGetParam.type = AWBGetParamTypePublishingVendorTagsInfo;

    result = AlgoGetParam(NULL, &algoGetParam);

    if (CamxResultSuccess == result)
    {
        // allocate memory to hold each vendor tag's data
        result = m_pAWBIOUtil->AllocateMemoryVendorTag(&algoGetParam);

        if (CamxResultSuccess == result)
        {
            m_pOutputVendorTagInfoList = static_cast<StatsVendorTagInfoList*>(
                algoGetParam.outputInfoList.pGetParamOutputs[AWBGetParamOutputTypePublishingVendorTagsInfo].pGetParamOutput);
        }
        else
        {
            m_pOutputVendorTagInfoList = NULL;
            CAMX_LOG_ERROR(CamxLogGroupAWB, "Cannot get publishlist");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::Initialize(
    const StatsInitializeData* pInitializeData)
{
    if (NULL == pInitializeData)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "pInitializeData is Null");
        return CamxResultEInvalidPointer;
    }

    CamxResult              result          = CamxResultSuccess;
    const CHAR*             pLibraryName    = NULL;
    const CHAR*             pLibraryPath    = NULL;
    VOID*                   pFuncCreateAWB  = NULL;

    m_pStaticSettings = pInitializeData->pHwContext->GetStaticSettings();

    if (NULL == m_pStaticSettings)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "m_pStaticSettings is Null");
        return CamxResultEInvalidPointer;
    }

    AWBAlgoCreateParamList              createParamList                       = { 0 };
    StatsTuningData                     statsTuningData                       = { 0 };
    AWBAlgoIlluminantsCalibrationFactor illuminantsCalibrationFactor          = { 0 };
    AWBAlgoCreateParam                  createParams[AWBCreateParamTypeCount] = {};
    StatsStreamInitConfig               statsStreamInitConfig                 = {};
    StatsCameraInfo                     cameraInfo                            = pInitializeData->cameraInfo;

    // Store the Camera Info
    m_cameraInfo = cameraInfo;
    m_isFixedFocus = FALSE;

    // Read from use case pool to check if we have results from Fast AEC
    statsStreamInitConfig.operationMode = StatsOperationModeNormal;
    StatsUtil::GetStatsStreamInitConfig(
        pInitializeData->pPipeline->GetPerFramePool(PoolType::PerUsecase), &statsStreamInitConfig);

    m_pAWBIOUtil = CAMX_NEW CAWBIOUtil;
    if (NULL != m_pAWBIOUtil)
    {
        result = m_pAWBIOUtil->Initialize(pInitializeData);

        HwCameraInfo hwcamerraInfo;

        m_pNode = pInitializeData->pNode;

        result = HwEnvironment::GetInstance()->GetCameraInfo(cameraInfo.cameraId, &hwcamerraInfo);

        if (CamxResultSuccess == result)
        {
            m_pAWBIOUtil->FillIlluminantCalibrationFactor(&hwcamerraInfo.pSensorCaps->OTPData.WBCalibration[0],
                &illuminantsCalibrationFactor);

            // AWB has dependency on AF for depth information (except when sensor is fixed focus),
            // AF node does not publish depth info when disableAFStatsProcessing is True,
            // hence m_isFixedFocus is set to TRUE when AF processing is disabled
            // so that AWB does not add dependencies on AF

            if ((TRUE == hwcamerraInfo.pSensorCaps->isFixedFocus) || (TRUE == m_pStaticSettings->disableAFStatsProcessing))
            {
                m_isFixedFocus = TRUE;
            }
            CAMX_LOG_VERBOSE(CamxLogGroupAWB, "fixed focus = %d", m_isFixedFocus);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupAWB, "Failed to get camera info");
        }

        statsTuningData.pTuningSetManager = reinterpret_cast<VOID*>(pInitializeData->pTuningDataManager->GetChromatix());

        createParams[AWBCreateParamsTypeTuningData].createParamType           = AWBCreateParamsTypeTuningData;
        createParams[AWBCreateParamsTypeTuningData].pCreateParam              = &statsTuningData;
        createParams[AWBCreateParamsTypeTuningData].sizeOfCreateParam         = sizeof(statsTuningData);

        createParams[AWBCreateParamsTypeLoggerFunctionPtr].createParamType    = AWBCreateParamsTypeLoggerFunctionPtr;
        createParams[AWBCreateParamsTypeLoggerFunctionPtr].pCreateParam       = reinterpret_cast<VOID*>(&StatsLoggerFunction);
        createParams[AWBCreateParamsTypeLoggerFunctionPtr].sizeOfCreateParam  = sizeof(StatsLoggingFunction);

        createParams[AWBCreateParamTypeOperationMode].createParamType         = AWBCreateParamTypeOperationMode;
        createParams[AWBCreateParamTypeOperationMode].pCreateParam            = reinterpret_cast<VOID*>(
                                                                                                 &statsStreamInitConfig);
        createParams[AWBCreateParamTypeOperationMode].sizeOfCreateParam       = sizeof(statsStreamInitConfig);

        ChiStatsSession* pStatsSession                                        = m_pAWBIOUtil->GetChiStatsSessionHandle();
        createParams[AWBCreateParamTypeSessionHandle].createParamType         = AWBCreateParamTypeSessionHandle;
        createParams[AWBCreateParamTypeSessionHandle].pCreateParam            = static_cast<VOID*>(pStatsSession);
        createParams[AWBCreateParamTypeSessionHandle].sizeOfCreateParam       = sizeof(ChiStatsSession);

        UINT* pOverrideCameraOpen                                             = (UINT *)&m_pStaticSettings->overrideCameraOpen;
        createParams[AWBCreateParamTypeCameraOpenIndicator].createParamType   = AWBCreateParamTypeCameraOpenIndicator;
        createParams[AWBCreateParamTypeCameraOpenIndicator].pCreateParam      = static_cast<VOID*>(pOverrideCameraOpen);
        createParams[AWBCreateParamTypeCameraOpenIndicator].sizeOfCreateParam = sizeof(UINT);

        createParams[AWBCreateParamTypeCameraInfo].createParamType            = AWBCreateParamTypeCameraInfo;
        createParams[AWBCreateParamTypeCameraInfo].pCreateParam               = static_cast<VOID*>(&cameraInfo);
        createParams[AWBCreateParamTypeCameraInfo].sizeOfCreateParam          = sizeof(StatsCameraInfo);

        createParams[AWBCreateParamsTypeCalibrationData].createParamType      = AWBCreateParamsTypeCalibrationData;
        createParams[AWBCreateParamsTypeCalibrationData].pCreateParam         = static_cast<VOID*>(
                                                                                            &illuminantsCalibrationFactor);
        createParams[AWBCreateParamsTypeCalibrationData].sizeOfCreateParam    = sizeof(illuminantsCalibrationFactor);

        createParamList.createParamsCount = AWBCreateParamTypeCount;
        createParamList.pCreateParams     = &createParams[0];

        if (CamxResultSuccess == result)
        {
            if (NULL != pInitializeData->initializecallback.pAWBCallback)
            {
                m_pfnCreate = pInitializeData->initializecallback.pAWBCallback->pfnCreate;
            }

            if (TRUE == m_pStaticSettings->disableAWBAlgoCHIOverload)
            {

                if (FALSE == m_pStaticSettings->enableCustomAlgoAWB)
                {
                    pLibraryName = pDefaultAlgorithmAWBLibraryName;
                    pLibraryPath = DefaultAlgorithmPath;
                }
                else
                {
                    pLibraryName = m_pStaticSettings->customAlgoAWBName;
                    pLibraryPath = m_pStaticSettings->customAlgoAWBPath;
                }

                pFuncCreateAWB = StatsUtil::LoadAlgorithmLib(&m_hHandle, pLibraryPath, pLibraryName, pFunctionName);

                if (NULL == pFuncCreateAWB)
                {
                    result = CamxResultEUnableToLoad;
                    CAMX_ASSERT_ALWAYS_MESSAGE("Unable to load the algo library! (%s) Path:%s", pLibraryName, pLibraryPath);

                    if (NULL != m_hHandle)
                    {
                        OsUtils::LibUnmap(m_hHandle);
                        m_hHandle = NULL;
                    }
                }
                else
                {
                    CREATEAWB pFNCreateAWB = reinterpret_cast<CREATEAWB>(pFuncCreateAWB);
                    result = (*pFNCreateAWB)(&createParamList, &m_pAWBAlgorithm);
                }
            }
            else if (NULL != m_pfnCreate)
            {
                result = (*m_pfnCreate)(&createParamList, &m_pAWBAlgorithm);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupAWB, "Failed to create algo instance: %s", Utils::CamxResultToString(result));
                }
            }
        }

        if ((CamxResultSuccess != result) || (NULL == m_pAWBAlgorithm))
        {
            CAMX_LOG_ERROR(CamxLogGroupAWB,
                "Failed to initialize results: %s, m_pAWBAlgorithm: %p, Create pointer: %p",
                Utils::CamxResultToString(result),
                m_pAWBAlgorithm,
                m_pStaticSettings->disableAWBAlgoCHIOverload ? pFuncCreateAWB : reinterpret_cast<VOID*>(m_pfnCreate));

            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            m_pAWBIOUtil->SetAwbAlgo(m_pAWBAlgorithm);
        }
        if (CamxResultSuccess == result)
        {
            result = VendorTagListAllocation();
        }

        if (CamxResultSuccess == result)
        {
            result = SetOverrideAlgoSetting();
        }

        if (CamxResultSuccess == result)
        {
            result = SetOperationModetoAlgo(pInitializeData->statsStreamInitConfig.operationMode);
        }

        if (CamxResultSuccess == result)
        {
            result = SetPipelineDelaytoAlgo(m_pNode->GetMaximumPipelineDelay());
        }

        if (CamxResultSuccess == result)
        {
            m_pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

            BOOL isEarlyPCREnabled = FALSE;
            m_pNode->IsEarlyPCREnabled(&isEarlyPCREnabled);
            if (FALSE == isEarlyPCREnabled)
            {
                result = PublishPreRequestOutput();
                if (result != CamxResultSuccess)
                {
                    CAMX_LOG_ERROR(CamxLogGroupAWB, "Failed to publish pre request data result: %s",
                                   Utils::CamxResultToString(result));
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "IOUtil Create failed - out of memory");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::ExecuteProcessRequest(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo)
{
    if (NULL == pStatsProcessRequestDataInfo || NULL == m_pNode ||
        NULL == m_pAWBIOUtil || NULL == m_pAWBAlgorithm)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "NULL pointer pStatsProcessRequestDataInfo: %p, "
        "m_pNode: %p, m_pAWBIOUtil: %p, m_pAWBAlgorithm: %p",
            pStatsProcessRequestDataInfo, m_pNode,
            m_pAWBIOUtil, m_pAWBAlgorithm);
        return CamxResultEFailed;
    }

    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupAWB, SCOPEEventCAWBStatsProcessorExecuteProcessRequest,
        pStatsProcessRequestDataInfo->requestId);

    CamxResult              result                          = CamxResultSuccess;
    AWBAlgoInputList        algoInputList                   = { 0 };
    AWBAlgoOutputList       algoOutputList                  = { 0 };
    AWBAlgoSetParamList     algoSetParamList                = { 0 };
    AWBAlgoGetParam         algoGetParam                    = {};
    AWBAlgoOutputList*      pFinalOutputList                = &algoOutputList;
    UINT64                  requestIdOffsetFromLastFlush    =
        m_pNode->GetRequestIdOffsetFromLastFlush(pStatsProcessRequestDataInfo->requestId);

    m_pAWBIOUtil->InvalidateIO();

    BOOL isAECSettled = FALSE;

    m_cameraInfo = pStatsProcessRequestDataInfo->cameraInfo;

    if ((TRUE == pStatsProcessRequestDataInfo->skipProcessing &&
        (FirstValidRequestId < requestIdOffsetFromLastFlush)) ||
        TRUE == m_pStaticSettings->disableAWBStatsProcessing)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAWB,
                         "Skipping AWB Algo processing for RequestId=%llu, skipProcessing=%d, disableAWBStatsProcessing=%d",
                         pStatsProcessRequestDataInfo->requestId,
                         pStatsProcessRequestDataInfo->skipProcessing,
                         m_pStaticSettings->disableAWBStatsProcessing);

        // If AWB stats processing is disabled we publish some hard coded values to the metadata pool
        algoGetParam.type = AWBGetParamTypeLastOutput;
        result = AlgoGetParam(pStatsProcessRequestDataInfo, &algoGetParam);
        pFinalOutputList = static_cast<AWBAlgoOutputList*>(
            algoGetParam.outputInfoList.pGetParamOutputs[AWBGetParamOutputTypeOutputList].pGetParamOutput);
    }
    else
    {
        m_pAWBIOUtil->ReadAWBProperties(requestIdOffsetFromLastFlush);

        BOOL parseStats = (m_pNode->GetMaximumPipelineDelay() < requestIdOffsetFromLastFlush) ? TRUE : FALSE;

        BOOL isEarlyPCREnabled = FALSE;
        m_pNode->IsEarlyPCREnabled(&isEarlyPCREnabled);

        if ((FirstValidRequestId == requestIdOffsetFromLastFlush) &&
            (TRUE == isEarlyPCREnabled))
        {
            m_pAWBIOUtil->PopulateCameraInformation(pStatsProcessRequestDataInfo);
            // Early PCR is enabled, pass proper tuning parameter to algo and get and publish initial AWB config
            m_pAWBIOUtil->SetAlgoChromatix(pStatsProcessRequestDataInfo->pTuningModeData, &algoSetParamList);
            result = m_pAWBAlgorithm->AWBSetParam(m_pAWBAlgorithm, &algoSetParamList);

            if (CamxResultSuccess == result)
            {
                result = PublishPreRequestOutput();
                if (result != CamxResultSuccess)
                {
                    CAMX_LOG_ERROR(CamxLogGroupAWB,
                        "Failed to publish pre request data result: %s", Utils::CamxResultToString(result));
                }
            }
        }


        // AWB read with offset 1 from AEC
        if (FirstValidRequestId < requestIdOffsetFromLastFlush)
        {
            isAECSettled = m_pAWBIOUtil->IsAECSettled();
        }


        if (CamxResultSuccess == result)
        {
            if (NULL == m_pAWBAlgorithm)
            {
                CAMX_LOG_ERROR(CamxLogGroupAWB, "m_pAWBAlgorithm is Null");
                return CamxResultEInvalidPointer;
            }
            result = m_pAWBIOUtil->GetAlgoSetParamInput(pStatsProcessRequestDataInfo, &algoSetParamList);
        }

        if (CamxResultSuccess == result)
        {
            if (((StatsOperationModeFastConvergence == pStatsProcessRequestDataInfo->operationMode) &&
                (FALSE == isAECSettled)))
            {
                CAMX_LOG_VERBOSE(CamxLogGroupAWB, "Skip AWB processing. ReqId %llu OpMode: %d AEC Settled %d",
                    pStatsProcessRequestDataInfo->requestId,
                    pStatsProcessRequestDataInfo->operationMode,
                    isAECSettled);
                parseStats = FALSE;
            }

            if ((StatsAlgoRoleDefault != m_cameraInfo.algoRole) &&
                (MultiCamera3ASyncDisabled != m_pStaticSettings->multiCamera3ASync) &&
                (TRUE == m_pNode->IsMultiCamera()))
            {
                result = m_pAWBIOUtil->SetPeerInfo(pStatsProcessRequestDataInfo, &algoSetParamList);
            }
        }

        if (CamxResultSuccess == result)
        {
            result = m_pAWBAlgorithm->AWBSetParam(m_pAWBAlgorithm, &algoSetParamList);
        }

        if (CamxResultSuccess == result)
        {
            result = m_pAWBIOUtil->GetAlgoProcessInput(pStatsProcessRequestDataInfo, &algoInputList, parseStats);
            if (CamxResultSuccess != result)
            {
                parseStats = FALSE;
            }
        }

        // Process stats
        if (CamxResultSuccess == result)
        {
            m_pAWBIOUtil->GetAlgoExpectedOutputList(pStatsProcessRequestDataInfo, &algoOutputList);
            result = m_pAWBAlgorithm->AWBProcess(m_pAWBAlgorithm, &algoInputList, &algoOutputList);
            if ((CamxResultSuccess == result) && (FALSE == parseStats))
            {
                if (CamxResultSuccess != IsValidAlgoOutput(&algoOutputList, pStatsProcessRequestDataInfo->requestId))
                {
                    algoGetParam.type = AWBGetParamTypeLastOutput;
                    result = AlgoGetParam(pStatsProcessRequestDataInfo, &algoGetParam);
                    pFinalOutputList = static_cast<AWBAlgoOutputList*>(
                        algoGetParam.outputInfoList.pGetParamOutputs[AWBGetParamOutputTypeOutputList].pGetParamOutput);
                }
                else
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupAWB,
                        "Valid AWB Algo output for RequestId=%llu",
                        pStatsProcessRequestDataInfo->requestId);
                }
            }

            // Publish cross pipeline property for multi camera
            m_pAWBIOUtil->PublishCrossProperty(pStatsProcessRequestDataInfo);
        }

        // query for Flash estimation state
        if (CamxResultSuccess == result)
        {
            algoGetParam.type = AWBGetParamTypeFlashEstimationState;
            result = AlgoGetParam(pStatsProcessRequestDataInfo, &algoGetParam);
        }

        if (CamxResultSuccess == result)
        {
            AWBAlgoFlashEstimationProgress* pFlashEstimationState = static_cast<AWBAlgoFlashEstimationProgress*>(
                algoGetParam.outputInfoList.pGetParamOutputs[AWBGetParamOutputTypeFlashEstimationProgress].pGetParamOutput);
            if (((NULL != pFlashEstimationState) && (AWBAlgoFlashEstimationDone == *pFlashEstimationState)) ||
                ((FirstValidRequestId == requestIdOffsetFromLastFlush) &&
                 (AWBAlgoMainFlash == m_pAWBIOUtil->GetAlgoFlashSate())))
            {
                // Preflash complete. Retrieve Flash Gain.
                algoGetParam.type = AWBGetParamTypeFlashOutput;
                result = AlgoGetParam(pStatsProcessRequestDataInfo, &algoGetParam);
            }
        }

        m_pAWBIOUtil->UpdateTraceEvents();
    }

    if ((CamxResultSuccess == result) && (StatsAlgoRoleDefault != m_cameraInfo.algoRole) &&
        (MultiCamera3ASyncDisabled != m_pStaticSettings->multiCamera3ASync))
    {
        algoGetParam.type = AWBGetParamTypePeerInfo;
        result = AlgoGetParam(pStatsProcessRequestDataInfo, &algoGetParam);

        if (CamxResultSuccess == result)
        {
            VOID* pPeerInfo = algoGetParam.outputInfoList.pGetParamOutputs[AWBGetParamOutputTypePeerInfo].pGetParamOutput;
            result = m_pAWBIOUtil->PublishPeerInfo(pStatsProcessRequestDataInfo, pPeerInfo);
        }
    }

    if (CamxResultSuccess == result)
    {
        result = m_pAWBIOUtil->PublishMetadata(pStatsProcessRequestDataInfo,
            pFinalOutputList);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::AddOEMDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::AddOEMDependencies(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    StatsDependency*                pStatsDependency)
{
    CamxResult              result             = CamxResultSuccess;
    AWBAlgoGetParam         algoGetParam       = {};
    StatsVendorTagInfoList* pVendorTagInfoList = NULL;

    // Get list of dependent vendor tags from algo
    algoGetParam.type     = AWBGetParamTypeDependentVendorTags;

    m_pAWBIOUtil->GetAlgoGetParamInputOutput(pStatsProcessRequestDataInfo, &algoGetParam);
    result = m_pAWBAlgorithm->AWBGetParam(m_pAWBAlgorithm, &algoGetParam);


    if (CamxResultSuccess == result)
    {
        AWBAlgoGetParamOutputList*  pGetParamOutput = &algoGetParam.outputInfoList;
        for (UINT32 i = 0; i < pGetParamOutput->getParamOutputCount; i++)
        {
            if ((AWBGetParamOutputTypeDependentVendorTags == pGetParamOutput->pGetParamOutputs[i].getParamOutputType) &&
                (0 != pGetParamOutput->pGetParamOutputs[i].sizeOfWrittenGetParamOutput))
            {
                pVendorTagInfoList =
                    static_cast<StatsVendorTagInfoList*>(algoGetParam.outputInfoList.pGetParamOutputs[i].pGetParamOutput);
                break;
            }
        }
    }

    // Find and add unpublished vendor tags to the dependency list
    // Note that some algo may not have any dependent vendor tags
    if ((CamxResultSuccess == result) && (NULL != pVendorTagInfoList))
    {
        UINT32  propertyCount = Utils::MinUINT32(pVendorTagInfoList->vendorTagCount,
                                                    (MaxStatsProperties - pStatsDependency->propertyCount));
        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "Vendor tag count is %d ", propertyCount);

        // Adding vendor tag dependencies
        for (UINT32 i = 0; i < propertyCount; i++)
        {
            pStatsDependency->properties[pStatsDependency->propertyCount + i].property =
                pVendorTagInfoList->vendorTag[i].vendorTagId;
            pStatsDependency->properties[pStatsDependency->propertyCount + i].offset =
                pVendorTagInfoList->vendorTag[i].appliedDelay;

            CAMX_LOG_VERBOSE(CamxLogGroupAWB,
                                "Added Vendor Tag(%u) to the dependent list with offset(%u). requestID(%llu)",
                                pVendorTagInfoList->vendorTag[i].vendorTagId,
                                pVendorTagInfoList->vendorTag[i].appliedDelay,
                                pStatsProcessRequestDataInfo->requestId);
        }

        pStatsDependency->propertyCount += propertyCount;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::GetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::GetDependencies(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    StatsDependency*                pStatsDependency)
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);

    CamxResult result                       = CamxResultSuccess;
    UINT64     requestIdOffsetFromLastFlush =
        m_pNode->GetRequestIdOffsetFromLastFlush(pStatsProcessRequestDataInfo->requestId);

    static const UINT StatsData[] =
    {
        InputControlCaptureIntent,
    };

    static const UINT StatsLength          = CAMX_ARRAY_SIZE(StatsData);
    VOID*             pData[StatsLength]   = { 0 };
    UINT64            offsets[StatsLength] = { 0 };

    m_pNode->GetDataList(StatsData, pData, offsets, StatsLength);
    ControlCaptureIntentValues* pCaptureIntent = NULL;

    if (NULL != pData[0])
    {
        pCaptureIntent = reinterpret_cast<ControlCaptureIntentValues*>(pData[0]);
    }

    // Ensure ordering
    if (FirstValidRequestId < requestIdOffsetFromLastFlush)
    {
        // Add previous frame(offset = 1) AWBS Frame/Stats Control properties as dependency.
        // for normal frame and when skipProcessing flag is set.
        pStatsDependency->properties[pStatsDependency->propertyCount++] = { PropertyIDAWBFrameControl, 1 };
        pStatsDependency->properties[pStatsDependency->propertyCount++] = { PropertyIDAWBStatsControl, 1 };
    }

    // For Snapshot case AWB need to put dependecy on AECFrameControl
    if ((NULL != pCaptureIntent)  && (ControlCaptureIntentStillCapture == *pCaptureIntent))
    {
        pStatsDependency->properties[pStatsDependency->propertyCount++] = { PropertyIDAECFrameControl, 0 };
    }
    else
    {
        // Add only OEM vendor tag dependencies
        result = AddOEMDependencies(pStatsProcessRequestDataInfo, pStatsDependency);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::IsDependenciesSatisfied
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::IsDependenciesSatisfied(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    BOOL*                           pIsSatisfied)
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);

    CamxResult      result = CamxResultSuccess;
    BOOL            isStatisfied = FALSE;
    StatsDependency statsDependency = { 0 };

    /// @todo  (CAMX-523): query algorithm to check if dependencies are satisfied.

    // Get stats property dependency list
    result = GetDependencies(pStatsProcessRequestDataInfo, &statsDependency);

    isStatisfied = (statsDependency.propertyCount == 0) ? TRUE : FALSE;

    *pIsSatisfied &= isStatisfied;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::IsValidAlgoOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::IsValidAlgoOutput(
    AWBAlgoOutputList* pAlgoOutputList,
    UINT64             requestId)
{
    CamxResult    result = CamxResultSuccess;
    AWBAlgoGains gains = { 0 };

    // Check if we received all the outputs.
    // Note that currently algo can skip outputting BG config data. Subnode needs fill the BG config data in such case.
    for (UINT32 i = 0; i < pAlgoOutputList->outputCount; i++)
    {
        if (pAlgoOutputList->pAWBOutputs[i].outputType == AWBOutputTypeGains)
        {
            gains = *reinterpret_cast<AWBAlgoGains*>(pAlgoOutputList->pAWBOutputs[i].pAWBOutput);

            CAMX_LOG_VERBOSE(CamxLogGroupAWB, "gains from algo for ReqId :%llu Red  = %f, green  = %f, blue  = %f",
                requestId, gains.red, gains.green, gains.blue);

            if ((0.0f == gains.red) || (0.0f == gains.green) || (0.0f == gains.blue))
            {
                result = CamxResultEFailed;
            }
            break;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::SetDefaultSensorResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::SetDefaultSensorResolution()
{
    CAMX_ENTRYEXIT(CamxLogGroupAWB);

    if (NULL == m_pAWBIOUtil || NULL == m_pAWBAlgorithm)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "m_pAWBIOUtil: %p or m_pAWBAlgorithm:%p is Null",
            m_pAWBIOUtil, m_pAWBAlgorithm);
        return CamxResultEInvalidPointer;
    }

    CamxResult       result      = CamxResultSuccess;
    StatsSensorInfo* pSensorInfo = NULL;

    result = m_pAWBIOUtil->GetDefaultSensorResolution(&pSensorInfo);
    if (CamxResultSuccess == result && NULL != pSensorInfo)
    {
        AWBAlgoSetParam      setParam[AWBSetParamTypeLastIndex] = { { 0 } };
        AWBAlgoSetParamList  setParamList;

        setParamList.inputParamsCount                            = 0;
        setParamList.pAWBSetParams                               = setParam;
        setParam[setParamList.inputParamsCount].pAWBSetParam     = static_cast<VOID*>(pSensorInfo);
        setParam[setParamList.inputParamsCount].setParamType     = AWBSetParamTypeSensorInfo;
        setParam[setParamList.inputParamsCount].sizeOfInputParam = sizeof(StatsSensorInfo);

        setParamList.inputParamsCount                            = setParamList.inputParamsCount + 1;
        setParam[setParamList.inputParamsCount].pAWBSetParam     = static_cast<VOID*>(&m_cameraInfo);
        setParam[setParamList.inputParamsCount].setParamType     = AWBSetParamTypeCameraInfo;
        setParam[setParamList.inputParamsCount].sizeOfInputParam = sizeof(StatsCameraInfo);
        setParamList.inputParamsCount                            = setParamList.inputParamsCount + 1;

        // Assuming the params are copied, so the pointer to local
        CDKResult cdkResult = m_pAWBAlgorithm->AWBSetParam(m_pAWBAlgorithm, &setParamList);
        result = StatsUtil::CdkResultToCamXResult(cdkResult);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "Failed to get the default sensor resolution");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::PublishPreRequestOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::PublishPreRequestOutput()
{
    CamxResult            result          = CamxResultSuccess;

    if (TRUE == m_pStaticSettings->disableAWBStatsProcessing)
    {
        AWBAlgoOutputList algoOutputList = { 0 };

        OverwriteAWBOutput(NULL, &algoOutputList);
    }
    else
    {
        result = SetDefaultSensorResolution();
        AWBAlgoGetParam getParam = {};

        if (CamxResultSuccess == result)
        {
            getParam.type = AWBGetParamTypeLastOutput;
            result = AlgoGetParam(NULL, &getParam);
        }

        // query for BG Configuration
        if (CamxResultSuccess == result)
        {
            getParam.type = AWBGetParamTypeBGConfig;
            result = AlgoGetParam(NULL, &getParam);
        }
    }

    m_pAWBIOUtil->PrePublishMetadata();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::AlgoGetParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::AlgoGetParam(
    const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
    AWBAlgoGetParam*                pGetParam)
{
    CamxResult result = CamxResultSuccess;

    m_pAWBIOUtil->GetAlgoGetParamInputOutput(pStatsProcessRequestDataInfo, pGetParam);

    // Get initial data

    result = m_pAWBAlgorithm->AWBGetParam(m_pAWBAlgorithm, pGetParam);


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::SetOverrideAlgoSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::SetOverrideAlgoSetting()
{
    CamxResult          result = CamxResultSuccess;
    AWBAlgoSetParamList algoParamList = { 0 };
    AWBAlgoSetParam     algoSetParam[1] = {};
    AWBAlgoOverrideSettingParam overrideSetting;
    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    if (NULL != pStaticSettings)
    {
        // AEC Profiling value
        overrideSetting.profile3A                 = pStaticSettings->profile3A;
        overrideSetting.enable3ADebugData         = pStaticSettings->enable3ADebugData;
        overrideSetting.enableConcise3ADebugData  = pStaticSettings->enableConcise3ADebugData;
        algoParamList.inputParamsCount = 0;
        algoParamList.pAWBSetParams    = algoSetParam;
        m_pAWBIOUtil->LoadSetInputParamList(&algoParamList,
                                            AWBSetParamTypeOverrideSettings,
                                            static_cast<const VOID*>(&overrideSetting),
                                            sizeof(AWBAlgoOverrideSettingParam));

        result = m_pAWBAlgorithm->AWBSetParam(m_pAWBAlgorithm, &algoParamList);
        if (CamxResultEUnsupported == result)
        {
            result = CamxResultSuccess;
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "m_pStaticSettings is NULL");
        result = CamxResultEFailed;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::SetPipelineDelaytoAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::SetPipelineDelaytoAlgo(
    UINT pipelineDelay)
{
    CamxResult          result = CamxResultSuccess;
    AWBAlgoSetParamList algoParamList = { 0 };
    AWBAlgoSetParam     algoSetParam[1] = {};
   // UINT pipelineDelay = pipelineDelay;

    algoParamList.inputParamsCount = 0;
    algoParamList.pAWBSetParams    = algoSetParam;
    m_pAWBIOUtil->LoadSetInputParamList(&algoParamList,
                                        AWBSetParamTypePipelineDelay,
                                        static_cast<const VOID*>(&pipelineDelay),
                                        sizeof(UINT));

    result = m_pAWBAlgorithm->AWBSetParam(m_pAWBAlgorithm, &algoParamList);
    if (CamxResultEUnsupported == result)
    {
        result = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::SetOperationModetoAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::SetOperationModetoAlgo(
    StatsOperationMode opMode)
{
    CamxResult          result                                   = CamxResultSuccess;
    const UINT32        operationModeSetParamCount               = 3;
    AWBAlgoSetParamList algoParamList                            = { 0 };
    AWBAlgoSetParam     algoSetParam[operationModeSetParamCount] = {};

    if (StatsOperationModeInvalid == opMode)
    {
        CAMX_LOG_ERROR(CamxLogGroupAWB, "Invalid opMode Set: %d", opMode);
        result = CamxResultEInvalidArg;
    }

    AWBAlgoOperationModeType opModeAWB;

    switch (opMode)
    {
        case StatsOperationModeFastConvergence:
            opModeAWB = AWBAlgoOperationModeFastConvergence;
            break;
        case StatsOperationModeNormal:
            opModeAWB = AWBAlgoOperationModeStreaming;
            break;
        default:
            break;
    }

    if (result == CamxResultSuccess)
    {
        algoSetParam[0].setParamType     = AWBSetParamTypeOperationMode;
        algoSetParam[0].sizeOfInputParam = sizeof(AWBAlgoOperationModeType);
        algoSetParam[0].pAWBSetParam     = static_cast<VOID*>(&opModeAWB);
        // Besides AWB operation mode, we still need to pass camera information as well
        algoSetParam[1].setParamType     = AWBSetParamTypeCameraInfo;
        algoSetParam[1].sizeOfInputParam = sizeof(StatsCameraInfo);
        algoSetParam[1].pAWBSetParam     = static_cast<VOID*>(&m_cameraInfo);
        // Passing fixed focus info as well to Algo.
        algoSetParam[2].setParamType = AWBSetParamTypeIsFixedFocus;
        algoSetParam[2].sizeOfInputParam = sizeof(BOOL);
        algoSetParam[2].pAWBSetParam = static_cast<VOID*>(&m_isFixedFocus);

        algoParamList.inputParamsCount   = operationModeSetParamCount;
        algoParamList.pAWBSetParams      = algoSetParam;
        CAMX_LOG_VERBOSE(CamxLogGroupAWB, "Mode set to Algo: %d", opModeAWB);
        result = m_pAWBAlgorithm->AWBSetParam(m_pAWBAlgorithm, &algoParamList);
    }

    if (CamxResultEUnsupported == result)
    {
        result = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::OverwriteAWBOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAWBStatsProcessor::OverwriteAWBOutput(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    AWBAlgoOutputList*             pOutputList)
{
    AWBAlgoGains*           pAWBGain        = NULL;

    m_pAWBIOUtil->GetAlgoExpectedOutputList(pStatsProcessRequestDataInfo, pOutputList);
    for (UINT32 i = 0; i < pOutputList->outputCount; i++)
    {
        switch (pOutputList->pAWBOutputs[i].outputType)
        {
            case AWBOutputTypeGains:
                pAWBGain           = static_cast<AWBAlgoGains*>(pOutputList->pAWBOutputs[i].pAWBOutput);
                pAWBGain->red      = m_pStaticSettings->rGain;
                pAWBGain->green    = m_pStaticSettings->gGain;
                pAWBGain->blue     = m_pStaticSettings->BGain;
                break;
            case AWBOutputTypeColorTemperature:
                *static_cast<UINT32*>(pOutputList->pAWBOutputs[i].pAWBOutput) = m_pStaticSettings->colorTemp;
                break;
            case AWBOutputTypeIlluminantType:
                *static_cast<StatsIlluminantType*>(pOutputList->pAWBOutputs[i].pAWBOutput) = StatsIlluminantTL84;
                break;
            case AWBOutputTypeSampleDecision:
                break;
            case AWBOutputTypeBGConfig:
                m_pAWBIOUtil->GetDefaultBGConfig(
                    static_cast<StatsBayerGridBayerExposureConfig*>(pOutputList->pAWBOutputs[i].pAWBOutput));
                break;
            case AWBOutputTypeState:
                *static_cast<AWBAlgoState*>(pOutputList->pAWBOutputs[i].pAWBOutput) = AWBAlgoStateConverged;
                break;
            case AWBOutputTypeMode:
                *static_cast<AWBAlgoMode*>(pOutputList->pAWBOutputs[i].pAWBOutput) = AWBAlgoModeAuto;
                break;
            case AWBOutputTypeLock:
                *static_cast<BOOL*>(pOutputList->pAWBOutputs[i].pAWBOutput) = FALSE;
                break;
            case AWBOutputTypeVendorTag:
                break;
            default:
                break;
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAWBStatsProcessor::~CAWBStatsProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAWBStatsProcessor::~CAWBStatsProcessor()
{
    // Destroy all the created objects.
    CAMX_LOG_INFO(CamxLogGroupAWB, "CAWBStatsProcessor::Destroy");

    StatsCameraInfo cameraInfo = m_cameraInfo;

    if (NULL != m_pAWBIOUtil)
    {
        CAMX_DELETE m_pAWBIOUtil;
        m_pAWBIOUtil = NULL;
    }

    if (NULL != m_pAWBAlgorithm)
    {
        AWBAlgoDestroyParamList destroyParamList                        = { 0 };
        AWBAlgoDestroyParam     destroyParams[AWBDestroyParamTypeCount] = {};
        UINT                    overrideCameraClose                     = m_pStaticSettings->overrideCameraClose;

        destroyParams[AWBDestroyParamTypeCameraCloseIndicator].destroyParamType = AWBDestroyParamTypeCameraCloseIndicator;
        destroyParams[AWBDestroyParamTypeCameraCloseIndicator].pParam           = &overrideCameraClose;
        destroyParams[AWBDestroyParamTypeCameraCloseIndicator].sizeOfParam      = sizeof(UINT);

        destroyParams[AWBDestroyParamTypeCameraInfo].destroyParamType = AWBDestroyParamTypeCameraInfo;
        destroyParams[AWBDestroyParamTypeCameraInfo].pParam = static_cast<VOID*>(&cameraInfo);
        destroyParams[AWBDestroyParamTypeCameraInfo].sizeOfParam = sizeof(cameraInfo);

        destroyParamList.paramCount = AWBDestroyParamTypeCount;
        destroyParamList.pParamList = &destroyParams[0];

        m_pAWBAlgorithm->AWBDestroy(m_pAWBAlgorithm, &destroyParamList);
        m_pAWBAlgorithm = NULL;
    }

    if (NULL != m_hHandle)
    {
        OsUtils::LibUnmap(m_hHandle);
        m_hHandle = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAWBStatsProcessor::GetPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAWBStatsProcessor::GetPublishList(
    const UINT32    maxTagArraySize,
    UINT32*         pTagArray,
    UINT32*         pTagCount,
    UINT32*         pPartialTagCount)
{
    CamxResult result             = CamxResultSuccess;
    UINT32     tagCount           = 0;
    UINT32     numMetadataTags    = CAMX_ARRAY_SIZE(AWBOutputMetadataTags);
    UINT32     numVendorTags      = CAMX_ARRAY_SIZE(AWBOutputVendorTags);
    UINT32     algoVendortagCount = 0;
    UINT32     tagID;

    if (NULL != m_pOutputVendorTagInfoList)
    {
        algoVendortagCount = m_pOutputVendorTagInfoList->vendorTagCount;
    }

    if (numMetadataTags + numVendorTags + algoVendortagCount <= maxTagArraySize)
    {
        for (UINT32 tagIndex = 0; tagIndex < numMetadataTags; ++tagIndex)
        {
            pTagArray[tagCount++] = AWBOutputMetadataTags[tagIndex];
        }

        for (UINT32 tagIndex = 0; tagIndex < numVendorTags; ++tagIndex)
        {
            result = VendorTagManager::QueryVendorTagLocation(
                AWBOutputVendorTags[tagIndex].pSectionName,
                AWBOutputVendorTags[tagIndex].pTagName,
                &tagID);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot query vendor tag %s %s",
                    AWBOutputVendorTags[tagIndex].pSectionName,
                    AWBOutputVendorTags[tagIndex].pTagName);
                break;
            }
            pTagArray[tagCount++] = tagID;
        }

        for (UINT32 tagIndex = 0; tagIndex < algoVendortagCount; ++tagIndex)
        {
            pTagArray[tagCount++] = m_pOutputVendorTagInfoList->vendorTag[tagIndex].vendorTagId;
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
