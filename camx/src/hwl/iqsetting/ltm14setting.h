// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ltm14setting.h
/// @brief LTM14 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LTM14SETTING_H
#define LTM14SETTING_H
#include "ltm_1_4_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

/// LTM module macro definitions
static const UINT16 LTM_LUT3D_Z_V14               = 9;
static const UINT16 LTM_LUT3D_W_V14               = 13;
static const UINT16 LTM_LUT3D_H_V14               = 10;
static const UINT16 DMIRAM_LTM_AVG3D_LENGTH_V14   = 140; // (13*10*9)/3/3, split into 3RAMS, each 32-bit entry has 3 LUT entries
static const UINT16 LTM_DC_CELLWIDTH_V14          = 24;
static const UINT16 LTM_DC_CELLHEIGHT_V14         = 24;
static const UINT16 LTM_IMG_WIDTH_DS_V14          = (LTM_DC_CELLWIDTH_V14*(LTM_LUT3D_W_V14 - 1));  // 320
static const UINT16 LTM_IMG_HEIGHT_DS_V14         = (LTM_DC_CELLHEIGHT_V14*(LTM_LUT3D_H_V14 - 1)); // 240

// #define MAX_DOWNSCALE_RATIO_DS_V20 128
static const UINT32 LTM_SCALER_PHASE_ACCUM_Q_BITS_V14 = 21;

static const UINT32 MASK_FILTER_KERNEL_SIZE_V14   = 6;
static const UINT32 LTM14_WEIGHT_LUT_SIZE         = 12;
static const UINT32 LTM14_GAMMA_LUT_SIZE          = 64;
static const UINT32 LTM14_SCALE_LUT_SIZE          = 65;
static const UINT32 LTM14_CURVE_LUT_SIZE          = 65;
static const UINT32 LTM14_LCE_SCALE_LUT_SIZE      = 17;
static const FLOAT  LTM14_RATIO_64                = 64.0f;
static const FLOAT  LTM14_RATIO_32                = 32.0f;
static const FLOAT  LTM14_RATIO_16                = 16.0f;

static const INT    LTM14_LUT_SIZE                = 64;
static const FLOAT  LTM14_MIN_VALUE               = 0.0f;
static const FLOAT  LTM14_MAX_VALUE               = 4095.0f;
static const FLOAT  LTM14_SCALE_MAX_VALUE         = 2047.0f;
static const FLOAT  LTM14_GAMMA_MIN_VALUE         = 0.0f;
static const FLOAT  LTM14_GAMMA_MAX_VALUE         = 4095.0f;
static const UINT32 LTM14_SCALE_Q                 = 8;
static const UINT32 LTM14_CLAMP_Q                 = 3;
static const UINT32 LTM14_BIT_DEPTH               = 12;

static const INT32 LTM14_ONE_BIT_MAX = 1;
static const INT32 LTM14_ONE_BIT_MIN = 0;
static const UINT16 LTM_e = 2;  // Variable shift factor as per LTM HPG
// @todo (CAMX-1234): according to pc-sim definition, t should be 2 instead of 0.
static const UINT16 LTM_t = static_cast<UINT16>((LTM_e <= 2) ? 0 : 2);
static const UINT LTM14NumBins                  = 1024;

/// @brief: LTM LUT structure
// NOWHINE NC004c: Share code with system team
struct LTM14LUTInStruct
{
    INT32*   pLUTTable;      ///< LUT Table. i.e. LA_CURVE, LTM_CURVE etc
    UINT16   numEntries;     ///< number of entries in the LUT Table
};
/// @brief LTM unpackfield structure


enum LTM14DarkBrightRegionState
{
    LTM14RegionIndexInit = 0,  ///< Region Index Init
    LTM14DarkBoostRegion,      ///< Region Index Dark
    LTM14BrightRegion,         ///< Region Index Bright
};

