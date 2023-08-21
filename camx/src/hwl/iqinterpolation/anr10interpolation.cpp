// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  anr10interpolation.cpp
/// @brief anr10 IQ setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "anr10interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation ANR10OperationTable[] =
{
    { ANR10Interpolation::LensPositionSearchNode,   2},
    { ANR10Interpolation::LensZoomSearchNode,       2},
    { ANR10Interpolation::PostScaleRatioSearchNode, 2},
    { ANR10Interpolation::PreScaleRatioSearchNode,  2},
    { ANR10Interpolation::DRCGainSearchNode,        2},
    { ANR10Interpolation::HDRAECSearchNode,         2},
    { ANR10Interpolation::AECSearchNode,            2},
    { ANR10Interpolation::CCTSearchNode,            2}
};

// Per XSD, interpolation tree has 10 level of trigger
static const UINT32 ANR10MaxNode            = 511; ///< (1 + 1*2 + 2*2 + 4*2 + 8*2 + 16 *2 + 32 * 2 + 64 * 2 + 128 * 2)
static const UINT32 ANR10MaxNonLeafNode     = 255; ///< (1 + 1*2 + 2*2 + 4*2 + 8*2 + 16 *2 + 32 * 2 + 64 * 2)
static const UINT32 ANR10InterpolationLevel = 9;   ///< Root->LensPos->LensZoom->PostScaleRatio->PreScaleRatio
                                                   ///< ->DRCGain->HDRAEC->AEC->CCT
