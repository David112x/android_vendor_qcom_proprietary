////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeihiststats12.cpp
/// @brief Image Histogram (IHist) class definition v1.2
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifeihiststats12.h"
#include "camxifeihiststats12titan17x.h"
#include "camxifeihiststats12titan480.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

class Utils;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IHistStats12::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IHistStats12::Create(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IHistStats12* pModule = CAMX_NEW IHistStats12;
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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IHistStats object.");
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
// IHistStats12::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IHistStats12::Initialize(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEIHistStats12Titan480;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEIHistStats12Titan17x;
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
// IHistStats12::GetDMITable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IHistStats12::GetDMITable(
    UINT32** ppDMITable)
{
    CAMX_UNREFERENCED_PARAM(ppDMITable);

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IHistStats12::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IHistStats12::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    CamxResult          result          = CamxResultSuccess;
    IHistStatsConfig*   pIHistConfig    = &pInputData->pStripeConfig->IHistStatsUpdateData.statsConfig;

    if ((0 == pIHistConfig->ROI.left)    &&
        (0 == pIHistConfig->ROI.top)     &&
        (0 == pIHistConfig->ROI.width)   &&
        (0 == pIHistConfig->ROI.height))
    {
        // If zero do default
        m_defaultConfig = TRUE;
        result          = CamxResultSuccess;
    }
    else
    {
        CropInfo*           pSensorCAMIFCrop    = &pInputData->pStripeConfig->CAMIFCrop;
        UINT32              inputWidth          = pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1;
        UINT32              inputHeight         = pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1;

        m_defaultConfig = FALSE;

        // CAMIF input width for YUV sensor would be twice that of sensor width, as each pixel accounts for Y and U/V data
        if (TRUE == pInputData->sensorData.isYUV)
        {
            inputWidth >>= 1;
        }

        // Validate ROI from Stats
        if (((pIHistConfig->ROI.left + pIHistConfig->ROI.width) > inputWidth)                         ||
            ((pIHistConfig->ROI.top + pIHistConfig->ROI.height) > inputHeight)                        ||
            (0                                                                == pIHistConfig->ROI.width)    ||
            (0                                                                == pIHistConfig->ROI.height)   ||
            ((IHistStats12ChannelYCC_Y                                        != pIHistConfig->channelYCC)          &&
            (IHistStats12ChannelYCC_Cb                                        != pIHistConfig->channelYCC)          &&
            (IHistStats12ChannelYCC_Cr                                        != pIHistConfig->channelYCC)))
        {
            CAMX_LOG_ERROR(CamxLogGroupISP,
                "Invalid config: ROI %d, %d, %d, %d. YCC: %u, max pixels per bin: %u",
                pIHistConfig->ROI.left,
                pIHistConfig->ROI.top,
                pIHistConfig->ROI.width,
                pIHistConfig->ROI.height,
                pIHistConfig->channelYCC,
                pIHistConfig->maxPixelSumPerBin);
            result = CamxResultEInvalidArg;
            CAMX_ASSERT_ALWAYS();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IHistStats12::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IHistStats12::CheckDependenceChange(
    const ISPInputData* pInputData)
{
    BOOL result = FALSE;

    if (TRUE == m_defaultConfig)
    {
        CropInfo*           pSensorCAMIFCrop    = &pInputData->pStripeConfig->CAMIFCrop;
        UINT32              inputWidth          = pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1;
        UINT32              inputHeight         = pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1;
        // Setting defaults
        // CAMIF input width for YUV sensor would be twice that of sensor width, as each pixel accounts for Y and U/V data
        if (TRUE == pInputData->sensorData.isYUV)
        {
            inputWidth >>= 1;
        }

        if ((inputWidth  != m_currentROI.width) ||
            (inputHeight != m_currentROI.height))
        {
            m_currentROI.left           = 0;
            m_currentROI.top            = 0;
            m_currentROI.width          = inputWidth;
            m_currentROI.height         = inputHeight;
            m_IHistYCCChannelSelection  = IHistStats12ChannelYCC_Y;
            m_maxPixelSumPerBin         = 0;

            result = TRUE;
        }
    }
    else
    {
        IHistStatsConfig* pIHistConfig = &pInputData->pStripeConfig->IHistStatsUpdateData.statsConfig;

        if ((m_currentROI.left          != pIHistConfig->ROI.left)   ||
            (m_currentROI.top           != pIHistConfig->ROI.top)    ||
            (m_currentROI.width         != pIHistConfig->ROI.width)  ||
            (m_currentROI.height        != pIHistConfig->ROI.height) ||
            (m_maxPixelSumPerBin        != pIHistConfig->maxPixelSumPerBin) ||
            (m_IHistYCCChannelSelection != pIHistConfig->channelYCC) ||
            (TRUE                       == pInputData->forceTriggerUpdate))
        {
            m_currentROI                = pIHistConfig->ROI;
            m_IHistYCCChannelSelection  = static_cast<UINT8>(pIHistConfig->channelYCC);
            m_maxPixelSumPerBin         = pIHistConfig->maxPixelSumPerBin;

            result = TRUE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IHistStats12::CalculateRegionConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IHistStats12::CalculateRegionConfiguration(
    IHist12ConfigData* pConfigData)
{
    CAMX_ASSERT_MESSAGE(NULL != pConfigData, "IHist Invalid input pointer");
    CAMX_ASSERT_MESSAGE(0 != m_currentROI.width, "IHist Invalid ROI Width");
    CAMX_ASSERT_MESSAGE(0 != m_currentROI.height, "IHist Invalid ROI height");

    UINT32 maxPixelSumPerBin    = 0;
    UINT32 shiftBits            = 0;
    UINT32 regionHNum           = (m_currentROI.width  / IHistStats12RegionWidth)  - 1;
    UINT32 regionVNum           = (m_currentROI.height / IHistStats12RegionHeight) - 1;

    // Check region minimum requirement criteria. Max value is based on CAMIF output which is the current config value.
    regionHNum = Utils::MaxUINT32(regionHNum, IHistStats12RegionMinHNum);
    regionVNum = Utils::MaxUINT32(regionVNum, IHistStats12RegionMinVNum);

    if (0 == m_maxPixelSumPerBin)
    {
        // Configure shift value base on total number of pixels used for Image Histogram
        maxPixelSumPerBin = static_cast<UINT32>(static_cast<FLOAT>(((regionHNum + 1) * (regionVNum + 1))) * 2.0);
        // From HLD, worst case scenario, all pixels fall into these number of bins
        maxPixelSumPerBin /= IHistStats12MinNumberOfBins;

        // Save max pixel value
        m_maxPixelSumPerBin = maxPixelSumPerBin;
    }
    else
    {
        maxPixelSumPerBin = m_maxPixelSumPerBin;
    }

    UINT32 minBits = Utils::MinBitsUINT32(maxPixelSumPerBin); // Get the minimum number of bits to represent all the pixels
    if (IHistStats12StatsBits < minBits)
    {
        minBits     -= IHistStats12StatsBits;
        shiftBits   = Utils::ClampUINT32(minBits, 0, IHistStats12MaxStatsShift);

        if (minBits != shiftBits)
        {
            UINT32 newMaxPixelSumPerBin = 0;

            // Calculate the saturation pixel values used per bin
            newMaxPixelSumPerBin    = Utils::AllOnes32LeftShift(32, IHistStats12StatsBits + shiftBits);
            newMaxPixelSumPerBin    = newMaxPixelSumPerBin ^ (~0);
            m_maxPixelSumPerBin     = newMaxPixelSumPerBin;
        }
    }
    else
    {
        shiftBits = 0;
    }

    // Saving final configuration
    pConfigData->shiftBits                          = static_cast<UINT8>(shiftBits);
    pConfigData->YCCChannel                         = m_IHistYCCChannelSelection;
    pConfigData->regionConfig.horizontalOffset      = m_currentROI.left;
    pConfigData->regionConfig.verticalOffset        = m_currentROI.top;
    pConfigData->regionConfig.horizontalRegionNum   = regionHNum;
    pConfigData->regionConfig.verticalRegionNum     = regionVNum;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IHistStats12::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IHistStats12::RunCalculation(
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
// IHistStats12::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IHistStats12::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        if (NULL != pInputData->pStripingInput)
        {
            IHistStatsConfig*   pIHistConfig    = &pInputData->pStripeConfig->IHistStatsUpdateData.statsConfig;
            UINT32              imageWidth      = pInputData->pStripeConfig->CAMIFCrop.lastPixel -
                                                  pInputData->pStripeConfig->CAMIFCrop.firstPixel + 1;
            UINT32              regionWidth     = (0 == pIHistConfig->ROI.width) ? imageWidth : pIHistConfig->ROI.width;
            UINT32              regionHNum      = regionWidth / IHistStats12RegionWidth;

            regionHNum = Utils::MaxUINT32(regionHNum, IHistStats12RegionMinHNum);

            pInputData->pStripingInput->stripingInput.iHistInput.enable           = m_moduleEnable;
            pInputData->pStripingInput->stripingInput.iHistInput.histRgnHorNum    = regionHNum - 1;
            pInputData->pStripingInput->stripingInput.iHistInput.histRgnHorOffset = pIHistConfig->ROI.left;
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
// IHistStats12::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IHistStats12::Execute(
    ISPInputData* pInputData)
{
    CamxResult  result          = CamxResultSuccess;
    VOID*       pSettingData    = static_cast<VOID*>(pInputData);

    if (NULL != pInputData)
    {
        m_inputConfigData.pISPInputData = pInputData;

        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);

        if (CamxResultSuccess == result &&
            TRUE == CheckDependenceChange(pInputData))
        {
            RunCalculation(pInputData);
            result = m_pHWSetting->CreateCmdList(pSettingData, &m_32bitDMIBufferOffsetDword);
        }

        if (CamxResultSuccess == result)
        {
            // Update CAMX metadata with the values for the current frame
            UpdateIFEInternalData(pInputData);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("IHist invalid ISPInputData pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IHistStats12::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IHistStats12::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    // Update metadata info
    pInputData->pCalculatedData->metadata.IHistStatsConfig.IHistConfig.maxPixelSumPerBin    = m_maxPixelSumPerBin;
    pInputData->pCalculatedData->metadata.IHistStatsConfig.IHistConfig.channelYCC           =
        static_cast<IHistChannelSelection>(m_IHistYCCChannelSelection);
    pInputData->pCalculatedData->metadata.IHistStatsConfig.IHistConfig.ROI                  = m_currentROI;
    pInputData->pCalculatedData->metadata.IHistStatsConfig.numBins                          = IHistStats12BinsPerChannel;

    m_pHWSetting->SetupRegisterSetting(&m_inputConfigData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IHistStats12::IHistStats12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IHistStats12::IHistStats12()
{
    m_type         = ISPStatsModuleType::IFEIHist;
    m_moduleEnable = TRUE;

    // Set fixed default configuration
    m_IHistYCCChannelSelection  = IHistStats12ChannelYCC_Y;
    m_maxPixelSumPerBin         = 0; // Use default estimation
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IHistStats12::~IHistStats12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IHistStats12::~IHistStats12()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
