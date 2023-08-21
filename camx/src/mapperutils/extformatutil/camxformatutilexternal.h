////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxformatutilexternal.h
/// @brief CamX Image Format Utility for External Components
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXFORMATUTILEXTERNAL_H
#define CAMXFORMATUTILEXTERNAL_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#pragma pack(push, 8)


// NOWHINE FILE GR017: Used intrinsic data types as this header is exposed to non-camera components
// NOWHINE FILE NC010: Underscores added for names to support C clients
// NOWHINE FILE CF003: Whiner wrongly interprets the return alignment due to in/out directions
// NOWHINE FILE GR016: Typedefs used for typecasting to pixel formats
// NOWHINE FILE DC002: Pointer direction added to parameter

#define CAMXFORMAT_VISIBILITY_PUBLIC __attribute__ ((visibility ("default")))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Result Codes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum : int
{
    CamxFormatResultSuccess         = 0,  ///< Operation was successful
    CamxFormatResultEFailed         = 1,  ///< Operation encountered unspecified error
    CamxFormatResultEUnsupported    = 2,  ///< Operation is not supported
    CamxFormatResultEInvalidState   = 3,  ///< Invalid state
    CamxFormatResultEInvalidArg     = 4,  ///< Invalid argument
    CamxFormatResultEInvalidPointer = 5,  ///< Invalid memory pointer
    CamxFormatResultENoSuch         = 6,  ///< No such item exists or is valid
    CamxFormatResultEOutOfBounds    = 7,  ///< Out of bounds
    CamxFormatResultENoMemory       = 8,  ///< Out of memory
    CamxFormatResultENoMore         = 10, ///< No more items available
    CamxFormatResultENeedMore       = 11, ///< Operation requires more
    CamxFormatResultEPrivLevel      = 13, ///< Privileges are insufficient for requested operation
    CamxFormatResultENotImplemented = 26, ///< Function or method is not implemented
} CamxFormatResult;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Plane types supported by the camera format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum
{
    CAMERA_PLANE_TYPE_RAW,     ///< RAW plane for Single planar formats including UYVY and thier variants
    CAMERA_PLANE_TYPE_Y,       ///< Y only
    CAMERA_PLANE_TYPE_UV,      ///< UV, VU, Cb, Cr planes for YUV variants
    CAMERA_PLANE_TYPE_U,       ///< U plane only
    CAMERA_PLANE_TYPE_V,       ///< V plane only
    CAMERA_PLANE_TYPE_META_Y,  ///< Metadata plane for Y
    CAMERA_PLANE_TYPE_META_VU, ///< Metadata plane for VU and UV
} CamxPlaneType;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  External camera pixel formats that are allocated by gralloc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum : unsigned int
{
    CAMERA_PIXEL_FORMAT_NV21_ZSL            = 0x113,        ///< NV21 format with alignment requirements for
                                                            ///   YUV reprocessing
    CAMERA_PIXEL_FORMAT_YUV_FLEX            = 0x125,        ///< YUV format with fliexible alignment defined by individual
                                                            ///   APIs
    CAMERA_PIXEL_FORMAT_UBWC_FLEX           = 0x126,        ///< YUV format with fliexible alignment defined by individual
                                                            ///   APIs
    CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX    = 0x127,        ///< YUV format with fliexible alignment defined by individual
                                                            ///   APIs
    CAMERA_PIXEL_FORMAT_NV12_VENUS          = 0x7FA30C04,   ///< NV12 video format
    CAMERA_PIXEL_FORMAT_NV12_HEIF           = 0x00000116,   ///< HEIF video YUV420 format
    CAMERA_PIXEL_FORMAT_YCbCr_420_SP_UBWC   = 0x7FA30C06,   ///< 8 bit YUV 420 semi-planar UBWC format
    CAMERA_PIXEL_FORMAT_YCbCr_420_TP10_UBWC = 0x7FA30C09,   ///< TP10 YUV 420 semi-planar UBWC format
    CAMERA_PIXEL_FORMAT_YCbCr_420_P010_UBWC = 0x124,        ///< P010 YUV 420 semi-planar UBWC format
    CAMERA_PIXEL_FORMAT_RAW_OPAQUE          = 0x24,         ///< Opaque RAW format
    CAMERA_PIXEL_FORMAT_RAW10               = 0x25,         ///< RAW10 format
    CAMERA_PIXEL_FORMAT_RAW12               = 0x26,         ///< RAW12 format
} CamxPixelFormat;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constant
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const int CamxFormatUtilMaxFlexibleFormats = 10;
static const int CamxFormatUtilMaxNumPlanes       = 4;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Structure for holding the list of flexible formats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    int             size;                                                  ///< Size of this structure
    unsigned int    count;                                                 ///< Count of the array
    CamxPixelFormat pixelFormat[CamxFormatUtilMaxFlexibleFormats];         ///< Array of pixel formats
    int             strideAlignment[CamxFormatUtilMaxFlexibleFormats];     ///< Array of stride alignments for each format
    int             scanlineAlignment[CamxFormatUtilMaxFlexibleFormats];   ///< Array of scanline alignments for each format
} CamxFlexFormatInfo;

