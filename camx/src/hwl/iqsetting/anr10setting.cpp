// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  anr10setting.cpp
/// @brief anr10 IQ setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "NcLibWarp.h"
#include "anr10setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ANR10Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ANR10Setting::CalculateHWSetting(
    const ANR10InputData*                                 pInput,
    anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct*    pData,
    anr_1_0_0::chromatix_anr10_reserveType*               pReserveData,
    anr_1_0_0::chromatix_anr10Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    BOOL  result = TRUE;
    INT32 intRet = 0;

    CAMX_UNREFERENCED_PARAM(pOutput);

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pReserveData)  &&
        (NULL != pModuleEnable))
    {
        UINT32               numOfPasses                 = pInput->numPasses;
        ANR_Chromatix*       pAnrChromatix               = static_cast<ANR_Chromatix*>(pInput->pNCChromatix);

        IQSettingUtils::Memset(pAnrChromatix, 0x0, sizeof(ANR_Chromatix)*pInput->numPasses);

        CAMX_ASSERT(pInput->numPasses <= PASS_NAME_MAX);

        result = MapChromatix2ANRChromatix(pReserveData->mod_anr10_pass_reserve_data,
                                           pData,
                                           pInput,
                                           numOfPasses,
                                           pAnrChromatix);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "pInput=%p, pData=%p", pInput, pData);
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ANR10Setting::GetAnrPerPassRegionData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
anr_1_0_0::anr10_rgn_dataType* ANR10Setting::GetAnrPerPassRegionData(
    anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct*  pUnpackedData,
    const ANR10InputData*                               pInput,
    UINT32                                              passType)
{
    anr_1_0_0::anr10_rgn_dataType*  pPassUnpackedData = NULL;

    if (pInput->numPasses < pInput->maxSupportedPassesForUsecase)
    {
        pPassUnpackedData = (0 == passType) ? &pUnpackedData->mod_anr10_pass_data[passType].anr10_rgn_data :
            &pUnpackedData->mod_anr10_pass_data[
                (pInput->maxSupportedPassesForUsecase - pInput->numPasses + passType)].anr10_rgn_data;
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "maxSupportedPasses %d,maxPasses %d,nclibPassindex %d,chromatixPassIndex %d",
            pInput->maxSupportedPassesForUsecase, pInput->numPasses, passType,
            (0 == passType) ? passType : (pInput->maxSupportedPassesForUsecase - pInput->numPasses + passType));
    }
    else
    {
        pPassUnpackedData = &pUnpackedData->mod_anr10_pass_data[passType].anr10_rgn_data;
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "maxSupportedPassess %d,maxPasses %d,chromatixPassindex-nclibPassIndex %d",
            pInput->maxSupportedPassesForUsecase, pInput->numPasses, passType);
    }
    return pPassUnpackedData;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ANR10Setting::GetAnrPerPassReservedData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
