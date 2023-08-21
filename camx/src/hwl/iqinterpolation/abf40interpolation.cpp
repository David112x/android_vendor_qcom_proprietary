// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  abf40interpolation.cpp
/// @brief ABF40 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "abf40interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation ABF40OperationTable[] =
{
    {ABF40Interpolation::DRCGainSearchNode, 2},
    {ABF40Interpolation::HDRAECSearchNode,  2},
    {ABF40Interpolation::AECSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ABF40Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ABF40Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    ABF40InputData*   pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftGGainWB, pInput->AWBleftGGainWB))             ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftBGainWB, pInput->AWBleftBGainWB))             ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftRGainWB, pInput->AWBleftRGainWB))             ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->BLSData.exposureTime, pInput->AECexposureTime))   ||
        (pTriggerData->sensorOffsetX != pInput->sensorOffsetX)                                           ||
        (pTriggerData->sensorOffsetY != pInput->sensorOffsetY))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->realGain          = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->leftGGainWB       = pInput->AWBleftGGainWB;
        pTriggerData->leftBGainWB       = pInput->AWBleftBGainWB;
        pTriggerData->leftRGainWB       = pInput->AWBleftRGainWB;
        pTriggerData->DRCGain           = pInput->DRCGain;

        pTriggerData->BLSData.luxIndex           = pInput->AECLuxIndex;
        pTriggerData->BLSData.realGain           = pInput->AECGain;
        pTriggerData->BLSData.AECSensitivity     = pInput->AECSensitivity;
        pTriggerData->BLSData.CCTTrigger         = pInput->AWBColorTemperature;
        pTriggerData->BLSData.exposureTime       = pInput->AECexposureTime;
        pTriggerData->BLSData.exposureGainRatio  = pInput->AECexposureGainRatio;
        pTriggerData->BLSData.DRCGain            = pInput->DRCGain;
        pTriggerData->BLSData.LEDTrigger         = pInput->LEDSensitivity;
        pTriggerData->BLSData.numberOfLED        = pInput->numberOfLED;
        pTriggerData->BLSData.LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;

        pTriggerData->sensorOffsetX = pInput->sensorOffsetX;
        pTriggerData->sensorOffsetY = pInput->sensorOffsetY;
        pTriggerData->sensorWidth   = pInput->sensorImageWidth;
        pTriggerData->sensorHeight  = pInput->sensorImageHeight;

        isChanged = TRUE;
    }

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ABF40Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ABF40Interpolation::RunInterpolation(
    const ABF40InputData*          pInput,
    abf_4_0_0::abf40_rgn_dataType* pData )
{
    BOOL result = TRUE;

    // The interpolation tree total Node
    TuningNode nodeSet[ABF40MaxmiumNode];

    // BPCBCC Trigger List
    ABF40TriggerList ABF40Trigger;

    if ((NULL != pInput) && (NULL != pData) && ( NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < ABF40MaxmiumNode; count++)
        {
            if (count < ABF40MaxmiumNoLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_abf40_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&ABF40Trigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(abf_4_0_0::chromatix_abf40Type::control_methodStruct));

        ABF40Trigger.triggerDRCgain = pInput->DRCGain;

        ABF40Trigger.triggerHDRAEC =
            IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                             pInput->exposureTime,
                                             pInput->AECSensitivity,
                                             pInput->exposureGainRatio);

        ABF40Trigger.triggerAEC =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        ABF40InterpolationLevel,
                                                        &ABF40OperationTable[0],
                                                        static_cast<VOID*>(&ABF40Trigger));
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
                                                       ABF40MaxmiumNoLeafNode,
                                                       ABF40InterpolationLevel,
                                                       ABF40Interpolation::DoInterpolation);
    }

    if (TRUE == result)
    {
        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(abf_4_0_0::abf40_rgn_dataType));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ABF40Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ABF40Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        /// pInput1 and pInput2 are the Data regions and Data region 2
        ///    Region1           Interpolation     Region2
        ///    ---------- | ------------------- | ----------
        ///    ratio= 0.0......ratio (>0 && <1).....ratio = 1.0
        abf_4_0_0::abf40_rgn_dataType* pInput1 = static_cast<abf_4_0_0::abf40_rgn_dataType*>(pData1);
        abf_4_0_0::abf40_rgn_dataType* pInput2 = static_cast<abf_4_0_0::abf40_rgn_dataType*>(pData2);
        abf_4_0_0::abf40_rgn_dataType* pOutput = static_cast<abf_4_0_0::abf40_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(abf_4_0_0::abf40_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = ABF40Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(abf_4_0_0::abf40_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(abf_4_0_0::abf40_rgn_dataType));
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
// ABF40Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ABF40Interpolation::InterpolationData(
    abf_4_0_0::abf40_rgn_dataType* pInput1,
    abf_4_0_0::abf40_rgn_dataType* pInput2,
    FLOAT                          ratio,
    abf_4_0_0::abf40_rgn_dataType* pOutput)
{
    INT  i;
    BOOL result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        pOutput->act_fac0       = IQSettingUtils::InterpolationFloatBilinear(pInput1->act_fac0, pInput2->act_fac0, ratio);
        pOutput->act_fac1       = IQSettingUtils::InterpolationFloatBilinear(pInput1->act_fac1, pInput2->act_fac1, ratio);
        pOutput->act_smth_thd0  = IQSettingUtils::InterpolationFloatBilinear(pInput1->act_smth_thd0,
                                                                             pInput2->act_smth_thd0,
                                                                             ratio);
        pOutput->act_smth_thd1  = IQSettingUtils::InterpolationFloatBilinear(pInput1->act_smth_thd1,
                                                                             pInput2->act_smth_thd1,
                                                                             ratio);
        pOutput->act_thd0       = IQSettingUtils::InterpolationFloatBilinear(pInput1->act_thd0, pInput2->act_thd0, ratio);
        pOutput->act_thd1       = IQSettingUtils::InterpolationFloatBilinear(pInput1->act_thd1, pInput2->act_thd1, ratio);
        pOutput->dark_thd       = IQSettingUtils::InterpolationFloatBilinear(pInput1->dark_thd, pInput2->dark_thd, ratio);
        pOutput->edge_count_low = IQSettingUtils::InterpolationFloatBilinear(pInput1->edge_count_low,
                                                                             pInput2->edge_count_low,
                                                                             ratio);

        pOutput->edge_detect_noise_scaler = IQSettingUtils::InterpolationFloatBilinear(pInput1->edge_detect_noise_scaler,
                                                                                       pInput2->edge_detect_noise_scaler,
                                                                                       ratio);
        pOutput->edge_detect_thd          = IQSettingUtils::InterpolationFloatBilinear(pInput1->edge_detect_thd,
                                                                                       pInput2->edge_detect_thd,
                                                                                       ratio);
        pOutput->edge_smooth_strength     = IQSettingUtils::InterpolationFloatBilinear(pInput1->edge_smooth_strength,
                                                                                       pInput2->edge_smooth_strength,
                                                                                       ratio);

        pOutput->minmax_bls     = IQSettingUtils::InterpolationFloatBilinear(pInput1->minmax_bls, pInput2->minmax_bls, ratio);
        pOutput->minmax_maxshft = IQSettingUtils::InterpolationFloatBilinear(pInput1->minmax_maxshft,
                                                                             pInput2->minmax_maxshft,
                                                                             ratio);
        pOutput->minmax_minshft = IQSettingUtils::InterpolationFloatBilinear(pInput1->minmax_minshft,
                                                                             pInput2->minmax_minshft,
                                                                             ratio);
        pOutput->minmax_offset  = IQSettingUtils::InterpolationFloatBilinear(pInput1->minmax_offset,
                                                                             pInput2->minmax_offset,
                                                                             ratio);

        for (i = 0; i < 32; i++)
        {
            pOutput->act_fac_lut_tab.act_fac_lut[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->act_fac_lut_tab.act_fac_lut[i],
                                                           pInput2->act_fac_lut_tab.act_fac_lut[i],
                                                           ratio);
        }

        for (i = 0; i < 2; i++)
        {
            pOutput->blkpix_lev_tab.blkpix_lev[i] = pInput1->blkpix_lev_tab.blkpix_lev[i];
        }

        for (i = 0; i < 65; i++)
        {
            pOutput->noise_std_lut_tab.noise_std_lut[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->noise_std_lut_tab.noise_std_lut[i],
                                                           pInput2->noise_std_lut_tab.noise_std_lut[i],
                                                           ratio);
        }

        for (i = 0; i < 42; i++)
        {
            pOutput->dark_fac_lut_tab.dark_fac_lut[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->dark_fac_lut_tab.dark_fac_lut[i],
                                                           pInput2->dark_fac_lut_tab.dark_fac_lut[i],
                                                           ratio);
        }

        for (i = 0; i < 18; i++)
        {
            pOutput->dist_ker_tab.dist_ker[i] = pInput1->dist_ker_tab.dist_ker[i];
        }

        for (i = 0; i < 4; i++)
        {
            pOutput->denoise_strength_tab.denoise_strength[i]     =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->denoise_strength_tab.denoise_strength[i],
                                                           pInput2->denoise_strength_tab.denoise_strength[i],
                                                           ratio);
            pOutput->curve_offset_tab.curve_offset[i]             =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->curve_offset_tab.curve_offset[i],
                                                           pInput2->curve_offset_tab.curve_offset[i],
                                                           ratio);
            pOutput->edge_smooth_scaler_tab.edge_smooth_scaler[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->edge_smooth_scaler_tab.edge_smooth_scaler[i],
                                                           pInput2->edge_smooth_scaler_tab.edge_smooth_scaler[i],
                                                           ratio);
            pOutput->edge_softness_tab.edge_softness[i]           =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->edge_softness_tab.edge_softness[i],
                                                           pInput2->edge_softness_tab.edge_softness[i],
                                                           ratio);
        }

        for (i = 0; i < 5; i++)
        {
            pOutput->noise_prsv_anchor_tab.noise_prsv_anchor[i]               =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->noise_prsv_anchor_tab.noise_prsv_anchor[i],
                                                           pInput2->noise_prsv_anchor_tab.noise_prsv_anchor[i],
                                                           ratio);
            pOutput->radial_edge_softness_adj_tab.radial_edge_softness_adj[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->radial_edge_softness_adj_tab.radial_edge_softness_adj[i],
                                                           pInput2->radial_edge_softness_adj_tab.radial_edge_softness_adj[i],
                                                           ratio);
            pOutput->radial_noise_prsv_adj_tab.radial_noise_prsv_adj[i]       =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->radial_noise_prsv_adj_tab.radial_noise_prsv_adj[i],
                                                           pInput2->radial_noise_prsv_adj_tab.radial_noise_prsv_adj[i],
                                                           ratio);
        }

        for (i = 0; i < 10; i++)
        {
            pOutput->noise_prsv_base_tab.noise_prsv_base[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->noise_prsv_base_tab.noise_prsv_base[i],
                    pInput2->noise_prsv_base_tab.noise_prsv_base[i],
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
// ABF40Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ABF40Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    abf_4_0_0::chromatix_abf40_coreType*   pParentDataType = NULL;
    ABF40TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<abf_4_0_0::chromatix_abf40_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_abf40_drc_gain_dataCount;
        pTriggerList    = static_cast<ABF40TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_abf40_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_abf40_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_abf40_drc_gain_data[regionOutput.endIndex]),
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
// ABF40Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ABF40Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    abf_4_0_0::mod_abf40_drc_gain_dataType*   pParentDataType = NULL;
    ABF40TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<abf_4_0_0::mod_abf40_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_abf40_hdr_aec_dataCount;
        pTriggerList    = static_cast<ABF40TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_abf40_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_abf40_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_abf40_hdr_aec_data[regionOutput.endIndex]),
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
// ABF40Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ABF40Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    abf_4_0_0::mod_abf40_hdr_aec_dataType*   pParentDataType = NULL;
    ABF40TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<abf_4_0_0::mod_abf40_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_abf40_aec_dataCount;
        pTriggerList    = static_cast<ABF40TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_abf40_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_abf40_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_abf40_aec_data[regionOutput.startIndex].abf40_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_abf40_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_abf40_aec_data[regionOutput.endIndex].abf40_rgn_data));
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
