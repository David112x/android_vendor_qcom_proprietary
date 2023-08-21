// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  tmc12interpolation.cpp
/// @brief TMC12 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "tmc12interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation TMC12OperationTable[] =
{
    {TMC12Interpolation::DRCGainSearchNode, 2},
    {TMC12Interpolation::HDRAECSearchNode,  2},
    {TMC12Interpolation::AECSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC12Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC12Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData*   pInput,
    TMC12InputData*     pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))                        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGainDark, pInput->DRCGainDark))                ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)))
    {
        pTriggerData->luxIndex                     = pInput->AECLuxIndex;
        pTriggerData->realGain                     = pInput->AECGain;
        pTriggerData->DRCGain                      = pInput->DRCGain;
        pTriggerData->DRCGainDark                  = pInput->DRCGainDark;
        pTriggerData->AECSensitivity               = pInput->AECSensitivity;
        pTriggerData->exposureTime                 = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio            = pInput->AECexposureGainRatio;
        pTriggerData->prevluxIndex                 = pInput->AECPrevLuxIndex;

        pTriggerData->pPreviousCalculatedHistogram = pInput->pPreviousCalculatedHistogram;
        pTriggerData->pPreviousCalculatedCDF       = pInput->pPreviousCalculatedCDF;

        pTriggerData->overrideDarkBoostOffset      = pInput->overrideDarkBoostOffset;
        pTriggerData->overrideFourthToneAnchor     = pInput->overrideFourthToneAnchor;

        if (NULL != pInput->pParsedBHISTStats)
        {
            pTriggerData->pGRHist = pInput->pParsedBHISTStats->BHistogramStats;
        }

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC12Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC12Interpolation::RunInterpolation(
    const TMC12InputData*          pInput,
    tmc_1_2_0::tmc12_rgn_dataType* pData)
{
    BOOL                result = TRUE;
    TuningNode          nodeSet[TMC12MaximumNode];      // The interpolation tree total Node
    TMC12TriggerList    tmcTrigger;                     // TMC12 trigger list

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < TMC12MaximumNode; count++)
        {
            if (count < TMC12MaximumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_tmc12_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&tmcTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(tmc_1_2_0::chromatix_tmc12Type::control_methodStruct));

        tmcTrigger.triggerDRCgain = pInput->DRCGain;

        tmcTrigger.triggerHDRAEC  =
            IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                             pInput->exposureTime,
                                             pInput->AECSensitivity,
                                             pInput->exposureGainRatio);

        tmcTrigger.triggerAEC     =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        TMC12InterpolationLevel,
                                                        TMC12OperationTable,
                                                        static_cast<VOID*>(&tmcTrigger));
        if (FALSE != result)
        {
            // Calculate the Interpolation Result
            result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                           TMC12MaximumNonLeafNode,
                                                           TMC12InterpolationLevel,
                                                           TMC12Interpolation::DoInterpolation);
            if (FALSE != result)
            {
                IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(tmc_1_2_0::tmc12_rgn_dataType));

                // Calculate ADRC Gain Curve
                CalculateGainCurve(pInput, pData);
            }
        }
    }
    else
    {
        /// @todo (CAMX-1460) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC12Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC12Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    tmc_1_2_0::tmc12_rgn_dataType* pInput1 = NULL;
    tmc_1_2_0::tmc12_rgn_dataType* pInput2 = NULL;
    tmc_1_2_0::tmc12_rgn_dataType* pOutput = NULL;
    BOOL                           result  = TRUE;

    if ((NULL != pData1) &&
        (NULL != pData2) &&
        (NULL != pResult))
    {
        pInput1 = static_cast<tmc_1_2_0::tmc12_rgn_dataType*>(pData1);
        pInput2 = static_cast<tmc_1_2_0::tmc12_rgn_dataType*>(pData2);
        pOutput = static_cast<tmc_1_2_0::tmc12_rgn_dataType*>(pResult);


        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(tmc_1_2_0::tmc12_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = TMC12Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(tmc_1_2_0::tmc12_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(tmc_1_2_0::tmc12_rgn_dataType));
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
/// TMC12Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC12Interpolation::InterpolationData(
    tmc_1_2_0::tmc12_rgn_dataType* pInput1,
    tmc_1_2_0::tmc12_rgn_dataType* pInput2,
    FLOAT                          ratio,
    tmc_1_2_0::tmc12_rgn_dataType* pOutput)
{
    FLOAT result    = 0.0f;
    BOOL  resultAPI = TRUE;

    if ((NULL != pInput1) &&
        (NULL != pInput2) &&
        (NULL != pOutput))
    {
        pOutput->gtm_percentage         = IQSettingUtils::InterpolationFloatBilinear(pInput1->gtm_percentage,
                                                                                     pInput2->gtm_percentage,
                                                                                     ratio);

        pOutput->ltm_percentage         = IQSettingUtils::InterpolationFloatBilinear(pInput1->ltm_percentage,
                                                                                     pInput2->ltm_percentage,
                                                                                     ratio);

        pOutput->dark_boost_ratio       = IQSettingUtils::InterpolationFloatBilinear(pInput1->dark_boost_ratio,
                                                                                     pInput2->dark_boost_ratio,
                                                                                     ratio);

        pOutput->dark_boost_offset      = IQSettingUtils::InterpolationFloatBilinear(pInput1->dark_boost_offset,
                                                                                     pInput2->dark_boost_offset,
                                                                                     ratio);

        pOutput->tone_bright_adj        = IQSettingUtils::InterpolationFloatBilinear(pInput1->tone_bright_adj,
                                                                                     pInput2->tone_bright_adj,
                                                                                     ratio);

        pOutput->tone_dark_adj          = IQSettingUtils::InterpolationFloatBilinear(pInput1->tone_dark_adj,
                                                                                     pInput2->tone_dark_adj,
                                                                                     ratio);

        pOutput->stretch_bright_str     = IQSettingUtils::InterpolationFloatBilinear(pInput1->stretch_bright_str,
                                                                                     pInput2->stretch_bright_str,
                                                                                     ratio);

        pOutput->stretch_dark_str       = IQSettingUtils::InterpolationFloatBilinear(pInput1->stretch_dark_str,
                                                                                     pInput2->stretch_dark_str,
                                                                                     ratio);

        pOutput->hist_supr_range_start  = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_supr_range_start,
                                                                                     pInput2->hist_supr_range_start,
                                                                                     ratio);

        pOutput->hist_supr_range_end    = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_supr_range_end,
                                                                                     pInput2->hist_supr_range_end,
                                                                                     ratio);

        pOutput->hist_boost_range_start = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_boost_range_start,
                                                                                     pInput2->hist_boost_range_start,
                                                                                     ratio);

        pOutput->hist_boost_range_end   = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_boost_range_end,
                                                                                     pInput2->hist_boost_range_end,
                                                                                     ratio);

        pOutput->hist_avg_range_start   = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_avg_range_start,
                                                                                     pInput2->hist_avg_range_start,
                                                                                     ratio);

        pOutput->hist_avg_range_end     = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_avg_range_end,
                                                                                     pInput2->hist_avg_range_end,
                                                                                     ratio);

        pOutput->hist_clip_slope        = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_clip_slope,
                                                                                     pInput2->hist_clip_slope,
                                                                                     ratio);

        pOutput->hist_enhance_clamp     = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_enhance_clamp,
                                                                                     pInput2->hist_enhance_clamp,
                                                                                     ratio);

        pOutput->contrast_bright_clip   = IQSettingUtils::InterpolationFloatBilinear(pInput1->contrast_bright_clip,
                                                                                     pInput2->contrast_bright_clip,
                                                                                     ratio);

        pOutput->contrast_dark_adj      = IQSettingUtils::InterpolationFloatBilinear(pInput1->contrast_dark_adj,
                                                                                     pInput2->contrast_dark_adj,
                                                                                     ratio);

        pOutput->contrast_he_bright     = IQSettingUtils::InterpolationFloatBilinear(pInput1->contrast_he_bright,
                                                                                     pInput2->contrast_he_bright,
                                                                                     ratio);

        pOutput->contrast_he_dark       = IQSettingUtils::InterpolationFloatBilinear(pInput1->contrast_he_dark,
                                                                                     pInput2->contrast_he_dark,
                                                                                     ratio);

        pOutput->hist_smoothing_str     = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_smoothing_str,
                                                                                     pInput2->hist_smoothing_str,
                                                                                     ratio);

        pOutput->hist_curve_smoothing_str           = IQSettingUtils::InterpolationFloatBilinear(
                                                                        pInput1->hist_curve_smoothing_str,
                                                                        pInput2->hist_curve_smoothing_str,
                                                                        ratio);

        pOutput->scene_change_smoothing_str         = IQSettingUtils::InterpolationFloatBilinear(
                                                                        pInput1->scene_change_smoothing_str,
                                                                        pInput2->scene_change_smoothing_str,
                                                                        ratio);

        pOutput->scene_change_curve_smoothing_str   = IQSettingUtils::InterpolationFloatBilinear(
                                                                        pInput1->scene_change_curve_smoothing_str,
                                                                        pInput2->scene_change_curve_smoothing_str,
                                                                        ratio);

        pOutput->scene_change_hist_delta_th1        = IQSettingUtils::InterpolationFloatBilinear(
                                                                        pInput1->scene_change_hist_delta_th1,
                                                                        pInput2->scene_change_hist_delta_th1,
                                                                        ratio);

        pOutput->scene_change_hist_delta_th2        = IQSettingUtils::InterpolationFloatBilinear(
                                                                        pInput1->scene_change_hist_delta_th2,
                                                                        pInput2->scene_change_hist_delta_th2,
                                                                        ratio);

        pOutput->scene_change_lux_idx_delta_th1     = IQSettingUtils::InterpolationFloatBilinear(
                                                                        pInput1->scene_change_lux_idx_delta_th1,
                                                                        pInput2->scene_change_lux_idx_delta_th1,
                                                                        ratio);

        pOutput->scene_change_lux_idx_delta_th2     = IQSettingUtils::InterpolationFloatBilinear(
                                                                        pInput1->scene_change_lux_idx_delta_th2,
                                                                        pInput2->scene_change_lux_idx_delta_th2,
                                                                        ratio);

        pOutput->core_rsv_para1         = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para1,
                                                                                     pInput2->core_rsv_para1,
                                                                                     ratio);

        pOutput->core_rsv_para2         = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para2,
                                                                                     pInput2->core_rsv_para2,
                                                                                     ratio);

        pOutput->core_rsv_para3         = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para3,
                                                                                     pInput2->core_rsv_para3,
                                                                                     ratio);

        pOutput->core_rsv_para4         = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para4,
                                                                                     pInput2->core_rsv_para4,
                                                                                     ratio);

        pOutput->core_rsv_para5         = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para5,
                                                                                     pInput2->core_rsv_para5,
                                                                                     ratio);

        for (UINT32 i = 0; i < TMC12_KNEE_POINTS_NUM; i++)
        {
            pOutput->tone_target_tab.tone_target[i]                 = IQSettingUtils::InterpolationFloatBilinear(
                                                                                    pInput1->tone_target_tab.tone_target[i],
                                                                                    pInput2->tone_target_tab.tone_target[i],
                                                                                    ratio);
            pOutput->tone_anchors_tab.tone_anchors[i]               = IQSettingUtils::InterpolationFloatBilinear(
                                                                                    pInput1->tone_anchors_tab.tone_anchors[i],
                                                                                    pInput2->tone_anchors_tab.tone_anchors[i],
                                                                                    ratio);
            pOutput->hist_conv_kernel_tab.hist_conv_kernel[i]       = IQSettingUtils::InterpolationFloatBilinear(
                                                                        pInput1->hist_conv_kernel_tab.hist_conv_kernel[i],
                                                                        pInput2->hist_conv_kernel_tab.hist_conv_kernel[i],
                                                                        ratio);
            pOutput->hist_enhance_ratio_tab.hist_enhance_ratio[i]   = IQSettingUtils::InterpolationFloatBilinear(
                                                                        pInput1->hist_enhance_ratio_tab.hist_enhance_ratio[i],
                                                                        pInput2->hist_enhance_ratio_tab.hist_enhance_ratio[i],
                                                                        ratio);
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
// TMC12Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TMC12Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tmc_1_2_0::chromatix_tmc12_coreType*   pParentDataType = NULL;
    TMC12TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tmc_1_2_0::chromatix_tmc12_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_tmc12_drc_gain_dataCount;
        pTriggerList    = static_cast<TMC12TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_tmc12_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_tmc12_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_tmc12_drc_gain_data[regionOutput.endIndex]),
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
// TMC12Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TMC12Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tmc_1_2_0::mod_tmc12_drc_gain_dataType*   pParentDataType = NULL;
    TMC12TriggerList*                         pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tmc_1_2_0::mod_tmc12_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_tmc12_hdr_aec_dataCount;
        pTriggerList    = static_cast<TMC12TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_tmc12_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_tmc12_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_tmc12_hdr_aec_data[regionOutput.endIndex]),
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
// TMC12Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TMC12Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tmc_1_2_0::mod_tmc12_hdr_aec_dataType*   pParentDataType = NULL;
    TMC12TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tmc_1_2_0::mod_tmc12_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_tmc12_aec_dataCount;
        pTriggerList    = static_cast<TMC12TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_tmc12_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_tmc12_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>
              (&pParentDataType->hdr_aec_data.mod_tmc12_aec_data[regionOutput.startIndex].tmc12_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_tmc12_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>
                    (&pParentDataType->hdr_aec_data.mod_tmc12_aec_data[regionOutput.endIndex].tmc12_rgn_data));
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
/// TMC12Interpolation::CalculateGainCurve
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC12Interpolation::CalculateGainCurve(
    const TMC12InputData*          pInput,
    tmc_1_2_0::tmc12_rgn_dataType* pTmcData)
{
    TMC12PerFrameInfo   tmc12PerFrameInfo   = { 0 };
    ADRCData*           pAdrcOutputData     = pInput->pAdrcOutputData;

    tmc12PerFrameInfo.pKneeX                = pAdrcOutputData->kneePoints.KneePointsTMC12.kneeX;
    tmc12PerFrameInfo.pKneeY                = pAdrcOutputData->kneePoints.KneePointsTMC12.kneeY;
    tmc12PerFrameInfo.pPchipCoeffficient    = pAdrcOutputData->pchipCoeffficient;
    tmc12PerFrameInfo.pContrastEnhanceCurve = pAdrcOutputData->contrastEnhanceCurve;
    tmc12PerFrameInfo.pProcessedHistogram   = pAdrcOutputData->currentCalculatedHistogram;
    tmc12PerFrameInfo.pCalculatedCDF        = pAdrcOutputData->currentCalculatedCDF;

    if (NULL != pInput->pGRHist)
    {
        HistogramPreProcess(pTmcData, pInput, &tmc12PerFrameInfo);
    }
    else
    {
        // Set zeros to the blending parameters when pGRHist is NULL
        SetZerosToTMCData(pTmcData);
    }

    CalculateKneePoints(pTmcData, pInput, &tmc12PerFrameInfo);

    if (NULL != pInput->pGRHist)
    {
        CalculateContrastEnhanceCurve(pTmcData, pInput, &tmc12PerFrameInfo);
    }

    pAdrcOutputData->enable           = pInput->pChromatix->enable_section.tmc_enable;
    pAdrcOutputData->version          = SWTMCVersion::TMC12;
    pAdrcOutputData->gtmEnable        = pInput->pChromatix->chromatix_tmc12_reserve.use_gtm;
    pAdrcOutputData->ltmEnable        = pInput->pChromatix->chromatix_tmc12_reserve.use_ltm;
    pAdrcOutputData->ltmPercentage    = pTmcData->ltm_percentage;
    pAdrcOutputData->gtmPercentage    = pTmcData->gtm_percentage;

    pAdrcOutputData->drcGainDark      = tmc12PerFrameInfo.drcGainDark;
    pAdrcOutputData->curveModel       = pInput->pChromatix->chromatix_tmc12_reserve.curve_model;
    pAdrcOutputData->contrastHEBright = pTmcData->contrast_he_bright;
    pAdrcOutputData->contrastHEDark   = pTmcData->contrast_he_dark;

    //  Post the previous calculated output to meta for the offline snapshot request
    IQSettingUtils::Memcpy(pAdrcOutputData->previousCalculatedHistogram,
                           pInput->pPreviousCalculatedHistogram,
                           sizeof(FLOAT) * MAX_ADRC_HIST_BIN_NUM);
    IQSettingUtils::Memcpy(pAdrcOutputData->previousCalculatedCDF,
                           pInput->pPreviousCalculatedCDF,
                           sizeof(FLOAT) * MAX_ADRC_HIST_BIN_NUM);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC12Interpolation::HistogramPreProcess
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC12Interpolation::HistogramPreProcess(
    tmc_1_2_0::tmc12_rgn_dataType* pTmcData,
    const TMC12InputData*          pInput,
    TMC12PerFrameInfo*             pTmc12PerFrameInfo)
{
    FLOAT* pTmcHist = pTmc12PerFrameInfo->pProcessedHistogram;

    FLOAT bayer_stat_buf[TMC12_HIST_BIN_NUM];
    IQSettingUtils::Memcpy(bayer_stat_buf, pInput->pGRHist, TMC12_HIST_BIN_NUM * sizeof(UINT32));

    // get histogram total count
    FLOAT n_hist = 0.0f;
    for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
    {
        //  should need double check for that
        bayer_stat_buf[i] = static_cast<FLOAT>(pInput->pGRHist[i]);
        n_hist += bayer_stat_buf[i];
    }

    // refine histogram to limit contrast
    FLOAT hist_max = pTmcData->hist_clip_slope * n_hist / TMC12_HIST_BIN_NUM;
    FLOAT hist_clipped = 0.0f;

    for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
    {
        if (bayer_stat_buf[i] < hist_max)
        {
            pTmcHist[i] = bayer_stat_buf[i];
        }
        else
        {
            pTmcHist[i] = hist_max;
            hist_clipped += (bayer_stat_buf[i] - hist_max);
        }
    }

    // remove under-saturated pixels
    if (pTmcHist[0] > pTmcHist[1])
    {
        hist_clipped += pTmcHist[0] - pTmcHist[1];
        pTmcHist[0] = pTmcHist[1];
    }

    // normalize bins
    FLOAT avgHistClipped = hist_clipped / TMC12_HIST_BIN_NUM;
    for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
    {
        pTmcHist[i] += avgHistClipped;
    }

    UINT32 bin_index_l;
    UINT32 bin_index_r;
    // smooth bins
    for (UINT32 j = 0; j < 4; j++)
    {
        for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
        {
            pTmcHist[i] = pTmcHist[i] * pTmcData->hist_conv_kernel_tab.hist_conv_kernel[0];
            for (UINT32 k = 1; k < 5; k++)
            {
                bin_index_r = IQSettingUtils::ClampUINT32(i + k, 0, TMC12_HIST_BIN_NUM - 1);
                bin_index_l = (i < k) ? 0 : IQSettingUtils::ClampUINT32(i - k, 0, TMC12_HIST_BIN_NUM - 1);
                pTmcHist[i] += pTmcHist[bin_index_r] * pTmcData->hist_conv_kernel_tab.hist_conv_kernel[k];
                pTmcHist[i] += pTmcHist[bin_index_l] * pTmcData->hist_conv_kernel_tab.hist_conv_kernel[k];
            }
        }
    }

    // get histogram total count after hist refinement
    n_hist = 0.0f;
    for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
    {
        n_hist += pTmcHist[i];
    }

    pTmc12PerFrameInfo->numOfHistogram = n_hist;

    // temporal processing
    FLOAT   sceneChangeFlag       = 0.0f;
    FLOAT   luxIdxDelta           = 0.0f;
    FLOAT   histDelta             = 0.0f;
    FLOAT   sceneChangeFlagLuxIdx = 0.0f;
    FLOAT   sceneChangeFlagHist   = 0.0f;
    FLOAT*  pTmcPreHist;

    // scene change condition1: lux idx delta
    luxIdxDelta = IQSettingUtils::AbsoluteFLOAT(pInput->luxIndex - pInput->prevluxIndex);

    // printf("\n_____TMC12___lux_idx_delta = %f\n", luxIdxDelta);

    if (luxIdxDelta >= pTmcData->scene_change_lux_idx_delta_th2)
    {
        sceneChangeFlagLuxIdx = 1;
    }
    else if ((luxIdxDelta >= pTmcData->scene_change_lux_idx_delta_th1) &&
             (luxIdxDelta <  pTmcData->scene_change_lux_idx_delta_th2))
    {
        sceneChangeFlagLuxIdx = (luxIdxDelta - pTmcData->scene_change_lux_idx_delta_th1) /
                                    (pTmcData->scene_change_lux_idx_delta_th2 - pTmcData->scene_change_lux_idx_delta_th1);
    }
    else
    {
        sceneChangeFlagLuxIdx = 0;
    }

    // the frame number should be replaced by define
    if ((pInput->frameNumber < 2) || (NULL == pInput->pPreviousCalculatedHistogram))
    {
        pTmcPreHist = pTmcHist;
    }
    else
    {
        pTmcPreHist = pInput->pPreviousCalculatedHistogram;
    }

    for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
    {
        histDelta += IQSettingUtils::AbsoluteFLOAT(pTmcPreHist[i] - pTmcHist[i]);
    }
    histDelta /= TMC12_HIST_BIN_NUM;

    // printf("\n_____TMC12___hist_delta = %f\n", histDelta);

    if (histDelta >= pTmcData->scene_change_hist_delta_th2)
    {
        sceneChangeFlagHist = 1;
    }
    else if ((histDelta >= pTmcData->scene_change_hist_delta_th1) &&
             (histDelta <  pTmcData->scene_change_hist_delta_th2))
    {
        sceneChangeFlagHist = (histDelta - pTmcData->scene_change_hist_delta_th1) /
                                 (pTmcData->scene_change_hist_delta_th2 - pTmcData->scene_change_hist_delta_th1);
    }
    else
    {
        sceneChangeFlagHist = 0;
    }

    // printf("\n_____TMC12___sceneChangeFlagHist = %f\n", sceneChangeFlagHist);
    // printf("\n_____TMC12___sceneChangeFlagLuxIdx = %f\n", sceneChangeFlagLuxIdx);

    // trigger sceneChangeFlag with OR condition of luxIdxDelta and histDelta
    // sceneChangeFlag = 1 - (1 - sceneChangeFlagLuxIdx) * (1 - sceneChangeFlagHist);

    if (0 == pInput->frameNumber)
    {
        sceneChangeFlag = 1.0f;
    }
    else
    {
        sceneChangeFlag = 1 - (1 - sceneChangeFlagLuxIdx) * (1 - sceneChangeFlagHist);
    }

    // if (0 == pInput->frameNumber)
    // {
    //     sceneChangeFlag = 1;
    // }

    // printf("\n_____TMC12___sceneChangeFlag = %f\n", sceneChangeFlag);

    // temporal smoothing on Hist
    FLOAT   inputSmoothingStr = 0.0f;

    // fallback to even Hist when scene change occurs
    if (sceneChangeFlag > 0.0f)
    {
        inputSmoothingStr = pTmcData->scene_change_smoothing_str * sceneChangeFlag;
    }
    else
    {
        inputSmoothingStr = pTmcData->hist_smoothing_str;
    }

    for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
    {
        pTmcHist[i] = inputSmoothingStr * pTmcPreHist[i] + (1 - inputSmoothingStr) * pTmcHist[i];
    }

    // // update preHist
    // for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
    // {
    //     pInput->pPrevSmoothGRHist[i] = pTmcHist[i];
    // }

    // update scene change flag
    pTmc12PerFrameInfo->sceneChangeFlag = sceneChangeFlag;

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC12Interpolation::CalculateKneePoints
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC12Interpolation::CalculateKneePoints(
    tmc_1_2_0::tmc12_rgn_dataType* pTmcData,
    const TMC12InputData*          pInput,
    TMC12PerFrameInfo*             pTmc12PerFrameInfo)
{
    FLOAT* pKneeX    = pTmc12PerFrameInfo->pKneeX;
    FLOAT* pKneeY    = pTmc12PerFrameInfo->pKneeY;
    FLOAT* pChipCoef = pTmc12PerFrameInfo->pPchipCoeffficient;

    FLOAT  anchorKneeX[TMC12_KNEE_POINTS_NUM];
    FLOAT  anchorKneeY[TMC12_KNEE_POINTS_NUM];
    FLOAT  histKneeX[TMC12_KNEE_POINTS_NUM] = { 0 };
    FLOAT  histKneeY[TMC12_KNEE_POINTS_NUM] = { 0 };

    // knee points from histogram
    CalculateAnchorKneePoints(pTmcData, pInput, anchorKneeX, anchorKneeY, &(pTmc12PerFrameInfo->drcGainDark));

    if (NULL != pInput->pGRHist)
    {
        // knee points from aec metadata
        CalculateHistKneePoints(pTmcData, pInput, histKneeX, histKneeY, pTmc12PerFrameInfo);
    }

    // blend knee points
    FLOAT currentHistEnhanceRatio = 0.0f;
    for (UINT32 i = 0; i < TMC12_KNEE_POINTS_NUM; i++)
    {
        currentHistEnhanceRatio = pTmcData->hist_enhance_ratio_tab.hist_enhance_ratio[i];

        pKneeX[i] = (currentHistEnhanceRatio * histKneeX[i]) + (1 - currentHistEnhanceRatio) * anchorKneeX[i];
        pKneeY[i] = (currentHistEnhanceRatio * histKneeY[i]) + (1 - currentHistEnhanceRatio) * anchorKneeY[i];
    }

    for (UINT32 i = 1; i < TMC12_KNEE_POINTS_NUM; i++)
    {
        if ((pKneeX[i] - pKneeX[i - 1]) <= 0.0f)
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
        }
    }

    for (UINT32 i = 1; i < TMC12_KNEE_POINTS_NUM; i++)
    {
        if ((pKneeY[i] - pKneeY[i - 1]) <= 0.0f)
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
        }
    }

    MaxLTMPercentageCheck(pTmcData, (pKneeY[TMC12_KNEE_MIN] / pKneeX[TMC12_KNEE_MIN]));

    CalculatePCHIPCoefficient(pKneeX, pKneeY, pChipCoef);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC12Interpolation::CalculateAnchorKneePoints
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC12Interpolation::CalculateAnchorKneePoints(
    tmc_1_2_0::tmc12_rgn_dataType* pTmcData,
    const TMC12InputData*          pInput,
    FLOAT*                         pAnchorKneeX,
    FLOAT*                         pAnchorKneeY,
    FLOAT*                         pDrcGainDark)
{
    UINT32 drcIdx            = 0;
    FLOAT  drcGain           = 0.0f;
    FLOAT  gainRatio         = 0.0f;
    FLOAT  drcGainDark       = 0.0f;
    FLOAT  kneeMidX          = 0.0f;
    FLOAT  kneeDarkX         = 0.0f;
    FLOAT  kneeDarkY         = 0.0f;
    FLOAT  darkBoostOffset   = (TRUE == IsOverrideDarkBoostOffsetValid(pInput->overrideDarkBoostOffset))?
                                pInput->overrideDarkBoostOffset:
                                pTmcData->dark_boost_offset;
    FLOAT  fourthToneAnchor  = (TRUE == IsOverrideFourthToneAnchorValid(pInput->overrideFourthToneAnchor,
                                                                        pTmcData->tone_anchors_tab.tone_anchors))?
                                pInput->overrideFourthToneAnchor:
                                pTmcData->tone_anchors_tab.tone_anchors[3];

    drcIdx      = IQSettingUtils::MinUINT32(static_cast<UINT32>(round(log(pInput->DRCGain) / log(TMC12_UNIT_GAIN_STEP))),
                                            TMC12_DRC_INDEX_MIN);   // TMC12_DRC_INDEX_MIN?
    drcGain     = powf(TMC12_DRC_GAIN_POW_X, static_cast<FLOAT>(drcIdx));
    gainRatio   = IQSettingUtils::ClampFLOAT((pInput->DRCGainDark - 1) * pTmcData->dark_boost_ratio + 1 +
                                              darkBoostOffset,
                                              TMC12_GAIN_RATIO_MIN,
                                              TMC12_GAIN_RATIO_MAX);
    drcGainDark = IQSettingUtils::MinFLOAT(drcGain * gainRatio, pInput->pChromatix->chromatix_tmc12_reserve.tone_max_ratio);
    gainRatio   = IQSettingUtils::MaxFLOAT(gainRatio, 1.0001f);

    kneeMidX    = pTmcData->tone_anchors_tab.tone_anchors[2] / drcGain;
    kneeDarkY   = pTmcData->tone_anchors_tab.tone_anchors[2] / gainRatio;
    kneeDarkY   = IQSettingUtils::MinFLOAT(kneeDarkY, pTmcData->tone_anchors_tab.tone_anchors[1]);
    kneeDarkX   = kneeDarkY / drcGainDark;

    pAnchorKneeX[0] = 0.0f;
    pAnchorKneeX[1] = kneeDarkX;
    pAnchorKneeX[2] = kneeMidX;
    pAnchorKneeX[3] = fourthToneAnchor;
    pAnchorKneeX[4] = 1.0f;

    pAnchorKneeY[0] = 0.0f;
    pAnchorKneeY[1] = kneeDarkY;
    pAnchorKneeY[2] = pTmcData->tone_anchors_tab.tone_anchors[2];
    pAnchorKneeY[3] = fourthToneAnchor;
    pAnchorKneeY[4] = 1.0f;

    *pDrcGainDark = drcGainDark;

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC12Interpolation::CalculateHistKneePoints
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC12Interpolation::CalculateHistKneePoints(
    tmc_1_2_0::tmc12_rgn_dataType* pTmcData,
    const TMC12InputData*          pInput,
    FLOAT*                         pKneeX,
    FLOAT*                         pKneeY,
    TMC12PerFrameInfo*             pTmc12PerFrameInfo)
{
    const FLOAT* pGamma15 = pInput->IPEGammaOutput;
    FLOAT*       pTmcHist = pTmc12PerFrameInfo->pProcessedHistogram;

    // get histogram total count
    FLOAT n_hist         = pTmc12PerFrameInfo->numOfHistogram;
    FLOAT sum_all        = 0.0f;
    FLOAT sum_all_cnt    = 0.0f;
    FLOAT sum_max        = 0.0f;
    FLOAT sum_max_cnt    = 0.0f;
    FLOAT sum_min        = 0.0f;
    FLOAT sum_min_cnt    = 0.0f;
    FLOAT sum_avg        = 0.0f;
    FLOAT sum_avg_cnt    = 0.0f;

    FLOAT currentTmcHist = 0.0f;
    FLOAT tmp            = 0.0f;

    for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
    {
        currentTmcHist = pTmcHist[i];
        tmp = currentTmcHist * log_bin_tmc12[i];
        sum_all     += tmp;
        sum_all_cnt += currentTmcHist;

        if ((sum_all_cnt >= (n_hist * pTmcData->hist_supr_range_start)) &&
            (sum_all_cnt <= (n_hist * pTmcData->hist_supr_range_end)))
        {
            sum_max     += tmp;
            sum_max_cnt += currentTmcHist;
        }

        if ((sum_all_cnt >= (n_hist * pTmcData->hist_boost_range_start)) &&
            (sum_all_cnt <= (n_hist * pTmcData->hist_boost_range_end)))
        {
            sum_min     += tmp;
            sum_min_cnt += currentTmcHist;
        }

        if ((sum_all_cnt >= (n_hist * pTmcData->hist_avg_range_start)) &&
            (sum_all_cnt <= (n_hist * pTmcData->hist_avg_range_end)))
        {
            sum_avg     += tmp;
            sum_avg_cnt += currentTmcHist;
        }
    }

    FLOAT lw_avg = static_cast<FLOAT>((sum_avg_cnt) ? exp(sum_avg / sum_avg_cnt) : static_cast<FLOAT>(0.0f)); // 14b
    FLOAT lw_max = static_cast<FLOAT>((sum_max_cnt) ? exp(sum_max / sum_max_cnt) : static_cast<FLOAT>(0.0f)); // 14b
    FLOAT lw_min = static_cast<FLOAT>((sum_min_cnt) ? exp(sum_min / sum_min_cnt) : static_cast<FLOAT>(0.0f)); // 14b
    FLOAT tar_x  = 0.0f;
    FLOAT tar_x_igamma[5] = { static_cast<FLOAT>(256.0f) };

    for (UINT32 ti = 0; ti < 5; ti++)
    {
        tar_x = IQSettingUtils::ClampFLOAT(pTmcData->tone_target_tab.tone_target[ti] * 1023, 0.0f, 1023.0f);
        for (UINT32 gi = 1; gi < 257; gi++) //  increase array?
        {
            if (tar_x <= pGamma15[gi])
            {
                tar_x_igamma[ti] = ((pGamma15[gi] - tar_x) * (gi - 1) + (tar_x - pGamma15[gi - 1]) * gi) /
                                    (pGamma15[gi] - pGamma15[gi - 1]);
                break;
            }
        }
    }

    FLOAT a = tar_x_igamma[2] / 256;
    FLOAT adaptive_a_Lw_max = pTmcData->stretch_bright_str * lw_max + (1.0f - pTmcData->stretch_bright_str) * 16383; // 14b
    FLOAT adaptive_a_Lw_min = pTmcData->stretch_dark_str   * lw_min + (1.0f - pTmcData->stretch_dark_str)   * 0; // 14b

    // used to avoid the singularity that occurs with 0 when taking logarithm operation.
    const FLOAT epsilon = 1.0f;
    FLOAT f = static_cast<FLOAT>((log(lw_avg + epsilon) * 2 - log(adaptive_a_Lw_min + epsilon) -
                                  log(adaptive_a_Lw_max + epsilon)) /
                                 (log(adaptive_a_Lw_max + epsilon) - log(adaptive_a_Lw_min + epsilon)));

    // adaptive key value for high key and low key scenes
    FLOAT adaptive_a = a * powf((f > 0)? pTmcData->tone_bright_adj: pTmcData->tone_dark_adj, f);
    FLOAT global_gain = adaptive_a / (lw_avg / 16383);

    global_gain = IQSettingUtils::ClampFLOAT(global_gain,
                                             pInput->DRCGain / pTmcData->hist_enhance_clamp,
                                             pInput->DRCGain * pTmcData->hist_enhance_clamp);

    // find bezier curve
    FLOAT gain_min_adj = global_gain * pTmcData->contrast_dark_adj;
    FLOAT gain_avg_adj = global_gain;

    // if global gain is higher, max knee should be suppressed more
    FLOAT gain_max_adj =  IQSettingUtils::ClampFLOAT(pTmcData->contrast_bright_clip * 2 - global_gain,
                                                     1.0f,
                                                     global_gain);

    pKneeX[0] = 0.0f;
    pKneeY[0] = 0.0f;
    pKneeX[1] = lw_min / 16383;
    pKneeY[1] = pKneeX[1] * gain_min_adj;
    pKneeX[2] = lw_avg / 16383;
    pKneeY[2] = pKneeX[2] * gain_avg_adj;
    pKneeX[3] = lw_max / 16383;
    pKneeY[3] = pKneeX[3] * gain_max_adj;
    pKneeX[4] = 1.0f;
    pKneeY[4] = 1.0f;

    for (UINT32 i = 1; i < TMC12_KNEE_POINTS_NUM; i++)
    {
        if ((pKneeX[i] - pKneeX[i - 1]) <= 0.0f)
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
        }
    }

    for (UINT32 i = 1; i < TMC12_KNEE_POINTS_NUM; i++)
    {
        if ((pKneeY[i] - pKneeY[i - 1]) <= 0.0f)
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
        }
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC12Interpolation::MaxLTMPercentageCheck
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC12Interpolation::MaxLTMPercentageCheck(
    tmc_1_2_0::tmc12_rgn_dataType* pTmcData,
    FLOAT                          maxGain)
{
    UINT32 drcIdx = IQSettingUtils::MinUINT32(static_cast<UINT32>(round(log(maxGain) / log(TMC12_UNIT_GAIN_STEP))),
                                            TMC12_DRC_INDEX_MIN);   // TMC12_DRC_INDEX_MIN?

    FLOAT  maxLTMPercengate = IQSettingUtils::MinFLOAT(log(TMC12_MAX_LTM_GAIN) /
                                                log(powf(TMC12_DRC_GAIN_POW_X, static_cast<FLOAT>(drcIdx))),
                                                1.0f);

    if (pTmcData->ltm_percentage > maxLTMPercengate)
    {
        pTmcData->ltm_percentage = maxLTMPercengate;
        pTmcData->gtm_percentage = 1 - pTmcData->ltm_percentage;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC12Interpolation::CalculatePCHIPCoefficient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC12Interpolation::CalculatePCHIPCoefficient(
    FLOAT* pIn,
    FLOAT* pOut,
    FLOAT* pCoef)
{
    FLOAT tmp;
    FLOAT w1;
    FLOAT w2;
    FLOAT h[TMC12_KNEE_POINTS_NUM - 1];
    FLOAT delta[TMC12_KNEE_POINTS_NUM - 1];
    FLOAT b[TMC12_KNEE_POINTS_NUM];
    FLOAT c[TMC12_KNEE_POINTS_NUM];
    FLOAT d[TMC12_KNEE_POINTS_NUM];;

    // prepare diff
    for (UINT32 i = 0; i < TMC12_KNEE_POINTS_NUM - 1; i++)
    {
        h[i] = pIn[i + 1] - pIn[i];
        delta[i] = (pOut[i + 1] - pOut[i]) / h[i];
    }

    // calculate d
    for (UINT32 i = 0; i < TMC12_KNEE_POINTS_NUM; i++)
    {
        if (i == 0)
        {
            tmp = ((2 * h[i] + h[i + 1]) * delta[i] - h[i] * delta[i + 1]) / (h[i] + h[i + 1]);
            d[i] = (delta[i] * delta[i + 1] < 0 && IQSettingUtils::AbsoluteFLOAT(tmp) >
                    IQSettingUtils::AbsoluteFLOAT(3 * delta[i])) ?
                    3 * delta[i] : (tmp * delta[i] < 0) ? 0 : tmp;
        }
        else if (i == TMC12_KNEE_POINTS_NUM - 1)
        {
            tmp = ((2 * h[i - 1] + h[i - 2]) * delta[i - 1] - h[i - 1] * delta[i - 2]) / (h[i - 2] + h[i - 1]);
            d[i] = (delta[i - 2] * delta[i - 1] < 0 && IQSettingUtils::AbsoluteFLOAT(tmp) >
                    IQSettingUtils::AbsoluteFLOAT(3 * delta[i - 1])) ?
                    3 * delta[i - 1] : (tmp * delta[i - 1] < 0) ? 0 : tmp;
        }
        else
        {
            w1 = 2 * h[i] + h[i - 1];
            w2 = h[i] + 2 * h[i - 1];
            d[i] = (w1 + w2) / (w1 / delta[i - 1] + w2 / delta[i]);
        }
    }

    // calcualte b and c
    for (UINT32 i = 0; i < TMC12_KNEE_POINTS_NUM - 1; i++)
    {
        b[i] = (d[i] - 2 * delta[i] + d[i + 1]) / h[i] / h[i];
        c[i] = (3 * delta[i] - 2 * d[i] - d[i + 1]) / h[i];
    }

    // fill output
    for (UINT32 i = 0; i < TMC12_KNEE_POINTS_NUM - 2; i++)
    {
        pCoef[i * 3 + 0] = d[i + 1];
        pCoef[i * 3 + 1] = c[i + 1];
        pCoef[i * 3 + 2] = b[i + 1];
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC12Interpolation::CalculateContrastEnhanceCurve
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC12Interpolation::CalculateContrastEnhanceCurve(
    tmc_1_2_0::tmc12_rgn_dataType*  pTmcData,
    const TMC12InputData*           pInput,
    TMC12PerFrameInfo*              pTmc12PerFrameInfo)
{
    FLOAT*          pTmcHist              = pTmc12PerFrameInfo->pProcessedHistogram;
    FLOAT*          pContrastEnhanceCurve = pTmc12PerFrameInfo->pContrastEnhanceCurve;
    FLOAT*          pCdf                  = pTmc12PerFrameInfo->pCalculatedCDF;
    FLOAT*          pKneeX                = pTmc12PerFrameInfo->pKneeX;
    FLOAT*          pKneeY                = pTmc12PerFrameInfo->pKneeY;
    const FLOAT*    pGamma15              = pInput->IPEGammaOutput;
    FLOAT*          pCdf_pre;
    FLOAT           log_bin_y[TMC12_HIST_BIN_NUM];
    FLOAT           log_bin_y_linear[TMC12_HIST_BIN_NUM];
    FLOAT           tmcOutGainCurve[TMC12_HIST_BIN_NUM];
    FLOAT           tmcInCurve[TMC12_HIST_BIN_NUM];

    // prepare current CDF
    pCdf[0] = pTmcHist[0];
    for (UINT32 i = 1; i < TMC12_HIST_BIN_NUM; i++)
    {
        pCdf[i] = pCdf[i - 1] + pTmcHist[i];
    }

    // prepare previous CDF
    if ((pInput->frameNumber < 2) || (NULL == pInput->pPreviousCalculatedCDF))
    {
        pCdf_pre = pCdf;
    }
    else
    {
        pCdf_pre = pInput->pPreviousCalculatedCDF;
    }

    // temporal smoothing on CDF
    for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
    {
        pCdf[i] = pTmcData->hist_curve_smoothing_str * pCdf_pre[i] + (1 - pTmcData->hist_curve_smoothing_str) * pCdf[i];
    }

    for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
    {
        tmcInCurve[i] = exp(log_bin_tmc12[i]) / exp(log_bin_tmc12[TMC12_HIST_BIN_NUM - 1]);
    }

    if (pInput->pAdrcOutputData->curveModel == 2)
    {
        // PCHIP curve
        IQSettingUtils::PCHIPCurve(pKneeX,
                                   pKneeY,
                                   &pInput->pAdrcOutputData->pchipCoeffficient[0],
                                   tmcInCurve,
                                   tmcOutGainCurve,
                                   TMC12_HIST_BIN_NUM - 1);
    }
    else
    {
        // Bezier curve
        IQSettingUtils::BezierCurve(pKneeX,
                                    pKneeY,
                                    tmcInCurve,
                                    tmcOutGainCurve,
                                    TMC12_HIST_BIN_NUM - 1);
    }

    // map output for each bin
    UINT32 gi = 1;
    for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
    {
        FLOAT tar_x = IQSettingUtils::ClampFLOAT(pCdf[i] / pCdf[TMC12_HIST_BIN_NUM - 1] * 1023, 0.0f, 1023.0f);
        FLOAT tar_x_igamma = 256.0f;
        for ( ; gi < 257; gi++)
        {
            if (tar_x <= pGamma15[gi])
            {
                tar_x_igamma = ((pGamma15[gi] - tar_x) * static_cast<FLOAT>(gi - 1) +
                                (tar_x - pGamma15[gi - 1]) * static_cast<FLOAT>(gi)) /
                                (pGamma15[gi] - pGamma15[gi - 1]);
                break;
            }
        }

        log_bin_y[i] = tar_x_igamma / 256;
    }

    // fallback to DRC gain curve when scene change occurs
    if (pTmc12PerFrameInfo->sceneChangeFlag > 0.0f)
    {
        // blending with scene chage level
        FLOAT sceneChangeCurveSmoothingStr = pTmcData->scene_change_curve_smoothing_str * pTmc12PerFrameInfo->sceneChangeFlag;
        for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
        {
            log_bin_y_linear[i] = IQSettingUtils::ClampFLOAT(tmcOutGainCurve[i] * exp(log_bin_tmc12[i]) /
                                    exp(log_bin_tmc12[TMC12_HIST_BIN_NUM - 1]), 0.0f, 1.0f);
        }

        for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
        {
            log_bin_y[i] = sceneChangeCurveSmoothingStr * log_bin_y_linear[i] +
                            (1 - sceneChangeCurveSmoothingStr) * log_bin_y[i];
        }
    }

    for (UINT32 i = 0; i < TMC12_HIST_BIN_NUM; i++)
    {
        pContrastEnhanceCurve[i] = log_bin_y[i];
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC12Interpolation::SetZerosToTMCData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID TMC12Interpolation::SetZerosToTMCData(
    tmc_1_2_0::tmc12_rgn_dataType* pTmcData)
{
    for (UINT32 i = 0; i < TMC12_KNEE_POINTS_NUM; i++)
    {
        pTmcData->hist_enhance_ratio_tab.hist_enhance_ratio[i] = 0.0f;
    }

    pTmcData->contrast_he_bright = 0.0f;
    pTmcData->contrast_he_dark   = 0.0f;;
}