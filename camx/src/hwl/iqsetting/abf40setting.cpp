// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  abf40setting.cpp
/// @brief ABF40 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "abf40setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ABF40Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ABF40Setting::CalculateHWSetting(
    ABF40InputData*                                       pInput,
    abf_4_0_0::abf40_rgn_dataType*                        pData,
    const abf_4_0_0::chromatix_abf40_reserveType*         pReserveType,
    abf_4_0_0::chromatix_abf40Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pReserveType)  &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))

    {
        ABF40UnpackedField* pUnpackedField = static_cast<ABF40UnpackedField*>(pOutput);

        UINT32  i;
        UINT32  j;
        INT32   minVal;
        INT32   maxVal;
        INT16   dd1;
        INT16   bb1;
        INT16   sign;
        UINT32  noiseStandard0;
        UINT32  noiseStandard1;
        UINT32  diff;
        UINT32  maxR2;
        // Chromatix has 1 extra entry to help compute slope
        INT16  RNRAnchor[ABF40_NUM_ANCHORS + 1];
        INT16  base_table0[ABF40_NUM_ANCHORS + 1];
        INT16  base_table1[ABF40_NUM_ANCHORS + 1];
        INT16  nprsvAnchor[ABF40_NUM_ANCHORS + 1];
        INT16  nprsvBase[10];

        // bilateral_en for hyst is in dependenceData.moduleEnable. This is stored at CheckDependenceChange()
        UINT32 subgridHOffset = pInput->sensorOffsetX;
        UINT32 subgridVOffset = pInput->sensorOffsetY;

        pUnpackedField->LUTBankSel  = pInput->LUTBankSel;

        pUnpackedField->enable      = (pModuleEnable->bilateral_en & pInput->moduleEnable) |
                                      (pModuleEnable->dirsmth_en   & 1)             |
                                      (pModuleEnable->minmax_en    & 1);

        pUnpackedField->bilateralEnable    = pModuleEnable->bilateral_en  & pInput->moduleEnable;
        pUnpackedField->crossProcessEnable = pReserveType->cross_plane_en & 1;
        pUnpackedField->darkDesatEnable    = pReserveType->dark_desat_en  & pUnpackedField->bilateralEnable;
        pUnpackedField->darkSmoothEnable   = pReserveType->dark_smooth_en & pUnpackedField->bilateralEnable;
        pUnpackedField->directSmoothEnable = pModuleEnable->dirsmth_en    & 1;
        pUnpackedField->actAdjEnable       = static_cast<UINT16>(pReserveType->act_adj_en         &
                                                                 pUnpackedField->bilateralEnable  &
                                                                 pUnpackedField->directSmoothEnable);
        pUnpackedField->minmaxEnable       = pModuleEnable->minmax_en     & 1;

        pUnpackedField->blockOpt = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->blk_opt,
                                                                                   ABF40_BLK_OPT_MIN,
                                                                                   ABF40_BLK_OPT_MAX));
        for (i = 0; i < ABF40_NUM_BLOCK_PIX_LEVELS; i++)
        {
            pUnpackedField->blockPixLevel[i] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(
                                                                    pData->blkpix_lev_tab.blkpix_lev[i]),
                                                                    ABF40_BLK_PIX_LEVEL_MIN,
                                                                    ABF40_BLK_PIX_LEVEL_MAX));
        }

        for (i = 0; i < DMIRAM_ABF40_NOISESTD_LENGTH; i++)
        {
            noiseStandard0 = IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                            pData->noise_std_lut_tab.noise_std_lut[i], 2),
                                                            ABF40_NOISE_STD_LUT_MIN,
                                                            ABF40_NOISE_STD_LUT_MAX);
            noiseStandard1 = IQSettingUtils::ClampUINT32(
                IQSettingUtils::FloatToQNumber(pData->noise_std_lut_tab.noise_std_lut[i + 1], 2),
                ABF40_NOISE_STD_LUT_MIN,
                ABF40_NOISE_STD_LUT_MAX);

            diff = IQSettingUtils::ClampUINT32(noiseStandard1 - noiseStandard0,
                                               ABF40_NOISE_STD_LUT_MIN,
                                               ABF40_NOISE_STD_LUT_MAX);
            pUnpackedField->noiseStdLUT[pUnpackedField->LUTBankSel][i] = (diff * (1 << 9)) | noiseStandard0;
        }

        for (i = 0; i < 3; i++)
        {
            for (j = 0; j < 6; j++)
            {
                if (j == 0 || j == 3)
                {
                    minVal = ABF40_DISTANCE_KER_0_MIN;
                    maxVal = ABF40_DISTANCE_KER_0_MAX;
                }
                else if (j == 1 || j == 4)
                {
                    minVal = ABF40_DISTANCE_KER_1_MIN;
                    maxVal = ABF40_DISTANCE_KER_1_MAX;
                }
                else
                {
                    minVal = ABF40_DISTANCE_KER_2_MIN;
                    maxVal = ABF40_DISTANCE_KER_2_MAX;
                }
                pUnpackedField->distanceLevel[i][j] = static_cast<UINT16>(
                    IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->dist_ker_tab.dist_ker[(i * 6) + j]),
                    minVal,
                    maxVal));
            }
        }

        for (i = 0; i < ABF40_NUM_CHANNELS; i++)
        {
            pUnpackedField->curveOffset[i]    = static_cast<UINT16>(
                IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->curve_offset_tab.curve_offset[i]),
                ABF40_CURVE_OFFSET_MIN,
                ABF40_CURVE_OFFSET_MAX));

            pUnpackedField->edgeSoftness[i]   =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                    IQSettingUtils::FloatToQNumber(pData->edge_softness_tab.edge_softness[i], ABF40_EDGE_SOFTNESS_Q_FACTOR),
                    ABF40_EDGE_SOFTNESS_MIN,
                    ABF40_EDGE_SOFTNESS_MAX));

            pUnpackedField->filterStrength[i] = static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                IQSettingUtils::FloatToQNumber(pData->denoise_strength_tab.denoise_strength[i], ABF40_FILTER_STRENGTH_Q_FACTOR),
                ABF40_FILTER_STRENGTH_MIN,
                ABF40_FILTER_STRENGTH_MAX));
        }

        pUnpackedField->minmaxMaxShift =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->minmax_maxshft),
                                                            ABF40_MINMAX_SHFT_MIN,
                                                            ABF40_MINMAX_SHFT_MAX));

        pUnpackedField->minmaxMinShift =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->minmax_minshft),
                                                            ABF40_MINMAX_SHFT_MIN,
                                                            ABF40_MINMAX_SHFT_MAX));

        pUnpackedField->minmaxOffset  =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->minmax_offset),
                                                            ABF40_MINMAX_OFFSET_MIN,
                                                            ABF40_MINMAX_OFFSET_MAX));

        pUnpackedField->minmaxBLS     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->minmax_bls),
                                                            ABF40_MINMAX_BLS_MIN,
                                                            ABF40_MINMAX_BLS_MAX));

        INT32 difference = static_cast<INT32>(subgridHOffset) - static_cast<INT32>(pInput->sensorWidth >> 1);
        pUnpackedField->bx =
            static_cast<INT16>(IQSettingUtils::ClampINT32(difference,
                                                          ABF40_BX_MIN,
                                                          ABF40_BX_MAX));
        difference = static_cast<INT32>(subgridVOffset) - static_cast<INT32>(pInput->sensorHeight >> 1);
        pUnpackedField->by =
            static_cast<INT16>(IQSettingUtils::ClampINT32(difference,
                                                          ABF40_BY_MIN,
                                                          ABF40_BY_MAX));
        pUnpackedField->rSquareInit =
            IQSettingUtils::ClampUINT32(pUnpackedField->bx * pUnpackedField->bx + pUnpackedField->by * pUnpackedField->by,
                                        ABF40_R_SQUARE_INIT_MIN,
                                        ABF40_R_SQUARE_INIT_MAX);

        maxR2 = (pInput->sensorWidth * pInput->sensorWidth / 4) + (pInput->sensorHeight * pInput->sensorHeight / 4);

        for (i = 0; i < (ABF40_R_SQUARE_SHFT_MAX + 1); i++)
        {
            if ((maxR2 >> i) <= (ABF40_R_SQUARE_MAX_VAL))
            {
                pUnpackedField->rSquareShift = static_cast<UINT16>(i);
                break;
            }
        }
        pUnpackedField->rSquareShift =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->rSquareShift,
                                                            ABF40_R_SQUARE_SHFT_MIN,
                                                            ABF40_R_SQUARE_SHFT_MAX));
        pUnpackedField->rSquareScale =
            static_cast<UINT16>(4095 * 64 / (maxR2 >> pUnpackedField->rSquareShift));

        pUnpackedField->rSquareScale =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->rSquareScale,
                                                            ABF40_R_SQUARE_SCALE_MIN,
                                                            ABF40_R_SQUARE_SCALE_MAX));

        for (i = 0; i < (ABF40_NUM_ANCHORS + 1); i++)
        { /// adjust anchor since they are sqr of distance
            RNRAnchor[i] =
                static_cast<INT16>(pow(pReserveType->radial_anchor_tab.radial_anchor[i] /
                                        pReserveType->radial_anchor_tab.radial_anchor[4], 2.0f) * 4095);
        }
        for (i = 0; i < ABF40_NUM_ANCHORS; i++)
        {
            pUnpackedField->RNRAnchor[i] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(RNRAnchor[i],
                                                                ABF40_RNR_ANCHOR_MIN,
                                                                ABF40_RNR_ANCHOR_MAX));
        }

        for (i = 0; i < (ABF40_NUM_ANCHORS + 1); i++)
        {
            base_table0[i] =
                static_cast<INT16>(IQSettingUtils::FloatToQNumber(
                    pData->radial_edge_softness_adj_tab.radial_edge_softness_adj[i],
                    QNumber_8U));
        }
        for (i = 0; i < ABF40_NUM_ANCHORS; i++)
        {
            pUnpackedField->RNRBase0[i] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(base_table0[i],
                                                                ABF40_RNR_BASE0_MIN,
                                                                ABF40_RNR_BASE0_MAX));
            ///  validate base0 table is monotonically increasing.
        }
        for (i = 0; i < (ABF40_NUM_ANCHORS + 1); i++)
        {
            base_table1[i] =
                static_cast<INT16>(IQSettingUtils::FloatToQNumber(pData->radial_noise_prsv_adj_tab.radial_noise_prsv_adj[i],
                                                                   QNumber_8U));
        }
        for (i = 0; i < ABF40_NUM_ANCHORS; i++)
        {
            pUnpackedField->RNRBase1[i] =
                static_cast<INT16>(IQSettingUtils::ClampINT32(base_table1[i],
                                                                ABF40_RNR_BASE1_MIN,
                                                                ABF40_RNR_BASE1_MAX));
        }
        for (i = 0; i < ABF40_NUM_ANCHORS; i++)
        {
            dd1 = static_cast<INT16>(IQSettingUtils::ClampINT32(RNRAnchor[i + 1] - RNRAnchor[i],
                                                                 1,
                                                                 (1 << 12) - 1));
            bb1 = static_cast<INT16>(IQSettingUtils::ClampINT32(base_table0[i + 1] - base_table0[i],
                                                                 0,
                                                                 4095));
            if (bb1 == 0)
            {
                pUnpackedField->RNRShift0[i] = 0;
            }
            else
            {
                pUnpackedField->RNRShift0[i] =
                    static_cast<UINT16>((log(static_cast<DOUBLE>(dd1)) +
                    log(4095.0) - log(static_cast<DOUBLE>(bb1))) / log(2.0));
            }

            pUnpackedField->RNRShift0[i]     =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->RNRShift0[i],
                                                                ABF40_RNR_SHIFT_MIN,
                                                                ABF40_RNR_SHIFT_MAX));

            pUnpackedField->RNRSlope0[i] =
                static_cast<UINT16>(IQSettingUtils::ClampINT32((bb1 << (pUnpackedField->RNRShift0[i])) / dd1,
                                                                ABF40_RNR_SLOPE0_MIN,
                                                                ABF40_RNR_SLOPE0_MAX));

            bb1  = static_cast<INT16>(IQSettingUtils::ClampINT32(base_table1[i + 1] - base_table1[i],
                                                                 -256,
                                                                  256));
            if (bb1 < 0)
            {
                sign = -1;
            }
            else
            {
                sign = 1;
            }
            bb1  = static_cast<INT16>(IQSettingUtils::AbsoluteINT(bb1));
            if (bb1 == 0)
            {
                pUnpackedField->RNRShift1[i] = 0;
            }
            else
            {
                pUnpackedField->RNRShift1[i] =
                    static_cast<INT16>((log(static_cast<DOUBLE>(dd1)) + log(255.0) - log(static_cast<DOUBLE>(bb1))) / log(2.0));
            }
            pUnpackedField->RNRShift1[i] =
                static_cast<INT16>(IQSettingUtils::ClampINT32(pUnpackedField->RNRShift1[i],
                                                                ABF40_RNR_SHIFT_MIN,
                                                                ABF40_RNR_SHIFT_MAX));
            pUnpackedField->RNRSlope1[i] = static_cast<INT16>((bb1 << (pUnpackedField->RNRShift1[i])) / dd1);
            pUnpackedField->RNRSlope1[i] =
                static_cast<INT16>(IQSettingUtils::ClampINT32(
                (pUnpackedField->RNRSlope1[i] * sign),
                    ABF40_RNR_SLOPE1_MIN,
                    ABF40_RNR_SLOPE1_MAX));
        }

        for (i = 0; i < (ABF40_NUM_ANCHORS + 1); i++)
        {
            nprsvAnchor[i]   =
                static_cast<INT16>(IQSettingUtils::ClampINT32(
                    IQSettingUtils::RoundFLOAT((pData->noise_prsv_anchor_tab.noise_prsv_anchor[i] * 4095)),
                    ABF40_NPRSV_ANCHOR_MIN,
                    ABF40_NPRSV_ANCHOR_MAX));
            nprsvBase[i]     =
                static_cast<INT16>(IQSettingUtils::ClampINT32(
                    IQSettingUtils::FloatToQNumber(pData->noise_prsv_base_tab.noise_prsv_base[i], QNumber_8U),
                    ABF40_NPRSV_BASE_MIN,
                    ABF40_NPRSV_BASE_MAX));
            nprsvBase[5 + i] =
                static_cast<INT16>(IQSettingUtils::ClampINT32(
                    IQSettingUtils::FloatToQNumber(pData->noise_prsv_base_tab.noise_prsv_base[i + 5], QNumber_8U),
                    ABF40_NPRSV_BASE_MIN,
                    ABF40_NPRSV_BASE_MAX));
        }
        nprsvAnchor[0] = 0;
        nprsvAnchor[4] = 4095;

        for (i = 0; i < ABF40_NUM_ANCHORS; i++)
        {
            pUnpackedField->nprsvAnchor[i]  = nprsvAnchor[i];
            pUnpackedField->nprsvBase[0][i] = nprsvBase[i];

            dd1 = static_cast<INT16>(IQSettingUtils::ClampINT32(nprsvAnchor[i + 1] - nprsvAnchor[i],
                                                                1,
                                                                (1 << 12) - 1));
            bb1 = static_cast<INT16>(IQSettingUtils::ClampINT32(nprsvBase[i + 1] - nprsvBase[i],
                                                               -256,
                                                                256));
            if (bb1 < 0)
            {
                sign = -1;
            }
            else
            {
                sign = 1;
            }
            bb1 = static_cast<INT16>(IQSettingUtils::AbsoluteINT(bb1));
            if (bb1 == 0)
            {
                pUnpackedField->nprsvShift[0][i] = 0;
            }
            else
            {
                pUnpackedField->nprsvShift[0][i] =
                    static_cast<UINT16>((log(static_cast<DOUBLE>(dd1)) +
                    log(255.0) - log(static_cast<DOUBLE>(bb1))) / log(2.0));
            }
            pUnpackedField->nprsvShift[0][i] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->nprsvShift[0][i],
                                                                ABF40_NPRSV_SHFT_MIN,
                                                                ABF40_NPRSV_SHFT_MAX));
            pUnpackedField->nprsvSlope[0][i] = static_cast<INT16>((bb1 << (pUnpackedField->nprsvShift[0][i])) / dd1);
            pUnpackedField->nprsvSlope[0][i] =
                static_cast<INT16>(IQSettingUtils::ClampINT32(
                    (pUnpackedField->nprsvSlope[0][i] * sign),
                    ABF40_NPRSV_SLOPE_MIN,
                    ABF40_NPRSV_SLOPE_MAX));

            pUnpackedField->nprsvBase[1][i] = nprsvBase[5 + i];

            dd1 = static_cast<UINT16>(IQSettingUtils::ClampINT32(nprsvAnchor[i + 1] - nprsvAnchor[i],
                                                                 1,
                                                                (1 << 12) - 1));
            bb1 = static_cast<UINT16>(IQSettingUtils::ClampINT32(nprsvBase[i + 6] - nprsvBase[5 + i],
                                                                -256,
                                                                 256));
            if (bb1 < 0)
            {
                sign = -1;
            }
            else
            {
                sign = 1;
            }
            bb1 = static_cast<INT16>(IQSettingUtils::AbsoluteINT(bb1));
            if (bb1 == 0)
            {
                pUnpackedField->nprsvShift[1][i] = 0;
            }
            else
            {
                pUnpackedField->nprsvShift[1][i] =
                    static_cast<UINT16>((log(static_cast<DOUBLE>(dd1)) +
                    log(255.0) - log(static_cast<DOUBLE>(bb1))) / log(2.0));
            }
            pUnpackedField->nprsvShift[1][i] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->nprsvShift[1][i],
                                                                ABF40_NPRSV_SHFT_MIN,
                                                                ABF40_NPRSV_SHFT_MAX));

            pUnpackedField->nprsvSlope[1][i] = static_cast<INT16>((bb1 << (pUnpackedField->nprsvShift[1][i])) / dd1);

            pUnpackedField->nprsvSlope[1][i] =
                static_cast<INT16>(IQSettingUtils::ClampINT32(
                    (pUnpackedField->nprsvSlope[1][i] * sign),
                    ABF40_NPRSV_SLOPE_MIN,
                    ABF40_NPRSV_SLOPE_MAX));
        }

        for (i = 0; i < DMIRAM_ABF40_ACTIVITY_LENGTH; i++)
        {
            pUnpackedField->activityFactorLUT[pUnpackedField->LUTBankSel][i] = static_cast<UINT16>(
                IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->act_fac_lut_tab.act_fac_lut[i]),
                                            ABF40_ACT_FAC_LUT_MIN,
                                            ABF40_ACT_FAC_LUT_MAX));
        }

        pUnpackedField->activityFactor0 =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->act_fac0),
                                                            ABF40_ACT_FAC0_MIN,
                                                            ABF40_ACT_FAC0_MAX));

        pUnpackedField->activityFactor1 =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->act_fac1),
                                                            ABF40_ACT_FAC1_MIN,
                                                            ABF40_ACT_FAC1_MAX));

        pUnpackedField->activityThreshold0 =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->act_thd0),
                                                            ABF40_ACT_THD0_MIN,
                                                            ABF40_ACT_THD0_MAX));
        pUnpackedField->activityThreshold1 =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->act_thd1),
                                                            ABF40_ACT_THD1_MIN,
                                                            ABF40_ACT_THD1_MAX));

        pUnpackedField->activitySmoothThreshold0 =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->act_smth_thd0),
                                                            ABF40_ACT_SMTH_THD_MIN,
                                                            ABF40_ACT_SMTH_THD_MAX));
        pUnpackedField->activitySmoothThreshold1 =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->act_smth_thd1),
                                                            ABF40_ACT_SMTH_THD_MIN,
                                                            ABF40_ACT_SMTH_THD_MAX));
        pUnpackedField->darkThreshold =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->dark_thd),
                                                            ABF40_DARK_THD_MIN,
                                                            ABF40_DARK_THD_MAX));
        for (i = 0; i < DMIRAM_ABF40_DARK_LENGTH; i++)
        {
            pUnpackedField->darkFactorLUT[0][i] = static_cast<UINT16>(
                IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->dark_fac_lut_tab.dark_fac_lut[i]),
                                            ABF40_DARK_FAC_LUT_MIN,
                                            ABF40_DARK_FAC_LUT_MAX));
            pUnpackedField->darkFactorLUT[1][i] = pUnpackedField->darkFactorLUT[0][i];
        }

        pUnpackedField->grRatio = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pInput->leftGGainWB * 256.0f / pInput->leftRGainWB),
                                        ABF40_RGB_GAIN_RATIO_MIN,
                                        ABF40_RGB_GAIN_RATIO_MAX));
        pUnpackedField->rgRatio = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pInput->leftRGainWB * 256.0f / pInput->leftGGainWB),
                                        ABF40_RGB_GAIN_RATIO_MIN,
                                        ABF40_RGB_GAIN_RATIO_MAX));
        pUnpackedField->gbRatio = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pInput->leftGGainWB * 256.0f / pInput->leftBGainWB),
                                        ABF40_RGB_GAIN_RATIO_MIN,
                                        ABF40_RGB_GAIN_RATIO_MAX));
        pUnpackedField->bgRatio = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pInput->leftBGainWB * 256.0f / pInput->leftGGainWB),
                                        ABF40_RGB_GAIN_RATIO_MIN,
                                        ABF40_RGB_GAIN_RATIO_MAX));
        pUnpackedField->rbRatio = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pInput->leftRGainWB * 256.0f / pInput->leftBGainWB),
                                        ABF40_RGB_GAIN_RATIO_MIN,
                                        ABF40_RGB_GAIN_RATIO_MAX));
        pUnpackedField->brRatio = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pInput->leftBGainWB * 256.0f / pInput->leftRGainWB),
                                        ABF40_RGB_GAIN_RATIO_MIN,
                                        ABF40_RGB_GAIN_RATIO_MAX));

        pUnpackedField->edgeDetectThreshold = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->edge_detect_thd),
                                        ABF40_EDGE_DETECT_THD_MIN,
                                        ABF40_EDGE_DETECT_THD_MAX));
        pUnpackedField->edgeCountLow  = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->edge_count_low),
                                        ABF40_EDGE_COUNT_LOW_MIN,
                                        ABF40_EDGE_COUNT_LOW_MAX));

        pUnpackedField->edgeDetectNoiseScalar = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(pData->edge_detect_noise_scaler, QNumber_6U),
                                        ABF40_EDGE_DETECT_NOISE_SCALER_MIN,
                                        ABF40_EDGE_DETECT_NOISE_SCALER_MAX));
        pUnpackedField->edgeSmoothStrength = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(pData->edge_smooth_strength, QNumber_6U),
                                        ABF40_EDGE_SMOOTH_STRENGTH_MIN,
                                        ABF40_EDGE_SMOOTH_STRENGTH_MAX));
        for (i = 0; i < ABF40_NUM_CHANNELS; i++)
        {
            pUnpackedField->edgeSmoothNoiseScalar[i] = static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                IQSettingUtils::FloatToQNumber(pData->edge_smooth_scaler_tab.edge_smooth_scaler[i], QNumber_8U),
                ABF40_EDGE_SMOOTH_NOISE_SCALAR_MIN,
                ABF40_EDGE_SMOOTH_NOISE_SCALAR_MAX));
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }
    return result;
}
