////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecst12titan480.cpp
/// @brief CAMXIFECST12TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifecst12titan480.h"
#include "cst12setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan480::IFECST12Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECST12Titan480::IFECST12Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFECST12Titan480RegLengthDWord));

    // Hardcode initial value for all the registers
    m_regCmd.moduleConfig.bitfields.EN                  = 0x1;
    m_regCmd.channelConfig.ch0Coefficient0.u32All       = 0x00750259;
    m_regCmd.channelConfig.ch0Coefficient1.u32All       = 0x00000132;
    m_regCmd.channelConfig.ch0OffsetConfig.u32All       = 0x00000000;
    m_regCmd.channelConfig.ch0ClampConfig.u32All        = 0x03ff0000;

    m_regCmd.channelConfig.ch1Coefficient0.u32All       = 0x01fe0eae;
    m_regCmd.channelConfig.ch1Coefficient1.u32All       = 0x00001f54;
    m_regCmd.channelConfig.ch1OffsetConfig.u32All       = 0x02000000;
    m_regCmd.channelConfig.ch1ClampConfig.u32All        = 0x03ff0000;

    m_regCmd.channelConfig.ch2Coefficient0.u32All       = 0x0fad0e55;
    m_regCmd.channelConfig.ch2Coefficient1.u32All       = 0x000001fe;
    m_regCmd.channelConfig.ch2OffsetConfig.u32All       = 0x02000000;
    m_regCmd.channelConfig.ch2ClampConfig.u32All        = 0x03ff0000;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12Titan480::CreateCmdList(
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
                                                  regIFE_IFE_0_PP_CLC_COLOR_XFORM_MODULE_CFG,
                                                  1,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.moduleConfig));
        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_COLOR_XFORM_COLOR_XFORM_CH0_COEFF_CFG_0,
                                                  sizeof(IFECST12Titan480ChannelConfig) / sizeof(UINT32),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.channelConfig));
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
// IFECST12Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFECST12Titan480RegCmd) <= sizeof(pIFETuningMetadata->metadata480.IFECSTData.CSTConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFECSTData.CSTConfig, &m_regCmd, sizeof(IFECST12Titan480RegCmd));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result  = CamxResultSuccess;
    CST12UnpackedField* pData   = static_cast<CST12UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        m_regCmd.moduleConfig.bitfields.EN                           = pData->enable;
        m_regCmd.channelConfig.ch0Coefficient0.bitfields.MATRIX_M00  = pData->m00;
        m_regCmd.channelConfig.ch0Coefficient0.bitfields.MATRIX_M01  = pData->m01;
        m_regCmd.channelConfig.ch0Coefficient1.bitfields.MATRIX_M02  = pData->m02;
        m_regCmd.channelConfig.ch0OffsetConfig.bitfields.OFFSET_O0   = pData->o0;
        m_regCmd.channelConfig.ch0OffsetConfig.bitfields.OFFSET_S0   = pData->s0;
        m_regCmd.channelConfig.ch0ClampConfig.bitfields.CLAMP_MIN    = pData->c00;
        m_regCmd.channelConfig.ch0ClampConfig.bitfields.CLAMP_MAX    = pData->c01;
        m_regCmd.channelConfig.ch1Coefficient0.bitfields.MATRIX_M10  = pData->m10;
        m_regCmd.channelConfig.ch1Coefficient0.bitfields.MATRIX_M11  = pData->m11;
        m_regCmd.channelConfig.ch1Coefficient1.bitfields.MATRIX_M12  = pData->m12;
        m_regCmd.channelConfig.ch1OffsetConfig.bitfields.OFFSET_O1   = pData->o1;
        m_regCmd.channelConfig.ch1OffsetConfig.bitfields.OFFSET_S1   = pData->s1;
        m_regCmd.channelConfig.ch1ClampConfig.bitfields.CLAMP_MIN    = pData->c10;
        m_regCmd.channelConfig.ch1ClampConfig.bitfields.CLAMP_MAX    = pData->c11;
        m_regCmd.channelConfig.ch2Coefficient0.bitfields.MATRIX_M20  = pData->m20;
        m_regCmd.channelConfig.ch2Coefficient0.bitfields.MATRIX_M21  = pData->m21;
        m_regCmd.channelConfig.ch2Coefficient1.bitfields.MATRIX_M22  = pData->m22;
        m_regCmd.channelConfig.ch2OffsetConfig.bitfields.OFFSET_O2   = pData->o2;
        m_regCmd.channelConfig.ch2OffsetConfig.bitfields.OFFSET_S2   = pData->s2;
        m_regCmd.channelConfig.ch2ClampConfig.bitfields.CLAMP_MIN    = pData->c20;
        m_regCmd.channelConfig.ch2ClampConfig.bitfields.CLAMP_MAX    = pData->c21;
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan480::CreateSubCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12Titan480::CreateSubCmdList(
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
            regIFE_IFE_0_PP_CLC_COLOR_XFORM_MODULE_CFG,
            sizeof(m_regCmd.moduleConfig) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd.moduleConfig));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFECST12Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    BOOL* pModuleEnable = static_cast<BOOL*>(pInput);

    if (NULL != pInput)
    {
        m_regCmd.moduleConfig.bitfields.EN = *pModuleEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan480::~IFECST12Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFECST12Titan480::~IFECST12Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECST12Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFECST12Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch0ClampConfig       [0x%x]", m_regCmd.channelConfig.ch0ClampConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch0Coefficient0      [0x%x]", m_regCmd.channelConfig.ch0Coefficient0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch0Coefficient1      [0x%x]", m_regCmd.channelConfig.ch0Coefficient1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch0OffsetConfig      [0x%x]", m_regCmd.channelConfig.ch0OffsetConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch1ClampConfig       [0x%x]", m_regCmd.channelConfig.ch1ClampConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch1Coefficient0      [0x%x]", m_regCmd.channelConfig.ch1Coefficient0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch1Coefficient1      [0x%x]", m_regCmd.channelConfig.ch1Coefficient1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch1OffsetConfig      [0x%x]", m_regCmd.channelConfig.ch1OffsetConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch2ClampConfig       [0x%x]", m_regCmd.channelConfig.ch2ClampConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch2Coefficient0      [0x%x]", m_regCmd.channelConfig.ch2Coefficient0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch2Coefficient1      [0x%x]", m_regCmd.channelConfig.ch2Coefficient1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "CST ch2OffsetConfig      [0x%x]", m_regCmd.channelConfig.ch2OffsetConfig.u32All);
}

CAMX_NAMESPACE_END
