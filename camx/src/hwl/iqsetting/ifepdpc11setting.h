// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifepdpc11setting.h
/// @brief PDPC11 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IFEPDPC11SETTING_H
#define IFEPDPC11SETTING_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "pdpc_1_1_0.h"

static const UINT32 DMIRAM_PDAF_LUT_LENGTH              = 64;
static const UINT32 PDPC11_RG_WB_GAIN_MIN               = 0;
static const UINT32 PDPC11_RG_WB_GAIN_MAX               = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT32 PDPC11_BG_WB_GAIN_MIN               = 0;
static const UINT32 PDPC11_BG_WB_GAIN_MAX               = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT32 PDPC11_GR_WB_GAIN_MIN               = 0;
static const UINT32 PDPC11_GR_WB_GAIN_MAX               = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT32 PDPC11_GB_WB_GAIN_MIN               = 0;
static const UINT32 PDPC11_GB_WB_GAIN_MAX               = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT32 PDPC11_PDAF_BLACKLEVEL_MIN          = 0;
static const UINT32 PDPC11_PDAF_BLACKLEVEL_MAX          = IQSettingUtils::MAXUINTBITFIELD(12);
static const UINT32 PDPC11_FMAX_PIXEL_MIN               = 0;
static const UINT32 PDPC11_FMAX_PIXEL_MAX               = IQSettingUtils::MAXUINTBITFIELD(8);
static const UINT32 PDPC11_FMIN_PIXEL_MIN               = 0;
static const UINT32 PDPC11_FMIN_PIXEL_MAX               = IQSettingUtils::MAXUINTBITFIELD(8);
static const UINT32 PDPC11_WB_GAIN_MIN                  = 0;
static const UINT32 PDPC11_WB_GAIN_MAX                  = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT32 PDPC11_BP_OFFSET_PIXEL_MIN          = 0;
static const UINT32 PDPC11_BP_OFFSET_PIXEL_MAX          = IQSettingUtils::MAXUINTBITFIELD(15);
static const UINT32 PDPC11_T2_BP_OFFSET_PIXEL_MIN       = 0;
static const UINT32 PDPC11_T2_BP_OFFSET_PIXEL_MAX       = IQSettingUtils::MAXUINTBITFIELD(15);
static const UINT16 PDPC11_PDAF_HDR_SELECTION_MIN       = 0;
static const UINT16 PDPC11_PDAF_HDR_SELECTION_MAX       = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(3));
static const UINT32 PDPC11_PDAF_HDR_EXP_RATIO_MIN       = (1 << 10);
static const UINT32 PDPC11_PDAF_HDR_EXP_RATIO_MAX       = (1 << 14);
static const UINT32 PDPC11_PDAF_HDR_EXP_RATIO_RECIP_MIN = 16;
static const UINT32 PDPC11_PDAF_HDR_EXP_RATIO_RECIP_MAX = 256;
static const UINT32 PDPC11_PDAF_X_END_MIN               = 0;
static const UINT32 PDPC11_PDAF_X_END_MAX               = IQSettingUtils::MAXUINTBITFIELD(14);
static const UINT32 PDPC11_PDAF_Y_END_MIN               = 0;
static const UINT32 PDPC11_PDAF_Y_END_MAX               = IQSettingUtils::MAXUINTBITFIELD(14);
static const UINT16 PDPC11_PDAF_MIN_GLOBAL_OFFSET       = 0;
static const UINT16 PDPC11_PDAF_MAX_GLOBAL_OFFSET       = static_cast<UINT16>(MAX_UINT14);
static const UINT8  HDR_ISP_OFF                         = 0;
static const UINT8  HDR_ISP_ON                          = 1;

/// @brief Unpacked Data for PDPC11 Module
// NOWHINE NC004: Share code with system team
struct PDPC11UnpackedField
{
    UINT16 enable;                               ///< whole block enable, disable to save power
    UINT16 pdaf_pdpc_en;                         ///< PDAF function enable
    UINT16 pdaf_dsbpc_en;                        ///< PDAF dsbpc function enable
    UINT16 pdaf_HDR_selection;                   ///< 3u; 0x0: non-HDR all lines; 0x1: iHDR T1 first pattern;
                                                 ///< 0x2: iHDR T2 first pattern; 0x4, 0x5, 0x6, 0x7: zzHDR patterns
    UINT16 pdaf_zzHDR_first_rb_exp;              ///< 1u; 0x0: T1 (long exp), 0x1: T2 (short exp)
    UINT16 pdaf_hdr_exp_ratio;                   ///< 15uQ10, 1024~16384 representing 1.0~16.0
    UINT16 pdaf_hdr_exp_ratio_recip;             ///< 9uQ8, 16~256 representing 1/16.0~1.0
    UINT16 pdaf_blacklevel;                      ///< (6+e)u; consider blacklevel in multiply exposure gain
    UINT16 fmax_pixel_Q6;                        ///< 8u; Bad pixel maximum threshold factor, Q6 unsigned
    UINT16 fmin_pixel_Q6;                        ///< 8u; Bad pixel minimum threshold factor, Q6 unsigned
    UINT32 bp_offset_g_pixel;                    ///< (9+e)u; Upper/Lower bad pixel threshold offset for green pixels
    UINT32 bp_offset_rb_pixel;                   ///< (9+e)u; Upper/Lower bad pixel threshold offset for red/blue pixels
    UINT32 t2_bp_offset_g_pixel;                 ///< (9+e)u; Upper/Lower bad pixel threshold offset for green pixels
    UINT32 t2_bp_offset_rb_pixel;                ///< (9+e)u; Upper/Lower bad pixel threshold offset for red/blue pixels
    UINT32 rg_wb_gain;                           ///< 17uQ12 = clip_17u(4096*AWB_R gain/AWB_G gain)
    UINT32 bg_wb_gain;                           ///< 17uQ12 = clip_17u(4096*AWB_B gain/AWB_G gain)
    UINT32 gr_wb_gain;                           ///< 17uQ12 = clip_17u(4096*AWB_G gain/AWB_R gain)
    UINT32 gb_wb_gain;                           ///< 17uQ12 = clip_17u(4096*AWB_G gain/AWB_B gain)
    UINT16 pdaf_global_offset_x;                 ///< 14u; PD pattern start global offset x
    UINT16 pdaf_global_offset_y;                 ///< 14u; PD pattern start global offset y
    UINT16 pdaf_x_end;                           ///< 14u; horizontal PDAF pixel end location (0 means first  pixel from left)
    UINT16 pdaf_y_end;                           ///< 14u; vertical PDAF pixel end location (0 means first line from top)
    UINT32 PDAF_PD_Mask[DMIRAM_PDAF_LUT_LENGTH]; ///< PD location mask for 64 32-bit words;
                                                 ///< for each bit 0: not PD pixel; 1: PD pixel
};

/// @brief Debugging Buffer for PDPC11 Module
// NOWHINE NC004c: Share code with system team
struct PDPC11DebugBuffer
{
    pdpc_1_1_0::pdpc11_rgn_dataType   interpolationResult; ///< Interpolated chromatix data
    PDPC11UnpackedField               unpackedData;        ///< Calculated unpacked data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements PDPC11 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class IFEPDPC11Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the input data
    /// @param  pData          Pointer to the intepolation result
    /// @param  pModeuleEnable Pointer to the variables to enable the module
    /// @param  pRegCmd        Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const PDPC11InputData*                                  pInput,
        pdpc_1_1_0::pdpc11_rgn_dataType*                        pData,
        pdpc_1_1_0::chromatix_pdpc11Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                   pRegCmd);
};

#endif // IFEPDPC11SETTING_H
