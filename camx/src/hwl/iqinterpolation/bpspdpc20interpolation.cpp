// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  bpspdpc20interpolation.cpp
/// @brief BPS pdpc20 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bpspdpc20interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation PDPC20OperationTable[] =
{
    {BPSPDPC20Interpolation::DRCGainSearchNode, 2},
    {BPSPDPC20Interpolation::HDRAECSearchNode,  2},
    {BPSPDPC20Interpolation::AECSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSPDPC20Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSPDPC20Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    PDPC20IQInput*    pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))                        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftGGainWB, pInput->AWBleftGGainWB))             ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftBGainWB, pInput->AWBleftBGainWB))             ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftRGainWB, pInput->AWBleftRGainWB))             ||
        (pTriggerData->zzHDRModeEnable != pInput->zzHDRModeEnable))
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
        pTriggerData->zzHDRModeEnable   = pInput->zzHDRModeEnable;

        isChanged = TRUE;
    }

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSPDPC20Interpolation::RunIntepolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSPDPC20Interpolation::RunInterpolation(
    const PDPC20IQInput*             pInput,
    pdpc_2_0_0::pdpc20_rgn_dataType* pData )
{
    BOOL                            result = TRUE;
    UINT32                          count  = 0;
    // The interpolation tree total Node
    TuningNode                      nodeSet[PDPC20MaxNode];
    // BPCBCC Trigger List
    PDPC20TriggerList               pdpcTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (count = 0; count < PDPC20MaxNode; count++)
        {
            if (count < PDPC20MaxNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_pdpc20_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&pdpcTrigger.controlType,
            &(pInput->pChromatix->control_method),
            sizeof(pdpc_2_0_0::chromatix_pdpc20Type::control_methodStruct));

        pdpcTrigger.triggerDRCgain  = pInput->DRCGain;

        pdpcTrigger.triggerHDRAEC   = IQSettingUtils::GettriggerHDRAEC(
                                        pInput->pChromatix->control_method.aec_hdr_control,
                                        pInput->exposureTime,
                                        pInput->AECSensitivity,
                                        pInput->exposureGainRatio);

        pdpcTrigger.triggerAEC      = IQSettingUtils::GettriggerAEC(
                                        pInput->pChromatix->control_method.aec_exp_control,
                                        pInput->luxIndex,
                                        pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(
                                    &nodeSet[0],
                                    PDPC20InterpolationLevel,
                                    &PDPC20OperationTable[0],
                                    static_cast<VOID*>(&pdpcTrigger));
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
                                    PDPC20MaxNonLeafNode,
                                    PDPC20InterpolationLevel,
                                    BPSPDPC20Interpolation::DoInterpolation);
        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(pdpc_2_0_0::pdpc20_rgn_dataType));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSPDPC20Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSPDPC20Interpolation::DoInterpolation(
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
        pdpc_2_0_0::pdpc20_rgn_dataType* pInput1 =
            static_cast<pdpc_2_0_0::pdpc20_rgn_dataType*>(pData1);
        pdpc_2_0_0::pdpc20_rgn_dataType* pInput2 =
            static_cast<pdpc_2_0_0::pdpc20_rgn_dataType*>(pData2);
        pdpc_2_0_0::pdpc20_rgn_dataType* pOutput =
            static_cast<pdpc_2_0_0::pdpc20_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(pdpc_2_0_0::pdpc20_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = BPSPDPC20Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(pdpc_2_0_0::pdpc20_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(pdpc_2_0_0::pdpc20_rgn_dataType));
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
/// BPSPDPC20Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSPDPC20Interpolation::InterpolationData(
    pdpc_2_0_0::pdpc20_rgn_dataType* pInput1,
    pdpc_2_0_0::pdpc20_rgn_dataType* pInput2,
    FLOAT                            ratio,
    pdpc_2_0_0::pdpc20_rgn_dataType* pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput1) &&
        (NULL != pInput2) &&
        (NULL != pOutput))
    {
        pOutput->bcc_offset    = IQSettingUtils::InterpolationFloatBilinear(pInput1->bcc_offset, pInput2->bcc_offset, ratio);
        pOutput->bcc_offset_t2 = IQSettingUtils::InterpolationFloatBilinear(pInput1->bcc_offset_t2,
                                                                            pInput2->bcc_offset_t2,
                                                                            ratio);
        pOutput->bpc_offset    = IQSettingUtils::InterpolationFloatBilinear(pInput1->bpc_offset, pInput2->bpc_offset, ratio);
        pOutput->bpc_offset_t2 = IQSettingUtils::InterpolationFloatBilinear(pInput1->bpc_offset_t2,
                                                                            pInput2->bpc_offset_t2,
                                                                            ratio);

        pOutput->cold_pixel_correction_disable = pInput1->cold_pixel_correction_disable;
        pOutput->correction_threshold          = IQSettingUtils::InterpolationFloatBilinear(pInput1->correction_threshold,
                                                                                            pInput2->correction_threshold,
                                                                                            ratio);
        pOutput->fmax_pixel_q6                 = IQSettingUtils::InterpolationFloatBilinear(pInput1->fmax_pixel_q6,
                                                                                            pInput2->fmax_pixel_q6,
                                                                                            ratio);
        pOutput->fmin_pixel_q6                 = IQSettingUtils::InterpolationFloatBilinear(pInput1->fmin_pixel_q6,
                                                                                            pInput2->fmin_pixel_q6,
                                                                                            ratio);
        pOutput->hot_pixel_correction_disable  = pInput1->hot_pixel_correction_disable;
        pOutput->remove_along_edge             = pInput1->remove_along_edge;
        pOutput->saturation_threshold          = IQSettingUtils::InterpolationFloatBilinear(pInput1->saturation_threshold,
                                                                                            pInput2->saturation_threshold,
                                                                                            ratio);
        pOutput->using_cross_channel           = pInput1->using_cross_channel;
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPDPC20Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT BPSPDPC20Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    pdpc_2_0_0::chromatix_pdpc20_coreType*   pParentDataType = NULL;
    PDPC20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pdpc_2_0_0::chromatix_pdpc20_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_pdpc20_drc_gain_dataCount;
        pTriggerList    = static_cast<PDPC20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_pdpc20_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_pdpc20_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_pdpc20_drc_gain_data[regionOutput.endIndex]),
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
// BPSPDPC20Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT BPSPDPC20Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    pdpc_2_0_0::mod_pdpc20_drc_gain_dataType*   pParentDataType = NULL;
    PDPC20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pdpc_2_0_0::mod_pdpc20_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_pdpc20_hdr_aec_dataCount;
        pTriggerList    = static_cast<PDPC20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_pdpc20_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_pdpc20_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_pdpc20_hdr_aec_data[regionOutput.endIndex]),
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
// BPSPDPC20Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT BPSPDPC20Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    pdpc_2_0_0::mod_pdpc20_hdr_aec_dataType*   pParentDataType = NULL;
    PDPC20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pdpc_2_0_0::mod_pdpc20_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_pdpc20_aec_dataCount;
        pTriggerList    = static_cast<PDPC20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_pdpc20_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pdpc20_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pdpc20_aec_data[regionOutput.startIndex].pdpc20_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pdpc20_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pdpc20_aec_data[regionOutput.endIndex].pdpc20_rgn_data));
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
