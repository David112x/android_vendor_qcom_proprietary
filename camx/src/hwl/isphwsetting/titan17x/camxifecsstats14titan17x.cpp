////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecsstats14titan17x.cpp
/// @brief CAMXIFECSSTATS14TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"
#include "camxifecsstats14.h"
#include "camxifecsstats14titan17x.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan17x::IFECSStats14Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECSStats14Titan17x::IFECSStats14Titan17x()
{
    SetCommandLength(
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFECS14RegionConfig) / RegisterWidthInBytes));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSStats14Titan17x::CreateCmdList(
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
                                          regIFE_IFE_0_VFE_STATS_CS_RGN_OFFSET_CFG,
                                          (sizeof(IFECS14RegionConfig) / RegisterWidthInBytes),
                                          reinterpret_cast<UINT32*>(&m_regCmd));

    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Failed to write command buffer");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan17x::AdjustHorizontalRegionNumber
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFECSStats14Titan17x::AdjustHorizontalRegionNumber(
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
// IFECSStats14Titan17x::GetHWCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSStats14Titan17x::GetHWCapability(
    VOID*   pHWCapability)
{
    CamxResult              result      = CamxResultSuccess;
    CS14StatsHWCapability*  pCapability = static_cast<CS14StatsHWCapability*>(pHWCapability);

    pCapability->inputDepth         = CSTitan17xInputDepth;
    pCapability->outputDepth        = CSTitan17xOutputDepth;
    pCapability->maxRegionWidth     = CSStats14Titan17xMaxRegionWidth;
    pCapability->minRegionWidth     = CSStats14Titan17xMinRegionWidth;
    pCapability->minRegionHeight    = CSStats14Titan17xMinRegionHeight;
    pCapability->minHorizRegions    = CSStats14Titan17xMinHorizRegions;
    pCapability->maxHorizRegions    = CSStats14Titan17xMaxHorizRegions;
    pCapability->minVertRegions     = CSStats14Titan17xMinVertRegions;
    pCapability->maxVertRegions     = CSStats14Titan17xMaxVertRegions;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSStats14Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);
    CamxResult      result      = CamxResultSuccess;
    CS14ConfigData* pConfigData = static_cast<CS14ConfigData*>(pInput);

    if ((NULL != pConfigData) && (NULL != pConfigData->pISPInputData))
    {
        m_regCmd.regionOffset.bitfields.RGN_H_OFFSET    = pConfigData->pCSConfig->CSConfig.statsHOffset;
        m_regCmd.regionOffset.bitfields.RGN_V_OFFSET    = pConfigData->pCSConfig->CSConfig.statsVOffset;
        m_regCmd.regionNum.bitfields.RGN_H_NUM          = pConfigData->pCSConfig->CSConfig.statsHNum - 1;
        m_regCmd.regionNum.bitfields.RGN_V_NUM          = pConfigData->pCSConfig->CSConfig.statsVNum - 1;
        m_regCmd.regionSize.bitfields.RGN_WIDTH         = pConfigData->pCSConfig->CSConfig.statsRgnWidth - 1;
        m_regCmd.regionSize.bitfields.RGN_HEIGHT        = pConfigData->pCSConfig->CSConfig.statsRgnHeight - 1;

        // Also we need to adjust rgnHNum to satisfy the hardware limitation
        // regionHNum when divided by 8 needs to have remainder [4, 7]
        m_regCmd.regionNum.bitfields.RGN_H_NUM =
            AdjustHorizontalRegionNumber(m_regCmd.regionNum.bitfields.RGN_H_NUM, 8, 4);

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
// IFECSStats14Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECSStats14Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    CS14ConfigData* pConfigData = static_cast<CS14ConfigData*>(pInput);

    if ((NULL != pConfigData) && (NULL != pConfigData->pISPInputData))
    {
        // Write to general stats configuration register
        pConfigData->pISPInputData->pCalculatedData->moduleEnable.statsModules.CSShiftBits = pConfigData->shiftBits;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Missing data for configuration");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFECSStats14Titan17x::CopyRegCmd(
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
// IFECSStats14Titan17x::~IFECSStats14Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECSStats14Titan17x::~IFECSStats14Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECSStats14Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECSStats14Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Offset Config HxV [%u x %u]",
                     m_regCmd.regionOffset.bitfields.RGN_H_OFFSET,
                     m_regCmd.regionOffset.bitfields.RGN_V_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Number Config HxV [%u x %u]",
                     m_regCmd.regionNum.bitfields.RGN_H_NUM,
                     m_regCmd.regionNum.bitfields.RGN_V_NUM);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Region Size Config WxH   [%u x %u]",
                     m_regCmd.regionSize.bitfields.RGN_WIDTH,
                     m_regCmd.regionSize.bitfields.RGN_HEIGHT);
}

CAMX_NAMESPACE_END
