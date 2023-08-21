// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ltm14interpolation.cpp
/// @brief IPE LTM14 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ltm14interpolation.h"
#include "ltm14setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation LTM14OperationTable[] =
{
    {LTM14Interpolation::DRCGainSearchNode, 2},
    {LTM14Interpolation::HDRAECSearchNode,  2},
    {LTM14Interpolation::AECSearchNode,     2}
};

// static const UINT32 LTM14MaxNode            = 15; ///< (1 + 1 * 2 + 2 * 2 + 4 * 2)
// static const UINT32 LTM14MaxNoLeafNode      = 7;  ///< (1 + 1 * 2 + 2 * 2)
// static const UINT32 LTM14InterpolationLevel = 4;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LTM14Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LTM14Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    LTM14InputData*   pTriggerData)
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
        pTriggerData->DRCGain           = pInput->DRCGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;

        isChanged = TRUE;
    }

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LTM14Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LTM14Interpolation::RunInterpolation(
    const LTM14InputData*          pInput,
    ltm_1_4_0::ltm14_rgn_dataType* pData)
{
    BOOL             result = TRUE;
    // The interpoloaion tree total Node
    TuningNode       nodeSet[LTM14MaxNode];
    // LTM Trigger List
    LTM14TriggerList ltmTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < LTM14MaxNode; count++)
        {
            // Initialize all the nodes
            for (UINT node = 0; node < LTM14MaxNoLeafNode; node++)
            {
                if (node < LTM14MaxNoLeafNode)
                {
                    IQSettingUtils::InitializeNode(&nodeSet[node], &pData[node + 1]);
                }
                else
                {
                    IQSettingUtils::InitializeNode(&nodeSet[node], NULL);
                }
            }
        }

        // Setup the Top Node
        nodeSet[0].pNodeData = static_cast<VOID*>(&pInput->pChromatix->chromatix_ltm14_core);
        IQSettingUtils::AddNodeToInterpolationTree(NULL, &nodeSet[0],
            static_cast<VOID*>(&pInput->pChromatix->chromatix_ltm14_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&ltmTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(ltm_1_4_0::chromatix_ltm14Type::control_methodStruct));

        ltmTrigger.triggerDRCgain = pInput->DRCGain;

        ltmTrigger.triggerHDRAEC  = IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                                                     pInput->exposureTime,
                                                                     pInput->AECSensitivity,
                                                                     pInput->exposureGainRatio);

        ltmTrigger.triggerAEC     = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                                  pInput->luxIndex,
                                                                  pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(
            &nodeSet[0], LTM14InterpolationLevel, &LTM14OperationTable[0], static_cast<VOID*>(&ltmTrigger));
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
                                                       LTM14MaxNoLeafNode,
                                                       LTM14InterpolationLevel,
                                                       LTM14Interpolation::DoInterpolation);
        if (FALSE != result)
        {
            IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(ltm_1_4_0::ltm14_rgn_dataType));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LTM14Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LTM14Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) &&
        (NULL != pData2) &&
        (NULL != pResult))
    {
        ltm_1_4_0::ltm14_rgn_dataType* pInput1 =
            static_cast<ltm_1_4_0::ltm14_rgn_dataType*>(pData1);
        ltm_1_4_0::ltm14_rgn_dataType* pInput2 =
            static_cast<ltm_1_4_0::ltm14_rgn_dataType*>(pData2);
        ltm_1_4_0::ltm14_rgn_dataType* pOutput =
            static_cast<ltm_1_4_0::ltm14_rgn_dataType*>(pResult);

        if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = LTM14Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(ltm_1_4_0::ltm14_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(ltm_1_4_0::ltm14_rgn_dataType));
            }
            else
            {
                result = FALSE;
                /// @todo (CAMX-1399) Need to add logging for Common library
            }
        }
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1399) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LTM14Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LTM14Interpolation::InterpolationData(
    ltm_1_4_0::ltm14_rgn_dataType* pInput1,
    ltm_1_4_0::ltm14_rgn_dataType* pInput2,
    FLOAT                          ratio,
    ltm_1_4_0::ltm14_rgn_dataType* pOutput)
{
    UINT32 i;
    BOOL   result = TRUE;

    if ((NULL != pInput1) &&
        (NULL != pInput2) &&
        (NULL != pOutput))
    {
        pOutput->y_ratio_max     = IQSettingUtils::InterpolationFloatBilinear(pInput1->y_ratio_max,
                                                                              pInput2->y_ratio_max,
                                                                              ratio);

        pOutput->ltm_strength    = IQSettingUtils::InterpolationFloatBilinear(pInput1->ltm_strength,
                                                                              pInput2->ltm_strength,
                                                                              ratio);

        pOutput->exp_atten_start = IQSettingUtils::InterpolationFloatBilinear(pInput1->exp_atten_start,
                                                                              pInput2->exp_atten_start,
                                                                              ratio);

        pOutput->exp_atten_end   = IQSettingUtils::InterpolationFloatBilinear(pInput1->exp_atten_end,
                                                                              pInput2->exp_atten_end,
                                                                              ratio);

        pOutput->dark_boost      = IQSettingUtils::InterpolationFloatBilinear(pInput1->dark_boost,
                                                                              pInput2->dark_boost,
                                                                              ratio);

        pOutput->bright_suppress = IQSettingUtils::InterpolationFloatBilinear(pInput1->bright_suppress,
                                                                              pInput2->bright_suppress,
                                                                              ratio);

        pOutput->lce_strength    = IQSettingUtils::InterpolationFloatBilinear(pInput1->lce_strength,
                                                                              pInput2->lce_strength,
                                                                              ratio);

        pOutput->p0              = IQSettingUtils::InterpolationFloatBilinear(pInput1->p0,
                                                                              pInput2->p0,
                                                                              ratio);

        pOutput->p1              = IQSettingUtils::InterpolationFloatBilinear(pInput1->p1,
                                                                              pInput2->p1,
                                                                              ratio);

        pOutput->dark_range      = IQSettingUtils::InterpolationFloatBilinear(pInput1->dark_range,
                                                                              pInput2->dark_range,
                                                                              ratio);

        pOutput->bright_range    = IQSettingUtils::InterpolationFloatBilinear(pInput1->bright_range,
                                                                              pInput2->bright_range,
                                                                              ratio);

        pOutput->dark_max        = IQSettingUtils::InterpolationFloatBilinear(pInput1->dark_max,
                                                                              pInput2->dark_max,
                                                                              ratio);

        pOutput->bright_max      = IQSettingUtils::InterpolationFloatBilinear(pInput1->bright_max,
                                                                              pInput2->bright_max,
                                                                              ratio);

        pOutput->dark_gamma      = IQSettingUtils::InterpolationFloatBilinear(pInput1->dark_gamma,
                                                                              pInput2->dark_gamma,
                                                                              ratio);

        pOutput->bright_gamma    = IQSettingUtils::InterpolationFloatBilinear(pInput1->bright_gamma,
                                                                              pInput2->bright_gamma,
                                                                              ratio);

        for (i = 0; i < LTM14_CURVE_LUT_SIZE; i++)
        {
            pOutput->la_curve_tab.la_curve[i] = IQSettingUtils::InterpolationFloatBilinear(
                                                    static_cast<FLOAT>(pInput1->la_curve_tab.la_curve[i]),
                                                    static_cast<FLOAT>(pInput2->la_curve_tab.la_curve[i]),
                                                    ratio);
        }

        for (i = 0; i < LTM14_SCALE_LUT_SIZE; i++)
        {
            pOutput->ltm_scale_tab.ltm_scale[i] = IQSettingUtils::InterpolationFloatBilinear(
                                                      static_cast<FLOAT>(pInput1->ltm_scale_tab.ltm_scale[i]),
                                                      static_cast<FLOAT>(pInput2->ltm_scale_tab.ltm_scale[i]),
                                                      ratio);
        }

        for (i = 0; i < LTM14_LCE_SCALE_LUT_SIZE; i++)
        {
            pOutput->lce_scale_pos_tab.lce_scale_pos[i] = IQSettingUtils::InterpolationFloatBilinear(
                                                              static_cast<FLOAT>(pInput1->lce_scale_pos_tab.lce_scale_pos[i]),
                                                              static_cast<FLOAT>(pInput2->lce_scale_pos_tab.lce_scale_pos[i]),
                                                              ratio);
        }

        for (i = 0; i < LTM14_LCE_SCALE_LUT_SIZE; i++)
        {
            pOutput->lce_scale_neg_tab.lce_scale_neg[i] = IQSettingUtils::InterpolationFloatBilinear(
                                                              static_cast<FLOAT>(pInput1->lce_scale_neg_tab.lce_scale_neg[i]),
                                                              static_cast<FLOAT>(pInput2->lce_scale_neg_tab.lce_scale_neg[i]),
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
// LTM14Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LTM14Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    ltm_1_4_0::chromatix_ltm14_coreType*   pParentDataType = NULL;
    LTM14TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<ltm_1_4_0::chromatix_ltm14_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_ltm14_drc_gain_dataCount;
        pTriggerList    = static_cast<LTM14TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_ltm14_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_ltm14_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_ltm14_drc_gain_data[regionOutput.endIndex]),
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
// LTM14Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LTM14Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    ltm_1_4_0::mod_ltm14_drc_gain_dataType*   pParentDataType = NULL;
    LTM14TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<ltm_1_4_0::mod_ltm14_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_ltm14_hdr_aec_dataCount;
        pTriggerList    = static_cast<LTM14TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_ltm14_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_ltm14_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_ltm14_hdr_aec_data[regionOutput.endIndex]),
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
// LTM14Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT LTM14Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    ltm_1_4_0::mod_ltm14_hdr_aec_dataType*   pParentDataType = NULL;
    LTM14TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<ltm_1_4_0::mod_ltm14_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_ltm14_aec_dataCount;
        pTriggerList    = static_cast<LTM14TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_ltm14_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_ltm14_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_ltm14_aec_data[regionOutput.startIndex].ltm14_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_ltm14_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_ltm14_aec_data[regionOutput.endIndex].ltm14_rgn_data));
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
