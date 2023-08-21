// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  hdr30setting.h
/// @brief HDR30 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HDR30SETTING_H
#define HDR30SETTING_H
#include "bc10setting.h"
#include "hdr_3_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const UINT16 HDR30_ZREC_FIRST_RB_EXP_MIN         = 0;
static const UINT16 HDR30_ZREC_FIRST_RB_EXP_MAX         = 1;
static const UINT16 HDR30_BLACK_IN_MIN                  = 0;
static const UINT16 HDR30_BLACK_IN_MAX                  = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(10));
static const UINT16 HDR30_LONG_EXP_BLACK_IN_MIN         = 0;
static const UINT16 HDR30_LONG_EXP_BLACK_IN_MAX         = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(10));
static const UINT16 HDR30_EXP_RATIO_MIN                 = 256;
static const UINT16 HDR30_EXP_RATIO_MAX                 = 4096;
static const UINT16 HDR30_EXP_RATIO_RECIP_MIN           = 256;
static const UINT16 HDR30_EXP_RATIO_RECIP_MAX           = 4096;
static const UINT16 HDR30_LONG_EXP_SATURATION_MIN       = 0;
static const UINT16 HDR30_LONG_EXP_SATURATION_MAX       = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT32 HDR30_RG_WB_GAIN_RATIO_MIN          = 0;
static const UINT32 HDR30_RG_WB_GAIN_RATIO_MAX          = IQSettingUtils::MAXUINTBITFIELD(13);
static const UINT32 HDR30_BG_WB_GAIN_RATIO_MIN          = 0;
static const UINT32 HDR30_BG_WB_GAIN_RATIO_MAX          = IQSettingUtils::MAXUINTBITFIELD(13);
static const UINT32 HDR30_GR_WB_GAIN_RATIO_MIN          = 0;
static const UINT32 HDR30_GR_WB_GAIN_RATIO_MAX          = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT32 HDR30_GB_WB_GAIN_RATIO_MIN          = 0;
static const UINT32 HDR30_GB_WB_GAIN_RATIO_MAX          = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT16 HDR30_MAC_HFS_ACT_WEIGHT_MIN        = 0;
static const UINT16 HDR30_MAC_HFS_ACT_WEIGHT_MAX        = 16;
static const UINT16 HDR30_MAC_HFS_ACT_TH2_MIN           = 0;
static const UINT16 HDR30_MAC_HFS_ACT_TH2_MAX           = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 HDR30_MAC_HFS_ACT_DTH_LOG2_MIN      = 2;
static const UINT16 HDR30_MAC_HFS_ACT_DTH_LOG2_MAX      = 14;
static const UINT16 HDR30_MAC_STATIC_TH2_MIN            = 0;
static const UINT16 HDR30_MAC_STATIC_TH2_MAX            = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 HDR30_MAC_STATIC_DTH_LOG2_MIN       = 2;
static const UINT16 HDR30_MAC_STATIC_DTH_LOG2_MAX       = 14;
static const UINT16 HDR30_MAC_HIGHLIGHT_STRENGTH_MIN    = 0;
static const UINT16 HDR30_MAC_HIGHLIGHT_STRENGTH_MAX    = 16;
static const UINT16 HDR_MAC_HIGHLIGHT_DTH_LOG2_MIN      = 2;
static const UINT16 HDR_MAC_HIGHLIGHT_DTH_LOG2_MAX      = 14;
static const UINT16 HDR30_MAC_LOW_LIGHT_STRENGTH_MIN    = 0;
static const UINT16 HDR30_MAC_LOW_LIGHT_STRENGTH_MAX    = 16;
static const UINT16 HDR30_MAC_DARK_TH2_MIN              = 0;
static const UINT16 HDR30_MAC_DARK_TH2_MAX              = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 HDR30_MAC_LOW_LIGHT_DTH_LOG2_MIN    = 2;
static const UINT16 HDR30_MAC_LOW_LIGHT_DTH_LOG2_MAX    = 14;
static const UINT16 HDR30_MAC_DILATION_MIN              = 0;
static const UINT16 HDR30_MAC_DILATION_MAX              = 5;
static const UINT16 HDR30_MAC_MOTION_SCALE_MIN          = 0;
static const UINT16 HDR30_MAC_MOTION_SCALE_MAX          = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(6));
static const UINT16 HDR30_MAC_MOTION0_DT0_MIN           = 0;
static const UINT16 HDR30_MAC_MOTION0_DT0_MAX           = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(8));
static const UINT16 HDR30_MAC_MOTION_STRICTNESS_MIN     = 0;
static const UINT16 HDR30_MAC_MOTION_STRICTNESS_MAX     = 16;
static const UINT16 HDR30_MAC_MOTION_TH1_MIN            = 0;
static const UINT16 HDR30_MAC_MOTION_TH1_MAX            = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 HDR_MAC_MOTION_DTH_LOG2_MIN         = 2;
static const UINT16 HDR_MAC_MOTION_DTH_LOG2_MAX         = 14;
static const UINT16 HDR_MAC_MOTION_G2_W_MIN_MIN         = 0;
static const UINT16 HDR_MAC_MOTION_G2_W_MIN_MAX         = 16;
static const UINT16 HDR_MAC_MOTION_G2_W_MAX_MIN         = 0;
static const UINT16 HDR_MAC_MOTION_G2_W_MAX_MAX         = 16;
static const UINT16 HDR30_REC_G_GRAD_TH1_MIN            = 0;
static const UINT16 HDR30_REC_G_GRAD_TH1_MAX            = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(12));
static const UINT16 HDR30_REC_G_GRAD_DTH_LOG2_MIN       = 0;
static const UINT16 HDR30_REC_G_GRAD_DTH_LOG2_MAX       = 12;
static const UINT16 HDR30_REC_RB_GRAD_TH1_MIN           = 0;
static const UINT16 HDR30_REC_RB_GRAD_TH1_MAX           = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(12));
static const UINT16 HDR30_REC_RB_GRAD_DTH_LOG2_MIN      = 0;
static const UINT16 HDR30_REC_RB_GRAD_DTH_LOG2_MAX      = 12;
static const UINT16 HDR30_REC_HL_DETAIL_POSITIVE_W_MIN  = 0;
static const UINT16 HDR30_REC_HL_DETAIL_POSITIVE_W_MAX  = 16;
static const UINT16 HDR30_REC_HL_DETAIL_NEGATIVE_W_MIN  = 0;
static const UINT16 HDR30_REC_HL_DETAIL_NEGATIVE_W_MAX  = 16;
static const UINT16 HDR30_REC_HL_DETAIL_TH_W_MIN        = 0;
static const UINT16 HDR30_REC_HL_DETAIL_TH_W_MAX        = 16;
static const UINT16 HDR30_REC_HL_MOTION_TH_LOG2_MIN     = 0;
static const UINT16 HDR30_REC_HL_MOTION_TH_LOG2_MAX     = 12;
static const UINT16 HDR30_REC_EDGE_SCALE_MIN            = 0;
static const UINT16 HDR30_REC_EDGE_SCALE_MAX            = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(6));
static const UINT16 HDR30_REC_EDGE_SHORT_EXP_W_MIN      = 0;
static const UINT16 HDR30_REC_EDGE_SHORT_EXP_W_MAX      = 16;
static const UINT16 HDR30_REC_EDGE_TH1_MIN              = 0;
static const UINT16 HDR30_REC_EDGE_TH1_MAX              = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 HDR30_REC_EDGE_DTH_LOG2_MIN         = 2;
static const UINT16 HDR30_REC_EDGE_DTH_LOG2_MAX         = 14;
static const UINT16 HDR_REC_BLENDING_W_MIN_MIN          = 0;
static const UINT16 HDR_REC_BLENDING_W_MIN_MAX          = 16;
static const UINT16 HDR_REC_BLENDING_W_MAX_MIN          = 0;
static const UINT16 HDR_REC_BLENDING_W_MAX_MAX          = 16;
static const UINT16 HDR30_REC_DETAIL_TH_W_MIN           = 0;
static const UINT16 HDR30_REC_DETAIL_TH_W_MAX           = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(6));
static const UINT16 HDR30_REC_DETAIL_CORING_STRENGTH_MIN = 0;
static const UINT16 HDR30_REC_DETAIL_CORING_STRENGTH_MAX = 16;
static const UINT16 HDR30_REC_DETAIL_CORING_TH2_MIN     = 0;
static const UINT16 HDR30_REC_DETAIL_CORING_TH2_MAX     = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT32 HDR30_REC_DETAIL_CORING_DTH_LOG2_MIN = 2;
static const UINT32 HDR30_REC_DETAIL_CORING_DTH_LOG2_MAX = 14;