/// @brief  Structure for holding plane layout info
typedef struct
{
    int          planeSize;      ///< plane Size
    int          stride;         ///< stride in bytes
    int          scanline;       ///< scanline
    unsigned int alignment;      ///< alignment value
} CamxPlaneLayoutInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////  APIs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetStrideInBytes
///
/// @brief  Function to get the stride of the plane given the pixel format
///
/// @param  [in]   pixelFormat         Gralloc pixel format
/// @param  [in]   planeType           Image plane type
/// @param  [in]   width               Width of the image
/// @param  [out]  pStride             Stride of the plane
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetStrideInBytes(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int             width,
    int*            pStride);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetStrideInPixels
///
/// @brief  Function to get the stride of the plane in pixels, given the pixel format
///
/// @param  [in]   pixelFormat         Gralloc pixel format
/// @param  [in]   planeType           Image plane type
/// @param  [in]   width               Width of the image
/// @param  [out]  pStride             Stride of the plane in pixels
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetStrideInPixels(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int             width,
    float*          pStride);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetScanline
///
/// @brief  Function to get the scanline of the plane given the pixel format
///
/// @param  [in]   pixelFormat         Gralloc pixel format
/// @param  [in]   planeType           Image plane type
/// @param  [in]   height              Height of the image
/// @param  [out]  pScanline           Scanline of the plane
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetScanline(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int             height,
    int*            pScanline);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneSize
///
/// @brief  Function to get the aligned plane size given the pixel format
///
/// @param  [in]   pixelFormat            Gralloc pixel format
/// @param  [in]   planeType              Image plane type
/// @param  [in]   width                  Width of the image
/// @param  [in]   height                 Height of the image
/// @param  [out]  pAlignedSize           Aligned plane size
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetPlaneSize(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int             width,
    int             height,
    unsigned int*   pAlignedSize);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetBufferSize
///
/// @brief  Function to get the buffer size given the pixel format, width and height
///
/// @param  [in]   pixelFormat          Gralloc pixel format
/// @param  [in]   width                Width of the image
/// @param  [in]   height               Height of the image
/// @param  [out]  pBufferSize          Buffer size of the image
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetBufferSize(
    CamxPixelFormat pixelFormat,
    int             width,
    int             height,
    unsigned int*   pBufferSize);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetUBWCInfo
///
/// @brief  Function to get UBWC formats and other information supported by Camera formats
///
/// @param  [in]   pixelFormat        Gralloc UBWC pixel format
/// @param  [out]  pIsSupported       Returns if UBWC is supported by the Camera output
/// @param  [out]  pIsPI              Returns if perceptually indistinguishable compression is supported.
///                                   Only valid if UBWC pixel format itself is supported
/// @param  [out]  pVersion           UBWC version supported for the format.
///                                   Valid values are 2,3,4.
///                                   Only valid if UBWC pixel format is supported
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetUBWCInfo(
    CamxPixelFormat pixelFormat,
    bool*           pIsSupported,
    bool*           pIsPI,
    int*            pVersion);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneAlignment
///
/// @brief  Function to get the plane alignment in bytes
///
/// @param  [in]   pixelFormat            Gralloc pixel format
/// @param  [in]   planeType              Image plane type
/// @param  [out]  pAlignment             Plane alignment in bytes
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetPlaneAlignment(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    unsigned int*   pAlignment);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneOffset
///
/// @brief  Function to get the plane offset given the pixel format and plane type
///
/// @param  [in]   pixelFormat         Gralloc pixel format
/// @param  [in]   planeType           Image plane type
/// @param  [out]  pOffset             Positive offset within the plane where valid data starts
/// @param  [in]   width               Image format width
/// @param  [in]   height              Image format height
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
/// NOWHINE FILE CP021: Default arguments are used (This is done to avoid mutual dependency on Gralloc)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetPlaneOffset(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int*            pOffset,
    int             width  = 0,
    int             height = 0);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneTypes
