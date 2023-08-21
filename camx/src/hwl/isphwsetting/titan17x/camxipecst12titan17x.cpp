////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipecst12titan17x.cpp
/// @brief IPECST12Titan17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "titan170_ipe.h"
#include "cst12setting.h"
#include "camxutils.h"
#include "camxipecst12titan17x.h"
#include "camxipecolortransform12.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief: This structure contains color transform channel 0 registers
/// Out_ch0 = (M10*(X0-S0) + M11*(X1-S1) + M12*(X2-S2)) rshift 9 + 1uQ1 + O1;
struct IPEColorTransformConfigChannel0
{
    IPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH0_COEFF_CFG_0   coefficient0;   ///< transform matrix coefficents (M10, M11)
    IPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH0_COEFF_CFG_1   coefficient1;   ///< transform matrix coefficents (M12)
    IPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH0_OFFSET_CFG    offset;         ///< offset param (O1, S1)
    IPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH0_CLAMP_CFG     clamp;          ///< clamp range (MAX, MIN)
} CAMX_PACKED;

/// @brief: This structure contains color transform channel 1 registers
/// Out_ch1 = (M10*(X0-S0) + M11*(X1-S1) + M12*(X2-S2)) rshift 9 + 1uQ1 + O1;
struct IPEColorTransformConfigChannel1
{
    IPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH1_COEFF_CFG_0   coefficient0;   ///< transform matrix coefficents (M10, M11)
    IPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH1_COEFF_CFG_1   coefficient1;   ///< transform matrix coefficents (M12)
    IPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH1_OFFSET_CFG    offset;         ///< offset param (O1, S1)
    IPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH1_CLAMP_CFG     clamp;          ///< clamp range (MAX, MIN)
} CAMX_PACKED;

/// @brief: This structure contains color transform channel 2 registers
/// Out_ch2 = (M10*(X0-S0) + M11*(X1-S1) + M12*(X2-S2)) rshift 9 + 1uQ1 + O1;
struct IPEColorTransformConfigChannel2
{
    IPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH2_COEFF_CFG_0   coefficient0;   ///< transform matrix coefficents (M10, M11)
    IPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH2_COEFF_CFG_1   coefficient1;   ///< transform matrix coefficents (M12)
    IPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH2_OFFSET_CFG    offset;         ///< offset param (O1, S1)
    IPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH2_CLAMP_CFG     clamp;          ///< clamp range (MAX, MIN)
} CAMX_PACKED;

