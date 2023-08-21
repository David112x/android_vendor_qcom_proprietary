// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ltm13setting.h
/// @brief LTM13 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LTM13SETTING_H
#define LTM13SETTING_H
#include "ltm_1_3_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"


/// LTM module macro definitions
static const UINT32 LTM_VAL_SHIFT_V13           = 5;        ///< (img_val >> VAL_SHIFT) fit into VAL_BIN
static const UINT32 LTM_VAL_OFFSET_V13          = 16;       ///<  (256/(VAL_BIN-1))/2

static const UINT32 LTM_VAL_BIN_V13             = 9;        ///< value bins
static const UINT32 LTM_LUT3D_W_V13             = 13;
static const UINT32 LTM_LUT3D_H_V13             = 10;
static const UINT32 LTM_LUT3D_Z_V13             = 9;
static const UINT32 LTM_DC_CELLWIDTH_V13        = 24;
static const UINT32 LTM_DC_CELLHEIGHT_V13       = 24;
static const UINT32 LTM_IMG_WIDTH_DS_V13        = (LTM_DC_CELLWIDTH_V13*(LTM_LUT3D_W_V13 - 1));
static const UINT32 LTM_IMG_HEIGHT_DS_V13       = (LTM_DC_CELLHEIGHT_V13*(LTM_LUT3D_H_V13 - 1));
static const UINT32 DMIRAM_LTM_AVG3D_LENGTH_V13 = 140;      ///< (13*10*9)/3/3, split into 3RAMS,
                                                            ///< each 32-bit entry has 3 LUT entries
static const UINT32 LTM_SCALE_PHASE_Q_V13       = 14;

// #define MAX_DOWNSCALE_RATIO_DS_V20 128
static const UINT32 LTM_SCALER_PHASE_ACCUM_Q_BITS_V13 = 21;

static const UINT32 MASK_FILTER_KERNEL_SIZE     = 6;
static const UINT32 LTM_WEIGHT_LUT_SIZE         = 12;
static const UINT32 LTM_GAMMA_LUT_SIZE          = 64;
static const UINT32 LTM_SCALE_LUT_SIZE          = 65;
static const UINT32 LTM_CURVE_LUT_SIZE          = 65;
static const UINT32 LCE_SCALE_LUT_SIZE          = 17;
static const FLOAT  RATIO_64                    = 64.0f;
static const FLOAT  RATIO_32                    = 32.0f;
static const FLOAT  RATIO_16                    = 16.0f;

static const INT    LTM_LUT_SIZE                = 64;
static const FLOAT  LTM_MIN_VALUE               = 0.0f;
static const FLOAT  LTM_MAX_VALUE               = 4095.0f;
static const FLOAT  LTM_SCALE_MAX_VALUE         = 2047.0f;
static const FLOAT  LTM_GAMMA_MIN_VALUE         = 0.0f;
static const FLOAT  LTM_GAMMA_MAX_VALUE         = 4095.0f;
static const UINT32 LTM_SCALE_Q                 = 10;
static const UINT32 LTM_CLAMP_Q                 = 3;
static const UINT32 LTM_BIT_DEPTH               = 12;

static const UINT LTM13NumBins                  = 1024;

static FLOAT inverseGammaIn[LTM_GAMMA_LUT_SIZE + 1] =
{
    0, 16, 32, 47, 63, 86, 110, 135,
    163, 191, 223, 255, 290, 326, 364, 403,
    443, 486, 529, 575, 622, 670, 720, 771,
    824, 879, 934, 992, 1050, 1110, 1172, 1235,
    1299, 1365, 1433, 1501, 1571, 1643, 1716, 1790,
    1866, 1943, 2021, 2101, 2183, 2265, 2349, 2435,
    2521, 2610, 2699, 2790, 2882, 2976, 3071, 3167,
    3265, 3364, 3464, 3566, 3669, 3774, 3879, 3987,
    4095
};

