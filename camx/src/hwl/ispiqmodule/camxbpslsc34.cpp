////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpslsc34.cpp
/// @brief CAMXBPSLSC34 class implementation
///        Corrects image intensity rolloff from center to corners due to lens optics by applying inverse gain to
///        compensate the lens rolloff effect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpslsc34.h"
#include "camxbpslsc34titan17x.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult          result         = CamxResultSuccess;
    CREATETINTLESS      pTintlessFunc  = NULL;
    LSC34TintlessRatio* pTintlessRatio = NULL;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSLSC34* pModule = CAMX_NEW BPSLSC34(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess == result)
            {
                // Load tintless library
                pTintlessFunc = reinterpret_cast<CREATETINTLESS>
                    (OsUtils::LoadPreBuiltLibrary(pTintlessLibName,
                        pTintlessFuncName,
                        &pModule->m_hHandle));

                if (NULL != pTintlessFunc)
                {
                    (*pTintlessFunc)(&pModule->m_pTintlessAlgo);
                }
                else
                {
                    result = CamxResultEInvalidPointer;
                    CAMX_LOG_ERROR(CamxLogGroupBPS, "Error loading tintless library");
                }
                // Allocate a Float array of 17 x 13 to store tintless ratio.
                // This will store the tintless ratio applied.
                // This will be used if tintless algo doesnt run
                pTintlessRatio = &(pModule->m_tintlessRatio);

                pTintlessRatio->pRGain  = static_cast<FLOAT*>(
                                               CAMX_CALLOC(sizeof(FLOAT) * BPSRolloffMeshPtHV34 * BPSRolloffMeshPtVV34));
                if (NULL == pTintlessRatio->pRGain)
                {
                    result = CamxResultENoMemory;
                }

                if (CamxResultSuccess == result)
                {
                    pTintlessRatio->pGrGain = static_cast<FLOAT*>(
                                                   CAMX_CALLOC(sizeof(FLOAT) * BPSRolloffMeshPtHV34 * BPSRolloffMeshPtVV34));
                }

                if (NULL == pTintlessRatio->pGrGain)
                {
                    result = CamxResultENoMemory;
                }

                if (CamxResultSuccess == result)
                {
                    pTintlessRatio->pGbGain = static_cast<FLOAT*>(
                                                   CAMX_CALLOC(sizeof(FLOAT) * BPSRolloffMeshPtHV34 * BPSRolloffMeshPtVV34));
                }

                if (NULL == pTintlessRatio->pGbGain)
                {
                    result = CamxResultENoMemory;
                }

                if (CamxResultSuccess == result)
                {
                    pTintlessRatio->pBGain  = static_cast<FLOAT*>(
                                                   CAMX_CALLOC(sizeof(FLOAT) * BPSRolloffMeshPtHV34 * BPSRolloffMeshPtVV34));
                }

                if (NULL == pTintlessRatio->pBGain)
                {
                    result = CamxResultENoMemory;
                }

                if (result != CamxResultSuccess)
                {
                    CAMX_LOG_ERROR(CamxLogGroupPProc, "Unable to initialize common library data, no memory");
                    CAMX_DELETE pModule;
                    pModule = NULL;
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Module initialization failed !!");
            }

            if (CamxResultSuccess != result)
            {
                pModule->Destroy();
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Memory allocation failed");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34::Initialize(
    BPSModuleCreateData* pCreateData)
{
    ResourceParams cmdBufferConfig = { 0 };
    CamxResult     result          = CamxResultSuccess;
    ISPInputData*  pInputData      = &pCreateData->initializationData;

    // 32 bit LUT buffer manager is created externally by BPS node
    m_pLUTCmdBufferManager = NULL;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Titan480 uses lsc40");
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSLSC34Titan17x::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid hardware Version: %d", pCreateData->titanVersion);
            break;
    }

    if (CamxResultSuccess == result)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength = m_pHWSetting->GetCommandLength();
        result = AllocateCommonLibraryData();
        if (result != CamxResultSuccess)
        {
            CAMX_DELETE m_pHWSetting;
            m_pHWSetting = NULL;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to create HW Setting Class, no memory");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::FillCmdBufferManagerParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34::FillCmdBufferManagerParams(
    const ISPInputData*     pInputData,
    IQModuleCmdBufferParam* pParam)
{
    CamxResult result                  = CamxResultSuccess;
    ResourceParams* pResourceParams    = NULL;
    CHAR*           pBufferManagerName = NULL;

    if ((NULL != pParam) && (NULL != pParam->pCmdBufManagerParam) && (NULL != pInputData))
    {
        // The Resource Params and Buffer Manager Name will be freed by caller Node
        pResourceParams = static_cast<ResourceParams*>(CAMX_CALLOC(sizeof(ResourceParams)));
        if (NULL != pResourceParams)
        {
            pBufferManagerName = static_cast<CHAR*>(CAMX_CALLOC((sizeof(CHAR) * MaxStringLength256)));
            if (NULL != pBufferManagerName)
            {
                OsUtils::SNPrintF(pBufferManagerName, (sizeof(CHAR) * MaxStringLength256), "CBM_%s_%s",
                                  (NULL != m_pNodeIdentifier) ? m_pNodeIdentifier : " ", "BPSLSC34");
                pResourceParams->resourceSize                 = (BPSLSC34DMILengthDword * sizeof(UINT32));
                pResourceParams->poolSize                     = (pInputData->requestQueueDepth * pResourceParams->resourceSize);
                pResourceParams->usageFlags.cmdBuffer         = 1;
                pResourceParams->cmdParams.type               = CmdType::CDMDMI;
                pResourceParams->alignment                    = CamxCommandBufferAlignmentInBytes;
                pResourceParams->cmdParams.enableAddrPatching = 0;
                pResourceParams->cmdParams.maxNumNestedAddrs  = 0;
                pResourceParams->memFlags                     = CSLMemFlagUMDAccess;
                pResourceParams->pDeviceIndices               = pInputData->pipelineBPSData.pDeviceIndex;
                pResourceParams->numDevices                   = 1;

                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].pBufferManagerName = pBufferManagerName;
                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].pParams            = pResourceParams;
                pParam->pCmdBufManagerParam[pParam->numberOfCmdBufManagers].ppCmdBufferManager = &m_pLUTCmdBufferManager;
                pParam->numberOfCmdBufManagers++;

            }
            else
            {
                result = CamxResultENoMemory;
                CAMX_FREE(pResourceParams);
                pResourceParams = NULL;
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Out Of Memory");
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Out Of Memory");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Input Error %p", pParam);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(lsc_3_4_0::lsc34_rgn_dataType) * (LSC34MaxmiumNonLeafNode + 1));

    if (NULL == m_dependenceData.pInterpolationData)
    {
        // Alloc for lsc_3_4_0::lsc34_rgn_dataType
        m_dependenceData.pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_dependenceData.pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::TranslateCalibrationTableToCommonLibrary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSLSC34::TranslateCalibrationTableToCommonLibrary(
    const ISPInputData* pInputData)
{
    INT32 index                = -1;

    CAMX_ASSERT(NULL != pInputData);

    if (NULL != pInputData->pOTPData)
    {
        if (NULL == m_pLSCCalibrationData)
        {
            // This allocation will happen once per session
            m_pLSCCalibrationData =
                static_cast<LSCCalibrationData*>(CAMX_CALLOC(MaxLightTypes *
                                                              sizeof(LSCCalibrationData)));
        }

        if (NULL == m_dependenceData.pCalibrationTable)
        {
            // This allocation will happen once per session
            m_dependenceData.numTables          = MaxLightTypes;
            m_dependenceData.pCalibrationTable =
                static_cast<LSC34CalibrationDataTable*>(CAMX_CALLOC(MaxLightTypes *
                                                                    sizeof(LSC34CalibrationDataTable)));
        }

        if ((NULL != m_dependenceData.pCalibrationTable) && (NULL != m_pLSCCalibrationData))
        {
            for (UINT32 lightIndex = 0; lightIndex < MaxLightTypes; lightIndex++)
            {
                if (TRUE == pInputData->pOTPData->LSCCalibration[lightIndex].isAvailable)
                {
                    index++;
                    // This memcpy will happen one first request and subsequent chromatix change
                    Utils::Memcpy(&(m_pLSCCalibrationData[index]),
                        &(pInputData->pOTPData->LSCCalibration[lightIndex]),
                        sizeof(LSCCalibrationData));

                    m_dependenceData.pCalibrationTable[index].pBGain =
                        m_pLSCCalibrationData[index].bGain;
                    m_dependenceData.pCalibrationTable[index].pRGain =
                        m_pLSCCalibrationData[index].rGain;
                    m_dependenceData.pCalibrationTable[index].pGBGain =
                        m_pLSCCalibrationData[index].gbGain;
                    m_dependenceData.pCalibrationTable[index].pGRGain =
                        m_pLSCCalibrationData[index].grGain;
                    m_dependenceData.toCalibrate = TRUE;
                    m_dependenceData.enableCalibration = TRUE;
                }
            }

            m_dependenceData.numberOfEEPROMTable = index + 1;

            if (-1 == index)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid index for EEPROM data");
                m_dependenceData.toCalibrate = FALSE;
                m_dependenceData.enableCalibration = FALSE;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Calibration data from sensor is not present");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSLSC34::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL    isChanged = FALSE;
    UINT16  scale     = 1;

    if ((NULL != pInputData)             &&
        (NULL != pInputData->pHwContext) &&
        (NULL != pInputData->pHALTagsData))
    {
        ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;

        if (NULL != pInputData->pOEMIQSetting)
        {
            m_moduleEnable = (static_cast<OEMBPSIQSetting*>(pInputData->pOEMIQSetting))->LSCEnable;
            isChanged      = (TRUE == m_moduleEnable);

            m_shadingMode        = ShadingModeFast;
            m_lensShadingMapMode = StatisticsLensShadingMapModeOff;
        }
        else
        {
            m_shadingMode        = pHALTagsData->shadingMode;
            m_lensShadingMapMode = pHALTagsData->statisticsLensShadingMapMode;
            tintless_2_0_0::chromatix_tintless20Type* pTintlessChromatix = NULL;

            TuningDataManager*                        pTuningManager    = pInputData->pTuningDataManager;
            CAMX_ASSERT(NULL != pTuningManager);

            if (TRUE == pTuningManager->IsValidChromatix())
            {
                CAMX_ASSERT(NULL != pInputData->pTuningData);

                // Search through the tuning data (tree), only when there
                // are changes to the tuning mode data as an optimization
                if (TRUE == pInputData->tuningModeChanged)
                {
                    m_pChromatix   = pTuningManager->GetChromatix()->GetModule_lsc34_bps(
                                          reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                          pInputData->pTuningData->noOfSelectionParameter);
                }
                pTintlessChromatix = pTuningManager->GetChromatix()->GetModule_tintless20_sw(
                                          reinterpret_cast<TuningMode*>(&pInputData->pTuningData->TuningMode[0]),
                                          pInputData->pTuningData->noOfSelectionParameter);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invaild chromatix");
            }

            CAMX_ASSERT(NULL != pTintlessChromatix);
            if (NULL != pTintlessChromatix)
            {
                m_dependenceData.pTintlessChromatix = pTintlessChromatix;
                m_dependenceData.enableTintless     = pTintlessChromatix->enable_section.tintless_en;
                if ( TRUE != pInputData->registerBETEn )
                {
                    m_dependenceData.enableTintless &=
                        (TRUE == pInputData->pHwContext->GetStaticSettings()->tintlessEnable);

                    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "BPS LSC34 Tintless %s",
                        (TRUE == m_dependenceData.enableTintless) ? "enabled" : "disabled");
                }
            }

            CAMX_ASSERT(NULL != m_pChromatix);
            if (NULL != m_pChromatix)
            {
                if (m_pChromatix != m_dependenceData.pChromatix)
                {
                    m_dependenceData.pChromatix = m_pChromatix;
                    m_moduleEnable              = m_pChromatix->enable_section.rolloff_enable;

                    isChanged                   = (TRUE == m_moduleEnable);

                    TranslateCalibrationTableToCommonLibrary(pInputData);
                }
                if ((TRUE == m_moduleEnable)  &&
                    (ShadingModeOff == pHALTagsData->shadingMode))
                {
                    m_moduleEnable = FALSE;
                    isChanged      = FALSE;
                }
                else if (FALSE == m_moduleEnable)
                {
                    m_shadingMode = ShadingModeOff;
                }

                /// @todo (CAMX-1164) Dual IFE changes for LSC to set the right sensor width/height.
                if (TRUE == m_moduleEnable)
                {
                    m_dependenceData.fullResWidth  = pInputData->sensorData.fullResolutionWidth;
                    m_dependenceData.fullResHeight = pInputData->sensorData.fullResolutionHeight;
                    scale                          = static_cast<UINT32>(pInputData->sensorData.sensorScalingFactor *
                                                                         pInputData->sensorData.sensorBinningFactor);
                    m_dependenceData.offsetX       = static_cast<UINT32>(pInputData->sensorData.sensorOut.offsetX     +
                                                                        (pInputData->sensorData.CAMIFCrop.firstPixel  *
                                                                         scale));
                    m_dependenceData.offsetY       = static_cast<UINT32>(pInputData->sensorData.sensorOut.offsetY     +
                                                                        (pInputData->sensorData.CAMIFCrop.firstLine   *
                                                                         scale));
                    m_dependenceData.imageWidth    = (pInputData->sensorData.CAMIFCrop.lastPixel  -
                                                      pInputData->sensorData.CAMIFCrop.firstPixel + 1);
                    m_dependenceData.imageHeight   = (pInputData->sensorData.CAMIFCrop.lastLine   -
                                                      pInputData->sensorData.CAMIFCrop.firstLine  + 1);
                    m_dependenceData.scalingFactor = scale;

                    if (TRUE == IQInterface::s_interpolationTable.LSC34TriggerUpdate(&(pInputData->triggerData),
                                                                                     &m_dependenceData))
                    {
                        isChanged = TRUE;
                    }
                }
                CAMX_LOG_VERBOSE(CamxLogGroupBPS, "colorCorrectionMode = %d, controlAWBMode = %d, controlMode = %d",
                                 pHALTagsData->colorCorrectionMode,
                                 pHALTagsData->controlAWBMode,
                                 pHALTagsData->controlMode);

            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to get Chromatix");
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input: pInputData %p", pInputData);
    }

    if (TRUE == isChanged)
    {
        m_tintlessConfig.statsConfig.camifWidth =
            pInputData->sensorData.CAMIFCrop.lastPixel - pInputData->sensorData.CAMIFCrop.firstPixel + 1;
        m_tintlessConfig.statsConfig.camifHeight =
            pInputData->sensorData.CAMIFCrop.lastLine - pInputData->sensorData.CAMIFCrop.firstLine + 1;
        m_tintlessConfig.statsConfig.postBayer = 0;
        m_tintlessConfig.statsConfig.statsBitDepth =
            pInputData->pStripeConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.outputBitDepth;
        m_tintlessConfig.statsConfig.statsRegionHeight =
            pInputData->pStripeConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.regionHeight;
        m_tintlessConfig.statsConfig.statsRegionWidth =
            pInputData->pStripeConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.regionWidth;
        m_tintlessConfig.statsConfig.statsNumberOfHorizontalRegions =
            pInputData->pStripeConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.horizontalNum;
        m_tintlessConfig.statsConfig.statsNumberOfVerticalRegions =
            pInputData->pStripeConfig->statsDataForISP.tintlessBGConfig.tintlessBGConfig.verticalNum;

        m_tintlessConfig.tintlessParamConfig.applyTemporalFiltering = 0;

        m_pTintlessBGStats = pInputData->pStripeConfig->statsDataForISP.pParsedTintlessBGStats;
    }

    isChanged = TRUE;

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::FetchDMIBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34::FetchDMIBuffer(
    const ISPInputData* pInputData)
{
    CamxResult      result          = CamxResultSuccess;
    PacketResource* pPacketResource = NULL;

    CAMX_UNREFERENCED_PARAM(pInputData);

    if (NULL != m_pLUTCmdBufferManager)
    {
        // Recycle the last updated LUT DMI cmd buffer
        if (NULL != m_pLUTDMICmdBuffer)
        {
            m_pLUTCmdBufferManager->Recycle(m_pLUTDMICmdBuffer);
        }

        // fetch a fresh LUT DMI cmd buffer
        result = m_pLUTCmdBufferManager->GetBuffer(&pPacketResource);
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "LUT Command Buffer Manager is NULL");
    }

    if (CamxResultSuccess == result)
    {
        m_pLUTDMICmdBuffer = static_cast<CmdBuffer*>(pPacketResource);
    }
    else
    {
        m_pLUTDMICmdBuffer = NULL;

        result             = CamxResultEUnableToLoad;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to fetch DMI Buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34::RunCalculation(
    ISPInputData*    pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        result = FetchDMIBuffer(pInputData);

        if (CamxResultSuccess == result)
        {
            LSC34OutputData outputData;

            // Update the LUT DMI buffer with Red LUT data
            outputData.pGRRLUTDMIBuffer  =
                reinterpret_cast<UINT32*>(m_pLUTDMICmdBuffer->BeginCommands(BPSLSC34DMILengthDword));
            CAMX_ASSERT(NULL != outputData.pGRRLUTDMIBuffer);
            // BET ONLY - InputData is different per module tested
            pInputData->pBetDMIAddr = static_cast<VOID*>(outputData.pGRRLUTDMIBuffer);

            // Update the LUT DMI buffer with Blue LUT data
            outputData.pGBBLUTDMIBuffer  =
                reinterpret_cast<UINT32*>(reinterpret_cast<UCHAR*>(outputData.pGRRLUTDMIBuffer) + BPSLSC34LUTTableSize);
            CAMX_ASSERT(NULL != outputData.pGBBLUTDMIBuffer);

            outputData.type              = PipelineType::BPS;
            outputData.pUnpackedField    = &m_unpackedData.unpackedData;

            if (((pInputData->pHALTagsData->controlAWBLock == m_AWBLock)            &&
                 (pInputData->pHALTagsData->controlAWBLock == ControlAWBLockOn))    ||
                (TRUE == pInputData->sensorData.isIHDR))
            {
                m_dependenceData.enableTintless = FALSE;
            }
            else
            {
                m_AWBLock = pInputData->pHALTagsData->controlAWBLock;
            }

            if (TRUE == m_dependenceData.enableTintless && NULL == pInputData->pOEMIQSetting)
            {
                m_dependenceData.pTintlessAlgo   = m_pTintlessAlgo;
                m_dependenceData.pTintlessConfig = &m_tintlessConfig;
                m_dependenceData.pTintlessStats  = m_pTintlessBGStats;
            }
            else
            {
                m_dependenceData.pTintlessAlgo   = NULL;
                m_dependenceData.pTintlessConfig = NULL;
                m_dependenceData.pTintlessStats  = NULL;
            }

            m_dependenceData.pTintlessRatio = &m_tintlessRatio;

            result = IQInterface::LSC34CalculateSetting(&m_dependenceData,
                                                        pInputData->pOEMIQSetting,
                                                        pInputData,
                                                        &outputData);

            if (CamxResultSuccess == result)
            {
                m_pLUTDMICmdBuffer->CommitCommands();

                if (TRUE == m_dependenceData.toCalibrate)
                {
                    m_dependenceData.toCalibrate = FALSE;
                }

                if (NULL != pInputData->pBPSTuningMetadata)
                {
                    m_pGRRLUTDMIBuffer = outputData.pGRRLUTDMIBuffer;
                    m_pGBBLUTDMIBuffer = outputData.pGBBLUTDMIBuffer;
                }
                if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
                {
                    m_pHWSetting->DumpRegConfig();
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "LSC Calculation Failed %d", result);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSLSC34::UpdateBPSInternalData(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    result = m_pHWSetting->UpdateFirmwareData(pInputData, m_moduleEnable);

    if (CamxResultSuccess == result)
    {
        if ((ShadingModeOff == m_shadingMode) &&
            (StatisticsLensShadingMapModeOn == m_lensShadingMapMode))
        {
            // Pass the unity matrix for shading mode is OFF.
            for (UINT16 i = 0; i < (4 * BPSRolloffMeshPtVV34 * BPSRolloffMeshPtHV34); i++)
            {
                pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[i] = 1.0f;
            }
        }
        else if (StatisticsLensShadingMapModeOn == pInputData->pHALTagsData->statisticsLensShadingMapMode)
        {
            IQInterface::CopyLSCMapData(pInputData, &m_unpackedData.unpackedData);
        }

        // Post tuning metadata if setting is enabled
        if (NULL != pInputData->pBPSTuningMetadata)
        {
            result = m_pHWSetting->UpdateTuningMetadata(pInputData);
            if (CamxResultSuccess == result)
            {
                CAMX_STATIC_ASSERT(BPSLSC34LUTTableSize <=
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.LSCMesh.GRRLUT));
                CAMX_STATIC_ASSERT(BPSLSC34LUTTableSize <=
                                sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.LSCMesh.GBBLUT));

                if (NULL != m_pGRRLUTDMIBuffer)
                {
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.LSCMesh.GRRLUT[0],
                                m_pGRRLUTDMIBuffer,
                                BPSLSC34LUTTableSize);
                }
                if (NULL != m_pGBBLUTDMIBuffer)
                {
                    Utils::Memcpy(&pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.LSCMesh.GBBLUT[0],
                                m_pGBBLUTDMIBuffer,
                                BPSLSC34LUTTableSize);
                }

                if ((TRUE == m_moduleEnable) && (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
                {
                    result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                        DebugDataTagID::TuningBPSLSC34PackedMesh,
                        DebugDataTagType::TuningLSC34MeshLUT,
                        1,
                        &pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.LSCMesh,
                        sizeof(pInputData->pBPSTuningMetadata->BPSTuningMetadata17x.BPSDMIData.LSCMesh));

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
                        result = CamxResultSuccess; // Non-fatal error
                    }
                }
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "UpdateTuningMetadata failed.");
                result = CamxResultSuccess; // Non-fatal error
            }
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "UpdateFirmwareData failed.");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34::Execute(
    ISPInputData* pInputData)
{
    CamxResult result           = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        /// @todo (CAMX-1164) Dual IFE changes for LSC.
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation(pInputData);
        }

        pInputData->p64bitDMIBuffer = m_pLUTDMICmdBuffer;

        if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
        {
            result                       = m_pHWSetting->CreateCmdList(pInputData, NULL);
            m_dependenceData.bankSelect ^= 1;
        }

        if (CamxResultSuccess == result)
        {
            UpdateBPSInternalData(pInputData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Operation failed %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input: pInputData %p hwSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSLSC34::DeallocateCommonLibraryData()
{
    if (NULL != m_dependenceData.pInterpolationData)
    {
        CAMX_FREE(m_dependenceData.pInterpolationData);
        m_dependenceData.pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::BPSLSC34
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSLSC34::BPSLSC34(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier   = pNodeIdentifier;
    m_type              = ISPIQModuleType::BPSLSC;
    m_moduleEnable      = FALSE;
    // m_dependenceData.bankSelect = 0;
    m_numLUT            = MaxLSC34NumDMITables;
    m_cmdLength         = 0;
    m_pGRRLUTDMIBuffer  = NULL;
    m_pGBBLUTDMIBuffer  = NULL;
    m_pChromatix        = NULL;
    m_pHWSetting        = NULL;
    m_pTinlessChromatix = NULL;
    m_AWBLock           = ControlAWBLockOff;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34::~BPSLSC34
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSLSC34::~BPSLSC34()
{
    m_tintlessRatio.isValid = FALSE;

    CAMX_FREE(m_tintlessRatio.pRGain);
    m_tintlessRatio.pRGain = NULL;
    CAMX_FREE(m_tintlessRatio.pGrGain);
    m_tintlessRatio.pGrGain = NULL;
    CAMX_FREE(m_tintlessRatio.pGbGain);
    m_tintlessRatio.pGbGain = NULL;
    CAMX_FREE(m_tintlessRatio.pBGain);
    m_tintlessRatio.pBGain = NULL;

    if (NULL != m_pLUTCmdBufferManager)
    {
        if (NULL != m_pLUTDMICmdBuffer)
        {
            m_pLUTCmdBufferManager->Recycle(m_pLUTDMICmdBuffer);
            m_pLUTDMICmdBuffer = NULL;
        }

        m_pLUTCmdBufferManager->Uninitialize();
        CAMX_DELETE m_pLUTCmdBufferManager;
        m_pLUTCmdBufferManager = NULL;
    }

    if (NULL != m_dependenceData.pCalibrationTable)
    {
        CAMX_FREE(m_dependenceData.pCalibrationTable);
        m_dependenceData.pCalibrationTable = NULL;
    }

    if (NULL != m_pLSCCalibrationData)
    {
        CAMX_FREE(m_pLSCCalibrationData);
        m_pLSCCalibrationData = NULL;
    }

    if (NULL != m_pTintlessAlgo)
    {
        m_pTintlessAlgo->TintlessDestroy(m_pTintlessAlgo);
        m_pTintlessAlgo = NULL;
    }

    if (NULL != m_hHandle)
    {
        CamX::OsUtils::LibUnmap(m_hHandle);
        m_hHandle = NULL;
    }

    m_pChromatix        = NULL;
    m_pTinlessChromatix = NULL;

    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
    DeallocateCommonLibraryData();
}

CAMX_NAMESPACE_END
