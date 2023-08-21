////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxlrmeproperty.h
/// @brief Define Qualcomm Technologies, Inc. FD proprietary data for holding internal properties/events
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXLRMEPROPERTY_H
#define CAMXLRMEPROPERTY_H

#include "camxdefs.h"

CAMX_NAMESPACE_BEGIN

/// @brief Structure LRME frame settings
typedef struct
{
    INT32 LRMEStepX;
    INT32 LRMEStepY;
    INT32 LRMERefValid;
    INT32 LRMEresultFormat;
    INT32 LRMETarOffsetX;
    INT32 LRMETarOffsetY;
    INT32 LRMERefOffsetX;
    INT32 LRMERefOffsetY;
    INT32 LRMEsubpelSearchEnable;
    INT32 LRMETarH;
    INT32 LRMETarW;                  /// Tar image width
    INT32 LRMEUpscaleFactor;
    INT32 fullWidth;
    INT32 fullHeight;
    INT32 alternateSkipProcessing;
} LRMEPropertyFrameSettings;

/// @brief enum to describe LRME/RANSAC transform type mask
enum LRMEtransformType
{
    RegularTransform          = 0,
    HFRInterpolationTransform = 1,
    UnityTransform            = 2,
};

static const UINT32 LRMETransform_method                           = 1;
static const UINT32 LRMEChromatix_enable                           = 1;
static const UINT32 LRMEEnable_transform_confidence                = 1;
static const UINT32 LRMETransform_confidence_mapping_base          = 100;
static const UINT32 LRMETransform_confidence_mapping_c1            = 100;
static const UINT32 LRMETransform_confidence_mapping_c2            = 255;
static const UINT32 LRMETransform_confidence_thr_to_force_identity = 128;

CAMX_NAMESPACE_END

#endif // CAMXLRMEPROPERTY_H