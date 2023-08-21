////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsawbbgstats14.cpp
/// @brief BPS AWBBG Stats14 Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpsawbbgstats14.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSAWBBGStats14::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSAWBBGStats14::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        pCreateData->pModule = CAMX_NEW BPSAWBBGStats14;
        if (NULL == pCreateData->pModule)
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Memory allocation failed");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input Null pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSAWBBGStats14::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSAWBBGStats14::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL        result          = FALSE;
    BGBEConfig* pAWBBGConfig    = &pInputData->pAWBStatsUpdateData->statsConfig.BGConfig;

    if ((m_AWBBGConfig.horizontalNum                        != pAWBBGConfig->horizontalNum)                         ||
        (m_AWBBGConfig.verticalNum                          != pAWBBGConfig->verticalNum)                           ||
        (m_AWBBGConfig.ROI.left                             != pAWBBGConfig->ROI.left)                              ||
        (m_AWBBGConfig.ROI.top                              != pAWBBGConfig->ROI.top)                               ||
        (m_AWBBGConfig.ROI.width                            != pAWBBGConfig->ROI.width)                             ||
        (m_AWBBGConfig.ROI.height                           != pAWBBGConfig->ROI.height)                            ||
        (m_AWBBGConfig.channelGainThreshold[ChannelIndexR]  != pAWBBGConfig->channelGainThreshold[ChannelIndexR])   ||
        (m_AWBBGConfig.channelGainThreshold[ChannelIndexGR] != pAWBBGConfig->channelGainThreshold[ChannelIndexGR])  ||
        (m_AWBBGConfig.channelGainThreshold[ChannelIndexB]  != pAWBBGConfig->channelGainThreshold[ChannelIndexB])   ||
        (m_AWBBGConfig.channelGainThreshold[ChannelIndexGB] != pAWBBGConfig->channelGainThreshold[ChannelIndexGB])  ||
        (m_AWBBGConfig.outputBitDepth                       != pAWBBGConfig->outputBitDepth)                        ||
        (m_AWBBGConfig.outputMode                           != pAWBBGConfig->outputMode)                            ||
        (m_AWBBGConfig.YStatsWeights[0]                     != pAWBBGConfig->YStatsWeights[0])                      ||
        (m_AWBBGConfig.YStatsWeights[1]                     != pAWBBGConfig->YStatsWeights[1])                      ||
        (m_AWBBGConfig.YStatsWeights[2]                     != pAWBBGConfig->YStatsWeights[2])                      ||
        (m_AWBBGConfig.greenType                            != pAWBBGConfig->greenType)                             ||
        (m_AWBBGConfig.enableQuadSync                       != pAWBBGConfig->enableQuadSync))
    {
        Utils::Memcpy(&m_AWBBGConfig, pAWBBGConfig, sizeof(BGBEConfig));
        result = TRUE;

        CAMX_ASSERT_MESSAGE(MaxAWBBGStatsNum >= (m_AWBBGConfig.horizontalNum * m_AWBBGConfig.verticalNum),
                            "AWBBGStatsNum out of bound");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSAWBBGStats14::AdjustROIParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSAWBBGStats14::AdjustROIParams()
{
    UINT32 regionWidth;
    UINT32 regionHeight;
    BOOL   isAdjusted = FALSE;

    CAMX_ASSERT_MESSAGE(0 != m_AWBBGConfig.horizontalNum, "Invalid horizontalNum");
    CAMX_ASSERT_MESSAGE(0 != m_AWBBGConfig.verticalNum, "Invalid verticalNum");

    CAMX_ASSERT_MESSAGE(0 != m_AWBBGConfig.ROI.width, "Invalid ROI Width");
    CAMX_ASSERT_MESSAGE(0 != m_AWBBGConfig.ROI.height, "Invalid ROI height");

    m_AWBBGConfig.horizontalNum = Utils::FloorUINT32(2, m_AWBBGConfig.horizontalNum);
    m_AWBBGConfig.verticalNum   = Utils::FloorUINT32(2, m_AWBBGConfig.verticalNum);

    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "hNum = %d, vNum = %d, ROI.w = %d, ROI.h = %d",
        m_AWBBGConfig.horizontalNum, m_AWBBGConfig.verticalNum,
        m_AWBBGConfig.ROI.width, m_AWBBGConfig.ROI.height);

    regionWidth  = m_AWBBGConfig.ROI.width / m_AWBBGConfig.horizontalNum;
    regionHeight = m_AWBBGConfig.ROI.height / m_AWBBGConfig.verticalNum;

    // The value n must be even to ensure correct Bayer pattern
    regionWidth  = Utils::FloorUINT32(2, regionWidth);
    regionHeight = Utils::FloorUINT32(2, regionHeight);

    // Check region minimum and maximum width criteria
    if (regionWidth < BPSAWBBGStats14RegionMinWidth)
    {
        regionWidth                 = BPSAWBBGStats14RegionMinWidth;
        m_AWBBGConfig.horizontalNum = Utils::FloorUINT32(2, (m_AWBBGConfig.ROI.width / regionWidth));
        isAdjusted                  = TRUE;
    }
    else if (regionWidth > BPSAWBBGStats14RegionMaxWidth)
    {
        regionWidth                 = BPSAWBBGStats14RegionMaxWidth;
        m_AWBBGConfig.horizontalNum = Utils::FloorUINT32(2, (m_AWBBGConfig.ROI.width / regionWidth));
        isAdjusted                  = TRUE;
    }

    // Check region minimum and maximum Height criteria
    if (regionHeight < BPSAWBBGStats14RegionMinHeight)
    {
        regionHeight                = BPSAWBBGStats14RegionMinHeight;
        m_AWBBGConfig.verticalNum   = Utils::FloorUINT32(2, (m_AWBBGConfig.ROI.height / regionHeight));
        isAdjusted                  = TRUE;
    }
    else if (regionHeight > BPSAWBBGStats14RegionMaxHeight)
    {
        regionHeight                = BPSAWBBGStats14RegionMaxHeight;
        m_AWBBGConfig.verticalNum   = Utils::FloorUINT32(2, (m_AWBBGConfig.ROI.height / regionHeight));
        isAdjusted                  = TRUE;
    }

    // Adjust Max Channel threshold values
    if (m_AWBBGConfig.channelGainThreshold[ChannelIndexR] > AWBBGStats14MaxChannelThreshold)
    {
        m_AWBBGConfig.channelGainThreshold[ChannelIndexR] = AWBBGStats14MaxChannelThreshold;
        isAdjusted                                        = TRUE;
    }

    if (m_AWBBGConfig.channelGainThreshold[ChannelIndexB] > AWBBGStats14MaxChannelThreshold)
    {
        m_AWBBGConfig.channelGainThreshold[ChannelIndexB] = AWBBGStats14MaxChannelThreshold;
        isAdjusted                                        = TRUE;
    }

    if (m_AWBBGConfig.channelGainThreshold[ChannelIndexGR] > AWBBGStats14MaxChannelThreshold)
    {
        m_AWBBGConfig.channelGainThreshold[ChannelIndexGR] = AWBBGStats14MaxChannelThreshold;
        isAdjusted                                         = TRUE;
    }

    if (m_AWBBGConfig.channelGainThreshold[ChannelIndexGB] > AWBBGStats14MaxChannelThreshold)
    {
        m_AWBBGConfig.channelGainThreshold[ChannelIndexGB] = AWBBGStats14MaxChannelThreshold;
        isAdjusted                                         = TRUE;
    }

    m_regionWidth  = Utils::FloorUINT32(2, regionWidth);
    m_regionHeight = Utils::FloorUINT32(2, regionHeight);
    m_isAdjusted   = isAdjusted;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSAWBBGStats14::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSAWBBGStats14::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    BGBEConfig* pAWBBGConfig     = &pInputData->pAWBStatsUpdateData->statsConfig.BGConfig;
    CamxResult  result           = CamxResultSuccess;

    // Validate ROI from Stats
    if (((pAWBBGConfig->ROI.left + pAWBBGConfig->ROI.width) > pInputData->pipelineBPSData.width)  ||
        ((pAWBBGConfig->ROI.top + pAWBBGConfig->ROI.height) > pInputData->pipelineBPSData.height) ||
        (pAWBBGConfig->ROI.width                          == 0)                                   ||
        (pAWBBGConfig->ROI.height                         == 0)                                   ||
        (pAWBBGConfig->horizontalNum                      == 0)                                   ||
        (pAWBBGConfig->verticalNum                        == 0)                                   ||
        (pAWBBGConfig->horizontalNum                      > BPSAWBBGStats14MaxHorizontalRegions)  ||
        (pAWBBGConfig->verticalNum                        > BPSAWBBGStats14MaxVerticalRegions))
    {
        CAMX_LOG_ERROR(CamxLogGroupISP,
                       "ROI %d, %d, %d, %d, horizontalNum %d, verticalNum %d",
                       pAWBBGConfig->ROI.left,
                       pAWBBGConfig->ROI.top,
                       pAWBBGConfig->ROI.width,
                       pAWBBGConfig->ROI.height,
                       pAWBBGConfig->horizontalNum,
                       pAWBBGConfig->verticalNum);
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS();
    }

    /// @todo (CAMX-856) Validate Region skip pattern, after support from stats

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSAWBBGStats14::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSAWBBGStats14::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        // Run through the settings only when module is enabled
        if (TRUE == pInputData->pipelineBPSData.pBPSPathEnabled[BPSOutputPortStatsBG])
        {
            // Check if dependent is valid and been updated compared to last request
            result = ValidateDependenceParams(pInputData);
            if ((CamxResultSuccess == result) &&
                (TRUE == CheckDependenceChange(pInputData)))
            {
                AdjustROIParams();
            }
        }
        if (CamxResultSuccess == result)
        {
            // Update CAMX metadata with the values for the current frame
            UpdateBPSInternalData(pInputData);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSAWBBGStats14::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSAWBBGStats14::UpdateBPSInternalData(
    ISPInputData* pInputData)
{
    BpsIQSettings*  pBPSIQSettings = reinterpret_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->bayerGridParameters.moduleCfg.EN            =
        pInputData->pipelineBPSData.pBPSPathEnabled[BPSOutputPortStatsBG];
    pBPSIQSettings->bayerGridParameters.moduleCfg.QUAD_SYNC_EN  = m_AWBBGConfig.enableQuadSync;
    pBPSIQSettings->bayerGridParameters.moduleCfg.SAT_STATS_EN  = (BGBESaturationEnabled == m_AWBBGConfig.outputMode) ? 1 : 0;
    pBPSIQSettings->bayerGridParameters.moduleCfg.SHIFT_BITS    = 0;

    pBPSIQSettings->bayerGridParameters.bgRgnWidth          = m_regionWidth - 1;
    pBPSIQSettings->bayerGridParameters.bgRgnHeight         = m_regionHeight - 1;
    pBPSIQSettings->bayerGridParameters.bgRgnVNum           = m_AWBBGConfig.verticalNum - 1;
    pBPSIQSettings->bayerGridParameters.bgRgnHNum           = m_AWBBGConfig.horizontalNum - 1;
    pBPSIQSettings->bayerGridParameters.bgRegionSampling    = 0xFFFF;
    pBPSIQSettings->bayerGridParameters.bgRoiHOffset        = Utils::FloorUINT32(2, m_AWBBGConfig.ROI.left);
    pBPSIQSettings->bayerGridParameters.bgRoiVOffset        = Utils::FloorUINT32(2, m_AWBBGConfig.ROI.top);
    pBPSIQSettings->bayerGridParameters.bgYOutputEnable     = (BGBEYStatsEnabled == m_AWBBGConfig.outputMode) ? 1 : 0;
    pBPSIQSettings->bayerGridParameters.greenChannelSelect  = m_AWBBGConfig.greenType;
    pBPSIQSettings->bayerGridParameters.a0Coefficient       = static_cast<uint32_t>(m_AWBBGConfig.YStatsWeights[0]);
    pBPSIQSettings->bayerGridParameters.a1Coefficient       = static_cast<uint32_t>(m_AWBBGConfig.YStatsWeights[1]);
    pBPSIQSettings->bayerGridParameters.a2Coefficient       = static_cast<uint32_t>(m_AWBBGConfig.YStatsWeights[2]);
    pBPSIQSettings->bayerGridParameters.rMax                = m_AWBBGConfig.channelGainThreshold[ChannelIndexR];
    pBPSIQSettings->bayerGridParameters.rMin                = 0;
    pBPSIQSettings->bayerGridParameters.grMax               = m_AWBBGConfig.channelGainThreshold[ChannelIndexGR];
    pBPSIQSettings->bayerGridParameters.grMin               = 0;
    pBPSIQSettings->bayerGridParameters.gbMax               = m_AWBBGConfig.channelGainThreshold[ChannelIndexGB];
    pBPSIQSettings->bayerGridParameters.gbMin               = 0;
    pBPSIQSettings->bayerGridParameters.bMax                = m_AWBBGConfig.channelGainThreshold[ChannelIndexB];
    pBPSIQSettings->bayerGridParameters.bMin                = 0;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "En                    %d",
                   pBPSIQSettings->bayerGridParameters.moduleCfg.EN);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "SAT_STATS_EN          %d",
                   pBPSIQSettings->bayerGridParameters.moduleCfg.SAT_STATS_EN);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "QUAD_SYNC_EN          %d",
                   pBPSIQSettings->bayerGridParameters.moduleCfg.QUAD_SYNC_EN);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "SHIFT_BITS          %d",
                   pBPSIQSettings->bayerGridParameters.moduleCfg.SHIFT_BITS);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bgRgnWidth            %d",
                   pBPSIQSettings->bayerGridParameters.bgRgnWidth);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bgRgnHeight           %d",
                   pBPSIQSettings->bayerGridParameters.bgRgnHeight);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bgRgnVNum             %d",
                   pBPSIQSettings->bayerGridParameters.bgRgnVNum);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bgRgnHNum             %d",
                   pBPSIQSettings->bayerGridParameters.bgRgnHNum);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bgRegionSampling      %d",
                   pBPSIQSettings->bayerGridParameters.bgRegionSampling);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bgRoiHOffset          %d",
                   pBPSIQSettings->bayerGridParameters.bgRoiHOffset);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bgRoiVOffset          %d",
                   pBPSIQSettings->bayerGridParameters.bgRoiVOffset);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bgYOutputEnable       %d",
                   pBPSIQSettings->bayerGridParameters.bgYOutputEnable);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "greenChannelSelect    %d",
                   pBPSIQSettings->bayerGridParameters.greenChannelSelect);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "a0Coefficient         %d",
                   pBPSIQSettings->bayerGridParameters.a0Coefficient);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "a1Coefficient         %d",
                   pBPSIQSettings->bayerGridParameters.a1Coefficient);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "a2Coefficient         %d",
                   pBPSIQSettings->bayerGridParameters.a2Coefficient);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "rMax                  %d",
                   pBPSIQSettings->bayerGridParameters.rMax);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "rMin                  %d",
                   pBPSIQSettings->bayerGridParameters.rMin);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "grMax                 %d",
                   pBPSIQSettings->bayerGridParameters.grMax);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "grMin                 %d",
                   pBPSIQSettings->bayerGridParameters.grMin);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "gbMax                 %d",
                   pBPSIQSettings->bayerGridParameters.gbMax);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bMax                  %d",
                   pBPSIQSettings->bayerGridParameters.bMax);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bMin                  %d",
                   pBPSIQSettings->bayerGridParameters.bMin);

    // Update AWB BG config to metadata, for parser and 3A to consume
    pInputData->pCalculatedData->metadata.AWBBGStatsConfig.AWBBGConfig  = m_AWBBGConfig;
    pInputData->pCalculatedData->metadata.AWBBGStatsConfig.isAdjusted   = m_isAdjusted;
    pInputData->pCalculatedData->metadata.AWBBGStatsConfig.regionWidth  = m_regionWidth;
    pInputData->pCalculatedData->metadata.AWBBGStatsConfig.regionHeight = m_regionHeight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSAWBBGStats14::BPSAWBBGStats14
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSAWBBGStats14::BPSAWBBGStats14()
{
    m_type         = ISPIQModuleType::BPSAWBBG;
    m_moduleEnable = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSAWBBGStats14::~BPSAWBBGStats14
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSAWBBGStats14::~BPSAWBBGStats14()
{
}

CAMX_NAMESPACE_END