static FLOAT inverseGammaInNormalized[LTM_GAMMA_LUT_SIZE + 1] =
{
    0.0002442f, 0.0041514f, 0.0080586f, 0.0117216f, 0.0156288f, 0.0212454f, 0.0271062f, 0.0332112f,
    0.0400488f, 0.0468864f, 0.0547008f, 0.0625152f, 0.0710622f, 0.0798534f, 0.0891330f, 0.0986568f,
    0.1084249f, 0.1189255f, 0.1294261f, 0.1406593f, 0.1521367f, 0.1638583f, 0.1760683f, 0.1885225f,
    0.2014652f, 0.2148962f, 0.2283272f, 0.2424908f, 0.2566544f, 0.2713064f, 0.2864468f, 0.3018315f,
    0.3174603f, 0.3335775f, 0.3501831f, 0.3667887f, 0.3838827f, 0.4014652f, 0.4192918f, 0.4373626f,
    0.4559218f, 0.4747252f, 0.4937728f, 0.5133089f, 0.5333333f, 0.5533577f, 0.5738705f, 0.5948717f,
    0.6158730f, 0.6376068f, 0.6593406f, 0.6815628f, 0.7040293f, 0.7269841f, 0.7501831f, 0.7736263f,
    0.7975579f, 0.8217338f, 0.8461538f, 0.8710622f, 0.8962148f, 0.9218559f, 0.9474969f, 0.9738705f,
    1.0000000f
};


static FLOAT ltmFixedGamma[LTM_GAMMA_LUT_SIZE + 1] =
{
    0, 259, 432, 577, 705, 822, 930, 1031,
    1127, 1218, 1304, 1387, 1468, 1545, 1620, 1692,
    1763, 1832, 1899, 1964, 2028, 2091, 2152, 2212,
    2271, 2329, 2386, 2442, 2497, 2551, 2604, 2657,
    2708, 2759, 2810, 2859, 2909, 2957, 3005, 3052,
    3099, 3145, 3191, 3236, 3281, 3325, 3369, 3412,
    3455, 3498, 3540, 3582, 3624, 3665, 3705, 3746,
    3786, 3825, 3865, 3904, 3943, 3981, 4019, 4057,
    4095
};


/// @brief HW SWI struct
// NOWHINE NC004c: Share code with system team
struct MNScaleDownInStruct_V20
{
    UINT16 enable;                 ///< Enable downscaler inside data collection module

    UINT16 h_en;                   ///< h_en
    UINT16 v_en;                   ///< v_en
    UINT16 input_h;                ///< Input width for early termination, n means n+1
    UINT16 input_v;                ///< Input height for early termination, n means n+1
    UINT16 early_termination_h;    ///< 0 or 1
    UINT16 early_termination_v;    ///< 0 or 1
    INT32  phase_init_h;           ///< For left padding, signed
    INT32  phase_init_v;           ///< For top  padding, signed
    UINT32 phase_step_h;           ///< N/M * (1 << SCALER_PHASE_ACCUM_Q_BITS)
    UINT32 phase_step_v;           ///< N/M * (1 << SCALER_PHASE_ACCUM_Q_BITS)
    UINT16 horizontal_interp_reso; ///< 3: [1x, 16x]; 2: (16x, 32x]; 1: (32x, 64x]; 0: (64x, 128x]
    UINT16 vertical_interp_reso;   ///< 3: [1x, 16x]; 2: (16x, 32x]; 1: (32x, 64x]; 0: (64x, 128x]
                                   ///< 2x2 output pixel rounding pattern selection
    UINT16 rounding_option_h;      ///< 0x0: 1111; 0x1: 1001; 0x2: 0110; 0x3: 0000
    UINT16 rounding_option_v;      ///< 0x0: 1111; 0x1: 1001; 0x2: 0110; 0x3: 0000
};

/// @brief LTM grid structure
// NOWHINE NC004c: Share code with system team
struct LtmGridStruct25b_IPE_V13
{
    UINT16 valsum[LTM_LUT3D_Z_V13]; ///< Value Sum
    UINT16 pixcnt[LTM_LUT3D_Z_V13]; ///< Pixel Count
};

/// @brief LTM Aeverage curve structure
// NOWHINE NC004c: Share code with system team
struct LtmGridStruct10b_V13
{
    INT16 avg[LTM_LUT3D_Z_V13];     ///< Average
} ;

/// @brief: LTM LUT structure
// NOWHINE NC004c: Share code with system team
struct LTM13LUTInStruct
{
    INT32*   pLUTTable;      ///< LUT Table. i.e. LA_CURVE, LTM_CURVE etc
    UINT16   numEntries;     ///< number of entries in the LUT Table
};

