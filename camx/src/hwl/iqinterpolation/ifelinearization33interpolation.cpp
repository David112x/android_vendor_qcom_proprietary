// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifelinearization33interpolation.cpp
/// @brief IFE Linearization33 tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ifelinearization33interpolation.h"
#include "ifelinearization33setting.h"

// This table defines the function of each level of node to search for the child node
static NodeOperation Linearization33OperationTable[] =
{
    {IFELinearization33Interpolation::DRCGainSearchNode, 2},
    {IFELinearization33Interpolation::HDRAECSearchNode,  2},
    {IFELinearization33Interpolation::LEDSearchNode,     3},
    {IFELinearization33Interpolation::AECSearchNode,     2},
    {IFELinearization33Interpolation::CCTSearchNode,     2}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFELinearization33Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFELinearization33Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData*         pInput,
    Linearization33InputData* pTriggerData)
{
    BOOL isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))                   ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->DRCGain,  pInput->DRCGain))                       ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->AECSensitivity, pInput->AECSensitivity))          ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureTime, pInput->AECexposureTime))           ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->exposureGainRatio, pInput->AECexposureGainRatio)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->CCTTrigger, pInput->AWBColorTemperature))         ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->LEDFirstEntryRatio, pInput->LEDFirstEntryRatio))  ||
        (pTriggerData->LEDTrigger != pInput->LEDSensitivity))
    {
        pTriggerData->luxIndex          = pInput->AECLuxIndex;
        pTriggerData->realGain          = pInput->AECGain;
        pTriggerData->DRCGain           = pInput->DRCGain;
        pTriggerData->AECSensitivity    = pInput->AECSensitivity;
        pTriggerData->exposureTime      = pInput->AECexposureTime;
        pTriggerData->exposureGainRatio = pInput->AECexposureGainRatio;
        pTriggerData->CCTTrigger        = pInput->AWBColorTemperature;
        pTriggerData->LEDTrigger        = pInput->LEDSensitivity;
        pTriggerData->numberOfLED       = pInput->numberOfLED;

        pTriggerData->LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "luxIndex %f  realGain %f  DRCGain %f  AECSensitivity %f exposureTime %f"
            "CCTTrigger %f LEDTrigger %f numberOfLED %d",
            pTriggerData->luxIndex,
            pTriggerData->realGain,
            pTriggerData->DRCGain,
            pTriggerData->AECSensitivity,
            pTriggerData->exposureTime,
            pTriggerData->CCTTrigger,
            pTriggerData->LEDTrigger,
            pTriggerData->numberOfLED);

        isChanged = TRUE;
    }

    return isChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFELinearization33Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFELinearization33Interpolation::RunInterpolation(
    const Linearization33InputData*                    pInput,
    linearization_3_3_0::linearization33_rgn_dataType* pData)
{
    BOOL                                              result = TRUE;
    UINT                                              count  = 0;

    // The interpoloaion tree total Node
    TuningNode                                        nodeSet[Linearization33MaxmiumNode];

    // Linearization Trigger List
    Linearization33TriggerList                        linearizationTrigger;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (count = 0; count < Linearization33MaxmiumNode; count++)
        {
            if (count < Linearization33MaxmiumNonLeafNode)
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
            static_cast<VOID*>(&pInput->pChromatix->chromatix_linearization33_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&linearizationTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(linearization_3_3_0::chromatix_linearization33Type::control_methodStruct));

        linearizationTrigger.triggerDRCgain = pInput->DRCGain;

        linearizationTrigger.triggerLED     = pInput->LEDTrigger;
        linearizationTrigger.numberOfLED    = pInput->numberOfLED;
        linearizationTrigger.triggerCCT     = pInput->CCTTrigger;
        linearizationTrigger.privateInfo    = pInput->pChromatix->private_information;

        linearizationTrigger.LEDFirstEntryRatio = pInput->LEDFirstEntryRatio;

        linearizationTrigger.triggerHDRAEC  =
            IQSettingUtils::GettriggerHDRAEC(pInput->pChromatix->control_method.aec_hdr_control,
                                             pInput->exposureTime,
                                             pInput->AECSensitivity,
                                             pInput->exposureGainRatio);

        linearizationTrigger.triggerAEC     =
            IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                          pInput->luxIndex,
                                          pInput->realGain);

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "triggerHDRAEC %f triggerAEC %f", linearizationTrigger.triggerHDRAEC,
                                                                            linearizationTrigger.triggerAEC);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        Linearization33InterpolationLevel,
                                                        &Linearization33OperationTable[0],
                                                        static_cast<VOID*>(&linearizationTrigger));
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
                                                       Linearization33MaxmiumNonLeafNode,
                                                       Linearization33InterpolationLevel,
                                                       IFELinearization33Interpolation::DoInterpolation);
    }
    if (FALSE != result)
    {
        IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(linearization_3_3_0::linearization33_rgn_dataType));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFELinearization33Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFELinearization33Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    linearization_3_3_0::linearization33_rgn_dataType* pInput1 = NULL;
    linearization_3_3_0::linearization33_rgn_dataType* pInput2 = NULL;
    linearization_3_3_0::linearization33_rgn_dataType* pOutput = NULL;
    BOOL                                               result  = TRUE;

    if (NULL != pData1 && NULL != pData2 && NULL != pResult)
    {
        // pInput1 and pInput2 are the Data regions and Data region 2
        //    Region1           Interpolation     Region2
        //    ---------- | ------------------- | ----------
        //    ratio= 0.0......ratio (>0 && <1).....ratio = 1.0
        pInput1 =
            static_cast<linearization_3_3_0::linearization33_rgn_dataType*>(pData1);
        pInput2 =
            static_cast<linearization_3_3_0::linearization33_rgn_dataType*>(pData2);
        pOutput =
            static_cast<linearization_3_3_0::linearization33_rgn_dataType*>(pResult);

        if (pInput1 == pInput2)
        {
            IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(linearization_3_3_0::linearization33_rgn_dataType));
        }
        else if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = IFELinearization33Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(linearization_3_3_0::linearization33_rgn_dataType));
            }
            else if (IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(linearization_3_3_0::linearization33_rgn_dataType));
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
/// IFELinearization33Interpolation::InterpSeg
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT IFELinearization33Interpolation::InterpSeg(
    FLOAT* pX,
    FLOAT* pY,
    FLOAT x0,
    FLOAT maxValue)
{
    FLOAT xi;
    FLOAT xnext;
    FLOAT yi;
    FLOAT ynext = 0.0f;
    FLOAT y0;
    INT i;

    // Search x0 for the corresponding segment #0 ~ #8
    if (x0 < *(pX))
    {
        xi = 0;
        xnext = *(pX);
        i = 0;
        yi = *(pY);
        ynext = *(pY + 1);
    }
    else if (x0 >= *(pX + 7))
    {
        xi = *(pX + 7);
        xnext = maxValue;
        i = 8;
        yi = *(pY + 8);
        ynext = maxValue;
    }
    else
    {
        for (i = 0; i<7; i++)
        {
            xi = *(pX + i);
            xnext = *(pX + i + 1);
            if ((x0 >= xi) && (x0 < xnext))
            {
                i++;
                break;
            }
        }
        yi    = *(pY + i);
        ynext = *(pY + i + 1);
    }
    // Compute y0 of x0 on the slope of segment #i = 0 ~ 8
    if (xnext != xi)
    {
        y0 = yi + (x0 - xi) * (ynext - yi) / (xnext - xi);
    }
    else
    {
        y0 = maxValue;
    }

    return y0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFELinearization33Interpolation::SegmentInterpolate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IFELinearization33Interpolation::SegmentInterpolate(
    FLOAT* pRLutP,
    FLOAT* pRLutBase,
    FLOAT* pInput2RLutP,
    FLOAT* pInput2RLutBase,
    FLOAT* pModOutputRLutP,
    FLOAT* pModOutputRLutBase,
    FLOAT  ratio,
    FLOAT  maxValue)
{
    FLOAT x0;
    FLOAT pRLutBasenewVal;
    FLOAT pInput2RLutBasenewVal;
    FLOAT pRLutPnew[8];
    FLOAT pRLutBasenew[9];
    FLOAT pInput2RLutPnew[8];
    FLOAT pInput2RLutBasenew[9];
    INT   i;

    *pModOutputRLutBase = 0;
    for (i = 0; i < 8; i++)
    {
        *(pRLutPnew + i) = *(pRLutP + i);
        *(pInput2RLutPnew + i) = *(pInput2RLutP + i);
        *(pRLutBasenew + i) = *(pRLutBase + i);
        *(pInput2RLutBasenew + i) = *(pInput2RLutBase + i);
    }
    *(pRLutBasenew + 8) = *(pRLutBase + 8);
    *(pInput2RLutBasenew + 8) = *(pInput2RLutBase + 8);

    for (i = 0; i<8; i++)
    {
        x0 = IQSettingUtils::InterpolationFloatBilinear(*(pRLutPnew + i), *(pInput2RLutPnew + i), ratio);
        pRLutBasenewVal = InterpSeg(pRLutPnew, pRLutBasenew, x0, maxValue);
        pInput2RLutBasenewVal = InterpSeg(pInput2RLutPnew, pInput2RLutBasenew, x0, maxValue);
        *(pModOutputRLutP + i) = x0;
        *(pModOutputRLutBase + i + 1) = IQSettingUtils::ClampFLOAT(
            IQSettingUtils::InterpolationFloatBilinear(pRLutBasenewVal, pInput2RLutBasenewVal, ratio), 0.0f, maxValue);
    }
    if (pModOutputRLutP[0] != pModOutputRLutBase[1])
    {
        *(pModOutputRLutBase + 1) = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFELinearization33Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFELinearization33Interpolation::InterpolationData(
    linearization_3_3_0::linearization33_rgn_dataType* pInput1,
    linearization_3_3_0::linearization33_rgn_dataType* pInput2,
    FLOAT                                              ratio,
    linearization_3_3_0::linearization33_rgn_dataType* pOutput)
{
    BOOL  resultAPI = TRUE;

    if (NULL != pInput1 && NULL != pInput2 && NULL != pOutput)
    {
        SegmentInterpolate(pInput1->r_lut_p_tab.r_lut_p, pInput1->r_lut_base_tab.r_lut_base,
            pInput2->r_lut_p_tab.r_lut_p, pInput2->r_lut_base_tab.r_lut_base,
            pOutput->r_lut_p_tab.r_lut_p, pOutput->r_lut_base_tab.r_lut_base, ratio, Linearization33MaximumLUTVal);

        SegmentInterpolate(pInput1->gr_lut_p_tab.gr_lut_p, pInput1->gr_lut_base_tab.gr_lut_base,
            pInput2->gr_lut_p_tab.gr_lut_p, pInput2->gr_lut_base_tab.gr_lut_base,
                pOutput->gr_lut_p_tab.gr_lut_p, pOutput->gr_lut_base_tab.gr_lut_base, ratio, Linearization33MaximumLUTVal);

        SegmentInterpolate(pInput1->gb_lut_p_tab.gb_lut_p, pInput1->gb_lut_base_tab.gb_lut_base,
            pInput2->gb_lut_p_tab.gb_lut_p, pInput2->gb_lut_base_tab.gb_lut_base,
            pOutput->gb_lut_p_tab.gb_lut_p, pOutput->gb_lut_base_tab.gb_lut_base, ratio, Linearization33MaximumLUTVal);

        SegmentInterpolate(pInput1->b_lut_p_tab.b_lut_p, pInput1->b_lut_base_tab.b_lut_base,
            pInput2->b_lut_p_tab.b_lut_p, pInput2->b_lut_base_tab.b_lut_base,
            pOutput->b_lut_p_tab.b_lut_p, pOutput->b_lut_base_tab.b_lut_base, ratio, Linearization33MaximumLUTVal);
    }
    else
    {
        resultAPI = FALSE;
    }

    return resultAPI;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33Interpolation::DRCGainSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFELinearization33Interpolation::DRCGainSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    linearization_3_3_0::chromatix_linearization33_coreType*   pParentDataType = NULL;
    Linearization33TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<linearization_3_3_0::chromatix_linearization33_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_linearization33_drc_gain_dataCount;
        pTriggerList    = static_cast<Linearization33TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->mod_linearization33_drc_gain_data[count].drc_gain_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerDRCgain;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_linearization33_drc_gain_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_linearization33_drc_gain_data[regionOutput.endIndex]),
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
// IFELinearization33Interpolation::HDRAECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFELinearization33Interpolation::HDRAECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    linearization_3_3_0::mod_linearization33_drc_gain_dataType*   pParentDataType = NULL;
    Linearization33TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<linearization_3_3_0::mod_linearization33_drc_gain_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->drc_gain_data.mod_linearization33_hdr_aec_dataCount;
        pTriggerList    = static_cast<Linearization33TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionHDRAEC(
                pTriggerList->controlType.aec_hdr_control,
                &(pParentDataType->drc_gain_data.mod_linearization33_hdr_aec_data[count].hdr_aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerHDRAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_linearization33_hdr_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->drc_gain_data.mod_linearization33_hdr_aec_data[regionOutput.endIndex]),
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
// IFELinearization33Interpolation::LEDSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFELinearization33Interpolation::LEDSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    UINT32 regionNumber = 0;
    FLOAT  rationLED2   = 0.0f;
    FLOAT  triggerValue = 0.0f;

    InterpolationOutput regionOutput;

    linearization_3_3_0::mod_linearization33_hdr_aec_dataType*   pParentDataType = NULL;
    Linearization33TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<linearization_3_3_0::mod_linearization33_hdr_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->hdr_aec_data.mod_linearization33_led_idx_dataCount;
        pTriggerList    = static_cast<Linearization33TriggerList*>(pTriggerData);
        triggerValue    = pTriggerList->triggerLED;

        if ((0 ==  pTriggerList->numberOfLED) || (1 == regionNumber))
        {
            regionOutput.startIndex         = 0;
            regionOutput.endIndex           = 0;
            regionOutput.interpolationRatio = 0;
        }
        else if ((1 == pTriggerList->numberOfLED) || (2 == pTriggerList->numberOfLED))
        {
            triggerValue = pTriggerList->triggerLED;

            if (triggerValue >= pTriggerList->privateInfo.led_sensitivity_trigger.end)
            {
                regionOutput.startIndex         = 1;
                regionOutput.endIndex           = 1;
                regionOutput.interpolationRatio = 0;
            }
            else if (triggerValue <= pTriggerList->privateInfo.led_sensitivity_trigger.start)
            {
                regionOutput.startIndex         = 0;
                regionOutput.endIndex           = 0;
                regionOutput.interpolationRatio = 0;
            }
            else
            {
                regionOutput.startIndex         = 0;
                regionOutput.endIndex           = 1;

                regionOutput.interpolationRatio = IQSettingUtils::CalculateInterpolationRatio(
                    static_cast<DOUBLE>(triggerValue),
                    static_cast<DOUBLE>(pTriggerList->privateInfo.led_sensitivity_trigger.start),
                    static_cast<DOUBLE>(pTriggerList->privateInfo.led_sensitivity_trigger.end));
            }

            if (2 == pTriggerList->numberOfLED)
            {
                rationLED2 = pTriggerList->LEDFirstEntryRatio;
            }
        }
        else
        {
            // Error case, treat as LED off -- Adding log here

            regionOutput.startIndex         = 0;
            regionOutput.endIndex           = 0;
            regionOutput.interpolationRatio = 0;
        }

        regionOutput.startIndex = IQSettingUtils::MinUINT32(regionOutput.startIndex, (regionNumber - 1));
        regionOutput.endIndex   = IQSettingUtils::MinUINT32(regionOutput.endIndex, (regionNumber - 1));

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_linearization33_led_idx_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_linearization33_led_idx_data[regionOutput.endIndex]),
                NULL);
            childCount++;
        }

        if ((rationLED2 != 0.0f) && ( 3 <= regionNumber))
        {
            // Adding Intepolation Value to ParentNode
            // If already has 2 child node, put to interpolationValue[1], otherwise put to interpolationValue[0]
            pParentNode->interpolationValue[childCount - 1] = (1 - rationLED2);

            // Set up one more child
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->hdr_aec_data.mod_linearization33_led_idx_data[2]),
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
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization33Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFELinearization33Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    linearization_3_3_0::mod_linearization33_led_idx_dataType*   pParentDataType = NULL;
    Linearization33TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<linearization_3_3_0::mod_linearization33_led_idx_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->led_idx_data.mod_linearization33_aec_dataCount;
        pTriggerList    = static_cast<Linearization33TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->led_idx_data.mod_linearization33_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->led_idx_data.mod_linearization33_aec_data[regionOutput.startIndex]),
           NULL);
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->led_idx_data.mod_linearization33_aec_data[regionOutput.endIndex]),
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
// IFELinearization33Interpolation::CCTSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IFELinearization33Interpolation::CCTSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    linearization_3_3_0::mod_linearization33_aec_dataType*   pParentDataType = NULL;
    Linearization33TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<linearization_3_3_0::mod_linearization33_aec_dataType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->aec_data.mod_linearization33_cct_dataCount;
        pTriggerList    = static_cast<Linearization33TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegion(
                &(pParentDataType->aec_data.mod_linearization33_cct_data[count].cct_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerCCT;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->aec_data.mod_linearization33_cct_data[regionOutput.startIndex]),
           static_cast<VOID*>
               (&pParentDataType->aec_data.mod_linearization33_cct_data[regionOutput.startIndex].linearization33_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->aec_data.mod_linearization33_cct_data[regionOutput.endIndex]),
                static_cast<VOID*>
                    (&pParentDataType->aec_data.mod_linearization33_cct_data[regionOutput.endIndex].linearization33_rgn_data));
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
