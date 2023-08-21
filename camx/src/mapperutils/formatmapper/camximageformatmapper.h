////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camximageformatmapper.h
/// @brief CamX Image Format Mapper functionality
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIMAGEFORMATMAPPER_H
#define CAMXIMAGEFORMATMAPPER_H

#include "camxformats.h"
#include "camxformatutilexternal.h"
#include "g_camxsettings.h"

// NOWHINE FILE CP011:  C++ using keyword
// NOWHINE FILE PR009: '/' usage in include

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Function pointer to get Plane layout info from externalformatutils lib API.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CamxFormatResult(*ExtLibGetPlanelayoutInfo)(
    CamxPixelFormat       pixelFormat,
    CamxPlaneType         planeType,
    INT                   width,
    INT                   height,
    CamxPlaneLayoutInfo*  pPlaneInfo);

#include "IServiceManager.h"
#include "vendor/qti/hardware/display/mapper/3.0/IQtiMapper.h"
#include "vendor/qti/hardware/display/mapper/2.0/IQtiMapper.h"

using ::android::sp;
using ::vendor::qti::hardware::display::mapperextensions::V1_0::IQtiMapperExtensions;

CAMX_NAMESPACE_BEGIN

#pragma pack(push, 8)

/// @brief Plane Component type definitions. "component" field in "PlaneLayoutInfo" struct will use this info.
enum CamxPlaneComponent
{
    CAMX_PLANE_COMPONENT_Y     = 1 << 0,      ///< luma
    CAMX_PLANE_COMPONENT_Cb    = 1 << 1,      ///< chroma blue
    CAMX_PLANE_COMPONENT_Cr    = 1 << 2,      ///< chroma red
    CAMX_PLANE_COMPONENT_R     = 1 << 10,     ///< red
    CAMX_PLANE_COMPONENT_G     = 1 << 11,     ///< green
    CAMX_PLANE_COMPONENT_B     = 1 << 12,     ///< blue
    CAMX_PLANE_COMPONENT_A     = 1 << 20,     ///< alpha
    CAMX_PLANE_COMPONENT_RAW   = 1 << 30,     ///< raw data plane
    CAMX_PLANE_COMPONENT_META  = 1 << 31,     ///< meta data plane
};

