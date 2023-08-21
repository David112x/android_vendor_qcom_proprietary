// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifeabf34setting.h
/// @brief ABF34 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IFEABF34SETTING_H
#define IFEABF34SETTING_H
#include "abf_3_4_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"


static const UINT32 DMIRAM_ABF34_NOISESTD_LENGTH    = 64;
// Const Min Max values for ABF34
static const INT32  MAX_CONTROL_DIMENSIONS_ABF34    = 4;
static const UINT32 ABF3_CHANNEL_MAX                = 4;
static const UINT32 ABF3_LEVEL_MAX                  = 2;
static const FLOAT  ABF34_BPC_FMAX_MIN              = 0.0f;
static const FLOAT  ABF34_BPC_FMAX_MAX              = 63.0f;
static const FLOAT  ABF34_BPC_FMIN_MIN              = 0.0f;
static const FLOAT  ABF34_BPC_FMIN_MAX              = 63.0f;
static const FLOAT  ABF34_BPC_MAXSHFT_MIN           = 0.0f;
static const FLOAT  ABF34_BPC_MAXSHFT_MAX           = 14.0f;
static const FLOAT  ABF34_BPC_MINSHFT_MIN           = 0.0f;
static const FLOAT  ABF34_BPC_MINSHFT_MAX           = 14.0f;
static const FLOAT  ABF34_BPC_OFFSET_MIN            = 0.0f;
static const FLOAT  ABF34_BPC_OFFSET_MAX            = 4095.0f;
static const INT32  ABF34_BPC_BLS_MIN               = 0;
static const INT32  ABF34_BPC_BLS_MAX               = 4095;
static const FLOAT  ABF34_BLKPIX_LEV_MIN            = 0.0f;
static const FLOAT  ABF34_BLKPIX_LEV_MAX            = 4.0f;
static const INT32  ABF34_NOISE_STD_LUT_LEVEL0_MIN  = 0;
static const INT32  ABF34_NOISE_STD_LUT_LEVEL0_MAX  = 0;
static const FLOAT  ABF34_DISTANCE_KER_0_MIN        = 1.0f;
static const FLOAT  ABF34_DISTANCE_KER_0_MAX        = 4.0f;
static const FLOAT  ABF34_DISTANCE_KER_1_MIN        = 0.0f;
static const FLOAT  ABF34_DISTANCE_KER_1_MAX        = 2.0f;
static const FLOAT  ABF34_DISTANCE_KER_2_MIN        = 0.0f;
static const FLOAT  ABF34_DISTANCE_KER_2_MAX        = 1.0f;
static const FLOAT  ABF34_CURVE_OFFSET_MIN          = 0.0f;
static const FLOAT  ABF34_CURVE_OFFSET_MAX          = 64.0f;
static const FLOAT  ABF34_NOISE_PRSV_ANCHOR_LO_MIN  = 0;
static const FLOAT  ABF34_NOISE_PRSV_ANCHOR_LO_MAX  = 1023;
static const INT32  ABF34_NOISE_PRSV_ANCHOR_GAP_MIN = 0;
static const INT32  ABF34_NOISE_PRSV_ANCHOR_GAP_MAX = 1023;
static const INT32  ABF34_NOISE_PRSV_LO_MIN         = 0;
static const INT32  ABF34_NOISE_PRSV_LO_MAX         = 256;
static const INT32  ABF34_NOISE_PRSV_SLOPE_MIN      = -256;
static const INT32  ABF34_NOISE_PRSV_SLOPE_MAX      = 256;
static const INT32  ABF34_NOISE_PRSV_SHFT_MIN       = 0;
static const INT32  ABF34_NOISE_PRSV_SHFT_MAX       = 10;
static const UINT32 ABFV34_NOISE_STD_LENGTH         = DMIRAM_ABF34_NOISESTD_LENGTH + 1;
static const INT32  ABF34_ANCHOR_TABLE_MIN          = 0;
static const INT32  ABF34_ANCHOR_TABLE_MAX          = 4095;
static const INT32  ABF34_BASE_TABLE_MIN            = 0;
static const INT32  ABF34_BASE_TABLE_MAX            = 255;
static const INT32  ABF34_SLOPE_TABLE_MIN           = 0;
static const INT32  ABF34_SLOPE_TABLE_MAX           = 255;
static const INT32  ABF34_SHIFT_TABLE_MIN           = 0;
static const INT32  ABF34_SHIFT_TABLE_MAX           = 15;
static const INT32  ABF34_BX_MIN                    = -3120;
static const INT32  ABF34_BX_MAX                    = 3110;
static const INT32  ABF34_BY_MIN                    = -8192;
static const INT32  ABF34_BY_MAX                    = -5;
static const INT32  ABF34_R_SQUARE_INIT_MIN         = 0;
static const INT32  ABF34_R_SQUARE_INIT_MAX         = 76843264;
static const INT32  ABF34_R_SQUARE_SHFT_MIN         = 0;
static const INT32  ABF34_R_SQUARE_SHFT_MAX         = 15;
static const UINT32 ABF34_RADIAL_POINTS             = 5;
static const UINT32 ABF34_BASETABLE_ENTRIES_RB_G    = 4;
static const UINT32 ABF34_NBITS                     = 9;
static const UINT32 ABF34_KERNEL_LENGTH             = 6;
static const UINT32 ABF34_ENABLE                    = 1;
static const UINT32 ABF34_DISABLE                   = 0;
static const UINT32 ABF34_CP_ENABLE                 = 1;
static const UINT32 ABF34_CP_DISABLE                = 0;
static const UINT32 ABF34_NOISESTDLUT_BASE_MIN      = 0;
static const UINT32 ABF34_NOISESTDLUT_BASE_MAX      = IQSettingUtils::MAXUINTBITFIELD(9);
static const UINT32 ABF34_NOISESTDLUT_DELTA_MIN     = 0;
static const UINT32 ABF34_NOISESTDLUT_DELTA_MAX     = IQSettingUtils::MAXUINTBITFIELD(9);
static const INT16  ABF34_MAX_RSQUARE_VAL           = 8191;
static const INT32  ABF34_NOISE_STD_QVAL            = 13;
static const INT32  ABF34_BASE_TABLE_Q_FACTOR       = 8;

