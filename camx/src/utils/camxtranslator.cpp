////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtranslator.cpp
/// @brief Translation utility class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdebugprint.h"
#include "camxdefs.h"
#include "camxosutils.h"
#include "camxtranslator.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Translator::ConvertROIFromCurrentToReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIRectangle Translator::ConvertROIFromCurrentToReference(
    const CHIDimension*  pReferenceFrameDimension,
    const CHIDimension*  pCurrentFrameDimension,
    const CHIRectangle*  pCurrentFrameMapWrtReference,
    const CHIRectangle*  pROIWrtCurrentFrame)
{
    CHIRectangle ROIWrtReference;

    if (NULL == pROIWrtCurrentFrame)
    {
        ROIWrtReference.left   = 1;
        ROIWrtReference.top    = 1;
        ROIWrtReference.width  = 1;
        ROIWrtReference.height = 1;
    }
    else if ((NULL == pReferenceFrameDimension) || (NULL == pCurrentFrameDimension) || (NULL == pCurrentFrameMapWrtReference))
    {
        ROIWrtReference = *pROIWrtCurrentFrame;
    }
    else if ((0 == pReferenceFrameDimension->width)     || (0 == pReferenceFrameDimension->height)     ||
             (0 == pCurrentFrameDimension->width)       || (0 == pCurrentFrameDimension->height)       ||
             (0 == pCurrentFrameMapWrtReference->width) || (0 == pCurrentFrameMapWrtReference->height) ||
             (0 == pROIWrtCurrentFrame->width)          || (0 == pROIWrtCurrentFrame->height))
    {
        ROIWrtReference = *pROIWrtCurrentFrame;
    }
    else
    {
        // Calculate downscale ratio of current frame map in reference frame to current frame dimensions
        FLOAT downscaleRatioWidth  = static_cast<FLOAT>(
                                     static_cast<FLOAT>(pCurrentFrameMapWrtReference->width)  /
                                     static_cast<FLOAT>(pCurrentFrameDimension->width));
        FLOAT downscaleRatioHeight = static_cast<FLOAT>(
                                     static_cast<FLOAT>(pCurrentFrameMapWrtReference->height) /
                                     static_cast<FLOAT>(pCurrentFrameDimension->height));

        CAMX_ASSERT(0 != downscaleRatioWidth);
        CAMX_ASSERT(0 != downscaleRatioHeight);

        // Calculate ROI w.r.t map rectangle in reference frame corresponds current frame.
        CHIRectangle ROIWrtCurrentFrameMapInReference;
        ROIWrtCurrentFrameMapInReference.left   = static_cast<UINT32>
                                                  (static_cast<FLOAT>(pROIWrtCurrentFrame->left)   * downscaleRatioWidth);
        ROIWrtCurrentFrameMapInReference.top    = static_cast<UINT32>
                                                  (static_cast<FLOAT>(pROIWrtCurrentFrame->top)    * downscaleRatioHeight);
        ROIWrtCurrentFrameMapInReference.width  = static_cast<UINT32>
                                                  (static_cast<FLOAT>(pROIWrtCurrentFrame->width)  * downscaleRatioWidth);
        ROIWrtCurrentFrameMapInReference.height = static_cast<UINT32>
                                                  (static_cast<FLOAT>(pROIWrtCurrentFrame->height) * downscaleRatioHeight);

        // Calculate ROI w.r.t reference frame
        ROIWrtReference.left   = ROIWrtCurrentFrameMapInReference.left + pCurrentFrameMapWrtReference->left;
        ROIWrtReference.top    = ROIWrtCurrentFrameMapInReference.top  + pCurrentFrameMapWrtReference->top;
        ROIWrtReference.width  = ROIWrtCurrentFrameMapInReference.width;
        ROIWrtReference.height = ROIWrtCurrentFrameMapInReference.height;
    }

    return ROIWrtReference;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Translator::ConvertROIFromReferenceToCurrent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIRectangle Translator::ConvertROIFromReferenceToCurrent(
    const CHIDimension*  pCurrentFrameDimension,
    const CHIRectangle*  pCurrentFrameMapWrtReference,
    const CHIRectangle*  pROIWrtReferenceFrame)
{
    CHIRectangle ROIWrtCurrent;

    if (NULL == pROIWrtReferenceFrame)
    {
        ROIWrtCurrent.left   = 1;
        ROIWrtCurrent.top    = 1;
        ROIWrtCurrent.width  = 1;
        ROIWrtCurrent.height = 1;
    }
    else if ((NULL == pCurrentFrameDimension) || (NULL == pCurrentFrameMapWrtReference))
    {
        ROIWrtCurrent = *pROIWrtReferenceFrame;
    }
    else if ((0 == pCurrentFrameDimension->width)       || (0 == pCurrentFrameDimension->height)       ||
             (0 == pCurrentFrameMapWrtReference->width) || (0 == pCurrentFrameMapWrtReference->height) ||
             (0 == pROIWrtReferenceFrame->width)        || (0 == pROIWrtReferenceFrame->height))
    {
        ROIWrtCurrent = *pROIWrtReferenceFrame;
    }
    else
    {
        // Calculate downscale ratio of current frame map in reference frame to current frame dimensions
        FLOAT downscaleRatioWidth  = static_cast<FLOAT>(
                                     static_cast<FLOAT>(pCurrentFrameMapWrtReference->width) /
                                     static_cast<FLOAT>(pCurrentFrameDimension->width));
        FLOAT downscaleRatioHeight = static_cast<FLOAT>(
                                     static_cast<FLOAT>(pCurrentFrameMapWrtReference->height) /
                                     static_cast<FLOAT>(pCurrentFrameDimension->height));

        CAMX_ASSERT(0 != downscaleRatioWidth);
        CAMX_ASSERT(0 != downscaleRatioHeight);

        // Calculate ROI w.r.t map rectangle in reference frame corresponds current frame.
        CHIRectangle ROIWrtCurrentFrameMapInReference;
        ROIWrtCurrentFrameMapInReference.left   = pROIWrtReferenceFrame->left   - pCurrentFrameMapWrtReference->left;
        ROIWrtCurrentFrameMapInReference.top    = pROIWrtReferenceFrame->top    - pCurrentFrameMapWrtReference->top;
        ROIWrtCurrentFrameMapInReference.width  = pROIWrtReferenceFrame->width;
        ROIWrtCurrentFrameMapInReference.height = pROIWrtReferenceFrame->height;

        // Calculate ROI w.r.t reference frame
        ROIWrtCurrent.left   = static_cast<UINT32>
                               (static_cast<FLOAT>(ROIWrtCurrentFrameMapInReference.left)   / downscaleRatioWidth);
        ROIWrtCurrent.top    = static_cast<UINT32>
                               (static_cast<FLOAT>(ROIWrtCurrentFrameMapInReference.top)    / downscaleRatioHeight);
        ROIWrtCurrent.width  = static_cast<UINT32>
                               (static_cast<FLOAT>(ROIWrtCurrentFrameMapInReference.width)  / downscaleRatioWidth);
        ROIWrtCurrent.height = static_cast<UINT32>
                               (static_cast<FLOAT>(ROIWrtCurrentFrameMapInReference.height) / downscaleRatioHeight);
    }

    return ROIWrtCurrent;
}

CAMX_NAMESPACE_END
