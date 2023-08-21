////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifetintlessbgstats15titan17x.cpp
/// @brief CAMXIFETINTLESSBGSTATS15TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifetintlessbgstats15.h"
#include "camxifetintlessbgstats15titan17x.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan17x::IFETintlessBGStats15Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFETintlessBGStats15Titan17x::IFETintlessBGStats15Titan17x()
{
    const UINT32 totalCommandLength =
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFETintlessBG15RegionConfig) / RegisterWidthInBytes)         +
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFE_IFE_0_VFE_STATS_AEC_BG_CH_Y_CFG) / RegisterWidthInBytes) +
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFE_IFE_0_VFE_STATS_AEC_BG_CFG) / RegisterWidthInBytes);

    SetCommandLength(totalCommandLength);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFETintlessBGStats15Titan17x::CreateCmdList(
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
                                          regIFE_IFE_0_VFE_STATS_AEC_BG_RGN_OFFSET_CFG,
                                          (sizeof(IFETintlessBG15RegionConfig) / RegisterWidthInBytes),
                                          reinterpret_cast<UINT32*>(&m_regCmd.regionConfig));

    if (CamxResultSuccess == result)
    {
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_STATS_AEC_BG_CH_Y_CFG,
                                              (sizeof(IFE_IFE_0_VFE_STATS_AEC_BG_CH_Y_CFG) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.lumaConfig));
    }

    if (CamxResultSuccess == result)
    {
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_STATS_AEC_BG_CFG,
                                              (sizeof(IFE_IFE_0_VFE_STATS_AEC_BG_CFG) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.tintlessBGStatsconfig));
    }

    if (CamxResultSuccess != result)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Failed to write command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFETintlessBGStats15Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pOutput);

    const TintlessBG15ConfigData*           pConfigData             = static_cast<TintlessBG15ConfigData*>(pInput);
    IFETintlessBG15RegionConfig*            pRegionConfig           = &m_regCmd.regionConfig;
    IFE_IFE_0_VFE_STATS_AEC_BG_CH_Y_CFG*    pLumaConfig             = &m_regCmd.lumaConfig;
    IFE_IFE_0_VFE_STATS_AEC_BG_CFG*         pTintlessBGStatsConfig  = &m_regCmd.tintlessBGStatsconfig;

    // The value offset must be even to ensure correct Bayer pattern.
    pRegionConfig->regionOffset.bitfields.RGN_H_OFFSET =
        Utils::FloorUINT32(2, pConfigData->pTintlessBGConfig->tintlessBGConfig.ROI.left);
    pRegionConfig->regionOffset.bitfields.RGN_V_OFFSET =
        Utils::FloorUINT32(2, pConfigData->pTintlessBGConfig->tintlessBGConfig.ROI.top);

    // The value is programmed starting from 0. Program a value of n means (n + 1)
    pRegionConfig->regionNumber.bitfields.RGN_H_NUM     = pConfigData->pTintlessBGConfig->tintlessBGConfig.horizontalNum - 1;
    pRegionConfig->regionNumber.bitfields.RGN_V_NUM     = pConfigData->pTintlessBGConfig->tintlessBGConfig.verticalNum - 1;
    pRegionConfig->regionSize.bitfields.RGN_WIDTH       = pConfigData->regionWidth - 1;
    pRegionConfig->regionSize.bitfields.RGN_HEIGHT      = pConfigData->regionHeight - 1;
    pRegionConfig->highThreshold0.bitfields.R_MAX       =
        pConfigData->pTintlessBGConfig->tintlessBGConfig.channelGainThreshold[ChannelIndexR];
    pRegionConfig->highThreshold0.bitfields.GR_MAX      =
        pConfigData->pTintlessBGConfig->tintlessBGConfig.channelGainThreshold[ChannelIndexGR];
    pRegionConfig->highThreshold1.bitfields.B_MAX       =
        pConfigData->pTintlessBGConfig->tintlessBGConfig.channelGainThreshold[ChannelIndexB];
    pRegionConfig->highThreshold1.bitfields.GB_MAX      =
        pConfigData->pTintlessBGConfig->tintlessBGConfig.channelGainThreshold[ChannelIndexGB];
    pRegionConfig->lowThreshold0.bitfields.R_MIN        = 0;
    pRegionConfig->lowThreshold0.bitfields.GR_MIN       = 0;
    pRegionConfig->lowThreshold1.bitfields.B_MIN        = 0;
    pRegionConfig->lowThreshold1.bitfields.GB_MIN       = 0;

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

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFETintlessBGStats15Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFETintlessBGStats15Titan17x::CopyRegCmd(
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
// IFETintlessBGStats15Titan17x::~IFETintlessBGStats15Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFETintlessBGStats15Titan17x::~IFETintlessBGStats15Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFETintlessBGStats15Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFETintlessBGStats15Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Offset Config  [0x%x]", m_regCmd.regionConfig.regionOffset);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Number Config  [0x%x]", m_regCmd.regionConfig.regionNumber);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Size Config    [0x%x]", m_regCmd.regionConfig.regionSize);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region highThreshold0 [0x%x]", m_regCmd.regionConfig.highThreshold0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region highThreshold1 [0x%x]", m_regCmd.regionConfig.highThreshold1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region lowThreshold0  [0x%x]", m_regCmd.regionConfig.lowThreshold0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region lowThreshold1  [0x%x]", m_regCmd.regionConfig.lowThreshold1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Luma Config           [0x%x]", m_regCmd.lumaConfig);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "tintlessBGStatsconfig [0x%x]", m_regCmd.tintlessBGStatsconfig);
}

CAMX_NAMESPACE_END
