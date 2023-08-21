////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebhiststats14.cpp
/// @brief Bayer Histogram (BHist) stats v1.4
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifebhiststats14.h"
#include "camxifebhiststats14titan17x.h"
#include "camxifebhiststats14titan480.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BHistStats14::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BHistStats14::Create(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        BHistStats14* pModule = CAMX_NEW BHistStats14;
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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create BHistStats object.");
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
// BHistStats14::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BHistStats14::Initialize(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEBHistStats14Titan480;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEBHistStats14Titan17x;
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
// BHistStats14::GetDMITable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BHistStats14::GetDMITable(
    UINT32** ppDMITable)
{
    CAMX_UNREFERENCED_PARAM(ppDMITable);

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BHistStats14::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BHistStats14::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    pInputData->pCalculatedData->metadata.BHistStatsConfig  = m_BHistConfig;

    m_pHWSetting->SetupRegisterSetting(&m_inputConfigData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BHistStats14::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BHistStats14::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL            result          = FALSE;
    BHistConfig*    pBHistConfig    = &pInputData->pStripeConfig->AECStatsUpdateData.statsConfig.BHistConfig;

    if ((m_BHistConfig.BHistConfig.channel    != pBHistConfig->channel)     ||
        (m_BHistConfig.BHistConfig.ROI.left   != pBHistConfig->ROI.left)    ||
        (m_BHistConfig.BHistConfig.ROI.top    != pBHistConfig->ROI.top)     ||
        (m_BHistConfig.BHistConfig.ROI.width  != pBHistConfig->ROI.width)   ||
        (m_BHistConfig.BHistConfig.ROI.height != pBHistConfig->ROI.height)  ||
        (m_BHistConfig.BHistConfig.uniform    != pBHistConfig->uniform)     ||
        (TRUE                                 == pInputData->forceTriggerUpdate))
    {
        m_BHistConfig.BHistConfig.ROI       = pBHistConfig->ROI;
        m_BHistConfig.BHistConfig.channel   = pBHistConfig->channel;
        m_BHistConfig.BHistConfig.uniform   = pBHistConfig->uniform;
        result                              = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BHistStats14::CalculateRegionConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BHistStats14::CalculateRegionConfiguration(
    BHist14ConfigData* pConfigData)
{
    UINT32              regionHorizNum;
    UINT32              regionVertNum;
    RectangleCoordinate newCoordinate;
    BHistRegionConfig*  pNewRegionConfig = &pConfigData->regionConfig;

    CAMX_ASSERT_MESSAGE(0 != m_BHistConfig.BHistConfig.ROI.width, "Invalid ROI Width");
    CAMX_ASSERT_MESSAGE(0 != m_BHistConfig.BHistConfig.ROI.height, "Invalid ROI height");

    // Calculate Offsets
    pNewRegionConfig->horizontalOffset      = Utils::FloorUINT32(m_inputConfigData.regionMultipleFactor,
                                                                 m_BHistConfig.BHistConfig.ROI.left);
    pNewRegionConfig->verticalOffset        = Utils::FloorUINT32(m_inputConfigData.regionMultipleFactor,
                                                                 m_BHistConfig.BHistConfig.ROI.top);

    // Calculate number of vertical and horizontal grids
    regionHorizNum                          = (m_BHistConfig.BHistConfig.ROI.width / BHistStats14RegionWidth) - 1;
    regionVertNum                           = (m_BHistConfig.BHistConfig.ROI.height / BHistStats14RegionHeight) - 1;

    pNewRegionConfig->horizontalRegionNum   = Utils::MaxUINT32(regionHorizNum, BHistStats14MinHorizRegions);
    pNewRegionConfig->verticalRegionNum     = Utils::MaxUINT32(regionVertNum, BHistStats14MinVertRegions);

    // Verify if region was adjusted
    newCoordinate.left      = pNewRegionConfig->horizontalOffset;
    newCoordinate.top       = pNewRegionConfig->verticalOffset;
    newCoordinate.width     = (pNewRegionConfig->horizontalRegionNum + 1) * BHistStats14RegionWidth;
    newCoordinate.height    = (pNewRegionConfig->verticalRegionNum + 1) * BHistStats14RegionHeight;

    if ((newCoordinate.left     != m_BHistConfig.BHistConfig.ROI.left)     ||
        (newCoordinate.top      != m_BHistConfig.BHistConfig.ROI.top)      ||
        (newCoordinate.width    != m_BHistConfig.BHistConfig.ROI.width)    ||
        (newCoordinate.height   != m_BHistConfig.BHistConfig.ROI.height))
    {
        m_BHistConfig.isAdjusted        = TRUE;
        m_BHistConfig.BHistConfig.ROI   = newCoordinate;
    }
    else
    {
        m_BHistConfig.isAdjusted        = FALSE;
    }

    pConfigData->channel    = MapColorChannelsToReg(m_BHistConfig.BHistConfig.channel);
    pConfigData->uniform    = MapUniformityBinToReg(m_BHistConfig.BHistConfig.uniform);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BHistStats14::MapColorChannelsToReg
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 BHistStats14::MapColorChannelsToReg(
    ColorChannel channel)
{
    INT32 mappedChannelRegister = 0;

    switch (channel)
    {
        case ColorChannel::ColorChannelB:
            mappedChannelRegister = ChannelSelectB;
            break;
        case ColorChannel::ColorChannelGB:
            mappedChannelRegister = ChannelSelectGb;
            break;
        case ColorChannel::ColorChannelGR:
            mappedChannelRegister = ChannelSelectGr;
            break;
        case ColorChannel::ColorChannelR:
            mappedChannelRegister = ChannelSelectR;
            break;
        case ColorChannel::ColorChannelY:
            mappedChannelRegister = ChannelSelectY;
            break;
        default:
            mappedChannelRegister = ChannelSelectY;
            break;
    }

    return mappedChannelRegister;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BHistStats14::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BHistStats14::RunCalculation(
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
// BHistStats14::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BHistStats14::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    INT32           inputWidth;
    INT32           inputHeight;
    CropInfo*       pSensorCAMIFCrop = &pInputData->pStripeConfig->CAMIFCrop;
    BHistConfig*    pBHistConfig     = &pInputData->pStripeConfig->AECStatsUpdateData.statsConfig.BHistConfig;
    CamxResult      result           = CamxResultSuccess;
    INT32           top              = 0;
    INT32           left             = 0;
    INT32           width            = 0;
    INT32           height           = 0;

    inputWidth  = pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1;
    inputHeight = pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1;

    // CAMIF input width for YUV sensor would be twice that of sensor width, as each pixel accounts for Y and U/V data
    if (TRUE == pInputData->sensorData.isYUV)
    {
        inputWidth >>= 1;
    }

    top    = pBHistConfig->ROI.top;
    left   = pBHistConfig->ROI.left;
    width  = pBHistConfig->ROI.width;
    height = pBHistConfig->ROI.height;

    // Validate ROI from Stats
    if ((left           <    0)             ||
        (top            <    0)             ||
        (width          <=   0)             ||
        (height         <=   0)             ||
        (inputWidth     <   (left + width)) ||
        (inputHeight    <   (top + height)) ||
        (0              ==  width)          ||
        (0              ==  height)         ||
        (FALSE          ==  isColorChannelValid(pBHistConfig->channel)))
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid config: ROI %d, %d, %d, %d, channel: %u",
                       pBHistConfig->ROI.left,
                       pBHistConfig->ROI.top,
                       pBHistConfig->ROI.width,
                       pBHistConfig->ROI.height,
                       pBHistConfig->channel);

        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BHistStats14::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BHistStats14::Execute(
    ISPInputData* pInputData)
{
    CamxResult  result          = CamxResultSuccess;
    VOID*       pSettingData    = static_cast<VOID*>(pInputData);

    CAMX_ASSERT_MESSAGE(NULL != pInputData, "BHist invalid ISPInputData pointer");

    m_inputConfigData.pISPInputData = pInputData;

    result = ValidateDependenceParams(pInputData);

    if (CamxResultSuccess == result &&
        TRUE              == CheckDependenceChange(pInputData))
    {
        RunCalculation(pInputData);
        result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
    }

    UpdateIFEInternalData(pInputData);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BHistStats14::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BHistStats14::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult   result         = CamxResultSuccess;
    UINT32       regionHorizNum = 0;
    BHistConfig* pHBistCfg      = &pInputData->pAECStatsUpdateData->statsConfig.BHistConfig;
    if (NULL != pInputData)
    {
        if (NULL != pInputData->pStripingInput)
        {
            pInputData->pStripingInput->stripingInput.bHistInput.bihistEnabled           = m_moduleEnable;
            regionHorizNum                                                                 =
                (pHBistCfg->ROI.width / BHistStats14RegionWidth) - 1;
            pInputData->pStripingInput->stripingInput.bHistInput.bihistRgnHorNum         =
                Utils::MaxUINT32(regionHorizNum, BHistStats14MinHorizRegions);
            pInputData->pStripingInput->stripingInput.bHistInput.bihistROIHorOffset      =
                Utils::FloorUINT32(m_inputConfigData.regionMultipleFactor, pHBistCfg->ROI.left);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BHistStats14::BHistStats14
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BHistStats14::BHistStats14()
{
    m_type                  = ISPStatsModuleType::IFEBHist;
    m_moduleEnable          = TRUE;

    /// @todo (CAMX-887) Support BHist stats tapout at HDR Recon module, Hardcode to tap out at Demosaic for now.
    // In Non uniform bin mode the pixel data will be mapped in the range of 0 - 1023
    // In unform bin the pixel data will be shifted from 14 bits to 10 bits without mapping
    m_BHistConfig.BHistConfig.uniform       = TRUE;
    m_BHistConfig.BHistConfig.channel       = ColorChannel::ColorChannelY;
    m_BHistConfig.numBins                   = MaxBHistBinNum;

    // Default input data
    m_inputConfigData.regionMultipleFactor  = MultipleFactorTwo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BHistStats14::~BHistStats14
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BHistStats14::~BHistStats14()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
