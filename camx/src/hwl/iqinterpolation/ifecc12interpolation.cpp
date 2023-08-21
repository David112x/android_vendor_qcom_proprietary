// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifecc12interpolation.cpp
/// @brief IFE cc12 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ifecc12interpolation.h"
#include "ifecc12setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation CC12OperationTable[] =
{
    {IFECC12Interpolation::DRCGainSearchNode, 2},
    {IFECC12Interpolation::HDRAECSearchNode,  2},
    {IFECC12Interpolation::LEDSearchNode,     3},
    {IFECC12Interpolation::AECSearchNode,     2},
    {IFECC12Interpolation::CCTSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECC12Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFECC12Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    CC12InputData*    pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECGain,  pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain,  pInput->DRCGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->colorTemperature, pInput->AWBColorTemperature))   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->LEDFirstEntryRatio, pInput->LEDFirstEntryRatio))  ||
        (pTriggerData->LEDTrigger != pInput->LEDSensitivity))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->AECGain           = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->colorTemperature  = pInput->AWBColorTemperature;
        pTriggerData->DRCGain           = pInput->DRCGain;
        pTriggerData->LEDTrigger        = pInput->LEDSensitivity;
        pTriggerData->numberOfLED       = pInput->numberOfLED;

        pTriggerData->LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECC12Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFECC12Interpolation::RunInterpolation(
    const CC12InputData*         pInput,
    cc_1_2_0::cc12_rgn_dataType* pData)
{
    BOOL result = TRUE;

    // The interpolation tree total Node
    TuningNode nodeSet[CC12MaxmiumNode];

    // Color Correction Trigger List
    CC12TriggerList cc12Trigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < CC12MaxmiumNode; count++)
        {
            if (count < CC12MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_cc12_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&cc12Trigger.controlType,
            &(pInput->pChromatix->control_method),
            sizeof(cc_1_2_0::chromatix_cc12Type::control_methodStruct));

        cc12Trigger.triggerDRCgain = pInput->DRCGain;

        cc12Trigger.triggerHDRAEC  = IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                         pInput->exposureTime,
                                         pInput->AECSensitivity,
                                         pInput->exposureGainRatio);

        cc12Trigger.triggerAEC     = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                         pInput->luxIndex,
                                         pInput->AECGain);

        cc12Trigger.triggerLED     = pInput->LEDTrigger;
        cc12Trigger.numberOfLED    = pInput->numberOfLED;
        cc12Trigger.privateInfo    = pInput->pChromatix->private_information;
        cc12Trigger.triggerCCT     = pInput->colorTemperature;

        cc12Trigger.LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(
            &nodeSet[0], CC12InterpolationLevel, &CC12OperationTable[0], static_cast<VOID*>(&cc12Trigger));
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
            CC12MaxmiumNonLeafNode,
            CC12InterpolationLevel,
            IFECC12Interpolation::DoInterpolation);

        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(cc_1_2_0::cc12_rgn_dataType));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFECC12Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFECC12Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        cc_1_2_0::cc12_rgn_dataType* pInput1 = static_cast<cc_1_2_0::cc12_rgn_dataType*>(pData1);
        cc_1_2_0::cc12_rgn_dataType* pInput2 = static_cast<cc_1_2_0::cc12_rgn_dataType*>(pData2);
        cc_1_2_0::cc12_rgn_dataType* pOutput = static_cast<cc_1_2_0::cc12_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(cc_1_2_0::cc12_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = IFECC12Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(cc_1_2_0::cc12_rgn_dataType));
            }
            else if (IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(cc_1_2_0::cc12_rgn_dataType));
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
/// IFECC12Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFECC12Interpolation::InterpolationData(
    cc_1_2_0::cc12_rgn_dataType* pInput1,
    cc_1_2_0::cc12_rgn_dataType* pInput2,
    FLOAT                        ratio,
    cc_1_2_0::cc12_rgn_dataType* pOutput)
{
    UINT  count     = 0;
    FLOAT result    = 0.0f;
    UINT  cTabSize  = sizeof(pOutput->c_tab)/sizeof(FLOAT);
    UINT  kTabSize  = sizeof(pOutput->k_tab)/sizeof(FLOAT);
    BOOL  resultAPI = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        for (count = 0; count < cTabSize; count++)
        {
            pOutput->c_tab.c[count] = IQSettingUtils::InterpolationFloatBilinear(pInput1->c_tab.c[count],
                                      pInput2->c_tab.c[count], ratio);
        }
        for (count = 0; count < kTabSize; count++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(pInput1->k_tab.k[count],
                         pInput2->k_tab.k[count], ratio);

            pOutput->k_tab.k[count] = result;
        }
    }
    else
    {
        resultAPI = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return resultAPI;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFECC12Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFECC12Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cc_1_2_0::chromatix_cc12_coreType*   pParentDataType = NULL;
    CC12TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cc_1_2_0::chromatix_cc12_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_cc12_drc_gain_dataCount;
        pTriggerList    = static_cast<CC12TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_cc12_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_cc12_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_cc12_drc_gain_data[regionOutput.endIndex]),
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
// IFECC12Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFECC12Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cc_1_2_0::mod_cc12_drc_gain_dataType*   pParentDataType = NULL;
    CC12TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cc_1_2_0::mod_cc12_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_cc12_hdr_aec_dataCount;
        pTriggerList    = static_cast<CC12TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_cc12_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_cc12_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_cc12_hdr_aec_data[regionOutput.endIndex]),
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
// IFECC12Interpolation::LEDSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFECC12Interpolation::LEDSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    UINT32 regionNumber = 0;
    FLOAT  rationLED2   = 0.0f;
    FLOAT  triggerValue = 0.0f;

    InterpolationOutput regionOutput;

    cc_1_2_0::mod_cc12_hdr_aec_dataType*   pParentDataType = NULL;
    CC12TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cc_1_2_0::mod_cc12_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_cc12_led_idx_dataCount;
        pTriggerList    = static_cast<CC12TriggerList*>(pTriggerData);
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
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cc12_led_idx_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cc12_led_idx_data[regionOutput.endIndex]),
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
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cc12_led_idx_data[2]),
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
// IFECC12Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFECC12Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cc_1_2_0::mod_cc12_led_idx_dataType*   pParentDataType = NULL;
    CC12TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cc_1_2_0::mod_cc12_led_idx_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->led_idx_data.mod_cc12_aec_dataCount;
        pTriggerList    = static_cast<CC12TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->led_idx_data.mod_cc12_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->led_idx_data.mod_cc12_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->led_idx_data.mod_cc12_aec_data[regionOutput.endIndex]),
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
// IFECC12Interpolation::CCTSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFECC12Interpolation::CCTSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cc_1_2_0::mod_cc12_aec_dataType*   pParentDataType = NULL;
    CC12TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cc_1_2_0::mod_cc12_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->aec_data.mod_cc12_cct_dataCount;
        pTriggerList    = static_cast<CC12TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->aec_data.mod_cc12_cct_data[count].cct_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerCCT;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->aec_data.mod_cc12_cct_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->aec_data.mod_cc12_cct_data[regionOutput.startIndex].cc12_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->aec_data.mod_cc12_cct_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->aec_data.mod_cc12_cct_data[regionOutput.endIndex].cc12_rgn_data));
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
