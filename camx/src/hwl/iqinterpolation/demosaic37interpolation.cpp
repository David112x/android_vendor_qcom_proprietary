// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  demosaic37interpolation.cpp
/// @brief Demosaic37 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "demosaic37interpolation.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation Demosaic37OperationTable[] =
{
    {Demosaic37Interpolation::DRCGainSearchNode, 2},
    {Demosaic37Interpolation::HDRAECSearchNode,  2},
    {Demosaic37Interpolation::AECSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Demosaic37Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Demosaic37Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData*    pInput,
    Demosaic37InputData* pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECGain, pInput->AECGain))               ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))               ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))  ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->AECGain           = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->imageWidth        = pInput->sensorImageWidth;
        pTriggerData->imageHeight       = pInput->sensorImageHeight;
        pTriggerData->DRCGain           = pInput->DRCGain;

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Demosaic37Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Demosaic37Interpolation::RunInterpolation(
    const Demosaic37InputData*               pInput,
    demosaic_3_7_0::demosaic37_rgn_dataType* pData)
{
    BOOL                                    result = TRUE;
    // The interpolation tree total Node
    TuningNode                              nodeSet[Demosaic37MaximumNode];
    // Demosaic Trigger List
    Demosaic37TriggerList                   demosaicTrigger;

    if ((NULL != pInput) && (NULL != pData))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < Demosaic37MaximumNode; count++)
        {
            if (count < Demosaic37MaximumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_demosaic37_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&demosaicTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(demosaic_3_7_0::chromatix_demosaic37Type::control_methodStruct));

        demosaicTrigger.triggerDRCgain = pInput->DRCGain;

        demosaicTrigger.triggerHDRAEC  =
            IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                             pInput->exposureTime,
                                             pInput->AECSensitivity,
                                             pInput->exposureGainRatio);
        demosaicTrigger.triggerAEC     =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->AECGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        Demosaic37InterpolationLevel,
                                                        &Demosaic37OperationTable[0],
                                                        static_cast<VOID*>(&demosaicTrigger));
        if (FALSE != result)
        {
            // Calculate the Interpolation Result
            result = IQSettingUtils::InterpolateTuningData(&nodeSet[0],
                                                           Demosaic37MaximumNonLeafNode,
                                                           Demosaic37InterpolationLevel,
                                                           Demosaic37Interpolation::DoInterpolation);
            if (FALSE != result)
            {
                IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(demosaic_3_7_0::demosaic37_rgn_dataType));
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
/// Demosaic37Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Demosaic37Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    demosaic_3_7_0::demosaic37_rgn_dataType* pInput1 = NULL;
    demosaic_3_7_0::demosaic37_rgn_dataType* pInput2 = NULL;
    demosaic_3_7_0::demosaic37_rgn_dataType* pOutput = NULL;
    BOOL                                     result  = TRUE;

    if ((NULL != pData1) &&
        (NULL != pData2) &&
        (NULL != pResult))
    {
        pInput1 = static_cast<demosaic_3_7_0::demosaic37_rgn_dataType*>(pData1);
        pInput2 = static_cast<demosaic_3_7_0::demosaic37_rgn_dataType*>(pData2);
        pOutput = static_cast<demosaic_3_7_0::demosaic37_rgn_dataType*>(pResult);


        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(demosaic_3_7_0::demosaic37_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = Demosaic37Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(demosaic_3_7_0::demosaic37_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(demosaic_3_7_0::demosaic37_rgn_dataType));
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
/// Demosaic37Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Demosaic37Interpolation::InterpolationData(
    demosaic_3_7_0::demosaic37_rgn_dataType* pInput1,
    demosaic_3_7_0::demosaic37_rgn_dataType* pInput2,
    FLOAT                                    ratio,
    demosaic_3_7_0::demosaic37_rgn_dataType* pOutput)
{
    FLOAT result    = 0.0f;
    BOOL  resultAPI = TRUE;

    if ((NULL != pInput1) &&
        (NULL != pInput2) &&
        (NULL != pOutput))
    {
        pOutput->edge_det_weight = IQSettingUtils::InterpolationFloatBilinear(pInput1->edge_det_weight,
                                                                              pInput2->edge_det_weight,
                                                                              ratio);

        result = IQSettingUtils::InterpolationFloatBilinear(pInput1->edge_det_noise_offset,
                                                            pInput2->edge_det_noise_offset,
                                                            ratio);

        pOutput->edge_det_noise_offset = IQSettingUtils::AbsoluteFLOAT(result);

        result = IQSettingUtils::InterpolationFloatBilinear(pInput1->dis_directional_g,
                                                            pInput2->dis_directional_g,
                                                            ratio);

        pOutput->dis_directional_g = IQSettingUtils::AbsoluteFLOAT(result);

        result = IQSettingUtils::InterpolationFloatBilinear(pInput1->dis_directional_rb,
                                                            pInput2->dis_directional_rb,
                                                            ratio);

        pOutput->dis_directional_rb = IQSettingUtils::AbsoluteFLOAT(result);

        pOutput->lambda_g = IQSettingUtils::InterpolationFloatBilinear(pInput1->lambda_g,
                                                                       pInput2->lambda_g,
                                                                       ratio);


        pOutput->lambda_rb = IQSettingUtils::InterpolationFloatBilinear(pInput1->lambda_rb,
                                                                        pInput2->lambda_rb,
                                                                        ratio);


        result = IQSettingUtils::InterpolationFloatBilinear(pInput1->en_dyna_clamp_g,
                                                            pInput2->en_dyna_clamp_g,
                                                            ratio);

        pOutput->en_dyna_clamp_g = IQSettingUtils::AbsoluteFLOAT(result);

        result = IQSettingUtils::InterpolationFloatBilinear(pInput1->en_dyna_clamp_rb,
                                                            pInput2->en_dyna_clamp_rb,
                                                            ratio);

        pOutput->en_dyna_clamp_rb = IQSettingUtils::AbsoluteFLOAT(result);
    }
    else
    {
        resultAPI = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return resultAPI;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Demosaic37Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Demosaic37Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    demosaic_3_7_0::chromatix_demosaic37_coreType*   pParentDataType = NULL;
    Demosaic37TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<demosaic_3_7_0::chromatix_demosaic37_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_demosaic37_drc_gain_dataCount;
        pTriggerList    = static_cast<Demosaic37TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_demosaic37_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_demosaic37_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_demosaic37_drc_gain_data[regionOutput.endIndex]),
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
// Demosaic37Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Demosaic37Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    demosaic_3_7_0::mod_demosaic37_drc_gain_dataType*   pParentDataType = NULL;
    Demosaic37TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<demosaic_3_7_0::mod_demosaic37_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_demosaic37_hdr_aec_dataCount;
        pTriggerList    = static_cast<Demosaic37TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_demosaic37_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_demosaic37_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_demosaic37_hdr_aec_data[regionOutput.endIndex]),
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
// Demosaic37Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Demosaic37Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    demosaic_3_7_0::mod_demosaic37_hdr_aec_dataType*   pParentDataType = NULL;
    Demosaic37TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<demosaic_3_7_0::mod_demosaic37_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_demosaic37_aec_dataCount;
        pTriggerList    = static_cast<Demosaic37TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_demosaic37_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_demosaic37_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>
              (&pParentDataType->hdr_aec_data.mod_demosaic37_aec_data[regionOutput.startIndex].demosaic37_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_demosaic37_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>
                    (&pParentDataType->hdr_aec_data.mod_demosaic37_aec_data[regionOutput.endIndex].demosaic37_rgn_data));
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
