////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebpcbcc50titan17x.cpp
/// @brief CAMXIFEBPCBCC50TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifebpcbcc50titan17x.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50Titan17x::IFEBPCBCC50Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBPCBCC50Titan17x::IFEBPCBCC50Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEBPCBCC50RegLengthDWord));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBPCBCC50Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult      result      = CamxResultSuccess;
    ISPInputData*   pInputData  = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*      pCmdBuffer  = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_BPC_CFG_0,
                                                  IFEBPCBCC50RegLengthDWord,
                                                  reinterpret_cast<UINT32*>(&m_regCmd));
        if (CamxResultSuccess != result)
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBPCBCC50Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult              result  = CamxResultSuccess;
    BPCBCC50UnpackedField*  pData   = static_cast<BPCBCC50UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        m_regCmd.configRegister0.bitfields.HOT_PIXEL_CORR_DISABLE  = pData->hotPixelCorrectionDisable;
        m_regCmd.configRegister0.bitfields.COLD_PIXEL_CORR_DISABLE = pData->coldPixelCorrectionDisable;
        m_regCmd.configRegister0.bitfields.SAME_CH_RECOVER         = pData->sameChannelRecovery;
        m_regCmd.configRegister0.bitfields.BLACK_LEVEL             = pData->black_level;

        m_regCmd.configRegister1.bitfields.RG_WB_GAIN_RATIO        = pData->rg_wb_gain_ratio;
        m_regCmd.configRegister2.bitfields.BG_WB_GAIN_RATIO        = pData->bg_wb_gain_ratio;
        m_regCmd.configRegister3.bitfields.GR_WB_GAIN_RATIO        = pData->gr_wb_gain_ratio;
        m_regCmd.configRegister4.bitfields.GB_WB_GAIN_RATIO        = pData->gb_wb_gain_ratio;

        m_regCmd.configRegister5.bitfields.BPC_OFFSET              = pData->bpcOffset;
        m_regCmd.configRegister5.bitfields.BCC_OFFSET              = pData->bccOffset;

        m_regCmd.configRegister6.bitfields.FMIN                    = pData->fmin;
        m_regCmd.configRegister6.bitfields.FMAX                    = pData->fmax;
        m_regCmd.configRegister6.bitfields.CORRECT_THRESHOLD       = pData->correctionThreshold;
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("data is pData %p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBPCBCC50Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEBPCBCC50Titan17x::CopyRegCmd(
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
// IFEBPCBCC50Titan17x::~IFEBPCBCC50Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBPCBCC50Titan17x::~IFEBPCBCC50Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBPCBCC50Titan17x::DumpRegConfig()
{
    // Print regster config
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "configRegister0 [0x%x]", m_regCmd.configRegister0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "configRegister1 [0x%x]", m_regCmd.configRegister1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "configRegister2 [0x%x]", m_regCmd.configRegister2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "configRegister3 [0x%x]", m_regCmd.configRegister3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "configRegister4 [0x%x]", m_regCmd.configRegister4.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "configRegister5 [0x%x]", m_regCmd.configRegister5.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "configRegister6 [0x%x]", m_regCmd.configRegister6.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "statsRegister   [0x%x]", m_regCmd.statsRegister.u32All);
}

CAMX_NAMESPACE_END
