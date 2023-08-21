// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  sce11interpolation.cpp
/// @brief SCE11 module tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "sce11interpolation.h"

/// @brief This table defines the function of each level of node to search for the child node
static NodeOperation SCE11OperationTable[] =
{
    {SCE11Interpolation::AECSearchNode, 2},
    {SCE11Interpolation::CCTSearchNode, 2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SCE11Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SCE11Interpolation::RunInterpolation(
    const SCE11InputData*          pInput,
    sce_1_1_0::sce11_rgn_dataType* pData)
{
    BOOL result = TRUE;
    UINT count  = 0;

    // The interpolation tree total Node
    TuningNode nodeSet[SCE11MaxNode];

    // SCE Trigger List
    SCE11TriggerList sceTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (count = 0; count < SCE11MaxNode; count++)
        {
            if (count < SCE11MaxNoLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_sce11_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&sceTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(sce_1_1_0::chromatix_sce11Type::control_methodStruct));

        sceTrigger.triggerAEC  =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        SCE11InterpolationLevel,
                                                        &SCE11OperationTable[0],
                                                        static_cast<VOID*>(&sceTrigger));

        if (FALSE != result)
        {
            // Calculate the Interpolation Result
            result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                           SCE11MaxNoLeafNode,
                                                           SCE11InterpolationLevel,
                                                           SCE11Interpolation::DoInterpolation);
            if (FALSE != result)
            {
                IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(sce_1_1_0::sce11_rgn_dataType));
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
/// SCE11Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SCE11Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    sce_1_1_0::sce11_rgn_dataType* pInput1 = NULL;
    sce_1_1_0::sce11_rgn_dataType* pInput2 = NULL;
    sce_1_1_0::sce11_rgn_dataType* pOutput = NULL;

    if ((NULL != pData1) &&
        (NULL != pData2) &&
        (NULL != pResult))
    {
        pInput1 = static_cast<sce_1_1_0::sce11_rgn_dataType*>(pData1);
        pInput2 = static_cast<sce_1_1_0::sce11_rgn_dataType*>(pData2);
        pOutput = static_cast<sce_1_1_0::sce11_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(sce_1_1_0::sce11_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = SCE11Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(sce_1_1_0::sce11_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(sce_1_1_0::sce11_rgn_dataType));
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
/// SCE11Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SCE11Interpolation::InterpolationData(
    sce_1_1_0::sce11_rgn_dataType* pInput1,
    sce_1_1_0::sce11_rgn_dataType* pInput2,
    FLOAT                          ratio,
    sce_1_1_0::sce11_rgn_dataType* pOutput)
{
    BOOL  calculationResult = TRUE;
    FLOAT result            = 0.0f;
    INT   i;

    if ((NULL != pInput1) &&
        (NULL != pInput2) &&
        (NULL != pOutput))
    {
        pOutput->shift_vector_cb = IQSettingUtils::InterpolationFloatBilinear(
                                       pInput1->shift_vector_cb,
                                       pInput2->shift_vector_cb,
                                       ratio);

        pOutput->shift_vector_cr = IQSettingUtils::InterpolationFloatBilinear(
                                       pInput1->shift_vector_cr,
                                       pInput2->shift_vector_cr,
                                       ratio);

        for (i = 0; i < 2; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle1.point1[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle1.point1[i]),
                         ratio);

            pOutput->ori_triangle.traingle1.point1[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle1.point2[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle1.point2[i]),
                         ratio);

            pOutput->ori_triangle.traingle1.point2[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle1.point3[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle1.point3[i]),
                         ratio);

            pOutput->ori_triangle.traingle1.point3[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle2.point1[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle2.point1[i]),
                         ratio);

            pOutput->ori_triangle.traingle2.point1[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle2.point2[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle2.point2[i]),
                         ratio);

            pOutput->ori_triangle.traingle2.point2[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle2.point3[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle2.point3[i]),
                         ratio);

            pOutput->ori_triangle.traingle2.point3[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle3.point1[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle3.point1[i]),
                         ratio);

            pOutput->ori_triangle.traingle3.point1[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle3.point2[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle3.point2[i]),
                         ratio);

            pOutput->ori_triangle.traingle3.point2[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle3.point3[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle3.point3[i]),
                         ratio);

            pOutput->ori_triangle.traingle3.point3[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle4.point1[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle4.point1[i]),
                         ratio);

            pOutput->ori_triangle.traingle4.point1[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle4.point2[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle4.point2[i]),
                         ratio);

            pOutput->ori_triangle.traingle4.point2[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle4.point3[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle4.point3[i]),
                         ratio);

            pOutput->ori_triangle.traingle4.point3[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle5.point1[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle5.point1[i]),
                         ratio);

            pOutput->ori_triangle.traingle5.point1[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle5.point2[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle5.point2[i]),
                         ratio);

            pOutput->ori_triangle.traingle5.point2[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->ori_triangle.traingle5.point3[i]),
                         static_cast<FLOAT>(pInput2->ori_triangle.traingle5.point3[i]),
                         ratio);

            pOutput->ori_triangle.traingle5.point3[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));
        }

        for (i = 0; i < 2; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle1.point1[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle1.point1[i]),
                         ratio);

            pOutput->target_triangle.traingle1.point1[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle1.point2[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle1.point2[i]),
                         ratio);

            pOutput->target_triangle.traingle1.point2[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle1.point3[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle1.point3[i]),
                         ratio);

            pOutput->target_triangle.traingle1.point3[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle2.point1[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle2.point1[i]),
                         ratio);

            pOutput->target_triangle.traingle2.point1[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle2.point2[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle2.point2[i]),
                         ratio);

            pOutput->target_triangle.traingle2.point2[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle2.point3[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle2.point3[i]),
                         ratio);

            pOutput->target_triangle.traingle2.point3[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle3.point1[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle3.point1[i]),
                         ratio);

            pOutput->target_triangle.traingle3.point1[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle3.point2[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle3.point2[i]),
                         ratio);

            pOutput->target_triangle.traingle3.point2[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle3.point3[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle3.point3[i]),
                         ratio);

            pOutput->target_triangle.traingle3.point3[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle4.point1[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle4.point1[i]),
                         ratio);

            pOutput->target_triangle.traingle4.point1[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle4.point2[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle4.point2[i]),
                         ratio);

            pOutput->target_triangle.traingle4.point2[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle4.point3[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle4.point3[i]),
                         ratio);

            pOutput->target_triangle.traingle4.point3[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle5.point1[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle5.point1[i]),
                         ratio);

            pOutput->target_triangle.traingle5.point1[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                         static_cast<FLOAT>(pInput1->target_triangle.traingle5.point2[i]),
                         static_cast<FLOAT>(pInput2->target_triangle.traingle5.point2[i]),
                         ratio);

            pOutput->target_triangle.traingle5.point2[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));

            result = IQSettingUtils::InterpolationFloatBilinear(
                        static_cast<FLOAT>(pInput1->target_triangle.traingle5.point3[i]),
                        static_cast<FLOAT>(pInput2->target_triangle.traingle5.point3[i]),
                        ratio);

            pOutput->target_triangle.traingle5.point3[i] = IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(result));
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        calculationResult = FALSE;
    }

    return calculationResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SCE11Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT SCE11Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    sce_1_1_0::chromatix_sce11_coreType*   pParentDataType = NULL;
    SCE11TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<sce_1_1_0::chromatix_sce11_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_sce11_aec_dataCount;
        pTriggerList    = static_cast<SCE11TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->mod_sce11_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_sce11_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_sce11_aec_data[regionOutput.endIndex]),
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
// SCE11Interpolation::CCTSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT SCE11Interpolation::CCTSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    sce_1_1_0::mod_sce11_aec_dataType*   pParentDataType = NULL;
    SCE11TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<sce_1_1_0::mod_sce11_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->aec_data.mod_sce11_cct_dataCount;
        pTriggerList    = static_cast<SCE11TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->aec_data.mod_sce11_cct_data[count].cct_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerCCT;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->aec_data.mod_sce11_cct_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->aec_data.mod_sce11_cct_data[regionOutput.startIndex].sce11_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->aec_data.mod_sce11_cct_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->aec_data.mod_sce11_cct_data[regionOutput.endIndex].sce11_rgn_data));
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
