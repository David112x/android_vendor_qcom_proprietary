// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------


#ifndef __DSX_REGISTERS_H__
#define __DSX_REGISTERS_H__

#include "CommonDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DSX_REG_TAG
{

    // 0 – Disabled
    // 1 – Enabled
    // When disabled all of the Luma DS logic is shut-down and the Luma module is bypassed (i.e. output = input)
    // format: 1u
    FIELD_UINT en_Y_MODULE_CFG;

    // 0 – Disabled
    // 1 – Enabled
    // When disabled all of the Chroma DS logic is shut-down and the Chroma module is bypassed (i.e. output = input)
    // format: 1u
    FIELD_UINT en_C_MODULE_CFG;

    // 0 – regular operation - cropping and scaling
    // 1 – only cropping is performed (determined by luma_start_location_x, luma_start_location_y, luma_out_width and luma_out_height)
    // format: 1u
    FIELD_UINT luma_mode;

    // 0 – regular operation - cropping and scaling
    // 1 – only cropping is performed (determined by chroma_start_location_x, chroma_start_location_y, chroma_out_width and chroma_out_height)
    // format: 1u
    FIELD_UINT chroma_mode;

    // horizontal starting location (including phase) for Luma. When mode==1 the integer value must be even (and fractional bits should be 0)
    // format: 31uQ17
    FIELD_UINT luma_start_location_x;

    // vertical starting location (including phase) for Luma. When mode==1 the integer value must be even (and fractional bits should be 0)
    // format: 31uQ17
    FIELD_UINT luma_start_location_y;

    // horizontal starting location (including phase) for Chroma. When mode==1 the fractional bits should be 0
    // format: 30uQ17
    FIELD_UINT chroma_start_location_x;

    // vertical starting location (including phase) for Chroma. When mode==1 the fractional bits should be 0
    // format: 30uQ17
    FIELD_UINT chroma_start_location_y;

    // horizontal scale ratio for Luma (step between locations of output pixels in input coordinates)
    // format: 20uQ17
    FIELD_UINT luma_scale_ratio_x;

    // vertical scale ratio for Luma (step between locations of output pixels in input coordinates)
    // format: 20uQ17
    FIELD_UINT luma_scale_ratio_y;

    // horizontal scale ratio for Chroma (step between locations of output pixels in input coordinates)
    // format: 20uQ17
    FIELD_UINT chroma_scale_ratio_x;

    // vertical scale ratio for Chroma (step between locations of output pixels in input coordinates)
    // format: 20uQ17
    FIELD_UINT chroma_scale_ratio_y;

    // output image width for Luma
    // format: 11u
    FIELD_UINT luma_out_width;

    // output image height for Luma
    // format: 14u
    FIELD_UINT luma_out_height;

    // output image width for Chroma (should be half that of Luma)
    // format: 10u
    FIELD_UINT chroma_out_width;

    // output image height for Chroma (should be half that of Luma)
    // format: 13u
    FIELD_UINT chroma_out_height;

    // Input image width for Luma. Restricted by pipeline (previous module line buffer).
    // format: 13u
    FIELD_UINT luma_input_width;

    // input image height for Luma
    // format: 15u
    FIELD_UINT luma_input_height;

    // input image width for Chroma
    // format: 12u
    FIELD_UINT chroma_input_width;

    // input image height for Chroma
    // format: 14u
    FIELD_UINT chroma_input_height;

    // horizontal kernel samples for Luma. Constraints: for every phase the kernel weights should "sum to one" and absolute value of the intermediate sum of kernel weights should better be less than 2 (the exact constraints appear in HLD).
    // format: 36u
    FIELD_UINT64 luma_kernel_weights_horiz[96];

    // vertical kernel samples for Luma. Constraints: for every phase the kernel weights should "sum to one" and absolute value of the intermediate sum of kernel weights should better be less than 2 (the exact constraints appear in HLD).
    // format: 36u
    FIELD_UINT64 luma_kernel_weights_vert[96];

    // horizontal kernel samples for Chroma. Constraints: for every phase the kernel weights should "sum to one" and absolute value of the intermediate sum of kernel weights should better be less than 2 (the exact constraints appear in HLD).
    // format: 36u
    FIELD_UINT64 chroma_kernel_weights_horiz[48];

    // vertical kernel samples for Chroma. Constraints: for every phase the kernel weights should "sum to one" and absolute value of the intermediate sum of kernel weights should better be less than 2 (the exact constraints appear in HLD).
    // format: 36u
    FIELD_UINT64 chroma_kernel_weights_vert[48];

    // enable external configuration of padding weights for Luma
    // format: 1u
    FIELD_UINT luma_padding_weights_en;

    // enable external configuration of padding weights for Chroma
    // format: 1u
    FIELD_UINT chroma_padding_weights_en;

    // weights used for first 6 input pixels in a column (for each pixel 4 weights, one per accumulator, are provided) - for Luma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one" and absolute value of intermediate sum of weights should be less than 2.
    // format: 13sQ11
    FIELD_INT luma_padding_weights_top[24];

    // weights used for last 6 input pixels in a column (for each pixel 4 weights, one per accumulator, are provided) - for Luma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one" and absolute value of intermediate sum of weights should be less than 2.
    // format: 13sQ11
    FIELD_INT luma_padding_weights_bot[24];

    // weights used for first 6 input pixels in a row (for each pixel 4 weights, one per accumulator, are provided) - for Luma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one" and absolute value of intermediate sum of weights should be less than 2.
    // format: 13sQ11
    FIELD_INT luma_padding_weights_left[24];

    // weights used for last 6 input pixels in a row (for each pixel 4 weights, one per accumulator, are provided) - for Luma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one" and absolute value of intermediate sum of weights should be less than 2.
    // format: 13sQ11
    FIELD_INT luma_padding_weights_right[24];

    // weights used for first 4 input pixels in a column (for each pixel 2 weights, one per accumulator, are provided) - for Chroma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one" and absolute value of intermediate sum of weights should be less than 2.
    // format: 13sQ11
    FIELD_INT chroma_padding_weights_top[8];

    // weights used for last 4 input pixels in a column (for each pixel 2 weights, one per accumulator, are provided) - for Chroma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one" and absolute value of intermediate sum of weights should be less than 2.
    // format: 13sQ11
    FIELD_INT chroma_padding_weights_bot[8];

    // weights used for first 4 input pixels in a row (for each pixel 2 weights, one per accumulator, are provided) - for Chroma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one" and absolute value of intermediate sum of weights should be less than 2.
    // format: 13sQ11
    FIELD_INT chroma_padding_weights_left[8];

    // weights used for last 4 input pixels in a row (for each pixel 2 weights, one per accumulator, are provided) - for Chroma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one" and absolute value of intermediate sum of weights should be less than 2.
    // format: 13sQ11
    FIELD_INT chroma_padding_weights_right[8];

} DSX_REG;

// ############ Functions ############
int32_t Validate_DSX_REG( DSX_REG* regStruct );
void SetDefaultVal_DSX_REG( DSX_REG* regStruct );
// ###################################
#ifdef __cplusplus
}
#endif


#endif //__DSX_REGISTERS_H__

