// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ica30interpolation.cpp
/// @brief IPE ica30 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ica30interpolation.h"
#include "icasetting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation ICA30OperationTable[] =
{
    { ICA30Interpolation::LensPositionSearchNode, 2 },
    { ICA30Interpolation::LensZoomSearchNode, 2 },
    { ICA30Interpolation::AECSearchNode, 2 },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICA30Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ICA30Interpolation::RunInterpolation(
    const ICAInputData*            pInput,
    ica_3_0_0::ica30_rgn_dataType* pData)
{
    BOOL result = TRUE;
    // The interpolation tree total Node
    TuningNode nodeSet[ICAMaxNode];

    // ICA Trigger List
    ICA30TriggerList icaTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        ica_3_0_0::chromatix_ica30Type* pChromatix = static_cast<ica_3_0_0::chromatix_ica30Type*>(pInput->pChromatix);

        // Initialize all the nodes
        for (UINT count = 0; count < ICAMaxNode; count++)
        {
            if (count < ICAMaxNoLeafNode)
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
                                                   static_cast<VOID*>(&pChromatix->chromatix_ica30_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&icaTrigger.controlType,
                               &(pChromatix->control_method),
                               sizeof(ica_3_0_0::chromatix_ica30Type::control_methodStruct));

        icaTrigger.triggerLensPosition = pInput->lensPosition;
        icaTrigger.triggerLensZoom     = pInput->lensZoomRatio;
        icaTrigger.triggerAEC          =
            IQSettingUtils::GettriggerAEC(pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->digitalGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(
            &nodeSet[0], ICAInterpolationLevel, &ICA30OperationTable[0], static_cast<VOID*>(&icaTrigger));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid input data %p, input %p",
                       pData, pInput);
        result = FALSE;
    }

    if (FALSE != result)
    {
        // Calculate the Interpolation Result
        result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                       ICAMaxNoLeafNode,
                                                       ICAInterpolationLevel,
                                                       ICA30Interpolation::DoInterpolation);
    }

    if (FALSE != result)
    {
        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(ica_3_0_0::ica30_rgn_dataType));
    }

    if (FALSE == result)
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "InterpolateTuningData result failed %d", result);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICA30Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ICA30Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        ica_3_0_0::ica30_rgn_dataType* pInput1 =
            static_cast<ica_3_0_0::ica30_rgn_dataType*>(pData1);
        ica_3_0_0::ica30_rgn_dataType* pInput2 =
            static_cast<ica_3_0_0::ica30_rgn_dataType*>(pData2);
        ica_3_0_0::ica30_rgn_dataType* pOutput =
            static_cast<ica_3_0_0::ica30_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(ica_3_0_0::ica30_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = ICA30Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(ica_3_0_0::ica30_rgn_dataType));
            }
            else if (IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(ica_3_0_0::ica30_rgn_dataType));
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "ivalid ratio %f", ratio);
                result = FALSE;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "ivalid data1 %p, data2 %p, result %p",
            pData1, pData2, pResult);
        result = FALSE;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ICA30Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ICA30Interpolation::InterpolationData(
    ica_3_0_0::ica30_rgn_dataType* pInput1,
    ica_3_0_0::ica30_rgn_dataType* pInput2,
    FLOAT                          ratio,
    ica_3_0_0::ica30_rgn_dataType* pOutput)
{
    UINT  i;
    FLOAT result = 0.0f;
    BOOL  resultAPI = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        pOutput->y_interpolation_type = IQSettingUtils::InterpolationFloatNearestNeighbour(
            pInput1->y_interpolation_type, pInput2->y_interpolation_type, ratio);

        for (i = 0; i < ICA30GridRegSize; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->ctc_grid_x_tab.ctc_grid_x[i],
                pInput2->ctc_grid_x_tab.ctc_grid_x[i],
                ratio);
            pOutput->ctc_grid_x_tab.ctc_grid_x[i] = result;

            result = IQSettingUtils::InterpolationFloatBilinear(
                pInput1->ctc_grid_y_tab.ctc_grid_y[i],
                pInput2->ctc_grid_y_tab.ctc_grid_y[i],
                ratio);
            pOutput->ctc_grid_y_tab.ctc_grid_y[i] = result;
        }

        for (i = 0; i < ICAInterpolationCoeffSets; i++)
        {
            result = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1->opg_interpolation_lut_0_tab.opg_interpolation_lut_0[i],
                pInput2->opg_interpolation_lut_0_tab.opg_interpolation_lut_0[i],
                ratio);
            pOutput->opg_interpolation_lut_0_tab.opg_interpolation_lut_0[i] = result;

            result = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1->opg_interpolation_lut_1_tab.opg_interpolation_lut_1[i],
                pInput2->opg_interpolation_lut_1_tab.opg_interpolation_lut_1[i],
                ratio);
            pOutput->opg_interpolation_lut_1_tab.opg_interpolation_lut_1[i] = result;

            result = IQSettingUtils::InterpolationFloatNearestNeighbour(
                pInput1->opg_interpolation_lut_2_tab.opg_interpolation_lut_2[i],
                pInput2->opg_interpolation_lut_2_tab.opg_interpolation_lut_2[i],
                ratio);
            pOutput->opg_interpolation_lut_2_tab.opg_interpolation_lut_2[i] = result;
        }
    }
    return resultAPI;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ICA30Interpolation::LensPositionSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ICA30Interpolation::LensPositionSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    ica_3_0_0::chromatix_ica30_coreType*   pParentDataType = NULL;
    ICA30TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<ica_3_0_0::chromatix_ica30_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_ica30_lens_posn_dataCount;
        pTriggerList    = static_cast<ICA30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_ica30_lens_posn_data[count].lens_posn_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerLensPosition;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_ica30_lens_posn_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_ica30_lens_posn_data[regionOutput.endIndex]),
                NULL);
            childCount++;
        }

        pParentNode->numChild = childCount;
    }
    else
    {
        childCount = 0;
    }

    return childCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ICA30Interpolation::LensZoomSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ICA30Interpolation::LensZoomSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    ica_3_0_0::mod_ica30_lens_posn_dataType*   pParentDataType = NULL;
    ICA30TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<ica_3_0_0::mod_ica30_lens_posn_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->lens_posn_data.mod_ica30_lens_zoom_dataCount;
        pTriggerList    = static_cast<ICA30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->lens_posn_data.mod_ica30_lens_zoom_data[count].lens_zoom_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerLensZoom;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->lens_posn_data.mod_ica30_lens_zoom_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->lens_posn_data.mod_ica30_lens_zoom_data[regionOutput.endIndex]),
                NULL);
            childCount++;
        }

        pParentNode->numChild = childCount;
    }
    else
    {
        childCount = 0;
    }

    return childCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ICA30Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ICA30Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    ica_3_0_0::mod_ica30_lens_zoom_dataType*   pParentDataType = NULL;
    ICA30TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<ica_3_0_0::mod_ica30_lens_zoom_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->lens_zoom_data.mod_ica30_aec_dataCount;
        pTriggerList    = static_cast<ICA30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->lens_zoom_data.mod_ica30_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->lens_zoom_data.mod_ica30_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->lens_zoom_data.mod_ica30_aec_data[regionOutput.startIndex].ica30_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->lens_zoom_data.mod_ica30_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->lens_zoom_data.mod_ica30_aec_data[regionOutput.endIndex].ica30_rgn_data));
            childCount++;
        }

        pParentNode->numChild = childCount;
    }
    else
    {
        childCount = 0;
    }

    return childCount;
};
