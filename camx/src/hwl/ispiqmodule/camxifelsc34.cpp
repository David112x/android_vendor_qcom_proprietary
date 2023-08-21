////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelsc34.cpp
/// @brief ifelsc34 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifelsc34titan17x.h"
#include "camxifelsc34.h"
#include "camxiqinterface.h"
#include "camxisphwsetting.h"
#include "camxnode.h"
#include "camxtuningdatamanager.h"
#include "parametertuningtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC34::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult          result         = CamxResultSuccess;
    CREATETINTLESS      pTintlessFunc  = NULL;
    UINT32              index          = 0;
    LSC34TintlessRatio* pTintlessRatio = NULL;

    if (NULL != pCreateData)
    {
        IFELSC34* pModule = CAMX_NEW IFELSC34;

        if (NULL != pModule)
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
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Error loading tintless library");
            }

            CamX::Utils::Memset(&pCreateData->initializationData.pStripeConfig->stateLSC,
                                0,
                                sizeof(pCreateData->initializationData.pStripeConfig->stateLSC));

            result = pModule->Initialize(pCreateData);

            // Allocate a Float array of 17 x 13 to store tintless ratio.
            // This will store the tintless ratio applied.
            // This will be used if tintless algo doesnt run
            pTintlessRatio = &(pModule->m_tintlessRatio);

            pTintlessRatio->pRGain  = static_cast<FLOAT*>(
                                           CAMX_CALLOC(sizeof(FLOAT) * IFERolloffMeshPtHV34 * IFERolloffMeshPtVV34));
            if (NULL == pTintlessRatio->pRGain)
            {
                result = CamxResultENoMemory;
            }

            if (CamxResultSuccess == result)
            {
                pTintlessRatio->pGrGain = static_cast<FLOAT*>(
                                               CAMX_CALLOC(sizeof(FLOAT) * IFERolloffMeshPtHV34 * IFERolloffMeshPtVV34));
            }

            if (NULL == pTintlessRatio->pGrGain)
            {
                result = CamxResultENoMemory;
            }

            if (CamxResultSuccess == result)
            {
                pTintlessRatio->pGbGain = static_cast<FLOAT*>(
                                               CAMX_CALLOC(sizeof(FLOAT) * IFERolloffMeshPtHV34 * IFERolloffMeshPtVV34));
            }

            if (NULL == pTintlessRatio->pGbGain)
            {
                result = CamxResultENoMemory;
            }

            if (CamxResultSuccess == result)
            {
                pTintlessRatio->pBGain  = static_cast<FLOAT*>(
                                               CAMX_CALLOC(sizeof(FLOAT) * IFERolloffMeshPtHV34 * IFERolloffMeshPtVV34));
            }

            if (NULL == pTintlessRatio->pBGain)
            {
                result = CamxResultENoMemory;
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to initialize common library data, no memory");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Memory allocation failed");
        }

        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Data for IFELSC34 Creation");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC34::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFELSC34Titan17x;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }
    // Initialize Tintless Static structures
    IQInterface::LSCMeshTableInit(
        pCreateData->initializationData.sensorID,
        pCreateData->initializationData.sensorData.sensorAspectRatioMode);

    if (NULL != m_pHWSetting)
    {
        m_cmdLength         = m_pHWSetting->GetCommandLength();
        m_32bitDMILength    = m_pHWSetting->Get32bitDMILength();
        m_pStripeInputState = CAMX_CALLOC(sizeof(LSCStripeState));
        result           = AllocateCommonLibraryData();

        if (NULL == m_pStripeInputState)
        {
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess != result)
        {
            CAMX_DELETE m_pHWSetting;

            if (NULL != m_pStripeInputState)
            {
                CAMX_FREE(m_pStripeInputState);
                m_pStripeInputState = NULL;
            }

            m_pHWSetting     = NULL;
            m_cmdLength      = 0;
            m_32bitDMILength = 0;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to initilize common library data, no memory");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, Titan 0x%x", pCreateData->titanVersion);
        result = CamxResultENoMemory;
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC34::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        m_pState = &pInputData->pStripeConfig->stateLSC;
        m_pState->dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);

        if (TRUE == CheckDependenceChange(pInputData))
        {

            result = RunCalculation(pInputData);
            if (CamxResultSuccess == result)
            {
                result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
                m_pState->dependenceData.bankSelect ^= 1;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "LSC module calculation Failed.");
            }
        }

        if (CamxResultSuccess == result)
        {
            UpdateIFEInternalData(pInputData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Operation failed %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input: pInputData %p m_pHWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELSC34::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    if (NULL != pInputData->pCalculatedData)
    {
        pInputData->pCalculatedData->moduleEnable.IQModules.rolloffEnable = m_moduleEnable;

        if ((ShadingModeOff == m_shadingMode || m_moduleEnable == FALSE) &&
            (StatisticsLensShadingMapModeOn == m_lensShadingMapMode))
        {
            // Pass the unity matrix for shading mode is OFF.
            for (UINT16 i = 0; i < (4 * IFERolloffMeshPtVV34 * IFERolloffMeshPtHV34); i++)
            {
                pInputData->pCalculatedData->lensShadingInfo.lensShadingMap[i] = 1.0f;
            }
        }
        else if (StatisticsLensShadingMapModeOn == pInputData->pHALTagsData->statisticsLensShadingMapMode)
        {
            IQInterface::CopyLSCMapData(pInputData, &m_unpackedData.unpackedData);
        }

        pInputData->pCalculatedData->lensShadingInfo.lensShadingMapSize.width  = IFERolloffMeshPtHV34;
        pInputData->pCalculatedData->lensShadingInfo.lensShadingMapSize.height = IFERolloffMeshPtVV34;
        pInputData->pCalculatedData->lensShadingInfo.lensShadingMapMode        = m_lensShadingMapMode;
        pInputData->pCalculatedData->lensShadingInfo.shadingMode               = m_shadingMode;

        // Post tuning metadata if setting is enabled
        if (NULL != pInputData->pIFETuningMetadata)
        {
            if (CamxResultSuccess != m_pHWSetting->UpdateTuningMetadata(pInputData->pIFETuningMetadata))
            {
                CAMX_LOG_WARN(CamxLogGroupISP, "Update Metadata failed");
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34::AllocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC34::AllocateCommonLibraryData()
{
    CamxResult result = CamxResultSuccess;

    UINT interpolationSize = (sizeof(lsc_3_4_0::lsc34_rgn_dataType) * (LSC34MaxmiumNonLeafNode + 1));

    if (NULL == m_pInterpolationData)
    {
        // Alloc for lsc_3_4_0::lsc34_rgn_dataType
        m_pInterpolationData = CAMX_CALLOC(interpolationSize);
        if (NULL == m_pInterpolationData)
        {
            result = CamxResultENoMemory;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34::TranslateCalibrationTableToCommonLibrary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELSC34::TranslateCalibrationTableToCommonLibrary(
    const ISPInputData* pInputData)
{
    INT32 index = -1;

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

        if (NULL == m_pState->dependenceData.pCalibrationTable)
        {
            // This allocation will happen once per session
            if (NULL == m_pCalibrationTable)
            {
                m_pCalibrationTable =
                    static_cast<LSC34CalibrationDataTable*>(CAMX_CALLOC(MaxLightTypes *
                        sizeof(LSC34CalibrationDataTable)));
            }
            CAMX_ASSERT(NULL != m_pCalibrationTable);
            m_pState->dependenceData.pCalibrationTable = m_pCalibrationTable;
            m_pState->dependenceData.numTables         = MaxLightTypes;
        }

        if ((NULL != m_pState->dependenceData.pCalibrationTable) && (NULL != m_pLSCCalibrationData))
        {
            for (UINT32 lightIndex = 0; lightIndex < MaxLightTypes; lightIndex++)
            {
                if (TRUE == pInputData->pOTPData->LSCCalibration[lightIndex].isAvailable)
                {
                    // This memcpy will happen one first request and subsequent chromatix change
                    index++;
                    Utils::Memcpy(&(m_pLSCCalibrationData[index]),
                        &(pInputData->pOTPData->LSCCalibration[lightIndex]),
                        sizeof(LSCCalibrationData));

                    m_pState->dependenceData.pCalibrationTable[index].pBGain =
                        m_pLSCCalibrationData[index].bGain;
                    m_pState->dependenceData.pCalibrationTable[index].pRGain =
                        m_pLSCCalibrationData[index].rGain;
                    m_pState->dependenceData.pCalibrationTable[index].pGBGain =
                        m_pLSCCalibrationData[index].gbGain;
                    m_pState->dependenceData.pCalibrationTable[index].pGRGain =
                        m_pLSCCalibrationData[index].grGain;
                    m_pState->dependenceData.toCalibrate = TRUE;
                    m_pState->dependenceData.enableCalibration = TRUE;
                }
            }

            m_pState->dependenceData.numberOfEEPROMTable = index + 1;

            if (-1 == index)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid index for EEPROM data");
                m_pState->dependenceData.toCalibrate = FALSE;
                m_pState->dependenceData.enableCalibration = FALSE;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Calibration data from sensor is not present");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFELSC34::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL   isChanged         = FALSE;
    /// @todo (CAMX-1813) These scale factor  comes from the Sensor
    UINT16 scale             = 1;
    UINT32 width;
    UINT32 height;
    UINT32 offsetX;
    UINT32 offsetY;
    BOOL   skipLSCProcessing = FALSE;
    BOOL   dynamicEnable     = Utils::IsBitSet(pInputData->IFEDynamicEnableMask, static_cast<UINT>(DynamicIFEMaskBit::IFELSC));

    if ((TRUE  == pInputData->pStripeConfig->overwriteStripes) &&
        (FALSE == pInputData->isPrepareStripeInputContext))
    {
        // Check if checkDependenceChange returned TRUE for PrepareStripingParameters
        // Check state of m_ISPFramelevelData->lscDependencyChanged which is currently contained in pInputData->pStripeConfig
        ISPInternalData* pFrameLevel = pInputData->pStripeConfig->pFrameLevelData->pFrameData;
        if (TRUE == pFrameLevel->lscDependencyChanged)
        {
            m_moduleEnable = TRUE;
            isChanged = TRUE;
        }
    }
    else
    {
        if ((NULL != pInputData->pHwContext)     &&
            (NULL != pInputData->pAECUpdateData) &&
            (NULL != pInputData->pAWBUpdateData) &&
            (NULL != pInputData->pHALTagsData))
        {
            ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;

            if (NULL != pInputData->pOEMIQSetting)
            {
                m_moduleEnable = (static_cast<OEMIFEIQSetting*>(pInputData->pOEMIQSetting))->LSCEnable;

                if (TRUE == m_moduleEnable)
                {
                    isChanged = TRUE;
                }

                m_shadingMode        = ShadingModeFast;
                m_lensShadingMapMode = StatisticsLensShadingMapModeOff;
            }
            else
            {
                m_shadingMode        = pHALTagsData->shadingMode;
                m_lensShadingMapMode = pHALTagsData->statisticsLensShadingMapMode;

                tintless_2_0_0::chromatix_tintless20Type* pTintlessChromatix = NULL;
                TuningDataManager*                        pTuningManager     = pInputData->pTuningDataManager;
                CAMX_ASSERT(NULL != pTuningManager);

                if ((NULL != pTuningManager) &&
                    (TRUE == pTuningManager->IsValidChromatix()) &&
                    (NULL != pInputData->pTuningData))
                {
                    CAMX_ASSERT(NULL != pInputData->pTuningData);

                    // Search through the tuning data (tree), only when there
                    // are changes to the tuning mode data as an optimization
                    if (TRUE == pInputData->tuningModeChanged)
                    {
                        m_pChromatix   = pTuningManager->GetChromatix()->GetModule_lsc34_ife(
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
                    m_pState->dependenceData.pTintlessChromatix = pTintlessChromatix;
                    m_pState->dependenceData.enableTintless     = pTintlessChromatix->enable_section.tintless_en;
                    if (TRUE != pInputData->registerBETEn)
                    {
                        m_pState->dependenceData.enableTintless &=
                            (TRUE == pInputData->pHwContext->GetStaticSettings()->tintlessEnable);
                        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "IFE LSC34 Tintless %s",
                            (TRUE == m_pState->dependenceData.enableTintless) ? "enabled" : "disabled");
                    }
                }

                CAMX_ASSERT(NULL != m_pChromatix);
                if (NULL != m_pChromatix)
                {
                    if ((NULL == m_pState->dependenceData.pChromatix)                                       ||
                        (m_pChromatix->SymbolTableID != m_pState->dependenceData.pChromatix->SymbolTableID) ||
                        (m_moduleEnable              != m_pChromatix->enable_section.rolloff_enable))
                    {
                        m_pState->dependenceData.pChromatix         = m_pChromatix;
                        m_pState->dependenceData.pInterpolationData = m_pInterpolationData;
                        m_moduleEnable                              = m_pChromatix->enable_section.rolloff_enable;
                        if (TRUE == m_moduleEnable)
                        {
                            isChanged = TRUE;
                        }

                        TranslateCalibrationTableToCommonLibrary(pInputData);
                    }
                }
            }

            if (TRUE == m_moduleEnable)
            {
                if (TRUE ==
                    IQInterface::s_interpolationTable.LSC34TriggerUpdate(&pInputData->triggerData, &m_pState->dependenceData))
                {
                    isChanged = TRUE;
                }
            }

            width   = (pInputData->sensorData.CAMIFCrop.lastPixel - pInputData->sensorData.CAMIFCrop.firstPixel + 1);
            height  = (pInputData->sensorData.CAMIFCrop.lastLine  - pInputData->sensorData.CAMIFCrop.firstLine + 1);
            offsetX = static_cast<UINT32>(pInputData->sensorData.sensorOut.offsetX     +
                                         (pInputData->sensorData.CAMIFCrop.firstPixel  *
                                          pInputData->sensorData.sensorScalingFactor   *
                                          pInputData->sensorData.sensorBinningFactor   *
                                          pInputData->sensorData.CSIDBinningFactor));
            offsetY = static_cast<UINT32>(pInputData->sensorData.sensorOut.offsetY     +
                                         (pInputData->sensorData.CAMIFCrop.firstLine   *
                                          pInputData->sensorData.sensorScalingFactor   *
                                          pInputData->sensorData.sensorBinningFactor   *
                                          pInputData->sensorData.CSIDBinningFactor));
            scale   = static_cast<UINT32>(pInputData->sensorData.sensorScalingFactor *
                                          pInputData->sensorData.sensorBinningFactor *
                                          pInputData->sensorData.CSIDBinningFactor);

            if (TRUE == pInputData->HVXData.DSEnabled)
            {
                width  = pInputData->HVXData.HVXOut.width;
                height = pInputData->HVXData.HVXOut.height;
            }

            if ((TRUE == m_moduleEnable)                             &&
                ((m_pState->dependenceData.imageWidth    != width)   ||
                 (m_pState->dependenceData.imageHeight   != height)  ||
                 (m_pState->dependenceData.scalingFactor != scale)   ||
                 (m_pState->dependenceData.offsetX       != offsetX) ||
                 (m_pState->dependenceData.offsetY       != offsetY) ||
                 (TRUE  == pInputData->forceTriggerUpdate)))
            {
                m_pState->dependenceData.fullResWidth  = pInputData->sensorData.fullResolutionWidth;
                m_pState->dependenceData.fullResHeight = pInputData->sensorData.fullResolutionHeight;
                m_pState->dependenceData.offsetX       = offsetX;
                m_pState->dependenceData.offsetY       = offsetY;
                m_pState->dependenceData.imageWidth    = width;
                m_pState->dependenceData.imageHeight   = height;
                m_pState->dependenceData.scalingFactor = scale;

                isChanged = TRUE;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupISP,
                           "Invalid Input: pAECUpdateData %p  pHwContext %p pNewAWBUpdate %p",
                           pInputData->pAECUpdateData,
                           pInputData->pHwContext,
                           pInputData->pAWBUpdateData);
        }

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

        m_tintlessConfig.tintlessParamConfig.applyTemporalFiltering = 1;

        m_pTintlessBGStats = pInputData->pStripeConfig->statsDataForISP.pParsedTintlessBGStats;

        m_moduleEnable &= dynamicEnable;
        if ((TRUE == m_moduleEnable) && (m_dynamicEnable != dynamicEnable))
        {
            isChanged = TRUE;
        }
        m_dynamicEnable = dynamicEnable;

        if ((TRUE == m_moduleEnable) &&
            (NULL != pInputData->pHALTagsData) &&
            (ShadingModeOff == pInputData->pHALTagsData->shadingMode))
        {
            m_moduleEnable = FALSE;
            isChanged      = FALSE;
        }

        if ((TRUE == m_pState->dependenceData.enableTintless) &&
            (TRUE == m_moduleEnable))
        {
            if (TRUE == pInputData->skipTintlessProcessing)
            {
                isChanged = FALSE;
            }
        }

        if ((NULL != pInputData->pHALTagsData) &&
            pInputData->pHALTagsData->colorCorrectionMode == ColorCorrectionModeTransformMatrix &&
            ((pInputData->pHALTagsData->controlAWBMode    == ControlAWBModeOff &&
              pInputData->pHALTagsData->controlMode       == ControlModeAuto) ||
              pInputData->pHALTagsData->controlMode       == ControlModeOff))
        {
            isChanged = FALSE;
        }

        if ((NULL != pInputData->pHALTagsData) &&
            (ControlAWBLockOn == pInputData->pHALTagsData->controlAWBLock) &&
            (ControlAELockOn  == pInputData->pHALTagsData->controlAECLock))
        {
            isChanged = FALSE;
        }

        // When called from PrepareStripingParameters pInputData->pCalculatedData will point to m_ISPFramelevelData
        pInputData->pCalculatedData->lscDependencyChanged = isChanged;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC34::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult      result       = CamxResultSuccess;
    LSCStripeState* pStripeState = NULL;

    if (NULL != pInputData)
    {
        m_pState                            = &pInputData->pStripeConfig->stateLSC;
        m_pState->dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        pStripeState                        = reinterpret_cast<LSCStripeState*>(m_pStripeInputState);

        if (TRUE == CheckDependenceChange(pInputData))
        {
            m_pState->dependenceData.fetchSettingOnly = TRUE;
            m_unpackedData.bIsDataReady = FALSE;

            result = RunCalculation(pInputData);

            if (CamxResultSuccess != result)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("LSC module calculation Failed.");
            }

            m_pState->dependenceData.fetchSettingOnly = FALSE;
            m_unpackedData.bIsDataReady = TRUE;
        }

        if ((NULL != pInputData->pStripingInput) && (NULL != pStripeState))
        {
            pInputData->pStripingInput->enableBits.rolloff = m_moduleEnable;
            pInputData->pStripingInput->stripingInput.rollOffParam.blockWidth       = pStripeState->bwidth_l;
            pInputData->pStripingInput->stripingInput.rollOffParam.bx_d1_l          = pStripeState->bx_d1_l;
            pInputData->pStripingInput->stripingInput.rollOffParam.bxStart          = pStripeState->bx_start_l;
            pInputData->pStripingInput->stripingInput.rollOffParam.lxStart          = pStripeState->lx_start_l;
            pInputData->pStripingInput->stripingInput.rollOffParam.meshGridBwidth   = pStripeState->meshGridBwidth_l;
            pInputData->pStripingInput->stripingInput.rollOffParam.numMeshgainHoriz = pStripeState->num_meshgain_h;
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Data");
        result = CamxResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELSC34::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult      result         = CamxResultSuccess;
    LSC34OutputData outputData;
    UINT32*         pLSCDMIAddr    = reinterpret_cast<UINT32*>(pInputData->p32bitDMIBufferAddr +
                                                               m_32bitDMIBufferOffsetDword +
                                                               (pInputData->pStripeConfig->stripeId * m_32bitDMILength));

    outputData.pUnpackedField     = &m_unpackedData.unpackedData;
    outputData.type               = PipelineType::IFE;
    outputData.pGRRLUTDMIBuffer   = pLSCDMIAddr;
    outputData.pGBBLUTDMIBuffer   = reinterpret_cast<UINT32*>((reinterpret_cast<UCHAR*>(outputData.pGRRLUTDMIBuffer) +
                                                              IFELSC34LUTTableSize));
    if (TRUE == pInputData->bankUpdate.isValid)
    {
        m_pState->dependenceData.bankSelect = pInputData->bankUpdate.LSCBank;
    }
    if (TRUE == m_unpackedData.bIsDataReady)
    {
        m_unpackedData.unpackedData.lx_start_l = m_pState->dependenceData.stripeOut.lx_start;
        m_unpackedData.unpackedData.bx_d1_l    = m_pState->dependenceData.stripeOut.bx_d1;
        m_unpackedData.unpackedData.bx_start_l = m_pState->dependenceData.stripeOut.bx_start;
        m_unpackedData.unpackedData.bank_sel   = m_pState->dependenceData.bankSelect;

        VOID* pInput = static_cast<VOID*>(outputData.pUnpackedField);

        result = m_pHWSetting->PackIQRegisterSetting(pInput, &outputData);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "LSC Packed Register setting is Failed");
        }
    }
    else
    {
        if ((pInputData->pHALTagsData->controlAWBLock == m_AWBLock)    &&
            (pInputData->pHALTagsData->controlAWBLock  == ControlAWBLockOn))
        {
            m_pState->dependenceData.enableTintless = FALSE;
        }
        else
        {
            m_AWBLock = pInputData->pHALTagsData->controlAWBLock;
        }

        if (TRUE == m_pState->dependenceData.enableTintless)
        {
            m_pState->dependenceData.pTintlessAlgo   = m_pTintlessAlgo;
            m_pState->dependenceData.pTintlessConfig = &m_tintlessConfig;
            m_pState->dependenceData.pTintlessStats  = m_pTintlessBGStats;
        }
        else
        {
            m_pState->dependenceData.pTintlessAlgo   = NULL;
            m_pState->dependenceData.pTintlessConfig = NULL;
            m_pState->dependenceData.pTintlessStats  = NULL;
        }

        m_pState->dependenceData.pTintlessRatio = &m_tintlessRatio;

        result = IQInterface::LSC34CalculateSetting(&m_pState->dependenceData,
                                                    pInputData->pOEMIQSetting,
                                                    pInputData,
                                                    &outputData);
        if (CamxResultSuccess == result)
        {
            Utils::Memcpy(m_pStripeInputState, &outputData.lscState, sizeof(LSCStripeState));
        }
    }

    if (CamxResultSuccess == result)
    {

        if (TRUE == m_pState->dependenceData.toCalibrate)
        {
            m_pState->dependenceData.toCalibrate = FALSE;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "LSC Calculation Failed.");
    }

    if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
    {
        m_pHWSetting->DumpRegConfig();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34::IFELSC34
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELSC34::IFELSC34()
{
    m_type                  = ISPIQModuleType::IFELSC;
    m_64bitDMILength        = 0;
    m_pChromatix            = NULL;
    m_AWBLock               = ControlAWBLockOff;
    m_shadingMode           = ShadingModeFast;
    m_lensShadingMapMode    = StatisticsLensShadingMapModeOff;
    m_pInterpolationData    = NULL;
    m_pStripeInputState     = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELSC34::DeallocateCommonLibraryData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELSC34::DeallocateCommonLibraryData()
{
    if (NULL != m_pInterpolationData)
    {
        CAMX_FREE(m_pInterpolationData);
        m_pInterpolationData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFELSC34::~IFELSC34
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELSC34::~IFELSC34()
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

    if (NULL != m_pCalibrationTable)
    {
        CAMX_FREE(m_pCalibrationTable);
        m_pCalibrationTable = NULL;
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

    m_pChromatix = NULL;
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
    DeallocateCommonLibraryData();

    if (NULL != m_pStripeInputState)
    {
        CAMX_FREE(m_pStripeInputState);
        m_pStripeInputState = NULL;
    }
}

CAMX_NAMESPACE_END
