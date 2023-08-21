////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifetintlessbgstats15titan480.cpp
/// @brief CAMXIFETINTLESSBGSTATS15TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifetintlessbgstats15.h"
#include "camxifetintlessbgstats15titan480.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan480::IFETintlessBGStats15Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFETintlessBGStats15Titan480::IFETintlessBGStats15Titan480()
{
    const UINT32 regCmdLengthInBytes[] =
    {
        sizeof(IFETintlessBG15Titan480RegionConfig),
        sizeof(IFETintlessBG15Titan480PixelThresholdConfig),
        sizeof(IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_CH_Y_CFG),
        sizeof(IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_MODULE_CFG)
    };

    UINT32 totalCommandLength = 0;
    for (UINT index = 0; index < CAMX_ARRAY_SIZE(regCmdLengthInBytes); index++)
    {
        totalCommandLength += PacketBuilder::RequiredWriteRegRangeSizeInDwords(regCmdLengthInBytes[index] /
                                                                               RegisterWidthInBytes);
    }

    SetCommandLength(totalCommandLength);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFETintlessBGStats15Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CAMX_ASSERT_MESSAGE(NULL != pInputData->pCmdBuffer, "tintlessbgstats invalid cmd buffer pointer");

    pCmdBuffer = pInputData->pCmdBuffer;

    result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                          regIFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_HORZ_RGN_CFG_0,
                                          (sizeof(IFETintlessBG15Titan480RegionConfig) / RegisterWidthInBytes),
                                          reinterpret_cast<UINT32*>(&m_regCmd.regionConfig));

    if (CamxResultSuccess == result)
    {
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_HI_THRESHOLD_CFG_0,
                                              (sizeof(IFETintlessBG15Titan480PixelThresholdConfig) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.pixelThresholdConfig));
    }

    if (CamxResultSuccess == result)
    {
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_CH_Y_CFG,
                                              (sizeof(IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_CH_Y_CFG) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.lumaConfig));
    }

    if (CamxResultSuccess == result)
    {
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_MODULE_CFG,
                                              (sizeof(IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_MODULE_CFG) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.tintlessBGStatsconfig));
    }

    if (CamxResultSuccess != result)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Failed to write command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFETintlessBGStats15Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pOutput);

    const TintlessBG15ConfigData*                   pConfigData             = static_cast<TintlessBG15ConfigData*>(pInput);
    IFETintlessBG15Titan480RegionConfig*            pRegionConfig           = &m_regCmd.regionConfig;
    IFETintlessBG15Titan480PixelThresholdConfig*    pPixelThresholdConfig   = &m_regCmd.pixelThresholdConfig;
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_CH_Y_CFG*    pLumaConfig             = &m_regCmd.lumaConfig;
    IFE_IFE_0_PP_CLC_STATS_TINTLESS_BG_MODULE_CFG*  pTintlessBGStatsConfig  = &m_regCmd.tintlessBGStatsconfig;

    // The value offset must be even to ensure correct Bayer pattern.
    pRegionConfig->horizontalRegionConfig0.bitfields.RGN_H_OFFSET =
        Utils::FloorUINT32(2, pConfigData->pTintlessBGConfig->tintlessBGConfig.ROI.left);
    pRegionConfig->verticalRegionConfig0.bitfields.RGN_V_OFFSET =
        Utils::FloorUINT32(2, pConfigData->pTintlessBGConfig->tintlessBGConfig.ROI.top);

    // The value is programmed starting from 0. Program a value of n means (n + 1)
    pRegionConfig->horizontalRegionConfig0.bitfields.RGN_H_NUM  =
        pConfigData->pTintlessBGConfig->tintlessBGConfig.horizontalNum - 1;
    pRegionConfig->verticalRegionConfig0.bitfields.RGN_V_NUM    =
        pConfigData->pTintlessBGConfig->tintlessBGConfig.verticalNum - 1;
    pRegionConfig->horizontalRegionConfig1.bitfields.RGN_WIDTH  = pConfigData->regionWidth - 1;
    pRegionConfig->verticalRegionConfig1.bitfields.RGN_HEIGHT   = pConfigData->regionHeight - 1;

    pPixelThresholdConfig->highThreshold0.bitfields.R_MAX   =
        pConfigData->pTintlessBGConfig->tintlessBGConfig.channelGainThreshold[ChannelIndexR];
    pPixelThresholdConfig->highThreshold0.bitfields.GR_MAX  =
        pConfigData->pTintlessBGConfig->tintlessBGConfig.channelGainThreshold[ChannelIndexGR];
    pPixelThresholdConfig->highThreshold1.bitfields.B_MAX   =
        pConfigData->pTintlessBGConfig->tintlessBGConfig.channelGainThreshold[ChannelIndexB];
    pPixelThresholdConfig->highThreshold1.bitfields.GB_MAX  =
        pConfigData->pTintlessBGConfig->tintlessBGConfig.channelGainThreshold[ChannelIndexGB];
    pPixelThresholdConfig->lowThreshold0.bitfields.R_MIN    = 0;
    pPixelThresholdConfig->lowThreshold0.bitfields.GR_MIN   = 0;
    pPixelThresholdConfig->lowThreshold1.bitfields.B_MIN    = 0;
    pPixelThresholdConfig->lowThreshold1.bitfields.GB_MIN   = 0;

    /// @todo (CAMX-856) Update luma config support after support from stats
    pLumaConfig->bitfields.COEF_A0      = 0;
    pLumaConfig->bitfields.COEF_A1      = 0;
    pLumaConfig->bitfields.COEF_A2      = 0;
    pLumaConfig->bitfields.G_SEL        = 0;
    pLumaConfig->bitfields.Y_STATS_EN   = 0;

    /// @todo (CAMX-856) Update TintlessBG power optimization config after support from stats
    pTintlessBGStatsConfig->bitfields.RGN_SAMPLE_PATTERN    = 0xFFFF;
    pTintlessBGStatsConfig->bitfields.SAT_STATS_EN          = 0;
    pTintlessBGStatsConfig->bitfields.SHIFT_BITS            = 0;
    pTintlessBGStatsConfig->bitfields.EN                    = 1; // Ensure to enable this module

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFETintlessBGStats15Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFETintlessBGStats15Titan480::CopyRegCmd(
    VOID* pData)
{
    UINT32 dataCopied = 0;

    if (NULL != pData)
    {
        Utils::Memcpy(pData, &m_regCmd, sizeof(m_regCmd));
        dataCopied = sizeof(m_regCmd);
    }

    return dataCopied;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan480::~IFETintlessBGStats15Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFETintlessBGStats15Titan480::~IFETintlessBGStats15Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFETintlessBGStats15Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Horizontal Region Config0  [0x%x]", m_regCmd.regionConfig.horizontalRegionConfig0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Horizontal Region Config1  [0x%x]", m_regCmd.regionConfig.horizontalRegionConfig1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Vertical Region Config0    [0x%x]", m_regCmd.regionConfig.verticalRegionConfig0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Vertical Region Config1    [0x%x]", m_regCmd.regionConfig.verticalRegionConfig1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pixel highThreshold0       [0x%x]", m_regCmd.pixelThresholdConfig.highThreshold0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pixel highThreshold1       [0x%x]", m_regCmd.pixelThresholdConfig.highThreshold1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pixel lowThreshold0        [0x%x]", m_regCmd.pixelThresholdConfig.lowThreshold0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pixel lowThreshold1        [0x%x]", m_regCmd.pixelThresholdConfig.lowThreshold1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Luma Config                [0x%x]", m_regCmd.lumaConfig);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "tintlessBGStatsconfig      [0x%x]", m_regCmd.tintlessBGStatsconfig);
}

CAMX_NAMESPACE_END
