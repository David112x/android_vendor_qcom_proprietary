// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  hdr30setting.cpp
/// @brief HDR30 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "hdr30setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDR30Setting::ScaleVal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT16 HDR30Setting::ScaleVal(
    INT16   inputValue,
    UINT16  scalingQ10,
    INT16   minValue,
    INT16   maxValue,
    INT16   blackLevel)
{
    // scale values
    FLOAT fVal        = static_cast<FLOAT>(scalingQ10) / 1024.f * (inputValue - blackLevel) + blackLevel;

    // clamp based on min and max values
    INT16 outputValue = IQSettingUtils::ClampUINT16(IQSettingUtils::RoundFLOAT(fVal), minValue, maxValue);

    return outputValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HDR30Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HDR30Setting::CalculateHWSetting(
    const HDR30InputData*                                 pInput,
    hdr_3_0_0::hdr30_rgn_dataType*                        pData,
    hdr_3_0_0::chromatix_hdr30_reserveType*               pReserveType,
    hdr_3_0_0::chromatix_hdr30Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    BOOL   result                   = TRUE;
    UINT32 BLSSOffset               = 0;
    UINT16 HDRZigZagReconSel        = 0;
    UINT16 HDRZigzagReconPattern    = 0;
    UINT16 HDRZigzagReconFirstRBExp = 0;
    FLOAT  HDRExpSensitivityRatio   = 1.0f;
    FLOAT  totalGainSqrt            = 1.0f;

    hdr_3_0_0::chromatix_hdr30_reserveType* pInputReserve;

    if ((NULL != pInput)           &&
        (NULL != pData)            &&
        (NULL != pOutput)          &&
        (NULL != pReserveType)     &&
        (NULL != pModuleEnable)    &&
        (NULL != pInput->pChromatix))
    {
        HDR30UnpackedField* pUnpackedField          = static_cast<HDR30UnpackedField*>(pOutput);
        pInputReserve                               = pReserveType;
        BLSSOffset                                  = pInput->blackLevelOffset;
        HDRExpSensitivityRatio                      = pInput->AECSensitivity;
        HDRZigzagReconFirstRBExp                    = pInput->ZZHDRFirstRBEXP;
        HDRZigZagReconSel                           = pInput->ZZHDRMode;
        HDRZigzagReconPattern                       = pInput->ZZHDRPattern;

        // parameters common to left/right panels
        pUnpackedField->enable                      = static_cast<UINT16>(pModuleEnable->hdr_enable);
        pUnpackedField->hdr_lsb_align               = static_cast<UINT16>(pInputReserve->hdr_lsb_align);
        pUnpackedField->hdr_linear_mode             = static_cast<UINT16>(pInputReserve->hdr_linear_mode);

        pUnpackedField->bayer_pattern               = static_cast<UINT16>(pInput->bayerPattern);
        pUnpackedField->hdr_zrec_pattern            = HDRZigzagReconPattern;
        pUnpackedField->hdr_zrec_first_rb_exp       = static_cast<UINT16>(IQSettingUtils::ClampUINT16(
                                                                          HDRZigzagReconFirstRBExp,
                                                                          HDR30_ZREC_FIRST_RB_EXP_MIN,
                                                                          HDR30_ZREC_FIRST_RB_EXP_MAX));
        // black level
        pUnpackedField->hdr_black_in                =
            static_cast<UINT16>(IQSettingUtils::ClampUINT16(IQSettingUtils::RoundFLOAT(
                                                           (static_cast<FLOAT>(BLSSOffset) / 16)),
                                                            HDR30_BLACK_IN_MIN,
                                                            HDR30_BLACK_IN_MAX)); // BLSS offset in 10 bit (Non-HDR mode)

        pUnpackedField->hdr_long_exp_black_in       = pUnpackedField->hdr_black_in;

        pUnpackedField->hdr_exp_ratio               =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                            HDRExpSensitivityRatio, QNumber_8U),
                                                            HDR30_EXP_RATIO_MIN,
                                                            HDR30_EXP_RATIO_MAX));

        if (FALSE == IQSettingUtils::FEqual(HDRExpSensitivityRatio, 0.0f))
        {
            pUnpackedField->hdr_exp_ratio_recip     =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(
                                                                FLOATMAX12 / HDRExpSensitivityRatio),
                                                                HDR30_EXP_RATIO_RECIP_MIN,
                                                                HDR30_EXP_RATIO_RECIP_MAX));
        }
        else
        {
            pUnpackedField->hdr_exp_ratio_recip     = HDR30_EXP_RATIO_RECIP_MAX;
        }

        UINT16 multi_factor_inv_Q10 = static_cast<UINT16>(1024.0f * 256.0f / pUnpackedField->hdr_exp_ratio  * 0.99f);
        INT16 val                   = ScaleVal(HDR30_LONG_EXP_SATURATION_MAX,
                                               multi_factor_inv_Q10,
                                               HDR30_LONG_EXP_SATURATION_MIN,
                                               HDR30_LONG_EXP_SATURATION_MAX,
                                               BLSSOffset);

        pUnpackedField->hdr_long_exp_saturation = IQSettingUtils::ClampUINT16(val,
                                                                              HDR30_LONG_EXP_SATURATION_MIN,
                                                                              HDR30_LONG_EXP_SATURATION_MAX);

         // WB parameters
        if (FALSE == IQSettingUtils::FEqual(pInput->leftGGainWB, 0.0f))
        {
            pUnpackedField->hdr_rg_wb_gain_ratio    =
                IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                           (pInput->leftRGainWB / pInput->leftGGainWB), QNumber_8U),
                                            HDR30_RG_WB_GAIN_RATIO_MIN,
                                            HDR30_RG_WB_GAIN_RATIO_MAX);

            pUnpackedField->hdr_bg_wb_gain_ratio    =
                IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                            (pInput->leftBGainWB / pInput->leftGGainWB), QNumber_8U),
                                            HDR30_BG_WB_GAIN_RATIO_MIN,
                                            HDR30_BG_WB_GAIN_RATIO_MAX);
        }
        else
        {
            pUnpackedField->hdr_rg_wb_gain_ratio    = HDR30_RG_WB_GAIN_RATIO_MIN;
            pUnpackedField->hdr_bg_wb_gain_ratio    = HDR30_BG_WB_GAIN_RATIO_MIN;
        }

        if (FALSE == IQSettingUtils::FEqual(pInput->leftRGainWB, 0.0f))
        {
            pUnpackedField->hdr_gr_wb_gain_ratio    =
            IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                       (pInput->leftGGainWB /pInput->leftRGainWB), QNumber_12U),
                                        HDR30_GR_WB_GAIN_RATIO_MIN,
                                        HDR30_GR_WB_GAIN_RATIO_MAX);
        }
        else
        {
            pUnpackedField->hdr_gr_wb_gain_ratio    = HDR30_GR_WB_GAIN_RATIO_MIN;
        }

        if (FALSE == IQSettingUtils::FEqual(pInput->leftBGainWB, 0.0f))
        {
            pUnpackedField->hdr_gb_wb_gain_ratio    =
            IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                       (pInput->leftGGainWB /pInput->leftBGainWB), QNumber_12U),
                                        HDR30_GB_WB_GAIN_RATIO_MIN,
                                        HDR30_GB_WB_GAIN_RATIO_MAX);
        }
        else
        {
            pUnpackedField->hdr_gb_wb_gain_ratio    = HDR30_GB_WB_GAIN_RATIO_MIN;
        }

        pUnpackedField->hdr_mac_hfs_act_weight      =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->hdr_mac_hfs_act_weight,
                                                            HDR30_MAC_HFS_ACT_WEIGHT_MIN,
                                                            HDR30_MAC_HFS_ACT_WEIGHT_MAX));

        pUnpackedField->hdr_mac_hfs_act_th2         =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->hdr_mac_hfs_act_th2,
                                                            HDR30_MAC_HFS_ACT_TH2_MIN,
                                                            HDR30_MAC_HFS_ACT_TH2_MAX));

        pUnpackedField->hdr_mac_hfs_act_dth_log2    =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->hdr_mac_hfs_act_dth_log2,
                                                            HDR30_MAC_HFS_ACT_DTH_LOG2_MIN,
                                                            HDR30_MAC_HFS_ACT_DTH_LOG2_MAX));

        pUnpackedField->hdr_mac_static_th2          =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_mac_static_th2,
                                                            HDR30_MAC_STATIC_TH2_MIN,
                                                            HDR30_MAC_STATIC_TH2_MAX));

        pUnpackedField->hdr_mac_static_dth_log2     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_mac_static_dth_log2,
                                                            HDR30_MAC_STATIC_DTH_LOG2_MIN,
                                                            HDR30_MAC_STATIC_DTH_LOG2_MAX));

        pUnpackedField->hdr_mac_highlight_strength  =
             static_cast<UINT16>(IQSettingUtils::ClampUINT16(IQSettingUtils::RoundFLOAT(
                                                            pData->hdr_mac_highlight_strength),
                                                            HDR30_MAC_HIGHLIGHT_STRENGTH_MIN,
                                                            HDR30_MAC_HIGHLIGHT_STRENGTH_MAX));

        UINT32 hdr_mac_highlight_dth_log2;
        hdr_mac_highlight_dth_log2 = IQSettingUtils::RoundFLOAT(
            log2f(((pData->hdr_mac_highlight_dth_log2 - 2) * pUnpackedField->hdr_long_exp_saturation) / 64));

        pUnpackedField->hdr_mac_highlight_dth_log2  =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(hdr_mac_highlight_dth_log2,
                                                            HDR_MAC_HIGHLIGHT_DTH_LOG2_MIN,
                                                            HDR_MAC_HIGHLIGHT_DTH_LOG2_MAX));

        pUnpackedField->hdr_mac_lowlight_strength   =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_mac_lowlight_strength,
                                                            HDR30_MAC_LOW_LIGHT_STRENGTH_MIN,
                                                            HDR30_MAC_LOW_LIGHT_STRENGTH_MAX));

        pUnpackedField->hdr_mac_dark_th2            =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_mac_dark_th2,
                                                            HDR30_MAC_DARK_TH2_MIN,
                                                            HDR30_MAC_DARK_TH2_MAX));

        pUnpackedField->hdr_mac_lowlight_dth_log2   =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_mac_lowlight_dth_log2,
                                                            HDR30_MAC_LOW_LIGHT_DTH_LOG2_MIN,
                                                            HDR30_MAC_LOW_LIGHT_DTH_LOG2_MAX));

        pUnpackedField->hdr_mac_dilation            =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->hdr_mac_dilation),
                                                            HDR30_MAC_DILATION_MIN,
                                                            HDR30_MAC_DILATION_MAX));

        pUnpackedField->hdr_mac_motion_scale        =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pInputReserve->hdr_mac_motion_scale),
                                                            HDR30_MAC_MOTION_SCALE_MIN,
                                                            HDR30_MAC_MOTION_SCALE_MAX));

        pUnpackedField->hdr_mac_motion_dt0          =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->hdr_mac_motion_dt0),
                                                            HDR30_MAC_MOTION0_DT0_MIN,
                                                            HDR30_MAC_MOTION0_DT0_MAX));

        pUnpackedField->hdr_mac_motion_strictness   =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_mac_motion_strictness,
                                                            HDR30_MAC_MOTION_STRICTNESS_MIN,
                                                            HDR30_MAC_MOTION_STRICTNESS_MAX));

        pUnpackedField->hdr_mac_motion_th1          =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pReserveType->hdr_mac_motion_th1),
                                                            HDR30_MAC_MOTION_TH1_MIN,
                                                            HDR30_MAC_MOTION_TH1_MAX));

        pUnpackedField->hdr_mac_motion_dth_log2     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pReserveType->hdr_mac_motion_dth_log2),
                                                            HDR_MAC_MOTION_DTH_LOG2_MIN,
                                                            HDR_MAC_MOTION_DTH_LOG2_MAX));

        pUnpackedField->hdr_mac_motion_g2_w_min     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pReserveType->hdr_mac_motion_g2_w_min),
                                                            HDR_MAC_MOTION_G2_W_MIN_MIN,
                                                            HDR_MAC_MOTION_G2_W_MIN_MAX));

        pUnpackedField->hdr_mac_motion_g2_w_max     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pReserveType->hdr_mac_motion_g2_w_max),
                                                            HDR_MAC_MOTION_G2_W_MAX_MIN,
                                                            HDR_MAC_MOTION_G2_W_MAX_MAX));

        pUnpackedField->hdr_rec_g_grad_th1          =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pReserveType->hdr_rec_g_grad_th1),
                                                            HDR30_REC_G_GRAD_TH1_MIN,
                                                            HDR30_REC_G_GRAD_TH1_MAX));

        pUnpackedField->hdr_rec_g_grad_dth_log2     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->hdr_rec_g_grad_dth_log2,
                                                            HDR30_REC_G_GRAD_DTH_LOG2_MIN,
                                                            HDR30_REC_G_GRAD_DTH_LOG2_MAX));

        pUnpackedField->hdr_rec_rb_grad_th1         =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->hdr_rec_rb_grad_th1,
                                                            HDR30_REC_RB_GRAD_TH1_MIN,
                                                            HDR30_REC_RB_GRAD_TH1_MAX));

        pUnpackedField->hdr_rec_rb_grad_dth_log2    =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->hdr_rec_rb_grad_dth_log2,
                                                            HDR30_REC_RB_GRAD_DTH_LOG2_MIN,
                                                            HDR30_REC_RB_GRAD_DTH_LOG2_MAX));

        pUnpackedField->hdr_rec_hl_detail_positive_w =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_rec_hl_detail_positive_w,
                                                            HDR30_REC_HL_DETAIL_POSITIVE_W_MIN,
                                                            HDR30_REC_HL_DETAIL_POSITIVE_W_MAX));

        pUnpackedField->hdr_rec_hl_detail_negative_w =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_rec_hl_detail_negative_w,
                                                            HDR30_REC_HL_DETAIL_NEGATIVE_W_MIN,
                                                            HDR30_REC_HL_DETAIL_NEGATIVE_W_MAX));

        pUnpackedField->hdr_rec_hl_detail_th_w      =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_rec_hl_detail_th_w,
                                                            HDR30_REC_HL_DETAIL_TH_W_MIN,
                                                            HDR30_REC_HL_DETAIL_TH_W_MAX));

        pUnpackedField->hdr_rec_hl_motion_th_log2   =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_rec_hl_motion_th_log2,
                                                            HDR30_REC_HL_MOTION_TH_LOG2_MIN,
                                                            HDR30_REC_HL_MOTION_TH_LOG2_MAX));

        pUnpackedField->hdr_rec_edge_scale          =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_rec_edge_scale,
                                                            HDR30_REC_EDGE_SCALE_MIN,
                                                            HDR30_REC_EDGE_SCALE_MAX));

        pUnpackedField->hdr_rec_edge_short_exp_w    =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_rec_edge_short_exp_w,
                                                            HDR30_REC_EDGE_SHORT_EXP_W_MIN,
                                                            HDR30_REC_EDGE_SHORT_EXP_W_MAX));

        pUnpackedField->hdr_rec_edge_th1            =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_rec_edge_th1,
                                                            HDR30_REC_EDGE_TH1_MIN,
                                                            HDR30_REC_EDGE_TH1_MAX));

        pUnpackedField->hdr_rec_edge_dth_log2       =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_rec_edge_dth_log2,
                                                            HDR30_REC_EDGE_DTH_LOG2_MIN,
                                                            HDR30_REC_EDGE_DTH_LOG2_MAX));

        pUnpackedField->hdr_rec_blending_w_min      =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->hdr_rec_blending_w_min),
                                                            HDR_REC_BLENDING_W_MIN_MIN,
                                                            HDR_REC_BLENDING_W_MIN_MAX));

        pUnpackedField->hdr_rec_blending_w_max      =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->hdr_rec_blending_w_max),
                                                            HDR_REC_BLENDING_W_MAX_MIN,
                                                            HDR_REC_BLENDING_W_MAX_MAX));

        pUnpackedField->hdr_rec_detail_th_w         =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_rec_detail_th_w,
                                                            HDR30_REC_DETAIL_TH_W_MIN,
                                                            HDR30_REC_DETAIL_TH_W_MAX));

        pUnpackedField->hdr_rec_detail_coring_strength =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_rec_detail_coring_strength,
                                                            HDR30_REC_DETAIL_CORING_STRENGTH_MIN,
                                                            HDR30_REC_DETAIL_CORING_STRENGTH_MAX));

        pUnpackedField->hdr_rec_detail_coring_th2   =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_rec_detail_coring_th2,
                                                            HDR30_REC_DETAIL_CORING_TH2_MIN,
                                                            HDR30_REC_DETAIL_CORING_TH2_MAX));

        pUnpackedField->hdr_rec_detail_coring_dth_log2 =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(pData->hdr_rec_detail_coring_dth_log2,
                                                            HDR30_REC_DETAIL_CORING_DTH_LOG2_MIN,
                                                            HDR30_REC_DETAIL_CORING_DTH_LOG2_MAX));
    }
    else
    {
        result = FALSE;
    }
    return result;
}
