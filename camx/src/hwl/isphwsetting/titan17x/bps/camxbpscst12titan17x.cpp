////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpscst12titan17x.cpp
/// @brief CAMXBPSCST12TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpscst12titan17x.h"
#include "cst12setting.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief CST register Configuration
struct BPSCST12RegCmd
{
    BPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH0_COEFF_CFG_0   channel0CoefficientConfig0; ///< Channel 0 Coefficient Config
    BPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH0_COEFF_CFG_1   channel0CoefficientConfig1; ///< Channel 0 Coefficient Config
    BPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH0_OFFSET_CFG    channel0OffsetConfig;       ///< Channel 0 Offset Config
    BPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH0_CLAMP_CFG     channel0ClampConfig;        ///< Channel 0 Clamp Config
    BPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH1_COEFF_CFG_0   channel1CoefficientConfig0; ///< Channel 0 Coefficient Config
    BPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH1_COEFF_CFG_1   channel1CoefficientConfig1; ///< Channel 0 Coefficient Config
    BPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH1_OFFSET_CFG    channel1OffsetConfig;       ///< Channel 0 Offset Config
    BPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH1_CLAMP_CFG     channel1ClampConfig;        ///< Channel 0 Clamp Config
    BPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH2_COEFF_CFG_0   channel2CoefficientConfig0; ///< Channel 0 Coefficient Config
    BPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH2_COEFF_CFG_1   channel2CoefficientConfig1; ///< Channel 0 Coefficient Config
    BPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH2_OFFSET_CFG    channel2OffsetConfig;       ///< Channel 0 Offset Config
    BPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH2_CLAMP_CFG     channel2ClampConfig;        ///< Channel 0 Clamp Config
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 BPSCST12RegLengthDWord = sizeof(BPSCST12RegCmd) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12Titan17x::BPSCST12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSCST12Titan17x::BPSCST12Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCST12Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSCST12Titan17x* pHWSetting = CAMX_NEW BPSCST12Titan17x;

    if (NULL != pHWSetting)
    {
        result = pHWSetting->Initialize();
        if (CamxResultSuccess == result)
        {
            (*ppHWSetting) = pHWSetting;
        }
        else
        {
            CAMX_DELETE pHWSetting;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to initialize in %s, no memory", __FUNCTION__);
        }
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "%s failed, no memory", __FUNCTION__);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSCST12Titan17x::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCST12Titan17x::Initialize()
{
    CamxResult result = CamxResultSuccess;

    BPSCST12RegCmd* pRegCmd = static_cast<BPSCST12RegCmd*>(CAMX_CALLOC(sizeof(BPSCST12RegCmd)));

    if (NULL != pRegCmd)
    {
        m_pRegCmd = pRegCmd;
        SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSCST12RegLengthDWord));

