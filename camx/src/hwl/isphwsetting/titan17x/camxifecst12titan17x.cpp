////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecst12titan17x.cpp
/// @brief CAMXIFECST12TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifecst12titan17x.h"
#include "cst12setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan17x::IFECST12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECST12Titan17x::IFECST12Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECST12RegLengthDWord));

    // Hardcode initial value for all the registers
    m_regCmd.ch0ClampConfig.u32All        = 0x03ff0000;
    m_regCmd.ch0Coefficient0.u32All       = 0x00750259;
    m_regCmd.ch0Coefficient1.u32All       = 0x00000132;
    m_regCmd.ch0OffsetConfig.u32All       = 0x00000000;
    m_regCmd.ch1ClampConfig.u32All        = 0x03ff0000;
    m_regCmd.ch1CoefficientConfig0.u32All = 0x01fe1eae;
    m_regCmd.ch1CoefficientConfig1.u32All = 0x00001f54;
    m_regCmd.ch1OffsetConfig.u32All       = 0x02000000;
    m_regCmd.ch2ClampConfig.u32All        = 0x03ff0000;
    m_regCmd.ch2CoefficientConfig0.u32All = 0x1fad1e55;
    m_regCmd.ch2CoefficientConfig1.u32All = 0x000001fe;
    m_regCmd.ch2OffsetConfig.u32All       = 0x02000000;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12Titan17x::CreateCmdList(
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
        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_COLOR_XFORM_CH0_COEFF_CFG_0,
                                                  IFECST12RegLengthDWord,
                                                  reinterpret_cast<UINT32*>(&m_regCmd));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer. result = %d", result);
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
// IFECST12Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFECST12RegCmd) <= sizeof(pIFETuningMetadata->metadata17x.IFECSTData.CSTConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFECSTData.CSTConfig, &m_regCmd, sizeof(IFECST12RegCmd));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result  = CamxResultSuccess;
    CST12UnpackedField* pData   = static_cast<CST12UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        m_regCmd.ch0Coefficient0.bitfields.MATRIX_M00       = pData->m00;
        m_regCmd.ch0Coefficient0.bitfields.MATRIX_M01       = pData->m01;
        m_regCmd.ch0Coefficient1.bitfields.MATRIX_M02       = pData->m02;
        m_regCmd.ch0OffsetConfig.bitfields.OFFSET_O0        = pData->o0;
        m_regCmd.ch0OffsetConfig.bitfields.OFFSET_S0        = pData->s0;
        m_regCmd.ch0ClampConfig.bitfields.CLAMP_MIN         = pData->c00;
        m_regCmd.ch0ClampConfig.bitfields.CLAMP_MAX         = pData->c01;
        m_regCmd.ch1CoefficientConfig0.bitfields.MATRIX_M10 = pData->m10;
        m_regCmd.ch1CoefficientConfig0.bitfields.MATRIX_M11 = pData->m11;
        m_regCmd.ch1CoefficientConfig1.bitfields.MATRIX_M12 = pData->m12;
        m_regCmd.ch1OffsetConfig.bitfields.OFFSET_O1        = pData->o1;
        m_regCmd.ch1OffsetConfig.bitfields.OFFSET_S1        = pData->s1;
        m_regCmd.ch1ClampConfig.bitfields.CLAMP_MAX         = pData->c11;
        m_regCmd.ch1ClampConfig.bitfields.CLAMP_MIN         = pData->c10;
        m_regCmd.ch2CoefficientConfig0.bitfields.MATRIX_M20 = pData->m20;
        m_regCmd.ch2CoefficientConfig0.bitfields.MATRIX_M21 = pData->m21;
        m_regCmd.ch2CoefficientConfig1.bitfields.MATRIX_M22 = pData->m22;
        m_regCmd.ch2OffsetConfig.bitfields.OFFSET_O2        = pData->o2;
        m_regCmd.ch2OffsetConfig.bitfields.OFFSET_S2        = pData->s2;
        m_regCmd.ch2ClampConfig.bitfields.CLAMP_MAX         = pData->c21;
        m_regCmd.ch2ClampConfig.bitfields.CLAMP_MIN         = pData->c20;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_ERROR(CamxLogGroupIQMod, "Not Implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan17x::~IFECST12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECST12Titan17x::~IFECST12Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECST12Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch0ClampConfig           [0x%x]", m_regCmd.ch0ClampConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch0Coefficient0          [0x%x]", m_regCmd.ch0Coefficient0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch0Coefficient1          [0x%x]", m_regCmd.ch0Coefficient1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch0OffsetConfig          [0x%x]", m_regCmd.ch0OffsetConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch1ClampConfig           [0x%x]", m_regCmd.ch1ClampConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch1CoefficientConfig0    [0x%x]", m_regCmd.ch1CoefficientConfig0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch1CoefficientConfig1    [0x%x]", m_regCmd.ch1CoefficientConfig1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch1OffsetConfig          [0x%x]", m_regCmd.ch1OffsetConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch2ClampConfig           [0x%x]", m_regCmd.ch2ClampConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch2CoefficientConfig0    [0x%x]", m_regCmd.ch2CoefficientConfig0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch2CoefficientConfig1    [0x%x]", m_regCmd.ch2CoefficientConfig1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch2OffsetConfig          [0x%x]", m_regCmd.ch2OffsetConfig.u32All);
}

CAMX_NAMESPACE_END
