// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------


#ifndef __ICA_CHROMATIX_H__
#define __ICA_CHROMATIX_H__

#include "CommonDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ICA_TOP_TAG
{
    // 0: 4x4 kernel
    // 1: Bi-linear (2x2)
    // format: 1u
    PARAM_UINT y_interpolation_type;

} ICA_TOP;

typedef struct ICA_CTC_TAG
{
    // 0: Disabled, 1: Enabled
    // format: 1u
    PARAM_UINT ctc_transform_grid_enable;

    // 0: 35x27 samples; 1: 67x51
    // format: 2u
    PARAM_UINT ctc_grid_transform_geometry;

    // UNDISTORTED_IPE_OUT_TO_DISTORTED_INPUT_GRID: Grid value for x
    // format: 18sQ4
    PARAM_INT ctc_grid_x[3417];

    // UNDISTORTED_IPE_OUT_TO_DISTORTED_INPUT_GRID: Grid value for y
    // format: 18sQ4
    PARAM_INT ctc_grid_y[3417];

} ICA_CTC;

typedef struct ICA_OPG_TAG
{
    // 0: Out-of-frame pixel is populated with a predefined value.
    // 1: Out-of-frame pixel is populated using duplication.
    // format: 1u
    PARAM_UINT opg_invalid_output_treatment_calculate;

    // Y Output sample values for out-of-input-frame pixels, in case calculate (bit 0) is equal to 0
    // format: 10u
    PARAM_UINT opg_invalid_output_treatment_y;

    // Cb Output sample values for out-of-input-frame pixels, in case calculate (bit 0) is equal to 0
    // format: 10u
    PARAM_UINT opg_invalid_output_treatment_cb;

    // Cr Output sample values for out-of-input-frame pixels, in case calculate (bit 0) is equal to 0
    // format: 10u
    PARAM_UINT opg_invalid_output_treatment_cr;

    // LUT(0)
    // format: 14s
    PARAM_INT opg_interpolation_lut_0[16];

    // LUT(1)
    // format: 14s
    PARAM_INT opg_interpolation_lut_1[16];

    // LUT(2)
    // format: 14s
    PARAM_INT opg_interpolation_lut_2[16];

} ICA_OPG;

typedef struct ICA_Chromatix_TAG
{
    ICA_TOP top;
    ICA_CTC ctc;
    ICA_OPG opg;
    // DISTORTED_INPUT_TO_UNDISTORTED_IPE_OUT_GRID: Grid value for x
    // format: 18sQ3
    PARAM_INT ld_i2u_grid_x[3417];

    // DISTORTED_INPUT_TO_UNDISTORTED_IPE_OUT_GRID: Grid value for y
    // format: 18sQ3
    PARAM_INT ld_i2u_grid_y[3417];

    // UNDISTORTED_IPE_OUT_TO_DISTORTED_INPUT_GRID: is grid valid
    // format: 1u
    PARAM_UINT ld_u2i_grid_valid;

    // DISTORTED_INPUT_TO_UNDISTORTED_IPE_OUT_GRID: is grid valid
    // format: 1u
    PARAM_UINT ld_i2u_grid_valid;

    // 0: 35x27 samples; 1: 67x51
    // format: 2u
    PARAM_UINT ld_i2u_grid_geometry;

    // 0: LDC calibration is done on Sensor full FOV;
    // 1: LDC calibration is done on Sensor mode output FOV (==IFE Input FOV);
    // 2: LDC calibration is done on Use case FOV based on IFE output @DZX1
    // format: 2u
    PARAM_UINT ldc_calib_domain;

    // The physical size of the undistorted sensor.
    // Width is in square pixels. Can be zero if no LD is needed.
    // format: 16u
    PARAM_UINT ld_full_out_width;

    // The physical size of the undistorted sensor.
    // Height is in square pixels. Can be zero if no LD is needed.
    // format: 16u
    PARAM_UINT ld_full_out_height;

    // 0 = custom/no model (grid is not by any of the following models model ), 1 = regular, 2= fisheye, 3 = reserved for a new model
    // format: 3u
    PARAM_UINT ldc_model_type;

    // Model parameters to be used with the selected model.
    // Parameters interpretation per model:
    // For model 1: [k1 k2 p1 p2 k3]
    // For model 2: [k1 k2 k3 k4]
    // format: float
    PARAM_FLOAT model_parameters[32];

    // Camera matrix \ Focal length that was extracted during camera calibration for LDC. Image sizes are as defined below and are not normilized and according to calibration images.
    // format: float
    PARAM_FLOAT focal_length_x;

    // Camera matrix \ Focal length that was extracted during camera calibration for LDC. Image sizes are as defined below and are not normilized and according to calibration images.
    // format: float
    PARAM_FLOAT focal_length_y;

    // Camera matrix \ Optical center that was extracted during camera calibration for LDC. Image sizes are as defined below and are not normilized and according to calibration images.
    // format: float
    PARAM_FLOAT optical_center_x;

    // Camera matrix \ Optical center that was extracted during camera calibration for LDC. Image sizes are as defined below and are not normilized and according to calibration images.
    // format: float
    PARAM_FLOAT optical_center_y;

    // The physical size of the distorted sensor.
    // Can be zero if no LD is needed.
    // format: 16u
    PARAM_UINT image_size_distorted_x;

    // The physical size of the distorted sensor.
    // Can be zero if no LD is needed.
    // format: 16u
    PARAM_UINT image_size_distorted_y;

    // Reserved parameter 1
    // format: float
    PARAM_FLOAT res_param_1;

    // Reserved parameter 2
    // format: float
    PARAM_FLOAT res_param_2;

    // Reserved parameter 3
    // format: float
    PARAM_FLOAT res_param_3;

    // Reserved parameter 4
    // format: float
    PARAM_FLOAT res_param_4;

    // Reserved parameter 5
    // format: float
    PARAM_FLOAT res_param_5;

    // Reserved parameter 6
    // format: float
    PARAM_FLOAT res_param_6;

    // Reserved parameter 7
    // format: float
    PARAM_FLOAT res_param_7;

    // Reserved parameter 8
    // format: float
    PARAM_FLOAT res_param_8;

    // Reserved parameter 9
    // format: float
    PARAM_FLOAT res_param_9;

    // Reserved parameter 10
    // format: float
    PARAM_FLOAT res_param_10;

    // Reserved Lut parameter 1
    // format: float
    PARAM_FLOAT res_lut_param_1[32];

    // Reserved Lut parameter 2
    // format: float
    PARAM_FLOAT res_lut_param_2[32];

} ICA_Chromatix;

// ############ Functions ############
int32_t Validate_ICA_Chromatix( ICA_Chromatix* regStruct, int hwVersion );
void SetDefaultVal_ICA_Chromatix( ICA_Chromatix* regStruct );
// ###################################
#ifdef __cplusplus
}
#endif


#endif //__ICA_CHROMATIX_H__

