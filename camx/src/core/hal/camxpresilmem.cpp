////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxpresilmem.cpp
/// @brief Camxpresilmem class declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhal3entry.h"
#include "camxhal3module.h"
#include "camxhwenvironment.h"
#include "camximageformatutils.h"
#include "camxmem.h"
#include "camxpresilmem.h"

/// @brief Presil memory handle
struct _CamxMemHandle
{
    VOID* pData; ///< Presil memory data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetCamXFormatFromHALFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamX::Format GetCamXFormatFromHALFormat()
{
    CamX::Format camxFormat;

    const CamX::StaticSettings* pStaticSettings = CamX::HwEnvironment::GetInstance()->GetStaticSettings();

    switch (pStaticSettings->outputFormat)
    {
        case CamX::OutputFormatYUV420NV12:
            camxFormat = CamX::Format::YUV420NV12;
            break;
        case CamX::OutputFormatUBWCNV12:
            camxFormat = CamX::Format::UBWCNV12;
            break;
        case CamX::OutputFormatUBWCTP10:
            camxFormat = CamX::Format::UBWCTP10;
            break;
        default:
            camxFormat = CamX::Format::YUV420NV12;
            break;
    }

    return camxFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetCamXFormatFromCHIFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamX::Format GetCamXFormatFromCHIFormat(
    UINT format)
{
    CamX::Format camxFormat;

    switch (format)
    {
        case ChiStreamFormatYCbCr420_888:
            camxFormat = CamX::Format::YUV420NV12;
            break;
        case ChiStreamFormatUBWCNV12:
            camxFormat = CamX::Format::UBWCNV12;
            break;
        case ChiStreamFormatUBWCTP10:
            camxFormat = CamX::Format::UBWCTP10;
            break;
        case ChiStreamFormatP010:
            camxFormat = CamX::Format::P010;
            break;
        case ChiStreamFormatPD10:
            camxFormat = CamX::Format::PD10;
            break;
        default:
            camxFormat = CamX::Format::YUV420NV12;
            break;
    }

    return camxFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetImageFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxMemResult GetImageFormat(
    UINT32             width,
    UINT32             height,
    UINT32             format,
    UINT32             usage,
    CamX::ImageFormat* pImageFormat)
{
    CamxMemResult   result      = CamxMemSuccess;
    CamX::Format    camxFormat  = CamX::Format::RawPlain16;
    ChiStreamFormat pixelFormat = ChiStreamFormatImplDefined;

    if (format == HAL_PIXEL_FORMAT_YCbCr_420_888 ||
        format == HAL_PIXEL_FORMAT_YCrCb_420_SP)
    {
        camxFormat  = CamX::Format::YUV420NV12;
        pixelFormat = ChiStreamFormatYCbCr420_888;
    }
    else if ((format == HAL_PIXEL_FORMAT_RAW10) ||
             (format == HAL_PIXEL_FORMAT_RAW_OPAQUE))
    {
        camxFormat  = CamX::Format::RawMIPI;
        pixelFormat = ChiStreamFormatRaw10;
    }
    else if (format == HAL_PIXEL_FORMAT_RAW16)
    {
        camxFormat = CamX::Format::RawPlain16;
        pixelFormat = ChiStreamFormatRawOpaque;
    }
    else if (format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
    {
        camxFormat = GetCamXFormatFromHALFormat();
    }
    else if ((format == ChiStreamFormatUBWCTP10)     ||
             (format == ChiStreamFormatUBWCNV12)     ||
             (format == ChiStreamFormatYCbCr420_888) ||
             (format == ChiStreamFormatP010)         ||
             (format == ChiStreamFormatPD10))
    {
        camxFormat  = GetCamXFormatFromCHIFormat(format);
        pixelFormat = ChiStreamFormatYCbCr420_888;
    }
    else
    {
        result = CamxMemFailed;
    }

    /// @todo (CAMX-1441) Add support for other Raw formats if needed

    if ((CamxMemSuccess == result) && (NULL != pImageFormat))
    {
        CamX::FormatParamInfo formatParamInfo = { 0 };

        formatParamInfo.isHALBuffer           = 1;
        formatParamInfo.grallocUsage          = usage;
        formatParamInfo.yuvPlaneAlign         = 4096;
        formatParamInfo.pixelFormat           = pixelFormat;
        pImageFormat->width                   = width;
        pImageFormat->height                  = height;
        pImageFormat->format                  = camxFormat;
        pImageFormat->colorSpace              = CamX::ColorSpace::BT601Full;
        pImageFormat->rotation                = CamX::Rotation::CW0Degrees;

        CamX::ImageFormatUtils::InitializeFormatParams(pImageFormat, &formatParamInfo);
    }

    return result;
}

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Exported Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CamxMemGetImageSizeStride
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC size_t CamxMemGetImageSizeStride(
    uint32_t       width,
    uint32_t       height,
    uint32_t       format,
    uint32_t       usage,
    uint32_t*      pPlaneStride,
    uint32_t*      pSliceHeight)
{
    CamxMemResult     result        = CamxMemSuccess;
    CamX::ImageFormat imageFormat   = { 0 };
    size_t            imageSize     = 0;

    result = GetImageFormat(width, height, format, usage, &imageFormat);

    if ((NULL != pPlaneStride) && (NULL != pSliceHeight) && (CamxMemSuccess == result))
    {
        if (TRUE == CamX::ImageFormatUtils::IsYUV(&imageFormat))
        {
            *pPlaneStride = imageFormat.formatParams.yuvFormat[0].planeStride;
            *pSliceHeight = imageFormat.formatParams.yuvFormat[0].sliceHeight;
        }
        else if (TRUE == CamX::ImageFormatUtils::IsRAW(&imageFormat))
        {
            *pPlaneStride = imageFormat.formatParams.rawFormat.stride;
            *pSliceHeight = imageFormat.formatParams.rawFormat.sliceHeight;
        }
        else
        {
            *pPlaneStride = 0;
            *pSliceHeight = 0;
        }
    }

    const CamX::ImageFormat* pImgFormat = &imageFormat;

    imageSize = CamX::ImageFormatUtils::GetTotalSize(pImgFormat);

    return imageSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CamxMemGetImageSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC size_t CamxMemGetImageSize(
    uint32_t       width,
    uint32_t       height,
    uint32_t       format,
    uint32_t       usage)
{
    return CamxMemGetImageSizeStride(width, height, format, usage, NULL, NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CamxMemGetTotalPlaneSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC size_t CamxMemGetTotalPlaneSize(
    uint32_t       width,
    uint32_t       height,
    uint32_t       format,
    uint32_t       usage)
{
    CamxMemResult     result      = CamxMemSuccess;
    CamX::ImageFormat imageFormat = { 0 };
    size_t            imageSize   = 0;

    result = GetImageFormat(width, height, format, usage, &imageFormat);

    const CamX::ImageFormat* pImageFormat   = &imageFormat;
    UINT                     numberOfPlanes = CamX::ImageFormatUtils::GetNumberOfPlanes(pImageFormat);

    for (UINT planeIndex = 0; planeIndex < numberOfPlanes; planeIndex++)
    {
        imageSize += CamX::ImageFormatUtils::GetPlaneSize(pImageFormat, planeIndex);
    }

    return imageSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CamxMemAlloc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC int32_t CamxMemAlloc(
    CamxMemHandle* phCamxMem,
    uint32_t       width,
    uint32_t       height,
    uint32_t       format,
    uint32_t       usageFlags)
{
    CAMX_UNREFERENCED_PARAM(usageFlags);

    CamxMemResult result     = CamxMemSuccess;
    CamX::Format camxFormat  = CamX::Format::RawPlain16;

    if (format == HAL_PIXEL_FORMAT_YCbCr_420_888 ||
        format == HAL_PIXEL_FORMAT_YCrCb_420_SP)
    {
        camxFormat = CamX::Format::YUV420NV12;
    }
    else if (format == HAL_PIXEL_FORMAT_RAW16)
    {
        camxFormat = CamX::Format::RawPlain16;
    }
    else if (format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
    {
        camxFormat = GetCamXFormatFromHALFormat();
    }
    else if (format == HAL_PIXEL_FORMAT_BLOB)
    {
        camxFormat = CamX::Format::Blob;
    }
    else
    {
        result = CamxMemFailed;
    }

    /// @todo (CAMX-1441) Add support for other Raw formats if needed

    if (CamxMemSuccess == result)
    {
        const UINT ImageFormatParamsSizeBytes = sizeof(CamX::FormatParams);
        const UINT BufferAddressSizeBytes     = 8;
        const UINT NativeHandleDataSizeBytes  = ImageFormatParamsSizeBytes + BufferAddressSizeBytes;

        size_t nativeHandleSize = sizeof(native_handle_t) + NativeHandleDataSizeBytes;

        // creating a new instance of native_handle for this buffer
        native_handle_t* phBuffer = reinterpret_cast<native_handle_t*>(CAMX_CALLOC(nativeHandleSize));
        if (NULL == phBuffer)
        {
            result = CamxMemFailed;
        }

        if (CamxMemSuccess == result)
        {
            phBuffer->version = sizeof(native_handle_t);
            phBuffer->numFds  = 0;
            phBuffer->numInts = 2;

            UINT* pImageBufferParams = reinterpret_cast<UINT*>(phBuffer + 1);
            // Contents of driver struct ImageFormat

            pImageBufferParams++; // Skip buffer address
            pImageBufferParams++; // Skip buffer address

            CamX::ImageFormat*    pImageFormat    = reinterpret_cast<CamX::ImageFormat*>(pImageBufferParams);
            CamX::FormatParamInfo formatParamInfo = {0};

            pImageFormat->width      = width;
            pImageFormat->height     = height;
            pImageFormat->format     = camxFormat;
            pImageFormat->colorSpace = CamX::ColorSpace::BT601Full;
            pImageFormat->rotation   = CamX::Rotation::CW0Degrees;

            CamX::ImageFormatUtils::InitializeFormatParams(pImageFormat, &formatParamInfo);

            size_t bufferSize = CamX::ImageFormatUtils::GetTotalSize(pImageFormat);

            intptr_t temp = reinterpret_cast<intptr_t>(CAMX_CALLOC_ALIGNED(bufferSize,
                                                       static_cast<UINT32>(pImageFormat->alignment)));
            *reinterpret_cast<intptr_t*>(&(phBuffer->data[0])) = temp;

            if (CamxMemSuccess == result)
            {
                *phCamxMem          = reinterpret_cast<_CamxMemHandle*>(CAMX_CALLOC(sizeof(CamxMemHandle)));
                if (NULL != (*phCamxMem))
                {
                    (*phCamxMem)->pData = reinterpret_cast<VOID*>(phBuffer);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CamxMemRelease
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_VISIBILITY_PUBLIC void CamxMemRelease(
    CamxMemHandle hCamxMem)
{
    native_handle_t* phStreamBuffer = reinterpret_cast<native_handle_t*>(hCamxMem->pData);
    if (NULL != phStreamBuffer)
    {
        CAMX_FREE(reinterpret_cast<VOID*>(*reinterpret_cast<INTPTR_T*>(phStreamBuffer->data)));
        CAMX_FREE(phStreamBuffer);
        CAMX_FREE(hCamxMem);
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus
