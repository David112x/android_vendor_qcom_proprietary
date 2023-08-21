// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  abf40setting.h
/// @brief ABF40 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef ABF40SETTING_H
#define ABF40SETTING_H
#include "abf_4_0_0.h"
#include "iqsettingutil.h"
#include "iqcommondefs.h"
#include "bls12setting.h"

static const UINT32 ABF40_NUM_CHANNELS                  = 4;
static const UINT32 ABF40_NUM_ANCHORS                   = 4;
static const UINT32 ABF40_NUM_BLOCK_PIX_LEVELS          = 2;
static const UINT32 ABF40_BLK_OPT_MIN                   = 0;
static const UINT32 ABF40_BLK_OPT_MAX                   = 2;
static const UINT32 ABF40_BLK_PIX_LEVEL_MIN             = 0;
static const UINT32 ABF40_BLK_PIX_LEVEL_MAX             = 4;
static const UINT32 ABF40_DISTANCE_KER_0_MIN            = 1;
static const UINT32 ABF40_DISTANCE_KER_0_MAX            = 4;
static const UINT32 ABF40_DISTANCE_KER_1_MIN            = 0;
static const UINT32 ABF40_DISTANCE_KER_1_MAX            = 2;
static const UINT32 ABF40_DISTANCE_KER_2_MIN            = 0;
static const UINT32 ABF40_DISTANCE_KER_2_MAX            = 1;
static const UINT32 ABF40_CURVE_OFFSET_MIN              = 0;
static const UINT32 ABF40_CURVE_OFFSET_MAX              = 127;
static const UINT32 ABF40_EDGE_SOFTNESS_MIN             = 0;
static const UINT32 ABF40_EDGE_SOFTNESS_MAX             = 4095;
static const UINT32 ABF40_EDGE_SOFTNESS_Q_FACTOR        = 8;
static const UINT32 ABF40_FILTER_STRENGTH_Q_FACTOR      = 8;
static const UINT32 ABF40_FILTER_STRENGTH_MIN           = 0;
static const UINT32 ABF40_FILTER_STRENGTH_MAX           = 256;
static const UINT32 ABF40_MINMAX_SHFT_MIN               = 0;
static const UINT32 ABF40_MINMAX_SHFT_MAX               = 15;
static const UINT32 ABF40_MINMAX_OFFSET_MIN             = 0;
static const UINT32 ABF40_MINMAX_OFFSET_MAX             = (1 << 12) - 1; ///<  (1<<(8+e))-1? e=4 according to Guo
static const UINT32 ABF40_MINMAX_BLS_MIN                = 0;
static const UINT32 ABF40_MINMAX_BLS_MAX                = (1<<12)-1; ///< (1<<(8+e))-1

