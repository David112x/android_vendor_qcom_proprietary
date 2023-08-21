// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  bpsgic30setting.cpp
/// @brief BPS GIC30 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bpsgic30setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSGIC30Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSGIC30Setting::CalculateHWSetting(
    const GIC30InputData*                                 pInput,
    gic_3_0_0::gic30_rgn_dataType*                        pData,
    gic_3_0_0::chromatix_gic30_reserveType*               pReserveType,
    gic_3_0_0::chromatix_gic30Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pOutput)       &&
        (NULL != pModuleEnable) &&
        (NULL != pReserveType))
    {
        UINT32  i;
        UINT16  noiseStdDifference;
        FLOAT   noiseStdVal0;
        FLOAT   noiseStdVal1;

        GIC30UnpackedField* pUnpackedField = static_cast<GIC30UnpackedField*>(pOutput);

        pUnpackedField->enable     = static_cast<UINT16>(pModuleEnable->gic_global_enable);
        pUnpackedField->enableGIC  = static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                                                             IQSettingUtils::RoundFLOAT(pData->enable_gic), 0, 1));
        pUnpackedField->enable3D   = 0;
        pUnpackedField->LUTBankSel = pInput->LUTBankSel;
        pUnpackedField->enablePNR  = static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                                                             IQSettingUtils::RoundFLOAT(pData->enable_pnr), 0, 1));

        for (i = 0; i < DMIRAM_GIC_NOISESTD_LENGTH_V30; i++)
        {
            noiseStdVal0 = IQSettingUtils::ClampFLOAT(pData->noise_std_lut_tab.noise_std_lut[i],
                                                      static_cast<FLOAT>(GIC30_NOISE_STD_LUT_LEVEL0_MIN),
                                                      static_cast<FLOAT>(GIC30_NOISE_STD_LUT_LEVEL0_PRE_MAX - 1));

            if (i < DMIRAM_GIC_NOISESTD_LENGTH_V30 - 1)
            {
                noiseStdVal1 = IQSettingUtils::ClampFLOAT(pData->noise_std_lut_tab.noise_std_lut[i + 1],
                                                          static_cast<FLOAT>(GIC30_NOISE_STD_LUT_LEVEL0_MIN),
                                                          static_cast<FLOAT>(GIC30_NOISE_STD_LUT_LEVEL0_PRE_MAX - 1));
            }
            else
            {
                noiseStdVal1 = IQSettingUtils::ClampFLOAT(pData->noise_std_lut_tab.noise_std_lut[i + 1],
                                                          static_cast<FLOAT>(GIC30_NOISE_STD_LUT_LEVEL0_MIN),
                                                          static_cast<FLOAT>(GIC30_NOISE_STD_LUT_LEVEL0_PRE_MAX));
            }

            noiseStdVal0 = static_cast<FLOAT>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(noiseStdVal0 * 4),
                                                                          GIC30_NOISE_STD_LUT_LEVEL0_MIN,
                                                                          GIC30_NOISE_STD_LUT_LEVEL0_MAX));
            noiseStdVal1 = static_cast<FLOAT>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(noiseStdVal1 * 4),
                                                                          GIC30_NOISE_STD_LUT_LEVEL0_MIN,
                                                                          GIC30_NOISE_STD_LUT_LEVEL0_MAX));

            noiseStdDifference = static_cast<INT16>(IQSettingUtils::ClampINT32(static_cast<INT32>(noiseStdVal1 - noiseStdVal0),
                                                                               GIC30_NOISE_STD_LUT_LEVEL0_DELTA_MIN,
                                                                               GIC30_NOISE_STD_LUT_LEVEL0_DELTA_MAX));

            pUnpackedField->noiseStdLUTLevel0[pUnpackedField->LUTBankSel][i] =
                static_cast<UINT32>((noiseStdDifference * GIC30_NOISE_STD_LUT_LEVEL0_SHIFT) |
                static_cast<UINT32>(noiseStdVal0));
        }

        pUnpackedField->GICNoiseScale       =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                                    IQSettingUtils::FloatToQNumber(pData->gic_noise_scale,
                                                                   GIC30_GICNoiseScale_Q_FACTOR),
                                                               GIC30_GICNoiseScale_MIN,
                                                               GIC30_GICNoiseScale_MAX));
        pUnpackedField->thinLineNoiseOffset =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->thin_line_noise_offset),
                                                                                       GIC30_thinLineNoiseOffset_MIN,
                                                                                       GIC30_thinLineNoiseOffset_MAX));
        pUnpackedField->GICStrength         =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(pData->gic_correction_strength,
                                                                GIC30_GICCorrectionStrength_Q_FACTOR),
                                                            GIC30_GICCorrectionStrength_MIN,
                                                            GIC30_GICCorrectionStrength_MAX));

        UINT32 width          = pInput->imageWidth;
        UINT32 height         = pInput->imageHeight;
        UINT32 subgridHOffset = pInput->sensorOffsetX;
        UINT32 subgridVOffset = pInput->sensorOffsetY;

        pUnpackedField->bx = static_cast<INT16>(IQSettingUtils::ClampINT32(static_cast<INT32>(subgridHOffset) -
                                                                                static_cast<INT32>(width / 2),
                                                                           GIC30_BX_MIN,
                                                                           GIC30_BX_MAX));
        pUnpackedField->by = static_cast<INT16>(IQSettingUtils::ClampINT32(static_cast<INT32>(subgridVOffset) -
                                                                                static_cast<INT32>(height / 2),
                                                                           GIC30_BY_MIN,
                                                                           GIC30_BY_MAX));

        pUnpackedField->rSquareInit = IQSettingUtils::ClampUINT32(pUnpackedField->bx * pUnpackedField->bx +
                                                                        pUnpackedField->by * pUnpackedField->by,
                                                                    GIC30_R_SQUARE_INIT_MIN,
                                                                    GIC30_R_SQUARE_INIT_MAX);

        UINT32 maxR2 = (width * width / 4) + (height * height / 4);
        for (i = 0; i < (GIC30_R_SQUARE_SHFT_MAX + 1); i++)
        {
            if ((maxR2 >> i) <= (GIC30_MAX_R_SQUARE_VAL))
            {
                pUnpackedField->rSquareShift = static_cast<UINT16>(i);
                break;
            }
        }
        pUnpackedField->rSquareShift = static_cast<UINT16>(IQSettingUtils::ClampINT32(pUnpackedField->rSquareShift,
                                                                                      GIC30_R_SQUARE_SHFT_MIN,
                                                                                      GIC30_R_SQUARE_SHFT_MAX));
        // rSquareInit>>r_square_shft range: 4096-8191 FIXME: maxR2<> rSquareInit, code inconsistent with comments
        pUnpackedField->rSquareScale = static_cast<UINT16>(4095 * 64 / (maxR2 >> pUnpackedField->rSquareShift));
        pUnpackedField->rSquareScale = static_cast<UINT16>(IQSettingUtils::ClampINT32(pUnpackedField->rSquareScale,
                                                                                      GIC30_R_SQUARE_SCALE_MIN,
                                                                                      GIC30_R_SQUARE_SCALE_MAX));

        // FIXME: make sure the following calculation of RNRAnchor is calculated correctly
        UINT16 RNRAnchor[NumAnchorBase + 1];     // 10u
        for (i = 0; i < (NumAnchorBase + 1); i++)
        { // adjust anchor since they are sqr of distance
            RNRAnchor[i] = static_cast<UINT16>(IQSettingUtils::RoundDOUBLE(
                                                    pow(pReserveType->radial_anchor_tab.radial_anchor[i] /
                                                        pReserveType->radial_anchor_tab.radial_anchor[NumAnchorBase],
                                                        2.0f) * 4095));
        }
        for (i = 0; i < NumAnchorBase; i++)
        {
            pUnpackedField->RNRAnchor[i] = static_cast<UINT16>(IQSettingUtils::ClampINT32(RNRAnchor[i],
                                                                                          GIC30_RNR_ANCHOR_MIN,
                                                                                          GIC30_RNR_ANCHOR_MAX));
        }

        // FIXME: make sure the following calculation of RNRBase is calculated correctly
        UINT16 RNRBase[NumAnchorBase + 1];       // 10u, 6 anchors
        FLOAT  RNRDeltaRatio;
        for (i = 0; i < (NumAnchorBase + 1); i++)
        {
            RNRDeltaRatio = IQSettingUtils::ClampFLOAT(pData->radial_pnr_str_adj_tab.radial_pnr_str_adj[i],
                                                       GIC30_RNR_DELTA_RATIO_MIN,
                                                       GIC30_RNR_DELTA_RATIO_MAX);
            RNRBase[i]    = static_cast<UINT16>(IQSettingUtils::RoundDOUBLE((1.0 + pow(static_cast<FLOAT>(i) / 4.0f, 2) *
                                                                                RNRDeltaRatio) * 256));
        }
        for (i = 0; i < NumAnchorBase; i++)
        {
            pUnpackedField->RNRBase[i] = static_cast<UINT16>(IQSettingUtils::ClampUINT32(RNRBase[i],
                                                                                          GIC30_RNR_BASE_MIN,
                                                                                          GIC30_RNR_BASE_MAX));
        }

        // FIXME: make sure the following calculation of RNRSlope & RNRShift is calculated correctly <--Updated
        INT16  RNRSlope[NumSlopeShift];      // 11s
        UINT16 RNRShift[NumSlopeShift];      // 4u
        FLOAT  diffAnchor;
        FLOAT  diffBase;
        for (i = 0; i < NumSlopeShift; i++)
        {
            diffAnchor = static_cast<FLOAT>(IQSettingUtils::ClampINT32(RNRAnchor[i + 1] - RNRAnchor[i],
                                                                       1,
                                                                       GIC30_RNR_ANCHOR_MAX));
            diffBase   = static_cast<FLOAT>(IQSettingUtils::ClampINT32(RNRBase[i + 1] - RNRBase[i],
                                                                       -GIC30_RNR_BASE_MAX,
                                                                       GIC30_RNR_BASE_MAX));
            INT sign = 1;
            if (diffBase < 0)
            {
                sign = -1;
            }
            diffBase     = IQSettingUtils::AbsoluteFLOAT(diffBase);
            if (diffBase == 0)
            {
                RNRShift[i] = 0;
            }
            else
            {
                RNRShift[i] = static_cast<UINT16>(((log(diffAnchor) +
                                                     log(static_cast<FLOAT>(GIC30_RNR_ANCHOR_MAX)) - log(diffBase)) /
                                                     log(2.0f)));
            }
            pUnpackedField->RNRShift[i] = static_cast<UINT16>(IQSettingUtils::ClampINT32(RNRShift[i],
                                                                                         GIC30_RNR_SHIFT_MIN,
                                                                                         GIC30_RNR_SHIFT_MAX));
            RNRSlope[i]                 = static_cast<INT16>((diffBase * (1 << pUnpackedField->RNRShift[i])) / diffAnchor);
            pUnpackedField->RNRSlope[i] = static_cast<INT16>(
                                                IQSettingUtils::ClampINT32((RNRSlope[i] * sign),
                                                                           GIC30_RNR_SLOPE_MIN,
                                                                           GIC30_RNR_SLOPE_MAX));
        }

        for (i = 0; i < NumNoiseScale; i++)
        {
            pUnpackedField->PNRNoiseScale[i] =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(
                                                                    pData->pnr_noise_scale_tab.pnr_noise_scale[i],
                                                                    GIC30_PNR_NoiseScale_Q_FACTOR),
                                                               GIC30_PNR_NoiseScale_MIN,
                                                               GIC30_PNR_NoiseScale_MAX));
        }

        pUnpackedField->PNRStrength =
            static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(
                                                                pData->pnr_correction_strength,
                                                                GIC30_PNR_CorrectionStrength_Q_FACTOR),
                                                           GIC30_PNR_CorrectionStrength_MIN,
                                                           GIC30_PNR_CorrectionStrength_MAX));
    }
    else
    {
        /// @todo (CAMX-1460) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}
