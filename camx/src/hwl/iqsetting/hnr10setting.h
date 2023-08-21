// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  hnr10setting.h
/// @brief HNR10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HNR10SETTING_H
#define HNR10SETTING_H

#include "hnr_1_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

// HNR global parameters and size constants
static const INT32 HNR_V10_MAX_FILENAME_LEN      = 512;
static const INT32 HNR_V10_MAX_ARR_NUM           = 64;
static const INT32 HNR_V10_MAX_FACE_NUM          = 5;
static const INT32 HNR_V10_LNR_ARR_NUM           = 33;
static const INT32 HNR_V10_LNR_ARR_INTVAL        = 8;
static const INT32 HNR_V10_FNR_ARR_NUM           = 17;
static const INT32 HNR_V10_FNR_ARR_INTVAL        = 8;
static const INT32 HNR_V10_FNR_AC_ARR_NUM        = 17;
static const INT32 HNR_V10_FNR_AC_ARR_INTVAL     = 16;
static const INT32 HNR_V10_SNR_ARR_NUM           = 17;
static const INT32 HNR_V10_RNR_ARR_NUM           = 6;
static const INT32 HNR_V10_NR_ARR_NUM            = 33;
static const INT32 HNR_V10_NR_ARR_INTVAL         = 8;
static const INT32 HNR_V10_BLEND_LNR_ARR_NUM     = 17;
static const INT32 HNR_V10_BLEND_LNR_ARR_INTVAL  = 16;
static const INT32 HNR_V10_BLEND_SNR_ARR_NUM     = 17;
static const INT32 HNR_V10_FNR_GAIN_SHIFT_IN_ARR = 16;
static const INT32 HNR_V10_CNR_ARR_NUM           = 25;

// Const Min Max values for HNR10
static const INT32 HNR10_LNR_GAIN_LUT_QFACTOR    = 5;
static const INT32 HNR10_LNR_GAIN_LUT_MIN        = 0;
static const INT32 HNR10_LNR_GAIN_LUT_MAX        = 63;

static const INT32 HNR10_LNR_SHIFT_MIN           = 0;
static const INT32 HNR10_LNR_SHIFT_MAX           = 3;

static const INT32 HNR10_CNR_GAIN_LUT_QFACTOR    = 5;
static const INT32 HNR10_CNR_GAIN_LUT_MIN        = 0;
static const INT32 HNR10_CNR_GAIN_LUT_MAX        = 63;

static const INT32 HNR10_CNR_LOW_THRD_U_MIN      = -128;
static const INT32 HNR10_CNR_LOW_THRD_U_MAX      = 127;
static const INT32 HNR10_CNR_THRD_GAP_U_MIN      = 0;
static const INT32 HNR10_CNR_THRD_GAP_U_MAX      = 7;
static const INT32 HNR10_CNR_LOW_THRD_V_MIN      = -128;
static const INT32 HNR10_CNR_LOW_THRD_V_MAX      = 127;
static const INT32 HNR10_CNR_THRD_GAP_V_MIN      = 0;
static const INT32 HNR10_CNR_THRD_GAP_V_MAX      = 7;
static const INT32 HNR10_CNR_ADJ_GAIN_MIN        = 0;
static const INT32 HNR10_CNR_ADJ_GAIN_MAX        = 63;
static const INT32 HNR10_CNR_SCALE_MIN           = 0;
static const INT32 HNR10_CNR_SCALE_MAX           = 15;

static const INT32  HNR10_FNR_ACTHR_LUT_MIN      = 0;
static const INT32  HNR10_FNR_ACTHR_LUT_MAX      = 63;
static const UINT32 HNR10_FNR_GAINARR_MIN        = 0;
static const UINT32 HNR10_FNR_GAINARR_MAX        = 63;
static const UINT32 HNR10_FNR_GAINCLAMPARR_MIN   = 0;
static const UINT32 HNR10_FNR_GAINCLAMPARR_MAX   = 255;
static const INT32  FNR_GAIN_SHIFT_IN_ARR        = 16;
static const INT32  HNR10_FNR_AC_SHFT_MIN        = 0;
static const INT32  HNR10_FNR_AC_SHFT_MAX        = 3;
static const INT32  HNR10_FNR_GAINARR_LUT_MIN    = 0;
static const INT32  HNR10_FNR_GAINARR_LUT_MAX    = 255;
static const INT32  HNR10_ABS_AMP_SHFT_MIN       = 0;
static const INT32  HNR10_ABS_AMP_SHFT_MAX       = 3;

static const INT32 HNR10_LPF3_PERCENT_MIN        = 0;
static const INT32 HNR10_LPF3_PERCENT_MAX        = 255;
static const INT32 HNR10_LPF3_OFFSET_MIN         = 0;
static const INT32 HNR10_LPF3_OFFSET_MAX         = 255;
static const INT32 HNR10_LPF3_STRENGTH_MIN       = 0;
static const INT32 HNR10_LPF3_STRENGTH_MAX       = 5;