/// @brief: This structure contains Color transform registers programmed by software
struct IPEColorTransformRegCmd
{
    IPEColorTransformConfigChannel0 configChannel0; ///< Output Channel0 configuration parameters
    IPEColorTransformConfigChannel1 configChannel1; ///< Output Channel1 configuration parameters
    IPEColorTransformConfigChannel2 configChannel2; ///< Output Channel2 configuration parameters
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IPEColorTransformRegLength = sizeof(IPEColorTransformRegCmd) / RegisterWidthInBytes;
CAMX_STATIC_ASSERT((12 * 4) == sizeof(IPEColorTransformRegCmd));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECST12Titan17x::IPECST12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPECST12Titan17x::IPECST12Titan17x()
{
    m_pRegCmd = CAMX_CALLOC(sizeof(IPEColorTransformRegCmd));

    if (NULL == m_pRegCmd)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate memory for Cmd Buffer");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECST12Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECST12Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pCSTHWSettingParams)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pCSTHWSettingParams);

    if ((NULL != pInputData)                                 &&
        (NULL != pInputData->pipelineIPEData.ppIPECmdBuffer) &&
        (NULL != m_pRegCmd))
    {
        CmdBuffer*    pCmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPreLTM];

        if (NULL != pCmdBuffer)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIPE_IPE_0_PPS_CLC_COLOR_XFORM_COLOR_XFORM_CH0_COEFF_CFG_0,
                                                  IPEColorTransformRegLength,
                                                  reinterpret_cast<UINT32*>(m_pRegCmd));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write command buffer");
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECST12Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECST12Titan17x::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)
{
    CamxResult               result  = CamxResultSuccess;
    CST12UnpackedField*      pData   = static_cast<CST12UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutputVal);

    if ((NULL != pData) && (NULL != m_pRegCmd))
    {
        IPEColorTransformRegCmd* pRegCmd = static_cast<IPEColorTransformRegCmd*>(m_pRegCmd);

        pRegCmd->configChannel0.coefficient0.bitfields.MATRIX_M00 = pData->m00;
        pRegCmd->configChannel0.coefficient0.bitfields.MATRIX_M01 = pData->m01;
        pRegCmd->configChannel0.coefficient1.bitfields.MATRIX_M02 = pData->m02;
        pRegCmd->configChannel0.offset.bitfields.OFFSET_O0        = pData->o0;
        pRegCmd->configChannel0.offset.bitfields.OFFSET_S0        = pData->s0;
        pRegCmd->configChannel0.clamp.bitfields.CLAMP_MIN         = pData->c00;
        pRegCmd->configChannel0.clamp.bitfields.CLAMP_MAX         = pData->c01;

        pRegCmd->configChannel1.coefficient0.bitfields.MATRIX_M10 = pData->m10;
        pRegCmd->configChannel1.coefficient0.bitfields.MATRIX_M11 = pData->m11;
        pRegCmd->configChannel1.coefficient1.bitfields.MATRIX_M12 = pData->m12;
        pRegCmd->configChannel1.offset.bitfields.OFFSET_O1        = pData->o1;
        pRegCmd->configChannel1.offset.bitfields.OFFSET_S1        = pData->s1;
        pRegCmd->configChannel1.clamp.bitfields.CLAMP_MIN         = pData->c10;
        pRegCmd->configChannel1.clamp.bitfields.CLAMP_MAX         = pData->c11;

        pRegCmd->configChannel2.coefficient0.bitfields.MATRIX_M20 = pData->m20;
        pRegCmd->configChannel2.coefficient0.bitfields.MATRIX_M21 = pData->m21;
        pRegCmd->configChannel2.coefficient1.bitfields.MATRIX_M22 = pData->m22;
        pRegCmd->configChannel2.offset.bitfields.OFFSET_O2        = pData->o2;
        pRegCmd->configChannel2.offset.bitfields.OFFSET_S2        = pData->s2;
        pRegCmd->configChannel2.clamp.bitfields.CLAMP_MIN         = pData->c20;
        pRegCmd->configChannel2.clamp.bitfields.CLAMP_MAX         = pData->c21;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input data is pData %p m_pRegCmd %p", pData, m_pRegCmd);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECST12Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECST12Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECST12Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECST12Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    IPETuningMetadata* pIPETuningMetadata = static_cast<IPETuningMetadata*>(pInputData->pIPETuningMetadata);
    IpeIQSettings*     pIPEIQSettings     = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);

    IPEColorTransformRegCmd* pRegCmd      = static_cast<IPEColorTransformRegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != pRegCmd))
    {
        CAMX_STATIC_ASSERT(sizeof(IPEColorTransformRegCmd) ==
            sizeof(pIPETuningMetadata->IPETuningMetadata17x.IPECSTData.CSTConfig));

        Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata17x.IPECSTData.CSTConfig,
                      pRegCmd,
                      sizeof(IPEColorTransformRegCmd));

        if (TRUE == pIPEIQSettings->colorTransformParameters.moduleCfg.EN)
        {
            DebugDataTagID dataTagID;
            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                dataTagID = DebugDataTagID::TuningIPECST12Register;
            }
            else
            {
                dataTagID = DebugDataTagID::TuningIPECST12RegisterOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                dataTagID,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pIPETuningMetadata->IPETuningMetadata17x.IPECSTData.CSTConfig),
                &pIPETuningMetadata->IPETuningMetadata17x.IPECSTData.CSTConfig,
                sizeof(pIPETuningMetadata->IPETuningMetadata17x.IPECSTData.CSTConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECST12Titan17x::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPECST12Titan17x::GetRegSize()
{
    return sizeof(IPEColorTransformRegCmd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECST12Titan17x::~IPECST12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPECST12Titan17x::~IPECST12Titan17x()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECST12Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPECST12Titan17x::DumpRegConfig()
{
    if (NULL != m_pRegCmd)
    {
        IPEColorTransformRegCmd* pRegCmd = static_cast<IPEColorTransformRegCmd*>(m_pRegCmd);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ColorTransform.Ch0.coefficient0  = %x\n", pRegCmd->configChannel0.coefficient0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ColorTransform.Ch0.coefficient1  = %x\n", pRegCmd->configChannel0.coefficient1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ColorTransform.Ch0.offset        = %x\n", pRegCmd->configChannel0.offset);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ColorTransform.Ch0.clamp         = %x\n", pRegCmd->configChannel0.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ColorTransform.Ch1.coefficient0  = %x\n", pRegCmd->configChannel1.coefficient0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ColorTransform.Ch1.coefficient1  = %x\n", pRegCmd->configChannel1.coefficient1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ColorTransform.Ch1.offset        = %x\n", pRegCmd->configChannel1.offset);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ColorTransform.Ch1.clamp         = %x\n", pRegCmd->configChannel1.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ColorTransform.Ch2.coefficient0  = %x\n", pRegCmd->configChannel2.coefficient0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ColorTransform.Ch2.coefficient1  = %x\n", pRegCmd->configChannel2.coefficient1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ColorTransform.Ch2.offset        = %x\n", pRegCmd->configChannel2.offset);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "ColorTransform.Ch2.clamp         = %x\n", pRegCmd->configChannel2.clamp);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Ch0 matrix coeff [0x%x] ", pRegCmd->configChannel0.coefficient0.u32All);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Error m_pRegCmd is NULL");
    }
}

CAMX_NAMESPACE_END
