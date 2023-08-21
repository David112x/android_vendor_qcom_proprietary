////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecsstats14titan480.cpp
/// @brief CAMXIFECSSTATS14TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"
#include "camxifecsstats14.h"
#include "camxifecsstats14titan480.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan480::IFECSStats14Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECSStats14Titan480::IFECSStats14Titan480()
{
    SetCommandLength(
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFECS14Titan480Config) / RegisterWidthInBytes));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSStats14Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CAMX_ASSERT_MESSAGE((NULL != pInputData->pCmdBuffer), "Invalid Input pointer!");

    pCmdBuffer = pInputData->pCmdBuffer;

    result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                          regIFE_IFE_0_PP_CLC_STATS_CS_MODULE_CFG,
                                          (sizeof(IFE_IFE_0_PP_CLC_STATS_CS_MODULE_CFG) / RegisterWidthInBytes),
                                          reinterpret_cast<UINT32*>(&m_regCmd.moduleConfig));
    if (CamxResultSuccess == result)
    {
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_STATS_CS_RGN_OFFSET_CFG,
                                              (sizeof(IFECS14Titan480RegionConfig) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.regionConfig));
    }

    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Failed to write command buffer");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan480::AdjustHorizontalRegionNumber
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFECSStats14Titan480::AdjustHorizontalRegionNumber(
    UINT32  regionHNum,
    UINT32  divider,
    UINT32  minRemainder)
{
    UINT32 remainder = regionHNum % divider;

    if ((regionHNum > minRemainder) && (remainder < minRemainder))
    {
        remainder = remainder + 1;
    }
    else
    {
        remainder = 0;
    }

    return (regionHNum - remainder);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan480::GetHWCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSStats14Titan480::GetHWCapability(
    VOID*   pHWCapability)
{
    CamxResult              result      = CamxResultSuccess;
    CS14StatsHWCapability*  pCapability = static_cast<CS14StatsHWCapability*>(pHWCapability);

    pCapability->inputDepth         = CSTitan480InputDepth;
    pCapability->outputDepth        = CSTitan480OutputDepth;
    pCapability->maxRegionWidth     = CSStats14Titan480MaxRegionWidth;
    pCapability->minRegionWidth     = CSStats14Titan480MinRegionWidth;
    pCapability->minRegionHeight    = CSStats14Titan480MinRegionHeight;
    pCapability->minHorizRegions    = CSStats14Titan480MinHorizRegions;
    pCapability->maxHorizRegions    = CSStats14Titan480MaxHorizRegions;
    pCapability->minVertRegions     = CSStats14Titan480MinVertRegions;
    pCapability->maxVertRegions     = CSStats14Titan480MaxVertRegions;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSStats14Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);
    CamxResult      result      = CamxResultSuccess;
    CS14ConfigData* pConfigData = static_cast<CS14ConfigData*>(pInput);

    if ((NULL != pConfigData) && (NULL != pConfigData->pISPInputData))
    {
        m_regCmd.moduleConfig.bitfields.EN                          = TRUE;
        m_regCmd.moduleConfig.bitfields.SHIFT_BITS                  = pConfigData->shiftBits;
        m_regCmd.regionConfig.regionOffset.bitfields.RGN_H_OFFSET   = pConfigData->pCSConfig->CSConfig.statsHOffset;
        m_regCmd.regionConfig.regionOffset.bitfields.RGN_V_OFFSET   = pConfigData->pCSConfig->CSConfig.statsVOffset;
        m_regCmd.regionConfig.regionNum.bitfields.RGN_H_NUM         = pConfigData->pCSConfig->CSConfig.statsHNum - 1;
        m_regCmd.regionConfig.regionNum.bitfields.RGN_V_NUM         = pConfigData->pCSConfig->CSConfig.statsVNum - 1;
        m_regCmd.regionConfig.regionSize.bitfields.RGN_WIDTH        = pConfigData->pCSConfig->CSConfig.statsRgnWidth - 1;
        m_regCmd.regionConfig.regionSize.bitfields.RGN_HEIGHT       = pConfigData->pCSConfig->CSConfig.statsRgnHeight - 1;

        // Also we need to adjust rgnHNum to satisfy the hardware limitation
        // regionHNum when divided by 8 needs to have remainder [4, 7]
        m_regCmd.regionConfig.regionNum.bitfields.RGN_H_NUM =
            AdjustHorizontalRegionNumber(m_regCmd.regionConfig.regionNum.bitfields.RGN_H_NUM, 8, 4);

        if (CamxLogGroupIQMod == (pConfigData->pISPInputData->dumpRegConfig & CamxLogGroupIQMod))
        {
            DumpRegConfig();
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Input");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSStats14Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFECSStats14Titan480::CopyRegCmd(
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
// IFECSStats14Titan480::~IFECSStats14Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECSStats14Titan480::~IFECSStats14Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECSStats14Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CS moduleConfig: 0x%x", m_regCmd.moduleConfig);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Offset Config HxV [%u x %u]",
                     m_regCmd.regionConfig.regionOffset.bitfields.RGN_H_OFFSET,
                     m_regCmd.regionConfig.regionOffset.bitfields.RGN_V_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Number Config HxV [%u x %u]",
                     m_regCmd.regionConfig.regionNum.bitfields.RGN_H_NUM,
                     m_regCmd.regionConfig.regionNum.bitfields.RGN_V_NUM);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Size Config WxH   [%u x %u]",
                     m_regCmd.regionConfig.regionSize.bitfields.RGN_WIDTH,
                     m_regCmd.regionConfig.regionSize.bitfields.RGN_HEIGHT);
}

CAMX_NAMESPACE_END
