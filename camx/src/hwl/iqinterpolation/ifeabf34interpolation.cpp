// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifeabf34interpolation.cpp
/// @brief IFE ABF34 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ifeabf34interpolation.h"
#include "ifeabf34setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation ABF34OperationTable[] =
{
    {IFEABF34Interpolation::DRCGainSearchNode, 2},
    {IFEABF34Interpolation::HDRAECSearchNode,  2},
    {IFEABF34Interpolation::AECSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEABF34Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEABF34Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    ABF34InputData*   pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain,  pInput->AECGain))                      ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))                        ||
        (pTriggerData->blackResidueOffset != pInput->blackLevelOffset)                                   ||
        (pTriggerData->CAMIFWidth         != pInput->CAMIFWidth)                                         ||
        (pTriggerData->CAMIFHeight        != pInput->CAMIFHeight)                                        ||
        (pTriggerData->sensorOffsetX      != pInput->sensorOffsetX)                                      ||
        (pTriggerData->sensorOffsetY      != pInput->sensorOffsetY))
    {
        pTriggerData->luxIndex           = pInput->AECLuxIndex;
        pTriggerData->realGain           = pInput->AECGain;
        pTriggerData->AECSensitivity     = pInput->AECSensitivity;
        pTriggerData->exposureTime       = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio  = pInput->AECexposureGainRatio;
        pTriggerData->CAMIFWidth         = pInput->sensorImageWidth;
        pTriggerData->CAMIFHeight        = pInput->sensorImageHeight;
        pTriggerData->DRCGain            = pInput->DRCGain;
        pTriggerData->blackResidueOffset = pInput->blackLevelOffset;
        pTriggerData->sensorOffsetX      = pInput->sensorOffsetX;
        pTriggerData->sensorOffsetY      = pInput->sensorOffsetY;

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEABF34Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEABF34Interpolation::RunInterpolation(
    const ABF34InputData*          pInput,
    abf_3_4_0::abf34_rgn_dataType* pData)
{
    BOOL                          result = TRUE;
    // The interpoloaion tree total Node
    TuningNode                    nodeSet[ABF34MaxmiumNode];

    // BPCBCC Trigger List
    ABF34TriggerList              ABFTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < ABF34MaxmiumNode; count++)
        {
            if (count < ABF34MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_abf34_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&ABFTrigger.controlType,
            &(pInput->pChromatix->control_method),
            sizeof(abf_3_4_0::chromatix_abf34Type::control_methodStruct));

        ABFTrigger.triggerDRCgain = pInput->DRCGain;
        ABFTrigger.triggerHDRAEC  =
            IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                             pInput->exposureTime,
                                             pInput->AECSensitivity,
                                             pInput->exposureGainRatio);
        ABFTrigger.triggerAEC     =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        ABF34InterpolationLevel,
                                                        &ABF34OperationTable[0],
                                                        static_cast<VOID*>(&ABFTrigger));
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
                                                       ABF34MaxmiumNonLeafNode,
                                                       ABF34InterpolationLevel,
                                                       IFEABF34Interpolation::DoInterpolation);

        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(abf_3_4_0::abf34_rgn_dataType));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEABF34Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEABF34Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    abf_3_4_0::abf34_rgn_dataType* pInput1 = NULL;
    abf_3_4_0::abf34_rgn_dataType* pInput2 = NULL;
    abf_3_4_0::abf34_rgn_dataType* pOutput = NULL;
    BOOL                           result  = TRUE;

    if ((NULL != pData1) &&
        (NULL != pData2) &&
        (NULL != pResult))
    {
        // pInput1 and pInput2 are the Data regions and Data region 2
        //    Region1           Interpolation     Region2
        //    ---------- | ------------------- | ----------
        //    ratio= 0.0......ratio (>0 && <1).....ratio = 1.0

        pInput1 = static_cast<abf_3_4_0::abf34_rgn_dataType*>(pData1);
        pInput2 = static_cast<abf_3_4_0::abf34_rgn_dataType*>(pData2);
        pOutput = static_cast<abf_3_4_0::abf34_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(abf_3_4_0::abf34_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = IFEABF34Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(abf_3_4_0::abf34_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(abf_3_4_0::abf34_rgn_dataType));
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
/// IFEABF34Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEABF34Interpolation::InterpolationData(
    abf_3_4_0::abf34_rgn_dataType* pInput1,
    abf_3_4_0::abf34_rgn_dataType* pInput2,
    FLOAT                          ratio,
    abf_3_4_0::abf34_rgn_dataType* pOutput)
{
    FLOAT  result = 0.0f;
    UINT32 count;
    BOOL   resultAPI = TRUE;

    if ((NULL != pInput1) &&
        (NULL != pInput2) &&
        (NULL != pOutput))
    {
        pOutput->bpc_fmax    = IQSettingUtils::InterpolationFloatBilinear(pInput1->bpc_fmax, pInput2->bpc_fmax, ratio);
        pOutput->bpc_fmin    = IQSettingUtils::InterpolationFloatBilinear(pInput1->bpc_fmin, pInput2->bpc_fmin, ratio);
        pOutput->bpc_maxshft = IQSettingUtils::InterpolationFloatBilinear(pInput1->bpc_maxshft, pInput2->bpc_maxshft, ratio);
        pOutput->bpc_minshft = IQSettingUtils::InterpolationFloatBilinear(pInput1->bpc_minshft, pInput2->bpc_minshft, ratio);
        pOutput->bpc_offset  = IQSettingUtils::InterpolationFloatBilinear(pInput1->bpc_offset, pInput2->bpc_offset, ratio);

        pOutput->blk_pix_matching_rb  =
            IQSettingUtils::InterpolationFloatBilinear(pInput1->blk_pix_matching_rb,
                                                       pInput2->blk_pix_matching_rb,
                                                       ratio);
        pOutput->blk_pix_matching_g   =
            IQSettingUtils::InterpolationFloatBilinear(pInput1->blk_pix_matching_g,
                                                       pInput2->blk_pix_matching_g,
                                                       ratio);
        pOutput->noise_prsv_anchor_lo =
            IQSettingUtils::InterpolationFloatBilinear(pInput1->noise_prsv_anchor_lo,
                                                       pInput2->noise_prsv_anchor_lo,
                                                       ratio);
        pOutput->noise_prsv_anchor_hi =
            IQSettingUtils::InterpolationFloatBilinear(pInput1->noise_prsv_anchor_hi,
                                                       pInput2->noise_prsv_anchor_hi,
                                                       ratio);

        pOutput->edge_softness  =
            IQSettingUtils::InterpolationFloatBilinear(pInput1->edge_softness,
                                                       pInput2->edge_softness,
                                                       ratio);

        for (count = 0; count < ABFV34_NOISE_STD_LENGTH; count++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(pInput1->noise_stdlut_level_tab.noise_stdlut_level[count],
                                                                pInput2->noise_stdlut_level_tab.noise_stdlut_level[count],
                                                                ratio);

            pOutput->noise_stdlut_level_tab.noise_stdlut_level[count] = result;
        }

        for (count = 0; count < ABF34_KERNEL_LENGTH; count++)
        {
            pOutput->distance_ker_tab.distance_ker[count] = pInput1->distance_ker_tab.distance_ker[count];
        }

        for (count = 0; count < ABF3_CHANNEL_MAX; count++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(pInput1->curve_offset_tab.curve_offset[count],
                                                                pInput2->curve_offset_tab.curve_offset[count],
                                                                ratio);

            pOutput->curve_offset_tab.curve_offset[count] = IQSettingUtils::AbsoluteFLOAT(result);
        }

        for (count = 0; count < ABF3_LEVEL_MAX; count++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(pInput1->noise_prsv_lo_tab.noise_prsv_lo[count],
                                                                pInput2->noise_prsv_lo_tab.noise_prsv_lo[count],
                                                                ratio);

            pOutput->noise_prsv_lo_tab.noise_prsv_lo[count] = result;
        }

        for (count = 0; count < ABF3_LEVEL_MAX; count++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(pInput1->noise_prsv_hi_tab.noise_prsv_hi[count],
                                                                pInput2->noise_prsv_hi_tab.noise_prsv_hi[count],
                                                                ratio);

            pOutput->noise_prsv_hi_tab.noise_prsv_hi[count] = result;
        }

        for (count = 0; count < 10; count++)
        {
            result = IQSettingUtils::InterpolationFloatBilinear(pInput1->radial_gain_tab.radial_gain[count],
                                                                pInput2->radial_gain_tab.radial_gain[count],
                                                                ratio);

            pOutput->radial_gain_tab.radial_gain[count] = result;
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
// IFEABF34Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFEABF34Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    abf_3_4_0::chromatix_abf34_coreType*   pParentDataType = NULL;
    ABF34TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<abf_3_4_0::chromatix_abf34_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_abf34_drc_gain_dataCount;
        pTriggerList    = static_cast<ABF34TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_abf34_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_abf34_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_abf34_drc_gain_data[regionOutput.endIndex]),
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
// IFEABF34Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFEABF34Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    abf_3_4_0::mod_abf34_drc_gain_dataType*   pParentDataType = NULL;
    ABF34TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<abf_3_4_0::mod_abf34_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_abf34_hdr_aec_dataCount;
        pTriggerList    = static_cast<ABF34TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_abf34_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_abf34_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_abf34_hdr_aec_data[regionOutput.endIndex]),
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
// IFEABF34Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFEABF34Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    abf_3_4_0::mod_abf34_hdr_aec_dataType*   pParentDataType = NULL;
    ABF34TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<abf_3_4_0::mod_abf34_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_abf34_aec_dataCount;
        pTriggerList    = static_cast<ABF34TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_abf34_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_abf34_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_abf34_aec_data[regionOutput.startIndex].abf34_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_abf34_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_abf34_aec_data[regionOutput.endIndex].abf34_rgn_data));
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
