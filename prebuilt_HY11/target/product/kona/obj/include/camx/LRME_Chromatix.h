// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------


#ifndef __LRME_CHROMATIX_H__
#define __LRME_CHROMATIX_H__

#include "CommonDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LRME_Chromatix_TAG
{

    // Enable bit for the unit
    // format: 1u
    PARAM_UINT en;

    // Horizontal search range to be used. The number of horizontal search locations is 2*search_range_x + 1. Search range is restricted to 2 options only: +/-12 x +/-8 or +/-8 x +/-6
    // format: 4u
    PARAM_UINT search_range_x;

    // Vertical search range to be used. The number of vertical search locations is 2*search_range_y + 1.  Search range is restricted to 2 options only: +/-12 x +/-8 or +/-8 x +/-6
    // format: 4u
    PARAM_UINT search_range_y;

    // Whether sub-pixel ME (Optical Flow) is enabled
    // format: 1u
    PARAM_UINT sub_pel_search_enable;

    // Maximum allowed SAD. If SAD exceeds this threshold the block will be marked invalid (probably ME problem due to occlusion, too fast motion, etc).
    // format: 15u
    PARAM_UINT max_allowed_sad;

    // Minimum allowed tarMAD value. If below this value (non-informative block) - the block will be marked invalid.
    // format: 10u
    PARAM_UINT min_allowed_tar_mad;

    // Locations with regular SAD difference below this threshold are considered to have exactly the same similarity measure (even if normalized SAD difference is big).
    // format: 10u
    PARAM_UINT meaningful_sad_diff;

    // Determines the lower limit of denominator used in normalized difference of SAD calculation to avoid overflow.
    // format: 10u
    PARAM_UINT min_sad_diff_denom;

    // Provides a requirement on minimal normalized difference of SADs at distance 1-9 pixels (for big distances the expected differences should be big). Default values: 0, 128, 128, 128, 128, 128, 128, 128 (no penalty to single pixel distance; afterwards same penalty for all other distances)
    // format: 9u
    PARAM_UINT robustness_measure_dist_map[8];

    // Whether to calculate and apply transform confidence (based on coverage of the frame by LRME-calculated valid MVs). Frames with higher coverage get higher confidence.
    // format: 1u
    PARAM_UINT enable_transform_confidence;

    // Defines mapping function from calculated transform confidence to actually used transform confidence. The calculated confidence is in the range 0:256 (8 bit fraction). The mapping is:  actual confidence =  transform_confidence_mapping_base when calculated confidence <= transform_confidence_mapping_c1; 256 when calculated confidence > transform_confidence_mapping_c2 and linear interpolation in-between.
    // format: 8u
    PARAM_UINT transform_confidence_mapping_base;

    // Defines mapping function from calculated transform confidence to actually used transform confidence. The calculated confidence is in the range 0:256 (8 bit fraction). The mapping is:  actual confidence =  transform_confidence_mapping_base when calculated confidence <= transform_confidence_mapping_c1; 256 when calculated confidence > transform_confidence_mapping_c2 and linear interpolation in-between.
    // format: 8uQ7
    PARAM_UINT transform_confidence_mapping_c1;

    // Defines mapping function from calculated transform confidence to actually used transform confidence. The calculated confidence is in the range 0:256 (8 bit fraction). The mapping is:  actual confidence =  transform_confidence_mapping_base when calculated confidence <= transform_confidence_mapping_c1; 256 when calculated confidence > transform_confidence_mapping_c2 and linear interpolation in-between.
    // format: 8uQ7
    PARAM_UINT transform_confidence_mapping_c2;

    // When calculated transform confidence (before mapping) is below this threshold, the transform is ignored and replaced by identity transform. Hysteresis is used on this parameter to avoid excessive switching
    // format: 8u
    PARAM_UINT transform_confidence_thr_to_force_identity_transform;

    // Defines which transform model to use during transform estimation stage (0=projective, 1=affine, 2=rigid, 3=rigid with post-processing)
    // format: 2u
    PARAM_UINT transform_model;

    // Defines the strategy of determining the search_range:
    // 0 = Flow decisions may override chromatix value
    // 1 = Chromatix value is used irrespective of flow decisions
    // format: 1u
    PARAM_UINT search_range_override;

} LRME_Chromatix;

// ############ Functions ############
int32_t Validate_LRME_Chromatix( LRME_Chromatix* regStruct );
void SetDefaultVal_LRME_Chromatix( LRME_Chromatix* regStruct );
// ###################################
#ifdef __cplusplus
}
#endif


#endif //__LRME_CHROMATIX_H__

