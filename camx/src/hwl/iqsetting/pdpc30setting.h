// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  pdpc30setting.h
/// @brief PDPC30 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef PDPC30SETTING_H
#define PDPC30SETTING_H

#include "pdpc_3_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const UINT8  HDR30_ISP_OFF                               = 0;
static const UINT8  HDR30_ISP_ON                                = 1;

static const UINT8  PDPC30_BIT_WIDTH                            = 14;
static const UINT16 PDPC30_NOISESTD_LENGTH                      = 64;
static const UINT32 PDPC30_BLACK_LEVEL_MIN                      = 0;
static const UINT32 PDPC30_BLACK_LEVEL_MAX                      = IQSettingUtils::MAXUINTBITFIELD(14);
static const UINT32 PDPC30_HDR_EXP_RATIO_MIN                    = (1 << 7);
static const UINT32 PDPC30_HDR_EXP_RATIO_MAX                    = IQSettingUtils::MAXUINTBITFIELD(13);
static const UINT32 PDPC30_HDR_EXP_RATIO__RECIP_MIN             = (1 << 7);
static const UINT32 PDPC30_HDR_EXP_RATIO_RECIP_MAX              = IQSettingUtils::MAXUINTBITFIELD(13);
static const UINT16 PDPC30_FMAXPIXEL_MIN                        = 0;
static const UINT16 PDPC30_FMAXPIXEL_MAX                        = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(7));
static const UINT32 PDPC30_FMINPIXEL_MIN                        = 0;
static const UINT16 PDPC30_FMINPIXEL_MAX                        = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(7));
static const UINT32 PDPC30_HOT_PIXEL_CORRECTION_DISABLE_MIN     = 0;
static const UINT32 PDPC30_HOT_PIXEL_CORRECTION_DISABLE_MAX     = 1;
static const UINT32 PDPC30_COLD_PIXEL_CORRECTION_DISABLE_MIN    = 0;
static const UINT32 PDPC30_COLD_PIXEL_CORRECTION_DISABLE_MAX    = 1;
static const UINT16 PDPC30_BPC_OFFSET_MIN                       = 0;
static const UINT16 PDPC30_BPC_OFFSET_MAX                       = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT32 PDPC30_BCC_OFFSET_MIN                       = 0;
static const UINT16 PDPC30_BCC_OFFSET_MAX                       = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT32 PDPD30_BPC_OFFSET_FLAT_MIN                  = 0;
static const UINT32 PDPD30_BPC_OFFSET_FLAT_MAX                  = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT32 PDPC30_BCC_OFFSET_FLAT_MIN                  = 0;
static const UINT32 PDPC30_BCC_OFFSET_FLAT_MAX                  = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 PDPC30_BPC_OFFSET_T2_MIN                    = 0;
static const UINT16 PDPC30_BPC_OFFSET_T2_MAX                    = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 PDPC30_BCC_OFFSET_T2_MIN                    = 0;
static const UINT16 PDPC30_BCC_OFFSET_T2_MAX                    = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 PDPC30_REMOVE_ALONG_EDGE_MIN                = 0;
static const UINT16 PDPC30_REMOVE_ALONG_EDGE_MAX                = 1;
static const UINT16 PDPC30_USING_CROSS_CHANNEL_MIN              = 0;
static const UINT16 PDPC30_USING_CROSS_CHANNEL_MAX              = 1;
static const UINT16 PDPC30_SATURATION_THRES_MIN                 = 0;
static const UINT16 PDPC30_SATURATION_THRES_MAX                 = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT32 PDPC30_RGB_WB_GAIN_MIN                      = 128;
static const UINT32 PDPC30_RGB_WB_GAIN_MAX                      = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT16 PDPC30_GLOBAL_OFFSET_X_MIN                  = 0;
static const UINT16 PDPC30_GLOBAL_OFFSET_X_MAX                  = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 PDPC30_GLOBAL_OFFSET_Y_MIN                  = 0;
static const UINT16 PDPC30_GLOBAL_OFFSET_Y_MAX                  = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT32 PDPC30_PDAF_X_END_MIN                       = 0;
static const UINT32 PDPC30_PDAF_X_END_MAX                       = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT32 PDPC30_PDAF_Y_END_MIN                       = 0;
static const UINT32 PDPC30_PDAF_Y_END_MAX                       = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 PDPC30_PDAF_HDR_SELECTION_MIN               = 0;
static const UINT16 PDPC30_PDAF_HDR_SELECTION_MAX               = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(3));

/// @brief PDPC Module Unpacked Data Structure
/// NOWHINE NC004c: Share code with system team
struct PDPC30UnpackedField
{
    UINT16 enable;                          ///< Module enable flag
    UINT16 PDAFPDPCEnable;                  ///< PDAF pdpc function enable
    UINT16 PDAFBPCEnable;                   ///< PDAF bpc function enable
    UINT16 PDAFGICEnable;                   ///< PDAF GIC function enable
    UINT16 LUTBankSelection;                ///< dmi lut bank selection

