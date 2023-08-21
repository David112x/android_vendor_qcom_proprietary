// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ipe2dlut10interpolation.cpp
/// @brief IPE TDL10 Interpolation calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ipe2dlut10interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation TDL10OperationTable[] =
{
    {IPETDL10Interpolation::DRCGainSearchNode, 2},
    {IPETDL10Interpolation::HDRAECSearchNode,  2},
    {IPETDL10Interpolation::LEDSearchNode,     3},
    {IPETDL10Interpolation::AECSearchNode,     2},
    {IPETDL10Interpolation::CCTSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPETDL10Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPETDL10Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData*  pInput,
    TDL10InputData*    pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain,  pInput->DRCGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->CCTTrigger, pInput->AWBColorTemperature))         ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->LEDFirstEntryRatio, pInput->LEDFirstEntryRatio))  ||
        (pTriggerData->LEDTrigger != pInput->LEDSensitivity))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->realGain          = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->CCTTrigger        = pInput->AWBColorTemperature;
        pTriggerData->DRCGain           = pInput->DRCGain;
        pTriggerData->LEDTrigger        = pInput->LEDSensitivity;
        pTriggerData->numberOfLED       = pInput->numberOfLED;

        pTriggerData->LEDFirstEntryRatio  = pInput->LEDFirstEntryRatio;

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPETDL10Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPETDL10Interpolation::RunInterpolation(
    const TDL10InputData*          pInput,
    tdl_1_0_0::tdl10_rgn_dataType* pData)
{
    BOOL                          result = TRUE;
    TuningNode                    nodeSet[TDL10MaxNode];    // The intepolation tree total Node
    TDL10TriggerList              TDLTrigger;    // TDL10 Trigger List

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < TDL10MaxNode; count++)
        {
            if (count < TDL10MaxNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_tdl10_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&TDLTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(tdl_1_0_0::chromatix_tdl10Type::control_methodStruct));

        TDLTrigger.triggerDRCgain = pInput->DRCGain;
        TDLTrigger.triggerCCT     = pInput->CCTTrigger;
        TDLTrigger.triggerLED     = pInput->LEDTrigger;
        TDLTrigger.numberOfLED    = pInput->numberOfLED;
        TDLTrigger.privateInfo    = pInput->pChromatix->private_information;

        TDLTrigger.LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;

        TDLTrigger.triggerHDRAEC = IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                                                    pInput->exposureTime,
                                                                    pInput->AECSensitivity,
                                                                    pInput->exposureGainRatio);

        TDLTrigger.triggerAEC =  IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                               pInput->luxIndex,
                                                               pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        TDL10InterpolationLevel,
                                                        &TDL10OperationTable[0],
                                                        static_cast<VOID*>(&TDLTrigger));
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
                                                       TDL10MaxNonLeafNode,
                                                       TDL10InterpolationLevel,
                                                       IPETDL10Interpolation::DoInterpolation);
        if (FALSE != result)
        {
            IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(tdl_1_0_0::tdl10_rgn_dataType));
        }
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPETDL10Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPETDL10Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        tdl_1_0_0::tdl10_rgn_dataType* pInput1 = static_cast<tdl_1_0_0::tdl10_rgn_dataType*>(pData1);
        tdl_1_0_0::tdl10_rgn_dataType* pInput2 = static_cast<tdl_1_0_0::tdl10_rgn_dataType*>(pData2);
        tdl_1_0_0::tdl10_rgn_dataType* pOutput = static_cast<tdl_1_0_0::tdl10_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(tdl_1_0_0::tdl10_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            IPETDL10Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(tdl_1_0_0::tdl10_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(tdl_1_0_0::tdl10_rgn_dataType));
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
/// IPETDL10Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPETDL10Interpolation::InterpolationData(
    tdl_1_0_0::tdl10_rgn_dataType* pInput1,
    tdl_1_0_0::tdl10_rgn_dataType* pInput2,
    FLOAT                          ratio,
    tdl_1_0_0::tdl10_rgn_dataType* pOutput)
{
    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        for (UINT i = 0; i < ((H_GRID - 1) * S_GRID); i++)
        {
            pOutput->lut_2d_h_tab.lut_2d_h[i] = IQSettingUtils::InterpolationFloatBilinear(pInput1->lut_2d_h_tab.lut_2d_h[i],
                                                                                           pInput2->lut_2d_h_tab.lut_2d_h[i],
                                                                                           ratio);

            pOutput->lut_2d_s_tab.lut_2d_s[i] = IQSettingUtils::InterpolationFloatBilinear(pInput1->lut_2d_s_tab.lut_2d_s[i],
                                                                                           pInput2->lut_2d_s_tab.lut_2d_s[i],
                                                                                           ratio);
        }

        pOutput->l_boundary_end_a = IQSettingUtils::InterpolationFloatBilinear(pInput1->l_boundary_end_a,
                                                                               pInput2->l_boundary_end_a,
                                                                               ratio);

        pOutput->l_boundary_end_b = IQSettingUtils::InterpolationFloatBilinear(pInput1->l_boundary_end_b,
                                                                               pInput2->l_boundary_end_b,
                                                                               ratio);

        pOutput->l_boundary_start_a = IQSettingUtils::InterpolationFloatBilinear(pInput1->l_boundary_start_a,
                                                                                 pInput2->l_boundary_start_a,
                                                                                 ratio);

        pOutput->l_boundary_start_b = IQSettingUtils::InterpolationFloatBilinear(pInput1->l_boundary_start_b,
                                                                                 pInput2->l_boundary_start_b,
                                                                                 ratio);

        pOutput->y_blend_factor_integer = IQSettingUtils::InterpolationFloatBilinear(pInput1->y_blend_factor_integer,
                                                                                     pInput2->y_blend_factor_integer,
                                                                                     ratio);
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETDL10Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPETDL10Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tdl_1_0_0::chromatix_tdl10_coreType*   pParentDataType = NULL;
    TDL10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tdl_1_0_0::chromatix_tdl10_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_tdl10_drc_gain_dataCount;
        pTriggerList    = static_cast<TDL10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_tdl10_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_tdl10_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_tdl10_drc_gain_data[regionOutput.endIndex]),
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
// IPETDL10Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPETDL10Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tdl_1_0_0::mod_tdl10_drc_gain_dataType*   pParentDataType = NULL;
    TDL10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tdl_1_0_0::mod_tdl10_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_tdl10_hdr_aec_dataCount;
        pTriggerList    = static_cast<TDL10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_tdl10_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_tdl10_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_tdl10_hdr_aec_data[regionOutput.endIndex]),
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
// IPETDL10Interpolation::LEDSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPETDL10Interpolation::LEDSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    UINT32 regionNumber = 0;
    FLOAT  rationLED2   = 0.0f;
    FLOAT  triggerValue = 0.0f;

    InterpolationOutput regionOutput;

    tdl_1_0_0::mod_tdl10_hdr_aec_dataType*   pParentDataType = NULL;
    TDL10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tdl_1_0_0::mod_tdl10_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_tdl10_led_idx_dataCount;
        pTriggerList    = static_cast<TDL10TriggerList*>(pTriggerData);
        triggerValue    = pTriggerList->triggerLED;

        if ((0 ==  pTriggerList->numberOfLED) || (1 == regionNumber))
        {
            regionOutput.startIndex         = 0;
            regionOutput.endIndex           = 0;
            regionOutput.interpolationRatio = 0;
        }
        else if ((1 == pTriggerList->numberOfLED) || (2 == pTriggerList->numberOfLED))
        {
            triggerValue = pTriggerList->triggerLED;

            if (triggerValue >= pTriggerList->privateInfo.led_sensitivity_trigger.end)
            {
                regionOutput.startIndex         = 1;
                regionOutput.endIndex           = 1;
                regionOutput.interpolationRatio = 0;
            }
            else if (triggerValue <= pTriggerList->privateInfo.led_sensitivity_trigger.start)
            {
                regionOutput.startIndex         = 0;
                regionOutput.endIndex           = 0;
                regionOutput.interpolationRatio = 0;
            }
            else
            {
                regionOutput.startIndex         = 0;
                regionOutput.endIndex           = 1;

                regionOutput.interpolationRatio = IQSettingUtils::CalculateInterpolationRatio(
                    static_cast<DOUBLE>(triggerValue),
                    static_cast<DOUBLE>(pTriggerList->privateInfo.led_sensitivity_trigger.start),
                    static_cast<DOUBLE>(pTriggerList->privateInfo.led_sensitivity_trigger.end));
            }

            if (2 == pTriggerList->numberOfLED)
            {
                rationLED2 = pTriggerList->LEDFirstEntryRatio;
            }
        }
        else
        {
            // Error case, treat as LED off -- Adding log here

            regionOutput.startIndex         = 0;
            regionOutput.endIndex           = 0;
            regionOutput.interpolationRatio = 0;
        }

        regionOutput.startIndex = IQSettingUtils::MinUINT32(regionOutput.startIndex, (regionNumber - 1));
        regionOutput.endIndex   = IQSettingUtils::MinUINT32(regionOutput.endIndex, (regionNumber - 1));

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_tdl10_led_idx_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_tdl10_led_idx_data[regionOutput.endIndex]),
                NULL);
            childCount++;
        }

        if ((rationLED2 != 0.0f) && ( 3 <= regionNumber))
        {
            // Adding Intepolation Value to ParentNode
            // If already has 2 child node, put to interpolationValue[1], otherwise put to interpolationValue[0]
            pParentNode->interpolationValue[childCount - 1] = (1 - rationLED2);

            // Set up one more child
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_tdl10_led_idx_data[2]),
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
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPETDL10Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPETDL10Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tdl_1_0_0::mod_tdl10_led_idx_dataType*   pParentDataType = NULL;
    TDL10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tdl_1_0_0::mod_tdl10_led_idx_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->led_idx_data.mod_tdl10_aec_dataCount;
        pTriggerList    = static_cast<TDL10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->led_idx_data.mod_tdl10_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->led_idx_data.mod_tdl10_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->led_idx_data.mod_tdl10_aec_data[regionOutput.endIndex]),
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
// IPETDL10Interpolation::CCTSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPETDL10Interpolation::CCTSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tdl_1_0_0::mod_tdl10_aec_dataType*   pParentDataType = NULL;
    TDL10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tdl_1_0_0::mod_tdl10_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->aec_data.mod_tdl10_cct_dataCount;
        pTriggerList    = static_cast<TDL10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->aec_data.mod_tdl10_cct_data[count].cct_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerCCT;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->aec_data.mod_tdl10_cct_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->aec_data.mod_tdl10_cct_data[regionOutput.startIndex].tdl10_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->aec_data.mod_tdl10_cct_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->aec_data.mod_tdl10_cct_data[regionOutput.endIndex].tdl10_rgn_data));
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