/// @brief LTM unpackfield structure
// NOWHINE NC004c: Share code with system team
struct LTM13UnpackedField
{
    ///< data-collection
    UINT16                   enable;                    ///< Module enable flag
    UINT16                   data_collect_en;           ///< 1
    UINT16                   dc_3dtable_avg_pong_sel;   ///< 1
    UINT16                   dc_init_cellnum_x;         ///< 4
    UINT16                   dc_init_dx;                ///< 5
    UINT16                   dc_3d_sum_clear;           ///< 1
    UINT16                   bin_init_cnt;              ///< 4
    UINT16                   dc_conv_start_cell_x;      ///< 4u
    UINT16                   dc_conv_end_cell_x;        ///< 4u
    UINT16                   dc_xstart;                 ///< 8u post-cropping
    UINT16                   dc_xend;                   ///< 8u post-cropping

    MNScaleDownInStruct_V20  scale_in_cfg;              ///< Down scale configuration

    ///< down-scaling
    UINT16                   ds_horizontal_skip_cnt;    ///< 10  pre-cropping
    UINT16                   ds_vertical_skip_cnt;      ///< 14
    ///< 0 - ds_size=288x216, cell_w/h=24; 1 - ds_size=144x108, cell_w/h=12; 2 - ds_size=72x54, cell_w/h=6;
    UINT16                   ds_fac;                    ///< 2

    ///< image processing
    UINT16                   img_process_en;            ///< 1
    UINT16                   ip_init_cellnum_x;         ///< 4
    UINT16                   ip_init_dx;                ///< 11
    UINT32                   ip_init_px;                ///< 18
    UINT16                   ip_init_cellnum_y;         ///< 4
    UINT16                   ip_init_dy;                ///< 11
    UINT32                   ip_init_py;                ///< 18
    UINT16                   ip_inv_cellwidth;          ///< 14
    UINT16                   ip_inv_cellheight;         ///< 14
    UINT16                   ip_cellwidth;              ///< 11
    UINT16                   ip_cellheight;             ///< 11
    UINT16                   ip_3dtable_avg_pong_sel;   ///< pong sel

    UINT16                   igamma_en;                 ///< 1
    UINT16                   la_en;                     ///< 1

    UINT16                   lut_bank_sel;              ///< 1u
    UINT16                   c1;                        ///< c1: 10
    UINT16                   c2;                        ///< c2: 10
    UINT16                   c3;                        ///< c3: 10
    UINT16                   c4;                        ///< c4: 11
    LTM13LUTInStruct         wt;                        ///< length LTM_WEIGHT_LUT_SIZE, weight LUT

    UINT16                   mask_filter_kernel[MASK_FILTER_KERNEL_SIZE]; ///< 4u, 5x5 mask filtering
    UINT16                   mask_filter_scale;                           ///< 12u
    UINT16                   mask_filter_shift;                           ///< 5u
    UINT16                   mask_filter_en;                              ///< 1u

    LTM13LUTInStruct         la_curve;                                    ///< length LTM_CURVE_LUT_SIZE - 1, (10+e)u
    LTM13LUTInStruct         mask_rect_curve;                             ///< length LTM_CURVE_LUT_SIZE - 1, (10+e)u
    LTM13LUTInStruct         ltm_curve;                                   ///< length LTM_CURVE_LUT_SIZE - 1, (10+e)s
    LTM13LUTInStruct         ltm_scale;                                   ///< length LTM_SCALE_LUT_SIZE - 1, (10+e)s
    LTM13LUTInStruct         lce_scale_pos;                               ///< length LCE_SCALE_LUT_SIZE - 1, (10+e)s
    LTM13LUTInStruct         lce_scale_neg;                               ///< length LCE_SCALE_LUT_SIZE - 1, (10+e)s
    LTM13LUTInStruct         igamma64;                                    ///< length LTM_GAMMA_LUT_SIZE - 1, (10+e)s
    UINT16                   lce_thd;                                     ///< lce thd

    UINT16                   y_ratio_max;                                 ///< 10u
    UINT16                   debug_out_sel;                               ///< 2u