    // for HDR
    UINT16 bayerPattern;                    ///< 1u; 0=RGGB, 1=GRBG, 2=BGGR, 3=GBRG
    UINT16 PDAFHDRSelection;                ///< 3u; 0x0: non-HDR all lines; 0x1: iHDR T1 first pattern; 0x2: iHDR T2 first
                                            ///< pattern; 0x4, 0x5, 0x6, 0x7: zzHDR patterns
    UINT16 PDAFzzHDRFirstrbExposure;        ///< 1u; 0x0: T1 (long exp), 0x1: T2 (short exp)
    UINT16 PDAFHDRExposureRatio;            ///< 15uQ10, 1024~16384 representing 1.0~16.0
    UINT16 PDAFHDRExposureRatioRecip;       ///< 13uQ12, 256~4096 representing 1/16.0~1.0
    UINT16 saturationThreshold;             ///< >=threshold treat as saturated pixel

    // BPC screening
    UINT16 fmaxFlat;                        ///< 8u; Bad pixel maximum threshold factor, Q6 unsigned
    UINT16 fminFlat;                        ///< 8u; Bad pixel minimum threshold factor, Q6 unsigned
    UINT32 bpcOffsetFlat;                   ///< 8u; Bad pixel offset factor, Q6 unsigned
    UINT32 bccOffsetFlat;                   ///< 8u; Bad cluster offset factor, Q6 unsigned
    UINT32 noiseStdLUTLevel0[2][PDPC30_NOISESTD_LENGTH];        ///< 64-entry, 32b per entry table stores shot noise,
                                                                ///< one for each level and each channel

    UINT16 fmax;                            ///< 8u; Bad pixel maximum threshold factor, Q6 unsigned
    UINT16 fmin;                            ///< 8u; Bad pixel minimum threshold factor, Q6 unsigned
    UINT32 bpcOffset;                       ///< 8u; Bad pixel offset factor, Q6 unsigned
    UINT32 bccOffset;                       ///< 8u; Bad cluster offset factor, Q6 unsigned
    UINT16 directionalBPCEnable;            ///< enable the directional recovery part using PD method.
    UINT16 flatThRecip;                     ///< Inverse of SAD_th, 10uQ6

    UINT32 blackLevel;                      ///< black level
    UINT16 useSameChannelOnly;              ///< determin to use same channel
    UINT16 singleBPCOnly;                   ///< determin to use single bpc
    UINT16 flatDetectionEn;                 ///< flat detection enable

    // AWB gain
    UINT32 rgWbGain4096;                    ///< 17uQ12 = clip_17u(4096*AWB_R gain/AWB_G gain)
    UINT32 bgWbGain4096;                    ///< 17uQ12 = clip_17u(4096*AWB_B gain/AWB_G gain)
    UINT32 grWbGain4096;                    ///< 17uQ12 = clip_17u(4096*AWB_G gain/AWB_R gain)
    UINT32 gbWbGain4096;                    ///< 17uQ12 = clip_17u(4096*AWB_G gain/AWB_B gain)

    // PDAF pixels
    UINT16 PDAFGlobalOffsetX;               ///< 14u; PD pattern start global offset x
    UINT16 PDAFGlobalOffsetY;               ///< 14u; PD pattern start global offset y
    UINT16 PDAFXend;                        ///< 14u; horizontal PDAF pixel end location (0 means first  pixel from left)
    UINT16 PDAFYend;                        ///< 14u; vertical PDAF pixel end location (0 means first line from top)
    UINT16 PDAFTableXOffset;                ///< 5u; X offset within a pattern period
    UINT16 PDAFTableYOffset;                ///< 6u; y offset within a pattern period
    UINT64 PDAFPDMask[2][64];               ///< PD location mask for 64 32-bit words; for each bit 0: not PD pixel; 1: PD pixel
    UINT16 dirTk;                           ///< threshold k ratio for directional detection.
    UINT16 dirOffset;                       ///< offset value for directional detection and recovery.

    // GIC
    UINT16 gicThinLineNoiseOffset;          ///< 14u, for adjusting the threshold of thin line detection
    UINT16 gicFilterStrength;               ///< 9uQ8, for adjusting the strength of GIC
    UINT16 fmaxGIC;                         ///< Bad pixel maximum threshold for gic
    UINT16 bpcOffsetGIC;                    ///< Bad pixel offset for gic
};

/// @brief PDPC Module debug buffer Structure
/// NOWHINE NC004c: Share code with system team
struct PDPC30DebugBuffer
{
    pdpc_3_0_0::pdpc30_rgn_dataType interpolationResult;    ///< Interpolated chromatix data
    PDPC30UnpackedField             unpackedData;           ///< Calculated unpacked data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements PDPC30 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NOWHINE NC004c: Share code with system team
class PDPC30Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to input data of PDPC30 Module
    /// @param  pData         Pointer to output of the interpolation algorithem
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to ouput register value
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const PDPC30IQInput*                                    pInput,
        pdpc_3_0_0::pdpc30_rgn_dataType*                        pData,
        pdpc_3_0_0::chromatix_pdpc30Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                   pRegCmd);
};

#endif // PDPC30SETTING_H
