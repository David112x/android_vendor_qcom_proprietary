// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  lenr10interpolation.cpp
/// @brief IPE LENR10 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "lenr10interpolation.h"
#include "lenr10setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation LENR10OperationTable[] =
{
    { LENR10Interpolation::TotalScaleRatioSearchNode, 2 },
    {LENR10Interpolation::DRCGainSearchNode,          2 },
    {LENR10Interpolation::HDRAECSearchNode,           2 },
    {LENR10Interpolation::AECSearchNode,              2 },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LENR10Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LENR10Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    LENR10InputData*   pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))                        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->totalScaleRatio, pInput->totalScaleRatio))        ||
        (pTriggerData->streamWidth      != pInput->ds4InputWidth)                                        ||
        (pTriggerData->streamHeight     != pInput->ds4InputHeight)                                       ||
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
        pTriggerData->streamWidth       = pInput->ds4InputWidth;
        pTriggerData->streamHeight      = pInput->ds4InputHeight;
        pTriggerData->horizontalOffset  = pInput->sensorOffsetX;
        pTriggerData->verticalOffset    = pInput->sensorOffsetY;

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LENR10Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LENR10Interpolation::RunInterpolation(
    const LENR10InputData*          pInput,
    lenr_1_0_0::lenr10_rgn_dataType* pData)
{
    BOOL                          result = TRUE;
    // The interpolation tree total Node
    TuningNode                    nodeSet[LENR10MaxmiumNode];

    // LENR10 Trigger List
    LENR10TriggerList              LENRTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < LENR10MaxmiumNode; count++)
        {
            if (count < LENR10MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_lenr10_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&LENRTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(lenr_1_0_0::chromatix_lenr10Type::control_methodStruct));

        LENRTrigger.triggerDRCgain         = pInput->DRCGain;
        LENRTrigger.triggerTotalScaleRatio = pInput->totalScaleRatio;

        LENRTrigger.triggerHDRAEC = IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                                                    pInput->exposureTime,
                                                                    pInput->AECSensitivity,
                                                                    pInput->exposureGainRatio);

        LENRTrigger.triggerAEC = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                              pInput->luxIndex,
                                                              pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        LENR10InterpolationLevel,
                                                        LENR10OperationTable,
                                                        static_cast<VOID*>(&LENRTrigger));
    }
    else
    {
        result = FALSE;
    }
    if (FALSE != result)
    {
        // Calculate the Interpolation Result
        result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                       LENR10MaxmiumNonLeafNode,
                                                       LENR10InterpolationLevel,
                                                       LENR10Interpolation::DoInterpolation);
    }
    if (FALSE != result)
    {
        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(lenr_1_0_0::lenr10_rgn_dataType));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LENR10Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LENR10Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    lenr_1_0_0::lenr10_rgn_dataType* pInput1 = NULL;
    lenr_1_0_0::lenr10_rgn_dataType* pInput2 = NULL;
    lenr_1_0_0::lenr10_rgn_dataType* pOutput = NULL;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        pInput1 = static_cast<lenr_1_0_0::lenr10_rgn_dataType*>(pData1);
        pInput2 = static_cast<lenr_1_0_0::lenr10_rgn_dataType*>(pData2);
        pOutput = static_cast<lenr_1_0_0::lenr10_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(lenr_1_0_0::lenr10_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = LENR10Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (FALSE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(lenr_1_0_0::lenr10_rgn_dataType));
            }
            else if (FALSE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(lenr_1_0_0::lenr10_rgn_dataType));
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
/// LENR10Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LENR10Interpolation::InterpolationData(
    lenr_1_0_0::lenr10_rgn_dataType* pInput1,
    lenr_1_0_0::lenr10_rgn_dataType* pInput2,
    FLOAT                          ratio,
    lenr_1_0_0::lenr10_rgn_dataType* pOutput)
{
    INT  i;
    BOOL result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        for (i = 0; i < 3; i++)
        {
            pOutput->lenr_dn4_8_16_bltr_th_tab.lenr_dn4_8_16_bltr_th[i] = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->lenr_dn4_8_16_bltr_th_tab.lenr_dn4_8_16_bltr_th[i],
                pInput2->lenr_dn4_8_16_bltr_th_tab.lenr_dn4_8_16_bltr_th[i],
                ratio);
            pOutput->lenr_dn4_8_16_bltr_gap_tab.lenr_dn4_8_16_bltr_gap[i] = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->lenr_dn4_8_16_bltr_gap_tab.lenr_dn4_8_16_bltr_gap[i],
                pInput2->lenr_dn4_8_16_bltr_gap_tab.lenr_dn4_8_16_bltr_gap[i],
                ratio);
            pOutput->lenr_dn4_8_16_bltr_ctrl_th_tab.lenr_dn4_8_16_bltr_ctrl_th[i] = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->lenr_dn4_8_16_bltr_ctrl_th_tab.lenr_dn4_8_16_bltr_ctrl_th[i],
                pInput2->lenr_dn4_8_16_bltr_ctrl_th_tab.lenr_dn4_8_16_bltr_ctrl_th[i],
                ratio);
            pOutput->lenr_dn4_8_16_bltr_ctrl_w_tab.lenr_dn4_8_16_bltr_ctrl_w[i] = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->lenr_dn4_8_16_bltr_ctrl_w_tab.lenr_dn4_8_16_bltr_ctrl_w[i],
                pInput2->lenr_dn4_8_16_bltr_ctrl_w_tab.lenr_dn4_8_16_bltr_ctrl_w[i],
                ratio);
            pOutput->lenr_dn4_8_16_lce_core_p_tab.lenr_dn4_8_16_lce_core_p[i] = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->lenr_dn4_8_16_lce_core_p_tab.lenr_dn4_8_16_lce_core_p[i],
                pInput2->lenr_dn4_8_16_lce_core_p_tab.lenr_dn4_8_16_lce_core_p[i],
                ratio);
            pOutput->lenr_dn4_8_16_lce_core_n_tab.lenr_dn4_8_16_lce_core_n[i] = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->lenr_dn4_8_16_lce_core_n_tab.lenr_dn4_8_16_lce_core_n[i],
                pInput2->lenr_dn4_8_16_lce_core_n_tab.lenr_dn4_8_16_lce_core_n[i],
                ratio);
            pOutput->lenr_dn4_8_16_lce_scale_p_tab.lenr_dn4_8_16_lce_scale_p[i] = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->lenr_dn4_8_16_lce_scale_p_tab.lenr_dn4_8_16_lce_scale_p[i],
                pInput2->lenr_dn4_8_16_lce_scale_p_tab.lenr_dn4_8_16_lce_scale_p[i],
                ratio);
            pOutput->lenr_dn4_8_16_lce_scale_n_tab.lenr_dn4_8_16_lce_scale_n[i] = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->lenr_dn4_8_16_lce_scale_n_tab.lenr_dn4_8_16_lce_scale_n[i],
                pInput2->lenr_dn4_8_16_lce_scale_n_tab.lenr_dn4_8_16_lce_scale_n[i],
                ratio);
            pOutput->lenr_dn4_8_16_lce_clamp_p_tab.lenr_dn4_8_16_lce_clamp_p[i] = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->lenr_dn4_8_16_lce_clamp_p_tab.lenr_dn4_8_16_lce_clamp_p[i],
                pInput2->lenr_dn4_8_16_lce_clamp_p_tab.lenr_dn4_8_16_lce_clamp_p[i],
                ratio);
            pOutput->lenr_dn4_8_16_lce_clamp_n_tab.lenr_dn4_8_16_lce_clamp_n[i] = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->lenr_dn4_8_16_lce_clamp_n_tab.lenr_dn4_8_16_lce_clamp_n[i],
                pInput2->lenr_dn4_8_16_lce_clamp_n_tab.lenr_dn4_8_16_lce_clamp_n[i],
                ratio);
        }

        for (i = 0; i < LENR_V10_SNR_ARR_NUM; i++)
        {
            pOutput->lenr_dn4_bltr_snr_gain_arr_tab.lenr_dn4_bltr_snr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                    pInput1->lenr_dn4_bltr_snr_gain_arr_tab.lenr_dn4_bltr_snr_gain_arr[i],
                    pInput2->lenr_dn4_bltr_snr_gain_arr_tab.lenr_dn4_bltr_snr_gain_arr[i],
                    ratio);

            pOutput->lenr_dn8_bltr_snr_gain_arr_tab.lenr_dn8_bltr_snr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                    pInput1->lenr_dn8_bltr_snr_gain_arr_tab.lenr_dn8_bltr_snr_gain_arr[i],
                    pInput2->lenr_dn8_bltr_snr_gain_arr_tab.lenr_dn8_bltr_snr_gain_arr[i],
                    ratio);

            pOutput->lenr_dn16_bltr_snr_gain_arr_tab.lenr_dn16_bltr_snr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                    pInput1->lenr_dn16_bltr_snr_gain_arr_tab.lenr_dn16_bltr_snr_gain_arr[i],
                    pInput2->lenr_dn16_bltr_snr_gain_arr_tab.lenr_dn16_bltr_snr_gain_arr[i],
                    ratio);

            pOutput->lenr_dn4_lce_snr_gain_arr_tab.lenr_dn4_lce_snr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                    pInput1->lenr_dn4_lce_snr_gain_arr_tab.lenr_dn4_lce_snr_gain_arr[i],
                    pInput2->lenr_dn4_lce_snr_gain_arr_tab.lenr_dn4_lce_snr_gain_arr[i],
                    ratio);

            pOutput->lenr_dn8_lce_snr_gain_arr_tab.lenr_dn8_lce_snr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                    pInput1->lenr_dn8_lce_snr_gain_arr_tab.lenr_dn8_lce_snr_gain_arr[i],
                    pInput2->lenr_dn8_lce_snr_gain_arr_tab.lenr_dn8_lce_snr_gain_arr[i],
                    ratio);

            pOutput->lenr_dn16_lce_snr_gain_arr_tab.lenr_dn16_lce_snr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                    pInput1->lenr_dn16_lce_snr_gain_arr_tab.lenr_dn16_lce_snr_gain_arr[i],
                    pInput2->lenr_dn16_lce_snr_gain_arr_tab.lenr_dn16_lce_snr_gain_arr[i],
                    ratio);
        }

        for (i = 0; i < 7; i++)
        {
            pOutput->bltr_rnr_gain_arr_tab.bltr_rnr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->bltr_rnr_gain_arr_tab.bltr_rnr_gain_arr[i],
                                                           pInput2->bltr_rnr_gain_arr_tab.bltr_rnr_gain_arr[i],
                                                           ratio);

            pOutput->lce_rnr_gain_arr_tab.lce_rnr_gain_arr[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->lce_rnr_gain_arr_tab.lce_rnr_gain_arr[i],
                                                           pInput2->lce_rnr_gain_arr_tab.lce_rnr_gain_arr[i],
                                                           ratio);
        }

        pOutput->skin_hue_max = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_hue_max,
                                                                           pInput2->skin_hue_max,
                                                                           ratio);

        pOutput->skin_hue_min = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_hue_min,
                                                                           pInput2->skin_hue_min,
                                                                           ratio);

        pOutput->skin_y_max = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_y_max,
                                                                         pInput2->skin_y_max,
                                                                         ratio);

        pOutput->skin_y_min = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_y_min,
                                                                         pInput2->skin_y_min,
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

        pOutput->skin_boundary_probability =
            IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_boundary_probability,
                                                       pInput2->skin_boundary_probability,
                                                       ratio);

        pOutput->face_boundary      = IQSettingUtils::InterpolationFloatBilinear(pInput1->face_boundary,
                                                                                 pInput2->face_boundary,
                                                                                 ratio);

        pOutput->face_transition    = IQSettingUtils::InterpolationFloatBilinear(pInput1->face_transition,
                                                                                 pInput2->face_transition,
                                                                                 ratio);

        pOutput->skin_percent = IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_percent,
                                                                           pInput2->skin_percent,
                                                                           ratio);

        pOutput->skin_non_skin_to_skin_q_ratio =
            IQSettingUtils::InterpolationFloatBilinear(pInput1->skin_non_skin_to_skin_q_ratio,
                                                       pInput2->skin_non_skin_to_skin_q_ratio,
                                                       ratio);
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LENR10Interpolation::TotalScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LENR10Interpolation::TotalScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    lenr_1_0_0::chromatix_lenr10_coreType*   pParentDataType = NULL;
    LENR10TriggerList*  pTriggerList = NULL;

    if ((NULL != pParentNode) &&
        (NULL != pTriggerData) &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<lenr_1_0_0::chromatix_lenr10_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_lenr10_total_scale_ratio_dataCount;
        pTriggerList    = static_cast<LENR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_lenr10_total_scale_ratio_data[count].total_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerTotalScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
            static_cast<VOID*>(&pParentDataType->mod_lenr10_total_scale_ratio_data[regionOutput.startIndex]),
            NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_lenr10_total_scale_ratio_data[regionOutput.endIndex]),
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
// LENR10Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LENR10Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    lenr_1_0_0::mod_lenr10_total_scale_ratio_dataType*   pParentDataType = NULL;
    LENR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<lenr_1_0_0::mod_lenr10_total_scale_ratio_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->total_scale_ratio_data.mod_lenr10_drc_gain_dataCount;
        pTriggerList    = static_cast<LENR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->total_scale_ratio_data.mod_lenr10_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->total_scale_ratio_data.mod_lenr10_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->total_scale_ratio_data.mod_lenr10_drc_gain_data[regionOutput.endIndex]),
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
// LENR10Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LENR10Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    lenr_1_0_0::mod_lenr10_drc_gain_dataType*   pParentDataType = NULL;
    LENR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<lenr_1_0_0::mod_lenr10_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_lenr10_hdr_aec_dataCount;
        pTriggerList    = static_cast<LENR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_lenr10_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_lenr10_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_lenr10_hdr_aec_data[regionOutput.endIndex]),
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
// LENR10Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LENR10Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    lenr_1_0_0::mod_lenr10_hdr_aec_dataType*   pParentDataType = NULL;
    LENR10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<lenr_1_0_0::mod_lenr10_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_lenr10_aec_dataCount;
        pTriggerList    = static_cast<LENR10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_lenr10_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_lenr10_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_lenr10_aec_data[regionOutput.startIndex].lenr10_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_lenr10_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_lenr10_aec_data[regionOutput.endIndex].lenr10_rgn_data));
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
