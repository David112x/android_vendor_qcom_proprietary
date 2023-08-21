// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cv12interpolation.cpp
/// @brief IPE CV12 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "cv12interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation CV12OperationTable[] =
{
    {CV12Interpolation::DRCGainSearchNode, 2},
    {CV12Interpolation::HDRAECSearchNode,  2},
    {CV12Interpolation::LEDSearchNode,     3},
    {CV12Interpolation::AECSearchNode,     2},
    {CV12Interpolation::CCTSearchNode,     2}
};

// static const UINT32 CV12MaxNode            = 91; ///< (1 + 1 * 2 + 2 * 2 + 4 * 3 + 12 * 2 + 24 * 2)
// static const UINT32 CV12MaxNonLeafNode     = 43; ///< (1 + 1 * 2 + 2 * 2 + 4 * 3 + 12 * 2)
// static const UINT32 CV12InterpolationLevel = 6;  ///< Core->DRCGain->HDRAEC->LED->AEC->CCT

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CV12Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CV12Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData*  pInput,
    CV12InputData*     pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->digitalGain, pInput->AECGain))                    ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain,  pInput->DRCGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->colorTemperature, pInput->AWBColorTemperature))   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->LEDFirstEntryRatio, pInput->LEDFirstEntryRatio))  ||
        (pTriggerData->LEDTrigger != pInput->LEDSensitivity))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->digitalGain       = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->DRCGain           = pInput->DRCGain;
        pTriggerData->colorTemperature  = pInput->AWBColorTemperature;
        pTriggerData->LEDTrigger        = pInput->LEDSensitivity;
        pTriggerData->numberOfLED       = pInput->numberOfLED;

        pTriggerData->LEDFirstEntryRatio  = pInput->LEDFirstEntryRatio;
        isChanged = TRUE;
    }

    // Update AEC Y histogram stretching only if this feature is enabled.
    if (TRUE == pInput->enableAECYHistStretching)
    {
        if ((FALSE == IQSettingUtils::FEqual(pTriggerData->clamp, pInput->AECYHistStretchClampOffset)) ||
            (FALSE == IQSettingUtils::FEqual(pTriggerData->stretch_factor, pInput->AECYHistStretchScaleFactor)))
        {
            pTriggerData->clamp             = pInput->AECYHistStretchClampOffset;
            pTriggerData->stretch_factor    = pInput->AECYHistStretchScaleFactor;

            isChanged = TRUE;
        }
    }
    else
    {
        if ((FALSE == IQSettingUtils::FEqual(pTriggerData->clamp, 0.0f)) ||
            (FALSE == IQSettingUtils::FEqual(pTriggerData->stretch_factor, 1.0f)))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupISP, "YHist Stretch feature not enabled. Setting a default value");
            pTriggerData->clamp             = 0.0f;
            pTriggerData->stretch_factor    = 1.0f;

            isChanged = TRUE;
        }
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CV12Interpolation::RunIntepolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CV12Interpolation::RunInterpolation(
    const CV12InputData*            pInput,
    cv_1_2_0::cv12_rgn_dataType*    pData)
{
    BOOL result = TRUE;

    // The interpolation tree total Node
    TuningNode nodeSet[CV12MaxNode];

    // CV12 Trigger List
    CV12TriggerList cv12Trigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < CV12MaxNode; count++)
        {
            if (count < CV12MaxNonLeafNode)
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], &pData[count + 1]);
            }
            else
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], NULL);
            }
        }

        // Setup the Top Node
        nodeSet[0].pNodeData = static_cast<VOID*>(&pInput->pChromatix->chromatix_cv12_core);
        IQSettingUtils::AddNodeToInterpolationTree(NULL, &nodeSet[0],
            static_cast<VOID*>(&pInput->pChromatix->chromatix_cv12_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&cv12Trigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(cv_1_2_0::chromatix_cv12Type::control_methodStruct));

        cv12Trigger.triggerDRCgain = pInput->DRCGain;
        cv12Trigger.triggerLED     = pInput->LEDTrigger;
        cv12Trigger.numberOfLED    = pInput->numberOfLED;
        cv12Trigger.privateInfo    = pInput->pChromatix->private_information;
        cv12Trigger.triggerCCT     = pInput->colorTemperature;

        cv12Trigger.LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;

        cv12Trigger.triggerHDRAEC  = IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                                                      pInput->exposureTime,
                                                                      pInput->AECSensitivity,
                                                                      pInput->exposureGainRatio);

        cv12Trigger.triggerAEC     = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                                   pInput->luxIndex,
                                                                   pInput->digitalGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(
            &nodeSet[0], CV12InterpolationLevel, &CV12OperationTable[0], static_cast<VOID*>(&cv12Trigger));
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    if (FALSE != result)
    {
        // Calculate the Interpolation Result
        result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                       CV12MaxNonLeafNode,
                                                       CV12InterpolationLevel,
                                                       CV12Interpolation::DoInterpolation);

        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(cv_1_2_0::cv12_rgn_dataType));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CV12Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CV12Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        cv_1_2_0::cv12_rgn_dataType* pInput1 = static_cast<cv_1_2_0::cv12_rgn_dataType*>(pData1);
        cv_1_2_0::cv12_rgn_dataType* pInput2 = static_cast<cv_1_2_0::cv12_rgn_dataType*>(pData2);
        cv_1_2_0::cv12_rgn_dataType* pOutput = static_cast<cv_1_2_0::cv12_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(cv_1_2_0::cv12_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = CV12Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(cv_1_2_0::cv12_rgn_dataType));
            }
            else if (IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(cv_1_2_0::cv12_rgn_dataType));
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
/// CV12Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CV12Interpolation::InterpolationData(
    cv_1_2_0::cv12_rgn_dataType*   pInput1,
    cv_1_2_0::cv12_rgn_dataType*   pInput2,
    FLOAT                          ratio,
    cv_1_2_0::cv12_rgn_dataType*   pOutput)
{
    BOOL  resultAPI = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        pOutput->am       = IQSettingUtils::InterpolationFloatBilinear(pInput1->am, pInput2->am, ratio);
        pOutput->ap       = IQSettingUtils::InterpolationFloatBilinear(pInput1->ap, pInput2->ap, ratio);
        pOutput->bm       = IQSettingUtils::InterpolationFloatBilinear(pInput1->bm, pInput2->bm, ratio);
        pOutput->bp       = IQSettingUtils::InterpolationFloatBilinear(pInput1->bp, pInput2->bp, ratio);
        pOutput->b_to_y   = IQSettingUtils::InterpolationFloatBilinear(pInput1->b_to_y, pInput2->b_to_y, ratio);
        pOutput->cm       = IQSettingUtils::InterpolationFloatBilinear(pInput1->cm, pInput2->cm, ratio);
        pOutput->cp       = IQSettingUtils::InterpolationFloatBilinear(pInput1->cp, pInput2->cp, ratio);
        pOutput->dm       = IQSettingUtils::InterpolationFloatBilinear(pInput1->dm, pInput2->dm, ratio);
        pOutput->dp       = IQSettingUtils::InterpolationFloatBilinear(pInput1->dp, pInput2->dp, ratio);
        pOutput->g_to_y   = IQSettingUtils::InterpolationFloatBilinear(pInput1->g_to_y, pInput2->g_to_y, ratio);
        pOutput->kcb      = IQSettingUtils::InterpolationFloatBilinear(pInput1->kcb, pInput2->kcb, ratio);
        pOutput->kcr      = IQSettingUtils::InterpolationFloatBilinear(pInput1->kcr, pInput2->kcr, ratio);
        pOutput->r_to_y   = IQSettingUtils::InterpolationFloatBilinear(pInput1->r_to_y, pInput2->r_to_y, ratio);
        pOutput->y_offset = IQSettingUtils::InterpolationFloatBilinear(pInput1->y_offset, pInput2->y_offset, ratio);
    }
    else
    {
        resultAPI = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return resultAPI;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CV12Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CV12Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cv_1_2_0::chromatix_cv12_coreType*   pParentDataType = NULL;
    CV12TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cv_1_2_0::chromatix_cv12_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_cv12_drc_gain_dataCount;
        pTriggerList    = static_cast<CV12TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_cv12_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_cv12_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_cv12_drc_gain_data[regionOutput.endIndex]),
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
// CV12Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CV12Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cv_1_2_0::mod_cv12_drc_gain_dataType*   pParentDataType = NULL;
    CV12TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cv_1_2_0::mod_cv12_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_cv12_hdr_aec_dataCount;
        pTriggerList    = static_cast<CV12TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_cv12_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_cv12_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_cv12_hdr_aec_data[regionOutput.endIndex]),
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
// CV12Interpolation::LEDSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CV12Interpolation::LEDSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    UINT32 regionNumber = 0;
    FLOAT  rationLED2   = 0.0f;
    FLOAT  triggerValue = 0.0f;

    InterpolationOutput regionOutput;

    cv_1_2_0::mod_cv12_hdr_aec_dataType*   pParentDataType = NULL;
    CV12TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cv_1_2_0::mod_cv12_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_cv12_led_dataCount;
        pTriggerList    = static_cast<CV12TriggerList*>(pTriggerData);
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
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cv12_led_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cv12_led_data[regionOutput.endIndex]),
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
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cv12_led_data[2]),
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
// CV12Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CV12Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cv_1_2_0::mod_cv12_led_dataType*   pParentDataType = NULL;
    CV12TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cv_1_2_0::mod_cv12_led_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->led_data.mod_cv12_aec_dataCount;
        pTriggerList    = static_cast<CV12TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->led_data.mod_cv12_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->led_data.mod_cv12_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->led_data.mod_cv12_aec_data[regionOutput.endIndex]),
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
// CV12Interpolation::CCTSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CV12Interpolation::CCTSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cv_1_2_0::mod_cv12_aec_dataType*   pParentDataType = NULL;
    CV12TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cv_1_2_0::mod_cv12_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->aec_data.mod_cv12_cct_dataCount;
        pTriggerList    = static_cast<CV12TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->aec_data.mod_cv12_cct_data[count].cct_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerCCT;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->aec_data.mod_cv12_cct_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->aec_data.mod_cv12_cct_data[regionOutput.startIndex].cv12_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->aec_data.mod_cv12_cct_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->aec_data.mod_cv12_cct_data[regionOutput.endIndex].cv12_rgn_data));
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
