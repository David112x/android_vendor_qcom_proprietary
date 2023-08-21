// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  bpsgic30setting.h
/// @brief GIC30 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BPSGIC30SETTING_H
#define BPSGIC30SETTING_H

#include "gic_3_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

const INT32 DMIRAM_GIC_NOISESTD_LENGTH_V30 = 64;
const UINT8 MaxNumLUT                      = 2;
const UINT8 NumAnchorBase                  = 6;
const UINT8 NumSlopeShift                  = 6;
const UINT8 NumNoiseScale                  = 4;

// NOWHINE NC004c: Share code with system team
struct GIC30UnpackedField
{
    UINT16 enable;                            ///< Module enable flag
    UINT16 enableGIC;                         ///< Enable GIC
    UINT16 enable3D;                          ///< Enable 3D, default disabled
    UINT16 LUTBankSel;                        ///< LUT bank select
    ///< 64-entry,32b per entry table stores shot noise, one for each level and each channel
    UINT32 noiseStdLUTLevel0[MaxNumLUT][DMIRAM_GIC_NOISESTD_LENGTH_V30];
    UINT16 GICNoiseScale;                     ///< 10uQ6
    UINT16 thinLineNoiseOffset;               ///< 14u, for adjustingng the threshold of thin line detection
    UINT16 GICStrength;                       ///< 9uQ8
    UINT16 enablePNR;                         ///< Enable PNR
    INT16  bx;                                ///< 14s, init_h_offset-h_center
    INT16  by;                                ///< 14s, init_v_offset-v_center
    UINT32 rSquareInit;                       ///< 28u, (init_h_offset-h_center)^2 + (init_v_offset-v_center)^2
    UINT16 rSquareScale;                      ///< 7u; range[0, 127]
    UINT16 rSquareShift;                      ///< 4u

    UINT16 RNRAnchor[NumAnchorBase];          ///< 10u
    UINT16 RNRBase[NumAnchorBase];            ///< 10u, 6 anchors
    INT16  RNRSlope[NumSlopeShift];           ///< 11s
    UINT16 RNRShift[NumSlopeShift];           ///< 4u
    UINT16 PNRNoiseScale[NumNoiseScale];      ///< 10uQ4
    UINT16 PNRStrength;                       ///< 9uQ8
};

// NOWHINE NC004c: Share code with system team
struct GIC30DebugBuffer
{
    gic_3_0_0::gic30_rgn_dataType interpolationResult; ///< Interpolated chromatix data
    GIC30UnpackedField            unpackedData;        ///< Calculated unpacked data
};

static const INT32 GIC30_GICNoiseScale_MIN                  = 0;
static const INT32 GIC30_GICNoiseScale_MAX                  = 1023;
static const INT32 GIC30_GICNoiseScale_Q_FACTOR             = 6;
static const INT32 GIC30_thinLineNoiseOffset_MIN            = 0;
static const INT32 GIC30_thinLineNoiseOffset_MAX            = 16383;
static const INT32 GIC30_GICCorrectionStrength_MIN          = 0;
static const INT32 GIC30_GICCorrectionStrength_MAX          = 256;
static const INT32 GIC30_GICCorrectionStrength_Q_FACTOR     = 8;
static const INT32 GIC30_NOISE_STD_LUT_LEVEL0_MIN           = 0;
static const INT32 GIC30_NOISE_STD_LUT_LEVEL0_PRE_MAX       = 512;
static const INT32 GIC30_NOISE_STD_LUT_LEVEL0_MAX           = (1 << 14) - 1;
static const INT32 GIC30_NOISE_STD_LUT_LEVEL0_DELTA_MIN     = -(1 << 13);
static const INT32 GIC30_NOISE_STD_LUT_LEVEL0_DELTA_MAX     = (1 << 13) - 1;
static const INT32 GIC30_NOISE_STD_LUT_LEVEL0_SHIFT         = (1 << 14);
static const INT32 GIC30_BX_MIN                             = -8192;        // -3120  // FIXME: taking from ABF
static const INT32 GIC30_BX_MAX                             = 8191;         // 3110   // FIXME: taking from ABF
static const INT32 GIC30_BY_MIN                             = -8192;        // -8192  // FIXME: taking from ABF
static const INT32 GIC30_BY_MAX                             = 8191;         // -5     // FIXME: taking from ABF
static const INT32 GIC30_R_SQUARE_INIT_MIN                  = 0;            // 25
static const INT32 GIC30_R_SQUARE_INIT_MAX                  = (1 << 27);    // 76843264
static const INT32 GIC30_R_SQUARE_SHFT_MIN                  = 0;
static const INT32 GIC30_R_SQUARE_SHFT_MAX                  = 15;
static const INT32 GIC30_MAX_R_SQUARE_VAL                   = 8191;
static const INT32 GIC30_R_SQUARE_SCALE_MIN                 = 0;
static const INT32 GIC30_R_SQUARE_SCALE_MAX                 = 127;
static const INT32 GIC30_RNR_ANCHOR_MIN                     = 0;
static const INT32 GIC30_RNR_ANCHOR_MAX                     = 1023;
static const INT32 GIC30_RNR_BASE_MIN                       = 0;
static const INT32 GIC30_RNR_BASE_MAX                       = 1023;
static const INT32 GIC30_RNR_SHIFT_MIN                      = 0;
static const INT32 GIC30_RNR_SHIFT_MAX                      = 15;
static const INT32 GIC30_RNR_SLOPE_MIN                      = -1024;
static const INT32 GIC30_RNR_SLOPE_MAX                      = 1023;
static const FLOAT GIC30_RNR_DELTA_RATIO_MIN                = 0.0f;
static const FLOAT GIC30_RNR_DELTA_RATIO_MAX                = 1.0f;
static const INT32 GIC30_PNR_NoiseScale_Q_FACTOR            = 6;
static const INT32 GIC30_PNR_NoiseScale_MIN                 = 0;
static const INT32 GIC30_PNR_NoiseScale_MAX                 = 1023;
static const INT32 GIC30_PNR_CorrectionStrength_Q_FACTOR    = 8;
static const INT32 GIC30_PNR_CorrectionStrength_MIN         = 0;
static const INT32 GIC30_PNR_CorrectionStrength_MAX         = 256;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements GIC30 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class BPSGIC30Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput          Pointer to the input data to GIC30 module
    /// @param  pData           Pointer to the interpolated data
    /// @param  pReserveType    Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable   Pointer to the variable(s) to enable this module
    /// @param  pUnpackedField  Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const GIC30InputData*                                 pInput,
        gic_3_0_0::gic30_rgn_dataType*                        pData,
        gic_3_0_0::chromatix_gic30_reserveType*               pReserveType,
        gic_3_0_0::chromatix_gic30Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pRegCmd);
};
#endif // BPSGIC30SETTING_H