static const INT32 LTM14_GAMMA_MAX = IQSettingUtils::MAXUINTBITFIELD(12+LTM_e);
static const INT32 LTM14_GAMMA_MIN = 0;
static const INT32 LTM14_MASK_FILTER_KERNEL_MAX = IQSettingUtils::MAXUINTBITFIELD(3);
static const INT32 LTM14_MASK_FILTER_KERNEL_MIN = 0;
static const INT32 LTM14_LA_CURVE_MAX = IQSettingUtils::MAXUINTBITFIELD(12);
static const INT32 LTM14_LA_CURVE_MIN = 0;
static const INT32 LTM14_LTM_CURVE_MAX = IQSettingUtils::MAXUINTBITFIELD(11);
static const INT32 LTM14_LTM_CURVE_MIN = -2048;
static const INT32 LTM14_LTM_SCALE_MAX = IQSettingUtils::MAXUINTBITFIELD(11);
static const INT32 LTM14_LTM_SCALE_MIN = -2048;
static const INT32 LTM14_LCE_SCALE_POS_MAX = IQSettingUtils::MAXUINTBITFIELD(11);
static const INT32 LTM14_LCE_SCALE_POS_MIN = -2048;
static const INT32 LTM14_LCE_SCALE_NEG_MAX = IQSettingUtils::MAXUINTBITFIELD(11);
static const INT32 LTM14_LCE_SCALE_NEG_MIN = -2048;
static const INT32 LTM14_LCE_THD_MIN = IQSettingUtils::MAXUINTBITFIELD(LTM_e);
static const INT32 LTM14_LCE_THD_MAX = IQSettingUtils::MAXUINTBITFIELD(10);
static const INT32 LTM14_Y_RATIO_MAX_MIN = 0;
static const INT32 LTM14_Y_RATIO_MAX_MAX = IQSettingUtils::MAXUINTBITFIELD(10);
static const INT32 LTM14_RGB2Y_CFG_1_MIN = 0;
static const INT32 LTM14_RGB2Y_CFG_1_MAX = IQSettingUtils::MAXUINTBITFIELD(6);
static const INT32 LTM14_RGB2Y_CFG_2_MIN = 0;
static const INT32 LTM14_RGB2Y_CFG_2_MAX = (IQSettingUtils::MAXUINTBITFIELD(6) + 1);
static const INT32 LTM14_MASK_FILTER_SCALE_MIN = 0;
static const INT32 LTM14_MASK_FILTER_SCALE_MAX = IQSettingUtils::MAXUINTBITFIELD(12);
static const INT32 LTM14_MASK_FILTER_SHIFT_MIN = 0;
static const INT32 LTM14_MASK_FILTER_SHIFT_MAX = IQSettingUtils::MAXUINTBITFIELD(5);
static const UINT16 LTM_INIT_CELLNUM_X_MIN = 0;
static const UINT16 LTM_INIT_CELLNUM_X_MAX = 12;
static const UINT16 LTM_INIT_DX_MIN = 0;
static const UINT16 LTM_INIT_DX_MAX = 1819;
static const UINT16 LTM_INIT_PX_MIN = 0;
static const UINT32 LTM_INIT_PX_MAX = IQSettingUtils::MAXUINTBITFIELD(16 + LTM_t);
static const UINT16 LTM_INIT_CELLNUM_Y_MIN = 0;
static const UINT16 LTM_INIT_CELLNUM_Y_MAX = 9;
static const UINT16 LTM_INIT_DY_MIN = 0;
static const UINT16 LTM_INIT_DY_MAX = 1819;
static const UINT16 LTM_INIT_PY_MIN = 0;
static const UINT32 LTM_INIT_PY_MAX = IQSettingUtils::MAXUINTBITFIELD(16 + LTM_t);
static const UINT16 LTM_INV_CELLWIDTH_MIN = 0;
static const UINT32 LTM_INV_CELLWIDTH_MAX = IQSettingUtils::MAXUINTBITFIELD(12 + LTM_t);
static const UINT16 LTM_INV_CELLHEIGHT_MIN = 0;
static const UINT32 LTM_INV_CELLHEIGHT_MAX = IQSettingUtils::MAXUINTBITFIELD(12 + LTM_t);
static const UINT16 LTM_CELLWIDTH_MIN = 0;
static const UINT16 LTM_CELLWIDTH_MAX = 1532;
static const UINT16 LTM_CELLHEIGHT_MIN = 0;
static const UINT16 LTM_CELLHEIGHT_MAX = 2042;



