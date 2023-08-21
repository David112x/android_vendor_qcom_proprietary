// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gamma15interpolation.cpp
/// @brief gamma15 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gamma15interpolation.h"
#include "gamma15setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation Gamma15OperationTable[] =
{
    {Gamma15Interpolation::DRCGainSearchNode, 2},
    {Gamma15Interpolation::HDRAECSearchNode,  2},
    {Gamma15Interpolation::LEDSearchNode,     3},
    {Gamma15Interpolation::AECSearchNode,     2},
    {Gamma15Interpolation::CCTSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Gamma15Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma15Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    Gamma15InputData* pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->digitalGain, pInput->AECGain))                    ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->colorTemperature, pInput->AWBColorTemperature))   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->LEDFirstEntryRatio, pInput->LEDFirstEntryRatio))  ||
        (pTriggerData->LEDTrigger != pInput->LEDSensitivity))
    {
        pTriggerData->luxIndex           = pInput->AECLuxIndex;
        pTriggerData->digitalGain        = pInput->AECGain;
        pTriggerData->AECSensitivity     = pInput->AECSensitivity;
        pTriggerData->exposureTime       = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio  = pInput->AECexposureGainRatio;
        pTriggerData->colorTemperature   = pInput->AWBColorTemperature;
        pTriggerData->LEDTrigger         = pInput->LEDSensitivity;
        pTriggerData->numberOfLED        = pInput->numberOfLED;
        pTriggerData->LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;

        isChanged = TRUE;
    }

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamma15Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma15Interpolation::RunInterpolation(
    const Gamma15InputData*                                pInput,
    gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct* pData)
{
    BOOL result = TRUE;
    UINT count;

    // The interpolation tree total Node
    TuningNode nodeSet[Gamma15MaxNode];

    // each valid node needs a scratch buffer to contain interpolation result for 3 channels
    gamma_1_5_0::mod_gamma15_channel_dataType channelBuffer[Gamma15MaxNonLeafNode * GammaLUTMax];

    // Color Correction Trigger List
    Gamma15TriggerList gamma15Trigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Assign memory for each channel data except for the root node
        for (count = 0; count < Gamma15MaxNonLeafNode; count++)
        {
            pData[count + 1].mod_gamma15_channel_data = &channelBuffer[count * GammaLUTMax];
        }

        // Initialize all the nodes
        for (count = 0; count < Gamma15MaxNode; count++)
        {
            if (count < Gamma15MaxNonLeafNode)
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], &pData[count + 1]);
            }
            else
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], NULL);
            }
        }

        // Setup the Top Node
        nodeSet[0].pNodeData = static_cast<VOID*>(&pInput->pChromatix->chromatix_gamma15_core);
        IQSettingUtils::AddNodeToInterpolationTree(NULL, &nodeSet[0],
            static_cast<VOID*>(&pInput->pChromatix->chromatix_gamma15_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&gamma15Trigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(gamma_1_5_0::chromatix_gamma15Type::control_methodStruct));

        gamma15Trigger.triggerDRCgain = pInput->DRCGain;

        gamma15Trigger.triggerHDRAEC  = IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                                                         pInput->exposureTime,
                                                                         pInput->AECSensitivity,
                                                                         pInput->exposureGainRatio);

        gamma15Trigger.triggerAEC     = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                                      pInput->luxIndex,
                                                                      pInput->digitalGain);

        gamma15Trigger.triggerLED     = pInput->LEDTrigger;
        gamma15Trigger.numberOfLED    = pInput->numberOfLED;
        gamma15Trigger.privateInfo    = pInput->pChromatix->private_information;
        gamma15Trigger.triggerCCT     = pInput->colorTemperature;

        gamma15Trigger.LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(
            &nodeSet[0], Gamma15InterpolationLevel, &Gamma15OperationTable[0], static_cast<VOID*>(&gamma15Trigger));
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
                                                       Gamma15MaxNonLeafNode,
                                                       Gamma15InterpolationLevel,
                                                       Gamma15Interpolation::DoInterpolation);

        // copy the interpolated data to output buffer
        gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct* pInterpolatedData =
            static_cast<gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct*>(nodeSet[0].pData);

        if (NULL != pInterpolatedData)
        {
            pData->mod_gamma15_channel_dataCount = pInterpolatedData->mod_gamma15_channel_dataCount;
            pData->mod_gamma15_channel_dataID    = pInterpolatedData->mod_gamma15_channel_dataID;
            IQSettingUtils::Memcpy(pData->mod_gamma15_channel_data,
                                   pInterpolatedData->mod_gamma15_channel_data,
                                   pData->mod_gamma15_channel_dataCount * sizeof(gamma_1_5_0::mod_gamma15_channel_dataType));
        }
        else
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
            result = FALSE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamma15Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma15Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result  = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct* pInput1 =
            static_cast<gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct*>(pData1);
        gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct* pInput2 =
            static_cast<gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct*>(pData2);
        gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct* pOutput =
            static_cast<gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct*>(pResult);

        if (pInput1 == pInput2)
        {
            result = CopyCCTData(pInput1, pOutput);
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = Gamma15Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (IQSettingUtils::FEqual(ratio, 0.0f))
            {
                result = CopyCCTData(pInput1, pOutput);
            }
            else if (IQSettingUtils::FEqual(ratio, 1.0f))
            {
                result = CopyCCTData(pInput2, pOutput);
            }
            else
            {
                /// @todo (CAMX-1812) Need to add logging for Common library
                result = FALSE;
            }
        }

        // output always has R/G/B gamma channels
        pOutput->mod_gamma15_channel_dataCount                           = GammaLUTMax;
        pOutput->mod_gamma15_channel_data[GammaLUTChannel0].channel_type = ispglobalelements::channel_rgb_type::channel_G;
        pOutput->mod_gamma15_channel_data[GammaLUTChannel1].channel_type = ispglobalelements::channel_rgb_type::channel_B;
        pOutput->mod_gamma15_channel_data[GammaLUTChannel2].channel_type = ispglobalelements::channel_rgb_type::channel_R;
    }
    else
    {
        /// @todo (CAMX-1812) Noeed to add logging for Common library
        result = FALSE;
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamma15Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma15Interpolation::InterpolationData(
    gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct* pInput1,
    gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct* pInput2,
    FLOAT                                                  ratio,
    gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct* pOutput)
{
    UINT  count;
    UINT  chType;
    BOOL  result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        // R/G/B 3 channels
        FLOAT* pInTable1[GammaLUTMax] = { NULL };
        FLOAT* pInTable2[GammaLUTMax] = { NULL };
        FLOAT* pOutTable[GammaLUTMax] = { NULL };

        // initialize output tables
        for (chType = 0; chType < GammaLUTMax; chType++)
        {
            pOutTable[chType] = pOutput->mod_gamma15_channel_data[chType].gamma15_rgn_data.table;
        }

        // initialize input1 tables
        result = GetGammaTables(pInput1, pInTable1);

        if (TRUE == result)
        {
            // initialize input2 tables
            result = GetGammaTables(pInput2, pInTable2);
            if (TRUE == result)
            {
                for (count = 0; count <= Gamma15LUTNumEntriesPerChannel; count++)
                {
                    pOutTable[GammaLUTChannel0][count] = IQSettingUtils::InterpolationFloatBilinear(
                        pInTable1[GammaLUTChannel0][count], pInTable2[GammaLUTChannel0][count], ratio);

                    pOutTable[GammaLUTChannel1][count] = IQSettingUtils::InterpolationFloatBilinear(
                        pInTable1[GammaLUTChannel1][count], pInTable2[GammaLUTChannel1][count], ratio);

                    pOutTable[GammaLUTChannel2][count] = IQSettingUtils::InterpolationFloatBilinear(
                        pInTable1[GammaLUTChannel2][count], pInTable2[GammaLUTChannel2][count], ratio);
                }
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
// Gamma15Interpolation::GetGammaTables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma15Interpolation::GetGammaTables(
    gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct* pInput,
    FLOAT*                                                 pInTable[GammaLUTMax])
{
    BOOL result = TRUE;

    if (pInput->mod_gamma15_channel_dataCount == 1)
    {
        // RGB 1 channel for all channels
        pInTable[GammaLUTChannel0] = pInput->mod_gamma15_channel_data[0].gamma15_rgn_data.table;
        pInTable[GammaLUTChannel1] = pInTable[GammaLUTChannel0];
        pInTable[GammaLUTChannel2] = pInTable[GammaLUTChannel0];
    }
    else
    {
        // handle R/G/B 3 channel cases
        if (pInput->mod_gamma15_channel_dataCount == 3)
        {
            for (UINT chType = 0; chType < GammaLUTMax; chType++)
            {
                if (pInput->mod_gamma15_channel_data[chType].channel_type ==
                    ispglobalelements::channel_rgb_type::channel_G)
                {
                    pInTable[GammaLUTChannel0] = pInput->mod_gamma15_channel_data[chType].gamma15_rgn_data.table;
                }
                else if (pInput->mod_gamma15_channel_data[chType].channel_type ==
                    ispglobalelements::channel_rgb_type::channel_B)
                {
                    pInTable[GammaLUTChannel1] = pInput->mod_gamma15_channel_data[chType].gamma15_rgn_data.table;
                }
                else
                {
                    // remaining channel R
                    pInTable[GammaLUTChannel2] = pInput->mod_gamma15_channel_data[chType].gamma15_rgn_data.table;
                }
            }
        }
        else
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
            result = FALSE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamma15Interpolation::CopyCCTData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma15Interpolation::CopyCCTData(
    gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct* pInput,
    gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct* pOutput)
{
    BOOL result = TRUE;
    UINT chType;

    // R/G/B 3 channels
    FLOAT*  pInTable[GammaLUTMax]  = { NULL };
    FLOAT*  pOutTable[GammaLUTMax] = { NULL };

    // initialize output tables
    for (chType = 0; chType < GammaLUTMax; chType++)
    {
        pOutTable[chType] = pOutput->mod_gamma15_channel_data[chType].gamma15_rgn_data.table;
    }

    result = GetGammaTables(pInput, pInTable);
    if (TRUE == result)
    {
        for (chType = GammaLUTChannel0; chType <= GammaLUTChannel2; chType++)
        {
            IQSettingUtils::Memcpy(pOutTable[chType],
                                   pInTable[chType],
                                   sizeof(FLOAT)*(Gamma15LUTNumEntriesPerChannel+1));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamma15Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Gamma15Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gamma_1_5_0::chromatix_gamma15_coreType*   pParentDataType = NULL;
    Gamma15TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gamma_1_5_0::chromatix_gamma15_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_gamma15_drc_gain_dataCount;
        pTriggerList    = static_cast<Gamma15TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_gamma15_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_gamma15_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_gamma15_drc_gain_data[regionOutput.endIndex]),
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
// Gamma15Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Gamma15Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gamma_1_5_0::mod_gamma15_drc_gain_dataType*   pParentDataType = NULL;
    Gamma15TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gamma_1_5_0::mod_gamma15_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_gamma15_hdr_aec_dataCount;
        pTriggerList    = static_cast<Gamma15TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_gamma15_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_gamma15_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_gamma15_hdr_aec_data[regionOutput.endIndex]),
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
// Gamma15Interpolation::LEDSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Gamma15Interpolation::LEDSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    UINT32 regionNumber = 0;
    FLOAT  rationLED2   = 0.0f;
    FLOAT  triggerValue = 0.0f;

    InterpolationOutput regionOutput;

    gamma_1_5_0::mod_gamma15_hdr_aec_dataType*   pParentDataType = NULL;
    Gamma15TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gamma_1_5_0::mod_gamma15_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_gamma15_led_idx_dataCount;
        pTriggerList    = static_cast<Gamma15TriggerList*>(pTriggerData);
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
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gamma15_led_idx_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gamma15_led_idx_data[regionOutput.endIndex]),
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
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gamma15_led_idx_data[2]),
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
// Gamma15Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Gamma15Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gamma_1_5_0::mod_gamma15_led_dataType*   pParentDataType = NULL;
    Gamma15TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gamma_1_5_0::mod_gamma15_led_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->led_idx_data.mod_gamma15_aec_dataCount;
        pTriggerList    = static_cast<Gamma15TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->led_idx_data.mod_gamma15_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->led_idx_data.mod_gamma15_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->led_idx_data.mod_gamma15_aec_data[regionOutput.endIndex]),
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
// Gamma15Interpolation::CCTSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Gamma15Interpolation::CCTSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gamma_1_5_0::mod_gamma15_aec_dataType*   pParentDataType = NULL;
    Gamma15TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gamma_1_5_0::mod_gamma15_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->aec_data.mod_gamma15_cct_dataCount;
        pTriggerList    = static_cast<Gamma15TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->aec_data.mod_gamma15_cct_data[count].cct_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerCCT;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->aec_data.mod_gamma15_cct_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->aec_data.mod_gamma15_cct_data[regionOutput.startIndex].cct_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->aec_data.mod_gamma15_cct_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->aec_data.mod_gamma15_cct_data[regionOutput.endIndex].cct_data));
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
