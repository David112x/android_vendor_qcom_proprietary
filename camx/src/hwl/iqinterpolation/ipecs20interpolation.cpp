// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ipecs20interpolation.cpp
/// @brief IPE CS20 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ipecs20interpolation.h"
#include "ipecs20setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation CS20OperationTable[] =
{
    {IPECS20Interpolation::DRCGainSearchNode, 2},
    {IPECS20Interpolation::HDRAECSearchNode,  2},
    {IPECS20Interpolation::AECSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECS20Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPECS20Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    CS20InputData*    pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->linearGain, pInput->AECGain))                  ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain, pInput->DRCGain))                     ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->linearGain        = pInput->AECGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->DRCGain           = pInput->DRCGain;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;

        isChanged =  TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECS20Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPECS20Interpolation::RunInterpolation(
    const CS20InputData*         pInput,
    cs_2_0_0::cs20_rgn_dataType* pData)
{
    BOOL                        result = TRUE;
    // The intepolation tree total Node
    TuningNode                  nodeSet[CS20MaxmiumNode];
    // CS Trigger List
    CS20TriggerList             CSTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (UINT count = 0; count < CS20MaxmiumNode; count++)
        {
            if (count < CS20MaxmiumNonLeafNode)
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], &pData[count + 1]);
            }
            else
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], NULL);
            }
        }

        // Setup the Top Node
        nodeSet[0].pNodeData = static_cast<VOID*>(&pInput->pChromatix->chromatix_cs20_core);
        IQSettingUtils::AddNodeToInterpolationTree(NULL, &nodeSet[0],
            static_cast<VOID*>(&pInput->pChromatix->chromatix_cs20_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&CSTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(cs_2_0_0::chromatix_cs20Type::control_methodStruct));

        CSTrigger.triggerDRCgain = pInput->DRCGain;

        CSTrigger.triggerHDRAEC =
            IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                             pInput->exposureTime,
                                             pInput->AECSensitivity,
                                             pInput->exposureGainRatio);
        CSTrigger.triggerAEC =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->linearGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        CS20InterpolationLevel,
                                                        &CS20OperationTable[0],
                                                        static_cast<VOID*>(&CSTrigger));
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
                                                       CS20MaxmiumNonLeafNode,
                                                       CS20InterpolationLevel,
                                                       IPECS20Interpolation::DoInterpolation);
        if (FALSE != result)
        {
            IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(cs_2_0_0::cs20_rgn_dataType));
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECS20Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPECS20Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    BOOL result = TRUE;

    if ((NULL != pData1) && (NULL != pData2) && (NULL != pResult))
    {
        cs_2_0_0::cs20_rgn_dataType* pInput1 = static_cast<cs_2_0_0::cs20_rgn_dataType*>(pData1);
        cs_2_0_0::cs20_rgn_dataType* pInput2 = static_cast<cs_2_0_0::cs20_rgn_dataType*>(pData2);
        cs_2_0_0::cs20_rgn_dataType* pOutput = static_cast<cs_2_0_0::cs20_rgn_dataType*>(pResult);


        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(cs_2_0_0::cs20_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = IPECS20Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(cs_2_0_0::cs20_rgn_dataType));
            }
            else if (IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(cs_2_0_0::cs20_rgn_dataType));
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
/// IPECS20Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPECS20Interpolation::InterpolationData(
    cs_2_0_0::cs20_rgn_dataType* pInput1,
    cs_2_0_0::cs20_rgn_dataType* pInput2,
    FLOAT                        ratio,
    cs_2_0_0::cs20_rgn_dataType* pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput1) && (NULL != pInput2) && (NULL != pOutput))
    {
        UINT i;
        for (i = 0; i < CHROMA_SUPP_LUT_SIZE; i++)
        {
            pOutput->c_thr1_lut_tab.c_thr1_lut[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->c_thr1_lut_tab.c_thr1_lut[i],
                                                           pInput2->c_thr1_lut_tab.c_thr1_lut[i],
                                                           ratio);
        }
        for (i = 0; i < CHROMA_SUPP_LUT_SIZE; i++)
        {
            pOutput->c_thr2_lut_tab.c_thr2_lut[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->c_thr2_lut_tab.c_thr2_lut[i],
                                                           pInput2->c_thr2_lut_tab.c_thr2_lut[i],
                                                           ratio);
        }
        for (i = 0; i < CHROMA_SUPP_LUT_SIZE; i++)
        {
            pOutput->knee_point_lut_tab.knee_point_lut[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->knee_point_lut_tab.knee_point_lut[i],
                                                           pInput2->knee_point_lut_tab.knee_point_lut[i],
                                                           ratio);
        }
        for (i = 0; i < CHROMA_SUPP_LUT_SIZE; i++)
        {
            pOutput->y_weight_lut_tab.y_weight_lut[i] =
                IQSettingUtils::InterpolationFloatBilinear(pInput1->y_weight_lut_tab.y_weight_lut[i],
                                                           pInput2->y_weight_lut_tab.y_weight_lut[i],
                                                           ratio);
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPECS20Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPECS20Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cs_2_0_0::chromatix_cs20_coreType*   pParentDataType = NULL;
    CS20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cs_2_0_0::chromatix_cs20_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_cs20_drc_gain_dataCount;
        pTriggerList    = static_cast<CS20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_cs20_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_cs20_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_cs20_drc_gain_data[regionOutput.endIndex]),
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
// IPECS20Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPECS20Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cs_2_0_0::mod_cs20_drc_gain_dataType*   pParentDataType = NULL;
    CS20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cs_2_0_0::mod_cs20_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_cs20_hdr_aec_dataCount;
        pTriggerList    = static_cast<CS20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_cs20_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_cs20_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_cs20_hdr_aec_data[regionOutput.endIndex]),
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
// IPECS20Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPECS20Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    cs_2_0_0::mod_cs20_hdr_aec_dataType*   pParentDataType = NULL;
    CS20TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<cs_2_0_0::mod_cs20_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_cs20_aec_dataCount;
        pTriggerList    = static_cast<CS20TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->hdr_aec_data.mod_cs20_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cs20_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cs20_aec_data[regionOutput.startIndex].cs20_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cs20_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_cs20_aec_data[regionOutput.endIndex].cs20_rgn_data));
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
