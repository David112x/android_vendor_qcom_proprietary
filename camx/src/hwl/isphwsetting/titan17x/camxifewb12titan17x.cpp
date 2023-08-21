////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifewb12titan17x.cpp
/// @brief CAMXIFEWB12TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifewb12titan17x.h"
#include "wb12setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB12Titan17x::IFEWB12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEWB12Titan17x::IFEWB12Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEWB12RegLengthDWord));

    // Hardcode initial value for all the registers
    m_regCmd.WBLeftConfig0.bitfields.G_GAIN         = 0x0080;
    m_regCmd.WBLeftConfig0.bitfields.B_GAIN         = 0x00bf;
    m_regCmd.WBLeftConfig1.bitfields.R_GAIN         = 0x0106;
    m_regCmd.WBLeftOffsetConfig0.bitfields.G_OFFSET = 0x0000;
    m_regCmd.WBLeftOffsetConfig0.bitfields.B_OFFSET = 0x0000;
    m_regCmd.WBLeftOffsetConfig1.bitfields.R_OFFSET = 0x0000;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB12Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB12Titan17x::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData) && (NULL != pInputData->pCmdBuffer))
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_DEMO_WB_LEFT_CFG_0,
                                                  IFEWB12RegLengthDWord,
                                                  reinterpret_cast<UINT32*>(&m_regCmd));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB12Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB12Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEWB12RegCmd) <= sizeof(pIFETuningMetadata->metadata17x.IFEWBData.WBconfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEWBData.WBconfig, &m_regCmd, sizeof(IFEWB12RegCmd));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB12Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB12Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult         result        = CamxResultSuccess;
    WB12UnpackedField* pData         = static_cast<WB12UnpackedField*>(pInput);
    WB12OutputData*    pOutputData   = static_cast<WB12OutputData*>(pOutput);

    if ((NULL != pOutputData) && (NULL != pData))
    {
        m_regCmd.WBLeftConfig0.bitfields.G_GAIN          = pData->gainChannel0Left;
        m_regCmd.WBLeftConfig0.bitfields.B_GAIN          = pData->gainChannel1Left;
        m_regCmd.WBLeftConfig1.bitfields.R_GAIN          = pData->gainChannel2Left;
        m_regCmd.WBLeftOffsetConfig0.bitfields.G_OFFSET  = pData->offsetChannel0Left;
        m_regCmd.WBLeftOffsetConfig0.bitfields.B_OFFSET  = pData->offsetChannel1Left;
        m_regCmd.WBLeftOffsetConfig1.bitfields.R_OFFSET  = pData->offsetChannel2Left;

        pOutputData->rGain = m_regCmd.WBLeftConfig1.bitfields.R_GAIN;
        pOutputData->gGain = m_regCmd.WBLeftConfig0.bitfields.G_GAIN;
        pOutputData->bGain = m_regCmd.WBLeftConfig0.bitfields.B_GAIN;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p pOutputData=0x%p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB12Titan17x::~IFEWB12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEWB12Titan17x::~IFEWB12Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB12Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEWB12Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "G_Gain   [0x%x]", m_regCmd.WBLeftConfig0.bitfields.G_GAIN);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "B_Gain   [0x%x]", m_regCmd.WBLeftConfig0.bitfields.B_GAIN);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "R_Gain   [0x%x]", m_regCmd.WBLeftConfig1.bitfields.R_GAIN);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "G_OFFSET [0x%x]", m_regCmd.WBLeftOffsetConfig0.bitfields.G_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "B_OFFSET [0x%x]", m_regCmd.WBLeftOffsetConfig0.bitfields.B_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "R_OFFSET [0x%x]", m_regCmd.WBLeftOffsetConfig1.bitfields.R_OFFSET);
}

CAMX_NAMESPACE_END
