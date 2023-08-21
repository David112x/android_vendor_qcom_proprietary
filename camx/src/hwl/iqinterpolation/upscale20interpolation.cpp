// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  upscale20interpolation.cpp
/// @brief upscale20 Tuning Data Interpolation module
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "upscale20interpolation.h"
#include "upscale20setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation Upscale20OperationTable[] =
{
    { Upscale20Interpolation::TotalScaleRatioSearchNode, 2 },
    { Upscale20Interpolation::AECSearchNode,             2 },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Upscale20Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Upscale20Interpolation::RunInterpolation(
    const Upscale20InputData*               pInput,
    upscale_2_0_0::upscale20_rgn_dataType*  pData)
{
    BOOL result = TRUE;

    // The interpolation tree total Node
    TuningNode                            nodeSet[Upscale20MaxNode];
    // CAC Trigger List
    Upscale20TriggerList                  upscaleTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < Upscale20MaxNode; count++)
        {
            if (count < Upscale20MaxNoLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_upscale20_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&upscaleTrigger.controlType,
            &(pInput->pChromatix->control_method),
            sizeof(upscale_2_0_0::chromatix_upscale20Type::control_methodStruct));

        upscaleTrigger.triggerTotalScaleRatio = pInput->totalScaleRatio;
        upscaleTrigger.triggerAEC             =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->AECGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        Upscale20InterpolationLevel,
                                                        &Upscale20OperationTable[0],
                                                        static_cast<VOID*>(&upscaleTrigger));
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    if (TRUE == result)
    {
        // Calculate the Interpolation Result
        result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                       Upscale20MaxNoLeafNode,
                                                       Upscale20InterpolationLevel,
                                                       Upscale20Interpolation::DoInterpolation);

        if (TRUE == result)
        {
            IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(upscale_2_0_0::upscale20_rgn_dataType));
        }
        else
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Upscale20Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Upscale20Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    upscale_2_0_0::upscale20_rgn_dataType* pInput1 =
        static_cast<upscale_2_0_0::upscale20_rgn_dataType*>(pData1);
    upscale_2_0_0::upscale20_rgn_dataType* pInput2 =
        static_cast<upscale_2_0_0::upscale20_rgn_dataType*>(pData2);
    upscale_2_0_0::upscale20_rgn_dataType* pOutput =
        static_cast<upscale_2_0_0::upscale20_rgn_dataType*>(pResult);
    BOOL                                   result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            Upscale20Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(upscale_2_0_0::upscale20_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(upscale_2_0_0::upscale20_rgn_dataType));
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
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Upscale20Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Upscale20Interpolation::InterpolationData(
    upscale_2_0_0::upscale20_rgn_dataType* pInput1,
    upscale_2_0_0::upscale20_rgn_dataType* pInput2,
    FLOAT                                  ratio,
    upscale_2_0_0::upscale20_rgn_dataType* pOutput)
{
    pOutput->sharp_factor = IQSettingUtils::InterpolationFloatBilinear(pInput1->sharp_factor, pInput2->sharp_factor, ratio);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Upscale20Interpolation::TotalScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Upscale20Interpolation::TotalScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    upscale_2_0_0::chromatix_upscale20_coreType*   pParentDataType = NULL;
    Upscale20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<upscale_2_0_0::chromatix_upscale20_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_upscale20_total_scale_ratio_dataCount;
        pTriggerList    = static_cast<Upscale20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_upscale20_total_scale_ratio_data[count].total_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerTotalScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_upscale20_total_scale_ratio_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_upscale20_total_scale_ratio_data[regionOutput.endIndex]),
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
// Upscale20Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Upscale20Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    upscale_2_0_0::mod_upscale20_total_scale_ratio_dataType*   pParentDataType = NULL;
    Upscale20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<upscale_2_0_0::mod_upscale20_total_scale_ratio_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->total_scale_ratio_data.mod_upscale20_aec_dataCount;
        pTriggerList    = static_cast<Upscale20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->total_scale_ratio_data.mod_upscale20_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->total_scale_ratio_data.mod_upscale20_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>
               (&pParentDataType->total_scale_ratio_data.mod_upscale20_aec_data[regionOutput.startIndex].upscale20_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->total_scale_ratio_data.mod_upscale20_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>
                   (&pParentDataType->total_scale_ratio_data.mod_upscale20_aec_data[regionOutput.endIndex].upscale20_rgn_data));
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
