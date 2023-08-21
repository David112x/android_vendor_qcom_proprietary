////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxformats.h
///
/// @brief Defines the format parameters for images.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXFORMATS_H
#define CAMXFORMATS_H

// Camera dependencies
#include "camxtypes.h"
#include "camxdefs.h"
#include "ipdefs.h"
CAMX_NAMESPACE_BEGIN

/// Maximum number of planes for all formats.
static const SIZE_T FormatsMaxPlanes = 4;

/// Maximum number of plane layouts for a given format.
const INT32 MaxPlaneLayOut = 4;

/// @brief Jpeg max size calculations
static const INT32 EXIFSize = 65536;

static const INT32 JpegPadding = 102400;

static const FLOAT JpegMult = 1.5;


/// @brief This enumerates pixel formats.
enum class Format
{
    Jpeg           = 0,  ///< JPEG format.
    Y8             = 1,  ///< Luma only, 8 bits per pixel.
    Y16            = 2,  ///< Luma only, 16 bits per pixel.
    YUV420NV12     = 3,  ///< YUV 420 format as described by the NV12 fourcc.
    YUV420NV21     = 4,  ///< YUV 420 format as described by the NV21 fourcc.
    YUV422NV16     = 5,  ///< YUV 422 format as described by the NV16 fourcc
    Blob           = 6,  ///< Any non image data
    RawYUV8BIT     = 7,  ///< Packed YUV/YVU raw format. 16 bpp: 8 bits Y and 8 bits UV.
                         ///< U and V are interleaved as YUYV or YVYV.
    RawPrivate     = 8,  ///< Private RAW formats where data is packed into 64bit word. P0 is stored at LSB.
                         ///  8BPP:  64-bit word contains 8 pixels p0-p7, p0 is stored at LSB.
                         ///  10BPP: 64-bit word contains 6 pixels p0-p5, most significant 4 bits are set to 0.
                         ///  12BPP: 64-bit word contains 5 pixels p0-p4, most significant 4 bits are set to 0.
                         ///  14BPP: 64-bit word contains 4 pixels p0-p3, most significant 8 bits are set to 0.
    RawMIPI        = 9,  ///< MIPI RAW formats based on MIPI CSI-2 specification.
                         ///  8BPP: Each pixel occupies one bytes, starting at LSB. Output width of image has no restrictions.
                         ///  10BPP: 4 pixels are held in every 5 bytes. Output width of image must be a multiple of 4 pixels.
                         ///  12BPP: 2 pixels are held in every 3 bytes. Output width of image must be a multiple of 2 pixels.
                         ///  14BPP: 4 pixels are held in every 7 bytes. Output width of image must be a multiple of 4 pixels.
    RawPlain16     = 10, ///< Plain16 RAW format. Single pixel is packed into two bytes, little endian format. Not all bits may
                         ///  be used as RAW data is generally 8, 10, or 12 bits per pixel. Lower order bits are filled first.
    RawMeta8BIT    = 11, ///< Generic 8-bit raw meta data for internal camera usage.
    UBWCTP10       = 12, ///< UBWC TP10 format (as per UBWC2.0 design specification)
    UBWCNV12       = 13, ///< UBWC NV12 format (as per UBWC2.0 design specification)
    UBWCNV124R     = 14, ///< UBWC NV12-4R format (as per UBWC2.0 design specification)
    YUV420NV12TP10 = 15, ///< YUV 420 format 10bits per comp tight packed format.
    YUV420NV21TP10 = 16, ///< YUV 420 format 10bits per comp tight packed format.
    YUV422NV16TP10 = 17, ///< YUV 422 format 10bits per comp tight packed format.
    PD10           = 18, ///< PD10 format
    RawMIPI8       = 19, ///< 8BPP: Each pixel occupies one bytes, starting at LSB. Output width of image has no restrictions.
    P010           = 22, ///< P010 format.
    RawPlain64     = 23, ///< RawPlain64 format
    UBWCP010       = 24, ///< UBWC P010 format (as per UBWC2.0 design specification)
    UBWCNV12FLEX   = 25  ///< UBWC NV12 FLEX format (as per UBWC2.0 design specification)
};


/// @brief This enumerates degrees of rotation in a clockwise direction. The specific variable or struct member must declare the
/// semantics of the rotation (e.g. the image HAS BEEN rotated or MUST BE rotated).
enum class Rotation
{
    CW0Degrees,     ///< Zero degree rotation.
    CW90Degrees,    ///< 90 degree clockwise rotation.
    CW180Degrees,   ///< 180 degree clockwise rotation.
    CW270Degrees    ///< 270 degree clockwise rotation.
};

/// @brief Enumeration of the color filter pattern for RAW outputs
enum class ColorFilterPattern
{
    Y,      ///< Monochrome pixel pattern.
    YUYV,   ///< YUYV pixel pattern.
    YVYU,   ///< YVYU pixel pattern.
    UYVY,   ///< UYVY pixel pattern.
    VYUY,   ///< VYUY pixel pattern.
    RGGB,   ///< RGGB pixel pattern.
    GRBG,   ///< GRBG pixel pattern.
    GBRG,   ///< GBRG pixel pattern.
    BGGR,   ///< BGGR pixel pattern.
    RGB     ///< RGB pixel pattern.
};

