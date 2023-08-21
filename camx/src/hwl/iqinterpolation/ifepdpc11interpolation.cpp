// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifepdpc11interpolation.cpp
/// @brief IFE PDPC11 tuning interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ifepdpc11interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation PDPC11OperationTable[] =
{
    {IFEPDPC11Interpolation::DRCGainSearchNode, 2},
    {IFEPDPC11Interpolation::HDRAECSearchNode,  2},
    {IFEPDPC11Interpolation::AECSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEPDPC11Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEPDPC11Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    PDPC11InputData*  pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain,  pInput->AECGain))                      ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))                        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (pTriggerData->blackLevelOffset != pInput->blackLevelOffset))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->realGain          = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->DRCGain           = pInput->DRCGain;
        pTriggerData->blackLevelOffset  = static_cast<UINT16>(pInput->blackLevelOffset);
        pTriggerData->leftGGainWB       = pInput->AWBleftGGainWB;
        pTriggerData->leftBGainWB       = pInput->AWBleftBGainWB;
        pTriggerData->leftRGainWB       = pInput->AWBleftRGainWB;

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEPDPC11Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEPDPC11Interpolation::RunInterpolation(
    const PDPC11InputData*            pInput,
    pdpc_1_1_0::pdpc11_rgn_dataType*  pData)
{
    BOOL                            result = TRUE;
    // The interpolation tree total Node
    TuningNode                      nodeSet[PDPC11MaxmiumNode];

    // PDPC Trigger List
    PDPC11TriggerList               PDPCTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < PDPC11MaxmiumNode; count++)
        {
            if (count < PDPC11MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_pdpc11_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&PDPCTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(pdpc_1_1_0::chromatix_pdpc11Type::control_methodStruct));

        PDPCTrigger.triggerDRCgain = pInput->DRCGain;

        PDPCTrigger.triggerHDRAEC =
            IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                             pInput->exposureTime,
                                             pInput->AECSensitivity,
                                             pInput->exposureGainRatio);

        PDPCTrigger.triggerAEC =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        PDPC11InterpolationLevel,
                                                        &PDPC11OperationTable[0],
                                                        static_cast<VOID*>(&PDPCTrigger));
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
                                                       PDPC11MaxmiumNonLeafNode,
                                                       PDPC11InterpolationLevel,
                                                       IFEPDPC11Interpolation::DoInterpolation);
        if (FALSE != result)
        {
            IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(pdpc_1_1_0::pdpc11_rgn_dataType));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEPDPC11Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEPDPC11Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    pdpc_1_1_0::pdpc11_rgn_dataType* pInput1 = NULL;
    pdpc_1_1_0::pdpc11_rgn_dataType* pInput2 = NULL;
    pdpc_1_1_0::pdpc11_rgn_dataType* pOutput = NULL;
    BOOL                             result  = TRUE;
    if ((NULL != pData1) &&
        (NULL != pData2) &&
        (NULL != pResult))
    {
        /// pInput1 and pInput2 are the Data regions and Data region 2
        ///    Region1           Interpolation     Region2
        ///    ---------- | ------------------- | ----------
        ///    ratio= 0.0......ratio (>0 && <1).....ratio = 1.0
        pInput1 = static_cast<pdpc_1_1_0::pdpc11_rgn_dataType*>(pData1);
        pInput2 = static_cast<pdpc_1_1_0::pdpc11_rgn_dataType*>(pData2);
        pOutput = static_cast<pdpc_1_1_0::pdpc11_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(pdpc_1_1_0::pdpc11_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = IFEPDPC11Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(pdpc_1_1_0::pdpc11_rgn_dataType));
            }
            else if (IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(pdpc_1_1_0::pdpc11_rgn_dataType));
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
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEPDPC11Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEPDPC11Interpolation::InterpolationData(
    pdpc_1_1_0::pdpc11_rgn_dataType* pInput1,
    pdpc_1_1_0::pdpc11_rgn_dataType* pInput2,
    FLOAT                            ratio,
    pdpc_1_1_0::pdpc11_rgn_dataType* pOutput)
{
    FLOAT result    = 0.0f;
    BOOL  resultAPI = TRUE;

    if ((NULL != pInput1) &&
        (NULL != pInput2) &&
        (NULL != pOutput))
    {
        result = IQSettingUtils::InterpolationFloatBilinear(pInput1->bp_offset_g_pixel,
                                                            pInput2->bp_offset_g_pixel,
                                                            ratio);

        pOutput->bp_offset_g_pixel = IQSettingUtils::AbsoluteFLOAT(result);

        result = IQSettingUtils::InterpolationFloatBilinear(pInput1->bp_offset_rb_pixel,
                                                            pInput2->bp_offset_rb_pixel,
                                                            ratio);

        pOutput->bp_offset_rb_pixel = IQSettingUtils::AbsoluteFLOAT(result);

        pOutput->fmax_pixel_q6 = IQSettingUtils::InterpolationFloatBilinear(pInput1->fmax_pixel_q6,
                                                                            pInput2->fmax_pixel_q6,
                                                                            ratio);

        pOutput->fmin_pixel_q6 = IQSettingUtils::InterpolationFloatBilinear(pInput1->fmin_pixel_q6,
                                                                            pInput2->fmin_pixel_q6,
                                                                            ratio);

        result = IQSettingUtils::InterpolationFloatBilinear(pInput1->t2_bp_offset_g_pixel,
                                                            pInput2->t2_bp_offset_g_pixel,
                                                            ratio);

        pOutput->t2_bp_offset_g_pixel = IQSettingUtils::AbsoluteFLOAT(result);

        result = IQSettingUtils::InterpolationFloatBilinear(pInput1->t2_bp_offset_rb_pixel,
                                                            pInput2->t2_bp_offset_rb_pixel,
                                                            ratio);

        pOutput->t2_bp_offset_rb_pixel = IQSettingUtils::AbsoluteFLOAT(result);
    }
    else
    {
        resultAPI = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return resultAPI;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC11Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFEPDPC11Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    pdpc_1_1_0::chromatix_pdpc11_coreType*   pParentDataType = NULL;
    PDPC11TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pdpc_1_1_0::chromatix_pdpc11_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_pdpc11_drc_gain_dataCount;
        pTriggerList    = static_cast<PDPC11TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_pdpc11_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_pdpc11_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_pdpc11_drc_gain_data[regionOutput.endIndex]),
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
// IFEPDPC11Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFEPDPC11Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    pdpc_1_1_0::mod_pdpc11_drc_gain_dataType*   pParentDataType = NULL;
    PDPC11TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pdpc_1_1_0::mod_pdpc11_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_pdpc11_hdr_aec_dataCount;
        pTriggerList    = static_cast<PDPC11TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_pdpc11_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_pdpc11_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_pdpc11_hdr_aec_data[regionOutput.endIndex]),
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
// IFEPDPC11Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFEPDPC11Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    pdpc_1_1_0::mod_pdpc11_hdr_aec_dataType*   pParentDataType = NULL;
    PDPC11TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<pdpc_1_1_0::mod_pdpc11_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_pdpc11_aec_dataCount;
        pTriggerList    = static_cast<PDPC11TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_pdpc11_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pdpc11_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pdpc11_aec_data[regionOutput.startIndex].pdpc11_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pdpc11_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_pdpc11_aec_data[regionOutput.endIndex].pdpc11_rgn_data));
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