// @brief HDR30 unpacked data field
// NOWHINE NC004c: Share code with system team
struct HDR30UnpackedField
{
    UINT16 enable;                         ///< HDR block enable, bypass input->output when disabled
    UINT16 hdr_lsb_align;                  ///< when 1 align to LSB by scaling outcome with hdr_exp_ratio_recip
    UINT16 hdr_linear_mode;                ///< input -> wb -> inv wb -> output (alignment depends on hdr_lsb_align)

    UINT16 bayer_pattern;                  ///< 1st bayer being R/Gr/Gb/B, 0=R, 1=Gr, 2=B, 3=Gb
    UINT16 hdr_zrec_pattern;               ///< Bayer starting R1: 0=R1G1G2B1, 1=R1G1G2B2, 2=R1G2G1B1, 3=R1G2G1B2
    UINT16 hdr_zrec_first_rb_exp;          ///< first R/B exposure 0=T1, 1=T2

    UINT16 hdr_black_in;                   ///< Output black level = hdr_black_in * 16.
    UINT16 hdr_long_exp_black_in;          ///< In case long exposure black is different than short exposure
    UINT16 hdr_exp_ratio;                  ///< 4096 representing 1.0~16.0
    UINT16 hdr_exp_ratio_recip;            ///< 1/exp_ratio
    UINT16 hdr_long_exp_saturation;        ///< long exposure saturation value

