// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  hdr23setting.h
/// @brief HDR23 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HDR23SETTING_H
#define HDR23SETTING_H
#include "hdr_2_3_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

// Const Min Max values for HDR23
static const UINT32 HDR23_PIXEL_PATTERN_MIN           = 0;
static const UINT32 HDR23_PIXEL_PATTERN_MAX           = 3;
static const UINT32 HDR23_RECON_MIN_FACTOR_MIN        = 0;
static const UINT32 HDR23_RECON_MIN_FACTOR_MAX        = 31;
static const UINT32 HDR23_RECON_FLAT_REGION_TH_MAX    = 1023;
static const UINT32 HDR23_RECON_FLAT_REGION_TH_MIN    = 0;
static const UINT32 HDR23_RECON_H_EDGE_TH1_MIN        = 0;
static const UINT32 HDR23_RECON_H_EDGE_TH1_MAX        = 1023;
static const UINT32 HDR23_RECON_H_EDGE_DTH_LOG2_MIN   = 4;
static const UINT32 HDR23_RECON_H_EDGE_DTH_LOG2_MAX   = 7;
static const UINT32 HDR23_RECON_MOTION_TH1_MIN        = 0;
static const UINT32 HDR23_RECON_MOTION_TH1_MAX        = 1023;
static const UINT32 HDR23_RECON_MOTION_DTH_LOG2_MIN   = 4;
static const UINT32 HDR23_RECON_MOTION_DTH_LOG2_MAX   = 7;
static const UINT32 HDR23_RECON_DARK_TH1_MIN          = 0;
static const UINT32 HDR23_RECON_DARK_TH1_MAX          = 1023;
static const UINT32 HDR23_RRECON_DARK_DTH_LOG2_MIN    = 0;
static const UINT32 HDR23_RRECON_DARK_DTH_LOG2_MAX    = 4;
static const UINT32 HDR23_HDR_ZREC_PREFILT_TAP0_MIN   = 0;
static const UINT32 HDR23_HDR_ZREC_PREFILT_TAP0_MAX   = 63;
static const UINT32 HDR23_HDR_ZREC_G_GRAD_TH1_MIN     = 0;
static const UINT32 HDR23_HDR_ZREC_G_GRAD_TH1_MAX     = 4095;
static const UINT32 HDR23_HDR_ZREC_RB_GRAD_TH1_MIN    = 0;
static const UINT32 HDR23_HDR_ZREC_RB_GRAD_TH1_MAX    = 4095;
static const UINT32 HDR23_MAC_MOTION0_TH1_MIN         = 0;
static const UINT32 HDR23_MAC_MOTION0_TH1_MAX         = 1023;
static const UINT32 HDR23_MAC_MOTION0_TH2_MIN         = 0;
static const UINT32 HDR23_MAC_MOTION0_TH2_MAX         = 255;
static const UINT32 HDR23_MAC_MOTION0_STRENGTH_MIN    = 0;
static const UINT32 HDR23_MAC_MOTION0_STRENGTH_MAX    = 16;
static const UINT32 HDR23_MAC_LOW_LIGHT_TH1_MIN       = 0;
static const UINT32 HDR23_MAC_LOW_LIGHT_TH1_MAX       = 16383; ///< ((1<<14)-1)
static const UINT32 HDR23_MAC_LOW_LIGHT_STRENGTH_MIN  = 0;
static const UINT32 HDR23_MAC_LOW_LIGHT_STRENGTH_MAX  = 16;
static const UINT32 HDR23_MAC_HIGH_LIGHT_TH1_MIN      = 0;
static const UINT32 HDR23_MAC_HIGH_LIGHT_TH1_MAX      = 16383; ///< ((1<<14)-1)
static const UINT32 HDR23_MAC_HIGH_LIGHT_DTH_LOG2_MIN = 2;
static const UINT32 HDR23_MAC_HIGH_LIGHT_DTH_LOG2_MAX = 14;

static const UINT32 HDR23_ZREC_PREFILT_TAP0_MIN       = 0;
static const UINT32 HDR23_ZREC_PREFILT_TAP0_MAX       = 64;
static const UINT32 HDR23_ZREC_G_GRAD_TH1_MIN         = 0;
static const UINT32 HDR23_ZREC_G_GRAD_TH1_MAX         = 4095; ///< MAX_UINT12
static const UINT32 HDR23_ZREC_RB_GRAD_TH1_MIN        = 0;
static const UINT32 HDR23_ZREC_RB_GRAD_TH1_MAX        = 4095; ///< MAX_UINT12

