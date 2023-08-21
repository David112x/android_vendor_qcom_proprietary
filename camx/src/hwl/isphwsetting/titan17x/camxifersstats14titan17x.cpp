////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifersstats14titan17x.cpp
/// @brief IFERSStats14Titan17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"
#include "camxifersstats14.h"
#include "camxifersstats14titan17x.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERSStats14Titan17x::IFERSStats14Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFERSStats14Titan17x::IFERSStats14Titan17x()
{
    SetCommandLength(
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERS14RegionConfig) / RegisterWidthInBytes));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERSStats14Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERSStats14Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CAMX_ASSERT_MESSAGE(NULL != pInputData->pCmdBuffer, "rsstats invalid cmd buffer pointer");

    if (NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_STATS_RS_RGN_OFFSET_CFG,
                                              (sizeof(IFERS14RegionConfig) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd));

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
// IFERSStats14Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERSStats14Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);
    CamxResult      result      = CamxResultSuccess;
    RS14ConfigData* pConfigData = static_cast<RS14ConfigData*>(pInput);

    if ((NULL != pConfigData) && (NULL != pConfigData->pISPInputData))
    {
        m_regCmd.regionOffset.bitfields.RGN_H_OFFSET    = pConfigData->pRSConfig->statsHOffset;
        m_regCmd.regionOffset.bitfields.RGN_V_OFFSET    = pConfigData->pRSConfig->statsVOffset;
        m_regCmd.regionNum.bitfields.RGN_H_NUM          = pConfigData->pRSConfig->RSConfig.statsHNum  - 1;
        m_regCmd.regionNum.bitfields.RGN_V_NUM          = pConfigData->pRSConfig->RSConfig.statsVNum  - 1;
        m_regCmd.regionSize.bitfields.RGN_HEIGHT        = pConfigData->pRSConfig->statsRgnHeight - 1;
        m_regCmd.regionSize.bitfields.RGN_WIDTH         = pConfigData->pRSConfig->statsRgnWidth - 1;

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
// IFERSStats14Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERSStats14Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    RS14ConfigData* pConfigData = static_cast<RS14ConfigData*>(pInput);

    if ((NULL != pConfigData) && (NULL != pConfigData->pISPInputData))
    {
        // Write to general stats configuration register
        pConfigData->pISPInputData->pCalculatedData->moduleEnable.statsModules.RSShiftBits  = pConfigData->shiftBits;
        pConfigData->pISPInputData->pCalculatedData->moduleEnable.statsModules.colorConversionEnable    =
            (FALSE == pConfigData->pRSConfig->RSConfig.statsRSCSColorConversionEnable) ? 0 : 1;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Missing data for configuration");
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERSStats14Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFERSStats14Titan17x::CopyRegCmd(
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
// IFERSStats14Titan17x::~IFERSStats14Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFERSStats14Titan17x::~IFERSStats14Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERSStats14Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERSStats14Titan17x::DumpRegConfig()
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
