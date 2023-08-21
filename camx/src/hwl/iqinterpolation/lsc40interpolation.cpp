// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  lsc40interpolation.cpp
/// @brief LSC40 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "lsc40interpolation.h"
#include "lsc40setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation LSC40OperationTable[] =
{
    { LSC40Interpolation::LensPositionSearchNode,   2 },
    { LSC40Interpolation::DRCGainSearchNode,        2 },
    { LSC40Interpolation::HDRAECSearchNode,         2 },
    { LSC40Interpolation::LEDSearchNode,            3 },
    { LSC40Interpolation::AECSearchNode,            2 },
    { LSC40Interpolation::CCTSearchNode,            2 }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC40Interpolation::InterpolateAndCalibrate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LSC40Interpolation::InterpolateAndCalibrate(
    lsc_4_0_0::lsc40_rgn_dataType* pData,
    const LSC40InputData*          pInput,
    lsc_4_0_0::lsc40_rgn_dataType* pOutput,
    FLOAT                          cctTrigger)
{
    BOOL   result                = TRUE;
    FLOAT  interpolationResult   = 0.0f;
    FLOAT  ratio                 = 0.0f;
    UINT32 index1                = 0;
    UINT32 index2                = 0;
    FLOAT  tempGrGbAverage       = 0.0f;
    UINT32 calibrationTableCount = 0;

    // Identify regions
    lsc_4_0_0::chromatix_lsc40_goldenType*    pGoldenTable = NULL;
    lsc_4_0_0::mod_lsc40_golden_cct_dataType* pGoldenTableCCTData = NULL;

    pGoldenTable = &(pInput->pChromatix->chromatix_lsc40_golden);
    pGoldenTableCCTData = &(pGoldenTable->mod_lsc40_golden_cct_data[0]);

    // If the trigger is less than index 0, then its in first region
    if (pGoldenTableCCTData->cct_trigger.end > cctTrigger)
    {
        index1 = 0;
        index2 = 0;
    }

    calibrationTableCount = IQSettingUtils::MinUINT32(pGoldenTable->mod_lsc40_golden_cct_dataCount,
                                                      pInput->numberOfEEPROMTable);

    for (UINT32 count = 1; count < calibrationTableCount - 1; count++)
    {
        pGoldenTableCCTData = &(pGoldenTable->mod_lsc40_golden_cct_data[count]);

        if (pGoldenTableCCTData->cct_trigger.start > cctTrigger)
        {
            index1 = count - 1;
            index2 = count;

            ratio  = (cctTrigger - pGoldenTable->mod_lsc40_golden_cct_data[index1].cct_trigger.start) /
                     (pGoldenTable->mod_lsc40_golden_cct_data[index2].cct_trigger.start -
                      pGoldenTable->mod_lsc40_golden_cct_data[index1].cct_trigger.end);
        }
        else if (pGoldenTableCCTData->cct_trigger.end > cctTrigger)
        {
            index1 = count;
            index2 = count;
            ratio  = 0.0f;
        }
    }

    pGoldenTableCCTData = &(pGoldenTable->mod_lsc40_golden_cct_data[calibrationTableCount - 1]);

    // If the trigger is less than index 0, then its in first region
    if (pGoldenTableCCTData->cct_trigger.start < cctTrigger)
    {
        index1 = calibrationTableCount - 1;
        index2 = calibrationTableCount - 1;
        ratio  = 1.0f;
    }

    // Interpolate the calibration tables and calibrate the result to get final table
    for (UINT32 rolloffSize = 0; rolloffSize < MESH_ROLLOFF40_SIZE; rolloffSize++)
    {
        interpolationResult = (((1 - ratio) * pInput->pCalibrationTable[index1].pBGain[rolloffSize]) +
            (ratio * pInput->pCalibrationTable[index2].pBGain[rolloffSize]));
        pOutput->b_gain_tab.b_gain[rolloffSize] = pData->b_gain_tab.b_gain[rolloffSize] * interpolationResult;

        interpolationResult = (((1 - ratio) * pInput->pCalibrationTable[index1].pRGain[rolloffSize]) +
            (ratio * pInput->pCalibrationTable[index2].pRGain[rolloffSize]));
        pOutput->r_gain_tab.r_gain[rolloffSize] = pData->r_gain_tab.r_gain[rolloffSize] * interpolationResult;

        interpolationResult = (((1 - ratio) * pInput->pCalibrationTable[index1].pGBGain[rolloffSize]) +
            (ratio * pInput->pCalibrationTable[index2].pGBGain[rolloffSize]));
        pOutput->gb_gain_tab.gb_gain[rolloffSize] = pData->gb_gain_tab.gb_gain[rolloffSize] * interpolationResult;

        interpolationResult = (((1 - ratio) * pInput->pCalibrationTable[index1].pGRGain[rolloffSize]) +
            (ratio * pInput->pCalibrationTable[index2].pGRGain[rolloffSize]));
        pOutput->gr_gain_tab.gr_gain[rolloffSize] = pData->gr_gain_tab.gr_gain[rolloffSize] * interpolationResult;

        // Here use the gr = gb gain to fix the cross pattern noise in the sky
        // also, use the gr/gb average to balance the gr gb channel difference.
        tempGrGbAverage     = (pOutput->gb_gain_tab.gb_gain[rolloffSize] + pOutput->gr_gain_tab.gr_gain[rolloffSize]) / (2.0f);
        pOutput->gb_gain_tab.gb_gain[rolloffSize] = tempGrGbAverage;
        pOutput->gr_gain_tab.gr_gain[rolloffSize] = tempGrGbAverage;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC40Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LSC40Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    LSC40InputData*   pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain,  pInput->AECGain))                      ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->CCTTrigger, pInput->AWBColorTemperature))         ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))                        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->LEDFirstEntryRatio, pInput->LEDFirstEntryRatio))  ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->lensPosTrigger, pInput->lensPosition))            ||
        (pTriggerData->LEDTrigger != pInput->LEDSensitivity))
    {
        pTriggerData->luxIndex           = pInput->AECLuxIndex;
        pTriggerData->realGain           = pInput->AECGain;
        pTriggerData->AECSensitivity     = pInput->AECSensitivity;
        pTriggerData->exposureTime       = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio  = pInput->AECexposureGainRatio;
        pTriggerData->CCTTrigger         = pInput->AWBColorTemperature;
        pTriggerData->LEDTrigger         = pInput->LEDSensitivity;
        pTriggerData->numberOfLED        = pInput->numberOfLED;
        pTriggerData->DRCGain            = pInput->DRCGain;
        pTriggerData->LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;
        pTriggerData->lensPosTrigger     = pInput->lensPosition;

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC40Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LSC40Interpolation::RunInterpolation(
    const LSC40InputData*          pInput,
    lsc_4_0_0::lsc40_rgn_dataType* pData)
{
    BOOL                           result        = TRUE;
    // The interpolotion tree total Node
    TuningNode                     nodeSet[LSC40MaxmiumNode];
    // Except leaf node, each valid node needs a scratch buffer to contain intepolation result
    lsc_4_0_0::lsc40_rgn_dataType* resultData   = NULL;
    // LSC Trigger List
    LSC40TriggerList               lscTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < LSC40MaxmiumNode; count++)
        {
            if (count < LSC40MaxmiumNonLeafNode)
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], &pData[count + 1]);
            }
            else
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], NULL);
            }
        }

        // Setup the Top Node
        nodeSet[0].pNodeData = static_cast<VOID*>(&pInput->pChromatix->chromatix_lsc40_core);
        IQSettingUtils::AddNodeToInterpolationTree(NULL, &nodeSet[0],
            static_cast<VOID*>(&pInput->pChromatix->chromatix_lsc40_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&lscTrigger.controlType,
            &(pInput->pChromatix->control_method),
            sizeof(lsc_4_0_0::chromatix_lsc40Type::control_methodStruct));

        lscTrigger.triggerDRCgain      = pInput->DRCGain;
        lscTrigger.triggerLED          = pInput->LEDTrigger;
        lscTrigger.numberOfLED         = pInput->numberOfLED;
        lscTrigger.triggerLensPosition = pInput->lensPosTrigger;
        lscTrigger.triggerCCT          = pInput->CCTTrigger;
        lscTrigger.privateInfo         = pInput->pChromatix->private_information;
        lscTrigger.LEDFirstEntryRatio  = pInput->LEDFirstEntryRatio;

        lscTrigger.triggerHDRAEC =  IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                                                     pInput->exposureTime,
                                                                     pInput->AECSensitivity,
                                                                     pInput->exposureGainRatio);

        lscTrigger.triggerAEC    = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                              pInput->luxIndex,
                                                              pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        LSC40InterpolationLevel,
                                                        &LSC40OperationTable[0],
                                                        static_cast<VOID*>(&lscTrigger));
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
                                                       LSC40MaxmiumNonLeafNode,
                                                       LSC40InterpolationLevel,
                                                       LSC40Interpolation::DoInterpolation);
    }

    if ((FALSE != result) && (TRUE == pInput->toCalibrate) && (TRUE == pInput->enableCalibration))
    {
        lsc_4_0_0::chromatix_lsc40_goldenType*    pGoldenTable          = NULL;
        lsc_4_0_0::mod_lsc40_golden_cct_dataType* pGoldenTableCCTData   = NULL;
        lsc_4_0_0::lsc40_golden_rgn_dataType*     pGoldenTableData      = NULL;
        UINT32                                    calibrationTableCount = 0;
        pGoldenTable = &(pInput->pChromatix->chromatix_lsc40_golden);

        calibrationTableCount = IQSettingUtils::MinUINT32(pGoldenTable->mod_lsc40_golden_cct_dataCount,
                                                          pInput->numberOfEEPROMTable);

        for (UINT32 count = 0; count < calibrationTableCount; count++)
        {
            pGoldenTableCCTData = &(pGoldenTable->mod_lsc40_golden_cct_data[count]);
            pGoldenTableData    = &(pGoldenTableCCTData->lsc40_golden_rgn_data);

            // For OEM1 the maximum number of tables is 2, hence each CCT type has one table only
            // Hence, we dont consider CCT type here
            for (UINT32 rolloffSize = 0; rolloffSize < MESH_ROLLOFF40_SIZE; rolloffSize++)
            {
                if (FALSE == IQSettingUtils::FEqual(
                    pInput->pCalibrationTable[count].pBGain[rolloffSize], 0.0f))
                {
                    pInput->pCalibrationTable[count].pBGain[rolloffSize] =
                        (pGoldenTableData->b_gain_tab.b_gain[rolloffSize] /
                            pInput->pCalibrationTable[count].pBGain[rolloffSize]);
                }

                if (FALSE == IQSettingUtils::FEqual(
                    pInput->pCalibrationTable[count].pRGain[rolloffSize], 0.0f))
                {
                    pInput->pCalibrationTable[count].pRGain[rolloffSize] =
                        (pGoldenTableData->r_gain_tab.r_gain[rolloffSize] /
                            pInput->pCalibrationTable[count].pRGain[rolloffSize]);
                }

                if (FALSE == IQSettingUtils::FEqual(
                    pInput->pCalibrationTable[count].pGBGain[rolloffSize], 0.0f))
                {
                    pInput->pCalibrationTable[count].pGBGain[rolloffSize] =
                        (pGoldenTableData->gb_gain_tab.gb_gain[rolloffSize] /
                            pInput->pCalibrationTable[count].pGBGain[rolloffSize]);
                }

                if (FALSE == IQSettingUtils::FEqual(
                    pInput->pCalibrationTable[count].pGRGain[rolloffSize], 0.0f))
                {
                    pInput->pCalibrationTable[count].pGRGain[rolloffSize] =
                        (pGoldenTableData->gr_gain_tab.gr_gain[rolloffSize] /
                            pInput->pCalibrationTable[count].pGRGain[rolloffSize]);
                }
            }
        }
    }

    if ((FALSE != result))
    {
        resultData = static_cast<lsc_4_0_0::lsc40_rgn_dataType*>(nodeSet[0].pData);

        if (TRUE == pInput->enableCalibration)
        {
            InterpolateAndCalibrate(resultData, pInput, pData, pInput->CCTTrigger);
        }
        else
        {
            IQSettingUtils::Memcpy(pData->r_gain_tab.r_gain,
                                   resultData->r_gain_tab.r_gain,
                                   (sizeof(FLOAT) * MESH_ROLLOFF40_SIZE));
            IQSettingUtils::Memcpy(pData->gr_gain_tab.gr_gain,
                                   resultData->gr_gain_tab.gr_gain,
                                   (sizeof(FLOAT)* MESH_ROLLOFF40_SIZE));
            IQSettingUtils::Memcpy(pData->gb_gain_tab.gb_gain,
                                   resultData->gb_gain_tab.gb_gain,
                                   (sizeof(FLOAT)* MESH_ROLLOFF40_SIZE));
            IQSettingUtils::Memcpy(pData->b_gain_tab.b_gain,
                                   resultData->b_gain_tab.b_gain,
                                   (sizeof(FLOAT)* MESH_ROLLOFF40_SIZE));
        }

        if ((NULL != pData) && (NULL != resultData))
        {
            pData->adaptive_gain_high       = resultData->adaptive_gain_high;
            pData->adaptive_gain_low        = resultData->adaptive_gain_low;
            pData->highlight_gain_strength  = resultData->highlight_gain_strength;
            pData->lowlight_gain_strength   = resultData->lowlight_gain_strength;
            pData->threshold_highlight      = resultData->threshold_highlight;
            pData->threshold_lowlight       = resultData->threshold_lowlight;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC40Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LSC40Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    lsc_4_0_0::lsc40_rgn_dataType* pInput1 = NULL;
    lsc_4_0_0::lsc40_rgn_dataType* pInput2 = NULL;
    lsc_4_0_0::lsc40_rgn_dataType* pOutput = NULL;
    BOOL                           result  = TRUE;

    if (NULL != pData1 && NULL != pData2 && NULL != pResult)
    {
        /// pInput1 and pInput2 are the Data regions and Data region 2
        ///    Region1           Interpolation     Region2
        ///    ---------- | ------------------- | ----------
        ///    ratio= 0.0......ratio (>0 && <1).....ratio = 1.0
        pInput1 =
            static_cast<lsc_4_0_0::lsc40_rgn_dataType*>(pData1);
        pInput2 =
            static_cast<lsc_4_0_0::lsc40_rgn_dataType*>(pData2);
        pOutput =
            static_cast<lsc_4_0_0::lsc40_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(lsc_4_0_0::lsc40_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = LSC40Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(lsc_4_0_0::lsc40_rgn_dataType));
            }
            else if (IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(lsc_4_0_0::lsc40_rgn_dataType));
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
// LSC40Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LSC40Interpolation::InterpolationData(
    lsc_4_0_0::lsc40_rgn_dataType* pInput1,
    lsc_4_0_0::lsc40_rgn_dataType* pInput2,
    FLOAT                          ratio,
    lsc_4_0_0::lsc40_rgn_dataType* pOutput)
{
    UINT count  = 0;
    BOOL result = TRUE;
    for (count = 0; count < MESH_ROLLOFF40_SIZE; count++)
    {
        pOutput->r_gain_tab.r_gain[count] =
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->r_gain_tab.r_gain[count]),
                                                       static_cast<FLOAT>(pInput2->r_gain_tab.r_gain[count]),
                                                       ratio);
        pOutput->b_gain_tab.b_gain[count] =
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->b_gain_tab.b_gain[count]),
                                                       static_cast<FLOAT>(pInput2->b_gain_tab.b_gain[count]),
                                                       ratio);
        pOutput->gb_gain_tab.gb_gain[count] =
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->gb_gain_tab.gb_gain[count]),
                                                       static_cast<FLOAT>(pInput2->gb_gain_tab.gb_gain[count]),
                                                       ratio);
        pOutput->gr_gain_tab.gr_gain[count] =
            IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->gr_gain_tab.gr_gain[count]),
                                                       static_cast<FLOAT>(pInput2->gr_gain_tab.gr_gain[count]),
                                                       ratio);
    }

    pOutput->threshold_highlight =
        static_cast<INT32>(IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->threshold_highlight),
            static_cast<FLOAT>(pInput2->threshold_highlight), ratio));

    pOutput->threshold_lowlight =
        static_cast<INT32>(IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->threshold_lowlight),
            static_cast<FLOAT>(pInput2->threshold_lowlight), ratio));

    pOutput->adaptive_gain_high =
        static_cast<INT32>(IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->adaptive_gain_high),
            static_cast<FLOAT>(pInput2->adaptive_gain_high), ratio));

    pOutput->adaptive_gain_low =
        static_cast<INT32>(IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->adaptive_gain_low),
            static_cast<FLOAT>(pInput2->adaptive_gain_low), ratio));

    pOutput->highlight_gain_strength =
        static_cast<INT32>(IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->highlight_gain_strength),
            static_cast<FLOAT>(pInput2->highlight_gain_strength), ratio));

    pOutput->lowlight_gain_strength =
        static_cast<INT32>(IQSettingUtils::InterpolationFloatBilinear(static_cast<FLOAT>(pInput1->lowlight_gain_strength),
            static_cast<FLOAT>(pInput2->lowlight_gain_strength), ratio));

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC40Interpolation::LensPositionSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LSC40Interpolation::LensPositionSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    lsc_4_0_0::chromatix_lsc40_coreType*   pParentDataType = NULL;
    LSC40TriggerList*  pTriggerList = NULL;

    if ((NULL != pParentNode) &&
        (NULL != pTriggerData) &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<lsc_4_0_0::chromatix_lsc40_coreType*>(pParentNode->pNodeData);
        regionNumber = pParentDataType->mod_lsc40_lens_position_dataCount;
        pTriggerList = static_cast<LSC40TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_lsc40_lens_position_data[count].lens_position_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerLensPosition;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
            static_cast<VOID*>(&pParentDataType->mod_lsc40_lens_position_data[regionOutput.startIndex]),
            NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_lsc40_lens_position_data[regionOutput.endIndex]),
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
// LSC40Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LSC40Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    lsc_4_0_0::mod_lsc40_lens_position_dataType*   pParentDataType = NULL;
    LSC40TriggerList*  pTriggerList = NULL;

    if ((NULL != pParentNode) &&
        (NULL != pTriggerData) &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<lsc_4_0_0::mod_lsc40_lens_position_dataType*>(pParentNode->pNodeData);
        regionNumber = pParentDataType->lens_position_data.mod_lsc40_drc_gain_dataCount;
        pTriggerList = static_cast<LSC40TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->lens_position_data.mod_lsc40_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
            static_cast<VOID*>(&pParentDataType->lens_position_data.mod_lsc40_drc_gain_data[regionOutput.startIndex]),
                                                   NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->lens_position_data.mod_lsc40_drc_gain_data[regionOutput.endIndex]),
                NULL);
            childCount++;
        }

        pParentNode->numChild = childCount;
    } else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        childCount = 0;
    }

    return childCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LSC40Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LSC40Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    lsc_4_0_0::mod_lsc40_drc_gain_dataType*   pParentDataType = NULL;
    LSC40TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<lsc_4_0_0::mod_lsc40_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_lsc40_hdr_aec_dataCount;
        pTriggerList    = static_cast<LSC40TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_lsc40_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_lsc40_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_lsc40_hdr_aec_data[regionOutput.endIndex]),
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
// LSC40Interpolation::LEDSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LSC40Interpolation::LEDSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    UINT32 regionNumber = 0;
    FLOAT  rationLED2   = 0.0f;
    FLOAT  triggerValue = 0.0f;

    InterpolationOutput regionOutput;

    lsc_4_0_0::mod_lsc40_hdr_aec_dataType*   pParentDataType = NULL;
    LSC40TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<lsc_4_0_0::mod_lsc40_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_lsc40_led_idx_dataCount;
        pTriggerList    = static_cast<LSC40TriggerList*>(pTriggerData);
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
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_lsc40_led_idx_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_lsc40_led_idx_data[regionOutput.endIndex]),
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
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_lsc40_led_idx_data[2]),
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
// LSC40Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LSC40Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    lsc_4_0_0::mod_lsc40_led_idx_dataType*   pParentDataType = NULL;
    LSC40TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<lsc_4_0_0::mod_lsc40_led_idx_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->led_idx_data.mod_lsc40_aec_dataCount;
        pTriggerList    = static_cast<LSC40TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->led_idx_data.mod_lsc40_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
            static_cast<VOID*>(&pParentDataType->led_idx_data.mod_lsc40_aec_data[regionOutput.startIndex]),
            NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->led_idx_data.mod_lsc40_aec_data[regionOutput.endIndex]),
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
// LSC40Interpolation::CCTSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LSC40Interpolation::CCTSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    lsc_4_0_0::mod_lsc40_aec_dataType*   pParentDataType = NULL;
    LSC40TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<lsc_4_0_0::mod_lsc40_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->aec_data.mod_lsc40_cct_dataCount;
        pTriggerList    = static_cast<LSC40TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->aec_data.mod_lsc40_cct_data[count].cct_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerCCT;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->aec_data.mod_lsc40_cct_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->aec_data.mod_lsc40_cct_data[regionOutput.startIndex].lsc40_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->aec_data.mod_lsc40_cct_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->aec_data.mod_lsc40_cct_data[regionOutput.endIndex].lsc40_rgn_data));
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