static const INT32 HNR10_BLEND_CNR_ADJ_GAIN_MIN  = 0;
static const INT32 HNR10_BLEND_CNR_ADJ_GAIN_MAX  = 63;
static const INT32 HNR10_BLEND_SNR_GAINARR_MIN   = 0;
static const INT32 HNR10_BLEND_SNR_GAINARR_MAX   = 31;
static const INT32 HNR10_BLEND_LNR_GAINARR_MIN   = 0;
static const INT32 HNR10_BLEND_LNR_GAINARR_MAX   = 255;

static const INT16  HNR10_RNR_GAIN_ARR_MAX       = 1023;
static const INT16  HNR10_RNR_GAIN_ARR_MIN       = -1023;
static const INT16  HNR10_RNR_THRD_ARR_MIN       = 0;
static const INT16  HNR10_RNR_THRD_ARR_MAX       = static_cast<INT16>(IQSettingUtils::MAXINT32BITFIELD(11));
static const INT32  HNR10_RNR_BX_MIN             = IQSettingUtils::MININT32BITFIELD(14);
static const INT32  HNR10_RNR_BX_MAX             = IQSettingUtils::MAXINT32BITFIELD(14);
static const INT32  HNR10_RNR_BY_MIN             = IQSettingUtils::MININT32BITFIELD(14);
static const INT32  HNR10_RNR_BY_MAX             = IQSettingUtils::MAXINT32BITFIELD(14);
static const UINT32 HNR10_RNR_R_SQUARE_INIT_MIN  = IQSettingUtils::MINUINTBITFIELD(28);
static const UINT32 HNR10_RNR_R_SQUARE_INIT_MAX  = IQSettingUtils::MAXUINTBITFIELD(28);
static const UINT32 HNR10_RNR_MIN_SHIFT          = 0;
static const UINT32 HNR10_RNR_MAX_SHIFT          = 15;
static const UINT32 HNR10_RNR_MIN_R_SQUARE_SCALE = 0;
static const UINT32 HNR10_RNR_MAX_R_SQUARE_SCALE = 127;
static const UINT32 HNR10_RNR_MIN_ANCHOR         = 0;
static const UINT32 HNR10_RNR_MAX_ANCHOR         = 1023;
static const UINT32 HNR10_RNR_MIN_BASE           = 0;
static const UINT32 HNR10_RNR_MAX_BASE           = 1023;
static const UINT32 HNR10_SKIN_HUE_MIN_MIN       = 0;
static const UINT32 HNR10_SKIN_HUE_MIN_MAX       = 1023;
static const UINT32 HNR10_SKIN_HUE_MAX_MIN       = 0;
static const UINT32 HNR10_SKIN_HUE_MAX_MAX       = 255;
static const UINT32 HNR_SNR_QSTEP_MIN            = 0;
static const UINT32 HNR_SNR_QSTEP_MAX            = 128;
static const UINT32 HNR_SNR_GAIN_MIN             = 0;
static const UINT32 HNR_SNR_GAIN_MAX             = 31;
static const INT    HNR_Q_BITS_NUMBER            = 8;
static const INT16  HNR10_R_SQUARE_SHFT_MAX      = 16;
static const INT16  HNR10_MAX_RSQUARE_VAL        = 8191;
static const FLOAT  HNR10_MIN_FACE_BOUNDARY      = 0.0f;
static const FLOAT  HNR10_MAX_FACE_BOUNDARY      = 8.0f;
static const FLOAT  HNR10_MIN_FACE_TRANSITION    = 0.0f;
static const FLOAT  HNR10_MAX_FACE_TRANSITION    = 8.0f;

/// @brief Unpacked HNR10 output from common IQ library
// NOWHINE NC004c: Share code with system team
struct HNR10UnpackedField
{
    // Module configuration
    UINT16 enable;                                 ///< Module enable flag, 1u, default:1
    UINT16 lnr_en;                                 ///< 1u
    UINT16 rnr_en;                                 ///< 1u
    UINT16 cnr_en;                                 ///< 1u
    UINT16 fd_snr_en;                              ///< 1u
    UINT16 snr_en;                                 ///< 1u
    UINT16 fnr_en;                                 ///< 1u
    UINT16 lpf3_en;                                ///< 1u
    UINT16 blend_enable;                           ///< 1u
    UINT16 blend_cnr_en;                           ///< 1u
    UINT16 blend_snr_en;                           ///< 1u

    // Bank selection
    UINT16 lut_bank_sel;                           ///< Bank selection

    // LNR
    UINT16 lnr_gain_arr[2][HNR_V10_LNR_ARR_NUM];   ///< 6uQ5-6uQ5 packed
    UINT16 lnr_shift;                              ///< 2u

