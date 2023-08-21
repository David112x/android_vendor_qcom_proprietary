////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdrbestats15titan17x.cpp
/// @brief CAMXIFEHDRBESTATS15TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifehdrbestats15titan17x.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"
#include "camxifehdrbestats15.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBEStats15Titan17x::IFEHDRBEStats15Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDRBEStats15Titan17x::IFEHDRBEStats15Titan17x()
{
    const UINT32 regCmdLengthInBytes[] =
    {
        sizeof(IFEHDRBE15RegionConfig),
        sizeof(IFE_IFE_0_VFE_STATS_HDR_BE_CH_Y_CFG),
        sizeof(IFE_IFE_0_VFE_STATS_HDR_BE_CFG)
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
// IFEHDRBEStats15Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBEStats15Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_STATS_HDR_BE_RGN_OFFSET_CFG,
                                              (sizeof(IFEHDRBE15RegionConfig) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.regionConfig));

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_HDR_BE_CH_Y_CFG,
                                                  (sizeof(IFE_IFE_0_VFE_STATS_HDR_BE_CH_Y_CFG) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.lumaConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_HDR_BE_CFG,
                                                  (sizeof(IFE_IFE_0_VFE_STATS_HDR_BE_CFG) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.HDRBEStatsConfig));
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
// IFEHDRBEStats15Titan17x::GetHWCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBEStats15Titan17x::GetHWCapability(
    VOID*   pHWCapability)
{
    CamxResult                  result      = CamxResultSuccess;
    HDRBE15StatsHWCapability*   pCapability = static_cast<HDRBE15StatsHWCapability*>(pHWCapability);

    pCapability->regionMinWidth         = HDRBEStats15Titan17xRegionMinWidth;
    pCapability->regionMaxWidth         = HDRBEStats15Titan17xRegionMaxWidth;
    pCapability->regionMinHeight        = HDRBEStats15Titan17xRegionMinHeight;
    pCapability->regionMaxHeight        = HDRBEStats15Titan17xRegionMaxHeight;
    pCapability->maxHorizRegions        = HDRBEStats15Titan17xMaxHorizontalregions;
    pCapability->maxVertRegions         = HDRBEStats15Titan17xMaxVerticalregions;
    pCapability->maxChannelThreshold    = HDRBEStats15Titan17xMaxChannelThreshold;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBEStats15Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBEStats15Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult                           result                     = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pOutput);

    const HDRBE15ConfigData*             pConfigData                = static_cast<HDRBE15ConfigData*>(pInput);
    IFEHDRBE15RegionConfig*              pRegionConfig              = &m_regCmd.regionConfig;
    IFE_IFE_0_VFE_STATS_HDR_BE_CH_Y_CFG* pLumaConfigBitfields       = &m_regCmd.lumaConfig;
    IFE_IFE_0_VFE_STATS_HDR_BE_CFG*      pHDRBEStatsConfigBitfields = &m_regCmd.HDRBEStatsConfig;

    // The value n must be even for Demosaic tap out and 4 for HDR recon tapout to ensure correct Bayer pattern
    pRegionConfig->regionOffset.bitfields.RGN_H_OFFSET =
        Utils::FloorUINT32(pConfigData->regionMultipleFactor, pConfigData->pHDRBEConfig->ROI.left);
    pRegionConfig->regionOffset.bitfields.RGN_V_OFFSET =
        Utils::FloorUINT32(pConfigData->regionMultipleFactor, pConfigData->pHDRBEConfig->ROI.top);

    // The value is programmed starting from 0. Program a value of n means (n + 1)
    pRegionConfig->regionNumber.bitfields.RGN_H_NUM = pConfigData->pHDRBEConfig->horizontalNum - 1;
    pRegionConfig->regionNumber.bitfields.RGN_V_NUM = pConfigData->pHDRBEConfig->verticalNum - 1;
    pRegionConfig->regionSize.bitfields.RGN_WIDTH   = pConfigData->regionWidth - 1;
    pRegionConfig->regionSize.bitfields.RGN_HEIGHT  = pConfigData->regionHeight - 1;
    pRegionConfig->highThreshold0.bitfields.R_MAX   = pConfigData->pHDRBEConfig->channelGainThreshold[ChannelIndexR];
    pRegionConfig->highThreshold0.bitfields.GR_MAX  = pConfigData->pHDRBEConfig->channelGainThreshold[ChannelIndexGR];
    pRegionConfig->highThreshold1.bitfields.B_MAX   = pConfigData->pHDRBEConfig->channelGainThreshold[ChannelIndexB];
    pRegionConfig->highThreshold1.bitfields.GB_MAX  = pConfigData->pHDRBEConfig->channelGainThreshold[ChannelIndexGB];
    pRegionConfig->lowThreshold0.bitfields.R_MIN    = 0;
    pRegionConfig->lowThreshold0.bitfields.GR_MIN   = 0;
    pRegionConfig->lowThreshold1.bitfields.B_MIN    = 0;
    pRegionConfig->lowThreshold1.bitfields.GB_MIN   = 0;

    pLumaConfigBitfields->bitfields.COEF_A0     = Utils::FloatToQNumber(pConfigData->pHDRBEConfig->YStatsWeights[0], Q7);
    pLumaConfigBitfields->bitfields.COEF_A1     = Utils::FloatToQNumber(pConfigData->pHDRBEConfig->YStatsWeights[1], Q7);
    pLumaConfigBitfields->bitfields.COEF_A2     = Utils::FloatToQNumber(pConfigData->pHDRBEConfig->YStatsWeights[2], Q7);
    pLumaConfigBitfields->bitfields.G_SEL       = pConfigData->pHDRBEConfig->greenType;
    pLumaConfigBitfields->bitfields.Y_STATS_EN  = (BGBEYStatsEnabled == pConfigData->pHDRBEConfig->outputMode) ? 1 : 0;

    /// @todo (CAMX-856) Update HDRBE power optimization config after support from stats
    pHDRBEStatsConfigBitfields->bitfields.RGN_SAMPLE_PATTERN    = 0xFFFF;
    pHDRBEStatsConfigBitfields->bitfields.SAT_STATS_EN          =
        (BGBESaturationEnabled == pConfigData->pHDRBEConfig->outputMode) ? 1 : 0;
    pHDRBEStatsConfigBitfields->bitfields.SHIFT_BITS            = 0;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBEStats15Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDRBEStats15Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    HDRBE15ConfigData* pConfigData = static_cast<HDRBE15ConfigData*>(pInput);

    if ((NULL != pConfigData) && (NULL != pConfigData->pISPInputData))
    {
        // Write to general stats configuration register
        pConfigData->pISPInputData->pCalculatedData->moduleEnable.statsModules.HDRBEFieldSelect  = pConfigData->fieldSelect;
        pConfigData->pISPInputData->pCalculatedData->moduleEnable.statsModules.HDRSatsSiteSelect =
            pConfigData->pISPInputData->statsTapOut.HDRBEStatsSrcSelection;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Missing data for configuration");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBEStats15Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEHDRBEStats15Titan17x::CopyRegCmd(
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
// IFEHDRBEStats15Titan17x::~IFEHDRBEStats15Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDRBEStats15Titan17x::~IFEHDRBEStats15Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDRBEStats15Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEHDRBEStats15Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Offset Config  [0x%x]", m_regCmd.regionConfig.regionOffset);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Number Config  [0x%x]", m_regCmd.regionConfig.regionNumber);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Size Config    [0x%x]", m_regCmd.regionConfig.regionSize);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region highThreshold0 [0x%x]", m_regCmd.regionConfig.highThreshold0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region highThreshold1 [0x%x]", m_regCmd.regionConfig.highThreshold1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region lowThreshold0  [0x%x]", m_regCmd.regionConfig.lowThreshold0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region lowThreshold1  [0x%x]", m_regCmd.regionConfig.lowThreshold1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Luma Config           [0x%x]", m_regCmd.lumaConfig);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDRBEStatsConfig      [0x%x]", m_regCmd.HDRBEStatsConfig);
}

CAMX_NAMESPACE_END
