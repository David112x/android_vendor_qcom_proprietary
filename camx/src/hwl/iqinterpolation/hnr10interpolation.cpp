// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  hnr10interpolation.cpp
/// @brief BPS HNR10 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "hnr10interpolation.h"
#include "hnr10setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation HNR10OperationTable[] =
{
    {HNR10Interpolation::TotalScaleRatioSearchNode, 2},
    {HNR10Interpolation::DRCGainSearchNode,         2},
    {HNR10Interpolation::HDRAECSearchNode,          2},
    {HNR10Interpolation::AECSearchNode,             2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HNR10Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HNR10Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    HNR10InputData*   pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))                        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->totalScaleRatio, pInput->totalScaleRatio))        ||
        (pTriggerData->streamWidth      != pInput->fullInputWidth)                                       ||
        (pTriggerData->streamHeight     != pInput->fullInputHeight)                                      ||
        (pTriggerData->horizontalOffset != pInput->sensorOffsetX)                                        ||
        (pTriggerData->verticalOffset   != pInput->sensorOffsetY))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->realGain          = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->DRCGain           = pInput->DRCGain;
        pTriggerData->totalScaleRatio   = pInput->totalScaleRatio;
        pTriggerData->streamWidth       = pInput->fullInputWidth;
        pTriggerData->streamHeight      = pInput->fullInputHeight;
        pTriggerData->horizontalOffset  = pInput->sensorOffsetX;
        pTriggerData->verticalOffset    = pInput->sensorOffsetY;

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HNR10Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HNR10Interpolation::RunInterpolation(
    const HNR10InputData*          pInput,
    hnr_1_0_0::hnr10_rgn_dataType* pData)
{
    BOOL                          result = TRUE;
    // The interpolation tree total Node
    TuningNode                    nodeSet[HNR10MaxmiumNode];
    // HNR10 Trigger List
    HNR10TriggerList              HNRTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < HNR10MaxmiumNode; count++)
        {
            if (count < HNR10MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_hnr10_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&HNRTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(hnr_1_0_0::chromatix_hnr10Type::control_methodStruct));

        HNRTrigger.triggerDRCgain         = pInput->DRCGain;
        HNRTrigger.triggerTotalScaleRatio = pInput->totalScaleRatio;

        HNRTrigger.triggerHDRAEC = IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                                                    pInput->exposureTime,
                                                                    pInput->AECSensitivity,
                                                                    pInput->exposureGainRatio);

        HNRTrigger.triggerAEC = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                              pInput->luxIndex,
                                                              pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        HNR10InterpolationLevel,
                                                        &HNR10OperationTable[0],
                                                        static_cast<VOID*>(&HNRTrigger));
    }

    if (FALSE != result)
    {
        // Calculate the Interpolation Result
        result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                       HNR10MaxmiumNonLeafNode,
                                                       HNR10InterpolationLevel,
                                                       HNR10Interpolation::DoInterpolation);
    }
    if (FALSE != result)
    {
        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(hnr_1_0_0::hnr10_rgn_dataType));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HNR10Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HNR10Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    hnr_1_0_0::hnr10_rgn_dataType* pInput1 = NULL;
    hnr_1_0_0::hnr10_rgn_dataType* pInput2 = NULL;
    hnr_1_0_0::hnr10_rgn_dataType* pOutput = NULL;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        pInput1 = static_cast<hnr_1_0_0::hnr10_rgn_dataType*>(pData1);
        pInput2 = static_cast<hnr_1_0_0::hnr10_rgn_dataType*>(pData2);
        pOutput = static_cast<hnr_1_0_0::hnr10_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(hnr_1_0_0::hnr10_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = HNR10Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(hnr_1_0_0::hnr10_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(hnr_1_0_0::hnr10_rgn_dataType));
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
/// HNR10Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HNR10Interpolation::InterpolationData(
    hnr_1_0_0::hnr10_rgn_dataType* pInput1,
    hnr_1_0_0::hnr10_rgn_dataType* pInput2,
    FLOAT                          ratio,
    hnr_1_0_0::hnr10_rgn_dataType* pOutput)
{
    INT  i;
    BOOL result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        pOutput->abs_amp_shift      = IQSettingUtils::InterpolationFloatBilinear(pInput1->abs_amp_shift,
                                                                                 pInput2->abs_amp_shift,
                                                                                 ratio);

        pOutput->blend_cnr_adj_gain = IQSettingUtils::InterpolationFloatBilinear(pInput1->blend_cnr_adj_gain,
                                                                                 pInput2->blend_cnr_adj_gain,
                                                                                 ratio);

        pOutput->cnr_adj_gain       = IQSettingUtils::InterpolationFloatBilinear(pInput1->cnr_adj_gain,
                                                                                 pInput2->cnr_adj_gain,
                                                                                 ratio);

        pOutput->cnr_low_gap_u      = IQSettingUtils::InterpolationFloatBilinear(pInput1->cnr_low_gap_u,
                                                                                 pInput2->cnr_low_gap_u,
                                                                                 ratio);

        pOutput->cnr_low_gap_v      = IQSettingUtils::InterpolationFloatBilinear(pInput1->cnr_low_gap_v,
                                                                                 pInput2->cnr_low_gap_v,
                                                                                 ratio);

        pOutput->cnr_low_thrd_u     = IQSettingUtils::InterpolationFloatBilinear(pInput1->cnr_low_thrd_u,
                                                                                 pInput2->cnr_low_thrd_u,
                                                                                 ratio);

        pOutput->cnr_low_thrd_v     = IQSettingUtils::InterpolationFloatBilinear(pInput1->cnr_low_thrd_v,
                                                                                 pInput2->cnr_low_thrd_v,
                                                                                 ratio);

        pOutput->cnr_scale          = IQSettingUtils::InterpolationFloatBilinear(pInput1->cnr_scale,
                                                                                 pInput2->cnr_scale,
                                                                                 ratio);

        pOutput->face_boundary      = IQSettingUtils::InterpolationFloatBilinear(pInput1->face_boundary,
                                                                                 pInput2->face_boundary,
                                                                                 ratio);

        pOutput->face_transition    = IQSettingUtils::InterpolationFloatBilinear(pInput1->face_transition,
                                                                                 pInput2->face_transition,
                                                                                 ratio);

        pOutput->fnr_ac_shift       = IQSettingUtils::InterpolationFloatBilinear(pInput1->fnr_ac_shift,
                                                                                 pInput2->fnr_ac_shift,
                                                                                 ratio);

        pOutput->lnr_shift          = IQSettingUtils::InterpolationFloatBilinear(pInput1->lnr_shift,
                                                                                 pInput2->lnr_shift,
                                                                                 ratio);

        pOutput->lpf3_offset         = IQSettingUtils::InterpolationFloatBilinear(pInput1->lpf3_offset,
                                                                                  pInput2->lpf3_offset,
                                                                                  ratio);

        pOutput->lpf3_percent       = IQSettingUtils::InterpolationFloatBilinear(pInput1->lpf3_percent,
                                                                                 pInput2->lpf3_percent,
                                                                                 ratio);

        pOutput->lpf3_strength      = IQSettingUtils::InterpolationFloatBilinear(pInput1->lpf3_strength,
                                                                                 pInput2->lpf3_strength,
                                                                                 ratio);

        pOutput->skin_boundary_probability =
            IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_boundary_probability,
                                                       pInput2->skin_boundary_probability,
                                                       ratio);

        pOutput->skin_hue_max = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_hue_max,
                                                                           pInput2->skin_hue_max,
                                                                           ratio);

        pOutput->skin_hue_min = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_hue_min,
                                                                           pInput2->skin_hue_min,
                                                                           ratio);

        pOutput->skin_non_skin_to_skin_q_ratio =
            IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_non_skin_to_skin_q_ratio,
                                                       pInput2->skin_non_skin_to_skin_q_ratio,
                                                       ratio);

        pOutput->skin_percent = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_percent,
                                                                           pInput2->skin_percent,
                                                                           ratio);

        pOutput->skin_saturation_max_y_max = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_saturation_max_y_max,
                                                                                        pInput2->skin_saturation_max_y_max,
                                                                                        ratio);

        pOutput->skin_saturation_max_y_min = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_saturation_max_y_min,
                                                                                        pInput2->skin_saturation_max_y_min,
                                                                                        ratio);

        pOutput->skin_saturation_min_y_max = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_saturation_min_y_max,
                                                                                        pInput2->skin_saturation_min_y_max,
                                                                                        ratio);

        pOutput->skin_saturation_min_y_min = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_saturation_min_y_min,
                                                                                        pInput2->skin_saturation_min_y_min,
                                                                                        ratio);

        pOutput->skin_y_max = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_y_max,
                                                                         pInput2->skin_y_max,
                                                                         ratio);

        pOutput->skin_y_min = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_y_min,
                                                                         pInput2->skin_y_min,
                                                                         ratio);

        pOutput->snr_skin_smoothing_str = IQSettingUtils::InterpolationFloatBilinear(pInput1->snr_skin_smoothing_str,
                                                                                     pInput2->snr_skin_smoothing_str,
                                                                                     ratio);

        for (i = 0; i < HNR_V10_BLEND_LNR_ARR_NUM; i++)
        {
            pOutput->blend_lnr_gain_arr_tab.blend_lnr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->blend_lnr_gain_arr_tab.blend_lnr_gain_arr[i],
                                                           pInput2->blend_lnr_gain_arr_tab.blend_lnr_gain_arr[i],
                                                           ratio);
        }

        for (i = 0; i < HNR_V10_BLEND_SNR_ARR_NUM; i++)
        {
            pOutput->blend_snr_gain_arr_tab.blend_snr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->blend_snr_gain_arr_tab.blend_snr_gain_arr[i],
                                                           pInput2->blend_snr_gain_arr_tab.blend_snr_gain_arr[i],
                                                           ratio);
        }

        for (i = 0; i < HNR_V10_CNR_ARR_NUM; i++)
        {
            pOutput->cnr_gain_arr_tab.cnr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->cnr_gain_arr_tab.cnr_gain_arr[i],
                                                           pInput2->cnr_gain_arr_tab.cnr_gain_arr[i],
                                                           ratio);
        }

        for (i = 0; i < HNR_V10_NR_ARR_NUM; i++)
        {
            pOutput->filtering_nr_gain_arr_tab.filtering_nr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->filtering_nr_gain_arr_tab.filtering_nr_gain_arr[i],
                                                           pInput2->filtering_nr_gain_arr_tab.filtering_nr_gain_arr[i],
                                                           ratio);
        }

        for (i = 0; i < HNR_V10_FNR_AC_ARR_NUM; i++)
        {
            pOutput->fnr_ac_th_tab.fnr_ac_th[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->fnr_ac_th_tab.fnr_ac_th[i],
                                                           pInput2->fnr_ac_th_tab.fnr_ac_th[i],
                                                           ratio);
        }

        for (i = 0; i < HNR_V10_FNR_ARR_NUM; i++)
        {
            pOutput->fnr_gain_arr_tab.fnr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->fnr_gain_arr_tab.fnr_gain_arr[i],
                                                           pInput2->fnr_gain_arr_tab.fnr_gain_arr[i],
                                                           ratio);
        }

        for (i = 0; i < HNR_V10_FNR_ARR_NUM; i++)
        {
            pOutput->fnr_gain_clamp_arr_tab.fnr_gain_clamp_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->fnr_gain_clamp_arr_tab.fnr_gain_clamp_arr[i],
                                                           pInput2->fnr_gain_clamp_arr_tab.fnr_gain_clamp_arr[i],
                                                           ratio);
        }

        for (i = 0; i < HNR_V10_LNR_ARR_NUM; i++)
        {
            pOutput->lnr_gain_arr_tab.lnr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->lnr_gain_arr_tab.lnr_gain_arr[i],
                                                           pInput2->lnr_gain_arr_tab.lnr_gain_arr[i],
                                                           ratio);
        }

        for (i = 0; i < HNR_V10_RNR_ARR_NUM + 1; i++)
        {
            pOutput->radial_noise_prsv_adj_tab.radial_noise_prsv_adj[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->radial_noise_prsv_adj_tab.radial_noise_prsv_adj[i],
                                                           pInput2->radial_noise_prsv_adj_tab.radial_noise_prsv_adj[i],
                                                           ratio);
        }

        for (i = 0; i < HNR_V10_SNR_ARR_NUM; i++)
        {
            pOutput->snr_gain_arr_tab.snr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->snr_gain_arr_tab.snr_gain_arr[i],
                                                           pInput2->snr_gain_arr_tab.snr_gain_arr[i],
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
// HNR10Interpolation::TotalScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT HNR10Interpolation::TotalScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    hnr_1_0_0::chromatix_hnr10_coreType*   pParentDataType = NULL;
    HNR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<hnr_1_0_0::chromatix_hnr10_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_hnr10_total_scale_ratio_dataCount;
        pTriggerList    = static_cast<HNR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_hnr10_total_scale_ratio_data[count].total_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerTotalScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_hnr10_total_scale_ratio_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_hnr10_total_scale_ratio_data[regionOutput.endIndex]),
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
// HNR10Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT HNR10Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    hnr_1_0_0::mod_hnr10_total_scale_ratio_dataType*   pParentDataType = NULL;
    HNR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<hnr_1_0_0::mod_hnr10_total_scale_ratio_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->total_scale_ratio_data.mod_hnr10_drc_gain_dataCount;
        pTriggerList    = static_cast<HNR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->total_scale_ratio_data.mod_hnr10_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->total_scale_ratio_data.mod_hnr10_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->total_scale_ratio_data.mod_hnr10_drc_gain_data[regionOutput.endIndex]),
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
// HNR10Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT HNR10Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    hnr_1_0_0::mod_hnr10_drc_gain_dataType*   pParentDataType = NULL;
    HNR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<hnr_1_0_0::mod_hnr10_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_hnr10_hdr_aec_dataCount;
        pTriggerList    = static_cast<HNR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_hnr10_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_hnr10_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_hnr10_hdr_aec_data[regionOutput.endIndex]),
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
// HNR10Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT HNR10Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    hnr_1_0_0::mod_hnr10_hdr_aec_dataType*   pParentDataType = NULL;
    HNR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<hnr_1_0_0::mod_hnr10_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_hnr10_aec_dataCount;
        pTriggerList    = static_cast<HNR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_hnr10_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_hnr10_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_hnr10_aec_data[regionOutput.startIndex].hnr10_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_hnr10_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_hnr10_aec_data[regionOutput.endIndex].hnr10_rgn_data));
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
