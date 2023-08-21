// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gtm10setting.cpp
/// @brief IFE GTM10 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "gtm10setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GTM10Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GTM10Setting::CalculateHWSetting(
    GTM10InputData*                                       pInput,
    const gtm_1_0_0::gtm10_rgn_dataType*                  pData,
    const gtm_1_0_0::chromatix_gtm10_reserveType*         pReserveType,
    gtm_1_0_0::chromatix_gtm10Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    INT32  kHighlight       = 16383;
    INT32  kLowlight        = 0;
    FLOAT  baseRatio        = 1.0f;
    FLOAT  extraRatio       = 0.0f;
    INT32  yRatioI          = 6;
    INT32  yRatioQ          = 12;
    INT32  yRatioSlopeI     = 6;
    INT32  yRatioSlopeQ     = 20;
    INT32  sumHist          = 0;
    INT32  maxVHist         = 0;
    INT32  minVHist         = 0;
    INT32  nHist            = 0;
    INT32  xDelta           = 0;
    INT32  xTmp             = 0;
    FLOAT  minPercentileNum = 0.001f;
    FLOAT  maxPercentileNum = 0.999f;
    FLOAT  sum              = 0.0f;
    FLOAT  stretchGain      = 1.0f;
    FLOAT  maxV             = 0.0f;
    FLOAT  key              = 0.0f;
    INT32  temp             = 0;
    FLOAT  tmp              = 0;
    DOUBLE tmpd             = 0.0f;
    BOOL   result           = TRUE;
    INT    yratio_q         = 12;
    INT    yratio_i         = 6;
    INT    yrslope_i        = 6;
    INT    yrslope_q        = 20;


    DOUBLE YratioBase[GTM10LUTSize + 1] = {0.0};
    INT32  YratioSlope[GTM10LUTSize];
    UINT32 hist[GTM10NumBins];

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pReserveType)  &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        INT32  GTMMaxValTh      = IQSettingUtils::RoundFLOAT(pData->maxval_th);
        INT32  GTMYoutMaxVal    = IQSettingUtils::RoundFLOAT(pData->yout_maxval);
        INT32  GTMMinValTh      = IQSettingUtils::RoundFLOAT(pData->minval_th);
        INT32  midLightThlow    = IQSettingUtils::RoundFLOAT(pData->midlight_threshold_low);
        INT32  midLightThHigh   = IQSettingUtils::RoundFLOAT(pData->midlight_threshold_high);
        INT32  maxRatio         = IQSettingUtils::RoundFLOAT(pData->max_ratio * GTM10MaxRatioFactor);

        FLOAT  tmcOutGtmGainCurve[GTM10LUTSize + 1];

        GTM10UnpackedField* pUnpackedField = static_cast<GTM10UnpackedField*>(pOutput);

        extraRatio = pInput->AECSensitivity * pData->a_middletone;

        if (pInput->pAdrcInputData         &&
            pInput->pAdrcInputData->gtmEnable)
        {
            if ((SWTMCVersion::TMC11 == pInput->pAdrcInputData->version) ||
                (SWTMCVersion::TMC12 == pInput->pAdrcInputData->version))
            {
                FLOAT* pKneeX;
                FLOAT* pKneeY;

                if (SWTMCVersion::TMC11 == pInput->pAdrcInputData->version)
                {
                    pKneeX = &pInput->pAdrcInputData->kneePoints.KneePointsTMC11.kneeX[0];
                    pKneeY = &pInput->pAdrcInputData->kneePoints.KneePointsTMC11.kneeY[0];
                }
                else
                {
                    // For (SWTMCVersion::TMC12 == pInput->pAdrcInputData->version)
                    pKneeX = &pInput->pAdrcInputData->kneePoints.KneePointsTMC12.kneeX[0];
                    pKneeY = &pInput->pAdrcInputData->kneePoints.KneePointsTMC12.kneeY[0];
                }

                if ( pInput->pAdrcInputData->curveModel == 2)
                {
                    // PCHIP curve
                    IQSettingUtils::PCHIPCurve(pKneeX,
                                               pKneeY,
                                               &pInput->pAdrcInputData->pchipCoeffficient[0],
                                               xInNormalized,
                                               tmcOutGtmGainCurve,
                                               GTM10LUTSize);
                }
                else
                {
                    // Bezier curve
                    IQSettingUtils::BezierCurve(pKneeX,
                                                pKneeY,
                                                xInNormalized,
                                                tmcOutGtmGainCurve,
                                                GTM10LUTSize);
                }

                IQSettingUtils::EnhanceGlobalContrast(xInNormalized,
                                                        tmcOutGtmGainCurve,
                                                        pInput->pAdrcInputData,
                                                        pInput->pAdrcInputData->gtmPercentage,
                                                        yratio_q,
                                                        yratio_i);
                for (UINT32 i = 0; i < GTM10LUTSize + 1; i++)
                {
                    YratioBase[i] = IQSettingUtils::ClampFLOAT(tmcOutGtmGainCurve[i] ,
                                                               0.0f,
                                                               static_cast<FLOAT>((1 << (yratio_i + yratio_q))) - 1);
                }
            }
            else
            {
                // For SWTMCVersion::TMC10 == pInput->pAdrcInputData->version
                IQSettingUtils::GainCurveSampling(pInput->pAdrcInputData->kneePoints.KneePointsTMC10.kneeX,
                                                  pInput->pAdrcInputData->kneePoints.KneePointsTMC10.kneeY,
                                                  pInput->pAdrcInputData->coefficient,
                                                  reinterpret_cast<const FLOAT*>(&xIn[0]),
                                                  tmcOutGtmGainCurve,
                                                  GTM10LUTSize,
                                                  pInput->pAdrcInputData->gtmPercentage,
                                                  pInput->pAdrcInputData->drcGainDark,
                                                  yratio_q);

                for (UINT32 i = 0; i < GTM10LUTSize + 1; i++)
                {
                    YratioBase[i] = static_cast<FLOAT>(IQSettingUtils::RoundFLOAT(tmcOutGtmGainCurve[i]));
                }
            }
        }
        else
        {
            if (pReserveType->manual_curve_enable)
            {
                for (UINT32 i = 0; i < GTM10LUTSize + 1; i++)
                {
                    YratioBase[i] = static_cast<FLOAT>(IQSettingUtils::RoundFLOAT(pData->manual_curve_strength *
                        (pData->yratio_base_manual_tab.yratio_base_manual[i] - 4096.0f) + 4096.0f));

                }
            }
            else
            {
                if (pReserveType->v2_enable)
                {
                    for (UINT32 i = 0; i < GTM10LUTSize + 1; i++)
                    {
                        if (i <= static_cast<UINT32>(IQSettingUtils::RoundFLOAT(pData->middletone_w)))
                        {
                            YratioBase[i] = 4096.0f * (baseRatio + extraRatio);
                        }
                        else
                        {
                            YratioBase[i] = 4096.0f * (baseRatio + extraRatio * static_cast<INT>(GTM10LUTSize - i) /
                                (GTM10LUTSize - IQSettingUtils::RoundFLOAT(pData->middletone_w)));
                        }
                    }
                }
                else
                {
                    // Ignore the initial BHist stats. The parsed bhist stats will lag the current request ID by pipeline delay
                    // Use the manual curve for the initial frames
                    if ((NULL != pInput->pGRHist) && (pInput->BHistRequestID > pInput->maxPipelineDelay))
                    {
                        // recover 1K bins from gr/gb/r/b bins
                        for (UINT32 i = 0; i < GTM10NumBins; i++)
                        {
                            hist[i] = pInput->pGRHist[i];
                            nHist += hist[i];
                        }

                        // using histogrm to compute key and max_Y
                        minPercentileNum = (nHist * pData->min_percentile);
                        maxPercentileNum = (nHist * pData->max_percentile);

                        for (UINT32 i = 0; i < GTM10NumBins; i++)
                        {
                            sumHist += hist[i];
                            if ((sumHist <= maxPercentileNum) && (i < GTM10NumBins - 1))
                            {
                                maxVHist = binToLuma[i + 1];
                            }
                            if ((sumHist < minPercentileNum) && (i < GTM10NumBins - 1))
                            {
                                minVHist = binToLuma[i + 1];
                            }
                            sum += hist[i] * logBinGTM[i];
                        }

                        // linear gain up
                        if (maxVHist < pData->luma_peak_th0)
                        {
                            stretchGain = pData->stretch_gain_0;
                        }
                        else if (maxVHist <= pData->luma_peak_th1)
                        {
                            stretchGain = (pData->luma_peak_th1 - maxVHist) * (pData->stretch_gain_0 - pData->stretch_gain_1) /
                                (pData->luma_peak_th1 - pData->luma_peak_th0) + pData->stretch_gain_1;
                        }
                        else
                        {
                            stretchGain = pData->stretch_gain_1;
                        }

                        // change maxVHist to boost up some low dynamic range images
                        // set maxVHist to maximum input to avoid highlight clipping
                        // Always treat as HDR MSB aligned
                        maxVHist = 16383;        // 14 bits

                        if (maxVHist < GTMMaxValTh)  // 1024
                        {
                            maxVHist = GTMMaxValTh;
                        }

                        if (minVHist > GTMMinValTh)  // 128
                        {
                            minVHist = GTMMinValTh;
                        }

                        key = static_cast<FLOAT>(exp(sum / nHist));

                        FLOAT f = static_cast<FLOAT>((2 * log(key + 1) - log(minVHist + 1) - log(maxVHist + 1)) /
                            (log(maxVHist + 1) - log(minVHist + 1)));
                        FLOAT a = pData->a_middletone * powf(4.0f, f);

                        if (pInput->totalNumFrames > 1)
                        {
                            /// @todo (CAMX-1843) Need to populate below parameters from Meta data/calculated values
                            // Could populate below prev params during create node with default values
                            if (pInput->frameNum == 0)
                            {
                                pInput->keyPrevious = key;
                                pInput->alphaPrevious = a;
                                pInput->maxVHistPrevious = maxVHist;
                                pInput->minVHistPrevious = minVHist;
                            }

                            key = key + (pInput->keyPrevious - key) * pData->temporal_w;
                            pInput->keyPrevious = key;
                            a = a + (pInput->alphaPrevious - a) * pData->temporal_w;
                            pInput->alphaPrevious = a;
                            maxVHist = static_cast<INT32>(maxVHist + (pInput->maxVHistPrevious - maxVHist) *
                                pData->temporal_w);
                            pInput->maxVHistPrevious = maxVHist;
                            minVHist = static_cast<INT32>(minVHist + (pInput->minVHistPrevious - minVHist) *
                                pData->temporal_w);
                            pInput->minVHistPrevious = minVHist;
                        }

                        maxV = (a * maxVHist) / key;

                        for (UINT32 i = 0; i < (GTM10LUTSize + 1); i++)
                        {
                            xTmp = (xIn[i] < 1) ? 1 : xIn[i];
                            tmp = (a * xTmp) / key;
                            tmpd = ((GTMYoutMaxVal * (1.0 + tmp / (maxV * maxV)) * tmp / (1.0 + tmp)) * 4096.0f / xTmp);
                            YratioBase[i] = static_cast<FLOAT>(tmpd);

                            // blending with 45 degree line
                            FLOAT w = 1.0f;
                            if (xIn[i] < midLightThlow)
                            {
                                w = (1.0f - pData->lowlight_w) * (xIn[i] - kLowlight) /
                                    (midLightThlow - kLowlight) + pData->lowlight_w;
                            }
                            else if (xIn[i] > midLightThHigh)
                            {
                                w = (pData->highlight_w - 1.0f) * (xIn[i] - midLightThHigh) /
                                    (kHighlight - midLightThHigh) + 1.0f;
                            }

                            w *= pData->middletone_w;
                            YratioBase[i] = YratioBase[i] * w + 4096 * (1.0f - w);
                            YratioBase[i] = YratioBase[i] * stretchGain;

                            // clip to max ratio
                            if (static_cast<INT32>(YratioBase[i]) >= maxRatio)
                            {
                                YratioBase[i] = static_cast<FLOAT>(maxRatio); // clip to max
                            }
                            if (static_cast<INT32>(YratioBase[i]) >= (1 << (yRatioI + yRatioQ)))
                            {
                                YratioBase[i] = static_cast<FLOAT>((1 << (yRatioI + yRatioQ)) - 1); // clip to max
                            }
                        }
                    }
                    else
                    {
                        // use manual curve
                        for (UINT32 i = 0; i < GTM10LUTSize + 1; i++)
                        {
                            YratioBase[i] = static_cast<FLOAT>(IQSettingUtils::RoundFLOAT(pData->manual_curve_strength *
                                (pData->yratio_base_manual_tab.yratio_base_manual[i] - 4096.0f) + 4096.0f));
                        }
                    }
                }
            }
        }
        for (UINT32 i = 0; i < GTM10LUTSize; i++)
        {
            xDelta = xIn[i + 1] - xIn[i];

            if (xDelta > 0)
            {
                temp = static_cast<INT32>((YratioBase[i + 1] - YratioBase[i]) * (1ULL << (yRatioSlopeQ - yRatioQ)));
                YratioSlope[i] = (temp >= 0) ? ((temp + xDelta / 2) / xDelta) : ((temp - xDelta / 2) / xDelta);
            }
            else
            {
                YratioSlope[i] = 0;
            }

            if (YratioSlope[i] >= (1 << (yRatioSlopeI + yRatioSlopeQ - 1)) ||
                                   (YratioSlope[i] <  (-1 << (yRatioSlopeI + yRatioSlopeQ - 1))))
            {
                YratioSlope[i] = (YratioSlope[i] > 0) ? ((1 << (yRatioSlopeI + yRatioSlopeQ - 1)) - 1) :
                    -(1 << (yRatioSlopeI + yRatioSlopeQ - 1));
            }
        }

        pUnpackedField->enable   = static_cast<UINT16>(pModuleEnable->gtm_enable);
        pUnpackedField->tableSel = pInput->LUTBankSel;

        for (UINT32 i = 0; i < GTM10LUTSize; i++)
        {
            pUnpackedField->YratioBase[pUnpackedField->tableSel][i]  = static_cast<UINT32>(YratioBase[i]) & 0x3FFFF;
            pUnpackedField->YratioSlope[pUnpackedField->tableSel][i] = YratioSlope[i] & 0x3FFFFFF;
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}
