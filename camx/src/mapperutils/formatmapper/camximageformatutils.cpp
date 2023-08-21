////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   camximageformatutils.cpp
/// @brief  Utility functions for camera image formats.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camximageformatmapper.h"
#include "camximageformatutils.h"
#include "camxincs.h"


CAMX_NAMESPACE_BEGIN

// Tile info for supported UBWC formats (from HAS)
static const UBWCTileInfo SupportedUBWCTileInfo[] =
{
    { 48, 64, 4, 256, 16, 4, 3 }, // UBWC_TP10-Y

    { 64, 64, 4, 256, 16, 1, 1 }, // UBWC_NV12-4R-Y

    { 32, 32, 8, 128, 32, 1, 1 }, // UBWC_NV12-Y

    { 32, 64, 4, 256, 16, 2, 1 }, // UBWC_P010
};
static const UINT32 BitWidthRaw8  = 0x2a;
static const UINT32 BitWidthRaw10 = 0x2b;
static const UINT32 BitWidthRaw12 = 0x2c;
static const UINT32 BitWidthRaw14 = 0x2d;
static const UINT32 UHDResolutionWidth  = 3840;  ///< UHD resolution width
static const UINT32 UHDResolutionHeight = 2160;  ///< UHD resolution height


UBWCPlaneModeConfig ImageFormatUtils::s_UBWCPlaneConfig = { 15, 0, 1, 1 , 0};

UBWCModeConfig1 ImageFormatUtils::s_UBWCModeConfig1 = { 0, 0 };

UBWCModeConfig1 ImageFormatUtils::s_UBWCModeConfig2 = { 0, 0 };

