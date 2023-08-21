// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  hdr30interpolation.cpp
/// @brief HDR30 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "hdr30interpolation.h"
#include "hdr30setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation HDR30OperationTable[] =
{
    {HDR30Interpolation::HDRAECSearchNode, 2},
    {HDR30Interpolation::AECSearchNode,    2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDR30Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HDR30Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    HDR30InputData*   pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECGain, pInput->AECGain))                        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftBGainWB, pInput->AWBleftBGainWB))             ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftGGainWB, pInput->AWBleftGGainWB))             ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftRGainWB, pInput->AWBleftRGainWB)))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->AECGain           = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->leftBGainWB       = pInput->AWBleftBGainWB;
        pTriggerData->leftGGainWB       = pInput->AWBleftGGainWB;
        pTriggerData->leftRGainWB       = pInput->AWBleftRGainWB;

        isChanged = TRUE;
    }

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDR30Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HDR30Interpolation::RunInterpolation(
    const HDR30InputData*          pInput,
    hdr_3_0_0::hdr30_rgn_dataType* pData)
{
    BOOL                result = TRUE;
    TuningNode          nodeSet[HDR30MaxmiumNode];  // The intepolation tree total Node
    HDR30TriggerList    HDRTrigger;                 // HDR Trigger List

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < HDR30MaxmiumNode; count++)
        {
            if (count < HDR30MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_hdr30_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&HDRTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(hdr_3_0_0::chromatix_hdr30Type::control_methodStruct));

        HDRTrigger.triggerHDRAEC =
            IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                             pInput->exposureTime,
                                             pInput->AECSensitivity,
                                             pInput->exposureGainRatio);
        HDRTrigger.triggerAEC    =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->AECGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree( &nodeSet[0],
                                                         HDR30InterpolationLevel,
                                                         &HDR30OperationTable[0],
                                                         static_cast<VOID*>(&HDRTrigger));
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
                                                       HDR30MaxmiumNonLeafNode,
                                                       HDR30InterpolationLevel,
                                                       HDR30Interpolation::DoInterpolation);
        if (FALSE != result)
        {
            IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(hdr_3_0_0::hdr30_rgn_dataType));
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDR30Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HDR30Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        hdr_3_0_0::hdr30_rgn_dataType* pInput1 = static_cast<hdr_3_0_0::hdr30_rgn_dataType*>(pData1);
        hdr_3_0_0::hdr30_rgn_dataType* pInput2 = static_cast<hdr_3_0_0::hdr30_rgn_dataType*>(pData2);
        hdr_3_0_0::hdr30_rgn_dataType* pOutput = static_cast<hdr_3_0_0::hdr30_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(hdr_3_0_0::hdr30_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = HDR30Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(hdr_3_0_0::hdr30_rgn_dataType));
            }
            else if (IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(hdr_3_0_0::hdr30_rgn_dataType));
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
// HDR30Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HDR30Interpolation::InterpolationData(
    hdr_3_0_0::hdr30_rgn_dataType* pInput1,
    hdr_3_0_0::hdr30_rgn_dataType* pInput2,
    FLOAT                          ratio,
    hdr_3_0_0::hdr30_rgn_dataType* pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        pOutput->hdr_mac_static_th2             = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_mac_static_th2),
                                                       static_cast<FLOAT>(pInput2->hdr_mac_static_th2),
                                                       ratio)));

        pOutput->hdr_mac_static_dth_log2        = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_mac_static_dth_log2),
                                                       static_cast<FLOAT>(pInput2->hdr_mac_static_dth_log2),
                                                       ratio)));

        pOutput->hdr_mac_dark_th2               = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_mac_dark_th2),
                                                       static_cast<FLOAT>(pInput2->hdr_mac_dark_th2),
                                                       ratio)));

        pOutput->hdr_mac_motion_strictness      = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_mac_motion_strictness),
                                                       static_cast<FLOAT>(pInput2->hdr_mac_motion_strictness),
                                                       ratio)));

        pOutput->hdr_mac_lowlight_strength      = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_mac_lowlight_strength),
                                                       static_cast<FLOAT>(pInput2->hdr_mac_lowlight_strength),
                                                       ratio)));

        pOutput->hdr_mac_lowlight_dth_log2      = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_mac_lowlight_dth_log2),
                                                       static_cast<FLOAT>(pInput2->hdr_mac_lowlight_dth_log2),
                                                       ratio)));

        pOutput->hdr_rec_hl_detail_positive_w   = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_rec_hl_detail_positive_w),
                                                       static_cast<FLOAT>(pInput2->hdr_rec_hl_detail_positive_w),
                                                       ratio)));

        pOutput->hdr_rec_hl_detail_negative_w   = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_rec_hl_detail_negative_w),
                                                       static_cast<FLOAT>(pInput2->hdr_rec_hl_detail_negative_w),
                                                       ratio)));

        pOutput->hdr_rec_hl_detail_th_w         = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_rec_hl_detail_th_w),
                                                       static_cast<FLOAT>(pInput2->hdr_rec_hl_detail_th_w),
                                                       ratio)));

        pOutput->hdr_rec_hl_motion_th_log2      = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_rec_hl_motion_th_log2),
                                                       static_cast<FLOAT>(pInput2->hdr_rec_hl_motion_th_log2),
                                                       ratio)));

        pOutput->hdr_rec_detail_th_w            = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_rec_detail_th_w),
                                                       static_cast<FLOAT>(pInput2->hdr_rec_detail_th_w),
                                                       ratio)));

        pOutput->hdr_rec_detail_coring_strength = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_rec_detail_coring_strength),
                                                       static_cast<FLOAT>(pInput2->hdr_rec_detail_coring_strength),
                                                       ratio)));

        pOutput->hdr_rec_detail_coring_th2      = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_rec_detail_coring_th2),
                                                       static_cast<FLOAT>(pInput2->hdr_rec_detail_coring_th2),
                                                       ratio)));

        pOutput->hdr_rec_detail_coring_dth_log2 = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_rec_detail_coring_dth_log2),
                                                       static_cast<FLOAT>(pInput2->hdr_rec_detail_coring_dth_log2),
                                                       ratio)));

        pOutput->hdr_mac_motion_dt0             = IQSettingUtils::InterpolationFloatBilinear(pInput1->hdr_mac_motion_dt0,
                                                                                             pInput2->hdr_mac_motion_dt0,
                                                                                             ratio);
        pOutput->hdr_rec_edge_scale             = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
           IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_rec_edge_scale),
                                                      static_cast<FLOAT>(pInput2->hdr_rec_edge_scale),
                                                      ratio)));

        pOutput->hdr_rec_edge_short_exp_w       = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_rec_edge_short_exp_w),
                                                       static_cast<FLOAT>(pInput2->hdr_rec_edge_short_exp_w),
                                                       ratio)));

        pOutput->hdr_rec_edge_th1               = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_rec_edge_th1),
                                                       static_cast<FLOAT>(pInput2->hdr_rec_edge_th1),
                                                       ratio)));

        pOutput->hdr_rec_edge_dth_log2          = static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hdr_rec_edge_dth_log2),
                                                       static_cast<FLOAT>(pInput2->hdr_rec_edge_dth_log2),
                                                       ratio)));

        pOutput->hdr_rec_blending_w_min     = IQSettingUtils::InterpolationFloatBilinear(pInput1->hdr_rec_blending_w_min,
                                                                                         pInput2->hdr_rec_blending_w_min,
                                                                                         ratio);

        pOutput->hdr_rec_blending_w_max     = IQSettingUtils::InterpolationFloatBilinear(pInput1->hdr_rec_blending_w_max,
                                                                                         pInput2->hdr_rec_blending_w_max,
                                                                                         ratio);

        pOutput->hdr_mac_highlight_strength = IQSettingUtils::InterpolationFloatBilinear(pInput1->hdr_mac_highlight_strength,
                                                                                         pInput2->hdr_mac_highlight_strength,
                                                                                         ratio);

        pOutput->hdr_mac_highlight_dth_log2 = IQSettingUtils::InterpolationFloatBilinear(pInput1->hdr_mac_highlight_dth_log2,
                                                                                         pInput2->hdr_mac_highlight_dth_log2,
                                                                                         ratio);

        pOutput->hdr_mac_dilation           = IQSettingUtils::InterpolationFloatBilinear(pInput1->hdr_mac_dilation,
                                                                                         pInput2->hdr_mac_dilation,
                                                                                         ratio);
    }
    else
    {
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDR30Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT HDR30Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    hdr_3_0_0::chromatix_hdr30_coreType*   pParentDataType = NULL;
    HDR30TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode) && (NULL != pTriggerData) && (NULL != pChildNode))
    {
        pParentDataType = static_cast<hdr_3_0_0::chromatix_hdr30_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_hdr30_hdr_aec_dataCount;
        pTriggerList    = static_cast<HDR30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->mod_hdr30_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode,
                                       &pChildNode[childCount],
                                       static_cast<VOID*>(&pParentDataType->mod_hdr30_hdr_aec_data[regionOutput.startIndex]),
                                       NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode,
                                        &pChildNode[childCount],
                                        static_cast<VOID*>(&pParentDataType->mod_hdr30_hdr_aec_data[regionOutput.endIndex]),
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
// HDR30Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT HDR30Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    hdr_3_0_0::mod_hdr30_hdr_aec_dataType*  pParentDataType = NULL;
    HDR30TriggerList*                       pTriggerList    = NULL;

    if ((NULL != pParentNode) && (NULL != pTriggerData) && (NULL != pChildNode))
    {
        pParentDataType = static_cast<hdr_3_0_0::mod_hdr30_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_hdr30_aec_dataCount;
        pTriggerList    = static_cast<HDR30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_hdr30_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode,
            &pChildNode[childCount],
            static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_hdr30_aec_data[regionOutput.startIndex]),
            static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_hdr30_aec_data[regionOutput.startIndex].hdr30_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode,
                &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_hdr30_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_hdr30_aec_data[regionOutput.endIndex].hdr30_rgn_data));
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
