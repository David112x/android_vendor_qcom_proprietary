// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// @file  pedestal13interpolation.cpp
// @brief pedestal13 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pedestal13interpolation.h"
#include "pedestal13setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation Pedestal13OperationTable[] =
{
    {Pedestal13Interpolation::DRCGainSearchNode, 2},
    {Pedestal13Interpolation::HDRAECSearchNode,  2},
    {Pedestal13Interpolation::LEDSearchNode,     3},
    {Pedestal13Interpolation::AECSearchNode,     2},
    {Pedestal13Interpolation::CCTSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pedestal13Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pedestal13Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData*    pInput,
    Pedestal13InputData* pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECGain,  pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTimeTrigger, pInput->AECexposureTime))    ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->CCTTrigger, pInput->AWBColorTemperature))         ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->LEDFirstEntryRatio, pInput->LEDFirstEntryRatio))  ||
        (pTriggerData->imageWidth  != pInput->sensorImageWidth)                                          ||
        (pTriggerData->imageHeight != pInput->sensorImageHeight)                                         ||
        (pTriggerData->LEDTrigger  != pInput->LEDSensitivity))
    {
        pTriggerData->luxIndex            = pInput->AECLuxIndex;
        pTriggerData->AECGain             = pInput->AECGain;
        pTriggerData->AECSensitivity      = pInput->AECSensitivity;
        pTriggerData->exposureTimeTrigger = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio   = pInput->AECexposureGainRatio;
        pTriggerData->CCTTrigger          = pInput->AWBColorTemperature;
        pTriggerData->imageWidth          = pInput->sensorImageWidth;
        pTriggerData->imageHeight         = pInput->sensorImageHeight;
        pTriggerData->DRCGain             = pInput->DRCGain;
        pTriggerData->LEDTrigger          = pInput->LEDSensitivity;
        pTriggerData->numberOfLED         = pInput->numberOfLED;

        pTriggerData->LEDFirstEntryRatio  = pInput->LEDFirstEntryRatio;

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pedestal13Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pedestal13Interpolation::RunInterpolation(
    const Pedestal13InputData*               pInput,
    pedestal_1_3_0::pedestal13_rgn_dataType* pData)
{
    BOOL                                     result = TRUE;
    UINT                                     count  = 0;
    // The interpolation tree total Node
    TuningNode                               nodeSet[Pedestal13MaxmiumNode];
    // Demosaic Trigger List
    Pedestal13TriggerList                    pedestalTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (count = 0; count < Pedestal13MaxmiumNode; count++)
        {
            if (count < Pedestal13MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_pedestal13_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&pedestalTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(pedestal_1_3_0::chromatix_pedestal13Type::control_methodStruct));

        pedestalTrigger.triggerDRCgain = pInput->DRCGain;
        pedestalTrigger.triggerLED     = pInput->LEDTrigger;
        pedestalTrigger.numberOfLED    = pInput->numberOfLED;
        pedestalTrigger.privateInfo    = pInput->pChromatix->private_information;
        pedestalTrigger.triggerCCT     = pInput->CCTTrigger;

        pedestalTrigger.LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;

        pedestalTrigger.triggerHDRAEC  = IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                                                          pInput->exposureTimeTrigger,
                                                                          pInput->AECSensitivity,
                                                                          pInput->exposureGainRatio);
        pedestalTrigger.triggerAEC     = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                                       pInput->luxIndex,
                                                                       pInput->AECGain);

        // Set up Interpolation Tree
        result                         = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                                                Pedestal13InterpolationLevel,
                                                                                &Pedestal13OperationTable[0],
                                                                                static_cast<VOID*>(&pedestalTrigger));

        if (FALSE != result)
        {
            // Calculate the Interpolation Result
            result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                           Pedestal13MaxmiumNonLeafNode,
                                                           Pedestal13InterpolationLevel,
                                                           Pedestal13Interpolation::DoInterpolation);


            if (FALSE != result)
            {
                IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(pedestal_1_3_0::pedestal13_rgn_dataType));
            }
            else
            {
                // @todo (CAMX-1812) Need to add logging for Common library
            }
        }
    }
    else
    {
        // @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pedestal13Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pedestal13Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    pedestal_1_3_0::pedestal13_rgn_dataType* pInput1 = NULL;
    pedestal_1_3_0::pedestal13_rgn_dataType* pInput2 = NULL;
    pedestal_1_3_0::pedestal13_rgn_dataType* pOutput = NULL;
    BOOL                                     result  = TRUE;

    if ((pData1 != NULL) && (pData2 != NULL) && (pResult != NULL))
    {
        pInput1 = static_cast<pedestal_1_3_0::pedestal13_rgn_dataType*>(pData1);
        pInput2 = static_cast<pedestal_1_3_0::pedestal13_rgn_dataType*>(pData2);
        pOutput = static_cast<pedestal_1_3_0::pedestal13_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(pedestal_1_3_0::pedestal13_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = Pedestal13Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(pedestal_1_3_0::pedestal13_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(pedestal_1_3_0::pedestal13_rgn_dataType));
            }
        }
    }
    else
    {
        result = FALSE;
        // @todo (CAMX-1812): Add log for the common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pedestal13Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pedestal13Interpolation::InterpolationData(
    pedestal_1_3_0::pedestal13_rgn_dataType* pInput1,
    pedestal_1_3_0::pedestal13_rgn_dataType* pInput2,
    FLOAT                                    ratio,
    pedestal_1_3_0::pedestal13_rgn_dataType* pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput1)  &&
        (NULL != pInput2)  &&
        (NULL != pOutput))
    {
        for (UINT32 i = 0; i < PED_LUT_LENGTH; i++)
        {
            pOutput->channel_black_level_b_tab.channel_black_level_b[i]   =
                IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->channel_black_level_b_tab.
                                                                                 channel_black_level_b[i]),
                                                           static_cast<FLOAT>(pInput2->channel_black_level_b_tab.
                                                                                 channel_black_level_b[i]),
                                                           ratio);

            pOutput->channel_black_level_gb_tab.channel_black_level_gb[i] =
                IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->channel_black_level_gb_tab.
                                                                                 channel_black_level_gb[i]),
                                                           static_cast<FLOAT>(pInput2->channel_black_level_gb_tab.
                                                                                 channel_black_level_gb[i]),
                                                           ratio);

            pOutput->channel_black_level_gr_tab.channel_black_level_gr[i] =
                IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->channel_black_level_gr_tab.
                                                                                 channel_black_level_gr[i]),
                                                           static_cast<FLOAT>(pInput2->channel_black_level_gr_tab.
                                                                                 channel_black_level_gr[i]),
                                                           ratio);

            pOutput->channel_black_level_r_tab.channel_black_level_r[i]   =
                IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->channel_black_level_r_tab.
                                                                                 channel_black_level_r[i]),
                                                           static_cast<FLOAT>(pInput2->channel_black_level_r_tab.
                                                                                 channel_black_level_r[i]),
                                                           ratio);
        }
    }
    else
    {
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pedestal13Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Pedestal13Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    pedestal_1_3_0::chromatix_pedestal13_coreType*   pParentDataType = NULL;
    Pedestal13TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pedestal_1_3_0::chromatix_pedestal13_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_pedestal13_drc_gain_dataCount;
        pTriggerList    = static_cast<Pedestal13TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_pedestal13_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_pedestal13_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_pedestal13_drc_gain_data[regionOutput.endIndex]),
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
// Pedestal13Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Pedestal13Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    pedestal_1_3_0::mod_pedestal13_drc_gain_dataType*   pParentDataType = NULL;
    Pedestal13TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pedestal_1_3_0::mod_pedestal13_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_pedestal13_hdr_aec_dataCount;
        pTriggerList    = static_cast<Pedestal13TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_pedestal13_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_pedestal13_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_pedestal13_hdr_aec_data[regionOutput.endIndex]),
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
// Pedestal13Interpolation::LEDSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Pedestal13Interpolation::LEDSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    UINT32 regionNumber = 0;
    FLOAT  rationLED2   = 0.0f;
    FLOAT  triggerValue = 0.0f;

    InterpolationOutput regionOutput;

    pedestal_1_3_0::mod_pedestal13_hdr_aec_dataType*   pParentDataType = NULL;
    Pedestal13TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pedestal_1_3_0::mod_pedestal13_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_pedestal13_led_idx_dataCount;
        pTriggerList    = static_cast<Pedestal13TriggerList*>(pTriggerData);
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
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pedestal13_led_idx_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pedestal13_led_idx_data[regionOutput.endIndex]),
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
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pedestal13_led_idx_data[2]),
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
// Pedestal13Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Pedestal13Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    pedestal_1_3_0::mod_pedestal13_led_idx_dataType*   pParentDataType = NULL;
    Pedestal13TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pedestal_1_3_0::mod_pedestal13_led_idx_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->led_idx_data.mod_pedestal13_aec_dataCount;
        pTriggerList    = static_cast<Pedestal13TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->led_idx_data.mod_pedestal13_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->led_idx_data.mod_pedestal13_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->led_idx_data.mod_pedestal13_aec_data[regionOutput.endIndex]),
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
// Pedestal13Interpolation::CCTSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Pedestal13Interpolation::CCTSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    pedestal_1_3_0::mod_pedestal13_aec_dataType*   pParentDataType = NULL;
    Pedestal13TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pedestal_1_3_0::mod_pedestal13_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->aec_data.mod_pedestal13_cct_dataCount;
        pTriggerList    = static_cast<Pedestal13TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->aec_data.mod_pedestal13_cct_data[count].cct_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerCCT;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->aec_data.mod_pedestal13_cct_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->aec_data.mod_pedestal13_cct_data[regionOutput.startIndex].pedestal13_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->aec_data.mod_pedestal13_cct_data[regionOutput.endIndex]),
                static_cast<VOID*>
                    (&pParentDataType->aec_data.mod_pedestal13_cct_data[regionOutput.endIndex].pedestal13_rgn_data));
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