/// @brief This enumerates color space specifications
enum ColorSpace
{
    Unknown            = 0x0000,                       ///< Default-assumption data space
    Arbitrary          = 0x0001,                       ///< Arbitrary dataspace
    StandardShift      = 16,                           ///< Standard shift
    TransferShift      = 22,                           ///< Transfer shift
    RangeShift         = 27,                           ///< Range shift
    StandardBT601_625  = 2 << StandardShift,           ///< This adjusts the luminance interpretation
                                                       ///   for RGB conversion from the one purely determined
                                                       ///   by the primaries to minimize the color shift into
                                                       ///   RGB space that uses BT.709 primaries.
    Smpte170M          = 3 << TransferShift,           ///< BT.601 525, BT.601 625, BT.709, BT.2020
    St2084             = 7 << TransferShift,           ///< ARIB STD-B67 Hybrid Log Gamma, refer graphics.h
    RangeFull          = 1 << RangeShift,              ///< Full range uses all values for Y, Cb and Cr from
                                                       ///   0 to 2 ^ b - 1, where b is the bit depth of the
                                                       ///   color format.
    JFIF               = 0x0101,                       ///< JPEG File Interchange Format(JFIF). Same model as
                                                       ///   BT.601-625, but all YCbCr values range from 0 to 255.
    V0JFIF             = StandardBT601_625 |
                         Smpte170M |
                         RangeFull,                    ///< JPEG File Interchange Format(JFIF). Same model as
                                                       ///   BT.601-625, but all YCbCr values range from
                                                       ///   0 to 255.
    BT601Full          = 0x0101,                       ///< ITU-R Recommendation BT.601
    BT601625           = 0x0102,                       ///< ITU-R Recommendation BT.601 - 625line
    BT601525           = 0x0103,                       ///< ITU-R Recommendation BT.601 - 525line
    BT709              = 0x0104,                       ///< ITU-R Recommendation BT.709
    SRGBLinear         = 0x0200,                       ///< The red, green, and blue components are stored in sRGB
                                                       ///   space, but are linear and not gamma-encoded.
    SRGB               = 0x0201,                       ///< The red, green and blue components are stored in sRGB
                                                       ///   space and converted to linear space
    Depth              = 0x1000,                       ///< The buffer contains depth ranging measurements from a
                                                       ///   Depth camera.
    BT2020             =  6 << StandardShift,          ///< BT.2020
    BT2020_PQ          = (BT2020 |                     ///< BT.2020-PQ
                          St2084 |
                          RangeFull)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This structure defines the raw format.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct RawFormat
{
    UINT32              bitsPerPixel;       ///< Bits per pixel.
    UINT32              stride;             ///< Stride in bytes.
    UINT32              sliceHeight;        ///< The number of lines in the plane which can be equal to or larger than actual
                                            ///  frame height.
    ColorFilterPattern  colorFilterPattern; ///< Color filter pattern of the RAW format.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This structure defines the YUV formats.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct YUVFormat
{
    UINT32 width;           ///< Width of the YUV plane in pixels.
                            ///  Tile aligned width in bytes for UBWC
    UINT32 height;          ///< Height of the YUV plane in pixels.
    UINT32 planeStride;     ///< The number of bytes between the first byte of two sequential lines on plane 1. It may be
                            ///  greater than nWidth * nDepth / 8 if the line includes padding.
                            ///  Macro-tile width aligned for UBWC
    UINT32 sliceHeight;     ///< The number of lines in the plane which can be equal to or larger than actual frame height.
                            ///  Tile height aligned for UBWC

    UINT32 metadataStride;  ///< Aligned meta data plane stride in bytes, used for UBWC formats
    UINT32 metadataHeight;  ///< Aligned meta data plane height in bytes, used for UBWC formats
    UINT32 metadataSize;    ///< Aligned metadata plane size in bytes, used for UBWC formats
    UINT32 pixelPlaneSize;  ///< Aligned pixel plane size in bytes, calculated once for UBWC formats
                            ///< and stored thereafter, since the calculations are expensive
    SIZE_T planeSize;       ///< Size in pixels for this plane.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This structure defines the JPEG format.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct JPEGFormat
{
    UINT32              maxJPEGSizeInBytes;       ///< Size of JPEG for max resolution in Bytes.
    UINT32              debugDataSize;            ///< DebugDataSize that need to be added in JpegSize
};

/// @brief UBWC version
enum UBWCVersion
{
    UBWCVersion2 = 0,    ///< UBWC version 2
    UBWCVersion3 = 1,    ///< UBWC version 3
    UBWCVersion4 = 2,    ///< UBWC version 4
    UBWCVersionMax       ///< Maximum value for UBWC version
};

/// @brief UBWC version mask
enum UBWCVersionMask
{
    UBWCVersion2Mask = 1 << UBWCVersion2,  ///< UBWC version Mask 2
    UBWCVersion3Mask = 1 << UBWCVersion3,  ///< UBWC version Mask 3
    UBWCVersion4Mask = 1 << UBWCVersion4,  ///< UBWC version Mask 4
};
const UINT32 UBWCVersionMask = UBWCVersion2Mask | UBWCVersion3Mask | UBWCVersion4Mask;

/// @brief UBWC lossy mode
enum UBWCLossyMode
{
    UBWCLossless = 0,    ///< UBWC Lossless
    UBWCLossy,           ///< UBWC Lossy
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This structure defines the UBWC version info.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct UBWCInfo
{
    UBWCVersion         version;       ///< Version of UBWC.
    UBWCLossyMode       lossy;         ///< Lossyness of UBWC.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This union contains parameters only specified in certain output format, YUV, raw or Jpeg.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
union FormatParams
{
    YUVFormat   yuvFormat[FormatsMaxPlanes];  ///< YUV format specific properties.
    RawFormat   rawFormat;                    ///< RAW format specific properties.
    JPEGFormat  jpegFormat;                   ///< JPEG format specific properties.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This union contains parameters only specified in certain output format, YUV, raw or Jpeg.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct AlignmentInfo
{
    UINT32 strideAlignment;      ///< Stride alignment
    UINT32 scanlineAlignment;    ///< Scanline alignment
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure to define a camera pixel format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CameraPixelFormat
{
    Format  format;             ///<  Color format
    UINT    strideAlignment;    ///<  Unique stride requirement for the format
    UINT    scanlineAlignment;  ///<  Unique scanline requirement for the format
    INT32   bpp;                ///<  Bits-per-pixel for RAW formats
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure to define a gralloc pixel format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct GrallocPixelFormat
{
    Format      format;        ///< Color format
    UINT        pixelFormat;   ///< Gralloc format describing the layout information
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Defines the format layout of an image
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ImagePixelLayout
{
    BOOL isGrallocFormat;        ///< Indicates whether the pixel format is a known gralloc format

    union
    {
        GrallocPixelFormat  grallocFormat; ///< Specifies the gralloc format
        CameraPixelFormat   cameraFormat;  ///<  Specifies internal unique format
    } u;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Gralloc properties, Usage flags and other parameters required for format resolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct GrallocProperties
{
    UINT         pixelFormat;        ///< Pixel format
    UINT64       grallocUsage;       ///< Gralloc usage flags
    ColorSpace   colorSpace;         ///< Color space for the stream
    UINT         staticFormat;       ///< Output format requirement to fallback for
                                     ///   Implementation defined formats
    BOOL         isRawFormat;        ///< Indicates Raw implementation defined format
    BOOL         isInternalBuffer;   ///< Indicates if the buffer to be allocated is internal
    BOOL         isMultiLayerFormat; ///< Indicates if it's a batched format which multiple layer images
};

/// @brief This structure is equivalent to IQtiMapper Hal structure.
struct PlaneLayoutInfo
{
    uint32_t component;             ///< Components represented by the CamxPlaneComponent
    uint32_t horizontalSubsampling; ///< Horizontal subsampling. Must be a positive power of 2
    uint32_t verticalSubsampling;   ///< Vertical subsampling. Must be a positive power of 2
    uint32_t offset;                ///< Offset to the first byte of the top-left pixel of the plane
    int32_t  pixelIncrement;        ///< Pixel increment
    int32_t  stridePixels;          ///< Row width or horizontal stride in pixels
    int32_t  strideBytes;           ///< Row width or horzontal stride in bytes
    int32_t  scanLines;             ///< Plane height or vertical stride
    uint32_t planeSize;             ///< Size of the plane in bytes
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Defines the format of an image
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ImageFormat
{
    UINT32          width;                             ///< Width of the image in pixels.
    UINT32          height;                            ///< Height of the image in pixels.
    UINT32          bufferSize;                        ///< Total Buffer size. This may not be same as sum of planeSizes.
                                                       ///< Some formats may use more than one buffer.
    Format          format;                            ///< Format of the image.
    ColorSpace      colorSpace;                        ///< Color space of the image.
    Rotation        rotation;                          ///< Rotation applied to the image.
    FormatParams    formatParams;                      ///< Format specific definitions.
    SIZE_T          alignment;                         ///< The required alignment in bytes of the
                                                       ///  starting address of the allocated buffer
    AlignmentInfo   planeAlignment[FormatsMaxPlanes];  ///< Stride and scanline alignment for each plane
    UBWCInfo        ubwcVerInfo;                       ///< UBWC format specific properties.
    UINT32          numPlanes;                         ///< Number of entries used in planeLayoutInfo array
    PlaneLayoutInfo planeLayoutInfo[MaxPlaneLayOut];   ///< PlayLayout info for a given image format
};

CAMX_NAMESPACE_END

#endif // CAMXFORMATS_H
