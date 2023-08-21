////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipecc13titan480.cpp
/// @brief CAMXIPECC13TITAN17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "cc13setting.h"
#include "camxutils.h"
#include "camxipecc13titan480.h"
#include "camxipecolorcorrection13.h"
#include "camxispiqmodule.h"
#include "titan480_ipe.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

// @brief: This structure contains Color Correction registers programmed by software
struct IPECC13RegConfig
{
    IPE_IPE_0_PPS_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_A_CFG_0     greenCoefficients0; ///< Coefficents for input green
    IPE_IPE_0_PPS_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_A_CFG_1     greenCoefficients1; ///< Coefficents for input green
    IPE_IPE_0_PPS_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_B_CFG_0     blueCoefficients0;  ///< Coefficents for input blue
    IPE_IPE_0_PPS_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_B_CFG_1     blueCoefficients1;  ///< Coefficents for input blue
    IPE_IPE_0_PPS_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_C_CFG_0     redCoefficients0;   ///< Coefficents for input red
    IPE_IPE_0_PPS_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_C_CFG_1     redCoefficients1;   ///< Coefficents for input red
    IPE_IPE_0_PPS_CLC_COLOR_CORRECT_COLOR_CORRECT_OFFSET_K_CFG_0    offsetParam0;       ///< Addition offset params for outputs
    IPE_IPE_0_PPS_CLC_COLOR_CORRECT_COLOR_CORRECT_OFFSET_K_CFG_1    offsetParam1;       ///< Addition offset params for outputs
    IPE_IPE_0_PPS_CLC_COLOR_CORRECT_COLOR_CORRECT_SHIFT_M_CFG       qFactor;            ///< Right shift for Q factor adjustment
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IPEColorCorrectionRegLength = sizeof(IPECC13RegConfig) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECC13Titan480::IPECC13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPECC13Titan480::IPECC13Titan480()
{
    m_pRegCmd = CAMX_CALLOC(sizeof(IPECC13RegConfig));

    if (NULL != m_pRegCmd)
    {
        // Hardcode initial value for all the registers
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate memory for Cmd Buffer");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECC13Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECC13Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData)                                 &&
        (NULL != pInputData->pipelineIPEData.ppIPECmdBuffer) &&
        (NULL != m_pRegCmd))
    {
        pCmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPostLTM];

        if (NULL != pCmdBuffer)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIPE_IPE_0_PPS_CLC_COLOR_CORRECT_COLOR_CORRECT_COEFF_A_CFG_0,
                                                  IPEColorCorrectionRegLength,
                                                  reinterpret_cast<UINT32*>(m_pRegCmd));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write command buffer");
            }
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Can't Find Cmd Buffer");
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
// IPECC13Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECC13Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutput)
{
    CamxResult           result  = CamxResultSuccess;
    CC13UnpackedField*   pData   = static_cast<CC13UnpackedField*>(pInput);
    IPECC13RegConfig*    pRegCmd = NULL;

    CAMX_UNREFERENCED_PARAM(pOutput);

    if ((NULL != m_pRegCmd) && (NULL != pData))
    {

        pRegCmd = static_cast<IPECC13RegConfig*>(m_pRegCmd);

        pRegCmd->greenCoefficients0.bitfields.MATRIX_A0 = pData->c0;
        pRegCmd->greenCoefficients0.bitfields.MATRIX_A1 = pData->c3;
        pRegCmd->greenCoefficients1.bitfields.MATRIX_A2 = pData->c6;
        pRegCmd->blueCoefficients0.bitfields.MATRIX_B0  = pData->c1;
        pRegCmd->blueCoefficients0.bitfields.MATRIX_B1  = pData->c4;
        pRegCmd->blueCoefficients1.bitfields.MATRIX_B2  = pData->c7;
        pRegCmd->redCoefficients0.bitfields.MATRIX_C0   = pData->c2;
        pRegCmd->redCoefficients0.bitfields.MATRIX_C1   = pData->c5;
        pRegCmd->redCoefficients1.bitfields.MATRIX_C2   = pData->c8;
        pRegCmd->offsetParam0.bitfields.OFFSET_K0       = pData->k0;
        pRegCmd->offsetParam0.bitfields.OFFSET_K1       = pData->k1;
        pRegCmd->offsetParam1.bitfields.OFFSET_K2       = pData->k2;
        pRegCmd->qFactor.bitfields.M_PARAM              = pData->qfactor;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Abort! NULL data pointer  pData %p m_pRegCmd ", pData, m_pRegCmd);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECC13Titan480::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPECC13Titan480::GetRegSize()
{
    return sizeof(IPECC13RegConfig);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECC13Titan480::~IPECC13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPECC13Titan480::~IPECC13Titan480()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECC13Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPECC13Titan480::DumpRegConfig()
{
    IPECC13RegConfig* pRegCmd = NULL;

    pRegCmd = static_cast<IPECC13RegConfig*>(m_pRegCmd);
    if (NULL != m_pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Green coeefficients0 = %x\n", pRegCmd->greenCoefficients0.bits);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Green coeefficients1 = %x\n", pRegCmd->greenCoefficients1.bits);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Blue  coeefficients0 = %x\n", pRegCmd->blueCoefficients0.bits);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Blue  coeefficients1 = %x\n", pRegCmd->blueCoefficients1.bits);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Red   coeefficients0 = %x\n", pRegCmd->redCoefficients0.bits);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Red   coeefficients1 = %x\n", pRegCmd->redCoefficients1.bits);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Offset Param0        = %x\n", pRegCmd->offsetParam0.bits);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Offset Param1        = %x\n", pRegCmd->offsetParam1.bits);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Q Factor             = %x\n", pRegCmd->qFactor.bits);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Error m_pRegCmd is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECC13Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECC13Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    IPETuningMetadata* pIPETuningMetadata = static_cast<IPETuningMetadata*>(pInputData->pIPETuningMetadata);
    IpeIQSettings*     pIPEIQSettings     = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
    IPECC13RegConfig*  pRegCmd            = static_cast<IPECC13RegConfig*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != pRegCmd))
    {
        CAMX_STATIC_ASSERT(sizeof(IPECC13RegConfig) ==
            sizeof(pIPETuningMetadata->IPETuningMetadata480.IPECCData.colorCorrectionConfig));

        Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata480.IPECCData.colorCorrectionConfig,
                      pRegCmd,
                      sizeof(IPECC13RegConfig));

        if (TRUE == pIPEIQSettings->colorCorrectParameters.moduleCfg.EN)
        {
            DebugDataTagID dataTagID;
            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                dataTagID = DebugDataTagID::TuningIPECC13Register;
            }
            else
            {
                dataTagID = DebugDataTagID::TuningIPECC13RegisterOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                dataTagID,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pIPETuningMetadata->IPETuningMetadata480.IPECCData.colorCorrectionConfig),
                &pIPETuningMetadata->IPETuningMetadata480.IPECCData.colorCorrectionConfig,
                sizeof(pIPETuningMetadata->IPETuningMetadata480.IPECCData.colorCorrectionConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECC13Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECC13Titan480::SetupRegisterSetting(
    VOID* pData)
{
    CamxResult        result     = CamxResultSuccess;
    ManualSetting*    pInput     = NULL;
    IPECC13RegConfig* pRegCmd    = NULL;
    FLOAT*            pManualCCM = NULL;
    UINT              arraySize  = 0;

    if ((NULL != pData) && (NULL != m_pRegCmd))
    {
        pInput  = static_cast<ManualSetting*>(pData);
        pRegCmd = static_cast<IPECC13RegConfig*>(m_pRegCmd);

        pManualCCM = pInput->pManualCCM;
        arraySize  = pInput->arraySize;

        if (TRUE == pInput->manualCCMEnable)
        {
            pRegCmd->greenCoefficients0.bitfields.MATRIX_A0 = Utils::FloatToQNumber(pManualCCM[1 * arraySize + 1], Q7);
            pRegCmd->greenCoefficients0.bitfields.MATRIX_A1 = Utils::FloatToQNumber(pManualCCM[2 * arraySize + 1], Q7);
            pRegCmd->greenCoefficients1.bitfields.MATRIX_A2 = Utils::FloatToQNumber(pManualCCM[0 * arraySize + 1], Q7);

            pRegCmd->blueCoefficients0.bitfields.MATRIX_B0  = Utils::FloatToQNumber(pManualCCM[1 * arraySize + 2], Q7);
            pRegCmd->blueCoefficients0.bitfields.MATRIX_B1  = Utils::FloatToQNumber(pManualCCM[2 * arraySize + 2], Q7);
            pRegCmd->blueCoefficients1.bitfields.MATRIX_B2  = Utils::FloatToQNumber(pManualCCM[0 * arraySize + 2], Q7);

            pRegCmd->redCoefficients0.bitfields.MATRIX_C0   = Utils::FloatToQNumber(pManualCCM[1 * arraySize + 0], Q7);
            pRegCmd->redCoefficients0.bitfields.MATRIX_C1   = Utils::FloatToQNumber(pManualCCM[2 * arraySize + 0], Q7);
            pRegCmd->redCoefficients1.bitfields.MATRIX_C2   = Utils::FloatToQNumber(pManualCCM[0 * arraySize + 0], Q7);

            pRegCmd->offsetParam0.bitfields.OFFSET_K0  = 0;
            pRegCmd->offsetParam0.bitfields.OFFSET_K1  = 0;
            pRegCmd->offsetParam1.bitfields.OFFSET_K2  = 0;
            pRegCmd->qFactor.bitfields.M_PARAM = 0;
        }
        else if (TRUE == pInput->manualAWBCCMEnable)
        {

            AWBCCMParams* pAWBCCM = pInput->pAWBCCM;
            UINT32        mFactor = 0;
            UINT32        qFactor = (NULL != pInput->pChromatix) ? pInput->pChromatix->chromatix_cc13_reserve.q_factor : 0;
            mFactor               = 1 << (qFactor + CC13_Q_FACTOR);
            // Consume CCM from AWB algo
            // Chromatix stores the coeffs in RGB order whereas VFE stores the coeffs in GBR order.Hence c0 maps to M[1][1]
            pRegCmd->greenCoefficients0.bitfields.MATRIX_A0 =
                Utils::RoundFLOAT((static_cast<FLOAT>(mFactor)) * pAWBCCM->CCM[1][1]);
            pRegCmd->greenCoefficients0.bitfields.MATRIX_A1 =
                Utils::RoundFLOAT((static_cast<FLOAT>(mFactor)) * pAWBCCM->CCM[2][1]);
            pRegCmd->greenCoefficients1.bitfields.MATRIX_A2 =
                Utils::RoundFLOAT((static_cast<FLOAT>(mFactor)) * pAWBCCM->CCM[0][1]);
            pRegCmd->blueCoefficients0.bitfields.MATRIX_B0  =
                Utils::RoundFLOAT((static_cast<FLOAT>(mFactor)) * pAWBCCM->CCM[1][2]);
            pRegCmd->blueCoefficients0.bitfields.MATRIX_B1  =
                Utils::RoundFLOAT((static_cast<FLOAT>(mFactor)) * pAWBCCM->CCM[2][2]);
            pRegCmd->blueCoefficients1.bitfields.MATRIX_B2  =
                Utils::RoundFLOAT((static_cast<FLOAT>(mFactor)) * pAWBCCM->CCM[0][2]);
            pRegCmd->redCoefficients0.bitfields.MATRIX_C0   =
                Utils::RoundFLOAT((static_cast<FLOAT>(mFactor)) * pAWBCCM->CCM[1][0]);
            pRegCmd->redCoefficients0.bitfields.MATRIX_C1   =
                Utils::RoundFLOAT((static_cast<FLOAT>(mFactor)) * pAWBCCM->CCM[2][0]);
            pRegCmd->redCoefficients1.bitfields.MATRIX_C2   =
                Utils::RoundFLOAT((static_cast<FLOAT>(mFactor)) * pAWBCCM->CCM[0][0]);
            pRegCmd->offsetParam0.bitfields.OFFSET_K0       = Utils::RoundFLOAT(pAWBCCM->CCMOffset[1]);
            pRegCmd->offsetParam0.bitfields.OFFSET_K1       = Utils::RoundFLOAT(pAWBCCM->CCMOffset[2]);
            pRegCmd->offsetParam1.bitfields.OFFSET_K2       = Utils::RoundFLOAT(pAWBCCM->CCMOffset[0]);
            pRegCmd->qFactor.bitfields.M_PARAM              = qFactor;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "NULL input: pData = %p m_pRegCmd = %p ", pData, m_pRegCmd);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECC13Titan480::SetupInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPECC13Titan480::SetupInternalData(
    VOID* pData)
{
    CamxResult result = CamxResultSuccess;

    ISPInternalData*  pCalculatedData = NULL;
    IPECC13RegConfig* pRegCmd         = NULL;

    if ((NULL != pData)&& (NULL != m_pRegCmd))
    {
        pRegCmd = static_cast<IPECC13RegConfig*>(m_pRegCmd);

        pCalculatedData = static_cast<ISPInternalData*>(pData);

        pCalculatedData->CCTransformMatrix[0][0].numerator   = Utils::RoundFLOAT(static_cast<FLOAT>(
            Utils::SignExtendQnumber(pRegCmd->redCoefficients1.bitfields.MATRIX_C2, 12)));
        pCalculatedData->CCTransformMatrix[0][0].denominator = Q7;
        pCalculatedData->CCTransformMatrix[0][1].numerator   = Utils::RoundFLOAT(static_cast<FLOAT>(
            Utils::SignExtendQnumber(pRegCmd->greenCoefficients1.bitfields.MATRIX_A2, 12)));
        pCalculatedData->CCTransformMatrix[0][1].denominator = Q7;
        pCalculatedData->CCTransformMatrix[0][2].numerator   = Utils::RoundFLOAT(static_cast<FLOAT>(
            Utils::SignExtendQnumber(pRegCmd->blueCoefficients1.bitfields.MATRIX_B2, 12)));
        pCalculatedData->CCTransformMatrix[0][2].denominator = Q7;

        pCalculatedData->CCTransformMatrix[1][0].numerator   = Utils::RoundFLOAT(static_cast<FLOAT>(
            Utils::SignExtendQnumber(pRegCmd->redCoefficients0.bitfields.MATRIX_C0, 12)));
        pCalculatedData->CCTransformMatrix[1][0].denominator = Q7;
        pCalculatedData->CCTransformMatrix[1][1].numerator   = Utils::RoundFLOAT(static_cast<FLOAT>(
            Utils::SignExtendQnumber(pRegCmd->greenCoefficients0.bitfields.MATRIX_A0, 12)));
        pCalculatedData->CCTransformMatrix[1][1].denominator = Q7;
        pCalculatedData->CCTransformMatrix[1][2].numerator   = Utils::RoundFLOAT(static_cast<FLOAT>(
            Utils::SignExtendQnumber(pRegCmd->blueCoefficients0.bitfields.MATRIX_B0, 12)));
        pCalculatedData->CCTransformMatrix[1][2].denominator = Q7;

        pCalculatedData->CCTransformMatrix[2][0].numerator   = Utils::RoundFLOAT(static_cast<FLOAT>(
            Utils::SignExtendQnumber(pRegCmd->redCoefficients0.bitfields.MATRIX_C1, 12)));
        pCalculatedData->CCTransformMatrix[2][0].denominator = Q7;
        pCalculatedData->CCTransformMatrix[2][1].numerator   = Utils::RoundFLOAT(static_cast<FLOAT>(
           Utils::SignExtendQnumber(pRegCmd->greenCoefficients0.bitfields.MATRIX_A1, 12)));
        pCalculatedData->CCTransformMatrix[2][1].denominator = Q7;
        pCalculatedData->CCTransformMatrix[2][2].numerator   = Utils::RoundFLOAT(static_cast<FLOAT>(
           Utils::SignExtendQnumber(pRegCmd->blueCoefficients0.bitfields.MATRIX_B1, 12)));
        pCalculatedData->CCTransformMatrix[2][2].denominator = Q7;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "NULL Input Buffer");
    }

    return result;
};

CAMX_NAMESPACE_END
