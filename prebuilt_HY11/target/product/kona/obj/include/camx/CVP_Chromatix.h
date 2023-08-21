// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------


#ifndef __CVP_CHROMATIX_H__
#define __CVP_CHROMATIX_H__

#include "CommonDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CVP_Chromatix_TAG
{
    // Enable image based registration
    // format: 1u
    PARAM_UINT image_based_alignment_enable;

    // Maximum allowed NCC If NCC exceeds this threshold the block will be marked invalid (probably ME problem due to occlusion, too fast motion, etc).
    // format: 11u
    PARAM_UINT robustness_max_allowed_ncc;

    // Minimum allowed tar variance value. If below this value (non-informative block) - the block will be marked invalid.
    // format: 16u
    PARAM_UINT robustness_min_allowed_tar_var;

    // Locations with NCC difference below this threshold are considered to have exactly the same similarity measure
    // format: 11u
    PARAM_UINT robustness_meaningful_ncc_diff;

    // Provides a requirement on minimal normalized difference of SADs at distance 1-8 pixels (for big distances the expected differences should be big). Default values: 0, 128, 128, 128, 128, 128, 128, 128 (no penalty to single pixel distance; afterwards same penalty for all other distances)
    // format: 9u
    PARAM_UINT robustness_measure_dist_map[8];

    // Enable 5x5 filtering before calculating descriptor
    // format: 1u
    PARAM_UINT descriptor_lpf;

    // Feature score shift value.
    // Camera SW translate chromatix range to FW range
    // 0: shift by 0, 1: shift by 4 ,2: shift by 8, 3 shift by 12
    // format: 2u
    PARAM_UINT fpx_score_shift;

    // Corner with Harris measure value lower than this value will be ignored, this parameter is related to   reg_flow_ hcd_cm_shift
    // format: 16u
    PARAM_UINT fpx_threshold;

    // Hamming distance between two descriptors below which the blocks are considered as match.
    // Camera SW Should force the number to be module 4.
    // format: 8u
    PARAM_UINT desc_match_threshold;

    // Enabling inlier tracking, Improve decistion's coherency between frames.
    // format: 1u
    PARAM_UINT inlier_track_enable;

    // Whether to calculate and apply transform confidence (based on coverage of the frame by  calculated valid MVs). Frames with higher coverage get higher confidence.
    // format: 1u
    PARAM_UINT enable_transform_confidence;

    // Defines mapping function from calculated transform confidence to actually used transform confidence. The calculated confidence is in the range 0:256 (8 bit fraction). The mapping is:  actual confidence =  transform_confidence_mapping_base when calculated confidence <= transform_confidence_mapping_c1; 256 when calculated confidence > transform_confidence_mapping_c2 and linear interpolation in-between. Not relevant for multi frame flows.
    // format: 8u
    PARAM_UINT transform_confidence_mapping_base;

    // Defines mapping function from calculated transform confidence to actually used transform confidence. The calculated confidence is in the range 0:256 (8 bit fraction). The mapping is:  actual confidence =  transform_confidence_mapping_base when calculated confidence <= transform_confidence_mapping_c1; 256 when calculated confidence > transform_confidence_mapping_c2 and linear interpolation in-between. Not relevant for multi frame flows.
    // format: 8uQ7
    PARAM_UINT transform_confidence_mapping_c1;

    // Defines mapping function from calculated transform confidence to actually used transform confidence. The calculated confidence is in the range 0:256 (8 bit fraction). The mapping is:  actual confidence =  transform_confidence_mapping_base when calculated confidence <= transform_confidence_mapping_c1; 256 when calculated confidence > transform_confidence_mapping_c2 and linear interpolation in-between. Not relevant for multi frame flows.
    // format: 8uQ7
    PARAM_UINT transform_confidence_mapping_c2;

    // When calculated transform confidence (before mapping) is below this threshold, the transform is ignored and replaced by identity transform. Hysteresis is used on this parameter to avoid excessive switching
    // format: 8u
    PARAM_UINT transform_confidence_thr_to_force_identity_transform;

    // Defines which transform model to use during transform estimation stage (0=projective, 1=affine, 2=rigid, 3=rigid with post-processing)
    // format: 2u
    PARAM_UINT transform_model;

    // This Parameter is relevant form multi frame flows. The parameter defines the input resolution of the alignement process. 0:1440p, 1:1280p, 2:1080p, 3:960p, 4:720p, 5:540p 6:270p. Maximal image width supported is 1920. In case input image width > 1920. The biggest resolution in which width <=1920 will be chosen.
    // format: 4u
    PARAM_UINT multi_frame_input_resolution;

    // This parameter is relevant to VSR video flow. In order to reduce the noise level, extra down scale is performed before registration. The parameter sets the controls the downscale ratio. In case value == 1 no downscaling is done before registration.
    // format: float
    PARAM_FLOAT video_registration_down_scale_ratio;

} CVP_Chromatix;

// ############ Functions ############
int32_t Validate_CVP_Chromatix( CVP_Chromatix* regStruct );
void SetDefaultVal_CVP_Chromatix( CVP_Chromatix* regStruct );
// ###################################
#ifdef __cplusplus
}
#endif


#endif //__CVP_CHROMATIX_H__

