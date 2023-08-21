////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeasf30titan17x.cpp
/// @brief IPEASF30Titan17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "titan170_ipe.h"
#include "asf30setting.h"
#include "camxutils.h"
#include "camxipeasf30titan17x.h"
#include "camxipeasf30.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief: ASF registers for top registers
struct IPEASF30TopRegisters
{
    // Top Registers:
    // |
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_CFG                       ASFLayer1Config;                  ///< L1 BPF output clamping
    IPE_IPE_0_PPS_CLC_ASF_LAYER_2_CFG                       ASFLayer2Config;                  ///< L2 BPF output clamping
    IPE_IPE_0_PPS_CLC_ASF_NORM_SCALE_CFG                    ASFNormalScaleConfig;             ///< L1/2 Act. BPF output norm.
    IPE_IPE_0_PPS_CLC_ASF_GAIN_CFG                          ASFGainConfig;                    ///< Mdn. Blend offset, Gain cap.
    IPE_IPE_0_PPS_CLC_ASF_NZ_FLAG                           ASFNegativeAndZeroFlag;           ///< Neg&Zero-out flgs Fltr Krnls
} CAMX_PACKED;

/// @brief: ASF registers for layer 1 config
struct IPEASF30Layer1Config
{
    //  |--> Configure Layer-1 Parameters:
    //  |    |--> Symmetric sharpening filter coefficients
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_SHARP_COEFF_CFG_0         ASFLayer1SharpnessCoeffConfig0;   ///< L1 SSF Coefficients 0,1
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_SHARP_COEFF_CFG_1         ASFLayer1SharpnessCoeffConfig1;   ///< L1 SSF Coefficients 2,3
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_SHARP_COEFF_CFG_2         ASFLayer1SharpnessCoeffConfig2;   ///< L1 SSF Coefficients 4,5
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_SHARP_COEFF_CFG_3         ASFLayer1SharpnessCoeffConfig3;   ///< L1 SSF Coefficients 6,7
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_SHARP_COEFF_CFG_4         ASFLayer1SharpnessCoeffConfig4;   ///< L1 SSF Coefficients 8,9
    //  |    |--> Low-pass filter coefficients
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_LPF_COEFF_CFG_0           ASFLayer1LPFCoeffConfig0;         ///< L1 LPF Coefficients 0,1
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_LPF_COEFF_CFG_1           ASFLayer1LPFCoeffConfig1;         ///< L1 LPF Coefficients 2,3
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_LPF_COEFF_CFG_2           ASFLayer1LPFCoeffConfig2;         ///< L1 LPF Coefficients 4,5
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_LPF_COEFF_CFG_3           ASFLayer1LPFCoeffConfig3;         ///< L1 LPF Coefficients 6,7
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_LPF_COEFF_CFG_4           ASFLayer1LPFCoeffConfig4;         ///< L1 LPF Coefficients 8,9
    //  |    |--> Band-pass filter coefficients
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_ACT_BPF_COEFF_CFG_0       ASFLayer1ActivityBPFCoeffConfig0; ///< L1 BPF Coefficients 0,1
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_ACT_BPF_COEFF_CFG_1       ASFLayer1ActivityBPFCoeffConfig1; ///< L1 BPF Coefficients 2,3
    IPE_IPE_0_PPS_CLC_ASF_LAYER_1_ACT_BPF_COEFF_CFG_2       ASFLayer1ActivityBPFCoeffConfig2; ///< L1 BPF Coefficients 4,5
} CAMX_PACKED;

/// @brief: ASF registers for layer 2 config
struct IPEASF30Layer2Config
{
    //  |--> Configure Layer-2 Parameters:
    //  |    |--> Symmetric sharpening filter coefficients
    IPE_IPE_0_PPS_CLC_ASF_LAYER_2_SHARP_COEFF_CFG_0         ASFLayer2SharpnessCoeffConfig0;   ///< L2 SSF Coefficients 0,1
    IPE_IPE_0_PPS_CLC_ASF_LAYER_2_SHARP_COEFF_CFG_1         ASFLayer2SharpnessCoeffConfig1;   ///< L2 SSF Coefficients 2,3
    IPE_IPE_0_PPS_CLC_ASF_LAYER_2_SHARP_COEFF_CFG_2         ASFLayer2SharpnessCoeffConfig2;   ///< L2 SSF Coefficients 4,5
    //  |    |--> Low-pass filter coefficients
    IPE_IPE_0_PPS_CLC_ASF_LAYER_2_LPF_COEFF_CFG_0           ASFLayer2LPFCoeffConfig0;         ///< L2 LPF Coefficients 0,1
    IPE_IPE_0_PPS_CLC_ASF_LAYER_2_LPF_COEFF_CFG_1           ASFLayer2LPFCoeffConfig1;         ///< L2 LPF Coefficients 2,3
    IPE_IPE_0_PPS_CLC_ASF_LAYER_2_LPF_COEFF_CFG_2           ASFLayer2LPFCoeffConfig2;         ///< L2 LPF Coefficients 4,5
    //  |    |--> Band-pass filter coefficients
    IPE_IPE_0_PPS_CLC_ASF_LAYER_2_ACT_BPF_COEFF_CFG_0       ASFLayer2ActivityBPFCoeffConfig0; ///< L2 BPF Coefficients 0,1
    IPE_IPE_0_PPS_CLC_ASF_LAYER_2_ACT_BPF_COEFF_CFG_1       ASFLayer2ActivityBPFCoeffConfig1; ///< L2 BPF Coefficients 2,3
    IPE_IPE_0_PPS_CLC_ASF_LAYER_2_ACT_BPF_COEFF_CFG_2       ASFLayer2ActivityBPFCoeffConfig2; ///< L2 BPF Coefficients 4,5
} CAMX_PACKED;

/// @brief: ASF registers for RNR config0
struct IPEASF30RNRConfig0
{
    //  |--> Configure RNR Parameters:
    //  |    |--> Radial Square Control Points table entries
    IPE_IPE_0_PPS_CLC_ASF_RNR_R_SQUARE_LUT_ENTRY_0          ASFRNRRSquareLUTEntry0;           ///< RS LUT 0 for radial noise
    IPE_IPE_0_PPS_CLC_ASF_RNR_R_SQUARE_LUT_ENTRY_1          ASFRNRRSquareLUTEntry1;           ///< RS LUT 1 for radial noise
    IPE_IPE_0_PPS_CLC_ASF_RNR_R_SQUARE_LUT_ENTRY_2          ASFRNRRSquareLUTEntry2;           ///< RS LUT 2 for radial noise
    //  |    |--> Radial Square Scale & Shift entries for radial noise reduction
    IPE_IPE_0_PPS_CLC_ASF_RNR_SQUARE_CFG_0                  ASFRNRSquareConfig0;              ///< RS Scale Radius&Shift values
} CAMX_PACKED;

