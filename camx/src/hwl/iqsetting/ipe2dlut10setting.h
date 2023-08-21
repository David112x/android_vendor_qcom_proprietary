// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ipe2dlut10setting.h
/// @brief TDL10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IPE2DLUT10SETTING_H
#define IPE2DLUT10SETTING_H

#include "tdl_1_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const UINT32 LUT1D_H                          = 12288;
static const UINT32 LUT1D_S                          = 2048;
static const UINT32 KINTEGERMAX                      = 255;
static const UINT32 KINTEGERMIN                      = 0;
static const INT16  LUT2D_MAX                        = 511;
static const INT16  LUT2D_MIN                        =-511;
static const UINT32 Y_BLEND_FACTOR_INTEGER_MIN       = 0;
static const UINT32 Y_BLEND_FACTOR_INTEGER_MAX       = 16;
static const UINT16 L_BOUNDARY_MIN                   = 0;
static const UINT16 L_BOUNDARY_MAX                   = 2047;

static const DOUBLE K_b                              = 0.114;  // 0.0001 1101 0010 1111 -> 0.114*2^9=58.368 =0x3a=001 1101 0
static const DOUBLE K_r                              = 0.299;  // 0.0100 1100 1000 1011 -> 0.299*2^9=152.088=0x99=100 1100 1

static const INT H_bits                              = 14;
static const INT S_bits                              = 12;
static const INT L_bits                              = 11;
static const INT Input_bits                          = 10;
static const INT Hue_q_factor                        = 11;
static const INT Saturation_q_factor                 = 11;
static const INT Saturation_inverse_q_factor         = 22;
static const INT Hue_inverse_q_factor                = 22;
static const INT Hue_inverse_significant_bits        = 13;
static const INT Saturation_inverse_significant_bits = 13;
static const INT Blend_q_factor                      = 4;
static const INT L_multiple                          = 1;      // 2046;
static const INT Coefficient_q_factor                = 9;      // move to int get_luma_delta()
static const INT H_ratio_q_factor                    = 10;     // unused
static const INT L_inverse_q_factor                  = 11;     // OLD: 9; // 10;
static const INT H_inverse_q_factor                  = 14;     // OLD: 9; // 10;
static const INT S_inverse_q_factor                  = 11;     // OLD: 9; // 10;
static const INT Delta_h_q_factor                    = 9;      // OLD: 11; // 8: 1 sign bit, 7 / 10: 1 sign bit,
                                                               // -30~+30 total 10+1 bit: x = 2047*30/60; // 1, 2, 3bit no! 0bit
static const INT Delta_s_q_factor                    = 9;      // OLD: 13; const int delta_s_shift_bit = 0; -0.249 ~ + 0.249
                                                               // total 9+1 bit : x = 2048*0.249/1.0 // 3bit no! 2bit okay

// NOWHINE NC004c: Share code with system team
struct LutHSInStruct_V10
{
    INT32  lut_2d_h[H_GRID*S_GRID];        ///< 12u
    INT32  lut_2d_s[H_GRID*S_GRID];        ///< 12u
    UINT32 lut_1d_h[H_GRID];               ///< 12u
    UINT32 lut_1d_s[S_GRID];               ///< 12u
    UINT32 lut_1d_h_inv[(H_GRID - 1)];     ///< 12u
    UINT32 lut_1d_s_inv[(S_GRID - 1)];     ///< 12u
};

// NOWHINE NC004c: Share code with system team
struct TDL10UnpackedField
{
    UINT16 enable;                     ///< 16u

    // user config
    LutHSInStruct_V10 hs_lut;          ///< 16u

    UINT16 h_shift;                    ///< 16u
    UINT16 s_shift;                    ///< 16u
    UINT16 l_boundary_start_A;         ///< 16u
    UINT16 l_boundary_start_B;         ///< 16u
    UINT16 l_boundary_end_A;           ///< 16u
    UINT16 l_boundary_end_B;           ///< 16u

    UINT16 y_blend_factor_integer;     ///< 16u
    UINT16 k_b_integer;                ///< 16u
    UINT16 k_r_integer;                ///< 16u

    // sw calculation
    UINT16 l_boundary_start_inv;       ///< 16u
    UINT16 l_boundary_end_inv;         ///< 16u
};

// NOWHINE NC004c: Share code with system team
struct TDL10DebugBuffer
{
    tdl_1_0_0::tdl10_rgn_dataType interpolationResult; ///< Interpolated chromatix data
    TDL10UnpackedField            unpackedData;        ///< Calculated unpacked data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements TDL10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class IPETDL10Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to the input data
    /// @param  pData         Pointer to the intepolation result
    /// @param  pReserveType  Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const TDL10InputData*                                 pInput,
        tdl_1_0_0::tdl10_rgn_dataType*                        pData,
        tdl_1_0_0::chromatix_tdl10_reserveType*               pReserveType,
        tdl_1_0_0::chromatix_tdl10Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pRegCmd);
};
#endif // IPE2DLUT10SETTING_H
