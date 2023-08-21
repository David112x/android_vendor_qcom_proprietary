// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cvp10interpolation.cpp
/// @brief CVP10 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "cvp10interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation CVP10OperationTable[] =
{
    { CVP10Interpolation::PreScaleRatioSearchNode, 2 },
    { CVP10Interpolation::HDRAECSearchNode,        2 },
    { CVP10Interpolation::AECSearchNode,           2 }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVP10Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CVP10Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    CVP10InputData*   pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))              ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))  ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)))

    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->realGain          = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->preScaleRatio     = pInput->preScaleRatio;
        isChanged                       = TRUE;
    }
    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVP10Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CVP10Interpolation::RunInterpolation(
    const CVP10InputData*          pInput,
    cvp_1_0_0::cvp10_rgn_dataType* pData)
{
    BOOL result = TRUE;

    // The interpolation tree total Node
    TuningNode nodeSet[CVP10MaxmiumNode];
    // CVP10 Trigger List
    CVP10TriggerList cvpTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < CVP10MaxmiumNode; count++)
        {
            if (count < CVP10MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_cvp10_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&cvpTrigger.controlType,
            &(pInput->pChromatix->control_method),
            sizeof(cvp_1_0_0::chromatix_cvp10Type::control_methodStruct));

        cvpTrigger.triggerPreScaleRatio = pInput->preScaleRatio;

        cvpTrigger.triggerHDRAEC =
            IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
            pInput->exposureTime,
            pInput->AECSensitivity,
            pInput->exposureGainRatio);

        cvpTrigger.triggerAEC =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
            pInput->luxIndex,
            pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
            CVP10InterpolationLevel,
            &CVP10OperationTable[0],
            static_cast<VOID*>(&cvpTrigger));
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
            CVP10MaxmiumNonLeafNode,
            CVP10InterpolationLevel,
            CVP10Interpolation::DoInterpolation);
    }

    if (TRUE == result)
    {
        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(cvp_1_0_0::cvp10_rgn_dataType));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CVP10Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CVP10Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        /// pInput1 and pInput2 are the Data regions and Data region 2
        ///    Region1           Interpolation     Region2
        ///    ---------- | ------------------- | ----------
        ///    ratio= 0.0......ratio (>0 && <1).....ratio = 1.0
        cvp_1_0_0::cvp10_rgn_dataType* pInput1 = static_cast<cvp_1_0_0::cvp10_rgn_dataType*>(pData1);
        cvp_1_0_0::cvp10_rgn_dataType* pInput2 = static_cast<cvp_1_0_0::cvp10_rgn_dataType*>(pData2);
        cvp_1_0_0::cvp10_rgn_dataType* pOutput = static_cast<cvp_1_0_0::cvp10_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(cvp_1_0_0::cvp10_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = CVP10Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(cvp_1_0_0::cvp10_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(cvp_1_0_0::cvp10_rgn_dataType));
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
/// CVP10Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CVP10Interpolation::InterpolationData(
    cvp_1_0_0::cvp10_rgn_dataType* pInput1,
    cvp_1_0_0::cvp10_rgn_dataType* pInput2,
    FLOAT                          ratio,
    cvp_1_0_0::cvp10_rgn_dataType* pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        pOutput->robustness_max_allowed_ncc = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->robustness_max_allowed_ncc,
            pInput2->robustness_max_allowed_ncc,
            ratio);
        pOutput->robustness_min_allowed_tar_var = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->robustness_min_allowed_tar_var,
            pInput2->robustness_min_allowed_tar_var,
            ratio);
        pOutput->robustness_meaningful_ncc_diff = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->robustness_meaningful_ncc_diff,
            pInput2->robustness_meaningful_ncc_diff,
            ratio);
        pOutput->fpx_threshold = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->fpx_threshold,
            pInput2->fpx_threshold,
            ratio);
        pOutput->desc_match_threshold = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->desc_match_threshold,
            pInput2->desc_match_threshold,
            ratio);
        pOutput->enable_transform_confidence = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->enable_transform_confidence,
            pInput2->enable_transform_confidence,
            ratio);
        pOutput->transform_confidence_mapping_base = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->transform_confidence_mapping_base,
            pInput2->transform_confidence_mapping_base,
            ratio);
        pOutput->transform_confidence_mapping_c1 = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->transform_confidence_mapping_c1,
            pInput2->transform_confidence_mapping_c1,
            ratio);

        pOutput->transform_confidence_mapping_c2 = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->transform_confidence_mapping_c2,
            pInput2->transform_confidence_mapping_c2,
            ratio);
        pOutput->transform_confidence_thr_to_force_identity_transform = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->transform_confidence_thr_to_force_identity_transform,
            pInput2->transform_confidence_thr_to_force_identity_transform,
            ratio);

        for (INT i = 0; i < 8; i++)
        {
            pOutput->robustness_measure_dist_map_tab.robustness_measure_dist_map[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                    pInput1->robustness_measure_dist_map_tab.robustness_measure_dist_map[i],
                    pInput2->robustness_measure_dist_map_tab.robustness_measure_dist_map[i],
                    ratio);
        }
        pOutput->multi_frame_input_resolution = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->multi_frame_input_resolution,
            pInput2->multi_frame_input_resolution,
            ratio);
        pOutput->video_registration_down_scale_ratio = IQSettingUtils::InterpolationFloatBilinear(
            pInput1->video_registration_down_scale_ratio,
            pInput2->video_registration_down_scale_ratio,
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
// CVP10Interpolation::PreScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CVP10Interpolation::PreScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cvp_1_0_0::chromatix_cvp10_coreType*   pParentDataType = NULL;
    CVP10TriggerList*  pTriggerList = NULL;

    if ((NULL != pParentNode) &&
        (NULL != pTriggerData) &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cvp_1_0_0::chromatix_cvp10_coreType*>(pParentNode->pNodeData);
        regionNumber = pParentDataType->mod_cvp10_pre_scale_ratio_dataCount;
        pTriggerList = static_cast<CVP10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_cvp10_pre_scale_ratio_data[count].pre_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerPreScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
            static_cast<VOID*>(&pParentDataType->mod_cvp10_pre_scale_ratio_data[regionOutput.startIndex]),
            NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_cvp10_pre_scale_ratio_data[regionOutput.endIndex]),
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
// CVP10Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CVP10Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cvp_1_0_0::mod_cvp10_pre_scale_ratio_dataType*   pParentDataType = NULL;
    CVP10TriggerList*  pTriggerList = NULL;

    if ((NULL != pParentNode) &&
        (NULL != pTriggerData) &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cvp_1_0_0::mod_cvp10_pre_scale_ratio_dataType*>(pParentNode->pNodeData);
        regionNumber = pParentDataType->pre_scale_ratio_data.mod_cvp10_hdr_aec_dataCount;
        pTriggerList = static_cast<CVP10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->pre_scale_ratio_data.mod_cvp10_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
            static_cast<VOID*>(&pParentDataType->pre_scale_ratio_data.mod_cvp10_hdr_aec_data[regionOutput.startIndex]),
            NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->pre_scale_ratio_data.mod_cvp10_hdr_aec_data[regionOutput.endIndex]),
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
// CVP10Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CVP10Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cvp_1_0_0::mod_cvp10_hdr_aec_dataType*   pParentDataType = NULL;
    CVP10TriggerList*  pTriggerList = NULL;

    if ((NULL != pParentNode) &&
        (NULL != pTriggerData) &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cvp_1_0_0::mod_cvp10_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber = pParentDataType->hdr_aec_data.mod_cvp10_aec_dataCount;
        pTriggerList = static_cast<CVP10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_cvp10_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
            static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cvp10_aec_data[regionOutput.startIndex]),
            static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cvp10_aec_data[regionOutput.startIndex].cvp10_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cvp10_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cvp10_aec_data[regionOutput.endIndex].cvp10_rgn_data));
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
