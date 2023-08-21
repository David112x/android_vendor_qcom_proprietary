// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------


#ifndef __DSX_CHROMATIX_H__
#define __DSX_CHROMATIX_H__

#include "CommonDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DSX_GENERAL_TAG
{
    // enable external configuration of padding weights for Luma (controlled by luma_padding_weights_* parameters)
    // format: 1u
    PARAM_UINT luma_padding_weights_en;

    // enable external configuration of padding weights for Chroma (controlled by chroma_padding_weights_* parameters)
    // format: 1u
    PARAM_UINT chroma_padding_weights_en;

    // Weights used for first 6 input pixels in a column (for each pixel 4 weights, one per accumulator, are provided) - for Luma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one".
    // format: 13sQ11
    PARAM_INT luma_padding_weights_top[24];

    // Weights used for last 6 input pixels in a column (for each pixel 4 weights, one per accumulator, are provided) - for Luma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one".
    // format: 13sQ11
    PARAM_INT luma_padding_weights_bot[24];

    // Weights used for first 6 input pixels in a row (for each pixel 4 weights, one per accumulator, are provided) - for Luma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one".
    // format: 13sQ11
    PARAM_INT luma_padding_weights_left[24];

    // Weights used for last 6 input pixels in a row (for each pixel 4 weights, one per accumulator, are provided) - for Luma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one".
    // format: 13sQ11
    PARAM_INT luma_padding_weights_right[24];

    // Weights used for first 4 input pixels in a column (for each pixel 2 weights, one per accumulator, are provided) - for Chroma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one".
    // format: 13sQ11
    PARAM_INT chroma_padding_weights_top[8];

    // Weights used for last 4 input pixels in a column (for each pixel 2 weights, one per accumulator, are provided) - for Chroma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one".
    // format: 13sQ11
    PARAM_INT chroma_padding_weights_bot[8];

    // Weights used for first 4 input pixels in a row (for each pixel 2 weights, one per accumulator, are provided) - for Chroma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one".
    // format: 13sQ11
    PARAM_INT chroma_padding_weights_left[8];

    // Weights used for last 4 input pixels in a row (for each pixel 2 weights, one per accumulator, are provided) - for Chroma. Constraints: at every output location affected by input pixels with externally-configured padding weights, the weights should approximately "sum to one".
    // format: 13sQ11
    PARAM_INT chroma_padding_weights_right[8];

} DSX_GENERAL;

typedef struct DSX_Chromatix_TAG
{
    DSX_GENERAL general;
    // Determines how kernel weights and padding weights are calculated:
    // 0 = run-time calculation by SW using "ds4 coefficients" from DS4 block + "ica coefficients" from ICA0 block + VSR scale ratio
    // 1 = use precalculated values from *_kernel_weights_* and *_padding_weights_* parameters, directly based on VSR scale ratio
    // format: 1u
    PARAM_UINT param_calc_mode;

    // Horizontal kernel samples for Luma (provided for phases>=0 starting with phase=0; negative phases are reflected symmetrically). The kernel is sampled  at 1/32-pixel steps for scale ratios <= 1/1.5 and at 1/64-pixel steps for scale ratios > 1/1.5. Constraints: weights for all pixels for a given phase should "sum to one"
    // format: 12sQ10
    PARAM_INT luma_kernel_weights_unpacked_horiz[192];

    // Vertical kernel samples for Luma (provided for phases>=0 starting with phase=0; negative phases are reflected symmetrically). The kernel is sampled either at 1/32-pixel steps (for scale ratios <= 1/1.5) or at 1/64-pixel steps (for scale ratios > 1/1.5). Constraints: weights for all pixels for a given phase should "sum to one"
    // format: 12sQ10
    PARAM_INT luma_kernel_weights_unpacked_vert[192];

    // Horizontal kernel samples for Chroma (provided for phases>=0 starting with phase=0; negative phases are reflected symmetrically). The kernel is sampled either at 1/32-pixel steps (for scale ratios <= 1/1.5) or at 1/64-pixel steps (for scale ratios > 1/1.5). Constraints: weights for all pixels for a given phase should "sum to one"
    // format: 12sQ10
    PARAM_INT chroma_kernel_weights_unpacked_horiz[96];

    // Vertical kernel samples for Chroma (provided for phases>=0 starting with phase=0; negative phases are reflected symmetrically). The kernel is sampled either at 1/32-pixel steps (for scale ratios <= 1/1.5) or at 1/64-pixel steps (for scale ratios > 1/1.5). Constraints: weights for all pixels for a given phase should "sum to one"
    // format: 12sQ10
    PARAM_INT chroma_kernel_weights_unpacked_vert[96];

} DSX_Chromatix;

// ############ Functions ############
int32_t Validate_DSX_Chromatix( DSX_Chromatix* regStruct );
void SetDefaultVal_DSX_Chromatix( DSX_Chromatix* regStruct );
// ###################################
#ifdef __cplusplus
}
#endif


#endif //__DSX_CHROMATIX_H__

