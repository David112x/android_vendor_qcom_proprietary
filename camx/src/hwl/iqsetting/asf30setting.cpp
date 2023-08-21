// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  asf30setting.cpp
/// @brief ASF30 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "asf30setting.h"
#include "NcLibWarp.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ASF30Setting::CalculateHWSetting(
    const ASF30InputData*                                 pInput,
    asf_3_0_0::asf30_rgn_dataType*                        pData,
    asf_3_0_0::chromatix_asf30_reserveType*               pReserveType,
    asf_3_0_0::chromatix_asf30Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    BOOL  result = TRUE;
    UINT  i;
    INT32 intVal;
    FLOAT floatVal;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pReserveType)  &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        ASF30UnpackedField* pUnpackedField = static_cast<ASF30UnpackedField*>(pOutput);

        pUnpackedField->enable                       = static_cast<UINT8>(pModuleEnable->asf_enable);
        pUnpackedField->skinEnable                   = static_cast<UINT8>(pModuleEnable->skin_enable);
        pUnpackedField->chromaGradientEnable         = static_cast<UINT8>(pModuleEnable->chroma_gradient_enable);

        pUnpackedField->specialEffectAbsoluteEnable  = pInput->specialEffectAbsoluteEnable;
        pUnpackedField->negateAbsoluteY1             = pInput->negateAbsoluteY1;
        pUnpackedField->specialEffectEnable          = pInput->specialEffectEnable;

        intVal   = IQSettingUtils::ClampINT32(pReserveType->edge_alignment_enable, MIN_BIT, MAX_BIT);
        pUnpackedField->edgeAlignmentEnable          = static_cast<UINT8>(intVal);

        intVal   = IQSettingUtils::ClampUINT32(pReserveType->layer_1_enable, MIN_BIT, MAX_BIT);
        pUnpackedField->layer1Enable                 = static_cast<UINT8>(intVal);

        intVal   = IQSettingUtils::ClampUINT32(pReserveType->layer_2_enable, MIN_BIT, MAX_BIT);
        pUnpackedField->layer2Enable                 = static_cast<UINT8>(intVal);

        intVal   = IQSettingUtils::ClampUINT32(pReserveType->radial_enable, MIN_BIT, MAX_BIT);
        pUnpackedField->radialEnable                 = static_cast<UINT8>(intVal);

        intVal   = IQSettingUtils::ClampUINT32(pReserveType->contrast_enable, MIN_BIT, MAX_BIT);
        pUnpackedField->contrastEnable               = static_cast<UINT8>(intVal);

        intVal   = IQSettingUtils::ClampINT32(pReserveType->face_enable, MIN_BIT, MAX_BIT);
        pUnpackedField->faceEnable          = static_cast<UINT8>(intVal);

        for (i = 0; i < NUM_OF_NZ_ENTRIES; i++)
        {
            pUnpackedField->nonZero[i] = pInput->nonZero[i];
        }

        floatVal = IQSettingUtils::ClampFLOAT((pData->layer_1_sp / 100.0f), SP_FLOAT_MIN, SP_FLOAT_MAX);
        intVal   = IQSettingUtils::FloatToQNumber(floatVal, SP_Q_BITS);
        pUnpackedField->smoothPercentage             = static_cast<UINT8>(intVal);

        intVal   = IQSettingUtils::RoundFLOAT(pData->flat_thresold * EA_CONV_FACTOR);
        pUnpackedField->flatThreshold                = IQSettingUtils::ClampUINT32(intVal, FLAT_THR_MIN, FLAT_THR_MAX);

        intVal   = IQSettingUtils::RoundFLOAT(pData->max_smoothing_clamp);
        pUnpackedField->maxSmoothingClamp            = IQSettingUtils::ClampUINT32(intVal, MAX_SMOOTH_MIN, MAX_SMOOTH_MAX);

        intVal   = IQSettingUtils::FloatToQNumber(pData->corner_threshold, CORNER_THRESHOLD_Q_BITS);
        pUnpackedField->cornerThreshold              = IQSettingUtils::ClampUINT32(intVal, CORNER_THR_MIN, CORNER_THR_MAX);

        intVal   = IQSettingUtils::FloatToQNumber(pData->smoothing_strength, SMOOTHING_STRENGTH_Q_BITS);
        pUnpackedField->smoothingStrength            = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(intVal, SMOOTH_STR_MIN, SMOOTH_STR_MAX));

        intVal   = IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->layer_1_clamp_ul), MIN_INT9, MAX_INT9);
        pUnpackedField->layer1RegTH                  = static_cast<INT16>(intVal);

        intVal   = IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->layer_1_clamp_ll), MIN_INT9, MAX_INT9);
        pUnpackedField->layer1RegTL                  = static_cast<INT16>(intVal);

        intVal   = IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->layer_2_clamp_ul), MIN_INT9, MAX_INT9);
        pUnpackedField->layer2RegTH                  = static_cast<INT16>(intVal);

        intVal   = IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->layer_2_clamp_ll), MIN_INT9, MAX_INT9);
        pUnpackedField->layer2RegTL                  = static_cast<INT16>(intVal);

        intVal   = IQSettingUtils::ClampINT16(static_cast<INT16>(pData->layer_1_l2_norm_en), MIN_BIT, MAX_BIT);
        pUnpackedField->layer1L2NormEn               = static_cast<UINT8>(intVal);

        intVal   = IQSettingUtils::ClampINT16(static_cast<INT16>(pData->layer_2_l2_norm_en), MIN_BIT, MAX_BIT);
        pUnpackedField->layer2L2NormEn               = static_cast<UINT8>(intVal);

        intVal   = IQSettingUtils::RoundFLOAT(pData->layer_1_activity_clamp_threshold);
        pUnpackedField->layer1ActivityClampThreshold = static_cast<UINT8>(
            IQSettingUtils::ClampINT32(intVal, MIN_UINT8, MAX_UINT8));

        intVal   = IQSettingUtils::RoundFLOAT(pData->layer_2_activity_clamp_threshold);
        pUnpackedField->layer2ActivityClampThreshold = static_cast<UINT8>(
            IQSettingUtils::ClampINT32(intVal, MIN_UINT8, MAX_UINT8));

        intVal   = IQSettingUtils::FloatToQNumber(pData->layer_1_norm_scale, NORM_SCALE_Q_BITS);
        pUnpackedField->layer1NormScale              = static_cast<UINT8>(
            IQSettingUtils::ClampINT32(intVal, MIN_UINT8, MAX_UINT8));

        intVal   = IQSettingUtils::FloatToQNumber(pData->layer_2_norm_scale, NORM_SCALE_Q_BITS);
        pUnpackedField->layer2NormScale              = static_cast<UINT8>(
            IQSettingUtils::ClampINT32(intVal, MIN_UINT8, MAX_UINT8));

        intVal   = IQSettingUtils::RoundFLOAT(pData->layer_1_gamma_corrected_luma_target);
        pUnpackedField->gammaCorrectedLumaTarget     = static_cast<UINT8>(
            IQSettingUtils::ClampINT32(intVal, MIN_UINT8, MAX_UINT8));

        intVal   = IQSettingUtils::FloatToQNumber(pData->layer_1_gain_cap, GAIN_CAP_Q_BITS);
        pUnpackedField->gainCap                      = static_cast<UINT8>(
            IQSettingUtils::ClampINT32(intVal, MIN_UINT8, MAX_UINT8));

        intVal   = IQSettingUtils::FloatToQNumber(pData->layer_1_median_blend_lower_offset, MEDIAN_BLEND_OFFSET_Q_BITS);
        pUnpackedField->medianBlendLowerOffset       = static_cast<UINT8>(
            IQSettingUtils::ClampINT32(intVal, MEDIAN_BLEND_OFFSET_MIN, MEDIAN_BLEND_OFFSET_MAX));

        intVal   = IQSettingUtils::FloatToQNumber(pData->layer_1_median_blend_upper_offset, MEDIAN_BLEND_OFFSET_Q_BITS);
        pUnpackedField->medianBlendUpperOffset       = static_cast<UINT8>(
            IQSettingUtils::ClampINT32(intVal, MEDIAN_BLEND_OFFSET_MIN, MEDIAN_BLEND_OFFSET_MAX));

        pUnpackedField->textureThr = IQSettingUtils::ClampUINT32(
            IQSettingUtils::RoundFLOAT(pData->max_smoothing_clamp), MIN_UINT8, MAX_UINT8); // added the missing variable

        pUnpackedField->similarityThr = IQSettingUtils::ClampUINT32(
            IQSettingUtils::RoundFLOAT(pData->corner_threshold*(1 << 10)), MIN_UINT16, MAX_UINT16);// added missing variable

        CalculateFilterSetting(pData, pUnpackedField);
        CalculateActNormLUTSetting(pData, pUnpackedField);
        CalculateWeightLUTSetting(pData, pUnpackedField);
        CalculateSoftThresholdLUTSetting(pData, pUnpackedField);
        CalculateGainPositiveLUTSetting(pData, pUnpackedField, pInput->sharpness);
        CalculateGainNegativeLUTSetting(pData, pUnpackedField, pInput->sharpness);
        CalculateGainWeightLUTSetting(pData, pUnpackedField);
        CalculateGainContrastNegLUTSetting(pData, pUnpackedField);
        CalculateGainContrastPosLUTSetting(pData, pUnpackedField);
        CalculateGainChromaNegLUTSetting(pData, pUnpackedField);
        CalculateGainChromaPosLUTSetting(pData, pUnpackedField);
        CalculateSkinGainActLUTSetting(pData, pUnpackedField);
        CalculateRNRSetting(pInput, pData, pReserveType, pUnpackedField);
        CalculateSkinSetting(pData, pUnpackedField);
        CalculateFaceSetting(pInput, pData, pUnpackedField);
    }
    else
    {
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateFilterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateFilterSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField)
{
    UINT  i;

    for (i = 0; i < LUT_LAYER1_FILTER_SIZE; i++)
    {
        pUnpackedField->layer1CornerFilter[i]  = IQSettingUtils::ClampINT16(
            static_cast<INT16>(pData->layer_1_hpf_symmetric_coeff_tab.layer_1_hpf_symmetric_coeff[i]),
            L1_CORNER_FILTER_MIN[i], L1_CORNER_FILTER_MAX[i]);

        pUnpackedField->layer1LowPassFilter[i] = IQSettingUtils::ClampINT16(
            static_cast<INT16>(pData->layer_1_lpf_symmetric_coeff_tab.layer_1_lpf_symmetric_coeff[i]),
            L1_LOWPASS_FILTER_MIN[i], L1_LOWPASS_FILTER_MAX[i]);
    }

    for (i = 0; i < LUT_ACTIVITY_BPF_SIZE; i++)
    {
        pUnpackedField->layer1ActivityBPF[i]   = IQSettingUtils::ClampINT16(
            static_cast<INT16>(pData->layer_1_activity_band_pass_coeff_tab.layer_1_activity_band_pass_coeff[i]),
            L1_ACTIVITY_BPF_MIN[i], L1_ACTIVITY_BPF_MAX[i]);
    }

    for (i = 0; i < LUT_LAYER2_FILTER_SIZE; i++)
    {
        pUnpackedField->layer2CornerFilter[i]  = IQSettingUtils::ClampINT16(
            static_cast<INT16>(pData->layer_2_hpf_symmetric_coeff_tab.layer_2_hpf_symmetric_coeff[i]),
            L2_CORNER_FILTER_MIN[i], L2_CORNER_FILTER_MAX[i]);

        pUnpackedField->layer2LowPassFilter[i] = IQSettingUtils::ClampINT16(
            static_cast<INT16>(pData->layer_2_lpf_symmetric_coeff_tab.layer_2_lpf_symmetric_coeff[i]),
            L2_LOWPASS_FILTER_MIN[i], L2_LOWPASS_FILTER_MAX[i]);

        pUnpackedField->layer2ActivityBPF[i]   = IQSettingUtils::ClampINT16(
            static_cast<INT16>(pData->layer_2_activity_band_pass_coeff_tab.layer_2_activity_band_pass_coeff[i]),
            L2_ACTIVITY_BPF_MIN[i], L2_ACTIVITY_BPF_MAX[i]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateActNormLUTSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateActNormLUTSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField)
{
    UINT32 i;
    UINT32 j;
    UINT32 lutVal1;
    UINT32 lutVal2;
    INT32  maxUint8 = static_cast<INT32>(MAX_UINT8+1);
    INT    index;
    FLOAT  ratio;

    UINT32 interpolationPoints = DMI_ACT_NORM_SIZE / LUT_ACTIVITY_NORM_SIZE;
    for (i = 0; i < LUT_ACTIVITY_NORM_SIZE; i++)
    {
        for (j = 0; j < interpolationPoints; j++)
        {
            index = i * interpolationPoints + j;
            ratio = j / static_cast<FLOAT>(interpolationPoints);
            if (i < LUT_ACTIVITY_NORM_SIZE - 1)
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->layer_1_activity_normalization_lut_tab.layer_1_activity_normalization_lut[i],
                    pData->layer_1_activity_normalization_lut_tab.layer_1_activity_normalization_lut[i + 1],
                    ratio), LUT_ACTIVITY_NORM_Q_BITS);

                lutVal2 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->layer_2_activity_normalization_lut_tab.layer_2_activity_normalization_lut[i],
                    pData->layer_2_activity_normalization_lut_tab.layer_2_activity_normalization_lut[i + 1],
                    ratio), LUT_ACTIVITY_NORM_Q_BITS);

                lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8 + 1);

                lutVal2 = IQSettingUtils::ClampINT32(lutVal2, MIN_UINT8, MAX_UINT8 + 1);

                pUnpackedField->layer1ActivityNormalizationLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32(maxUint8 - static_cast<INT32>(lutVal1),
                                                                  MIN_UINT8, MAX_UINT8));

                pUnpackedField->layer2ActivityNormalizationLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32(maxUint8 - static_cast<INT32>(lutVal2),
                                                                  MIN_UINT8, MAX_UINT8));
            }
            else
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(
                    pData->layer_1_activity_normalization_lut_tab.layer_1_activity_normalization_lut[i],
                    LUT_ACTIVITY_NORM_Q_BITS);

                lutVal2 = IQSettingUtils::FloatToQNumber(
                    pData->layer_2_activity_normalization_lut_tab.layer_2_activity_normalization_lut[i],
                    LUT_ACTIVITY_NORM_Q_BITS);

                lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8 + 1);

                lutVal2 = IQSettingUtils::ClampINT32(lutVal2, MIN_UINT8, MAX_UINT8 + 1);

                pUnpackedField->layer1ActivityNormalizationLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32(maxUint8 - static_cast<INT32>(lutVal1),
                                                                  MIN_UINT8, MAX_UINT8));

                pUnpackedField->layer2ActivityNormalizationLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32(maxUint8 - static_cast<INT32>(lutVal2),
                                                                  MIN_UINT8, MAX_UINT8));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateWeightLUTSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateWeightLUTSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField)
{
    UINT32 i;
    UINT32 j;
    UINT32 lutVal1;
    UINT32 lutVal2;

    UINT32 interpolationPoints = DMI_WEIGHT_MOD_SIZE / LUT_WEIGHT_MOD_SIZE;
    for (i = 0; i < LUT_WEIGHT_MOD_SIZE; i++)
    {
        for (j = 0; j < interpolationPoints; j++)
        {
            INT   index = i * interpolationPoints + j;
            FLOAT ratio = j / static_cast<FLOAT>(interpolationPoints);
            if (i < LUT_WEIGHT_MOD_SIZE - 1)
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->layer_1_weight_modulation_lut_tab.layer_1_weight_modulation_lut[i],
                    pData->layer_1_weight_modulation_lut_tab.layer_1_weight_modulation_lut[i + 1],
                    ratio), LUT_WEIGHT_MOD_Q_BITS);

                lutVal2 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->layer_2_weight_modulation_lut_tab.layer_2_weight_modulation_lut[i],
                    pData->layer_2_weight_modulation_lut_tab.layer_2_weight_modulation_lut[i + 1],
                    ratio), LUT_WEIGHT_MOD_Q_BITS);

                pUnpackedField->layer1SoftThresholdWeightLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8));

                pUnpackedField->layer2SoftThresholdWeightLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32(lutVal2, MIN_UINT8, MAX_UINT8));
            }
            else
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(
                    pData->layer_1_weight_modulation_lut_tab.layer_1_weight_modulation_lut[i],
                    LUT_WEIGHT_MOD_Q_BITS);

                lutVal2 = IQSettingUtils::FloatToQNumber(
                    pData->layer_2_weight_modulation_lut_tab.layer_2_weight_modulation_lut[i],
                    LUT_WEIGHT_MOD_Q_BITS);

                pUnpackedField->layer1SoftThresholdWeightLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8));

                pUnpackedField->layer2SoftThresholdWeightLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32(lutVal2, MIN_UINT8, MAX_UINT8));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateSoftThresholdLUTSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateSoftThresholdLUTSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField)
{
    UINT32 i;
    UINT32 j;
    UINT32 lutVal1;
    UINT32 lutVal2;

    UINT32 interpolationPoints = DMI_SOFT_SIZE / LUT_SOFT_THRESHOLD_SIZE;
    for (i = 0; i < LUT_SOFT_THRESHOLD_SIZE; i++)
    {
        for (j = 0; j < interpolationPoints; j++)
        {
            INT   index = i * interpolationPoints + j;
            FLOAT ratio = j / static_cast<FLOAT>(interpolationPoints);
            if (i < LUT_SOFT_THRESHOLD_SIZE - 1)
            {
                lutVal1 = IQSettingUtils::RoundFLOAT(IQSettingUtils::InterpolationFloatBilinear(
                    pData->layer_1_soft_threshold_lut_tab.layer_1_soft_threshold_lut[i],
                    pData->layer_1_soft_threshold_lut_tab.layer_1_soft_threshold_lut[i + 1], ratio));

                lutVal2 = IQSettingUtils::RoundFLOAT(IQSettingUtils::InterpolationFloatBilinear(
                    pData->layer_2_soft_threshold_lut_tab.layer_2_soft_threshold_lut[i],
                    pData->layer_2_soft_threshold_lut_tab.layer_2_soft_threshold_lut[i + 1], ratio));

                pUnpackedField->layer1SoftThresholdLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8));

                pUnpackedField->layer2SoftThresholdLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32(lutVal2, MIN_UINT8, MAX_UINT8));
            }
            else
            {
                lutVal1 = IQSettingUtils::RoundFLOAT(
                    pData->layer_1_soft_threshold_lut_tab.layer_1_soft_threshold_lut[i]);

                lutVal2 = IQSettingUtils::RoundFLOAT(
                    pData->layer_2_soft_threshold_lut_tab.layer_2_soft_threshold_lut[i]);

                pUnpackedField->layer1SoftThresholdLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8));

                pUnpackedField->layer2SoftThresholdLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32(lutVal2, MIN_UINT8, MAX_UINT8));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateGainPositiveLUTSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateGainPositiveLUTSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField,
    FLOAT                               sharpnessVal)
{
    UINT32 i;
    UINT32 j;
    UINT32 lutVal1;
    UINT32 lutVal2;

    UINT32 interpolationPoints = DMI_GAINPOSNEG_SIZE / LUT_GAINPOSNEG_SIZE;
    for (i = 0; i < LUT_GAINPOSNEG_SIZE; i++)
    {
        for (j = 0; j < interpolationPoints; j++)
        {
            INT   index = i * interpolationPoints + j;
            FLOAT ratio = j / static_cast<FLOAT>(interpolationPoints);
            if (i < LUT_GAINPOSNEG_SIZE - 1)
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->layer_1_gain_positive_lut_tab.layer_1_gain_positive_lut[i],
                    pData->layer_1_gain_positive_lut_tab.layer_1_gain_positive_lut[i + 1], ratio), LUT_GAIN_POSNEG_Q_BITS);

                lutVal2 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->layer_2_gain_positive_lut_tab.layer_2_gain_positive_lut[i],
                    pData->layer_2_gain_positive_lut_tab.layer_2_gain_positive_lut[i + 1], ratio), LUT_GAIN_POSNEG_Q_BITS);

                pUnpackedField->layer1GainPositiveLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampFLOAT(lutVal1 * sharpnessVal,
                                                                  static_cast<FLOAT>(MIN_UINT8),
                                                                  static_cast<FLOAT>(MAX_UINT8)));

                pUnpackedField->layer2GainPositiveLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampFLOAT(lutVal2 * sharpnessVal,
                                                                  static_cast<FLOAT>(MIN_UINT8),
                                                                  static_cast<FLOAT>(MAX_UINT8)));
            }
            else
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(
                    pData->layer_1_gain_positive_lut_tab.layer_1_gain_positive_lut[i], LUT_GAIN_POSNEG_Q_BITS);

                lutVal2 = IQSettingUtils::FloatToQNumber(
                    pData->layer_2_gain_positive_lut_tab.layer_2_gain_positive_lut[i], LUT_GAIN_POSNEG_Q_BITS);

                pUnpackedField->layer1GainPositiveLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampFLOAT(lutVal1 * sharpnessVal,
                                                                  static_cast<FLOAT>(MIN_UINT8),
                                                                  static_cast<FLOAT>(MAX_UINT8)));

                pUnpackedField->layer2GainPositiveLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampFLOAT(lutVal2 * sharpnessVal,
                                                                  static_cast<FLOAT>(MIN_UINT8),
                                                                  static_cast<FLOAT>(MAX_UINT8)));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateGainNegativeLUTSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateGainNegativeLUTSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField,
    FLOAT                               sharpnessVal)
{
    UINT32 i;
    UINT32 j;
    UINT32 lutVal1;
    UINT32 lutVal2;

    UINT32 interpolationPoints = DMI_GAINPOSNEG_SIZE / LUT_GAINPOSNEG_SIZE;
    for (i = 0; i < LUT_GAINPOSNEG_SIZE; i++)
    {
        for (j = 0; j < interpolationPoints; j++)
        {
            INT   index = i * interpolationPoints + j;
            FLOAT ratio = j / static_cast<FLOAT>(interpolationPoints);
            if (i < LUT_GAINPOSNEG_SIZE - 1)
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->layer_1_gain_negative_lut_tab.layer_1_gain_negative_lut[i],
                    pData->layer_1_gain_negative_lut_tab.layer_1_gain_negative_lut[i + 1], ratio), LUT_GAIN_POSNEG_Q_BITS);

                lutVal2 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->layer_2_gain_negative_lut_tab.layer_2_gain_negative_lut[i],
                    pData->layer_2_gain_negative_lut_tab.layer_2_gain_negative_lut[i + 1], ratio), LUT_GAIN_POSNEG_Q_BITS);

                pUnpackedField->layer1GainNegativeLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampFLOAT(lutVal1 * sharpnessVal,
                                                                  static_cast<FLOAT>(MIN_UINT8),
                                                                  static_cast<FLOAT>(MAX_UINT8)));

                pUnpackedField->layer2GainNegativeLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampFLOAT(lutVal2 * sharpnessVal,
                                                                  static_cast<FLOAT>(MIN_UINT8),
                                                                  static_cast<FLOAT>(MAX_UINT8)));

            }
            else
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(
                    pData->layer_1_gain_negative_lut_tab.layer_1_gain_negative_lut[i], LUT_GAIN_POSNEG_Q_BITS);

                lutVal2 = IQSettingUtils::FloatToQNumber(
                    pData->layer_2_gain_negative_lut_tab.layer_2_gain_negative_lut[i], LUT_GAIN_POSNEG_Q_BITS);

                pUnpackedField->layer1GainNegativeLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampFLOAT(lutVal1 * sharpnessVal,
                                                                  static_cast<FLOAT>(MIN_UINT8),
                                                                  static_cast<FLOAT>(MAX_UINT8)));

                pUnpackedField->layer2GainNegativeLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampFLOAT(lutVal2 * sharpnessVal,
                                                                  static_cast<FLOAT>(MIN_UINT8),
                                                                  static_cast<FLOAT>(MAX_UINT8)));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateGainWeightLUTSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateGainWeightLUTSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField)
{
    UINT32 i;
    UINT32 j;
    UINT32 lutVal1;
    UINT32 lutVal2;

    UINT32 interpolationPoints = DMI_GAINWEIGHT_SIZE / LUT_GAINWEIGHT_SIZE;
    for (i = 0; i < LUT_GAINWEIGHT_SIZE; i++)
    {
        for (j = 0; j < interpolationPoints; j++)
        {
            INT   index = i * interpolationPoints + j;
            FLOAT ratio = j / static_cast<FLOAT>(interpolationPoints);
            if (i < LUT_GAINWEIGHT_SIZE - 1)
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->layer_1_gain_weight_lut_tab.layer_1_gain_weight_lut[i],
                    pData->layer_1_gain_weight_lut_tab.layer_1_gain_weight_lut[i + 1], ratio), LUT_GAIN_WEIGHT_Q_BITS);

                lutVal2 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->layer_2_gain_weight_lut_tab.layer_2_gain_weight_lut[i],
                    pData->layer_2_gain_weight_lut_tab.layer_2_gain_weight_lut[i + 1], ratio), LUT_GAIN_WEIGHT_Q_BITS);
                lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8 + 1);
                lutVal2 = IQSettingUtils::ClampINT32(lutVal2, MIN_UINT8, MAX_UINT8 + 1);

                pUnpackedField->layer1GainWeightLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal1, MIN_UINT8, MAX_UINT8));

                pUnpackedField->layer2GainWeightLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal2, MIN_UINT8, MAX_UINT8));
            }
            else
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(pData->layer_1_gain_weight_lut_tab.layer_1_gain_weight_lut[i],
                    LUT_GAIN_WEIGHT_Q_BITS);
                lutVal2 = IQSettingUtils::FloatToQNumber(pData->layer_2_gain_weight_lut_tab.layer_2_gain_weight_lut[i],
                    LUT_GAIN_WEIGHT_Q_BITS);
                lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8 + 1);
                lutVal2 = IQSettingUtils::ClampINT32(lutVal2, MIN_UINT8, MAX_UINT8 + 1);

                pUnpackedField->layer1GainWeightLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal1, MIN_UINT8, MAX_UINT8));

                pUnpackedField->layer2GainWeightLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal2, MIN_UINT8, MAX_UINT8));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateGainContrastNegLUTSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateGainContrastNegLUTSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField)
{
    UINT32 i;
    UINT32 j;
    UINT32 lutVal1;

    UINT32 interpolationPoints = DMI_GAINCONTRAST_SIZE / LUT_GAINCONTRAST_SIZE;
    for (i = 0; i < LUT_GAINCONTRAST_SIZE; i++)
    {
        for (j = 0; j < interpolationPoints; j++)
        {
            INT   index = i * interpolationPoints + j;
            FLOAT ratio = j / static_cast<FLOAT>(interpolationPoints);
            if (i < LUT_GAINCONTRAST_SIZE - 1)
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->gain_contrast_negative_tab.gain_contrast_negative[i],
                    pData->gain_contrast_negative_tab.gain_contrast_negative[i + 1], ratio), LUT_GAIN_CONTRAST_Q_BITS);
                lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8 + 1);
                pUnpackedField->gainContrastNegativeLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal1, MIN_UINT8, MAX_UINT8));
            }
            else
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(
                    pData->gain_contrast_negative_tab.gain_contrast_negative[i], LUT_GAIN_CONTRAST_Q_BITS);
                lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8 + 1);
                pUnpackedField->gainContrastNegativeLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal1, MIN_UINT8, MAX_UINT8));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateGainContrastPosLUTSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateGainContrastPosLUTSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField)
{
    UINT32 i;
    UINT32 j;
    UINT32 lutVal1;

    UINT32 interpolationPoints = DMI_GAINCONTRAST_SIZE / LUT_GAINCONTRAST_SIZE;
    for (i = 0; i < LUT_GAINCONTRAST_SIZE; i++)
    {
        for (j = 0; j < interpolationPoints; j++)
        {
            INT   index = i * interpolationPoints + j;
            FLOAT ratio = j / static_cast<FLOAT>(interpolationPoints);
            if (i < LUT_GAINCONTRAST_SIZE - 1)
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->gain_contrast_positive_tab.gain_contrast_positive[i],
                    pData->gain_contrast_positive_tab.gain_contrast_positive[i + 1], ratio), LUT_GAIN_CONTRAST_Q_BITS);
                lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8 + 1);
                pUnpackedField->gainContrastPositiveLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal1, MIN_UINT8, MAX_UINT8));
            }
            else
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(
                    pData->gain_contrast_positive_tab.gain_contrast_positive[i], LUT_GAIN_CONTRAST_Q_BITS);
                lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8 + 1);
                pUnpackedField->gainContrastPositiveLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal1, MIN_UINT8, MAX_UINT8));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateGainChromaNegLUTSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateGainChromaNegLUTSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField)
{
    UINT32 i;
    UINT32 j;
    UINT32 lutVal1;

    UINT32 interpolationPoints = DMI_GAINCHROMA_SIZE / LUT_GAINCHROMA_SIZE;
    for (i = 0; i < LUT_GAINCHROMA_SIZE; i++)
    {
        for (j = 0; j < interpolationPoints; j++)
        {
            INT   index = i * interpolationPoints + j;
            FLOAT ratio = j / static_cast<FLOAT>(interpolationPoints);
            if (i < LUT_GAINCHROMA_SIZE - 1)
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->gain_chroma_negative_tab.gain_chroma_negative[i],
                    pData->gain_chroma_negative_tab.gain_chroma_negative[i + 1], ratio), LUT_GAIN_CHROMA_Q_BITS);
                lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8 + 1);
                pUnpackedField->chromaGradientNegativeLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal1, MIN_UINT8, MAX_UINT8));
            }
            else
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(pData->gain_chroma_negative_tab.gain_chroma_negative[i],
                    LUT_GAIN_CHROMA_Q_BITS);
                lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8 + 1);
                pUnpackedField->chromaGradientNegativeLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal1, MIN_UINT8, MAX_UINT8));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateGainChromaPosLUTSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateGainChromaPosLUTSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField)
{
    UINT32 i;
    UINT32 j;
    UINT32 lutVal1;

    UINT32 interpolationPoints = DMI_GAINCHROMA_SIZE / LUT_GAINCHROMA_SIZE;
    for (i = 0; i < LUT_GAINCHROMA_SIZE; i++)
    {
        for (j = 0; j < interpolationPoints; j++)
        {
            INT   index = i * interpolationPoints + j;
            FLOAT ratio = j / static_cast<FLOAT>(interpolationPoints);
            if (i < LUT_GAINCHROMA_SIZE - 1)
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(IQSettingUtils::InterpolationFloatBilinear(
                    pData->gain_chroma_positive_tab.gain_chroma_positive[i],
                    pData->gain_chroma_positive_tab.gain_chroma_positive[i + 1], ratio), LUT_GAIN_CHROMA_Q_BITS);
                lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, (MAX_UINT8 + 1));
                pUnpackedField->chromaGradientPositiveLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal1, MIN_UINT8, MAX_UINT8));
            }
            else
            {
                lutVal1 = IQSettingUtils::FloatToQNumber(pData->gain_chroma_positive_tab.gain_chroma_positive[i],
                    LUT_GAIN_CHROMA_Q_BITS);
                lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, (MAX_UINT8 + 1));
                pUnpackedField->chromaGradientPositiveLut[index] =
                    static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal1, MIN_UINT8, MAX_UINT8));
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateSkinGainActLUTSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateSkinGainActLUTSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField)
{
    UINT32 i;
    UINT32 lutVal1;
    UINT32 lutVal2;

    for (i = 0; i < LUT_SKIN_SIZE; i++)
    {
        lutVal1 = IQSettingUtils::FloatToQNumber(pData->skin_gain_tab.skin_gain[i], LUT_SKIN_Q_BITS);
        lutVal2 = IQSettingUtils::FloatToQNumber(pData->skin_activity_tab.skin_activity[i], LUT_SKIN_Q_BITS);
        lutVal1 = IQSettingUtils::ClampINT32(lutVal1, MIN_UINT8, MAX_UINT8 + 1);
        lutVal2 = IQSettingUtils::ClampINT32(lutVal2, MIN_UINT8, MAX_UINT8 + 1);
        pUnpackedField->skinGainLut[i]     =
            static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal1, MIN_UINT8, MAX_UINT8));
        pUnpackedField->skinActivityLut[i] =
            static_cast<UINT8>(IQSettingUtils::ClampINT32((MAX_UINT8 + 1) - lutVal2, MIN_UINT8, MAX_UINT8));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateRNRSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateRNRSetting(
    const ASF30InputData*                   pInput,
    asf_3_0_0::asf30_rgn_dataType*          pData,
    asf_3_0_0::chromatix_asf30_reserveType* pReserveType,
    ASF30UnpackedField*                     pUnpackedField)
{
    UINT32 i;
    INT32  exp = 0;
    INT32 xOffset = static_cast<INT32>(pInput->sensorOffsetX);
    INT32 yOffset = static_cast<INT32>(pInput->sensorOffsetY);
    INT32 xWidth  = static_cast<INT32>(pInput->chYStreamInWidth >> 1);
    INT32 yHeight = static_cast<INT32>(pInput->chYStreamInHeight >> 1);

    pUnpackedField->rSquareScale                           = 1 << R_SQUARE_SCALE_FACTOR;
    pUnpackedField->initialOffsetMinusCenterHorizontal     = static_cast<INT16>(xOffset - xWidth);
    pUnpackedField->initialOffsetMinusCenterVertical       = static_cast<INT16>(yOffset - yHeight);
    pUnpackedField->initialRSquare                         =
        pUnpackedField->initialOffsetMinusCenterHorizontal * pUnpackedField->initialOffsetMinusCenterHorizontal +
        pUnpackedField->initialOffsetMinusCenterVertical   * pUnpackedField->initialOffsetMinusCenterVertical;

    DOUBLE fractional = frexp(static_cast<DOUBLE>(pUnpackedField->initialRSquare), &exp);
    INT32  fractionWithQ = static_cast<INT32>(floor(fractional * (1 << (R_SQAURE_Q_FACTOR))));

    if (fractionWithQ >= (1 << R_SQAURE_Q_FACTOR))
    {
        exp++;
    }
    if ((exp - static_cast<INT32>(R_SQAURE_Q_FACTOR)) >= 0)
    {
        pUnpackedField->rSquareShift = static_cast<UINT8>(exp - R_SQAURE_Q_FACTOR);
    }
    else
    {
        pUnpackedField->rSquareShift = 0;
    }

    INT   luma_r_square_table[NUM_LUMA_R_SQUARE_ENTRIES];

    for (i = 0; i < NUM_LUMA_R_SQUARE_ENTRIES; i++)
    {
        INT32 pseudo_mantissa = static_cast<INT32>(floor(pUnpackedField->initialRSquare *
                                                         pReserveType->radial_anchor_tab.radial_anchor[i] /
                                                         (1 << pUnpackedField->rSquareShift)));
        if (i < NUM_LUMA_R_SQUARE_ENTRIES -1)
        {
            pUnpackedField->rSquareTable[i] = pseudo_mantissa;
        }
        luma_r_square_table[i] = pseudo_mantissa;
    }

    FLOAT asf_radial_gain_cf[NUM_ASF_RADIAL_ENTRIES];
    FLOAT asf_radial_activity_cf[NUM_ASF_RADIAL_ENTRIES];
    FLOAT asf_radial_cf_limit = 2047.0f / 256.0f;

    for (i = 0; i < NUM_ASF_RADIAL_ENTRIES; i++)
    {
        asf_radial_gain_cf[i]     = pData->radial_gain_adj_tab.radial_gain_adj[i];
        asf_radial_gain_cf[i]     = IQSettingUtils::ClampFLOAT(asf_radial_gain_cf[i], 0, asf_radial_cf_limit);
        asf_radial_activity_cf[i] = pData->radial_activity_adj_tab.radial_activity_adj[i];
        asf_radial_activity_cf[i] = IQSettingUtils::ClampFLOAT(asf_radial_activity_cf[i], 0, asf_radial_cf_limit);
    }

    for (i = 0; i < NUM_ASF_RADIAL_ENTRIES - 1; i++)
    {
        pUnpackedField->radialGainCFTable[i]     = static_cast<INT16>(
            IQSettingUtils::FloatToQNumber(asf_radial_gain_cf[i], RADIAL_CF_Q_BITS));
        pUnpackedField->radialActivityCFTable[i] = static_cast<INT16>(
            IQSettingUtils::FloatToQNumber(asf_radial_activity_cf[i], RADIAL_CF_Q_BITS));

        // y bilat_scale slope
        DOUBLE slope;

        // asf radial stuff
        slope = (asf_radial_gain_cf[i + 1] - asf_radial_gain_cf[i]) /
            (luma_r_square_table[i + 1] - luma_r_square_table[i]);
        fractional = frexp(slope, &exp);
        if (fractional < 0)
        {
            fractional *= -1;
        }
        fractionWithQ = static_cast<INT32>(ceil(fractional * (1 << 11)));
        if (fractionWithQ >= static_cast<INT32>(MAX_FRACTION_WITH_Q))
        {
            fractional = 0.5;
            exp++;
        }
        if (-exp + 2 >= 0)
        {
            pUnpackedField->radialGainCFShift[i]    = static_cast<UINT8>(-exp + 2);
            pUnpackedField->radialGainSlopeTable[i] =
                static_cast<INT16>(ceil(fractional * (1 << RADIAL_GAIN_SLOPE_Q_FACTOR)));
        }
        else
        {
            pUnpackedField->radialGainCFShift[i]    = 0;
            pUnpackedField->radialGainSlopeTable[i] =
                static_cast<INT16>(ceil(fractional * (1ULL << (RADIAL_GAIN_SLOPE_Q_FACTOR + exp - 2))));
        }
        if (slope < 0)
        {
            pUnpackedField->radialGainSlopeTable[i] *= -1;
        }

        slope = (asf_radial_activity_cf[i + 1] - asf_radial_activity_cf[i]) /
            (luma_r_square_table[i + 1] - luma_r_square_table[i]);
        fractional = frexp(slope, &exp);
        if (fractional < 0)
        {
            fractional *= -1;
        }
        fractionWithQ = static_cast<INT32>(ceil(fractional * (1 << RADIAL_ACT_SLOPE_Q_FACTOR)));
        if (fractionWithQ >= static_cast<INT32>(MAX_FRACTION_WITH_Q))
        {
            fractional = 0.5;
            exp++;
        }
        if (-exp + 2 >= 0)
        {
            pUnpackedField->radialActivityCFShift[i]    = static_cast<UINT8>(-exp + 2);
            pUnpackedField->radialActivitySlopeTable[i] =
                static_cast<INT16>(ceil(fractional * (1 << RADIAL_ACT_SLOPE_Q_FACTOR)));
        }
        else
        {
            pUnpackedField->radialActivityCFShift[i]    = 0;
            pUnpackedField->radialActivitySlopeTable[i] =
                static_cast<INT16>(ceil(fractional * (1ULL << (RADIAL_ACT_SLOPE_Q_FACTOR + exp - 2))));
        }
        if (slope < 0)
        {
            pUnpackedField->radialActivitySlopeTable[i] *= -1;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateSkinSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateSkinSetting(
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField)
{
    pUnpackedField->minHue              = IQSettingUtils::ClampUINT32(
        abs(IQSettingUtils::FloatToQNumber(pData->skin_hue_min, 8)), 0, 1023);
    pUnpackedField->maxHue              = IQSettingUtils::ClampUINT32(
        abs(IQSettingUtils::FloatToQNumber(pData->skin_hue_max, 8)), 0, 255);
    pUnpackedField->minY                = IQSettingUtils::ClampUINT32(
        IQSettingUtils::FloatToQNumber(pData->skin_y_min, 8), 0, 255);
    pUnpackedField->maxY                = IQSettingUtils::ClampUINT32(
        IQSettingUtils::FloatToQNumber(pData->skin_y_max, 8), 0, 255);
    pUnpackedField->boundaryProbability = static_cast<UINT16>(IQSettingUtils::ClampUINT32(
        IQSettingUtils::RoundFLOAT(pData->skin_boundary_probability), 1, 15));

    FLOAT minSlY = pData->skin_saturation_min_ymin;
    FLOAT maxSlY = pData->skin_saturation_max_ymin;
    FLOAT minShY = pData->skin_saturation_min_ymax;
    FLOAT maxShY = pData->skin_saturation_max_ymax;

    pUnpackedField->minShY = IQSettingUtils::ClampUINT32(
        IQSettingUtils::FloatToQNumber(minShY, 8), 0, 255);
    pUnpackedField->maxShY = IQSettingUtils::ClampUINT32(
        IQSettingUtils::FloatToQNumber(maxShY, 8), 0, 255);

    if (pData->skin_y_max > pData->skin_y_min)
    {
        pUnpackedField->paraSmin = IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
            (minSlY - minShY) / (pData->skin_y_max - pData->skin_y_min), SKIN_SATURATIN_Q_FACTOR),
            MIN_UINT8, MAX_UINT8);
        pUnpackedField->paraSmax = IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
            (maxSlY - maxShY) / (pData->skin_y_max - pData->skin_y_min), SKIN_SATURATIN_Q_FACTOR),
            MIN_UINT8, MAX_UINT8);
    }
    else
    {
        pUnpackedField->paraSmin = 0;
        pUnpackedField->paraSmax = 0;
    }

    FLOAT skinQ              = ((100.0f - pData->skin_percent) /
        (200.0f *(16 - pData->skin_boundary_probability)))*(1 << SKIN_SATURATIN_Q_FACTOR);
    pUnpackedField->skinQ    = static_cast<UINT16>(IQSettingUtils::ClampINT32(
        IQSettingUtils::RoundFLOAT(skinQ), SKIN_Q_NONQ_MIN, SKIN_Q_NONQ_MAX));
    pUnpackedField->nonskinQ = static_cast<UINT16>(IQSettingUtils::ClampINT32(
        IQSettingUtils::RoundFLOAT(pData->skin_nonskin_to_skin_qratio*skinQ), SKIN_Q_NONQ_MIN, SKIN_Q_NONQ_MAX));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Setting::CalculateFaceSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ASF30Setting::CalculateFaceSetting(
    const ASF30InputData*               pInput,
    asf_3_0_0::asf30_rgn_dataType*      pData,
    ASF30UnpackedField*                 pUnpackedField)
{
    UINT32 i;
    INT32  exp;
    DOUBLE fractional;
    INT32  fractionWithQ;

    if (NULL != pInput->pFDData)
    {
        if (pInput->pFDData->numberOfFace > MAX_FACE_NUM)
        {
            pUnpackedField->faceNum = 0;
            pUnpackedField->faceEnable = false;
        }
        else if (pInput->pFDData->numberOfFace <= 0)
        {
            pUnpackedField->skinEnable = false;
            pUnpackedField->faceNum = 0;
            pUnpackedField->faceEnable = false;
        }
        else
        {
            pUnpackedField->faceNum = pInput->pFDData->numberOfFace;
            pUnpackedField->faceEnable = true;
        }
        pUnpackedField->faceEnable =
            (pInput->pChromatix->chromatix_asf30_reserve.face_enable == 0) ? false : true; // added as per simulator
        INT16  faceR[MAX_FACE_NUM];

        pUnpackedField->faceHorizontalOffset = pInput->faceHorzOffset;
        pUnpackedField->faceVerticalOffset = pInput->faceVertOffset;

        NcLibWarpGeomOut*  pWarpOut = static_cast<NcLibWarpGeomOut*>(pInput->pWarpGeometriesOutput);
        struct FDData* pFDConfig = NULL;

        pFDConfig = (NULL != pWarpOut) ?
            reinterpret_cast<struct FDData*>(pWarpOut->fdConfig) :
            pInput->pFDData;

        if (NULL != pFDConfig)
        {
            pUnpackedField->faceNum =
                IQSettingUtils::ClampUINT16(static_cast<UINT16>(pFDConfig->numberOfFace), 0, MAX_FACE_NUM);
            if (pUnpackedField->faceNum > 0)
            {
                for (i = 0; i < pUnpackedField->faceNum; i++)
                {
                    pUnpackedField->faceCenterHorizontal[i] = static_cast<UINT16>(pFDConfig->faceCenterX[i]);
                    pUnpackedField->faceCenterVertical[i] = static_cast<UINT16>(pFDConfig->faceCenterY[i]);
                    faceR[i] = static_cast<INT16>(pFDConfig->faceRadius[i]);
                }

                for (i = 0; i < pUnpackedField->faceNum; i++)
                {
                    FLOAT r_boundary = (faceR[i] * pData->face_boundary);
                    FLOAT r_transition = (faceR[i] * pData->face_transition);

                    fractional = frexp(IQSettingUtils::MaxFLOAT(r_transition, r_boundary), &exp);
                    fractionWithQ = static_cast<INT32>(ceil(fractional * (1 << FACE_Q_BITS)));

                    if (fractionWithQ >= (1 << FACE_Q_BITS))
                    {
                        exp++;
                    }
                    if ((exp - static_cast<INT32>(FACE_Q_BITS)) >= 0)
                    {
                        pUnpackedField->faceRadiusShift[i] = static_cast<UINT16>(exp - FACE_Q_BITS);
                    }
                    else
                    {
                        pUnpackedField->faceRadiusShift[i] = 0;
                    }

                    pUnpackedField->faceRadiusBoundary[i] = static_cast<UINT16>(
                        ceil(r_boundary / (1 << pUnpackedField->faceRadiusShift[i])));

                    INT faceRadiusTransition = static_cast<INT>(ceil(r_transition / (1 << pUnpackedField->faceRadiusShift[i])));

                    if (faceRadiusTransition > pUnpackedField->faceRadiusBoundary[i])
                    {
                        DOUBLE delta = 1.0 / (faceRadiusTransition - pUnpackedField->faceRadiusBoundary[i]);
                        fractional = frexp(delta, &exp);
                        fractionWithQ = static_cast<INT32>(ceil(fractional * (1 << FACE_Q_BITS)));
                        if (fractionWithQ >= (1 << FACE_Q_BITS))
                        {
                            fractional = 0.5;
                            exp++;
                        }
                        if (-exp + 1 >= 0)
                        {
                            pUnpackedField->faceSlopeShift[i] = static_cast<UINT16>(-exp + 1);
                        }
                        else
                        {
                            pUnpackedField->faceSlopeShift[i] = 0;
                        }
                        pUnpackedField->faceRadiusSlope[i] = static_cast<UINT16>(ceil(fractional * (1 << FACE_Q_BITS)));
                    }
                    else
                    {
                        pUnpackedField->faceRadiusSlope[i] = static_cast<UINT16>((1 << FACE_Q_BITS) - 1);
                        pUnpackedField->faceSlopeShift[i] = 0;
                    }
                }
            }
        }
    }
    else
    {
        pUnpackedField->faceNum = 0;
        pUnpackedField->faceEnable = false;
    }
}
