// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  asf30interpolation.cpp
/// @brief ASF30 Tuning Data Interpolation module
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "asf30interpolation.h"
#include "asf30setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation ASF30OperationTable[] =
{
    { ASF30Interpolation::TotalScaleRatioSearchNode, 2},
    { ASF30Interpolation::DRCGainSearchNode,         2},
    { ASF30Interpolation::HDRAECSearchNode,          2},
    { ASF30Interpolation::AECSearchNode,             2},
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ASF30Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    ASF30InputData*   pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                      ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->digitalGain, pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))             ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))                           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->totalScaleRatio, pInput->totalScaleRatio))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))              ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)))
    {
        pTriggerData->luxIndex             = pInput->AECLuxIndex;
        pTriggerData->digitalGain          = pInput->AECGain;
        pTriggerData->AECSensitivity       = pInput->AECSensitivity;
        pTriggerData->DRCGain              = pInput->DRCGain;
        pTriggerData->exposureTime         = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio    = pInput->AECexposureGainRatio;
        pTriggerData->totalScaleRatio      = pInput->totalScaleRatio;

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ASF30Interpolation::RunInterpolation(
    const ASF30InputData*          pInput,
    asf_3_0_0::asf30_rgn_dataType* pData)
{
    BOOL result = TRUE;

    // The interpolation tree total Node
    TuningNode nodeSet[ASF30MaxNode];

    // ASF Trigger List
    ASF30TriggerList asfTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < ASF30MaxNode; count++)
        {
            if (count < ASF30MaxNoLeafNode)
            {
                // If this is not a Leaf Node, Provide a buffer to hold the calculation data
                IQSettingUtils::InitializeNode(&nodeSet[count], &pData[count + 1]);
            }
            else
            {
                // For the leaf nodes, the data buffer will point to the chromatix buffer
                IQSettingUtils::InitializeNode(&nodeSet[count], NULL);
            }
        }

        // Set up first Node in the Tree
        IQSettingUtils::AddNodeToInterpolationTree(NULL, &nodeSet[0],
            static_cast<VOID*>(&pInput->pChromatix->chromatix_asf30_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&asfTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(asf_3_0_0::chromatix_asf30Type::control_methodStruct));

        asfTrigger.triggerTotalScaleRatio = pInput->totalScaleRatio;

        asfTrigger.triggerDRCgain         = pInput->DRCGain;

        asfTrigger.triggerHDRAEC          = IQSettingUtils::GettriggerHDRAEC(
                                                pInput->pChromatix->control_method.aec_hdr_control,
                                                pInput->exposureTime,
                                                pInput->AECSensitivity,
                                                pInput->exposureGainRatio);

        asfTrigger.triggerAEC             = IQSettingUtils::GettriggerAEC(
                                                pInput->pChromatix->control_method.aec_exp_control,
                                                pInput->luxIndex,
                                                pInput->digitalGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(
            &nodeSet[0], ASF30InterpolationLevel, &ASF30OperationTable[0], static_cast<VOID*>(&asfTrigger));
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
                                                       ASF30MaxNoLeafNode,
                                                       ASF30InterpolationLevel,
                                                       ASF30Interpolation::DoInterpolation);

        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(asf_3_0_0::asf30_rgn_dataType));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ASF30Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        asf_3_0_0::asf30_rgn_dataType* pInput1 =
            static_cast<asf_3_0_0::asf30_rgn_dataType*>(pData1);
        asf_3_0_0::asf30_rgn_dataType* pInput2 =
            static_cast<asf_3_0_0::asf30_rgn_dataType*>(pData2);
        asf_3_0_0::asf30_rgn_dataType* pOutput =
            static_cast<asf_3_0_0::asf30_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(asf_3_0_0::asf30_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = ASF30Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(asf_3_0_0::asf30_rgn_dataType));
            }
            else if (IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(asf_3_0_0::asf30_rgn_dataType));
            }
            else
            {
                /// @todo (CAMX-1812) Need to add logging for Common library
                result = FALSE;
            }
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASF30Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ASF30Interpolation::InterpolationData(
    asf_3_0_0::asf30_rgn_dataType* pInput1,
    asf_3_0_0::asf30_rgn_dataType* pInput2,
    FLOAT                          ratio,
    asf_3_0_0::asf30_rgn_dataType* pOutput)
{
    UINT  i;
    FLOAT result    = 0.0f;
    BOOL  resultAPI = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        pOutput->face_boundary                       = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->face_boundary, pInput2->face_boundary, ratio);

        pOutput->face_transition                     = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->face_transition, pInput2->face_transition, ratio);

        pOutput->flat_thresold                       = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->flat_thresold, pInput2->flat_thresold, ratio);

        pOutput->layer_1_activity_clamp_threshold    = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_1_activity_clamp_threshold, pInput2->layer_1_activity_clamp_threshold, ratio);

        pOutput->layer_1_clamp_ll                    = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_1_clamp_ll, pInput2->layer_1_clamp_ll, ratio);

        pOutput->layer_1_clamp_ul                    = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_1_clamp_ul, pInput2->layer_1_clamp_ul, ratio);

        pOutput->layer_1_gain_cap                    = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_1_gain_cap, pInput2->layer_1_gain_cap, ratio);

        pOutput->layer_1_gamma_corrected_luma_target = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_1_gamma_corrected_luma_target, pInput2->layer_1_gamma_corrected_luma_target, ratio);

        pOutput->layer_1_l2_norm_en = pInput1->layer_1_l2_norm_en;

        pOutput->layer_1_median_blend_lower_offset   = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_1_median_blend_lower_offset, pInput2->layer_1_median_blend_lower_offset, ratio);

        pOutput->layer_1_median_blend_upper_offset   = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_1_median_blend_upper_offset, pInput2->layer_1_median_blend_upper_offset, ratio);

        pOutput->layer_1_norm_scale                  = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_1_norm_scale, pInput2->layer_1_norm_scale, ratio);

        pOutput->layer_1_sp                          = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_1_sp, pInput2->layer_1_sp, ratio);

        pOutput->layer_2_activity_clamp_threshold    = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_2_activity_clamp_threshold, pInput2->layer_2_activity_clamp_threshold, ratio);

        pOutput->layer_2_clamp_ll                    = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_2_clamp_ll, pInput2->layer_2_clamp_ll, ratio);

        pOutput->layer_2_clamp_ul                    = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_2_clamp_ul, pInput2->layer_2_clamp_ul, ratio);


        pOutput->layer_2_l2_norm_en = pInput1->layer_2_l2_norm_en;

        pOutput->layer_2_norm_scale                  = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->layer_2_norm_scale, pInput2->layer_2_norm_scale, ratio);

        pOutput->corner_threshold                    = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->corner_threshold, pInput2->corner_threshold, ratio);

        pOutput->skin_boundary_probability           = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->skin_boundary_probability, pInput2->skin_boundary_probability, ratio);

        pOutput->skin_hue_max                        = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->skin_hue_max, pInput2->skin_hue_max, ratio);

        pOutput->skin_hue_min                        = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->skin_hue_min, pInput2->skin_hue_min, ratio);

        pOutput->skin_nonskin_to_skin_qratio         = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->skin_nonskin_to_skin_qratio, pInput2->skin_nonskin_to_skin_qratio, ratio);

        pOutput->skin_percent                        = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->skin_percent, pInput2->skin_percent, ratio);

        pOutput->skin_saturation_max_ymax            = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->skin_saturation_max_ymax, pInput2->skin_saturation_max_ymax, ratio);

        pOutput->skin_saturation_max_ymin            = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->skin_saturation_max_ymin, pInput2->skin_saturation_max_ymin, ratio);

        pOutput->skin_saturation_min_ymax            = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->skin_saturation_min_ymax, pInput2->skin_saturation_min_ymax, ratio);

        pOutput->skin_saturation_min_ymin            = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->skin_saturation_min_ymin, pInput2->skin_saturation_min_ymin, ratio);

        pOutput->skin_y_max                          = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->skin_y_max, pInput2->skin_y_max, ratio);

        pOutput->skin_y_min                          = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->skin_y_min, pInput2->skin_y_min, ratio);

        pOutput->smoothing_strength                  = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->smoothing_strength, pInput2->smoothing_strength, ratio);

        pOutput->max_smoothing_clamp                 = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->max_smoothing_clamp, pInput2->max_smoothing_clamp, ratio);

        for (i = 0; i < LUT_GAINCHROMA_SIZE; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->gain_chroma_negative_tab.gain_chroma_negative[i],
                pInput2->gain_chroma_negative_tab.gain_chroma_negative[i],
                ratio);
            pOutput->gain_chroma_negative_tab.gain_chroma_negative[i] = result;

            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->gain_chroma_positive_tab.gain_chroma_positive[i],
                pInput2->gain_chroma_positive_tab.gain_chroma_positive[i],
                ratio);
            pOutput->gain_chroma_positive_tab.gain_chroma_positive[i] = result;
        }

        for (i = 0; i < LUT_GAINCONTRAST_SIZE; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->gain_contrast_negative_tab.gain_contrast_negative[i],
                pInput2->gain_contrast_negative_tab.gain_contrast_negative[i],
                ratio);
            pOutput->gain_contrast_negative_tab.gain_contrast_negative[i] = result;

            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->gain_contrast_positive_tab.gain_contrast_positive[i],
                pInput2->gain_contrast_positive_tab.gain_contrast_positive[i],
                ratio);
            pOutput->gain_contrast_positive_tab.gain_contrast_positive[i] = result;
        }

        for (i = 0; i < LUT_ACTIVITY_BPF_SIZE; i++)
        {
            pOutput->layer_1_activity_band_pass_coeff_tab.layer_1_activity_band_pass_coeff[i] =
                pInput1->layer_1_activity_band_pass_coeff_tab.layer_1_activity_band_pass_coeff[i];

            pOutput->layer_2_activity_band_pass_coeff_tab.layer_2_activity_band_pass_coeff[i] =
                pInput1->layer_2_activity_band_pass_coeff_tab.layer_2_activity_band_pass_coeff[i];
        }

        for (i = 0; i < LUT_ACTIVITY_NORM_SIZE; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->layer_1_activity_normalization_lut_tab.layer_1_activity_normalization_lut[i],
                pInput2->layer_1_activity_normalization_lut_tab.layer_1_activity_normalization_lut[i],
                ratio);
            pOutput->layer_1_activity_normalization_lut_tab.layer_1_activity_normalization_lut[i] = result;

            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->layer_2_activity_normalization_lut_tab.layer_2_activity_normalization_lut[i],
                pInput2->layer_2_activity_normalization_lut_tab.layer_2_activity_normalization_lut[i],
                ratio);
            pOutput->layer_2_activity_normalization_lut_tab.layer_2_activity_normalization_lut[i] = result;
        }

        for (i = 0; i < LUT_GAINPOSNEG_SIZE; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->layer_1_gain_negative_lut_tab.layer_1_gain_negative_lut[i],
                pInput2->layer_1_gain_negative_lut_tab.layer_1_gain_negative_lut[i],
                ratio);
            pOutput->layer_1_gain_negative_lut_tab.layer_1_gain_negative_lut[i] = result;

            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->layer_1_gain_positive_lut_tab.layer_1_gain_positive_lut[i],
                pInput2->layer_1_gain_positive_lut_tab.layer_1_gain_positive_lut[i],
                ratio);
            pOutput->layer_1_gain_positive_lut_tab.layer_1_gain_positive_lut[i] = result;

            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->layer_2_gain_negative_lut_tab.layer_2_gain_negative_lut[i],
                pInput2->layer_2_gain_negative_lut_tab.layer_2_gain_negative_lut[i],
                ratio);
            pOutput->layer_2_gain_negative_lut_tab.layer_2_gain_negative_lut[i] = result;

            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->layer_2_gain_positive_lut_tab.layer_2_gain_positive_lut[i],
                pInput2->layer_2_gain_positive_lut_tab.layer_2_gain_positive_lut[i],
                ratio);
            pOutput->layer_2_gain_positive_lut_tab.layer_2_gain_positive_lut[i] = result;
        }

        for (i = 0; i < LUT_GAINWEIGHT_SIZE; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->layer_1_gain_weight_lut_tab.layer_1_gain_weight_lut[i],
                pInput2->layer_1_gain_weight_lut_tab.layer_1_gain_weight_lut[i],
                ratio);
            pOutput->layer_1_gain_weight_lut_tab.layer_1_gain_weight_lut[i] = result;

            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->layer_2_gain_weight_lut_tab.layer_2_gain_weight_lut[i],
                pInput2->layer_2_gain_weight_lut_tab.layer_2_gain_weight_lut[i],
                ratio);
            pOutput->layer_2_gain_weight_lut_tab.layer_2_gain_weight_lut[i] = result;
        }

        for (i = 0; i < LUT_SOFT_THRESHOLD_SIZE; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->layer_1_soft_threshold_lut_tab.layer_1_soft_threshold_lut[i],
                pInput2->layer_1_soft_threshold_lut_tab.layer_1_soft_threshold_lut[i],
                ratio);
            pOutput->layer_1_soft_threshold_lut_tab.layer_1_soft_threshold_lut[i] = result;

            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->layer_2_soft_threshold_lut_tab.layer_2_soft_threshold_lut[i],
                pInput2->layer_2_soft_threshold_lut_tab.layer_2_soft_threshold_lut[i],
                ratio);
            pOutput->layer_2_soft_threshold_lut_tab.layer_2_soft_threshold_lut[i] = result;
        }

        for (i = 0; i < LUT_WEIGHT_MOD_SIZE; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(
                    pInput1->layer_1_weight_modulation_lut_tab.layer_1_weight_modulation_lut[i],
                    pInput2->layer_1_weight_modulation_lut_tab.layer_1_weight_modulation_lut[i],
                    ratio);
            pOutput->layer_1_weight_modulation_lut_tab.layer_1_weight_modulation_lut[i]           = result;

            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->layer_2_weight_modulation_lut_tab.layer_2_weight_modulation_lut[i],
                pInput2->layer_2_weight_modulation_lut_tab.layer_2_weight_modulation_lut[i],
                ratio);
            pOutput->layer_2_weight_modulation_lut_tab.layer_2_weight_modulation_lut[i]           = result;
        }

        for (i = 0; i < LUT_LAYER1_FILTER_SIZE; i++)
        {
            pOutput->layer_1_hpf_symmetric_coeff_tab.layer_1_hpf_symmetric_coeff[i] =
                pInput1->layer_1_hpf_symmetric_coeff_tab.layer_1_hpf_symmetric_coeff[i];

            pOutput->layer_1_lpf_symmetric_coeff_tab.layer_1_lpf_symmetric_coeff[i] =
                pInput1->layer_1_lpf_symmetric_coeff_tab.layer_1_lpf_symmetric_coeff[i];
        }

        for (i = 0; i < LUT_LAYER2_FILTER_SIZE; i++)
        {
            pOutput->layer_2_hpf_symmetric_coeff_tab.layer_2_hpf_symmetric_coeff[i] =
                pInput1->layer_2_hpf_symmetric_coeff_tab.layer_2_hpf_symmetric_coeff[i];

            pOutput->layer_2_lpf_symmetric_coeff_tab.layer_2_lpf_symmetric_coeff[i] =
                pInput1->layer_2_lpf_symmetric_coeff_tab.layer_2_lpf_symmetric_coeff[i];
        }

        for (i = 0; i < NUM_ASF_RADIAL_ENTRIES; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->radial_activity_adj_tab.radial_activity_adj[i],
                pInput2->radial_activity_adj_tab.radial_activity_adj[i],
                ratio);
            pOutput->radial_activity_adj_tab.radial_activity_adj[i] = result;

            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->radial_gain_adj_tab.radial_gain_adj[i],
                pInput2->radial_gain_adj_tab.radial_gain_adj[i],
                ratio);
            pOutput->radial_gain_adj_tab.radial_gain_adj[i]         = result;
        }

        for (i = 0; i < LUT_SKIN_SIZE; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->skin_activity_tab.skin_activity[i],
                pInput2->skin_activity_tab.skin_activity[i],
                ratio);
            pOutput->skin_activity_tab.skin_activity[i] = result;

            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->skin_gain_tab.skin_gain[i],
                pInput2->skin_gain_tab.skin_gain[i],
                ratio);
            pOutput->skin_gain_tab.skin_gain[i]         = result;
        }
    }
    else
    {
        resultAPI = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return resultAPI;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ASF30Interpolation::TotalScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ASF30Interpolation::TotalScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    asf_3_0_0::chromatix_asf30_coreType*   pParentDataType = NULL;
    ASF30TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<asf_3_0_0::chromatix_asf30_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_asf30_total_scale_ratio_dataCount;
        pTriggerList    = static_cast<ASF30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_asf30_total_scale_ratio_data[count].total_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerTotalScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_asf30_total_scale_ratio_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_asf30_total_scale_ratio_data[regionOutput.endIndex]),
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
// ASF30Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ASF30Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    asf_3_0_0::mod_asf30_total_scale_ratio_dataType*   pParentDataType = NULL;
    ASF30TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<asf_3_0_0::mod_asf30_total_scale_ratio_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->total_scalor_ratio_data.mod_asf30_drc_gain_dataCount;
        pTriggerList    = static_cast<ASF30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->total_scalor_ratio_data.mod_asf30_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->total_scalor_ratio_data.mod_asf30_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->total_scalor_ratio_data.mod_asf30_drc_gain_data[regionOutput.endIndex]),
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
// ASF30Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ASF30Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    asf_3_0_0::mod_asf30_drc_gain_dataType*   pParentDataType = NULL;
    ASF30TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<asf_3_0_0::mod_asf30_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_asf30_hdr_aec_dataCount;
        pTriggerList    = static_cast<ASF30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_asf30_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_asf30_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_asf30_hdr_aec_data[regionOutput.endIndex]),
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
// ASF30Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ASF30Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    asf_3_0_0::mod_asf30_hdr_aec_dataType*   pParentDataType = NULL;
    ASF30TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<asf_3_0_0::mod_asf30_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_asf30_aec_dataCount;
        pTriggerList    = static_cast<ASF30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_asf30_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_asf30_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_asf30_aec_data[regionOutput.startIndex].asf30_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_asf30_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_asf30_aec_data[regionOutput.endIndex].asf30_rgn_data));
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