/// @brief Pixel Format. Matches the gralloc values
enum GrallocFormat
{
    GrallocFormatUnknown       = 0,            ///< Undefined or Unknown format
    GrallocFormatY8            = 0x20203859,   ///< Y 8
    GrallocFormatY16           = 0x20363159,   ///< Y 16
    GrallocFormatRaw8          = 0x00000123,   ///< Raw8 format
    GrallocFormatRaw10         = 0x00000025,   ///< Raw 10
    GrallocFormatRaw12         = 0x00000026,   ///< Raw 12
    GrallocFormatRaw16         = 0x00000020,   ///< Raw16 format
    GrallocFormatRaw64         = 0x00000027,   ///< Raw64 format
    GrallocFormatBlob          = 0x00000021,   ///< Carries data which does not have a standard image structure (e.g. JPEG)
    GrallocFormatYCbCr420_SP   = 0x00000109,   ///< YCbCr420 OEM specific format
    GrallocFormatYCrCb420_SP   = 0x00000011,   ///< YCrCb420_SP format
    GrallocFormatNV21ZSL       = 0x00000113,   ///< NV12 ZSL Format
    GrallocFormatYUV420Flex1   = 0x00000125,   ///< Camera flexible YUV 420888 format
    GrallocFormatYUV420Flex2   = 0x00000126,   ///< Camera flexible YUV 420888 format
    GrallocFormatYUV420Flex3   = 0x00000127,   ///< Camera flexible YUV 420888 format
    GrallocFormatYCbCr422SP    = 0x00000010,   ///< YUV422 format
    GrallocFormatYCrCb422SP    = 0x0000010B,   ///< YVU422 format
    GrallocFormatNV12Venus     = 0x7FA30C04,   ///< NV12 video format
    GrallocFormatNV21Venus     = 0x00000114,   ///< NV21 video format
    GrallocFormatNV21Adreno    = 0x7FA30C01,   ///< NV21 adreno format
    GrallocFormatNV12HEIF      = 0x00000116,   ///< HEIF video YUV420 format
    GrallocFormatCbYCrY        = 0x00000120,   ///< Blob format
    GrallocFormatYCbCr444_SP   = 0x0000010F,   ///< YCbCr420_SP format
    GrallocFormatYCrCb444_SP   = 0x00000110,   ///< YCrCb420_SP format
    GrallocFormatP010          = 0x7FA30C0A,   ///< P010
    GrallocFormatPD10          = 0x7FA30C08,   ///< PD10
    GrallocFormatUBWCTP10      = 0x7FA30C09,   ///< UBWCTP10
    GrallocFormatUBWCNV12      = 0x7FA30C06,   ///< UBWCNV12
    GrallocFormatUBWCNV124R    = 0x00000028,   ///< UBWCNV12-4R
    GrallocFormatYBWC10        = 0x4C595559,   ///< Blob format
    GrallocFormatYCbCr420_888  = 0x00000023,   ///< Efficient YCbCr/YCrCb 4:2:0 buffer layout, layout-independent
    GrallocFormatImplDefined   = 0x00000022,   ///< Format is up to the device-specific Gralloc implementation.
    GrallocFormatRawOpaque     = 0x00000024,   ///< Raw Opaque
    GrallocFormatUBWCNV12FLEX  = 0x00000126    ///< UBWCNV12 Flex
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Class that implements Format Mapping class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FormatMapper
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  Get the instance of singleton FormatMapper object.
    ///
    /// @return Pointer to the FormatMapper object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static FormatMapper* GetInstance();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFormat
    ///
    /// @brief  Utility Function to get format type, given the gralloc properties
    ///
    /// @param  rGrallocProperties Gralloc usage flags and other properties required for buffer allocation
    /// @param  rFormat            Output format
    /// @param  rGrallocFormat     pixelFormat value (valid for external formats)
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetFormat(
       GrallocProperties&   rGrallocProperties,
       Format&              rFormat,
       UINT32&              rGrallocFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillImageFormatInfo
    ///
    /// @brief  Utility Function to fill the image format structure given the gralloc properties
    ///
    /// @param  rGrallocProperties Gralloc usage flags and other properties required for buffer allocation
    /// @param  pImageFormat       Pointer to theImage format
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillImageFormatInfo(
        GrallocProperties&   rGrallocProperties,
        ImageFormat*         pImageFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetImagePixelLayout
    ///
    /// @brief  Utility Function to get the image pixel layout, given the gralloc properties
    ///
    /// @param  rGrallocProperties Gralloc usage flags and other properties required for buffer allocation
    /// @param  rPixelLayout       Unique Image format structure
    /// @param  rGrallocFormat     pixelFormat value
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetImagePixelLayout(
        GrallocProperties&   rGrallocProperties,
        ImagePixelLayout&    rPixelLayout,
        UINT32&              rGrallocFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPlaneLayoutInfo
    ///
    /// @brief  Get plane layout info for a given Camx format.
    ///
    /// @param  rGrallocProperties Gralloc usage flags and other properties required for buffer allocation
    /// @param  pFormat            Camx image format info
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPlaneLayoutInfo(
        GrallocProperties&  rGrallocProperties,
        ImageFormat*        pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateBufferSize
    ///
    /// @brief  Function to validate buffer size with gralloc size
    ///
    /// @param  phBufferHandle          Buffer Handle
    /// @param  bufferSize              Buffer Size value
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ValidateBufferSize(
        VOID*   phBufferHandle,
        SIZE_T  bufferSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUnalignedBufferSize
    ///
    /// @brief  Function to get buffer size from gralloc
    ///
    /// @param  phBufferHandle          Buffer Handle
    /// @param  bufferSize              Buffer Size value
    /// @param  format                  Format of the image buffer
    /// @param  pGrallocBufferSize      Pointer to read Gralloc Buffer Size value
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetUnalignedBufferSize(
        VOID*   phBufferHandle,
        SIZE_T  bufferSize,
        Format  format,
        SIZE_T* pGrallocBufferSize);

private:
    FormatMapper()                                       = default;
    FormatMapper(const FormatMapper& rOther)             = delete;
    FormatMapper(const FormatMapper&& rrOther)            = delete;
    FormatMapper& operator=(const FormatMapper& rOther)  = delete;
    FormatMapper& operator=(const FormatMapper&& rrOther) = delete;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Second phase initialization of the format mapper class
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~FormatMapper
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~FormatMapper();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRigidFormatFromImplDefinedFormat
    ///
    /// @brief  Get rigid format given implementation defined format given the gralloc properties
    ///
    /// @param  rGrallocProperties Gralloc usage flags and other properties required for buffer allocation
    /// @param  rPixelLayout       Unique Image format structure
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetRigidFormatFromImplDefinedFormat(
        GrallocProperties&   rGrallocProperties,
        ImagePixelLayout&    rPixelLayout);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRigidYUVFormat
    ///
    /// @brief  Get Rigid YUV format given the gralloc properties
    ///
    /// @param  rGrallocProperties Gralloc usage flags and other properties required for buffer allocation
    /// @param  rPixelLayout       Unique Image format structure
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetRigidYUVFormat(
        GrallocProperties&   rGrallocProperties,
        ImagePixelLayout&    rPixelLayout);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRigidRAWFormat
    ///
    /// @brief  Get rigid RAW  format given the gralloc properties
    ///
    /// @param  rGrallocProperties Gralloc usage flags and other properties required for buffer allocation
    /// @param  rPixelLayout       Unique Image format structure
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetRigidRAWFormat(
        GrallocProperties&   rGrallocProperties,
        ImagePixelLayout&    rPixelLayout);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapGrallocFormatToFormat
    ///
    /// @brief  Map gralloc format to camera defined format
    ///
    /// @param  grFormat  Gralloc format
    /// @param  rFormat   Camera format
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult MapGrallocFormatToFormat(
        GrallocFormat grFormat,
        Format&       rFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapOutputFormatTypeToFormat
    ///
    /// @brief  Map gralloc format given gralloc properties to camera defined format
    ///
    /// @param  outputFormat  Output format type for the settings
    /// @param  rFormat       Camera format
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult MapOutputFormatTypeToFormat(
        OutputFormatType outputFormat,
        Format&          rFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGrallocPixelFormat
    ///
    /// @brief  Get gralloc pixel format given gralloc properties
    ///
    /// @param  rGrallocProperties Gralloc usage flags and other properties required for buffer allocation
    /// @param  rGrPixelFormat     Gralloc pixel format
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetGrallocPixelFormat(
        GrallocProperties&  rGrallocProperties,
        GrallocPixelFormat&  rGrPixelFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraPixelFormat
    ///
    /// @brief  Get camera pixel format given gralloc properties
    ///
    /// @param  rGrallocProperties Gralloc usage flags and other properties required for buffer allocation
    /// @param  rCamPixelFormat    Camera format
    ///
    /// @return Camera format
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetCameraPixelFormat(
        GrallocProperties&   rGrallocProperties,
        CameraPixelFormat&   rCamPixelFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFlexibleFormat
    ///
    /// @brief  Returns if the gralloc format is a flexible format
    ///
    /// @param  grFormat Gralloc format
    ///
    /// @return TRUE/FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsFlexibleFormat(
        GrallocFormat grFormat)
    {
        BOOL isFlexibleFormat;

        switch (grFormat)
        {
            case GrallocFormatImplDefined:
            case GrallocFormatYCbCr420_888:
            case GrallocFormatRawOpaque:
                isFlexibleFormat = TRUE;
                break;

            default:
                isFlexibleFormat = FALSE;
                break;
        }

        return isFlexibleFormat;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsRAWFormat
    ///
    /// @brief  Returns if the gralloc format is a RAW format
    ///
    /// @param  grFormat Gralloc format
    ///
    /// @return TRUE/FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsRAWFormat(
        GrallocFormat grFormat)
    {
        BOOL isRAWFormat;

        switch (grFormat)
        {
            case GrallocFormatRaw8:
            case GrallocFormatRaw10:
            case GrallocFormatRaw12:
            case GrallocFormatRaw16:
            case GrallocFormatRaw64:
            case GrallocFormatRawOpaque:
                isRAWFormat = TRUE;
                break;

            default:
                isRAWFormat = FALSE;
                break;
        }

        return isRAWFormat;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsYUVFormat
    ///
    /// @brief  Returns if the gralloc format is a YUV format
    ///
    /// @param  grFormat Gralloc format
    ///
    /// @return TRUE/FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsYUVFormat(
        GrallocFormat grFormat)
    {
        BOOL isYUVFormat;

        switch (grFormat)
        {
            case GrallocFormatYCbCr420_SP:
            case GrallocFormatYCrCb420_SP:
            case GrallocFormatNV21ZSL:
            case GrallocFormatYUV420Flex1:
            case GrallocFormatYUV420Flex2:
            case GrallocFormatYUV420Flex3:
            case GrallocFormatYCbCr422SP:
            case GrallocFormatYCrCb422SP:
            case GrallocFormatNV12Venus:
            case GrallocFormatNV21Venus:
            case GrallocFormatNV21Adreno:
            case GrallocFormatNV12HEIF:
            case GrallocFormatCbYCrY:
            case GrallocFormatYCbCr444_SP:
            case GrallocFormatYCrCb444_SP:
            case GrallocFormatP010:
            case GrallocFormatPD10:
            case GrallocFormatYCbCr420_888:
                isYUVFormat = TRUE;
                break;

            default:
                isYUVFormat = FALSE;
                break;
        }

        return isYUVFormat;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CameraFormat
    ///
    /// @brief  Returns camera format enumeration given the image layout
    ///
    /// @param  pixelLayout       Unique Image format structure
    ///
    /// @return Camera Format enumeration
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static Format CameraFormat(
        ImagePixelLayout    pixelLayout)
    {
        return (TRUE == pixelLayout.isGrallocFormat) ?
            pixelLayout.u.grallocFormat.format :
            pixelLayout.u.cameraFormat.format;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdatePlaneLayoutInfo
    ///
    /// @brief  Internal API to update plane layout info in imageformat for a given Camx format.
    ///
    /// @param  pFormat           Camx image format info
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdatePlaneLayoutInfo(
        ImageFormat*        pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetYUVPlaneInfoFromExtLib
    ///
    /// @brief  Internal API to update plane layout info in imageformat for a given Camx format.
    ///
    /// @param  pixelFormat     Pixel Format
    /// @param  pImageFormat    Camx image format info
    ///
    /// @return CamxResultSuccess if successful, Errors specified by CamxResults otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetYUVPlaneInfoFromExtLib(
        UINT32          pixelFormat,
        ImageFormat*    pImageFormat);

    BOOL                        m_bQtiMapperUsed;       ///< Flag to indicate status of QtiMapper availability
    CamxFlexFormatInfo          m_yuvFlexFormatInfo;    ///< Alignment requirements for flex formats
    ExtLibGetPlanelayoutInfo    m_pFGetExtLibPlaneInfo; ///< External Format Library function ptrs

    sp<IQtiMapperExtensions> mQtiMapperExtensions;    ///< Qti Mapper Extensions member
};

CAMX_NAMESPACE_END

#pragma pack(pop)

#endif // CAMXIMAGEFORMATMAPPER_H
