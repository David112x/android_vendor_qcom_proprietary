// NOWHINE ENTIRE FILE  --- keep the file to be in sync with PCSIM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010,2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*****************************************************************/
/* FILE NAME:   qseed3_misc.h       TYPE: C Header               */
/* DESCRIPTION: Functions to be called by SW (phase step calc)   */
/*              or fetch set up (init phase calc and pixel ext   */
/*              cal)                                             */
/*                                                               */
/* Revision History:                                             */
/*  Author      Date        Comments                             */
/*  ------      --------    ----------                           */
/*  ateng       5/18/10     Initial check-in                     */
/*****************************************************************/
#ifndef _UPSCALE_V20_MISC_H
#define _UPSCALE_V20_MISC_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const UINT8  MaxYorUVChannels      = 2;
static const UINT32 CoeffLUTSizeA         = 42;
static const UINT32 CoeffLUTSizeB         = 24;
static const UINT32 CoeffLUTSizeC         = 24;
static const UINT32 CoeffLUTSizeD         = 20;
static const UINT32 FixPixelUnitScaleBits = 21;
static const UINT32 FixPixelUnitScale     = (1 << FixPixelUnitScaleBits);
static const UINT32 FixPhaseUnitScaleBits = 15;
static const UINT32 FirPhaseResidual      = (1 << (FixPhaseUnitScaleBits - 1));

enum MDP_Chroma_Sample_Site {
    MDP_CHROMA_SAMPLE_SITE_NONE,
    MDP_CHROMA_SAMPLE_SITE_A,
    MDP_CHROMA_SAMPLE_SITE_B,
    MDP_CHROMA_SAMPLE_SITE_C,
    MDP_CHROMA_SAMPLE_SITE_D,
    MDP_CHROMA_SAMPLE_SITE_E,
    MDP_CHROMA_SAMPLE_SITE_F,
    MDP_CHROMA_SAMPLE_SITE_G,
    MDP_CHROMA_SAMPLE_SITE_H,
    MDP_CHROMA_SAMPLE_SITE_I
};

struct UpdateIn
{
    UINT32  prevYCfg;
    UINT32  curYCfg;
    UINT32  prevUVCfg;
    UINT32  curUVCfg;
    UINT32  prevBlndCfg;
    UINT32  curBlndCfg;
    BOOL    isInitProgram;
    UINT32  prevColorSpace;
    UINT32  curColorSpace;
    UINT32  prevPhaseStepHor[MaxYorUVChannels];
    UINT32  prevPhaseStepVer[MaxYorUVChannels];
    UINT32  curPhaseStepHor[MaxYorUVChannels]; //input: horizontal phase step size [step_hor_y, step_hor_uv]
    UINT32  curPhaseStepVer[MaxYorUVChannels];
    UINT32* pTwoDFilterA;
    UINT32* pTwoDFilterB;
    UINT32* pTwoDFilterC;
    UINT32* pTwoDFilterD;
    UINT8   blurry_level; // this is a flag for blurry setting: filters are chosen to blur images, not based on scaling ratios. it is disabled in auto-configuration mode.
};

struct UpdateOut
{
    UINT8   enable_dir_wr;
    UINT8   enable_y_cir_wr;
    UINT8   enable_uv_cir_wr;
    UINT8   enable_y_sep_wr;
    UINT8   enable_uv_sep_wr;
    UINT8   y_cir_lut_value;
    UINT8   y_sep_lut_value;
    UINT8   uv_cir_lut_value;
    UINT8   uv_sep_lut_value;
    UINT32* dir_lut;
    UINT32* y_circ_lut;
    UINT32* y_sep_lut;
    UINT32* uv_circ_lut;
    UINT32* uv_sep_lut;
};

struct YUVCfgs
{
    UINT32  YCfg;
    UINT32  UVCfg;
    UINT32  BlndCfg;
};

enum hal_mdp_qseed3_force_repeat
{
    FORCE_REPEAT_NONE   = 0x0,
    FORCE_REPEAT_TOP    = 0x1,
    FORCE_REPEAT_LEFT   = 0x2,
    FORCE_REPEAT_BOTTOM = 0x4,
    FORCE_REPEAT_RIGHT  = 0x8,
    FORCE_REPEAT_ALL    = 0xF
};