/// @brief ABF34 Module Unpacked Data Field
// NOWHINE NC004c: Share code with system team
struct ABF34UnpackedField
{
    UINT16     enable;                                                         ///< Module enable flag
    UINT16     lut_bank_sel;                                                   ///< Module lut_bank_sel flag
    UINT16     cross_process_en;                                               ///< Module cross_process_en flag
    UINT16     filter_en;                                                      ///< Module filter_en flag
    UINT16     single_bpc_en;                                                  ///< Module single_bpc_en flag
    UINT16     bpc_fmax;                                                       ///< Module bpc_fmax flag
    UINT16     bpc_fmin;                                                       ///< Module bpc_fmin flag
    UINT16     bpc_maxshft;                                                    ///< Module bpc_maxshft flag
    UINT16     bpc_minshft;                                                    ///< Module bpc_minshft flag
    UINT16     bpc_offset;                                                     ///< Module bpc_offset flag
    UINT16     bpc_bls;                                                        ///< Module bpc_bls flag
    UINT16     blk_pix_matching_rb;                                            ///< Module blk_pix_matching_rb flag
    UINT16     blk_pix_matching_g;                                             ///< Module blk_pix_matching_g flag
    UINT32     noise_std_lut_level0[MaxLUTBank][DMIRAM_ABF34_NOISESTD_LENGTH]; ///< Module noise_std_lut_level0 flag
    UINT16     distance_ker[2][3];                                             ///< Module distance_ker flag
    UINT16     curve_offset[4];                                                ///< Module curve_offset flag
    UINT16     noise_prsv_anchor_lo;                                           ///< Module noise_prsv_anchor_lo flag
    UINT16     noise_prsv_anchor_gap;                                          ///< Module noise_prsv_anchor_gap flag
    UINT16     noise_prsv_lo[2];                                               ///< Module noise_prsv_lo flag
    INT16      noise_prsv_slope[2];                                            ///< Module noise_prsv_slope flag
    UINT16     noise_prsv_shft[2];                                             ///< Module noise_prsv_shft flag
    UINT16     anchor_table[4];                                                ///< Module anchor_table flag
    UINT16     base_table[2][4];                                               ///< Module base_table flag
    UINT16     slope_table[2][4];                                              ///< Module slope_table flag
    UINT16     shift_table[2][4];                                              ///< Module shift_table flag
    INT16      bx;                                                             ///< Module bx flag
    INT16      by;                                                             ///< Module by flag
    UINT32     r_square_init;                                                  ///< Module r_square_init flag
    UINT16     r_square_shft;                                                  ///< Module r_square_shft flag
    UINT16     edge_softness[4];                                               ///< Module edge_softness flag
};

// NOWHINE NC004c: Share code with system team
struct ABF34DebugBuffer
{
    abf_3_4_0::abf34_rgn_dataType  interpolationResult;    ///< Interpolated chromatix data
    ABF34UnpackedField             unpackedData;           ///< Calculated unpacked data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements ABF34 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class IFEABF34Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateNoiseStandardLUT
    ///
    /// @brief  Calculate GenerateNoiseStandardLUT
    ///
    /// @param  pLut_table Input LUT table
    /// @param  pLut0      Output LUT tale after packing
    /// @param  length     Lenght of table
    /// @param  absflag    Absolute value required or not
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GenerateNoiseStandardLUT(
        FLOAT*  pLut_table,
        UINT32* pLut0,
        INT32   length,
        BOOL    absflag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput          Pointer to the input data
    /// @param  pData           Pointer to the interpolation result
    /// @param  pModuleEnable   Pointer to the Variables to enable this module
    /// @param  pReserveType    Pointer to the reserveType of this module
    /// @param  pUnpackedField  Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const ABF34InputData*                                 pInput,
        abf_3_4_0::abf34_rgn_dataType*                        pData,
        abf_3_4_0::chromatix_abf34Type::enable_sectionStruct* pModuleEnable,
        abf_3_4_0::chromatix_abf34_reserveType*               pReserveType,
        VOID*                                                 pUnpackedField);
};
#endif // IFEABF34SETTING_H