CAMX_STATIC_ASSERT(ANRMaxNonLeafNode == ANR10MaxNonLeafNode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ANR10Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ANR10Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    ANR10InputData*   pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                    ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECGain,  pInput->AECGain))                        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))            ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio))  ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->CCTTrigger, pInput->AWBColorTemperature))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->lensPosition, pInput->lensPosition))               ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->lensZoom, pInput->lensZoom))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->postScaleRatio, pInput->postScaleRatio))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->preScaleRatio, pInput->preScaleRatio))             ||
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

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ANR10Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ANR10Interpolation::RunInterpolation(
    const ANR10InputData*                             pInput,
    anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pData)
{
    BOOL             result = TRUE;
    UINT             count  = 0;
    TuningNode       nodeSet[ANR10MaxNode];           // The intepolation tree total Node
    ANR10TriggerList ANR10Trigger;                    // Color Correction Trigger List
    anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pOutputData = &pData[0];

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (count = 0; count < ANR10MaxNode; count++)
        {
            if (count < ANR10MaxNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_anr10_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&ANR10Trigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(anr_1_0_0::chromatix_anr10Type::control_methodStruct));

        ANR10Trigger.triggerLensPosition   = pInput->lensPosition;
        ANR10Trigger.triggerLensZoom       = pInput->lensZoom;
        ANR10Trigger.triggerPostScaleRatio = pInput->postScaleRatio;
        ANR10Trigger.triggerPreScaleRatio  = pInput->preScaleRatio;

        ANR10Trigger.triggerDRCgain        = pInput->DRCGain;

        ANR10Trigger.triggerHDRAEC         =
            IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                             pInput->exposureTime,
                                             pInput->AECSensitivity,
                                             pInput->exposureGainRatio);
        ANR10Trigger.triggerAEC            =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->AECGain);

        ANR10Trigger.triggerCCT            = pInput->CCTTrigger;

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        ANR10InterpolationLevel,
                                                        &ANR10OperationTable[0],
                                                        static_cast<VOID*>(&ANR10Trigger));
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
                                                       ANR10MaxNonLeafNode,
                                                       ANR10InterpolationLevel,
                                                       ANR10Interpolation::DoInterpolation);
    }

    if (FALSE != result)
    {
        IQSettingUtils::Memcpy(pOutputData, nodeSet[0].pData, sizeof(anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ANR10Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ANR10Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pInput1 =
            static_cast<anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct*>(pData1);
        anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pInput2 =
            static_cast<anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct*>(pData2);
        anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pOutput =
            static_cast<anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct*>(pResult);

        if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = ANR10Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct));
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
/// ANR10Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ANR10Interpolation::InterpolationData(
    anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pInput1,
    anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pInput2,
    FLOAT                                              ratio,
    anr_1_0_0::mod_anr10_cct_dataType::cct_dataStruct* pOutput)
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
            passType             = static_cast<UINT>(pInput1->mod_anr10_pass_data[count].pass_trigger);
            input1Pass[passType] = count;
            passType             = static_cast<UINT>(pInput2->mod_anr10_pass_data[count].pass_trigger);
            input2Pass[passType] = count;
        }

        for (passType = static_cast<UINT>(ispglobalelements::trigger_pass::PASS_FULL);
             passType <= static_cast<UINT>(ispglobalelements::trigger_pass::PASS_DC64);
             passType++)
        {
            UINT input1Count = input1Pass[passType];
            UINT input2Count = input2Pass[passType];
            UINT index;

            anr_1_0_0::anr10_rgn_dataType*  pInput1Rgn = &pInput1->mod_anr10_pass_data[input1Count].anr10_rgn_data;
            anr_1_0_0::anr10_rgn_dataType*  pInput2Rgn = &pInput2->mod_anr10_pass_data[input2Count].anr10_rgn_data;
            anr_1_0_0::anr10_rgn_dataType*  pOutputRgn = &pOutput->mod_anr10_pass_data[passType].anr10_rgn_data;

            pOutput->mod_anr10_pass_data[passType].pass_trigger = pInput1->mod_anr10_pass_data[input1Count].pass_trigger;

            pOutputRgn->luma_chroma_filter_config.threshold_lut_control_avg_block_size
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->luma_chroma_filter_config.threshold_lut_control_avg_block_size,
                pInput2Rgn->luma_chroma_filter_config.threshold_lut_control_avg_block_size,
                ratio);
            pOutputRgn->luma_filter_config.filter_isotropic_min_filter_size
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_config.filter_isotropic_min_filter_size,
                pInput2Rgn->luma_filter_config.filter_isotropic_min_filter_size,
                ratio);
            pOutputRgn->luma_filter_config.filter_manual_derivatives_flags
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->luma_filter_config.filter_manual_derivatives_flags,
                pInput2Rgn->luma_filter_config.filter_manual_derivatives_flags,
                ratio);
            pOutputRgn->luma_filter_config.dcind_isotropic_min_size
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_config.dcind_isotropic_min_size,
                pInput2Rgn->luma_filter_config.dcind_isotropic_min_size,
                ratio);
            pOutputRgn->luma_filter_config.dcind_manual_derivatives_flags
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->luma_filter_config.dcind_manual_derivatives_flags,
                pInput2Rgn->luma_filter_config.dcind_manual_derivatives_flags,
                ratio);
            pOutputRgn->luma_filter_config.second_derivative_max_influence_radius_filtering
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_config.second_derivative_max_influence_radius_filtering,
                pInput2Rgn->luma_filter_config.second_derivative_max_influence_radius_filtering,
                ratio);
            pOutputRgn->luma_filter_config.second_derivative_max_influence_radius_dc_indication
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_config.second_derivative_max_influence_radius_dc_indication,
                pInput2Rgn->luma_filter_config.second_derivative_max_influence_radius_dc_indication,
                ratio);
            pOutputRgn->chroma_filter_config.filter_isotropic_min_filter_size
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_config.filter_isotropic_min_filter_size,
                pInput2Rgn->chroma_filter_config.filter_isotropic_min_filter_size,
                ratio);
            pOutputRgn->chroma_filter_config.filter_manual_derivatives_flags
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->chroma_filter_config.filter_manual_derivatives_flags,
                pInput2Rgn->chroma_filter_config.filter_manual_derivatives_flags,
                ratio);
            pOutputRgn->chroma_filter_config.dcind_isotropic_min_size
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_config.dcind_isotropic_min_size,
                pInput2Rgn->chroma_filter_config.dcind_isotropic_min_size,
                ratio);
            pOutputRgn->chroma_filter_config.dcind_manual_derivatives_flags
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->chroma_filter_config.dcind_manual_derivatives_flags,
                pInput2Rgn->chroma_filter_config.dcind_manual_derivatives_flags,
                ratio);
            pOutputRgn->luma_filter_kernel.edge_kernel_size
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.edge_kernel_size,
                pInput2Rgn->luma_filter_kernel.edge_kernel_size,
                ratio);
            pOutputRgn->luma_filter_kernel.automatic_definition_granularity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.automatic_definition_granularity,
                pInput2Rgn->luma_filter_kernel.automatic_definition_granularity,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_edge_kernel_1x1_center_coefficient
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_edge_kernel_1x1_center_coefficient,
                pInput2Rgn->luma_filter_kernel.manual_edge_kernel_1x1_center_coefficient,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_edge_kernel_3x3_horver_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_edge_kernel_3x3_horver_shift,
                pInput2Rgn->luma_filter_kernel.manual_edge_kernel_3x3_horver_shift,
                ratio);

            pOutputRgn->luma_filter_kernel.manual_edge_kernel_3x3_diag_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_edge_kernel_3x3_diag_shift,
                pInput2Rgn->luma_filter_kernel.manual_edge_kernel_3x3_diag_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_edge_kernel_5x5_horver_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_edge_kernel_5x5_horver_shift,
                pInput2Rgn->luma_filter_kernel.manual_edge_kernel_5x5_horver_shift,
                ratio);

            pOutputRgn->luma_filter_kernel.manual_edge_kernel_5x5_diag_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_edge_kernel_5x5_diag_shift,
                pInput2Rgn->luma_filter_kernel.manual_edge_kernel_5x5_diag_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_edge_kernel_5x5_complement_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_edge_kernel_5x5_complement_shift,
                pInput2Rgn->luma_filter_kernel.manual_edge_kernel_5x5_complement_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_edge_kernel_7x7_horver_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_edge_kernel_7x7_horver_shift,
                pInput2Rgn->luma_filter_kernel.manual_edge_kernel_7x7_horver_shift,
                ratio);

            pOutputRgn->luma_filter_kernel.manual_edge_kernel_7x7_diag_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_edge_kernel_7x7_diag_shift,
                pInput2Rgn->luma_filter_kernel.manual_edge_kernel_7x7_diag_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_edge_kernel_7x7_complement_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_edge_kernel_7x7_complement_shift,
                pInput2Rgn->luma_filter_kernel.manual_edge_kernel_7x7_complement_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_edge_kernel_9x9_horver_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_edge_kernel_9x9_horver_shift,
                pInput2Rgn->luma_filter_kernel.manual_edge_kernel_9x9_horver_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_edge_kernel_9x9_diag_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_edge_kernel_9x9_diag_shift,
                pInput2Rgn->luma_filter_kernel.manual_edge_kernel_9x9_diag_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_edge_kernel_9x9_complement_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_edge_kernel_9x9_complement_shift,
                pInput2Rgn->luma_filter_kernel.manual_edge_kernel_9x9_complement_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_blend_weight
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_blend_weight,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_blend_weight,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_size
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_size,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_size,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_1x1_center_coefficient
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_1x1_center_coefficient,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_1x1_center_coefficient,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_3x3_horver_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_3x3_horver_shift,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_3x3_horver_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_3x3_diag_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_3x3_diag_shift,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_3x3_diag_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_5x5_horver_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_5x5_horver_shift,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_5x5_horver_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_5x5_diag_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_5x5_diag_shift,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_5x5_diag_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_5x5_complement_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_5x5_complement_shift,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_5x5_complement_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_7x7_horver_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_7x7_horver_shift,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_7x7_horver_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_7x7_diag_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_7x7_diag_shift,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_7x7_diag_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_7x7_complement_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_7x7_complement_shift,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_7x7_complement_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_7x7_complement_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_7x7_complement_shift,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_7x7_complement_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_9x9_horver_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_9x9_horver_shift,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_9x9_horver_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_9x9_diag_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_9x9_diag_shift,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_9x9_diag_shift,
                ratio);
            pOutputRgn->luma_filter_kernel.manual_flat_kernel_9x9_complement_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_kernel.manual_flat_kernel_9x9_complement_shift,
                pInput2Rgn->luma_filter_kernel.manual_flat_kernel_9x9_complement_shift,
                ratio);
            pOutputRgn->chroma_filter_kernel.kernel_size
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_kernel.kernel_size,
                pInput2Rgn->chroma_filter_kernel.kernel_size,
                ratio);
            pOutputRgn->luma_peak_management.detect_hard_decision_environment_activity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_peak_management.detect_hard_decision_environment_activity,
                pInput2Rgn->luma_peak_management.detect_hard_decision_environment_activity,
                ratio);
            pOutputRgn->luma_peak_management.detect_hard_decision_distance_from_average
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_peak_management.detect_hard_decision_distance_from_average,
                pInput2Rgn->luma_peak_management.detect_hard_decision_distance_from_average,
                ratio);
            pOutputRgn->luma_peak_management.detect_soft_decision_distance_from_average_lower_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_peak_management.detect_soft_decision_distance_from_average_lower_limit,
                pInput2Rgn->luma_peak_management.detect_soft_decision_distance_from_average_lower_limit,
                ratio);
            pOutputRgn->luma_peak_management.detect_soft_decision_distance_from_average_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_peak_management.detect_soft_decision_distance_from_average_offset,
                pInput2Rgn->luma_peak_management.detect_soft_decision_distance_from_average_offset,
                ratio);
            pOutputRgn->luma_peak_management.detect_soft_decision_distance_from_average_slope
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_peak_management.detect_soft_decision_distance_from_average_slope,
                pInput2Rgn->luma_peak_management.detect_soft_decision_distance_from_average_slope,
                ratio);
            pOutputRgn->luma_peak_management.detect_extreme_decision_distance_from_maxmin
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_peak_management.detect_extreme_decision_distance_from_maxmin,
                pInput2Rgn->luma_peak_management.detect_extreme_decision_distance_from_maxmin,
                ratio);
            pOutputRgn->luma_peak_management.detect_dcsize_decision_sensitivity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_peak_management.detect_dcsize_decision_sensitivity,
                pInput2Rgn->luma_peak_management.detect_dcsize_decision_sensitivity,
                ratio);
            pOutputRgn->luma_peak_management.correction_mode
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->luma_peak_management.correction_mode,
                pInput2Rgn->luma_peak_management.correction_mode,
                ratio);
            pOutputRgn->luma_peak_management.correction_area_smart_min_inner_distance
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_peak_management.correction_area_smart_min_inner_distance,
                pInput2Rgn->luma_peak_management.correction_area_smart_min_inner_distance,
                ratio);
            pOutputRgn->luma_peak_management.correction_isotropic_activity_threshold
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_peak_management.correction_isotropic_activity_threshold,
                pInput2Rgn->luma_peak_management.correction_isotropic_activity_threshold,
                ratio);
            pOutputRgn->chroma_peak_management.detect_hard_decision_environment_activity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_peak_management.detect_hard_decision_environment_activity,
                pInput2Rgn->chroma_peak_management.detect_hard_decision_environment_activity,
                ratio);
            pOutputRgn->chroma_peak_management.detect_hard_decision_distance_from_average
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_peak_management.detect_hard_decision_distance_from_average,
                pInput2Rgn->chroma_peak_management.detect_hard_decision_distance_from_average,
                ratio);
            pOutputRgn->chroma_peak_management.detect_soft_decision_distance_from_average_lower_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_peak_management.detect_soft_decision_distance_from_average_lower_limit,
                pInput2Rgn->chroma_peak_management.detect_soft_decision_distance_from_average_lower_limit,
                ratio);
            pOutputRgn->chroma_peak_management.detect_soft_decision_distance_from_average_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_peak_management.detect_soft_decision_distance_from_average_offset,
                pInput2Rgn->chroma_peak_management.detect_soft_decision_distance_from_average_offset,
                ratio);
            pOutputRgn->chroma_peak_management.detect_soft_decision_distance_from_average_slope
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_peak_management.detect_soft_decision_distance_from_average_slope,
                pInput2Rgn->chroma_peak_management.detect_soft_decision_distance_from_average_slope,
                ratio);
            pOutputRgn->chroma_peak_management.detect_extreme_decision_distance_from_maxmin
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_peak_management.detect_extreme_decision_distance_from_maxmin,
                pInput2Rgn->chroma_peak_management.detect_extreme_decision_distance_from_maxmin,
                ratio);
            pOutputRgn->chroma_peak_management.detect_dcsize_decision_sensitivity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_peak_management.detect_dcsize_decision_sensitivity,
                pInput2Rgn->chroma_peak_management.detect_dcsize_decision_sensitivity,
                ratio);
            pOutputRgn->chroma_peak_management.correction_mode
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->chroma_peak_management.correction_mode,
                pInput2Rgn->chroma_peak_management.correction_mode,
                ratio);
            pOutputRgn->chroma_peak_management.correction_area_smart_min_inner_distance
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_peak_management.correction_area_smart_min_inner_distance,
                pInput2Rgn->chroma_peak_management.correction_area_smart_min_inner_distance,
                ratio);
            pOutputRgn->chroma_peak_management.correction_isotropic_activity_threshold
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_peak_management.correction_isotropic_activity_threshold,
                pInput2Rgn->chroma_peak_management.correction_isotropic_activity_threshold,
                ratio);

            pOutputRgn->inter_length_thr_modification.luma_input_indication_thr_modification_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->inter_length_thr_modification.luma_input_indication_thr_modification_scale,
                pInput2Rgn->inter_length_thr_modification.luma_input_indication_thr_modification_scale,
                ratio);
            pOutputRgn->inter_length_thr_modification.chroma_input_indication_thr_modification_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->inter_length_thr_modification.chroma_input_indication_thr_modification_scale,
                pInput2Rgn->inter_length_thr_modification.chroma_input_indication_thr_modification_scale,
                ratio);

            pOutputRgn->inter_length_output_indication.luma_center_binarization_minflatval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->inter_length_output_indication.luma_center_binarization_minflatval,
                pInput2Rgn->inter_length_output_indication.luma_center_binarization_minflatval,
                ratio);
            pOutputRgn->inter_length_output_indication.luma_neighbours_binarization_minflatval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->inter_length_output_indication.luma_neighbours_binarization_minflatval,
                pInput2Rgn->inter_length_output_indication.luma_neighbours_binarization_minflatval,
                ratio);
            pOutputRgn->inter_length_output_indication.luma_neighbours_parallel_dist
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->inter_length_output_indication.luma_neighbours_parallel_dist,
                pInput2Rgn->inter_length_output_indication.luma_neighbours_parallel_dist,
                ratio);
            pOutputRgn->inter_length_output_indication.luma_neighbours_perpendicular_dist
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->inter_length_output_indication.luma_neighbours_perpendicular_dist,
                pInput2Rgn->inter_length_output_indication.luma_neighbours_perpendicular_dist,
                ratio);
            pOutputRgn->inter_length_output_indication.luma_neighbours_agreement_sensitivity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->inter_length_output_indication.luma_neighbours_agreement_sensitivity,
                pInput2Rgn->inter_length_output_indication.luma_neighbours_agreement_sensitivity,
                ratio);
            pOutputRgn->inter_length_output_indication.chroma_center_binarization_minflatval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->inter_length_output_indication.chroma_center_binarization_minflatval,
                pInput2Rgn->inter_length_output_indication.chroma_center_binarization_minflatval,
                ratio);
            pOutputRgn->inter_length_output_indication.chroma_neighbours_binarization_minflatval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->inter_length_output_indication.chroma_neighbours_binarization_minflatval,
                pInput2Rgn->inter_length_output_indication.chroma_neighbours_binarization_minflatval,
                ratio);
            pOutputRgn->inter_length_output_indication.chroma_neighbours_perpendicular_dist
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->inter_length_output_indication.chroma_neighbours_perpendicular_dist,
                pInput2Rgn->inter_length_output_indication.chroma_neighbours_perpendicular_dist,
                ratio);
            pOutputRgn->inter_length_output_indication.chroma_neighbours_agreement_sensitivity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->inter_length_output_indication.chroma_neighbours_agreement_sensitivity,
                pInput2Rgn->inter_length_output_indication.chroma_neighbours_agreement_sensitivity,
                ratio);
            pOutputRgn->grey_treatment.enable_grey_treatment_dcblend2_chroma_modification
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->grey_treatment.enable_grey_treatment_dcblend2_chroma_modification,
                pInput2Rgn->grey_treatment.enable_grey_treatment_dcblend2_chroma_modification,
                ratio);
            pOutputRgn->grey_treatment.detect_grey_condition_chromaticity_radius
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->grey_treatment.detect_grey_condition_chromaticity_radius,
                pInput2Rgn->grey_treatment.detect_grey_condition_chromaticity_radius,
                ratio);
            pOutputRgn->grey_treatment.detect_grey_condition_chromaticity_thr_low
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->grey_treatment.detect_grey_condition_chromaticity_thr_low,
                pInput2Rgn->grey_treatment.detect_grey_condition_chromaticity_thr_low,
                ratio);
            pOutputRgn->grey_treatment.detect_grey_condition_chromaticity_thr_high
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->grey_treatment.detect_grey_condition_chromaticity_thr_high,
                pInput2Rgn->grey_treatment.detect_grey_condition_chromaticity_thr_high,
                ratio);

            pOutputRgn->grey_treatment.detect_grey_condition_y_max_derivative_radius
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->grey_treatment.detect_grey_condition_y_max_derivative_radius,
                pInput2Rgn->grey_treatment.detect_grey_condition_y_max_derivative_radius,
                ratio);
            pOutputRgn->grey_treatment.detect_grey_condition_y_max_derivative_thr_low
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->grey_treatment.detect_grey_condition_y_max_derivative_thr_low,
                pInput2Rgn->grey_treatment.detect_grey_condition_y_max_derivative_thr_low,
                ratio);
            pOutputRgn->grey_treatment.detect_grey_condition_y_max_derivative_thr_high
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->grey_treatment.detect_grey_condition_y_max_derivative_thr_high,
                pInput2Rgn->grey_treatment.detect_grey_condition_y_max_derivative_thr_high,
                ratio);
            pOutputRgn->grey_treatment.detect_greydcsize_neighbors_chromaticity_thr
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->grey_treatment.detect_greydcsize_neighbors_chromaticity_thr,
                pInput2Rgn->grey_treatment.detect_greydcsize_neighbors_chromaticity_thr,
                ratio);

            pOutputRgn->grey_treatment.detect_greydcsize_center_detail_chromaticity_thr
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->grey_treatment.detect_greydcsize_center_detail_chromaticity_thr,
                pInput2Rgn->grey_treatment.detect_greydcsize_center_detail_chromaticity_thr,
                ratio);
            pOutputRgn->grey_treatment.thr_modification_target_y
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->grey_treatment.thr_modification_target_y,
                pInput2Rgn->grey_treatment.thr_modification_target_y,
                ratio);
            pOutputRgn->grey_treatment.thr_modification_target_u
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->grey_treatment.thr_modification_target_u,
                pInput2Rgn->grey_treatment.thr_modification_target_u,
                ratio);
            pOutputRgn->grey_treatment.thr_modification_target_v
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->grey_treatment.thr_modification_target_v,
                pInput2Rgn->grey_treatment.thr_modification_target_v,
                ratio);

            pOutputRgn->grey_treatment.isotropic_filter_blend_factor_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->grey_treatment.isotropic_filter_blend_factor_scale,
                pInput2Rgn->grey_treatment.isotropic_filter_blend_factor_scale,
                ratio);
            pOutputRgn->grey_treatment.isotropic_filter_blend_factor_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->grey_treatment.isotropic_filter_blend_factor_offset,
                pInput2Rgn->grey_treatment.isotropic_filter_blend_factor_offset,
                ratio);
            pOutputRgn->grey_treatment.isotropic_filter_size
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->grey_treatment.isotropic_filter_size,
                pInput2Rgn->grey_treatment.isotropic_filter_size,
                ratio);

            pOutputRgn->chroma_filter_extension.median_detect_override_detail_condition_chromaticity_thr
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.median_detect_override_detail_condition_chromaticity_thr,
                pInput2Rgn->chroma_filter_extension.median_detect_override_detail_condition_chromaticity_thr,
                ratio);
            pOutputRgn->chroma_filter_extension.median_detect_override_detail_condition_y_max_derivative_thr
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.median_detect_override_detail_condition_y_max_derivative_thr,
                pInput2Rgn->chroma_filter_extension.median_detect_override_detail_condition_y_max_derivative_thr,
                ratio);
            pOutputRgn->chroma_filter_extension.median_detect_corner_detail_sensitivity_y
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.median_detect_corner_detail_sensitivity_y,
                pInput2Rgn->chroma_filter_extension.median_detect_corner_detail_sensitivity_y,
                ratio);
            pOutputRgn->chroma_filter_extension.median_detect_corner_detail_sensitivity_uv
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.median_detect_corner_detail_sensitivity_uv,
                pInput2Rgn->chroma_filter_extension.median_detect_corner_detail_sensitivity_uv,
                ratio);
            pOutputRgn->chroma_filter_extension.median_detect_triple_chromaticities_detail_thr_up_down
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.median_detect_triple_chromaticities_detail_thr_up_down,
                pInput2Rgn->chroma_filter_extension.median_detect_triple_chromaticities_detail_thr_up_down,
                ratio);

            pOutputRgn->chroma_filter_extension.median_detect_triple_chromaticities_detail_thr_external
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.median_detect_triple_chromaticities_detail_thr_external,
                pInput2Rgn->chroma_filter_extension.median_detect_triple_chromaticities_detail_thr_external,
                ratio);
            pOutputRgn->chroma_filter_extension.median_detect_isotropic_self_decision_enforce_detail
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->chroma_filter_extension.median_detect_isotropic_self_decision_enforce_detail,
                pInput2Rgn->chroma_filter_extension.median_detect_isotropic_self_decision_enforce_detail,
                ratio);
            pOutputRgn->chroma_filter_extension.median_detect_isotropic_neighbors_detail_sensitivity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.median_detect_isotropic_neighbors_detail_sensitivity,
                pInput2Rgn->chroma_filter_extension.median_detect_isotropic_neighbors_detail_sensitivity,
                ratio);
            pOutputRgn->chroma_filter_extension.median_detect_isotropic_neighbors_detail_sensitivity_far_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.median_detect_isotropic_neighbors_detail_sensitivity_far_scale,
                pInput2Rgn->chroma_filter_extension.median_detect_isotropic_neighbors_detail_sensitivity_far_scale,
                ratio);
            pOutputRgn->chroma_filter_extension.median_detect_directional_self_decision_enforce_detail
                = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1Rgn->chroma_filter_extension.median_detect_directional_self_decision_enforce_detail,
                pInput2Rgn->chroma_filter_extension.median_detect_directional_self_decision_enforce_detail,
                ratio);

            pOutputRgn->chroma_filter_extension.median_detect_directional_neighbors_detail_sensitivity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.median_detect_directional_neighbors_detail_sensitivity,
                pInput2Rgn->chroma_filter_extension.median_detect_directional_neighbors_detail_sensitivity,
                ratio);
            pOutputRgn->chroma_filter_extension.median_detect_directional_neighbors_detail_sensitivity_far_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.median_detect_directional_neighbors_detail_sensitivity_far_scale,
                pInput2Rgn->chroma_filter_extension.median_detect_directional_neighbors_detail_sensitivity_far_scale,
                ratio);
            pOutputRgn->chroma_filter_extension.bilateral_decision_minimalsize
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.bilateral_decision_minimalsize,
                pInput2Rgn->chroma_filter_extension.bilateral_decision_minimalsize,
                ratio);
            pOutputRgn->chroma_filter_extension.bilateral_filtersize
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.bilateral_filtersize,
                pInput2Rgn->chroma_filter_extension.bilateral_filtersize,
                ratio);
            pOutputRgn->chroma_filter_extension.chroma_flat_indication_extension_threshold_9x9
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.chroma_flat_indication_extension_threshold_9x9,
                pInput2Rgn->chroma_filter_extension.chroma_flat_indication_extension_threshold_9x9,
                ratio);

            pOutputRgn->chroma_filter_extension.chroma_flat_indication_extension_threshold_11x11
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.chroma_flat_indication_extension_threshold_11x11,
                pInput2Rgn->chroma_filter_extension.chroma_flat_indication_extension_threshold_11x11,
                ratio);
            pOutputRgn->chroma_filter_extension.chroma_grey_indication_extension_threshold_9x9
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.chroma_grey_indication_extension_threshold_9x9,
                pInput2Rgn->chroma_filter_extension.chroma_grey_indication_extension_threshold_9x9,
                ratio);
            pOutputRgn->chroma_filter_extension.chroma_grey_indication_extension_threshold_11x11
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_extension.chroma_grey_indication_extension_threshold_11x11,
                pInput2Rgn->chroma_filter_extension.chroma_grey_indication_extension_threshold_11x11,
                ratio);

            pOutputRgn->luma_smoothing_treatment.edge_smoothing_binarization_minflatval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.edge_smoothing_binarization_minflatval,
                pInput2Rgn->luma_smoothing_treatment.edge_smoothing_binarization_minflatval,
                ratio);
            pOutputRgn->luma_smoothing_treatment.edge_smoothing_binarization_maxedgeval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.edge_smoothing_binarization_maxedgeval,
                pInput2Rgn->luma_smoothing_treatment.edge_smoothing_binarization_maxedgeval,
                ratio);
            pOutputRgn->luma_smoothing_treatment.edge_smoothing_agreement_number
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.edge_smoothing_agreement_number,
                pInput2Rgn->luma_smoothing_treatment.edge_smoothing_agreement_number,
                ratio);
            pOutputRgn->luma_smoothing_treatment.transition_smoothing_filter_size
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.transition_smoothing_filter_size,
                pInput2Rgn->luma_smoothing_treatment.transition_smoothing_filter_size,
                ratio);
            pOutputRgn->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_minflatval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_minflatval,
                pInput2Rgn->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_minflatval,
                ratio);
            pOutputRgn->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_minedgeval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_minedgeval,
                pInput2Rgn->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_minedgeval,
                ratio);

            pOutputRgn->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_maxedgeval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_maxedgeval,
                pInput2Rgn->luma_smoothing_treatment.transition_isotropic_neighbours_binarization_maxedgeval,
                ratio);
            pOutputRgn->luma_smoothing_treatment.transition_isotropic_neighbours_agreement_sensitivity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.transition_isotropic_neighbours_agreement_sensitivity,
                pInput2Rgn->luma_smoothing_treatment.transition_isotropic_neighbours_agreement_sensitivity,
                ratio);
            pOutputRgn->luma_smoothing_treatment.transition_isotropic_neighbours_agreement_flat_vs_voters
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.transition_isotropic_neighbours_agreement_flat_vs_voters,
                pInput2Rgn->luma_smoothing_treatment.transition_isotropic_neighbours_agreement_flat_vs_voters,
                ratio);
            pOutputRgn->luma_smoothing_treatment.flat_isotropic_3x3_neighbours_binarization_minflatval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.flat_isotropic_3x3_neighbours_binarization_minflatval,
                pInput2Rgn->luma_smoothing_treatment.flat_isotropic_3x3_neighbours_binarization_minflatval,
                ratio);
            pOutputRgn->luma_smoothing_treatment.flat_isotropic_3x3_neighbours_agreement_sensitivity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.flat_isotropic_3x3_neighbours_agreement_sensitivity,
                pInput2Rgn->luma_smoothing_treatment.flat_isotropic_3x3_neighbours_agreement_sensitivity,
                ratio);
            pOutputRgn->luma_smoothing_treatment.flat_isotropic_5x5_neighbours_agreement_sensitivity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.flat_isotropic_5x5_neighbours_agreement_sensitivity,
                pInput2Rgn->luma_smoothing_treatment.flat_isotropic_5x5_neighbours_agreement_sensitivity,
                ratio);

            pOutputRgn->luma_smoothing_treatment.smoothing_kernel_3x3_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.smoothing_kernel_3x3_shift,
                pInput2Rgn->luma_smoothing_treatment.smoothing_kernel_3x3_shift,
                ratio);
            pOutputRgn->luma_smoothing_treatment.smoothing_kernel_5x5_horver_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_smoothing_treatment.smoothing_kernel_5x5_horver_shift,
                pInput2Rgn->luma_smoothing_treatment.smoothing_kernel_5x5_horver_shift,
                ratio);

            pOutputRgn->chroma_smoothing_treatment.edge_smoothing_binarization_minflatval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_smoothing_treatment.edge_smoothing_binarization_minflatval,
                pInput2Rgn->chroma_smoothing_treatment.edge_smoothing_binarization_minflatval,
                ratio);
            pOutputRgn->chroma_smoothing_treatment.edge_smoothing_binarization_maxedgeval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_smoothing_treatment.edge_smoothing_binarization_maxedgeval,
                pInput2Rgn->chroma_smoothing_treatment.edge_smoothing_binarization_maxedgeval,
                ratio);
            pOutputRgn->chroma_smoothing_treatment.edge_smoothing_agreement_number
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_smoothing_treatment.edge_smoothing_agreement_number,
                pInput2Rgn->chroma_smoothing_treatment.edge_smoothing_agreement_number,
                ratio);
            pOutputRgn->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_minflatval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_minflatval,
                pInput2Rgn->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_minflatval,
                ratio);
            pOutputRgn->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_minedgeval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_minedgeval,
                pInput2Rgn->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_minedgeval,
                ratio);

            pOutputRgn->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_maxedgeval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_maxedgeval,
                pInput2Rgn->chroma_smoothing_treatment.transition_isotropic_neighbours_binarization_maxedgeval,
                ratio);
            pOutputRgn->chroma_smoothing_treatment.transition_isotropic_neighbours_agreement_sensitivity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_smoothing_treatment.transition_isotropic_neighbours_agreement_sensitivity,
                pInput2Rgn->chroma_smoothing_treatment.transition_isotropic_neighbours_agreement_sensitivity,
                ratio);
            pOutputRgn->chroma_smoothing_treatment.transition_isotropic_neighbours_agreement_flat_vs_voters
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_smoothing_treatment.transition_isotropic_neighbours_agreement_flat_vs_voters,
                pInput2Rgn->chroma_smoothing_treatment.transition_isotropic_neighbours_agreement_flat_vs_voters,
                ratio);
            pOutputRgn->chroma_smoothing_treatment.flat_isotropic_3x3_neighbours_binarization_minflatval
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_smoothing_treatment.flat_isotropic_3x3_neighbours_binarization_minflatval,
                pInput2Rgn->chroma_smoothing_treatment.flat_isotropic_3x3_neighbours_binarization_minflatval,
                ratio);
            pOutputRgn->chroma_smoothing_treatment.flat_isotropic_3x3_neighbours_agreement_sensitivity
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_smoothing_treatment.flat_isotropic_3x3_neighbours_agreement_sensitivity,
                pInput2Rgn->chroma_smoothing_treatment.flat_isotropic_3x3_neighbours_agreement_sensitivity,
                ratio);
            pOutputRgn->chroma_smoothing_treatment.smoothing_kernel_1x1_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_smoothing_treatment.smoothing_kernel_1x1_shift,
                pInput2Rgn->chroma_smoothing_treatment.smoothing_kernel_1x1_shift,
                ratio);
            pOutputRgn->chroma_smoothing_treatment.smoothing_kernel_3x3_shift
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_smoothing_treatment.smoothing_kernel_3x3_shift,
                pInput2Rgn->chroma_smoothing_treatment.smoothing_kernel_3x3_shift,
                ratio);
            pOutputRgn->lnr.elliptic_xc = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr.elliptic_xc,
                pInput2Rgn->lnr.elliptic_xc,
                ratio);
            pOutputRgn->lnr.elliptic_yc = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr.elliptic_yc,
                pInput2Rgn->lnr.elliptic_yc,
                ratio);
            pOutputRgn->lnr.elliptic_a = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr.elliptic_a,
                pInput2Rgn->lnr.elliptic_a,
                ratio);
            for (index = 0; index < LUMA_FILTER_LUT_Y_SIZE; index++)
            {
                pOutputRgn->lnr.luma_filter_lut_thr_y_tab.luma_filter_lut_thr_y[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->lnr.luma_filter_lut_thr_y_tab.luma_filter_lut_thr_y[index],
                    pInput2Rgn->lnr.luma_filter_lut_thr_y_tab.luma_filter_lut_thr_y[index],
                    ratio);
            }
            for (index = 0; index < LUMA_FILTER_LUT_UV_SIZE; index++)
            {
                pOutputRgn->lnr.luma_filter_lut_thr_uv_tab.luma_filter_lut_thr_uv[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->lnr.luma_filter_lut_thr_uv_tab.luma_filter_lut_thr_uv[index],
                    pInput2Rgn->lnr.luma_filter_lut_thr_uv_tab.luma_filter_lut_thr_uv[index],
                    ratio);
            }
            for (index = 0; index < CHROMA_FILTER_LUT_Y_SIZE; index++)
            {
                pOutputRgn->lnr.chroma_filter_lut_thr_y_tab.chroma_filter_lut_thr_y[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->lnr.chroma_filter_lut_thr_y_tab.chroma_filter_lut_thr_y[index],
                    pInput2Rgn->lnr.chroma_filter_lut_thr_y_tab.chroma_filter_lut_thr_y[index],
                    ratio);
            }
            for (index = 0; index < CHROMA_FILTER_LUT_UV_SIZE; index++)
            {
                pOutputRgn->lnr.chroma_filter_lut_thr_uv_tab.chroma_filter_lut_thr_uv[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->lnr.chroma_filter_lut_thr_uv_tab.chroma_filter_lut_thr_uv[index],
                    pInput2Rgn->lnr.chroma_filter_lut_thr_uv_tab.chroma_filter_lut_thr_uv[index],
                    ratio);
            }
            for (index = 0; index < STRENGTH_MODIFIER_RADIUS_BLEND; index++)
            {
                pOutputRgn->lnr.strength_modifier_radius_blend_lut_tab.strength_modifier_radius_blend_lut[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->lnr.strength_modifier_radius_blend_lut_tab.strength_modifier_radius_blend_lut[index],
                    pInput2Rgn->lnr.strength_modifier_radius_blend_lut_tab.strength_modifier_radius_blend_lut[index],
                    ratio);
            }
            pOutputRgn->lnr.luma_lnr_dcblend2_target_factor = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr.luma_lnr_dcblend2_target_factor,
                pInput2Rgn->lnr.luma_lnr_dcblend2_target_factor,
                ratio);
            pOutputRgn->lnr.luma_lnr_flat_kernel_weight_target_factor = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr.luma_lnr_flat_kernel_weight_target_factor,
                pInput2Rgn->lnr.luma_lnr_flat_kernel_weight_target_factor,
                ratio);
            pOutputRgn->lnr.chroma_lnr_cnr_dcblend2_target_factor = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr.chroma_lnr_cnr_dcblend2_target_factor,
                pInput2Rgn->lnr.chroma_lnr_cnr_dcblend2_target_factor,
                ratio);
            pOutputRgn->lnr.automatic_influence_luma_luts = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr.automatic_influence_luma_luts,
                pInput2Rgn->lnr.automatic_influence_luma_luts,
                ratio);
            pOutputRgn->lnr.automatic_influence_chroma_luts = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr.automatic_influence_chroma_luts,
                pInput2Rgn->lnr.automatic_influence_chroma_luts,
                ratio);
            pOutputRgn->lnr.automatic_influence_modifier_radius_blend_lut
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->lnr.automatic_influence_modifier_radius_blend_lut,
                pInput2Rgn->lnr.automatic_influence_modifier_radius_blend_lut,
                ratio);
            for (index = 0; index < DETECT_ANGLE_START; index++)
            {
                pOutputRgn->cnr.detect_angle_start_tab.detect_angle_start[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.detect_angle_start_tab.detect_angle_start[index],
                    pInput2Rgn->cnr.detect_angle_start_tab.detect_angle_start[index],
                    ratio);
            }
            for (index = 0; index < DETECT_ANGLE_END; index++)
            {
                pOutputRgn->cnr.detect_angle_end_tab.detect_angle_end[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.detect_angle_end_tab.detect_angle_end[index],
                    pInput2Rgn->cnr.detect_angle_end_tab.detect_angle_end[index],
                    ratio);
            }
            for (index = 0; index < DETECT_CHROMATICITY_START; index++)
            {
                pOutputRgn->cnr.detect_chromaticity_start_tab.detect_chromaticity_start[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.detect_chromaticity_start_tab.detect_chromaticity_start[index],
                    pInput2Rgn->cnr.detect_chromaticity_start_tab.detect_chromaticity_start[index],
                    ratio);
            }
            for (index = 0; index < DETECT_CHROMATICITY_END; index++)
            {
                pOutputRgn->cnr.detect_chromaticity_end_tab.detect_chromaticity_end[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.detect_chromaticity_end_tab.detect_chromaticity_end[index],
                    pInput2Rgn->cnr.detect_chromaticity_end_tab.detect_chromaticity_end[index],
                    ratio);
            }
            for (index = 0; index < DETECT_LUMA_START; index++)
            {
                pOutputRgn->cnr.detect_luma_start_tab.detect_luma_start[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.detect_luma_start_tab.detect_luma_start[index],
                    pInput2Rgn->cnr.detect_luma_start_tab.detect_luma_start[index],
                    ratio);
            }
            for (index = 0; index < DETECT_LUMA_END; index++)
            {
                pOutputRgn->cnr.detect_luma_end_tab.detect_luma_end[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.detect_luma_end_tab.detect_luma_end[index],
                    pInput2Rgn->cnr.detect_luma_end_tab.detect_luma_end[index],
                    ratio);
            }
            pOutputRgn->cnr.detect_color0_skin_saturation_min_y_min = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->cnr.detect_color0_skin_saturation_min_y_min,
                pInput2Rgn->cnr.detect_color0_skin_saturation_min_y_min,
                ratio);
            pOutputRgn->cnr.detect_color0_skin_saturation_max_y_min = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->cnr.detect_color0_skin_saturation_max_y_min,
                pInput2Rgn->cnr.detect_color0_skin_saturation_max_y_min,
                ratio);
            pOutputRgn->cnr.detect_color0_skin_saturation_min_y_max = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->cnr.detect_color0_skin_saturation_min_y_max,
                pInput2Rgn->cnr.detect_color0_skin_saturation_min_y_max,
                ratio);
            pOutputRgn->cnr.detect_color0_skin_saturation_max_y_max = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->cnr.detect_color0_skin_saturation_max_y_max,
                pInput2Rgn->cnr.detect_color0_skin_saturation_max_y_max,
                ratio);
            for (index = 0; index < BOUNDARY_WEIGHT; index++)
            {
                pOutputRgn->cnr.boundary_weight_tab.boundary_weight[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.boundary_weight_tab.boundary_weight[index],
                    pInput2Rgn->cnr.boundary_weight_tab.boundary_weight[index],
                    ratio);
            }
            for (index = 0; index < TRANSITION_RATIO; index++)
            {
                pOutputRgn->cnr.transition_ratio_tab.transition_ratio[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.transition_ratio_tab.transition_ratio[index],
                    pInput2Rgn->cnr.transition_ratio_tab.transition_ratio[index],
                    ratio);
            }
            pOutputRgn->cnr.color0_transition_ratio_external
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->cnr.color0_transition_ratio_external,
                pInput2Rgn->cnr.color0_transition_ratio_external,
                ratio);
            for (index = 0; index < LUMA_FILTER_THR_SCALE; index++)
            {
                pOutputRgn->cnr.luma_filter_threshold_scale_y_tab.luma_filter_threshold_scale_y[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.luma_filter_threshold_scale_y_tab.luma_filter_threshold_scale_y[index],
                    pInput2Rgn->cnr.luma_filter_threshold_scale_y_tab.luma_filter_threshold_scale_y[index],
                    ratio);
                pOutputRgn->cnr.luma_filter_threshold_scale_uv_tab.luma_filter_threshold_scale_uv[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.luma_filter_threshold_scale_uv_tab.luma_filter_threshold_scale_uv[index],
                    pInput2Rgn->cnr.luma_filter_threshold_scale_uv_tab.luma_filter_threshold_scale_uv[index],
                    ratio);
            }
            for (index = 0; index < LUMA_FILTER_OFFSET; index++)
            {
                pOutputRgn->cnr.luma_filter_offset_y_tab.luma_filter_offset_y[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.luma_filter_offset_y_tab.luma_filter_offset_y[index],
                    pInput2Rgn->cnr.luma_filter_offset_y_tab.luma_filter_offset_y[index],
                    ratio);
                pOutputRgn->cnr.luma_filter_offset_u_tab.luma_filter_offset_u[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.luma_filter_offset_u_tab.luma_filter_offset_u[index],
                    pInput2Rgn->cnr.luma_filter_offset_u_tab.luma_filter_offset_u[index],
                    ratio);
                pOutputRgn->cnr.luma_filter_offset_v_tab.luma_filter_offset_v[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.luma_filter_offset_v_tab.luma_filter_offset_v[index],
                    pInput2Rgn->cnr.luma_filter_offset_v_tab.luma_filter_offset_v[index],
                    ratio);
            }
            for (index = 0; index < CHROMA_FILTER_THR_SCALE; index++)
            {
                pOutputRgn->cnr.chroma_filter_threshold_scale_y_tab.chroma_filter_threshold_scale_y[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.chroma_filter_threshold_scale_y_tab.chroma_filter_threshold_scale_y[index],
                    pInput2Rgn->cnr.chroma_filter_threshold_scale_y_tab.chroma_filter_threshold_scale_y[index],
                    ratio);
                pOutputRgn->cnr.chroma_filter_threshold_scale_uv_tab.chroma_filter_threshold_scale_uv[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.chroma_filter_threshold_scale_uv_tab.chroma_filter_threshold_scale_uv[index],
                    pInput2Rgn->cnr.chroma_filter_threshold_scale_uv_tab.chroma_filter_threshold_scale_uv[index],
                    ratio);
            }
            for (index = 0; index < CHROMA_FILTER_OFFSET; index++)
            {
                pOutputRgn->cnr.chroma_filter_offset_y_tab.chroma_filter_offset_y[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.chroma_filter_offset_y_tab.chroma_filter_offset_y[index],
                    pInput2Rgn->cnr.chroma_filter_offset_y_tab.chroma_filter_offset_y[index],
                    ratio);
                pOutputRgn->cnr.chroma_filter_offset_u_tab.chroma_filter_offset_u[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.chroma_filter_offset_u_tab.chroma_filter_offset_u[index],
                    pInput2Rgn->cnr.chroma_filter_offset_u_tab.chroma_filter_offset_u[index],
                    ratio);
                pOutputRgn->cnr.chroma_filter_offset_v_tab.chroma_filter_offset_v[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.chroma_filter_offset_v_tab.chroma_filter_offset_v[index],
                    pInput2Rgn->cnr.chroma_filter_offset_v_tab.chroma_filter_offset_v[index],
                    ratio);
            }
            pOutputRgn->cnr.luma_filter_base_far_modifier_y = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->cnr.luma_filter_base_far_modifier_y,
                pInput2Rgn->cnr.luma_filter_base_far_modifier_y,
                ratio);
            pOutputRgn->cnr.luma_filter_base_far_modifier_uv = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->cnr.luma_filter_base_far_modifier_uv,
                pInput2Rgn->cnr.luma_filter_base_far_modifier_uv,
                ratio);
            pOutputRgn->cnr.chroma_filter_base_far_modifier_y = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->cnr.chroma_filter_base_far_modifier_y,
                pInput2Rgn->cnr.chroma_filter_base_far_modifier_y,
                ratio);
            pOutputRgn->cnr.chroma_filter_base_far_modifier_uv = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->cnr.chroma_filter_base_far_modifier_uv,
                pInput2Rgn->cnr.chroma_filter_base_far_modifier_uv,
                ratio);
            for (index = 0; index < LUMA_DCBLEND2_WEIGHT; index++)
            {
                pOutputRgn->cnr.luma_dcblend2_weight_scale_tab.luma_dcblend2_weight_scale[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.luma_dcblend2_weight_scale_tab.luma_dcblend2_weight_scale[index],
                    pInput2Rgn->cnr.luma_dcblend2_weight_scale_tab.luma_dcblend2_weight_scale[index],
                    ratio);
            }
            for (index = 0; index < CHROMA_DCBLEND2_WEIGHT; index++)
            {
                pOutputRgn->cnr.chroma_dcblend2_weight_restricted_scale_tab.chroma_dcblend2_weight_restricted_scale[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.chroma_dcblend2_weight_restricted_scale_tab.chroma_dcblend2_weight_restricted_scale[index],
                    pInput2Rgn->cnr.chroma_dcblend2_weight_restricted_scale_tab.chroma_dcblend2_weight_restricted_scale[index],
                    ratio);
            }
            for (index = 0; index < LUMA_FLAT_KERNEL_BLEND_WEIGHT; index++)
            {
                pOutputRgn->cnr.luma_flat_kernel_blend_weight_scale_tab.luma_flat_kernel_blend_weight_scale[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->cnr.luma_flat_kernel_blend_weight_scale_tab.luma_flat_kernel_blend_weight_scale[index],
                    pInput2Rgn->cnr.luma_flat_kernel_blend_weight_scale_tab.luma_flat_kernel_blend_weight_scale[index],
                    ratio);
            }
            for (index = 0; index < Y_THR_PER_Y; index++)
            {
                pOutputRgn->luma_filter_detection_thresholds.y_threshold_per_y_tab.y_threshold_per_y[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->luma_filter_detection_thresholds.y_threshold_per_y_tab.y_threshold_per_y[index],
                    pInput2Rgn->luma_filter_detection_thresholds.y_threshold_per_y_tab.y_threshold_per_y[index],
                    ratio);
            }
            for (index = 0; index < Y_THR_PER_UV; index++)
            {
                pOutputRgn->luma_filter_detection_thresholds.y_threshold_per_uv_tab.y_threshold_per_uv[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->luma_filter_detection_thresholds.y_threshold_per_uv_tab.y_threshold_per_uv[index],
                    pInput2Rgn->luma_filter_detection_thresholds.y_threshold_per_uv_tab.y_threshold_per_uv[index],
                    ratio);
            }
            pOutputRgn->luma_filter_detection_thresholds.y_threshold_top_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.y_threshold_top_limit,
                pInput2Rgn->luma_filter_detection_thresholds.y_threshold_top_limit,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.y_threshold_bottom_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.y_threshold_bottom_limit,
                pInput2Rgn->luma_filter_detection_thresholds.y_threshold_bottom_limit,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.y_threshold_close3_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.y_threshold_close3_mod_scale,
                pInput2Rgn->luma_filter_detection_thresholds.y_threshold_close3_mod_scale,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.y_threshold_close3_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.y_threshold_close3_mod_offset,
                pInput2Rgn->luma_filter_detection_thresholds.y_threshold_close3_mod_offset,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.y_threshold_der2_close3_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.y_threshold_der2_close3_mod_scale,
                pInput2Rgn->luma_filter_detection_thresholds.y_threshold_der2_close3_mod_scale,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.y_threshold_der2_close3_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.y_threshold_der2_close3_mod_offset,
                pInput2Rgn->luma_filter_detection_thresholds.y_threshold_der2_close3_mod_offset,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.y_threshold_far_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.y_threshold_far_mod_scale,
                pInput2Rgn->luma_filter_detection_thresholds.y_threshold_far_mod_scale,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.y_threshold_far_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.y_threshold_far_mod_offset,
                pInput2Rgn->luma_filter_detection_thresholds.y_threshold_far_mod_offset,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.y_threshold_close_external_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.y_threshold_close_external_mod_scale,
                pInput2Rgn->luma_filter_detection_thresholds.y_threshold_close_external_mod_scale,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.y_threshold_close_external_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.y_threshold_close_external_mod_offset,
                pInput2Rgn->luma_filter_detection_thresholds.y_threshold_close_external_mod_offset,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.y_threshold_far_external_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.y_threshold_far_external_mod_scale,
                pInput2Rgn->luma_filter_detection_thresholds.y_threshold_far_external_mod_scale,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.y_threshold_far_external_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.y_threshold_far_external_mod_offset,
                pInput2Rgn->luma_filter_detection_thresholds.y_threshold_far_external_mod_offset,
                ratio);
            for (index = 0; index < U_THR_PER_Y; index++)
            {
                pOutputRgn->luma_filter_detection_thresholds.u_threshold_per_y_tab.u_threshold_per_y[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->luma_filter_detection_thresholds.u_threshold_per_y_tab.u_threshold_per_y[index],
                    pInput2Rgn->luma_filter_detection_thresholds.u_threshold_per_y_tab.u_threshold_per_y[index],
                    ratio);
            }
            for (index = 0; index < U_THR_PER_UV; index++)
            {
                pOutputRgn->luma_filter_detection_thresholds.u_threshold_per_uv_tab.u_threshold_per_uv[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->luma_filter_detection_thresholds.u_threshold_per_uv_tab.u_threshold_per_uv[index],
                    pInput2Rgn->luma_filter_detection_thresholds.u_threshold_per_uv_tab.u_threshold_per_uv[index],
                    ratio);
            }
            pOutputRgn->luma_filter_detection_thresholds.u_threshold_top_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.u_threshold_top_limit,
                pInput2Rgn->luma_filter_detection_thresholds.u_threshold_top_limit,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.u_threshold_bottom_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.u_threshold_bottom_limit,
                pInput2Rgn->luma_filter_detection_thresholds.u_threshold_bottom_limit,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.u_threshold_far_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.u_threshold_far_mod_scale,
                pInput2Rgn->luma_filter_detection_thresholds.u_threshold_far_mod_scale,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.u_threshold_far_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.u_threshold_far_mod_offset,
                pInput2Rgn->luma_filter_detection_thresholds.u_threshold_far_mod_offset,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.u_threshold_far_external_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.u_threshold_far_external_mod_scale,
                pInput2Rgn->luma_filter_detection_thresholds.u_threshold_far_external_mod_scale,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.u_threshold_far_external_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.u_threshold_far_external_mod_offset,
                pInput2Rgn->luma_filter_detection_thresholds.u_threshold_far_external_mod_offset,
                ratio);
            for (index = 0; index < V_THR_PER_Y; index++)
            {
                pOutputRgn->luma_filter_detection_thresholds.v_threshold_per_y_tab.v_threshold_per_y[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->luma_filter_detection_thresholds.v_threshold_per_y_tab.v_threshold_per_y[index],
                    pInput2Rgn->luma_filter_detection_thresholds.v_threshold_per_y_tab.v_threshold_per_y[index],
                    ratio);
            }
            for (index = 0; index < V_THR_PER_UV; index++)
            {
                pOutputRgn->luma_filter_detection_thresholds.v_threshold_per_uv_tab.v_threshold_per_uv[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->luma_filter_detection_thresholds.v_threshold_per_uv_tab.v_threshold_per_uv[index],
                    pInput2Rgn->luma_filter_detection_thresholds.v_threshold_per_uv_tab.v_threshold_per_uv[index],
                    ratio);
            }
            pOutputRgn->luma_filter_detection_thresholds.v_threshold_top_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.v_threshold_top_limit,
                pInput2Rgn->luma_filter_detection_thresholds.v_threshold_top_limit,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.v_threshold_bottom_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.v_threshold_bottom_limit,
                pInput2Rgn->luma_filter_detection_thresholds.v_threshold_bottom_limit,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.v_threshold_far_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.v_threshold_far_mod_scale,
                pInput2Rgn->luma_filter_detection_thresholds.v_threshold_far_mod_scale,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.v_threshold_far_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.v_threshold_far_mod_offset,
                pInput2Rgn->luma_filter_detection_thresholds.v_threshold_far_mod_offset,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.v_threshold_far_external_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.v_threshold_far_external_mod_scale,
                pInput2Rgn->luma_filter_detection_thresholds.v_threshold_far_external_mod_scale,
                ratio);
            pOutputRgn->luma_filter_detection_thresholds.v_threshold_far_external_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->luma_filter_detection_thresholds.v_threshold_far_external_mod_offset,
                pInput2Rgn->luma_filter_detection_thresholds.v_threshold_far_external_mod_offset,
                ratio);
            for (index = 0; index < Y_THR_PER_Y; index++)
            {
                pOutputRgn->chroma_filter_detection_thresholds.y_threshold_per_y_tab.y_threshold_per_y[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->chroma_filter_detection_thresholds.y_threshold_per_y_tab.y_threshold_per_y[index],
                    pInput2Rgn->chroma_filter_detection_thresholds.y_threshold_per_y_tab.y_threshold_per_y[index],
                    ratio);
            }
            for (index = 0; index < Y_THR_PER_UV; index++)
            {
                pOutputRgn->chroma_filter_detection_thresholds.y_threshold_per_uv_tab.y_threshold_per_uv[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->chroma_filter_detection_thresholds.y_threshold_per_uv_tab.y_threshold_per_uv[index],
                    pInput2Rgn->chroma_filter_detection_thresholds.y_threshold_per_uv_tab.y_threshold_per_uv[index],
                    ratio);
            }
            pOutputRgn->chroma_filter_detection_thresholds.y_threshold_top_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.y_threshold_top_limit,
                pInput2Rgn->chroma_filter_detection_thresholds.y_threshold_top_limit,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.y_threshold_bottom_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.y_threshold_bottom_limit,
                pInput2Rgn->chroma_filter_detection_thresholds.y_threshold_bottom_limit,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.y_threshold_far_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.y_threshold_far_mod_scale,
                pInput2Rgn->chroma_filter_detection_thresholds.y_threshold_far_mod_scale,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.y_threshold_far_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.y_threshold_far_mod_offset,
                pInput2Rgn->chroma_filter_detection_thresholds.y_threshold_far_mod_offset,
                ratio);
            for (index = 0; index < U_THR_PER_Y; index++)
            {
                pOutputRgn->chroma_filter_detection_thresholds.u_threshold_per_y_tab.u_threshold_per_y[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->chroma_filter_detection_thresholds.u_threshold_per_y_tab.u_threshold_per_y[index],
                    pInput2Rgn->chroma_filter_detection_thresholds.u_threshold_per_y_tab.u_threshold_per_y[index],
                    ratio);
            }
            for (index = 0; index < U_THR_PER_UV; index++)
            {
                pOutputRgn->chroma_filter_detection_thresholds.u_threshold_per_uv_tab.u_threshold_per_uv[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->chroma_filter_detection_thresholds.u_threshold_per_uv_tab.u_threshold_per_uv[index],
                    pInput2Rgn->chroma_filter_detection_thresholds.u_threshold_per_uv_tab.u_threshold_per_uv[index],
                    ratio);
            }
            pOutputRgn->chroma_filter_detection_thresholds.u_threshold_top_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.u_threshold_top_limit,
                pInput2Rgn->chroma_filter_detection_thresholds.u_threshold_top_limit,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.u_threshold_bottom_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.u_threshold_bottom_limit,
                pInput2Rgn->chroma_filter_detection_thresholds.u_threshold_bottom_limit,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.u_threshold_far_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.u_threshold_far_mod_scale,
                pInput2Rgn->chroma_filter_detection_thresholds.u_threshold_far_mod_scale,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.u_threshold_far_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.u_threshold_far_mod_offset,
                pInput2Rgn->chroma_filter_detection_thresholds.u_threshold_far_mod_offset,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.u_threshold_distant_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.u_threshold_distant_mod_scale,
                pInput2Rgn->chroma_filter_detection_thresholds.u_threshold_distant_mod_scale,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.u_threshold_distant_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.u_threshold_distant_mod_offset,
                pInput2Rgn->chroma_filter_detection_thresholds.u_threshold_distant_mod_offset,
                ratio);
            for (index = 0; index < V_THR_PER_Y; index++)
            {
                pOutputRgn->chroma_filter_detection_thresholds.v_threshold_per_y_tab.v_threshold_per_y[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->chroma_filter_detection_thresholds.v_threshold_per_y_tab.v_threshold_per_y[index],
                    pInput2Rgn->chroma_filter_detection_thresholds.v_threshold_per_y_tab.v_threshold_per_y[index],
                    ratio);
            }
            for (index = 0; index < V_THR_PER_UV; index++)
            {
                pOutputRgn->chroma_filter_detection_thresholds.v_threshold_per_uv_tab.v_threshold_per_uv[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->chroma_filter_detection_thresholds.v_threshold_per_uv_tab.v_threshold_per_uv[index],
                    pInput2Rgn->chroma_filter_detection_thresholds.v_threshold_per_uv_tab.v_threshold_per_uv[index],
                    ratio);
            }
            pOutputRgn->chroma_filter_detection_thresholds.v_threshold_top_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.v_threshold_top_limit,
                pInput2Rgn->chroma_filter_detection_thresholds.v_threshold_top_limit,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.v_threshold_bottom_limit
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.v_threshold_bottom_limit,
                pInput2Rgn->chroma_filter_detection_thresholds.v_threshold_bottom_limit,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.v_threshold_far_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.v_threshold_far_mod_scale,
                pInput2Rgn->chroma_filter_detection_thresholds.v_threshold_far_mod_scale,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.v_threshold_far_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.v_threshold_far_mod_offset,
                pInput2Rgn->chroma_filter_detection_thresholds.v_threshold_far_mod_offset,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.v_threshold_distant_mod_scale
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.v_threshold_distant_mod_scale,
                pInput2Rgn->chroma_filter_detection_thresholds.v_threshold_distant_mod_scale,
                ratio);
            pOutputRgn->chroma_filter_detection_thresholds.v_threshold_distant_mod_offset
                = IQSettingUtils::InterpolationFloatBilinear(
                pInput1Rgn->chroma_filter_detection_thresholds.v_threshold_distant_mod_offset,
                pInput2Rgn->chroma_filter_detection_thresholds.v_threshold_distant_mod_offset,
                ratio);
            for (index = 0; index < DCBLEN2_LUMA_STRENGTH_FUNCTION; index++)
            {
                pOutputRgn->dcblend2.dcblend2_luma_strength_function_tab.dcblend2_luma_strength_function[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->dcblend2.dcblend2_luma_strength_function_tab.dcblend2_luma_strength_function[index],
                    pInput2Rgn->dcblend2.dcblend2_luma_strength_function_tab.dcblend2_luma_strength_function[index],
                    ratio);
            }
            for (index = 0; index < DCBLEN2_CHROMA_STRENGTH_FUNCTION; index++)
            {
                pOutputRgn->dcblend2.dcblend2_chroma_strength_function_tab.dcblend2_chroma_strength_function[index]
                    = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1Rgn->dcblend2.dcblend2_chroma_strength_function_tab.dcblend2_chroma_strength_function[index],
                    pInput2Rgn->dcblend2.dcblend2_chroma_strength_function_tab.dcblend2_chroma_strength_function[index],
                    ratio);
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
// ANR10Interpolation::LensPositionSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ANR10Interpolation::LensPositionSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    anr_1_0_0::chromatix_anr10_coreType*   pParentDataType = NULL;
    ANR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<anr_1_0_0::chromatix_anr10_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_anr10_lens_posn_dataCount;
        pTriggerList    = static_cast<ANR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_anr10_lens_posn_data[count].lens_posn_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerLensPosition;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_anr10_lens_posn_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_anr10_lens_posn_data[regionOutput.endIndex]),
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
// ANR10Interpolation::LensZoomSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ANR10Interpolation::LensZoomSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    anr_1_0_0::mod_anr10_lens_posn_dataType*   pParentDataType = NULL;
    ANR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<anr_1_0_0::mod_anr10_lens_posn_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->lens_posn_data.mod_anr10_lens_zoom_dataCount;
        pTriggerList    = static_cast<ANR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->lens_posn_data.mod_anr10_lens_zoom_data[count].lens_zoom_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerLensZoom;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->lens_posn_data.mod_anr10_lens_zoom_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->lens_posn_data.mod_anr10_lens_zoom_data[regionOutput.endIndex]),
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
// ANR10Interpolation::PostScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ANR10Interpolation::PostScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    anr_1_0_0::mod_anr10_lens_zoom_dataType*   pParentDataType = NULL;
    ANR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<anr_1_0_0::mod_anr10_lens_zoom_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->lens_zoom_data.mod_anr10_post_scale_ratio_dataCount;
        pTriggerList    = static_cast<ANR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->lens_zoom_data.mod_anr10_post_scale_ratio_data[count].post_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerPostScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->lens_zoom_data.mod_anr10_post_scale_ratio_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->lens_zoom_data.mod_anr10_post_scale_ratio_data[regionOutput.endIndex]),
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
// ANR10Interpolation::PreScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ANR10Interpolation::PreScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    anr_1_0_0::mod_anr10_post_scale_ratio_dataType*   pParentDataType = NULL;
    ANR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<anr_1_0_0::mod_anr10_post_scale_ratio_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->post_scale_ratio_data.mod_anr10_pre_scale_ratio_dataCount;
        pTriggerList    = static_cast<ANR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->post_scale_ratio_data.mod_anr10_pre_scale_ratio_data[count].pre_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerPreScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->post_scale_ratio_data.mod_anr10_pre_scale_ratio_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>
                (&pParentDataType->post_scale_ratio_data.mod_anr10_pre_scale_ratio_data[regionOutput.endIndex]),
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
// ANR10Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ANR10Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    anr_1_0_0::mod_anr10_pre_scale_ratio_dataType*   pParentDataType = NULL;
    ANR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<anr_1_0_0::mod_anr10_pre_scale_ratio_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->pre_scale_ratio_data.mod_anr10_drc_gain_dataCount;
        pTriggerList    = static_cast<ANR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->pre_scale_ratio_data.mod_anr10_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->pre_scale_ratio_data.mod_anr10_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->pre_scale_ratio_data.mod_anr10_drc_gain_data[regionOutput.endIndex]),
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
// ANR10Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ANR10Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    anr_1_0_0::mod_anr10_drc_gain_dataType*   pParentDataType = NULL;
    ANR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<anr_1_0_0::mod_anr10_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_anr10_hdr_aec_dataCount;
        pTriggerList    = static_cast<ANR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_anr10_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_anr10_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_anr10_hdr_aec_data[regionOutput.endIndex]),
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
// ANR10Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ANR10Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    anr_1_0_0::mod_anr10_hdr_aec_dataType*   pParentDataType = NULL;
    ANR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<anr_1_0_0::mod_anr10_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_anr10_aec_dataCount;
        pTriggerList    = static_cast<ANR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_anr10_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_anr10_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_anr10_aec_data[regionOutput.endIndex]),
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
// ANR10Interpolation::CCTSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ANR10Interpolation::CCTSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    anr_1_0_0::mod_anr10_aec_dataType*   pParentDataType = NULL;
    ANR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<anr_1_0_0::mod_anr10_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->aec_data.mod_anr10_cct_dataCount;
        pTriggerList    = static_cast<ANR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->aec_data.mod_anr10_cct_data[count].cct_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerCCT;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->aec_data.mod_anr10_cct_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->aec_data.mod_anr10_cct_data[regionOutput.startIndex].cct_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->aec_data.mod_anr10_cct_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->aec_data.mod_anr10_cct_data[regionOutput.endIndex].cct_data));
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