anr_1_0_0::mod_anr10_pass_reserve_dataType::pass_dataStruct* ANR10Setting::GetAnrPerPassReservedData(
    anr_1_0_0::mod_anr10_pass_reserve_dataType*         pReserveData,
    const ANR10InputData*                               pInput,
    UINT32                                              passType)
{
    anr_1_0_0::mod_anr10_pass_reserve_dataType::pass_dataStruct* pPassReserveData = NULL;

    if (pInput->numPasses < pInput->maxSupportedPassesForUsecase)
    {
        pPassReserveData = (0 == passType) ?
            &pReserveData[passType].pass_data :
            &pReserveData[(pInput->maxSupportedPassesForUsecase - pInput->numPasses + passType)].pass_data;
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "maxSupportedPasses %d,maxPasses %d,nclibPassindex %d,chromatixPassIndex %d",
            pInput->maxSupportedPassesForUsecase, pInput->numPasses, passType,
            (0 == passType) ? passType : (pInput->maxSupportedPassesForUsecase - pInput->numPasses + passType));
    }
    else
    {
        pPassReserveData = &pReserveData[passType].pass_data;
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "maxSupportedPassess %d,maxPasses %d,chromatixPassindex-nclibPassIndex %d",
            pInput->maxSupportedPassesForUsecase, pInput->numPasses, passType);
    }
    return pPassReserveData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ANR10Setting::MapChromatix2ANRChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ANR10Setting::MapChromatix2ANRChromatix(
    anr_1_0_0::mod_anr10_pass_reserve_dataType*         pReserveData,
    anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct*  pUnpackedData,
    const ANR10InputData*                               pInput,
    UINT32                                              passNumMax,
    ANR_Chromatix*                                      pANRChromatix)
{
    ANR_Chromatix*                                               pPassANRChromatix = NULL;
    anr_1_0_0::anr10_rgn_dataType*                               pPassUnpackedData = NULL;
    anr_1_0_0::mod_anr10_pass_reserve_dataType::pass_dataStruct* pPassReserveData  = NULL;
    BOOL                                                         result            = TRUE;

    UINT16 lumaEnable   = 0;
    UINT16 chormaEnable = 0;
    UINT   index        = 0;

    for (UINT passType = 0; passType < passNumMax; passType++)
    {
        pPassReserveData  = GetAnrPerPassReservedData(pReserveData, pInput, passType);
        if (NULL != pPassReserveData)
        {
            lumaEnable   = lumaEnable | (pPassReserveData->top.enable_luma_noise_reduction_pass & 1);
            chormaEnable = chormaEnable | (pPassReserveData->top.enable_chroma_noise_reduction_pass & 1);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid pPassReserveData %p passType %d", pPassReserveData, passType);
        }
    }

    for (UINT passType = 0; passType < passNumMax; passType++)
    {
        pPassReserveData  = GetAnrPerPassReservedData(pReserveData, pInput, passType);
        pPassUnpackedData = GetAnrPerPassRegionData(pUnpackedData, pInput, passType);

        if ((NULL == pPassReserveData) || (NULL == pPassUnpackedData))
        {
            result = FALSE;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid pPassReserveData %p or pPassUnpackedData %p for passType %d",
                pPassReserveData, pPassUnpackedData, passType);
            break;
        }

        pPassANRChromatix = &pANRChromatix[passType];

        // Top level prams
        pPassANRChromatix->top.enable_luma_noise_reduction   = lumaEnable;
        pPassANRChromatix->top.enable_chroma_noise_reduction = chormaEnable;


        pPassANRChromatix->top.enable_luma_noise_reduction_pass   = pPassReserveData->top.enable_luma_noise_reduction_pass;
        pPassANRChromatix->top.enable_chroma_noise_reduction_pass = pPassReserveData->top.enable_chroma_noise_reduction_pass;

        // power control
        pPassANRChromatix->power_control.enable_chroma_filter_extension                       =
            pPassReserveData->power_control.enable_chroma_filter_extension;
        pPassANRChromatix->power_control.enable_luma_smoothing_treatment_and_peak_treatment   =
            pPassReserveData->power_control.enable_luma_smoothing_treatment_and_peak_treatment;
        pPassANRChromatix->power_control.enable_chroma_smoothing_treatment_and_peak_treatment =
            pPassReserveData->power_control.enable_chroma_smoothing_treatment_and_peak_treatment;
        pPassANRChromatix->power_control.enable_luma_chroma_filter_all_thresholds_per_uv      =
            pPassReserveData->power_control.enable_luma_chroma_filter_all_thresholds_per_uv;
        pPassANRChromatix->power_control.enable_luma_chroma_filter_uv_thresholds_per_yuv      =
            pPassReserveData->power_control.enable_luma_chroma_filter_uv_thresholds_per_yuv;
        pPassANRChromatix->power_control.enable_luma_filter_uv_thresholds                     =
            pPassReserveData->power_control.enable_luma_filter_uv_thresholds;


        // Filter Configuration
        pPassANRChromatix->luma_chroma_filter_config.threshold_lut_control_avg_block_size =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->luma_chroma_filter_config.threshold_lut_control_avg_block_size);
        pPassANRChromatix->luma_filter_config.filter_decision_mode                        =
            pPassReserveData->luma_filter_config.filter_decision_mode;
        pPassANRChromatix->luma_filter_config.filter_isotropic_min_filter_size            =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_config.filter_isotropic_min_filter_size);
        pPassANRChromatix->luma_filter_config.filter_enable_external_derivatives          =
            pPassReserveData->luma_filter_config.filter_enable_external_derivatives;
        pPassANRChromatix->luma_filter_config.filter_manual_derivatives_flags             =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->luma_filter_config.filter_manual_derivatives_flags);
        pPassANRChromatix->luma_filter_config.dcind_decision_mode                         =
            pPassReserveData->luma_filter_config.dcind_decision_mode;
        pPassANRChromatix->luma_filter_config.dcind_isotropic_min_size                    =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_config.dcind_isotropic_min_size);
        pPassANRChromatix->luma_filter_config.dcind_enable_external_derivatives           =
            pPassReserveData->luma_filter_config.dcind_enable_external_derivatives;
        pPassANRChromatix->luma_filter_config.dcind_manual_derivatives_flags              =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->luma_filter_config.dcind_manual_derivatives_flags);
        pPassANRChromatix->luma_filter_config.enable_use_second_derivative_for_luma_3x3   =
            pPassReserveData->luma_filter_config.enable_use_second_derivative_for_luma_3x3;

        // Filter config second derivative
        pPassANRChromatix->luma_filter_config.second_derivative_max_influence_radius_filtering    =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->luma_filter_config.second_derivative_max_influence_radius_filtering);
        pPassANRChromatix->luma_filter_config.second_derivative_max_influence_radius_dc_indication =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_config.second_derivative_max_influence_radius_dc_indication);

        // Chroma filter
        pPassANRChromatix->chroma_filter_config.filter_decision_mode             =
            pPassReserveData->chroma_filter_config.filter_decision_mode;
        pPassANRChromatix->chroma_filter_config.filter_isotropic_min_filter_size =
            IQSettingUtils::Ceiling(pPassUnpackedData->chroma_filter_config.filter_isotropic_min_filter_size);
        pPassANRChromatix->chroma_filter_config.filter_manual_derivatives_flags  =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_config.filter_manual_derivatives_flags);


        // Chroma filter config
        pPassANRChromatix->chroma_filter_config.dcind_decision_mode            =
            pPassReserveData->chroma_filter_config.dcind_decision_mode;
        pPassANRChromatix->chroma_filter_config.dcind_isotropic_min_size       =
            IQSettingUtils::Ceiling(pPassUnpackedData->chroma_filter_config.dcind_isotropic_min_size);
        pPassANRChromatix->chroma_filter_config.dcind_manual_derivatives_flags =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_config.dcind_manual_derivatives_flags);

        // Luma filter config
        pPassANRChromatix->luma_filter_kernel.edge_kernel_size                          =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.edge_kernel_size);
        pPassANRChromatix->luma_filter_kernel.kernel_definition_mode                    =
            pPassReserveData->luma_filter_kernel.kernel_definition_mode;
        pPassANRChromatix->luma_filter_kernel.automatic_definition_granularity          =
            IQSettingUtils::Floor(pPassUnpackedData->luma_filter_kernel.automatic_definition_granularity);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_1x1_center_coefficient =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_edge_kernel_1x1_center_coefficient);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_3x3_horver_shift       =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_edge_kernel_3x3_horver_shift);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_3x3_diag_shift         =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_edge_kernel_3x3_diag_shift);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_5x5_horver_shift       =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_edge_kernel_5x5_horver_shift);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_5x5_diag_shift         =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_edge_kernel_5x5_diag_shift);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_5x5_complement_shift   =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_edge_kernel_5x5_complement_shift);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_7x7_horver_shift       =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_edge_kernel_7x7_horver_shift);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_7x7_diag_shift         =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_edge_kernel_7x7_diag_shift);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_7x7_complement_shift   =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_edge_kernel_7x7_complement_shift);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_9x9_horver_shift       =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_edge_kernel_9x9_horver_shift);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_9x9_diag_shift         =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_edge_kernel_9x9_diag_shift);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_9x9_complement_shift   =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_edge_kernel_9x9_complement_shift);
        pPassANRChromatix->luma_filter_kernel.manual_edge_kernel_complement_mode        =
            pPassReserveData->luma_filter_kernel.manual_edge_kernel_complement_mode;

        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_blend_weight           =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_blend_weight);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_size                   =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_size);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_1x1_center_coefficient =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_1x1_center_coefficient);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_3x3_horver_shift       =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_3x3_horver_shift);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_3x3_diag_shift         =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_3x3_diag_shift);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_5x5_horver_shift       =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_5x5_horver_shift);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_5x5_diag_shift         =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_5x5_diag_shift);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_5x5_complement_shift   =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_5x5_complement_shift);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_7x7_horver_shift       =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_7x7_horver_shift);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_7x7_diag_shift         =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_7x7_diag_shift);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_7x7_complement_shift   =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_7x7_complement_shift);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_9x9_horver_shift       =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_9x9_horver_shift);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_9x9_diag_shift         =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_9x9_diag_shift);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_9x9_complement_shift   =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_filter_kernel.manual_flat_kernel_9x9_complement_shift);
        pPassANRChromatix->luma_filter_kernel.manual_flat_kernel_complement_mode        =
            pPassReserveData->luma_filter_kernel.manual_flat_kernel_complement_mode;
        pPassANRChromatix->chroma_filter_kernel.kernel_size                             =
            IQSettingUtils::Ceiling(pPassUnpackedData->chroma_filter_kernel.kernel_size);

        // Luma peak management
        pPassANRChromatix->luma_peak_management.enable_luma_peak_management                            =
            pPassReserveData->luma_peak_management.enable_luma_peak_management;
        pPassANRChromatix->luma_peak_management.detect_hard_decision_environment_activity              =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->luma_peak_management.detect_hard_decision_environment_activity);
        pPassANRChromatix->luma_peak_management.detect_hard_decision_distance_from_average             =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->luma_peak_management.detect_hard_decision_distance_from_average);
        pPassANRChromatix->luma_peak_management.detect_soft_decision_distance_from_average_lower_limit =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_peak_management.detect_soft_decision_distance_from_average_lower_limit);
        pPassANRChromatix->luma_peak_management.detect_soft_decision_distance_from_average_offset      =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_peak_management.detect_soft_decision_distance_from_average_offset);
        pPassANRChromatix->luma_peak_management.detect_soft_decision_distance_from_average_slope       =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_peak_management.detect_soft_decision_distance_from_average_slope);
        pPassANRChromatix->luma_peak_management.detect_extreme_decision_distance_from_maxmin           =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_peak_management.detect_extreme_decision_distance_from_maxmin);
        pPassANRChromatix->luma_peak_management.detect_dcsize_decision_sensitivity                     =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_peak_management.detect_dcsize_decision_sensitivity);
        pPassANRChromatix->luma_peak_management.correction_mode                                        =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->luma_peak_management.correction_mode);
        pPassANRChromatix->luma_peak_management.correction_area_smart_min_inner_distance               =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->luma_peak_management.correction_area_smart_min_inner_distance);
        pPassANRChromatix->luma_peak_management.correction_isotropic_activity_threshold                =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_peak_management.correction_isotropic_activity_threshold);

        // Chroma Peak management
        pPassANRChromatix->chroma_peak_management.enable_chroma_peak_management                          =
            pPassReserveData->chroma_peak_management.enable_chroma_peak_management;
        pPassANRChromatix->chroma_peak_management.detect_hard_decision_environment_activity              =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_peak_management.detect_hard_decision_environment_activity);
        pPassANRChromatix->chroma_peak_management.detect_hard_decision_distance_from_average             =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_peak_management.detect_hard_decision_distance_from_average);
        pPassANRChromatix->chroma_peak_management.detect_soft_decision_distance_from_average_lower_limit =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_peak_management.detect_soft_decision_distance_from_average_lower_limit);
        pPassANRChromatix->chroma_peak_management.detect_soft_decision_distance_from_average_offset      =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_peak_management.detect_soft_decision_distance_from_average_offset);
        pPassANRChromatix->chroma_peak_management.detect_soft_decision_distance_from_average_slope       =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_peak_management.detect_soft_decision_distance_from_average_slope);
        pPassANRChromatix->chroma_peak_management.detect_extreme_decision_distance_from_maxmin           =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_peak_management.detect_extreme_decision_distance_from_maxmin);
        pPassANRChromatix->chroma_peak_management.detect_dcsize_decision_sensitivity                     =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_peak_management.detect_dcsize_decision_sensitivity);
        pPassANRChromatix->chroma_peak_management.correction_mode                                        =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_peak_management.correction_mode);
        pPassANRChromatix->chroma_peak_management.correction_area_smart_min_inner_distance               =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->chroma_peak_management.correction_area_smart_min_inner_distance);
        pPassANRChromatix->chroma_peak_management.correction_isotropic_activity_threshold                =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_peak_management.correction_isotropic_activity_threshold);
        pPassANRChromatix->inter_length_thr_modification.luma_input_indication_thr_modification_scale    =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->inter_length_thr_modification.luma_input_indication_thr_modification_scale);
        pPassANRChromatix->inter_length_thr_modification.chroma_input_indication_thr_modification_scale  =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->inter_length_thr_modification.chroma_input_indication_thr_modification_scale);

        pPassANRChromatix->inter_length_output_indication.luma_center_binarization_minflatval            =
            IQSettingUtils::Ceiling(pPassUnpackedData->inter_length_output_indication.luma_center_binarization_minflatval);
        pPassANRChromatix->inter_length_output_indication.luma_neighbours_impact_enable                  =
            pPassReserveData->inter_length_output_indication.luma_neighbours_impact_enable;
        pPassANRChromatix->inter_length_output_indication.luma_neighbours_binarization_minflatval        =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->inter_length_output_indication.luma_neighbours_binarization_minflatval);
        pPassANRChromatix->inter_length_output_indication.luma_neighbours_parallel_dist                  =
            IQSettingUtils::Ceiling(pPassUnpackedData->inter_length_output_indication.luma_neighbours_parallel_dist);
        pPassANRChromatix->inter_length_output_indication.luma_neighbours_perpendicular_dist             =
            IQSettingUtils::Ceiling(pPassUnpackedData->inter_length_output_indication.luma_neighbours_perpendicular_dist);
        pPassANRChromatix->inter_length_output_indication.luma_neighbours_agreement_sensitivity          =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->inter_length_output_indication.luma_neighbours_agreement_sensitivity);
        pPassANRChromatix->inter_length_output_indication.chroma_center_binarization_minflatval          =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->inter_length_output_indication.chroma_center_binarization_minflatval);
        pPassANRChromatix->inter_length_output_indication.chroma_neighbours_impact_enable                =
            pPassReserveData->inter_length_output_indication.chroma_neighbours_impact_enable;
        pPassANRChromatix->inter_length_output_indication.chroma_neighbours_binarization_minflatval      =
            IQSettingUtils::Ceiling(
                pPassUnpackedData->inter_length_output_indication.chroma_neighbours_binarization_minflatval);
        pPassANRChromatix->inter_length_output_indication.chroma_neighbours_perpendicular_dist           =
            IQSettingUtils::Ceiling(pPassUnpackedData->inter_length_output_indication.chroma_neighbours_perpendicular_dist);
        pPassANRChromatix->inter_length_output_indication.chroma_neighbours_agreement_sensitivity        =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->inter_length_output_indication.chroma_neighbours_agreement_sensitivity);

        pPassANRChromatix->grey_treatment.enable_grey_treatment_thr_modification                         =
            pPassReserveData->grey_treatment.enable_grey_treatment_thr_modification;
        pPassANRChromatix->grey_treatment.enable_grey_treatment_isotropic_filter_blend                   =
            pPassReserveData->grey_treatment.enable_grey_treatment_isotropic_filter_blend;
        pPassANRChromatix->grey_treatment.enable_grey_treatment_dcblend2_chroma_modification             =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->grey_treatment.enable_grey_treatment_dcblend2_chroma_modification);
        pPassANRChromatix->grey_treatment.detect_grey_condition_chromaticity_radius                      =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.detect_grey_condition_chromaticity_radius);
        pPassANRChromatix->grey_treatment.detect_grey_condition_chromaticity_thr_low                     =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.detect_grey_condition_chromaticity_thr_low);
        pPassANRChromatix->grey_treatment.detect_grey_condition_chromaticity_thr_high                    =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.detect_grey_condition_chromaticity_thr_high);
        pPassANRChromatix->grey_treatment.detect_grey_condition_y_max_derivative_radius                  =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.detect_grey_condition_y_max_derivative_radius);
        pPassANRChromatix->grey_treatment.detect_grey_condition_y_max_derivative_thr_low                 =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.detect_grey_condition_y_max_derivative_thr_low);
        pPassANRChromatix->grey_treatment.detect_grey_condition_y_max_derivative_thr_high                =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.detect_grey_condition_y_max_derivative_thr_high);
        pPassANRChromatix->grey_treatment.detect_greydcsize_neighbors_chromaticity_thr                   =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.detect_greydcsize_neighbors_chromaticity_thr);
        pPassANRChromatix->grey_treatment.detect_greydcsize_center_detail_chromaticity_thr               =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.detect_greydcsize_center_detail_chromaticity_thr);
        pPassANRChromatix->grey_treatment.thr_modification_target_y                                      =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.thr_modification_target_y);
        pPassANRChromatix->grey_treatment.thr_modification_target_u                                      =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.thr_modification_target_u);
        pPassANRChromatix->grey_treatment.thr_modification_target_v                                      =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.thr_modification_target_v);
        pPassANRChromatix->grey_treatment.isotropic_filter_blend_factor_scale                            =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.isotropic_filter_blend_factor_scale);
        pPassANRChromatix->grey_treatment.isotropic_filter_blend_factor_offset                           =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.isotropic_filter_blend_factor_offset);
        pPassANRChromatix->grey_treatment.isotropic_filter_size                                          =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->grey_treatment.isotropic_filter_size);

        // Chroma filter extnesion
        pPassANRChromatix->chroma_filter_extension.enable_chroma_filter_extension_median                            =
            pPassReserveData->chroma_filter_extension.enable_chroma_filter_extension_median;
        pPassANRChromatix->chroma_filter_extension.median_detect_override_detail_condition_chromaticity_thr         =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_extension.median_detect_override_detail_condition_chromaticity_thr);
        pPassANRChromatix->chroma_filter_extension.median_detect_override_detail_condition_y_max_derivative_thr     =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_extension.median_detect_override_detail_condition_y_max_derivative_thr);
        pPassANRChromatix->chroma_filter_extension.median_detect_corner_detail_sensitivity_y                        =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_extension.median_detect_corner_detail_sensitivity_y);
        pPassANRChromatix->chroma_filter_extension.median_detect_corner_detail_sensitivity_uv                       =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_extension.median_detect_corner_detail_sensitivity_uv);
        pPassANRChromatix->chroma_filter_extension.median_detect_triple_chromaticities_detail_thr_up_down           =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_extension.median_detect_triple_chromaticities_detail_thr_up_down);
        pPassANRChromatix->chroma_filter_extension.median_detect_triple_chromaticities_detail_thr_external          =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_extension.median_detect_triple_chromaticities_detail_thr_external);
        pPassANRChromatix->chroma_filter_extension.median_detect_isotropic_self_decision_enforce_detail             =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_extension.median_detect_isotropic_self_decision_enforce_detail);
        pPassANRChromatix->chroma_filter_extension.median_detect_isotropic_neighbors_detail_sensitivity             =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_extension.median_detect_isotropic_neighbors_detail_sensitivity);
        pPassANRChromatix->chroma_filter_extension.median_detect_isotropic_neighbors_detail_sensitivity_far_scale   =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_extension.median_detect_isotropic_neighbors_detail_sensitivity_far_scale);
        pPassANRChromatix->chroma_filter_extension.median_detect_directional_self_decision_enforce_detail           =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_extension.median_detect_directional_self_decision_enforce_detail);
        pPassANRChromatix->chroma_filter_extension.median_detect_directional_neighbors_detail_sensitivity           =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_extension.median_detect_directional_neighbors_detail_sensitivity);
        pPassANRChromatix->chroma_filter_extension.median_detect_directional_neighbors_detail_sensitivity_far_scale =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_filter_extension.median_detect_directional_neighbors_detail_sensitivity_far_scale);

        // Chroma filter extension
        pPassANRChromatix->chroma_filter_extension.enable_chroma_filter_extension_bilateral         =
            pPassReserveData->chroma_filter_extension.enable_chroma_filter_extension_bilateral;
        pPassANRChromatix->chroma_filter_extension.bilateral_decision_minimalsize                   =
            IQSettingUtils::Ceiling(pPassUnpackedData->chroma_filter_extension.bilateral_decision_minimalsize);
        pPassANRChromatix->chroma_filter_extension.bilateral_filtersize                             =
            IQSettingUtils::Ceiling(pPassUnpackedData->chroma_filter_extension.bilateral_filtersize);
        pPassANRChromatix->chroma_filter_extension.chroma_flat_indication_extension_threshold_9x9   =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->chroma_filter_extension.chroma_flat_indication_extension_threshold_9x9);
        pPassANRChromatix->chroma_filter_extension.chroma_flat_indication_extension_threshold_11x11 =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->chroma_filter_extension.chroma_flat_indication_extension_threshold_11x11);
        pPassANRChromatix->chroma_filter_extension.chroma_grey_indication_extension_threshold_9x9   =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->chroma_filter_extension.chroma_grey_indication_extension_threshold_9x9);
        pPassANRChromatix->chroma_filter_extension.chroma_grey_indication_extension_threshold_11x11 =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->chroma_filter_extension.chroma_grey_indication_extension_threshold_11x11);

        // Luma smootening treatment
        pPassANRChromatix->luma_smoothing_treatment.enable_luma_edge_smoothing             =
            pPassReserveData->luma_smoothing_treatment.enable_luma_edge_smoothing;
        pPassANRChromatix->luma_smoothing_treatment.edge_smoothing_binarization_minflatval =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->luma_smoothing_treatment.edge_smoothing_binarization_minflatval);
        pPassANRChromatix->luma_smoothing_treatment.edge_smoothing_binarization_maxedgeval =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->luma_smoothing_treatment.edge_smoothing_binarization_maxedgeval);
        pPassANRChromatix->luma_smoothing_treatment.edge_smoothing_agreement_number        =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->luma_smoothing_treatment.edge_smoothing_agreement_number);

        // Luma smootening treatment
        pPassANRChromatix->luma_smoothing_treatment.enable_luma_transition_smoothing                         =
            pPassReserveData->luma_smoothing_treatment.enable_luma_transition_smoothing;
        pPassANRChromatix->luma_smoothing_treatment.transition_smoothing_filter_size                         =
            IQSettingUtils::Ceiling(pPassUnpackedData->luma_smoothing_treatment.transition_smoothing_filter_size);
        pPassANRChromatix->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_minflatval  =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_minflatval);
        pPassANRChromatix->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_minedgeval  =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_minedgeval);
        pPassANRChromatix->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_maxedgeval  =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_maxedgeval);
        pPassANRChromatix->luma_smoothing_treatment.transition_isotropic_neighbours_agreement_sensitivity    =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_smoothing_treatment.transition_isotropic_neighbours_agreement_sensitivity);
        pPassANRChromatix->luma_smoothing_treatment.transition_isotropic_neighbours_agreement_flat_vs_voters =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_smoothing_treatment.transition_isotropic_neighbours_agreement_flat_vs_voters);
        pPassANRChromatix->luma_smoothing_treatment.flat_isotropic_3x3_neighbours_binarization_minflatval    =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->luma_smoothing_treatment.flat_isotropic_3x3_neighbours_binarization_minflatval);
        pPassANRChromatix->luma_smoothing_treatment.flat_isotropic_3x3_neighbours_agreement_sensitivity      =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_smoothing_treatment.flat_isotropic_3x3_neighbours_agreement_sensitivity);
        pPassANRChromatix->luma_smoothing_treatment.flat_isotropic_5x5_neighbours_agreement_sensitivity      =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_smoothing_treatment.flat_isotropic_5x5_neighbours_agreement_sensitivity);
        pPassANRChromatix->luma_smoothing_treatment.smoothing_kernel_3x3_shift                               =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->luma_smoothing_treatment.smoothing_kernel_3x3_shift);
        pPassANRChromatix->luma_smoothing_treatment.smoothing_kernel_5x5_horver_shift                        =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->luma_smoothing_treatment.smoothing_kernel_5x5_horver_shift);

        // Chroma smoothening treatment
        pPassANRChromatix->chroma_smoothing_treatment.enable_chroma_edge_smoothing                             =
            pPassReserveData->chroma_smoothing_treatment.enable_chroma_edge_smoothing;
        pPassANRChromatix->chroma_smoothing_treatment.edge_smoothing_binarization_minflatval                   =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_smoothing_treatment.edge_smoothing_binarization_minflatval);
        pPassANRChromatix->chroma_smoothing_treatment.edge_smoothing_binarization_maxedgeval                   =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_smoothing_treatment.edge_smoothing_binarization_maxedgeval);
        pPassANRChromatix->chroma_smoothing_treatment.edge_smoothing_agreement_number                          =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_smoothing_treatment.edge_smoothing_agreement_number);
        pPassANRChromatix->chroma_smoothing_treatment.enable_chroma_transition_smoothing                       =
            pPassReserveData->chroma_smoothing_treatment.enable_chroma_transition_smoothing;
        pPassANRChromatix->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_minflatval  =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_minflatval);
        pPassANRChromatix->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_minedgeval  =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_minedgeval);
        pPassANRChromatix->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_maxedgeval  =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_maxedgeval);
        pPassANRChromatix->chroma_smoothing_treatment.transition_isotropic_neighbours_agreement_sensitivity    =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_smoothing_treatment.transition_isotropic_neighbours_agreement_sensitivity);
        pPassANRChromatix->chroma_smoothing_treatment.transition_isotropic_neighbours_agreement_flat_vs_voters =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_smoothing_treatment.transition_isotropic_neighbours_agreement_flat_vs_voters);
        pPassANRChromatix->chroma_smoothing_treatment.flat_isotropic_3x3_neighbours_binarization_minflatval    =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->chroma_smoothing_treatment.flat_isotropic_3x3_neighbours_binarization_minflatval);
        pPassANRChromatix->chroma_smoothing_treatment.flat_isotropic_3x3_neighbours_agreement_sensitivity      =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->chroma_smoothing_treatment.flat_isotropic_3x3_neighbours_agreement_sensitivity);
        pPassANRChromatix->chroma_smoothing_treatment.smoothing_kernel_1x1_shift                               =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->chroma_smoothing_treatment.smoothing_kernel_1x1_shift);
        pPassANRChromatix->chroma_smoothing_treatment.smoothing_kernel_3x3_shift                               =
            IQSettingUtils::Ceiling(
            pPassUnpackedData->chroma_smoothing_treatment.smoothing_kernel_3x3_shift);

        pPassANRChromatix->lnr.enable_lnr                                                                      =
            pPassReserveData->lnr.enable_lnr;
        pPassANRChromatix->lnr.use_luts_from_full_pass_configuration                                           =
            pPassReserveData->lnr.use_luts_from_full_pass_configuration;
        pPassANRChromatix->lnr.elliptic_xc                                                                     =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr.elliptic_xc);
        pPassANRChromatix->lnr.elliptic_yc                                                                     =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr.elliptic_yc);
        pPassANRChromatix->lnr.elliptic_a =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr.elliptic_a);

        // LNR
        for (index = 0; index < 17; index++)
        {
            pPassANRChromatix->lnr.luma_filter_lut_thr_y[index]    =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->lnr.luma_filter_lut_thr_y_tab.luma_filter_lut_thr_y[index]);
            pPassANRChromatix->lnr.luma_filter_lut_thr_uv[index]   =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->lnr.luma_filter_lut_thr_uv_tab.luma_filter_lut_thr_uv[index]);
            pPassANRChromatix->lnr.chroma_filter_lut_thr_y[index]  =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->lnr.chroma_filter_lut_thr_y_tab.chroma_filter_lut_thr_y[index]);
            pPassANRChromatix->lnr.chroma_filter_lut_thr_uv[index] =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->lnr.chroma_filter_lut_thr_uv_tab.chroma_filter_lut_thr_uv[index]);
        }

        // LNR
        for (index = 0; index < 17; index++)
        {
            pPassANRChromatix->lnr.strength_modifier_radius_blend_lut[index] =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->lnr.strength_modifier_radius_blend_lut_tab.strength_modifier_radius_blend_lut[index]);
        }

        // LNR
        pPassANRChromatix->lnr.luma_lnr_dcblend2_target_factor               =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr.luma_lnr_dcblend2_target_factor);
        pPassANRChromatix->lnr.luma_lnr_flat_kernel_weight_target_factor     =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr.luma_lnr_flat_kernel_weight_target_factor);
        pPassANRChromatix->lnr.chroma_lnr_cnr_dcblend2_target_factor         =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr.chroma_lnr_cnr_dcblend2_target_factor);
        pPassANRChromatix->lnr.automatic_influence_luma_luts                 =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr.automatic_influence_luma_luts);
        pPassANRChromatix->lnr.automatic_influence_chroma_luts               =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr.automatic_influence_chroma_luts);
        pPassANRChromatix->lnr.automatic_influence_modifier_radius_blend_lut =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr.automatic_influence_modifier_radius_blend_lut);

        // CNR
        pPassANRChromatix->cnr.enable_cnr       = pPassReserveData->cnr.enable_cnr;
        pPassANRChromatix->cnr.input_select     = pPassReserveData->cnr.input_select;
        pPassANRChromatix->cnr.number_of_colors = pPassReserveData->cnr.number_of_colors;

        // CNR
        pPassANRChromatix->cnr.detect_color0_saturation_mode           =
            pPassReserveData->cnr.detect_color0_saturation_mode;
        pPassANRChromatix->cnr.detect_color0_skin_saturation_min_y_min =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->cnr.detect_color0_skin_saturation_min_y_min);
        pPassANRChromatix->cnr.detect_color0_skin_saturation_max_y_min =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->cnr.detect_color0_skin_saturation_max_y_min);
        pPassANRChromatix->cnr.detect_color0_skin_saturation_min_y_max =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->cnr.detect_color0_skin_saturation_min_y_max);
        pPassANRChromatix->cnr.detect_color0_skin_saturation_max_y_max =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->cnr.detect_color0_skin_saturation_max_y_max);

        // CNR
        pPassANRChromatix->cnr.detect_color0_external_enable      =
            pPassReserveData->cnr.detect_color0_external_enable;
        pPassANRChromatix->cnr.color0_transition_ratio_external   =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->cnr.color0_transition_ratio_external);
        pPassANRChromatix->cnr.luma_filter_base_far_modifier_y    =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->cnr.luma_filter_base_far_modifier_y);
        pPassANRChromatix->cnr.luma_filter_base_far_modifier_uv   =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->cnr.luma_filter_base_far_modifier_uv);
        pPassANRChromatix->cnr.chroma_filter_base_far_modifier_y  =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->cnr.chroma_filter_base_far_modifier_y);
        pPassANRChromatix->cnr.chroma_filter_base_far_modifier_uv =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->cnr.chroma_filter_base_far_modifier_uv);
        pPassANRChromatix->cnr.face_detection_boundary            = pPassReserveData->cnr.face_detection_boundary;
        pPassANRChromatix->cnr.face_detection_transition          = pPassReserveData->cnr.face_detection_transition;

        for (index = 0; index < 5; index++)
        {
            // CNR
            pPassANRChromatix->cnr.detect_angle_start[index]                      =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.detect_angle_start_tab.detect_angle_start[index]);
            pPassANRChromatix->cnr.detect_angle_end[index]                        =
                IQSettingUtils::RoundFLOAT(
                    pPassUnpackedData->cnr.detect_angle_end_tab.detect_angle_end[index]);
            pPassANRChromatix->cnr.detect_chromaticity_calc_mode[index]           =
                pPassReserveData->cnr.detect_chromaticity_calc_mode_tab.detect_chromaticity_calc_mode[index];
            pPassANRChromatix->cnr.detect_chromaticity_start[index]               =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.detect_chromaticity_start_tab.detect_chromaticity_start[index]);
            pPassANRChromatix->cnr.detect_chromaticity_end[index]                 =
                IQSettingUtils::RoundFLOAT(
                    pPassUnpackedData->cnr.detect_chromaticity_end_tab.detect_chromaticity_end[index]);
            pPassANRChromatix->cnr.detect_luma_start[index]                       =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.detect_luma_start_tab.detect_luma_start[index]);
            pPassANRChromatix->cnr.detect_luma_end[index]                         =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.detect_luma_end_tab.detect_luma_end[index]);
            pPassANRChromatix->cnr.boundary_weight[index]                         =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.boundary_weight_tab.boundary_weight[index]);
            pPassANRChromatix->cnr.transition_ratio[index]                        =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.transition_ratio_tab.transition_ratio[index]);
            pPassANRChromatix->cnr.luma_filter_threshold_scale_y[index]           =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.luma_filter_threshold_scale_y_tab.luma_filter_threshold_scale_y[index]);
            pPassANRChromatix->cnr.luma_filter_threshold_scale_uv[index]          =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.luma_filter_threshold_scale_uv_tab.luma_filter_threshold_scale_uv[index]);
            pPassANRChromatix->cnr.luma_filter_offset_y[index]                    =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.luma_filter_offset_y_tab.luma_filter_offset_y[index]);
            pPassANRChromatix->cnr.luma_filter_offset_u[index]                    =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.luma_filter_offset_u_tab.luma_filter_offset_u[index]);
            pPassANRChromatix->cnr.luma_filter_offset_v[index]                    =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.luma_filter_offset_v_tab.luma_filter_offset_v[index]);
            pPassANRChromatix->cnr.chroma_filter_threshold_scale_y[index]         =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.chroma_filter_threshold_scale_y_tab.chroma_filter_threshold_scale_y[index]);
            pPassANRChromatix->cnr.chroma_filter_threshold_scale_uv[index]        =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.chroma_filter_threshold_scale_uv_tab.chroma_filter_threshold_scale_uv[index]);
            pPassANRChromatix->cnr.chroma_filter_offset_y[index]                  =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.chroma_filter_offset_y_tab.chroma_filter_offset_y[index]);
            pPassANRChromatix->cnr.chroma_filter_offset_u[index]                  =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.chroma_filter_offset_u_tab.chroma_filter_offset_u[index]);
            pPassANRChromatix->cnr.chroma_filter_offset_v[index]                  =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.chroma_filter_offset_v_tab.chroma_filter_offset_v[index]);
            pPassANRChromatix->cnr.face_detection_dependency[index]               =
                pPassReserveData->cnr.face_detection_dependency_tab.face_detection_dependency[index];
            pPassANRChromatix->cnr.luma_dcblend2_weight_scale[index]              =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.luma_dcblend2_weight_scale_tab.luma_dcblend2_weight_scale[index]);

            pPassANRChromatix->cnr.chroma_dcblend2_weight_restricted_scale[index] =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.chroma_dcblend2_weight_restricted_scale_tab.
                chroma_dcblend2_weight_restricted_scale[index]);
            pPassANRChromatix->cnr.luma_flat_kernel_blend_weight_scale[index]     =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->cnr.luma_flat_kernel_blend_weight_scale_tab.luma_flat_kernel_blend_weight_scale[index]);
        }


        // Luma filter detection
        pPassANRChromatix->luma_filter_detection_thresholds.control_per_uv_limit                  =
            pPassReserveData->luma_filter_detection_thresholds.control_per_uv_limit;
        pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_top_limit                 =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_top_limit);
        pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_bottom_limit              =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_bottom_limit);

        pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_close3_mod_scale          =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_close3_mod_scale);
        pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_close3_mod_offset         =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_close3_mod_offset);
        pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_der2_close3_mod_scale     =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_der2_close3_mod_scale);
        pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_der2_close3_mod_offset    =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_der2_close3_mod_offset);
        pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_far_mod_scale             =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_far_mod_scale);
        pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_far_mod_offset            =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_far_mod_offset);
        pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_close_external_mod_scale  =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_close_external_mod_scale);
        pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_close_external_mod_offset =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_close_external_mod_offset);

        pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_far_external_mod_scale    =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_far_external_mod_scale);
        pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_far_external_mod_offset   =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_far_external_mod_offset);
        pPassANRChromatix->luma_filter_detection_thresholds.u_threshold_top_limit                 =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.u_threshold_top_limit);
        pPassANRChromatix->luma_filter_detection_thresholds.u_threshold_bottom_limit              =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.u_threshold_bottom_limit);
        pPassANRChromatix->luma_filter_detection_thresholds.u_threshold_far_mod_scale             =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.u_threshold_far_mod_scale);
        pPassANRChromatix->luma_filter_detection_thresholds.u_threshold_far_mod_offset            =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.u_threshold_far_mod_offset);
        pPassANRChromatix->luma_filter_detection_thresholds.u_threshold_far_external_mod_scale    =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.u_threshold_far_external_mod_scale);
        pPassANRChromatix->luma_filter_detection_thresholds.u_threshold_far_external_mod_offset   =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.u_threshold_far_external_mod_offset);
        pPassANRChromatix->luma_filter_detection_thresholds.v_threshold_top_limit                 =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.v_threshold_top_limit);
        pPassANRChromatix->luma_filter_detection_thresholds.v_threshold_bottom_limit              =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.v_threshold_bottom_limit);
        pPassANRChromatix->luma_filter_detection_thresholds.v_threshold_far_mod_scale             =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.v_threshold_far_mod_scale);
        pPassANRChromatix->luma_filter_detection_thresholds.v_threshold_far_mod_offset            =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.v_threshold_far_mod_offset);
        pPassANRChromatix->luma_filter_detection_thresholds.v_threshold_far_external_mod_scale    =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.v_threshold_far_external_mod_scale);
        pPassANRChromatix->luma_filter_detection_thresholds.v_threshold_far_external_mod_offset   =
            IQSettingUtils::RoundFLOAT(
            pPassUnpackedData->luma_filter_detection_thresholds.v_threshold_far_external_mod_offset);

        // Luma filter detection thresholds
        for (index = 0; index < 17; index++)
        {
            pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_per_y[index] =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_per_y_tab.y_threshold_per_y[index]);
            pPassANRChromatix->luma_filter_detection_thresholds.u_threshold_per_y[index] =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->luma_filter_detection_thresholds.u_threshold_per_y_tab.u_threshold_per_y[index]);
            pPassANRChromatix->luma_filter_detection_thresholds.v_threshold_per_y[index] =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->luma_filter_detection_thresholds.v_threshold_per_y_tab.v_threshold_per_y[index]);
        }

        // Luma filter detection thresholds
        for (index = 0; index < 8; index++)
        {
            pPassANRChromatix->luma_filter_detection_thresholds.y_threshold_per_uv[index] =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->luma_filter_detection_thresholds.y_threshold_per_uv_tab.y_threshold_per_uv[index]);
            pPassANRChromatix->luma_filter_detection_thresholds.u_threshold_per_uv[index] =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->luma_filter_detection_thresholds.u_threshold_per_uv_tab.u_threshold_per_uv[index]);
            pPassANRChromatix->luma_filter_detection_thresholds.v_threshold_per_uv[index] =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->luma_filter_detection_thresholds.v_threshold_per_uv_tab.v_threshold_per_uv[index]);
        }

        // Chroma filter detection thresholds
        pPassANRChromatix->chroma_filter_detection_thresholds.control_per_uv_limit =
            pPassReserveData->chroma_filter_detection_thresholds.control_per_uv_limit;

        pPassANRChromatix->chroma_filter_detection_thresholds.y_threshold_top_limit          =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.y_threshold_top_limit);
        pPassANRChromatix->chroma_filter_detection_thresholds.y_threshold_bottom_limit       =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.y_threshold_bottom_limit);
        pPassANRChromatix->chroma_filter_detection_thresholds.y_threshold_far_mod_scale      =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.y_threshold_far_mod_scale);
        pPassANRChromatix->chroma_filter_detection_thresholds.y_threshold_far_mod_offset     =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.y_threshold_far_mod_offset);
        pPassANRChromatix->chroma_filter_detection_thresholds.u_threshold_top_limit          =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.u_threshold_top_limit);
        pPassANRChromatix->chroma_filter_detection_thresholds.u_threshold_bottom_limit       =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.u_threshold_bottom_limit);
        pPassANRChromatix->chroma_filter_detection_thresholds.u_threshold_far_mod_scale      =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.u_threshold_far_mod_scale);
        pPassANRChromatix->chroma_filter_detection_thresholds.u_threshold_far_mod_offset     =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.u_threshold_far_mod_offset);
        pPassANRChromatix->chroma_filter_detection_thresholds.u_threshold_distant_mod_scale  =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.u_threshold_distant_mod_scale);
        pPassANRChromatix->chroma_filter_detection_thresholds.u_threshold_distant_mod_offset =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.u_threshold_distant_mod_offset);
        pPassANRChromatix->chroma_filter_detection_thresholds.v_threshold_top_limit          =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.v_threshold_top_limit);
        pPassANRChromatix->chroma_filter_detection_thresholds.v_threshold_bottom_limit       =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.v_threshold_bottom_limit);
        pPassANRChromatix->chroma_filter_detection_thresholds.v_threshold_far_mod_scale      =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.v_threshold_far_mod_scale);
        pPassANRChromatix->chroma_filter_detection_thresholds.v_threshold_far_mod_offset     =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.v_threshold_far_mod_offset);
        pPassANRChromatix->chroma_filter_detection_thresholds.v_threshold_distant_mod_scale  =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.v_threshold_distant_mod_scale);
        pPassANRChromatix->chroma_filter_detection_thresholds.v_threshold_distant_mod_offset =
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->chroma_filter_detection_thresholds.v_threshold_distant_mod_offset);

        // Chroma filter detection thresholds
        for (index = 0; index < 17; index++)
        {
            pPassANRChromatix->chroma_filter_detection_thresholds.y_threshold_per_y[index] =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->chroma_filter_detection_thresholds.y_threshold_per_y_tab.y_threshold_per_y[index]);
            pPassANRChromatix->chroma_filter_detection_thresholds.u_threshold_per_y[index] =
                IQSettingUtils::RoundFLOAT(
                    pPassUnpackedData->chroma_filter_detection_thresholds.u_threshold_per_y_tab.u_threshold_per_y[index]);
            pPassANRChromatix->chroma_filter_detection_thresholds.v_threshold_per_y[index] =
                IQSettingUtils::RoundFLOAT(
                    pPassUnpackedData->chroma_filter_detection_thresholds.v_threshold_per_y_tab.v_threshold_per_y[index]);
        }

        // Chroma filter detection thresholds
        for (index = 0; index < 8; index++)
        {
            pPassANRChromatix->chroma_filter_detection_thresholds.y_threshold_per_uv[index] =
                IQSettingUtils::RoundFLOAT(
                    pPassUnpackedData->chroma_filter_detection_thresholds.y_threshold_per_uv_tab.y_threshold_per_uv[index]);
            pPassANRChromatix->chroma_filter_detection_thresholds.u_threshold_per_uv[index] =
                IQSettingUtils::RoundFLOAT(
                    pPassUnpackedData->chroma_filter_detection_thresholds.u_threshold_per_uv_tab.u_threshold_per_uv[index]);
            pPassANRChromatix->chroma_filter_detection_thresholds.v_threshold_per_uv[index] =
                IQSettingUtils::RoundFLOAT(
                    pPassUnpackedData->chroma_filter_detection_thresholds.v_threshold_per_uv_tab.v_threshold_per_uv[index]);
        }

        // DC Blend
        pPassANRChromatix->dcblend1.enable_dcblend1_chroma =
            pPassReserveData->dcblend1.enable_dcblend1_chroma;

        pPassANRChromatix->dcblend2.enable_dcblend2_luma   =
            pPassReserveData->dcblend2.enable_dcblend2_luma;
        pPassANRChromatix->dcblend2.enable_dcblend2_chroma =
            pPassReserveData->dcblend2.enable_dcblend2_chroma;

        // DC Blend
        for (index = 0; index < 5; index++)
        {

            pPassANRChromatix->dcblend2.dcblend2_luma_strength_function[index] =
                IQSettingUtils::RoundFLOAT(
                 pPassUnpackedData->dcblend2.dcblend2_luma_strength_function_tab.dcblend2_luma_strength_function[index]);
        }

        // DC Blend
        for (index = 0; index < 6; index++)
        {
            pPassANRChromatix->dcblend2.dcblend2_chroma_strength_function[index] =
                IQSettingUtils::RoundFLOAT(
                pPassUnpackedData->dcblend2.dcblend2_chroma_strength_function_tab.dcblend2_chroma_strength_function[index]);
        }
        // Dithering
        pPassANRChromatix->dithering.dithering_y_en =
            pPassReserveData->dithering.dithering_y_en;
        pPassANRChromatix->dithering.dithering_c_en =
            pPassReserveData->dithering.dithering_c_en;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ANR10Setting::UpdateGeometryParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ANR10Setting::UpdateGeometryParameters(
    const ANR10InputData*  pInput,
    NCLIB_CONTEXT_ANR*     pNclibContext)
{

    NcLibWarpGeomOut*  pWarpOut = static_cast<NcLibWarpGeomOut*>(pInput->pWarpGeometriesOutput);
    FD_CONFIG_CONTEXT* pFDConfig = NULL;

    pFDConfig = (NULL != pWarpOut) ? pWarpOut->fdConfig :
        reinterpret_cast<FD_CONFIG_CONTEXT*>(pInput->pFDData);
    pNclibContext->optical_center_x = (NULL != pWarpOut) ? pWarpOut->anr_lnr_opt_center_x : -1;
    pNclibContext->optical_center_y = (NULL != pWarpOut) ? pWarpOut->anr_lnr_opt_center_y : -1;
    if (NULL != pFDConfig)
    {
        pNclibContext->fdConfig.faceNum = pFDConfig->faceNum;
        CAMX_ASSERT(pFDConfig->faceNum <= MAX_FACE_NUM);
        for (UINT i = 0; i < pNclibContext->fdConfig.faceNum; i++)
        {
            pNclibContext->fdConfig.faceCenterX[i] = pFDConfig->faceCenterX[i];
            pNclibContext->fdConfig.faceCenterY[i] = pFDConfig->faceCenterY[i];
            pNclibContext->fdConfig.faceRadius[i]  = pFDConfig->faceRadius[i];
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ANR10Setting::GetInitializationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ANR10Setting::GetInitializationData(
    struct ANRNcLibOutputData* pData)
{
    pData->ANR10ChromatixSize  = (sizeof(ANR_Chromatix) * PASS_TYPE_NUM);

    return TRUE;
}