static const UINT32 HDR23_OUTPUT_BIT_WIDTH            = 10;
static const UINT32 HDR23_BLACK_IN_MIN                = 0;
static const UINT32 HDR23_BLACK_IN_MAX                = 255;
static const UINT32 HDR23_BLACK_OUT_MIN               = 0;
static const UINT32 HDR23_BLACK_OUT_MAX               = 4095;
static const UINT32 HDR23_ZREC_G_GRAD_DTH_LOG2_MIN    = 0;
static const UINT32 HDR23_ZREC_G_GRAD_DTH_LOG2_MAX    = 12;
static const UINT32 HDR23_ZREC_RB_GRAD_DTH_LOG2_MIN   = 0;
static const UINT32 HDR23_ZREC_RB_GRAD_DTH_LOG2_MAX   = 12;
static const UINT32 HDR23_RECON_EDGE_LPF_TAP0_MIN     = 0;
static const UINT32 HDR23_RECON_EDGE_LPF_TAP0_MAX     = 5;
static const UINT32 HDR23_MAC_MOTION_DILATION_MIN     = 0;
static const UINT32 HDR23_MAC_MOTION_DILATION_MAX     = 5;
static const UINT32 HDR23_WB_GAIN_RATIO_MIN           = 0;
static const UINT32 HDR23_WB_GAIN_RATIO_MAX           = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT32 HDR23_MAC_MOTION0_DT0_MIN         = 1;   ///< 1 in HPG
static const UINT32 HDR23_MAC_MOTION0_DT0_MAX         = 63;
static const UINT32 HDR23_MAC_LOW_LIGHT_DTH_LOG2_MIN  = 2;
static const UINT32 HDR23_MAC_LOW_LIGHT_DTH_LOG2_MAX  = 14;
static const UINT32 HDR23_MAC_SMOOTH_TH1_MIN          = 0;
static const UINT32 HDR23_MAC_SMOOTH_TH1_MAX          = 256;
static const UINT32 HDR23_MAC_SMOOTH_DTH_LOG2_MIN     = 2;
static const UINT32 HDR23_MAC_SMOOTH_DTH_LOG2_MAX     = 8;
static const UINT32 HDR23_MAC_SMOOTH_TAP0_MIN         = 0;
static const UINT32 HDR23_MAC_SMOOTH_TAP0_MAX         = 5;
static const UINT32 HDR23_EXP_RATIO_RECIP_MIN         = 16;
static const UINT32 HDR23_EXP_RATIO_RECIP_MAX         = 256;
static const UINT32 HDR23_MAC_SMOOTH_FILTER_MIN       = 0;
static const UINT32 HDR23_MAC_SMOOTH_FILTER_MAX       = 1;
static const UINT32 HDR23_MAC_SQRT_ANALOG_GAIN_MIN    = 16;
static const UINT32 HDR23_MAC_SQRT_ANALOG_GAIN_MAX    = 63;
static const UINT32 HDR23_ZREC_EN_MIN                 = 0;
static const UINT32 HDR23_ZREC_EN_MAX                 = 1;
static const UINT32 HDR23_ZREC_FIRST_RB_EXP_MIN       = 0;
static const UINT32 HDR23_ZREC_FIRST_RB_EXP_MAX       = 1;

/// @brief HDR23 unpacked data field
// NOWHINE NC004c: Share code with system team
struct HDR23UnpackedField
{
    // control, from VFE top level
    UINT16 hdr_recon_en;                          ///< HDR RECON block enable, bypass input->output when disabled
    UINT16 hdr_mac_en;                            ///< HDR MAC block enable, bypass input->output when disabled
                                                  ///< image info, VFE-global variables
    // parameters common to left/right panels
    UINT16 hdr_first_field;                       ///< 0~1, 1st line being 0=T1 (long) or 1=T2 (short)
    UINT16 hdr_MSB_align;                         ///< when 1 align to MSB by scaling outcome with 16/hdr_exp_ratio
    UINT16 hdr_rec_linear_mode;                   ///< when 1 and exp_ratio==1, T1 = input, T2 = 0
    UINT16 hdr_mac_linear_mode;                   ///< when 1 and exp_ratio==1, output = T1->wb->clip->inv_wb
    UINT16 hdr_exp_ratio;                         ///< 15uQ10, 1024~16384 representing 1.0~16.0
    UINT16 hdr_exp_ratio_recip;                   ///< 9uQ8, 16~256 representing 1/16~1.0, exp_ratio reciprocal
    UINT16 hdr_rec_edge_lpf_tap0;                 ///< 3uQ4, 0~5, 1st and 3rd tap of LPF before edge dir detection
    UINT16 hdr_mac_dilation;                      ///< 3u(0~5) motion dilation distance, upto 5=[-5,5]
    UINT16 hdr_mac_smooth_filter_en;              ///< boolean

