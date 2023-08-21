// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------


#ifndef __ICA_REGISTERS_V30_H__
#define __ICA_REGISTERS_V30_H__

#include "CommonDefs.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ICA_REG_v30_TAG
{

    // Defines if ICA is enabled.
    // format: 1u
    FIELD_UINT EN;

    ////////////////////////////
    // Defines ICA mode of operation.
    ////////////////////////////
    // 0 - Warping on, ICA Core is active.
    // Inverse transform calculation is performed by CTC.
    // 1 - Warping off, ICA Core is active.
    // Inverse transform calculation is NOT performed by CTC. Effectively this mode is functional bypass (with possible cropping). All formats should be supported in this mode.
    // 2 - Bypass - ICA Core is inactive and fully bypassed. Only PD formats are supported in this mode.
    // format: 2u
    FIELD_UINT MODE;

    ////////////////////////////
    // Defines ICA input formats.
    ////////////////////////////
    // 0: UBWC_TP10
    // 1: UBWC_NV12-4R
    // 2: UBWC_P010_MA (MSB Aligned)
    // 3: UBWC_P010_LA (LSB Aligned )
    // 4: Linear_TP10
    // 5: Linear_PD10
    // 6: Linear_PY10 (Not part of PoR )
    // 7: Linear_NV12
    // 8: Linear_PD8 (Not part of PoR )
    // 9: Linear_PY8 (Not part of PoR )
    // 10: Linear_P010_MA
    // 11: Linear_P010_LA
    // 12-15: Reserved
    //
    // Mode 6 and 9 are supported by PcSim (and therefore were not removed since we have tests that run in this mode)
    // format: 4u
    FIELD_UINT INPUT_FORMAT;

    ////////////////////////////
    // Defines ICA output formats.
    ////////////////////////////
    // 0: YUV420
    // 1: Y-only (Not part of PoR)
    //
    // Mode 1 is supported by PcSim (and therefore was not removed since we have tests that run in this mode)
    // format: 1u
    FIELD_UINT OUTPUT_FORMAT;

    ////////////////////////////
    // Defines the width of the input frame minus 1.
    ////////////////////////////
    // format: 14u
    FIELD_UINT INPUT_FRAME_WIDTH_MINUS_1;

    ////////////////////////////
    // Defines the height of the input frame minus 1.
    ////////////////////////////
    // format: 14u
    FIELD_UINT INPUT_FRAME_HEIGHT_MINUS_1;

    ////////////////////////////
    // Defines the width of the output strip minus 1.
    // In Titan PT, the maximal strip width is 336.
    ////////////////////////////
    // format: 9u
    FIELD_UINT OUTPUT_STRIP_WIDTH_MINUS_1;

    ////////////////////////////
    // Defines the height of the output strip minus 1.
    ////////////////////////////
    // format: 14u
    FIELD_UINT OUTPUT_STRIP_HEIGHT_MINUS_1;

    ////////////////////////////
    // Defines whether luma samples are interpolated using a 4x4 symmetric kernel or bi-linear interpolation.
    ////////////////////////////
    // 0: 4x4 kernel
    // 1: Bi-linear (2x2)
    // format: 1u
    FIELD_UINT Y_INTERPOLATION_TYPE;

    ////////////////////////////
    // In case of 8 bit formats (as defined by INPUT_FORMAT), this parameter defines whether the 8-bit sample is aligned to the MSB or LSB within the 10-bit output.
    // For Titan PT/HT, this parameter is expected to be set to 1.
    ////////////////////////////
    // 0: LSB aligned
    // 1: MSB aligned
    // format: 1u
    FIELD_UINT EIGHT_BIT_OUTPUT_ALIGNMENT;

    ////////////////////////////
    // Relevant only for functional bypass mode (MODE = 2) and only for PD formats (as defined by INPUT_FORMAT).
    // Defines where the strip starts within the first 128 bits coming in from the FE. Used by the Unpacker to crop out-of-strip data; meaning is inferred based on INPUT_FORMAT.
    ////////////////////////////
    // format: 3u
    FIELD_UINT PACKED_FORMAT_PHASE;

    ////////////////////////////
    // Defines the width of the valid region minus 1 inside the input frame. The valid region starts at x=0. Coordinates outside of the valid region will be marked invalid
    ////////////////////////////
    // 19 bits as 0.14.5.
    // format: 19uQ5
    FIELD_UINT CONTROLLER_VALID_WIDTH_MINUS_1;

    ////////////////////////////
    // Defines the height of the valid region minus 1 inside the input frame. The valid region starts at y=0. Coordinates outside of the valid region will be marked invalid.
    ////////////////////////////
    // 19 bits as 0.14.5.
    // format: 19uQ5
    FIELD_UINT CONTROLLER_VALID_HEIGHT_MINUS_1;

    ////////////////////////////
    // Defines whether the MRU is enabled or not.
    // Used as a fallback in case of issues with the MRU.
    // This parameter is single-buffered.
    ////////////////////////////
    // 0: disabled
    // 1: enabled
    // format: 1u
    FIELD_UINT CONTROLLER_MRU_ENABLE;

    ////////////////////////////
    // Defines whether Prefetching is stalled in case a UBWC macro-tile may be stored opportunistically in the UBWC Cache, but all entries in UBWC Cache are either “taken” or “moved”.
    // This parameter is single-buffered.
    ////////////////////////////
    // 0: disabled
    // 1: enabled
    // format: 1u
    FIELD_UINT CONTROLLER_ENABLE_UBWC_FULL_STALL;

    ////////////////////////////
    // Defines minimal distance between Prefetching and Processing. This distance is maintained before the entrance to the Cache Map in order to avoid a Cache Management error in which Processing accesses a macro-tile before Prefetching reached it. Important: this parameter should be 0 (setting it to higher values may result in deadlock).
    // This parameter is single-buffered.
    ////////////////////////////
    // format: 4u
    FIELD_UINT CONTROLLER_MIN_PREF_PROC_DISTANCE;

    ////////////////////////////
    // Defines whether macro-tile fetching (as part of Prefetching) and mini-tile fetching (as part of Processing) should be performed for invalid pixels.
    // If disabled, fetching is avoided when possible. This parameter can only be disabled when OPG_INVALID_OUTPUT_TREATMENT.calculate = 0.
    // This parameter is single-buffered.
    ////////////////////////////
    // format: 1u
    FIELD_UINT CONTROLLER_ENABLE_INVALID_PIXEL_FETCH;

    ////////////////////////////
    // Defines maximal distance between Prefetching and Processing. This distance is maintained before the entrance to the Cache Map in order to avoid overflow of the Cache Map counters.
    // This parameter is single-buffered.
    ////////////////////////////
    // format: 14u
    FIELD_UINT CONTROLLER_MAX_PREF_PROC_DISTANCE;

    ////////////////////////////
    // Defines the x coordinate of the beginning of the strip.
    // Must be even.
    ////////////////////////////
    // Must be even.
    // format: 14u
    FIELD_UINT CTC_OUTPUT_STRIP_START_X;

    ////////////////////////////
    // Defines the y coordinate of the beginning of the strip  - used to allow vertical cropping in warp-off mode.  Can be non-zero only in warp-off mode (TOP.MODE == 1).
    // Must be even.
    ////////////////////////////
    // Must be even.
    // format: 14u
    FIELD_UINT CTC_OUTPUT_STRIP_START_Y;

    ////////////////////////////
    // Defines half of the output frame width.
    // Used for shifting the output pixel x coordinates from the output domain to the shifted output domain.
    ////////////////////////////
    // format: 14u
    FIELD_UINT CTC_HALF_OUTPUT_FRAME_WIDTH;

    ////////////////////////////
    // Defines half of the output frame height.
    // Used for shifting the output pixel y coordinates from the output domain to the shifted output domain.
    ////////////////////////////
    // format: 14u
    FIELD_UINT CTC_HALF_OUTPUT_FRAME_HEIGHT;

    ////////////////////////////
    // Defines the scale factor used to convert the x output coordinates from the output resolution domain to the virtual resolution domain.
    //
    // Scale factor of the x coordinate in relation to virtual resolution.
    ////////////////////////////
    // mantissa
    // format: 18s
    FIELD_INT CTC_O2V_SCALE_FACTOR_X_M;

    // exponent
    // format: 6s
    FIELD_INT CTC_O2V_SCALE_FACTOR_X_E;

    ////////////////////////////
    // Defines the scale factor used to convert the y output coordinates from the output resolution domain to the virtual resolution domain.
    //
    // Scale factor of the y coordinate in relation to virtual resolution.
    ////////////////////////////
    // mantissa
    // format: 18s
    FIELD_INT CTC_O2V_SCALE_FACTOR_Y_M;

    // exponent
    // format: 6s
    FIELD_INT CTC_O2V_SCALE_FACTOR_Y_E;

    ////////////////////////////
    // Defines the x offset used to convert the output coordinates from the output resolution domain to the virtual resolution domain.
    //
    // Offset of the x coordinate virtual domain resolution
    ////////////////////////////
    // allowed values: -131071:-65536 | 0 | 65536:131071
    // format: 18s
    FIELD_INT CTC_O2V_OFFSET_X_M;

    // format: 6s
    FIELD_INT CTC_O2V_OFFSET_X_E;

    ////////////////////////////
    // Defines the y offset used to convert the output coordinates from the output resolution domain to the virtual resolution domain.
    //
    // Offset of the y coordinate in virtual domain resolution
    ////////////////////////////
    // allowed values: -131071:-65536 | 0 | 65536:131071
    // format: 18s
    FIELD_INT CTC_O2V_OFFSET_Y_M;

    // format: 6s
    FIELD_INT CTC_O2V_OFFSET_Y_E;

    ////////////////////////////
    // Defines which transformations are part of the inverse transform calculation.
    // Note: translation_only is mutual exclusive with the rest of the fields. In case it is enabled, the rest of the fields are ignored and Translation-Only transform is applied.
    //
    // Defines which transformations are part of the inverse transform calculation (relevant only for ‘Warping On’ mode, as defined by MODE)
    ////////////////////////////
    // 0: Disabled, 1: Enabled
    // format: 1u
    FIELD_UINT CTC_TRANSFORM_PERSPECTIVE_ENABLE;

    // 0: Disabled, 1: Enabled
    // format: 1u
    FIELD_UINT CTC_TRANSFORM_GRID_ENABLE;

    // 0: Disabled, 1: Enabled
    // format: 1u
    FIELD_UINT CTC_TRANSFORM_REFINEMENT_ENABLE;

    // 0: Disabled, 1: Enabled
    // format: 1u
    FIELD_UINT CTC_TRANSFORM_TRANSLATION_ONLY;

    // 0:Disabled, 1:Used only for calculation of correction, 2:Actually applied
    // format: 2u
    FIELD_UINT CTC_TRANSFORM_LMC_MODE;

    ////////////////////////////
    // Defines the width of the perspective transform partitioning geometry.
    // Relevant only when CTC_TRANSFORM.perspective_enable is equal to 1.
    // The height is defined by CTC_PERSP_TRANSFORM_GEOMETRY_N
    ////////////////////////////
    // The geometry of the perspective transform partitioning is described in terms of MxN, when M means number of columns and N means number of rows.
    // Valid combinations are those in which MxN<=9.
    // format: 4u
    FIELD_UINT CTC_PERSP_TRANSFORM_GEOMETRY_M;

    ////////////////////////////
    // Defines the height of the perspective transform partitioning geometry.
    // Relevant only when CTC_TRANSFORM.perspective_enable is equal to 1.
    // The width is defined by CTC_PERSP_TRANSFORM_GEOMETRY_M.
    ////////////////////////////
    // The geometry of the perspective transform partitioning is described in terms of MxN, when M means number of columns and N means number of rows.
    // Valid combinations are those in which MxN<=9.
    // format: 4u
    FIELD_UINT CTC_PERSP_TRANSFORM_GEOMETRY_N;

    ////////////////////////////
    // Relevant only in case MODE = 0 (‘Warping On’) and CTC_TRANSFORM.translation_only is equal to 1.
    // Defines the translation that should be induced on the x coordinate by CTC.
    ////////////////////////////
    // 20 bits allocated as 1.14.5
    // format: 20sQ5
    FIELD_INT CTC_X_TRANSLATION;

    ////////////////////////////
    // Relevant only in case MODE = 0 (‘Warping On’) and CTC_TRANSFORM.translation_only is equal to 1.
    // Defines the translation that should be induced on the y coordinate by CTC.
    ////////////////////////////
    // 20 bits allocated as 1.14.5
    // format: 20sQ5
    FIELD_INT CTC_Y_TRANSLATION;

    ////////////////////////////
    // Defines the inverse scale factor used to convert the x coordinate result of the inverse transform calculation from the virtual resolution domain to the input resolution domain.
    // Inverse scale factor of the x coordinates in relation to input resolution
    ////////////////////////////
    // format: 18s
    FIELD_INT CTC_V2I_INV_SCALE_FACTOR_X_M;

    // format: 6s
    FIELD_INT CTC_V2I_INV_SCALE_FACTOR_X_E;

    ////////////////////////////
    // Defines the inverse scale factor used to convert the y coordinate result of the inverse transform calculation from the virtual resolution domain to the input resolution domain.
    // Inverse scale factor of the y coordinates in relation to input resolution
    ////////////////////////////
    // format: 18s
    FIELD_INT CTC_V2I_INV_SCALE_FACTOR_Y_M;

    // format: 6s
    FIELD_INT CTC_V2I_INV_SCALE_FACTOR_Y_E;

    ////////////////////////////
    // Defines the offset used to convert the x coordinate result of the inverse transform calculation from the virtual resolution domain to the input resolution domain.
    // Offset of the x coordinate in relation to input resolution.
    ////////////////////////////
    // allowed values: -131071:-65536 | 0 | 65536:131071
    // format: 18s
    FIELD_INT CTC_V2I_OFFSET_X_M;

    // format: 6s
    FIELD_INT CTC_V2I_OFFSET_X_E;

    ////////////////////////////
    // Defines the offset used to convert the y coordinate result of the inverse transform calculation from the virtual resolution domain to the input resolution domain.
    // Offset of the y coordinate in relation to input resolution.
    ////////////////////////////
    // allowed values: -131071:-65536 | 0 | 65536:131071
    // format: 18s
    FIELD_INT CTC_V2I_OFFSET_Y_M;

    // format: 6s
    FIELD_INT CTC_V2I_OFFSET_Y_E;

    ////////////////////////////
    // Defines how the 18 bits allocated for coordinate representation are divided between the integer and the fractional part during CTC inverse transform calculations.
    ////////////////////////////
    // Number of bits allocated for the integer part and the fractional part in the input pixel coordinate.
    // The configuration is based on the input image resolution.
    // 0: 1.14.3
    // 1: 1.13.4
    // 2: 1.12.5
    // 3: 1.11.6
    // 4: 1.10.7
    // 5: 1.9.8
    // 6: 1.8.9
    // 7: 1.7.10
    // format: 3u
    FIELD_UINT CTC_INPUT_COORD_PRECISION;

    ////////////////////////////
    // Defines the width of the DC4-based correction grid.
    ////////////////////////////
    // 0 indicates the grid is disabled
    // format: 4u
    FIELD_UINT CTC_REFINEMENT_DC4_GRID_WIDTH;

    ////////////////////////////
    // Defines the height of the DC4-based correction grid.
    ////////////////////////////
    // 0 indicates the grid is disabled
    // format: 4u
    FIELD_UINT CTC_REFINEMENT_DC4_GRID_HEIGHT;

    ////////////////////////////
    // Defines the width of the DC16-based correction grid.
    ////////////////////////////
    // 0 indicates the grid is disabled
    // format: 3u
    FIELD_UINT CTC_REFINEMENT_DC16_GRID_WIDTH;

    ////////////////////////////
    // Defines the height of the DC16-based correction grid.
    ////////////////////////////
    // 0 indicates the grid is disabled
    // format: 3u
    FIELD_UINT CTC_REFINEMENT_DC16_GRID_HEIGHT;

    ////////////////////////////
    // Defines the width of the DC64-based correction grid.
    ////////////////////////////
    // 0 indicates the grid is disabled
    // format: 2u
    FIELD_UINT CTC_REFINEMENT_DC64_GRID_WIDTH;

    ////////////////////////////
    // Defines the height of the DC64-based correction grid.
    ////////////////////////////
    // 0 indicates the grid is disabled
    // format: 2u
    FIELD_UINT CTC_REFINEMENT_DC64_GRID_HEIGHT;

    ////////////////////////////
    // Defines the number of fractional bits within the 7-bit signed correction vectors of the DC4-based correction grid.
    ////////////////////////////
    // Specifies F in 1.6-F.F
    // format: 3u
    FIELD_UINT CTC_REFINEMENT_DC4_GRID_FRAC_BITS;

    ////////////////////////////
    // Defines the number of fractional bits within the 7-bit signed correction vectors of the DC16-based correction grid.
    ////////////////////////////
    // Specifies F in 1.6-F.F
    // format: 3u
    FIELD_UINT CTC_REFINEMENT_DC16_GRID_FRAC_BITS;

    ////////////////////////////
    // Defines the number of fractional bits within the 7-bit signed correction vectors of the DC64-based correction grid.
    ////////////////////////////
    // Specifies F in 1.6-F.F
    // format: 3u
    FIELD_UINT CTC_REFINEMENT_DC64_GRID_FRAC_BITS;

    ////////////////////////////
    // Defines the x coordinate of Nth column in the non-uniform DC4 grid. Value should be in the shifted output domain (i.e. shifted to the left by half the output frame width).
    ////////////////////////////
    // format: 14s
    FIELD_INT CTC_REFINEMENT_DC4_LOCATION_X[10];

    ////////////////////////////
    // Defines the y coordinate of Nth row in the non-uniform DC4 grid. Value should be in the shifted output domain (i.e. shifted to the top by half the output frame height).
    ////////////////////////////
    // format: 14s
    FIELD_INT CTC_REFINEMENT_DC4_LOCATION_Y[8];

    ////////////////////////////
    // Defines the x coordinate of Nth column in the non-uniform DC16 grid. Value should be in the shifted output domain (i.e. shifted to the left by half the output frame width).
    ////////////////////////////
    // format: 14s
    FIELD_INT CTC_REFINEMENT_DC16_LOCATION_X[4];

    ////////////////////////////
    // Defines the y coordinate of Nth row in the non-uniform DC16 grid. Value should be in the shifted output domain (i.e. shifted to the top by half the output frame height).
    ////////////////////////////
    // format: 14s
    FIELD_INT CTC_REFINEMENT_DC16_LOCATION_Y[4];

    ////////////////////////////
    // Defines the x coordinate of Nth column in the non-uniform DC64 grid. Value should be in the shifted output domain (i.e. shifted to the left by half the output frame width).
    ////////////////////////////
    // format: 14s
    FIELD_INT CTC_REFINEMENT_DC64_LOCATION_X[2];

    ////////////////////////////
    // Defines the y coordinate of Nth row in the non-uniform DC64 grid. Value should be in the shifted output domain (i.e. shifted to the top by half the output frame height).
    ////////////////////////////
    // format: 14s
    FIELD_INT CTC_REFINEMENT_DC64_LOCATION_Y[2];

    ////////////////////////////
    // Defines whether and how preprocessing should be performed as part of the Registration Refinement operation.
    // * Bit 0 specifies whether the preprocessing operation should be performed.
    // * Bit 1 specifies whether the correction vectors (preprocessed or not) should be copied to the scratch pad.
    ////////////////////////////
    // 0: No preprocessing and no scratch pad population are performed.
    // Existing data in the scratch pad can be used. Relevant for non-first stripes
    // 2: Only scratch pad population is performed.
    // Fallback in case of preprocessing issues.
    // 3: Both preprocessing and scratch pad population are performed.
    // Existing data in scratch pad can’t be used. Relevant for first stripes
    // format: 2u
    FIELD_UINT CTC_REFINEMENT_PREPROCESSING_MODE;

    ////////////////////////////
    // Relevant only in case MODE = 0 (‘Warping On’) and CTC_TRANSFORM.grid_enable is equal to 1.
    // Defines the number of active grid samples.
    ////////////////////////////
    // 0: 35x27 samples; 1: 67x51
    // format: 2u
    FIELD_UINT CTC_GRID_TRANSFORM_GEOMETRY;

    ////////////////////////////
    // Maps output coordinates to input coordinates
    ////////////////////////////
    // Grid value for x
    // refers to LD_U2I as defined in the flows document
    // format: 18sQ4
    FIELD_INT CTC_GRID_X[3417];

    // Grid value for y
    // refers to LD_U2I as defined in the flows document
    // format: 18sQ4
    FIELD_INT CTC_GRID_Y[3417];

    // refinement grid value for x
    // format: 7s
    FIELD_INT CTC_REFINEMENT_GRID_DX[100];

    // refinement grid value for y
    // format: 7s
    FIELD_INT CTC_REFINEMENT_GRID_DY[100];

    // validity of refinement grid value
    // format: 1u
    FIELD_UINT CTC_REFINEMENT_GRID_VALID[100];

    // mantissa of perspective param. Allowed values: -131071:-65536 | 0 | 65536:131071
    // format: 18s
    FIELD_INT CTC_PERSPECTIVE_PARAMS_M[72];

    // exponent of perspective param
    // format: 6s
    FIELD_INT CTC_PERSPECTIVE_PARAMS_E[72];

    // Threshold on LMC-refinement difference
    // format: 5uQ4
    FIELD_UINT CTC_LMC_MV_DIFF_THR;

    ////////////////////////////
    // Defines how out-of-input-frame pixels are treated.
    // Note: In case of 8 bps, only the 8 LSbits are taken from the values configured into y, cb and cr fields.
    ////////////////////////////
    // 0: Out-of-frame pixel is populated with a predefined value.
    // 1: Out-of-frame pixel is populated using duplication.
    // format: 1u
    FIELD_UINT OPG_INVALID_OUTPUT_TREATMENT_CALCULATE;

    // Y Output sample values for out-of-input-frame pixels, in case calculate (bit 0) is equal to 0
    // format: 10u
    FIELD_UINT OPG_INVALID_OUTPUT_TREATMENT_Y;

    // Cb Output sample values for out-of-input-frame pixels, in case calculate (bit 0) is equal to 0
    // format: 10u
    FIELD_UINT OPG_INVALID_OUTPUT_TREATMENT_CB;

    // Cr Output sample values for out-of-input-frame pixels, in case calculate (bit 0) is equal to 0
    // format: 10u
    FIELD_UINT OPG_INVALID_OUTPUT_TREATMENT_CR;

    ////////////////////////////
    // When Y_INTERPOLATION_TYPE = 0, luma samples are interpolated using the coefficients in this LUT.
    // These fields contain the first 2 coefficients.
    ////////////////////////////
    // LUT(0)
    // format: 14s
    FIELD_INT OPG_INTERPOLATION_LUT_0[16];

    // LUT(1)
    // format: 14s
    FIELD_INT OPG_INTERPOLATION_LUT_1[16];

    ////////////////////////////
    // When Y_INTERPOLATION_TYPE = 0, luma samples are interpolated using the coefficients in this LUT.
    // This field contains the 3rd coefficient.
    ////////////////////////////
    // LUT(2)
    // format: 14s
    FIELD_INT OPG_INTERPOLATION_LUT_2[16];

} ICA_REG_v30;

// ############ Functions ############
int32_t Validate_ICA_REG_v30( ICA_REG_v30* regStruct, int hwVersion );
void SetDefaultVal_ICA_REG_v30( ICA_REG_v30* regStruct, int hwVersion );
// ###################################
#ifdef __cplusplus
}
#endif


#endif //__ICA_REGISTERS_V30_H__

