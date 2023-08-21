// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cvp10setting.cpp
/// @brief CVP10 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "cvp10setting.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVP10Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CVP10Setting::CalculateHWSetting(
    CVP10InputData*                                       pInput,
    cvp_1_0_0::cvp10_rgn_dataType*                        pData,
    cvp_1_0_0::chromatix_cvp10_reserveType*               pReserveType,
    VOID*                                                 pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pReserveType)  &&
        (NULL != pOutput))

    {
        CVP10UnpackedField* pUnpackedField = static_cast<CVP10UnpackedField*>(pOutput);

        pUnpackedField->robustness_max_allowed_NCC = static_cast<FLOAT>(IQSettingUtils::ClampUINT32(
            IQSettingUtils::RoundFLOAT(pData->robustness_max_allowed_ncc),
            ROBUSTNESS_MAX_ALLOWED_NCC_MIN,
            ROBUSTNESS_MAX_ALLOWED_NCC_MAX));

        pUnpackedField->robustness_min_allowed_tar_var = static_cast<FLOAT>(IQSettingUtils::ClampUINT32(
            IQSettingUtils::RoundFLOAT(pData->robustness_min_allowed_tar_var),
            ROBUSTNESS_MIN_ALLOWED_TAR_VAR_MIN,
            ROBUSTNESS_MIN_ALLOWED_TAR_VAR_MAX));

        pUnpackedField->robustness_meaningful_ncc_diff = static_cast<FLOAT>(IQSettingUtils::ClampUINT32(
            IQSettingUtils::RoundFLOAT(pData->robustness_meaningful_ncc_diff),
            ROBUSTNESS_MEANINGFUL_NCC_DIFF_MIN,
            ROBUSTNESS_MEANINGFUL_NCC_DIFF_MAX));

        pUnpackedField->fpx_threshold = static_cast<FLOAT>(IQSettingUtils::ClampUINT32(
            IQSettingUtils::RoundFLOAT(pData->fpx_threshold),
            FPX_THRESHOLD_MIN,
            FPX_THRESHOLD_MAX));


        pUnpackedField->desc_match_threshold = static_cast<FLOAT>(IQSettingUtils::ClampUINT32(
            IQSettingUtils::RoundFLOAT(pData->desc_match_threshold),
            DESC_MATCH_THRESHOLD_MIN,
            DESC_MATCH_THRESHOLD_MAX));

        pUnpackedField->enable_transform_confidence = static_cast<FLOAT>(IQSettingUtils::ClampUINT32(
            IQSettingUtils::RoundFLOAT(pData->enable_transform_confidence),
            ENABLE_TRANSFORM_CONFIDENCE_MIN,
            ENABLE_TRANSFORM_CONFIDENCE_MAX));

        pUnpackedField->transform_confidence_mapping_base = static_cast<FLOAT>(IQSettingUtils::ClampUINT32(
            IQSettingUtils::RoundFLOAT(pData->transform_confidence_mapping_base),
            TRANSFORM_CONFIDENCE_MAPPING_BASE_MIN,
            TRANSFORM_CONFIDENCE_MAPPING_BASE_MAX));

        pUnpackedField->transform_confidence_mapping_c1 = static_cast<FLOAT>(IQSettingUtils::ClampUINT32(
            IQSettingUtils::RoundFLOAT(pData->transform_confidence_mapping_c1),
            TRANSFORM_CONFIDENCE_MAPPING_C1_MIN,
            TRANSFORM_CONFIDENCE_MAPPING_C1_MAX));

        pUnpackedField->transform_confidence_mapping_c2 = static_cast<FLOAT>(IQSettingUtils::ClampUINT32(
            IQSettingUtils::RoundFLOAT(pData->transform_confidence_mapping_c2),
            TRANSFORM_CONFIDENCE_MAPPING_C2_MIN,
            TRANSFORM_CONFIDENCE_MAPPING_C2_MAX));

        pUnpackedField->transform_confidence_thr_to_force_identity_transform = static_cast<FLOAT>(IQSettingUtils::ClampUINT32(
            IQSettingUtils::RoundFLOAT(pData->transform_confidence_thr_to_force_identity_transform),
            TRANSFORM_CONFIDENCE_THR_TO_FORCE_IDENTITY_TRANSFORM_MIN,
            TRANSFORM_CONFIDENCE_THR_TO_FORCE_IDENTITY_TRANSFORM_MAX));

        pUnpackedField->descriptor_Lpf = IQSettingUtils::ClampUINT32(pReserveType->descriptor_lpf,
            DESCRIPTOR_LPF_MIN,
            DESCRIPTOR_LPF_MAX);

        pUnpackedField->fpx_score_shift = IQSettingUtils::ClampUINT32(pReserveType->fpx_score_shift,
            FPX_SCORE_SHIFT_MIN,
            FPX_SCORE_SHIFT_MAX);

        pUnpackedField->inlier_track_enable = IQSettingUtils::ClampUINT32(pReserveType->inlier_track_enable,
            INLIER_TRACK_ENABLE_MIN,
            INLIER_TRACK_ENABLE_MAX);

        pUnpackedField->transform_model = IQSettingUtils::ClampUINT32(pReserveType->transform_model,
            TRANSFORM_MODEL_MIN,
            TRANSFORM_MODEL_MAX);

        for (INT i = 0; i < 8; i++)
        {
            pUnpackedField->robustness_measure_dist_map [i]= IQSettingUtils::ClampUINT32(
                IQSettingUtils::RoundFLOAT(pData->robustness_measure_dist_map_tab.robustness_measure_dist_map[i]),
                ROBUSTNESS_MEASURE_DIST_MAP_MIN,
                ROBUSTNESS_MEASURE_DIST_MAP_MAX);
        }
        pUnpackedField->multi_frame_input_resolution = IQSettingUtils::ClampUINT32(
            static_cast<UINT32>(pData->multi_frame_input_resolution),
            MULTI_FRAME_INPUT_RES_MIN,
            MULTI_FRAME_INPUT_RES_MAX);
        pUnpackedField->video_registration_down_scale_ratio = IQSettingUtils::ClampFLOAT(
            pData->video_registration_down_scale_ratio,
            VIDEO_REG_DS_RATIO_MIN,
            VIDEO_REG_DS_RATIO_MAX);
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }
    return result;
}
