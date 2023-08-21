// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  bpspdpc20setting.h
/// @brief PDPC20 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BPSPDPC20SETTING_H
#define BPSPDPC20SETTING_H

#include "pdpc_2_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

/// @brief PDPC Module Unpacked Data Structure
/// NOWHINE NC004c: Share code with system team
struct PDPC20UnpackedField
{
    UINT16 enable;                          ///< Module enable flag
    UINT16 PDAFPDPCEnable;                  ///< PDAF pdpc function enable
    UINT16 PDAFDSBPCEnable;                 ///< PDAF dsbpc function enable
    UINT16 LUTBankSelection;                ///< dmi lut bank selection
    UINT16 leftCropEnable;                  ///< 1u; Crop Due to Strip Operation
    UINT16 rightCropEnable;                 ///< 1u; Crop Due to Strip Operation
    // for HDR
    UINT16 bayerPattern;                    ///< 1u; 0=RGGB, 1=GRBG, 2=BGGR, 3=GBRG
    UINT16 PDAFHDRSelection;                ///< 3u; 0x0: non-HDR all lines; 0x1: iHDR T1 first pattern; 0x2: iHDR T2 first
                                            ///< pattern; 0x4, 0x5, 0x6, 0x7: zzHDR patterns
    UINT16 PDAFzzHDRFirstrbExposure;        ///< 1u; 0x0: T1 (long exp), 0x1: T2 (short exp)
    UINT16 PDAFHDRExposureRatio;            ///< 15uQ10, 1024~16384 representing 1.0~16.0
    UINT16 PDAFHDRExposureRatioRecip;       ///< 9uQ8, 16~256 representing 1/16.0~1.0
    // for DBPC
    UINT16 fmaxPixelQ6;                     ///< 8u; Bad pixel maximum threshold factor,
                                            ///< Q6 unsigned - Occasionally (change in light conditions)
    UINT16 fminPixelQ6;                     ///< 8u; Bad pixel minimum threshold factor,
                                            ///< Q6 unsigned - Occasionally (change in light conditions)
    UINT32 blackLevel;                      ///< (6+e)u default: 12u
    UINT32 hotPixelCorrectionDisable;       ///< 1u
    UINT32 coldPixelCorrectionDisable;      ///< 1u
    UINT16 bpcOffset;                       ///< (8+e)u default: 14u
    UINT16 bccOffset;                       ///< (8+e)u default: 14u
    UINT16 bpcOffsetT2;                     ///< (8+e)u default: 14u
    UINT16 bccOffsetT2;                     ///< (8+e)u default: 14u
    UINT32 correctionThreshold;             ///< (7+e)u default: 13u
    UINT16 removeAlongEdge;                 ///< 1u
    UINT16 usingCrossChannel;               ///< 1u
    // for PDAF pixels
    UINT32 rgWbGain4096;                    ///< 17uQ12 = clip_17u(4096*AWB_R gain/AWB_G gain)
    UINT32 bgWbGain4096;                    ///< 17uQ12 = clip_17u(4096*AWB_B gain/AWB_G gain)
    UINT32 grWbGain4096;                    ///< 17uQ12 = clip_17u(4096*AWB_G gain/AWB_R gain)
    UINT32 gbWbGain4096;                    ///< 17uQ12 = clip_17u(4096*AWB_G gain/AWB_B gain)
    UINT16 PDAFGlobalOffsetX;               ///< 14u; PD pattern start global offset x
    UINT16 PDAFGlobalOffsetY;               ///< 14u; PD pattern start global offset y
    UINT16 PDAFXend;                        ///< 14u; horizontal PDAF pixel end location (0 means first  pixel from left)
    UINT16 PDAFYend;                        ///< 14u; vertical PDAF pixel end location (0 means first line from top)
    // 2 banks
    UINT32 PDAFPDMask[2][64];               ///< PD location mask for 64 32-bit words; for each bit 0: not PD pixel; 1: PD pixel
    UINT16 saturationThreshold;             ///< greater or equal threshold treat as saturated pixel.
    UINT16 PDAFTableXOffset;                ///< 5u; X offset within a pattern period
    UINT16 PDAFTableYOffset;                ///< 6u; y offset within a pattern period
};

/// @brief PDPC Module debug buffer Structure
/// NOWHINE NC004c: Share code with system team
struct PDPC20DebugBuffer
{
    pdpc_2_0_0::pdpc20_rgn_dataType interpolationResult;    ///< Interpolated chromatix data
    PDPC20UnpackedField             unpackedData;           ///< Calculated unpacked data
};

