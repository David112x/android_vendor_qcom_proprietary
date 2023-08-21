// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gtm10interpolation.cpp
/// @brief IFE GTM10 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "gtm10interpolation.h"
#include "gtm10setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation GTM10OperationTable[] =
{
    {GTM10Interpolation::DRCGainSearchNode, 2},
    {GTM10Interpolation::HDRAECSearchNode,  2},
    {GTM10Interpolation::AECSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GTM10Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GTM10Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    GTM10InputData*   pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain,  pInput->AECGain))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))                     ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->realGain          = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->DRCGain           = pInput->DRCGain;
        if (NULL != pInput->pParsedBHISTStats)
        {
            pTriggerData->pGRHist          = pInput->pParsedBHISTStats->BHistogramStats;
            pTriggerData->BHistRequestID   = pInput->pParsedBHISTStats->requestID;
            pTriggerData->maxPipelineDelay = pInput->maxPipelineDelay;
        }

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GTM10Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GTM10Interpolation::RunInterpolation(
    const GTM10InputData*          pInput,
    gtm_1_0_0::gtm10_rgn_dataType* pData)
{
    BOOL                          result = TRUE;
    ///< The interpolation tree total Node
    TuningNode                    nodeSet[GTM10MaxmiumNode];
    // GTM Trigger List
    GTM10TriggerList              GTMTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        ///< Initializes all the nodes
        for (UINT count = 0; count < GTM10MaxmiumNode; count++)
        {
            if (count < GTM10MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_gtm10_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&GTMTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(gtm_1_0_0::chromatix_gtm10Type::control_methodStruct));

        GTMTrigger.triggerDRCgain = pInput->DRCGain;
        GTMTrigger.triggerHDRAEC  = IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                                                     pInput->exposureTime,
                                                                     pInput->AECSensitivity,
                                                                     pInput->exposureGainRatio);
        GTMTrigger.triggerAEC     = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                                  pInput->luxIndex,
                                                                  pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        GTM10InterpolationLevel,
                                                        &GTM10OperationTable[0],
                                                        static_cast<VOID*>(&GTMTrigger));

        if (FALSE != result)
        {
            // Calculate the Interpolation Result
            result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                           GTM10MaxmiumNonLeafNode,
                                                           GTM10InterpolationLevel,
                                                           GTM10Interpolation::DoInterpolation);
        }
        if (FALSE != result)
        {
            IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(gtm_1_0_0::gtm10_rgn_dataType));
        }
    }
    else
    {
        /// @todo (CAMX-1460) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GTM10Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GTM10Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        gtm_1_0_0::gtm10_rgn_dataType* pInput1 = static_cast<gtm_1_0_0::gtm10_rgn_dataType*>(pData1);
        gtm_1_0_0::gtm10_rgn_dataType* pInput2 = static_cast<gtm_1_0_0::gtm10_rgn_dataType*>(pData2);
        gtm_1_0_0::gtm10_rgn_dataType* pOutput = static_cast<gtm_1_0_0::gtm10_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(gtm_1_0_0::gtm10_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = GTM10Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(gtm_1_0_0::gtm10_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(gtm_1_0_0::gtm10_rgn_dataType));
            }
            else
            {
                result = FALSE;
                /// @todo (CAMX-1460) Need to add logging for Common library
            }
        }
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1460) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GTM10Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GTM10Interpolation::InterpolationData(
    gtm_1_0_0::gtm10_rgn_dataType* pInput1,
    gtm_1_0_0::gtm10_rgn_dataType* pInput2,
    FLOAT                          ratio,
    gtm_1_0_0::gtm10_rgn_dataType* pOutput)
{
    UINT32 i;
    BOOL   result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        pOutput->maxval_th           = IQSettingUtils::InterpolationFloatBilinear(pInput1->maxval_th,
                                                                                  pInput2->maxval_th,
                                                                                  ratio);
        pOutput->key_min_th          = IQSettingUtils::InterpolationFloatBilinear(pInput1->key_min_th,
                                                                                  pInput2->key_min_th,
                                                                                  ratio);
        pOutput->key_max_th          = IQSettingUtils::InterpolationFloatBilinear(pInput1->key_max_th,
                                                                                  pInput2->key_max_th,
                                                                                  ratio);
        pOutput->key_hist_bin_weight = IQSettingUtils::InterpolationFloatBilinear(pInput1->key_hist_bin_weight,
                                                                                  pInput2->key_hist_bin_weight,
                                                                                  ratio);
        pOutput->yout_maxval         = IQSettingUtils::InterpolationFloatBilinear(pInput1->yout_maxval,
                                                                                  pInput2->yout_maxval,
                                                                                  ratio);
        pOutput->minval_th           = IQSettingUtils::InterpolationFloatBilinear(pInput1->minval_th,
                                                                                  pInput2->minval_th,
                                                                                  ratio);
        pOutput->a_middletone        = IQSettingUtils::InterpolationFloatBilinear(pInput1->a_middletone,
                                                                                  pInput2->a_middletone,
                                                                                  ratio);
        pOutput->middletone_w        = IQSettingUtils::InterpolationFloatBilinear(pInput1->middletone_w,
                                                                                  pInput2->middletone_w,
                                                                                  ratio);
        pOutput->temporal_w          = IQSettingUtils::InterpolationFloatBilinear(pInput1->temporal_w,
                                                                                  pInput2->temporal_w,
                                                                                  ratio);
        pOutput->max_percentile      = IQSettingUtils::InterpolationFloatBilinear(pInput1->max_percentile,
                                                                                  pInput2->max_percentile,
                                                                                  ratio);
        pOutput->min_percentile      = IQSettingUtils::InterpolationFloatBilinear(pInput1->min_percentile,
                                                                                  pInput2->min_percentile,
                                                                                  ratio);
        pOutput->reserved_1          = IQSettingUtils::InterpolationFloatBilinear(pInput1->reserved_1,
                                                                                  pInput2->reserved_1,
                                                                                  ratio);
        pOutput->reserved_2          = IQSettingUtils::InterpolationFloatBilinear(pInput1->reserved_2,
                                                                                  pInput2->reserved_2,
                                                                                  ratio);
        pOutput->extra_ratio_factor  = IQSettingUtils::InterpolationFloatBilinear(pInput1->extra_ratio_factor,
                                                                                  pInput2->extra_ratio_factor,
                                                                                  ratio);
        pOutput->dark_index_range    = IQSettingUtils::InterpolationFloatBilinear(pInput1->dark_index_range,
                                                                                  pInput2->dark_index_range,
                                                                                  ratio);

        /// Do interpolation over all the 65 chromatix entries
        for (i = 0; i < GTM10LUTSize + 1; i++)
        {
            pOutput->yratio_base_manual_tab.yratio_base_manual[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->yratio_base_manual_tab.yratio_base_manual[i],
                                                           pInput2->yratio_base_manual_tab.yratio_base_manual[i],
                                                           ratio);
        }

        pOutput->manual_curve_strength   = IQSettingUtils::InterpolationFloatBilinear(pInput1->manual_curve_strength,
                                                                                      pInput2->manual_curve_strength,
                                                                                      ratio);
        pOutput->midlight_threshold_low  = IQSettingUtils::InterpolationFloatBilinear(pInput1->midlight_threshold_low,
                                                                                      pInput2->midlight_threshold_low,
                                                                                      ratio);
        pOutput->midlight_threshold_high = IQSettingUtils::InterpolationFloatBilinear(pInput1->midlight_threshold_high,
                                                                                      pInput2->midlight_threshold_high,
                                                                                      ratio);
        pOutput->lowlight_w              = IQSettingUtils::InterpolationFloatBilinear(pInput1->lowlight_w,
                                                                                      pInput2->lowlight_w,
                                                                                      ratio);
        pOutput->highlight_w             = IQSettingUtils::InterpolationFloatBilinear(pInput1->highlight_w,
                                                                                      pInput2->highlight_w,
                                                                                      ratio);
        pOutput->max_ratio               = IQSettingUtils::InterpolationFloatBilinear(pInput1->max_ratio,
                                                                                      pInput2->max_ratio,
                                                                                      ratio);
        pOutput->luma_peak_th0           = IQSettingUtils::InterpolationFloatBilinear(pInput1->luma_peak_th0,
                                                                                      pInput2->luma_peak_th0,
                                                                                      ratio);
        pOutput->luma_peak_th1           = IQSettingUtils::InterpolationFloatBilinear(pInput1->luma_peak_th1,
                                                                                      pInput2->luma_peak_th1,
                                                                                      ratio);
        pOutput->stretch_gain_0          = IQSettingUtils::InterpolationFloatBilinear(pInput1->stretch_gain_0,
                                                                                      pInput2->stretch_gain_0,
                                                                                      ratio);
        pOutput->stretch_gain_1          = IQSettingUtils::InterpolationFloatBilinear(pInput1->stretch_gain_1,
                                                                                      pInput2->stretch_gain_1,
                                                                                      ratio);
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1460) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GTM10Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT GTM10Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gtm_1_0_0::chromatix_gtm10_coreType*   pParentDataType = NULL;
    GTM10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gtm_1_0_0::chromatix_gtm10_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_gtm10_drc_gain_dataCount;
        pTriggerList    = static_cast<GTM10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_gtm10_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_gtm10_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_gtm10_drc_gain_data[regionOutput.endIndex]),
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
// GTM10Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT GTM10Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gtm_1_0_0::mod_gtm10_drc_gain_dataType*   pParentDataType = NULL;
    GTM10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gtm_1_0_0::mod_gtm10_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_gtm10_hdr_aec_dataCount;
        pTriggerList    = static_cast<GTM10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_gtm10_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_gtm10_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_gtm10_hdr_aec_data[regionOutput.endIndex]),
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
// GTM10Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT GTM10Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gtm_1_0_0::mod_gtm10_hdr_aec_dataType*   pParentDataType = NULL;
    GTM10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gtm_1_0_0::mod_gtm10_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_gtm10_aec_dataCount;
        pTriggerList    = static_cast<GTM10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_gtm10_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gtm10_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gtm10_aec_data[regionOutput.startIndex].gtm10_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gtm10_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gtm10_aec_data[regionOutput.endIndex].gtm10_rgn_data));
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