    // RNR
    INT16  rnr_bx;                                 ///< 14s, init_h_offset-h_center
    INT16  rnr_by;                                 ///< 14s, init_v_offset-v_center
    UINT32 rnr_r_square_init;                      ///< 28u, (init_h_offset-h_center)^2 + (init_v_offset-v_center)^2
    UINT16 rnr_r_square_scale;                     ///< 7u; range[0, 127]
    UINT16 rnr_r_square_shift;                     ///< 4u  range[0, 15]

    UINT16 rnr_anchor[HNR_V10_RNR_ARR_NUM];        ///< 10u
    UINT16 rnr_base  [HNR_V10_RNR_ARR_NUM];        ///< 10u, 6 anchors
    INT16  rnr_slope [HNR_V10_RNR_ARR_NUM];        ///< 11s
    UINT16 rnr_shift [HNR_V10_RNR_ARR_NUM];        ///< 4u

    // CNR
    UINT16 cnr_gain_arr[HNR_V10_CNR_ARR_NUM];      ///< 6uQ5
    INT16  cnr_low_thrd_u;                         ///< 8s
    UINT16 cnr_thrd_gap_u;                         ///< 3u
    INT16  cnr_low_thrd_v;                         ///< 8s
    UINT16 cnr_thrd_gap_v;                         ///< 3u
    UINT16 cnr_adj_gain;                           ///< 6uQ5
    UINT16 cnr_scale;                              ///< 4u, n means n+1

    // SNR/FD-SNR
    UINT16 snr_skin_hue_min;                       ///< 10u
    UINT16 snr_skin_hue_max;                       ///< 8u
    UINT16 snr_skin_y_min;                         ///< 8u
    UINT16 snr_skin_y_max;                         ///< 8u
    UINT16 snr_skin_ymax_sat_min;                  ///< 8u
    UINT16 snr_skin_ymax_sat_max;                  ///< 8u
    UINT16 snr_sat_min_slope;                      ///< 8u
    UINT16 snr_sat_max_slope;                      ///< 8u
    UINT16 snr_boundary_probability;               ///< 4u
    UINT16 snr_qstep_skin;                         ///< 8uQ8
    UINT16 snr_qstep_nonskin;                      ///< 8uQ8
    UINT16 snr_skin_smoothing_str;                 ///< 2u, [0, 2]
    UINT16 snr_gain_arr[2][HNR_V10_SNR_ARR_NUM];   ///< 5u

    UINT16 face_num;                                                   ///< 3u, [0, 4], n means n+1
    UINT16 face_center_horizontal[HNR_V10_MAX_FACE_NUM];               ///< 16u
    UINT16 face_center_vertical[HNR_V10_MAX_FACE_NUM];                 ///< 16u
    UINT16 face_radius_boundary[HNR_V10_MAX_FACE_NUM];                 ///< 8u
    UINT16 face_radius_slope[HNR_V10_MAX_FACE_NUM];                    ///< 8u
    UINT16 face_radius_shift[HNR_V10_MAX_FACE_NUM];                    ///< 4u
    UINT16 face_slope_shift[HNR_V10_MAX_FACE_NUM];                     ///< 3u
    UINT16 face_horizontal_offset;                                     ///< 16u
    UINT16 face_vertical_offset;                                       ///< 16u

    // FNR
    UINT32 merged_fnr_gain_arr_gain_clamp_arr[2][HNR_V10_FNR_ARR_NUM]; ///< 6u-6u packed plus 8u-8u packed
    UINT16 fnr_ac_th_arr[2][HNR_V10_FNR_ARR_NUM];                      ///< 6u-6u packed
    UINT16 fnr_ac_shift;                                               ///< 2u

    // NR filtering
    UINT16 abs_amp_shift;                                              ///< 2u
    UINT16 filtering_nr_gain_arr[HNR_V10_NR_ARR_NUM];                  ///< Register, not DMI LUT.
                                                                       ///< No need to be double-buffered.
    // LPF
    UINT16 lpf3_percent;                                               ///< 8uQ8
    UINT16 lpf3_offset;                                                ///< 8u
    UINT16 lpf3_strength;                                              ///< 3u

    // Blending
    UINT16 blend_lnr_gain_arr[2][HNR_V10_BLEND_LNR_ARR_NUM];           ///< 8u-8u packed
    UINT16 blend_cnr_adj_gain;                                         ///< 6uQ5
    UINT16 blend_snr_gain_arr[2][HNR_V10_BLEND_SNR_ARR_NUM];           ///< 5u
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements HNR10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class HNR10Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to the input data
    /// @param  pData         Pointer to the intepolation result
    /// @param  pReserveType  Pointer to the reserveType of this module
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const HNR10InputData*                                 pInput,
        hnr_1_0_0::hnr10_rgn_dataType*                        pData,
        hnr_1_0_0::chromatix_hnr10_reserveType*               pReserveType,
        hnr_1_0_0::chromatix_hnr10Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pRegCmd);
};
#endif // HNR10SETTING_H
