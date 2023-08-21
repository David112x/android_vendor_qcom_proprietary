// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  tintless20interpolation.cpp
/// @brief Tintless20 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "tintless20interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation Tintless20OperationTable[] =
{
    { Tintless20Interpolation::HDRAECSearchNode, 2},
    { Tintless20Interpolation::AECSearchNode,    2},
};

static const UINT32 Tintless20MaxmiumNode        = 7;  // (1 + 1 * 2 + 2 * 2)
static const UINT32 Tintless20MaxmiumNonLeafNode = 3;  // (1 + 1 * 2)
static const UINT32 Tintless20InterpolationLevel = 3;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Tintless20Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Tintless20Interpolation::RunInterpolation(
    const Tintless20InterpolationInput*      pInput,
    tintless_2_0_0::tintless20_rgn_dataType* pData)
{
    BOOL                           result = TRUE;
    // The interpolotion tree total Node
    TuningNode                     nodeSet[Tintless20MaxmiumNode];
    // Except leaf node, each valid node needs a scratch buffer to contain intepolation result
    tintless_2_0_0::tintless20_rgn_dataType* pDataBuffers = NULL;
    // Tintless Trigger List
    Tintless20TriggerList               tintlessTrigger;

    pDataBuffers = static_cast<tintless_2_0_0::tintless20_rgn_dataType*>(
        CAMX_CALLOC(sizeof(tintless_2_0_0::tintless20_rgn_dataType) * Tintless20MaxmiumNonLeafNode));

    if ((NULL != pInput) && (NULL != pData) && (NULL != pDataBuffers))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < Tintless20MaxmiumNode; count++)
        {
            if (count < Tintless20MaxmiumNonLeafNode)
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], &pDataBuffers[count]);
            }
            else
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], NULL);
            }
        }

        // Setup the Top Node
        IQSettingUtils::AddNodeToInterpolationTree(NULL, &nodeSet[0],
            static_cast<VOID*>(&pInput->pTintlessChromatix->chromatix_tintless20_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&tintlessTrigger.controlType,
                               &(pInput->pTintlessChromatix->control_method),
                               sizeof(tintless_2_0_0::chromatix_tintless20Type::control_methodStruct));

        tintlessTrigger.triggerHDRAEC = IQSettingUtils::GettriggerHDRAEC(
            pInput->pTintlessChromatix->control_method.aec_hdr_control,
            pInput->exposureTime,
            pInput->AECSensitivity,
            pInput->exposureGainRatio);

        tintlessTrigger.triggerAEC = IQSettingUtils::GettriggerAEC(pInput->pTintlessChromatix->control_method.aec_exp_control,
                                                                   pInput->luxIndex,
                                                                   pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        Tintless20InterpolationLevel,
                                                        &Tintless20OperationTable[0],
                                                        static_cast<VOID*>(&tintlessTrigger));
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
                                                       Tintless20MaxmiumNonLeafNode,
                                                       Tintless20InterpolationLevel,
                                                       Tintless20Interpolation::DoInterpolation);
    }

    if (FALSE != result)
    {
        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(tintless_2_0_0::tintless20_rgn_dataType));
    }


    if (NULL != pDataBuffers)
    {
        CAMX_FREE(pDataBuffers);
        pDataBuffers = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Tintless20Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Tintless20Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    tintless_2_0_0::tintless20_rgn_dataType* pInput1 = NULL;
    tintless_2_0_0::tintless20_rgn_dataType* pInput2 = NULL;
    tintless_2_0_0::tintless20_rgn_dataType* pOutput = NULL;
    BOOL                                     result  = TRUE;

    if (NULL != pData1 && NULL != pData2 && NULL != pResult)
    {
        /// pInput1 and pInput2 are the Data regions and Data region 2
        ///    Region1           Interpolation     Region2
        ///    ---------- | ------------------- | ----------
        ///    ratio= 0.0......ratio (>0 && <1).....ratio = 1.0
        pInput1 =
            static_cast<tintless_2_0_0::tintless20_rgn_dataType*>(pData1);
        pInput2 =
            static_cast<tintless_2_0_0::tintless20_rgn_dataType*>(pData2);
        pOutput =
            static_cast<tintless_2_0_0::tintless20_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(tintless_2_0_0::tintless20_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = Tintless20Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(tintless_2_0_0::tintless20_rgn_dataType));
            }
            else if (IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(tintless_2_0_0::tintless20_rgn_dataType));
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
/// Tintless20Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Tintless20Interpolation::InterpolationData(
    tintless_2_0_0::tintless20_rgn_dataType* pInput1,
    tintless_2_0_0::tintless20_rgn_dataType* pInput2,
    FLOAT                                    ratio,
    tintless_2_0_0::tintless20_rgn_dataType* pOutput)
{
    BOOL result = TRUE;
    pOutput->center_weight =
        IQSettingUtils::InterpolationFloatBilinear(pInput1->center_weight,
                                                   pInput2->center_weight,
                                                   ratio);
    pOutput->corner_weight =
        IQSettingUtils::InterpolationFloatBilinear(pInput1->corner_weight,
                                                   pInput2->corner_weight,
                                                   ratio);
    pOutput->tintless_trace_percentage =
        IQSettingUtils::InterpolationFloatBilinear(pInput1->tintless_trace_percentage,
                                                   pInput2->tintless_trace_percentage,
                                                   ratio);
    pOutput->tintless_high_accuracy_mode =
        IQSettingUtils::InterpolationFloatBilinear(pInput1->tintless_high_accuracy_mode,
                                                   pInput2->tintless_high_accuracy_mode,
                                                   ratio);
    pOutput->tintless_update_delay =
        IQSettingUtils::InterpolationFloatBilinear(pInput1->tintless_update_delay,
                                                   pInput2->tintless_update_delay,
                                                   ratio);
    for (UINT index = 0; index < 16; index++)
    {
        pOutput->tintless_threshold_tab.tintless_threshold[index] =
            IQSettingUtils::InterpolationFloatBilinear(pInput1->tintless_threshold_tab.tintless_threshold[index],
                                                       pInput2->tintless_threshold_tab.tintless_threshold[index],
                                                       ratio);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tintless20Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Tintless20Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tintless_2_0_0::chromatix_tintless20_coreType*   pParentDataType = NULL;
    Tintless20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tintless_2_0_0::chromatix_tintless20_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_tintless20_hdr_aec_dataCount;
        pTriggerList    = static_cast<Tintless20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->mod_tintless20_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_tintless20_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_tintless20_hdr_aec_data[regionOutput.endIndex]),
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
// Tintless20Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Tintless20Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tintless_2_0_0::mod_tintless20_hdr_aec_dataType*   pParentDataType = NULL;
    Tintless20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tintless_2_0_0::mod_tintless20_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_tintless20_aec_dataCount;
        pTriggerList    = static_cast<Tintless20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_tintless20_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_tintless20_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>
               (&pParentDataType->hdr_aec_data.mod_tintless20_aec_data[regionOutput.startIndex].tintless20_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_tintless20_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>
                    (&pParentDataType->hdr_aec_data.mod_tintless20_aec_data[regionOutput.endIndex].tintless20_rgn_data));
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