static const UINT8  BPS_HDR_ISP_OFF                          = 0;
static const UINT8  BPS_HDR_ISP_ON                           = 1;

static const UINT32 PDPC20_BLACK_LEVEL_MIN                   = 0;
static const UINT32 PDPC20_BLACK_LEVEL_MAX                   = IQSettingUtils::MAXUINTBITFIELD(12);;
static const UINT32 PDPC20_HDR_EXP_RATIO_MIN                 = (1 << 10);
static const UINT32 PDPC20_HDR_EXP_RATIO_MAX                 = (1 << 14);
static const UINT32 PDPC20_HDR_EXP_RATIO__RECIP_MIN          = 16;
static const UINT32 PDPC20_HDR_EXP_RATIO_RECIP_MAX           = 256;
static const UINT16 PDPC20_FMAXPIXEL_MIN                     = 0;
static const UINT16 PDPC20_FMAXPIXEL_MAX                     = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(7));
static const UINT32 PDPC20_FMINPIXEL_MIN                     = 0;
static const UINT16 PDPC20_FMINPIXEL_MAX                     = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(7));
static const UINT32 PDPC20_HOT_PIXEL_CORRECTION_DISABLE_MIN  = 0;
static const UINT32 PDPC20_HOT_PIXEL_CORRECTION_DISABLE_MAX  = 1;
static const UINT32 PDPC20_COLD_PIXEL_CORRECTION_DISABLE_MIN = 0;
static const UINT32 PDPC20_COLD_PIXEL_CORRECTION_DISABLE_MAX = 1;
static const UINT16 PDPC20_BPC_OFFSET_MIN                    = 0;
static const UINT16 PDPC20_BPC_OFFSET_MAX                    = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT32 PDPC20_BCC_OFFSET_MIN                    = 0;
static const UINT16 PDPC20_BCC_OFFSET_MAX                    = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 PDPC20_BPC_OFFSET_T2_MIN                 = 0;
static const UINT16 PDPC20_BPC_OFFSET_T2_MAX                 = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 PDPC20_BCC_OFFSET_T2_MIN                 = 0;
static const UINT16 PDPC20_BCC_OFFSET_T2_MAX                 = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT32 PDPC20_CORRECTION_THRES_MIN              = 0;
static const UINT32 PDPC20_CORRECTION_THRES_MAX              = IQSettingUtils::MAXUINTBITFIELD(14);
static const UINT32 PDPC20_BCC_CORRECTION_THRES_MAX          = IQSettingUtils::MAXUINTBITFIELD(14);
static const UINT16 PDPC20_REMOVE_ALONG_EDGE_MIN             = 0;
static const UINT16 PDPC20_REMOVE_ALONG_EDGE_MAX             = 1;
static const UINT16 PDPC20_USING_CROSS_CHANNEL_MIN           = 0;
static const UINT16 PDPC20_USING_CROSS_CHANNEL_MAX           = 1;
static const UINT16 PDPC20_SATURATION_THRES_MIN              = 0;
static const UINT16 PDPC20_SATURATION_THRES_MAX              = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT32 PDPC20_RGB_WB_GAIN_MIN                   = 128;
static const UINT32 PDPC20_RGB_WB_GAIN_MAX                   = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT16 PDPC20_GLOBAL_OFFSET_X_MIN               = 0;
static const UINT16 PDPC20_GLOBAL_OFFSET_X_MAX               = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 PDPC20_GLOBAL_OFFSET_Y_MIN               = 0;
static const UINT16 PDPC20_GLOBAL_OFFSET_Y_MAX               = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT32 PDPC20_PDAF_X_END_MIN                    = 0;
static const UINT32 PDPC20_PDAF_X_END_MAX                    = IQSettingUtils::MAXUINTBITFIELD(14);
static const UINT32 PDPC20_PDAF_Y_END_MIN                    = 0;
static const UINT32 PDPC20_PDAF_Y_END_MAX                    = IQSettingUtils::MAXUINTBITFIELD(14);
static const UINT16 PDPC20_PDAF_HDR_SELECTION_MIN            = 0;
static const UINT16 PDPC20_PDAF_HDR_SELECTION_MAX            = 7;  // 3 bits in HDR_SELECTION register

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements PDPC20 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NOWHINE NC004c: Share code with system team
class BPSPDPC20Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to input data of PDPC20 Module
    /// @param  pData         Pointer to output of the interpolation algorithem
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to ouput register value
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const PDPC20IQInput*                                    pInput,
        pdpc_2_0_0::pdpc20_rgn_dataType*                        pData,
        pdpc_2_0_0::chromatix_pdpc20Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                   pRegCmd);
};

#endif // BPSPDPC20SETTING_H
