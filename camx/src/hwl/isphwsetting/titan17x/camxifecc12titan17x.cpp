////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecc12titan17x.cpp
/// @brief CAMXIFECC12TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifecc12titan17x.h"
#include "ifecc12setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12Titan17x::IFECC12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECC12Titan17x::IFECC12Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECC12RegLengthDWord));

    // Hardcode initial value for all the registers
    /// @todo (CAMX-677) This hardcode value is to support initial presil test. Will replace with memset to 0
    m_regCmd.coefficientRegister0.u32All = 0x80;
    m_regCmd.coefficientRegister1.u32All = 0x0;
    m_regCmd.coefficientRegister2.u32All = 0x0;
    m_regCmd.coefficientRegister3.u32All = 0x0;
    m_regCmd.coefficientRegister4.u32All = 0x80;
    m_regCmd.coefficientRegister5.u32All = 0x0;
    m_regCmd.coefficientRegister6.u32All = 0x0;
    m_regCmd.coefficientRegister7.u32All = 0x0;
    m_regCmd.coefficientRegister8.u32All = 0x80;
    m_regCmd.offsetRegister0.u32All      = 0x0;
    m_regCmd.offsetRegister1.u32All      = 0x0;
    m_regCmd.offsetRegister2.u32All      = 0x0;
    m_regCmd.coefficientQRegister.u32All = 0x0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC12Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData) && (NULL != pInputData->pCmdBuffer))
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_COLOR_CORRECT_COEFF_0,
                                              IFECC12RegLengthDWord,
                                              reinterpret_cast<UINT32*>(&m_regCmd));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC12Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFECC12RegCmd) <= sizeof(pIFETuningMetadata->metadata17x.IFECCData.CCConfig1));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFECCData.CCConfig1, &m_regCmd, sizeof(IFECC12RegCmd));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC12Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result  = CamxResultSuccess;
    CC12UnpackedField*  pData   = static_cast<CC12UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        m_regCmd.coefficientRegister0.bitfields.CN            = pData->c0_l;
        m_regCmd.coefficientRegister0.bitfields.RIGHT_CN      = pData->c0_r;
        m_regCmd.coefficientRegister1.bitfields.CN            = pData->c1_l;
        m_regCmd.coefficientRegister1.bitfields.RIGHT_CN      = pData->c1_r;
        m_regCmd.coefficientRegister2.bitfields.CN            = pData->c2_l;
        m_regCmd.coefficientRegister2.bitfields.RIGHT_CN      = pData->c2_r;
        m_regCmd.coefficientRegister3.bitfields.CN            = pData->c3_l;
        m_regCmd.coefficientRegister3.bitfields.RIGHT_CN      = pData->c3_r;
        m_regCmd.coefficientRegister4.bitfields.CN            = pData->c4_l;
        m_regCmd.coefficientRegister4.bitfields.RIGHT_CN      = pData->c4_r;
        m_regCmd.coefficientRegister5.bitfields.CN            = pData->c5_l;
        m_regCmd.coefficientRegister5.bitfields.RIGHT_CN      = pData->c5_r;
        m_regCmd.coefficientRegister6.bitfields.CN            = pData->c6_l;
        m_regCmd.coefficientRegister6.bitfields.RIGHT_CN      = pData->c6_r;
        m_regCmd.coefficientRegister7.bitfields.CN            = pData->c7_l;
        m_regCmd.coefficientRegister7.bitfields.RIGHT_CN      = pData->c7_r;
        m_regCmd.coefficientRegister8.bitfields.CN            = pData->c8_l;
        m_regCmd.coefficientRegister8.bitfields.RIGHT_CN      = pData->c8_r;
        m_regCmd.offsetRegister0.bitfields.KN                 = pData->k0_l;
        m_regCmd.offsetRegister0.bitfields.RIGHT_KN           = pData->k0_r;
        m_regCmd.offsetRegister1.bitfields.KN                 = pData->k1_l;
        m_regCmd.offsetRegister1.bitfields.RIGHT_KN           = pData->k1_r;
        m_regCmd.offsetRegister2.bitfields.KN                 = pData->k2_l;
        m_regCmd.offsetRegister2.bitfields.RIGHT_KN           = pData->k2_r;
        m_regCmd.coefficientQRegister.bitfields.QFACTOR       = pData->qfactor_l;
        m_regCmd.coefficientQRegister.bitfields.RIGHT_QFACTOR = pData->qfactor_r;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input params are NULL pData=0x%p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC12Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12Titan17x::~IFECC12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECC12Titan17x::~IFECC12Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECC12Titan17x::DumpRegConfig()
{
}

CAMX_NAMESPACE_END