static const UINT32 ABF40_R_SQUARE_INIT_MIN             = 0;
static const UINT32 ABF40_R_SQUARE_INIT_MAX             = IQSettingUtils::MAXUINTBITFIELD(28);
static const UINT32 ABF40_R_SQUARE_SHFT_MIN             = 0;
static const UINT32 ABF40_R_SQUARE_SHFT_MAX             = 15;
static const UINT32 ABF40_R_SQUARE_SCALE_MIN            = 0;
static const UINT32 ABF40_R_SQUARE_SCALE_MAX            = 127;
static const INT32  ABF40_R_SQUARE_MAX_VAL              = 8191;
static const UINT32 ABF40_RNR_ANCHOR_MIN                = 0;
static const UINT32 ABF40_RNR_ANCHOR_MAX                = 4095;
static const UINT32 ABF40_RNR_BASE0_MIN                 = 256;
static const UINT32 ABF40_RNR_BASE0_MAX                 = 4095;
static const UINT32 ABF40_RNR_BASE1_MIN                 = 0;
static const UINT32 ABF40_RNR_BASE1_MAX                 = 256;
static const UINT32 ABF40_RNR_SHIFT_MIN                 = 0;
static const UINT32 ABF40_RNR_SHIFT_MAX                 = 15;
static const UINT32 ABF40_RNR_SLOPE0_MIN                = 0;
static const UINT32 ABF40_RNR_SLOPE0_MAX                = 4095;
static const INT32  ABF40_RNR_SLOPE1_MIN                = -512;
static const INT32  ABF40_RNR_SLOPE1_MAX                = 511;
static const UINT32 ABF40_NPRSV_ANCHOR_MIN              = 0;
static const UINT32 ABF40_NPRSV_ANCHOR_MAX              = 4095;
static const UINT32 ABF40_NPRSV_BASE_MIN                = 0;
static const UINT32 ABF40_NPRSV_BASE_MAX                = 256;
static const UINT32 ABF40_NPRSV_SHFT_MIN                = 0;
static const UINT32 ABF40_NPRSV_SHFT_MAX                = 15;
static const INT32  ABF40_NPRSV_SLOPE_MIN               = -512;
static const INT32  ABF40_NPRSV_SLOPE_MAX               = 511;
static const UINT32 ABF40_ACT_FAC_LUT_MIN               = 0;
static const UINT32 ABF40_ACT_FAC_LUT_MAX               = 511;
static const UINT32 ABF40_ACT_FAC0_MIN                  = 0;
static const UINT32 ABF40_ACT_FAC0_MAX                  = 8191;
static const UINT32 ABF40_ACT_FAC1_MIN                  = 0 ;
static const UINT32 ABF40_ACT_FAC1_MAX                  = (1<<13)-1;
static const UINT32 ABF40_ACT_THD0_MIN                  = 1;
static const UINT32 ABF40_ACT_THD0_MAX                  = (1<<13)-1;
static const UINT32 ABF40_ACT_THD1_MIN                  = 0;
static const UINT32 ABF40_ACT_THD1_MAX                  = (1<<12)-1;
static const UINT32 ABF40_ACT_SMTH_THD_MIN              = 0;
static const UINT32 ABF40_ACT_SMTH_THD_MAX              = 255;
static const UINT32 ABF40_DARK_THD_MIN                  = 0;
static const UINT32 ABF40_DARK_THD_MAX                  = (1<<12)-1;
static const UINT32 ABF40_DARK_FAC_LUT_MIN              = 0;
static const UINT32 ABF40_DARK_FAC_LUT_MAX              = 256;
static const UINT32 ABF40_RGB_GAIN_RATIO_MIN            = 0;
static const UINT32 ABF40_RGB_GAIN_RATIO_MAX            = 4095;
static const UINT32 ABF40_EDGE_DETECT_THD_MIN           = 0;
static const UINT32 ABF40_EDGE_DETECT_THD_MAX           = 15;
static const UINT32 ABF40_EDGE_COUNT_LOW_MIN            = 0;
static const UINT32 ABF40_EDGE_COUNT_LOW_MAX            = 16;
static const UINT32 ABF40_EDGE_DETECT_NOISE_SCALER_MIN  = 0;
static const UINT32 ABF40_EDGE_DETECT_NOISE_SCALER_MAX  = 4095;
static const UINT32 ABF40_EDGE_SMOOTH_STRENGTH_MIN      = 0;
static const UINT32 ABF40_EDGE_SMOOTH_STRENGTH_MAX      = 64;
static const UINT32 ABF40_EDGE_SMOOTH_NOISE_SCALAR_MIN  = 0;
static const UINT32 ABF40_EDGE_SMOOTH_NOISE_SCALAR_MAX  = 4095;
static const UINT32 DMIRAM_ABF40_NOISESTD_LENGTH        = 64;
static const UINT32 DMIRAM_ABF40_ACTIVITY_LENGTH        = 32;
static const UINT32 DMIRAM_ABF40_DARK_LENGTH            = 42;
static const UINT32 ABF40_NOISE_STD_LUT_MIN             = 0;
static const UINT32 ABF40_NOISE_STD_LUT_MAX             = 511;
static const UINT32 WIDTH                               = 4032;
static const UINT32 HEIGHT                              = 3024;
static const INT32  ABF40_BX_MIN                        = IQSettingUtils::MININT32BITFIELD(14);
static const INT32  ABF40_BX_MAX                        = IQSettingUtils::MAXINT32BITFIELD(14);
static const INT32  ABF40_BY_MIN                        = IQSettingUtils::MININT32BITFIELD(14);
static const INT32  ABF40_BY_MAX                        = IQSettingUtils::MAXINT32BITFIELD(14);

// NOWHINE NC004c: Share code with system team
struct ABF40UnpackedField
{
    UINT16 enable;                          ///< ABF40_Module Enable
    UINT16 LUTBankSel;                      ///< ABF40_Module LookUpTable Bank selection
    UINT16 bilateralEnable;                 ///< ABF_MODULE_CFG.FILTER_EN in SWI
    UINT16 crossProcessEnable;              ///< Cross Process Enable
    UINT16 directSmoothEnable;              ///< Dir Smth Enable
    UINT16 darkDesatEnable;                 ///< Dark Desat Enable
    UINT16 darkSmoothEnable;                ///< Dark Smooth Enable
    UINT16 actAdjEnable;                    ///< Act Adj Enable
    UINT16 minmaxEnable;                    ///< MinMax Enable
    UINT16 blockOpt;                        ///< ABF_MODULE_CFG.BLOCK_MATCH_PATTERN_RB in SWI

    UINT16 blockPixLevel[ABF40_NUM_BLOCK_PIX_LEVELS];   ///< [0] = PIX_MATCH_LEVEL_RB, [1] = PIX_MATCH_LEVEL_G in SWI
    UINT16 distanceLevel[3][6];                         ///< 3u/2u/2u, [][]; [][0-2, R/B; 3-5, Gr/Gb] , DISTANCE_LEVEL in SWI
    UINT16 curveOffset[ABF40_NUM_CHANNELS];             ///< 7u, ch 4
    UINT16 edgeSoftness[ABF40_NUM_CHANNELS];            ///< 9u, ch 4   [0/1/2/3]=[R,GR,GB,B]
    UINT16 filterStrength[ABF40_NUM_CHANNELS];          ///< 9u, ch 4   [0/1/2/3]=[R,GR,GB,B]