/// @brief: ASF registers for RNR config1
struct IPEASF30RNRConfig1
{
    //  |    |--> Activity Correction Factor for Control Points
    IPE_IPE_0_PPS_CLC_ASF_RNR_ACTIVITY_CF_LUT_ENTRY_0       ASFRNRActivityCFLUTEntry0;        ///< RNR activity CF LUT 0
    IPE_IPE_0_PPS_CLC_ASF_RNR_ACTIVITY_CF_LUT_ENTRY_1       ASFRNRActivityCFLUTEntry1;        ///< RNR activity CF LUT 1
    IPE_IPE_0_PPS_CLC_ASF_RNR_ACTIVITY_CF_LUT_ENTRY_2       ASFRNRActivityCFLUTEntry2;        ///< RNR activity CF LUT 2
    //  |    |--> Table of Slopes between for Activity Control Points
    IPE_IPE_0_PPS_CLC_ASF_RNR_ACTIVITY_SLOPE_LUT_ENTRY_0    ASFRNRActivitySlopeLUTEntry0;     ///< RNR act.slps btwn CPs LUT0
    IPE_IPE_0_PPS_CLC_ASF_RNR_ACTIVITY_SLOPE_LUT_ENTRY_1    ASFRNRActivitySlopeLUTEntry1;     ///< RNR act.slps btwn CPs LUT1
    IPE_IPE_0_PPS_CLC_ASF_RNR_ACTIVITY_SLOPE_LUT_ENTRY_2    ASFRNRActivitySlopeLUTEntry2;     ///< RNR act.slps btwn CPs LUT2
    //  |    |--> Table of Shift value for Slopes between Activity Control Points
    IPE_IPE_0_PPS_CLC_ASF_RNR_ACTIVITY_CF_SHIFT_LUT_ENTRY_0 ASFRNRActivityCFShiftLUTEntry0;   ///< RNR shifts for slps LUT 0
    IPE_IPE_0_PPS_CLC_ASF_RNR_ACTIVITY_CF_SHIFT_LUT_ENTRY_1 ASFRNRActivityCFShiftLUTEntry1;   ///< RNR shifts for slps LUT 1
    IPE_IPE_0_PPS_CLC_ASF_RNR_ACTIVITY_CF_SHIFT_LUT_ENTRY_2 ASFRNRActivityCFShiftLUTEntry2;   ///< RNR shifts for slps LUT 2
    //  |    |--> Table of Correction Factor for Control Points
    IPE_IPE_0_PPS_CLC_ASF_RNR_GAIN_CF_LUT_ENTRY_0           ASFRNRGainCFLUTEntry0;            ///< RNR gain CF for CP LUT 0
    IPE_IPE_0_PPS_CLC_ASF_RNR_GAIN_CF_LUT_ENTRY_1           ASFRNRGainCFLUTEntry1;            ///< RNR gain CF for CP LUT 1
    IPE_IPE_0_PPS_CLC_ASF_RNR_GAIN_CF_LUT_ENTRY_2           ASFRNRGainCFLUTEntry2;            ///< RNR gain CF for CP LUT 2
    //  |    |--> Table of Slopes between for Gain Control Points
    IPE_IPE_0_PPS_CLC_ASF_RNR_GAIN_SLOPE_LUT_ENTRY_0        ASFRNRGainSlopeLUTEntry0;         ///< RNR gain slps btwn CPs LUT0
    IPE_IPE_0_PPS_CLC_ASF_RNR_GAIN_SLOPE_LUT_ENTRY_1        ASFRNRGainSlopeLUTEntry1;         ///< RNR gain slps btwn CPs LUT1
    IPE_IPE_0_PPS_CLC_ASF_RNR_GAIN_SLOPE_LUT_ENTRY_2        ASFRNRGainSlopeLUTEntry2;         ///< RNR gain slps btwn CPs LUT2
    //  |    |--> Table of Shift value for Slopes between Gain Control Points
    IPE_IPE_0_PPS_CLC_ASF_RNR_GAIN_CF_SHIFT_LUT_ENTRY_0     ASFRNRGainCFShiftLUTEntry0;       ///< G-CF shifts for slps LUT0
    IPE_IPE_0_PPS_CLC_ASF_RNR_GAIN_CF_SHIFT_LUT_ENTRY_1     ASFRNRGainCFShiftLUTEntry1;       ///< G-CF shifts for slps LUT1
    IPE_IPE_0_PPS_CLC_ASF_RNR_GAIN_CF_SHIFT_LUT_ENTRY_2     ASFRNRGainCFShiftLUTEntry2;       ///< G-CF shifts for slps LUT2
} CAMX_PACKED;

/// @brief: ASF registers for threshold limits config
struct IPEASF30ThresholdLimitsConfig
{
    //  |--> Configure thresholds limits:
    IPE_IPE_0_PPS_CLC_ASF_THRESHOLD_CFG_0                   ASFThresholdConfig0;              ///< Texture, Flat rgn detection
    IPE_IPE_0_PPS_CLC_ASF_THRESHOLD_CFG_1                   ASFThresholdConfig1;              ///< Smooth strength,Similarity Th
} CAMX_PACKED;

/// @brief: ASF registers for skin tone detection config
struct IPEASF30SkinToneDetectionConfig
{
    //  |--> Configure Skin Tone detection parameters:
    IPE_IPE_0_PPS_CLC_ASF_SKIN_CFG_0                        ASFSkinConfig0;                   ///< Luma(Y) Min, Hue Min & Max
    //  |    |--> boundary Prob. for Skin Tone, Quantization level for non skin, within pre-def. skin-tone regions
    IPE_IPE_0_PPS_CLC_ASF_SKIN_CFG_1                        ASFSkinConfig1;                   ///< Boundary Prob.,Qntzn.,LumaMax
    IPE_IPE_0_PPS_CLC_ASF_SKIN_CFG_2                        ASFSkinConfig2;                   ///< S Min, S Max on Y and on Luma
} CAMX_PACKED;

/// @brief: ASF registers for face detection config
struct IPEASF30FaceDetectionConfig
{
    //  |--> Configure Face Detection parameters:
    //  |    |-->  Faces detected (0 -> Whole image is part of a Face)
    IPE_IPE_0_PPS_CLC_ASF_FACE_CFG                          ASFFaceConfig;                    ///< # Faces detected 0-5,En/Di FD

    //  |    |-->  Face #1 center vertical co-ordinate
    IPE_IPE_0_PPS_CLC_ASF_FACE_1_CENTER_CFG                 ASFFace1CenterConfig;             ///< Face #1 Center vert. co-ord
    //  |    |-->  Face #1 transitional slope, slope shift, boundary and radius shift
    IPE_IPE_0_PPS_CLC_ASF_FACE_1_RADIUS_CFG                 ASFFace1RadiusConfig;             ///< F1 slp, slp.shft, bndr, r-shf
    IPE_IPE_0_PPS_CLC_ASF_FACE_2_CENTER_CFG                 ASFFace2CenterConfig;             ///< Face #2 Center vert. co-ord
    //  |    |-->  Face #2 transitional slope, slope shift, boundary and radius shift
    IPE_IPE_0_PPS_CLC_ASF_FACE_2_RADIUS_CFG                 ASFFace2RadiusConfig;             ///< F2 slp, slp.shft, bndr, r-shf
    IPE_IPE_0_PPS_CLC_ASF_FACE_3_CENTER_CFG                 ASFFace3CenterConfig;             ///< Face #3 Center vert. co-ord
    //  |    |-->  Face #3 transitional slope, slope shift, boundary and radius shift
    IPE_IPE_0_PPS_CLC_ASF_FACE_3_RADIUS_CFG                 ASFFace3RadiusConfig;             ///< F3 slp, slp.shft, bndr, r-shf
    IPE_IPE_0_PPS_CLC_ASF_FACE_4_CENTER_CFG                 ASFFace4CenterConfig;             ///< Face #4 Center vert. co-ord
    //  |    |-->  Face #4 transitional slope, slope shift, boundary and radius shift
    IPE_IPE_0_PPS_CLC_ASF_FACE_4_RADIUS_CFG                 ASFFace4RadiusConfig;             ///< F4 slp, slp.shft, bndr, r-shf
    IPE_IPE_0_PPS_CLC_ASF_FACE_5_CENTER_CFG                 ASFFace5CenterConfig;             ///< Face #5 Center vert. co-ord
    //  |    |-->  Face #5 transitional slope, slope shift, boundary and radius shift
    IPE_IPE_0_PPS_CLC_ASF_FACE_5_RADIUS_CFG                 ASFFace5RadiusConfig;             ///< F5 slp, slp.shft, bndr, r-shf
} CAMX_PACKED;

CAMX_END_PACKED

/// @brief: This structure contains ASF registers programmed by software
struct IPEASFRegCmd
{
    IPEASF30TopRegisters                ASFTopResigters;            ///< ASF top registers
    IPEASF30Layer1Config                layer1Config;               ///< ASF layer 1 config
    IPEASF30Layer2Config                layer2Config;               ///< ASF layer 2 config
    IPEASF30RNRConfig0                  rnrConfig0;                  ///< ASF rnr config0
    IPEASF30RNRConfig1                  rnrConfig1;                  ///< ASF rnr config1
    IPEASF30ThresholdLimitsConfig       thresholdLimitsConfig;      ///< ASF threshold limits config
    IPEASF30SkinToneDetectionConfig     skinToneDetectionConfig;    ///< ASF skin tone detection config
    IPEASF30FaceDetectionConfig         faceDetectionConfig;        ///< ASF face detection config
};

