////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeawbbgstats14titan17x.cpp
/// @brief CAMXIFEAWBBGSTATS14TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifeawbbgstats14titan17x.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"
#include "camxifeawbbgstats14.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEAWBBGStats14Titan17x::IFEAWBBGStats14Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEAWBBGStats14Titan17x::IFEAWBBGStats14Titan17x()
{
    const UINT32 regCmdLengthInBytes[] =
    {
        sizeof(IFEAWBBG14RegionConfig),
        sizeof(IFE_IFE_0_VFE_STATS_AWB_BG_CH_Y_CFG),
        sizeof(IFE_IFE_0_VFE_STATS_AWB_BG_CFG)
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
// IFEAWBBGStats14Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEAWBBGStats14Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CAMX_ASSERT_MESSAGE(NULL != pInputData->pCmdBuffer, "awgbg invalid cmd buffer pointer");

    if (NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_STATS_AWB_BG_RGN_OFFSET_CFG,
                                              (sizeof(IFEAWBBG14RegionConfig) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.regionConfig));

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_AWB_BG_CH_Y_CFG,
                                                  (sizeof(IFE_IFE_0_VFE_STATS_AWB_BG_CH_Y_CFG) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.lumaConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_AWB_BG_CFG,
                                                  (sizeof(IFE_IFE_0_VFE_STATS_AWB_BG_CFG) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.AWBBGStatsconfig));
        }

        if (CamxResultSuccess != result)
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEAWBBGStats14Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEAWBBGStats14Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pOutput);

    const AWBBG14ConfigData*                pConfigData         = static_cast<AWBBG14ConfigData*>(pInput);
    IFEAWBBG14RegionConfig*                 pRegionConfig       = &m_regCmd.regionConfig;
    IFE_IFE_0_VFE_STATS_AWB_BG_CH_Y_CFG*    pLumaConfig         = &m_regCmd.lumaConfig;
    IFE_IFE_0_VFE_STATS_AWB_BG_CFG*         pAWBBGStatsCOnfig   = &m_regCmd.AWBBGStatsconfig;

    // The value offset must be even to ensure correct Bayer pattern.
    pRegionConfig->regionOffset.bitfields.RGN_H_OFFSET = Utils::FloorUINT32(2, pConfigData->pAWBBGConfig->ROI.left);
    pRegionConfig->regionOffset.bitfields.RGN_V_OFFSET = Utils::FloorUINT32(2, pConfigData->pAWBBGConfig->ROI.top);

    // The value is programmed starting from 0. Program a value of n means (n + 1)
    pRegionConfig->regionNumber.bitfields.RGN_H_NUM     = pConfigData->pAWBBGConfig->horizontalNum - 1;
    pRegionConfig->regionNumber.bitfields.RGN_V_NUM     = pConfigData->pAWBBGConfig->verticalNum - 1;
    pRegionConfig->regionSize.bitfields.RGN_WIDTH       = pConfigData->regionWidth - 1;
    pRegionConfig->regionSize.bitfields.RGN_HEIGHT      = pConfigData->regionHeight - 1;
    pRegionConfig->highThreshold0.bitfields.R_MAX       = pConfigData->pAWBBGConfig->channelGainThreshold[ChannelIndexR];
    pRegionConfig->highThreshold0.bitfields.GR_MAX      = pConfigData->pAWBBGConfig->channelGainThreshold[ChannelIndexGR];
    pRegionConfig->highThreshold1.bitfields.B_MAX       = pConfigData->pAWBBGConfig->channelGainThreshold[ChannelIndexB];
    pRegionConfig->highThreshold1.bitfields.GB_MAX      = pConfigData->pAWBBGConfig->channelGainThreshold[ChannelIndexGB];
    pRegionConfig->lowThreshold0.bitfields.R_MIN        = 0;
    pRegionConfig->lowThreshold0.bitfields.GR_MIN       = 0;
    pRegionConfig->lowThreshold1.bitfields.B_MIN        = 0;
    pRegionConfig->lowThreshold1.bitfields.GB_MIN       = 0;

    /// @todo (CAMX-856) Update luma config support after support from stats
    pLumaConfig->bitfields.COEF_A0      = Utils::FloatToQNumber(pConfigData->pAWBBGConfig->YStatsWeights[0], Q7);
    pLumaConfig->bitfields.COEF_A1      = Utils::FloatToQNumber(pConfigData->pAWBBGConfig->YStatsWeights[1], Q7);
    pLumaConfig->bitfields.COEF_A2      = Utils::FloatToQNumber(pConfigData->pAWBBGConfig->YStatsWeights[2], Q7);
    pLumaConfig->bitfields.G_SEL        = pConfigData->pAWBBGConfig->greenType;
    pLumaConfig->bitfields.Y_STATS_EN   = (BGBEYStatsEnabled == pConfigData->pAWBBGConfig->outputMode) ? 1 : 0;

    /// @todo (CAMX-856) Update AWBBG power optimization config after support from stats
    pAWBBGStatsCOnfig->bitfields.RGN_SAMPLE_PATTERN      = 0xFFFF;
    pAWBBGStatsCOnfig->bitfields.SAT_STATS_EN            =
        (BGBESaturationEnabled == pConfigData->pAWBBGConfig->outputMode) ? 1 : 0;
    pAWBBGStatsCOnfig->bitfields.SHIFT_BITS              = 0;


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEAWBBGStats14Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEAWBBGStats14Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEAWBBGStats14Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEAWBBGStats14Titan17x::CopyRegCmd(
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
// IFEAWBBGStats14Titan17x::~IFEAWBBGStats14Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEAWBBGStats14Titan17x::~IFEAWBBGStats14Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEAWBBGStats14Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEAWBBGStats14Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Offset Config  [0x%x]", m_regCmd.regionConfig.regionOffset);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Number Config  [0x%x]", m_regCmd.regionConfig.regionNumber);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Size Config    [0x%x]", m_regCmd.regionConfig.regionSize);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region highThreshold0 [0x%x]", m_regCmd.regionConfig.highThreshold0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region highThreshold1 [0x%x]", m_regCmd.regionConfig.highThreshold1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region lowThreshold0  [0x%x]", m_regCmd.regionConfig.lowThreshold0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region lowThreshold1  [0x%x]", m_regCmd.regionConfig.lowThreshold1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Luma Config           [0x%x]", m_regCmd.lumaConfig);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "AWBBGStatsconfig      [0x%x]", m_regCmd.AWBBGStatsconfig);
}

CAMX_NAMESPACE_END
