// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  bc10interpolation.cpp
/// @brief bc10 tunning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bc10interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation BC10OperationTable[] =
{
    {BC10Interpolation::PostScaleRatioSearchNode, 2},
    {BC10Interpolation::PreScaleRatioSearchNode,  2},
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BC10Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BC10Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    BC10InputData*    pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->postScaleRatio, pInput->postScaleRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->preScaleRatio, pInput->preScaleRatio)))
    {
        pTriggerData->postScaleRatio = pInput->postScaleRatio;
        pTriggerData->preScaleRatio  = pInput->preScaleRatio;

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BC10Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BC10Interpolation::RunInterpolation(
    const BC10InputData*                   pInput,
    bincorr_1_0_0::bincorr10_rgn_dataType* pData)
{
    BOOL                         result = TRUE;
    UINT                         count = 0;
    TuningNode                   nodeSet[BC10MaxmiumNode];
    BC10TriggerList              bcTrigger;

    if ((NULL != pInput) && (NULL != pData)  && (NULL != pInput->pChromatixBC))
    {
        // Initialize all the nodes
        for (count = 0; count < BC10MaxmiumNode; count++)
        {
            if (count < BC10MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatixBC->chromatix_bincorr10_core), NULL);


        bcTrigger.triggerPostScaleRatio = pInput->postScaleRatio;
        bcTrigger.triggerPreScaleRatio  = pInput->preScaleRatio;

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        BC10InterpolationLevel,
                                                        &BC10OperationTable[0],
                                                        static_cast<VOID*>(&bcTrigger));
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
                                                       BC10MaxmiumNonLeafNode,
                                                       BC10InterpolationLevel,
                                                       BC10Interpolation::DoInterpolation);
        if (TRUE == result)
        {
            IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(bincorr_1_0_0::bincorr10_rgn_dataType));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BC10Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BC10Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    bincorr_1_0_0::bincorr10_rgn_dataType* pInput1 = NULL;
    bincorr_1_0_0::bincorr10_rgn_dataType* pInput2 = NULL;
    bincorr_1_0_0::bincorr10_rgn_dataType* pOutput = NULL;
    BOOL                                   result  = TRUE;

    // ....Region1           Interpolation     Region2
    //    ---------- | ------------------- | ----------
    // ....ratio= 0.0......ratio (>0 && <1).....ratio = 1.0

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        pInput1 = static_cast<bincorr_1_0_0::bincorr10_rgn_dataType*>(pData1);
        pInput2 = static_cast<bincorr_1_0_0::bincorr10_rgn_dataType*>(pData2);
        pOutput = static_cast<bincorr_1_0_0::bincorr10_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(bincorr_1_0_0::bincorr10_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = BC10Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(bincorr_1_0_0::bincorr10_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(bincorr_1_0_0::bincorr10_rgn_dataType));
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
/// BC10Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BC10Interpolation::InterpolationData(
    bincorr_1_0_0::bincorr10_rgn_dataType* pInput1,
    bincorr_1_0_0::bincorr10_rgn_dataType* pInput2,
    FLOAT                                  ratio,
    bincorr_1_0_0::bincorr10_rgn_dataType* pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        pOutput->hor_bin_corr_w1 = IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hor_bin_corr_w1),
                static_cast<FLOAT>(pInput2->hor_bin_corr_w1), ratio));
        pOutput->hor_bin_corr_w2 = IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->hor_bin_corr_w2),
                static_cast<FLOAT>(pInput2->hor_bin_corr_w2), ratio));
        pOutput->ver_bin_corr_w1 = IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->ver_bin_corr_w1),
                static_cast<FLOAT>(pInput2->ver_bin_corr_w1), ratio));
        pOutput->ver_bin_corr_w2 = IQSettingUtils::RoundFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->ver_bin_corr_w2),
                static_cast<FLOAT>(pInput2->ver_bin_corr_w2), ratio));
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BC10Interpolation::PostScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT BC10Interpolation::PostScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    bincorr_1_0_0::chromatix_bincorr10_coreType*   pParentDataType = NULL;
    BC10TriggerList*  pTriggerList                                 = NULL;

    if ((NULL != pParentNode) && (NULL != pTriggerData) && (NULL != pChildNode))
    {
        pParentDataType = static_cast<bincorr_1_0_0::chromatix_bincorr10_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_bincorr10_post_scale_ratio_dataCount;
        pTriggerList    = static_cast<BC10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_bincorr10_post_scale_ratio_data[count].post_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerPostScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_bincorr10_post_scale_ratio_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_bincorr10_post_scale_ratio_data[regionOutput.endIndex]),
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
// BC10Interpolation::PreScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT BC10Interpolation::PreScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    bincorr_1_0_0::mod_bincorr10_post_scale_ratio_dataType*   pParentDataType = NULL;
    BC10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode) && (NULL != pTriggerData) && (NULL != pChildNode))
    {
        pParentDataType = static_cast<bincorr_1_0_0::mod_bincorr10_post_scale_ratio_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->post_scale_ratio_data.mod_bincorr10_pre_scale_ratio_dataCount;
        pTriggerList    = static_cast<BC10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->post_scale_ratio_data.mod_bincorr10_pre_scale_ratio_data[count].pre_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerPreScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->post_scale_ratio_data.mod_bincorr10_pre_scale_ratio_data[
               regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->post_scale_ratio_data.mod_bincorr10_pre_scale_ratio_data[
               regionOutput.startIndex].bincorr10_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>
                (&pParentDataType->post_scale_ratio_data.mod_bincorr10_pre_scale_ratio_data[regionOutput.endIndex]),
                (&pParentDataType->post_scale_ratio_data.mod_bincorr10_pre_scale_ratio_data[
                    regionOutput.endIndex].bincorr10_rgn_data));
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
