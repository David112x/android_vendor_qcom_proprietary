// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  tmc11interpolation.cpp
/// @brief TMC11 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "tmc11interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation TMC11OperationTable[] =
{
    {TMC11Interpolation::DRCGainSearchNode, 2},
    {TMC11Interpolation::HDRAECSearchNode,  2},
    {TMC11Interpolation::AECSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC11Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC11Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData*    pInput,
    TMC11InputData* pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGainDark, pInput->DRCGainDark))                ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain)))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->realGain          = pInput->AECGain;
        pTriggerData->DRCGain           = pInput->DRCGain;
        pTriggerData->DRCGainDark       = pInput->DRCGainDark;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;

        if (NULL != pInput->pParsedBHISTStats)
        {
            pTriggerData->pGRHist = pInput->pParsedBHISTStats->BHistogramStats;
        }

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC11Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC11Interpolation::RunInterpolation(
    const TMC11InputData*               pInput,
    tmc_1_1_0::tmc11_rgn_dataType* pData)
{
    BOOL                                    result = TRUE;
    // The interpolation tree total Node
    TuningNode                              nodeSet[TMC11MaximumNode];
    TMC11TriggerList                        tmcTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < TMC11MaximumNode; count++)
        {
            if (count < TMC11MaximumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_tmc11_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&tmcTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(tmc_1_1_0::chromatix_tmc11Type::control_methodStruct));

        tmcTrigger.triggerDRCgain = pInput->DRCGain;

        tmcTrigger.triggerHDRAEC  =
            IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                             pInput->exposureTime,
                                             pInput->AECSensitivity,
                                             pInput->exposureGainRatio);

        tmcTrigger.triggerAEC     =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        TMC11InterpolationLevel,
                                                        &TMC11OperationTable[0],
                                                        static_cast<VOID*>(&tmcTrigger));
        if (FALSE != result)
        {
            // Calculate the Interpolation Result
            result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                           TMC11MaximumNonLeafNode,
                                                           TMC11InterpolationLevel,
                                                           TMC11Interpolation::DoInterpolation);
            if (FALSE != result)
            {
                IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(tmc_1_1_0::tmc11_rgn_dataType));

                // Calculate ADRC Gain Curve
                CalcGainCurve(pInput, pData);
            }
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
/// TMC11Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC11Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    tmc_1_1_0::tmc11_rgn_dataType* pInput1 = NULL;
    tmc_1_1_0::tmc11_rgn_dataType* pInput2 = NULL;
    tmc_1_1_0::tmc11_rgn_dataType* pOutput = NULL;
    BOOL                                     result  = TRUE;

    if ((NULL != pData1) &&
        (NULL != pData2) &&
        (NULL != pResult))
    {
        pInput1 = static_cast<tmc_1_1_0::tmc11_rgn_dataType*>(pData1);
        pInput2 = static_cast<tmc_1_1_0::tmc11_rgn_dataType*>(pData2);
        pOutput = static_cast<tmc_1_1_0::tmc11_rgn_dataType*>(pResult);


        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(tmc_1_1_0::tmc11_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = TMC11Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(tmc_1_1_0::tmc11_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(tmc_1_1_0::tmc11_rgn_dataType));
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
/// TMC11Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC11Interpolation::InterpolationData(
    tmc_1_1_0::tmc11_rgn_dataType* pInput1,
    tmc_1_1_0::tmc11_rgn_dataType* pInput2,
    FLOAT                          ratio,
    tmc_1_1_0::tmc11_rgn_dataType* pOutput)
{
    FLOAT result    = 0.0f;
    BOOL  resultAPI = TRUE;

    if ((NULL != pInput1) &&
        (NULL != pInput2) &&
        (NULL != pOutput))
    {
        pOutput->gtm_percentage = IQSettingUtils::InterpolationFloatBilinear(pInput1->gtm_percentage,
                                                                              pInput2->gtm_percentage,
                                                                              ratio);

        pOutput->ltm_percentage = IQSettingUtils::InterpolationFloatBilinear(pInput1->ltm_percentage,
                                                                              pInput2->ltm_percentage,
                                                                              ratio);

        pOutput->dark_boost_ratio = IQSettingUtils::InterpolationFloatBilinear(pInput1->dark_boost_ratio,
                                                                              pInput2->dark_boost_ratio,
                                                                              ratio);

        pOutput->tone_bright_adj = IQSettingUtils::InterpolationFloatBilinear(pInput1->tone_bright_adj,
                                                                              pInput2->tone_bright_adj,
                                                                              ratio);

        pOutput->tone_dark_adj   = IQSettingUtils::InterpolationFloatBilinear(pInput1->tone_dark_adj,
                                                                              pInput2->tone_dark_adj,
                                                                              ratio);

        pOutput->stretch_bright_str = IQSettingUtils::InterpolationFloatBilinear(pInput1->stretch_bright_str,
                                                                              pInput2->stretch_bright_str,
                                                                              ratio);

        pOutput->stretch_dark_str = IQSettingUtils::InterpolationFloatBilinear(pInput1->stretch_dark_str,
                                                                              pInput2->stretch_dark_str,
                                                                              ratio);

        pOutput->hist_supr_range_start = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_supr_range_start,
                                                                              pInput2->hist_supr_range_start,
                                                                              ratio);

        pOutput->hist_supr_range_end   = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_supr_range_end,
                                                                              pInput2->hist_supr_range_end,
                                                                              ratio);

        pOutput->hist_boost_range_start = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_boost_range_start,
                                                                              pInput2->hist_boost_range_start,
                                                                              ratio);

        pOutput->hist_boost_range_end = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_boost_range_end,
                                                                              pInput2->hist_boost_range_end,
                                                                              ratio);

        pOutput->hist_avg_range_start = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_avg_range_start,
                                                                              pInput2->hist_avg_range_start,
                                                                              ratio);

        pOutput->hist_avg_range_end   = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_avg_range_end,
                                                                              pInput2->hist_avg_range_end,
                                                                              ratio);

        pOutput->hist_clip_slope = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_clip_slope,
                                                                              pInput2->hist_clip_slope,
                                                                              ratio);

        pOutput->hist_enhance_clamp = IQSettingUtils::InterpolationFloatBilinear(pInput1->hist_enhance_clamp,
                                                                              pInput2->hist_enhance_clamp,
                                                                              ratio);

        pOutput->contrast_bright_clip = IQSettingUtils::InterpolationFloatBilinear(pInput1->contrast_bright_clip,
                                                                              pInput2->contrast_bright_clip,
                                                                              ratio);

        pOutput->contrast_dark_adj   = IQSettingUtils::InterpolationFloatBilinear(pInput1->contrast_dark_adj,
                                                                              pInput2->contrast_dark_adj,
                                                                              ratio);

        pOutput->contrast_he_bright = IQSettingUtils::InterpolationFloatBilinear(pInput1->contrast_he_bright,
                                                                              pInput2->contrast_he_bright,
                                                                              ratio);

        pOutput->contrast_he_dark = IQSettingUtils::InterpolationFloatBilinear(pInput1->contrast_he_dark,
                                                                              pInput2->contrast_he_dark,
                                                                              ratio);

        pOutput->core_rsv_para1 = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para1,
                                                                              pInput2->core_rsv_para1,
                                                                              ratio);

        pOutput->core_rsv_para2   = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para2,
                                                                              pInput2->core_rsv_para2,
                                                                              ratio);

        pOutput->core_rsv_para3 = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para3,
                                                                              pInput2->core_rsv_para3,
                                                                              ratio);

        pOutput->core_rsv_para4 = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para4,
                                                                              pInput2->core_rsv_para4,
                                                                              ratio);

        pOutput->core_rsv_para5 = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para5,
                                                                              pInput2->core_rsv_para5,
                                                                              ratio);

        for (UINT32 i = 0; i < 5; i++)
        {
            pOutput->tone_target_tab.tone_target[i] = IQSettingUtils::InterpolationFloatBilinear(
                                                          pInput1->tone_target_tab.tone_target[i],
                                                          pInput2->tone_target_tab.tone_target[i],
                                                          ratio);
            pOutput->tone_anchors_tab.tone_anchors[i] = IQSettingUtils::InterpolationFloatBilinear(
                                                          pInput1->tone_anchors_tab.tone_anchors[i],
                                                          pInput2->tone_anchors_tab.tone_anchors[i],
                                                          ratio);
            pOutput->hist_conv_kernel_tab.hist_conv_kernel[i] = IQSettingUtils::InterpolationFloatBilinear(
                                                                    pInput1->hist_conv_kernel_tab.hist_conv_kernel[i],
                                                                    pInput2->hist_conv_kernel_tab.hist_conv_kernel[i],
                                                                    ratio);
            pOutput->hist_enhance_ratio_tab.hist_enhance_ratio[i] = IQSettingUtils::InterpolationFloatBilinear(
                                                                        pInput1->hist_enhance_ratio_tab.hist_enhance_ratio[i],
                                                                        pInput2->hist_enhance_ratio_tab.hist_enhance_ratio[i],
                                                                        ratio);
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
// TMC11Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TMC11Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tmc_1_1_0::chromatix_tmc11_coreType*   pParentDataType = NULL;
    TMC11TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tmc_1_1_0::chromatix_tmc11_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_tmc11_drc_gain_dataCount;
        pTriggerList    = static_cast<TMC11TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_tmc11_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_tmc11_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_tmc11_drc_gain_data[regionOutput.endIndex]),
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
// TMC11Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TMC11Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tmc_1_1_0::mod_tmc11_drc_gain_dataType*   pParentDataType = NULL;
    TMC11TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tmc_1_1_0::mod_tmc11_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_tmc11_hdr_aec_dataCount;
        pTriggerList    = static_cast<TMC11TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_tmc11_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_tmc11_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_tmc11_hdr_aec_data[regionOutput.endIndex]),
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
// TMC11Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TMC11Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tmc_1_1_0::mod_tmc11_hdr_aec_dataType*   pParentDataType = NULL;
    TMC11TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tmc_1_1_0::mod_tmc11_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_tmc11_aec_dataCount;
        pTriggerList    = static_cast<TMC11TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_tmc11_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_tmc11_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>
              (&pParentDataType->hdr_aec_data.mod_tmc11_aec_data[regionOutput.startIndex].tmc11_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_tmc11_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>
                    (&pParentDataType->hdr_aec_data.mod_tmc11_aec_data[regionOutput.endIndex].tmc11_rgn_data));
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
/// TMC11Interpolation::CalcGainCurve
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC11Interpolation::CalcGainCurve(
    const TMC11InputData*          pInput,
    tmc_1_1_0::tmc11_rgn_dataType* pTmcData)
{
    UINT32 drcIdx           = 0;
    FLOAT  drcGain          = 0.0f;
    FLOAT  gainRatio        = 0.0f;
    FLOAT  drcGainDark      = 0.0f;
    FLOAT  kneeMidX         = 0.0f;
    FLOAT  kneeDarkX        = 0.0f;
    FLOAT  kneeDarkY        = 0.0f;
    FLOAT  kneeX[TMC11_KNEE_POINTS_NUM];
    FLOAT  kneeY[TMC11_KNEE_POINTS_NUM];
    FLOAT  maxLTMPercengate = 0.0f;
    FLOAT  pchipCoeffficient[MAX_ADRC_LUT_PCHIP_COEF_SIZE];
    FLOAT  contrastEnhanceCurve[MAX_ADRC_CONTRAST_CURVE];
    FLOAT  anchorKneeX[TMC11_KNEE_POINTS_NUM];
    FLOAT  anchorKneeY[TMC11_KNEE_POINTS_NUM];
    FLOAT  histKneeX[TMC11_KNEE_POINTS_NUM];
    FLOAT  histKneeY[TMC11_KNEE_POINTS_NUM];
    FLOAT  processedHist[TMC11_HIST_BIN_NUM];

    drcIdx      = IQSettingUtils::MinUINT32(static_cast<UINT32>(round(log(pInput->DRCGain) / log(TMC11_UNIT_GAIN_STEP))),
                                            TMC11_DRC_INDEX_MIN);

    drcGain     = powf(TMC11_DRC_GAIN_POW_X, static_cast<FLOAT>(drcIdx));

    gainRatio   = IQSettingUtils::ClampFLOAT((pInput->DRCGainDark - 1) * pTmcData->dark_boost_ratio + 1 +
                                              pTmcData->core_rsv_para1, TMC11_GAIN_RATIO_MIN, TMC11_GAIN_RATIO_MAX);

    drcGainDark = drcGain * gainRatio;

    drcIdx      = IQSettingUtils::MinUINT32(static_cast<UINT32>(round(log(drcGainDark) / log(TMC11_UNIT_GAIN_STEP))),
                                            TMC11_DRC_INDEX_MIN);

    maxLTMPercengate = logf(TMC11_MAX_LTM_GAIN) / logf(powf(TMC11_DRC_GAIN_POW_X, static_cast<FLOAT>(drcIdx)));
    if (maxLTMPercengate > 1)
    {
        maxLTMPercengate = 1;
    }

    if (pTmcData->ltm_percentage > maxLTMPercengate)
    {
        pTmcData->ltm_percentage = maxLTMPercengate;
        pTmcData->gtm_percentage = 1 - pTmcData->ltm_percentage;
    }

    if (TRUE == IQSettingUtils::FEqual(gainRatio, TMC11_GAIN_RATIO_MIN))
    {
        gainRatio = 1.0001f;
    }

    kneeMidX  = pTmcData->tone_anchors_tab.tone_anchors[2] / drcGain;
    kneeDarkY = pTmcData->tone_anchors_tab.tone_anchors[2] / gainRatio;
    kneeDarkY = IQSettingUtils::ClampFLOAT(kneeDarkY, kneeDarkY, pTmcData->tone_anchors_tab.tone_anchors[1]);
    kneeDarkX = kneeDarkY / drcGainDark;

    anchorKneeX[0] = 0.0f;
    anchorKneeX[1] = kneeDarkX;
    anchorKneeX[2] = kneeMidX;
    anchorKneeX[3] = pTmcData->tone_anchors_tab.tone_anchors[3];
    anchorKneeX[4] = 1.0f;

    anchorKneeY[0] = 0.0f;
    anchorKneeY[1] = kneeDarkY;
    anchorKneeY[2] = pTmcData->tone_anchors_tab.tone_anchors[2];
    anchorKneeY[3] = pTmcData->tone_anchors_tab.tone_anchors[3];
    anchorKneeY[4] = 1.0f;

    if (NULL != pInput->pGRHist)
    {
        HistogramPreProcess(pTmcData, pInput->pGRHist, processedHist);
        CalculateHistKneePoints(pInput, pTmcData, histKneeX, histKneeY, processedHist);
    }
    else
    {
        for (UINT32 i = 0; i < TMC11_KNEE_POINTS_NUM; i++)
        {
            histKneeX[i] = 1.0f;
            histKneeY[i] = 1.0f;
        }
    }

    // blend knee points
    for (UINT32 i = 0; i < TMC11_KNEE_POINTS_NUM; i++)
    {
        kneeX[i] = pTmcData->hist_enhance_ratio_tab.hist_enhance_ratio[i] * histKneeX[i] +
                   (1 - pTmcData->hist_enhance_ratio_tab.hist_enhance_ratio[i]) * anchorKneeX[i];
        kneeY[i] = pTmcData->hist_enhance_ratio_tab.hist_enhance_ratio[i] * histKneeY[i] +
                   (1 - pTmcData->hist_enhance_ratio_tab.hist_enhance_ratio[i]) * anchorKneeY[i];
    }

    for (UINT32 i = 1; i < TMC11_KNEE_POINTS_NUM; i++)
    {
        if ((kneeX[i] - kneeX[i - 1]) <= 0.0f)
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
        }
    }

    for (UINT32 i = 1; i < TMC11_KNEE_POINTS_NUM; i++)
    {
        if ((kneeY[i] - kneeY[i - 1]) <= 0.0f)
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
        }
    }

    CalculateKneePoints(kneeX, kneeY, pchipCoeffficient);


    if (NULL != pInput->pGRHist)
    {
        CalculateContrastEnhanceCurve(pInput, processedHist, contrastEnhanceCurve);
    }
    else
    {
        IQSettingUtils::Memset(contrastEnhanceCurve, 1.0f, sizeof(FLOAT) * MAX_ADRC_CONTRAST_CURVE);
    }

    pInput->pAdrcOutputData->enable        = pInput->pChromatix->enable_section.tmc_enable;
    pInput->pAdrcOutputData->version       = SWTMCVersion::TMC11;
    pInput->pAdrcOutputData->gtmEnable     = pInput->pChromatix->chromatix_tmc11_reserve.use_gtm;
    pInput->pAdrcOutputData->ltmEnable     = pInput->pChromatix->chromatix_tmc11_reserve.use_ltm;
    pInput->pAdrcOutputData->ltmPercentage = pTmcData->ltm_percentage;
    pInput->pAdrcOutputData->gtmPercentage = pTmcData->gtm_percentage;
    IQSettingUtils::Memcpy(&pInput->pAdrcOutputData->kneePoints.KneePointsTMC11.kneeX[0],
                           &kneeX[0],
                           sizeof(FLOAT) * TMC11_KNEE_POINTS_NUM);
    IQSettingUtils::Memcpy(&pInput->pAdrcOutputData->kneePoints.KneePointsTMC11.kneeY[0],
                           &kneeY[0],
                           sizeof(FLOAT) * TMC11_KNEE_POINTS_NUM);
    IQSettingUtils::Memcpy(&pInput->pAdrcOutputData->pchipCoeffficient[0], &pchipCoeffficient[0],
                           sizeof(FLOAT) * MAX_ADRC_LUT_PCHIP_COEF_SIZE);
    IQSettingUtils::Memcpy(&pInput->pAdrcOutputData->contrastEnhanceCurve[0], &contrastEnhanceCurve[0],
                           sizeof(FLOAT) * MAX_ADRC_CONTRAST_CURVE);

    pInput->pAdrcOutputData->drcGainDark      = drcGainDark;
    pInput->pAdrcOutputData->curveModel       = pInput->pChromatix->chromatix_tmc11_reserve.curve_model;
    pInput->pAdrcOutputData->contrastHEBright = pTmcData->contrast_he_bright;
    pInput->pAdrcOutputData->contrastHEDark   = pTmcData->contrast_he_dark;

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC11Interpolation::CalculateKneePoints
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC11Interpolation::CalculateKneePoints(
    FLOAT* pIn,
    FLOAT* pOut,
    FLOAT* pCoef)
{
    FLOAT tmp;
    FLOAT w1;
    FLOAT w2;
    FLOAT h[TMC11_KNEE_POINTS_NUM - 1];
    FLOAT delta[TMC11_KNEE_POINTS_NUM - 1];
    FLOAT b[TMC11_KNEE_POINTS_NUM];
    FLOAT c[TMC11_KNEE_POINTS_NUM];
    FLOAT d[TMC11_KNEE_POINTS_NUM];;

    // prepare diff
    for (UINT32 i = 0; i < TMC11_KNEE_POINTS_NUM - 1; i++)
    {
        h[i] = pIn[i + 1] - pIn[i];
        delta[i] = (pOut[i + 1] - pOut[i]) / h[i];
    }

    // calculate d
    for (UINT32 i = 0; i < TMC11_KNEE_POINTS_NUM; i++)
    {
        if (i == 0)
        {
            tmp = ((2 * h[i] + h[i + 1]) * delta[i] - h[i] * delta[i + 1]) / (h[i] + h[i + 1]);
            d[i] = (delta[i] * delta[i + 1] < 0 && IQSettingUtils::AbsoluteFLOAT(tmp) >
                    IQSettingUtils::AbsoluteFLOAT(3 * delta[i])) ?
                    3 * delta[i] : (tmp * delta[i] < 0) ? 0 : tmp;
        }
        else if (i == TMC11_KNEE_POINTS_NUM - 1)
        {
            tmp = ((2 * h[i - 1] + h[i - 2]) * delta[i - 1] - h[i - 1] * delta[i - 2]) / (h[i - 2] + h[i - 1]);
            d[i] = (delta[i - 2] * delta[i - 1] < 0 && IQSettingUtils::AbsoluteFLOAT(tmp) >
                    IQSettingUtils::AbsoluteFLOAT(3 * delta[i - 1])) ?
                    3 * delta[i - 1] : (tmp * delta[i - 1] < 0) ? 0 : tmp;
        }
        else
        {
            w1 = 2 * h[i] + h[i - 1];
            w2 = h[i] + 2 * h[i - 1];
            d[i] = (w1 + w2) / (w1 / delta[i - 1] + w2 / delta[i]);
        }
    }

    // calcualte b and c
    for (UINT32 i = 0; i < TMC11_KNEE_POINTS_NUM - 1; i++)
    {
        b[i] = (d[i] - 2 * delta[i] + d[i + 1]) / h[i] / h[i];
        c[i] = (3 * delta[i] - 2 * d[i] - d[i + 1]) / h[i];
    }

    // fill output
    for (UINT32 i = 0; i < TMC11_KNEE_POINTS_NUM - 2; i++)
    {
        pCoef[i * 3 + 0] = d[i + 1];
        pCoef[i * 3 + 1] = c[i + 1];
        pCoef[i * 3 + 2] = b[i + 1];
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC11Interpolation::HistogramPreProcess
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC11Interpolation::HistogramPreProcess(
    tmc_1_1_0::tmc11_rgn_dataType* pTmcData,
    UINT32*                        pOriHist,
    FLOAT*                         pTmcHist)
{
    UINT32 bayer_stat_buf[TMC11_HIST_BIN_NUM];
    IQSettingUtils::Memcpy(bayer_stat_buf, pOriHist, TMC11_HIST_BIN_NUM * sizeof(UINT32));

    // get histogram total count
    float n_hist = 0;
    for (UINT32 i = 0; i < TMC11_HIST_BIN_NUM; i++)
    {
        n_hist += bayer_stat_buf[i];
    }

    // refine histogram to limit contrast
    float hist_max = pTmcData->hist_clip_slope * n_hist / TMC11_HIST_BIN_NUM;
    float hist_clipped = 0.0f;

    for (UINT32 i = 0; i < TMC11_HIST_BIN_NUM; i++)
    {
        if (bayer_stat_buf[i] < hist_max)
        {
            pTmcHist[i] = static_cast<FLOAT>(bayer_stat_buf[i]);
        }
        else
        {
            pTmcHist[i] = hist_max;
            hist_clipped += static_cast<FLOAT>(bayer_stat_buf[i]) - hist_max;
        }
    }

    // remove under-saturated pixels
    if (pTmcHist[0] > pTmcHist[1])
    {
        hist_clipped += pTmcHist[0] - pTmcHist[1];
        pTmcHist[0] = pTmcHist[1];
    }

    // normalize bins
    FLOAT avgHistClipped = hist_clipped / TMC11_HIST_BIN_NUM;
    for (UINT32 i = 0; i < TMC11_HIST_BIN_NUM; i++)
    {
        pTmcHist[i] += avgHistClipped;
    }

    UINT32 bin_index_l;
    UINT32 bin_index_r;
    // smooth bins
    for (UINT32 j = 0; j < 4; j++)
    {
        for (UINT32 i = 0; i < TMC11_HIST_BIN_NUM; i++)
        {
            pTmcHist[i] = pTmcHist[i] * pTmcData->hist_conv_kernel_tab.hist_conv_kernel[0];
            for (UINT32 k = 1; k < 5; k++)
            {
                bin_index_r = IQSettingUtils::ClampUINT32(i + k, 0, TMC11_HIST_BIN_NUM - 1);
                bin_index_l = (i < k) ? 0 : IQSettingUtils::ClampUINT32(i - k, 0, TMC11_HIST_BIN_NUM - 1);
                pTmcHist[i] += pTmcHist[bin_index_r] * pTmcData->hist_conv_kernel_tab.hist_conv_kernel[k];
                pTmcHist[i] += pTmcHist[bin_index_l] * pTmcData->hist_conv_kernel_tab.hist_conv_kernel[k];
            }
        }
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC11Interpolation::CalculateHistKneePoints
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC11Interpolation::CalculateHistKneePoints(
    const TMC11InputData*          pInput,
    tmc_1_1_0::tmc11_rgn_dataType* pTmcData,
    FLOAT*                         kneeX,
    FLOAT*                         kneeY,
    FLOAT*                         pTmcHist)
{
    const FLOAT* pGamma15 = pInput->IPEGammaOutput;

    // get histogram total count
    FLOAT n_hist = 0;
    for (UINT32 i = 0; i < TMC11_HIST_BIN_NUM; i++)
    {
        n_hist += pTmcHist[i];
    }

    FLOAT sum_all = 0.0;
    FLOAT sum_all_cnt = 0;
    FLOAT sum_max = 0.0;
    FLOAT sum_max_cnt = 0;
    FLOAT sum_min = 0.0;
    FLOAT sum_min_cnt = 0;
    FLOAT sum_avg = 0.0;
    FLOAT sum_avg_cnt = 0;

    for (UINT32 i = 0; i < TMC11_HIST_BIN_NUM; i++)
    {
        sum_all += pTmcHist[i] * log_bin_tmc11[i];
        sum_all_cnt += pTmcHist[i];

        if (sum_all_cnt >= n_hist * pTmcData->hist_supr_range_start && sum_all_cnt <= n_hist * pTmcData->hist_supr_range_end)
        {
            sum_max += pTmcHist[i] * log_bin_tmc11[i];
            sum_max_cnt += pTmcHist[i];
        }

        if (sum_all_cnt >= n_hist * pTmcData->hist_boost_range_start && sum_all_cnt <= n_hist * pTmcData->hist_boost_range_end)
        {
            sum_min += pTmcHist[i] * log_bin_tmc11[i];
            sum_min_cnt += pTmcHist[i];
        }

        if (sum_all_cnt >= n_hist * pTmcData->hist_avg_range_start && sum_all_cnt <= n_hist * pTmcData->hist_avg_range_end)
        {
            sum_avg += pTmcHist[i] * log_bin_tmc11[i];
            sum_avg_cnt += pTmcHist[i];
        }
    }

    FLOAT lw_avg = static_cast<FLOAT>((sum_avg_cnt) ? exp(sum_avg / sum_avg_cnt) : static_cast<FLOAT>(0.0f)); // 14b
    FLOAT lw_max = static_cast<FLOAT>((sum_max_cnt) ? exp(sum_max / sum_max_cnt) : static_cast<FLOAT>(0.0f)); // 14b
    FLOAT lw_min = static_cast<FLOAT>((sum_min_cnt) ? exp(sum_min / sum_min_cnt) : static_cast<FLOAT>(0.0f)); // 14b
    FLOAT tar_x = static_cast<FLOAT>(0.0f);
    FLOAT tar_x_igamma[5] = { static_cast<FLOAT>(256.0f) };

    for (UINT32 ti = 0; ti < 5; ti++)
    {
        tar_x = IQSettingUtils::ClampFLOAT(pTmcData->tone_target_tab.tone_target[ti] * 1023, 0.0f, 1023.0f);
        for (UINT32 gi = 1; gi < 257; gi++)
        {
            if (tar_x <= pGamma15[gi])
            {
                tar_x_igamma[ti] = ((pGamma15[gi] - tar_x) * (gi - 1) + (tar_x - pGamma15[gi - 1]) * gi) /
                                    (pGamma15[gi] - pGamma15[gi - 1]);
                break;
            }
        }
    }

    FLOAT a = tar_x_igamma[2] / 256;
    FLOAT adaptive_a_Lw_max = pTmcData->stretch_bright_str * lw_max + (1.0f - pTmcData->stretch_bright_str) * 16383; // 14b
    FLOAT adaptive_a_Lw_min = pTmcData->stretch_dark_str * lw_min + (1.0f - pTmcData->stretch_dark_str) * 0; // 14b

    // used to avoid the singularity that occurs with 0 when taking logarithm operation.
    const FLOAT epsilon = 1.0;
    FLOAT f = static_cast<FLOAT>((log(lw_avg + epsilon) * 2 - log(adaptive_a_Lw_min + epsilon) -
                                  log(adaptive_a_Lw_max + epsilon)) /
              (log(adaptive_a_Lw_max + epsilon) - log(adaptive_a_Lw_min + epsilon)));

    // adaptive key value for high key and low key scenes
    FLOAT adaptive_a = a * powf((f > 0) ? pTmcData->tone_bright_adj : pTmcData->tone_dark_adj, f);
    FLOAT global_gain = adaptive_a / (lw_avg / 16383);

    global_gain = IQSettingUtils::ClampFLOAT(global_gain, pInput->DRCGain / pTmcData->hist_enhance_clamp,
                                             pInput->DRCGain * pTmcData->hist_enhance_clamp);

    // find bezier curve
    FLOAT gain_min_adj = global_gain * pTmcData->contrast_dark_adj;
    FLOAT gain_avg_adj = global_gain;

    // if global gain is higher, max knee should be suppressed more
    FLOAT gain_max_adj =  IQSettingUtils::ClampFLOAT(pTmcData->contrast_bright_clip * 2 - global_gain, 1.0f, global_gain);

    kneeX[0] = 0;
    kneeY[0] = 0;
    kneeX[1] = lw_min / 16383;
    kneeY[1] = kneeX[1] * gain_min_adj;
    kneeX[2] = lw_avg / 16383;
    kneeY[2] = kneeX[2] * gain_avg_adj;
    kneeX[3] = lw_max / 16383;
    kneeY[3] = kneeX[3] * gain_max_adj;
    kneeX[4] = 1;
    kneeY[4] = 1;

    for (UINT32 i = 1; i < TMC11_KNEE_POINTS_NUM; i++)
    {
        if ((kneeX[i] - kneeX[i - 1]) <= 0.0f)
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
        }
    }

    for (UINT32 i = 1; i < TMC11_KNEE_POINTS_NUM; i++)
    {
        if ((kneeY[i] - kneeY[i - 1]) <= 0.0f)
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
        }
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC11Interpolation::CalculateContrastEnhanceCurve
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC11Interpolation::CalculateContrastEnhanceCurve(
    const TMC11InputData*   pInput,
    FLOAT*                  pTmcHist,
    FLOAT*                  pContrastEnhanceCurve)
{
    const FLOAT* pGamma15 = pInput->IPEGammaOutput;

    // histogram equalization
    FLOAT cdf[TMC11_HIST_BIN_NUM];

    // prepare CDF
    cdf[0] = pTmcHist[0];
    for (UINT32 i = 1; i < TMC11_HIST_BIN_NUM; i++)
    {
        cdf[i] = cdf[i - 1] + pTmcHist[i];
    }

    // map output for each bin
    UINT32 gi = 1;
    for (UINT32 i = 0; i < TMC11_HIST_BIN_NUM; i++)
    {
        FLOAT tar_x = IQSettingUtils::ClampFLOAT(cdf[i] / cdf[TMC11_HIST_BIN_NUM - 1] * 1023, 0.0f, 1023.0f);
        FLOAT tar_x_igamma = 256.0f;
        for ( ; gi < 257; gi++)
        {
            if (tar_x <= pGamma15[gi])
            {
                tar_x_igamma = ((pGamma15[gi] - tar_x) * static_cast<FLOAT>(gi - 1) +
                                (tar_x - pGamma15[gi - 1]) * static_cast<FLOAT>(gi)) / (pGamma15[gi] - pGamma15[gi - 1]);
                break;
            }
        }

        pContrastEnhanceCurve[i] = tar_x_igamma / 256;
    }

    return TRUE;
}
