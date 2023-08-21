////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifetintlessbgstats15.cpp
/// @brief Tintless BG Stats v1.5 Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifetintlessbgstats15.h"
#include "camxifetintlessbgstats15titan17x.h"
#include "camxifetintlessbgstats15titan480.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TintlessBGStats15::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TintlessBGStats15::Create(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        TintlessBGStats15* pModule = CAMX_NEW TintlessBGStats15;
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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create IFETINTLESSBGStats object.");
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
// TintlessBGStats15::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TintlessBGStats15::Initialize(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFETintlessBGStats15Titan480;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFETintlessBGStats15Titan17x;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_cmdLength = m_pHWSetting->GetCommandLength();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, no memory");
        result = CamxResultENoMemory;
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TintlessBGStats15::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TintlessBGStats15::CheckDependenceChange(
    const ISPInputData* pInputData)
{
    BOOL        result            = FALSE;
    BGBEConfig* pTintlessBGConfig = &pInputData->pStripeConfig->AECStatsUpdateData.statsConfig.TintlessBGConfig;


    if ((m_tintlessBGConfig.tintlessBGConfig.horizontalNum != pTintlessBGConfig->horizontalNum)                        ||
        (m_tintlessBGConfig.tintlessBGConfig.verticalNum   != pTintlessBGConfig->verticalNum)                          ||
        (m_tintlessBGConfig.tintlessBGConfig.ROI.left      != pTintlessBGConfig->ROI.left)                             ||
        (m_tintlessBGConfig.tintlessBGConfig.ROI.top       != pTintlessBGConfig->ROI.top)                              ||
        (m_tintlessBGConfig.tintlessBGConfig.ROI.width     != pTintlessBGConfig->ROI.width)                            ||
        (m_tintlessBGConfig.tintlessBGConfig.ROI.height    != pTintlessBGConfig->ROI.height)                           ||
        (m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexR]
                                                           != pTintlessBGConfig->channelGainThreshold[ChannelIndexR])  ||
        (m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexGR]
                                                           != pTintlessBGConfig->channelGainThreshold[ChannelIndexGR]) ||
        (m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexB]
                                                           != pTintlessBGConfig->channelGainThreshold[ChannelIndexB])  ||
        (m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexGB]
                                                           != pTintlessBGConfig->channelGainThreshold[ChannelIndexGB]) ||
        (TRUE                                              == pInputData->forceTriggerUpdate))
    {
        m_tintlessBGConfig.tintlessBGConfig.horizontalNum = pTintlessBGConfig->horizontalNum;
        m_tintlessBGConfig.tintlessBGConfig.verticalNum   = pTintlessBGConfig->verticalNum;
        m_tintlessBGConfig.tintlessBGConfig.ROI.left      = pTintlessBGConfig->ROI.left;
        m_tintlessBGConfig.tintlessBGConfig.ROI.top       = pTintlessBGConfig->ROI.top;
        m_tintlessBGConfig.tintlessBGConfig.ROI.height    = pTintlessBGConfig->ROI.height;
        m_tintlessBGConfig.tintlessBGConfig.ROI.width     = pTintlessBGConfig->ROI.width;
        m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexR]  =
            pTintlessBGConfig->channelGainThreshold[ChannelIndexR];
        m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexGR] =
            pTintlessBGConfig->channelGainThreshold[ChannelIndexGR];
        m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexB]  =
            pTintlessBGConfig->channelGainThreshold[ChannelIndexB];
        m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexGB] =
            pTintlessBGConfig->channelGainThreshold[ChannelIndexGB];

        result = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TintlessBGStats15::AdjustROIParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TintlessBGStats15::AdjustROIParams(
    UINT32* pRegionWidth,
    UINT32* pRegionHeight)
{
    UINT32 regionWidth;
    UINT32 regionHeight;

    CAMX_ASSERT_MESSAGE(NULL != pRegionWidth, "Invalid input pointer");
    CAMX_ASSERT_MESSAGE(NULL != pRegionHeight, "Invalid input pointer");

    regionWidth  = (m_tintlessBGConfig.tintlessBGConfig.ROI.width / m_tintlessBGConfig.tintlessBGConfig.horizontalNum);
    regionHeight = (m_tintlessBGConfig.tintlessBGConfig.ROI.height / m_tintlessBGConfig.tintlessBGConfig.verticalNum);

    m_tintlessBGConfig.isAdjusted                      = FALSE;
    m_tintlessBGConfig.tintlessBGConfig.outputBitDepth = IFEPipelineBitWidth;

    // The value n must be even to ensure correct Bayer pattern
    regionWidth  = Utils::FloorUINT32(2, regionWidth);
    regionHeight = Utils::FloorUINT32(2, regionHeight);

    // Check region minimum width criteria
    if (TintlessBGStats15RegionMinWidth > regionWidth)
    {
        regionWidth                                       = TintlessBGStats15RegionMinWidth;
        m_tintlessBGConfig.tintlessBGConfig.horizontalNum = (m_tintlessBGConfig.tintlessBGConfig.ROI.width / regionWidth);
        m_tintlessBGConfig.isAdjusted                     = TRUE;
    }
    else if (TintlessBGStats15RegionMaxWidth < regionWidth)
    {
        regionWidth                                       = TintlessBGStats15RegionMaxWidth;
        m_tintlessBGConfig.tintlessBGConfig.horizontalNum = (m_tintlessBGConfig.tintlessBGConfig.ROI.width / regionWidth);
        m_tintlessBGConfig.isAdjusted                     = TRUE;
    }
    // Check region minimum Height criteria
    if (TintlessBGStats15RegionMinHeight > regionHeight)
    {
        regionHeight                                    = TintlessBGStats15RegionMinHeight;
        m_tintlessBGConfig.tintlessBGConfig.verticalNum = (m_tintlessBGConfig.tintlessBGConfig.ROI.height / regionHeight);
        m_tintlessBGConfig.isAdjusted                   = TRUE;
    }
    else if (TintlessBGStats15RegionMaxHeight < regionHeight)
    {
        regionHeight                                    = TintlessBGStats15RegionMaxHeight;
        m_tintlessBGConfig.tintlessBGConfig.verticalNum = (m_tintlessBGConfig.tintlessBGConfig.ROI.height / regionHeight);
        m_tintlessBGConfig.isAdjusted                   = TRUE;
    }

    CAMX_ASSERT(TintlessBGStats15RegionMaxWidth       > regionWidth);
    CAMX_ASSERT(TintlessBGStats15RegionMaxHeight      > regionHeight);
    CAMX_ASSERT(TintlessBGStats15RegionMinHeight      < regionHeight);
    CAMX_ASSERT(TintlessBGStats15RegionMinWidth       < regionWidth);
    CAMX_ASSERT(TintlessBGStats15MaxVerticalregions   >= m_tintlessBGConfig.tintlessBGConfig.verticalNum);
    CAMX_ASSERT(TintlessBGStats15MaxHorizontalregions >= m_tintlessBGConfig.tintlessBGConfig.horizontalNum);
    CAMX_ASSERT(0                                     != m_tintlessBGConfig.tintlessBGConfig.verticalNum);
    CAMX_ASSERT(0                                     != m_tintlessBGConfig.tintlessBGConfig.horizontalNum);
    // Adjust Max Channel threshold values
    if (TintlessBGStats15MaxChannelThreshold < m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexR])
    {
        m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexR] = TintlessBGStats15MaxChannelThreshold;
        m_tintlessBGConfig.isAdjusted                                           = TRUE;
    }

    if (TintlessBGStats15MaxChannelThreshold < m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexB])
    {
        m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexB] = TintlessBGStats15MaxChannelThreshold;
        m_tintlessBGConfig.isAdjusted                                           = TRUE;
    }

    if (TintlessBGStats15MaxChannelThreshold < m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexGR])
    {
        m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexGR] = TintlessBGStats15MaxChannelThreshold;
        m_tintlessBGConfig.isAdjusted                                            = TRUE;
    }

    if (TintlessBGStats15MaxChannelThreshold < m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexGB])
    {
        m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexGB] = TintlessBGStats15MaxChannelThreshold;
        m_tintlessBGConfig.isAdjusted                                            = TRUE;
    }

    *pRegionWidth  = regionWidth;
    *pRegionHeight = regionHeight;

    regionWidth = Utils::FloorUINT32(2, regionWidth);
    regionHeight = Utils::FloorUINT32(2, regionHeight);

    m_tintlessBGConfig.tintlessBGConfig.regionHeight = regionHeight;
    m_tintlessBGConfig.tintlessBGConfig.regionWidth  = regionWidth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TintlessBGStats15::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TintlessBGStats15::RunCalculation(
    ISPInputData* pInputData)
{
    UINT32 regionWidth;
    UINT32 regionHeight;

    AdjustROIParams(&regionWidth, &regionHeight);

    m_inputConfigData.pTintlessBGConfig = &m_tintlessBGConfig;
    m_inputConfigData.regionHeight      = regionHeight;
    m_inputConfigData.regionWidth       = regionWidth;

    m_pHWSetting->PackIQRegisterSetting(&m_inputConfigData, NULL);

    if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
    {
        m_pHWSetting->DumpRegConfig();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TintlessBGStats15::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TintlessBGStats15::UpdateIFEInternalData(
    ISPInputData* pInputData
    ) const
{
    pInputData->pCalculatedData->metadata.tintlessBGStats                        = m_tintlessBGConfig;
    pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.regionWidth    =
        m_tintlessBGConfig.tintlessBGConfig.regionWidth;
    pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.regionHeight   =
        m_tintlessBGConfig.tintlessBGConfig.regionHeight;
    pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.outputBitDepth = IFEPipelineBitWidth;

    pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.channelGainThreshold[ChannelIndexR] =
        m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexR];
    pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.channelGainThreshold[ChannelIndexB] =
        m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexB];
    pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.channelGainThreshold[ChannelIndexGR] =
        m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexGR];
    pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.channelGainThreshold[ChannelIndexGB] =
        m_tintlessBGConfig.tintlessBGConfig.channelGainThreshold[ChannelIndexGB];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TintlessBGStats15::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TintlessBGStats15::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        if (NULL != pInputData->pStripingInput)
        {
            if (pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.horizontalNum != 0 &&
                pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.verticalNum != 0)
            {
                pInputData->pStripingInput->enableBits.BGTintless = m_moduleEnable;
                pInputData->pStripingInput->stripingInput.BGTintlessEnable =
                    static_cast<int16_t>(m_moduleEnable);
                pInputData->pStripingInput->stripingInput.BGTintlessInput.BGEnabled = m_moduleEnable;
                pInputData->pStripingInput->stripingInput.BGTintlessInput.BGRgnHorizNum =
                    pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.horizontalNum - 1;
                pInputData->pStripingInput->stripingInput.BGTintlessInput.BGRgnVertNum =
                    pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.verticalNum - 1;
                pInputData->pStripingInput->stripingInput.BGTintlessInput.BGRgnWidth =
                    (pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.ROI.width /
                        pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.horizontalNum) - 1;
                pInputData->pStripingInput->stripingInput.BGTintlessInput.BGROIHorizOffset =
                    pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.ROI.left;
                pInputData->pStripingInput->stripingInput.BGTintlessInput.BGYOutputEnable =
                    (BGBEYStatsEnabled == pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.outputMode) ? 1 : 0;
                pInputData->pStripingInput->stripingInput.BGTintlessInput.BGSatOutputEnable =
                    (BGBESaturationEnabled ==
                        pInputData->pAECStatsUpdateData->statsConfig.TintlessBGConfig.outputMode) ? 1 : 0;
                pInputData->pStripingInput->stripingInput.BGTintlessInput.BGRegionSampling = 0xFFFF;
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
// TintlessBGStats15::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TintlessBGStats15::ValidateDependenceParams(
    ISPInputData* pInputData
    ) const
{
    UINT32      inputWidth;
    UINT32      inputHeight;
    CropInfo*   pSensorCAMIFCrop  = NULL;
    BGBEConfig* pTintlessBGConfig = NULL;
    CamxResult  result            = CamxResultSuccess;

    CAMX_ASSERT(NULL != pInputData);
    CAMX_ASSERT(NULL != pInputData->pStripeConfig);

    pSensorCAMIFCrop  = &pInputData->pStripeConfig->CAMIFCrop;
    pTintlessBGConfig = &pInputData->pStripeConfig->AECStatsUpdateData.statsConfig.TintlessBGConfig;

    inputWidth  = (pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1);
    inputHeight = (pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1);

    // CAMIF input width for YUV sensor would be twice that of sensor width, as each pixel accounts for Y and U/V data
    if (TRUE == pInputData->sensorData.isYUV)
    {
        inputWidth >>= 1;
    }

    // Validate ROI from Stats
    if (((pTintlessBGConfig->ROI.left + pTintlessBGConfig->ROI.width) > inputWidth)                        ||
        ((pTintlessBGConfig->ROI.top + pTintlessBGConfig->ROI.height) > inputHeight)                       ||
        (0                                                            == pTintlessBGConfig->ROI.width)     ||
        (0                                                            == pTintlessBGConfig->ROI.height)    ||
        (0                                                            == pTintlessBGConfig->horizontalNum) ||
        (0                                                            == pTintlessBGConfig->verticalNum)   ||
        (TintlessBGStats15MaxHorizontalregions                        <  pTintlessBGConfig->horizontalNum) ||
        (TintlessBGStats15MaxVerticalregions                          <  pTintlessBGConfig->verticalNum))
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "ROI %d, %d, %d, %d, horizontalNum %d, verticalNum %d",
                       pTintlessBGConfig->ROI.left,
                       pTintlessBGConfig->ROI.top,
                       pTintlessBGConfig->ROI.width,
                       pTintlessBGConfig->ROI.height,
                       pTintlessBGConfig->horizontalNum,
                       pTintlessBGConfig->verticalNum);
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS();
    }

    /// @todo (CAMX-856) Validate Region skip pattern, after support from stats

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TintlessBGStats15::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult TintlessBGStats15::Execute(
    ISPInputData* pInputData)
{
    CamxResult result       = CamxResultSuccess;
    VOID*      pSettingData = static_cast<VOID*>(pInputData);

    if (NULL != pInputData)
    {
        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);
        if (CamxResultSuccess == result)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                RunCalculation(pInputData);
                CAMX_ASSERT(NULL != pInputData->pCmdBuffer);
                result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
            }
        }
        if (CamxResultSuccess == result)
        {
            UpdateIFEInternalData(pInputData);
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
// TintlessBGStats15::TintlessBGStats15
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TintlessBGStats15::TintlessBGStats15()
{
    m_type         = ISPStatsModuleType::IFETintlessBG;
    m_moduleEnable = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TintlessBGStats15::~TintlessBGStats15
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TintlessBGStats15::~TintlessBGStats15()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
