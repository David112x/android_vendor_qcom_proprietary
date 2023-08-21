////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxformatutilexternal.cpp
/// @brief External Image Format Utility
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <fcntl.h>
#include <mutex>
#include <unistd.h>

#define LOG_TAG "Camx"

#include "camxdebugprint.h"
#include "camxdefs.h"
#include "camxformatutilexternal.h"
#include "camxosutils.h"
#include "camxtypes.h"
#include "g_formatmapper.h"
#include "log/log.h"
#include <media/msm_media_info.h>

// NOWHINE FILE PR007b: Whiner incorrectly concludes as non-library files
// NOWHINE FILE GR017: Used intrinsic data types as this header is exposed to non-camera components
// NOWHINE FILE PR009: '/' used in include for log header file
// NOWHINE FILE PR008:  include related warning <>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Namespaces
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constant Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const CHAR* SOCDeviceName            = "/sys/devices/soc0/soc_id"; ///< SOC filename from which the SocId must be queried
static const UINT  ChipsetBufferSize        = 32;                         ///< Size of the chipset buffer
static const UINT  CameraTitanSocInvalid    = 0;                          ///< Invalid SOC Id
static const UINT  CameraTitanSocSDM845     = 321;                        ///< SDM845 SOC Id
static const UINT  CameraTitanSocSDM670     = 336;                        ///< SDM670 SOC Id
static const UINT  CameraTitanSocSDM855     = 339;                        ///< SDM855 SOC Id
static const UINT  CameraTitanSocSDM855P    = 361;                        ///< SDM855P SOC Id
static const UINT  CameraTitanSocQCS605     = 347;                        ///< QCS605 SOC Id
static const UINT  CameraTitanSocSM6150     = 355;                        ///< SM6150 SOC Id
static const UINT  CameraTitanSocSM7150     = 365;                        ///< SM7150 SOC Id
static const UINT  CameraTitanSocSM7150P    = 366;                        ///< SM7150P SOC Id
static const UINT  CameraTitanSocSDM710     = 360;                        ///< SDM710 SOC Id
static const UINT  CameraTitanSocSXR1120    = 370;                        ///< SXR1120 SOC Id
static const UINT  CameraTitanSocSXR1130    = 371;                        ///< SXR1130 SOC Id
static const UINT  CameraTitanSocSDM712     = 393;                        ///< SDM712 SOC Id
static const UINT  CameraTitanSocSDM865     = 356;                        ///< SDM865 SOC Id
static const UINT  CameraTitanSocSM7250     = 400;                        ///< SDM7250 SOC Id
static const UINT  CameraTitanSocQSM7250    = 440;                        ///< QSM7250 SOC Id
static const UINT  CameraTitanSocSM6350     = 434;                        ///< SM6350 SOC Id
static const UINT  CameraTitanSocSM7225     = 459;                        ///< SM6350 AB SOC Id
static const UINT  CameraTitanSocKamorta    = 417;                        ///< Kamorta SOC Id
static const UINT  MaxUbwcFormats           = 5;                          ///< Maximum number of UBWC formats
static const UINT  DefaultStrideAlign       = 64;                         ///< Default stride alignment value
static const UINT  DefaultScanlineAlign     = 64;                         ///< Default scanline alignment value
static const UINT  DefaultPlaneAlignment    = 4096;                       ///< Default plane size alignment value


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Camera Format Capability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// @brief Camera Capability
struct CamxExternalFormatCaps
{
    bool        m_isInitialized;           ///< Indicate whether the structure is initialized
    UINT        m_socId;                   ///< Camera SOC Id
    std::mutex  m_lock;                    ///< Lock to protect concurrent initialization
    bool        m_isUbwcFormatSupported;   ///< Checks if UBWC format is supported
    UINT        m_ubwcFormatCount;         ///< Number of UBWC formats

