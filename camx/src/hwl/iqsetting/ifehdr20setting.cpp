// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifehdr20setting.cpp
/// @brief IFE HDR20 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ifehdr20setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEHDR20Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEHDR20Setting::CalculateHWSetting(
    const HDR20InputData*                   pInput,
    hdr_2_0_0::hdr20_rgn_dataType*          pData,
    hdr_2_0_0::chromatix_hdr20_reserveType* pReserveType,
    VOID*                                   pOutput)
{
    BOOL                                    result                   = TRUE;
    FLOAT                                   totalGainSqrt            = 1.0f;
    FLOAT                                   HDRExpSensitivityRatio   = 1.0f;
    UINT32                                  BLSSOffset               = 0;
    UINT32                                  HDRZigzagReconSel        = 0;
    UINT16                                  HDRZigzagReconPattern    = 0;
    UINT32                                  HDRZigzagReconFirstRBExp = 0;
    hdr_2_0_0::chromatix_hdr20_reserveType* pInputReserve;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pOutput) && (NULL != pReserveType))
    {
        HDR20UnpackedField* pUnpackedField = static_cast<HDR20UnpackedField*>(pOutput);

        pInputReserve            = pReserveType;
        BLSSOffset               = pInput->blackLevelOffset;
        HDRExpSensitivityRatio   = pInput->AECSensitivity;
        HDRZigzagReconFirstRBExp = pInput->ZZHDRFirstRBEXP;
        HDRZigzagReconSel        = pInput->HDRZigzagReconSel;
        HDRZigzagReconPattern    = pInput->ZZHDRPattern;

        // parameters common to left/right panels
        pUnpackedField->hdr_first_field = pInput->RECONFirstField;

        pUnpackedField->hdr_rec_linear_mode = 1;
        pUnpackedField->hdr_mac_linear_mode = 1;


        pUnpackedField->hdr_recon_en        = static_cast<UINT16>(pInputReserve->hdr_recon_en);
        pUnpackedField->hdr_mac_en          = static_cast<UINT16>(pInputReserve->hdr_mac_en);

        pUnpackedField->hdr_MSB_align       = static_cast<UINT16>(pInputReserve->hdr_msb_align);

        pUnpackedField->hdr_exp_ratio       =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(HDRExpSensitivityRatio, QNumber_10U),
                                                            NMAX10,
                                                            MAX_UINT14 + 1));

        pUnpackedField->hdr_exp_ratio_recip =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(FLOATMAX8 /HDRExpSensitivityRatio),
                                                            HDR_EXP_RATIO_RECIP_MIN,
                                                            HDR_EXP_RATIO_RECIP_MAX));

        pUnpackedField->hdr_rec_edge_lpf_tap0    =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pInputReserve->recon_edge_lpf_tap0,
                                                            HDR20_RECON_EDGE_LPF_TAP0_MIN,
                                                            HDR20_RECON_EDGE_LPF_TAP0_MAX));

        pUnpackedField->hdr_mac_dilation         =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pInputReserve->mac_motion_dilation,
                                                            HDR20_MAC_MOTION_DILATION_MIN,
                                                            HDR20_MAC_MOTION_DILATION_MAX));

        pUnpackedField->hdr_mac_smooth_filter_en =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pInputReserve->mac_smooth_enable,
                                                            HDR_MAC_SMOOTH_FILTER_MIN,
                                                            HDR_MAC_SMOOTH_FILTER_MAX));

        // HDR Reconstruction parameters
        pUnpackedField->hdr_rec_flat_region_th   =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->recon_flat_region_th),
                                                            HDR20_RECON_FLAT_REGION_TH_MIN,
                                                            HDR20_RECON_FLAT_REGION_TH_MAX));

        pUnpackedField->hdr_rec_min_factor       =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->recon_min_factor),
                                                            HDR20_RECON_MIN_FACTOR_MIN,
                                                            HDR20_RECON_MIN_FACTOR_MAX));

        pUnpackedField->hdr_rec_h_edge_th1       =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->recon_h_edge_th1),
                                                            HDR20_RECON_H_EDGE_TH1_MIN,
                                                            HDR20_RECON_H_EDGE_TH1_MAX));

        pUnpackedField->hdr_rec_h_edge_dth_log2  =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->recon_h_edge_dth_log2),
                                                            HDR20_RECON_H_EDGE_DTH_LOG2_MIN,
                                                            HDR20_RECON_H_EDGE_DTH_LOG2_MAX));

        pUnpackedField->hdr_rec_motion_th1       =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->recon_motion_th1),
                                                            HDR20_RECON_MOTION_TH1_MIN,
                                                            HDR20_RECON_MOTION_TH1_MAX));

        pUnpackedField->hdr_rec_motion_dth_log2  =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->recon_motion_dth_log2),
                                                            HDR20_RECON_MOTION_DTH_LOG2_MIN,
                                                            HDR20_RECON_MOTION_DTH_LOG2_MAX));

        pUnpackedField->hdr_rec_dark_th1         =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->recon_dark_th1),
                                                            HDR20_RECON_DARK_TH1_MIN,
                                                            HDR20_RECON_DARK_TH1_MAX));

        pUnpackedField->hdr_rec_dark_dth_log2    =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->recon_dark_dth_log2),
                                                            HDR20_RRECON_DARK_DTH_LOG2_MIN,
                                                            HDR20_RRECON_DARK_DTH_LOG2_MAX));

        //  HDR Motion Adaptive Combine parameters
        pUnpackedField->hdr_mac_sqrt_analog_gain = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(totalGainSqrt , QNumber_4U),
                                        HDR_MAC_SQRT_ANALOG_GAIN_MIN,
                                        HDR_MAC_SQRT_ANALOG_GAIN_MAX)); // 16~63 in HPG

        pUnpackedField->hdr_mac_motion0_th1      =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->mac_motion0_th1),
                                                            HDR20_MAC_MOTION0_TH1_MIN,
                                                            HDR20_MAC_MOTION0_TH1_MAX));

        pUnpackedField->hdr_mac_motion0_th2      =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->mac_motion0_th2),
                                                            HDR20_MAC_MOTION0_TH2_MIN,
                                                            HDR20_MAC_MOTION0_TH2_MAX));

        pUnpackedField->hdr_mac_motion0_dt0      =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pInputReserve->mac_motion0_dt0,
                                                            HDR20_MAC_MOTION0_DT0_MIN,
                                                            HDR20_MAC_MOTION0_DT0_MAX));

        pUnpackedField->hdr_mac_motion_strength  =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->mac_motion0_strength),
                                                            HDR20_MAC_MOTION0_STRENGTH_MIN,
                                                            HDR20_MAC_MOTION0_STRENGTH_MAX));

        pUnpackedField->hdr_mac_lowlight_strength =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->mac_low_light_strength),
                                                            HDR20_MAC_LOW_LIGHT_STRENGTH_MIN,
                                                            HDR20_MAC_LOW_LIGHT_STRENGTH_MAX));

        pUnpackedField->hdr_mac_lowlight_th1      =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->mac_low_light_th1),
                                                            HDR20_MAC_LOW_LIGHT_TH1_MIN,
                                                            HDR20_MAC_LOW_LIGHT_TH1_MAX));

        pUnpackedField->hdr_mac_lowlight_dth_log2 =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pInputReserve->mac_low_light_dth_log2,
                                                            HDR20_MAC_LOW_LIGHT_DTH_LOG2_MIN,
                                                            HDR20_MAC_LOW_LIGHT_DTH_LOG2_MAX));

        pUnpackedField->hdr_mac_hilight_th1      =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->mac_high_light_th1),
                                                            HDR20_MAC_HIGH_LIGHT_TH1_MIN,
                                                            HDR20_MAC_HIGH_LIGHT_TH1_MAX));

        pUnpackedField->hdr_mac_hilight_dth_log2 =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->mac_high_light_dth_log2),
                                                            HDR20_MAC_HIGH_LIGHT_DTH_LOG2_MIN,
                                                            HDR20_MAC_HIGH_LIGHT_DTH_LOG2_MAX));

        pUnpackedField->hdr_mac_smooth_tap0       =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pInputReserve->mac_smooth_tap0,
                                                            HDR20_MAC_SMOOTH_TAP0_MIN,
                                                            HDR20_MAC_SMOOTH_TAP0_MAX));

        pUnpackedField->hdr_mac_smooth_th1        =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pInputReserve->mac_smooth_th1,
                                                            HDR20_MAC_SMOOTH_TH1_MIN,
                                                            HDR20_MAC_SMOOTH_TH1_MAX));

        pUnpackedField->hdr_mac_smooth_dth_log2  =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pInputReserve->mac_smooth_dth_log2,
                                                            HDR20_MAC_SMOOTH_DTH_LOG2_MIN,
                                                            HDR20_MAC_SMOOTH_DTH_LOG2_MAX));

        // zigzag HDR recon additional parameters
        pUnpackedField->hdr_zrec_en               =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(HDRZigzagReconSel,
                                                            HDR_ZREC_EN_MIN,
                                                            HDR_ZREC_EN_MAX));

        pUnpackedField->hdr_zrec_pattern          = HDRZigzagReconPattern;

        pUnpackedField->hdr_zrec_first_rb_exp     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(HDRZigzagReconFirstRBExp,
                                                            HDR_ZREC_FIRST_RB_EXP_MIN,
                                                            HDR_ZREC_FIRST_RB_EXP_MAX));

        pUnpackedField->hdr_zrec_prefilt_tap0     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->hdr_zrec_prefilt_tap0),
                                                            HDR20_ZREC_PREFILT_TAP0_MIN,
                                                            HDR20_ZREC_PREFILT_TAP0_MAX));

        pUnpackedField->hdr_zrec_g_grad_th1       =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->hdr_zrec_g_grad_th1),
                                                            HDR20_ZREC_G_GRAD_TH1_MIN,
                                                            HDR20_ZREC_G_GRAD_TH1_MAX));

        pUnpackedField->hdr_zrec_g_grad_dth_log2  =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pInputReserve->hdr_zrec_g_grad_dth_log2,
                                                            HDR20_ZREC_G_GRAD_DTH_LOG2_MIN,
                                                            HDR20_ZREC_G_GRAD_DTH_LOG2_MAX));

        pUnpackedField->hdr_zrec_rb_grad_th1      =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->hdr_zrec_rb_grad_th1),
                                                            HDR20_ZREC_RB_GRAD_TH1_MIN,
                                                            HDR20_ZREC_RB_GRAD_TH1_MAX));

        pUnpackedField->hdr_zrec_rb_grad_dth_log2 =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pInputReserve->hdr_zrec_rb_grad_dth_log2,
                                                            HDR20_ZREC_RB_GRAD_DTH_LOG2_MIN,
                                                            HDR20_ZREC_RB_GRAD_DTH_LOG2_MAX));

        // black level
        pUnpackedField->hdr_black_in             =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32((BLSSOffset >> (PIPELINE_BITWIDTH - HDR20_OUTPUT_BIT_WIDTH)),
                                                            HDR20_BLACK_IN_MIN,
                                                            HDR20_BLACK_IN_MAX));

        pUnpackedField->hdr_black_out            =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32((pUnpackedField->hdr_black_in <<
                                                            (PIPELINE_BITWIDTH - HDR20_OUTPUT_BIT_WIDTH)),
                                                            HDR20_BLACK_OUT_MIN,
                                                            HDR20_BLACK_OUT_MAX)); ///< BLSS offset in 12 bit (HDR mode)

        // WB parameters
        /* WB 1.0 in HDR Auto/off use case only*/
        if (pUnpackedField->hdr_rec_linear_mode == 1 && pUnpackedField->hdr_mac_linear_mode == 1)
        {
            pUnpackedField->hdr_rg_wb_gain_ratio = 4096;
            pUnpackedField->hdr_bg_wb_gain_ratio = 4096;
            pUnpackedField->hdr_gb_wb_gain_ratio = 4096;
            pUnpackedField->hdr_gr_wb_gain_ratio = 4096;
        }
        else
        {
            if (FALSE == IQSettingUtils::FEqual(pInput->leftGGainWB, 0.0f))  // Divide by 0 check
            {
                pUnpackedField->hdr_rg_wb_gain_ratio =
                    IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber((pInput->leftRGainWB / pInput->leftGGainWB),
                                                                                QNumber_12U),
                                                HDR20_WB_GAIN_RATIO_MIN,
                                                HDR20_WB_GAIN_RATIO_MAX);

                pUnpackedField->hdr_bg_wb_gain_ratio =
                    IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber((pInput->leftBGainWB / pInput->leftGGainWB),
                                                                                QNumber_12U),
                                                HDR20_WB_GAIN_RATIO_MIN,
                                                HDR20_WB_GAIN_RATIO_MAX);
            }
            else
            {
                pUnpackedField->hdr_rg_wb_gain_ratio = HDR20_WB_GAIN_RATIO_MIN;
                pUnpackedField->hdr_bg_wb_gain_ratio = HDR20_WB_GAIN_RATIO_MIN;
            }

            if (FALSE == IQSettingUtils::FEqual(pInput->leftRGainWB, 0.0f))
            {
                pUnpackedField->hdr_gr_wb_gain_ratio =
                    IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber((pInput->leftGGainWB / pInput->leftRGainWB),
                                                                                QNumber_12U),
                                                HDR20_WB_GAIN_RATIO_MIN,
                                                HDR20_WB_GAIN_RATIO_MAX);
            }
            else
            {
                pUnpackedField->hdr_gr_wb_gain_ratio = HDR20_WB_GAIN_RATIO_MIN;
            }

            if (FALSE == IQSettingUtils::FEqual(pInput->leftBGainWB, 0.0f))
            {
                pUnpackedField->hdr_gb_wb_gain_ratio =
                    IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber((pInput->leftGGainWB / pInput->leftBGainWB),
                                                                                QNumber_12U),
                                                HDR20_WB_GAIN_RATIO_MIN,
                                                HDR20_WB_GAIN_RATIO_MAX);
            }
            else
            {
                pUnpackedField->hdr_gb_wb_gain_ratio = HDR20_WB_GAIN_RATIO_MIN;
            }
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }
    return result;
}
