////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecamiflitetitan17x.cpp
/// @brief CAMXIFECAMIFTITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifecamiflitetitan17x.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFLiteTitan17x::IFECAMIFLiteTitan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIFLiteTitan17x::IFECAMIFLiteTitan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECAMIFLiteRegLengthDWord));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFLiteTitan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFLiteTitan17x::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData && NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_CAMIF_LITE_CMD,
                                              IFECAMIFLiteRegLengthDWord,
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
// IFECAMIFLiteTitan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFLiteTitan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult                    result        = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);
    CAMX_UNREFERENCED_PARAM(pOutput);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFLiteTitan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECAMIFLiteTitan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pInput);

    /// @todo (CAMX-677) Set up the regiser value based on the input data
    m_regCmd.statusRegister.u32All                              = 0x0;
    m_regCmd.statusRegister.bitfields.CLEAR_CAMIF_STATUS        = 0x1;
    m_regCmd.configRegister.u32All                              = 0x0;

    if (NULL != pInputData->pPDHwConfig)
    {
        m_regCmd.configRegister.bitfields.PD_OUTPUT_EN          = pInputData->pPDHwConfig->enablePDHw;
    }
    else
    {
        m_regCmd.configRegister.bitfields.PD_OUTPUT_EN          = 0;
    }

    m_regCmd.skipPeriodRegister.u32All                          = 0x0;
    m_regCmd.skipPeriodRegister.bitfields.IRQ_SUBSAMPLE_PERIOD  = 0;
    m_regCmd.irqSubsamplePattern.bitfields.PATTERN              = 0x1;
    m_regCmd.irqEpochRegister.u32All                            = 0x0;
    m_regCmd.irqEpochRegister.bitfields.EPOCH1_LINE             = IFECAMIFLITEEPOCHIRQLine1;
    m_regCmd.irqEpochRegister.bitfields.EPOCH0_LINE             = IFECAMIFLITEEPOCHIRQLine0;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFLiteTitan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFECAMIFLiteTitan17x::CopyRegCmd(
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
// IFECAMIFLiteTitan17x::~IFECAMIFLiteTitan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECAMIFLiteTitan17x::~IFECAMIFLiteTitan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECAMIFLiteTitan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECAMIFLiteTitan17x::DumpRegConfig()
{
}

CAMX_NAMESPACE_END