static const INT32 LTM14_IP_invWH_Q = 18;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements LTM14 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class LTM14Setting
{
public:



    /// @brief HW SWI struct
    struct MNScaleDownInStruct_Y_V21
    {
        UINT16 enable;                 ///< Enable downscaler inside data collection module

        // Pre-crop
        UINT16 pre_crop_en;            ///< Pre-crop enable
        UINT16 first_pixel;            ///< 14u
        UINT16 last_pixel;             ///< 14u
        UINT16 first_line;             ///< 14u
        UINT16 last_line;              ///< 14u

        // MNDS v2.0 SWI (with early termination bug fix in datapath)
        UINT16 h_en;                   ///< h_en
        UINT16 v_en;                   ///< v_en
        UINT16 input_h;                ///< Input width for early termination, n means n+1
        UINT16 input_v;                ///< Input height for early termination, n means n+1
        UINT16 early_termination_h;    ///< 0 or 1
        UINT16 early_termination_v;    ///< 0 or 1
        UINT16 drop_first_output_h;    ///< 0: keep; 1: drop
        UINT16 drop_first_output_v;    ///< 0: keep; 1: drop
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
    struct LtmGridStruct25b_IPE_V14
    {
        UINT16 valsum[LTM_LUT3D_Z_V14]; ///< Value Sum
        UINT16 pixcnt[LTM_LUT3D_Z_V14]; ///< Pixel Count
    };

    /// @brief LTM Aeverage curve structure
    struct LtmGridStruct10b_V14
    {
        INT16 avg[LTM_LUT3D_Z_V14];     ///< Average
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to input data of LTM14 Module
    /// @param  pData         Pointer to output of the interpolation algorithm
    /// @param  pReserveData  Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to ouput register value
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        LTM14InputData*                                       pInput,
        ltm_1_4_0::ltm14_rgn_dataType*                        pData,
        ltm_1_4_0::chromatix_ltm14_reserveType*               pReserveData,
        ltm_1_4_0::chromatix_ltm14Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateAdrcLtmCurves
    ///
    /// @brief  Generate ADRC LTM Curve
    ///
    /// @param  pInEqvlnt   Pointer to input data of LTM14 Module
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
CAMX_NAMESPACE_BEGIN
struct LTM14UnpackedField
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
    ///< MNDS v2.1 pre-crop should be disabled and be hidden by C-API, HW doesn't have pre-crop
    LTM14Setting::MNScaleDownInStruct_Y_V21  scale_in_cfg;

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
    LTM14LUTInStruct         wt;                        ///< length LTM_WEIGHT_LUT_SIZE, weight LUT

    UINT16                   mask_filter_kernel[MASK_FILTER_KERNEL_SIZE_V14]; ///< 4u, 5x5 mask filtering
    UINT16                   mask_filter_scale;                           ///< 12u
    UINT16                   mask_filter_shift;                           ///< 5u
    UINT16                   mask_filter_en;                              ///< 1u

    LTM14LUTInStruct         la_curve;                                    ///< length LTM_CURVE_LUT_SIZE - 1, (10+e)u
    LTM14LUTInStruct         mask_rect_curve;                             ///< length LTM_CURVE_LUT_SIZE - 1, (10+e)u
    LTM14LUTInStruct         ltm_curve;                                   ///< length LTM_CURVE_LUT_SIZE - 1, (10+e)s
    LTM14LUTInStruct         ltm_scale;                                   ///< length LTM_SCALE_LUT_SIZE - 1, (10+e)s
    LTM14LUTInStruct         lce_scale_pos;                               ///< length LCE_SCALE_LUT_SIZE - 1, (10+e)s
    LTM14LUTInStruct         lce_scale_neg;                               ///< length LCE_SCALE_LUT_SIZE - 1, (10+e)s
    LTM14LUTInStruct         igamma64;                                    ///< length LTM_GAMMA_LUT_SIZE - 1, (10+e)s
    UINT16                   lce_thd;                                     ///< lce thd

    UINT16                   y_ratio_max;                                 ///< 10u
    UINT16                   debug_out_sel;                               ///< 2u

    ///< Additional LTM non-volatile variables (pass across frames)
    LTM14Setting::LtmGridStruct25b_IPE_V14 LUT25b[LTM_LUT3D_H_V14 * LTM_LUT3D_W_V14];   ///< lut25b
    ///< DMI ping-pong buffer of 3D bilateral averages
    LTM14Setting::LtmGridStruct10b_V14     avg3d[DMIRAM_LTM_AVG3D_LENGTH_V14];
};
CAMX_NAMESPACE_END
#endif // LTM14SETTING_H
