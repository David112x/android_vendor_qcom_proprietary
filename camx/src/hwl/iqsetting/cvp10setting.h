// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cvp10setting.h
/// @brief CVP10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE FILE DC012: struct members not documented.

#ifndef CVP10SETTING_H
#define CVP10SETTING_H
#include "cvp_1_0_0.h"
#include "iqsettingutil.h"
#include "iqcommondefs.h"

static const INT32 ROBUSTNESS_MAX_ALLOWED_NCC_MIN                           = 0;
static const INT32 ROBUSTNESS_MAX_ALLOWED_NCC_MAX                           = 2047;
static const INT32 ROBUSTNESS_MIN_ALLOWED_TAR_VAR_MIN                       = 0;
static const INT32 ROBUSTNESS_MIN_ALLOWED_TAR_VAR_MAX                       = 65535;
static const INT32 ROBUSTNESS_MEANINGFUL_NCC_DIFF_MIN                       = 0;
static const INT32 ROBUSTNESS_MEANINGFUL_NCC_DIFF_MAX                       = 2047;
static const INT32 FPX_THRESHOLD_MIN                                        = 0;
static const INT32 FPX_THRESHOLD_MAX                                        = 65535;
static const INT32 DESC_MATCH_THRESHOLD_MIN                                 = 32;
static const INT32 DESC_MATCH_THRESHOLD_MAX                                 = 92;
static const INT32 ENABLE_TRANSFORM_CONFIDENCE_MIN                          = 0;
static const INT32 ENABLE_TRANSFORM_CONFIDENCE_MAX                          = 1;
static const INT32 TRANSFORM_CONFIDENCE_MAPPING_BASE_MIN                    = 0;
static const INT32 TRANSFORM_CONFIDENCE_MAPPING_BASE_MAX                    = 255;
static const INT32 TRANSFORM_CONFIDENCE_MAPPING_C1_MIN                      = 0;
static const INT32 TRANSFORM_CONFIDENCE_MAPPING_C1_MAX                      = 255;
static const INT32 TRANSFORM_CONFIDENCE_MAPPING_C2_MIN                      = 0;
static const INT32 TRANSFORM_CONFIDENCE_MAPPING_C2_MAX                      = 255;
static const INT32 TRANSFORM_CONFIDENCE_THR_TO_FORCE_IDENTITY_TRANSFORM_MIN = 0;
static const INT32 TRANSFORM_CONFIDENCE_THR_TO_FORCE_IDENTITY_TRANSFORM_MAX = 255;
static const UINT32 DESCRIPTOR_LPF_MIN                                      = 0;
static const UINT32 DESCRIPTOR_LPF_MAX                                      = 1;
static const UINT32 FPX_SCORE_SHIFT_MIN                                     = 0;
static const UINT32 FPX_SCORE_SHIFT_MAX                                     = 4;
static const UINT32 INLIER_TRACK_ENABLE_MIN                                 = 0;
static const UINT32 INLIER_TRACK_ENABLE_MAX                                 = 1;
static const UINT32 TRANSFORM_MODEL_MIN                                     = 0;
static const UINT32 TRANSFORM_MODEL_MAX                                     = 3;
static const UINT32 ROBUSTNESS_MEASURE_DIST_MAP_MIN                         =0;
static const UINT32 ROBUSTNESS_MEASURE_DIST_MAP_MAX                         =511;
static const UINT32 MULTI_FRAME_INPUT_RES_MIN                               = 0;
static const UINT32 MULTI_FRAME_INPUT_RES_MAX                               = 6;
static const FLOAT  VIDEO_REG_DS_RATIO_MIN                                  = 1.0f;
static const FLOAT  VIDEO_REG_DS_RATIO_MAX                                  = 6.0f;

// NOWHINE NC004c: Share code with system team
struct CVP10UnpackedField
{

    FLOAT   robustness_max_allowed_NCC;
    FLOAT   robustness_min_allowed_tar_var;
    FLOAT   robustness_meaningful_ncc_diff;
    FLOAT   fpx_threshold;
    FLOAT   desc_match_threshold;
    FLOAT   enable_transform_confidence;
    FLOAT   transform_confidence_mapping_base;
    FLOAT   transform_confidence_mapping_c1;
    FLOAT   transform_confidence_mapping_c2;
    FLOAT   transform_confidence_thr_to_force_identity_transform;
    UINT32  robustness_measure_dist_map[8];
    UINT32  descriptor_Lpf;
    UINT32  fpx_score_shift;
    UINT32  inlier_track_enable;
    UINT32  transform_model;
    UINT32  multi_frame_input_resolution;
    FLOAT   video_registration_down_scale_ratio;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements CVP10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class CVP10Setting
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
        CVP10InputData*                                       pInput,
        cvp_1_0_0::cvp10_rgn_dataType*                        pData,
        cvp_1_0_0::chromatix_cvp10_reserveType*               pReserveType,
        VOID*                                                 pUnpackedField);
};
#endif // CVP10SETTING_H