////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   camxnodeutils.h
///
/// @brief  Utility functions for camera image sensor and submodules.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXNODEUTILS_H
#define CAMXNODEUTILS_H

#include "camxutils.h"
#include "chivendortag.h"


CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Static utility class for Node.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class NodeUtils
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertInputCropToPortCrop
    ///
    /// @brief  Convert input crop info to port dimention specific crop
    ///
    /// @param  width                   Width of parent output port
    /// @param  height                  Height of parent output port
    /// @param  pCropInfo               Input crop info
    /// @param  pConvertedCropInfo      Converted output crop info
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ConvertInputCropToPortCrop(
        UINT                width,
        UINT                height,
        StreamCropInfo*     pCropInfo,
        StreamCropInfo*     pConvertedCropInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckForROIBound
    ///
    /// @brief  Check and adjust top, left, width, and height values so that ROI falls within frame.
    ///
    /// @param  pCurrentROI    ROI rectangle
    /// @param  width          Width of the frame
    /// @param  height         Height of the frame
    /// @param  pNodeIdString  Pointer to node identifier string
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CheckForROIBound(
        CHIRectangle*   pCurrentROI,
        INT32           width,
        INT32           height,
        const CHAR*     pNodeIdString);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFovWithActiveArray
    ///
    /// @brief  Give FOV infomation of current node to ActiveArray
    ///
    /// @param  parentFOVtoActiveArray       Parentnode crop to active array
    /// @param  currentNodeAppliedCrop       What is the crop applied by the current node
    /// @param  currentNodeInputWidth        current node width
    /// @param  currentNodeInputHeight       current node height
    /// @param  pCurrentNodeFOVtoActiveArray current node crop/fov w.r.t active array
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetFovWithActiveArray(
        CHIRectangle       parentFOVtoActiveArray,
        CHIRectangle       currentNodeAppliedCrop,
        UINT32             currentNodeInputWidth,
        UINT32             currentNodeInputHeight,
        CHIRectangle*      pCurrentNodeFOVtoActiveArray);

private:

    NodeUtils()                            = delete;                            ///< Disallow the copy constructor
    NodeUtils(const NodeUtils&)            = delete;                            ///< Disallow the copy constructor
    NodeUtils& operator=(const NodeUtils&) = delete;                            ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIMAGESENSORUTILS_H
