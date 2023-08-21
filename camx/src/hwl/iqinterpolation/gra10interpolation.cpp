// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gra10interpolation.cpp
/// @brief gra10 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gra10setting.h"
#include "gra10interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation GRA10OperationTable[] =
{
    { GRA10Interpolation::PreScaleRatioSearchNode, 2 },
    { GRA10Interpolation::AECSearchNode, 2 },
    { GRA10Interpolation::CCTSearchNode, 2 }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GRA10Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GRA10Interpolation::CheckUpdateTrigger(
    const ISPIQTriggerData* pInput,
    GRA10IQInput*           pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                 ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->linearGain, pInput->AECGain))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->preScaleRatio, pInput->preScaleRatio))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->colorTemperature, pInput->AWBColorTemperature)))
    {
        pTriggerData->luxIndex         = pInput->AECLuxIndex;
        pTriggerData->linearGain       = pInput->AECGain;
        pTriggerData->preScaleRatio    = pInput->preScaleRatio;
        pTriggerData->colorTemperature = pInput->AWBColorTemperature;

        isChanged = TRUE;
    }

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GRA10Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GRA10Interpolation::RunInterpolation(
    const GRA10IQInput*            pInput,
    gra_1_0_0::gra10_rgn_dataType* pData)
{
    BOOL                          result = TRUE;
    UINT                          count = 0;

    // The interpolation tree total Node
    TuningNode                    nodeSet[GRA10MaxNode];

    // GRA Trigger List
    GRA10TriggerList              GRATrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (count = 0; count < GRA10MaxNode; count++)
        {
            if (count < GRA10MaxNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_gra10_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&GRATrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(gra_1_0_0::chromatix_gra10Type::control_methodStruct));

        GRATrigger.preScaleRatioTrigger = pInput->preScaleRatio;

        GRATrigger.triggerAEC           = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                                        pInput->luxIndex,
                                                                        pInput->linearGain);

        GRATrigger.triggerCCT           = pInput->colorTemperature;

        // Set up Interpolation Tree
        result                          = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                                                 GRA10InterpolationLevel,
                                                                                 &GRA10OperationTable[0],
                                                                                 static_cast<VOID*>(&GRATrigger));
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
                                                       GRA10MaxNonLeafNode,
                                                       GRA10InterpolationLevel,
                                                       GRA10Interpolation::DoInterpolation);
    }

    if (FALSE != result)
    {
        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(gra_1_0_0::gra10_rgn_dataType));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GRA10Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GRA10Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    gra_1_0_0::gra10_rgn_dataType* pInput1 = NULL;
    gra_1_0_0::gra10_rgn_dataType* pInput2 = NULL;
    gra_1_0_0::gra10_rgn_dataType* pOutput = NULL;
    BOOL result                            = TRUE;


    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        pInput1 = static_cast<gra_1_0_0::gra10_rgn_dataType*>(pData1);
        pInput2 = static_cast<gra_1_0_0::gra10_rgn_dataType*>(pData2);
        pOutput = static_cast<gra_1_0_0::gra10_rgn_dataType*>(pResult);

        if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            GRA10Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(gra_1_0_0::gra10_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(gra_1_0_0::gra10_rgn_dataType));
            }
            else
            {
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
/// GRA10Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GRA10Interpolation::InterpolationData(
    gra_1_0_0::gra10_rgn_dataType* pInput1,
    gra_1_0_0::gra10_rgn_dataType* pInput2,
    FLOAT                          ratio,
    gra_1_0_0::gra10_rgn_dataType* pOutput)
{
    FLOAT  result = 0.0f;
    UINT32 i      = 0;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        result = IQSettingUtils::InterpolationFloatBilinear(pInput1->grain_strength,
                                                            pInput2->grain_strength,
                                                            ratio);

        pOutput->grain_strength = result;

        for (i = 0; i < GRA10LUTNumEntriesPerChannel; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(pInput1->cb_weight_lut_tab.cb_weight_lut[i],
                                                                pInput2->cb_weight_lut_tab.cb_weight_lut[i],
                                                                ratio);

            pOutput->cb_weight_lut_tab.cb_weight_lut[i] = result;
        }

        for (i = 0; i < GRA10LUTNumEntriesPerChannel; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(pInput1->cr_weight_lut_tab.cr_weight_lut[i],
                                                                pInput2->cr_weight_lut_tab.cr_weight_lut[i],
                                                                ratio);
            pOutput->cr_weight_lut_tab.cr_weight_lut[i] = result;
        }

        for (i = 0; i < GRA10LUTNumEntriesPerChannel; i++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(pInput1->y_weight_lut_tab.y_weight_lut[i],
                                                                pInput2->y_weight_lut_tab.y_weight_lut[i],
                                                                ratio);
            pOutput->y_weight_lut_tab.y_weight_lut[i]   = result;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GRA10Interpolation::PreScaleRatioSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT GRA10Interpolation::PreScaleRatioSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gra_1_0_0::chromatix_gra10_coreType*   pParentDataType = NULL;
    GRA10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gra_1_0_0::chromatix_gra10_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_gra10_pre_scale_ratio_dataCount;
        pTriggerList    = static_cast<GRA10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_gra10_pre_scale_ratio_data[count].pre_scale_ratio_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->preScaleRatioTrigger;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_gra10_pre_scale_ratio_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_gra10_pre_scale_ratio_data[regionOutput.endIndex]),
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
// GRA10Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT GRA10Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gra_1_0_0::mod_gra10_pre_scale_ratio_dataType*   pParentDataType = NULL;
    GRA10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gra_1_0_0::mod_gra10_pre_scale_ratio_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->pre_scale_ratio_data.mod_gra10_aec_dataCount;
        pTriggerList    = static_cast<GRA10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->pre_scale_ratio_data.mod_gra10_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->pre_scale_ratio_data.mod_gra10_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->pre_scale_ratio_data.mod_gra10_aec_data[regionOutput.endIndex]),
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
// GRA10Interpolation::CCTSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT GRA10Interpolation::CCTSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gra_1_0_0::mod_gra10_aec_dataType*   pParentDataType = NULL;
    GRA10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gra_1_0_0::mod_gra10_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->aec_data.mod_gra10_cct_dataCount;
        pTriggerList    = static_cast<GRA10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->aec_data.mod_gra10_cct_data[count].cct_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerCCT;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->aec_data.mod_gra10_cct_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->aec_data.mod_gra10_cct_data[regionOutput.startIndex].gra10_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->aec_data.mod_gra10_cct_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->aec_data.mod_gra10_cct_data[regionOutput.endIndex].gra10_rgn_data));
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