    ////// @brief UBWC format capability
    struct UBWCFormatCaps
    {
        CamxPixelFormat   ubwcFormat;          ///< Name of the pixel format
        bool              isSupported;         ///< UBWC is supported or not
        UINT              version;             ///< Supported UBWC version
        bool              isLossySupported;    ///< Supported UBWC lossy formats or not
    } m_ubwcFormatCaps[MaxUbwcFormats];
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetChipsetVersion
///
/// @brief  Gets the chipset version for the camera
///
/// @return UINT Chipset version
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT GetChipsetVersion()
{
    INT32  socFd;
    CHAR   buf[ChipsetBufferSize] = { 0 };
    UINT   chipsetVersion         = -1;
    INT    ret                    = 0;

    socFd = open(SOCDeviceName, O_RDONLY);

    if (0 < socFd)
    {
        ret = read(socFd, buf, sizeof(buf) - 1);

        if (-1 == ret)
        {
            ALOGE("[CAMX_EXT_FORMAT_LIB] Unable to read soc_id");
        }
        else
        {
            chipsetVersion = atoi(buf);
        }

        close(socFd);
    }

    return chipsetVersion;
}

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
/// GetIndexForCameraExternalImageFormat
///
/// @brief  Get the index for a given Pixel Format.
///
/// @param  format    Pixel Format.
///
/// @return Index to the mapped entry. (-1 means not available)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static INT32 GetIndexForCameraExternalImageFormat(
    CamxPixelFormat format)
{
    UINT count = CAMX_ARRAY_SIZE(mapperUtil_formats);
    for (UINT formatIndex = 0; formatIndex < count; ++formatIndex)
    {
        if (format == mapperUtil_formats[formatIndex].format)
        {
            return formatIndex;
        }
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// InitializeCameraCaps
///
/// @brief  Initializes the camera capability
///
/// @param  rSOCCaps Camera capability for a particular SOC
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID InitializeCameraCaps(
    CamxExternalFormatCaps& rSOCCaps)
{
    CamxExternalFormatCaps::UBWCFormatCaps* pUbwcFormatCaps;

    rSOCCaps.m_socId = GetChipsetVersion();

    switch (rSOCCaps.m_socId)
    {
        case CameraTitanSocSDM855:
        case CameraTitanSocSDM855P:
            rSOCCaps.m_isUbwcFormatSupported   = true;
            rSOCCaps.m_ubwcFormatCount         = 3;

            pUbwcFormatCaps                   = &rSOCCaps.m_ubwcFormatCaps[0];
            pUbwcFormatCaps->ubwcFormat       = CAMERA_PIXEL_FORMAT_YCbCr_420_SP_UBWC;
            pUbwcFormatCaps->version          = 3;
            pUbwcFormatCaps->isLossySupported = false;
            pUbwcFormatCaps->isSupported      = true;

            pUbwcFormatCaps                   = &rSOCCaps.m_ubwcFormatCaps[1];
            pUbwcFormatCaps->ubwcFormat       = CAMERA_PIXEL_FORMAT_YCbCr_420_P010_UBWC;
            pUbwcFormatCaps->version          = 3;
            pUbwcFormatCaps->isLossySupported = false;
            pUbwcFormatCaps->isSupported      = true;

            pUbwcFormatCaps                   = &rSOCCaps.m_ubwcFormatCaps[2];
            pUbwcFormatCaps->ubwcFormat       = CAMERA_PIXEL_FORMAT_YCbCr_420_TP10_UBWC;
            pUbwcFormatCaps->version          = 3;
            pUbwcFormatCaps->isLossySupported = false;
            pUbwcFormatCaps->isSupported      = true;
            break;

        case CameraTitanSocInvalid:
            rSOCCaps.m_isUbwcFormatSupported   = false;
            rSOCCaps.m_ubwcFormatCount         = 0;
            break;

        case CameraTitanSocSDM865:
            rSOCCaps.m_isUbwcFormatSupported   = true;
            rSOCCaps.m_ubwcFormatCount         = 3;

            pUbwcFormatCaps                   = &rSOCCaps.m_ubwcFormatCaps[0];
            pUbwcFormatCaps->ubwcFormat       = CAMERA_PIXEL_FORMAT_YCbCr_420_SP_UBWC;
            pUbwcFormatCaps->version          = 4;
            pUbwcFormatCaps->isLossySupported = true;
            pUbwcFormatCaps->isSupported      = true;

            pUbwcFormatCaps                   = &rSOCCaps.m_ubwcFormatCaps[1];
            pUbwcFormatCaps->ubwcFormat       = CAMERA_PIXEL_FORMAT_YCbCr_420_P010_UBWC;
            pUbwcFormatCaps->version          = 4;
            pUbwcFormatCaps->isLossySupported = true;
            pUbwcFormatCaps->isSupported      = true;

            pUbwcFormatCaps                   = &rSOCCaps.m_ubwcFormatCaps[2];
            pUbwcFormatCaps->ubwcFormat       = CAMERA_PIXEL_FORMAT_YCbCr_420_TP10_UBWC;
            pUbwcFormatCaps->version          = 4;
            pUbwcFormatCaps->isLossySupported = true;
            pUbwcFormatCaps->isSupported      = true;
            break;

        default:
            rSOCCaps.m_isUbwcFormatSupported   = true;
            rSOCCaps.m_ubwcFormatCount         = 3;

            pUbwcFormatCaps                   = &rSOCCaps.m_ubwcFormatCaps[0];
            pUbwcFormatCaps->ubwcFormat       = CAMERA_PIXEL_FORMAT_YCbCr_420_SP_UBWC;
            pUbwcFormatCaps->version          = 2;
            pUbwcFormatCaps->isLossySupported = false;
            pUbwcFormatCaps->isSupported      = true;

            pUbwcFormatCaps                   = &rSOCCaps.m_ubwcFormatCaps[1];
            pUbwcFormatCaps->ubwcFormat       = CAMERA_PIXEL_FORMAT_YCbCr_420_P010_UBWC;
            pUbwcFormatCaps->version          = 2;
            pUbwcFormatCaps->isLossySupported = false;
            pUbwcFormatCaps->isSupported      = true;

            pUbwcFormatCaps                   = &rSOCCaps.m_ubwcFormatCaps[2];
            pUbwcFormatCaps->ubwcFormat       = CAMERA_PIXEL_FORMAT_YCbCr_420_TP10_UBWC;
            pUbwcFormatCaps->version          = 2;
            pUbwcFormatCaps->isLossySupported = false;
            pUbwcFormatCaps->isSupported      = true;
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxExternalFormatCaps& GetInstance()
{
    static CamxExternalFormatCaps s_socCaps;

    std::lock_guard<std::mutex> guard(s_socCaps.m_lock);

    if (!s_socCaps.m_isInitialized)
    {
        InitializeCameraCaps(s_socCaps);

        s_socCaps.m_isInitialized = true;
    }

    return s_socCaps;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneTypes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetPlaneTypes(
    CamxPixelFormat pixelFormat,
    CamxPlaneType*  pPlanetypesArray,
    int*            pPlanetypeCount)
{
    INT32            index   = GetIndexForCameraExternalImageFormat(pixelFormat);
    CamxFormatResult result  = CamxFormatResultEFailed;

    if (NULL != pPlanetypesArray)
    {
        switch (pixelFormat)
        {
            case CAMERA_PIXEL_FORMAT_NV21_ZSL:
            case CAMERA_PIXEL_FORMAT_YUV_FLEX:
            case CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX:
                    pPlanetypesArray[0] = CAMERA_PLANE_TYPE_Y;
                    pPlanetypesArray[1] = CAMERA_PLANE_TYPE_UV;
                    *pPlanetypeCount    = 2;
                    result              = CamxFormatResultSuccess;
                break;

            case CAMERA_PIXEL_FORMAT_UBWC_FLEX:
            {
                if ((-1 != index) && (0 == mapperUtil_formats[index].isPrivateFormat))
                {
                    pPlanetypesArray[0] = CAMERA_PLANE_TYPE_Y;
                    pPlanetypesArray[1] = CAMERA_PLANE_TYPE_UV;
                    pPlanetypesArray[2] = CAMERA_PLANE_TYPE_META_Y;
                    pPlanetypesArray[3] = CAMERA_PLANE_TYPE_META_VU;
                    *pPlanetypeCount    = 4;
                }
                else
                {
                    pPlanetypesArray[0] = CAMERA_PLANE_TYPE_Y;
                    pPlanetypesArray[1] = CAMERA_PLANE_TYPE_UV;
                    *pPlanetypeCount    = 2;
                }

                result = CamxFormatResultSuccess;
            }
            break;

            case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
            case CAMERA_PIXEL_FORMAT_RAW10:
            case CAMERA_PIXEL_FORMAT_RAW12:
                pPlanetypesArray[0] = CAMERA_PLANE_TYPE_RAW;
                *pPlanetypeCount    = 1;
                result              = CamxFormatResultSuccess;
                break;

            default:
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetScanlineForFlexFormats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetScanlineForFlexFormats(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int             height,
    int             index,
    int*            pScanline)
{
    INT32 venusFormat   = -1;

    if (0 == mapperUtil_formats[index].isPrivateFormat)
    {
        if (CAMERA_PIXEL_FORMAT_YUV_FLEX == pixelFormat)
        {
            venusFormat = COLOR_FMT_NV12;
        }
        else if (CAMERA_PIXEL_FORMAT_UBWC_FLEX == pixelFormat)
        {
            venusFormat = COLOR_FMT_NV12_UBWC;
        }
    }

    if (-1 != venusFormat)
    {
        if (CAMERA_PLANE_TYPE_Y == planeType)
        {
            *pScanline = VENUS_Y_SCANLINES(venusFormat, height);
        }
        else if (CAMERA_PLANE_TYPE_UV == planeType)
        {
            *pScanline = VENUS_UV_SCANLINES(venusFormat, height);
        }
        else if (CAMERA_PLANE_TYPE_META_Y == planeType)
        {
            *pScanline = VENUS_Y_META_SCANLINES(venusFormat, height);
        }
        else if (CAMERA_PLANE_TYPE_META_VU == planeType)
        {
            *pScanline = VENUS_UV_META_SCANLINES(venusFormat, height);
        }
    }
    else
    {
        if (CAMERA_PLANE_TYPE_Y == planeType)
        {
            *pScanline = CamX::Utils::AlignGeneric32(height, mapperUtil_formats[index].scanLineAlign);
        }
        else if (CAMERA_PLANE_TYPE_UV == planeType)
        {
            *pScanline = CamX::Utils::AlignGeneric32((height >> 1), mapperUtil_formats[index].scanLineAlign);
        }
    }

    ALOGV("%s h %d, scanline %d, planeType %d, venus Format %d, pixelFormat %d ,private %d, index %d, align %d",
          __FUNCTION__, height, *pScanline, planeType, venusFormat,
          pixelFormat, mapperUtil_formats[index].isPrivateFormat, index,
          mapperUtil_formats[index].scanLineAlign);
    return CamxFormatResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetStrideForFlexFormats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetStrideForFlexFormats(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int             width,
    int             index,
    int*            pStride)
{
    INT32 venusFormat = -1;

    if (0 == mapperUtil_formats[index].isPrivateFormat)
    {
        if (CAMERA_PIXEL_FORMAT_YUV_FLEX == pixelFormat)
        {
            venusFormat = COLOR_FMT_NV12;
        }
        else if (CAMERA_PIXEL_FORMAT_UBWC_FLEX == pixelFormat)
        {
            venusFormat = COLOR_FMT_NV12_UBWC;
        }
    }

    if (-1 != venusFormat)
    {
        if (CAMERA_PLANE_TYPE_Y == planeType)
        {
            *pStride = VENUS_Y_STRIDE(venusFormat, width);
        }
        else if (CAMERA_PLANE_TYPE_UV == planeType)
        {
            *pStride = VENUS_UV_STRIDE(venusFormat, width);
        }
        else if (CAMERA_PLANE_TYPE_META_Y == planeType)
        {
            *pStride = VENUS_Y_META_STRIDE(venusFormat, width);
        }
        else if (CAMERA_PLANE_TYPE_META_VU == planeType)
        {
            *pStride = VENUS_UV_META_STRIDE(venusFormat, width);
        }
    }
    else
    {
        *pStride = CamX::Utils::AlignGeneric32(width, mapperUtil_formats[index].strideAlign);
    }

   ALOGV("%s called,  w %d, stride %d, planeType %d, venudFormat %d, pixelFormat %d ,private %d, index %d, align %d",
        __FUNCTION__, width, *pStride, planeType, venusFormat,
        pixelFormat, mapperUtil_formats[index].isPrivateFormat, index,
        mapperUtil_formats[index].strideAlign);
    return CamxFormatResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneLayoutInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetPlaneLayoutInfo(
    CamxPixelFormat       pixelFormat,
    CamxPlaneType         planeType,
    int                   width,
    int                   height,
    CamxPlaneLayoutInfo*  pPlaneInfo)
{
    CamxFormatResult        result  = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSocCaps = GetInstance();

    if ((NULL != pPlaneInfo) && (0 < width))
    {
        INT32 index           = GetIndexForCameraExternalImageFormat(pixelFormat);
        // Default value unless mentioned in xml
        INT32 strideAlign     = DefaultStrideAlign;
        INT32 scanLineAlign   = DefaultScanlineAlign;
        pPlaneInfo->alignment = DefaultPlaneAlignment;

        if (-1 != index)
        {
            strideAlign           = mapperUtil_formats[index].strideAlign;
            scanLineAlign         = mapperUtil_formats[index].scanLineAlign;
            pPlaneInfo->alignment = mapperUtil_formats[index].planeAlign;
        }

        ALOGV("%s called, index %d, pixelFormat %d, planeType %d, width %d,  height %d def stride %d, scanline %d, align %d",
              __FUNCTION__, index, pixelFormat,  planeType, width, height,
              strideAlign,  scanLineAlign,  pPlaneInfo->alignment);
        switch (pixelFormat)
        {
            case CAMERA_PIXEL_FORMAT_NV21_ZSL:
            {
                if (CAMERA_PLANE_TYPE_Y == planeType)
                {
                    switch (rSocCaps.m_socId)
                    {
                        default:
                            pPlaneInfo->stride    = CamX::Utils::AlignGeneric32(width, strideAlign);
                            pPlaneInfo->scanline  = CamX::Utils::AlignGeneric32(height, scanLineAlign);
                            pPlaneInfo->planeSize = PadToSize(pPlaneInfo->stride * pPlaneInfo->scanline,
                                                              pPlaneInfo->alignment);
                           ALOGV("%s called,  Y wXh %dx%d, strideXscanline %dx%d, size %d",
                                 __FUNCTION__, width, height, pPlaneInfo->stride,
                                 pPlaneInfo->scanline,  pPlaneInfo->planeSize);
                            result                = CamxFormatResultSuccess;
                            break;
                    }
                }
                else if (CAMERA_PLANE_TYPE_UV == planeType)
                {
                    switch (rSocCaps.m_socId)
                    {
                        default:
                            pPlaneInfo->stride    = CamX::Utils::AlignGeneric32(width, strideAlign);
                            pPlaneInfo->scanline  = CamX::Utils::AlignGeneric32((height >> 1), scanLineAlign);
                            pPlaneInfo->planeSize = PadToSize(pPlaneInfo->stride * pPlaneInfo->scanline,
                                                              pPlaneInfo->alignment);
                           ALOGV("%s called,  UV wXh %dx%d, strideXscanline %dx%d, size %d",
                                 __FUNCTION__, width, (height >> 1), pPlaneInfo->stride,
                                 pPlaneInfo->scanline,  pPlaneInfo->planeSize);
                            result                = CamxFormatResultSuccess;
                            break;
                    }
                }
            }
            break;

            case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
            case CAMERA_PIXEL_FORMAT_RAW10:
            {
                if (CameraTitanSocKamorta != rSocCaps.m_socId)
                {
                    pPlaneInfo->stride    = CamX::Utils::AlignGeneric32((width * 10 / 8), 16);
                }
                else
                {
                    pPlaneInfo->stride    = CamX::Utils::AlignGeneric32((width * 10 / 8), 80);
                }
                pPlaneInfo->scanline  = height;
                pPlaneInfo->planeSize = PadToSize(pPlaneInfo->stride * pPlaneInfo->scanline,
                                                  pPlaneInfo->alignment);
                result                = CamxFormatResultSuccess;

            }
            break;

            case CAMERA_PIXEL_FORMAT_RAW12:
            {
                if (CameraTitanSocKamorta != rSocCaps.m_socId)
                {
                    pPlaneInfo->stride    = CamX::Utils::AlignGeneric32((width * 12 / 8), 16);
                }
                else
                {
                    pPlaneInfo->stride    = CamX::Utils::AlignGeneric32((width * 12 / 8), 48);
                }
                pPlaneInfo->scanline  = height;
                pPlaneInfo->planeSize = PadToSize(pPlaneInfo->stride * pPlaneInfo->scanline,
                                                  pPlaneInfo->alignment);
                result                = CamxFormatResultSuccess;

            }
            break;

            default:
            {
                if (-1 != index)
                {
                    CamxFormatUtil_GetScanlineForFlexFormats(pixelFormat, planeType, height,
                                                             index, &pPlaneInfo->scanline);
                    CamxFormatUtil_GetStrideForFlexFormats(pixelFormat, planeType, width,
                                                           index, &pPlaneInfo->stride);

                    pPlaneInfo->planeSize = PadToSize(pPlaneInfo->stride * pPlaneInfo->scanline,
                                                      pPlaneInfo->alignment);

                    result = CamxFormatResultSuccess;
                }
            }
            break;
        }

        ALOGV("%s called, wXh %dx%d, strideXscanline %dx%d, format %d, planeType %d, size %d",
              __FUNCTION__, width, height, pPlaneInfo->stride, pPlaneInfo->scanline, pixelFormat, planeType,
              pPlaneInfo->planeSize);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetStrideInBytes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetStrideInBytes(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int             width,
    int*            pStride)
{
    CamxFormatResult        result   = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if ((NULL != pStride) && (0 < width))
    {
        INT32 index       = GetIndexForCameraExternalImageFormat(pixelFormat);
        INT32 strideAlign = DefaultStrideAlign;

        if (-1 != index)
        {
            strideAlign = mapperUtil_formats[index].strideAlign;
        }

        ALOGV("%s called,  w %d, stride align %d, planeType %d, pixelFormat %d index %d",
              __FUNCTION__, width, strideAlign, planeType, pixelFormat, index);
        switch (pixelFormat)
        {
            case CAMERA_PIXEL_FORMAT_NV21_ZSL:
                {
                    if (CAMERA_PLANE_TYPE_Y == planeType)
                    {
                        switch (rSOCCaps.m_socId)
                        {
                            default:
                                *pStride = CamX::Utils::AlignGeneric32(width, strideAlign);
                                result   = CamxFormatResultSuccess;
                                break;
                        }
                    }
                    else if (CAMERA_PLANE_TYPE_UV == planeType)
                    {
                        switch (rSOCCaps.m_socId)
                        {
                            default:
                                *pStride = CamX::Utils::AlignGeneric32(width, strideAlign);
                                result   = CamxFormatResultSuccess;
                                break;
                        }
                    }
                }
                break;

            case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
            case CAMERA_PIXEL_FORMAT_RAW10:
                if (CameraTitanSocKamorta != rSOCCaps.m_socId)
                {
                    *pStride = CamX::Utils::AlignGeneric32((width * 10 / 8), 16);
                }
                else
                {
                    *pStride = CamX::Utils::AlignGeneric32((width * 10 / 8), 80);
                }
                result   = CamxFormatResultSuccess;
                break;
            case CAMERA_PIXEL_FORMAT_RAW12:
                if (CameraTitanSocKamorta != rSOCCaps.m_socId)
                {
                    *pStride = CamX::Utils::AlignGeneric32((width * 12 / 8), 16);
                }
                else
                {
                    *pStride = CamX::Utils::AlignGeneric32((width * 12 / 8), 48);
                }
                result   = CamxFormatResultSuccess;
                break;

            default:
                {
                    if (-1 != index)
                    {
                        result = CamxFormatUtil_GetStrideForFlexFormats(pixelFormat, planeType,
                                                                        width, index, pStride);
                    }
                }
                break;
        }
        ALOGV("%s called,  w %d, stride align %d, planeType %d, pixelFormat %d index %d, stride %d",
              __FUNCTION__, width, strideAlign, planeType, pixelFormat, index, *pStride);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetStrideInPixels
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetStrideInPixels(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int             width,
    float*          pStride)
{
    CamxFormatResult        result   = CamxFormatResultEUnsupported;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if ((NULL != pStride) && (0 < width))
    {
        switch (pixelFormat)
        {
            case CAMERA_PIXEL_FORMAT_NV21_ZSL:
                {
                    if ((CAMERA_PLANE_TYPE_Y == planeType) || (CAMERA_PLANE_TYPE_UV == planeType))
                    {
                       switch (rSOCCaps.m_socId)
                       {
                           default:
                               {
                                   INT strideInBytes;
                                   INT bpp;

                                   result = CamxFormatUtil_GetPerPlaneBpp(pixelFormat, planeType, &bpp);

                                   if (CamxFormatResultSuccess == result)
                                   {
                                       result = CamxFormatUtil_GetStrideInBytes(pixelFormat, planeType, width, &strideInBytes);
                                   }

                                   if (CamxFormatResultSuccess == result)
                                   {
                                       *pStride = strideInBytes * bpp / 8.0f;
                                   }
                               }
                               break;
                        }
                    }
                }
                break;

            case CAMERA_PIXEL_FORMAT_YUV_FLEX:
            case CAMERA_PIXEL_FORMAT_UBWC_FLEX:
            case CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX:
            case CAMERA_PIXEL_FORMAT_RAW10:
            case CAMERA_PIXEL_FORMAT_RAW12:
                // In Gralloc, width is used as strideinPixels value. If needed revisit this logic
                *pStride = width;
                result   = CamxFormatResultSuccess;
                break;
            case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
                {
                    INT strideInBytes = 0;

                    result = CamxFormatUtil_GetStrideInBytes(pixelFormat, planeType, width, &strideInBytes);

                    if (CamxFormatResultSuccess == result)
                    {
                        *pStride = strideInBytes;
                    }
                }
                break;

            default:
                break;
        }
    }

        ALOGV("%s called,  w %d, planeType %d, pixelFormat %d stride %f",
              __FUNCTION__, width, planeType, pixelFormat, *pStride);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetScanline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetScanline(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int             height,
    int*            pScanline)
{
    CamxFormatResult        result   = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if ((NULL != pScanline) && (0 < height))
    {
        INT32 index         = GetIndexForCameraExternalImageFormat(pixelFormat);
        INT32 scanLineAlign = DefaultScanlineAlign;

        if (-1 != index)
        {
            scanLineAlign = mapperUtil_formats[index].scanLineAlign;
        }

        ALOGV("%s called,  h %d, scanline align %d, planeType %d, pixelFormat %d index %d",
        __FUNCTION__, height, scanLineAlign, planeType, pixelFormat, index);
        switch (pixelFormat)
        {
            case CAMERA_PIXEL_FORMAT_NV21_ZSL:
                {
                    if (CAMERA_PLANE_TYPE_Y == planeType)
                    {
                        switch (rSOCCaps.m_socId)
                        {
                            default:
                                *pScanline = CamX::Utils::AlignGeneric32(height, scanLineAlign);
                                result     = CamxFormatResultSuccess;
                                break;
                        }
                    }
                    else if (CAMERA_PLANE_TYPE_UV == planeType)
                    {
                        switch (rSOCCaps.m_socId)
                        {
                            default:
                                *pScanline = CamX::Utils::AlignGeneric32((height >> 1), scanLineAlign);
                                result     = CamxFormatResultSuccess;
                                break;
                        }
                    }
                }
                break;

           case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
           case CAMERA_PIXEL_FORMAT_RAW10:
           case CAMERA_PIXEL_FORMAT_RAW12:
               *pScanline = height;
               result     = CamxFormatResultSuccess;
               break;

            default:
                {
                    if (-1 != index)
                    {
                        result = CamxFormatUtil_GetScanlineForFlexFormats(pixelFormat, planeType,
                                                                          height, index, pScanline);
                    }
                }
                break;
        }
        ALOGV("%s called,  h %d, scanline align %d, planeType %d, pixelFormat %d index %d, scaline %d",
              __FUNCTION__, height, scanLineAlign, planeType, pixelFormat, index, *pScanline);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetPlaneSize(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int             width,
    int             height,
    unsigned int*   pAlignedSize)
{
    CamxFormatResult        result  = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    INT32 stride   = 0;
    INT32 scanline = 0;
    UINT  alignment;

    if (NULL != pAlignedSize)
    {
        result = CamxFormatUtil_GetStrideInBytes(pixelFormat, planeType, width, &stride);

        if (CamxFormatResultSuccess == result)
        {
            result = CamxFormatUtil_GetScanline(pixelFormat, planeType, height, &scanline);
        }

        if (CamxFormatResultSuccess == result)
        {
            result = CamxFormatUtil_GetPlaneAlignment(pixelFormat, planeType, &alignment);
        }

        if (CamxFormatResultSuccess == result)
        {
            switch (pixelFormat)
            {
               case CAMERA_PIXEL_FORMAT_NV21_ZSL:
               case CAMERA_PIXEL_FORMAT_YUV_FLEX:
               case CAMERA_PIXEL_FORMAT_UBWC_FLEX:
               case CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX:
                   *pAlignedSize = PadToSize(stride * scanline, alignment);
                   break;

               default:
                   *pAlignedSize = stride * scanline;
                   break;
            }
        }
    }

    ALOGV("%s  called, wXh %dx%d, strideXscanline %dx%d, format %d, planeType %d",
          __FUNCTION__, width, height, stride, scanline, pixelFormat, planeType);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetBufferSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetBufferSize(
    CamxPixelFormat pixelFormat,
    int             width,
    int             height,
    unsigned int*   pBufferSize)
{
    CamxFormatResult result = CamxFormatResultEFailed;

    CamxPlaneType planeTypes[CamxFormatUtilMaxNumPlanes] = {};
    INT           planeCount                             = 0;
    UINT          bufferSize                             = 0;

    if (NULL != pBufferSize)
    {
        INT32  index       = GetIndexForCameraExternalImageFormat(pixelFormat);
        INT32  venusFormat = -1;
        UINT32 bufCount    = 1;

        if (-1 != index)
        {
            bufCount = mapperUtil_formats[index].batchCount;

            if (0 == mapperUtil_formats[index].isPrivateFormat)
            {
                if (CAMERA_PIXEL_FORMAT_YUV_FLEX == pixelFormat)
                {
                    venusFormat = COLOR_FMT_NV12;
                }
                else if (CAMERA_PIXEL_FORMAT_UBWC_FLEX == pixelFormat)
                {
                    venusFormat = COLOR_FMT_NV12_UBWC;
                }
            }
        }

        if (-1 != venusFormat)
        {
            *pBufferSize = (VENUS_BUFFER_SIZE(venusFormat, width, height) * bufCount);
            result       = CamxFormatResultSuccess;
        }
        else
        {
            result = CamxFormatUtil_GetPlaneTypes(pixelFormat, planeTypes, &planeCount);

            if ((CamxFormatResultSuccess == result) && (0 < planeCount))
            {
                for (INT index = 0; index < planeCount; ++index)
                {
                    UINT planeSize = 0;

                    result =  CamxFormatUtil_GetPlaneSize(pixelFormat, planeTypes[index],
                                                          width, height, &planeSize);

                    if (CamxFormatResultSuccess == result)
                    {
                        bufferSize += planeSize;
                    }
                }

                if (CamxFormatResultSuccess == result)
                {
                    bufferSize = PadToSize(bufferSize, DefaultPlaneAlignment);
                    *pBufferSize = (bufferSize * bufCount);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneAlignment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetPlaneAlignment(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    unsigned int*   pAlignment)
{
    CamxFormatResult        result  = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if (NULL != pAlignment)
    {
        INT32 index = GetIndexForCameraExternalImageFormat(pixelFormat);

        if (-1 != index)
        {
            *pAlignment = mapperUtil_formats[index].planeAlign;
            result      = CamxFormatResultSuccess;
        }
        else
        {
            switch (pixelFormat)
            {
                case CAMERA_PIXEL_FORMAT_NV21_ZSL:
                case CAMERA_PIXEL_FORMAT_YUV_FLEX:
                case CAMERA_PIXEL_FORMAT_UBWC_FLEX:
                case CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX:
                    if ((CAMERA_PLANE_TYPE_Y  == planeType) ||
                        (CAMERA_PLANE_TYPE_UV == planeType))
                    {
                        *pAlignment = DefaultPlaneAlignment;
                        result      = CamxFormatResultSuccess;
                    }
                    break;

                case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
                case CAMERA_PIXEL_FORMAT_RAW10:
                case CAMERA_PIXEL_FORMAT_RAW12:
                    if (CAMERA_PLANE_TYPE_RAW  == planeType)
                    {
                        *pAlignment = DefaultPlaneAlignment;
                        result      = CamxFormatResultSuccess;
                    }
                    break;

                default:
                    result = CamxFormatResultEInvalidArg;
                    break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetUBWCInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetUBWCInfo(
    CamxPixelFormat pixelFormat,
    bool*           pIsSupported,
    bool*           pIsPI,
    int*            pVersion)
{
    CamxFormatResult        result  = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if ((NULL != pIsSupported) && (NULL != pIsPI) && (NULL != pVersion))
    {
        for (UINT index = 0; index < rSOCCaps.m_ubwcFormatCount; ++index)
        {
            if (rSOCCaps.m_ubwcFormatCaps[index].ubwcFormat == pixelFormat)
            {
                CamxExternalFormatCaps::UBWCFormatCaps& ubwcCaps = rSOCCaps.m_ubwcFormatCaps[index];

                *pIsSupported      = ubwcCaps.isSupported;
                *pVersion          = ubwcCaps.version;
                *pIsPI             = ubwcCaps.isLossySupported;
                result             = CamxFormatResultSuccess;
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneOffset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetPlaneOffset(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int*            pOffset,
    int             width,
    int             height)
{
    CamxFormatResult        result   = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if (NULL != pOffset)
    {
        INT index = GetIndexForCameraExternalImageFormat(pixelFormat);

        switch (pixelFormat)
        {
            case CAMERA_PIXEL_FORMAT_NV21_ZSL:
            case CAMERA_PIXEL_FORMAT_YUV_FLEX:
            case CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX:
                if (CAMERA_PLANE_TYPE_Y == planeType)
                {
                    *pOffset = 0;
                    result   = CamxFormatResultSuccess;
                }
                else if (CAMERA_PLANE_TYPE_UV == planeType)
                {
                    *pOffset = 0;
                    result   = CamxFormatResultSuccess;

                    if ((0 != height) && (0 != width))
                    {
                         unsigned int alignedSize = 0;
                         result = CamxFormatUtil_GetPlaneSize(pixelFormat, CAMERA_PLANE_TYPE_Y,
                                                              width, height, &alignedSize);
                         *pOffset = alignedSize;
                    }
                }
                break;

            case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
            case CAMERA_PIXEL_FORMAT_RAW10:
            case CAMERA_PIXEL_FORMAT_RAW12:
                if (CAMERA_PLANE_TYPE_RAW  == planeType)
                {
                    *pOffset = 0;
                    result   = CamxFormatResultSuccess;
                }
                break;

            case CAMERA_PIXEL_FORMAT_UBWC_FLEX:
                if ((-1 != index) && (0 == mapperUtil_formats[index].isPrivateFormat))
                {
                    *pOffset = 0;
                    result   = CamxFormatResultSuccess;

                    if (CAMERA_PLANE_TYPE_META_Y == planeType)
                    {
                        *pOffset = 0;
                        result   = CamxFormatResultSuccess;
                    }
                    else if (CAMERA_PLANE_TYPE_Y == planeType)
                    {
                        if ((0 != height) && (0 != width))
                        {
                             unsigned int alignedSize = 0;
                             result = CamxFormatUtil_GetPlaneSize(pixelFormat, CAMERA_PLANE_TYPE_META_Y,
                                                                  width, height, &alignedSize);
                             *pOffset = alignedSize;
                        }
                    }
                    else if (CAMERA_PLANE_TYPE_META_VU == planeType)
                    {
                        if ((0 != height) && (0 != width))
                        {
                             unsigned int alignedSize = 0;
                             result = CamxFormatUtil_GetPlaneSize(pixelFormat, CAMERA_PLANE_TYPE_META_Y,
                                                                  width, height, &alignedSize);
                             *pOffset = alignedSize;
                             result = CamxFormatUtil_GetPlaneSize(pixelFormat, CAMERA_PLANE_TYPE_Y,
                                                                  width, height, &alignedSize);
                             *pOffset += alignedSize;
                        }
                    }
                    else if (CAMERA_PLANE_TYPE_UV == planeType)
                    {
                        if ((0 != height) && (0 != width))
                        {
                             unsigned int alignedSize = 0;
                             result = CamxFormatUtil_GetPlaneSize(pixelFormat, CAMERA_PLANE_TYPE_META_Y,
                                                                  width, height, &alignedSize);
                             *pOffset = alignedSize;
                             result = CamxFormatUtil_GetPlaneSize(pixelFormat, CAMERA_PLANE_TYPE_Y,
                                                                  width, height, &alignedSize);
                             *pOffset += alignedSize;
                             result = CamxFormatUtil_GetPlaneSize(pixelFormat, CAMERA_PLANE_TYPE_META_VU,
                                                                  width, height, &alignedSize);
                             *pOffset += alignedSize;
                        }
                    }
                }
                else
                {
                    if (CAMERA_PLANE_TYPE_Y  == planeType)
                    {
                        *pOffset = 0;
                        result   = CamxFormatResultSuccess;
                    }
                    else if (CAMERA_PLANE_TYPE_UV == planeType)
                    {
                        *pOffset = 0;
                        result   = CamxFormatResultSuccess;

                        if ((0 != height) && (0 != width))
                        {
                             unsigned int alignedSize = 0;
                             result = CamxFormatUtil_GetPlaneSize(pixelFormat, CAMERA_PLANE_TYPE_Y,
                                                                  width, height, &alignedSize);
                             *pOffset = alignedSize;
                        }
                    }
                }
                break;

           default:
               break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_IsPerPlaneFdNeeded
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_IsPerPlaneFdNeeded(
    CamxPixelFormat pixelFormat,
    bool*           pIsPerPlaneFdNeeded)
{
    CamxFormatResult        result  = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if (NULL != pIsPerPlaneFdNeeded)
    {
        switch (pixelFormat)
        {
           case CAMERA_PIXEL_FORMAT_NV21_ZSL:
           case CAMERA_PIXEL_FORMAT_YUV_FLEX:
           case CAMERA_PIXEL_FORMAT_UBWC_FLEX:
           case CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX:
           case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
           case CAMERA_PIXEL_FORMAT_RAW10:
           case CAMERA_PIXEL_FORMAT_RAW12:
               *pIsPerPlaneFdNeeded = false;
               result               = CamxFormatResultSuccess;
               break;

           default:
               break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetBpp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetBpp(
    CamxPixelFormat pixelFormat,
    int*            pBpp)
{
    CamxFormatResult        result  = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if (NULL != pBpp)
    {
        switch (pixelFormat)
        {
           case CAMERA_PIXEL_FORMAT_NV21_ZSL:
           case CAMERA_PIXEL_FORMAT_YUV_FLEX:
           case CAMERA_PIXEL_FORMAT_UBWC_FLEX:
           case CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX:
           case CAMERA_PIXEL_FORMAT_RAW12:
               *pBpp  = 12;
               result = CamxFormatResultSuccess;
               break;

           case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
           case CAMERA_PIXEL_FORMAT_RAW10:
               *pBpp  = 10;
               result = CamxFormatResultSuccess;
               break;
               break;

           default:
               break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPerPlaneBpp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetPerPlaneBpp(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int*            pBpp)
{
    CamxFormatResult        result  = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if (NULL != pBpp)
    {
        switch (pixelFormat)
        {
           case CAMERA_PIXEL_FORMAT_NV21_ZSL:
           case CAMERA_PIXEL_FORMAT_YUV_FLEX:
           case CAMERA_PIXEL_FORMAT_UBWC_FLEX:
           case CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX:
               if ((CAMERA_PLANE_TYPE_Y       == planeType) ||
                   (CAMERA_PLANE_TYPE_UV      == planeType) ||
                   (CAMERA_PLANE_TYPE_META_Y  == planeType) ||
                   (CAMERA_PLANE_TYPE_META_VU == planeType))
               {
                   *pBpp  = 8;
                   result = CamxFormatResultSuccess;
               }
               break;

           case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
           case CAMERA_PIXEL_FORMAT_RAW10:
               if (CAMERA_PLANE_TYPE_RAW  == planeType)
               {
                   *pBpp  = 10;
                   result = CamxFormatResultSuccess;
               }
               break;
           case CAMERA_PIXEL_FORMAT_RAW12:
               if (CAMERA_PLANE_TYPE_RAW  == planeType)
               {
                   *pBpp  = 12;
                   result = CamxFormatResultSuccess;
               }
               break;

           default:
               break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneStartAddressAlignment
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetPlaneStartAddressAlignment(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int*            pAlignment)
{
    CamxFormatResult        result  = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if (NULL != pAlignment)
    {
        switch (pixelFormat)
        {
           case CAMERA_PIXEL_FORMAT_NV21_ZSL:
           case CAMERA_PIXEL_FORMAT_YUV_FLEX:
           case CAMERA_PIXEL_FORMAT_UBWC_FLEX:
           case CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX:
               if ((CAMERA_PLANE_TYPE_Y       == planeType) ||
                   (CAMERA_PLANE_TYPE_UV      == planeType) ||
                   (CAMERA_PLANE_TYPE_META_Y  == planeType) ||
                   (CAMERA_PLANE_TYPE_META_VU == planeType))
               {
                   *pAlignment = 1;
                   result      = CamxFormatResultSuccess;
               }
               break;

           case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
           case CAMERA_PIXEL_FORMAT_RAW10:
           case CAMERA_PIXEL_FORMAT_RAW12:
               if (CAMERA_PLANE_TYPE_RAW  == planeType)
               {
                   *pAlignment = 1;
                   result      = CamxFormatResultSuccess;
               }
               break;

           default:
               break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetSubsamplingFactor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetSubsamplingFactor(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    bool            isHorizontal,
    int*            pSubsamplingFactor)
{
    CamxFormatResult        result  = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if (NULL != pSubsamplingFactor)
    {
        INT index = GetIndexForCameraExternalImageFormat(pixelFormat);

        switch (pixelFormat)
        {
            case CAMERA_PIXEL_FORMAT_NV21_ZSL:
            case CAMERA_PIXEL_FORMAT_YUV_FLEX:
            case CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX:
            {
                if (CAMERA_PLANE_TYPE_Y  == planeType)
                {
                    *pSubsamplingFactor = 0;
                    result              = CamxFormatResultSuccess;
                }
                else if (CAMERA_PLANE_TYPE_UV == planeType)
                {
                    *pSubsamplingFactor = isHorizontal ? 1 : 2; // H2V2 formats
                    result              = CamxFormatResultSuccess;
                }
            }
            break;

            case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
            case CAMERA_PIXEL_FORMAT_RAW10:
            case CAMERA_PIXEL_FORMAT_RAW12:
            {
                if (CAMERA_PLANE_TYPE_RAW  == planeType)
                {
                    *pSubsamplingFactor = 0;
                    result              = CamxFormatResultSuccess;
                }
            }
            break;

            case CAMERA_PIXEL_FORMAT_UBWC_FLEX:
            {
                if (CAMERA_PLANE_TYPE_Y  == planeType)
                {
                    *pSubsamplingFactor = 0;
                    result              = CamxFormatResultSuccess;
                }
                else if (CAMERA_PLANE_TYPE_UV == planeType)
                {
                    *pSubsamplingFactor = isHorizontal ? 1 : 2; // H2V2 formats
                    result              = CamxFormatResultSuccess;
                }
                else if ((CAMERA_PLANE_TYPE_META_VU == planeType) ||
                         (CAMERA_PLANE_TYPE_META_Y  == planeType))
                {
                    *pSubsamplingFactor = 0;
                    result              = CamxFormatResultSuccess;
                }
            }
            break;

            default:
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPlaneCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetPlaneCount(
    CamxPixelFormat pixelFormat,
    int*            pPlaneCount)
{
    CamxFormatResult        result  = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if (NULL != pPlaneCount)
    {
        INT index = GetIndexForCameraExternalImageFormat(pixelFormat);

        switch (pixelFormat)
        {
            case CAMERA_PIXEL_FORMAT_NV21_ZSL:
            case CAMERA_PIXEL_FORMAT_YUV_FLEX:
            case CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX:
                *pPlaneCount = 2;
                result       = CamxFormatResultSuccess;
                break;

            case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
            case CAMERA_PIXEL_FORMAT_RAW10:
            case CAMERA_PIXEL_FORMAT_RAW12:
                *pPlaneCount = 1;
                result       = CamxFormatResultSuccess;
                break;

            case CAMERA_PIXEL_FORMAT_UBWC_FLEX:
            {
                if ((-1 != index) && (0 == mapperUtil_formats[index].isPrivateFormat))
                {
                    *pPlaneCount = 4;
                }
                else
                {
                    *pPlaneCount = 2;
                }

                result = CamxFormatResultSuccess;

            }
            break;

            default:
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetPixelIncrement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetPixelIncrement(
    CamxPixelFormat pixelFormat,
    CamxPlaneType   planeType,
    int*            pPixelIncrement)
{
    CamxFormatResult        result  = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if (NULL != pPixelIncrement)
    {
        switch (pixelFormat)
        {
            case CAMERA_PIXEL_FORMAT_NV21_ZSL:
            case CAMERA_PIXEL_FORMAT_YUV_FLEX:
            case CAMERA_PIXEL_FORMAT_UBWC_FLEX:
            case CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX:
                {
                    if (CAMERA_PLANE_TYPE_Y  == planeType)
                    {
                        *pPixelIncrement = 1;
                        result           = CamxFormatResultSuccess;
                    }
                    else if (CAMERA_PLANE_TYPE_UV == planeType)
                    {
                        *pPixelIncrement  = 2;
                        result            = CamxFormatResultSuccess;
                    }
                    else if ((CAMERA_PLANE_TYPE_META_Y  == planeType) ||
                             (CAMERA_PLANE_TYPE_META_VU == planeType))
                    {
                        *pPixelIncrement  = 0;
                        result            = CamxFormatResultSuccess;
                    }
                }
                break;

            case CAMERA_PIXEL_FORMAT_RAW_OPAQUE:
                {
                    if (CAMERA_PLANE_TYPE_RAW  == planeType)
                    {
                        *pPixelIncrement = 1;
                        result           = CamxFormatResultSuccess;
                    }
                }
                break;
            case CAMERA_PIXEL_FORMAT_RAW10:
            case CAMERA_PIXEL_FORMAT_RAW12:
                {
                    if (CAMERA_PLANE_TYPE_RAW  == planeType)
                    {
                        *pPixelIncrement = 0;
                        result           = CamxFormatResultSuccess;
                    }
                }
                break;

            default:
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CamxFormatUtil_GetFlexibleYUVFormats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" CamxFormatResult CamxFormatUtil_GetFlexibleYUVFormats(
    CamxFlexFormatInfo* pFlexFormatInfo)
{
    CamxFormatResult        result  = CamxFormatResultEFailed;
    CamxExternalFormatCaps& rSOCCaps = GetInstance();

    if ((NULL != pFlexFormatInfo) && (sizeof(CamxFlexFormatInfo) == pFlexFormatInfo->size))
    {
        UINT count = CAMX_ARRAY_SIZE(mapperUtil_formats);
        ALOGV("CamxFormatUtil_GetFlexibleYUVFormats array count size %d, CamxFormatUtilMaxFlexibleFormats %d",
              count, CamxFormatUtilMaxFlexibleFormats);

        if (CamxFormatUtilMaxFlexibleFormats < count)
        {
            ALOGW("%s update entries only for Max Limit", __FUNCTION__);
            count = CamxFormatUtilMaxFlexibleFormats;
        }

        for (UINT formatIndex = 0; formatIndex < count; ++formatIndex)
        {
            pFlexFormatInfo->pixelFormat[formatIndex]       = mapperUtil_formats[formatIndex].format;
            pFlexFormatInfo->strideAlignment[formatIndex]   = mapperUtil_formats[formatIndex].strideAlign;
            pFlexFormatInfo->scanlineAlignment[formatIndex] = mapperUtil_formats[formatIndex].scanLineAlign;
        }

        if (count + 1 < CamxFormatUtilMaxFlexibleFormats)
        {
            if ((CameraTitanSocSDM865  == rSOCCaps.m_socId) || (CameraTitanSocSM7250  == rSOCCaps.m_socId) ||
                (CameraTitanSocQSM7250 == rSOCCaps.m_socId) || (CameraTitanSocSM6350 == rSOCCaps.m_socId) ||
                (CameraTitanSocSM7225  == rSOCCaps.m_socId))
            {
                pFlexFormatInfo->strideAlignment[count]   = 512;
                pFlexFormatInfo->scanlineAlignment[count] = 512;
            }
            else
            {
                pFlexFormatInfo->strideAlignment[count]   = 128;
                pFlexFormatInfo->scanlineAlignment[count] = 32;
            }
            pFlexFormatInfo->pixelFormat[count] = CAMERA_PIXEL_FORMAT_NV12_VENUS;
            count++;

            pFlexFormatInfo->pixelFormat[count]       = CAMERA_PIXEL_FORMAT_NV12_HEIF;
            pFlexFormatInfo->strideAlignment[count]   = 512;
            pFlexFormatInfo->scanlineAlignment[count] = 512;
            count++;
        }
        else
        {
            ALOGV("%s NV12 Formats not added, count %d, CamxFormatUtilMaxFlexibleFormats %d",
                  __FUNCTION__, count, CamxFormatUtilMaxFlexibleFormats);
        }

        pFlexFormatInfo->count = count;
        result                 = CamxFormatResultSuccess;
    }

    return result;
}
