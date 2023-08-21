// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017, 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gamma16interpolation.cpp
/// @brief Gamma16 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gamma16interpolation.h"
#include "gamma16setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation Gamma16OperationTable[] =
{
    {Gamma16Interpolation::DRCGainSearchNode, 2},
    {Gamma16Interpolation::HDRAECSearchNode,  2},
    {Gamma16Interpolation::LEDSearchNode,     3},
    {Gamma16Interpolation::AECSearchNode,     2},
    {Gamma16Interpolation::CCTSearchNode,     2}
};

static const UINT32 Gamma16MaxNode            = 91; ///< (1 + 1 * 2 + 2 * 2 + 4 * 3 + 12 * 2 + 24 * 2)
static const UINT32 Gamma16MaxNonLeafNode     = 43; ///< (1 + 1 * 2 + 2 * 2 + 4 * 3 + 12 * 2)
static const UINT32 Gamma16InterpolationLevel = 6;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Gamma16Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma16Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    Gamma16InputData* pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))                        ||
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

        pTriggerData->LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;

        isChanged = TRUE;
    }

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Gamma16Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma16Interpolation::RunInterpolation(
    const Gamma16InputData*                    pInput,
    gamma_1_6_0::mod_gamma16_channel_dataType* pData)
{
    BOOL result = TRUE;
    UINT count;

    ///< The interpolation tree total Node
    TuningNode nodeSet[Gamma16MaxNode];

    // Except leaf node, each valid node needs a scratch buffer to contain intepolation result
    gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct dataBuffer[Gamma16MaxNonLeafNode];
    // each valid node needs a scratch buffer to contain interpolation result for 3 channels
    gamma_1_6_0::mod_gamma16_channel_dataType channelBuffer[Gamma16MaxNonLeafNode * GammaLUTMax];

    // Assign memory for each channel data except for the root node
    for (count = 0; count < Gamma16MaxNonLeafNode; count++)
    {
        dataBuffer[count].mod_gamma16_channel_data = &channelBuffer[count * GammaLUTMax];
    }

    // Gamma Trigger List
    Gamma16TriggerList gammaTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        ///< Initializes all the nodes
        for (count = 0; count < Gamma16MaxNode; count++)
        {
            if (count < Gamma16MaxNonLeafNode)
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], &dataBuffer[count]);
            }
            else
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], NULL);
            }
        }

        // Setup the Top Node
        IQSettingUtils::AddNodeToInterpolationTree(NULL, &nodeSet[0],
            static_cast<VOID*>(&pInput->pChromatix->chromatix_gamma16_core), NULL);


        // Setup Trigger Data
        IQSettingUtils::Memcpy(&gammaTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(gamma_1_6_0::chromatix_gamma16Type::control_methodStruct));

        gammaTrigger.triggerDRCgain = pInput->DRCGain;
        gammaTrigger.triggerLED     = pInput->LEDTrigger;
        gammaTrigger.numberOfLED    = pInput->numberOfLED;
        gammaTrigger.privateInfo    = pInput->pChromatix->private_information;
        gammaTrigger.triggerCCT     = pInput->CCTTrigger;

        gammaTrigger.LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;

        gammaTrigger.triggerHDRAEC  = IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                                                       pInput->exposureTime,
                                                                       pInput->AECSensitivity,
                                                                       pInput->exposureGainRatio);
        gammaTrigger.triggerAEC     = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                                    pInput->luxIndex,
                                                                    pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        Gamma16InterpolationLevel,
                                                        &Gamma16OperationTable[0],
                                                        static_cast<VOID*>(&gammaTrigger));

        if (FALSE != result)
        {
            // Calculate the Interpolation Result
            result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                           Gamma16MaxNonLeafNode,
                                                           Gamma16InterpolationLevel,
                                                           Gamma16Interpolation::DoInterpolation);
        }

        if (FALSE != result)
        {
            // copy the interpolated data to output buffer
            gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pInterpolatedData =
                static_cast<gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct*>(nodeSet[0].pData);
            IQSettingUtils::Memcpy(pData,
                                   pInterpolatedData->mod_gamma16_channel_data,
                                   pInterpolatedData->mod_gamma16_channel_dataCount *
                                       sizeof(gamma_1_6_0::mod_gamma16_channel_dataType));
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
/// Gamma16Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma16Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pInput1 =
            static_cast<gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct*>(pData1);
        gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pInput2 =
            static_cast<gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct*>(pData2);
        gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pOutput =
            static_cast<gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct*>(pResult);

        if (pInput1 == pInput2)
        {
            result = CopyCCTData(pInput1, pOutput);
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = Gamma16Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                result = CopyCCTData(pInput1, pOutput);
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                result = CopyCCTData(pInput2, pOutput);
            }
            else
            {
                result = FALSE;
                /// @todo (CAMX-1812) Need to add logging for Common library
            }
        }

        // output always has R/G/B gamma channels
        pOutput->mod_gamma16_channel_dataCount = GammaLUTMax;
        pOutput->mod_gamma16_channel_data[GammaLUTChannel0].channel_type = ispglobalelements::channel_rgb_type::channel_G;
        pOutput->mod_gamma16_channel_data[GammaLUTChannel1].channel_type = ispglobalelements::channel_rgb_type::channel_B;
        pOutput->mod_gamma16_channel_data[GammaLUTChannel2].channel_type = ispglobalelements::channel_rgb_type::channel_R;
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Gamma16Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma16Interpolation::InterpolationData(
    gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pInput1,
    gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pInput2,
    FLOAT                                                  ratio,
    gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pOutput)
{
    UINT count;
    UINT chType;
    BOOL result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        pOutput->mod_gamma16_channel_dataCount = GammaLUTMax;

        // R/G/B 3 channels
        FLOAT*  pInTable1[GammaLUTMax] = { NULL };
        FLOAT*  pInTable2[GammaLUTMax] = { NULL };
        FLOAT*  pOutTable[GammaLUTMax] = { NULL };

        // initialize output tables
        for (chType = 0; chType < GammaLUTMax; chType++)
        {
            pOutTable[chType] = pOutput->mod_gamma16_channel_data[chType].gamma16_rgn_data.table;
        }

        // initialize input1 tables
        result = GetGammaTables(pInput1, pInTable1);

        if (TRUE == result)
        {
            result = GetGammaTables(pInput2, pInTable2);

            if (TRUE == result)
            {
                for (count = 0; count < GAMMA16_TABLE_LENGTH; count++)
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
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamma16Interpolation::GetGammaTables
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma16Interpolation::GetGammaTables(
    gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pInput,
    FLOAT*                                                 pOutTable[GammaLUTMax])
{
    BOOL result = TRUE;

    if (pInput->mod_gamma16_channel_dataCount == 1)
    {
        // RGB 1 channel for all channels
        pOutTable[GammaLUTChannel0] = pInput->mod_gamma16_channel_data[0].gamma16_rgn_data.table;
        pOutTable[GammaLUTChannel1] = pOutTable[GammaLUTChannel0];
        pOutTable[GammaLUTChannel2] = pOutTable[GammaLUTChannel0];
    }
    else
    {
        // handle R/G/B 3 channel cases
        if (pInput->mod_gamma16_channel_dataCount == 3)
        {
            for (UINT chType = 0; chType < GammaLUTMax; chType++)
            {
                if (pInput->mod_gamma16_channel_data[chType].channel_type ==
                    ispglobalelements::channel_rgb_type::channel_G)
                {
                    pOutTable[GammaLUTChannel0] = pInput->mod_gamma16_channel_data[chType].gamma16_rgn_data.table;
                }
                else if (pInput->mod_gamma16_channel_data[chType].channel_type ==
                    ispglobalelements::channel_rgb_type::channel_B)
                {
                    pOutTable[GammaLUTChannel1] = pInput->mod_gamma16_channel_data[chType].gamma16_rgn_data.table;
                }
                else
                {
                    // remaining channel R
                    pOutTable[GammaLUTChannel2] = pInput->mod_gamma16_channel_data[chType].gamma16_rgn_data.table;
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
// Gamma16Interpolation::CopyCCTData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma16Interpolation::CopyCCTData(
    gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pInput,
    gamma_1_6_0::mod_gamma16_cct_dataType::cct_dataStruct* pOutput)
{
    BOOL result = TRUE;
    UINT chType;

    // R/G/B 3 channels
    FLOAT*  pInTable[GammaLUTMax]  = { NULL };
    FLOAT*  pOutTable[GammaLUTMax] = { NULL };

    // initialize output tables
    for (chType = 0; chType < GammaLUTMax; chType++)
    {
        pOutTable[chType] = pOutput->mod_gamma16_channel_data[chType].gamma16_rgn_data.table;
    }

    result = GetGammaTables(pInput, pInTable);
    if (TRUE == result)
    {
        for (chType = GammaLUTChannel0; chType <= GammaLUTChannel2; chType++)
        {
            IQSettingUtils::Memcpy(pOutTable[chType],
                                   pInTable[chType],
                                   sizeof(FLOAT)*(DMIRAM_RGB_CH0_LENGTH_RGBLUT_V16 + 1));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamma16Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Gamma16Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gamma_1_6_0::chromatix_gamma16_coreType*   pParentDataType = NULL;
    Gamma16TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gamma_1_6_0::chromatix_gamma16_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_gamma16_drc_gain_dataCount;
        pTriggerList    = static_cast<Gamma16TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_gamma16_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_gamma16_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_gamma16_drc_gain_data[regionOutput.endIndex]),
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
// Gamma16Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Gamma16Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gamma_1_6_0::mod_gamma16_drc_gain_dataType*   pParentDataType = NULL;
    Gamma16TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gamma_1_6_0::mod_gamma16_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_gamma16_hdr_aec_dataCount;
        pTriggerList    = static_cast<Gamma16TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_gamma16_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_gamma16_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_gamma16_hdr_aec_data[regionOutput.endIndex]),
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
// Gamma16Interpolation::LEDSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Gamma16Interpolation::LEDSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    UINT32 regionNumber = 0;
    FLOAT  rationLED2   = 0.0f;
    FLOAT  triggerValue = 0.0f;

    InterpolationOutput regionOutput;

    gamma_1_6_0::mod_gamma16_hdr_aec_dataType*   pParentDataType = NULL;
    Gamma16TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gamma_1_6_0::mod_gamma16_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_gamma16_led_idx_dataCount;
        pTriggerList    = static_cast<Gamma16TriggerList*>(pTriggerData);
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
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gamma16_led_idx_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gamma16_led_idx_data[regionOutput.endIndex]),
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
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gamma16_led_idx_data[2]),
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
// Gamma16Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Gamma16Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gamma_1_6_0::mod_gamma16_led_dataType*   pParentDataType = NULL;
    Gamma16TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gamma_1_6_0::mod_gamma16_led_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->led_idx_data.mod_gamma16_aec_dataCount;
        pTriggerList    = static_cast<Gamma16TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->led_idx_data.mod_gamma16_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->led_idx_data.mod_gamma16_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->led_idx_data.mod_gamma16_aec_data[regionOutput.endIndex]),
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
// Gamma16Interpolation::CCTSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Gamma16Interpolation::CCTSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gamma_1_6_0::mod_gamma16_aec_dataType*   pParentDataType = NULL;
    Gamma16TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gamma_1_6_0::mod_gamma16_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->aec_data.mod_gamma16_cct_dataCount;
        pTriggerList    = static_cast<Gamma16TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->aec_data.mod_gamma16_cct_data[count].cct_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerCCT;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->aec_data.mod_gamma16_cct_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->aec_data.mod_gamma16_cct_data[regionOutput.startIndex].cct_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->aec_data.mod_gamma16_cct_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->aec_data.mod_gamma16_cct_data[regionOutput.endIndex].cct_data));
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