static const UINT32 IPEASFRegLength = sizeof(IPEASFRegCmd) / RegisterWidthInBytes;
CAMX_STATIC_ASSERT((65) == IPEASFRegLength);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30Titan17x::IPEASF30Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEASF30Titan17x::IPEASF30Titan17x()
{
    m_pRegCmd = CAMX_CALLOC(sizeof(IPEASFRegCmd));

    if (NULL == m_pRegCmd)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate memory for Cmd Buffer");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pASFHWSettingParams)
{
    CamxResult result                          = CamxResultSuccess;

    ISPInputData*       pInputData             = static_cast<ISPInputData*>(pSettingData);
    ASFHwSettingParams* pModuleHWSettingParams = reinterpret_cast<ASFHwSettingParams*>(pASFHWSettingParams);

    m_pLUTDMICmdBuffer                         = pModuleHWSettingParams->pLUTDMICmdBuffer;

    result = WriteLUTtoDMI(pInputData);
    if (CamxResultSuccess == result)
    {
        result = WriteConfigCmds(pInputData);
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "ASF configuration failed");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30Titan17x::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pOutputVal);

    ASF30UnpackedField*     pData   = static_cast<ASF30UnpackedField*>(pInput);

    if ((NULL != pData) && (NULL != m_pRegCmd))
    {
        // Fill up Registers
        IPEASFRegCmd* pRegCmd = static_cast<IPEASFRegCmd*>(m_pRegCmd);

        // ASFTopResigters
        pRegCmd->ASFTopResigters.ASFLayer1Config.bitfields.ACTIVITY_CLAMP_THRESHOLD   = pData->layer1ActivityClampThreshold;
        pRegCmd->ASFTopResigters.ASFLayer1Config.bitfields.CLAMP_LL                   = pData->layer1RegTL;
        pRegCmd->ASFTopResigters.ASFLayer1Config.bitfields.CLAMP_UL                   = pData->layer1RegTH;

        pRegCmd->ASFTopResigters.ASFLayer2Config.bitfields.ACTIVITY_CLAMP_THRESHOLD   = pData->layer2ActivityClampThreshold;
        pRegCmd->ASFTopResigters.ASFLayer2Config.bitfields.CLAMP_LL                   = pData->layer2RegTL;
        pRegCmd->ASFTopResigters.ASFLayer2Config.bitfields.CLAMP_UL                   = pData->layer2RegTH;

        pRegCmd->ASFTopResigters.ASFNormalScaleConfig.bitfields.LAYER_1_L2_NORM_EN    = pData->layer1L2NormEn;
        pRegCmd->ASFTopResigters.ASFNormalScaleConfig.bitfields.LAYER_1_NORM_SCALE    = pData->layer1NormScale;
        pRegCmd->ASFTopResigters.ASFNormalScaleConfig.bitfields.LAYER_2_L2_NORM_EN    = pData->layer2L2NormEn;
        pRegCmd->ASFTopResigters.ASFNormalScaleConfig.bitfields.LAYER_2_NORM_SCALE    = pData->layer2NormScale;

        pRegCmd->ASFTopResigters.ASFGainConfig.bitfields.GAMMA_CORRECTED_LUMA_TARGET  = pData->gammaCorrectedLumaTarget;
        pRegCmd->ASFTopResigters.ASFGainConfig.bitfields.GAIN_CAP                     = pData->gainCap;
        pRegCmd->ASFTopResigters.ASFGainConfig.bitfields.MEDIAN_BLEND_LOWER_OFFSET    = pData->medianBlendLowerOffset;
        pRegCmd->ASFTopResigters.ASFGainConfig.bitfields.MEDIAN_BLEND_UPPER_OFFSET    = pData->medianBlendUpperOffset;

        pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_0           = pData->nonZero[0];
        pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_1           = pData->nonZero[1];
        pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_2           = pData->nonZero[2];
        pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_3           = pData->nonZero[3];
        pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_4           = pData->nonZero[4];
        pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_5           = pData->nonZero[5];
        pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_6           = pData->nonZero[6];
        pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_7           = pData->nonZero[7];

        // layer1Config
        pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig0.bitfields.COEFF0         = pData->layer1CornerFilter[0];
        pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig0.bitfields.COEFF1         = pData->layer1CornerFilter[1];
        pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig1.bitfields.COEFF2         = pData->layer1CornerFilter[2];
        pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig1.bitfields.COEFF3         = pData->layer1CornerFilter[3];
        pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig2.bitfields.COEFF4         = pData->layer1CornerFilter[4];
        pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig2.bitfields.COEFF5         = pData->layer1CornerFilter[5];
        pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig3.bitfields.COEFF6         = pData->layer1CornerFilter[6];
        pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig3.bitfields.COEFF7         = pData->layer1CornerFilter[7];
        pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig4.bitfields.COEFF8         = pData->layer1CornerFilter[8];
        pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig4.bitfields.COEFF9         = pData->layer1CornerFilter[9];

        pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig0.bitfields.COEFF0               = pData->layer1LowPassFilter[0];
        pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig0.bitfields.COEFF1               = pData->layer1LowPassFilter[1];
        pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig1.bitfields.COEFF2               = pData->layer1LowPassFilter[2];
        pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig1.bitfields.COEFF3               = pData->layer1LowPassFilter[3];
        pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig2.bitfields.COEFF4               = pData->layer1LowPassFilter[4];
        pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig2.bitfields.COEFF5               = pData->layer1LowPassFilter[5];
        pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig3.bitfields.COEFF6               = pData->layer1LowPassFilter[6];
        pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig3.bitfields.COEFF7               = pData->layer1LowPassFilter[7];
        pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig4.bitfields.COEFF8               = pData->layer1LowPassFilter[8];
        pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig4.bitfields.COEFF9               = pData->layer1LowPassFilter[9];

        pRegCmd->layer1Config.ASFLayer1ActivityBPFCoeffConfig0.bitfields.COEFF0       = pData->layer1ActivityBPF[0];
        pRegCmd->layer1Config.ASFLayer1ActivityBPFCoeffConfig0.bitfields.COEFF1       = pData->layer1ActivityBPF[1];
        pRegCmd->layer1Config.ASFLayer1ActivityBPFCoeffConfig1.bitfields.COEFF2       = pData->layer1ActivityBPF[2];
        pRegCmd->layer1Config.ASFLayer1ActivityBPFCoeffConfig1.bitfields.COEFF3       = pData->layer1ActivityBPF[3];
        pRegCmd->layer1Config.ASFLayer1ActivityBPFCoeffConfig2.bitfields.COEFF4       = pData->layer1ActivityBPF[4];
        pRegCmd->layer1Config.ASFLayer1ActivityBPFCoeffConfig2.bitfields.COEFF5       = pData->layer1ActivityBPF[5];

        // layer2Config
        pRegCmd->layer2Config.ASFLayer2SharpnessCoeffConfig0.bitfields.COEFF0         = pData->layer2CornerFilter[0];
        pRegCmd->layer2Config.ASFLayer2SharpnessCoeffConfig0.bitfields.COEFF1         = pData->layer2CornerFilter[1];
        pRegCmd->layer2Config.ASFLayer2SharpnessCoeffConfig1.bitfields.COEFF2         = pData->layer2CornerFilter[2];
        pRegCmd->layer2Config.ASFLayer2SharpnessCoeffConfig1.bitfields.COEFF3         = pData->layer2CornerFilter[3];
        pRegCmd->layer2Config.ASFLayer2SharpnessCoeffConfig2.bitfields.COEFF4         = pData->layer2CornerFilter[4];
        pRegCmd->layer2Config.ASFLayer2SharpnessCoeffConfig2.bitfields.COEFF5         = pData->layer2CornerFilter[5];

        pRegCmd->layer2Config.ASFLayer2LPFCoeffConfig0.bitfields.COEFF0               = pData->layer2LowPassFilter[0];
        pRegCmd->layer2Config.ASFLayer2LPFCoeffConfig0.bitfields.COEFF1               = pData->layer2LowPassFilter[1];
        pRegCmd->layer2Config.ASFLayer2LPFCoeffConfig1.bitfields.COEFF2               = pData->layer2LowPassFilter[2];
        pRegCmd->layer2Config.ASFLayer2LPFCoeffConfig1.bitfields.COEFF3               = pData->layer2LowPassFilter[3];
        pRegCmd->layer2Config.ASFLayer2LPFCoeffConfig2.bitfields.COEFF4               = pData->layer2LowPassFilter[4];
        pRegCmd->layer2Config.ASFLayer2LPFCoeffConfig2.bitfields.COEFF5               = pData->layer2LowPassFilter[5];

        pRegCmd->layer2Config.ASFLayer2ActivityBPFCoeffConfig0.bitfields.COEFF0       = pData->layer2ActivityBPF[0];
        pRegCmd->layer2Config.ASFLayer2ActivityBPFCoeffConfig0.bitfields.COEFF1       = pData->layer2ActivityBPF[1];
        pRegCmd->layer2Config.ASFLayer2ActivityBPFCoeffConfig1.bitfields.COEFF2       = pData->layer2ActivityBPF[2];
        pRegCmd->layer2Config.ASFLayer2ActivityBPFCoeffConfig1.bitfields.COEFF3       = pData->layer2ActivityBPF[3];
        pRegCmd->layer2Config.ASFLayer2ActivityBPFCoeffConfig2.bitfields.COEFF4       = pData->layer2ActivityBPF[4];
        pRegCmd->layer2Config.ASFLayer2ActivityBPFCoeffConfig2.bitfields.COEFF5       = pData->layer2ActivityBPF[5];

        // rnrConfig
        pRegCmd->rnrConfig0.ASFRNRRSquareLUTEntry0.bitfields.R_SQUARE                  = pData->rSquareTable[0];
        pRegCmd->rnrConfig0.ASFRNRRSquareLUTEntry1.bitfields.R_SQUARE                  = pData->rSquareTable[1];
        pRegCmd->rnrConfig0.ASFRNRRSquareLUTEntry2.bitfields.R_SQUARE                  = pData->rSquareTable[2];

        pRegCmd->rnrConfig0.ASFRNRSquareConfig0.bitfields.R_SQUARE_SCALE               = pData->rSquareScale;
        pRegCmd->rnrConfig0.ASFRNRSquareConfig0.bitfields.R_SQUARE_SHIFT               = pData->rSquareShift;

        pRegCmd->rnrConfig1.ASFRNRActivityCFLUTEntry0.bitfields.ACTIVITY_CF            = pData->radialActivityCFTable[0];
        pRegCmd->rnrConfig1.ASFRNRActivityCFLUTEntry1.bitfields.ACTIVITY_CF            = pData->radialActivityCFTable[1];
        pRegCmd->rnrConfig1.ASFRNRActivityCFLUTEntry2.bitfields.ACTIVITY_CF            = pData->radialActivityCFTable[2];

        pRegCmd->rnrConfig1.ASFRNRActivitySlopeLUTEntry0.bitfields.ACTIVITY_SLOPE      = pData->radialActivitySlopeTable[0];
        pRegCmd->rnrConfig1.ASFRNRActivitySlopeLUTEntry1.bitfields.ACTIVITY_SLOPE      = pData->radialActivitySlopeTable[1];
        pRegCmd->rnrConfig1.ASFRNRActivitySlopeLUTEntry2.bitfields.ACTIVITY_SLOPE      = pData->radialActivitySlopeTable[2];

        pRegCmd->rnrConfig1.ASFRNRActivityCFShiftLUTEntry0.bitfields.ACTIVITY_CF_SHIFT = pData->radialActivityCFShift[0];
        pRegCmd->rnrConfig1.ASFRNRActivityCFShiftLUTEntry1.bitfields.ACTIVITY_CF_SHIFT = pData->radialActivityCFShift[1];
        pRegCmd->rnrConfig1.ASFRNRActivityCFShiftLUTEntry2.bitfields.ACTIVITY_CF_SHIFT = pData->radialActivityCFShift[2];

        pRegCmd->rnrConfig1.ASFRNRGainCFLUTEntry0.bitfields.GAIN_CF                    = pData->radialGainCFTable[0];
        pRegCmd->rnrConfig1.ASFRNRGainCFLUTEntry1.bitfields.GAIN_CF                    = pData->radialGainCFTable[1];
        pRegCmd->rnrConfig1.ASFRNRGainCFLUTEntry2.bitfields.GAIN_CF                    = pData->radialGainCFTable[2];

        pRegCmd->rnrConfig1.ASFRNRGainSlopeLUTEntry0.bitfields.GAIN_SLOPE              = pData->radialGainSlopeTable[0];
        pRegCmd->rnrConfig1.ASFRNRGainSlopeLUTEntry1.bitfields.GAIN_SLOPE              = pData->radialGainSlopeTable[1];
        pRegCmd->rnrConfig1.ASFRNRGainSlopeLUTEntry2.bitfields.GAIN_SLOPE              = pData->radialGainSlopeTable[2];

        pRegCmd->rnrConfig1.ASFRNRGainCFShiftLUTEntry0.bitfields.GAIN_CF_SHIFT         = pData->radialGainCFShift[0];
        pRegCmd->rnrConfig1.ASFRNRGainCFShiftLUTEntry1.bitfields.GAIN_CF_SHIFT         = pData->radialGainCFShift[1];
        pRegCmd->rnrConfig1.ASFRNRGainCFShiftLUTEntry2.bitfields.GAIN_CF_SHIFT         = pData->radialGainCFShift[2];

        // thresholdLimitsConfig
        pRegCmd->thresholdLimitsConfig.ASFThresholdConfig0.bitfields.FLAT_TH          = pData->flatThreshold;
        pRegCmd->thresholdLimitsConfig.ASFThresholdConfig0.bitfields.TEXTURE_TH       = pData->maxSmoothingClamp;
        pRegCmd->thresholdLimitsConfig.ASFThresholdConfig1.bitfields.SIMILARITY_TH    = pData->cornerThreshold;
        pRegCmd->thresholdLimitsConfig.ASFThresholdConfig1.bitfields.SMOOTH_STRENGTH  = pData->smoothingStrength;

        // skinToneDetectionConfig
        pRegCmd->skinToneDetectionConfig.ASFSkinConfig0.bitfields.H_MAX               = pData->maxHue;
        pRegCmd->skinToneDetectionConfig.ASFSkinConfig0.bitfields.H_MIN               = pData->minHue;
        pRegCmd->skinToneDetectionConfig.ASFSkinConfig0.bitfields.Y_MIN               = pData->minY;
        pRegCmd->skinToneDetectionConfig.ASFSkinConfig1.bitfields.Y_MAX               = pData->maxY;
        pRegCmd->skinToneDetectionConfig.ASFSkinConfig1.bitfields.BOUNDARY_PROB       = pData->boundaryProbability;
        pRegCmd->skinToneDetectionConfig.ASFSkinConfig1.bitfields.Q_NON_SKIN          = pData->nonskinQ;
        pRegCmd->skinToneDetectionConfig.ASFSkinConfig1.bitfields.Q_SKIN              = pData->skinQ;
        pRegCmd->skinToneDetectionConfig.ASFSkinConfig2.bitfields.SHY_MAX             = pData->maxShY;
        pRegCmd->skinToneDetectionConfig.ASFSkinConfig2.bitfields.SHY_MIN             = pData->minShY;
        pRegCmd->skinToneDetectionConfig.ASFSkinConfig2.bitfields.SMAX_PARA           = pData->paraSmax;
        pRegCmd->skinToneDetectionConfig.ASFSkinConfig2.bitfields.SMIN_PARA           = pData->paraSmin;

        // faceDetectionConfig
        pRegCmd->faceDetectionConfig.ASFFaceConfig.bitfields.FACE_EN                        = pData->faceEnable;
        pRegCmd->faceDetectionConfig.ASFFaceConfig.bitfields.FACE_NUM                       = pData->faceNum;

        pRegCmd->faceDetectionConfig.ASFFace1CenterConfig.bitfields.FACE_CENTER_HORIZONTAL  = pData->faceCenterHorizontal[0];
        pRegCmd->faceDetectionConfig.ASFFace1CenterConfig.bitfields.FACE_CENTER_VERTICAL    = pData->faceCenterVertical[0];
        pRegCmd->faceDetectionConfig.ASFFace1RadiusConfig.bitfields.FACE_RADIUS_BOUNDARY    = pData->faceRadiusBoundary[0];
        pRegCmd->faceDetectionConfig.ASFFace1RadiusConfig.bitfields.FACE_RADIUS_SHIFT       = pData->faceRadiusShift[0];
        pRegCmd->faceDetectionConfig.ASFFace1RadiusConfig.bitfields.FACE_RADIUS_SLOPE       = pData->faceRadiusSlope[0];
        pRegCmd->faceDetectionConfig.ASFFace1RadiusConfig.bitfields.FACE_SLOPE_SHIFT        = pData->faceSlopeShift[0];

        pRegCmd->faceDetectionConfig.ASFFace2CenterConfig.bitfields.FACE_CENTER_HORIZONTAL  = pData->faceCenterHorizontal[1];
        pRegCmd->faceDetectionConfig.ASFFace2CenterConfig.bitfields.FACE_CENTER_VERTICAL    = pData->faceCenterVertical[1];
        pRegCmd->faceDetectionConfig.ASFFace2RadiusConfig.bitfields.FACE_RADIUS_BOUNDARY    = pData->faceRadiusBoundary[1];
        pRegCmd->faceDetectionConfig.ASFFace2RadiusConfig.bitfields.FACE_RADIUS_SHIFT       = pData->faceRadiusShift[1];
        pRegCmd->faceDetectionConfig.ASFFace2RadiusConfig.bitfields.FACE_RADIUS_SLOPE       = pData->faceRadiusSlope[1];
        pRegCmd->faceDetectionConfig.ASFFace2RadiusConfig.bitfields.FACE_SLOPE_SHIFT        = pData->faceSlopeShift[1];

        pRegCmd->faceDetectionConfig.ASFFace3CenterConfig.bitfields.FACE_CENTER_HORIZONTAL  = pData->faceCenterHorizontal[2];
        pRegCmd->faceDetectionConfig.ASFFace3CenterConfig.bitfields.FACE_CENTER_VERTICAL    = pData->faceCenterVertical[2];
        pRegCmd->faceDetectionConfig.ASFFace3RadiusConfig.bitfields.FACE_RADIUS_BOUNDARY    = pData->faceRadiusBoundary[2];
        pRegCmd->faceDetectionConfig.ASFFace3RadiusConfig.bitfields.FACE_RADIUS_SHIFT       = pData->faceRadiusShift[2];
        pRegCmd->faceDetectionConfig.ASFFace3RadiusConfig.bitfields.FACE_RADIUS_SLOPE       = pData->faceRadiusSlope[2];
        pRegCmd->faceDetectionConfig.ASFFace3RadiusConfig.bitfields.FACE_SLOPE_SHIFT        = pData->faceSlopeShift[2];

        pRegCmd->faceDetectionConfig.ASFFace4CenterConfig.bitfields.FACE_CENTER_HORIZONTAL  = pData->faceCenterHorizontal[3];
        pRegCmd->faceDetectionConfig.ASFFace4CenterConfig.bitfields.FACE_CENTER_VERTICAL    = pData->faceCenterVertical[3];
        pRegCmd->faceDetectionConfig.ASFFace4RadiusConfig.bitfields.FACE_RADIUS_BOUNDARY    = pData->faceRadiusBoundary[3];
        pRegCmd->faceDetectionConfig.ASFFace4RadiusConfig.bitfields.FACE_RADIUS_SHIFT       = pData->faceRadiusShift[3];
        pRegCmd->faceDetectionConfig.ASFFace4RadiusConfig.bitfields.FACE_RADIUS_SLOPE       = pData->faceRadiusSlope[3];
        pRegCmd->faceDetectionConfig.ASFFace4RadiusConfig.bitfields.FACE_SLOPE_SHIFT        = pData->faceSlopeShift[3];

        pRegCmd->faceDetectionConfig.ASFFace5CenterConfig.bitfields.FACE_CENTER_HORIZONTAL  = pData->faceCenterHorizontal[4];
        pRegCmd->faceDetectionConfig.ASFFace5CenterConfig.bitfields.FACE_CENTER_VERTICAL    = pData->faceCenterVertical[4];
        pRegCmd->faceDetectionConfig.ASFFace5RadiusConfig.bitfields.FACE_RADIUS_BOUNDARY    = pData->faceRadiusBoundary[4];
        pRegCmd->faceDetectionConfig.ASFFace5RadiusConfig.bitfields.FACE_RADIUS_SHIFT       = pData->faceRadiusShift[4];
        pRegCmd->faceDetectionConfig.ASFFace5RadiusConfig.bitfields.FACE_RADIUS_SLOPE       = pData->faceRadiusSlope[4];
        pRegCmd->faceDetectionConfig.ASFFace5RadiusConfig.bitfields.FACE_SLOPE_SHIFT        = pData->faceSlopeShift[4];
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input data is NULL ");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    IPETuningMetadata* pIPETuningMetadata = static_cast<IPETuningMetadata*>(pInputData->pIPETuningMetadata);
    IpeIQSettings*     pIPEIQSettings     = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);;
    IPEASFRegCmd*      pRegCmd            = static_cast<IPEASFRegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != pRegCmd))
    {
        CAMX_STATIC_ASSERT(sizeof(IPEASFRegCmd) == sizeof(pIPETuningMetadata->IPETuningMetadata17x.IPEASFData.ASFConfig));

        Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata17x.IPEASFData.ASFConfig,
                      pRegCmd,
                      sizeof(IPEASFRegCmd));

        if (TRUE == pIPEIQSettings->asfParameters.moduleCfg.EN)
        {
            DebugDataTagID dataTagID;
            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                dataTagID = DebugDataTagID::TuningIPEASF30Register;
            }
            else
            {
                dataTagID = DebugDataTagID::TuningIPEASF30RegisterOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                dataTagID,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pIPETuningMetadata->IPETuningMetadata17x.IPEASFData.ASFConfig),
                &pIPETuningMetadata->IPETuningMetadata17x.IPEASFData.ASFConfig,
                sizeof(pIPETuningMetadata->IPETuningMetadata17x.IPEASFData.ASFConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30Titan17x::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPEASF30Titan17x::GetRegSize()
{
    return sizeof(IPEASFRegCmd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30Titan17x::~IPEASF30Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPEASF30Titan17x::~IPEASF30Titan17x()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPEASF30Titan17x::DumpRegConfig()
{
    if (NULL != m_pRegCmd)
    {
        IPEASFRegCmd* pRegCmd = static_cast<IPEASFRegCmd*>(m_pRegCmd);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LAYER 1 Activity Clamp Threshold = %x\n",
                         pRegCmd->ASFTopResigters.ASFLayer1Config.bitfields.ACTIVITY_CLAMP_THRESHOLD);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LAYER 1 Clamp Lower Bound        = %x\n",
                         pRegCmd->ASFTopResigters.ASFLayer1Config.bitfields.CLAMP_LL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LAYER 1 Clamp Upper Bound        = %x\n",
                         pRegCmd->ASFTopResigters.ASFLayer1Config.bitfields.CLAMP_UL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LAYER 2 Activity Clamp Threshold = %x\n",
                         pRegCmd->ASFTopResigters.ASFLayer2Config.bitfields.ACTIVITY_CLAMP_THRESHOLD);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LAYER 2 Clamp Lower Bound        = %x\n",
                         pRegCmd->ASFTopResigters.ASFLayer2Config.bitfields.CLAMP_LL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LAYER 2 Clamp Upper Bound        = %x\n",
                         pRegCmd->ASFTopResigters.ASFLayer2Config.bitfields.CLAMP_UL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LAYER 1 L2 NORM EN               = %x\n",
                         pRegCmd->ASFTopResigters.ASFNormalScaleConfig.bitfields.LAYER_1_L2_NORM_EN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LAYER 1 Norm Scale               = %x\n",
                         pRegCmd->ASFTopResigters.ASFNormalScaleConfig.bitfields.LAYER_1_NORM_SCALE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LAYER 2 L2 NORM EN               = %x\n",
                         pRegCmd->ASFTopResigters.ASFNormalScaleConfig.bitfields.LAYER_2_L2_NORM_EN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "LAYER 2 Norm Scale               = %x\n",
                         pRegCmd->ASFTopResigters.ASFNormalScaleConfig.bitfields.LAYER_2_L2_NORM_EN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Gain Cap                         = %x\n",
                         pRegCmd->ASFTopResigters.ASFGainConfig.bitfields.GAIN_CAP);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Gamma Corrected Luma Target      = %x\n",
                         pRegCmd->ASFTopResigters.ASFGainConfig.bitfields.GAMMA_CORRECTED_LUMA_TARGET);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Median Blend Lower Offset        = %x\n",
                         pRegCmd->ASFTopResigters.ASFGainConfig.bitfields.MEDIAN_BLEND_LOWER_OFFSET);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Median Blend Upper Offset        = %x\n",
                         pRegCmd->ASFTopResigters.ASFGainConfig.bitfields.MEDIAN_BLEND_UPPER_OFFSET);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NZ Flag 0                        = %x\n",
                         pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NZ Flag 1                        = %x\n",
                         pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NZ Flag 2                        = %x\n",
                         pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NZ Flag 3                        = %x\n",
                         pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NZ Flag 4                        = %x\n",
                         pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NZ Flag 5                        = %x\n",
                         pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NZ Flag 6                        = %x\n",
                         pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "NZ Flag 7                        = %x\n",
                         pRegCmd->ASFTopResigters.ASFNegativeAndZeroFlag.bitfields.NZ_FLAG_7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Sharping Coeff 0          = %x\n",
                         pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig0.bitfields.COEFF0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Sharping Coeff 1          = %x\n",
                         pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig0.bitfields.COEFF1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Sharping Coeff 2          = %x\n",
                         pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig1.bitfields.COEFF2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Sharping Coeff 3          = %x\n",
                         pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig1.bitfields.COEFF3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Sharping Coeff 4          = %x\n",
                         pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig2.bitfields.COEFF4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Sharping Coeff 5          = %x\n",
                         pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig2.bitfields.COEFF5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Sharping Coeff 6          = %x\n",
                         pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig3.bitfields.COEFF6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Sharping Coeff 7          = %x\n",
                         pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig3.bitfields.COEFF7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Sharping Coeff 8          = %x\n",
                         pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig4.bitfields.COEFF8);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Sharping Coeff 9          = %x\n",
                         pRegCmd->layer1Config.ASFLayer1SharpnessCoeffConfig4.bitfields.COEFF9);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Low Pass Filter Coeff 0   = %x\n",
                         pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig0.bitfields.COEFF0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Low Pass Filter Coeff 1   = %x\n",
                         pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig0.bitfields.COEFF1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Low Pass Filter Coeff 2   = %x\n",
                         pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig1.bitfields.COEFF2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Low Pass Filter Coeff 3   = %x\n",
                         pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig1.bitfields.COEFF3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Low Pass Filter Coeff 4   = %x\n",
                         pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig2.bitfields.COEFF4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Low Pass Filter Coeff 5   = %x\n",
                         pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig2.bitfields.COEFF5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Low Pass Filter Coeff 6   = %x\n",
                         pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig3.bitfields.COEFF6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Low Pass Filter Coeff 7   = %x\n",
                         pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig3.bitfields.COEFF7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Low Pass Filter Coeff 8   = %x\n",
                         pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig4.bitfields.COEFF8);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Low Pass Filter Coeff 9   = %x\n",
                         pRegCmd->layer1Config.ASFLayer1LPFCoeffConfig4.bitfields.COEFF9);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Band Pass Filter Coeff 0  = %x\n",
                         pRegCmd->layer1Config.ASFLayer1ActivityBPFCoeffConfig0.bitfields.COEFF0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Band Pass Filter Coeff 1  = %x\n",
                         pRegCmd->layer1Config.ASFLayer1ActivityBPFCoeffConfig0.bitfields.COEFF1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Band Pass Filter Coeff 2  = %x\n",
                         pRegCmd->layer1Config.ASFLayer1ActivityBPFCoeffConfig1.bitfields.COEFF2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Band Pass Filter Coeff 3  = %x\n",
                         pRegCmd->layer1Config.ASFLayer1ActivityBPFCoeffConfig1.bitfields.COEFF3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Band Pass Filter Coeff 4  = %x\n",
                         pRegCmd->layer1Config.ASFLayer1ActivityBPFCoeffConfig2.bitfields.COEFF4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer1 Band Pass Filter Coeff 5  = %x\n",
                         pRegCmd->layer1Config.ASFLayer1ActivityBPFCoeffConfig2.bitfields.COEFF5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Sharping Coeff 0          = %x\n",
                         pRegCmd->layer2Config.ASFLayer2SharpnessCoeffConfig0.bitfields.COEFF0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Sharping Coeff 1          = %x\n",
                         pRegCmd->layer2Config.ASFLayer2SharpnessCoeffConfig0.bitfields.COEFF1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Sharping Coeff 2          = %x\n",
                         pRegCmd->layer2Config.ASFLayer2SharpnessCoeffConfig1.bitfields.COEFF2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Sharping Coeff 3          = %x\n",
                         pRegCmd->layer2Config.ASFLayer2SharpnessCoeffConfig1.bitfields.COEFF3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Sharping Coeff 4          = %x\n",
                         pRegCmd->layer2Config.ASFLayer2SharpnessCoeffConfig2.bitfields.COEFF4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Sharping Coeff 5          = %x\n",
                         pRegCmd->layer2Config.ASFLayer2SharpnessCoeffConfig2.bitfields.COEFF5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Low Pass Filter Coeff 0   = %x\n",
                         pRegCmd->layer2Config.ASFLayer2LPFCoeffConfig0.bitfields.COEFF0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Low Pass Filter Coeff 1   = %x\n",
                         pRegCmd->layer2Config.ASFLayer2LPFCoeffConfig0.bitfields.COEFF1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Low Pass Filter Coeff 2   = %x\n",
                         pRegCmd->layer2Config.ASFLayer2LPFCoeffConfig1.bitfields.COEFF2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Low Pass Filter Coeff 3   = %x\n",
                         pRegCmd->layer2Config.ASFLayer2LPFCoeffConfig1.bitfields.COEFF3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Low Pass Filter Coeff 4   = %x\n",
                         pRegCmd->layer2Config.ASFLayer2LPFCoeffConfig2.bitfields.COEFF4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Low Pass Filter Coeff 5   = %x\n",
                         pRegCmd->layer2Config.ASFLayer2LPFCoeffConfig2.bitfields.COEFF5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Band Pass Filter Coeff 0  = %x\n",
                         pRegCmd->layer2Config.ASFLayer2ActivityBPFCoeffConfig0.bitfields.COEFF0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Band Pass Filter Coeff 1  = %x\n",
                         pRegCmd->layer2Config.ASFLayer2ActivityBPFCoeffConfig0.bitfields.COEFF1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Band Pass Filter Coeff 2  = %x\n",
                         pRegCmd->layer2Config.ASFLayer2ActivityBPFCoeffConfig1.bitfields.COEFF2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Band Pass Filter Coeff 3  = %x\n",
                         pRegCmd->layer2Config.ASFLayer2ActivityBPFCoeffConfig1.bitfields.COEFF3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Band Pass Filter Coeff 4  = %x\n",
                         pRegCmd->layer2Config.ASFLayer2ActivityBPFCoeffConfig2.bitfields.COEFF4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Layer2 Band Pass Filter Coeff 5  = %x\n",
                         pRegCmd->layer2Config.ASFLayer2ActivityBPFCoeffConfig2.bitfields.COEFF5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR LUT Entry 0 RSquare          = %x\n",
                         pRegCmd->rnrConfig0.ASFRNRRSquareLUTEntry0.bitfields.R_SQUARE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR LUT Entry 1 RSquare          = %x\n",
                         pRegCmd->rnrConfig0.ASFRNRRSquareLUTEntry1.bitfields.R_SQUARE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR LUT Entry 2 RSquare          = %x\n",
                         pRegCmd->rnrConfig0.ASFRNRRSquareLUTEntry2.bitfields.R_SQUARE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR RSquare Scale                = %x\n",
                         pRegCmd->rnrConfig0.ASFRNRSquareConfig0.bitfields.R_SQUARE_SCALE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR RSquare Shift                = %x\n",
                         pRegCmd->rnrConfig0.ASFRNRSquareConfig0.bitfields.R_SQUARE_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 0 Activity CF            = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRActivityCFLUTEntry0.bitfields.ACTIVITY_CF);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 1 Activity CF            = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRActivityCFLUTEntry1.bitfields.ACTIVITY_CF);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 2 Activity CF            = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRActivityCFLUTEntry2.bitfields.ACTIVITY_CF);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 0 Activity Slope         = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRActivitySlopeLUTEntry0.bitfields.ACTIVITY_SLOPE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 1 Activity Slope         = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRActivitySlopeLUTEntry1.bitfields.ACTIVITY_SLOPE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 2 Activity Slope         = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRActivitySlopeLUTEntry2.bitfields.ACTIVITY_SLOPE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 0 Activity CF Shift      = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRActivityCFShiftLUTEntry0.bitfields.ACTIVITY_CF_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 1 Activity CF Shift      = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRActivityCFShiftLUTEntry1.bitfields.ACTIVITY_CF_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 2 Activity CF Shift      = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRActivityCFShiftLUTEntry2.bitfields.ACTIVITY_CF_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 0 Gain CF                = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRGainCFLUTEntry0.bitfields.GAIN_CF);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 1 Gain CF                = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRGainCFLUTEntry1.bitfields.GAIN_CF);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 2 Gain CF                = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRGainCFLUTEntry2.bitfields.GAIN_CF);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 0 Gain Slope             = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRGainSlopeLUTEntry0.bitfields.GAIN_SLOPE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 1 Gain Slope             = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRGainSlopeLUTEntry1.bitfields.GAIN_SLOPE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 2 Gain Slope             = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRGainSlopeLUTEntry2.bitfields.GAIN_SLOPE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 0 Gain CF Shift          = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRGainCFShiftLUTEntry0.bitfields.GAIN_CF_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 1 Gain CF Shift          = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRGainCFShiftLUTEntry1.bitfields.GAIN_CF_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "RNR Lut 2 Gain CF Shift          = %x\n",
                         pRegCmd->rnrConfig1.ASFRNRGainCFShiftLUTEntry2.bitfields.GAIN_CF_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Flat Threshold                   = %x\n",
                         pRegCmd->thresholdLimitsConfig.ASFThresholdConfig0.bitfields.FLAT_TH);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Max Smoothing Clamp              = %x\n",
                         pRegCmd->thresholdLimitsConfig.ASFThresholdConfig0.bitfields.TEXTURE_TH);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Corner Threshold                 = %x\n",
                         pRegCmd->thresholdLimitsConfig.ASFThresholdConfig1.bitfields.SIMILARITY_TH);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Smooth Strength                  = %x\n",
                         pRegCmd->thresholdLimitsConfig.ASFThresholdConfig1.bitfields.SMOOTH_STRENGTH);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Hue Min                          = %x\n",
                         pRegCmd->skinToneDetectionConfig.ASFSkinConfig0.bitfields.H_MIN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Hue Max                          = %x\n",
                         pRegCmd->skinToneDetectionConfig.ASFSkinConfig0.bitfields.H_MAX);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Y Min                            = %x\n",
                         pRegCmd->skinToneDetectionConfig.ASFSkinConfig0.bitfields.Y_MIN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Y Max                            = %x\n",
                         pRegCmd->skinToneDetectionConfig.ASFSkinConfig1.bitfields.Y_MAX);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Q Non Skin                       = %x\n",
                         pRegCmd->skinToneDetectionConfig.ASFSkinConfig1.bitfields.Q_NON_SKIN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Q Skin                           = %x\n",
                         pRegCmd->skinToneDetectionConfig.ASFSkinConfig1.bitfields.Q_SKIN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Boundary Probability             = %x\n",
                         pRegCmd->skinToneDetectionConfig.ASFSkinConfig1.bitfields.BOUNDARY_PROB);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SHY Max                          = %x\n",
                         pRegCmd->skinToneDetectionConfig.ASFSkinConfig2.bitfields.SHY_MAX);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SHY Min                          = %x\n",
                         pRegCmd->skinToneDetectionConfig.ASFSkinConfig2.bitfields.SHY_MIN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SMax Para                        = %x\n",
                         pRegCmd->skinToneDetectionConfig.ASFSkinConfig2.bitfields.SMAX_PARA);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SMin Para                        = %x\n",
                         pRegCmd->skinToneDetectionConfig.ASFSkinConfig2.bitfields.SMIN_PARA);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face Enable                      = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFaceConfig.bitfields.FACE_EN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face Number                      = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFaceConfig.bitfields.FACE_NUM);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face1 Center Cfg Horz Offset     = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace1CenterConfig.bitfields.FACE_CENTER_HORIZONTAL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face1 Center Cfg Vert Offset     = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace1CenterConfig.bitfields.FACE_CENTER_VERTICAL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face1 Radius Cfg Radius Boundary = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace1RadiusConfig.bitfields.FACE_RADIUS_BOUNDARY);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face1 Radius Cfg Radius Shift    = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace1RadiusConfig.bitfields.FACE_RADIUS_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face1 Radius Cfg Radius Slope    = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace1RadiusConfig.bitfields.FACE_RADIUS_SLOPE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face1 Radius Cfg Slope Shift     = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace1RadiusConfig.bitfields.FACE_SLOPE_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face2 Center Cfg Horz Offse      = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace2CenterConfig.bitfields.FACE_CENTER_HORIZONTAL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face2 Center Cfg Vert Offset     = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace2CenterConfig.bitfields.FACE_CENTER_VERTICAL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face2 Radius Cfg Radius Boundary = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace2RadiusConfig.bitfields.FACE_RADIUS_BOUNDARY);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face2 Radius Cfg Radius Shift    = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace2RadiusConfig.bitfields.FACE_RADIUS_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face2 Radius Cfg Radius Slope    = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace2RadiusConfig.bitfields.FACE_RADIUS_SLOPE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face2 Radius Cfg Slope Shift     = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace2RadiusConfig.bitfields.FACE_SLOPE_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face3 Center Cfg Horz Offse      = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace3CenterConfig.bitfields.FACE_CENTER_HORIZONTAL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face3 Center Cfg Vert Offset     = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace3CenterConfig.bitfields.FACE_CENTER_VERTICAL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face3 Radius Cfg Radius Boundary = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace3RadiusConfig.bitfields.FACE_RADIUS_BOUNDARY);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face3 Radius Cfg Radius Shift    = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace3RadiusConfig.bitfields.FACE_RADIUS_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face3 Radius Cfg Radius Slope    = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace3RadiusConfig.bitfields.FACE_RADIUS_SLOPE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face3 Radius Cfg Slope Shift     = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace3RadiusConfig.bitfields.FACE_SLOPE_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face4 Center Cfg Horz Offse      = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace4CenterConfig.bitfields.FACE_CENTER_HORIZONTAL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face4 Center Cfg Vert Offset     = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace4CenterConfig.bitfields.FACE_CENTER_VERTICAL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face4 Radius Cfg Radius Boundary = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace4RadiusConfig.bitfields.FACE_RADIUS_BOUNDARY);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face4 Radius Cfg Radius Shift    = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace4RadiusConfig.bitfields.FACE_RADIUS_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face4 Radius Cfg Radius Slope    = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace4RadiusConfig.bitfields.FACE_RADIUS_SLOPE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face4 Radius Cfg Slope Shift     = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace4RadiusConfig.bitfields.FACE_SLOPE_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face5 Center Cfg Horz Offse      = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace5CenterConfig.bitfields.FACE_CENTER_HORIZONTAL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face5 Center Cfg Vert Offset     = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace5CenterConfig.bitfields.FACE_CENTER_VERTICAL);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face5 Radius Cfg Radius Boundary = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace5RadiusConfig.bitfields.FACE_RADIUS_BOUNDARY);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face5 Radius Cfg Radius Shift    = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace5RadiusConfig.bitfields.FACE_RADIUS_SHIFT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face5 Radius Cfg Radius Slope    = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace5RadiusConfig.bitfields.FACE_RADIUS_SLOPE);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Face5 Radius Cfg Slope Shift     = %x\n",
                         pRegCmd->faceDetectionConfig.ASFFace5RadiusConfig.bitfields.FACE_SLOPE_SHIFT);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30Titan17x::WriteLUTEntrytoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE CamxResult IPEASF30Titan17x::WriteLUTEntrytoDMI(
    CmdBuffer*   pDMICmdBuffer,
    const UINT8  LUTIndex,
    UINT32*      pLUTOffset,
    const UINT32 lutSize)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pDMICmdBuffer) && (NULL != pLUTOffset) && (NULL != m_pLUTDMICmdBuffer))
    {
        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_ASF_DMI_CFG,
                                         LUTIndex,
                                         m_pLUTDMICmdBuffer,
                                         *pLUTOffset,
                                         lutSize);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Error write LUT entry to DMI !");
        }

        // update LUT Offset
        *pLUTOffset += lutSize;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc,
                       "Pointer parameters are empty, pDMICmdBuffer=%p, pLUTOffset=%p, m_pLUTDMICmdBuffer=%p",
                       pDMICmdBuffer,
                       pLUTOffset,
                       m_pLUTDMICmdBuffer);

        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30Titan17x::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30Titan17x::WriteLUTtoDMI(
    ISPInputData* pInputData)
{
    CamxResult  result        = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pLUTDMICmdBuffer))
    {
        CmdBuffer* pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];
        UINT32     LUTOffset     = 0;

        WriteLUTEntrytoDMI(pDMICmdBuffer,
                           Layer1ActivityNormalGainPosGainNegSoftThLUT,
                           &LUTOffset,
                           IPEASFLUTNumEntries[Layer1ActivityNormalGainPosGainNegSoftThLUT] * sizeof(UINT32));

        WriteLUTEntrytoDMI(pDMICmdBuffer,
                           Layer1GainWeightLUT,
                           &LUTOffset,
                           IPEASFLUTNumEntries[Layer1GainWeightLUT] * sizeof(UINT32));

        WriteLUTEntrytoDMI(pDMICmdBuffer,
                           Layer1SoftThresholdWeightLUT,
                           &LUTOffset,
                           IPEASFLUTNumEntries[Layer1SoftThresholdWeightLUT] * sizeof(UINT32));

        WriteLUTEntrytoDMI(pDMICmdBuffer,
                           Layer2ActivityNormalGainPosGainNegSoftThLUT,
                           &LUTOffset,
                           IPEASFLUTNumEntries[Layer2ActivityNormalGainPosGainNegSoftThLUT] * sizeof(UINT32));

        WriteLUTEntrytoDMI(pDMICmdBuffer,
                           Layer2GainWeightLUT,
                           &LUTOffset,
                           IPEASFLUTNumEntries[Layer2GainWeightLUT] * sizeof(UINT32));

        WriteLUTEntrytoDMI(pDMICmdBuffer,
                           Layer2SoftThresholdWeightLUT,
                           &LUTOffset,
                           IPEASFLUTNumEntries[Layer2SoftThresholdWeightLUT] * sizeof(UINT32));

        WriteLUTEntrytoDMI(pDMICmdBuffer,
                           ChromaGradientPosNegLUT,
                           &LUTOffset,
                           IPEASFLUTNumEntries[ChromaGradientPosNegLUT] * sizeof(UINT32));

        WriteLUTEntrytoDMI(pDMICmdBuffer,
                           ContrastPosNegLUT,
                           &LUTOffset,
                           IPEASFLUTNumEntries[ContrastPosNegLUT] * sizeof(UINT32));

        WriteLUTEntrytoDMI(pDMICmdBuffer,
                           SkinActivityGainLUT,
                           &LUTOffset,
                           IPEASFLUTNumEntries[SkinActivityGainLUT] * sizeof(UINT32));
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid parameters pInputData: %p", pInputData);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPEASF30Titan17x::WriteConfigCmds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPEASF30Titan17x::WriteConfigCmds(
    ISPInputData* pInputData)
{
    CamxResult  result     = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pRegCmd) && (NULL != pInputData->pipelineIPEData.ppIPECmdBuffer))
    {
        IPEASFRegCmd* pRegCmd     = static_cast<IPEASFRegCmd*>(m_pRegCmd);
        CmdBuffer*    pCmdBuffer  = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPostLTM];

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIPE_IPE_0_PPS_CLC_ASF_LAYER_1_CFG,
                                              sizeof(IPEASF30TopRegisters) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&pRegCmd->ASFTopResigters));
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Write IPEASF30TopRegisters failed!");
        }

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIPE_IPE_0_PPS_CLC_ASF_LAYER_1_SHARP_COEFF_CFG_0,
                                              sizeof(IPEASF30Layer1Config) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&pRegCmd->layer1Config));
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Write IPEASF30Layer1Config failed!");
        }

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIPE_IPE_0_PPS_CLC_ASF_LAYER_2_SHARP_COEFF_CFG_0,
                                              sizeof(IPEASF30Layer2Config) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&pRegCmd->layer2Config));
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Write IPEASF30Layer2Config failed!");
        }

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIPE_IPE_0_PPS_CLC_ASF_RNR_R_SQUARE_LUT_ENTRY_0,
                                              sizeof(IPEASF30RNRConfig0) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&pRegCmd->rnrConfig0));
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Write IPEASF30RNRConfig0 failed!");
        }

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIPE_IPE_0_PPS_CLC_ASF_RNR_ACTIVITY_CF_LUT_ENTRY_0,
                                              sizeof(IPEASF30RNRConfig1) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&pRegCmd->rnrConfig1));
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Write IPEASF30RNRConfig1 failed!");
        }

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIPE_IPE_0_PPS_CLC_ASF_THRESHOLD_CFG_0,
                                              sizeof(IPEASF30ThresholdLimitsConfig) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&pRegCmd->thresholdLimitsConfig));
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Write IPEASF30ThresholdLimitsConfig failed!");
        }

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIPE_IPE_0_PPS_CLC_ASF_SKIN_CFG_0,
                                              sizeof(IPEASF30SkinToneDetectionConfig) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&pRegCmd->skinToneDetectionConfig));
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Write IPEASF30SkinToneDetectionConfig failed!");
        }

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIPE_IPE_0_PPS_CLC_ASF_FACE_CFG,
                                              sizeof(IPEASF30FaceDetectionConfig) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&pRegCmd->faceDetectionConfig));
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Write IPEASF30FaceDetectionConfig failed!");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid input pInputData = %p, m_pRegCmd = %p", pInputData, m_pRegCmd);
    }
    return result;
}

CAMX_NAMESPACE_END
