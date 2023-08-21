// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  lenr10setting.cpp
/// @brief IPE LENR10 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <math.h>
#include "lenr10setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LENR10Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL LENR10Setting::CalculateHWSetting(
    const LENR10InputData*                                  pInput,
    lenr_1_0_0::lenr10_rgn_dataType*                        pData,
    lenr_1_0_0::chromatix_lenr10_reserveType*               pReserveType,
    lenr_1_0_0::chromatix_lenr10Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                   pOutput)
{
    BOOL   result  = TRUE;
    UINT32 sensorOffsetX = 0;
    UINT32 sensorOffsetY = 0;
    INT32  sWidth  = 640;
    INT32  sHeight = 480;
    INT    index;
    INT16  faceRadius[MAX_FACE_NUM];
    INT32  tempSNRSlope;
    INT    exp;
    INT    count;
    FLOAT  fractional;
    FLOAT  rBoundary;
    FLOAT  rTransition;
    FLOAT  slyMin;
    FLOAT  slyMax;
    FLOAT  shyMin;
    FLOAT  shyMax;
    FLOAT  qSkin;
    INT    pseudoMantissa;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pReserveType)  &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        LENR10UnpackedField* pUnpackedField = static_cast<LENR10UnpackedField*>(pOutput);
        INT val                             = 0;
        sensorOffsetX                       = pInput->horizontalOffset;
        sensorOffsetY                       = pInput->verticalOffset;
        sWidth                              = pInput->streamWidth;
        sHeight                             = pInput->streamHeight;
        pUnpackedField->enable              = pModuleEnable->enable;
        pUnpackedField->LENRBltrEn          = pModuleEnable->lenr_bltr_en;
        pUnpackedField->LENRLceEn           = pModuleEnable->lenr_lce_en;
        pUnpackedField->LENRBltrLayer1Only  = pReserveType->lenr_bltr_layer1_only;
        pUnpackedField->rnrEn               = pReserveType->rnr_en;
        pUnpackedField->snrEn               = pReserveType->snr_en;
        pUnpackedField->fnrEn               = pReserveType->fnr_en;
        pUnpackedField->fdSnrEn             = pReserveType->fd_snr_en & pUnpackedField->snrEn;

        pUnpackedField->LENRDn4BltrTh = static_cast<INT16>(
            IQSettingUtils::ClampINT16(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_bltr_th_tab.lenr_dn4_8_16_bltr_th[0]), 0, 1023));
        pUnpackedField->LENRDn8BltrTh = static_cast<INT16>(
            IQSettingUtils::ClampINT16(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_bltr_th_tab.lenr_dn4_8_16_bltr_th[1]), 0, 1023));
        pUnpackedField->LENRDn16BltrTh = static_cast<INT16>(
            IQSettingUtils::ClampINT16(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_bltr_th_tab.lenr_dn4_8_16_bltr_th[2]), 0, 1023));

        pUnpackedField->LENRDn4BltrGap = static_cast<INT16>(
            IQSettingUtils::ClampINT16(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_bltr_gap_tab.lenr_dn4_8_16_bltr_gap[0]), 0, 15));
        pUnpackedField->LENRDn8BltrGap = static_cast<INT16>(
            IQSettingUtils::ClampINT16(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_bltr_gap_tab.lenr_dn4_8_16_bltr_gap[1]), 0, 15));
        pUnpackedField->LENRDn16BltrGap = static_cast<INT16>(
            IQSettingUtils::ClampINT16(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_bltr_gap_tab.lenr_dn4_8_16_bltr_gap[2]), 0, 15));

        pUnpackedField->LENRDn4BltrCtrlTh = static_cast<INT16>(
            IQSettingUtils::ClampINT16(IQSettingUtils::RoundFLOAT(
                pData->lenr_dn4_8_16_bltr_ctrl_th_tab.lenr_dn4_8_16_bltr_ctrl_th[0]), 0, 1023));
        pUnpackedField->LENRDn8BltrCtrlTh = static_cast<INT16>(
            IQSettingUtils::ClampINT16(IQSettingUtils::RoundFLOAT(
                pData->lenr_dn4_8_16_bltr_ctrl_th_tab.lenr_dn4_8_16_bltr_ctrl_th[1]), 0, 1023));
        pUnpackedField->LENRDn16BltrCtrlTh = static_cast<INT16>(
            IQSettingUtils::ClampINT16(IQSettingUtils::RoundFLOAT(
                pData->lenr_dn4_8_16_bltr_ctrl_th_tab.lenr_dn4_8_16_bltr_ctrl_th[2]), 0, 1023));

        pUnpackedField->LENRDn4BltrCtrlW = static_cast<INT16>(
            IQSettingUtils::ClampINT16(IQSettingUtils::RoundFLOAT(
                pData->lenr_dn4_8_16_bltr_ctrl_w_tab.lenr_dn4_8_16_bltr_ctrl_w[0]), 0, 1023));
        pUnpackedField->LENRDn8BltrCtrlW = static_cast<INT16>(
            IQSettingUtils::ClampINT16(IQSettingUtils::RoundFLOAT(
                pData->lenr_dn4_8_16_bltr_ctrl_w_tab.lenr_dn4_8_16_bltr_ctrl_w[1]), 0, 1023));
        pUnpackedField->LENRDn16BltrCtrlW = static_cast<INT16>(
            IQSettingUtils::ClampINT16(IQSettingUtils::RoundFLOAT(
                pData->lenr_dn4_8_16_bltr_ctrl_w_tab.lenr_dn4_8_16_bltr_ctrl_w[2]), 0, 1023));
        pUnpackedField->LENRDn4BltrClampEn = static_cast<INT16>(
            pReserveType->lenr_dn4_8_16_bltr_clamp_en_tab.lenr_dn4_8_16_bltr_clamp_en[0]);
        pUnpackedField->LENRDn8BltrClampEn = static_cast<INT16>(
            pReserveType->lenr_dn4_8_16_bltr_clamp_en_tab.lenr_dn4_8_16_bltr_clamp_en[1]);
        pUnpackedField->LENRDn16BltrClampEn = static_cast<INT16>(
            pReserveType->lenr_dn4_8_16_bltr_clamp_en_tab.lenr_dn4_8_16_bltr_clamp_en[2]);

        // Check as per document Range[0, 1023]
        pUnpackedField->LENRDn4BltrClampP = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                pReserveType->lenr_dn4_8_16_bltr_clamp_p_tab.lenr_dn4_8_16_bltr_clamp_p[0], -1023, 1023));
        // Check as per document Range[0, 1023]
        pUnpackedField->LENRDn8BltrClampP = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                pReserveType->lenr_dn4_8_16_bltr_clamp_p_tab.lenr_dn4_8_16_bltr_clamp_p[1], -1023, 1023));
        // Check as per document Range[0, 1023]
        pUnpackedField->LENRDn16BltrClampP = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                pReserveType->lenr_dn4_8_16_bltr_clamp_p_tab.lenr_dn4_8_16_bltr_clamp_p[2], -1023, 1023));

        // Check as per document Range[0, 1023]
        pUnpackedField->LENRDn4BltrClampN = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                pReserveType->lenr_dn4_8_16_bltr_clamp_n_tab.lenr_dn4_8_16_bltr_clamp_n[0], -1023, 1023));

        // Check as per document Range[0, 1023]
        pUnpackedField->LENRDn8BltrClampN = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                pReserveType->lenr_dn4_8_16_bltr_clamp_n_tab.lenr_dn4_8_16_bltr_clamp_n[1], -1023, 1023));

        // Check as per document Range[0, 1023]
        pUnpackedField->LENRDn16BltrClampN = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                pReserveType->lenr_dn4_8_16_bltr_clamp_n_tab.lenr_dn4_8_16_bltr_clamp_n[2], -1023, 1023));

        SetDNFNRArr(pReserveType->lenr_dn4_bltr_fnr_gain_arr_tab.lenr_dn4_bltr_fnr_gain_arr,
            pUnpackedField->LENRDn4BltrFnrGainArr,
            LENR_V10_FNR_AC_ARR_NUM, LENR10_FNR_GAINARR_MIN, LENR10_FNR_GAINARR_MAX);

        SetDNSNRArr(pData->lenr_dn4_bltr_snr_gain_arr_tab.lenr_dn4_bltr_snr_gain_arr,
            pUnpackedField->LENRDn4BltrSnrGainArr,
            LENR_V10_SNR_ARR_NUM, LENR10_SNR_GAIN_MIN, LENR10_SNR_GAIN_MAX);

        SetDNFNRArr(pReserveType->lenr_dn8_bltr_fnr_gain_arr_tab.lenr_dn8_bltr_fnr_gain_arr,
            pUnpackedField->LENRDn8BltrFnrGainArr,
            LENR_V10_FNR_AC_ARR_NUM, LENR10_FNR_GAINARR_MIN, LENR10_FNR_GAINARR_MAX);

        SetDNSNRArr(pData->lenr_dn8_bltr_snr_gain_arr_tab.lenr_dn8_bltr_snr_gain_arr,
            pUnpackedField->LENRDn8BltrSnrGainArr,
            LENR_V10_SNR_ARR_NUM, LENR10_SNR_GAIN_MIN, LENR10_SNR_GAIN_MAX);

        SetDNFNRArr(pReserveType->lenr_dn16_bltr_fnr_gain_arr_tab.lenr_dn16_bltr_fnr_gain_arr,
            pUnpackedField->LENRDn16BltrFnrGainArr,
            LENR_V10_FNR_AC_ARR_NUM, LENR10_FNR_GAINARR_MIN, LENR10_FNR_GAINARR_MAX);
        SetDNSNRArr(pData->lenr_dn16_bltr_snr_gain_arr_tab.lenr_dn16_bltr_snr_gain_arr,
            pUnpackedField->LENRDn16BltrSnrGainArr,
            LENR_V10_SNR_ARR_NUM, LENR10_SNR_GAIN_MIN, LENR10_SNR_GAIN_MAX);

        SetRNRAdj(pReserveType->bltr_rnr_thrd_arr_tab.bltr_rnr_thrd_arr,
            pData->bltr_rnr_gain_arr_tab.bltr_rnr_gain_arr,
            pUnpackedField->LENRAllBltrRnrAnchor,
            pUnpackedField->LENRAllBltrRnrBase,
            pUnpackedField->LENRAllBltrRnrShift,
            pUnpackedField->LENRAllBltrRnrSlope);

        pUnpackedField->LENRDn4LceCoreP = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_core_p_tab.lenr_dn4_8_16_lce_core_p[0]), 0, 1023));
        pUnpackedField->LENRDn8LceCoreP = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_core_p_tab.lenr_dn4_8_16_lce_core_p[1]), 0, 1023));
        pUnpackedField->LENRDn16LceCoreP = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_core_p_tab.lenr_dn4_8_16_lce_core_p[2]), 0, 1023));

        pUnpackedField->LENRDn4LceCoreN = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_core_n_tab.lenr_dn4_8_16_lce_core_n[0]), 0, 1023));
        pUnpackedField->LENRDn8LceCoreN = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_core_n_tab.lenr_dn4_8_16_lce_core_n[1]), 0, 1023));

        pUnpackedField->LENRDn16LceCoreN = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_core_n_tab.lenr_dn4_8_16_lce_core_n[2]), 0, 1023));

        pUnpackedField->LENRDn4LceScaleP = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_scale_p_tab.lenr_dn4_8_16_lce_scale_p[0]), 0, 255));
        pUnpackedField->LENRDn8LceScaleP = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_scale_p_tab.lenr_dn4_8_16_lce_scale_p[1]), 0, 255));
        pUnpackedField->LENRDn16LceScaleP = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_scale_p_tab.lenr_dn4_8_16_lce_scale_p[2]), 0, 255));

        pUnpackedField->LENRDn4LceScaleN = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_scale_n_tab.lenr_dn4_8_16_lce_scale_n[0]), 0, 255));
        pUnpackedField->LENRDn8LceScaleN = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_scale_n_tab.lenr_dn4_8_16_lce_scale_n[1]), 0, 255));
        pUnpackedField->LENRDn16LceScaleN = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_scale_n_tab.lenr_dn4_8_16_lce_scale_n[2]), 0, 255));

        pUnpackedField->LENRDn4LceClampP = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_clamp_p_tab.lenr_dn4_8_16_lce_clamp_p[0]), -1023, 1023));
        pUnpackedField->LENRDn8LceClampP = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_clamp_p_tab.lenr_dn4_8_16_lce_clamp_p[1]), -1023, 1023));
        pUnpackedField->LENRDn16LceClampP = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_clamp_p_tab.lenr_dn4_8_16_lce_clamp_p[2]), -1023, 1023));

        pUnpackedField->LENRDn4LceClampN = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_clamp_n_tab.lenr_dn4_8_16_lce_clamp_n[0]), -1023, 1023));
        pUnpackedField->LENRDn8LceClampN = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_clamp_n_tab.lenr_dn4_8_16_lce_clamp_n[1]), -1023, 1023));
        pUnpackedField->LENRDn16LceClampN = static_cast<INT16>(
            IQSettingUtils::ClampINT32(
                IQSettingUtils::RoundFLOAT(pData->lenr_dn4_8_16_lce_clamp_n_tab.lenr_dn4_8_16_lce_clamp_n[2]), -1023, 1023));

        SetDNFNRArr(pReserveType->lenr_dn4_lce_fnr_gain_arr_tab.lenr_dn4_lce_fnr_gain_arr,
            pUnpackedField->LENRDn4LceFnrGainArr,
            LENR_V10_FNR_AC_ARR_NUM, LENR10_FNR_GAINARR_MIN, LENR10_FNR_GAINARR_MAX);

        SetDNSNRArr(pData->lenr_dn4_lce_snr_gain_arr_tab.lenr_dn4_lce_snr_gain_arr,
            pUnpackedField->LENRDn4LceSnrGainArr,
            LENR_V10_SNR_ARR_NUM, LENR10_SNR_GAIN_MIN, LENR10_SNR_GAIN_MAX);

        SetDNFNRArr(pReserveType->lenr_dn8_lce_fnr_gain_arr_tab.lenr_dn8_lce_fnr_gain_arr,
            pUnpackedField->LENRDn8LceFnrGainArr,
            LENR_V10_FNR_AC_ARR_NUM, LENR10_FNR_GAINARR_MIN, LENR10_FNR_GAINARR_MAX);
        SetDNSNRArr(pData->lenr_dn8_lce_snr_gain_arr_tab.lenr_dn8_lce_snr_gain_arr,
            pUnpackedField->LENRDn8LceSnrGainArr,
            LENR_V10_SNR_ARR_NUM, LENR10_SNR_GAIN_MIN, LENR10_SNR_GAIN_MAX);

        SetDNFNRArr(pReserveType->lenr_dn16_lce_fnr_gain_arr_tab.lenr_dn16_lce_fnr_gain_arr,
            pUnpackedField->LENRDn16LceFnrGainArr,
            LENR_V10_FNR_AC_ARR_NUM, LENR10_FNR_GAINARR_MIN, LENR10_FNR_GAINARR_MAX);
        SetDNSNRArr(pData->lenr_dn16_lce_snr_gain_arr_tab.lenr_dn16_lce_snr_gain_arr,
            pUnpackedField->LENRDn16LceSnrGainArr,
            LENR_V10_SNR_ARR_NUM, LENR10_SNR_GAIN_MIN, LENR10_SNR_GAIN_MAX);

        SetRNRAdj(pReserveType->lce_rnr_thrd_arr_tab.lce_rnr_thrd_arr, pData->lce_rnr_gain_arr_tab.lce_rnr_gain_arr,
            pUnpackedField->LENRAllLceRnrAnchor,
            pUnpackedField->LENRAllLceRnrBase,
            pUnpackedField->LENRAllLceRnrShift,
            pUnpackedField->LENRAllLceRnrSlope);

        pUnpackedField->LENRAllLceKernelCenter = static_cast<INT16>(
            IQSettingUtils::ClampINT16(pReserveType->lenr_all_lce_kernel_tab.lenr_all_lce_kernel[0], -2048, 2047));

        INT count_msb = 0;
        for (INT j = 0; j < 9; j++)
        {
            INT16 val = IQSettingUtils::ClampINT16(
                pReserveType->lenr_all_lce_kernel_tab.lenr_all_lce_kernel[j + 1], -1024, 1024);
            INT16 sign = -2*((val >> 15) & 0x1) + 1;
            val = val*sign;
            if (val == 0)
            {
                sign = 0;
            }
            INT16 count_msb_bits = 0;
            INT16 msb[3] = { 0, 0, 0 };
            for (INT i = 12; i >= 0; i--)
            {
                msb[2 - count_msb_bits] = i;
                count_msb_bits += ((val >> i) & 0x1);

                if (count_msb_bits >= 3)
                {
                    break;
                }
            }
            if (count_msb_bits == 1)
            {
                if (msb[2] > 0)
                {
                    msb[2] = msb[1] = msb[2] - 1;
                }
                if (msb[1] > 0)
                {
                    msb[0] = msb[1] = msb[1] - 1;
                }
            }
            else if (count_msb_bits == 2)
            {
                if (msb[1] > 0)
                {
                    msb[0] = msb[1] = msb[1] - 1;
                }
                else if (msb[2] > 0)
                {
                    msb[2] = msb[1] = msb[2] - 1;
                }
            }
            pUnpackedField->LENRAllLceKernel[j][0] = sign;
            pUnpackedField->LENRAllLceKernel[j][1] = msb[2];
            pUnpackedField->LENRAllLceKernel[j][2] = msb[1];
            pUnpackedField->LENRAllLceKernel[j][3] = msb[0];
        }

        pUnpackedField->LENRAllFnrShift = static_cast<INT16>(
            IQSettingUtils::ClampINT16(pReserveType->lenr_all_fnr_shift, 0, 3));

        INT widthDn4x  = sWidth;
        INT heightDn4x = sHeight;

        // Crop
        pUnpackedField->cropStripeToImageBoundaryDistLeft = 0;
        pUnpackedField->cropOutputStripeWidth = 0;
        pUnpackedField->cropFrameBoundaryPixAlignRight =
            pInput->cropFrameBoundaryPixAlignRight; // 0: 4-pix align, 1: 2-pix align

        pUnpackedField->postCropEnable     = 0;
        pUnpackedField->postCropFirstPixel = 0;
        pUnpackedField->postCropLastPixel  = 0;
        pUnpackedField->postCropFirstLine  = 0;
        pUnpackedField->postCropLastLine   = 0;

        // Pre-crop: crop_frame_boundary_pix_align_right
        if (pUnpackedField->cropFrameBoundaryPixAlignRight == 1)    // 2-pixel align
        {
            widthDn4x = widthDn4x - 2;
        }
        SetRNRParam(widthDn4x, heightDn4x,
            &pUnpackedField->LENRDn4RnrBx,
            &pUnpackedField->LENRDn4RnrBy,
            &pUnpackedField->LENRDn4RnrRSquareInit,
            &pUnpackedField->LENRDn4RnrRSquareShift,
            &pUnpackedField->LENRDn4RnrRSquareScale);

        SetRNRParam((widthDn4x + 1) / 2 + 1, (heightDn4x + 1) / 2 + 1,
            &pUnpackedField->LENRDn8RnrBx,
            &pUnpackedField->LENRDn8RnrBy,
            &pUnpackedField->LENRDn8RnrRSquareInit,
            &pUnpackedField->LENRDn8RnrRSquareShift,
            &pUnpackedField->LENRDn8RnrRSquareScale);

        SetRNRParam((widthDn4x + 3) / 4 + 1, (heightDn4x + 3) / 4 + 1,
            &pUnpackedField->LENRDn16RnrBx,
            &pUnpackedField->LENRDn16RnrBy,
            &pUnpackedField->LENRDn16RnrRSquareInit,
            &pUnpackedField->LENRDn16RnrRSquareShift,
            &pUnpackedField->LENRDn16RnrRSquareScale);

        pUnpackedField->snrSkinHueMin = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(abs(IQSettingUtils::RoundFLOAT(pData->skin_hue_min*(1 << 8))),
            LENR10_SKIN_HUE_MIN_MIN,
            LENR10_SKIN_HUE_MIN_MAX));
        pUnpackedField->snrSkinHueMax = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(abs(IQSettingUtils::RoundFLOAT(pData->skin_hue_max*(1 << 8))),
            LENR10_SKIN_HUE_MAX_MIN,
            LENR10_SKIN_HUE_MAX_MAX));
        pUnpackedField->snrSkinYMin = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->skin_y_min*(1 << 8)),
            LENR10_SKIN_HUE_MAX_MIN,
            LENR10_SKIN_HUE_MAX_MAX));

        pUnpackedField->snrSkinYMax = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->skin_y_max*(1 << 8)),
            LENR10_SKIN_HUE_MAX_MIN,
            LENR10_SKIN_HUE_MAX_MAX));

        pUnpackedField->snrBoundaryProbability = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32((IQSettingUtils::RoundFLOAT(pData->skin_boundary_probability)), 1, 15));

        slyMin = pData->skin_saturation_min_y_min;
        slyMax = pData->skin_saturation_max_y_min;

        shyMin = pData->skin_saturation_min_y_max;
        shyMax = pData->skin_saturation_max_y_max;

        pUnpackedField->snrSkinYmaxSatMin =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(shyMin*(1 << 8)),
                LENR10_SKIN_HUE_MAX_MIN,
                LENR10_SKIN_HUE_MAX_MAX));
        pUnpackedField->snrSkinYmaxSatMax =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(shyMax*(1 << 8)),
                LENR10_SKIN_HUE_MAX_MIN,
                LENR10_SKIN_HUE_MAX_MAX));

        // VALIDATE_PARAM(Ymax != Ymin);
        if (pData->skin_y_max > pData->skin_y_min)
        {
            tempSNRSlope = IQSettingUtils::FloatToQNumber((slyMin - shyMin) /
                (pData->skin_y_max - pData->skin_y_min),
                QNumber_8U);
            pUnpackedField->snrSatMinSlope = static_cast<UINT16>(IQSettingUtils::ClampUINT32(tempSNRSlope,
                LENR10_SKIN_HUE_MAX_MIN,
                LENR10_SKIN_HUE_MAX_MAX));
            tempSNRSlope = IQSettingUtils::FloatToQNumber((slyMax - shyMax) /
                (pData->skin_y_max - pData->skin_y_min),
                QNumber_8U);
            pUnpackedField->snrSatMaxSlope = static_cast<UINT16>(IQSettingUtils::ClampUINT32(tempSNRSlope,
                LENR10_SKIN_HUE_MAX_MIN,
                LENR10_SKIN_HUE_MAX_MAX));
        }
        else
        {
            pUnpackedField->snrSatMinSlope = 0;
            pUnpackedField->snrSatMaxSlope = 0;
        }
        qSkin = static_cast<FLOAT>(
            ((100.0f - pData->skin_percent) / (200.0f * (16.0f - pData->skin_boundary_probability))) * (1 << 8));
        pUnpackedField->snrQstepSkin = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(qSkin),
                LENR10_SNR_QSTEP_MIN,
                LENR10_SNR_QSTEP_MAX));
        pUnpackedField->snrQstepNonskin = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->skin_non_skin_to_skin_q_ratio * qSkin),
                LENR10_SNR_QSTEP_MIN,
                LENR10_SNR_QSTEP_MAX));
        // FD_SNR
        pUnpackedField->faceNum = pInput->pFDData->numberOfFace;
        pUnpackedField->faceHorizontalOffset = 0;
        pUnpackedField->faceVerticalOffset = 0;

        for (index = 0; index < LENR_V10_MAX_FACE_NUM; index++)
        {
            faceRadius[index] = pInput->pFDData->faceRadius[index];
            pUnpackedField->faceCenterHorizontal[index] = pInput->pFDData->faceCenterX[index];
            pUnpackedField->faceCenterVertical[index] = pInput->pFDData->faceCenterY[index];
        }
        pData->face_boundary = IQSettingUtils::ClampFLOAT(pData->face_boundary,
            LENR10_MIN_FACE_BOUNDARY,
            LENR10_MAX_FACE_BOUNDARY);
        pData->face_transition = IQSettingUtils::ClampFLOAT(pData->face_transition,
            LENR10_MIN_FACE_TRANSITION,
            LENR10_MAX_FACE_TRANSITION);
        if (pUnpackedField->fdSnrEn && pUnpackedField->faceNum > 0)
        {
            for (count = 0; count < pUnpackedField->faceNum; count++)
            {
                rBoundary = (faceRadius[count] * pData->face_boundary);
                rTransition = (faceRadius[count] * pData->face_transition);

                fractional = static_cast<FLOAT>(frexp(IQSettingUtils::MaxFLOAT(rTransition, rBoundary), &exp));
                pseudoMantissa = static_cast<INT>(ceil(fractional * (1 << LENR_Q_BITS_NUMBER)));
                if (pseudoMantissa >= (1 << LENR_Q_BITS_NUMBER))
                {
                    exp++;
                }
                if (exp - LENR_Q_BITS_NUMBER >= 0)
                {
                    pUnpackedField->faceRadiusShift[count] = static_cast<UINT16>(exp - LENR_Q_BITS_NUMBER);
                }
                else
                {
                    pUnpackedField->faceRadiusShift[count] = 0;
                }
                pUnpackedField->faceRadiusBoundary[count] =
                    static_cast<UINT16>(ceil(rBoundary / (1 << pUnpackedField->faceRadiusShift[count])));

                INT faceRadiusTransition =
                    static_cast<INT>(ceil(rTransition / (1 << pUnpackedField->faceRadiusShift[count])));

                if (faceRadiusTransition>pUnpackedField->faceRadiusBoundary[count])
                {
                    DOUBLE delta = 1.0 / (faceRadiusTransition - pUnpackedField->faceRadiusBoundary[count]);
                    fractional = static_cast<FLOAT>(frexp(delta, &exp));
                    pseudoMantissa = static_cast<INT>(ceil(fractional * (1 << LENR_Q_BITS_NUMBER)));

                    if (pseudoMantissa >= (1 << LENR_Q_BITS_NUMBER))
                    {
                        fractional = 0.5;
                        exp++;
                    }
                    pUnpackedField->faceSlopeShift[count] = static_cast<UINT16>((-exp + 1));
                    pUnpackedField->faceRadiusSlope[count] = static_cast<UINT16>(ceil(fractional * (1 << LENR_Q_BITS_NUMBER)));
                }
                else
                {
                    pUnpackedField->faceRadiusSlope[count] = static_cast<UINT16>((1 << LENR_Q_BITS_NUMBER) - 1);
                    pUnpackedField->faceSlopeShift[count] = 0;
                }
            }
        }
        else
        {
            pUnpackedField->fdSnrEn = 0;
            IQSettingUtils::Memset(pUnpackedField->faceCenterHorizontal, 0x0, sizeof(INT16)* LENR_V10_MAX_FACE_NUM);
            IQSettingUtils::Memset(pUnpackedField->faceCenterVertical, 0x0, sizeof(INT16)* LENR_V10_MAX_FACE_NUM);
            IQSettingUtils::Memset(pUnpackedField->faceRadiusBoundary, 0x0, sizeof(INT16)* LENR_V10_MAX_FACE_NUM);
            IQSettingUtils::Memset(pUnpackedField->faceRadiusSlope, 0x0, sizeof(INT16)* LENR_V10_MAX_FACE_NUM);
            IQSettingUtils::Memset(pUnpackedField->faceRadiusShift, 0x0, sizeof(INT16)* LENR_V10_MAX_FACE_NUM);
            IQSettingUtils::Memset(pUnpackedField->faceSlopeShift, 0x0, sizeof(INT16)* LENR_V10_MAX_FACE_NUM);
        }
        pUnpackedField->cnrScale = static_cast<UINT16>(
            IQSettingUtils::ClampINT32(pReserveType->cnr_scale,
            LENR10_CNR_SCALE_MIN,
            LENR10_CNR_SCALE_MAX));

        pUnpackedField->us4InitPhV = 2;
        pUnpackedField->us4InitPhH = 2;
        pUnpackedField->cropStripeToImageBoundaryDistLeft = 0;
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LENR10Setting::SetRNRAdj
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LENR10Setting::SetRNRAdj(
    FLOAT*  pRnrThrdArr,
    FLOAT*  pRnrGainArr,
    UINT16* pRnrAnchor,
    UINT16* pRnrBase,
    UINT16* pRnrShift,
    INT16*  pRnrSlope)
{
    for (INT i = 0; i < LENR_V10_RNR_ARR_NUM+1; i++)
    {
        FLOAT tempRNRThreshold = powf(pRnrThrdArr[i] / pRnrThrdArr[LENR_V10_RNR_ARR_NUM], 2.0f);
        pRnrThrdArr[i] = static_cast<INT16>(IQSettingUtils::FloatToQNumber(tempRNRThreshold, QNumber_10U));
        pRnrGainArr[i] = static_cast<INT16>(IQSettingUtils::FloatToQNumber(pRnrGainArr[i], QNumber_9U));
    }
    for (INT i = 0; i < LENR_V10_RNR_ARR_NUM; i++)
    {
        pRnrAnchor[i] = static_cast<UINT16>(pRnrThrdArr[i]);
        pRnrBase[i] = static_cast<UINT16>(pRnrGainArr[i]);

        UINT16 dd1 = static_cast<UINT16>(
            IQSettingUtils::ClampINT16(static_cast<INT16>(pRnrThrdArr[i + 1] - pRnrThrdArr[i]), 0, (1 << 10) - 1));
        INT16 bb1 = IQSettingUtils::ClampINT16(static_cast<INT16>((pRnrGainArr[i + 1] - pRnrGainArr[i])), -1023, 1023);
        UINT8 sign = (bb1 > 0) ? 1 : 0;
        bb1 = abs(bb1);

        if (bb1 == 0)
        {
            pRnrShift[i] = 0;
        }
        else
        {
            pRnrShift[i] = static_cast<UINT16>(IQSettingUtils::ClampFLOAT(
                static_cast<FLOAT>(((log(static_cast<FLOAT>(dd1)) + log(1023.0f) - log(static_cast<FLOAT>(bb1))) / log(2.0f))),
                static_cast<FLOAT>(LENR10_RNR_MIN_ANCHOR), static_cast<FLOAT>(LENR10_RNR_MAX_ANCHOR)));
        }
        pRnrShift[i] = IQSettingUtils::ClampINT16(pRnrShift[i], LENR10_RNR_MIN_SHIFT, LENR10_RNR_MAX_SHIFT);
        pRnrSlope[i] = (dd1 != 0) ? (bb1 << pRnrShift[i]) / dd1 : 0;
        pRnrSlope[i] = (sign) ? pRnrSlope[i] : -pRnrSlope[i];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LENR10Setting::SetDNFNRArr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LENR10Setting::SetDNFNRArr(
    UINT32* pDnFnrArr,
    UINT16* pUnpackDnFnrArr,
    INT32 size,
    INT32 clamp_min,
    INT32 clamp_max)
{
    INT32 index = 0;
    INT32 currentVal = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pDnFnrArr[0], clamp_min, clamp_max));
    INT32 val = 0;
    INT32 nextVal = 0;
    for (index = 0; index < size - 1; index++)
    {
        nextVal = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pDnFnrArr[index + 1], clamp_min, clamp_max));
        pUnpackDnFnrArr[index] = (nextVal << 6) + currentVal;
        currentVal = nextVal;
    }
    pUnpackDnFnrArr[size - 1] = (currentVal << 6) + currentVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LENR10Setting::SetDNSNRArr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LENR10Setting::SetDNSNRArr(
    FLOAT*  pDnSnrArr,
    UINT16* pUnpackDnSnrArr,
    INT32   size,
    INT32   clamp_min,
    INT32   clamp_max)
{
    INT32 index = 0;
    INT32 val = 0;
    for (index = 0; index < size; index++)
    {
        pUnpackDnSnrArr[index] =
            static_cast<UINT16>(IQSettingUtils::ClampUINT16(
                IQSettingUtils::RoundFLOAT(pDnSnrArr[index]) - 1, clamp_min, clamp_max));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LENR10Setting::SetRNRParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LENR10Setting::SetRNRParam(
    INT32   width,
    INT32   height,
    INT16*  pRnrBx,
    INT16*  pRnrBy,
    UINT32* pRnrRSquareInit,
    UINT16* pRnrRSquareShift,
    UINT16* pRnrRSquareScale)
{
    // 14s, init_h_offset-h_center, need modification for strip processing
    *pRnrBx = 0 - (width >> 1);  // 2 - (width>>1);
    // 14s, init_v_offset-v_center, need modification for strip processing
    *pRnrBy = 0 - (height >> 1); // 2 - (height>>1);
    // 28u, (init_h_offset-h_center)^2 + (init_v_offset-v_center)^2
    *pRnrRSquareInit = *pRnrBx * *pRnrBx + *pRnrBy * *pRnrBy;

    INT rSquareMax = (width >> 1) * (width >> 1) + (height >> 1) * (height >> 1);
    for (INT i = 0; i < 16; i++)
    {
        if ((rSquareMax >> i) <= 8191)
        { // r_square_max>>r_square_shft range: [4096, 8191]
            *pRnrRSquareShift = i; // 4u
            break;
        }
    }
    if (*pRnrRSquareShift != 0)
    {
        *pRnrRSquareScale = 4095 * 64 / (rSquareMax >> *pRnrRSquareShift);   // 7u; range[0, 64]
        *pRnrRSquareScale = IQSettingUtils::ClampUINT16(*pRnrRSquareScale, 0, 127);
    }
}