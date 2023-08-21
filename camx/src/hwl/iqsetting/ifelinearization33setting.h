// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifelinearization33setting.h
/// @brief Linearization33 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IFELINEARIZATION33SETTING_H
#define IFELINEARIZATION33SETTING_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "linearization_3_3_0.h"

static const UINT MaxKneepoints = 8;
static const UINT MaxSlope      = 9;

/// @brief Linearization33 Module Unpacked Data Structure
// NOWHINE NC004c: Share code with system team
struct Linearization33UnpackedField
{
    UINT16 lut_bank_sel;                           ///< Module lut_bank_sel flag
    UINT16 r_lut_p_l[MaxKneepoints];               ///< Module r_lut_p_l flag
    UINT16 r_lut_base_l[MaxLUTBank][MaxSlope];     ///< Module r_lut_base_l flag
    UINT32 r_lut_delta_l[MaxLUTBank][MaxSlope];    ///< Module r_lut_delta_l flag
    UINT16 gr_lut_p_l[MaxKneepoints];              ///< Module gr_lut_p_l flag
    UINT16 gr_lut_base_l[MaxLUTBank][MaxSlope];    ///< Module gr_lut_base_l flag
    UINT32 gr_lut_delta_l[MaxLUTBank][MaxSlope];   ///< Module gr_lut_delta_l flag
    UINT16 gb_lut_p_l[MaxKneepoints];              ///< Module gb_lut_p_l flag
    UINT16 gb_lut_base_l[MaxLUTBank][MaxSlope];    ///< Module gb_lut_base_l flag
    UINT32 gb_lut_delta_l[MaxLUTBank][MaxSlope];   ///< Module gb_lut_delta_l flag
    UINT16 b_lut_p_l[MaxKneepoints];               ///< Module b_lut_p_l flag
    UINT16 b_lut_base_l[MaxLUTBank][MaxSlope];     ///< Module b_lut_base_l flag
    UINT32 b_lut_delta_l[MaxLUTBank][MaxSlope];    ///< Module b_lut_delta_l flag
    UINT16 r_lut_p_r[MaxKneepoints];               ///< Module r_lut_p_r flag
    UINT16 r_lut_base_r[MaxLUTBank][MaxSlope];     ///< Module r_lut_base_r flag
    UINT32 r_lut_delta_r[MaxLUTBank][MaxSlope];    ///< Module r_lut_delta_r flag
    UINT16 gr_lut_p_r[MaxKneepoints];              ///< Module gr_lut_p_r flag
    UINT16 gr_lut_base_r[MaxLUTBank][MaxSlope];    ///< Module gr_lut_base_r flag
    UINT32 gr_lut_delta_r[MaxLUTBank][MaxSlope];   ///< Module gr_lut_delta_r flag
    UINT16 gb_lut_p_r[MaxKneepoints];              ///< Module gb_lut_p_r flag
    UINT16 gb_lut_base_r[MaxLUTBank][MaxSlope];    ///< Module gb_lut_base_r flag
    UINT32 gb_lut_delta_r[MaxLUTBank][MaxSlope];   ///< Module gb_lut_delta_r flag
    UINT16 b_lut_p_r[MaxKneepoints];               ///< Module b_lut_p_r flag
    UINT16 b_lut_base_r[MaxLUTBank][MaxSlope];     ///< Module b_lut_base_r flag
    UINT32 b_lut_delta_r[MaxLUTBank][MaxSlope];    ///< Module b_lut_delta_r flag
};

/// @brief Linearization33 Module Debugging Buffer
// NOWHINE NC004c: Share code with system team
struct Linearization33DebugBuffer
{
    linearization_3_3_0::linearization33_rgn_dataType interpolationResult; ///< Interpolated chromatix data
    Linearization33UnpackedField                      unpackedData;        ///< Calculated unpacked data
};

static const INT32  LINEARIZATION33_LUT_MIN           = 0;
static const INT32  LINEARIZATION33_LUT_MAX           = 16383; ///< (1 << 14) - 1;
static const UINT32 LINEARIZATION33_Q_FACTOR_SLOPE    = 11;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Linearization33 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class IFELinearization33Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput          Pointer to the Input data
    /// @param  pData           Pointer to the interpolatin result
    /// @param  pUnpackedField  Pointer to the unpacked value
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const Linearization33InputData*                    pInput,
        linearization_3_3_0::linearization33_rgn_dataType* pData,
        VOID*                                              pUnpackedField);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateDelta
    ///
    /// @brief  CalculateDelta Function for Linearization33 Module
    ///
    /// @param  pLutP           pointer to the LUT variable
    /// @param  pLutBase        pointer to LUT base
    /// @param  pLutDelta       pointer to the LUT Delta
    /// @param  pedestalEnable  Pedestal is enabled or not
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateDelta(
        FLOAT* pLutP,
        FLOAT* pLutBase,
        FLOAT* pLutDelta,
        BOOL   pedestalEnable);
};
#endif // IFELINEARIZATION33SETTING_H