    UINT16 hdr_rg_wb_gain_ratio;           ///< rg wb gain ratio
    UINT32 hdr_gr_wb_gain_ratio;           ///< gr wb gain ratio
    UINT16 hdr_bg_wb_gain_ratio;           ///< bg wb gain ratio
    UINT32 hdr_gb_wb_gain_ratio;           ///< gb wb gain ratio

    UINT16 hdr_mac_hfs_act_weight;         ///< weight between hfs_act & edge map
    UINT16 hdr_mac_hfs_act_th2;            ///< large than th2, high HFS activity
    UINT16 hdr_mac_hfs_act_dth_log2;       ///< HDR Mac HFS Act Dth log2
    UINT16 hdr_mac_static_th2;             ///< larger than th2, non-static region
    UINT16 hdr_mac_static_dth_log2;        ///< HDR Mac static Dth log2

    UINT16 hdr_mac_highlight_strength;     ///< MAC Highlight strength
    UINT16 hdr_mac_highlight_dth_log2;     ///< log2 of long exposure saturation value - th1
    UINT16 hdr_mac_lowlight_strength;      ///< lowlight adaptation strength
    UINT16 hdr_mac_dark_th2;               ///< lowlight threshold_2
    UINT16 hdr_mac_lowlight_dth_log2;      ///< log2 of lowlight th2 - th1

    UINT16 hdr_mac_dilation;               ///< motion dilation distance
    UINT16 hdr_mac_motion_scale;           ///< motion value scaling factor
    UINT16 hdr_mac_motion_dt0;             ///< motion dt0
    UINT16 hdr_mac_motion_strictness;      ///< motion strictness weight

    UINT16 hdr_mac_motion_th1;             ///< motion th1
    UINT16 hdr_mac_motion_dth_log2;        ///< motion dth log2
    UINT16 hdr_mac_motion_g2_w_min;        ///< static area T2 weight
    UINT16 hdr_mac_motion_g2_w_max;        ///< moving area T2 weight

    UINT16 hdr_rec_g_grad_th1;             ///< threshold_1 for G gradient
    UINT16 hdr_rec_g_grad_dth_log2;        ///< log2 of th2 - th1 for G gradient
    UINT16 hdr_rec_rb_grad_th1;            ///< threshold_1 for R/B gradient
    UINT16 hdr_rec_rb_grad_dth_log2;       ///< log2 of th2 - th1 for R/B gradient

    UINT16 hdr_rec_hl_detail_positive_w;   ///< REC hl positive w
    UINT16 hdr_rec_hl_detail_negative_w;   ///< REC hl negative w
    UINT16 hdr_rec_hl_detail_th_w;         ///< REC hl th w
    UINT16 hdr_rec_hl_motion_th_log2;      ///< log2 of intensity/threshold ratio, default to 2 (threshold = 1/4 * intensity)

    UINT16 hdr_rec_edge_scale;             ///< edge value scaling factor
    UINT16 hdr_rec_edge_short_exp_w;       ///< weight of short exposure edge map

    UINT16 hdr_rec_edge_th1;               ///< less than th1, use blending_w_min. Larger than th2, use blending_w_max
    UINT16 hdr_rec_edge_dth_log2;          ///< REC edge dth log2
    UINT16 hdr_rec_blending_w_min;         ///< REC blending W Min
    UINT16 hdr_rec_blending_w_max;         ///< REC blending W Max

    UINT16 hdr_rec_detail_th_w;            ///< detail threshold scaling factor
    UINT16 hdr_rec_detail_coring_strength; ///< detail coring strength
    UINT16 hdr_rec_detail_coring_th2;      ///< larger than th2, no coring
    UINT16 hdr_rec_detail_coring_dth_log2; ///< detail coring dth log2
};

// NOWHINE NC004c: Share code with system team
struct HDR30BC10UnpackedField
{
    HDR30UnpackedField* pUnpackedRegisterDataHDR30; ///< HDR3.0 data
    BC10UnpackedField*  pUnpackedRegisterDataBC10;  ///< Bin correction 1.0 data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements HDR30 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class HDR30Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the input data
    /// @param  pData          Pointer to the interpolation result
    /// @param  pReserveType   Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable  Pointer to the variable(s) to enable this module
    /// @param  pUnpackedField Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const HDR30InputData*                                 pInput,
        hdr_3_0_0::hdr30_rgn_dataType*                        pData,
        hdr_3_0_0::chromatix_hdr30_reserveType*               pReserveType,
        hdr_3_0_0::chromatix_hdr30Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pUnpackedField);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ScaleVal
    ///
    /// @brief  Calculate the scale value based on black level
    ///
    /// @param  input               Input value
    /// @param  scalingQ10          the scaling Q 10
    /// @param  min                 Minimum value
    /// @param  max                 Maximum value
    /// @param  blackLevel          Black level value
    ///
    /// @return round and clamp value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT16 ScaleVal(
        INT16  input,
        UINT16 scalingQ10,
        INT16  min,
        INT16  max,
        INT16  blackLevel);
};
#endif // HDR30SETTING_H
