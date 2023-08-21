// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  bpsgic30interpolation.cpp
/// @brief BPS GIC30 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bpsgic30interpolation.h"
#include "bpsgic30setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation GIC30OperationTable[] =
{
    {BPSGIC30Interpolation::HDRAECSearchNode, 2},
    {BPSGIC30Interpolation::AECSearchNode,    2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSGIC30Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSGIC30Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    GIC30InputData*   pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                    ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))                        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTimeTrigger, pInput->AECexposureTime))     ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio))  ||
        (pTriggerData->sensorOffsetX     != pInput->sensorOffsetX)                                        ||
        (pTriggerData->sensorOffsetY     != pInput->sensorOffsetY)                                        ||
        (pTriggerData->imageWidth  != pInput->sensorImageWidth)                                           ||
        (pTriggerData->imageHeight != pInput->sensorImageHeight))
    {
        pTriggerData->luxIndex            = pInput->AECLuxIndex;
        pTriggerData->realGain            = pInput->AECGain;
        pTriggerData->AECSensitivity      = pInput->AECSensitivity;
        pTriggerData->exposureTimeTrigger = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio   = pInput->AECexposureGainRatio;
        pTriggerData->imageWidth          = pInput->sensorImageWidth;
        pTriggerData->imageHeight         = pInput->sensorImageHeight;
        pTriggerData->sensorOffsetX       = pInput->sensorOffsetX;
        pTriggerData->sensorOffsetY       = pInput->sensorOffsetY;

        isChanged = TRUE;
    }

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSGIC30Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSGIC30Interpolation::RunInterpolation(
    const GIC30InputData*          pInput,
    gic_3_0_0::gic30_rgn_dataType* pData)
{
    BOOL                          result = TRUE;
    // The interpolation tree total Node
    TuningNode                    nodeSet[GIC30MaxmiumNode];
    // GIC Trigger List
    GIC30TriggerList              GICTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < GIC30MaxmiumNode; count++)
        {
            if (count < GIC30MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_gic30_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&GICTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(gic_3_0_0::chromatix_gic30Type::control_methodStruct));

        GICTrigger.triggerHDRAEC  = IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                                                     pInput->exposureTimeTrigger,
                                                                     pInput->AECSensitivity,
                                                                     pInput->exposureGainRatio);
        GICTrigger.triggerAEC     = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                                  pInput->luxIndex,
                                                                  pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        GIC30InterpolationLevel,
                                                        &GIC30OperationTable[0],
                                                        static_cast<VOID*>(&GICTrigger));
        if (FALSE != result)
        {
            // Calculate the Interpolation Result
            result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                           GIC30MaxmiumNonLeafNode,
                                                           GIC30InterpolationLevel,
                                                           BPSGIC30Interpolation::DoInterpolation);
        }
        if (FALSE != result)
        {
            IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(gic_3_0_0::gic30_rgn_dataType));
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
/// BPSGIC30Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSGIC30Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        gic_3_0_0::gic30_rgn_dataType* pInput1 = static_cast<gic_3_0_0::gic30_rgn_dataType*>(pData1);
        gic_3_0_0::gic30_rgn_dataType* pInput2 = static_cast<gic_3_0_0::gic30_rgn_dataType*>(pData2);
        gic_3_0_0::gic30_rgn_dataType* pOutput = static_cast<gic_3_0_0::gic30_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(gic_3_0_0::gic30_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = BPSGIC30Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(gic_3_0_0::gic30_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(gic_3_0_0::gic30_rgn_dataType));
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
/// BPSGIC30Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSGIC30Interpolation::InterpolationData(
    gic_3_0_0::gic30_rgn_dataType* pInput1,
    gic_3_0_0::gic30_rgn_dataType* pInput2,
    FLOAT                          ratio,
    gic_3_0_0::gic30_rgn_dataType* pOutput)
{
    INT32 i;
    BOOL  result = TRUE;

    if ((NULL != pInput1) &&
        (NULL != pInput2) &&
        (NULL != pOutput))
    {
        pOutput->enable_gic              = pInput1->enable_gic;


        pOutput->enable_pnr              = pInput1->enable_pnr;


        pOutput->gic_noise_scale         = IQSettingUtils::InterpolationFloatBilinear(pInput1->gic_noise_scale,
                                                                                      pInput2->gic_noise_scale,
                                                                                      ratio);

        pOutput->gic_correction_strength = IQSettingUtils::InterpolationFloatBilinear(pInput1->gic_correction_strength,
                                                                                      pInput2->gic_correction_strength,
                                                                                      ratio);

        pOutput->thin_line_noise_offset  = IQSettingUtils::InterpolationFloatBilinear(pInput1->thin_line_noise_offset,
                                                                                      pInput2->thin_line_noise_offset,
                                                                                      ratio);

        pOutput->pnr_correction_strength = IQSettingUtils::InterpolationFloatBilinear(pInput1->pnr_correction_strength,
                                                                                      pInput2->pnr_correction_strength,
                                                                                      ratio);

        for (i = 0; i < (DMIRAM_GIC_NOISESTD_LENGTH_V30 + 1); i++)
        {
            pOutput->noise_std_lut_tab.noise_std_lut[i] = IQSettingUtils::InterpolationFloatBilinear(
                                                              pInput1->noise_std_lut_tab.noise_std_lut[i],
                                                              pInput2->noise_std_lut_tab.noise_std_lut[i],
                                                              ratio);
        }

        for (i = 0; i < NumNoiseScale; i++)
        {
            pOutput->pnr_noise_scale_tab.pnr_noise_scale[i] = IQSettingUtils::InterpolationFloatBilinear(
                                                                  pInput1->pnr_noise_scale_tab.pnr_noise_scale[i],
                                                                  pInput2->pnr_noise_scale_tab.pnr_noise_scale[i],
                                                                  ratio);
        }

        for (i = 0; i < (NumAnchorBase + 1); i++)
        {
            pOutput->radial_pnr_str_adj_tab.radial_pnr_str_adj[i] = IQSettingUtils::InterpolationFloatBilinear(
                                                                        pInput1->radial_pnr_str_adj_tab.radial_pnr_str_adj[i],
                                                                        pInput2->radial_pnr_str_adj_tab.radial_pnr_str_adj[i],
                                                                        ratio);
        }
    }
    else
    {
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSGIC30Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT BPSGIC30Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gic_3_0_0::chromatix_gic30_coreType*   pParentDataType = NULL;
    GIC30TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gic_3_0_0::chromatix_gic30_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_gic30_hdr_aec_dataCount;
        pTriggerList    = static_cast<GIC30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->mod_gic30_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_gic30_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_gic30_hdr_aec_data[regionOutput.endIndex]),
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
// BPSGIC30Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT BPSGIC30Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    gic_3_0_0::mod_gic30_hdr_aec_dataType*   pParentDataType = NULL;
    GIC30TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<gic_3_0_0::mod_gic30_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_gic30_aec_dataCount;
        pTriggerList    = static_cast<GIC30TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_gic30_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gic30_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gic30_aec_data[regionOutput.startIndex].gic30_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gic30_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_gic30_aec_data[regionOutput.endIndex].gic30_rgn_data));
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