///
/// @brief  Function to get plane types for a particular pixel format
///
/// @param  [in]   pixelFormat         Gralloc pixel format
/// @param  [out]  pPlanetypesArray    Array of plane types. The caller must allocate the memory and pass to this function.
///                                    Array size must be equal to CamxFormatUtilMaxNumPlaness
/// @param  [out]  pPlanetypeCount     Count of the plane types for the specified format.
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetPlaneTypes(
    CamxPixelFormat pixelFormat,
    CamxPlaneType*  pPlanetypesArray,
    int*            pPlanetypeCount);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_IsPerPlaneFdNeeded
///
/// @brief  Function to check if per-plane fd is needed for the given format
///
/// @param  [in]   pixelFormat         Gralloc pixel format
/// @param  [out]  pIsPerPlaneFdNeeded Check if per-plane Fd is needed
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_IsPerPlaneFdNeeded(
    CamxPixelFormat pixelFormat,
    bool*           pIsPerPlaneFdNeeded);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetBpp
///
/// @brief  Function to get the bits-per-pixel for a given format
///
/// @param  [in]   pixelFormat         Gralloc pixel format
/// @param  [out]  pBpp                Bits per-pixel
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetBpp(
    CamxPixelFormat pixelFormat,
    int*            pBpp);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPerPlaneBpp
///
/// @brief  Function to get the per-plane bits-per-pixel for a given format
///
/// @param  [in]   pixelFormat         Gralloc pixel format
/// @param  [in]   planeType           Image plane type
/// @param  [out]  pBpp                Bits per-pixel
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetPerPlaneBpp(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int*            pBpp);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetSubsamplingFactor
///
/// @brief  Function to get the subsampling factor the color components
///
/// @param  [in]   pixelFormat         Gralloc pixel format
/// @param  [in]   planeType           Image plane type
/// @param  [in]   isHorizontal        Flag to indicate whether its horizontal or vertical
/// @param  [out]  pSubsamplingFactor  Subsampling Factor
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetSubsamplingFactor(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    bool            isHorizontal,
    int*            pSubsamplingFactor);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneCount
///
/// @brief  Function to get the plane count
///
/// @param  [in]   pixelFormat      Gralloc pixel format
/// @param  [out]  pPlaneCount      Pointer to the plane count
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetPlaneCount(
    CamxPixelFormat pixelFormat,
    int*            pPlaneCount);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPixelIncrement
///
/// @brief  Function to get the pixel increment
///
/// @param  [in]   pixelFormat      Gralloc pixel format
/// @param  [in]   planeType        Image plane type
/// @param  [out]  pPixelIncrement  Pointer to the pixel increment
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetPixelIncrement(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int*            pPixelIncrement);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneStartAddressAlignment
///
/// @brief  Function to get the per-plane start address alignment given the pixel format and plane type
///
/// @param  [in]   pixelFormat         Gralloc pixel format
/// @param  [in]   planeType           Image plane type
/// @param  [out]  pAlignment          Start address alignment in bytes
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetPlaneStartAddressAlignment(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int*            pAlignment);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneLayoutInfo
///
/// @brief  Function to get plane layout info through one API
///
/// @param  [in]   pixelFormat         Gralloc pixel format
/// @param  [in]   planeType           Image plane type
/// @param  [in]   width               Width of the image
/// @param  [in]   height              Height of the image
/// @param  [out]  pPlaneInfo          Plane Info structure pointer
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetPlaneLayoutInfo(
    CamxPixelFormat       pixelFormat,
    CamxPlaneType         planeType,
    int                   width,
    int                   height,
    CamxPlaneLayoutInfo*  pPlaneInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetFlexibleYUVFormats
///
/// @brief  Function to get the stride/scanline alignment requirements for flexible formats.
///
/// @param  [in/out]   pFlexFormatInfo    CamxFlexFormatInfo structure pointer. The memory must be allocated by the caller.
///                                       size field must be filled by the caller and size must be set.
///                                       Rest of the fields will be updated on
///                                       successful completion of this function
///
/// @return CamxFormatResultSuccess if successful, Errors specified by CamxFormatResult otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetFlexibleYUVFormats(
    CamxFlexFormatInfo* pFlexFormatInfo);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CAMXFORMATUTILEXTERNAL_H