struct SurfaceInfo
{
    UINT32                 surf_comp_input_width[MaxYorUVChannels];
    UINT32                 surf_comp_output_width[MaxYorUVChannels];
    UINT32                 surf_comp_input_height[MaxYorUVChannels];
    UINT32                 surf_comp_output_height[MaxYorUVChannels];
    UINT32                 stripe_comp_width[MaxYorUVChannels];
    UINT32                 chroma_subsample_x_flg;
    UINT32                 chroma_subsample_y_flg;
    MDP_Chroma_Sample_Site chroma_site_io;
    UINT32                 comp_phase_step_x[MaxYorUVChannels];
    UINT32                 comp_phase_step_y[MaxYorUVChannels];
    UINT32                 comp_in_offset_v[MaxYorUVChannels];
};

struct StripeCfg
{
    UINT32  stripe_comp_in_width[MaxYorUVChannels];     ///< [Y, UV]
    UINT32  stripe_comp_in_offset_h[MaxYorUVChannels];  ///< [Y, UV]
    UINT32  stripe_comp_out_offset_h[MaxYorUVChannels]; ///< [Y, UV]
    UINT32  stripe_comp_output_width[MaxYorUVChannels]; ///< [Y, UV]
    UINT8   y_phase_init_h;                             ///< Horizontal initial phase for Y
    UINT8   uv_phase_init_h;                            ///< Horizontal initial phase for UV
    UINT8   y_phase_init_v;                             ///< Horizontal initial phase for Y
    UINT8   uv_phase_init_v;                            ///< Horizontal initial phase for UV
    INT32   overfetch_left[MaxYorUVChannels];           ///< [Y, UV]
    INT32   overfetch_right[MaxYorUVChannels];          ///< [Y, UV]
    INT32   overfetch_top[MaxYorUVChannels];            ///< [Y, UV]
    INT32   overfetch_bot[MaxYorUVChannels];            ///< [Y, UV]
    UINT8   y_preload_h;                                ///< Horizontal preload value for Y
    UINT8   uv_preload_h;                               ///< Horizontal preload value for UV
    UINT32  required_comp_input_width[MaxYorUVChannels];///< [Y, UV]
    UINT8   y_preload_v;                                ///< Horizontal preload value for Y
    UINT8   uv_preload_v;                               ///< Horizontal preload value for UV
};