    ///< Additional LTM non-volatile variables (pass across frames)
    LtmGridStruct25b_IPE_V13 LUT25b[LTM_LUT3D_H_V13 * LTM_LUT3D_W_V13];   ///< lut25b
    LtmGridStruct10b_V13     avg3d[DMIRAM_LTM_AVG3D_LENGTH_V13];          ///< DMI ping-pong buffer of 3D bilateral averages
};

enum LTMDarkBrightRegionState
{
    LTMRegionIndexInit = 0,  ///< Region Index Init
    LTMDarkBoostRegion,      ///< Region Index Dark
    LTMBrightRegion,         ///< Region Index Bright
};

static const INT32 ONE_BIT_MAX              = 1;
static const INT32 ONE_BIT_MIN              = 0;
static const INT32 GAMMA_MAX                = (IQSettingUtils::MAXUINTBITFIELD(14));
static const INT32 GAMMA_MIN                = 0;
static const INT32 MASK_FILTER_KERNEL_MAX   = IQSettingUtils::MAXUINTBITFIELD(3);
static const INT32 MASK_FILTER_KERNEL_MIN   = 0;
static const INT32 LA_CURVE_MAX             = IQSettingUtils::MAXUINTBITFIELD(12);
static const INT32 LA_CURVE_MIN             = 0;
static const INT32 LTM_CURVE_MAX            = IQSettingUtils::MAXUINTBITFIELD(11);
static const INT32 LTM_CURVE_MIN            = -2048;
static const INT32 LTM_SCALE_MAX            = IQSettingUtils::MAXUINTBITFIELD(11);
static const INT32 LTM_SCALE_MIN            = -2048;
static const INT32 LCE_SCALE_POS_MAX        = IQSettingUtils::MAXUINTBITFIELD(11);
static const INT32 LCE_SCALE_POS_MIN        = -2048;
static const INT32 LCE_SCALE_NEG_MAX        = IQSettingUtils::MAXUINTBITFIELD(11);
static const INT32 LCE_SCALE_NEG_MIN        = -2048;
static const INT32 LCE_THD_MIN              = 0;
static const INT32 LCE_THD_MAX              = IQSettingUtils::MAXUINTBITFIELD(10);
static const INT32 Y_RATIO_MAX_MIN          = 0;
static const INT32 Y_RATIO_MAX_MAX          = IQSettingUtils::MAXUINTBITFIELD(10);
static const INT32 RGB2Y_CFG_1_MIN          = 0;
static const INT32 RGB2Y_CFG_1_MAX          = IQSettingUtils::MAXUINTBITFIELD(6);
static const INT32 RGB2Y_CFG_2_MIN          = 0;
static const INT32 RGB2Y_CFG_2_MAX          = (IQSettingUtils::MAXUINTBITFIELD(6) + 1);
static const INT32 MASK_FILTER_SCALE_MIN    = 0;
static const INT32 MASK_FILTER_SCALE_MAX    = (IQSettingUtils::MAXUINTBITFIELD(12) - 1);
static const INT32 MASK_FILTER_SHIFT_MIN    = 0;
static const INT32 MASK_FILTER_SHIFT_MAX    = (IQSettingUtils::MAXUINTBITFIELD(5) - 1);

static const INT32 IP_invWH_Q               = 18;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements LTM13 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class LTM13Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to input data of LTM13 Module
    /// @param  pData         Pointer to output of the interpolation algorithm
    /// @param  pReserveData  Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to ouput register value
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        LTM13InputData*                                       pInput,
        ltm_1_3_0::ltm13_rgn_dataType*                        pData,
        ltm_1_3_0::chromatix_ltm13_reserveType*               pReserveData,
        ltm_1_3_0::chromatix_ltm13Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateAdrcLtmCurves
    ///
    /// @brief  Generate ADRC LTM Curve
    ///
    /// @param  pInEqvlnt   Pointer to input data of LTM13 Module
    /// @param  pGain       Pointer to gain
    /// @param  pGamma      Pointer to Gamma module
    /// @param  pLtmScale   Pointer to LTM Scale
    /// @param  pLtmCurve   Pointer to LTM Curve
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void GenerateAdrcLtmCurves(
        FLOAT*        pInEqvlnt,
        FLOAT*        pGain,
        FLOAT*        pGamma,
        FLOAT*        pLtmScale,
        FLOAT*        pLtmCurve);
};

#endif // LTM13SETTING_H
