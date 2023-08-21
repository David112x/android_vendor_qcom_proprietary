// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  pdpc30interpolation.cpp
/// @brief pdpc30 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pdpc30interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation PDPC30OperationTable[] =
{
    {PDPC30Interpolation::DRCGainSearchNode, 2},
    {PDPC30Interpolation::HDRAECSearchNode,  2},
    {PDPC30Interpolation::AECSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PDPC30Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PDPC30Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    PDPC30IQInput*    pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))              ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))               ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))  ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftGGainWB, pInput->AWBleftGGainWB))    ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftBGainWB, pInput->AWBleftBGainWB))    ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->leftRGainWB, pInput->AWBleftRGainWB))    ||
        (pTriggerData->zzHDRModeEnable != pInput->zzHDRModeEnable))
    {
        pTriggerData->luxIndex        = pInput->AECLuxIndex;
        pTriggerData->realGain        = pInput->AECGain;
        pTriggerData->AECSensitivity  = pInput->AECSensitivity;
        pTriggerData->exposureTime    = pInput->AECexposureTime;
        pTriggerData->DRCGain         = pInput->DRCGain;
        pTriggerData->leftGGainWB     = pInput->AWBleftGGainWB;
        pTriggerData->leftBGainWB     = pInput->AWBleftBGainWB;
        pTriggerData->leftRGainWB     = pInput->AWBleftRGainWB;
        pTriggerData->zzHDRModeEnable = pInput->zzHDRModeEnable;

        isChanged = TRUE;
    }

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PDPC30Interpolation::RunIntepolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PDPC30Interpolation::RunInterpolation(
    const PDPC30IQInput*             pInput,
    pdpc_3_0_0::pdpc30_rgn_dataType* pData )
{
    BOOL                result = TRUE;
    // The interpolation tree total Node
    TuningNode          nodeSet[PDPC30MaxNode];
    // BPCBCC Trigger List
    PDPC30TriggerList   pdpcTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < PDPC30MaxNode; count++)
        {
            if (count < PDPC30MaxNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_pdpc30_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&pdpcTrigger.controlType,
            &(pInput->pChromatix->control_method),
            sizeof(pdpc_3_0_0::chromatix_pdpc30Type::control_methodStruct));

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
        result                      = IQSettingUtils::SetupInterpolationTree(
                                        &nodeSet[0],
                                        PDPC30InterpolationLevel,
                                        &PDPC30OperationTable[0],
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
        result                      = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                        PDPC30MaxNonLeafNode,
                                        PDPC30InterpolationLevel,
                                        PDPC30Interpolation::DoInterpolation);
        if (FALSE != result)
        {
            IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(pdpc_3_0_0::pdpc30_rgn_dataType));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PDPC30Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PDPC30Interpolation::DoInterpolation(
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
        pdpc_3_0_0::pdpc30_rgn_dataType* pInput1 =
            static_cast<pdpc_3_0_0::pdpc30_rgn_dataType*>(pData1);
        pdpc_3_0_0::pdpc30_rgn_dataType* pInput2 =
            static_cast<pdpc_3_0_0::pdpc30_rgn_dataType*>(pData2);
        pdpc_3_0_0::pdpc30_rgn_dataType* pOutput =
            static_cast<pdpc_3_0_0::pdpc30_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(pdpc_3_0_0::pdpc30_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = PDPC30Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(pdpc_3_0_0::pdpc30_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(pdpc_3_0_0::pdpc30_rgn_dataType));
            }
            else
            {
                result = FALSE;
            }
        }
    }
    else
    {
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PDPC30Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PDPC30Interpolation::InterpolationData(
    pdpc_3_0_0::pdpc30_rgn_dataType* pInput1,
    pdpc_3_0_0::pdpc30_rgn_dataType* pInput2,
    FLOAT                            ratio,
    pdpc_3_0_0::pdpc30_rgn_dataType* pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        pOutput->fmax_flat                  = IQSettingUtils::InterpolationFloatBilinear(pInput1->fmax_flat,
                                                                                         pInput2->fmax_flat,
                                                                                         ratio);
        pOutput->fmin_flat                  = IQSettingUtils::InterpolationFloatBilinear(pInput1->fmin_flat,
                                                                                         pInput2->fmin_flat,
                                                                                         ratio);
        pOutput->bpc_offset_flat            = IQSettingUtils::InterpolationFloatBilinear(pInput1->bpc_offset_flat,
                                                                                         pInput2->bpc_offset_flat,
                                                                                         ratio);
        pOutput->bcc_offset_flat            = IQSettingUtils::InterpolationFloatBilinear(pInput1->bcc_offset_flat,
                                                                                         pInput2->bcc_offset_flat,
                                                                                         ratio);
        pOutput->fmax                       = IQSettingUtils::InterpolationFloatBilinear(pInput1->fmax,
                                                                                         pInput2->fmax,
                                                                                         ratio);
        pOutput->fmin                       = IQSettingUtils::InterpolationFloatBilinear(pInput1->fmin,
                                                                                         pInput2->fmin,
                                                                                         ratio);
        pOutput->bpc_offset                 = IQSettingUtils::InterpolationFloatBilinear(pInput1->bpc_offset,
                                                                                         pInput2->bpc_offset,
                                                                                         ratio);
        pOutput->bcc_offset                 = IQSettingUtils::InterpolationFloatBilinear(pInput1->bcc_offset,
                                                                                         pInput2->bcc_offset,
                                                                                         ratio);
        pOutput->flat_detection_en          = pInput1->flat_detection_en;
        pOutput->flat_th_recip              = IQSettingUtils::InterpolationFloatBilinear(pInput1->flat_th_recip,
                                                                                         pInput2->flat_th_recip,
                                                                                         ratio);
        pOutput->saturation_threshold       = IQSettingUtils::InterpolationFloatBilinear(pInput1->saturation_threshold,
                                                                                         pInput2->saturation_threshold,
                                                                                         ratio);
        pOutput->use_same_channel_only      = pInput1->use_same_channel_only;
        pOutput->single_bpc_only            = pInput1->single_bpc_only;
        pOutput->directional_bpc_en         = pInput1->directional_bpc_en;
        pOutput->dir_tk                     = IQSettingUtils::InterpolationFloatBilinear(pInput1->dir_tk,
                                                                                         pInput2->dir_tk,
                                                                                         ratio);
        pOutput->dir_offset                 = IQSettingUtils::InterpolationFloatBilinear(pInput1->dir_offset,
                                                                                         pInput2->dir_offset,
                                                                                         ratio);
        pOutput->fmax_gic                   = IQSettingUtils::InterpolationFloatBilinear(pInput1->fmax_gic,
                                                                                         pInput2->fmax_gic,
                                                                                         ratio);
        pOutput->bpc_offset_gic             = IQSettingUtils::InterpolationFloatBilinear(pInput1->bpc_offset_gic,
                                                                                         pInput2->bpc_offset_gic,
                                                                                         ratio);
        pOutput->gic_thin_line_noise_offset = IQSettingUtils::InterpolationFloatBilinear(pInput1->gic_thin_line_noise_offset,
                                                                                         pInput2->gic_thin_line_noise_offset,
                                                                                         ratio);
        pOutput->gic_filter_strength        = IQSettingUtils::InterpolationFloatBilinear(pInput1->gic_filter_strength,
                                                                                         pInput2->gic_filter_strength,
                                                                                         ratio);

        for (UINT32 i = 0; i < PDPC30_NOISESTD_LENGTH + 1; i++)
        {
            pOutput->noise_std_lut_tab.noise_std_lut[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->noise_std_lut_tab.noise_std_lut[i],
                pInput2->noise_std_lut_tab.noise_std_lut[i],
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
// PDPC30Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT PDPC30Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion                            triggerRegion[MaxNumRegion];
    InterpolationOutput                      regionOutput;
    pdpc_3_0_0::chromatix_pdpc30_coreType*   pParentDataType = NULL;
    PDPC30TriggerList*                       pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pdpc_3_0_0::chromatix_pdpc30_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_pdpc30_drc_gain_dataCount;
        pTriggerList    = static_cast<PDPC30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_pdpc30_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(
            pParentNode,
            &pChildNode[childCount],
            static_cast<VOID*>(&pParentDataType->mod_pdpc30_drc_gain_data[regionOutput.startIndex]),
            NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(
                pParentNode,
                &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_pdpc30_drc_gain_data[regionOutput.endIndex]),
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
// PDPC30Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT PDPC30Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    pdpc_3_0_0::mod_pdpc30_drc_gain_dataType*   pParentDataType = NULL;
    PDPC30TriggerList*                          pTriggerList    = NULL;

    if ((NULL != pParentNode) && (NULL != pTriggerData) && (NULL != pChildNode))
    {
        pParentDataType = static_cast<pdpc_3_0_0::mod_pdpc30_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_pdpc30_hdr_aec_dataCount;
        pTriggerList    = static_cast<PDPC30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_pdpc30_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_pdpc30_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_pdpc30_hdr_aec_data[regionOutput.endIndex]),
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
// PDPC30Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT PDPC30Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    pdpc_3_0_0::mod_pdpc30_hdr_aec_dataType*   pParentDataType = NULL;
    PDPC30TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pdpc_3_0_0::mod_pdpc30_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_pdpc30_aec_dataCount;
        pTriggerList    = static_cast<PDPC30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_pdpc30_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(
            pParentNode,
            &pChildNode[childCount],
            static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pdpc30_aec_data[regionOutput.startIndex]),
            static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pdpc30_aec_data[regionOutput.startIndex].pdpc30_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(
                pParentNode,
                &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pdpc30_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pdpc30_aec_data[regionOutput.endIndex].pdpc30_rgn_data));
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
