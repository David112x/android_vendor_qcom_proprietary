////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpscc13titan17x.cpp
/// @brief CAMXBPSCC13TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpscc13titan17x.h"
#include "cc13setting.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief Color Correction register Configuration
struct BPSCC13RegConfig
{
    BPS_BPS_0_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_A_CFG_0  coefficientAConfig0; ///< coefficient A Config 0
    BPS_BPS_0_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_A_CFG_1  coefficientAConfig1; ///< coefficient A Config 1
    BPS_BPS_0_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_B_CFG_0  coefficientBConfig0; ///< coefficient B Config 0
    BPS_BPS_0_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_B_CFG_1  coefficientBConfig1; ///< coefficient B Config 1
    BPS_BPS_0_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_C_CFG_0  coefficientCConfig0; ///< coefficient C Config 0
    BPS_BPS_0_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_C_CFG_1  coefficientCConfig1; ///< coefficient C Config 1
    BPS_BPS_0_CLC_COLOR_CORRECT_COLOR_CORRECT_OFFSET_K_CFG_0 offsetKConfig0;      ///< Offset K Config 0
    BPS_BPS_0_CLC_COLOR_CORRECT_COLOR_CORRECT_OFFSET_K_CFG_1 offsetKConfig1;      ///< Offset K Config 1
    BPS_BPS_0_CLC_COLOR_CORRECT_COLOR_CORRECT_SHIFT_M_CFG    shiftMConfig;        ///< Shift M Config
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 BPSCC13RegConfigLengthDWord = sizeof(BPSCC13RegConfig) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCC13Titan17x::BPSCC13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSCC13Titan17x::BPSCC13Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCC13Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCC13Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSCC13Titan17x* pHWSetting = CAMX_NEW BPSCC13Titan17x;

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
/// BPSCC13Titan17x::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCC13Titan17x::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pRegCmd = CAMX_CALLOC(sizeof(BPSCC13RegConfig));

    if (NULL != m_pRegCmd)
    {
        SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSCC13RegConfigLengthDWord));
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for register configuration!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCC13Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCC13Titan17x::CreateCmdList(
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
                                              regBPS_BPS_0_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_A_CFG_0,
                                              BPSCC13RegConfigLengthDWord,
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
// BPSCC13Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCC13Titan17x::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*        pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings*       pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->colorCorrectParameters.moduleCfg.EN = moduleEnable;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCC13Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCC13Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*     pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSCC13RegConfig*  pRegCmd            = static_cast<BPSCC13RegConfig*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSCC13RegConfig) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSCCData.colorCorrectionConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSCCData.colorCorrectionConfig,
                        pRegCmd,
                        sizeof(BPSCC13RegConfig));

        if (TRUE == pBPSIQSettings->colorCorrectParameters.moduleCfg.EN)
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSCCRegister,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata17x.BPSCCData.colorCorrectionConfig),
                &pBPSTuningMetadata->BPSTuningMetadata17x.BPSCCData.colorCorrectionConfig,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSCCData.colorCorrectionConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCC13Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSCC13Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult         result  = CamxResultSuccess;
    CC13UnpackedField* pData   = static_cast<CC13UnpackedField*>(pInput);
    BPSCC13RegConfig*  pRegCmd = static_cast<BPSCC13RegConfig*>(m_pRegCmd);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        pRegCmd->coefficientAConfig0.bitfields.MATRIX_A0 = pData->c0;
        pRegCmd->coefficientAConfig0.bitfields.MATRIX_A1 = pData->c3;
        pRegCmd->coefficientAConfig1.bitfields.MATRIX_A2 = pData->c6;
        pRegCmd->coefficientBConfig0.bitfields.MATRIX_B0 = pData->c1;
        pRegCmd->coefficientBConfig0.bitfields.MATRIX_B1 = pData->c4;
        pRegCmd->coefficientBConfig1.bitfields.MATRIX_B2 = pData->c7;
        pRegCmd->coefficientCConfig0.bitfields.MATRIX_C0 = pData->c2;
        pRegCmd->coefficientCConfig0.bitfields.MATRIX_C1 = pData->c5;
        pRegCmd->coefficientCConfig1.bitfields.MATRIX_C2 = pData->c8;
        pRegCmd->offsetKConfig0.bitfields.OFFSET_K0      = pData->k0;
        pRegCmd->offsetKConfig0.bitfields.OFFSET_K1      = pData->k1;
        pRegCmd->offsetKConfig1.bitfields.OFFSET_K2      = pData->k2;
        pRegCmd->shiftMConfig.bitfields.M_PARAM          = pData->qfactor;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Input data is pData %p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCC13Titan17x::~BPSCC13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSCC13Titan17x::~BPSCC13Titan17x()
{
    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSCC13Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSCC13Titan17x::DumpRegConfig()
{
    BPSCC13RegConfig* pRegCmd = static_cast<BPSCC13RegConfig*>(m_pRegCmd);

    if (NULL != pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "coefficient A Config 0 [0x%x]", pRegCmd->coefficientAConfig0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "coefficient A Config 1 [0x%x]", pRegCmd->coefficientAConfig1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "coefficient B Config 0 [0x%x]", pRegCmd->coefficientBConfig0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "coefficient B Config 1 [0x%x]", pRegCmd->coefficientBConfig1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "coefficient C Config 0 [0x%x]", pRegCmd->coefficientCConfig0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "coefficient C Config 1 [0x%x]", pRegCmd->coefficientCConfig1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Offset K Config 0      [0x%x]", pRegCmd->offsetKConfig0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Offset K Config 1      [0x%x]", pRegCmd->offsetKConfig1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Shift M Config         [0x%x]", pRegCmd->shiftMConfig);
    }
}

CAMX_NAMESPACE_END
