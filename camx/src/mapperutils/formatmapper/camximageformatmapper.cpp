////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   camximageformatmapper.cpp
/// @brief  Utility functions for camera image format mapping
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "camxcommontypes.h"
#include "camxdebugprint.h"
#include "camxdefs.h"
#include "camxformatutilexternal.h"
#include "camximageformatutils.h"
#include "camximageformatmapper.h"
#include "camxosutils.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Function ptr to get FlexibleYUVFormats from externalformatutils lib API.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CamxFormatResult(*ExtGetFlexibleYUVFormats)(
    CamxFlexFormatInfo* pFlexFormatInfo);

using ::android::hardware::hidl_vec;
using ::android::hardware::graphics::mapper::V2_0::Error;
using ::android::hardware::graphics::mapper::V2_0::IMapper;
using ::android::hardware::graphics::mapper::V2_0::YCbCrLayout;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::vendor::qti::hardware::display::mapperextensions::V1_0::PlaneComponent;
using ::vendor::qti::hardware::display::mapperextensions::V1_0::PlaneLayout;
using ::vendor::qti::hardware::display::mapper::V2_0::IQtiMapper;

using Error3_0       = ::android::hardware::graphics::mapper::V3_0::Error;
using IMapper3_0     = ::android::hardware::graphics::mapper::V3_0::IMapper;
using ExtensionError = ::vendor::qti::hardware::display::mapperextensions::V1_0::Error;
using IQtiMapper3_0  = ::vendor::qti::hardware::display::mapper::V3_0::IQtiMapper;


CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Namespaces
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FormatMapper* FormatMapper::GetInstance()
{
    static FormatMapper* pMapper;
    static std::mutex    s_lock;

    static std::lock_guard<std::mutex> s_guard(s_lock);

    if (NULL == pMapper)
    {
        pMapper = CAMX_NEW FormatMapper;

        if (NULL != pMapper)
        {
            CamxResult result = pMapper->Initialize();

            if (CamxResultSuccess != result)
            {
                CAMX_DELETE pMapper;
                pMapper = NULL;
            }
        }

    }

    return pMapper;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::Initialize()
{
    CamxResult      result   = CamxResultEFailed;
    UINT32          version  = 0;
    Return<void>    ret;


#if defined(_LP64)
    const CHAR libFilename[] = "/vendor/lib64/libcamxexternalformatutils.so";
#else // _LP64
    const CHAR libFilename[] = "/vendor/lib/libcamxexternalformatutils.so";
#endif // _LP64

    ExtGetFlexibleYUVFormats pFuncPtr  = NULL;
    CamX::OSLIBRARYHANDLE    hHandle   = CamX::OsUtils::LibMap(libFilename);
    sp<IQtiMapper3_0>        qtiMapper = IQtiMapper3_0::castFrom(IMapper3_0::getService());

    // Reset to FALSE
    m_bQtiMapperUsed     = FALSE;
    mQtiMapperExtensions = NULL;

    if (qtiMapper != NULL)
    {
        auto hidl_cb = [&](const auto& tmpError, const auto& _extensions)
        {
            if (Error3_0::NONE == tmpError)
            {
                m_bQtiMapperUsed     = TRUE;
                mQtiMapperExtensions = _extensions;
                result               = CamxResultSuccess;
            }
        };

        ret = qtiMapper->getMapperExtensions(hidl_cb);

        qtiMapper.clear();
        qtiMapper = NULL;
        version   = 3;
    }
    else
    {
        auto hidl_cb = [&](const auto& tmpError, const auto& _extensions)
        {
            if (Error::NONE == tmpError)
            {
                m_bQtiMapperUsed     = TRUE;
                mQtiMapperExtensions = _extensions;
                result               = CamxResultSuccess;
            }
        };

        sp<IQtiMapper>  qtiMapper2_0 = IQtiMapper::castFrom(IMapper::getService());

        if (qtiMapper2_0 != NULL)
        {
            ret = qtiMapper2_0->getMapperExtensions(hidl_cb);

            qtiMapper2_0.clear();
            qtiMapper2_0 = NULL;
            version      = 2;
        }
    }

    if (FALSE == ret.isOk())
    {
        CAMX_LOG_ERROR(CamxLogGroupFormat, "Unable to send response for version %d, Exception : %s",
                       version, ret.description().c_str());
        m_bQtiMapperUsed = FALSE;
        result           = CamxResultEFailed;
    }

    if (TRUE == m_bQtiMapperUsed)
    {
        CAMX_LOG_INFO(CamxLogGroupFormat, "mQtiMapperExtensions is loaded, version %d", version);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupFormat, "mQtiMapperExtensions is not loaded");
    }

    // Even if QtiMapper is NULL, we can still support internal formats in CamxFormatMapper class
    if (NULL != hHandle)
    {
        pFuncPtr                = reinterpret_cast<ExtGetFlexibleYUVFormats>
                    (CamX::OsUtils::LibGetAddr(hHandle, "CamxFormatUtil_GetFlexibleYUVFormats"));
        m_pFGetExtLibPlaneInfo  = reinterpret_cast<ExtLibGetPlanelayoutInfo>
                    (CamX::OsUtils::LibGetAddr(hHandle, "CamxFormatUtil_GetPlaneLayoutInfo"));
    }

    if (NULL != pFuncPtr)
    {
        m_yuvFlexFormatInfo.size = sizeof(CamxFlexFormatInfo);
        result                   = pFuncPtr(&m_yuvFlexFormatInfo);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupFormat, "CamxFormatUtil_GetFlexibleYUVFormats returned %d", result);
        }
        else
        {
            // Print all the alignment values used in external format library
            for (UINT formatIndex = 0; formatIndex < m_yuvFlexFormatInfo.count; ++formatIndex)
            {
                CAMX_LOG_CONFIG(CamxLogGroupFormat, "format 0x%x uses alignment %dX%d",
                                m_yuvFlexFormatInfo.pixelFormat[formatIndex],
                                m_yuvFlexFormatInfo.strideAlignment[formatIndex],
                                m_yuvFlexFormatInfo.scanlineAlignment[formatIndex]);
            }
        }
    }

    if ((FALSE == m_bQtiMapperUsed) && (CamxResultSuccess != result))
    {
        CAMX_LOG_ERROR(CamxLogGroupFormat, "LibMap failed for libcamxexternalformatutils");
        result = CamxResultEFailed;
    }
    else
    {
        result = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  FormatMapper::~FormatMapper
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FormatMapper::~FormatMapper()
{
    mQtiMapperExtensions.clear();
    mQtiMapperExtensions = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::GetFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::GetFormat(
    GrallocProperties&   rGrallocProperties,
    Format&              rFormat,
    UINT32&              rGrallocFormat)
{
    CamxResult       result   = CamxResultEFailed;
    GrallocFormat    grFormat = static_cast<GrallocFormat>(rGrallocProperties.pixelFormat);
    ImagePixelLayout imageLayout;

    if (GrallocFormatImplDefined == grFormat)
    {
        result = GetRigidFormatFromImplDefinedFormat(rGrallocProperties, imageLayout);
    }
    else if (GrallocFormatYCbCr420_888 == grFormat)
    {
        result = GetRigidYUVFormat(rGrallocProperties, imageLayout);
    }
    else if ((GrallocFormatRawOpaque == grFormat) ||
        (GrallocFormatRaw10 == grFormat) ||
        (GrallocFormatRaw12 == grFormat))
    {
        result = GetRigidRAWFormat(rGrallocProperties, imageLayout);
    }
    else
    {
        imageLayout.isGrallocFormat = FALSE;

        result = MapGrallocFormatToFormat(grFormat, imageLayout.u.cameraFormat.format);
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == imageLayout.isGrallocFormat)
        {
            rFormat        = imageLayout.u.grallocFormat.format;
            rGrallocFormat = imageLayout.u.grallocFormat.pixelFormat;
        }
        else
        {
            rFormat        = imageLayout.u.cameraFormat.format;
            rGrallocFormat = GrallocFormatUnknown;
        }
    }

    // Override the format based on colorspace
    if ((Format::Blob == rFormat) &&
        ((JFIF == rGrallocProperties.colorSpace) ||
        (V0JFIF == rGrallocProperties.colorSpace)))
    {
        rFormat = Format::Jpeg;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupFormat, "Getting format type %d grFormat 0x%x, grallocFormat %d",
                     rFormat, grFormat, rGrallocFormat);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::GetRigidYUVFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::GetRigidYUVFormat(
    GrallocProperties&   rGrallocProperties,
    ImagePixelLayout&    rPixelLayout)
{
    Format format =
        ((TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageHwCameraRead)) ||
        (TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageHwCameraZSL))) ?
         Format::YUV420NV21 : Format::YUV420NV12;

    if (TRUE == rGrallocProperties.isInternalBuffer)
    {
        if ((TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageHwCameraWrite)) &&
            (TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageSwReadOften)))
        {
            format = Format::YUV420NV21;
        }
        else
        {
            format = Format::YUV420NV12;
        }
        rPixelLayout.isGrallocFormat                  = FALSE;
        rPixelLayout.u.cameraFormat.format            = format;
        rPixelLayout.u.cameraFormat.strideAlignment   = 0;
        rPixelLayout.u.cameraFormat.scanlineAlignment = 0;
    }
    else
    {
        GrallocFormat grFormat = static_cast<GrallocFormat>(rGrallocProperties.pixelFormat);

        rPixelLayout.isGrallocFormat             = TRUE;
        rPixelLayout.u.grallocFormat.format      = format;

        if (TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageHwVideoEncoder))
        {
            rPixelLayout.u.grallocFormat.pixelFormat = GrallocFormatNV12Venus;
        }
        else if (TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageHwImageEncoder))
        {
            rPixelLayout.u.grallocFormat.pixelFormat = GrallocFormatNV12HEIF;
        }
        else if ((TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageHwCameraRead)) ||
                 (GrallocFormatYCbCr420_888 == grFormat))
        {
            rPixelLayout.u.grallocFormat.pixelFormat = GrallocFormatNV21ZSL;
        }
        else
        {
            rPixelLayout.u.grallocFormat.pixelFormat = GrallocFormatNV12Venus;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupFormat, "Rigid YUV format type %d grallocFormat 0x%x", format,
                     rPixelLayout.isGrallocFormat ? static_cast<UINT>(rPixelLayout.u.grallocFormat.pixelFormat) : 0x0);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::GetRigidRAWFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::GetRigidRAWFormat(
    GrallocProperties&   rGrallocProperties,
    ImagePixelLayout&    rPixelLayout)
{
    if (TRUE == rGrallocProperties.isInternalBuffer)
    {
        rPixelLayout.u.cameraFormat.bpp             = 10;
        rPixelLayout.u.cameraFormat.format          = Format::RawMIPI;
        rPixelLayout.isGrallocFormat                = FALSE;
    }
    else
    {
        rPixelLayout.u.grallocFormat.pixelFormat    = GrallocFormatRaw10;
        rPixelLayout.u.grallocFormat.format         = Format::RawMIPI;
        rPixelLayout.isGrallocFormat                = TRUE;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::GetRigidFormatFromImplDefinedFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::GetRigidFormatFromImplDefinedFormat(
    GrallocProperties&   rGrallocProperties,
    ImagePixelLayout&    rPixelLayout)

{
    CamxResult         result         = CamxResultSuccess;
    OutputFormatType   staticFormat   = static_cast<OutputFormatType>(rGrallocProperties.staticFormat);
    GrallocFormat      grallocFormat  = GrallocFormatUnknown;
    Format             format;

    if ((BT2020 == rGrallocProperties.colorSpace) || (BT2020_PQ == rGrallocProperties.colorSpace))
    {
        format        = Format::UBWCTP10;
        grallocFormat = GrallocFormatUBWCTP10;
    }
    else if ((TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageHwVideoEncoder)) ||
             ((FALSE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageHwCameraRead)) &&
              (FALSE == rGrallocProperties.isInternalBuffer)))
    {
        if (TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageUBWC|GrallocUsage10Bit))
        {
            format        = Format::UBWCTP10;
            grallocFormat = GrallocFormatUBWCTP10;
        }
        else if (TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageUBWC))
        {
            if (TRUE == rGrallocProperties.isMultiLayerFormat)
            {
                format        = Format::UBWCNV12FLEX;
                grallocFormat = GrallocFormatUBWCNV12FLEX;
            }
            else
            {
                format        = Format::UBWCNV12;
                grallocFormat = GrallocFormatUBWCNV12;
            }
        }
        else if (TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsage10Bit))
        {
            format        = Format::P010;
            grallocFormat = GrallocFormatP010;
        }
        else if (TRUE == rGrallocProperties.isInternalBuffer)
        {
            format = (OutputFormatYUV420NV21 == staticFormat) ?
                Format::YUV420NV21 : Format::YUV420NV12;
        }
        else
        {
            format        = Format::YUV420NV12;
            grallocFormat = GrallocFormatNV12Venus;
        }
    }
    else if ((TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageHwCameraRead)) ||
             (TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageSwReadOften)))
    {
        if (rGrallocProperties.isRawFormat)
        {
            format        = Format::RawMIPI;
            grallocFormat = GrallocFormatRaw10;
        }
        else if ((OutputFormatYUV420NV21 == staticFormat) ||
                 ((TRUE == Utils::IsBitMaskSet64(rGrallocProperties.grallocUsage, GrallocUsageHwCameraZSL)) &&
                  (FALSE == rGrallocProperties.isInternalBuffer)))
        {
            format        = Format::YUV420NV21;
            grallocFormat = GrallocFormatNV21ZSL;
        }
        else
        {
            format        = Format::YUV420NV12;
            grallocFormat = GrallocFormatNV12Venus;
        }
    }
    else
    {
        result = MapOutputFormatTypeToFormat(staticFormat, format);
    }

    if (TRUE == rGrallocProperties.isInternalBuffer)
    {
        rPixelLayout.isGrallocFormat                  = FALSE;
        rPixelLayout.u.cameraFormat.format            = format;
        rPixelLayout.u.cameraFormat.strideAlignment   = 0;
        rPixelLayout.u.cameraFormat.scanlineAlignment = 0;
    }
    else
    {
        rPixelLayout.isGrallocFormat             = TRUE;
        rPixelLayout.u.grallocFormat.pixelFormat = grallocFormat;
        rPixelLayout.u.grallocFormat.format      = format;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupFormat, "Rigid Impl defined format type %d, grallocUsage 0x%llx, grallocFormat 0x%x",
                     format, rGrallocProperties.grallocUsage, static_cast<UINT>(grallocFormat));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::MapOutputFormatTypeToFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::MapOutputFormatTypeToFormat(
    OutputFormatType outputFormat,
    Format&          rFormat)
{
    switch (outputFormat)
    {
        case OutputFormatYUV420NV12:
            rFormat = Format::YUV420NV12;
            break;

        case OutputFormatYUV420NV21:
            rFormat = Format::YUV420NV21;
            break;

        case OutputFormatUBWCNV12:
            rFormat = Format::UBWCNV12;
            break;

        case OutputFormatUBWCTP10:
            rFormat = Format::UBWCTP10;
            break;

        case OutputFormatRAWPLAIN16:
            rFormat = Format::RawPlain16;
            break;

        case OutputFormatRAWPLAIN64:
            rFormat = Format::RawPlain64;
            break;

        case OutputFormatPD10:
            rFormat = Format::PD10;
            break;

        case OutputFormatP010:
            rFormat = Format::P010;
            break;

        default:
            rFormat = Format::YUV420NV12;
            break;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::UpdatePlaneLayoutInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::UpdatePlaneLayoutInfo(
    ImageFormat*         pImageFormat)
{
    BOOL isYUVFormat  = ImageFormatUtils::IsYUV(pImageFormat);

    if (TRUE == isYUVFormat)
    {
        YUVFormat* pYPlane = &pImageFormat->formatParams.yuvFormat[0];
        YUVFormat* pCPlane = &pImageFormat->formatParams.yuvFormat[1];

        pYPlane->width       = pImageFormat->width;
        pYPlane->height      = pImageFormat->height;
        pCPlane->width       = pImageFormat->width;
        pCPlane->height      = pImageFormat->height >> 1;
        pYPlane->planeStride = pImageFormat->planeLayoutInfo[0].strideBytes;
        pYPlane->sliceHeight = pImageFormat->planeLayoutInfo[0].scanLines;
        pCPlane->planeStride = pImageFormat->planeLayoutInfo[1].strideBytes;
        pCPlane->sliceHeight = pImageFormat->planeLayoutInfo[1].scanLines;
        pYPlane->planeSize   = pImageFormat->planeLayoutInfo[0].planeSize;
        pCPlane->planeSize   = pImageFormat->planeLayoutInfo[1].planeSize;
        CAMX_LOG_INFO(CamxLogGroupFormat,
            "YUV grallocProperties wxh %dx%d, StrideXsliceHeight %dx%d, %dx%d, size %d, %d",
            pImageFormat->width, pImageFormat->height,
            pYPlane->planeStride, pYPlane->sliceHeight,
            pCPlane->planeStride, pCPlane->sliceHeight,
            pYPlane->planeSize, pCPlane->planeSize);
    }
    else if (TRUE == ImageFormatUtils::IsRAW(pImageFormat))
    {
        CAMX_LOG_INFO(CamxLogGroupFormat,
            "RAW grallocProperties wxh %dx%d, StrideXsliceHeight old %dx%d, %dx%d, size %d",
            pImageFormat->width, pImageFormat->height,
            pImageFormat->formatParams.rawFormat.stride,
            pImageFormat->formatParams.rawFormat.sliceHeight,
            pImageFormat->planeLayoutInfo[0].strideBytes,
            pImageFormat->planeLayoutInfo[0].scanLines,
            pImageFormat->planeLayoutInfo[0].planeSize);
        pImageFormat->formatParams.rawFormat.stride      = pImageFormat->planeLayoutInfo[0].strideBytes;
        pImageFormat->formatParams.rawFormat.sliceHeight = pImageFormat->planeLayoutInfo[0].scanLines;
    }
    else
    {
        YUVFormat* pYPlane = &pImageFormat->formatParams.yuvFormat[0];
        YUVFormat* pCPlane = &pImageFormat->formatParams.yuvFormat[1];

        pYPlane->planeSize = 0;
        pCPlane->planeSize = 0;

        for (UINT32 index = 0; index < pImageFormat->numPlanes; ++index)
        {
            PlaneLayoutInfo* pPlaneLayoutInfo = &pImageFormat->planeLayoutInfo[index];
            UINT8            bMetaData        = FALSE;

            if (CAMX_PLANE_COMPONENT_META ==
                static_cast<CamxPlaneComponent>(CAMX_PLANE_COMPONENT_META & pPlaneLayoutInfo->component))
            {
                bMetaData = TRUE;
            }

            if (CAMX_PLANE_COMPONENT_Y ==
                static_cast<CamxPlaneComponent>(CAMX_PLANE_COMPONENT_Y & pPlaneLayoutInfo->component))
            {
                if (TRUE == bMetaData)
                {
                    pYPlane->metadataStride = (pPlaneLayoutInfo->strideBytes * 1024) / 64;
                    pYPlane->metadataSize   = pPlaneLayoutInfo->planeSize;
                    pYPlane->metadataHeight = pYPlane->metadataSize / pYPlane->metadataStride;
                    pYPlane->planeSize     += pYPlane->metadataSize;
                }
                else
                {
                    pYPlane->planeStride    = pPlaneLayoutInfo->strideBytes;
                    pYPlane->sliceHeight    = pPlaneLayoutInfo->scanLines;
                    pYPlane->pixelPlaneSize = pPlaneLayoutInfo->planeSize;
                    pYPlane->planeSize     += pPlaneLayoutInfo->planeSize;
                }
            }
            else if (CAMX_PLANE_COMPONENT_Cb ==
                     static_cast<CamxPlaneComponent>(CAMX_PLANE_COMPONENT_Cb & pPlaneLayoutInfo->component))
            {
                if (TRUE == bMetaData)
                {
                    pCPlane->metadataStride = (pPlaneLayoutInfo->strideBytes * 1024) / 64;
                    pCPlane->metadataSize   = pPlaneLayoutInfo->planeSize;
                    pCPlane->metadataHeight = pCPlane->metadataSize / pCPlane->metadataStride;
                    pCPlane->planeSize     += pPlaneLayoutInfo->planeSize;
                }
                else
                {
                    pCPlane->planeStride    = pPlaneLayoutInfo->strideBytes;
                    pCPlane->sliceHeight    = pPlaneLayoutInfo->scanLines;
                    pCPlane->pixelPlaneSize = pPlaneLayoutInfo->planeSize;
                    pCPlane->planeSize     += pPlaneLayoutInfo->planeSize;
                }
            }
        }
        CAMX_LOG_INFO(CamxLogGroupFormat,
                      "UBWC grallocProperties wxh %dx%d, StrideXsliceHeight %dx%d, %dx%d, size %d, %d"
                      "meta stride*sliceheight Y %dx%d, metastride*sliceheight C %dx%d, metasize Y %d, C %d",
                      pImageFormat->width, pImageFormat->height,
                      pYPlane->planeStride, pYPlane->sliceHeight,
                      pCPlane->planeStride, pCPlane->sliceHeight,
                      pYPlane->planeSize, pCPlane->planeSize,
                      pYPlane->metadataStride, pYPlane->metadataHeight,
                      pCPlane->metadataStride, pCPlane->metadataHeight,
                      pYPlane->metadataSize, pCPlane->metadataSize);
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::GetYUVPlaneInfoFromExtLib
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::GetYUVPlaneInfoFromExtLib(
    UINT32          pixelFormat,
    ImageFormat*    pImageFormat)
{
    UINT        index               = 0;
    INT         strideAlignment     = 2;
    INT         scanlineAlignment   = 2;
    CamxResult  result              = CamxResultEFailed;

    CamxPlaneLayoutInfo planeInfo   = {0};

    for (index = 0; index < m_yuvFlexFormatInfo.count; ++index)
    {
        if (pixelFormat == m_yuvFlexFormatInfo.pixelFormat[index])
        {
            strideAlignment   = m_yuvFlexFormatInfo.strideAlignment[index];
            scanlineAlignment = m_yuvFlexFormatInfo.scanlineAlignment[index];
            result            = CamxResultSuccess;
            break;
        }
    }

    YUVFormat* pYPlane  = &pImageFormat->formatParams.yuvFormat[0];
    YUVFormat* pCPlane  = &pImageFormat->formatParams.yuvFormat[1];

    pYPlane->width      = pImageFormat->width;
    pYPlane->height     = pImageFormat->height;
    pCPlane->width      = pImageFormat->width;
    pCPlane->height     = pImageFormat->height >> 1;


    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupFormat, "GrallocFormat %d is not available in m_yuvFlexFormatInfo array",
                      pixelFormat);
    }
    else if (NULL == m_pFGetExtLibPlaneInfo)
    {
        CAMX_LOG_INFO(CamxLogGroupFormat, "m_pFGetExtLibPlaneInfo is NULL");
        result = CamxResultEUnsupported;
    }
    else
    {
        result = m_pFGetExtLibPlaneInfo(m_yuvFlexFormatInfo.pixelFormat[index],
                                        CAMERA_PLANE_TYPE_Y,
                                        pImageFormat->width,
                                        pImageFormat->height,
                                        &planeInfo);
    }

    // If Pixel Format is supported by External library, then contnue to get for UV plane as well
    if (CamxFormatResultSuccess == result)
    {
        pYPlane->planeStride = planeInfo.stride;
        pYPlane->sliceHeight = planeInfo.scanline;
        pYPlane->planeSize   = planeInfo.planeSize;

        result = m_pFGetExtLibPlaneInfo(m_yuvFlexFormatInfo.pixelFormat[index],
                                        CAMERA_PLANE_TYPE_UV, pImageFormat->width,
                                        pImageFormat->height, &planeInfo);

        pCPlane->planeStride = planeInfo.stride;
        pCPlane->sliceHeight = planeInfo.scanline;
        pCPlane->planeSize   = planeInfo.planeSize;
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupFormat, "GrallocFormat %d is not supported in extformatlib",
                         pixelFormat);
        pYPlane->planeStride = Utils::AlignGeneric32(pImageFormat->width,  strideAlignment);
        pYPlane->sliceHeight = Utils::AlignGeneric32(pImageFormat->height, scanlineAlignment);
        pCPlane->planeStride = Utils::AlignGeneric32(pImageFormat->width,  strideAlignment);
        pCPlane->sliceHeight = Utils::AlignGeneric32((pImageFormat->height >> 1), scanlineAlignment) ;
        pYPlane->planeSize   = ImageFormatUtils::GetPlaneSize(pImageFormat, 0);
        pCPlane->planeSize   = ImageFormatUtils::GetPlaneSize(pImageFormat, 1);
        result               = CamxResultSuccess;
    }

    CAMX_LOG_INFO(CamxLogGroupFormat,
        "YUV extformatlib Properties wxh %dx%d, StrideXslice %dx%d, %dx%d, size %zd, %zd, align %dx%d, format %d",
        pImageFormat->width, pImageFormat->height,
        pYPlane->planeStride, pYPlane->sliceHeight,
        pCPlane->planeStride, pCPlane->sliceHeight,
        pYPlane->planeSize, pCPlane->planeSize,
        strideAlignment, scanlineAlignment,
        pixelFormat);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::FillImageFormatInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::FillImageFormatInfo(
    GrallocProperties&   rGrallocProperties,
    ImageFormat*         pImageFormat)
{
    CamxResult          result               = CamxResultEFailed;
    BOOL                isQtiMapperAvailable = m_bQtiMapperUsed;
    INT                 strideAlignment      = 1;
    INT                 scanlineAlignment    = 1;
    ImagePixelLayout    pixelLayout;

    if ((NULL != pImageFormat) && (FALSE == rGrallocProperties.isInternalBuffer))
    {
        BOOL isYUVFormat  = ImageFormatUtils::IsYUV(pImageFormat);
        BOOL isUBWCFormat = ImageFormatUtils::IsUBWC(pImageFormat->format);

        if (Format::UBWCNV124R == pImageFormat->format)
        {
            CAMX_LOG_WARN(CamxLogGroupFormat, "format %d is not supported by Gralloc, use internal calc",
                          pImageFormat->format);
            isQtiMapperAvailable = FALSE;
        }

        if ((TRUE == isYUVFormat) || (TRUE == rGrallocProperties.isRawFormat) || (TRUE == isUBWCFormat))
        {
            if (TRUE == isQtiMapperAvailable)
            {
                result = GetPlaneLayoutInfo(rGrallocProperties, pImageFormat);
                if ((CamxResultSuccess == result) && (0 < pImageFormat->numPlanes))
                {
                    result = UpdatePlaneLayoutInfo(pImageFormat);
                }
                else
                {
                    isQtiMapperAvailable = FALSE;
                }
            }
        }

        if ((FALSE == isQtiMapperAvailable) && (TRUE == isUBWCFormat))
        {
            YUVFormat* pYPlane = &pImageFormat->formatParams.yuvFormat[0];
            YUVFormat* pCPlane = &pImageFormat->formatParams.yuvFormat[1];

            ImageFormatUtils::SetupUBWCPlanes(pImageFormat);
            CAMX_LOG_INFO(CamxLogGroupFormat,
                "UBWC extformatlib Properties wxh %dx%d, StrideXsliceHeight %dx%d, %dx%d, size %d, %d",
                pImageFormat->width, pImageFormat->height,
                pYPlane->planeStride, pYPlane->sliceHeight,
                pCPlane->planeStride, pCPlane->sliceHeight,
                pYPlane->planeSize, pCPlane->planeSize);
        }
        else if ((TRUE == isYUVFormat) && (FALSE == isQtiMapperAvailable))
        {
            result = GetRigidYUVFormat(rGrallocProperties, pixelLayout);
            GetYUVPlaneInfoFromExtLib(pixelLayout.u.grallocFormat.pixelFormat, pImageFormat);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::MapGrallocFormatToFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::MapGrallocFormatToFormat(
    GrallocFormat grFormat,
    Format&       rFormat)
{
    CamxResult result = CamxResultSuccess;

    switch (grFormat)
    {
        case GrallocFormatY8:
            rFormat = Format::Y8;
            break;

        case GrallocFormatY16:
            rFormat = Format::Y16;
            break;

        case GrallocFormatRaw8:
            rFormat = Format::RawMIPI8;
            break;

        case GrallocFormatRaw10:
            rFormat = Format::RawMIPI;
            break;

        case GrallocFormatRaw16:
            rFormat = Format::RawPlain16;
            break;

        case GrallocFormatRaw64:
            rFormat = Format::RawPlain64;
            break;

        case GrallocFormatBlob:
            rFormat = Format::Blob;
            break;

        case GrallocFormatYCbCr420_SP:
        case GrallocFormatYUV420Flex2:
        case GrallocFormatYUV420Flex3:
        case GrallocFormatNV12Venus:
        case GrallocFormatNV12HEIF:
            rFormat = Format::YUV420NV12;
            break;

        case GrallocFormatYCrCb420_SP:
        case GrallocFormatNV21ZSL:
        case GrallocFormatYUV420Flex1:
        case GrallocFormatNV21Adreno:
        case GrallocFormatNV21Venus:
            rFormat = Format::YUV420NV21;
            break;

        case GrallocFormatYCbCr422SP:
            rFormat = Format::YUV422NV16;
            break;

        case GrallocFormatP010:
            rFormat = Format::P010;
            break;

        case GrallocFormatPD10:
            rFormat = Format::PD10;
            break;

        case GrallocFormatUBWCTP10:
            rFormat = Format::UBWCTP10;
            break;

        case GrallocFormatUBWCNV12:
            rFormat = Format::UBWCNV12;
            break;

        case GrallocFormatUBWCNV124R:
            rFormat = Format::UBWCNV124R;
            break;

        case GrallocFormatYCrCb422SP:
        case GrallocFormatCbYCrY:
        case GrallocFormatYCbCr444_SP:
        case GrallocFormatYCrCb444_SP:
        case GrallocFormatYBWC10:
        default:
            result = CamxResultENoSuch;
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::GetCameraPixelFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::GetCameraPixelFormat(
    GrallocProperties&   rGrallocProperties,
    CameraPixelFormat&   rCamPixelFormat)
{
    UINT32     grFormat = 0;
    CamxResult result   = GetFormat(rGrallocProperties, rCamPixelFormat.format, grFormat);

    if (CamxResultSuccess == result)
    {
        for (UINT index = 0; index < m_yuvFlexFormatInfo.count; ++index)
        {
            if (rGrallocProperties.pixelFormat == m_yuvFlexFormatInfo.pixelFormat[index])
            {
                rCamPixelFormat.strideAlignment   = m_yuvFlexFormatInfo.strideAlignment[index];
                rCamPixelFormat.scanlineAlignment = m_yuvFlexFormatInfo.scanlineAlignment[index];
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::GetGrallocPixelFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::GetGrallocPixelFormat(
    GrallocProperties&   rGrallocProperties,
    GrallocPixelFormat&  rGrPixelFormat)
{
    UINT32     grFormat = 0;
    CamxResult result   = GetFormat(rGrallocProperties, rGrPixelFormat.format, grFormat);

    if (CamxResultSuccess == result)
    {
        rGrPixelFormat.pixelFormat = grFormat;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::GetImagePixelLayout
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::GetImagePixelLayout(
    GrallocProperties&   rGrallocProperties,
    ImagePixelLayout&    rPixelLayout,
    UINT32&              rGrallocFormat)
{
    CamxResult result;

    if (FALSE == rGrallocProperties.isInternalBuffer)
    {
        Format format;

        rPixelLayout.isGrallocFormat = TRUE;

        format = rPixelLayout.u.grallocFormat.format;
        result = GetFormat(rGrallocProperties, format, rGrallocFormat);

        // If grallocFormat is not updated by GetFormat API, then use default pixelFormat value
        if (GrallocFormatUnknown == static_cast<GrallocFormat>(rGrallocFormat))
        {
            rGrallocFormat = rGrallocProperties.pixelFormat;
        }
    }
    else
    {
        rPixelLayout.isGrallocFormat = FALSE;

        result = GetCameraPixelFormat(rGrallocProperties, rPixelLayout.u.cameraFormat);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::GetPlaneLayoutInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::GetPlaneLayoutInfo(
    GrallocProperties& rGrallocProperties,
    ImageFormat*       pFormat)
{
    CamxResult result = CamxResultEFailed;

    if ((mQtiMapperExtensions != NULL) && (FALSE == rGrallocProperties.isInternalBuffer))
    {
        ExtensionError error       = ExtensionError::NONE;
        UINT32         pixelFormat = 0;
        INT32          planeCount  = 0;

        ImagePixelLayout pixelLayout;
        result = GetImagePixelLayout(rGrallocProperties, pixelLayout, pixelFormat);

        if ((CamxResultSuccess == result) && (TRUE == pixelLayout.isGrallocFormat))
        {
            hidl_vec<PlaneLayout> planeInfo;
            UINT32 bufferSize = 0;

            auto hidl_cb = [&](const auto& tmpError, const auto& size, const auto& plane_info)
            {
                if (ExtensionError::NONE != tmpError)
                {
                    error = tmpError;
                }
                else
                {
                    bufferSize = size;
                    planeInfo  = plane_info;
                }
            };

            Return<void> ret = mQtiMapperExtensions->getFormatLayout(pixelFormat,
                                                                     rGrallocProperties.grallocUsage, 0,
                                                                     pFormat->width, pFormat->height, hidl_cb);

            if (false == ret.isOk())
            {
                CAMX_LOG_ERROR(CamxLogGroupFormat, "Unable to send response. Exception : %s",
                               ret.description().c_str());
                mQtiMapperExtensions.clear();
                mQtiMapperExtensions = NULL;
                m_bQtiMapperUsed     = FALSE;
                result               = CamxResultEFailed;
            }
            else if (ExtensionError::NONE == error)
            {
                result              = CamxResultSuccess;
                planeCount          = planeInfo.size();
                planeCount          = (planeCount > MaxPlaneLayOut) ? MaxPlaneLayOut : planeCount;
                pFormat->numPlanes  = planeCount;
                pFormat->bufferSize = bufferSize;
                CAMX_LOG_INFO(CamxLogGroupFormat,
                                 "planeLayoutSize %d, pixelFormat %d, bufferSize %d, usage 0x%" PRIu64 ", format %d",
                                 planeCount, pixelFormat, bufferSize,
                                 rGrallocProperties.grallocUsage, pFormat->format);

                for (INT32 index = 0; index < planeCount; ++index)
                {
                    pFormat->planeLayoutInfo[index].component             = planeInfo[index].component;
                    pFormat->planeLayoutInfo[index].horizontalSubsampling = planeInfo[index].h_subsampling;
                    pFormat->planeLayoutInfo[index].verticalSubsampling   = planeInfo[index].v_subsampling;
                    pFormat->planeLayoutInfo[index].offset                = planeInfo[index].offset;
                    pFormat->planeLayoutInfo[index].pixelIncrement        = planeInfo[index].pixel_increment;
                    pFormat->planeLayoutInfo[index].stridePixels          = planeInfo[index].stride;
                    pFormat->planeLayoutInfo[index].strideBytes           = planeInfo[index].stride_bytes;
                    pFormat->planeLayoutInfo[index].scanLines             = planeInfo[index].scanlines;
                    pFormat->planeLayoutInfo[index].planeSize             = planeInfo[index].size;
                    CAMX_LOG_INFO(CamxLogGroupFormat, "component %d,horizontalSubsampling %d,verticalSubsampling %d,offset %d,"
                                  "pixelIncrement %d,stridePixels %d,strideBytes %d,scanLines %d,planeSize %d",
                                  pFormat->planeLayoutInfo[index].component,
                                  pFormat->planeLayoutInfo[index].horizontalSubsampling,
                                  pFormat->planeLayoutInfo[index].verticalSubsampling,
                                  pFormat->planeLayoutInfo[index].offset,
                                  pFormat->planeLayoutInfo[index].pixelIncrement,
                                  pFormat->planeLayoutInfo[index].stridePixels,
                                  pFormat->planeLayoutInfo[index].strideBytes,
                                  pFormat->planeLayoutInfo[index].scanLines,
                                  pFormat->planeLayoutInfo[index].planeSize);
                }
            }
            else
            {
                result = CamxResultEInvalidArg;
                CAMX_LOG_ERROR(CamxLogGroupFormat, "getFormatLayout failed, pixelFormat %d, [%dx%d], usage 0x%" PRIu64 "",
                               pixelFormat, pFormat->width, pFormat->height, rGrallocProperties.grallocUsage);
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupFormat,
                          "GetImagePixelLayout failed for pixelFormat %d, ImageFormat %d, [%dx%d], usage 0x%" PRIu64 "",
                          pixelFormat, pFormat->format,
                          pFormat->width, pFormat->height,
                          rGrallocProperties.grallocUsage);
        }
    }
    else if (FALSE == rGrallocProperties.isInternalBuffer)
    {
        CAMX_LOG_ERROR(CamxLogGroupFormat, "getFormatLayout failed, mQtiMapper is null");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::ValidateBufferSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::ValidateBufferSize(
    VOID*   phBufHandle,
    SIZE_T  bufSize)
{
    CamxResult result            = CamxResultSuccess;
    SIZE_T     grallocBufferSize = 0;

    if (NULL == phBufHandle)
    {
        CAMX_LOG_ERROR(CamxLogGroupFormat, "phBufHandle is NULL");
        result = CamxResultEInvalidArg;
    }

    else if (mQtiMapperExtensions != NULL)
    {
        ExtensionError error   = ExtensionError::NONE;
        auto           hidl_cb = [&](const auto tmpError, const auto bufferSize)
        {
            if (ExtensionError::NONE != tmpError)
            {
                error = tmpError;
            }
            else
            {
                grallocBufferSize = bufferSize;
            }
        };

        Return<void> ret = mQtiMapperExtensions->getSize(phBufHandle, hidl_cb);

        if (false == ret.isOk())
        {
            CAMX_LOG_WARN(CamxLogGroupFormat, "QtiMapper Unable to send response. Exception : %s",
                          ret.description().c_str());
            mQtiMapperExtensions.clear();
            mQtiMapperExtensions = NULL;
            m_bQtiMapperUsed     = FALSE;
        }
        else if (ExtensionError::NONE != error)
        {
            CAMX_LOG_WARN(CamxLogGroupFormat, "getSize returned error %d, ignore size value",
                          error);
            result = CamxResultEFailed;
        }
        else if (grallocBufferSize < bufSize)
        {
            CAMX_LOG_ERROR(CamxLogGroupFormat, "grallocBufferSize %zu is less than TotalSize %zu",
                           grallocBufferSize, bufSize);
            result = CamxResultEResource;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupFormat, "grallocBufferSize %zu is %s TotalSize %zu",
                grallocBufferSize,
                (grallocBufferSize > bufSize) ? "greater than" : "equal to",
                bufSize);
        }
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FormatMapper::GetUnalignedBufferSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FormatMapper::GetUnalignedBufferSize(
    VOID*   phBufHandle,
    SIZE_T  bufSize,
    Format  format,
    SIZE_T* pGrallocBufferSize)
{
    CamxResult result            = CamxResultSuccess;
    SIZE_T     unaligned_grallocBufferSize = 0;

    if (NULL == phBufHandle || NULL == pGrallocBufferSize)
    {
        CAMX_LOG_ERROR(CamxLogGroupFormat, "phBufHandle is NULL");
        result = CamxResultEInvalidArg;
    }

    // only Jpeg is supported
    else if (Format::Jpeg != format)
    {
        CAMX_LOG_ERROR(CamxLogGroupFormat, "format  %d is not supported", format);
        result = CamxResultEUnsupported;
    }

    else if (mQtiMapperExtensions != NULL)
    {
        ExtensionError error   = ExtensionError::NONE;
        auto           hidl_cb = [&](const auto tmpError, const auto bufferSize)
        {
            if (ExtensionError::NONE != tmpError)
            {
                error = tmpError;
            }
            else
            {
                unaligned_grallocBufferSize = bufferSize;
            }
        };

        Return<void> ret = mQtiMapperExtensions->getUnalignedWidth(phBufHandle, hidl_cb);
        if (false == ret.isOk())
        {
            CAMX_LOG_WARN(CamxLogGroupFormat, "QtiMapper Unable to send response. Exception : %s",
                          ret.description().c_str());
            mQtiMapperExtensions.clear();
            mQtiMapperExtensions = NULL;
            m_bQtiMapperUsed     = FALSE;
        }
        else if (ExtensionError::NONE != error)
        {
            CAMX_LOG_WARN(CamxLogGroupFormat, "getSize returned error %d, ignore size value",
                          error);
            result = CamxResultEFailed;
        }
        else if (unaligned_grallocBufferSize < bufSize)
        {
            CAMX_LOG_ERROR(CamxLogGroupFormat, "grallocBufferSize %zu is less than TotalSize %zu",
                           unaligned_grallocBufferSize, bufSize);
            result = CamxResultEResource;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupFormat, "unaligned_grallocBufferSize %zu is %s TotalSize %zu",
                unaligned_grallocBufferSize,
                (unaligned_grallocBufferSize > bufSize) ? "greater than" : "equal to",
                bufSize);
            *(pGrallocBufferSize) = unaligned_grallocBufferSize;
        }
    }
    return result;
}

CAMX_NAMESPACE_END