    ///< WB parameters
    UINT32 hdr_rg_wb_gain_ratio;                  ///< 17uQ12, R gain vs G
    UINT32 hdr_bg_wb_gain_ratio;                  ///< 17uQ12, B gain vs G
    UINT32 hdr_gr_wb_gain_ratio;                  ///< 17uQ12, G gain vs R
    UINT32 hdr_gb_wb_gain_ratio;                  ///< 17uQ12, G gain vs B

    ///< HDR Reconstruction parameters
    UINT16 hdr_rec_flat_region_th;                ///< 10u, flat region threshold
    UINT16 hdr_rec_min_factor;                    ///< 5uQ4 (0~16), min to mid factor to force dir=0
    UINT16 hdr_rec_h_edge_th1;                    ///< 10u, horz edge threshold_1
    UINT16 hdr_rec_h_edge_dth_log2;               ///< 4u (4~8), log2(th2-th1)
    UINT16 hdr_rec_motion_th1;                    ///< 10u, motion threshold_1
    UINT16 hdr_rec_motion_dth_log2;               ///< 4u (4~8), log2(th2-th1)
    UINT16 hdr_rec_dark_th1;                      ///< 10u, dark level threshold_1
    UINT16 hdr_rec_dark_dth_log2;                 ///< 3u (0~4), log2(th2 - th1),

    ///<  HDR Motion Adaptive Combine parameters
    UINT16 hdr_mac_sqrt_analog_gain;              ///< 7uQ4 (16~90),  sqrt analog gain, gain <= 32.0,
                                                  ///< sqrt <= 5.66
    UINT16 hdr_mac_motion0_th1;                   ///< 10u, noise floor
    UINT16 hdr_mac_motion0_th2;                   ///< 8u, noise-luma slope
    UINT16 hdr_mac_motion0_dt0;                   ///< 6u (1~63), additive term to luma_from_noise
    UINT16 hdr_mac_motion_strength;               ///< 5uQ4 (0~16), motion adaptation strength
                                                  ///< was called motion_strength0
    UINT16 hdr_mac_lowlight_strength;             ///< 5uQ4 (0~16), lowlight adaptation strength
                                                  ///< was called motion_strength1
    UINT16 hdr_mac_lowlight_th1;                  ///< 14u, lowlight threshold_1
    UINT16 hdr_mac_lowlight_dth_log2;             ///< 4u (2~14), log2 of lowlight th2 - th1
    UINT16 hdr_mac_hilight_th1;                   ///< 14u, highlight threshold_1
    UINT16 hdr_mac_hilight_dth_log2;              ///< 4u (2~14), log2 of highlight th2 - th1
    UINT16 hdr_mac_smooth_tap0;                   ///< 3uQ4 (0~5), smooth filter 1st and 3rd tap
    UINT16 hdr_mac_smooth_th1;                    ///< 9u, smooth threshold_1
    UINT16 hdr_mac_smooth_dth_log2;               ///< 4u (2~8), log2 of smooth th2 - th1

    // zigzag HDR recon additional parameters
    UINT16 hdr_zrec_en;                           ///< Zigzag HDR RECON block enable
    UINT16 hdr_zrec_pattern;                      ///< 0~3, Bayer starting R1: 0=R1G1G2B1, 1=R1G1G2B2,
                                                  ///<                         2=R1G2G1B1, 3=R1G2G1B2
    UINT16 hdr_zrec_first_rb_exp;                 ///< 0~1, first R/B exposure 0=T1, 1=T2
    UINT16 hdr_zrec_prefilt_tap0;                 ///< 7u, lowpass prefilter side-taps, 0~64, 0 to turn off
    UINT16 hdr_zrec_g_grad_th1;                   ///< 12u, threshold_1 for G gradient
    UINT16 hdr_zrec_g_grad_dth_log2;              ///< 4u, log2 of th2 - th1 for G gradient, 0~12
    UINT16 hdr_zrec_rb_grad_th1;                  ///< 12u, threshold_1 for R/B gradient
    UINT16 hdr_zrec_rb_grad_dth_log2;             ///< 4u, log2 of th2 - th1 for R/B gradient, 0~12
    // black level
    UINT16 hdr_black_in;                          ///< 8u, black level input specific for HDR block
    UINT16 hdr_black_out;                         ///< 12u, black level on output (LSB-aligned)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements HDR23 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class IFEHDR23Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput       Pointer to the Input data
    /// @param  pData        Pointer to the interpolatin result
    /// @param  pReserveType Pointer to the Chromatix ReserveType data
    /// @param  pRegCmd      Pointer to the generated register settings
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const HDR23InputData*                   pInput,
        hdr_2_3_0::hdr23_rgn_dataType*          pData,
        hdr_2_3_0::chromatix_hdr23_reserveType* pReserveType,
        VOID*                                   pRegCmd);
};
#endif // HDR23SETTING_H
