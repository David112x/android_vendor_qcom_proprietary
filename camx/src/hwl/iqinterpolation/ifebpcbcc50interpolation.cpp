// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifebpcbcc50interpolation.cpp
/// @brief IFE BPCBCC50 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ifebpcbcc50interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation BPCBCC50OperationTable[] =
{
    {IFEBPCBCC50Interpolation::DRCGainSearchNode, 2},
    {IFEBPCBCC50Interpolation::HDRAECSearchNode,  2},
    {IFEBPCBCC50Interpolation::AECSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEBPCBCC50Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEBPCBCC50Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData*  pInput,
    BPCBCC50InputData* pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))                        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftGGainWB, pInput->AWBleftGGainWB))             ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftBGainWB, pInput->AWBleftBGainWB))             ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftRGainWB, pInput->AWBleftRGainWB))             ||
        (pTriggerData->blackLevelOffset != pInput->blackLevelOffset))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->realGain          = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->DRCGain           = pInput->DRCGain;
        pTriggerData->leftGGainWB       = pInput->AWBleftGGainWB;
        pTriggerData->leftBGainWB       = pInput->AWBleftBGainWB;
        pTriggerData->leftRGainWB       = pInput->AWBleftRGainWB;
        pTriggerData->blackLevelOffset  = pInput->blackLevelOffset;

        isChanged = TRUE;
    }

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEBPCBCC50Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEBPCBCC50Interpolation::RunInterpolation(
    const BPCBCC50InputData*             pInput,
    bpcbcc_5_0_0::bpcbcc50_rgn_dataType* pData)
{
    BOOL result = TRUE;
    UINT count  = 0;

    ///< The interpolation tree total Node
    TuningNode                          nodeSet[BPCBCC50MaxmiumNode];

    BPCBCC50TriggerList                 bpcbccTrigger; ///< BPCBCC Trigger List

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (count = 0; count < BPCBCC50MaxmiumNode; count++)
        {
            if (count < BPCBCC50MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_bpcbcc50_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&bpcbccTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(bpcbcc_5_0_0::chromatix_bpcbcc50Type::control_methodStruct));

        bpcbccTrigger.triggerDRCgain = pInput->DRCGain;

        bpcbccTrigger.triggerHDRAEC  = IQSettingUtils::GettriggerHDRAEC(
                                          pInput->pChromatix->control_method.aec_hdr_control,
                                          pInput->exposureTime, pInput->AECSensitivity, pInput->exposureGainRatio);

        bpcbccTrigger.triggerAEC     = IQSettingUtils::GettriggerAEC(
                                         pInput->pChromatix->control_method.aec_exp_control,
                                         pInput->luxIndex, pInput->realGain);

        // Set up Interpolation Tree
        result                       = IQSettingUtils::SetupInterpolationTree(
                                          &nodeSet[0],
                                          BPCBCC50InterpolationLevel,
                                          &BPCBCC50OperationTable[0],
                                          static_cast<VOID*>(&bpcbccTrigger));
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    if (FALSE != result)
    {
        // Calculate the Interpolation Result
        result = IQSettingUtils::InterpolateTuningData(
                                    &nodeSet[0],
                                    BPCBCC50MaxmiumNonLeafNode,
                                    BPCBCC50InterpolationLevel,
                                    IFEBPCBCC50Interpolation::DoInterpolation);
        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(bpcbcc_5_0_0::bpcbcc50_rgn_dataType));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEBPCBCC50Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEBPCBCC50Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    bpcbcc_5_0_0::bpcbcc50_rgn_dataType* pInput1 = NULL;
    bpcbcc_5_0_0::bpcbcc50_rgn_dataType* pInput2 = NULL;
    bpcbcc_5_0_0::bpcbcc50_rgn_dataType* pOutput = NULL;
    BOOL                                 result  = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        /// pInput1 and pInput2 are the Data regions and Data region 2
        ///    Region1           Interpolation     Region2
        ///    ---------- | ------------------- | ----------
        ///    ratio= 0.0......ratio (>0 && <1).....ratio = 1.0
        pInput1 = static_cast<bpcbcc_5_0_0::bpcbcc50_rgn_dataType*>(pData1);
        pInput2 = static_cast<bpcbcc_5_0_0::bpcbcc50_rgn_dataType*>(pData2);
        pOutput = static_cast<bpcbcc_5_0_0::bpcbcc50_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(bpcbcc_5_0_0::bpcbcc50_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = IFEBPCBCC50Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(bpcbcc_5_0_0::bpcbcc50_rgn_dataType));
            }
            else if (IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(bpcbcc_5_0_0::bpcbcc50_rgn_dataType));
            }
            else
            {
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
/// IFEBPCBCC50Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEBPCBCC50Interpolation::InterpolationData(
    bpcbcc_5_0_0::bpcbcc50_rgn_dataType* pInput1,
    bpcbcc_5_0_0::bpcbcc50_rgn_dataType* pInput2,
    FLOAT                                ratio,
    bpcbcc_5_0_0::bpcbcc50_rgn_dataType* pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        pOutput->hot_pixel_correction_disable  = pInput1->hot_pixel_correction_disable;
        pOutput->cold_pixel_correction_disable = pInput1->cold_pixel_correction_disable;
        pOutput->same_channel_recovery         = pInput1->same_channel_recovery;
        pOutput->fmax                          = IQSettingUtils::InterpolationFloatBilinear(
                                                    pInput1->fmax,
                                                    pInput2->fmax,
                                                    ratio);
        pOutput->fmin                          = IQSettingUtils::InterpolationFloatBilinear(
                                                    pInput1->fmin,
                                                    pInput2->fmin,
                                                    ratio);
        pOutput->bpc_offset                    = IQSettingUtils::InterpolationFloatBilinear(
                                                    pInput1->bpc_offset,
                                                    pInput2->bpc_offset,
                                                    ratio);
        pOutput->bcc_offset                    = IQSettingUtils::InterpolationFloatBilinear(
                                                    pInput1->bcc_offset,
                                                    pInput2->bcc_offset,
                                                    ratio);
        pOutput->correction_threshold          = IQSettingUtils::InterpolationFloatBilinear(
                                                    pInput1->correction_threshold,
                                                    pInput2->correction_threshold,
                                                    ratio);
    }
    else
    {
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBPCBCC50Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFEBPCBCC50Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    bpcbcc_5_0_0::chromatix_bpcbcc50_coreType*   pParentDataType = NULL;
    BPCBCC50TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<bpcbcc_5_0_0::chromatix_bpcbcc50_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_bpcbcc50_drc_gain_dataCount;
        pTriggerList    = static_cast<BPCBCC50TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_bpcbcc50_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_bpcbcc50_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_bpcbcc50_drc_gain_data[regionOutput.endIndex]),
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
// IFEBPCBCC50Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFEBPCBCC50Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    bpcbcc_5_0_0::mod_bpcbcc50_drc_gain_dataType*   pParentDataType = NULL;
    BPCBCC50TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<bpcbcc_5_0_0::mod_bpcbcc50_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_bpcbcc50_hdr_aec_dataCount;
        pTriggerList    = static_cast<BPCBCC50TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_bpcbcc50_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_bpcbcc50_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_bpcbcc50_hdr_aec_data[regionOutput.endIndex]),
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
// IFEBPCBCC50Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFEBPCBCC50Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    bpcbcc_5_0_0::mod_bpcbcc50_hdr_aec_dataType*   pParentDataType = NULL;
    BPCBCC50TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<bpcbcc_5_0_0::mod_bpcbcc50_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_bpcbcc50_aec_dataCount;
        pTriggerList    = static_cast<BPCBCC50TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_bpcbcc50_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_bpcbcc50_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_bpcbcc50_aec_data[regionOutput.startIndex].bpcbcc50_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_bpcbcc50_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>
                    (&pParentDataType->hdr_aec_data.mod_bpcbcc50_aec_data[regionOutput.endIndex].bpcbcc50_rgn_data));
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
