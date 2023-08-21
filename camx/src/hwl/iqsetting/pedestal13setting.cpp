// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// @file  pedestal13setting.cpp
// @brief pedestal13 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pedestal13setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pedestal13Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pedestal13Setting::CalculateHWSetting(
    const Pedestal13InputData*                                      pInput,
    pedestal_1_3_0::pedestal13_rgn_dataType*                        pData,
    pedestal_1_3_0::chromatix_pedestal13Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                           pOutput)
{
    BOOL   result         = TRUE;
    INT32  index          = 0;
    INT32  scaleCubic     = 0;
    INT32  deltaH         = 0;
    INT32  deltaV         = 0;
    INT32  subgridH       = 0;
    INT32  subgridV       = 0;
    INT32  xStart         = 0;
    INT32  yStart         = 0;
    INT32  subGridX       = 0;
    INT32  subGridY       = 0;
    UINT32 streamInWidth  = 0;
    UINT32 streamInHeight = 0;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        Pedestal13UnpackedField* pUnpackedField = static_cast<Pedestal13UnpackedField*>(pOutput);

        for (UINT32 k = 0; k < 2; k++ )
        {
            for (UINT32 i = 0; i < PED_MESH_PT_V_V13; i++ )
            {
                for (UINT32 j = 0; j < PED_MESH_PT_H_V13; j++ )
                {
                    index = i * PED_MESH_PT_H_V13 + j;
                    pUnpackedField->meshTblT1L[k][0][i][j] =
                        static_cast<UINT16>(
                            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->channel_black_level_r_tab.
                                                                                    channel_black_level_r[index]),
                                                        PEDESTAL13_MIN,
                                                        PEDESTAL13_MAX));

                    pUnpackedField->meshTblT1R[k][0][i][j] =
                        static_cast<UINT16>(
                            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->channel_black_level_r_tab.
                                                                                    channel_black_level_r[index]),
                                                        PEDESTAL13_MIN,
                                                        PEDESTAL13_MAX));

                    pUnpackedField->meshTblT1L[k][1][i][j] =
                        static_cast<UINT16>(
                            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->channel_black_level_gr_tab.
                                                                                    channel_black_level_gr[index]),
                                                        PEDESTAL13_MIN,
                                                        PEDESTAL13_MAX));

                    pUnpackedField->meshTblT1R[k][1][i][j] =
                        static_cast<UINT16>(
                            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->channel_black_level_gr_tab.
                                                                                    channel_black_level_gr[index]),
                                                        PEDESTAL13_MIN,
                                                        PEDESTAL13_MAX));

                    pUnpackedField->meshTblT1L[k][2][i][j] =
                        static_cast<UINT16>(
                            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->channel_black_level_gb_tab.
                                                                                    channel_black_level_gb[index]),
                                                        PEDESTAL13_MIN,
                                                        PEDESTAL13_MAX));

                    pUnpackedField->meshTblT1R[k][2][i][j] =
                        static_cast<UINT16>(
                            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->channel_black_level_gb_tab.
                                                                                    channel_black_level_gb[index]),
                                                        PEDESTAL13_MIN,
                                                        PEDESTAL13_MAX));

                    pUnpackedField->meshTblT1L[k][3][i][j] =
                        static_cast<UINT16>(
                            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->channel_black_level_b_tab.
                                                                                    channel_black_level_b[index]),
                                                        PEDESTAL13_MIN,
                                                        PEDESTAL13_MAX));

                    pUnpackedField->meshTblT1R[k][3][i][j] =
                        static_cast<UINT16>(
                            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->channel_black_level_b_tab.
                                                                                     channel_black_level_b[index]),
                                                        PEDESTAL13_MIN,
                                                        PEDESTAL13_MAX));
                }
            }
        }

        streamInWidth  = pInput->imageWidth;
        streamInHeight = pInput->imageHeight;

        InterpGridOptimization(streamInWidth, streamInHeight, &scaleCubic, &deltaH, &deltaV, &subgridH, &subgridV);

        pUnpackedField->enable           = static_cast<UINT16>(pModuleEnable->pedestal_enable);
        pUnpackedField->intpFactorL      = static_cast<UINT32>((log(static_cast<FLOAT>(scaleCubic)) / log(2.0) + 0.5));
        pUnpackedField->intpFactorR      = static_cast<UINT32>((log(static_cast<FLOAT>(scaleCubic)) / log(2.0) + 0.5));
        pUnpackedField->bHeightL         = subgridV - 1; // hw starts from index 0
        pUnpackedField->bHeightR         = subgridV - 1;
        pUnpackedField->bWidthL          = subgridH - 1;
        pUnpackedField->bWidthR          = subgridH - 1;
        pUnpackedField->meshGridbHeightL = subgridV * scaleCubic - 1;
        pUnpackedField->meshGridbHeightR = subgridV * scaleCubic - 1;
        pUnpackedField->meshGridbWidthL  = subgridH * scaleCubic - 1;
        pUnpackedField->meshGridbWidthR  = subgridH * scaleCubic - 1;
        pUnpackedField->xDeltaL          = IQSettingUtils::RoundFLOAT((static_cast<FLOAT>(1 << QNumber_20U)) /
                                                (static_cast<FLOAT>(pUnpackedField->bWidthL + 1))); // 17uQ20
        pUnpackedField->xDeltaR          = IQSettingUtils::RoundFLOAT((static_cast<FLOAT>(1 << QNumber_20U)) /
                                                (static_cast<FLOAT>(pUnpackedField->bWidthL + 1)));
        pUnpackedField->yDeltaL          = IQSettingUtils::RoundFLOAT((static_cast<FLOAT>(1 << QNumber_20U)) /
                                                (static_cast<FLOAT>(pUnpackedField->bHeightL + 1))); // 17uQ20
        pUnpackedField->yDeltaR          = IQSettingUtils::RoundFLOAT((static_cast<FLOAT>(1 << QNumber_20U)) /
                                                (static_cast<FLOAT>(pUnpackedField->bHeightL + 1)));
        xStart                           = deltaH * 2;
        yStart                           = deltaV * 2;
        subGridX                         = (xStart >> 1) / subgridH;  // Starting SG x index
        subGridY                         = (yStart >> 1) / subgridV;  // Starting SG y index
        pUnpackedField->lxStartL         = subGridX / scaleCubic;
        pUnpackedField->lxStartR         = subGridX / scaleCubic;
        pUnpackedField->lyStartL         = subGridY / scaleCubic;
        pUnpackedField->lyStartR         = subGridY / scaleCubic;
        pUnpackedField->bxStartL         = subGridX & (scaleCubic - 1);
        pUnpackedField->bxStartR         = subGridX & (scaleCubic - 1);
        pUnpackedField->byStartL         = subGridY & (scaleCubic - 1);
        pUnpackedField->byStartR         = subGridY & (scaleCubic - 1);
        pUnpackedField->bxD1L            = deltaH % subgridH;
        pUnpackedField->bxD1R            = deltaH % subgridH;
        pUnpackedField->byE1L            = deltaV % subgridV;
        pUnpackedField->byE1R            = deltaV % subgridV;
        pUnpackedField->byInitE1L        = pUnpackedField->byE1R * pUnpackedField->yDeltaL;
        pUnpackedField->byInitE1R        = pUnpackedField->byE1R * pUnpackedField->yDeltaL;
        pUnpackedField->bankSel          = pInput->LUTBankSel;
        pUnpackedField->bayerPattern     = pInput->bayerPattern;
        pUnpackedField->leftImgWd        = static_cast<UINT16>(static_cast<FLOAT>(streamInWidth) /2.0f);
        pUnpackedField->enable3D         = 0;
        pUnpackedField->HDRen            = 0;
        pUnpackedField->scaleBypass      = 0; // default
    }
    else
    {
        // @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pedestal13Setting::InterpGridOptimization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pedestal13Setting::InterpGridOptimization(
    INT32  rawWidth,
    INT32  rawHeight,
    INT32* pScaleCubic,
    INT32* pDeltaH,
    INT32* pDeltaV,
    INT32* pSubgridH,
    INT32* pSubgridV)
{
    INT32 level    = 3 + 1;           // Initial bicubic level level as 1 more than maximum
    INT32 nWidth   = rawWidth >> 1;   // per-channel image width
    INT32 nHeight  = rawHeight >> 1;  // per-channel image height
    INT32 numGridH = PED_MESH_H_V13;
    INT32 numGridV = PED_MESH_V_V13;
    INT32 sGridH;
    INT32 sGridV;
    INT32 gridH;
    INT32 gridV;
    INT32 deltaH;
    INT32 deltaV;

    // @todo (CAMX-1730) rewrite below snippet to be more readable
    do
    {
        level--;
        sGridH = (nWidth + numGridH - 1) / numGridH;     // Ceil
        sGridH = (sGridH + (1 << level) - 1) >> level;   // Ceil
        gridH  = sGridH << level;                        // Bayer grid width
        deltaH = gridH * numGridH - nWidth;              // two-side overhead
        sGridV = (nHeight + numGridV - 1) / numGridV;    // Ceil
        sGridV = (sGridV + (1 << level) - 1) >> level;   // Ceil
        gridV  = sGridV << level;                        // Bayer grid height
        deltaV = gridV * numGridV - nHeight;             // two-side overhead
    } while ((level > 0)                       &&
             ((gridH  <  18)                   ||
              (gridV  <  9)                    ||
              (sGridH <  9)                    ||
              (sGridV <  9)                    ||
              (deltaH >= gridH)                ||
              (deltaV >= gridV)                ||
              ((gridH - (deltaH + 1) / 2) < 18)||
              ((sGridH - (((deltaH + 1) / 2) % sGridH)) < 9)));

    *pScaleCubic = 1 << level;
    *pDeltaH     = (deltaH + 1) >> 1;
    *pDeltaV     = (deltaV + 1) >> 1;
    *pSubgridH   = sGridH;
    *pSubgridV   = sGridV;

    return;
}
