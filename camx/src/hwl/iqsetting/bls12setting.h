// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  bls12setting.h
/// @brief BLS12 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BLS12SETTING_H
#define BLS12SETTING_H

#include "bls_1_2_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

/// @brief Unpacked BLS12 output from common IQ library
// NOWHINE NC004c: Share code with system team
struct BLS12UnpackedField
{
    UINT16 enable;        ///< Module enable flag
    UINT16 offset;        ///< identical across 4 channels
    UINT32 scale;         ///< identical across 4 channels
    UINT16 bayerPattern;  ///< Bayer pattern, 2u, 0: RGGB, 1: GRBG, 2: BGGR, 3: GBRG
    UINT16 thresholdR;    ///< Threshold for R channel
    UINT16 thresholdGR;   ///< Threshold for GR channel
    UINT16 thresholdGB;   ///< Threshold for GB channel
    UINT16 thresholdB;    ///< Threshold for B channel
};

/// @brief Debug BLS12 output from common IQ library
// NOWHINE NC004c: Share code with system team
struct BLS12DebugBuffer
{
    bls_1_2_0::bls12_rgn_dataType interpolationResult;   ///< Interpolated chromatix data
    BLS12UnpackedField            unpackedData;          ///< Calculated unpacked data
};

static const UINT32 BLS12_SCALE_Q_FACTOR = 11;
static const UINT32 BLS12_SCALE_MIN      = 0;
static const UINT32 BLS12_SCALE_MAX      = 131071;
static const UINT32 BLS12_OFFSET_MIN     = 0;
static const UINT32 BLS12_OFFSET_MAX     = 16383;
static const UINT32 BLS12_THR_R_MIN      = 0;
static const UINT32 BLS12_THR_R_MAX      = 16383;
static const UINT32 BLS12_THR_GR_MIN     = 0;
static const UINT32 BLS12_THR_GR_MAX     = 16383;
static const UINT32 BLS12_THR_GB_MIN     = 0;
static const UINT32 BLS12_THR_GB_MAX     = 16383;
static const UINT32 BLS12_THR_B_MIN      = 0;
static const UINT32 BLS12_THR_B_MAX      = 16383;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements BLS12 module setting calculation5
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class BLS12Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the input data
    /// @param  pData          Pointer to the interpolation result
    /// @param  pModuleEnable  Pointer to control variable that enables the module
    /// @param  pUnpackedField Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const BLS12InputData*                                 pInput,
        bls_1_2_0::bls12_rgn_dataType*                        pData,
        bls_1_2_0::chromatix_bls12Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pUnpackedField);
};
#endif // BLS12SETTING_H
