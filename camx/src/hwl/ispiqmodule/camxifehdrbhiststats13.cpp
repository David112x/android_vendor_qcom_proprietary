////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdrbhiststats13.cpp
/// @brief HDR Bayer Histogram (HDRBHist) stats v1.3
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifehdrbhiststats13.h"
#include "camxifehdrbhiststats13titan17x.h"
#include "camxifehdrbhiststats13titan480.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBHistStats13::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HDRBHistStats13::Create(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        HDRBHistStats13* pModule = CAMX_NEW HDRBHistStats13;
        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Module initialization failed !!");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create HDRBHistStats object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input Null pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBHistStats13::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HDRBHistStats13::Initialize(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEHDRBHistStats13Titan480;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEHDRBHistStats13Titan17x;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_cmdLength         = m_pHWSetting->GetCommandLength();
        m_32bitDMILength    = m_pHWSetting->Get32bitDMILength();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, Titan 0x%x", pCreateData->titanVersion);
        result = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBHistStats13::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HDRBHistStats13::UpdateIFEInternalData(
    const ISPInputData* pInputData)
{
    pInputData->pCalculatedData->metadata.HDRBHistStatsConfig   = m_HDRBHistConfig;

    m_pHWSetting->SetupRegisterSetting(&m_inputConfigData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBHistStats13::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HDRBHistStats13::CheckDependenceChange(
    const ISPInputData* pInputData)
{
    BOOL                    configRequired  = FALSE;
    const HDRBHistConfig*   pHDRBHistConfig = &pInputData->pStripeConfig->AECStatsUpdateData.statsConfig.HDRBHistConfig;
    HDRBHistConfig*         pCurrentConfig  = &m_HDRBHistConfig.HDRBHistConfig;

    if ((pCurrentConfig->ROI.left           != pHDRBHistConfig->ROI.left)           ||
        (pCurrentConfig->ROI.top            != pHDRBHistConfig->ROI.top)            ||
        (pCurrentConfig->ROI.width          != pHDRBHistConfig->ROI.width)          ||
        (pCurrentConfig->ROI.height         != pHDRBHistConfig->ROI.height)         ||
        (pCurrentConfig->greenChannelInput  != pHDRBHistConfig->greenChannelInput)  ||
        (pCurrentConfig->inputFieldSelect   != pHDRBHistConfig->inputFieldSelect)   ||
        (TRUE                               == pInputData->forceTriggerUpdate))
    {
        pCurrentConfig->ROI                 = pHDRBHistConfig->ROI;
        pCurrentConfig->greenChannelInput   = pHDRBHistConfig->greenChannelInput;
        pCurrentConfig->inputFieldSelect    = pHDRBHistConfig->inputFieldSelect;

        configRequired = TRUE;
    }
    else
    {
        configRequired              = FALSE;
        m_HDRBHistConfig.isAdjusted = FALSE;
    }

    return configRequired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBHistStats13::CalculateRegionConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HDRBHistStats13::CalculateRegionConfiguration(
    HDRBHist13ConfigData* pConfigData)
{
    HDRBHistConfig*         pHDRBHistConfig = &m_HDRBHistConfig.HDRBHistConfig;
    HDRBHistRegionConfig*   pRegionConfig   = &pConfigData->regionConfig;
    RectangleCoordinate newCoordinate;

    CAMX_ASSERT_MESSAGE(0 != pHDRBHistConfig->ROI.width, "Invalid ROI width");
    CAMX_ASSERT_MESSAGE(0 != pHDRBHistConfig->ROI.height, "Invalid ROI height");

    m_HDRBHistConfig.isAdjusted = FALSE;

    // Set region
    pRegionConfig->offsetHorizNum   = Utils::FloorUINT32(pConfigData->regionMultipleFactor, pHDRBHistConfig->ROI.left);
    pRegionConfig->offsetVertNum    = Utils::FloorUINT32(pConfigData->regionMultipleFactor, pHDRBHistConfig->ROI.top);

    pRegionConfig->regionHorizNum   = (pHDRBHistConfig->ROI.width / HDRBHistStats13RegionWidth) - 1;
    pRegionConfig->regionVertNum    = (pHDRBHistConfig->ROI.height / HDRBHistStats13RegionHeight) - 1;
    pRegionConfig->regionHorizNum   = Utils::MaxUINT32(pRegionConfig->regionHorizNum, HDRBHistStats13MinHorizregions);
    pRegionConfig->regionVertNum    = Utils::MaxUINT32(pRegionConfig->regionVertNum, HDRBHistStats13MinVertregions);

    // Verify if region was adjusted
    newCoordinate.left      = pRegionConfig->offsetHorizNum;
    newCoordinate.top       = pRegionConfig->offsetVertNum;
    newCoordinate.width     = (pRegionConfig->regionHorizNum + 1) * HDRBHistStats13RegionWidth;
    newCoordinate.height    = (pRegionConfig->regionVertNum + 1) * HDRBHistStats13RegionHeight;

    if ((newCoordinate.left     != pHDRBHistConfig->ROI.left)    ||
        (newCoordinate.top      != pHDRBHistConfig->ROI.top)     ||
        (newCoordinate.width    != pHDRBHistConfig->ROI.width)   ||
        (newCoordinate.height   != pHDRBHistConfig->ROI.height))
    {
        pHDRBHistConfig->ROI = newCoordinate;
        m_HDRBHistConfig.isAdjusted = TRUE;
    }

    // Set green channel
    switch (pHDRBHistConfig->greenChannelInput)
    {
        case HDRBHistSelectGB:
            pConfigData->greenChannelSelect = HDRBHistGreenChannelSelectGb;
            break;
        case HDRBHistSelectGR:
            pConfigData->greenChannelSelect = HDRBHistGreenChannelSelectGr;
            break;
        default:
            pConfigData->greenChannelSelect                     = HDRBHistGreenChannelSelectGb;
            m_HDRBHistConfig.HDRBHistConfig.greenChannelInput   = HDRBHistSelectGB;
            m_HDRBHistConfig.isAdjusted                         = TRUE;
            break;
    }

    // Set input field/mode
    switch (pHDRBHistConfig->inputFieldSelect)
    {
        case HDRBHistInputAll:
            pConfigData->inputFieldSelect   = HDRBHistFieldSelectAll;
            break;
        case HDRBHistInputLongExposure:
            pConfigData->inputFieldSelect   = HDRBHistFieldSelectT1;
            break;
        case HDRBHistInputShortExposure:
            pConfigData->inputFieldSelect   = HDRBHistFieldSelectT2;
            break;
        default:
            pConfigData->inputFieldSelect       = HDRBHistFieldSelectAll;
            pHDRBHistConfig->inputFieldSelect   = HDRBHistInputAll;
            m_HDRBHistConfig.isAdjusted         = TRUE;
            break;
    }

    if ((HDRBHistFieldSelectAll == pConfigData->inputFieldSelect) &&
        (0 == pConfigData->pISPInputData->sensorData.ZZHDRColorPattern))
    {
        // Non-HDR
        pConfigData->ZZHDRFirstRBEXP    = 0;
        pConfigData->ZZHDRPattern       = 0;
    }
    else if ((HDRBHistFieldSelectT1 <= pConfigData->inputFieldSelect) &&
             (0 < pConfigData->pISPInputData->sensorData.ZZHDRColorPattern))
    {
        // HDR sensor
        pConfigData->ZZHDRFirstRBEXP    = static_cast<UINT16>(pConfigData->pISPInputData->sensorData.ZZHDRFirstExposure);
        pConfigData->ZZHDRPattern       = pConfigData->pISPInputData->sensorData.ZZHDRColorPattern;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Error in configuration values default to non-HDR");
        pConfigData->ZZHDRFirstRBEXP    = 0;
        pConfigData->ZZHDRPattern       = 0;
    }

    m_HDRBHistConfig.numBins = HDRBHistBinsPerChannel;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBHistStats13::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HDRBHistStats13::RunCalculation(
    ISPInputData* pInputData)
{
    CalculateRegionConfiguration(&m_inputConfigData);

    m_pHWSetting->PackIQRegisterSetting(&m_inputConfigData, NULL);

    if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
    {
        m_pHWSetting->DumpRegConfig();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBHistStats13::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HDRBHistStats13::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;
    if (NULL != pInputData)
    {
        if (NULL != pInputData->pStripingInput)
        {
            HDRBHistConfig* pHDRBHistCfg = &pInputData->pAECStatsUpdateData->statsConfig.HDRBHistConfig;

            pInputData->pStripingInput->stripingInput.HDRBhistInput.bihistEnabled      = m_moduleEnable;
            pInputData->pStripingInput->stripingInput.HDRBhistInput.bihistROIHorOffset =
                Utils::FloorUINT32(m_inputConfigData.regionMultipleFactor, pHDRBHistCfg->ROI.left);
            pInputData->pStripingInput->stripingInput.HDRBhistInput.bihistRgnHorNum    =
                pHDRBHistCfg->ROI.width / HDRBHistStats13RegionWidth - 1;
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
// HDRBHistStats13::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HDRBHistStats13::ValidateDependenceParams(
    ISPInputData* pInputData
    ) const
{
    CamxResult      result              = CamxResultSuccess;
    CropInfo*       pSensorCAMIFCrop    = &pInputData->pStripeConfig->CAMIFCrop;
    HDRBHistConfig* pHDRBHistConfig     = &pInputData->pStripeConfig->AECStatsUpdateData.statsConfig.HDRBHistConfig;
    INT32           inputWidth          = pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1;
    INT32           inputHeight         = pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1;
    INT32           top                 = 0;
    INT32           left                = 0;
    INT32           width               = 0;
    INT32           height              = 0;

    // CAMIF input width for YUV sensor would be twice that of sensor width, as each pixel accounts for Y and U/V data
    if (TRUE == pInputData->sensorData.isYUV)
    {
        inputWidth >>= 1;
    }

    top    = pHDRBHistConfig->ROI.top;
    left   = pHDRBHistConfig->ROI.left;
    width  = pHDRBHistConfig->ROI.width;
    height = pHDRBHistConfig->ROI.height;

    // Validate ROI from Stats
    if ((left            <  0)           ||
        (top             <  0)           ||
        (width           <=  0)          ||
        (height          <=  0)          ||
        ((left + width)  > inputWidth)   ||
        ((top  + height) > inputHeight)  ||
        (width           == 0)           ||
        (height          == 0))
    {
        CAMX_LOG_ERROR(CamxLogGroupISP,
                       "Invalid config: ROI %d, %d, %d, %d",
                       pHDRBHistConfig->ROI.left,
                       pHDRBHistConfig->ROI.top,
                       pHDRBHistConfig->ROI.width,
                       pHDRBHistConfig->ROI.height);

        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBHistStats13::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HDRBHistStats13::Execute(
    ISPInputData* pInputData)
{
    CamxResult  result       = CamxResultSuccess;
    VOID*       pSettingData = static_cast<VOID*>(pInputData);

    CAMX_ASSERT_MESSAGE(NULL != pInputData, "HDRBHist invalid ISPInputData pointer");
    CAMX_ASSERT_MESSAGE(NULL != pInputData->pCmdBuffer, "HDRBHist invalid pCmdBuffer");

    if ((NULL != pInputData) && (NULL != pInputData->pCmdBuffer))
    {
        m_inputConfigData.pISPInputData = pInputData;

        result = ValidateDependenceParams(pInputData);
        if ((CamxResultSuccess == result) &&
            (TRUE == CheckDependenceChange(pInputData)))
        {
            RunCalculation(pInputData);
            result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
        }

        UpdateIFEInternalData(pInputData);
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBHistStats13::HDRBHistStats13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HDRBHistStats13::HDRBHistStats13()
{
    m_type          = ISPStatsModuleType::IFEHDRBHist;
    m_moduleEnable  = TRUE;

    // Set default input configuration
    m_inputConfigData.greenChannelSelect    = HDRBHistGreenChannelSelectGr;
    m_inputConfigData.inputFieldSelect      = HDRBHistFieldSelectAll;
    /// @note To keep simple configuration between non-HDR and HDR sensors, using worst multipe factor
    m_inputConfigData.regionMultipleFactor  = MultipleFactorFour;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBHistStats13::~HDRBHistStats13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HDRBHistStats13::~HDRBHistStats13()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
