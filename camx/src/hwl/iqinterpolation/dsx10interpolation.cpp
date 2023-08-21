// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  dsx10interpolation.cpp
/// @brief dsx10 IQ setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "dsx10interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation DSX10OperationTable[] =
{
    { DSX10Interpolation::ScaleRatioSearchNode,   2},
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DSX10Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DSX10Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    DSX10InputData*   pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->scaleRatio, pInput->dsxSRScaleRatio)))
    {
        pTriggerData->scaleRatio = pInput->dsxSRScaleRatio;

        isChanged = TRUE;
    }

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DSX10Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DSX10Interpolation::RunInterpolation(
    const DSX10InputData*          pInput,
    dsx_1_0_0::dsx10_rgn_dataType* pData)
{
    BOOL                          result  = TRUE;
    UINT                          count   = 0;

    // The interpolation tree total Node
    TuningNode                    nodeSet[DSX10MaxmiumNode];
    // TMC Trigger List
    DSX10TriggerList              dsxTrigger;

    if ((NULL != pInput) &&
        (NULL != pData)  &&
        (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (count = 0; count < DSX10MaxmiumNode; count++)
        {
            if (count < DSX10MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_dsx10_core), NULL);

        dsxTrigger.triggerScaleRatio = pInput->scaleRatio;

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        DSX10InterpolationLevel,
                                                        &DSX10OperationTable[0],
                                                        static_cast<VOID*>(&dsxTrigger));
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
                                                       DSX10MaxmiumNonLeafNode,
                                                       DSX10InterpolationLevel,
                                                       DSX10Interpolation::DoInterpolation);
        if (TRUE == result)
        {
            IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(dsx_1_0_0::dsx10_rgn_dataType));

            // Calculate ADRC Gain Curve
            // CalcGainCurve(pInput, pData); NCLib here?
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DSX10Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DSX10Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    dsx_1_0_0::dsx10_rgn_dataType* pInput1 = NULL;
    dsx_1_0_0::dsx10_rgn_dataType* pInput2 = NULL;
    dsx_1_0_0::dsx10_rgn_dataType* pOutput = NULL;
    BOOL                           result  = TRUE;

    // ....Region1           Interpolation     Region2
    //    ---------- | ------------------- | ----------
    // ....ratio= 0.0......ratio (>0 && <1).....ratio = 1.0

    if ((NULL != pData1) &&
        (NULL != pData2) &&
        (NULL != pResult))
    {
        pInput1 = static_cast<dsx_1_0_0::dsx10_rgn_dataType*>(pData1);
        pInput2 = static_cast<dsx_1_0_0::dsx10_rgn_dataType*>(pData2);
        pOutput = static_cast<dsx_1_0_0::dsx10_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(dsx_1_0_0::dsx10_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = DSX10Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(dsx_1_0_0::dsx10_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(dsx_1_0_0::dsx10_rgn_dataType));
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
/// dSX10Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DSX10Interpolation::InterpolationData(
    dsx_1_0_0::dsx10_rgn_dataType* pInput1,
    dsx_1_0_0::dsx10_rgn_dataType* pInput2,
    FLOAT                          ratio,
    dsx_1_0_0::dsx10_rgn_dataType* pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput1) &&
        (NULL != pInput2) &&
        (NULL != pOutput))
    {
        for (UINT32 i = 0; i < DSX_CHROMA_KERNAL_WEIGHTS; i++)
        {
            pOutput->chroma_kernel_weights_unpacked_horiz_tab.chroma_kernel_weights_unpacked_horiz[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                    pInput1->chroma_kernel_weights_unpacked_horiz_tab.chroma_kernel_weights_unpacked_horiz[i],
                    pInput2->chroma_kernel_weights_unpacked_horiz_tab.chroma_kernel_weights_unpacked_horiz[i],
                    ratio);
            pOutput->chroma_kernel_weights_unpacked_vert_tab.chroma_kernel_weights_unpacked_vert[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                    pInput1->chroma_kernel_weights_unpacked_vert_tab.chroma_kernel_weights_unpacked_vert[i],
                    pInput2->chroma_kernel_weights_unpacked_vert_tab.chroma_kernel_weights_unpacked_vert[i],
                    ratio);
        }

        for (UINT32 i = 0; i < DSX_CHROMA_PADDING_WEIGHTS; i++)
        {
            pOutput->chroma_padding_weights_bot_tab.chroma_padding_weights_bot[i] = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->chroma_padding_weights_bot_tab.chroma_padding_weights_bot[i],
                pInput2->chroma_padding_weights_bot_tab.chroma_padding_weights_bot[i],
                ratio);
        }

        pOutput->chroma_padding_weights_en = pInput1->chroma_padding_weights_en;

        for (UINT32 i = 0; i < DSX_CHROMA_PADDING_WEIGHTS; i++)
        {
            pOutput->chroma_padding_weights_left_tab.chroma_padding_weights_left[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                pInput1->chroma_padding_weights_left_tab.chroma_padding_weights_left[i],
                pInput2->chroma_padding_weights_left_tab.chroma_padding_weights_left[i],
                ratio);
            pOutput->chroma_padding_weights_right_tab.chroma_padding_weights_right[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                pInput1->chroma_padding_weights_right_tab.chroma_padding_weights_right[i],
                pInput2->chroma_padding_weights_right_tab.chroma_padding_weights_right[i],
                ratio);
            pOutput->chroma_padding_weights_top_tab.chroma_padding_weights_top[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                pInput1->chroma_padding_weights_top_tab.chroma_padding_weights_top[i],
                pInput2->chroma_padding_weights_top_tab.chroma_padding_weights_top[i],
                ratio);
        }

        for (UINT32 i = 0; i < DSX_LUMA_KERNAL_WEIGHTS; i++)
        {
            pOutput->luma_kernel_weights_unpacked_horiz_tab.luma_kernel_weights_unpacked_horiz[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                pInput1->luma_kernel_weights_unpacked_horiz_tab.luma_kernel_weights_unpacked_horiz[i],
                pInput2->luma_kernel_weights_unpacked_horiz_tab.luma_kernel_weights_unpacked_horiz[i],
                ratio);
            pOutput->luma_kernel_weights_unpacked_vert_tab.luma_kernel_weights_unpacked_vert[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                pInput1->luma_kernel_weights_unpacked_vert_tab.luma_kernel_weights_unpacked_vert[i],
                pInput2->luma_kernel_weights_unpacked_vert_tab.luma_kernel_weights_unpacked_vert[i],
                ratio);
        }

        for (UINT32 i = 0; i < DSX_LUMA_PADDING_WEIGHTS; i++)
        {
            pOutput->luma_padding_weights_bot_tab.luma_padding_weights_bot[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                pInput1->luma_padding_weights_bot_tab.luma_padding_weights_bot[i],
                pInput2->luma_padding_weights_bot_tab.luma_padding_weights_bot[i],
                ratio);
        }

        pOutput->luma_padding_weights_en = pInput1->luma_padding_weights_en;

        for (UINT32 i = 0; i < DSX_LUMA_PADDING_WEIGHTS; i++)
        {
            pOutput->luma_padding_weights_left_tab.luma_padding_weights_left[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                pInput1->luma_padding_weights_left_tab.luma_padding_weights_left[i],
                pInput2->luma_padding_weights_left_tab.luma_padding_weights_left[i],
                ratio);
            pOutput->luma_padding_weights_right_tab.luma_padding_weights_right[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                pInput1->luma_padding_weights_right_tab.luma_padding_weights_right[i],
                pInput2->luma_padding_weights_right_tab.luma_padding_weights_right[i],
                ratio);
            pOutput->luma_padding_weights_top_tab.luma_padding_weights_top[i] =
                IQSettingUtils::InterpolationFloatBilinear(
                pInput1->luma_padding_weights_top_tab.luma_padding_weights_top[i],
                pInput2->luma_padding_weights_top_tab.luma_padding_weights_top[i],
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
// DSX10Interpolation::ScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT DSX10Interpolation::ScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    dsx_1_0_0::chromatix_dsx10_coreType*   pParentDataType = NULL;
    DSX10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<dsx_1_0_0::chromatix_dsx10_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_dsx10_sr_scale_ratio_dataCount;
        pTriggerList    = static_cast<DSX10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_dsx10_sr_scale_ratio_data[count].sr_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerScaleRatio;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_dsx10_sr_scale_ratio_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->mod_dsx10_sr_scale_ratio_data[regionOutput.startIndex].dsx10_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_dsx10_sr_scale_ratio_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->mod_dsx10_sr_scale_ratio_data[regionOutput.endIndex].dsx10_rgn_data));
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
