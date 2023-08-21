// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  linearization34setting.h
/// @brief Linearization34 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LINEARIZATION34SETTING_H
#define LINEARIZATION34SETTING_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "linearization_3_4_0.h"

static const UINT MAX_KNEE_POINTS = 8;
static const UINT MAX_SLOPE       = 9;
static const UINT MAX_LUT_BANK    = 2;

/// @brief Linearization34 Module Unpacked Data Structure
// NOWHINE NC004c: Share code with system team
struct Linearization34UnpackedField
{
    UINT16 enable;                                  ///< Module enable flag
    UINT16 bayerPattern;                            ///< 1u; 0=RGGB, 1=GRBG, 2=BGGR, 3=GBRG
    UINT16 LUTbankSelection;                        ///< dmi lut bank selection
    UINT16 rLUTkneePointL[MAX_KNEE_POINTS];         ///< Module r_lut_p_l flag
    UINT16 rLUTbaseL[MAX_LUT_BANK][MAX_SLOPE];      ///< Module r_lut_base_l flag
    UINT32 rLUTdeltaL[MAX_LUT_BANK][MAX_SLOPE];     ///< Module r_lut_delta_l flag
    UINT16 grLUTkneePointL[MAX_KNEE_POINTS];        ///< Module gr_lut_p_l flag
    UINT16 grLUTbaseL[MAX_LUT_BANK][MAX_SLOPE];     ///< Module gr_lut_base_l flag
    UINT32 grLUTdeltaL[MAX_LUT_BANK][MAX_SLOPE];    ///< Module gr_lut_delta_l flag
    UINT16 gbLUTkneePointL[MAX_KNEE_POINTS];        ///< Module gb_lut_p_l flag
    UINT16 gbLUTbaseL[MAX_LUT_BANK][MAX_SLOPE];     ///< Module gb_lut_base_l flag
    UINT32 gbLUTdeltaL[MAX_LUT_BANK][MAX_SLOPE];    ///< Module gb_lut_delta_l flag
    UINT16 bLUTkneePointL[MAX_KNEE_POINTS];         ///< Module b_lut_p_l flag
    UINT16 bLUTbaseL[MAX_LUT_BANK][MAX_SLOPE];      ///< Module b_lut_base_l flag
    UINT32 bLUTdeltaL[MAX_LUT_BANK][MAX_SLOPE];     ///< Module b_lut_delta_l flag
};

/// @brief Linearization34 Module debug buffer
// NOWHINE NC004c: Share code with system team
struct Linearization34DebugBuffer
{
    linearization_3_4_0::linearization34_rgn_dataType interpolationResult;    ///< Interpolated chromatix data
    Linearization34UnpackedField                      unpackedData;           ///< Calculated unpacked data
};

static const UINT32 LINEARIZATION34_LUT_MIN                = 0;
static const UINT32 LINEARIZATION34_LUT_MAX                = 16383;           ///< (1 << 14) - 1;
static const UINT32 LINEARIZATION34_Q_FACTOR_SLOPE         = 12;
static const UINT32 LINEARIZATION34_CHR_PARAMETER_BITWIDTH = 12;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Linearization34 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class Linearization34Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to the input data
    /// @param  pData         Pointer to the intepolation result
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const Linearization34IQInput*                                             pInput,
        linearization_3_4_0::linearization34_rgn_dataType*                        pData,
        linearization_3_4_0::chromatix_linearization34Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                                     pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateDelta
    ///
    /// @brief  CalculateDelta Function for Linearization34 Module
    ///
    /// @param  pLutP           Pointer to the LUT variable
    /// @param  pLutBase        Pointer to LUT base
    /// @param  pLutDelta       Pointer to the LUT Delta
    /// @param  pedestalEnable  Pedestal is enabled or not
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void CalculateDelta(
        FLOAT* pLutP,
        FLOAT* pLutBase,
        FLOAT* pLutDelta,
        BOOL   pedestalEnable);
};

#endif // LINEARIZATION34SETTING_H