    UINT32 noiseStdLUT[MaxLUTBank][DMIRAM_ABF40_NOISESTD_LENGTH]; ///< 64-entry,32b per entry table stores shot noise,

    UINT16 minmaxMaxShift;                  ///< ABF_Module MINMAX_MAXSHFT
    UINT16 minmaxMinShift;                  ///< ABF_Module MINMAX_MINSHFT
    UINT16 minmaxOffset;                    ///< (bs-2)u
    UINT16 minmaxBLS;                       ///< (bs-2)u

    INT16  bx;                                  ///< 14s, init_h_offset-h_center
    INT16  by;                                  ///< 14s, init_v_offset-v_center
    INT16  RNRSlope1[ABF40_NUM_ANCHORS];        ///< 10s, RNR_THRESH_SLOPE in SWI
    INT16  nprsvSlope[2][ABF40_NUM_ANCHORS];    ///< 9s, NP_SLOPE in SWI
    UINT16 rSquareScale;                        ///< 7u
    UINT16 rSquareShift;                        ///< 4u
    UINT16 RNRAnchor[ABF40_NUM_ANCHORS];        ///< 12u
    UINT16 RNRBase0[ABF40_NUM_ANCHORS];         ///< 8u, 4 anchors, 2 tables for bilateral coeff, RNR_NOISE_BASE in SWI
    UINT16 RNRSlope0[ABF40_NUM_ANCHORS];        ///< 12u, RNR_NOISE_SLOPE in SWI
    UINT16 RNRShift0[ABF40_NUM_ANCHORS];        ///< 4u, RNR_NOISE_SHIFT in SWI
    INT16  RNRBase1[ABF40_NUM_ANCHORS];         ///< 8u, 4 anchors, 2 tables for soft-thresholding, RNR_THRESH_BASE in SWI
    INT16  RNRShift1[ABF40_NUM_ANCHORS];        ///< 4u, RNR_THRESH_SHIFT in SWI
    UINT32 rSquareInit;                         ///< 28u, (init_h_offset-h_center)^2 + (init_v_offset-v_center)^2

    UINT16 nprsvAnchor[ABF40_NUM_ANCHORS];      ///< 12u, NP_ANCHOR in SWI
    UINT16 nprsvBase[2][ABF40_NUM_ANCHORS];     ///< 8u, 4 anchors, 2 tables [0] for R/B, [1] for [Gr/Gb], NP_BASE in SWI
    UINT16 nprsvShift[2][ABF40_NUM_ANCHORS];    ///< 4u, NP_SHIFT in SWI

    UINT16 activityFactorLUT[MaxLUTBank][DMIRAM_ABF40_ACTIVITY_LENGTH];     ///< LUT for activity adjust
    UINT16 activityFactor0;                                                 ///< normalization parameters
    UINT16 activityFactor1;                                                 ///< normalization parameters
    UINT16 activityThreshold0;                                              ///< normalization parameters
    UINT16 activityThreshold1;                                              ///< normalization parameters
    UINT16 activitySmoothThreshold0;                                        ///< smoothing kernel
    UINT16 activitySmoothThreshold1;                                        ///< smoothing kernel

    UINT16 darkThreshold;                                       ///< normalization parameters
    UINT16 darkFactorLUT[MaxLUTBank][DMIRAM_ABF40_DARK_LENGTH]; ///< parameter for dark_factor adjustment
    UINT16 grRatio;                                             ///< 12u Green-Red ratio
    UINT16 rgRatio;                                             ///< 12u Red-Green ratio
    UINT16 gbRatio;                                             ///< 12u Green-Blue ratio
    UINT16 bgRatio;                                             ///< 12u Blue-Green ratio
    UINT16 rbRatio;                                             ///< 12u Red-Blue ratio
    UINT16 brRatio;                                             ///< 12u Blue-Red ratio

    UINT16 edgeDetectThreshold;                         ///< 4uQ2
    UINT16 edgeCountLow;                                ///< parameter for edge_detection
    UINT16 edgeDetectNoiseScalar;                       ///< 12uQ6
    UINT16 edgeSmoothStrength;                          ///< Q6
    UINT16 edgeSmoothNoiseScalar[ABF40_NUM_CHANNELS];   ///< 12uQ8, [0/1/2/3]=[R,GR,GB,B]
};

// NOWHINE NC004c: Share code with system team
struct ABF40BLS12UnpackedField
{
    ABF40UnpackedField* pUnpackedRegisterDataABF; ///< ABF40_register Data
    BLS12UnpackedField* pUnpackedRegisterDataBLS; ///< BLS12_register Data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements ABF40 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class ABF40Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the input data
    /// @param  pData          Pointer to the interpolation result
    /// @param  pReserveType   Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable  Pointer to control variable that enables the module
    /// @param  pUnpackedField Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        ABF40InputData*                                       pInput,
        abf_4_0_0::abf40_rgn_dataType*                        pData,
        const abf_4_0_0::chromatix_abf40_reserveType*         pReserveType,
        abf_4_0_0::chromatix_abf40Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pUnpackedField);
};
#endif // ABF40SETTING_H
