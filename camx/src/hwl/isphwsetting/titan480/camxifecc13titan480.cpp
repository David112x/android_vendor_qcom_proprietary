////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecc13titan480.cpp
/// @brief CAMXIFECC13TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifecc13titan480.h"
#include "cc13setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC13Titan480::IFECC13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECC13Titan480::IFECC13Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECC13RegLengthDWord1) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECC13RegLengthDWord2));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC13Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC13Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData) && (NULL != pInputData->pCmdBuffer))
    {
        result = PacketBuilder::WriteRegRange(pInputData->pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_COLOR_CORRECT_MODULE_CFG,
                                              IFECC13RegLengthDWord1,
                                              reinterpret_cast<UINT32*>(&m_regCmd1));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pInputData->pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_A_CFG_0,
                                                  IFECC13RegLengthDWord2,
                                                  reinterpret_cast<UINT32*>(&m_regCmd2));
        }

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
// IFECC13Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC13Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFECC13RegCmd1) <= sizeof(pIFETuningMetadata->metadata480.IFECCData.CCConfig1));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFECCData.CCConfig1, &m_regCmd1, sizeof(IFECC13RegCmd1));

        CAMX_STATIC_ASSERT(sizeof(IFECC13RegCmd2) <= sizeof(pIFETuningMetadata->metadata480.IFECCData.CCConfig2));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFECCData.CCConfig2, &m_regCmd2, sizeof(IFECC13RegCmd2));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC13Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC13Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result  = CamxResultSuccess;
    CC13UnpackedField*  pData   = static_cast<CC13UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        m_regCmd1.moduleConfig.bitfields.EN                  = 1;

        // Out_ch0 = A0*G + B0*B + C0*R + K0;
        // Out_ch1 = A1*G + B1*B + C1*R + K1;
        // Out_ch2 = A2*G + B2*B + C2*R + K2,
        m_regCmd2.coefficientAConfig0.bitfields.MATRIX_A0    = pData->c0;
        m_regCmd2.coefficientAConfig0.bitfields.MATRIX_A1    = pData->c3;
        m_regCmd2.coefficientAConfig1.bitfields.MATRIX_A2    = pData->c6;
        m_regCmd2.coefficientBConfig0.bitfields.MATRIX_B0    = pData->c1;
        m_regCmd2.coefficientBConfig0.bitfields.MATRIX_B1    = pData->c4;
        m_regCmd2.coefficientBConfig1.bitfields.MATRIX_B2    = pData->c7;
        m_regCmd2.coefficientCConfig0.bitfields.MATRIX_C0    = pData->c2;
        m_regCmd2.coefficientCConfig0.bitfields.MATRIX_C1    = pData->c5;
        m_regCmd2.coefficientCConfig1.bitfields.MATRIX_C2    = pData->c8;
        m_regCmd2.offsetKConfig0.bitfields.OFFSET_K0         = pData->k0;
        m_regCmd2.offsetKConfig0.bitfields.OFFSET_K1         = pData->k1;
        m_regCmd2.offsetKConfig1.bitfields.OFFSET_K2         = pData->k2;
        m_regCmd2.shiftMConfig.bitfields.M_PARAM             = pData->qfactor;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC13Titan480::CreateSubCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC13Titan480::CreateSubCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData &&
        NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
            regIFE_IFE_0_PP_CLC_COLOR_CORRECT_MODULE_CFG,
            sizeof(m_regCmd1.moduleConfig) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd1.moduleConfig));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC13Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECC13Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    BOOL* pModuleEnable = static_cast<BOOL*>(pInput);

    if (NULL != pInput)
    {
        m_regCmd1.moduleConfig.bitfields.EN = *pModuleEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC13Titan480::~IFECC13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECC13Titan480::~IFECC13Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC13Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECC13Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CC13 coefficientAConfig0 : 0x%x", m_regCmd2.coefficientAConfig0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CC13 coefficientAConfig1 : 0x%x", m_regCmd2.coefficientAConfig1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CC13 coefficientBConfig0 : 0x%x", m_regCmd2.coefficientBConfig0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CC13 coefficientBConfig1 : 0x%x", m_regCmd2.coefficientBConfig1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CC13 coefficientCConfig0 : 0x%x", m_regCmd2.coefficientCConfig0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CC13 coefficientCConfig1 : 0x%x", m_regCmd2.coefficientCConfig1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CC13 offsetKConfig0      : 0x%x", m_regCmd2.offsetKConfig0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CC13 offsetKConfig1      : 0x%x", m_regCmd2.offsetKConfig1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CC13 shiftMConfig        : 0x%x", m_regCmd2.shiftMConfig.u32All);
}

CAMX_NAMESPACE_END
