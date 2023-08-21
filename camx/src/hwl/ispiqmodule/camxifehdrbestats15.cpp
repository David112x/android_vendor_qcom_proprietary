////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdrbestats15.cpp
/// @brief HDRBE Stats15 Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifehdrbestats15.h"
#include "camxifehdrbestats15titan17x.h"
#include "camxifehdrbestats15titan480.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBEStats15::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HDRBEStats15::Create(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        HDRBEStats15* pModule = CAMX_NEW HDRBEStats15;
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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFEHDRBEStats object.");
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
// HDRBEStats15::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HDRBEStats15::Initialize(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEHDRBEStats15Titan480;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEHDRBEStats15Titan17x;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_cmdLength = m_pHWSetting->GetCommandLength();
        result      = m_pHWSetting->GetHWCapability(&m_hwCapability);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, Titan 0x%x", pCreateData->titanVersion);
        result = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBEStats15::GetDMITable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HDRBEStats15::GetDMITable(
    UINT32** ppDMITable)
{
    CAMX_UNREFERENCED_PARAM(ppDMITable);

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBEStats15::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HDRBEStats15::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL        result          = FALSE;
    BGBEConfig* pHDRBEConfig    = &pInputData->pStripeConfig->AECStatsUpdateData.statsConfig.BEConfig;

    if ((m_HDRBEConfig.horizontalNum                        != pHDRBEConfig->horizontalNum)                         ||
        (m_HDRBEConfig.verticalNum                          != pHDRBEConfig->verticalNum)                           ||
        (m_HDRBEConfig.ROI.left                             != pHDRBEConfig->ROI.left)                              ||
        (m_HDRBEConfig.ROI.top                              != pHDRBEConfig->ROI.top)                               ||
        (m_HDRBEConfig.ROI.width                            != pHDRBEConfig->ROI.width)                             ||
        (m_HDRBEConfig.ROI.height                           != pHDRBEConfig->ROI.height)                            ||
        (m_HDRBEConfig.outputBitDepth                       != pHDRBEConfig->outputBitDepth)                        ||
        (m_HDRBEConfig.outputMode                           != pHDRBEConfig->outputMode)                            ||
        (m_HDRBEConfig.YStatsWeights[0]                     != pHDRBEConfig->YStatsWeights[0])                      ||
        (m_HDRBEConfig.YStatsWeights[1]                     != pHDRBEConfig->YStatsWeights[1])                      ||
        (m_HDRBEConfig.YStatsWeights[2]                     != pHDRBEConfig->YStatsWeights[2])                      ||
        (m_HDRBEConfig.greenType                            != pHDRBEConfig->greenType)                             ||
        (m_HDRBEConfig.channelGainThreshold[ChannelIndexR]  != pHDRBEConfig->channelGainThreshold[ChannelIndexR])   ||
        (m_HDRBEConfig.channelGainThreshold[ChannelIndexGR] != pHDRBEConfig->channelGainThreshold[ChannelIndexGR])  ||
        (m_HDRBEConfig.channelGainThreshold[ChannelIndexB]  != pHDRBEConfig->channelGainThreshold[ChannelIndexB])   ||
        (m_HDRBEConfig.channelGainThreshold[ChannelIndexGB] != pHDRBEConfig->channelGainThreshold[ChannelIndexGB])  ||
        (TRUE                                               == pInputData->forceTriggerUpdate))
    {
        Utils::Memcpy(&m_HDRBEConfig, pHDRBEConfig, sizeof(BGBEConfig));

        result = TRUE;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBEStats15::AdjustROIParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HDRBEStats15::AdjustROIParams()
{
    UINT32 regionWidth;
    UINT32 regionHeight;
    BOOL   isAdjusted = FALSE;


    if ((m_HDRBEConfig.ROI.width  < (m_hwCapability.regionMinWidth * m_HDRBEConfig.horizontalNum)) ||
        (m_HDRBEConfig.ROI.height < m_hwCapability.regionMinHeight * m_HDRBEConfig.verticalNum))
    {
        m_HDRBEConfig.ROI.width =
            (m_HDRBEConfig.ROI.width < (m_hwCapability.regionMinWidth * m_HDRBEConfig.horizontalNum)) ?
            (m_hwCapability.regionMinWidth * m_HDRBEConfig.horizontalNum) : m_HDRBEConfig.ROI.width;
        m_HDRBEConfig.ROI.height =
            (m_HDRBEConfig.ROI.height < (m_hwCapability.regionMinHeight * m_HDRBEConfig.verticalNum)) ?
            (m_hwCapability.regionMinHeight * m_HDRBEConfig.verticalNum) : m_HDRBEConfig.ROI.height;

        isAdjusted = TRUE;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Adjusted Min ROI config HNum %d ROI [%d %d %d %d]",
                         m_HDRBEConfig.horizontalNum,
                         m_HDRBEConfig.ROI.left,
                         m_HDRBEConfig.ROI.top,
                         m_HDRBEConfig.ROI.width,
                         m_HDRBEConfig.ROI.height);
    }

    if ((m_HDRBEConfig.ROI.width  > (m_hwCapability.regionMaxWidth * m_HDRBEConfig.horizontalNum)) ||
        (m_HDRBEConfig.ROI.height > m_hwCapability.regionMaxHeight * m_HDRBEConfig.verticalNum))
    {
        m_HDRBEConfig.ROI.width =
            (m_HDRBEConfig.ROI.width > (m_hwCapability.regionMaxWidth * m_HDRBEConfig.horizontalNum)) ?
            (m_hwCapability.regionMaxWidth * m_HDRBEConfig.horizontalNum) : m_HDRBEConfig.ROI.width;
        m_HDRBEConfig.ROI.height =
            (m_HDRBEConfig.ROI.height > (m_hwCapability.regionMaxHeight * m_HDRBEConfig.verticalNum)) ?
            (m_hwCapability.regionMaxHeight * m_HDRBEConfig.verticalNum) : m_HDRBEConfig.ROI.height;

        isAdjusted = TRUE;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Adjusted Max ROI config HNum %d ROI [%d %d %d %d]",
                         m_HDRBEConfig.horizontalNum,
                         m_HDRBEConfig.ROI.left,
                         m_HDRBEConfig.ROI.top,
                         m_HDRBEConfig.ROI.width,
                         m_HDRBEConfig.ROI.height);
    }

    CAMX_ASSERT_MESSAGE(0 != m_HDRBEConfig.horizontalNum, "Invalid horizontalNum");
    CAMX_ASSERT_MESSAGE(0 != m_HDRBEConfig.verticalNum, "Invalid verticalNum");

    CAMX_ASSERT_MESSAGE(0 != m_HDRBEConfig.ROI.width, "Invalid ROI Width");
    CAMX_ASSERT_MESSAGE(0 != m_HDRBEConfig.ROI.height, "Invalid ROI height");

    regionWidth  = m_HDRBEConfig.ROI.width / m_HDRBEConfig.horizontalNum;
    regionHeight = m_HDRBEConfig.ROI.height / m_HDRBEConfig.verticalNum;

    // The value n must be even for Demosaic tap out and 4 for HDR recon tapout to ensure correct Bayer pattern
    regionWidth  = Utils::FloorUINT32(m_regionMultipleFactor, regionWidth);
    regionHeight = Utils::FloorUINT32(m_regionMultipleFactor, regionHeight);

    // Check region minimum and maximum width criteria
    if (regionWidth < m_hwCapability.regionMinWidth)
    {
        regionWidth                 = m_hwCapability.regionMinWidth;
        m_HDRBEConfig.ROI.width     = m_HDRBEConfig.horizontalNum * regionWidth;
        isAdjusted                  = TRUE;
    }
    else if (regionWidth > m_hwCapability.regionMaxWidth)
    {
        regionWidth                 = m_hwCapability.regionMaxWidth;
        m_HDRBEConfig.ROI.width     = m_HDRBEConfig.horizontalNum * regionWidth;
        isAdjusted                  = TRUE;
    }

    // Check region minimum and maximum Height criteria
    if (regionHeight < m_hwCapability.regionMinHeight)
    {
        regionHeight              = m_hwCapability.regionMinHeight;
        m_HDRBEConfig.ROI.height  = m_HDRBEConfig.verticalNum * regionHeight;
        isAdjusted                = TRUE;
    }
    else if (regionHeight > m_hwCapability.regionMaxHeight)
    {
        regionHeight              = m_hwCapability.regionMaxHeight;
        m_HDRBEConfig.ROI.height  = m_HDRBEConfig.verticalNum * regionHeight;
        isAdjusted                = TRUE;
    }

    // Adjust Max Channel threshold values
    if (m_HDRBEConfig.channelGainThreshold[ChannelIndexR] > m_hwCapability.maxChannelThreshold)
    {
        m_HDRBEConfig.channelGainThreshold[ChannelIndexR] = m_hwCapability.maxChannelThreshold;
        isAdjusted                                        = TRUE;
    }

    if (m_HDRBEConfig.channelGainThreshold[ChannelIndexB] > m_hwCapability.maxChannelThreshold)
    {
        m_HDRBEConfig.channelGainThreshold[ChannelIndexB] = m_hwCapability.maxChannelThreshold;
        isAdjusted                                        = TRUE;
    }

    if (m_HDRBEConfig.channelGainThreshold[ChannelIndexGR] > m_hwCapability.maxChannelThreshold)
    {
        m_HDRBEConfig.channelGainThreshold[ChannelIndexGR] = m_hwCapability.maxChannelThreshold;
        isAdjusted                                         = TRUE;
    }

    if (m_HDRBEConfig.channelGainThreshold[ChannelIndexGB] > m_hwCapability.maxChannelThreshold)
    {
        m_HDRBEConfig.channelGainThreshold[ChannelIndexGB] = m_hwCapability.maxChannelThreshold;
        isAdjusted                                         = TRUE;
    }

    m_regionWidth  = regionWidth;
    m_regionHeight = regionHeight;
    m_isAdjusted   = isAdjusted;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBEStats15::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HDRBEStats15::RunCalculation(
    ISPInputData* pInputData)
{
    AdjustROIParams();

    m_inputConfigData.pHDRBEConfig = &m_HDRBEConfig;
    m_inputConfigData.regionHeight          = m_regionHeight;
    m_inputConfigData.regionWidth           = m_regionWidth;
    m_inputConfigData.regionMultipleFactor  = m_regionMultipleFactor;
    m_inputConfigData.fieldSelect           = m_fieldSelect;

    /// @todo (CAMX-887) Add Black level subtraction and scaler module support for tapping HDR BE stats before HDR recon module
    m_pHWSetting->PackIQRegisterSetting(&m_inputConfigData, NULL);

    if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
    {
        m_pHWSetting->DumpRegConfig();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBEStats15::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HDRBEStats15::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {

        m_inputConfigData.pISPInputData = pInputData;
        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);

        if (CamxResultSuccess == result)
        {
            CheckDependenceChange(pInputData);
            RunCalculation(pInputData);
        }

        if (NULL != pInputData->pStripingInput)
        {
            if ((m_HDRBEConfig.horizontalNum != 0) && (m_HDRBEConfig.verticalNum != 0))
            {
                pInputData->pStripingInput->enableBits.BE                       = m_moduleEnable;
                pInputData->pStripingInput->stripingInput.BEEnable              = static_cast<int16_t>(m_moduleEnable);
                pInputData->pStripingInput->stripingInput.BEInput.BEEnable      = m_moduleEnable;
                pInputData->pStripingInput->stripingInput.BEInput.BERgnHorizNum = m_HDRBEConfig.horizontalNum - 1;
                pInputData->pStripingInput->stripingInput.BEInput.BERgnVertNum  = m_HDRBEConfig.verticalNum - 1;
                pInputData->pStripingInput->stripingInput.BEInput.BERgnWidth    =
                    (m_HDRBEConfig.ROI.width / m_HDRBEConfig.horizontalNum) - 1;

                pInputData->pStripingInput->stripingInput.BEInput.BEROIHorizOffset = m_HDRBEConfig.ROI.left;
                pInputData->pStripingInput->stripingInput.tappingPointBE           =
                    pInputData->statsTapOut.HDRBEStatsSrcSelection;
                /// @todo (CAMX-856) Update HDRBE power optimization config after support from stats
                pInputData->pStripingInput->stripingInput.BEInput.BESatOutputEnable =
                    (BGBESaturationEnabled == m_HDRBEConfig.outputMode) ? 1 : 0;
                pInputData->pStripingInput->stripingInput.BEInput.BEYOutputEnable   =
                    (BGBEYStatsEnabled == m_HDRBEConfig.outputMode) ? 1 : 0;
            }
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
// HDRBEStats15::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HDRBEStats15::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    INT32       inputWidth;
    INT32       inputHeight;
    CropInfo*   pSensorCAMIFCrop = &pInputData->pStripeConfig->CAMIFCrop;
    BGBEConfig* pHDRBEConfig     = &pInputData->pStripeConfig->AECStatsUpdateData.statsConfig.BEConfig;
    CamxResult  result           = CamxResultSuccess;
    INT32       top              = 0;
    INT32       left             = 0;
    INT32       width            = 0;
    INT32       height           = 0;

    inputWidth  = pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1;
    inputHeight = pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1;

    // CAMIF input width for YUV sensor would be twice that of sensor width, as each pixel accounts for Y and U/V data
    if (TRUE == pInputData->sensorData.isYUV)
    {
        inputWidth >>= 1;
    }

    top    = pHDRBEConfig->ROI.top;
    left   = pHDRBEConfig->ROI.left;
    width  = pHDRBEConfig->ROI.width;
    height = pHDRBEConfig->ROI.height;

    // Validate ROI from Stats
    if (((left + width)              >  inputWidth)                                        ||
        ((top + height)              >  inputHeight)                                       ||
        (width                       <= 0)                                                 ||
        (height                      <= 0)                                                 ||
        (top                         <  0)                                                 ||
        (left                        <  0)                                                 ||
        (pHDRBEConfig->horizontalNum == 0)                                                 ||
        (pHDRBEConfig->verticalNum   == 0)                                                 ||
        (pHDRBEConfig->horizontalNum > static_cast<INT32>(m_hwCapability.maxHorizRegions)) ||
        (pHDRBEConfig->verticalNum   > static_cast<INT32>(m_hwCapability.maxVertRegions)))
    {
        CAMX_LOG_ERROR(CamxLogGroupISP,
                       "Invalid config: ROI %d, %d, %d, %d, horizontalNum %d, verticalNum %d input [%d %d]",
                       pHDRBEConfig->ROI.left,
                       pHDRBEConfig->ROI.top,
                       pHDRBEConfig->ROI.width,
                       pHDRBEConfig->ROI.height,
                       pHDRBEConfig->horizontalNum,
                       pHDRBEConfig->verticalNum,
                       inputWidth,
                       inputHeight);
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS();
    }

    /// @todo (CAMX-856) Validate Region skip pattern, after support from stats
    /// @todo (CAMX-887) Bandwidth check to avoid overflow

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBEStats15::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HDRBEStats15::Execute(
    ISPInputData* pInputData)
{
    CamxResult  result          = CamxResultSuccess;
    VOID*       pSettingData    = static_cast<VOID*>(pInputData);

    if (NULL != pInputData)
    {
        m_inputConfigData.pISPInputData = pInputData;
        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);

        if ((CamxResultSuccess == result) &&
            (TRUE              == CheckDependenceChange(pInputData)))
        {
            RunCalculation(pInputData);
            result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
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
// HDRBEStats15::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HDRBEStats15::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    // Update HDR BE config Info for internal metadata
    pInputData->pCalculatedData->metadata.HDRBEStatsConfig.HDRBEConfig  = m_HDRBEConfig;
    pInputData->pCalculatedData->metadata.HDRBEStatsConfig.isAdjusted   = m_isAdjusted;
    pInputData->pCalculatedData->metadata.HDRBEStatsConfig.regionWidth  = m_regionWidth;
    pInputData->pCalculatedData->metadata.HDRBEStatsConfig.regionHeight = m_regionHeight;

    m_pHWSetting->SetupRegisterSetting(&m_inputConfigData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBEStats15::HDRBEStats15
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HDRBEStats15::HDRBEStats15()
{
    m_type         = ISPStatsModuleType::IFEHDRBE;
    m_moduleEnable = TRUE;

    m_regionMultipleFactor = MultipleFactorTwo;
    m_fieldSelect          = FieldSelectAllLines;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDRBEStats15::~HDRBEStats15
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HDRBEStats15::~HDRBEStats15()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
