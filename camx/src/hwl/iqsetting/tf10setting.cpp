// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  tf10setting.cpp
/// @brief tf10 IQ setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "NcLibWarp.h"
#include "tf10setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TF10Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TF10Setting::CalculateHWSetting(
    const TF10InputData*                                pInput,
    tf_1_0_0::mod_tf10_cct_dataType::cct_dataStruct*    pData,
    tf_1_0_0::chromatix_tf10_reserveType*               pReserveData,
    tf_1_0_0::chromatix_tf10Type::enable_sectionStruct* pModuleEnable,
    VOID*                                               pOutput)
{
    BOOL result = TRUE;
    CAMX_UNREFERENCED_PARAM(pOutput);

    if ((NULL != pInput) && (NULL != pData))
    {
        TF_Chromatix*         pTFChromatix    = static_cast<TF_Chromatix*>(pInput->pNCChromatix);
        tf_1_0_0::tf10_rgn_dataType*  pPassUnpackedData;

        CAMX_ASSERT(pInput->maxUsedPasses <= PASS_NAME_MAX);

        IQSettingUtils::Memset(pTFChromatix, 0, sizeof(TF_Chromatix)*PASS_NAME_MAX);

        for (UINT32 passType = 0; passType < pInput->maxUsedPasses; passType++)
        {
            if (passType == PASS_TYPE_FULL)
            {
                pPassUnpackedData = &pData->mod_tf10_pass_data[passType].tf10_rgn_data;
                if (pInput->bypassMode == TRUE)
                {
                    pPassUnpackedData->anr_final_blender_luma_min_strength_high_freq   = 0.0;
                    pPassUnpackedData->anr_final_blender_luma_min_strength_low_freq    = 0.0;
                    pPassUnpackedData->anr_final_blender_chroma_min_strength_high_freq = 0.0;
                    pPassUnpackedData->anr_final_blender_chroma_min_strength_low_freq  = 0.0;
                    CAMX_LOG_VERBOSE(CamxLogGroupPProc, " BYPASS Mode: %d, assign 0.0", pInput->bypassMode);
                }
                CAMX_LOG_VERBOSE(CamxLogGroupPProc, "bypass:%d luma: %f %f chroma: %f %f",
                    pInput->bypassMode,
                    pPassUnpackedData->anr_final_blender_luma_min_strength_high_freq,
                    pPassUnpackedData->anr_final_blender_luma_min_strength_low_freq,
                    pPassUnpackedData->anr_final_blender_chroma_min_strength_high_freq,
                    pPassUnpackedData->anr_final_blender_chroma_min_strength_low_freq);
            }

            pTFChromatix[passType].master_en = pModuleEnable->master_en;
            // This change is only there until we get separate chromatix for Preview and Snapshot
            if (pInput->useCase == CONFIG_VIDEO)
            {
                pReserveData->mod_tf10_pass_reserve_data[passType].pass_data.is_anr_strength_blender_mode = 0;
            }
        }
        result = MapChromatix2TFChromatix(pReserveData->mod_tf10_pass_reserve_data,
                                 pData,
                                 pInput,
                                 pInput->maxUsedPasses,
                                 pTFChromatix);

    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        CAMX_LOG_ERROR(CamxLogGroupPProc, "pInput=%p, pData=%p", pInput, pData);
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TF10Setting::GetTFPerPassRegionData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tf_1_0_0::tf10_rgn_dataType* TF10Setting::GetTFPerPassRegionData(
    tf_1_0_0::mod_tf10_cct_dataType::cct_dataStruct* pUnpackedData,
    const TF10InputData*                             pInput,
    UINT32                                           passType)
{
    tf_1_0_0::tf10_rgn_dataType*  pPassUnpackedData = NULL;

    if (pInput->maxUsedPasses < pInput->maxSupportedPassesForUsecase)
    {
        pPassUnpackedData = (0 == passType) ? &pUnpackedData->mod_tf10_pass_data[passType].tf10_rgn_data :
            &pUnpackedData->mod_tf10_pass_data[
                (pInput->maxSupportedPassesForUsecase - pInput->maxUsedPasses + passType)].tf10_rgn_data;
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "maxSupportedPasses %d,maxPasses %d,nclibPassindex %d,chromatixPassIndex %d",
            pInput->maxSupportedPassesForUsecase, pInput->maxUsedPasses, passType,
            (0 == passType) ? passType : (pInput->maxSupportedPassesForUsecase - pInput->maxUsedPasses + passType));
    }
    else
    {
        pPassUnpackedData = &pUnpackedData->mod_tf10_pass_data[passType].tf10_rgn_data;
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "maxSupportedPassess %d,maxPasses %d,chromatixPassindex-nclibPassIndex %d",
            pInput->maxSupportedPassesForUsecase, pInput->maxUsedPasses, passType);
    }
    return pPassUnpackedData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TF10Setting::GetTFPerPassReservedData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tf_1_0_0::mod_tf10_pass_reserve_dataType::pass_dataStruct* TF10Setting::GetTFPerPassReservedData(
    tf_1_0_0::mod_tf10_pass_reserve_dataType*        pReserveData,
    const TF10InputData*                             pInput,
    UINT32                                           passType)
{
    tf_1_0_0::mod_tf10_pass_reserve_dataType::pass_dataStruct* pPassReserveData = NULL;

    if (pInput->maxUsedPasses < pInput->maxSupportedPassesForUsecase)
    {
        pPassReserveData = (0 == passType) ?
            &pReserveData[passType].pass_data :
            &pReserveData[(pInput->maxSupportedPassesForUsecase - pInput->maxUsedPasses + passType)].pass_data;
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "maxSupportedPasses %d,maxPasses %d,nclibPassindex %d,chromatixPassIndex %d",
            pInput->maxSupportedPassesForUsecase, pInput->maxUsedPasses, passType,
            (0 == passType) ? passType : (pInput->maxSupportedPassesForUsecase - pInput->maxUsedPasses + passType));
    }
    else
    {
        pPassReserveData = &pReserveData[passType].pass_data;
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "maxSupportedPassess %d,maxPasses %d,chromatixPassindex-nclibPassIndex %d",
            pInput->maxSupportedPassesForUsecase, pInput->maxUsedPasses, passType);
    }
    return pPassReserveData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TF10Setting::MapChromatix2TFChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TF10Setting::MapChromatix2TFChromatix(
    tf_1_0_0::mod_tf10_pass_reserve_dataType*        pReserveData,
    tf_1_0_0::mod_tf10_cct_dataType::cct_dataStruct* pUnpackedData,
    const TF10InputData*                             pInput,
    UINT32                                           passNumMax,
    TF_Chromatix*                                    pTFChromatix)
{
    TF_Chromatix*                                              pPassTFChromatix;
    tf_1_0_0::tf10_rgn_dataType*                               pPassUnpackedData;
    tf_1_0_0::mod_tf10_pass_reserve_dataType::pass_dataStruct* pPassReserveData;
    BOOL                                                       result = TRUE;

    UINT32    idx;
    UINT32    passType;

    for (passType = 0; passType < passNumMax; passType++)
    {
        pPassTFChromatix  = &pTFChromatix[passType];
        pPassUnpackedData = GetTFPerPassRegionData(pUnpackedData, pInput, passType);
        pPassReserveData = GetTFPerPassReservedData(pReserveData, pInput, passType);

        if ((NULL == pPassReserveData) || (NULL == pPassUnpackedData))
        {
            result = FALSE;
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid pPassReserveData %p or  pPassUnpackedData %p for passType %d",
                pPassReserveData, pPassUnpackedData, passType);
            break;
        }


        pPassTFChromatix->en                                      = static_cast<UINT>(pPassUnpackedData->en);
        pPassTFChromatix->scene_cut_recovery_time                 =
            static_cast<PARAM_UINT>(pPassUnpackedData->scene_cut_recovery_time);
        pPassTFChromatix->video_first_frame_spatial_nr_percentage =
            static_cast<PARAM_UINT>(pPassUnpackedData->video_first_frame_spatial_nr_percentage);
        pPassTFChromatix->smear_inputs_for_decisions              =
             pPassReserveData->smear_inputs_for_decisions;
        pPassTFChromatix->enable_noise_est_by_luma                = pPassReserveData->enable_noise_est_by_luma;
        pPassTFChromatix->enable_noise_est_by_chroma              = pPassReserveData->enable_noise_est_by_chroma;
        pPassTFChromatix->indications_premultiply_factor          = pPassReserveData->indications_premultiply_factor;
        pPassTFChromatix->invert_temporal_blending_weights        = pPassReserveData->invert_temporal_blending_weights;
        pPassTFChromatix->blending_mode                           = pPassReserveData->blending_mode;
        pPassTFChromatix->constant_blending_factor_y              =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->constant_blending_factor_y));
        pPassTFChromatix->constant_blending_factor_c              =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->constant_blending_factor_c));
        pPassTFChromatix->enable_lnr                              = pPassReserveData->enable_lnr;
        pPassTFChromatix->use_indications                         =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->use_indications));
        pPassTFChromatix->disable_luma_ghost_detection            = pPassReserveData->disable_luma_ghost_detection;
        pPassTFChromatix->disable_chroma_ghost_detection          = pPassReserveData->disable_chroma_ghost_detection;
        pPassTFChromatix->use_anr_for_decisions_y                 = pPassReserveData->use_anr_for_decisions_y;
        pPassTFChromatix->use_anr_for_decisions_c                 = pPassReserveData->use_anr_for_decisions_c;
        pPassTFChromatix->padding_by_reflection                   = pPassReserveData->padding_by_reflection;
        pPassTFChromatix->sad_y_calc_mode                         =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->sad_y_calc_mode));
        pPassTFChromatix->sad_c_calc_mode                         =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->sad_c_calc_mode));
        pPassTFChromatix->is_dci_mode                             =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->is_dci_mode));
        pPassTFChromatix->indications_affect_fs_decision_also_directly = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->indications_affect_fs_decision_also_directly));
        pPassTFChromatix->is_same_blending_for_all_frequencies    =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->is_same_blending_for_all_frequencies));
        pPassTFChromatix->out_of_frame_pixels_confidence          =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->out_of_frame_pixels_confidence));
        pPassTFChromatix->apply_fs_rank_filter                    =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->apply_fs_rank_filter));
        pPassTFChromatix->apply_fs_lpf                            =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->apply_fs_lpf));
        pPassTFChromatix->dither_y                                =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->dither_y));
        pPassTFChromatix->dither_cb                               =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->dither_cb));
        pPassTFChromatix->dither_cr                               =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->dither_cr));
        pPassTFChromatix->morph_erode_en                          = pPassReserveData->morph_erode_en;
        pPassTFChromatix->morph_erode_size                        =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->morph_erode_size));
        pPassTFChromatix->morph_dilate_en                         = pPassReserveData->morph_dilate_en;
        pPassTFChromatix->morph_dilate_size                       =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->morph_dilate_size));
        pPassTFChromatix->tr_enable                               =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->tr_enable));
        pPassTFChromatix->tr_fs_threshold                         =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->tr_fs_threshold));
        pPassTFChromatix->tr_dead_zone                            =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->tr_dead_zone));
        pPassTFChromatix->tr_block_num_x                          =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->tr_block_num_x));
        pPassTFChromatix->tr_block_num_y                          =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->tr_block_num_y));
        pPassTFChromatix->lnr_opt_center_x                        =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr_opt_center_x));
        pPassTFChromatix->lnr_opt_center_y                        =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr_opt_center_y));
        pPassTFChromatix->lnr_ellipses_bounding_rect_w            =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr_ellipses_bounding_rect_w));
        pPassTFChromatix->lnr_ellipses_bounding_rect_h            =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr_ellipses_bounding_rect_h));
        for (idx = 0; idx < 17; idx++)
        {
            pPassTFChromatix->noise_params_y_ytb[idx]  = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->noise_params_y_ytb_tab.noise_params_y_ytb[idx]));
            pPassTFChromatix->noise_params_cb_ytb[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->noise_params_cb_ytb_tab.noise_params_cb_ytb[idx]));
            pPassTFChromatix->noise_params_cr_ytb[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->noise_params_cr_ytb_tab.noise_params_cr_ytb[idx]));
        }

        for (idx = 0; idx < 8; idx++)
        {
            pPassTFChromatix->noise_params_y_ctb[idx]  = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->noise_params_y_ctb_tab.noise_params_y_ctb[idx]));
            pPassTFChromatix->noise_params_cb_ctb[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->noise_params_cb_ctb_tab.noise_params_cb_ctb[idx]));
            pPassTFChromatix->noise_params_cr_ctb[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->noise_params_cr_ctb_tab.noise_params_cr_ctb[idx]));
        }

        pPassTFChromatix->noise_params_y_uv_limit  = pPassReserveData->noise_params_y_uv_limit;
        pPassTFChromatix->noise_params_cb_uv_limit = pPassReserveData->noise_params_cb_uv_limit;
        pPassTFChromatix->noise_params_cr_uv_limit = pPassReserveData->noise_params_cr_uv_limit;
        pPassTFChromatix->noise_params_y_top_lim   =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->noise_params_y_top_lim));
        pPassTFChromatix->noise_params_cb_top_lim  =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->noise_params_cb_top_lim));
        pPassTFChromatix->noise_params_cr_top_lim  =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->noise_params_cr_top_lim));
        pPassTFChromatix->noise_params_y_bot_lim   =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->noise_params_y_bot_lim));
        pPassTFChromatix->noise_params_cb_bot_lim  =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->noise_params_cb_bot_lim));
        pPassTFChromatix->noise_params_cr_bot_lim  =
            static_cast<PARAM_UINT>(IQSettingUtils::RoundFLOAT(pPassUnpackedData->noise_params_cr_bot_lim));

        for (idx = 0; idx < 9; idx++)
        {
            pPassTFChromatix->fs_decision_params_y_c1[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_y_c1_tab.fs_decision_params_y_c1[idx]));
            pPassTFChromatix->fs_decision_params_y_c2[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_y_c2_tab.fs_decision_params_y_c2[idx]));
            pPassTFChromatix->fs_decision_params_y_c3[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_y_c3_tab.fs_decision_params_y_c3[idx]));
            pPassTFChromatix->fs_decision_params_y_c4[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_y_c4_tab.fs_decision_params_y_c4[idx]));
            pPassTFChromatix->fs_decision_params_c_c1[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_c_c1_tab.fs_decision_params_c_c1[idx]));
            pPassTFChromatix->fs_decision_params_c_c2[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_c_c2_tab.fs_decision_params_c_c2[idx]));
            pPassTFChromatix->fs_decision_params_c_c3[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_c_c3_tab.fs_decision_params_c_c3[idx]));
            pPassTFChromatix->fs_decision_params_c_c4[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_c_c4_tab.fs_decision_params_c_c4[idx]));
            pPassTFChromatix->fs_to_a1_map_y[idx]          = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_to_a1_map_y_tab.fs_to_a1_map_y[idx]));
            pPassTFChromatix->fs_to_a1_map_c[idx]          = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_to_a1_map_c_tab.fs_to_a1_map_c[idx]));
            pPassTFChromatix->fs_to_a4_map_y[idx]          = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_to_a4_map_y_tab.fs_to_a4_map_y[idx]));
            pPassTFChromatix->fs_to_a4_map_c[idx]          = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_to_a4_map_c_tab.fs_to_a4_map_c[idx]));
        }

        pPassTFChromatix->fs_decision_params_oof_y_c1 = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_oof_y_c1));
        pPassTFChromatix->fs_decision_params_oof_y_c2 = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_oof_y_c2));
        pPassTFChromatix->fs_decision_params_oof_y_c3 = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_oof_y_c3));
        pPassTFChromatix->fs_decision_params_oof_y_c4 = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_oof_y_c4));
        pPassTFChromatix->fs_decision_params_oof_c_c1 = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_oof_c_c1));
        pPassTFChromatix->fs_decision_params_oof_c_c2 = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_oof_c_c2));
        pPassTFChromatix->fs_decision_params_oof_c_c3 = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_oof_c_c3));
        pPassTFChromatix->fs_decision_params_oof_c_c4 = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->fs_decision_params_oof_c_c4));
        pPassTFChromatix->a3_t1_scale_y               = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a3_t1_scale_y));
        pPassTFChromatix->a3_t1_scale_c               = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a3_t1_scale_c));
        pPassTFChromatix->a3_t2_scale_y               = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a3_t2_scale_y));
        pPassTFChromatix->a3_t2_scale_c               = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a3_t2_scale_c));
        pPassTFChromatix->a3_t1_offs_y                = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a3_t1_offs_y));
        pPassTFChromatix->a3_t1_offs_c                = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a3_t1_offs_c));
        pPassTFChromatix->a3_t2_offs_y                = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a3_t2_offs_y));
        pPassTFChromatix->a3_t2_offs_c                = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a3_t2_offs_c));

        pPassTFChromatix->a2_min_y                    = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a2_min_y));
        pPassTFChromatix->a2_max_y                    = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a2_max_y));
        pPassTFChromatix->a2_min_c                    = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a2_min_c));
        pPassTFChromatix->a2_max_c                    = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a2_max_c));
        pPassTFChromatix->a2_slope_y                  = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a2_slope_y));
        pPassTFChromatix->a2_slope_c                  = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->a2_slope_c));
        for (idx = 0; idx < 16; idx++)
        {
            pPassTFChromatix->lnr_lut_y[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr_lut_y_tab.lnr_lut_y[idx]));
            pPassTFChromatix->lnr_lut_c[idx] = static_cast<PARAM_UINT>(
                IQSettingUtils::RoundFLOAT(pPassUnpackedData->lnr_lut_c_tab.lnr_lut_c[idx]));
        }
        pPassTFChromatix->padding_by_reflection_override = pPassReserveData->padding_by_reflection_override;
        pPassTFChromatix->is_anr_strength_blender_mode   = pPassReserveData->is_anr_strength_blender_mode;

        // ANR final blender parameters
        pPassTFChromatix->anr_final_blender_luma_min_strength_high_freq   = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->anr_final_blender_luma_min_strength_high_freq));
        pPassTFChromatix->anr_final_blender_luma_min_strength_low_freq    = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->anr_final_blender_luma_min_strength_low_freq));
        pPassTFChromatix->anr_final_blender_chroma_min_strength_high_freq = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->anr_final_blender_chroma_min_strength_high_freq));
        pPassTFChromatix->anr_final_blender_chroma_min_strength_low_freq  = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->anr_final_blender_chroma_min_strength_low_freq));
        pPassTFChromatix->anr_final_blender_strength_decision_ythr_low    = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->anr_final_blender_strength_decision_ythr_low));
        pPassTFChromatix->anr_final_blender_strength_decision_ythr_high   = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->anr_final_blender_strength_decision_ythr_high));
        pPassTFChromatix->anr_final_blender_strength_decision_cthr_low    = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->anr_final_blender_strength_decision_cthr_low));
        pPassTFChromatix->anr_final_blender_strength_decision_cthr_high   = static_cast<PARAM_UINT>(
            IQSettingUtils::RoundFLOAT(pPassUnpackedData->anr_final_blender_strength_decision_cthr_high));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TF10Setting::CopyUINT32toINT32ArrayWithSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TF10Setting::CopyUINT32toINT32ArrayWithSize(
    INT32*        pDstArrayName,
    const UINT32* pSrcArrayName,
    INT           arraySize)
{
    for (INT lutIter = 0; lutIter < arraySize; lutIter++)
    {
        pDstArrayName[lutIter] = static_cast<INT32>(pSrcArrayName[lutIter]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TF10Setting::CopyINT32toINT32ArrayWithSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TF10Setting::CopyINT32toINT32ArrayWithSize(
    INT32*       pDstArrayName,
    const INT32* pSrcArrayName,
    INT          arraySize)
{
    for (INT lutIter = 0; lutIter < arraySize; lutIter++)
    {
        pDstArrayName[lutIter] = pSrcArrayName[lutIter];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TF10Setting::CopyUINT32toUINT8ArrayWithSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TF10Setting::CopyUINT32toUINT8ArrayWithSize(
    UINT8*        pDstArrayName,
    const UINT32* pSrcArrayName,
    INT           arraySize)
{
    for (INT lutIter = 0; lutIter < arraySize; lutIter++)
    {
        pDstArrayName[lutIter] = static_cast<UINT8>(pSrcArrayName[lutIter]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TF10Setting::GetInitializationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TF10Setting::GetInitializationData(
    struct TFNcLibOutputData* pData)
{
    pData->TFChromatixSize  = (sizeof(TF_Chromatix) * PASS_TYPE_NUM);

    return TRUE;
}
