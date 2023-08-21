////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifersstats14titan480.cpp
/// @brief IFERSStats14Titan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"
#include "camxifersstats14.h"
#include "camxifersstats14titan480.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERSStats14Titan480::IFERSStats14Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFERSStats14Titan480::IFERSStats14Titan480()
{
    SetCommandLength(
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFERS14Titan480Config) / RegisterWidthInBytes));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERSStats14Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERSStats14Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CAMX_ASSERT_MESSAGE(NULL != pInputData->pCmdBuffer, "RS stats invalid cmd buffer pointer");

    if (NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_STATS_RS_MODULE_CFG,
                                              (sizeof(IFE_IFE_0_PP_CLC_STATS_RS_MODULE_CFG) / RegisterWidthInBytes),
                                              reinterpret_cast<UINT32*>(&m_regCmd.moduleConfig));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_RS_RGN_OFFSET_CFG,
                                                  (sizeof(IFERS14Titan480RegionConfig) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.regionConfig));
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
// IFERSStats14Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERSStats14Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);
    CamxResult           result        = CamxResultSuccess;
    RS14ConfigData*      pConfigData   = static_cast<RS14ConfigData*>(pInput);

    if ((NULL != pConfigData) && (NULL != pConfigData->pISPInputData))
    {
        m_regCmd.moduleConfig.bitfields.EN                          = TRUE;
        m_regCmd.moduleConfig.bitfields.SHIFT_BITS                  = pConfigData->shiftBits;
        m_regCmd.regionConfig.regionOffset.bitfields.RGN_H_OFFSET   = pConfigData->pRSConfig->statsHOffset;
        m_regCmd.regionConfig.regionOffset.bitfields.RGN_V_OFFSET   = pConfigData->pRSConfig->statsVOffset;
        m_regCmd.regionConfig.regionNum.bitfields.RGN_H_NUM         = pConfigData->pRSConfig->RSConfig.statsHNum  - 1;
        m_regCmd.regionConfig.regionNum.bitfields.RGN_V_NUM         = pConfigData->pRSConfig->RSConfig.statsVNum  - 1;
        m_regCmd.regionConfig.regionSize.bitfields.RGN_HEIGHT       = pConfigData->pRSConfig->statsRgnHeight - 1;
        m_regCmd.regionConfig.regionSize.bitfields.RGN_WIDTH        = pConfigData->pRSConfig->statsRgnWidth - 1;

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
// IFERSStats14Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERSStats14Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERSStats14Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFERSStats14Titan480::CopyRegCmd(
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
// IFERSStats14Titan480::~IFERSStats14Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFERSStats14Titan480::~IFERSStats14Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERSStats14Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERSStats14Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RS moduleConfig: en: %u, shift bits: %u",
                     m_regCmd.moduleConfig.bitfields.EN,
                     m_regCmd.moduleConfig.bitfields.SHIFT_BITS);
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
