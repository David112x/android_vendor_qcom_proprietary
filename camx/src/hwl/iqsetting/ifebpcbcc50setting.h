// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifebpcbcc50setting.h
/// @brief BPCBCC50 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IFEBPCBCC50SETTING_H
#define IFEBPCBCC50SETTING_H

#include "bpcbcc_5_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const UINT32 BPCBCC50_PIXEL_CORRECTION_DISABLE_MIN = 0;
static const UINT32 BPCBCC50_PIXEL_CORRECTION_DISABLE_MAX = 1;
static const UINT32 BPCBCC50_SAME_CHANNEL_RECOVERY_MIN    = 0;
static const UINT32 BPCBCC50_SAME_CHANNEL_RECOVERY_MAX    = 1;
static const UINT32 BPCBCC50_FMAX_MIN                     = 64;
static const UINT32 BPCBCC50_FMAX_MAX                     = 127;
static const UINT32 BPCBCC50_FMIN_MIN                     = 0;
static const UINT32 BPCBCC50_FMIN_MAX                     = 64;
static const UINT32 BPCBCC50_BPC_OFFSET_MIN               = 0;
static const UINT32 BPCBCC50_BPC_OFFSET_MAX               = 16383;
static const UINT32 BPCBCC50_BCC_OFFSET_MIN               = 0;
static const UINT32 BPCBCC50_BCC_OFFSET_MAX               = 16383;
static const UINT32 BPCBCC50_CORRECTION_THRESHOLD_MIN     = 0;
static const UINT32 BPCBCC50_CORRECTION_THRESHOLD_MAX     = 8191;
static const UINT32 BPCBCC50_BLACKLEVEL_MIN               = 0;
static const UINT32 BPCBCC50_BLACKLEVEL_MAX               = MAX_UINT12;
static const UINT32 BPCBCC50_BG_WB_GAIN_RATIO_MIN         = 128;
static const UINT32 BPCBCC50_BG_WB_GAIN_RATIO_MAX         = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT32 BPCBCC50_GR_WB_GAIN_RATIO_MIN         = 128;
static const UINT32 BPCBCC50_GR_WB_GAIN_RATIO_MAX         = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT32 BPCBCC50_GB_WB_GAIN_RATIO_MIN         = 128;
static const UINT32 BPCBCC50_GB_WB_GAIN_RATIO_MAX         = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT32 BPCBCC50_RG_WB_GAIN_RATIO_MIN         = 128;
static const UINT32 BPCBCC50_RG_WB_GAIN_RATIO_MAX         = IQSettingUtils::MAXUINTBITFIELD(17);

// NOWHINE NC004c: Share code with system team
struct BPCBCC50UnpackedField
{
    UINT32 enable;                      ///< Module enable flag
    UINT32 hotPixelCorrectionDisable;   ///< hotPixelCorrectionDisable field
    UINT32 coldPixelCorrectionDisable;  ///< coldPixelCorrectionDisable field
    UINT32 sameChannelRecovery;         ///< sameChannelRecovery field
    UINT32 fmax;                        ///< fmax field
    UINT32 fmin;                        ///< fmin field
    UINT32 bpcOffset;                   ///< bccOffset field
    UINT32 bccOffset;                   ///< bccOffset field
    UINT32 correctionThreshold;         ///< correctionThreshold field
    UINT32 black_level;                 ///< black_level field
    UINT32 bg_wb_gain_ratio;            ///< BG Gain Ratio field
    UINT32 gr_wb_gain_ratio;            ///< GR Gain Ratio field
    UINT32 gb_wb_gain_ratio;            ///< GB Gain Ratio field
    UINT32 rg_wb_gain_ratio;            ///< RG Gain Ratio field
    UINT32 hot_bad_pix_cnt;             ///< hot_bad_pix_cnt field
    UINT32 cold_bad_pix_cnt;            ///< cold_bad_pix_cnt field
};

// NOWHINE NC004c: Share code with system team
struct BPCBCC50DebugBuffer
{
    bpcbcc_5_0_0::bpcbcc50_rgn_dataType interpolationResult; ///< Interpolated chromatix data
    BPCBCC50UnpackedField               unpackedData;        ///< Calculated unpacked data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements BPCBCC50 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class IFEBPCBCC50Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput       Pointer to the input data
    /// @param  pData        Pointer to the intepolation result
    /// @param  moduleEnable Control Variable to enable the module
    /// @param  pRegCmd      Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const BPCBCC50InputData*              pInput,
        bpcbcc_5_0_0::bpcbcc50_rgn_dataType*  pData,
        globalelements::enable_flag_type      moduleEnable,
        VOID*                                 pRegCmd);
};
#endif // IFEBPCBCC50SETTING_H