class TwoDUpscaleMISCS
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TwoDUpscaleMISCS
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TwoDUpscaleMISCS() {}

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~TwoDUpscaleMISCS
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~TwoDUpscaleMISCS() {}

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// cal_phase_step_for_qseed3
    ///
    /// @brief  Calculate phase step for qseed3
    ///
    /// @param  comp_in_width       Input pointer to Y/UV input width array
    /// @param  comp_in_height      Input pointer to Y/UV input height array
    /// @param  comp_out_width      Input pointer to Y/UV output width array
    /// @param  comp_out_height     Input pointer to Y/UV output height array
    /// @param  comp_phase_step_x   Output pointer to Y/UV horizontal phase step array
    /// @param  comp_phase_step_y   Output pointer to Y/UV vertical phase step array
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID cal_phase_step_for_qseed3(
        UINT32 *comp_in_width,
        UINT32 *comp_in_height,
        UINT32 *comp_out_width,
        UINT32 *comp_out_height,
        UINT32 *comp_phase_step_x,
        UINT32 *comp_phase_step_y);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// cal_start_phase_common_qseed3
    ///
    /// @brief  Calculate start phase common for qseed3
    ///
    /// @param  chroma_subsample_x_flg   Input flag 1 if chroma component is subsampled horizontally; 0: if not subsampled.
    ///                                  This is the case after rot90 if there is any.
    /// @param  chroma_subsample_y_flg   Input flag 1 if chroma component is subsampled vertically; 0: if not subsampled.
    ///                                  This is the case after rot90 if there is any.
    /// @param  chroma_site_io           Input original chroma site info. ( not take rot90 and flip into account).
    /// @param  comp_phase_step_x        Input pointer to Y/UV horizontal phase step size array
    /// @param  comp_phase_step_y        Input pointer to Y/UV vertical phase step size array
    /// @param  input_comp_offset        Input pointer to Y/UV ROI0 input width array
    /// @param  output_comp_offset       Input pointer to Y/UV ROI0 output width array
    /// @param  comp_init_phase_x        Output pointer to calculated Y/UV horizontal initial phase array of right ROI
    /// @param  comp_init_phase_y        Output pointer to calculated Y/UV vertical initial phases array of right ROI
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID cal_start_phase_common_qseed3(
        UINT32 chroma_subsample_x_flg,
        UINT32 chroma_subsample_y_flg,
        MDP_Chroma_Sample_Site chroma_site_io,
        UINT32* comp_phase_step_x,
        UINT32* comp_phase_step_y,
        UINT32* input_comp_offset,
        UINT32* output_comp_offset,
        INT32*  comp_init_phase_x,
        INT32*  comp_init_phase_y);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// derive_init_phase_for_qseed3
    ///
    /// @brief  Derive initial phase for qseed3
    ///
    /// @param  comp_init_phase_x   Input pointer to Y/UV horizontal initial phase array
    /// @param  comp_init_phase_y   Input pointer to Y/UV vertical initial phase array
    /// @param  i32InitPhaseX       Output pointer to derived Y/UV horizontal initial phase array
    /// @param  i32InitPhaseY       Output pointer to derived Y/UV vertical initial phase array
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID derive_init_phase_for_qseed3(
        INT32*  comp_init_phase_x,
        INT32*  comp_init_phase_y,
        INT32*  i32InitPhaseX,
        INT32*  i32InitPhaseY);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// cal_num_extended_pels_for_qseed3
    ///
    /// @brief  Calculate number of extended pixels for qseed3
    ///
    /// @param  enable                         Input flag 1 to enable calculation using extended pixels, 0 not using
    /// @param  comp_in_width,                 Input pointer to Y/UV input width array
    /// @param  comp_in_height,                Input pointer to Y/UV input height array
    /// @param  comp_out_width,                Input pointer to Y/UV output width array
    /// @param  comp_out_height,               Input pointer to Y/UV output height array
    /// @param  comp_phase_step_x,             Input pointer to Y/UV horizontal phase step array
    /// @param  comp_phase_step_y,             Input pointer to Y/UV vertical phase step array
    /// @param  comp_init_phase_x,             Input pointer to Y/UV horizontal initial phase array
    /// @param  comp_init_phase_y,             Input pointer to Y/UV vertical initial phase array
    /// @param  comp_num_left_extended_pels    Output pointer to Y/UV number of left extended pixels
    /// @param  comp_num_right_extended_pels   Output pointer to Y/UV number of right extended pixels
    /// @param  comp_num_top_extended_pels     Output pointer to Y/UV number of top extended pixels
    /// @param  comp_num_bot_extended_pels     Output pointer to Y/UV number of bottom extended pixels
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID cal_num_extended_pels_for_qseed3(
        UINT32  enable,
        UINT32* comp_in_width,
        UINT32* comp_in_height,
        UINT32* comp_out_width,
        UINT32* comp_out_height,
        UINT32* comp_phase_step_x,
        UINT32* comp_phase_step_y,
        INT32*  comp_init_phase_x,
        INT32*  comp_init_phase_y,
        INT32*  comp_num_left_extended_pels,
        INT32*  comp_num_right_extended_pels,
        INT32*  comp_num_top_extended_pels,
        INT32*  comp_num_bot_extended_pels);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// cal_preload_pels_for_qseed3
    ///
    /// @brief  Calculate number of preloaded pixels for qseed3
    ///
    /// @param  pCompNumLeftExtendedPels    Input pointer to Y/UV number of left extended pixels array
    /// @param  pCompNumRightExtendedPels   Input pointer to Y/UV number of right extended pixels array
    /// @param  pCompNumTopExtendedPels     Input pointer to Y/UV number of top extended pixels array
    /// @param  pCompNumBotExtendedPels     Input pointer to Y/UV number of bottom extended pixels array
    /// @param  phase_step_y_h              Input Y horizontal phase step size
    /// @param  phase_step_y_v              Input Y vertical phase step size
    /// @param  repeatOnly                  Input selection from four effective bits:for LSB to MSB the order is
    ///                                     top, left, bottom and right.0:overfetch 1: pixel repeat
    /// @param  pCompInHorOffset            Input pointer to horizontal input ROI offsets, with no overfetched pixels.
    /// @param  pCompInVerOffset            Input pointer to vertical input ROI offsets, with no overfetched pixels.
    /// @param  pCompSurInWidth             Input pointer to surface image width array
    /// @param  pCompSurInHeight            Input pointer to surface image height array
    /// @param  pCompROIInWidth             Input pointer to ROI width array
    /// @param  pCompROIInHeight            Input pointer to ROI height array
    /// @param  outWidth                    Input width after scale
    /// @param  outHeight                   Input height after scale
    /// @param  pYPreloadH                  Output pointer to number of horizontal preload pixel for Y channel
    /// @param  pYPreloadV                  Output pointer to number of verticla preload pixel for Y channel
    /// @param  pUVPreloadH                 Output pointer to number of horizontal preload pixel for UV channel
    /// @param  pUVPreloadV                 Output pointer to number of verticla preload pixel for UV channel
    /// @param  pInputROIWidth              Output pointer to Y/UV input ROI width array to set up fetching
    /// @param  pInputROIHeight             Output pointer to Y/UV input ROI height array to set up fetching
    /// @param  pInputROIHorOffset          Output pointer to Y/UV input ROI horizontal offset array to set up fetching
    /// @param  pInputROIVerOffset          Output pointer to Y/UV input ROI vertical offset array to set up fetching
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID cal_preload_pels_for_qseed3(
        INT32*  pCompNumLeftExtendedPels,
        INT32*  pCompNumRightExtendedPels,
        INT32*  pCompNumTopExtendedPels,
        INT32*  pCompNumBotExtendedPels,
        UINT32  phase_step_y_h,
        UINT32  phase_step_y_v,
        UINT8   repeatOnly,
        UINT32* pCompInHorOffset,
        UINT32* pCompInVerOffset,
        UINT32* pCompSurInWidth,
        UINT32* pCompSurInHeight,
        UINT32* pCompROIInWidth,
        UINT32* pCompROIInHeight,
        UINT32  outWidth,
        UINT32  outHeight,
        UINT8*  pYPreloadH,
        UINT8*  pYPreloadV,
        UINT8*  pUVPreloadH,
        UINT8*  pUVPreloadV,
        UINT32* pInputROIWidth,
        UINT32* pInputROIHeight,
        UINT32* pInputROIHorOffset,
        UINT32* pInputROIVerOffset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// cal_de_curve_for_qseed3
    ///
    /// @brief  Calculate detail enhancer curve for qseed3
    ///
    /// @param  ThrshldT1   Input threhsold T1
    /// @param  ThrshldT2   Input threhsold T2
    /// @param  ThrshldTq   Input threhsold Tq
    /// @param  ThrshldTd   Input threhsold Td
    /// @param  PrecBitN    Output pointer to number of precision bits
    /// @param  CurveA0     Output pointer to curve point A0
    /// @param  CurveB0     Output pointer to curve point B0
    /// @param  CurveC0     Output pointer to curve point C0
    /// @param  CurveA1     Output pointer to curve point A1
    /// @param  CurveB1     Output pointer to curve point B1
    /// @param  CurveC1     Output pointer to curve point C1
    /// @param  CurveA2     Output pointer to curve point A2
    /// @param  CurveB2     Output pointer to curve point B2
    /// @param  CurveC2     Output pointer to curve point C2
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID cal_de_curve_for_qseed3(
        UINT16  ThrshldT1,
        UINT16  ThrshldT2,
        UINT16  ThrshldTq,
        UINT16  ThrshldTd,
        UINT8*  PrecBitN,
        INT16*  CurveA0,
        INT16*  CurveB0,
        INT16*  CurveC0,
        INT16*  CurveA1,
        INT16*  CurveB1,
        INT16*  CurveC1,
        INT16*  CurveA2,
        INT16*  CurveB2,
        INT16*  CurveC2);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CoeffLUTsUpdate
    ///
    /// @brief  Update coefficients for LUTs selection
    ///
    /// @param  pInParams    Input|Output pointer to the UpdateIn data structure
    /// @param  pOutParams   Output pointer to the UpdateOut data structure
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CoeffLUTsUpdate(
        UpdateIn*  pInParams,
        UpdateOut* pOutParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StripingForTwoDScale
    ///
    /// @brief  Calculate striping information for 2D scale
    ///
    /// @param  Surface   Input surface information
    /// @param  pStripe   Output array containing striping information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID StripingForTwoDScale(
        SurfaceInfo Surface,
        StripeCfg   pStripe[]);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// cal_init_phase_core
    ///
    /// @brief  Core algorithm to calculate initial phase
    ///
    /// @param  new_delta_phase        Input new delta phase
    /// @param  chroma_subsample_flg   Input chroma subsample flag
    /// @param  comp_phase_step        Input pointer to array of phase step
    /// @param  skip                   Input but not used
    /// @param  comp_init_phase        Output pointer to array of initial phase calculated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID cal_init_phase_core(
        INT32*  new_delta_phase,
        UINT32  chroma_subsample_flg,
        UINT32* comp_phase_step,
        UINT16* skip,
        INT32*  comp_init_phase);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// cal_init_phase_overall
    ///
    /// @brief  Calculate overall initial phase
    ///
    /// @param  chroma_subsample_x_flg   Input flag 1 if chroma component is subsampled horizontally; 0: if not subsampled.
    ///                                  This is the case after rot90 if there is any.
    /// @param  chroma_subsample_y_flg   Input flag 1 if chroma component is subsampled vertically; 0: if not subsampled.
    ///                                  This is the case after rot90 if there is any.
    /// @param  chroma_site_io           Input original chroma site info. ( not take rot90 and flip into account).
    /// @param  comp_phase_step_x        Input pointer to Y/UV horizontal phase step size array
    /// @param  comp_phase_step_y        Input pointer to Y/UV vertical phase step size array
    /// @param  comp_init_phase_x        Output pointer to calculated Y/UV horizontal initial phase array of right ROI
    /// @param  comp_init_phase_y        Output pointer to calculated Y/UV vertical initial phases array of right ROI
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID cal_init_phase_overall(
        UINT32  chroma_subsample_x_flg,
        UINT32  chroma_subsample_y_flg,
        MDP_Chroma_Sample_Site chroma_site_io,
        UINT32* comp_phase_step_x,
        UINT32* comp_phase_step_y,
        INT32*  comp_init_phase_x,
        INT32*  comp_init_phase_y);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// cal_num_extended_pels_core_for_qseed3
    ///
    /// @brief  Core algorithm to calculate number of extended pixels for qseed3
    ///
    /// @param  extra               Input extra pixels to add or drop
    /// @param  num_extended_pels   Output pointer to number of extended pixels
    /// @param  end_ind             Input flag to indicate number of extended pixel to 1 when extra is 0
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID cal_num_extended_pels_core_for_qseed3(
        INT32  extra,
        INT32* num_extended_pels,
        BOOL   end_ind);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// clean_frac
    ///
    /// @brief  Remove fraction from an integer number
    ///
    /// @param  in   Input data for fraction cleaning
    ///
    /// @return INT32 value after cleaning of fractions
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT32 clean_frac(
        INT32 in);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// range_check
    ///
    /// @brief  Check the range of curves
    ///
    /// @param  CurveA0     Input pointer to curve point A0
    /// @param  CurveB0     Input pointer to curve point B0
    /// @param  CurveC0     Input pointer to curve point C0
    /// @param  CurveA1     Input pointer to curve point A1
    /// @param  CurveB1     Input pointer to curve point B1
    /// @param  CurveC1     Input pointer to curve point C1
    /// @param  CurveA2     Input pointer to curve point A2
    /// @param  CurveB2     Input pointer to curve point B2
    /// @param  CurveC2     Input pointer to curve point C2
    ///
    /// @return INT8 value for the check result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT8 range_check(
        INT16* CurveA0,
        INT16* CurveB0,
        INT16* CurveC0,
        INT16* CurveA1,
        INT16* CurveB1,
        INT16* CurveC1,
        INT16* CurveA2,
        INT16* CurveB2,
        INT16* CurveC2);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// cal_de_curve_core
    ///
    /// @brief  Core algorithm to calculate detail enhancer curves
    ///
    /// @param  T1          Input threhsold T1
    /// @param  T2          Input threhsold T2
    /// @param  Tq          Input threhsold Tq
    /// @param  Td          Input threhsold Td
    /// @param  Nbits       Input number of precision bits
    /// @param  CurveA0     Output pointer to curve point A0
    /// @param  CurveB0     Output pointer to curve point B0
    /// @param  CurveC0     Output pointer to curve point C0
    /// @param  CurveA1     Output pointer to curve point A1
    /// @param  CurveB1     Output pointer to curve point B1
    /// @param  CurveC1     Output pointer to curve point C1
    /// @param  CurveA2     Output pointer to curve point A2
    /// @param  CurveB2     Output pointer to curve point B2
    /// @param  CurveC2     Output pointer to curve point C2
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID cal_de_curve_core(
        UINT16 T1,
        UINT16 T2,
        UINT16 Tq,
        UINT16 Td,
        UINT8  Nbits,
        INT16* CurveA0,
        INT16* CurveB0,
        INT16* CurveC0,
        INT16* CurveA1,
        INT16* CurveB1,
        INT16* CurveC1,
        INT16* CurveA2,
        INT16* CurveB2,
        INT16* CurveC2);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// detCircVal
    ///
    /// @brief  Determine the circular value for circular LUT selection
    ///
    /// @param  phase_step     Input phase step
    /// @param  blurry_level   Input blurry level
    ///
    /// @return UINT8 circular value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT8 detCircVal(
        UINT32 phase_step,
        UINT8  blurry_level);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// detSepVal
    ///
    /// @brief  Determine the separable value for separable LUT selection
    ///
    /// @param  phase_step     Input phase step
    /// @param  blurry_level   Input blurry level
    ///
    /// @return UINT8 separable value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT8 detSepVal(
        UINT32 phase_step,
        UINT8  blurry_level);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DetFlags
    ///
    /// @brief  Determine flags for LUT selections
    ///
    /// @param  pInParams          Input data from UpdateIn structure
    /// @param  enable_dir_wr      Output pointer to enable flag for directional LUT
    /// @param  enable_y_cir_wr    Output pointer to enable flag for Y circular LUT
    /// @param  enable_uv_cir_wr   Output pointer to enable flag for UV circular LUT
    /// @param  enable_y_sep_wr    Output pointer to enable flag for Y separable LUT
    /// @param  enable_uv_sep_wr   Output pointer to enable flag for UV separable LUT
    /// @param  y_cir_lut_value    Output pointer to Y circular LUT selection index
    /// @param  y_sep_lut_value    Output pointer to Y separable LUT selection index
    /// @param  uv_cir_lut_value   Output pointer to UV circular LUT selection index
    /// @param  uv_sep_lut_value   Output pointer to UV separable LUT selection index
    ///
    /// @return UINT8 separable value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DetFlags(
        UpdateIn* pInParams,
        UINT8*    enable_dir_wr,
        UINT8*    enable_y_cir_wr,
        UINT8*    enable_uv_cir_wr,
        UINT8*    enable_y_sep_wr,
        UINT8*    enable_uv_sep_wr,
        UINT8*    y_cir_lut_value,
        UINT8*    y_sep_lut_value,
        UINT8*    uv_cir_lut_value,
        UINT8*    uv_sep_lut_value);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// loadCircSepLUTs
    ///
    /// @brief  Load circular and separable LUTs
    ///
    /// @param  pInParams   Input|Output pointer to data from UpdateIn structure
    /// @param  pLUTs       Input 2-dimension array LUT: dirLUT, sepLUT or cirLUT
    /// @param  offsetA     Input offset A for 2-dimension array of filter A
    /// @param  offsetBC    Input offset BC for 2-dimension array of filter B and filter C
    /// @param  offsetD     Input offset A for 2-dimension array of filter D
    /// @param  nSet        Input to determine which set of pLUTs is used
    ///
    /// @return UINT8 separable value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID loadCircSepLUTs(
        UpdateIn* pInParams,
        UINT32    pLUTs[][60],
        INT16     offsetA,
        INT16     offsetBC,
        INT16     offsetD,
        INT16     nSet);
};

#endif // _UPSCALE_V20_MISC_H
