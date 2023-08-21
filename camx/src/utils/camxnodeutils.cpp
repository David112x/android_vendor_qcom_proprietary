////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   camxnodeutils.cpp
/// @brief  Utility functions for Camx Node.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdebugprint.h"
#include "camxnodeutils.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NodeUtils::ConvertInputCropToPortCrop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NodeUtils::ConvertInputCropToPortCrop(
    UINT                width,
    UINT                height,
    StreamCropInfo*     pCropInfo,
    StreamCropInfo*     pConvertedCropInfo)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCropInfo) && (NULL != pConvertedCropInfo))
    {
        if (width == pCropInfo->frameDimension.width && height == pCropInfo->frameDimension.height)
        {
            // No conversion is required as port dimesion matches input crop info
            *pConvertedCropInfo = *pCropInfo;
        }
        else
        {
            // As output port dimension is different, convert crop
            if ((width != 0) && (height != 0))
            {
                FLOAT widthRatio                = pCropInfo->frameDimension.width / static_cast<FLOAT>(width);
                FLOAT heightRatio               = pCropInfo->frameDimension.height / static_cast<FLOAT>(height);

                pConvertedCropInfo->crop.left   = static_cast<INT32>((pCropInfo->crop.left * widthRatio));
                pConvertedCropInfo->crop.top    = static_cast<INT32>((pCropInfo->crop.top * heightRatio));
                pConvertedCropInfo->crop.width  = static_cast<INT32>((pCropInfo->crop.width * widthRatio));
                pConvertedCropInfo->crop.height = static_cast<INT32>((pCropInfo->crop.height * heightRatio));

                pConvertedCropInfo->fov         = pCropInfo->fov;

                pConvertedCropInfo->frameDimension.width  = width;
                pConvertedCropInfo->frameDimension.height = height;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "ConvertInputCropToPortCrop width or height is 0");
                result = CamxResultEInvalidArg;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "ConvertInputCropToPortCrop NULL pCropInfo or pConvertedCropInfo");
        result = CamxResultEInvalidPointer;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeUtils::CheckForROIBound
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID NodeUtils::CheckForROIBound(
    CHIRectangle*   pCurrentROI,
    INT32           width,
    INT32           height,
    const CHAR*     pNodeIdString)
{
    if ((NULL != pCurrentROI) && (NULL != pNodeIdString))
    {
        if ((0 < width) && (0 < height))
        {
            if (pCurrentROI->left < 0)
            {
                pCurrentROI->width = pCurrentROI->left + pCurrentROI->width;
                pCurrentROI->left  = 0;
            }

            if (pCurrentROI->top < 0)
            {
                pCurrentROI->height = pCurrentROI->top + pCurrentROI->height;
                pCurrentROI->top    = 0;
            }

            if ((pCurrentROI->left + pCurrentROI->width) > width)
            {
                pCurrentROI->width = width - pCurrentROI->left;
            }

            if ((pCurrentROI->top + pCurrentROI->height) > height)
            {
                pCurrentROI->height = height - pCurrentROI->top;
            }

            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node::%s current frame size %dx%d, ROI [%d, %d, %d, %d]",
                pNodeIdString, width, height,
                pCurrentROI->left, pCurrentROI->top, pCurrentROI->width, pCurrentROI->height);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Invalid frame size %dx%d", pNodeIdString, width, height);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid ptr nodeId string %p , roi %p", pNodeIdString, pCurrentROI);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeUtils::GetFovWithActiveArray
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID NodeUtils::GetFovWithActiveArray(
    CHIRectangle   parentFOVtoActiveArray,
    CHIRectangle   currentNodeAppliedCrop,
    UINT32         currentNodeInputWidth,
    UINT32         currentNodeInputHeight,
    CHIRectangle*  pCurrentNodeFOVtoActiveArray)
{
    FLOAT scaleRatioWidth  = static_cast<FLOAT>(static_cast<FLOAT>(parentFOVtoActiveArray.width) /
                                                static_cast<FLOAT>(currentNodeInputWidth));
    FLOAT scaleRatioHeight = static_cast<FLOAT>(static_cast<FLOAT>(parentFOVtoActiveArray.height) /
                                                static_cast<FLOAT>(currentNodeInputHeight));

    pCurrentNodeFOVtoActiveArray->left   = parentFOVtoActiveArray.left + static_cast<UINT32>
                                           (static_cast<FLOAT>(currentNodeAppliedCrop.left) * scaleRatioWidth);
    pCurrentNodeFOVtoActiveArray->top    = parentFOVtoActiveArray.top + static_cast<UINT32>
                                           (static_cast<FLOAT>(currentNodeAppliedCrop.top) * scaleRatioHeight);
    pCurrentNodeFOVtoActiveArray->width  = static_cast<UINT32>
                                           (static_cast<FLOAT>(currentNodeAppliedCrop.width) * scaleRatioWidth);
    pCurrentNodeFOVtoActiveArray->height = static_cast<UINT32>
                                           (static_cast<FLOAT>(currentNodeAppliedCrop.height) * scaleRatioHeight);
}
CAMX_NAMESPACE_END
