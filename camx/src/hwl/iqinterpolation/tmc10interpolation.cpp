// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  tmc10interpolation.cpp
/// @brief tmc10 tunning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "tmc10interpolation.h"

static const FLOAT  TMC10_UNIT_GAIN_STEP   = 1.03f;
static const INT32  TMC10_DRC_INDEX_MIN    = 94;
static const FLOAT  TMC10_GAIN_RATIO_MIN   = 1.0f;
static const FLOAT  TMC10_GAIN_RATIO_MAX   = 16.0f;
static const FLOAT  TMC10_DRC_GAIN_POW_X   = 1.03f;
static const INT    TMC10_KNEE_POINTS_NUM  = 4;
static const UINT32 TMC10_COEF_NUM         = 6;
static const FLOAT  TMC10_MAX_LTM_GAIN     = 8.0f;

// This table defines the function of each level of node to search for the child node
static NodeOperation TMC10OperationTable[] =
{
    {TMC10Interpolation::AECSearchNode, 2},
};

static const UINT32 TMC10MaxmiumNode         = 3; ///< (1 + 1 * 2)
static const UINT32 TMC10MaxmiumNonLeafNode  = 1;
static const UINT32 TMC10InterpolationLevel  = 2;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC10Interpolation::CheckUpdateTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC10Interpolation::CheckUpdateTrigger(
    ISPIQTriggerData* pInput,
    TMC10InputData*   pTriggerData)
{
    BOOL  isChanged = FALSE;

    if ((FALSE == IQSettingUtils::FEqual(pTriggerData->drcGainDark, pInput->DRCGainDark)) ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->realGain, pInput->AECGain))        ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->luxIndex, pInput->AECLuxIndex))    ||
        (FALSE == IQSettingUtils::FEqual(pTriggerData->drcGain, pInput->DRCGain)))
    {
        pTriggerData->luxIndex    = pInput->AECLuxIndex;
        pTriggerData->realGain    = pInput->AECGain;
        pTriggerData->drcGain     = pInput->DRCGain;
        pTriggerData->drcGainDark = pInput->DRCGainDark;

        if (pTriggerData->drcGain < 1.0f)
        {
            pTriggerData->drcGain = 1.0f;
        }

        if (pTriggerData->drcGainDark < 1.0f)
        {
            pTriggerData->drcGainDark = 1.0f;
        }

        isChanged = TRUE;
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC10Interpolation::CalculateKneePoints
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC10Interpolation::CalculateKneePoints(
    FLOAT* pIn,
    FLOAT* pOut,
    FLOAT* pCoef)
{
    BOOL result  = TRUE;
    FLOAT delta1 = 0;
    FLOAT delta2 = 0;
    FLOAT delta3 = 0;
    FLOAT b2     = 0;
    FLOAT c2     = 0;
    FLOAT b3     = 0;
    FLOAT c3     = 0;
    FLOAT d4_tmp = 0;

    if ((NULL != pIn)  &&
        (NULL != pOut) &&
        (NULL != pCoef))
    {
        FLOAT h1 = pIn[1] - pIn[0];
        FLOAT h2 = pIn[2] - pIn[1];
        FLOAT h3 = pIn[3] - pIn[2];

        if (FALSE == IQSettingUtils::FEqual(h1, 0.0f))
        {
            delta1 = (pOut[1] - pOut[0]) / h1;
        }

        if (FALSE == IQSettingUtils::FEqual(h2, 0.0f))
        {
            delta2 = (pOut[2] - pOut[1]) / h2;
        }

        if (FALSE == IQSettingUtils::FEqual(h3, 0.0f))
        {
            delta3 = (pOut[3] - pOut[2]) / h3;
        }

        FLOAT d2_w1 = (2.0f * h2) + h1;
        FLOAT d2_w2 = h2 + (2.0f * h1);
        FLOAT d2 = (d2_w1 + d2_w2) / ((d2_w1 / delta1) + (d2_w2 / delta2));

        FLOAT d3_w1 = (2.0f * h3) + h2;
        FLOAT d3_w2 = h3 + (2.0f * h2);
        FLOAT d3 = (d3_w1 + d3_w2) / ((d3_w1 / delta2) + (d3_w2 / delta3));

        if (FALSE == IQSettingUtils::FEqual((h2 + h3), 0.0f))
        {
            d4_tmp = ((((2.0f * h3) + h2) * delta3) - (h3 * delta2)) / (h2 + h3);
        }
        FLOAT d4 = ((delta2 * delta3) < 0 && IQSettingUtils::AbsoluteFLOAT(d4_tmp) >
            IQSettingUtils::AbsoluteFLOAT(3 * delta3)) ? 3 * delta3 :
            (d4_tmp * delta3 < 0) ? 0 : d4_tmp;

        if (FALSE == IQSettingUtils::FEqual(h2, 0.0f))
        {
            b2 = (d2 - (2.0f * delta2) + d3) / (h2 * h2);
            c2 = ((3.0f * delta2) - (2.0f * d2) - d3) / h2;
        }

        if (FALSE == IQSettingUtils::FEqual(h3, 0.0f))
        {
            b3 = (d3 - (2.0f * delta3) + d4) / (h3 * h3);
            c3 = ((3.0f * delta3) - (2.0f * d3) - d4) / h3;
        }

        pCoef[0] = d2;    // use for knee_dark <= x <= knee_mid
        pCoef[1] = c2;    // use for knee_dark <= x <= knee_mid
        pCoef[2] = b2;    // use for knee_dark <= x <= knee_mid
        pCoef[3] = d3;    // use for knee_mid <= x <= 1.0f
        pCoef[4] = c3;    // use for knee_mid <= x <= 1.0f
        pCoef[5] = b3;    // use for knee_mid <= x <= 1.0f
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC10Interpolation::CalcGainCurve
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC10Interpolation::CalcGainCurve(
    const TMC10InputData*          pInput,
    tmc_1_0_0::tmc10_rgn_dataType* pTmcData)
{
    UINT32 drcIdx           = 0;
    FLOAT  drcGain          = 0.0f;
    FLOAT  gainRatio        = 0.0f;
    FLOAT  drcGainDark      = 0.0f;
    FLOAT  kneeMidX         = 0.0f;
    FLOAT  kneeDarkX        = 0.0f;
    FLOAT  kneeDarkY        = 0.0f;
    FLOAT  kneeX[TMC10_KNEE_POINTS_NUM];
    FLOAT  kneeY[TMC10_KNEE_POINTS_NUM];
    FLOAT  coef[TMC10_COEF_NUM];
    FLOAT  maxLTMPercengate = 0.0f;

    drcIdx      = IQSettingUtils::MinUINT32(static_cast<UINT32>(round(log(pInput->drcGain) / log(TMC10_UNIT_GAIN_STEP))),
                                            TMC10_DRC_INDEX_MIN);

    drcGain     = powf(TMC10_DRC_GAIN_POW_X, static_cast<FLOAT>(drcIdx));

    gainRatio   = IQSettingUtils::ClampFLOAT((pInput->drcGainDark - 1) * pTmcData->dark_boost_ratio + 1 +
        pTmcData->core_rsv_para1, TMC10_GAIN_RATIO_MIN, TMC10_GAIN_RATIO_MAX);

    drcGainDark = drcGain * gainRatio;
    if (TRUE == IQSettingUtils::FEqualCoarse(gainRatio, TMC10_GAIN_RATIO_MIN, 0.0001f))
    {
        gainRatio = 1.0001f;
    }

    drcIdx      = IQSettingUtils::MinUINT32(static_cast<UINT32>(round(log(drcGainDark) / log(TMC10_UNIT_GAIN_STEP))),
                                            TMC10_DRC_INDEX_MIN);

    maxLTMPercengate = logf(TMC10_MAX_LTM_GAIN) / logf(powf(TMC10_DRC_GAIN_POW_X, static_cast<FLOAT>(drcIdx)));
    if (maxLTMPercengate > 1)
    {
        maxLTMPercengate = 1;
    }

    if (pTmcData->ltm_percentage > maxLTMPercengate)
    {
        pTmcData->ltm_percentage = maxLTMPercengate;
        pTmcData->gtm_percentage = 1 - pTmcData->ltm_percentage;
    }

    if (TRUE == IQSettingUtils::FEqual(gainRatio, TMC10_GAIN_RATIO_MIN))
    {
        gainRatio = 1.0001f;
    }

    if (FALSE == IQSettingUtils::FEqual(drcGain, 0.0f))
    {
        kneeMidX = pTmcData->knee_out_mid / drcGain;
    }

    if (FALSE == IQSettingUtils::FEqual(gainRatio, 0.0f))
    {
        kneeDarkY = pTmcData->knee_out_mid / gainRatio;
    }

    if (FALSE == IQSettingUtils::FEqual(drcGainDark, 0.0f))
    {
        kneeDarkX = kneeDarkY / drcGainDark;
    }

    kneeX[0] = 0.0f;
    kneeX[1] = kneeDarkX;
    kneeX[2] = kneeMidX;
    kneeX[3] = 1.0f;

    kneeY[0] = 0.0f;
    kneeY[1] = kneeDarkY;
    kneeY[2] = pTmcData->knee_out_mid;
    kneeY[3] = 1.0f;

    for (INT i = 1; i < TMC10_KNEE_POINTS_NUM; i++)
    {
        if ((kneeX[i] - kneeX[i - 1]) <= 0.0f)
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
        }
    }

    for (INT i = 1; i < TMC10_KNEE_POINTS_NUM; i++)
    {
        if ((kneeY[i] - kneeY[i - 1]) <= 0.0f)
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
        }
    }

    CalculateKneePoints(kneeX, kneeY, coef);

    if ((NULL != pInput->pAdrcOutputData) && (NULL != pInput->pChromatix))
    {
        pInput->pAdrcOutputData->enable        = pInput->pChromatix->enable_section.adrc_isp_enable;
        pInput->pAdrcOutputData->version       = SWTMCVersion::TMC10;
        pInput->pAdrcOutputData->gtmEnable     = pInput->pChromatix->chromatix_tmc10_reserve.use_gtm;
        pInput->pAdrcOutputData->ltmEnable     = pInput->pChromatix->chromatix_tmc10_reserve.use_ltm;
        pInput->pAdrcOutputData->ltmPercentage = pTmcData->ltm_percentage;
        pInput->pAdrcOutputData->gtmPercentage = pTmcData->gtm_percentage;
        IQSettingUtils::Memcpy(&pInput->pAdrcOutputData->kneePoints.KneePointsTMC10.kneeX[0],
                           &kneeX[0],
                           sizeof(FLOAT) * TMC10_KNEE_POINTS_NUM);
        IQSettingUtils::Memcpy(&pInput->pAdrcOutputData->kneePoints.KneePointsTMC10.kneeY[0],
                           &kneeY[0],
                           sizeof(FLOAT) * TMC10_KNEE_POINTS_NUM);
        IQSettingUtils::Memcpy(&pInput->pAdrcOutputData->coefficient[0], &coef[0], sizeof(FLOAT) * TMC10_COEF_NUM);
        pInput->pAdrcOutputData->drcGainDark   = drcGainDark;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC10Interpolation::RunInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC10Interpolation::RunInterpolation(
    const TMC10InputData*          pInput,
    tmc_1_0_0::tmc10_rgn_dataType* pData)
{
    BOOL                          result  = TRUE;
    UINT                          count   = 0;

    // The interpolation tree total Node
    TuningNode                    nodeSet[TMC10MaxmiumNode];
    // Except leaf node, each valid node needs a scratch buffer to contain intepolation result
    tmc_1_0_0::tmc10_rgn_dataType dataBuffer[TMC10MaxmiumNonLeafNode];
    // TMC Trigger List
    TMC10TriggerList              tmcTrigger;

    if ((NULL != pInput) &&
        (NULL != pData)  &&
        (NULL != pInput->pChromatix))
    {
        // Initialize all the nodes
        for (count = 0; count < TMC10MaxmiumNode; count++)
        {
            if (count < TMC10MaxmiumNonLeafNode)
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], &dataBuffer[count]);
            }
            else
            {
                IQSettingUtils::InitializeNode(&nodeSet[count], NULL);
            }
        }

        // Setup the Top Node
        IQSettingUtils::AddNodeToInterpolationTree(NULL, &nodeSet[0],
            static_cast<VOID*>(&pInput->pChromatix->chromatix_tmc10_core), NULL);

        // Setup Trigger Data
        IQSettingUtils::Memcpy(&tmcTrigger.controlType,
                               &(pInput->pChromatix->control_method),
                               sizeof(tmc_1_0_0::chromatix_tmc10Type::control_methodStruct));

        tmcTrigger.triggerAEC     = IQSettingUtils::GettriggerAEC(pInput->pChromatix->control_method.aec_exp_control,
                                                                  pInput->luxIndex,
                                                                  pInput->realGain);

        // Set up Interpolation Tree
        result = IQSettingUtils::SetupInterpolationTree(&nodeSet[0],
                                                        TMC10InterpolationLevel,
                                                        &TMC10OperationTable[0],
                                                        static_cast<VOID*>(&tmcTrigger));
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
                                                       TMC10MaxmiumNonLeafNode,
                                                       TMC10InterpolationLevel,
                                                       TMC10Interpolation::DoInterpolation);
        if (TRUE == result)
        {
            IQSettingUtils::Memcpy(pData, nodeSet[0].pData, sizeof(tmc_1_0_0::tmc10_rgn_dataType));

            // Calculate ADRC Gain Curve
            CalcGainCurve(pInput, pData);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TMC10Interpolation::DoInterpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC10Interpolation::DoInterpolation(
    VOID* pData1,
    VOID* pData2,
    FLOAT ratio,
    VOID* pResult)
{
    tmc_1_0_0::tmc10_rgn_dataType* pInput1 = NULL;
    tmc_1_0_0::tmc10_rgn_dataType* pInput2 = NULL;
    tmc_1_0_0::tmc10_rgn_dataType* pOutput = NULL;
    BOOL                           result  = TRUE;

    // ....Region1           Interpolation     Region2
    //    ---------- | ------------------- | ----------
    // ....ratio= 0.0......ratio (>0 && <1).....ratio = 1.0

    if ((NULL != pData1) &&
        (NULL != pData2) &&
        (NULL != pResult))
    {
        pInput1 = static_cast<tmc_1_0_0::tmc10_rgn_dataType*>(pData1);
        pInput2 = static_cast<tmc_1_0_0::tmc10_rgn_dataType*>(pData2);
        pOutput = static_cast<tmc_1_0_0::tmc10_rgn_dataType*>(pResult);

        if ((ratio > 0.0f) && (ratio < 1.0f))
        {
            result = TMC10Interpolation::InterpolationData(pInput1, pInput2, ratio, pOutput);
        }
        else
        {
            if (TRUE == IQSettingUtils::FEqual(ratio, 0.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput1, sizeof(tmc_1_0_0::tmc10_rgn_dataType));
            }
            else if (TRUE == IQSettingUtils::FEqual(ratio, 1.0f))
            {
                IQSettingUtils::Memcpy(pOutput, pInput2, sizeof(tmc_1_0_0::tmc10_rgn_dataType));
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
/// TMC10Interpolation::InterpolationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TMC10Interpolation::InterpolationData(
    tmc_1_0_0::tmc10_rgn_dataType* pInput1,
    tmc_1_0_0::tmc10_rgn_dataType* pInput2,
    FLOAT                          ratio,
    tmc_1_0_0::tmc10_rgn_dataType* pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput1) &&
        (NULL != pInput2) &&
        (NULL != pOutput))
    {
        pOutput->gtm_percentage    = IQSettingUtils::InterpolationFloatBilinear(pInput1->gtm_percentage,
                                                                                pInput2->gtm_percentage,
                                                                                ratio);
        pOutput->ltm_percentage    = IQSettingUtils::InterpolationFloatBilinear(pInput1->ltm_percentage,
                                                                                pInput2->ltm_percentage,
                                                                                ratio);
        pOutput->la_percentage     = IQSettingUtils::InterpolationFloatBilinear(pInput1->la_percentage,
                                                                                pInput2->la_percentage,
                                                                                ratio);
        pOutput->gamma_percentage  = IQSettingUtils::InterpolationFloatBilinear(pInput1->gamma_percentage,
                                                                                pInput2->gamma_percentage,
                                                                                ratio);
        pOutput->knee_out_mid      = IQSettingUtils::InterpolationFloatBilinear(pInput1->knee_out_mid,
                                                                                pInput2->knee_out_mid,
                                                                                ratio);
        pOutput->dark_boost_ratio  = IQSettingUtils::InterpolationFloatBilinear(pInput1->dark_boost_ratio,
                                                                                pInput2->dark_boost_ratio,
                                                                                ratio);
        pOutput->core_rsv_para1    = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para1,
                                                                                pInput2->core_rsv_para1,
                                                                                ratio);
        pOutput->core_rsv_para2    = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para2,
                                                                                pInput2->core_rsv_para2,
                                                                                ratio);
        pOutput->core_rsv_para3    = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para3,
                                                                                pInput2->core_rsv_para3,
                                                                                ratio);
        pOutput->core_rsv_para4    = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para4,
                                                                                pInput2->core_rsv_para4,
                                                                                ratio);
        pOutput->core_rsv_para5    = IQSettingUtils::InterpolationFloatBilinear(pInput1->core_rsv_para5,
                                                                                pInput2->core_rsv_para5,
                                                                                ratio);
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMC10Interpolation::AECSearchNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT TMC10Interpolation::AECSearchNode(
    TuningNode* pParentNode,
    VOID*       pTriggerData,
    TuningNode* pChildNode)
{
    UINT   childCount   = 0;
    FLOAT  triggerValue = 0.0f;
    UINT32 regionNumber = 0;

    TriggerRegion       triggerRegion[MaxNumRegion];
    InterpolationOutput regionOutput;

    tmc_1_0_0::chromatix_tmc10_coreType*   pParentDataType = NULL;
    TMC10TriggerList*  pTriggerList    = NULL;

    if ((NULL != pParentNode)    &&
        (NULL != pTriggerData)   &&
        (NULL != pChildNode))
    {
        pParentDataType = static_cast<tmc_1_0_0::chromatix_tmc10_coreType*>(pParentNode->pNodeData);
        regionNumber    = pParentDataType->mod_tmc10_aec_dataCount;
        pTriggerList    = static_cast<TMC10TriggerList*>(pTriggerData);

        for (UINT count = 0; count < regionNumber; count++)
        {
            IQSettingUtils::CopyTriggerRegionAEC(
                pTriggerList->controlType.aec_exp_control,
                &(pParentDataType->mod_tmc10_aec_data[count].aec_trigger),
                &(triggerRegion[count]));
        }

        triggerValue = pTriggerList->triggerAEC;

        // Find the index based on the trigger value
        IQSettingUtils::GetIndexPtTrigger(&triggerRegion[0], regionNumber, triggerValue, &regionOutput);

        // Adding Intepolation Value to ParentNode
        pParentNode->interpolationValue[0] = regionOutput.interpolationRatio;

        // Set up child 1
        IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
           static_cast<VOID*>(&pParentDataType->mod_tmc10_aec_data[regionOutput.startIndex]),
           static_cast<VOID*>(&pParentDataType->mod_tmc10_aec_data[regionOutput.startIndex].tmc10_rgn_data));
        childCount++;

        if (regionOutput.startIndex != regionOutput.endIndex)
        {
            // Set up child 2
            IQSettingUtils::AddNodeToInterpolationTree(pParentNode, &pChildNode[childCount],
                static_cast<VOID*>(&pParentDataType->mod_tmc10_aec_data[regionOutput.endIndex]),
                static_cast<VOID*>(&pParentDataType->mod_tmc10_aec_data[regionOutput.endIndex].tmc10_rgn_data));
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