BOOL ImageFormatUtils::s_UBWCLimitationOnScaleRatio = TRUE;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PadToSize
///
/// @brief  Adjust size to be an even multiple of the padding requirement.
///
/// @param  size    The size to pad.
/// @param  padding The padding requirement.
///
/// @return The adjusted size.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CAMX_INLINE SIZE_T PadToSize(
    SIZE_T size,
    SIZE_T padding)
{
    return ((size + padding - 1) / padding * padding);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetTotalSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T ImageFormatUtils::GetTotalSize(
    const ImageFormat* pFormat)
{
    SIZE_T bufferSize = 0;
    CAMX_ASSERT(NULL != pFormat);
    if (NULL != pFormat)
    {
        if (0 != pFormat->bufferSize)
        {
            bufferSize = pFormat->bufferSize;
        }
        else
        {
            UINT numberOfPlanes = GetNumberOfPlanes(pFormat);

            for (UINT planeIndex = 0; planeIndex < numberOfPlanes; planeIndex++)
            {
                bufferSize += GetPlaneSize(pFormat, planeIndex);
            }
        }
    }

    return bufferSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetNumberOfPlanes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ImageFormatUtils::GetNumberOfPlanes(
    const ImageFormat* pFormat)
{
    UINT numberOfPlanes = 0;

    CAMX_ASSERT(NULL != pFormat);
    if (NULL != pFormat)
    {
        switch (pFormat->format)
        {
            case Format::RawYUV8BIT:
            case Format::RawPrivate:
            case Format::RawMIPI:
            case Format::RawPlain16:
            case Format::RawMeta8BIT:
            case Format::Jpeg:
            case Format::Blob:
            case Format::Y8:
            case Format::Y16:
            case Format::PD10:
            case Format::RawMIPI8:
            case Format::RawPlain64:
                numberOfPlanes = 1;
                break;
            case Format::YUV420NV12:
            case Format::YUV420NV21:
            case Format::YUV422NV16:
            case Format::YUV420NV12TP10:
            case Format::YUV420NV21TP10:
            case Format::YUV422NV16TP10:
            case Format::P010:
                numberOfPlanes = 2;
                break;
            // Listing UBWC formats separate, for clarity
            case Format::UBWCTP10:
            case Format::UBWCNV12:
            case Format::UBWCNV12FLEX:
            case Format::UBWCNV124R:
            case Format::UBWCP010:
                numberOfPlanes = 2;
                break;
            default:
                CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Format: %d", pFormat->format);
                break;
        }
    }

    CAMX_ASSERT(numberOfPlanes < FormatsMaxPlanes);
    return numberOfPlanes;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetPlaneSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T ImageFormatUtils::GetPlaneSize(
    const ImageFormat* pFormat,
    UINT               planeIndex)
{
    SIZE_T planeSize = 0;
    CAMX_ASSERT(NULL != pFormat);

    if ((NULL != pFormat) && (planeIndex < pFormat->numPlanes) &&
        (0 != pFormat->planeLayoutInfo[planeIndex].planeSize))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupFormat, "planeSize from gralloc calc %d, PlaneLayout %dx%d, format %d",
                         pFormat->planeLayoutInfo[planeIndex].planeSize,
                         pFormat->planeLayoutInfo[planeIndex].stridePixels,
                         pFormat->planeLayoutInfo[planeIndex].scanLines,
                         pFormat->format);

        if (TRUE == IsUBWC(pFormat->format))
        {
            planeSize = pFormat->formatParams.yuvFormat[planeIndex].planeSize;
        }
        else
        {
            planeSize = pFormat->planeLayoutInfo[planeIndex].planeSize;
        }
    }

    if ((NULL != pFormat) && (0 == planeSize))
    {
        if (TRUE == IsYUV(pFormat))
        {
            if (planeIndex < GetNumberOfPlanes(pFormat))
            {
                const YUVFormat* pYUVFormat = &(pFormat->formatParams.yuvFormat[planeIndex]);
                planeSize = pYUVFormat->planeStride * pYUVFormat->sliceHeight;
            }
        }
        else if (TRUE == IsUBWC(pFormat->format))
        {
            planeSize = pFormat->formatParams.yuvFormat[planeIndex].metadataSize +
                pFormat->formatParams.yuvFormat[planeIndex].pixelPlaneSize;
        }
        else if (TRUE == IsRAW(pFormat))
        {
            CAMX_ASSERT(0 == planeIndex);
            if (0 == planeIndex)
            {
                planeSize = GetRawSize(pFormat);
            }
        }
        else if (Format::Jpeg == pFormat->format)
        {
            /// @todo (CAMX-308) Abstract JPEG buffer size assumptions
            /// For JPEG make the assumption that worst case will be 12 bpp. In general data size will be much less.

            // Making JPEG size calculation algorithm same as Framework
            if (0 != pFormat->formatParams.jpegFormat.maxJPEGSizeInBytes)
            {
                UINT32 kMinJpegBufferSize = 256 * 1024 + sizeof(Camera3JPEGBlob);
                FLOAT scaleFactor = ((1.0f * pFormat->width * pFormat->height) /
                    ((pFormat->formatParams.jpegFormat.maxJPEGSizeInBytes - sizeof(Camera3JPEGBlob) - EXIFSize - JpegPadding -
                    pFormat->formatParams.jpegFormat.debugDataSize) / JpegMult));

                CAMX_ASSERT(kMinJpegBufferSize < pFormat->formatParams.jpegFormat.maxJPEGSizeInBytes);
                planeSize = static_cast<SIZE_T>(scaleFactor *
                    (pFormat->formatParams.jpegFormat.maxJPEGSizeInBytes - kMinJpegBufferSize) +
                    kMinJpegBufferSize);
                CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "JPEGMaxSize %d scale %f alignment %d", planeSize, scaleFactor,
                    pFormat->alignment);
            }
            else
            {
                planeSize = ((pFormat->width * pFormat->height) * JpegMult) + sizeof(Camera3JPEGBlob) + EXIFSize +
                            JpegPadding + pFormat->formatParams.jpegFormat.debugDataSize;
                CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "JPEGMaxSize %d alignment %d", planeSize, pFormat->alignment);
            }
        }
        else if (Format::Blob == pFormat->format || Format::Y8 == pFormat->format)
        {
            planeSize = pFormat->width * pFormat->height;
        }
        else if (Format::PD10 == pFormat->format)
        {
            planeSize = pFormat->formatParams.yuvFormat[0].planeStride * pFormat->formatParams.yuvFormat[0].sliceHeight;
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid format");
        }

        // Pad the plane size for the required alignment if not 0. 0 plane size means there was an error with the format
        // definition and we should return 0.
        if (0 != planeSize)
        {
            planeSize = PadToSize(planeSize, pFormat->alignment);
        }
    }

    return planeSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::IsYUV
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageFormatUtils::IsYUV(
    const ImageFormat* pFormat)
{
    BOOL isYUV = FALSE;

    CAMX_ASSERT(NULL != pFormat);
    if (NULL != pFormat)
    {
        switch (pFormat->format)
        {
            case Format::Y8:
            case Format::Y16:
            case Format::YUV420NV12:
            case Format::YUV420NV21:
            case Format::YUV422NV16:
            case Format::YUV420NV12TP10:
            case Format::YUV420NV21TP10:
            case Format::YUV422NV16TP10:
            case Format::PD10:
            case Format::P010:
                isYUV = TRUE;
                break;
            default:
                isYUV = FALSE;
                break;
        }
    }

    return isYUV;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::IsRAW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageFormatUtils::IsRAW(
    const ImageFormat* pFormat)
{
    BOOL isRAW = FALSE;

    CAMX_ASSERT(NULL != pFormat);
    if (NULL != pFormat)
    {
        switch (pFormat->format)
        {
            case Format::RawYUV8BIT:
            case Format::RawPrivate:
            case Format::RawMIPI:
            case Format::RawMIPI8:
            case Format::RawPlain16:
            case Format::RawMeta8BIT:
            case Format::RawPlain64:
                isRAW = TRUE;
                break;
            default:
                isRAW = FALSE;
                break;
        }
    }

    return isRAW;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::IsUBWC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageFormatUtils::IsUBWC(
    Format format)
{
    BOOL isUBWC = FALSE;

    switch (format)
    {
        case Format::UBWCTP10:
        case Format::UBWCNV12:
        case Format::UBWCNV12FLEX:
        case Format::UBWCNV124R:
        case Format::UBWCP010:
            isUBWC = TRUE;
            break;
        default:
            isUBWC = FALSE;
            break;
    }

    return isUBWC;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::Is10BitUBWCFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageFormatUtils::Is10BitUBWCFormat(
    Format format)
{
    BOOL is10BitUBWC = FALSE;

    switch (format)
    {
        case Format::UBWCTP10:
        case Format::UBWCP010:
            is10BitUBWC = TRUE;
            break;
        default:
            break;
    }

    return is10BitUBWC;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::Is8BitUBWCFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageFormatUtils::Is8BitUBWCFormat(
    Format format)
{
    BOOL is8BitUBWC = FALSE;

    switch (format)
    {
        case Format::UBWCNV12:
        case Format::UBWCNV124R:
        case Format::UBWCNV12FLEX:
            is8BitUBWC = TRUE;
            break;
        default:
            break;
    }

    return is8BitUBWC;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::Is10BitFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageFormatUtils::Is10BitFormat(
    Format format)
{
    BOOL is10BitFormat = FALSE;

    switch (format)
    {
        case Format::YUV420NV12TP10:
        case Format::YUV420NV21TP10:
        case Format::YUV422NV16TP10:
        case Format::UBWCTP10:
        case Format::UBWCP010:
        case Format::PD10:
        case Format::P010:
            is10BitFormat = TRUE;
            break;
        default:
            is10BitFormat = FALSE;
            break;
    }

    return is10BitFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::Is8BitFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageFormatUtils::Is8BitFormat(
    Format format)
{
    BOOL is8BitFormat = FALSE;

    switch (format)
    {
        case Format::YUV420NV12TP10:
        case Format::YUV420NV21TP10:
        case Format::YUV422NV16TP10:
        case Format::UBWCTP10:
        case Format::PD10:
        case Format::P010:
            is8BitFormat = FALSE;
            break;
        default:
            is8BitFormat = TRUE;
            break;
    }

    return is8BitFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::IsUHDResolution
///
/// @brief  Tests if passed resolution is greater than or equal to UHD resolution
///
/// @param  width   Width of resolution
/// @param  height  Height of resolution
///
/// @return TRUE if resolution is greater than equal to UHDresolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageFormatUtils::IsUHDResolution(
    UINT  width,
    UINT  height)
{
    BOOL bUHDresolution = FALSE;

    if ((UHDResolutionWidth * UHDResolutionHeight) <= (width * height))
    {
        bUHDresolution = TRUE;
    }

    return bUHDresolution;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::SetupUBWCPlanes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageFormatUtils::SetupUBWCPlanes(
    ImageFormat* pFormat)
{
    CAMX_ASSERT(NULL != pFormat);

    const UBWCTileInfo* pTileInfo = GetUBWCTileInfo(pFormat);

    if (NULL != pTileInfo)
    {
        UINT numberOfPlanes = GetNumberOfPlanes(pFormat);

        for (UINT planeIndex = 0; planeIndex < numberOfPlanes; planeIndex++)
        {
            YUVFormat* pPlane       = &pFormat->formatParams.yuvFormat[planeIndex];
            UINT localWidth         = Utils::AlignGeneric32(pPlane->width, pTileInfo->widthPixels);
            pPlane->width           = (localWidth / pTileInfo->BPPDenominator) * pTileInfo->BPPNumerator;
            FLOAT local1            = static_cast<FLOAT>((static_cast<FLOAT>(pPlane->width) / pTileInfo->widthBytes)) / 64;
            pPlane->metadataStride  = Utils::Ceiling(local1) * 1024;
            FLOAT local2            = static_cast<FLOAT>((static_cast<FLOAT>(pPlane->height) / pTileInfo->height)) / 16;
            UINT  localMetaSize     = Utils::Ceiling(local2) * pPlane->metadataStride;
            pPlane->metadataSize    = Utils::ByteAlign32(localMetaSize, 4096);
            pPlane->metadataHeight  = pPlane->metadataSize / pPlane->metadataStride;
            pPlane->planeStride     = Utils::ByteAlign32(pPlane->width, pTileInfo->widthMacroTile);
            pPlane->sliceHeight     = Utils::ByteAlign32(pPlane->height, pTileInfo->heightMacroTile);
            pPlane->pixelPlaneSize  = Utils::ByteAlign32((pPlane->planeStride * pPlane->sliceHeight), 4096);
            pPlane->planeSize       = GetPlaneSize(pFormat, planeIndex);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupFormat, "pTileInfo is NULL");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetRawSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T ImageFormatUtils::GetRawSize(
    const ImageFormat* pFormat)
{
    SIZE_T planeSize = 0;

    CAMX_ASSERT(NULL != pFormat);

    const RawFormat* pRawFormat = &(pFormat->formatParams.rawFormat);

    switch (pFormat->format)
    {
        case Format::RawYUV8BIT:
            CAMX_ASSERT(16 == pRawFormat->bitsPerPixel);
            planeSize = (pRawFormat->bitsPerPixel)/8 * pRawFormat->stride * pRawFormat->sliceHeight;
            break;
        case Format::RawPrivate:
            if (8 == pRawFormat->bitsPerPixel)
            {
                // 1 byte per pixel (8 bytes per 8 pixels)
                planeSize = pRawFormat->stride * pRawFormat->sliceHeight;
            }
            else if (10 == pRawFormat->bitsPerPixel)
            {
                // 8 bytes per 6 pixels
                CAMX_ASSERT(0 == pRawFormat->stride % 6);
                planeSize = pRawFormat->stride * pRawFormat->sliceHeight * 8 / 6;
            }
            else if (12 == pRawFormat->bitsPerPixel)
            {
                // 8 bytes per 5 pixels
                CAMX_ASSERT(0 == pRawFormat->stride % 5);
                planeSize = pRawFormat->stride * pRawFormat->sliceHeight * 8 / 5;
            }
            else if (14 == pRawFormat->bitsPerPixel)
            {
                // 8 bytes per 4 pixels
                CAMX_ASSERT(0 == pRawFormat->stride % 4);
                planeSize = pRawFormat->stride * pRawFormat->sliceHeight * 8 / 4;
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Invalid bpp: %d for Private format: %d", pRawFormat->bitsPerPixel, pFormat->format);
            }
            break;

        case Format::RawMIPI8:
            planeSize = pRawFormat->stride * pRawFormat->sliceHeight;
            break;

        case Format::RawMIPI:
            if (8 == pRawFormat->bitsPerPixel)
            {
                // 1 byte per 1 pixel
                planeSize = PadToSize((pRawFormat->stride * pRawFormat->sliceHeight), pFormat->alignment);
            }
            else if (10 == pRawFormat->bitsPerPixel)
            {
                // 5 bytes per 4 pixels
                CAMX_ASSERT(0 == pRawFormat->stride % 4);
                planeSize = PadToSize((pRawFormat->stride * pRawFormat->sliceHeight), pFormat->alignment);
            }
            else if (12 == pRawFormat->bitsPerPixel)
            {
                // 3 bytes per 2 pixels
                CAMX_ASSERT(0 == pRawFormat->stride % 2);
                planeSize = PadToSize((pRawFormat->stride * pRawFormat->sliceHeight), pFormat->alignment);
            }
            else if (14 == pRawFormat->bitsPerPixel)
            {
                // 7 bytes per 4 pixels
                CAMX_ASSERT(0 == pRawFormat->stride % 4);
                planeSize = pRawFormat->stride * pRawFormat->sliceHeight * 7 / 4;
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Invalid bpp: %d for Private format: %d", pRawFormat->bitsPerPixel, pFormat->format);
            }
            break;
        case Format::RawPlain16:
            CAMX_ASSERT((8 == pRawFormat->bitsPerPixel)  || (10 == pRawFormat->bitsPerPixel) ||
                        (12 == pRawFormat->bitsPerPixel) || (14 == pRawFormat->bitsPerPixel) ||
                        (16 == pRawFormat->bitsPerPixel));
            planeSize = pRawFormat->stride * pRawFormat->sliceHeight;
            break;
        case Format::RawPlain64:
            CAMX_ASSERT(8 == pRawFormat->bitsPerPixel);
            planeSize = pRawFormat->stride * pRawFormat->sliceHeight;
            break;
        case Format::RawMeta8BIT:
            CAMX_ASSERT(8 == pRawFormat->bitsPerPixel);
            planeSize = pRawFormat->stride * pRawFormat->sliceHeight;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Private format: %d", pFormat->format);
            break;
    }

    return planeSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetBytesPerPixel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT ImageFormatUtils::GetBytesPerPixel(
    const ImageFormat* const pImageFormat)
{
    FLOAT            bytesPerPix = 0.0f;
    const RawFormat* pRawFormat  = &(pImageFormat->formatParams.rawFormat);

    switch (pImageFormat->format)
    {
        case CamX::Format::Y8:
            bytesPerPix = 1.0f;
            break;

        case CamX::Format::Y16:
            bytesPerPix = 2.0f;
            break;

        case CamX::Format::YUV420NV12:
        case CamX::Format::YUV420NV21:
            bytesPerPix = 3.0f / 2.0f;
            break;

        case CamX::Format::YUV422NV16:
            bytesPerPix = 2.0f;
            break;

        case Format::P010:
            // 10-bit per sample is packed in 16-bit word with 6 dummy bits (lsb aligned).
            // P010 currently assumes on YUV420 even though 422 is also possible.
            // (Y:(10+6)*4 + CbCr:(10+6)*2 ) /bytes:8 / pixels:4 = 3 bytes
            bytesPerPix = 3.0f;
            break;

        case Format::UBWCTP10:
        case Format::UBWCNV12:
        case Format::UBWCNV12FLEX:
        case Format::UBWCNV124R:
        {
            UINT  tileWidthInPixels;
            if (Format::UBWCTP10 == pImageFormat->format)
            {
                tileWidthInPixels = 48;
            }
            else
            {
                tileWidthInPixels = 64;
            }

            const UINT  bytesPerTile     = 256;
            const FLOAT chromaMultiplier = 1.5f; // Account for chroma plane as well
            const FLOAT UBWClineSpread   = 4.0f; // Number of lines over which the UBWC is distributed

            bytesPerPix = (bytesPerTile / tileWidthInPixels) * chromaMultiplier / UBWClineSpread;
            break;
        }

        case Format::UBWCP010:
            bytesPerPix = 3.0f;
            break;
        case Format::YUV420NV12TP10:
        case Format::YUV420NV21TP10:
            // 3 10bit samples per 32 bit word (with 2 bit padding); So 12 Y pixels will require (3+3) CbCr pixels after
            // subsampling in both subsampling by 2 in both horizontal and vertical (YUV420)
            // (Y:(10*3 + 2)*4 + CbCr:(10*3 + 2)*2 )/bytes:8 / pixels:12  = 2 bytes
            bytesPerPix = 2;
            break;

        case Format::YUV422NV16TP10:
            // 3 10bit samples per 32 bit word (with 2 bit padding); So 12 Y pixels will require (3+3) CbCr pixels
            // subsampling in both subsampling by 2 only in the horizontal direction (YUV422)
            // (Y:(10*3 + 2)*4 + CbCr:(10*3 + 2)*2*2 )/bytes:8 / pixels:12 = 2.666667 bytes
            bytesPerPix = 2.666667f;
            break;

        case CamX::Format::PD10:
            // 2x2 pixels in 8 bytes
            bytesPerPix = 8.0f / 4.0f;
            break;

        case CamX::Format::RawMIPI8:
        case CamX::Format::RawMeta8BIT:
            bytesPerPix = 1.0f;
            break;

        case CamX::Format::RawMIPI:
            bytesPerPix = pRawFormat->bitsPerPixel / 8.0f;
            break;

        case CamX::Format::RawYUV8BIT:
        case CamX::Format::RawPlain16:
            bytesPerPix = 2.0f;
            break;

        case CamX::Format::RawPrivate:
            switch (pRawFormat->bitsPerPixel)
            {
                case 8:
                    bytesPerPix = 1.0f;
                    break;
                case 10:
                    bytesPerPix = (64.0f / 8.0f) / 6.0f;
                    break;
                case 12:
                    bytesPerPix = (64.0f / 8.0f) / 5.0f;
                    break;
                case 14:
                    bytesPerPix = (64.0f / 8.0f) / 4.0f;
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
    CAMX_ASSERT(0 != bytesPerPix);
    return bytesPerPix;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageFormatUtils::GetFormat(
    GrallocProperties&   rGrallocProperties,
    Format&              rFormat)
{
    UINT32        grFormat = 0;
    CamxResult    result   = CamxResultEFailed;
    FormatMapper* pMapper  = FormatMapper::GetInstance();

    if (NULL != pMapper)
    {
        result = pMapper->GetFormat(rGrallocProperties, rFormat, grFormat);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::ValidateBufferSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageFormatUtils::ValidateBufferSize(
    VOID*   phBufferHandle,
    SIZE_T  bufferSize)
{
    CamxResult    result   = CamxResultEFailed;
    FormatMapper* pMapper  = FormatMapper::GetInstance();

    if (NULL != pMapper)
    {
        result = pMapper->ValidateBufferSize(phBufferHandle, bufferSize);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetUnalignedBufferSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageFormatUtils::GetUnalignedBufferSize(
    VOID*   phBufferHandle,
    SIZE_T  bufferSize,
    Format  format,
    SIZE_T* pGrallocBufferSize)
{
    CamxResult    result   = CamxResultEFailed;
    FormatMapper* pMapper  = FormatMapper::GetInstance();

    if (NULL != pMapper)
    {
        result = pMapper->GetUnalignedBufferSize(phBufferHandle, bufferSize, format, pGrallocBufferSize);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::InitializeFormatParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageFormatUtils::InitializeFormatParams(
    ImageFormat*       pImageFormat,
    FormatParamInfo*   pFormatParamInfo)
{
    FormatMapper* pMapper  = FormatMapper::GetInstance();

    GrallocProperties grallocProperties;
    grallocProperties.colorSpace          = pImageFormat->colorSpace;
    grallocProperties.grallocUsage        = pFormatParamInfo->grallocUsage;
    grallocProperties.isInternalBuffer    = !(pFormatParamInfo->isHALBuffer);
    grallocProperties.isRawFormat         = IsRAW(pImageFormat);
    grallocProperties.pixelFormat         = pFormatParamInfo->pixelFormat;
    grallocProperties.staticFormat        = pFormatParamInfo->outputFormat;
    grallocProperties.isMultiLayerFormat  = (pImageFormat->format == Format::UBWCNV12FLEX) ? TRUE : FALSE;

    // Reset the info
    pImageFormat->numPlanes  = 0;
    pImageFormat->bufferSize = 0;

    if (TRUE == grallocProperties.isRawFormat)
    {
        InitializeRawFormatParams(pImageFormat, pFormatParamInfo);
    }
    else if (pImageFormat->format == Format::Y8)
    {
        pImageFormat->formatParams.rawFormat.bitsPerPixel = 8;
        pImageFormat->formatParams.yuvFormat[0].width     = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[0].height    = pImageFormat->height;

        if ((0 != pFormatParamInfo->sliceHeight) && (0 != pFormatParamInfo->planeStride))
        {
            pImageFormat->formatParams.yuvFormat[0].planeStride = pFormatParamInfo->planeStride;
            pImageFormat->formatParams.yuvFormat[0].sliceHeight = pFormatParamInfo->sliceHeight;
        }
        else
        {
            pImageFormat->formatParams.yuvFormat[0].planeStride =
                Utils::AlignGeneric32(pImageFormat->width, pImageFormat->planeAlignment[0].strideAlignment);
            pImageFormat->formatParams.yuvFormat[0].sliceHeight =
                Utils::AlignGeneric32(pImageFormat->height, pImageFormat->planeAlignment[0].scanlineAlignment);
        }

        pImageFormat->formatParams.yuvFormat[0].planeSize = GetPlaneSize(pImageFormat, 0);
    }
    else if (pImageFormat->format == Format::Blob)
    {
        pImageFormat->formatParams.rawFormat.bitsPerPixel = 8;
        pImageFormat->formatParams.rawFormat.stride       = pImageFormat->width;
        pImageFormat->formatParams.rawFormat.sliceHeight  = pImageFormat->height;
        ///@ todo (CAMX-1797) Is this good enough or do we need to handle it properly?
        pImageFormat->formatParams.yuvFormat[0].width       = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[0].height      = pImageFormat->height;
        pImageFormat->formatParams.yuvFormat[0].planeStride = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[0].sliceHeight = pImageFormat->height;
        pImageFormat->formatParams.yuvFormat[0].planeSize   = GetPlaneSize(pImageFormat, 0);
    }
    else if ((pImageFormat->format == Format::YUV420NV12) ||
             (pImageFormat->format == Format::YUV420NV21))
    {
        if (TRUE == pFormatParamInfo->isHALBuffer)
        {
            // There are many NV12 formats that gralloc can allocate for the HAL buffer.
            // Initialize the format params and alignment appropriately.
            if (NULL != pMapper)
            {
                pMapper->FillImageFormatInfo(grallocProperties, pImageFormat);
            }
        }
        else
        {
            pImageFormat->planeAlignment[0].strideAlignment   = (0 == pImageFormat->planeAlignment[0].strideAlignment)   ?
                2 : Utils::EvenCeilingUINT32(pImageFormat->planeAlignment[0].strideAlignment);
            pImageFormat->planeAlignment[0].scanlineAlignment = (0 == pImageFormat->planeAlignment[0].scanlineAlignment) ?
                2 : Utils::EvenCeilingUINT32(pImageFormat->planeAlignment[0].scanlineAlignment);
            pImageFormat->planeAlignment[1].strideAlignment   = (0 == pImageFormat->planeAlignment[1].strideAlignment)   ?
                2 : Utils::EvenCeilingUINT32(pImageFormat->planeAlignment[1].strideAlignment);
            pImageFormat->planeAlignment[1].scanlineAlignment = (0 == pImageFormat->planeAlignment[1].scanlineAlignment) ?
                2 : Utils::EvenCeilingUINT32(pImageFormat->planeAlignment[1].scanlineAlignment);

            pImageFormat->formatParams.yuvFormat[0].width  = pImageFormat->width;
            pImageFormat->formatParams.yuvFormat[0].height = pImageFormat->height;
            pImageFormat->formatParams.yuvFormat[1].width  = pImageFormat->width;
            pImageFormat->formatParams.yuvFormat[1].height = pImageFormat->height >> 1;
            if (pFormatParamInfo->sliceHeight != 0 && pFormatParamInfo->planeStride != 0)
            {
                pImageFormat->formatParams.yuvFormat[0].planeStride = Utils::EvenCeilingUINT32(pFormatParamInfo->planeStride);
                pImageFormat->formatParams.yuvFormat[0].sliceHeight = Utils::EvenCeilingUINT32(pFormatParamInfo->sliceHeight);
                pImageFormat->formatParams.yuvFormat[1].planeStride = Utils::EvenCeilingUINT32(pFormatParamInfo->planeStride);
                pImageFormat->formatParams.yuvFormat[1].sliceHeight =
                    Utils::EvenCeilingUINT32(pFormatParamInfo->sliceHeight >> 1);
            }
            else
            {
                pImageFormat->formatParams.yuvFormat[0].planeStride =
                    Utils::AlignGeneric32(pImageFormat->width, pImageFormat->planeAlignment[0].strideAlignment);
                pImageFormat->formatParams.yuvFormat[0].sliceHeight =
                    Utils::AlignGeneric32(pImageFormat->height, pImageFormat->planeAlignment[0].scanlineAlignment);
                pImageFormat->formatParams.yuvFormat[1].planeStride =
                    Utils::AlignGeneric32(pImageFormat->width, pImageFormat->planeAlignment[1].strideAlignment);
                pImageFormat->formatParams.yuvFormat[1].sliceHeight =
                    Utils::AlignGeneric32((pImageFormat->height + 1) >> 1, pImageFormat->planeAlignment[1].scanlineAlignment);
            }

            pImageFormat->formatParams.yuvFormat[0].planeSize = GetPlaneSize(pImageFormat, 0);
            pImageFormat->formatParams.yuvFormat[1].planeSize = GetPlaneSize(pImageFormat, 1);
        }
    }
    else if (pImageFormat->format == Format::P010)
    {
        // Venus requirements from msm_media_info.h
        // Y_Stride : Width * 2 aligned to 256
        // UV_Stride : Width * 2 aligned to 256
        // Y_Scanlines : Height aligned to 32
        // UV_Scanlines : Height / 2 aligned to 16
        // Extradata : Arbitrary(software - imposed) padding
        // Total size = align((Y_Stride * Y_Scanlines
        //                     + UV_Stride * UV_Scanlines
        //                     + max(Extradata, Y_Stride * 8), 4096)
        // P010 is 2 bytes per pixel where the least significant 6 bits are essentially ignored.
        UINT32 pitchInBytes = Utils::AlignGeneric32((pImageFormat->width * 2), 256);
        UINT32 ySliceHeight = Utils::AlignGeneric32(pImageFormat->height, 32);
        UINT32 uvSliceHeight = Utils::AlignGeneric32((pImageFormat->height >> 1), 16);

        pImageFormat->formatParams.yuvFormat[0].width = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[0].height = pImageFormat->height;
        pImageFormat->formatParams.yuvFormat[0].planeStride = pitchInBytes;
        // Venus requires the height to be aligned to 32
        pImageFormat->formatParams.yuvFormat[0].sliceHeight = ySliceHeight;
        pImageFormat->formatParams.yuvFormat[0].planeSize = GetPlaneSize(pImageFormat, 0);
        pImageFormat->formatParams.yuvFormat[1].width = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[1].height = pImageFormat->height >> 1;
        pImageFormat->formatParams.yuvFormat[1].planeStride = pitchInBytes;
        // Venus requires the height to be aligned to 32
        pImageFormat->formatParams.yuvFormat[1].sliceHeight = uvSliceHeight;
        pImageFormat->formatParams.yuvFormat[1].planeSize = GetPlaneSize(pImageFormat, 1);

        // XBALCI
        /* pImageFormat->formatParams.yuvFormat[0].width =
            Utils::AlignGeneric32((pImageFormat->width * 2), 16);
        pImageFormat->formatParams.yuvFormat[0].height = pImageFormat->height;
        pImageFormat->formatParams.yuvFormat[0].planeStride =
            Utils::AlignGeneric32((pImageFormat->width * 2), 128);
        pImageFormat->formatParams.yuvFormat[0].sliceHeight = Utils::AlignGeneric32(pImageFormat->height, 32);
        pImageFormat->formatParams.yuvFormat[0].planeSize = GetPlaneSize(pImageFormat, 0);
        pImageFormat->formatParams.yuvFormat[1].width =
            Utils::AlignGeneric32((pImageFormat->width * 2), 16);
        pImageFormat->formatParams.yuvFormat[1].height = pImageFormat->height >> 1;
        pImageFormat->formatParams.yuvFormat[1].planeStride =
            Utils::AlignGeneric32((pImageFormat->width * 2), 128);
        pImageFormat->formatParams.yuvFormat[1].sliceHeight = Utils::AlignGeneric32((pImageFormat->height >> 1), 16);
        pImageFormat->formatParams.yuvFormat[1].planeSize = GetPlaneSize(pImageFormat, 1); */
    }
    else if ((pImageFormat->format == Format::UBWCTP10) ||
             (pImageFormat->format == Format::UBWCP010) ||
             (pImageFormat->format == Format::UBWCNV12) ||
             (pImageFormat->format == Format::UBWCNV12FLEX) ||
             (pImageFormat->format == Format::UBWCNV124R))
    {
        // Align width and height to even pixel
        pImageFormat->width                            = Utils::EvenCeilingUINT32(pImageFormat->width);
        pImageFormat->height                           = Utils::EvenCeilingUINT32(pImageFormat->height);
        pImageFormat->formatParams.yuvFormat[0].width  = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[0].height = pImageFormat->height;
        pImageFormat->formatParams.yuvFormat[1].width  = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[1].height = pImageFormat->height >> 1;
        pImageFormat->ubwcVerInfo.version              = GetUBWCVersionForLink(pFormatParamInfo);
        pImageFormat->ubwcVerInfo.lossy                = GetUBWCLossyMode(pFormatParamInfo);
        CAMX_LOG_VERBOSE(CamxLogGroupFormat, "UBWC verison %d, lossy %d, format %d",
                         pImageFormat->ubwcVerInfo.version,
                         pImageFormat->ubwcVerInfo.lossy,
                         pImageFormat->format);

        if ((FALSE == pFormatParamInfo->isHALBuffer) || (NULL == pMapper))
        {
            SetupUBWCPlanes(pImageFormat);
        }
        else
        {
            pMapper->FillImageFormatInfo(grallocProperties, pImageFormat);
        }
    }
    else if (pImageFormat->format == Format::PD10)
    {
        // A PD10 pack consists of 2x2 10b YUV420 2 pixels(4 Y, 1 Cb and 1 Cr components)
        // Each PD10 pack is 8B 3 aligned(60 bits data + 4 bits dummy data.
        // For PD format, bus treats every 2 lines as a single buffer line (2x2 packing).
        // Every 2x2 block is packed into 8 bytes, so buffer_width = image_width/2*8 = image_width*4.
        // Buffer_height is half of the actual image height.
        pImageFormat->formatParams.yuvFormat[0].width           = pImageFormat->width * 4;
        pImageFormat->formatParams.yuvFormat[0].height          = Utils::EvenCeilingUINT32(pImageFormat->height / 2);
        pImageFormat->formatParams.yuvFormat[0].planeStride     =
             Utils::AlignGeneric32(Utils::EvenCeilingUINT32(pImageFormat->width /2) * 8, 192);
        pImageFormat->formatParams.yuvFormat[0].planeStride = Utils::AlignGeneric32(
            pImageFormat->formatParams.yuvFormat[0].planeStride, 256);

        pImageFormat->formatParams.yuvFormat[0].sliceHeight     =
            Utils::AlignGeneric32(Utils::EvenCeilingUINT32(pImageFormat->height / 2), 32);
        pImageFormat->formatParams.yuvFormat[0].planeSize       = GetPlaneSize(pImageFormat, 0);
    }
    else if (pImageFormat->format == Format::YUV422NV16)
    {
        pImageFormat->formatParams.yuvFormat[0].width       = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[0].height      = pImageFormat->height;
        pImageFormat->formatParams.yuvFormat[0].planeStride = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[0].sliceHeight = pImageFormat->height;
        pImageFormat->formatParams.yuvFormat[0].planeSize   = GetPlaneSize(pImageFormat, 0);
        pImageFormat->formatParams.yuvFormat[1].width       = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[1].height      = pImageFormat->height;
        pImageFormat->formatParams.yuvFormat[1].planeStride = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[1].sliceHeight = pImageFormat->height;
        pImageFormat->formatParams.yuvFormat[1].planeSize   = GetPlaneSize(pImageFormat, 1);
    }
    else if (pImageFormat->format == Format::YUV422NV16TP10)
    {
        pImageFormat->formatParams.yuvFormat[0].width       = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[0].height      = pImageFormat->height;
        // For TP10 every 4 bytes has 3 pixels. Aligning the stride to make sure it is multiple of 3
        pImageFormat->formatParams.yuvFormat[0].planeStride = (Utils::AlignGeneric32(pImageFormat->width, 3)) * 4 / 3;
        pImageFormat->formatParams.yuvFormat[0].planeStride =
            Utils::AlignGeneric32(pImageFormat->formatParams.yuvFormat[0].planeStride, 64);
        pImageFormat->formatParams.yuvFormat[0].sliceHeight = Utils::AlignGeneric32(pImageFormat->height, 4);
        pImageFormat->formatParams.yuvFormat[0].planeSize   = GetPlaneSize(pImageFormat, 0);
        pImageFormat->formatParams.yuvFormat[1].width       = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[1].height      = pImageFormat->height;
        // For TP10 every 4 bytes has 3 pixels. Aligning the stride to make sure it is multiple of 3
        pImageFormat->formatParams.yuvFormat[1].planeStride = (Utils::AlignGeneric32(pImageFormat->width, 3)) * 4 / 3;
        pImageFormat->formatParams.yuvFormat[1].planeStride =
            Utils::AlignGeneric32(pImageFormat->formatParams.yuvFormat[1].planeStride, 64);
        pImageFormat->formatParams.yuvFormat[1].sliceHeight = Utils::AlignGeneric32((pImageFormat->height >> 1), 4);
        pImageFormat->formatParams.yuvFormat[1].planeSize   = GetPlaneSize(pImageFormat, 1);
    }
    else if ((pImageFormat->format == Format::YUV420NV12TP10) ||
             (pImageFormat->format == Format::YUV420NV21TP10))
    {
        pImageFormat->formatParams.yuvFormat[0].width       = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[0].height      = pImageFormat->height;
        // For TP10 every 4 bytes has 3 pixels. Aligning the stride to make sure it is multiple of 3
        pImageFormat->formatParams.yuvFormat[0].planeStride = (Utils::AlignGeneric32(pImageFormat->width, 3)) * 4 / 3;
        pImageFormat->formatParams.yuvFormat[0].planeStride =
            Utils::AlignGeneric32(pImageFormat->formatParams.yuvFormat[0].planeStride, 64);
        pImageFormat->formatParams.yuvFormat[0].sliceHeight = Utils::AlignGeneric32(pImageFormat->height, 4);
        pImageFormat->formatParams.yuvFormat[0].planeSize   = GetPlaneSize(pImageFormat, 0);
        pImageFormat->formatParams.yuvFormat[1].width       = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[1].height      = pImageFormat->height >> 1;
        // For TP10 every 4 bytes has 3 pixels. Aligning the stride to make sure it is multiple of 3
        pImageFormat->formatParams.yuvFormat[1].planeStride = (Utils::AlignGeneric32(pImageFormat->width, 3)) * 4 / 3;
        pImageFormat->formatParams.yuvFormat[1].planeStride =
            Utils::AlignGeneric32(pImageFormat->formatParams.yuvFormat[1].planeStride, 64);
        pImageFormat->formatParams.yuvFormat[1].sliceHeight = Utils::AlignGeneric32((pImageFormat->height >> 1), 4);
        pImageFormat->formatParams.yuvFormat[1].planeSize   = GetPlaneSize(pImageFormat, 1);
    }
    else if ((Format::Y16 == pImageFormat->format) && (ColorSpace::Depth == pImageFormat->colorSpace))
    {
        pImageFormat->formatParams.yuvFormat[0].width       = pImageFormat->width;
        pImageFormat->formatParams.yuvFormat[0].height      = pImageFormat->height;
        pImageFormat->formatParams.yuvFormat[0].planeStride = pImageFormat->width * 2;
        pImageFormat->formatParams.yuvFormat[0].sliceHeight = pImageFormat->height;
    }
    else if (Format::Jpeg != pImageFormat->format)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Unexpected buffer format 0x%x", pImageFormat->format);
    }

    if ((TRUE == IsUBWC(pImageFormat->format)) ||
       ((TRUE == IsYUV(pImageFormat))         &&
        (TRUE == pFormatParamInfo->isHALBuffer)))
    {
        // 4kB alignment
        pImageFormat->alignment = 4096;

        // Update YUV Plane alignment value through static settings.
        // This is needed for some OEM's to update plane alignment value.
        if (Format::YUV420NV21 == pImageFormat->format)
        {
            pImageFormat->alignment = pFormatParamInfo->yuvPlaneAlign;
            CAMX_LOG_INFO(CamxLogGroupFormat, "internal nodes for YUV format %d uses alignment value %d",
                          pImageFormat->format, pImageFormat->alignment);
        }
    }
    else if (pImageFormat->format == Format::Jpeg)
    {
        pImageFormat->formatParams.jpegFormat.maxJPEGSizeInBytes = pFormatParamInfo->maxJPEGSize;
        pImageFormat->formatParams.jpegFormat.debugDataSize = pFormatParamInfo ->debugDataSize;
        pImageFormat->alignment = 1;
    }
    else
    {
        // 16B/128bits alignement
        pImageFormat->alignment = 16;
    }

    if (((TRUE == IsYUV(pImageFormat)) || (TRUE == IsUBWC(pImageFormat->format))) &&
        (FALSE == pFormatParamInfo->isHALBuffer))
    {
        YUVFormat* pYPlane = &pImageFormat->formatParams.yuvFormat[0];
        YUVFormat* pCPlane = &pImageFormat->formatParams.yuvFormat[1];
        CAMX_LOG_VERBOSE(CamxLogGroupFormat,
            "YUV format %d Properties wxh %dx%d, StrideXsliceHeight %dx%d, %dx%d, size %d, %d, align %dx%d",
            pImageFormat->format, pImageFormat->width, pImageFormat->height,
            pYPlane->planeStride, pYPlane->sliceHeight,
            pCPlane->planeStride, pCPlane->sliceHeight,
            pYPlane->planeSize, pCPlane->planeSize,
            pImageFormat->planeAlignment[0].strideAlignment, pImageFormat->planeAlignment[0].scanlineAlignment);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::InitializeRawFormatParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageFormatUtils::InitializeRawFormatParams(
    ImageFormat*       pImageFormat,
    FormatParamInfo*   pFormatParamInfo)
{
    FormatMapper* pMapper  = FormatMapper::GetInstance();

    GrallocProperties grallocProperties;
    grallocProperties.colorSpace          = pImageFormat->colorSpace;
    grallocProperties.grallocUsage        = pFormatParamInfo->grallocUsage;
    grallocProperties.isInternalBuffer    = !(pFormatParamInfo->isHALBuffer);
    grallocProperties.isRawFormat         = TRUE;
    grallocProperties.pixelFormat         = pFormatParamInfo->pixelFormat;
    grallocProperties.staticFormat        = pFormatParamInfo->outputFormat;
    grallocProperties.isMultiLayerFormat  = FALSE;

    ///@ todo (CAMX-2695) Remove the hardcode value for bitsperpixel, color pattern
    if (pImageFormat->format == Format::RawPlain16)
    {
        pImageFormat->formatParams.rawFormat.bitsPerPixel       = 10;
        pImageFormat->formatParams.rawFormat.stride             = pImageFormat->width * 2;
        pImageFormat->formatParams.rawFormat.sliceHeight        = pImageFormat->height;
        pImageFormat->formatParams.rawFormat.colorFilterPattern = ColorFilterPattern::RGGB;
        ///@ todo (CAMX-346) Is this good enough or do we need to handle it properly?
    }
    ///@ todo (CAMX-2695) Remove the hardcode value for bitsperpixel, color pattern
    else if (pImageFormat->format == Format::RawPlain64)
    {
        pImageFormat->formatParams.rawFormat.bitsPerPixel       = 8;
        pImageFormat->formatParams.rawFormat.stride             = pImageFormat->width * 8;
        pImageFormat->formatParams.rawFormat.sliceHeight        = pImageFormat->height;
        pImageFormat->formatParams.rawFormat.colorFilterPattern = ColorFilterPattern::RGGB;
        ///@ todo (CAMX-346) Is this good enough or do we need to handle it properly?
    }
    else if (pImageFormat->format == Format::RawMIPI8)
    {
        pImageFormat->formatParams.rawFormat.bitsPerPixel       = 8;
        pImageFormat->formatParams.rawFormat.stride             = pImageFormat->width;
        pImageFormat->formatParams.rawFormat.colorFilterPattern = ColorFilterPattern::Y;
        pImageFormat->formatParams.rawFormat.sliceHeight        = pImageFormat->height;
    }
    else if (pImageFormat->format == Format::RawMIPI)
    {
        UINT32 bitWidth = (0 == pFormatParamInfo->bitWidth) ? BitWidthRaw10: pFormatParamInfo->bitWidth;

        if (bitWidth == BitWidthRaw8)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupFormat, "use raw 8");
            pImageFormat->formatParams.rawFormat.bitsPerPixel       = 8;
            pImageFormat->formatParams.rawFormat.stride             = pImageFormat->width;
            pImageFormat->formatParams.rawFormat.colorFilterPattern = ColorFilterPattern::RGGB;
            pImageFormat->formatParams.rawFormat.sliceHeight        = pImageFormat->height;
        }
        else if (bitWidth == BitWidthRaw10)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupFormat, "use raw 10");
            pImageFormat->formatParams.rawFormat.bitsPerPixel       = 10;
            pImageFormat->formatParams.rawFormat.stride             = Utils::AlignGeneric32((pImageFormat->width * 5 / 4), 16);
            pImageFormat->formatParams.rawFormat.colorFilterPattern = ColorFilterPattern::RGGB;
            pImageFormat->formatParams.rawFormat.sliceHeight        = pImageFormat->height;
        }
        else if (bitWidth == BitWidthRaw12)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupFormat, "use raw 12");
            pImageFormat->formatParams.rawFormat.bitsPerPixel       = 12;
            pImageFormat->formatParams.rawFormat.stride             = Utils::AlignGeneric32((pImageFormat->width * 3 / 2), 16);
            pImageFormat->formatParams.rawFormat.colorFilterPattern = ColorFilterPattern::BGGR;
            pImageFormat->formatParams.rawFormat.sliceHeight        = pImageFormat->height;
        }
        else if (bitWidth == BitWidthRaw14)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupFormat, "use raw 14");
            pImageFormat->formatParams.rawFormat.bitsPerPixel       = 14;
            pImageFormat->formatParams.rawFormat.stride             = Utils::AlignGeneric32((pImageFormat->width * 7 / 4), 16);
            pImageFormat->formatParams.rawFormat.colorFilterPattern = ColorFilterPattern::RGGB;
            pImageFormat->formatParams.rawFormat.sliceHeight        = pImageFormat->height;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupFormat, "default use raw 10");
            pImageFormat->formatParams.rawFormat.bitsPerPixel       = 10;
            pImageFormat->formatParams.rawFormat.stride             = Utils::AlignGeneric32((pImageFormat->width * 5 / 4), 16);
            pImageFormat->formatParams.rawFormat.colorFilterPattern = ColorFilterPattern::RGGB;
            pImageFormat->formatParams.rawFormat.sliceHeight        = pImageFormat->height;
        }
        ///@ todo (CAMX-346) Is this good enough or do we need to handle it properly?
    }

    if ((NULL != pMapper) && (TRUE == pFormatParamInfo->isHALBuffer))
    {
        pMapper->FillImageFormatInfo(grallocProperties, pImageFormat);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupFormat, "WxH %dx%d, StrideXsliceHeight %dx%d, bps %d, isHALBuffer %d, format %d",
                     pImageFormat->width, pImageFormat->height,
                     pImageFormat->formatParams.rawFormat.stride,
                     pImageFormat->formatParams.rawFormat.sliceHeight,
                     pImageFormat->formatParams.rawFormat.bitsPerPixel,
                     pFormatParamInfo->isHALBuffer, pImageFormat->format);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetUBWCTileInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const UBWCTileInfo* ImageFormatUtils::GetUBWCTileInfo(
    const ImageFormat* pFormat)
{
    // pointer to tile info for a UBWC formats
    const UBWCTileInfo*        pTileInfo       = NULL;
    Format                     UBWCFormat      = pFormat->format;
    BOOL                       validUBWCFormat = TRUE;
    UINT                       indexUBWC       = 0;

    CAMX_ASSERT(NULL != pFormat);
    switch (UBWCFormat)
    {
        case Format::UBWCTP10:
            indexUBWC = 0;
            break;
        case Format::UBWCNV124R:
            indexUBWC = 1;
            break;
        case Format::UBWCNV12:
        case Format::UBWCNV12FLEX:
            indexUBWC = 2;
            break;
        case Format::UBWCP010:
            indexUBWC = 3;
            break;
        default:
            validUBWCFormat = FALSE;
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid UBWC format: %d", UBWCFormat);
            break;
    }

    if (TRUE == validUBWCFormat)
    {
        pTileInfo = &(SupportedUBWCTileInfo[indexUBWC]);
    }

    return pTileInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetUBWCHWVersionExternal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ImageFormatUtils::GetUBWCHWVersion()
{
    return ImageFormatUtils::s_UBWCModeConfig1.UBWCVersion;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetUBWCHWVersionExternal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ImageFormatUtils::GetUBWCHWVersionExternal()
{
    return ImageFormatUtils::s_UBWCModeConfig2.UBWCVersion;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetUBWCScaleRatioLimitationFlag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageFormatUtils::GetUBWCScaleRatioLimitationFlag()
{
    return ImageFormatUtils::s_UBWCLimitationOnScaleRatio;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetUBWCVersion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ImageFormatUtils::GetUBWCVersion()
{
    return  (ImageFormatUtils::s_UBWCModeConfig1.UBWCVersion);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetUBWCModeConfig1
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ImageFormatUtils::GetUBWCModeConfig1(
    const ImageFormat* pFormat)
{
    UINT32 modeConfig = 0;

    modeConfig |= (pFormat->ubwcVerInfo.lossy << 4);
    modeConfig |= pFormat->ubwcVerInfo.version;

    return modeConfig;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetUBWCModeConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ImageFormatUtils::GetUBWCModeConfig(
    const ImageFormat* pFormat,
    UINT               planeIndex)
{
    UINT32 modeConfig = 0;
    switch (pFormat->format)
    {
        case Format::UBWCTP10:
            modeConfig              = (0 == planeIndex) ? 0x2 : 0x3;
            break;
        case Format::UBWCNV12:
        case Format::UBWCNV12FLEX:
            modeConfig              = (0 == planeIndex) ? 0x0 : 0x1;
            break;
        case Format::UBWCNV124R:
            modeConfig              = (0 == planeIndex) ? 0x4 : 0x5;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid UBWC format: %d", pFormat->format);
            break;
    }

    modeConfig   = (modeConfig << 10);                                             // MODE_SEL
    modeConfig  |= (ImageFormatUtils::s_UBWCPlaneConfig.highestValueBit << 5);     // HIGHESTBANKBIT_VAL
    modeConfig  |= (ImageFormatUtils::s_UBWCPlaneConfig.highestBankL1Enable<< 4);  // HIGHESTBANK_LV1_EN
    modeConfig  |= (ImageFormatUtils::s_UBWCPlaneConfig.highestBankEnable << 3);   // HIGHESTBANKBIT_EN
    modeConfig  |= (ImageFormatUtils::s_UBWCPlaneConfig.bankSpreadEnable << 2);    // BANKSPREAD_EN
    modeConfig  |= (ImageFormatUtils::s_UBWCPlaneConfig.eightChannelEnable << 16); // 8CHANNEL_EN

    modeConfig  |= (0x3);                                                          // COMPRESS_EN<<1 | UBWC_EN
    CAMX_LOG_VERBOSE(CamxLogGroupFormat, "modeConfig 0x%x format:%d planeIndex:%d highestValueBit:%d,"
        "highestBankL1Enable:%d highestBankEnable:%d bankSpreadEnable:%d 8channel %d",
        modeConfig, pFormat->format, planeIndex,
        ImageFormatUtils::s_UBWCPlaneConfig.highestValueBit,
        ImageFormatUtils::s_UBWCPlaneConfig.highestBankL1Enable,
        ImageFormatUtils::s_UBWCPlaneConfig.highestBankEnable,
        ImageFormatUtils::s_UBWCPlaneConfig.bankSpreadEnable,
        ImageFormatUtils::s_UBWCPlaneConfig.eightChannelEnable);
    return modeConfig;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageFormatUtils::GetUBWCPartialTileInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageFormatUtils::GetUBWCPartialTileInfo(
    const struct UBWCTileInfo*     pTileInfo,
    struct UBWCPartialTileInfo*    pUBWCPartialTileParam,
    UINT                           startOffsetPixel,
    UINT                           widthPixels)
{
    if (NULL != pTileInfo)
    {
        UINT alignWidthPixels    = Utils::AlignGeneric32(widthPixels, pTileInfo->BPPDenominator);
        UINT startOffsetBytes    = (startOffsetPixel * pTileInfo->BPPNumerator / pTileInfo->BPPDenominator);
        UINT alignEndOffsetPixel = (startOffsetPixel + alignWidthPixels);
        UINT alignEndOffsetBytes = (alignEndOffsetPixel * pTileInfo->BPPNumerator / pTileInfo->BPPDenominator);
        INT  alignBitMask =  static_cast<INT>(alignEndOffsetBytes & 0x3F);

        pUBWCPartialTileParam->partialTileBytesLeft  =
            (startOffsetBytes & 0x3F);
        pUBWCPartialTileParam->partialTileBytesRight =
            (alignBitMask == 0) ? 0 : (static_cast<INT>(pTileInfo->widthBytes) - alignBitMask);
        // dual ife will require align this down
        pUBWCPartialTileParam->horizontalInit        = startOffsetPixel;
        pUBWCPartialTileParam->verticalInit          = 0;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupFormat, "pTileInfo is NULL");
    }
}

CAMX_NAMESPACE_END
