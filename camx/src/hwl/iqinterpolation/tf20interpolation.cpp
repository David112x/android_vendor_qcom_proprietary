// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  tf20interpolation.cpp
/// @brief tf20 IQ setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "tf20interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation TF20OperationTable[] =
{
    { TF20Interpolation::LensPositionSearchNode,   2},
    { TF20Interpolation::LensZoomSearchNode,       2},
    { TF20Interpolation::PostScaleRatioSearchNode, 2},
    { TF20Interpolation::PreScaleRatioSearchNode,  2},
    { TF20Interpolation::DRCGainSearchNode,        2},
    { TF20Interpolation::HDRAECSearchNode,         2},
    { TF20Interpolation::AECSearchNode,            2},
    { TF20Interpolation::CCTSearchNode,            2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TF20Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TF20Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    TF20InputData*    pTriggerData)
{
    BOOL isChanged = FALSE;
    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECGain,  pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->CCTTrigger, pInput->AWBColorTemperature))         ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->lensPosition, pInput->lensPosition))              ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->lensZoom, pInput->lensZoom))                      ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->postScaleRatio, pInput->postScaleRatio))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->preScaleRatio, pInput->preScaleRatio))            ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain,  pInput->DRCGain)))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->AECGain           = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->CCTTrigger        = pInput->AWBColorTemperature;
        pTriggerData->lensPosition      = pInput->lensPosition;
        pTriggerData->lensZoom          = pInput->lensZoom;
        pTriggerData->postScaleRatio    = pInput->postScaleRatio;
        pTriggerData->preScaleRatio     = pInput->preScaleRatio;
        pTriggerData->DRCGain           = pInput->DRCGain;

        isChanged = TRUE;
    }
    CAMX_LOG_VERBOSE(CamxLogGroupPProc, " X isChanged %d", isChanged);

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TF20Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TF20Interpolation::RunInterpolation(
    const TF20InputData*                             pInput,
    tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct* pData)
{
    BOOL            result = TRUE;
    UINT            count  = 0;
    TuningNode      nodeSet[TF20MaxNode];           // The intepolation tree total Node
    TF20TriggerList TF20Trigger;                    // Color Correction Trigger List

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (count = 0; count < TF20MaxNode; count++)
        {
            if (count < TF20MaxNonLeafNode)
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], &pData[count + 1]);
            }
            else
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], NULL);
            }
        }

        // Setup the Top Node
        IQSettingUtils::AddNodeToInterpolationTree(NULL, &nodeSet[0],
            static_cast<VOID*>(&pInput->pChromatix->chromatix_tf20_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&TF20Trigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(tf_2_0_0::chromatix_tf20Type::control_methodStruct));

        TF20Trigger.triggerLensPosition   = pInput->lensPosition;
        TF20Trigger.triggerLensZoom       = pInput->lensZoom;
        TF20Trigger.triggerPostScaleRatio = pInput->postScaleRatio;
        TF20Trigger.triggerPreScaleRatio  = pInput->preScaleRatio;

        TF20Trigger.triggerDRCgain        = pInput->DRCGain;

        TF20Trigger.triggerHDRAEC         =
            IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                             pInput->exposureTime,
                                             pInput->AECSensitivity,
                                             pInput->exposureGainRatio);
        TF20Trigger.triggerAEC            =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->AECGain);

        TF20Trigger.triggerCCT            = pInput->CCTTrigger;

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        TF20InterpolationLevel,
                                                        &TF20OperationTable[0],
                                                        static_cast<VOID*>(&TF20Trigger));
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    if (FALSE != result)
    {
        // Calculate the Interpolation Result
        result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                       TF20MaxNonLeafNode,
                                                       TF20InterpolationLevel,
                                                       TF20Interpolation::DoInterpolation);
    }

    if (FALSE != result)
    {
        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TF20Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TF20Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct* pInput1 =
            static_cast<tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct*>(pData1);
        tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct* pInput2 =
            static_cast<tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct*>(pData2);
        tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct* pOutput =
            static_cast<tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = TF20Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct));
            }
            else
            {
                result = FALSE;
                /// @todo (CAMX-1812) Need to add logging for Common library
            }
        }
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TF20Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL TF20Interpolation::InterpolationData(
    tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct* pInput1,
    tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct* pInput2,
    FLOAT                                            ratio,
    tf_2_0_0::mod_tf20_cct_dataType::cct_dataStruct* pOutput)
{
    UINT count;
    UINT passType;
    BOOL result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        UINT input1Pass[MAX_NUM_PASSES];
        UINT input2Pass[MAX_NUM_PASSES];

        // find the order for the right PASS
        for (count = 0; count < MAX_NUM_PASSES; count++)
        {
            passType             = static_cast<UINT>(pInput1->mod_tf20_pass_data[count].pass_trigger);
            input1Pass[passType] = count;
            passType             = static_cast<UINT>(pInput2->mod_tf20_pass_data[count].pass_trigger);
            input2Pass[passType] = count;
        }

        for (passType = static_cast<UINT>(ispglobalelements::trigger_pass::PASS_FULL);
            passType <= static_cast<UINT>(ispglobalelements::trigger_pass::PASS_DC64);
            passType++)
        {
            UINT input1Count = input1Pass[passType];
            UINT input2Count = input2Pass[passType];
            UINT index;

            tf_2_0_0::tf20_rgn_dataType*  pInput1Rgn = &pInput1->mod_tf20_pass_data[input1Count].tf20_rgn_data;
            tf_2_0_0::tf20_rgn_dataType*  pInput2Rgn = &pInput2->mod_tf20_pass_data[input2Count].tf20_rgn_data;
            tf_2_0_0::tf20_rgn_dataType*  pOutputRgn = &pOutput->mod_tf20_pass_data[passType].tf20_rgn_data;

            pOutput->mod_tf20_pass_data[passType].pass_trigger = pInput1->mod_tf20_pass_data[input1Count].pass_trigger;

            pOutputRgn->en = pInput1Rgn->en;

            for (index = 0; index < NOISE_PARAM_YTB_SIZE; index++)
            {
                pOutputRgn->noise_params_y_ytb_tab.noise_params_y_ytb[index]   = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->noise_params_y_ytb_tab.noise_params_y_ytb[index],
                    pInput2Rgn->noise_params_y_ytb_tab.noise_params_y_ytb[index],
                    ratio);
                pOutputRgn->noise_params_cb_ytb_tab.noise_params_cb_ytb[index] = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->noise_params_cb_ytb_tab.noise_params_cb_ytb[index],
                    pInput2Rgn->noise_params_cb_ytb_tab.noise_params_cb_ytb[index],
                    ratio);
                pOutputRgn->noise_params_cr_ytb_tab.noise_params_cr_ytb[index] = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->noise_params_cr_ytb_tab.noise_params_cr_ytb[index],
                    pInput2Rgn->noise_params_cr_ytb_tab.noise_params_cr_ytb[index],
                    ratio);
            }

            for (index = 0; index < NOISE_PARAM_CTB_SIZE; index++)
            {
                pOutputRgn->noise_params_y_ctb_tab.noise_params_y_ctb[index]   = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->noise_params_y_ctb_tab.noise_params_y_ctb[index],
                    pInput2Rgn->noise_params_y_ctb_tab.noise_params_y_ctb[index],
                    ratio);
                pOutputRgn->noise_params_cb_ctb_tab.noise_params_cb_ctb[index] = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->noise_params_cb_ctb_tab.noise_params_cb_ctb[index],
                    pInput2Rgn->noise_params_cb_ctb_tab.noise_params_cb_ctb[index],
                    ratio);
                pOutputRgn->noise_params_cr_ctb_tab.noise_params_cr_ctb[index] = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->noise_params_cr_ctb_tab.noise_params_cr_ctb[index],
                    pInput2Rgn->noise_params_cr_ctb_tab.noise_params_cr_ctb[index],
                    ratio);
            }

            pOutputRgn->noise_params_y_top_lim = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->noise_params_y_top_lim,
                pInput2Rgn->noise_params_y_top_lim,
                ratio);
            pOutputRgn->noise_params_y_bot_lim = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->noise_params_y_bot_lim,
                pInput2Rgn->noise_params_y_bot_lim,
                ratio);
            pOutputRgn->noise_params_cb_top_lim = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->noise_params_cb_top_lim,
                pInput2Rgn->noise_params_cb_top_lim,
                ratio);
            pOutputRgn->noise_params_cb_bot_lim = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->noise_params_cb_bot_lim,
                pInput2Rgn->noise_params_cb_bot_lim,
                ratio);
            pOutputRgn->noise_params_cr_top_lim = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->noise_params_cr_top_lim,
                pInput2Rgn->noise_params_cr_top_lim,
                ratio);
            pOutputRgn->noise_params_cr_bot_lim = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->noise_params_cr_bot_lim,
                pInput2Rgn->noise_params_cr_bot_lim,
                ratio);
            pOutputRgn->a2_max_y                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a2_max_y,
                pInput2Rgn->a2_max_y,
                ratio);
            pOutputRgn->a2_min_y                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a2_min_y,
                pInput2Rgn->a2_min_y,
                ratio);
            pOutputRgn->a2_slope_y              = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a2_slope_y,
                pInput2Rgn->a2_slope_y,
                ratio);
            pOutputRgn->a2_max_c                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a2_max_c,
                pInput2Rgn->a2_max_c,
                ratio);
            pOutputRgn->a2_min_c                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a2_min_c,
                pInput2Rgn->a2_min_c,
                ratio);
            pOutputRgn->a2_slope_c              = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a2_slope_c,
                pInput2Rgn->a2_slope_c,
                ratio);

            pOutputRgn->dither_y                = ((ratio + 0.500f) >= 1.0f) ? pInput2Rgn->dither_y : pInput1Rgn->dither_y;
            pOutputRgn->dither_cb               = ((ratio + 0.500f) >= 1.0f) ? pInput2Rgn->dither_cb : pInput1Rgn->dither_cb;
            pOutputRgn->dither_cr               = ((ratio + 0.500f) >= 1.0f) ? pInput2Rgn->dither_cr : pInput1Rgn->dither_cr;

            for (index = 0; index < FS_TO_A1_A4_MAP_SIZE; index++)
            {
                pOutputRgn->fs_to_a1_map_y_tab.fs_to_a1_map_y[index] = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->fs_to_a1_map_y_tab.fs_to_a1_map_y[index],
                    pInput2Rgn->fs_to_a1_map_y_tab.fs_to_a1_map_y[index],
                    ratio);
                pOutputRgn->fs_to_a1_map_c_tab.fs_to_a1_map_c[index] = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->fs_to_a1_map_c_tab.fs_to_a1_map_c[index],
                    pInput2Rgn->fs_to_a1_map_c_tab.fs_to_a1_map_c[index],
                    ratio);
                pOutputRgn->fs_to_a4_map_y_tab.fs_to_a4_map_y[index] = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->fs_to_a4_map_y_tab.fs_to_a4_map_y[index],
                    pInput2Rgn->fs_to_a4_map_y_tab.fs_to_a4_map_y[index],
                    ratio);
                pOutputRgn->fs_to_a4_map_c_tab.fs_to_a4_map_c[index] = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->fs_to_a4_map_c_tab.fs_to_a4_map_c[index],
                    pInput2Rgn->fs_to_a4_map_c_tab.fs_to_a4_map_c[index],
                    ratio);
            }

            pOutputRgn->lnr_opt_center_x = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr_opt_center_x,
                pInput2Rgn->lnr_opt_center_x,
                ratio);
            pOutputRgn->lnr_opt_center_y = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr_opt_center_y,
                pInput2Rgn->lnr_opt_center_y,
                ratio);

            for (index = 0; index < LNR_LUT_SIZE; index++)
            {
                pOutputRgn->lnr_lut_y_tab.lnr_lut_y[index] = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->lnr_lut_y_tab.lnr_lut_y[index],
                    pInput2Rgn->lnr_lut_y_tab.lnr_lut_y[index],
                    ratio);
                pOutputRgn->lnr_lut_c_tab.lnr_lut_c[index] = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->lnr_lut_c_tab.lnr_lut_c[index],
                    pInput2Rgn->lnr_lut_c_tab.lnr_lut_c[index],
                    ratio);
            }

            pOutputRgn->lnr_ellipses_bounding_rect_w         = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr_ellipses_bounding_rect_w,
                pInput2Rgn->lnr_ellipses_bounding_rect_w,
                ratio);
            pOutputRgn->lnr_ellipses_bounding_rect_h         = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr_ellipses_bounding_rect_h,
                pInput2Rgn->lnr_ellipses_bounding_rect_h,
                ratio);
            pOutputRgn->is_same_blending_for_all_frequencies = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->is_same_blending_for_all_frequencies,
                pInput2Rgn->is_same_blending_for_all_frequencies,
                ratio);

            for (index = 0; index < FS_DECISION_PARAM_SIZE; index++)
            {
                pOutputRgn->fs_decision_params_y_c1_tab.fs_decision_params_y_c1[index] =
                    IQSettingUtils::InterpolationFloatBilinear(
                        pInput1Rgn->fs_decision_params_y_c1_tab.fs_decision_params_y_c1[index],
                        pInput2Rgn->fs_decision_params_y_c1_tab.fs_decision_params_y_c1[index],
                        ratio);
                pOutputRgn->fs_decision_params_y_c2_tab.fs_decision_params_y_c2[index] =
                    IQSettingUtils::InterpolationFloatBilinear(
                        pInput1Rgn->fs_decision_params_y_c2_tab.fs_decision_params_y_c2[index],
                        pInput2Rgn->fs_decision_params_y_c2_tab.fs_decision_params_y_c2[index],
                        ratio);
                pOutputRgn->fs_decision_params_y_c3_tab.fs_decision_params_y_c3[index] =
                    IQSettingUtils::InterpolationFloatBilinear(
                        pInput1Rgn->fs_decision_params_y_c3_tab.fs_decision_params_y_c3[index],
                        pInput2Rgn->fs_decision_params_y_c3_tab.fs_decision_params_y_c3[index],
                        ratio);
                pOutputRgn->fs_decision_params_y_c4_tab.fs_decision_params_y_c4[index] =
                    IQSettingUtils::InterpolationFloatBilinear(
                        pInput1Rgn->fs_decision_params_y_c4_tab.fs_decision_params_y_c4[index],
                        pInput2Rgn->fs_decision_params_y_c4_tab.fs_decision_params_y_c4[index],
                        ratio);
                pOutputRgn->fs_decision_params_c_c1_tab.fs_decision_params_c_c1[index] =
                    IQSettingUtils::InterpolationFloatBilinear(
                        pInput1Rgn->fs_decision_params_c_c1_tab.fs_decision_params_c_c1[index],
                        pInput2Rgn->fs_decision_params_c_c1_tab.fs_decision_params_c_c1[index],
                        ratio);
                pOutputRgn->fs_decision_params_c_c2_tab.fs_decision_params_c_c2[index] =
                    IQSettingUtils::InterpolationFloatBilinear(
                        pInput1Rgn->fs_decision_params_c_c2_tab.fs_decision_params_c_c2[index],
                        pInput2Rgn->fs_decision_params_c_c2_tab.fs_decision_params_c_c2[index],
                        ratio);
                pOutputRgn->fs_decision_params_c_c3_tab.fs_decision_params_c_c3[index] =
                    IQSettingUtils::InterpolationFloatBilinear(
                        pInput1Rgn->fs_decision_params_c_c3_tab.fs_decision_params_c_c3[index],
                        pInput2Rgn->fs_decision_params_c_c3_tab.fs_decision_params_c_c3[index],
                        ratio);
                pOutputRgn->fs_decision_params_c_c4_tab.fs_decision_params_c_c4[index] =
                    IQSettingUtils::InterpolationFloatBilinear(
                        pInput1Rgn->fs_decision_params_c_c4_tab.fs_decision_params_c_c4[index],
                        pInput2Rgn->fs_decision_params_c_c4_tab.fs_decision_params_c_c4[index],
                        ratio);
            }

            pOutputRgn->fs_decision_params_oof_y_c1   = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->fs_decision_params_oof_y_c1,
                pInput2Rgn->fs_decision_params_oof_y_c1,
                ratio);
            pOutputRgn->fs_decision_params_oof_y_c2   = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->fs_decision_params_oof_y_c2,
                pInput2Rgn->fs_decision_params_oof_y_c2,
                ratio);
            pOutputRgn->fs_decision_params_oof_y_c3   = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->fs_decision_params_oof_y_c3,
                pInput2Rgn->fs_decision_params_oof_y_c3,
                ratio);
            pOutputRgn->fs_decision_params_oof_y_c4   = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->fs_decision_params_oof_y_c4,
                pInput2Rgn->fs_decision_params_oof_y_c4,
                ratio);
            pOutputRgn->fs_decision_params_oof_c_c1   = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->fs_decision_params_oof_c_c1,
                pInput2Rgn->fs_decision_params_oof_c_c1,
                ratio);
            pOutputRgn->fs_decision_params_oof_c_c2   = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->fs_decision_params_oof_c_c2,
                pInput2Rgn->fs_decision_params_oof_c_c2,
                ratio);
            pOutputRgn->fs_decision_params_oof_c_c3   = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->fs_decision_params_oof_c_c3,
                pInput2Rgn->fs_decision_params_oof_c_c3,
                ratio);
            pOutputRgn->fs_decision_params_oof_c_c4   = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->fs_decision_params_oof_c_c4,
                pInput2Rgn->fs_decision_params_oof_c_c4,
                ratio);

            pOutputRgn->apply_fs_rank_filter          = ((ratio + 0.500f) >= 1.0f) ?
                pInput2Rgn->apply_fs_rank_filter : pInput1Rgn->apply_fs_rank_filter;

            pOutputRgn->apply_fs_lpf                  = ((ratio + 0.500f) >= 1.0f) ?
                pInput2Rgn->apply_fs_lpf : pInput1Rgn->apply_fs_lpf;

            pOutputRgn->sad_y_calc_mode               = ((ratio + 0.500f) >= 1.0f) ?
                pInput2Rgn->sad_y_calc_mode : pInput1Rgn->sad_y_calc_mode;

            pOutputRgn->sad_c_calc_mode               = ((ratio + 0.500f) >= 1.0f) ?
                pInput2Rgn->sad_c_calc_mode : pInput1Rgn->sad_c_calc_mode;

            pOutputRgn->use_indications               = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->use_indications,
                pInput2Rgn->use_indications,
                ratio);

            pOutputRgn->morph_erode_size              = ((ratio + 0.500f) >= 1.0f) ?
                pInput2Rgn->morph_erode_size : pInput1Rgn->morph_erode_size;

            pOutputRgn->morph_dilate_size             = ((ratio + 0.500f) >= 1.0f) ?
                pInput2Rgn->morph_dilate_size : pInput1Rgn->morph_dilate_size;

            pOutputRgn->tr_enable                     = ((ratio + 0.500f) >= 1.0f) ?
                pInput2Rgn->tr_enable : pInput1Rgn->tr_enable;

            pOutputRgn->tr_block_num_x                = ((ratio + 0.500f) >= 1.0f) ?
                pInput2Rgn->tr_block_num_x : pInput1Rgn->tr_block_num_x;

            pOutputRgn->tr_block_num_y                = ((ratio + 0.500f) >= 1.0f) ?
                pInput2Rgn->tr_block_num_y : pInput1Rgn->tr_block_num_y;

            pOutputRgn->tr_fs_threshold               = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->tr_fs_threshold,
                pInput2Rgn->tr_fs_threshold,
                ratio);
            pOutputRgn->tr_dead_zone                  = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->tr_dead_zone,
                pInput2Rgn->tr_dead_zone,
                ratio);

            pOutputRgn->tr_count_percentage_threshold = ((ratio + 0.500f) >= 1.0f) ?
                pInput2Rgn->tr_count_percentage_threshold : pInput1Rgn->tr_count_percentage_threshold;

            pOutputRgn->is_dci_mode                   = ((ratio + 0.500f) >= 1.0f) ?
                pInput2Rgn->is_dci_mode : pInput1Rgn->is_dci_mode;

            pOutputRgn->a3_t1_scale_y                 = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a3_t1_scale_y,
                pInput2Rgn->a3_t1_scale_y,
                ratio);
            pOutputRgn->a3_t1_offs_y                  = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a3_t1_offs_y,
                pInput2Rgn->a3_t1_offs_y,
                ratio);
            pOutputRgn->a3_t2_scale_y                 = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a3_t2_scale_y,
                pInput2Rgn->a3_t2_scale_y,
                ratio);
            pOutputRgn->a3_t2_offs_y                  = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a3_t2_offs_y,
                pInput2Rgn->a3_t2_offs_y,
                ratio);
            pOutputRgn->a3_t1_scale_c                 = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a3_t1_scale_c,
                pInput2Rgn->a3_t1_scale_c,
                ratio);
            pOutputRgn->a3_t1_offs_c                  = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a3_t1_offs_c,
                pInput2Rgn->a3_t1_offs_c,
                ratio);
            pOutputRgn->a3_t2_scale_c                 = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a3_t2_scale_c,
                pInput2Rgn->a3_t2_scale_c,
                ratio);
            pOutputRgn->a3_t2_offs_c                  = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->a3_t2_offs_c,
                pInput2Rgn->a3_t2_offs_c,
                ratio);
            pOutputRgn->constant_blending_factor_y    = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->constant_blending_factor_y,
                pInput2Rgn->constant_blending_factor_y,
                ratio);
            pOutputRgn->constant_blending_factor_c    = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->constant_blending_factor_c,
                pInput2Rgn->constant_blending_factor_c,
                ratio);
            pOutputRgn->scene_cut_recovery_time       = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->scene_cut_recovery_time,
                pInput2Rgn->scene_cut_recovery_time,
                ratio);

            pOutputRgn->out_of_frame_pixels_confidence                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->out_of_frame_pixels_confidence,
                pInput2Rgn->out_of_frame_pixels_confidence,
                ratio);
            pOutputRgn->indications_affect_fs_decision_also_directly  = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->indications_affect_fs_decision_also_directly,
                pInput2Rgn->indications_affect_fs_decision_also_directly,
                ratio);
            pOutputRgn->video_first_frame_spatial_nr_percentage       = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->video_first_frame_spatial_nr_percentage,
                pInput2Rgn->video_first_frame_spatial_nr_percentage,
                ratio);
            pOutputRgn->anr_final_blender_luma_min_strength_high_freq = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->anr_final_blender_luma_min_strength_high_freq,
                pInput2Rgn->anr_final_blender_luma_min_strength_high_freq,
                ratio);
            pOutputRgn->anr_final_blender_luma_min_strength_low_freq  = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->anr_final_blender_luma_min_strength_low_freq,
                pInput2Rgn->anr_final_blender_luma_min_strength_low_freq,
                ratio);
            pOutputRgn->anr_final_blender_chroma_min_strength_high_freq = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->anr_final_blender_chroma_min_strength_high_freq,
                pInput2Rgn->anr_final_blender_chroma_min_strength_high_freq,
                ratio);
            pOutputRgn->anr_final_blender_chroma_min_strength_low_freq = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->anr_final_blender_chroma_min_strength_low_freq,
                pInput2Rgn->anr_final_blender_chroma_min_strength_low_freq,
                ratio);
            pOutputRgn->anr_final_blender_strength_decision_ythr_low = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->anr_final_blender_strength_decision_ythr_low,
                pInput2Rgn->anr_final_blender_strength_decision_ythr_low,
                ratio);
            pOutputRgn->anr_final_blender_strength_decision_ythr_high = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->anr_final_blender_strength_decision_ythr_high,
                pInput2Rgn->anr_final_blender_strength_decision_ythr_high,
                ratio);
            pOutputRgn->anr_final_blender_strength_decision_cthr_low = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->anr_final_blender_strength_decision_cthr_low,
                pInput2Rgn->anr_final_blender_strength_decision_cthr_low,
                ratio);
            pOutputRgn->anr_final_blender_strength_decision_cthr_high = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->anr_final_blender_strength_decision_cthr_high,
                pInput2Rgn->anr_final_blender_strength_decision_cthr_high,
                ratio);

            pOutputRgn->lmc_enable = ((ratio + 0.500f) >= 1.0f) ? pInput2Rgn->lmc_enable : pInput1Rgn->lmc_enable;
            pOutputRgn->lmc_default_sad_c1 = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lmc_default_sad_c1,
                pInput2Rgn->lmc_default_sad_c1,
                ratio);
            pOutputRgn->lmc_default_sad_c2 = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lmc_default_sad_c2,
                pInput2Rgn->lmc_default_sad_c2,
                ratio);
            pOutputRgn->lmc_sad_c1 = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lmc_sad_c1,
                pInput2Rgn->lmc_sad_c1,
                ratio);
            pOutputRgn->lmc_sad_c2 = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lmc_sad_c2,
                pInput2Rgn->lmc_sad_c2,
                ratio);
            pOutputRgn->lmc_smoothness_m = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lmc_smoothness_m,
                pInput2Rgn->lmc_smoothness_m,
                ratio);
            pOutputRgn->lmc_smoothness_a = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lmc_smoothness_a,
                pInput2Rgn->lmc_smoothness_a,
                ratio);
            pOutputRgn->lmc_max_x = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lmc_max_x,
                pInput2Rgn->lmc_max_x,
                ratio);
            pOutputRgn->lmc_max_y = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lmc_max_y,
                pInput2Rgn->lmc_max_y,
                ratio);
            pOutputRgn->lmc_conf = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lmc_conf,
                pInput2Rgn->lmc_conf,
                ratio);
            pOutputRgn->lmc_irreg = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lmc_irreg,
                pInput2Rgn->lmc_irreg,
                ratio);

            pOutputRgn->lmc_mv_diff_thr = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lmc_mv_diff_thr,
                pInput2Rgn->lmc_mv_diff_thr,
                ratio);
        }
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TF20Interpolation::LensPositionSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TF20Interpolation::LensPositionSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tf_2_0_0::chromatix_tf20_coreType*   pParentDataType = NULL;
    TF20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tf_2_0_0::chromatix_tf20_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_tf20_lens_posn_dataCount;
        pTriggerList    = static_cast<TF20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_tf20_lens_posn_data[count].lens_posn_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerLensPosition;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_tf20_lens_posn_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_tf20_lens_posn_data[regionOutput.endIndex]),
                NULL);
            childCount++;
        }

        pParentNode->numChild = childCount;
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        childCount = 0;
    }

    return childCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TF20Interpolation::LensZoomSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TF20Interpolation::LensZoomSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tf_2_0_0::mod_tf20_lens_posn_dataType*   pParentDataType = NULL;
    TF20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tf_2_0_0::mod_tf20_lens_posn_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->lens_posn_data.mod_tf20_lens_zoom_dataCount;
        pTriggerList    = static_cast<TF20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->lens_posn_data.mod_tf20_lens_zoom_data[count].lens_zoom_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerLensZoom;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->lens_posn_data.mod_tf20_lens_zoom_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->lens_posn_data.mod_tf20_lens_zoom_data[regionOutput.endIndex]),
                NULL);
            childCount++;
        }

        pParentNode->numChild = childCount;
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        childCount = 0;
    }

    return childCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TF20Interpolation::PostScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TF20Interpolation::PostScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tf_2_0_0::mod_tf20_lens_zoom_dataType*   pParentDataType = NULL;
    TF20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tf_2_0_0::mod_tf20_lens_zoom_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->lens_zoom_data.mod_tf20_post_scale_ratio_dataCount;
        pTriggerList    = static_cast<TF20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->lens_zoom_data.mod_tf20_post_scale_ratio_data[count].post_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerPostScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->lens_zoom_data.mod_tf20_post_scale_ratio_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->lens_zoom_data.mod_tf20_post_scale_ratio_data[regionOutput.endIndex]),
                NULL);
            childCount++;
        }

        pParentNode->numChild = childCount;
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        childCount = 0;
    }

    return childCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TF20Interpolation::PreScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TF20Interpolation::PreScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tf_2_0_0::mod_tf20_post_scale_ratio_dataType*   pParentDataType = NULL;
    TF20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tf_2_0_0::mod_tf20_post_scale_ratio_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->post_scale_ratio_data.mod_tf20_pre_scale_ratio_dataCount;
        pTriggerList    = static_cast<TF20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->post_scale_ratio_data.mod_tf20_pre_scale_ratio_data[count].pre_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerPreScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->post_scale_ratio_data.mod_tf20_pre_scale_ratio_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>
                    (&pParentDataType->post_scale_ratio_data.mod_tf20_pre_scale_ratio_data[regionOutput.endIndex]),
                NULL);
            childCount++;
        }

        pParentNode->numChild = childCount;
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        childCount = 0;
    }

    return childCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TF20Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TF20Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tf_2_0_0::mod_tf20_pre_scale_ratio_dataType*   pParentDataType = NULL;
    TF20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tf_2_0_0::mod_tf20_pre_scale_ratio_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->pre_scale_ratio_data.mod_tf20_drc_gain_dataCount;
        pTriggerList    = static_cast<TF20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->pre_scale_ratio_data.mod_tf20_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->pre_scale_ratio_data.mod_tf20_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->pre_scale_ratio_data.mod_tf20_drc_gain_data[regionOutput.endIndex]),
                NULL);
            childCount++;
        }

        pParentNode->numChild = childCount;
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        childCount = 0;
    }

    return childCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TF20Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TF20Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tf_2_0_0::mod_tf20_drc_gain_dataType*   pParentDataType = NULL;
    TF20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tf_2_0_0::mod_tf20_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_tf20_hdr_aec_dataCount;
        pTriggerList    = static_cast<TF20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_tf20_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_tf20_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_tf20_hdr_aec_data[regionOutput.endIndex]),
                NULL);
            childCount++;
        }

        pParentNode->numChild = childCount;
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        childCount = 0;
    }

    return childCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TF20Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TF20Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tf_2_0_0::mod_tf20_hdr_aec_dataType*   pParentDataType = NULL;
    TF20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tf_2_0_0::mod_tf20_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_tf20_aec_dataCount;
        pTriggerList    = static_cast<TF20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_tf20_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_tf20_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_tf20_aec_data[regionOutput.endIndex]),
                NULL);
            childCount++;
        }

        pParentNode->numChild = childCount;
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        childCount = 0;
    }

    return childCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TF20Interpolation::CCTSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TF20Interpolation::CCTSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tf_2_0_0::mod_tf20_aec_dataType*   pParentDataType = NULL;
    TF20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tf_2_0_0::mod_tf20_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->aec_data.mod_tf20_cct_dataCount;
        pTriggerList    = static_cast<TF20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->aec_data.mod_tf20_cct_data[count].cct_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerCCT;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->aec_data.mod_tf20_cct_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->aec_data.mod_tf20_cct_data[regionOutput.startIndex].cct_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->aec_data.mod_tf20_cct_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->aec_data.mod_tf20_cct_data[regionOutput.endIndex].cct_data));
            childCount++;
        }

        pParentNode->numChild = childCount;
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        childCount = 0;
    }

    return childCount;
};
