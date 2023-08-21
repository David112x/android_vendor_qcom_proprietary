// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  hnr10setting.cpp
/// @brief BPS HNR10 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "hnr10setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HNR10Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HNR10Setting::CalculateHWSetting(
    const HNR10InputData*                                 pInput,
    hnr_1_0_0::hnr10_rgn_dataType*                        pData,
    hnr_1_0_0::chromatix_hnr10_reserveType*               pReserveType,
    hnr_1_0_0::chromatix_hnr10Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    BOOL   result  = TRUE;
    INT32  sensorOffsetX = 0;
    INT32  sensorOffsetY = 0;
    INT32  sWidth  = 640;
    INT32  sHeight = 480;
    INT    index;
    INT16  faceRadius[MAX_FACE_NUM];
    UINT16 LNRGainLSB;
    UINT16 LNRGainMSB;
    UINT16 FNRAcThLSB;
    UINT16 FNRAcThMSB;
    UINT16 blendLnrGainLSB;
    UINT16 blendLnrGainMSB;
    UINT32 FNRGainArrLSB;
    UINT32 FNRGainArrMSB;
    UINT32 FNRGainClampArrLSB;
    UINT32 FNRGainClampArrMSB;
    FLOAT  tempRNRThreshold;
    INT32  tempSNRSlope;
    INT    exp;
    INT    count;
    UINT16 rSquareShift;
    FLOAT  fractional;
    INT    rSquareMax;
    UINT16 anchorDifference;
    INT16  baseDifference;
    INT16  RNRThreshold[HNR_V10_RNR_ARR_NUM + 1];
    INT16  RNRGain[HNR_V10_RNR_ARR_NUM + 1];
    FLOAT  rBoundary;
    FLOAT  rTransition;
    FLOAT  slyMin;
    FLOAT  slyMax;
    FLOAT  shyMin;
    FLOAT  shyMax;
    FLOAT  qSkin;
    INT    pseudo_mantissa;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pReserveType)  &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        HNR10UnpackedField* pUnpackedField = static_cast<HNR10UnpackedField*>(pOutput);
        sensorOffsetX      = pInput->horizontalOffset;
        sensorOffsetY      = pInput->verticalOffset;
        sWidth             = pInput->streamWidth;
        sHeight            = pInput->streamHeight;

        pUnpackedField->enable = static_cast<UINT16>(pModuleEnable->hnr_nr_enable);

        pUnpackedField->lnr_en = static_cast<UINT16>(pReserveType->lnr_en);
        pUnpackedField->rnr_en = static_cast<UINT16>(pReserveType->rnr_en);
        pUnpackedField->cnr_en = static_cast<UINT16>(pReserveType->cnr_en);
        pUnpackedField->snr_en = static_cast<UINT16>(pReserveType->snr_en);
        pUnpackedField->fnr_en = static_cast<UINT16>(pReserveType->fnr_en);

        pUnpackedField->blend_enable = static_cast<UINT16>(pModuleEnable->hnr_blend_enable);

        pUnpackedField->blend_cnr_en = (pReserveType->blend_cnr_en & pUnpackedField->blend_enable);
        pUnpackedField->blend_snr_en = (pReserveType->blend_snr_en & pUnpackedField->blend_enable);
        pUnpackedField->fd_snr_en    = (pReserveType->fd_snr_en  & pUnpackedField->snr_en);
        pUnpackedField->lpf3_en      = static_cast<UINT16>(pReserveType->lpf3_en);

        pUnpackedField->lut_bank_sel = pInput->LUTBankSel;

        for (index = 0; index < HNR_V10_LNR_ARR_NUM; index++)
        {
            LNRGainLSB = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->lnr_gain_arr_tab.lnr_gain_arr[index]),
                                           HNR10_LNR_GAIN_LUT_MIN,
                                           HNR10_LNR_GAIN_LUT_MAX));
            if (index < HNR_V10_LNR_ARR_NUM - 1)
            {
                LNRGainMSB = static_cast<UINT16>(
                    IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->lnr_gain_arr_tab.lnr_gain_arr[index + 1]),
                                               HNR10_LNR_GAIN_LUT_MIN,
                                               HNR10_LNR_GAIN_LUT_MAX));
            }
            else
            {
                LNRGainMSB = static_cast<UINT16>(
                    IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->lnr_gain_arr_tab.lnr_gain_arr[index]),
                                               HNR10_LNR_GAIN_LUT_MIN,
                                               HNR10_LNR_GAIN_LUT_MAX));
            }
            pUnpackedField->lnr_gain_arr[pUnpackedField->lut_bank_sel][index] =
                static_cast<UINT16>((LNRGainMSB << 6) | LNRGainLSB);
        }

        pUnpackedField->lnr_shift = static_cast<UINT16>(
            IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->lnr_shift),
                                       HNR10_LNR_SHIFT_MIN,
                                       HNR10_LNR_SHIFT_MAX));

        for (index = 0; index < HNR_V10_CNR_ARR_NUM; index++)
        {
            pUnpackedField->cnr_gain_arr[index] = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->cnr_gain_arr_tab.cnr_gain_arr[index]),
                                           HNR10_CNR_GAIN_LUT_MIN,
                                           HNR10_CNR_GAIN_LUT_MAX));
        }

        pUnpackedField->cnr_low_thrd_u = static_cast<INT16>(
            IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->cnr_low_thrd_u),
                                       HNR10_CNR_LOW_THRD_U_MIN,
                                       HNR10_CNR_LOW_THRD_U_MAX));

        pUnpackedField->cnr_thrd_gap_u = static_cast<UINT16>(
            IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->cnr_low_gap_u),
                                       HNR10_CNR_THRD_GAP_U_MIN,
                                       HNR10_CNR_THRD_GAP_U_MAX));

        pUnpackedField->cnr_low_thrd_v = static_cast<INT16>(
            IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->cnr_low_thrd_v),
                                       HNR10_CNR_LOW_THRD_V_MIN,
                                       HNR10_CNR_LOW_THRD_V_MAX));

        pUnpackedField->cnr_thrd_gap_v = static_cast<UINT16>(
            IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->cnr_low_gap_v),
                                       HNR10_CNR_THRD_GAP_V_MIN,
                                       HNR10_CNR_THRD_GAP_V_MAX));

        pUnpackedField->cnr_adj_gain   = static_cast<UINT16>(
            IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->cnr_adj_gain),
                                       HNR10_CNR_ADJ_GAIN_MIN,
                                       HNR10_CNR_ADJ_GAIN_MAX));

        pUnpackedField->cnr_scale      = static_cast<UINT16>(
            IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->cnr_scale),
                                       HNR10_CNR_SCALE_MIN,
                                       HNR10_CNR_SCALE_MAX));

        for (index = 0; index < HNR_V10_FNR_ARR_NUM; index++)
        {
            FNRAcThLSB = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->fnr_ac_th_tab.fnr_ac_th[index]),
                                           HNR10_FNR_ACTHR_LUT_MIN,
                                           HNR10_FNR_ACTHR_LUT_MAX));
            if (index < HNR_V10_FNR_ARR_NUM - 1)
            {
                FNRAcThMSB = static_cast<UINT16>(
                    IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->fnr_ac_th_tab.fnr_ac_th[index + 1]),
                                               HNR10_FNR_ACTHR_LUT_MIN,
                                               HNR10_FNR_ACTHR_LUT_MAX));
            }
            else
            {
                FNRAcThMSB = static_cast<UINT16>(
                    IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->fnr_ac_th_tab.fnr_ac_th[index]),
                                               HNR10_FNR_ACTHR_LUT_MIN,
                                               HNR10_FNR_ACTHR_LUT_MAX));
            }
            pUnpackedField->fnr_ac_th_arr[pUnpackedField->lut_bank_sel][index] = (FNRAcThMSB << 6) | FNRAcThLSB;
        }

        for (index = 0; index < HNR_V10_FNR_ARR_NUM; index++)
        {
            FNRGainArrLSB =
                IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->fnr_gain_arr_tab.fnr_gain_arr[index]),
                                            HNR10_FNR_GAINARR_MIN,
                                            HNR10_FNR_GAINARR_MAX);
            FNRGainClampArrLSB =
                IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->fnr_gain_clamp_arr_tab.fnr_gain_clamp_arr[index]),
                                            HNR10_FNR_GAINCLAMPARR_MIN,
                                            HNR10_FNR_GAINCLAMPARR_MAX);

            if (index < HNR_V10_FNR_ARR_NUM - 1)
            {
                FNRGainArrMSB =
                    IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->fnr_gain_arr_tab.fnr_gain_arr[index + 1]),
                                                HNR10_FNR_GAINARR_MIN,
                                                HNR10_FNR_GAINARR_MAX);
                FNRGainClampArrMSB =
                    IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(
                                                pData->fnr_gain_clamp_arr_tab.fnr_gain_clamp_arr[index + 1]),
                                                HNR10_FNR_GAINCLAMPARR_MIN,
                                                HNR10_FNR_GAINCLAMPARR_MAX);
            }
            else
            {
                FNRGainArrMSB =
                    IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->fnr_gain_arr_tab.fnr_gain_arr[index]),
                                                HNR10_FNR_GAINARR_MIN,
                                                HNR10_FNR_GAINARR_MAX);
                FNRGainClampArrMSB =
                    IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->fnr_gain_clamp_arr_tab.
                                                                               fnr_gain_clamp_arr[index]),
                                                HNR10_FNR_GAINCLAMPARR_MIN,
                                                HNR10_FNR_GAINCLAMPARR_MAX);
            }
            pUnpackedField->merged_fnr_gain_arr_gain_clamp_arr[pUnpackedField->lut_bank_sel][index] =
                (((FNRGainArrMSB << 6) | FNRGainArrLSB) << FNR_GAIN_SHIFT_IN_ARR) |
                ((FNRGainClampArrMSB << 8) | FNRGainClampArrLSB);
        }

        pUnpackedField->fnr_ac_shift = static_cast<UINT16>(
            IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->fnr_ac_shift),
                                       HNR10_FNR_AC_SHFT_MIN,
                                       HNR10_FNR_AC_SHFT_MAX));

        pUnpackedField->abs_amp_shift = static_cast<UINT16>(
            IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->abs_amp_shift),
                                       HNR10_ABS_AMP_SHFT_MIN,
                                       HNR10_ABS_AMP_SHFT_MAX));

        for (index = 0; index < HNR_V10_NR_ARR_NUM; index++)
        {
            pUnpackedField->filtering_nr_gain_arr[index] = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(
                                           pData->filtering_nr_gain_arr_tab.filtering_nr_gain_arr[index]),
                                           HNR10_FNR_GAINARR_LUT_MIN,
                                           HNR10_FNR_GAINARR_LUT_MAX));
        }

        pUnpackedField->face_horizontal_offset = 0;
        pUnpackedField->face_vertical_offset   = 0;
        pUnpackedField->face_num               = 0;

        if (NULL != pInput->pFDData)
        {
            pUnpackedField->face_num = static_cast<UINT16>(pInput->pFDData->numberOfFace);
            for (index = 0; index < pUnpackedField->face_num; index++)
            {
                faceRadius[index]                             = static_cast<INT16>(pInput->pFDData->faceRadius[index]);
                pUnpackedField->face_center_horizontal[index] = static_cast<UINT16>(pInput->pFDData->faceCenterX[index]);
                pUnpackedField->face_center_vertical[index]   = static_cast<UINT16>(pInput->pFDData->faceCenterY[index]);
            }
        }

        pData->face_boundary   = IQSettingUtils::ClampFLOAT(pData->face_boundary,
                                                            HNR10_MIN_FACE_BOUNDARY,
                                                            HNR10_MAX_FACE_BOUNDARY);
        pData->face_transition = IQSettingUtils::ClampFLOAT(pData->face_transition,
                                                            HNR10_MIN_FACE_TRANSITION,
                                                            HNR10_MAX_FACE_TRANSITION);

        if (pUnpackedField->fd_snr_en && pUnpackedField->face_num > 0)
        {
            for (count = 0; count < pUnpackedField->face_num; count++)
            {
                rBoundary   = (faceRadius[count] * pData->face_boundary);
                rTransition = (faceRadius[count] * pData->face_transition);

                fractional      = static_cast<FLOAT>(frexp(IQSettingUtils::MaxFLOAT(rTransition, rBoundary), &exp));
                pseudo_mantissa = static_cast<INT>(ceil(fractional * (1 << HNR_Q_BITS_NUMBER)));
                if (pseudo_mantissa >= (1 << HNR_Q_BITS_NUMBER))
                {
                    exp++;
                }
                if (exp - HNR_Q_BITS_NUMBER >= 0)
                {
                    pUnpackedField->face_radius_shift[count] = static_cast<UINT16>(exp - HNR_Q_BITS_NUMBER);
                }
                else
                {
                    pUnpackedField->face_radius_shift[count] = 0;
                }
                pUnpackedField->face_radius_boundary[count] =
                    static_cast<UINT16>(ceil(rBoundary / (1 << pUnpackedField->face_radius_shift[count])));

                INT face_radius_transition =
                    static_cast<INT>(ceil(rTransition / (1 << pUnpackedField->face_radius_shift[count])));

                if (face_radius_transition>pUnpackedField->face_radius_boundary[count])
                {
                    DOUBLE delta    = 1.0 / (face_radius_transition - pUnpackedField->face_radius_boundary[count]);
                    fractional      = static_cast<FLOAT>(frexp(delta, &exp));
                    pseudo_mantissa = static_cast<INT>(ceil(fractional * (1 << HNR_Q_BITS_NUMBER)));

                    if (pseudo_mantissa >= (1 << HNR_Q_BITS_NUMBER))
                    {
                        fractional = 0.5;
                        exp++;
                    }
                    pUnpackedField->face_slope_shift[count]  = static_cast<UINT16>((-exp + 1));
                    pUnpackedField->face_radius_slope[count] = static_cast<UINT16>(ceil(fractional * (1 << HNR_Q_BITS_NUMBER)));
                }
                else
                {
                    pUnpackedField->face_radius_slope[count] = static_cast<UINT16>((1 << HNR_Q_BITS_NUMBER) - 1);
                    pUnpackedField->face_slope_shift[count]  = 0;
                }
            }
        }
        else
        {
            pUnpackedField->fd_snr_en = 0;
            IQSettingUtils::Memset(pUnpackedField->face_center_horizontal, 0x0, sizeof(INT16)* HNR_V10_MAX_FACE_NUM);
            IQSettingUtils::Memset(pUnpackedField->face_center_vertical, 0x0, sizeof(INT16) * HNR_V10_MAX_FACE_NUM);
            IQSettingUtils::Memset(pUnpackedField->face_radius_boundary, 0x0, sizeof(INT16) * HNR_V10_MAX_FACE_NUM);
            IQSettingUtils::Memset(pUnpackedField->face_radius_slope, 0x0, sizeof(INT16) * HNR_V10_MAX_FACE_NUM);
            IQSettingUtils::Memset(pUnpackedField->face_radius_shift, 0x0, sizeof(INT16) * HNR_V10_MAX_FACE_NUM);
            IQSettingUtils::Memset(pUnpackedField->face_slope_shift, 0x0, sizeof(INT16) * HNR_V10_MAX_FACE_NUM);
        }

        pUnpackedField->rnr_bx =
            static_cast<UINT16>(IQSettingUtils::ClampINT32((sensorOffsetX - (sWidth >> 1)),
                                                            HNR10_RNR_BX_MIN,
                                                            HNR10_RNR_BX_MAX));
        pUnpackedField->rnr_by =
            static_cast<UINT16>(IQSettingUtils::ClampINT32((sensorOffsetY - (sHeight >> 1)),
                                                            HNR10_RNR_BY_MIN,
                                                            HNR10_RNR_BY_MAX));

        pUnpackedField->rnr_r_square_init = IQSettingUtils::ClampUINT32(
            (pUnpackedField->rnr_bx) * (pUnpackedField->rnr_bx) + (pUnpackedField->rnr_by) * (pUnpackedField->rnr_by),
            HNR10_RNR_R_SQUARE_INIT_MIN,
            HNR10_RNR_R_SQUARE_INIT_MAX);

        rSquareMax = static_cast<INT>((sWidth >> 1) * (sWidth >> 1) + (sHeight >> 1) * (sHeight >> 1));

        for (rSquareShift = 0; rSquareShift < HNR10_R_SQUARE_SHFT_MAX; rSquareShift++)
        {
            if ((rSquareMax >> rSquareShift) <= HNR10_MAX_RSQUARE_VAL)
            {
                // rSquareMax>>r_square_shft range: [4096, 8191]
                pUnpackedField->rnr_r_square_shift = rSquareShift; // 4u
                break;
            }
        }
        pUnpackedField->rnr_r_square_scale =
            static_cast<UINT16>(4095 * 64 / (rSquareMax >> pUnpackedField->rnr_r_square_shift)); // 7u; range[0, 64]

        pUnpackedField->rnr_r_square_scale = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(pUnpackedField->rnr_r_square_scale,
                                        HNR10_RNR_MIN_R_SQUARE_SCALE,
                                        HNR10_RNR_MAX_R_SQUARE_SCALE));

        for (index = 0; index < HNR_V10_RNR_ARR_NUM + 1; index++)
        {
            tempRNRThreshold    = powf(pReserveType->radial_anchor_tab.radial_anchor[index] /
                                       pReserveType->radial_anchor_tab.radial_anchor[HNR_V10_RNR_ARR_NUM], 2.0f);
            RNRThreshold[index] = static_cast<INT16>(IQSettingUtils::FloatToQNumber(tempRNRThreshold, QNumber_10U));
            RNRGain[index]      = static_cast<INT16>(IQSettingUtils::FloatToQNumber(
                                                         pData->radial_noise_prsv_adj_tab.radial_noise_prsv_adj[index],
                                                         QNumber_9U));
        }

        for (index = 0; index < HNR_V10_RNR_ARR_NUM; index++)
        {
            pUnpackedField->rnr_anchor[index] = static_cast<UINT16>(IQSettingUtils::ClampINT32(RNRThreshold[index],
                                                                                             HNR10_RNR_MIN_ANCHOR,
                                                                                             HNR10_RNR_MAX_ANCHOR));

            pUnpackedField->rnr_base[index]   = static_cast<UINT16>(IQSettingUtils::ClampINT32(RNRGain[index],
                                                                                             HNR10_RNR_MIN_BASE,
                                                                                             HNR10_RNR_MAX_BASE));

            anchorDifference = IQSettingUtils::ClampINT16((RNRThreshold[index + 1] - RNRThreshold[index]),
                                                          HNR10_RNR_THRD_ARR_MIN,
                                                          HNR10_RNR_THRD_ARR_MAX);
            baseDifference   = IQSettingUtils::ClampINT16((RNRGain[index + 1] - RNRGain[index]),
                                                          HNR10_RNR_GAIN_ARR_MIN,
                                                          HNR10_RNR_GAIN_ARR_MAX);

            INT sign = 1;

            if (baseDifference < 0)
            {
                sign = -1;
            }

            baseDifference = static_cast<INT16>(IQSettingUtils::AbsoluteINT(baseDifference));

            if (0 == baseDifference)
            {
                pUnpackedField->rnr_shift[index] = 0;
            }
            else
            {
                pUnpackedField->rnr_shift[index] =
                    static_cast<UINT16>((log(static_cast<DOUBLE>(anchorDifference)) +
                        log(static_cast<DOUBLE>(1023)) - log(static_cast<DOUBLE>(baseDifference))) /
                        log(static_cast<DOUBLE>(2)));
            }
            pUnpackedField->rnr_shift[index] = static_cast<UINT16>(
                IQSettingUtils::ClampUINT32(pUnpackedField->rnr_shift[index], HNR10_RNR_MIN_SHIFT, HNR10_RNR_MAX_SHIFT));

            if (0 == anchorDifference)
            {
                pUnpackedField->rnr_slope[index] = 0;
            }
            else
            {
                pUnpackedField->rnr_slope[index] =  (baseDifference << pUnpackedField->rnr_shift[index]) / anchorDifference;
            }

            pUnpackedField->rnr_slope[index] =
                static_cast<INT16>(pUnpackedField->rnr_slope[index] * sign);
        }

        pUnpackedField->snr_skin_hue_min = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(abs(IQSettingUtils::RoundFLOAT(pData->skin_hue_min*(1 << 8))),
                                        HNR10_SKIN_HUE_MIN_MIN,
                                        HNR10_SKIN_HUE_MIN_MAX));
        pUnpackedField->snr_skin_hue_max = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(abs(IQSettingUtils::RoundFLOAT(pData->skin_hue_max*(1 << 8))),
                                        HNR10_SKIN_HUE_MAX_MIN,
                                        HNR10_SKIN_HUE_MAX_MAX));
        pUnpackedField->snr_skin_y_min  = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->skin_y_min*(1 << 8)),
                                        HNR10_SKIN_HUE_MAX_MIN,
                                        HNR10_SKIN_HUE_MAX_MAX));

        pUnpackedField->snr_skin_y_max  = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->skin_y_max*(1 << 8)),
                                        HNR10_SKIN_HUE_MAX_MIN,
                                        HNR10_SKIN_HUE_MAX_MAX));

        pUnpackedField->snr_boundary_probability = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32((IQSettingUtils::RoundFLOAT(pData->skin_boundary_probability)), 1, 15));

        slyMin = pData->skin_saturation_min_y_min;
        slyMax = pData->skin_saturation_max_y_min;

        shyMin = pData->skin_saturation_min_y_max;
        shyMax = pData->skin_saturation_max_y_max;

        pUnpackedField->snr_skin_ymax_sat_min =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(shyMin*(1 << 8)),
                                                            HNR10_SKIN_HUE_MAX_MIN,
                                                            HNR10_SKIN_HUE_MAX_MAX));
        pUnpackedField->snr_skin_ymax_sat_max =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(shyMax*(1 << 8)),
                                                            HNR10_SKIN_HUE_MAX_MIN,
                                                            HNR10_SKIN_HUE_MAX_MAX));

         // VALIDATE_PARAM(Ymax != Ymin);
        if (pData->skin_y_max > pData->skin_y_min)
        {
            tempSNRSlope = IQSettingUtils::FloatToQNumber((slyMin - shyMin) /
                                                             (pData->skin_y_max - pData->skin_y_min),
                                                          QNumber_8U);
            pUnpackedField->snr_sat_min_slope = static_cast<UINT16>(IQSettingUtils::ClampINT32(tempSNRSlope,
                                                                                                HNR10_SKIN_HUE_MAX_MIN,
                                                                                                HNR10_SKIN_HUE_MAX_MAX));
            tempSNRSlope = IQSettingUtils::FloatToQNumber((slyMax - shyMax) /
                                                             (pData->skin_y_max - pData->skin_y_min),
                                                          QNumber_8U);
            pUnpackedField->snr_sat_max_slope = static_cast<UINT16>(IQSettingUtils::ClampINT32(tempSNRSlope,
                                                                                                HNR10_SKIN_HUE_MAX_MIN,
                                                                                                HNR10_SKIN_HUE_MAX_MAX));
        }
        else
        {
            pUnpackedField->snr_sat_min_slope = 0;
            pUnpackedField->snr_sat_max_slope = 0;
        }

        qSkin = static_cast<FLOAT>(
            ((100.0f - pData->skin_percent) / (200.0f * (16.0f - pData->skin_boundary_probability))) * (1 << 8));
        pUnpackedField->snr_qstep_skin    = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(qSkin),
                                        HNR_SNR_QSTEP_MIN,
                                        HNR_SNR_QSTEP_MAX));

        pUnpackedField->snr_qstep_nonskin = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->skin_non_skin_to_skin_q_ratio * qSkin),
                                        HNR_SNR_QSTEP_MIN,
                                        HNR_SNR_QSTEP_MAX));

        pUnpackedField->snr_skin_smoothing_str = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->snr_skin_smoothing_str), 0, 2));

        for (index = 0; index < HNR_V10_SNR_ARR_NUM; index++)
        {
            pUnpackedField->snr_gain_arr[pUnpackedField->lut_bank_sel][index] = static_cast<UINT16>(
                IQSettingUtils::ClampUINT32((IQSettingUtils::RoundFLOAT(pData->snr_gain_arr_tab.snr_gain_arr[index]) -1),
                                            HNR_SNR_GAIN_MIN,
                                            HNR_SNR_GAIN_MAX));
        }

        pUnpackedField->lpf3_percent =
            static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->lpf3_percent),
                                                           HNR10_LPF3_PERCENT_MIN,
                                                           HNR10_LPF3_PERCENT_MAX));
        pUnpackedField->lpf3_offset  =
            static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->lpf3_offset),
                                                           HNR10_LPF3_OFFSET_MIN,
                                                           HNR10_LPF3_OFFSET_MAX));

        pUnpackedField->lpf3_strength =
            static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->lpf3_strength),
                                                           HNR10_LPF3_STRENGTH_MIN,
                                                           HNR10_LPF3_STRENGTH_MAX));

        pUnpackedField->blend_cnr_adj_gain =
            static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->blend_cnr_adj_gain),
                                                           HNR10_BLEND_CNR_ADJ_GAIN_MIN,
                                                           HNR10_BLEND_CNR_ADJ_GAIN_MAX));

        for (index = 0; index < HNR_V10_BLEND_SNR_ARR_NUM; index++)
        {
            INT blendSNRGain = IQSettingUtils::RoundFLOAT(pData->blend_snr_gain_arr_tab.blend_snr_gain_arr[index]) - 1;

            pUnpackedField->blend_snr_gain_arr[pUnpackedField->lut_bank_sel][index] =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(blendSNRGain,
                                                               HNR10_BLEND_SNR_GAINARR_MIN,
                                                               HNR10_BLEND_SNR_GAINARR_MAX));
        }

        for (index = 0; index < HNR_V10_BLEND_LNR_ARR_NUM; index++)
        {
            blendLnrGainLSB = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->blend_lnr_gain_arr_tab.blend_lnr_gain_arr[index]),
                                           HNR10_BLEND_LNR_GAINARR_MIN,
                                           HNR10_BLEND_LNR_GAINARR_MAX));

            if (index < HNR_V10_BLEND_LNR_ARR_NUM - 1)
            {
                blendLnrGainMSB = static_cast<UINT16>(
                    IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->blend_lnr_gain_arr_tab.
                                                                              blend_lnr_gain_arr[index + 1]),
                                               HNR10_BLEND_LNR_GAINARR_MIN,
                                               HNR10_BLEND_LNR_GAINARR_MAX));
            }
            else
            {
                blendLnrGainMSB = static_cast<UINT16>(
                    IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->blend_lnr_gain_arr_tab.
                                                                              blend_lnr_gain_arr[index]),
                                               HNR10_BLEND_LNR_GAINARR_MIN,
                                               HNR10_BLEND_LNR_GAINARR_MAX));
            }
            pUnpackedField->blend_lnr_gain_arr[pUnpackedField->lut_bank_sel][index] =
                static_cast<UINT16>((blendLnrGainMSB << 8) | blendLnrGainLSB);
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }
    return result;
}
