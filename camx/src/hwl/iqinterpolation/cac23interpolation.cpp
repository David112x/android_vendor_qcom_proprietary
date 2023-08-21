// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// @file  cac23interpolation.cpp
// @brief IPE CAC23 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "cac23interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation CAC23OperationTable[] =
{
    { CAC23Interpolation::TotalScaleRatioSearchNode, 2 },
    { CAC23Interpolation::AECSearchNode, 2 },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAC23Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAC23Interpolation::RunInterpolation(
    const CAC23InputData*          pInput,
    cac_2_3_0::cac23_rgn_dataType* pData)
{
    BOOL result = TRUE;

    // The interpolation tree total Node
    TuningNode nodeSet[CAC23MaxNode];

    // CAC Trigger List
    CAC23TriggerList cacTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < CAC23MaxNode; count++)
        {
            if (count < CAC23MaxNoLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_cac23_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&cacTrigger.controlType,
            &(pInput->pChromatix->control_method),
            sizeof(cac_2_3_0::chromatix_cac23Type::control_methodStruct));

        cacTrigger.triggerTotalScaleRatio = pInput->totalScaleRatio;
        cacTrigger.triggerAEC             =
            IQSettingUtils::GettriggerAEC(
                pInput->pChromatix->control_method.aec_exp_control,
                pInput->luxIndex,
                pInput->digitalGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(
            &nodeSet[0], CAC23InterpolationLevel, &CAC23OperationTable[0], static_cast<VOID*>(&cacTrigger));
    }
    else
    {
        // @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    if (FALSE != result)
    {
        // Calculate the Interpolation Result
        result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
            CAC23MaxNoLeafNode,
            CAC23InterpolationLevel,
            CAC23Interpolation::DoInterpolation);
    }

    if (FALSE != result)
    {
        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(cac_2_3_0::cac23_rgn_dataType));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAC23Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAC23Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        cac_2_3_0::cac23_rgn_dataType* pInput1 =
            static_cast<cac_2_3_0::cac23_rgn_dataType*>(pData1);
        cac_2_3_0::cac23_rgn_dataType* pInput2 =
            static_cast<cac_2_3_0::cac23_rgn_dataType*>(pData2);
        cac_2_3_0::cac23_rgn_dataType* pOutput =
            static_cast<cac_2_3_0::cac23_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(cac_2_3_0::cac23_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            CAC23Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(cac_2_3_0::cac23_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(cac_2_3_0::cac23_rgn_dataType));
            }
            else
            {
                result = FALSE;
                // @todo (CAMX-1812) Need to add logging for Common library
            }
        }
    }
    else
    {
        result = FALSE;
        // @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAC23Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAC23Interpolation::InterpolationData(
    cac_2_3_0::cac23_rgn_dataType* pInput1,
    cac_2_3_0::cac23_rgn_dataType* pInput2,
    FLOAT                          ratio,
    cac_2_3_0::cac23_rgn_dataType* pOutput)
{
    FLOAT result = 0.0f;

    // if the ratio is in between 0.0 and 1.0,
    // it means the trigger value is in between region[index].end and region[index+1].start
    // in this case cac_en is binary flags which is always slected first region data in chromatix
    result                    = (ratio > 0.0f) ? pInput1->cac_en: pInput2->cac_en;

    pOutput->cac_en           = static_cast<FLOAT>(IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result)));

    result                    = IQSettingUtils::InterpolationFloatBilinear(
                                    pInput1->y_spot_thr,
                                    pInput2->y_spot_thr,
                                    ratio);
    pOutput->y_spot_thr       = static_cast<FLOAT>(IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result)));

    result                    = IQSettingUtils::InterpolationFloatBilinear(
                                    pInput1->y_saturation_thr,
                                    pInput2->y_saturation_thr,
                                    ratio);
    pOutput->y_saturation_thr = static_cast<FLOAT>(IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result)));

    result                    = IQSettingUtils::InterpolationFloatBilinear(
                                    pInput1->c_spot_thr,
                                    pInput2->c_spot_thr,
                                    ratio);
    pOutput->c_spot_thr       = static_cast<FLOAT>(IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result)));

    result                    = IQSettingUtils::InterpolationFloatBilinear(
                                    pInput1->c_saturation_thr,
                                    pInput2->c_saturation_thr,
                                    ratio);
    pOutput->c_saturation_thr = static_cast<FLOAT>(IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAC23Interpolation::TotalScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CAC23Interpolation::TotalScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cac_2_3_0::chromatix_cac23_coreType*   pParentDataType = NULL;
    CAC23TriggerList*  pTriggerList = NULL;

    if ((NULL != pParentNode) &&
        (NULL != pTriggerData) &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cac_2_3_0::chromatix_cac23_coreType*>(pParentNode->pNodeData);
        regionNumber = pParentDataType->mod_cac23_total_scale_ratio_dataCount;
        pTriggerList = static_cast<CAC23TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_cac23_total_scale_ratio_data[count].total_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerTotalScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
            static_cast<VOID*>(&pParentDataType->mod_cac23_total_scale_ratio_data[regionOutput.startIndex]),
            NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_cac23_total_scale_ratio_data[regionOutput.endIndex]),
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
// CAC23Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CAC23Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cac_2_3_0::mod_cac23_total_scale_ratio_dataType*   pParentDataType = NULL;
    CAC23TriggerList*  pTriggerList = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cac_2_3_0::mod_cac23_total_scale_ratio_dataType*>(pParentNode->pNodeData);
        regionNumber = pParentDataType->total_scale_ratio_data.mod_cac23_aec_dataCount;
        pTriggerList    = static_cast<CAC23TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {

            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->total_scale_ratio_data.mod_cac23_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
            static_cast<VOID*>(
                &pParentDataType->total_scale_ratio_data.mod_cac23_aec_data[regionOutput.startIndex]),
            static_cast<VOID*>(
                &pParentDataType->total_scale_ratio_data.mod_cac23_aec_data[regionOutput.startIndex].cac23_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(
                    &pParentDataType->total_scale_ratio_data.mod_cac23_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(
                    &pParentDataType->total_scale_ratio_data.mod_cac23_aec_data[regionOutput.endIndex].cac23_rgn_data));

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