        pRegCmd->channel0CoefficientConfig0.u32All = 0x00750259;
        pRegCmd->channel0CoefficientConfig1.u32All = 0x00000132;
        pRegCmd->channel0OffsetConfig.u32All       = 0x00000000;
        pRegCmd->channel0ClampConfig.u32All        = 0x03ff0000;
        pRegCmd->channel1CoefficientConfig0.u32All = 0x01fe1eae;
        pRegCmd->channel1CoefficientConfig1.u32All = 0x00001f54;
        pRegCmd->channel1OffsetConfig.u32All       = 0x02000000;
        pRegCmd->channel1ClampConfig.u32All        = 0x03ff0000;
        pRegCmd->channel2CoefficientConfig0.u32All = 0x1fad1e55;
        pRegCmd->channel2CoefficientConfig1.u32All = 0x000001fe;
        pRegCmd->channel2OffsetConfig.u32All       = 0x02000000;
        pRegCmd->channel2ClampConfig.u32All        = 0x03ff0000;
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for register configuration!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCST12Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    if (NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regBPS_BPS_0_CLC_COLOR_XFORM_COLOR_XFORM_CH0_COEFF_CFG_0,
                                              BPSCST12RegLengthDWord,
                                              static_cast<UINT32*>(m_pRegCmd));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCST12Titan17x::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->colorXformParameters.moduleCfg.EN = moduleEnable;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCST12Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*     pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSCST12RegCmd*    pRegCmd            = static_cast<BPSCST12RegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSCST12RegCmd) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSCSTData.CSTConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSCSTData.CSTConfig,
                        pRegCmd,
                        sizeof(BPSCST12RegCmd));

        if (TRUE == pBPSIQSettings->colorXformParameters.moduleCfg.EN)
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSCST12Register,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata17x.BPSCSTData.CSTConfig),
                &pBPSTuningMetadata->BPSTuningMetadata17x.BPSCSTData.CSTConfig,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSCSTData.CSTConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCST12Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result  = CamxResultSuccess;
    CST12UnpackedField* pData   = static_cast<CST12UnpackedField*>(pInput);
    BPSCST12RegCmd*     pRegCmd = static_cast<BPSCST12RegCmd*>(m_pRegCmd);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        pRegCmd->channel0CoefficientConfig0.bitfields.MATRIX_M00 = pData->m00;
        pRegCmd->channel0CoefficientConfig0.bitfields.MATRIX_M01 = pData->m01;
        pRegCmd->channel0CoefficientConfig1.bitfields.MATRIX_M02 = pData->m02;
        pRegCmd->channel0OffsetConfig.bitfields.OFFSET_O0        = pData->o0;
        pRegCmd->channel0OffsetConfig.bitfields.OFFSET_S0        = pData->s0;
        pRegCmd->channel0ClampConfig.bitfields.CLAMP_MIN         = pData->c00;
        pRegCmd->channel0ClampConfig.bitfields.CLAMP_MAX         = pData->c01;
        pRegCmd->channel1CoefficientConfig0.bitfields.MATRIX_M10 = pData->m10;
        pRegCmd->channel1CoefficientConfig0.bitfields.MATRIX_M11 = pData->m11;
        pRegCmd->channel1CoefficientConfig1.bitfields.MATRIX_M12 = pData->m12;
        pRegCmd->channel1OffsetConfig.bitfields.OFFSET_O1        = pData->o1;
        pRegCmd->channel1OffsetConfig.bitfields.OFFSET_S1        = pData->s1;
        pRegCmd->channel1ClampConfig.bitfields.CLAMP_MAX         = pData->c11;
        pRegCmd->channel1ClampConfig.bitfields.CLAMP_MIN         = pData->c10;
        pRegCmd->channel2CoefficientConfig0.bitfields.MATRIX_M20 = pData->m20;
        pRegCmd->channel2CoefficientConfig0.bitfields.MATRIX_M21 = pData->m21;
        pRegCmd->channel2CoefficientConfig1.bitfields.MATRIX_M22 = pData->m22;
        pRegCmd->channel2OffsetConfig.bitfields.OFFSET_O2        = pData->o2;
        pRegCmd->channel2OffsetConfig.bitfields.OFFSET_S2        = pData->s2;
        pRegCmd->channel2ClampConfig.bitfields.CLAMP_MAX         = pData->c21;
        pRegCmd->channel2ClampConfig.bitfields.CLAMP_MIN         = pData->c20;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Input data is pData %p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12Titan17x::~BPSCST12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSCST12Titan17x::~BPSCST12Titan17x()
{
    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCST12Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSCST12Titan17x::DumpRegConfig()
{
    BPSCST12RegCmd* pRegCmd = static_cast<BPSCST12RegCmd*>(m_pRegCmd);

    if (NULL != pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "channel0CoefficientConfig 0  = 0x%x\n", pRegCmd->channel0CoefficientConfig0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "channel0CoefficientConfig 1  = 0x%x\n", pRegCmd->channel0CoefficientConfig1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "channel0OffsetConfig         = 0x%x\n", pRegCmd->channel0OffsetConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "channel1CoefficientConfig 0  = 0x%x\n", pRegCmd->channel1CoefficientConfig0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "channel1CoefficientConfig 1  = 0x%x\n", pRegCmd->channel1CoefficientConfig1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "channel1OffsetConfig         = 0x%x\n", pRegCmd->channel1OffsetConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "channel1ClampConfig          = 0x%x\n", pRegCmd->channel1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "channel2CoefficientConfig 0  = 0x%x\n", pRegCmd->channel2CoefficientConfig0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "channel2CoefficientConfig 1  = 0x%x\n", pRegCmd->channel2CoefficientConfig1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "channel2OffsetConfig         = 0x%x\n", pRegCmd->channel2OffsetConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "channel2ClampConfig          = 0x%x\n", pRegCmd->channel2ClampConfig);
    }
}

CAMX_NAMESPACE_END
