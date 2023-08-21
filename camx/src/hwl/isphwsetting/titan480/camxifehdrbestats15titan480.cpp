////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdrbestats15titan480.cpp
/// @brief CAMXIFEHDRBESTATS15TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifehdrbestats15.h"
#include "camxifehdrbestats15titan480.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBEStats15Titan480::IFEHDRBEStats15Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDRBEStats15Titan480::IFEHDRBEStats15Titan480()
{
    const UINT32 regCmdLengthInBytes[] =
    {
        sizeof(IFE_IFE_0_PP_CLC_STATS_HDR_BE_MODULE_CFG),
        sizeof(IFE_IFE_0_PP_CLC_STATS_HDR_BE_MODULE_CFG1),
        sizeof(IFE_IFE_0_PP_CLC_STATS_HDR_BE_CH_Y_CFG),
        sizeof(IFEHDRBE15Titan480RegionConfig)
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
// IFEHDRBEStats15Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBEStats15Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CAMX_ASSERT_MESSAGE(NULL != pInputData->pCmdBuffer, "HDR BE invalid cmd buffer pointer");

    if (NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_STATS_HDR_BE_HORZ_RGN_CFG_0,
                                              (sizeof(IFEHDRBE15Titan480RegionConfig) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.regionConfig));

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_HDR_BE_CH_Y_CFG,
                                                  (sizeof(IFE_IFE_0_PP_CLC_STATS_HDR_BE_CH_Y_CFG) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.lumaConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_HDR_BE_MODULE_CFG,
                                                  (sizeof(IFE_IFE_0_PP_CLC_STATS_HDR_BE_MODULE_CFG) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.HDRBEStatsConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_HDR_BE_MODULE_CFG1,
                                                  (sizeof(IFE_IFE_0_PP_CLC_STATS_HDR_BE_MODULE_CFG1) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.HDRBEStatsConfig1));
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
// IFEHDRBEStats15Titan480::GetHWCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBEStats15Titan480::GetHWCapability(
    VOID*   pHWCapability)
{
    CamxResult                  result      = CamxResultSuccess;
    HDRBE15StatsHWCapability*   pCapability = static_cast<HDRBE15StatsHWCapability*>(pHWCapability);

    pCapability->regionMinWidth         = HDRBEStats15Titan480RegionMinWidth;
    pCapability->regionMaxWidth         = HDRBEStats15Titan480RegionMaxWidth;
    pCapability->regionMinHeight        = HDRBEStats15Titan480RegionMinHeight;
    pCapability->regionMaxHeight        = HDRBEStats15Titan480RegionMaxHeight;
    pCapability->maxHorizRegions        = HDRBEStats15Titan480MaxHorizontalregions;
    pCapability->maxVertRegions         = HDRBEStats15Titan480MaxVerticalregions;
    pCapability->maxChannelThreshold    = HDRBEStats15Titan480MaxChannelThreshold;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBEStats15Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBEStats15Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult                                  result             = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pOutput);
    const HDRBE15ConfigData*                    pConfigData        = static_cast<HDRBE15ConfigData*>(pInput);
    IFEHDRBE15Titan480RegionConfig*             pRegionConfig      = &m_regCmd.regionConfig;
    IFE_IFE_0_PP_CLC_STATS_HDR_BE_CH_Y_CFG*     pLumaConfig        = &m_regCmd.lumaConfig;
    IFE_IFE_0_PP_CLC_STATS_HDR_BE_MODULE_CFG*   pHDRBEStatsConfig  = &m_regCmd.HDRBEStatsConfig;

    // The value n must be even for Demosaic tap out and 4 for HDR recon tapout to ensure correct Bayer pattern
    pRegionConfig->horizontalRegionConfig0.bitfields.RGN_H_OFFSET =
        Utils::FloorUINT32(pConfigData->regionMultipleFactor, pConfigData->pHDRBEConfig->ROI.left);
    pRegionConfig->verticalRegionConfig0.bitfields.RGN_V_OFFSET =
        Utils::FloorUINT32(pConfigData->regionMultipleFactor, pConfigData->pHDRBEConfig->ROI.top);

    // The value is programmed starting from 0. Program a value of n means (n + 1)
    pRegionConfig->horizontalRegionConfig0.bitfields.RGN_H_NUM  = pConfigData->pHDRBEConfig->horizontalNum - 1;
    pRegionConfig->verticalRegionConfig0.bitfields.RGN_V_NUM    = pConfigData->pHDRBEConfig->verticalNum - 1;
    pRegionConfig->horizontalRegionConfig1.bitfields.RGN_WIDTH  = pConfigData->regionWidth - 1;
    pRegionConfig->verticalRegionConfig1.bitfields.RGN_HEIGHT   = pConfigData->regionHeight - 1;

    pRegionConfig->highThreshold0.bitfields.R_MAX   = pConfigData->pHDRBEConfig->channelGainThreshold[ChannelIndexR];
    pRegionConfig->highThreshold0.bitfields.GR_MAX  = pConfigData->pHDRBEConfig->channelGainThreshold[ChannelIndexGR];
    pRegionConfig->highThreshold1.bitfields.B_MAX   = pConfigData->pHDRBEConfig->channelGainThreshold[ChannelIndexB];
    pRegionConfig->highThreshold1.bitfields.GB_MAX  = pConfigData->pHDRBEConfig->channelGainThreshold[ChannelIndexGB];
    pRegionConfig->lowThreshold0.bitfields.R_MIN    = 0;
    pRegionConfig->lowThreshold0.bitfields.GR_MIN   = 0;
    pRegionConfig->lowThreshold1.bitfields.B_MIN    = 0;
    pRegionConfig->lowThreshold1.bitfields.GB_MIN   = 0;

    pLumaConfig->bitfields.COEF_A0     = Utils::FloatToQNumber(pConfigData->pHDRBEConfig->YStatsWeights[0], Q7);
    pLumaConfig->bitfields.COEF_A1     = Utils::FloatToQNumber(pConfigData->pHDRBEConfig->YStatsWeights[1], Q7);
    pLumaConfig->bitfields.COEF_A2     = Utils::FloatToQNumber(pConfigData->pHDRBEConfig->YStatsWeights[2], Q7);
    pLumaConfig->bitfields.G_SEL       = pConfigData->pHDRBEConfig->greenType;
    pLumaConfig->bitfields.Y_STATS_EN  = (BGBEYStatsEnabled == pConfigData->pHDRBEConfig->outputMode) ? 1 : 0;

    /// @todo (CAMX-856) Update HDRBE power optimization config after support from stats
    pHDRBEStatsConfig->bitfields.RGN_SAMPLE_PATTERN    = 0xFFFF;
    pHDRBEStatsConfig->bitfields.SAT_STATS_EN          =
        (BGBESaturationEnabled == pConfigData->pHDRBEConfig->outputMode) ? 1 : 0;;
    pHDRBEStatsConfig->bitfields.SHIFT_BITS            = 0;
    pHDRBEStatsConfig->bitfields.EN                    = 1; // Ensure to enable this module

    // Additional regsiter settings
    m_regCmd.HDRBEStatsConfig1.bitfields.FIELD_SEL  = pConfigData->fieldSelect;
    m_regCmd.HDRBEStatsConfig1.bitfields.HDR_SEL    = 0;    // Non HDR mode

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBEStats15Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBEStats15Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBEStats15Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEHDRBEStats15Titan480::CopyRegCmd(
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
// IFEHDRBEStats15Titan480::~IFEHDRBEStats15Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDRBEStats15Titan480::~IFEHDRBEStats15Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBEStats15Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEHDRBEStats15Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Horizontal Region Config0  [0x%x]", m_regCmd.regionConfig.horizontalRegionConfig0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Horizontal Region Config1  [0x%x]", m_regCmd.regionConfig.horizontalRegionConfig1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Vertical Region Config0    [0x%x]", m_regCmd.regionConfig.verticalRegionConfig0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Vertical Region Config1    [0x%x]", m_regCmd.regionConfig.verticalRegionConfig1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region highThreshold0      [0x%x]", m_regCmd.regionConfig.highThreshold0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region highThreshold1      [0x%x]", m_regCmd.regionConfig.highThreshold1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region lowThreshold0       [0x%x]", m_regCmd.regionConfig.lowThreshold0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region lowThreshold1       [0x%x]", m_regCmd.regionConfig.lowThreshold1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Luma Config                [0x%x]", m_regCmd.lumaConfig);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDRBEStatsConfig           [0x%x]", m_regCmd.HDRBEStatsConfig);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDRBEStatsConfig1          [0x%x]", m_regCmd.HDRBEStatsConfig1);
}

CAMX_NAMESPACE_END
