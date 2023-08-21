// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  iqsettingutil.cpp
/// @brief Util functions for IQ Setting Classes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "demux_1_3_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "NcLibChipInfo.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::SetupInterpolationTree
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQSettingUtils::SetupInterpolationTree(
    TuningNode*    pTipNode,
    UINT           numOfLevel,
    NodeOperation* pOperationTable,
    VOID*          pTriggerList)
{
    BOOL  result                 = TRUE;
    UINT  nodeIndexPerChildLevel = 0;    ///< Index of node at child level
    UINT  nodeIndex              = 0;    ///< Index of the node in the node array
    UINT  count1                 = 0;
    UINT  count2                 = 0;
    UINT  totalNodePerLevel      = 1;
    UINT  childNodeIndex         = 0;
    UINT  numChildNode           = 0;
    UINT  childCount             = 0;

    if ((NULL != pTipNode) && (0 != numOfLevel) && (NULL != pOperationTable))
    {
        nodeIndex = 0;

        // Go though each level above leaf node
        for (count1 = 0; count1 < (numOfLevel-1); count1++)
        {
            nodeIndexPerChildLevel = 0;

            // Calculate the number of nodes at this level
            if ( count1 > 0)
            {
                totalNodePerLevel = totalNodePerLevel * pOperationTable[count1 -1].numChildPerNode;
            }
            else
            {
                totalNodePerLevel = 1;
            }

            // Go through all the nodes at this level
            for (count2 = 0; count2 < totalNodePerLevel; count2++)
            {
                numChildNode = pOperationTable[count1].numChildPerNode;

                if (TRUE == pTipNode[nodeIndex + count2].isValid)
                {
                    // calculate the child node index
                    childNodeIndex = nodeIndex + totalNodePerLevel + nodeIndexPerChildLevel;
                    // search and set up the  child nodes
                    childCount = pOperationTable[count1].pSearchChildNode(
                                     &pTipNode[nodeIndex + count2], pTriggerList,
                                     &pTipNode[childNodeIndex]);

                    if (0 == childCount)
                    {
                        // Adding Fatal Error Logging
                        break;
                    }
                }

                nodeIndexPerChildLevel += numChildNode;
            }

            // Increate the overall cound index
            nodeIndex += totalNodePerLevel;
        }
    }

    return result;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::GainCurveSampling
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQSettingUtils::GainCurveSampling(
    FLOAT*       pKneeInput,
    FLOAT*       pKneeOutput,
    FLOAT*       pCoef,
    const FLOAT* pInput,
    FLOAT*       pOutput,
    UINT32       lutSize,
    FLOAT        rate,
    FLOAT        gain,
    UINT32       qFactor)
{
    FLOAT  xInputNorm  = 0.0f;
    FLOAT  yOutputNorm = 0.0f;
    UINT32 i;

    for (i = 1; i < (lutSize + 1); i++)
    {
        xInputNorm = pInput[i] / pInput[lutSize];
        yOutputNorm = 0;

        if (xInputNorm <= pKneeInput[1])
        {
            // actively boost dark area
            yOutputNorm = xInputNorm * gain;
        }
        else if (xInputNorm <= pKneeInput[2])
        {
            // fully recovered from under-exposure image
            yOutputNorm = pKneeOutput[1] + (xInputNorm - pKneeInput[1]) * (pCoef[0] + (xInputNorm - pKneeInput[1]) *
                (pCoef[1] + (xInputNorm - pKneeInput[1]) * pCoef[2]));
        }
        else
        {
            yOutputNorm = pKneeOutput[2] + (xInputNorm - pKneeInput[2]) * (pCoef[3] + (xInputNorm - pKneeInput[2]) *
                (pCoef[4] + (xInputNorm - pKneeInput[2]) * pCoef[5]));
        }

        yOutputNorm = IQSettingUtils::ClampFLOAT(yOutputNorm, MIN_NORM_OUTPUT_GAIN, MAX_NORM_OUTPUT_GAIN);

        pOutput[i] = yOutputNorm / xInputNorm;
        pOutput[i] = IQSettingUtils::ClampFLOAT(pOutput[i], MIN_NORM_OUTPUT_GAIN, gain);

    }
    pOutput[0] = pOutput[1];

    for (i = 0; i < (lutSize + 1); i++)
    {
        pOutput[i] = powf(pOutput[i], rate) *  static_cast<FLOAT>(1 << qFactor);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::InitializeNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IQSettingUtils::InitializeNode(
    TuningNode* pNode,
    VOID*       pDataBuffer)
{
    pNode->level     = 0;
    pNode->isValid   = FALSE;
    pNode->numChild  = 0;
    pNode->pNodeData = NULL;
    pNode->pData     = pDataBuffer;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::AddNodeToInterpolationTree
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IQSettingUtils::AddNodeToInterpolationTree(
    TuningNode* pParentNode,
    TuningNode* pChildNode,
    VOID*       pData,
    VOID*       pTuningData)
{
    pChildNode->isValid   = TRUE;
    pChildNode->pNodeData = pData;

    if (NULL != pTuningData)
    {
        pChildNode->pData = pTuningData;
    }

    if (NULL != pParentNode)
    {
        pParentNode->pChildNode[pParentNode->numChild] = pChildNode;
        pParentNode->numChild++;
        pChildNode->level = pParentNode->level + 1;
    }
    else
    {
        // Starting with level 1
        pChildNode->level = 1;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::InterpolateTuningData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQSettingUtils::InterpolateTuningData(
    TuningNode*       pNode,
    UINT              numOfNoLeafNode,
    UINT              numOfLevel,
    DoInterpolation   pDoInterpolation)
{
    BOOL result = TRUE;

    if (NULL != pNode)
    {
        for (INT count = (numOfNoLeafNode - 1); count >= 0; count--)
        {
            if ((TRUE == pNode[count].isValid) &&
                (numOfLevel > pNode[count].level))
            {
                if (1 == pNode[count].numChild)
                {
                    if (count > 0)
                    {
                        pNode[count].pData = pNode[count].pChildNode[0]->pData;
                    }
                    else
                    {
                        // On the top node, we don't want to reuse the chromatix buffer
                        pDoInterpolation(
                            pNode[count].pChildNode[0]->pData,
                            pNode[count].pChildNode[0]->pData,
                            0,
                            pNode[count].pData);
                    }
                }
                else if (2 == pNode[count].numChild)
                {
                    pDoInterpolation(
                                     pNode[count].pChildNode[0]->pData,
                                     pNode[count].pChildNode[1]->pData,
                                     pNode[count].interpolationValue[0],
                                     pNode[count].pData);
                }
                else if (3 == pNode[count].numChild)
                {
                    pDoInterpolation(
                                     pNode[count].pChildNode[1]->pData,
                                     pNode[count].pChildNode[2]->pData,
                                     pNode[count].interpolationValue[1],
                                     pNode[count].pData);

                    pDoInterpolation(
                                     pNode[count].pChildNode[0]->pData,
                                     pNode[count].pData,
                                     pNode[count].interpolationValue[0],
                                     pNode[count].pData);
                }
            }
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }
    return result;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::GetIndexPtTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IQSettingUtils::GetIndexPtTrigger(
    TriggerRegion*       pTriggerSet,
    UINT32               numRegion,
    FLOAT                triggerValue,
    InterpolationOutput* pOutput0)
{
    UINT32 startRegionIndex    = 0;
    FLOAT  interpolationRatio  = 0.0f;
    BOOL   regionIdentified    = FALSE;

    if ((numRegion > 0) && (NULL != pTriggerSet) && (NULL != pOutput0))
    {
        pOutput0->startIndex = 0;
        pOutput0->endIndex   = 0;

        for (startRegionIndex= 0; startRegionIndex < numRegion; startRegionIndex++)
        {
            // If trigger is beyone the last region
            if ((startRegionIndex == (numRegion -1))  &&
                (triggerValue > pTriggerSet[startRegionIndex].end))
            {
                pOutput0->startIndex    = startRegionIndex;
                pOutput0->endIndex      = startRegionIndex;
                regionIdentified        = TRUE;
                break;
            }  // If trigger is between two regions
            else if ((triggerValue > pTriggerSet[startRegionIndex].end)        &&
                     (triggerValue < pTriggerSet[startRegionIndex + 1].start))
            {
                interpolationRatio = IQSettingUtils::CalculateInterpolationRatio(
                                  static_cast<DOUBLE>(triggerValue),
                                  static_cast<DOUBLE>(pTriggerSet[startRegionIndex].end),
                                  static_cast<DOUBLE>(pTriggerSet[startRegionIndex + 1].start));

                pOutput0->startIndex    = startRegionIndex;
                pOutput0->endIndex      = startRegionIndex + 1;
                regionIdentified        = TRUE;
                break;
            } // If trigger is in one region
            else if (triggerValue <= pTriggerSet[startRegionIndex].end)
            {
                pOutput0->startIndex = startRegionIndex;
                pOutput0->endIndex   = pOutput0->startIndex;
                regionIdentified     = TRUE;
                break;
            }
        }

        if (FALSE == regionIdentified)
        {
            pOutput0->startIndex = 0;
            pOutput0->endIndex   = 0;
            /// @todo (CAMX-1812) Need to add logging for Common library
        }

        pOutput0->interpolationRatio = interpolationRatio;
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::ModuleInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQSettingUtils::ModuleInitialize(
    IQLibraryData* pData)
{
    BOOL result = TRUE;

    pData->pCustomticData = NULL;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::ModuleUninitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQSettingUtils::ModuleUninitialize(
    IQLibraryData* pData)
{
    BOOL result = TRUE;

    pData->pCustomticData = NULL;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::DumpTriggerCondition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IQSettingUtils::DumpTriggerCondition(
    ISPIQTriggerData* pTriggerData)
{
    CAMX_LOG_INFO(CamxLogGroupISP, "AECexposureTime = %f",      pTriggerData->AECexposureTime);
    CAMX_LOG_INFO(CamxLogGroupISP, "AECexposureGainRatio = %f", pTriggerData->AECexposureGainRatio);
    CAMX_LOG_INFO(CamxLogGroupISP, "AECSensitivity = %f",       pTriggerData->AECSensitivity);
    CAMX_LOG_INFO(CamxLogGroupISP, "AECGain = %f",              pTriggerData->AECGain);
    CAMX_LOG_INFO(CamxLogGroupISP, "AECLuxIndex = %f",          pTriggerData->AECLuxIndex);
    CAMX_LOG_INFO(CamxLogGroupISP, "AWBleftGGainWB = %f",       pTriggerData->AWBleftGGainWB);
    CAMX_LOG_INFO(CamxLogGroupISP, "AWBleftBGainWB = %f",       pTriggerData->AWBleftBGainWB);
    CAMX_LOG_INFO(CamxLogGroupISP, "AWBleftRGainWB = %f",       pTriggerData->AWBleftRGainWB);
    CAMX_LOG_INFO(CamxLogGroupISP, "AWBColorTemperature = %f",  pTriggerData->AWBColorTemperature);
    CAMX_LOG_INFO(CamxLogGroupISP, "DRCGain = %f",              pTriggerData->DRCGain);
    CAMX_LOG_INFO(CamxLogGroupISP, "lensPosition = %f",         pTriggerData->lensPosition);
    CAMX_LOG_INFO(CamxLogGroupISP, "lensZoom = %f",             pTriggerData->lensZoom);
    CAMX_LOG_INFO(CamxLogGroupISP, "postScaleRatio = %f",       pTriggerData->postScaleRatio);
    CAMX_LOG_INFO(CamxLogGroupISP, "preScaleRatio = %f",        pTriggerData->preScaleRatio);
    CAMX_LOG_INFO(CamxLogGroupISP, "sensorImageWidth = %d",     pTriggerData->sensorImageWidth);
    CAMX_LOG_INFO(CamxLogGroupISP, "sensorImageHeight = %d",    pTriggerData->sensorImageHeight);
    CAMX_LOG_INFO(CamxLogGroupISP, "CAMIFWidth = %d",           pTriggerData->CAMIFWidth);
    CAMX_LOG_INFO(CamxLogGroupISP, "CAMIFHeight = %d",          pTriggerData->CAMIFHeight);
    CAMX_LOG_INFO(CamxLogGroupISP, "numberOfLED = %d",          pTriggerData->numberOfLED);
    CAMX_LOG_INFO(CamxLogGroupISP, "LEDSensitivity = %d",       pTriggerData->LEDSensitivity);
    CAMX_LOG_INFO(CamxLogGroupISP, "bayerPattern = %d",         pTriggerData->bayerPattern);
    CAMX_LOG_INFO(CamxLogGroupISP, "sensorOffsetX = %d",        pTriggerData->sensorOffsetX);
    CAMX_LOG_INFO(CamxLogGroupISP, "sensorOffsetY = %d",        pTriggerData->sensorOffsetY);
    CAMX_LOG_INFO(CamxLogGroupISP, "blackLevelOffset = %d",     pTriggerData->blackLevelOffset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::IQSetHardwareVersion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQSettingUtils::IQSetHardwareVersion(
    UINT32 titanVersion,
    UINT32 hardwareVersion)
{
    NcLibChipInfo::Set(titanVersion, hardwareVersion);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::GetDynamicEnableFlag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQSettingUtils::GetDynamicEnableFlag(
    INT32                                       dynEnTrig_enable,
    ispglobalelements::control_var_type         dynEnTrig_hyst_control_var,
    ispglobalelements::hyst_direction           dynEnTrig_hyst_mode,
    ispglobalelements::trigger_pt_couplet_type* couplet_ptr,
    VOID*                                       dynamicTriggerPtr,
    BOOL*                                       pEnable)
{
    FLOAT trigger      = 0.0f;
    FLOAT triggerStart = 0.0f;
    FLOAT triggerEnd   = 0.0f;

    ISPIQTriggerData* pDynamicTrigger = static_cast<ISPIQTriggerData*>(dynamicTriggerPtr);

    if (TRUE == dynEnTrig_enable)
    {
        switch (dynEnTrig_hyst_control_var)
        {
            case ispglobalelements::control_var_type::control_lens_zoom:
                trigger = pDynamicTrigger->lensZoom;
                break;
            case ispglobalelements::control_var_type::control_lux_idx:
                trigger = pDynamicTrigger->AECLuxIndex;
                break;
            case ispglobalelements::control_var_type::control_gain:
                trigger = pDynamicTrigger->AECGain;
                break;
            case ispglobalelements::control_var_type::control_drc_gain:
                trigger = pDynamicTrigger->DRCGain;
                break;
            case ispglobalelements::control_var_type::control_exp_time_ratio:
                trigger = pDynamicTrigger->AECexposureTime;
                break;
            case ispglobalelements::control_var_type::control_aec_exp_sensitivity_ratio:
                trigger = pDynamicTrigger->AECSensitivity;
                break;
            case ispglobalelements::control_var_type::control_cct:
                trigger = pDynamicTrigger->AWBColorTemperature;
                break;
            case ispglobalelements::control_var_type::control_lens_position:
                trigger = pDynamicTrigger->lensPosition;
                break;
            case ispglobalelements::control_var_type::control_total_scale_ratio:
                trigger = pDynamicTrigger->totalScaleRatio;
                break;
            case  ispglobalelements::control_var_type::control_post_scale_ratio:
                trigger = pDynamicTrigger->postScaleRatio;
                break;
            case ispglobalelements::control_var_type::control_pre_scale_ratio:
                trigger = pDynamicTrigger->preScaleRatio;
                break;
            default:
                break;
        }

        triggerStart = couplet_ptr->start1;
        triggerEnd   = couplet_ptr->end1;

        if ((dynEnTrig_hyst_control_var == ispglobalelements::control_var_type::control_gain) ||
            (dynEnTrig_hyst_control_var == ispglobalelements::control_var_type::control_exp_time_ratio))
        {
            triggerStart = couplet_ptr->start2;
            triggerEnd   = couplet_ptr->end2;
        }

        if (dynEnTrig_hyst_mode == ispglobalelements::hyst_direction::UPWARD)
        {
            if (trigger >= triggerEnd)
            {
                *pEnable = TRUE;
            }
            else if (trigger < triggerStart)
            {
                *pEnable = FALSE;
            }
        }
        else
        {
            if (trigger > triggerEnd)
            {
                *pEnable = FALSE;
            }
            else if (trigger <= triggerStart)
            {
                *pEnable = TRUE;
            }
        }
    }
    else
    {
        *pEnable = TRUE;
    }

    return *pEnable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::PCHIPCurve
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQSettingUtils::PCHIPCurve(
    FLOAT*        pKneeIn,
    FLOAT*        pKneeOut,
    FLOAT*        pPCHIPCoef,
    const  FLOAT* pIn,
    FLOAT*        pOut,
    UINT32        lutSize)
{
    FLOAT linearGain = pKneeOut[1] / pKneeIn[1];

    for (UINT32 i = 0; i < (lutSize + 1); i++)
    {
        FLOAT xInNorm = pIn[i];
        FLOAT yOutNorm = 0;

        if (xInNorm <= pKneeIn[1])
        {
            // actively boost dark area
            yOutNorm = xInNorm * linearGain;
        }
        else if (xInNorm <= pKneeIn[2])
        {
            // fully recovered from under-exposure image
            yOutNorm = pKneeOut[1] + (xInNorm - pKneeIn[1]) * (pPCHIPCoef[0] + (xInNorm - pKneeIn[1]) *
                       (pPCHIPCoef[1] + (xInNorm - pKneeIn[1]) * pPCHIPCoef[2]));
        }
        else if (xInNorm <= pKneeIn[3])
        {
            // fully recovered from under-exposure image
            yOutNorm = pKneeOut[2] + (xInNorm - pKneeIn[2]) * (pPCHIPCoef[3] + (xInNorm - pKneeIn[2]) *
                       (pPCHIPCoef[4] + (xInNorm - pKneeIn[2]) * pPCHIPCoef[5]));
        }
        else
        {
            yOutNorm = pKneeOut[3] + (xInNorm - pKneeIn[3]) * (pPCHIPCoef[6] + (xInNorm - pKneeIn[3]) *
                       (pPCHIPCoef[7] + (xInNorm - pKneeIn[3]) * pPCHIPCoef[8]));
        }

        yOutNorm = IQSettingUtils::ClampFLOAT(yOutNorm, 0.0f, 1.0f);
        xInNorm = xInNorm == 0 ? 1 : xInNorm;
        pOut[i] = yOutNorm / xInNorm;
        pOut[i] = IQSettingUtils::ClampFLOAT(pOut[i], 1.0f, linearGain);
    }

    pOut[0] = pOut[1];

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::BezierCurve
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IQSettingUtils::BezierCurve(
    FLOAT*        pKneeIn,
    FLOAT*        pKneeOut,
    const  FLOAT* pIn,
    FLOAT*        pOut,
    UINT32        lutSize)
{
    FLOAT linearGain = pKneeOut[1] / pKneeIn[1];
    const INT32 bezier_num = 1000;
    FLOAT bezier_x[bezier_num];
    FLOAT bezier_y[bezier_num];

    for (UINT32 i = 0; i < bezier_num; i++)
    {
        FLOAT t = static_cast<FLOAT>(i) / (bezier_num - 1);

        bezier_x[i] = pKneeIn[1] + (pKneeIn[4] - pKneeIn[1]) * (pKneeIn[0] * (1 - t) * (1 - t) * (1 - t) *
                      (1 - t) + pKneeIn[1] * (1 - t) * (1 - t) * (1 - t) * t * 4 + pKneeIn[2] * (1 - t) *
                      (1 - t) * t * t * 6 + pKneeIn[3] * (1 - t) * t * t * t * 4 + pKneeIn[4] * t * t * t * t);
        bezier_y[i] = pKneeOut[1] + (pKneeOut[4] - pKneeOut[1]) * (pKneeOut[0] * (1 - t) * (1 - t) * (1 - t) *
                      (1 - t) + pKneeOut[1] * (1 - t) * (1 - t) * (1 - t) * t * 4 + pKneeOut[2] * (1 - t) *
                      (1 - t) * t * t * 6 + pKneeOut[3] * (1 - t) * t * t * t * 4 + pKneeOut[4] * t * t * t * t);
    }

    for (UINT32 i = 0; i < lutSize + 1; i++)
    {
        FLOAT xInNorm = pIn[i];
        FLOAT yOutNorm = 0;

        if (xInNorm <= pKneeIn[1])
        {
            // actively boost dark area
            yOutNorm = xInNorm * linearGain;
        }
        else
        {
            // simple interpolation
            for (UINT32 bi = 1; bi < bezier_num; bi++)
            {
                if (xInNorm <= bezier_x[bi])
                {
                    yOutNorm = ((bezier_x[bi] - xInNorm) * bezier_y[bi - 1] + (xInNorm - bezier_x[bi - 1]) * bezier_y[bi]) /
                                (bezier_x[bi] - bezier_x[bi - 1]);
                    break;
                }
            }
        }

        yOutNorm = IQSettingUtils::ClampFLOAT(yOutNorm, 0.0f, 1.0f);
        xInNorm = xInNorm == 0 ? 1 : xInNorm;
        pOut[i] = yOutNorm / xInNorm;
        pOut[i] = IQSettingUtils::ClampFLOAT(pOut[i], 1.0f, linearGain);
    }

    // pOut[0] = pOut[1];

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IQSettingUtils::EnhanceGlobalContrast
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IQSettingUtils::EnhanceGlobalContrast(
    const FLOAT*    pIn,
    FLOAT*          pGain,
    ADRCData*       pADRCData,
    FLOAT           percentage,
    UINT32          scaleFactor,
    UINT32          clampFactor)
{
    // increase global contrast by blending HE curve
    BOOL  isContrastCurveValid = ((pADRCData->contrastHEBright > 0.0f) | (pADRCData->contrastHEDark > 0.0f));
    FLOAT scale                = static_cast<FLOAT>(1 << scaleFactor);
    FLOAT clampMax             = static_cast<FLOAT>((1 << (clampFactor + scaleFactor)) - 1);
    UINT32 hi                  = 1;

    for (UINT32 i = 0; i < LUTSize + 1; i++)
    {
        FLOAT yOutHist = 1.0f;
        FLOAT yOutTmc = 1.0f;
        FLOAT xInNorm = pIn[i];

        if (TRUE == isContrastCurveValid)
        {
            for ( ; hi < 1024; hi++)
            {
                if (xInNorm <= logBinNormalized[hi])
                {
                    FLOAT logBin_i = logBinNormalized[hi];
                    FLOAT logBin_i_m1 = logBinNormalized[hi - 1];

                    yOutHist = ((logBin_i - xInNorm) * pADRCData->contrastEnhanceCurve[hi - 1] + (xInNorm - logBin_i_m1) *
                                pADRCData->contrastEnhanceCurve[hi]) / (logBin_i - logBin_i_m1);

                    break;
                }
            }
        }

        yOutTmc = pGain[i] * xInNorm;

        if (yOutHist > yOutTmc)
        {
            yOutTmc = yOutTmc * (1.0f - pADRCData->contrastHEBright) + yOutHist * pADRCData->contrastHEBright;
        }
        else if (yOutHist < yOutTmc)
        {
            yOutTmc = yOutTmc * (1.0f - pADRCData->contrastHEDark) + yOutHist * pADRCData->contrastHEDark;
        }

        yOutTmc = IQSettingUtils::ClampFLOAT(yOutTmc, 0.0f, 1.0f);

        pGain[i] = powf((yOutTmc / xInNorm), percentage) * scale;
    }

    return;
}
