////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxfdutils.h
/// @brief FD utility class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXFDUTILS_H
#define CAMXFDUTILS_H

#include <stdio.h>
#include <stdarg.h>

#include "camxfdconfig.h"
#include "g_camxsettings.h"
#include "chifdproperty.h"
#include "fdsetmanager.h"
#include "chieisdefs.h"

CAMX_NAMESPACE_BEGIN

/// @brief Forward declare structure describing current face stabilization data
struct StabilizationData;

/// @brief Forward declare node class
class Node;

/// @brief Describes processing type
enum FDProcessingType
{
    InvalidFDType    =-1,    ///< Invalid
    InternalFDType   = 0,    ///< HW-specific FD processing
    SWFDType         = 1,    ///< SW-specific FD processing
    DLFDType         = 2     ///< DL-specific FD processing
};

/// @brief This enum indicates the type of source for FD tuning configuration
enum FDConfigSelector
{
    FDSelectorDefault,      ///< Default config that driver populates
    FDSelectorPreview,      ///< Configuration in preview use case
    FDSelectorVideo,        ///< Configuration in video use case
    FDSelectorTurbo,        ///< Configuration in turbo mode
};

/// @brief This enum indicates the sensor flip type
enum FDFlipType
{
    FlipNone,               ///< No Flip
    FlipHorizontal,         ///< Horizontal flip
    FlipVertical,           ///< Vertical flip
    FlipHorizontalVertical, ///< Horizontal + Vertical flip
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the FD utility functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FDUtils
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FDTypeEnumToString
    ///
    /// @brief  Converts a FDTypeEnum to a string value for printing
    ///
    /// @param  val The enum value to print
    ///
    /// @return fdtype corresponding to enum.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const CHAR* FDTypeEnumToString(
        FDProcessingType val);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertToStabilizationData
    ///
    /// @brief  Convert the FD result data to stabilization structures
    ///
    /// @param  pResults           Pointer to FD result data
    /// @param  pStabilizationData Pointer to stabilization data structure
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ConvertToStabilizationData(
        FDResults*         pResults,
        StabilizationData* pStabilizationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertFromStabilizationData
    ///
    /// @brief  Convert the stabilization data structures to FD result data
    ///
    /// @param  pStabilizationData Pointer to stabilization data structure
    /// @param  pResults           Pointer to FD result data
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ConvertFromStabilizationData(
        StabilizationData* pStabilizationData,
        FDResults*         pResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SortFaces
    ///
    /// @brief  Sorts the input faces from largest to smallest
    ///
    /// @param  pFaceResults Pointer to FDResults of faces to sort
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SortFaces(
        FDResults* pFaceResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFDConfig
    ///
    /// @brief  Gets the face detection configuration
    ///
    /// @param  pNode       Pointer to node instance
    /// @param  source      Source from where to fetch FD tuning configuration
    /// @param  hwHybrid    Whether need to fetch tuning data for Hw hybrid or sw
    /// @param  fdtype      FDtype to choose fd config
    /// @param  frontCamera Whether need to fetch tuning data for front or back camera
    /// @param  selector    Use case selector to choose FD config
    /// @param  pFaceConfig Pointer to FDResults of faces to sort
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetFDConfig(
        Node*               pNode,
        FDConfigSourceType  source,
        BOOL                hwHybrid,
        FDProcessingType    fdtype,
        BOOL                frontCamera,
        FDConfigSelector    selector,
        FDConfig*           pFaceConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFaceSize
    ///
    /// @brief  Gets the face detection configuration
    ///
    /// @param  pFaceSizeInfo Face size config info
    /// @param  minSize       Minimum allowed face size
    /// @param  dimension     Dimension of frame
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetFaceSize(
        const FDFaceSize* pFaceSizeInfo,
        UINT32            minSize,
        UINT32            dimension);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadFDTuningData
    ///
    /// @brief  Load the FD tuning data
    ///
    /// @param  FaceDetectionCtrlType  Pointer to fd tuning data struct
    /// @param  frontCamera Whether need to fetch tuning data for front or back camera
    /// @param  FDConfigSelector       config selector
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult LoadFDTuningData(
        camxfdconfig::FaceDetectionCtrlType*    pFdCtrlType,
        BOOL                                    frontCamera,
        FDConfigSelector                        selector);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOrientationFromAccel
    ///
    /// @brief  Get orientation angle info from Accelerometer/Gravity values
    ///
    /// @param  pAccelValues        Pointer to input accelerometer values
    /// @param  pOrientationAngle   Pointer to save orientation angle
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetOrientationFromAccel(
        FLOAT* pAccelValues,
        INT32* pOrientationAngle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AlignAccelToCamera
    ///
    /// @brief  Align accelerometer values to camera axis
    ///
    /// @param  pAccelValues    Pointer to input accelerometer values
    /// @param  sensoMountAngle Sensor mount angle
    /// @param  cameraPosition  Camera position
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult AlignAccelToCamera(
        FLOAT*  pAccelValues,
        UINT32  sensoMountAngle,
        UINT32  cameraPosition);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ApplyFlipOnDeviceOrientation
    ///
    /// @brief  Apply sensor flip and determine final device orientation
    ///
    /// @param  flipType            Sensor flip mask
    /// @param  pOrientationAngle   Pointer to save orientation angle
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ApplyFlipOnDeviceOrientation(
        FDFlipType  flipType,
        INT32*      pOrientationAngle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertFDPointsFromCurrentToReference
    ///
    /// @brief  Convert points w.r.t current frame to values w.r.t reference Frame
    ///
    /// @param  pReferenceFrameDimension     Dimensions of reference frame
    /// @param  pCurrentFrameDimension       Dimensions of current frame
    /// @param  pCurrentFrameMapWrtReference Current frame map info w.r.t reference frame
    /// @param  pCurrentPoints               Points w.r.t current frame
    /// @param  pConvertedPoints             Points w.r.t reference frame (output)
    /// @param  numPoints                    Number of points to convert
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ConvertFDPointsFromCurrentToReference(
        const CHIDimension*  pReferenceFrameDimension,
        const CHIDimension*  pCurrentFrameDimension,
        const CHIRectangle*  pCurrentFrameMapWrtReference,
        const FDPoint*       pCurrentPoints,
        FDPoint*             pConvertedPoints,
        const UINT32         numPoints);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertROIForImageStabilization
    ///
    /// @brief  Convert face ROI based on margin ratio of image stabilization
    ///
    /// @param  pFrameDimension     Dimensions of reference frame
    /// @param  pMarginRatio        Margin ratio of image stabilization
    /// @param  pPreConversionROI   ROI before conversion
    /// @param  pPostConversionROI  ROI after conversion (output)
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ConvertROIForImageStabilization(
        const CHIDimension*  pFrameDimension,
        const MarginRequest* pMarginRatio,
        const CHIRectangle*  pPreConversionROI,
        CHIRectangle*        pPostConversionROI);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertFDPointsForImageStabilization
    ///
    /// @brief  Convert points based on margin ratio of image stabilization
    ///
    /// @param  pFrameDimension           Dimensions of reference frame
    /// @param  pMarginRatio              Margin ratio of image stabilization
    /// @param  pPreConversionPoints      Points before conversion
    /// @param  pPostConversionPoints     Points after conversion (output)
    /// @param  numPoints                 Number of points to convert
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ConvertFDPointsForImageStabilization(
        const CHIDimension*  pFrameDimension,
        const MarginRequest* pMarginRatio,
        const FDPoint*       pPreConversionPoints,
        FDPoint*             pPostConversionPoints,
        const UINT32         numPoints);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FDUtils
    ///
    /// @brief Default constructor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDefaultFDConfig
    ///
    /// @brief  Gets the face detection default configuration
    ///
    /// @param  hwHybrid    Whether need to fetch tuning data for Hw hybrid or sw
    /// @param  fdtype      FDtype to choose fd config
    /// @param  frontCamera Whether need to fetch tuning data for front or back camera
    /// @param  selector    Use case selector to choose FD config
    /// @param  pFaceConfig Pointer to FDResults of faces to sort
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetDefaultFDConfig(
        BOOL                hwHybrid,
        FDProcessingType    fdtype,
        BOOL                frontCamera,
        FDConfigSelector    selector,
        FDConfig*           pFaceConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFDConfigFromVendorTag
    ///
    /// @brief  Gets the face detection configuration from vendor tag
    ///
    /// @param  pNode       Pointer to node instance
    /// @param  hwHybrid    Whether need to fetch tuning data for Hw hybrid or sw
    /// @param  frontCamera Whether need to fetch tuning data for front or back camera
    /// @param  selector    Use case selector to choose FD config
    /// @param  pFaceConfig Pointer to FDResults of faces to sort
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetFDConfigFromVendorTag(
        Node*               pNode,
        BOOL                hwHybrid,
        BOOL                frontCamera,
        FDConfigSelector    selector,
        FDConfig*           pFaceConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFDConfigFromBinary
    ///
    /// @brief  Gets the face detection configuration from binary
    ///
    /// @param  hwHybrid    Whether need to fetch tuning data for Hw hybrid or sw
    /// @param  frontCamera Whether need to fetch tuning data for front or back camera
    /// @param  selector    Use case selector to choose FD config
    /// @param  pFaceConfig Pointer to FDResults of faces to sort
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetFDConfigFromBinary(
        BOOL                hwHybrid,
        BOOL                frontCamera,
        FDConfigSelector    selector,
        FDConfig*           pFaceConfig);

    FDUtils() = default;
    FDUtils(const FDUtils&) = delete;
    FDUtils& operator=(const FDUtils&) = delete;
};

CAMX_NAMESPACE_END

#endif // CAMXFDUTILS_H
