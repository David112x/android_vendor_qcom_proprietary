////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcommontypes.h
/// @brief Declarations of common and wrapped types.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCOMMONTYPES_H
#define CAMXCOMMONTYPES_H

#include "camxcsl.h"
#include "camxdefs.h"
#include "camxtypes.h"
#include "camxutils.h"
#include "chivendortag.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Camera3CbOps;
struct Camera3Device;
struct Camera3StreamBufferSet;
struct HwDevice;
struct HwModuleMethods;
struct VendorTagQueryOps;
class  MetaBuffer;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Type Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Defines the prototype for the device close method
typedef INT (*CloseFunc)(HwDevice*);
/// @brief Defines the opaque type for Metadata
typedef VOID Metadata;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// system/camera_metadata.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Enumerates the valid types of vendor tags. This redefines the unnamed enum defined in system/camera_metadata.h.
///        Additional information can be found there.
enum class VendorTagType : UINT8
{
    Byte     = 0,   ///< Unsigned 8-bit integer
    Int32    = 1,   ///< Signed 32-bit integer
    Float    = 2,   ///< 32-bit float
    Int64    = 3,   ///< Signed 64-bit integer
    Double   = 4,   ///< 64-bit float
    Rational = 5,   ///< A 64-bit fraction
    Error    = 255, ///< Complies with COMMON API if an invalid vendor tag specified
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// system/graphics.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Enumerate the pixel formats. This enum redefines the unnamed enum defined in system/graphics.h. Additional
///        information can be found there.
enum HALPixelFormat
{
    // "Linear" color pixel formats. When used with ANativeWindow, the dataspace field describes the color space of the buffer.
    HALPixelFormatRGBA8888      = 0x00000001,   ///< RGBA8888
    HALPixelFormatRGBX8888      = 0x00000002,   ///< RGBX8888
    HALPixelFormatRGB888        = 0x00000003,   ///< RGB888
    HALPixelFormatRGB565        = 0x00000004,   ///< RGB565
    HALPixelFormatBGRA8888      = 0x00000005,   ///< BGRA8888

    // YUV pixel formats. When used with ANativeWindow, the dataspace field describes the color space of the buffer.
    HALPixelFormatYV12          = 0x32315659,   ///< YCrCb 4:2:0 Planar, 12-bits per pixel
    HALPixelFormatY8            = 0x20203859,   ///< Y-only Planar, 8-bits per pixel
    HALPixelFormatY16           = 0x20363159,   ///< Y-only Planar, 16-bits per pixel

    // Raw sensor pixel formats. When used with ANativeWindow, the dataspace should be HAL_DATASPACE_ARBITRARY
    HALPixelFormatRaw16         = 0x00000020,   ///< 16-bit per pixel, Bayer pattern sensor-dependent
    HALPixelFormatRaw10         = 0x00000025,   ///< 10-bit per pixel, densely packed per row, Bayer pattern sensor-dependent
    HALPixelFormatRaw12         = 0x00000026,   ///< 12-bit per pixel, densely packed per row, Bayer pattern sensor-dependent
    HALPixelFormatRawOpaque     = 0x00000024,   ///< Implementation-dependent

    // Generic pixel formats. When used with ANativeWindow, the dataSpace field describes the color space of the buffer.
    HALPixelFormatBlob          = 0x00000021,   ///< Carries data which does not have a standard image structure (e.g. JPEG)
    HALPixelFormatImplDefined   = 0x00000022,   ///< Format is up to the device-specific Gralloc implementation.

    // Flexible YCbCr 4:2:0 formats. When used with ANativeWindow, the dataSpace field describes the color space of the buffer.
    HALPixelFormatYCbCr420_888  = 0x00000023,   ///< Efficient YCbCr/YCrCb 4:2:0 buffer layout, layout-independent

    // Flexible YCbCr 4:2:2 formats
    HALPixelFormatYCbCr422_888  = 0x00000027,   ///< Efficient YCbCr/YCrCb 4:2:2 buffer layout, layout-independent

    // Flexible YCbCr 4:4:4 formats
    HALPixelFormatYCbCr444_888  = 0x00000028,   ///< Efficient YCbCr/YCrCb 4:2:2 buffer layout, layout-independent

    // Flexible RGB formats
    HALPixelFormatFlexRGB888    = 0x00000029,   ///< Efficient RGB/BGR/RGBX/BGRX buffer layout, layout-independent
    HALPixelFormatFlexRGBA8888  = 0x0000002A,   ///< Efficient RGBA/BGRA/ARGB/ABGR buffer layout, layout-independent

    // Legacy formats (deprecated), used by ImageFormat.java
    HALPixelFormatYCbCr422SP    = 0x00000010, ///< YCbCr 4:2:2 Planar, 16-bits per pixel (NV16)
    HALPixelFormatYCrCb420SP    = 0x00000011, ///< YCrCb 4:2:0 Planar, 12-bits per pixel (NV21)
    HALPixelFormatYCbCr422I     = 0x00000014  ///< YCbCr 4:2:2 Planar, 16-bits per pixel (YUY2)

    // Change the MaxHALPixelFormats below when adding new format.
};

/// @brief Maximum number of available HAL pixel formats.
static const UINT MaxHALPixelFormats = 22;

/// @brief Enumerates the way in which pixel values should be interpreted. This enum redefines the android_dataspace_t enum
///        defined in system/graphics.h. Additional information can be found there.
enum AndroidDataspace
{
    HALDataspaceUnknown             = 0x0000,                           ///< Default-assumption data space
    HALDataspaceArbitrary           = 0x0001,                           ///< Arbitrary dataspace
    HALDataspaceStandardShift       = 16,                               ///< Standard shift
    HALDataspaceTransferShift       = 22,                               ///< Transfer shift
    HALDataspaceRangeShift          = 27,                               ///< Range shift
    HALDataspaceStandardBT601_625   = 2 << HALDataspaceStandardShift,   ///< This adjusts the luminance interpretation
                                                                        ///  for RGB conversion from the one purely determined
                                                                        ///  by the primaries to minimize the color shift into
                                                                        ///  RGB space that uses BT.709 primaries.
    HALDataspaceTransferSmpte170M   = 3 << HALDataspaceTransferShift,   ///< BT.601 525, BT.601 625, BT.709, BT.2020
    HALDataspaceRangeFull           = 1 << HALDataspaceRangeShift,      ///< Full range uses all values for Y, Cb and Cr from
                                                                        ///  0 to 2 ^ b - 1, where b is the bit depth of the
                                                                        ///  color format.
    HALDataspaceJFIF                = 0x0101,                           ///< JPEG File Interchange Format(JFIF). Same model as
                                                                        ///  BT.601-625, but all YCbCr values range from
                                                                        ///  0 to 255.
    HALDataspaceV0JFIF              = HALDataspaceStandardBT601_625 |
                                      HALDataspaceTransferSmpte170M |
                                      HALDataspaceRangeFull,            ///< JPEG File Interchange Format(JFIF). Same model as
                                                                        ///  BT.601-625, but all YCbCr values range from
                                                                        ///  0 to 255.
    HALDataspaceBT601_625           = 0x0102,                           ///< ITU-R Recommendation BT.601 - 625line
                                                                        ///  (https://www.itu.int/rec/R-REC-BT.601/en)
    HALDataspaceBT601_525           = 0x0103,                           ///< ITU-R Recommendation BT.601 - 525line
                                                                        ///  (https://www.itu.int/rec/R-REC-BT.601/en)
    HALDataspaceBT709               = 0x0104,                           ///< ITU-R Recommendation BT.709
                                                                        ///  (https://www.itu.int/rec/R-REC-BT.709/en)
    HALDataspaceSRGBLinear          = 0x0200,                           ///< The red, green, and blue components are stored in
                                                                        ///  sRGB space, but are linear and not gamma-encoded.
    HALDataspaceSRGB                = 0x0201,                           ///< The red, green and blue components are stored in
                                                                        ///  sRGB space and converted to linear space
    HALDataspaceDepth               = 0x1000,                           ///< The buffer contains depth ranging measurements
                                                                        ///  from a depth camera.
    HALDataspaceJPEGAPPSegments     = 0x1003,                           ///< The buffer used for jpeg app segments for
                                                                        ///  HEIC or HEVC
    HALDataspaceHEIF                = 0x1004                            ///< The buffer used for HEIF format
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// hardware/gralloc.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Enumerates the valid operations on gralloc buffers. These values redefine the unnamed enum defined in
///        hardware/gralloc.h. Additional information can be found there.
typedef UINT32 GrallocUsage;
typedef UINT64 GrallocUsage64;
typedef UINT64 GrallocProducerUsage;
typedef UINT64 GrallocConsumerUsage;
typedef UINT64 GrallocPrivFlags;


static const GrallocUsage64 GrallocUsageSwReadNever       = 0x00000000;   ///< Buffer is never read in software
static const GrallocUsage64 GrallocUsageSwReadRarely      = 0x00000002;   ///< Buffer is rarely read in software
static const GrallocUsage64 GrallocUsageSwReadOften       = 0x00000003;   ///< Buffer is often read in software
static const GrallocUsage64 GrallocUsageSwReadMask        = 0x0000000F;   ///< Mask for the software read values

static const GrallocUsage64 GrallocUsageSwWriteNever      = 0x00000000;   ///< Buffer is never written in software
static const GrallocUsage64 GrallocUsageSwWriteRarely     = 0x00000020;   ///< Buffer is rarely written in software
static const GrallocUsage64 GrallocUsageSwWriteOften      = 0x00000030;   ///< Buffer is often written in software
static const GrallocUsage64 GrallocUsageSwWriteMask       = 0x000000F0;   ///< Mask for the software write values

static const GrallocUsage64 GrallocUsageHwTexture         = 0x00000100;   ///< Buffer will be used as an OpenGL ES texture
static const GrallocUsage64 GrallocUsageHwRender          = 0x00000200;   ///< Buffer will be used as an OpenGL ES render target
static const GrallocUsage64 GrallocUsageHw2D              = 0x00000400;   ///< Buffer will be used by the 2D hardware blitter
static const GrallocUsage64 GrallocUsageHwComposer        = 0x00000800;   ///< Buffer will be used by the HWComposer HAL module
static const GrallocUsage64 GrallocUsageHwFrameBuffer     = 0x00001000;   ///< Buffer will be used with the framebuffer device

static const GrallocUsage64 GrallocUsageExternalDisplay   = 0x00002000;   ///< Buffer should be displayed full-screen on an
                                                                        ///  external display when possible

static const GrallocUsage64 GrallocUsageProtected         = 0x00004000;   ///< Must have a hardware-protected path to external
                                                                        ///  display sink for this buffer.  If a hardware-
                                                                        ///  protected path is not available, then either don't
                                                                        ///  composite only this buffer (preferred) to the
                                                                        ///  external sink, or (less desirable) do not route the
                                                                        ///  entire composition to the external sink.

static const GrallocUsage64 GrallocUsageCursor            = 0x00008000;   ///< Buffer may be used as a cursor

static const GrallocUsage64 GrallocUsageHwVideoEncoder    = 0x00010000;   ///< Buffer will be used with the HW video encoder
static const GrallocUsage64 GrallocUsageHwCameraWrite     = 0x00020000;   ///< Buffer will be written by the HW camera pipeline
static const GrallocUsage64 GrallocUsageHwCameraRead      = 0x00040000;   ///< Buffer will be read by the HW camera pipeline
static const GrallocUsage64 GrallocUsageHwCameraZSL       = 0x00060000;   ///< Buffer will be used in zero-shutter-lag queue
static const GrallocUsage64 GrallocUsageHwCameraMask      = 0x00060000;   ///< Mask for the camera access values
static const GrallocUsage64 GrallocUsageHwMask            = 0x00071F00;   ///< Mask for the software usage bit-mask

static const GrallocUsage64 GrallocUsageRenderScript      = 0x00100000;   ///< Buffer will be used as a RenderScript Allocation

static const GrallocUsage GrallocUsageForeignBuffers    = 0x00200000;   ///< Set by the consumer to indicate to the producer
                                                                        ///  that they may attach a buffer that they did not
                                                                        ///  detach from the BufferQueue. Will be filtered out
                                                                        ///  by GrallocUsageAllocMask, so gralloc modules will
                                                                        ///  not need to handle this flag.
static const GrallocUsage64 GrallocUsageHwImageEncoder  = 0x8000000;    ///< Mask for the image encoding HEIF format

static const GrallocUsage GrallocUsageAllocMask         = ~(GrallocUsageForeignBuffers);    ///< Mask of all flags which could
                                                                                            ///  be passed to a gralloc module
                                                                                            ///  for buffer allocation. Any
                                                                                            ///  flags not in this mask do not
                                                                                            ///  need to be handled by gralloc
                                                                                            ///  modules.

static const GrallocUsage64 GrallocUsagePrivate0       = 0x10000000;   ///< UBWC usage flag
static const GrallocUsage64 GrallocUsagePrivate1       = 0x20000000;   ///< Implementation-specific private usage flag
static const GrallocUsage64 GrallocUsagePrivate3       = 0x80000000;   ///< Implementation-specific private usage flag
static const GrallocUsage64 GrallocUsagePrivateMask    = 0xF0000000;   ///< Implementation-specific private usage flag mask
static const GrallocUsage64 GrallocUsage10Bit          = 0x40000000;   ///< 10 bit usage flag
static const GrallocUsage64 GrallocUsageProducerCamera = (1 << 17);    ///< Camera Producer
static const GrallocUsage64 GrallocUsageConsumerCamera = (1 << 18);    ///< Camera Consumer
static const GrallocUsage64 GrallocUsageProducerVideoNV21Encoder = (1ULL << 48);       ///< Video NV21 Encoder Producer
static const GrallocUsage64 GrallocUsagePrivateAllocUBWCPI       = 0x0002000000000000; ///< UBWC PI usage flag

static const GrallocUsage64 GrallocUsagePrivateCDSP    = (1ULL << 50); ///< This flag is set while CDSP accesses the buffer

static const GrallocUsage64 GrallocUsageUBWC = GrallocUsagePrivate0;     ///< UBWC

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cutils/native_handle.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief A handle to a buffer. This structure redefines the native_handle_t structure defined in cutils/native_handle.h.
///        Additional information can be found there.
#ifdef _MSC_VER
// NOWHINE PR006: warning is disabled here in order to keep structure definition identical to Android
#pragma warning(disable : 4200)
#endif // _MSC_VER
struct NativeHandle
{
    INT version;    ///< The version of this structure (sizeof(NativeHandle))
    INT numFDs;     ///< Number of file-descriptors at &data[0]
    INT numInts;    ///< Number of ints at &data[numFds]

    INT data[0];    ///< numFDs + numInts ints. This field is included in the Android definition of the type, but is illegal in
                    ///  standard C/C++. For compilers that enforce this, like Visual C++, this results in a warning. The type
                    ///  has a size of 0 bytes, so it does not matter whether it is compiled. Therefore, comment it out to make
                    ///  it visually similar to the native_handle.h type.
};

/// @brief Mimicing the way handles are on the android side
typedef const NativeHandle* BufferHandle;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// hardware/camera3.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief This defines the general operation mode for the HAL (for a given stream configuration). This enum redefines the
///        camera3_stream_configuration_mode_t enum defined in hardware/camera3.h. Additional information can be found there.
enum Camera3StreamConfigMode
{
    StreamConfigModeNormal                  = 0x0000,   ///< Normal stream configuration
                                                        ///  operation mode
    StreamConfigModeConstrainedHighSpeed    = 0x0001,   ///< Special constrained high speed
                                                        ///  operation mode for devices that can
                                                        ///  not support high speed output in
                                                        ///  NORMAL mode
    StreamConfigModeVendorStart             = 0x8000,   ///< First value for vendor-defined
                                                        ///  stream configuration modes
    StreamConfigModeVendorEnd               = 0xEFFF,   ///< End value for vendor-defined
                                                        ///  stream configuration modes
    StreamConfigModeQTIStart                = 0xF000,   ///< First value for QTI-defined
                                                        ///  stream configuration modes
    StreamConfigModeQTITorchWidget          = 0xF001,   ///< Operation mode for Torch Widget.
    StreamConfigModeVideoHdr                = 0xF002,   ///< Video HDR On
    StreamConfigModeQTIEISRealTime          = 0xF004,   ///< Operation mode for Real-Time EIS recording usecase.
    StreamConfigModeQTIEISLookAhead         = 0xF008,   ///< Operation mode for Look Ahead EIS recording usecase.
    StreamConfigModeQTIFOVC                 = 0xF010,   ///< Field of View compensation in recording usecase.
    StreamConfigModeQTIFAEC                 = 0xF020,   ///< Fast AEC mode
    StreamConfigModeFastShutter             = 0xF040,   ///< FastShutter mode
    StreamConfigModeSuperSlowMotionFRC      = 0xF080,   ///< Super Slow Motion with FRC interpolation usecase.
    StreamConfigModeSensorMode              = 0xF0000,  ///< Sensor Mode selection for >=60fps
                                                        ///  require 4 bits for sensor mode index
};

/// @brief Request template types for camera3_device_ops.construct_default_request_settings(). This enum redefines the
/// camera3_request_template enum declared in hardware/camera3.h. Additional information can be found there.
enum Camera3RequestTemplate
{
    RequestTemplatePreview                 = 0x00000001,   ///< Standard camera preview operation with
                                                           ///  3A on auto
    RequestTemplateStillCapture            = 0x00000002,   ///< Standard camera high-quality still
                                                           ///  capture with 3A and flash on auto
    RequestTemplateVideoRecord             = 0x00000003,   ///< Standard video recording plus preview
                                                           ///  with 3A on auto, torch off
    RequestTemplateVideoSnapshot           = 0x00000004,   ///< High-quality still capture while
                                                           ///  recording video
    RequestTemplateZeroShutterLag          = 0x00000005,   ///< Zero-shutter-lag mode with 3A on auto
    RequestTemplateManual                  = 0x00000006,   ///< A basic template for direct application
                                                           ///  control of capture parameters
    RequestTemplateCount                   = 0x00000006,   ///< The number of request templates supported
    RequestTemplateVendorStart             = 0x40000000    ///< First value for vendor-defined request
                                                           ///  templates
};

/// @brief The type of the camera stream, which defines whether the camera HAL device is the producer or the consumer for that
///        stream, and how the buffers of the stream relate to the other streams. This enum redefines the camera3_stream_type_t
///        enum defined in hardware/camera3.h. Additional information can be found there.
enum Camera3StreamType
{
    StreamTypeOutput        = 0,    ///< This stream is an output stream
    StreamTypeInput         = 1,    ///< This stream is an input stream
    StreamTypeBidirectional = 2     ///< This stream can be used for input and output
};

/// @brief The required counterclockwise rotation of camera stream. This structure redefines the camera3_stream_rotation enum
///        defined in hardware/camera3.h. Additional information can be found there.
enum Camera3StreamRotation
{
    StreamRotationCCW0      = 0,    ///< No rotation
    StreamRotationCCW90     = 1,    ///< Rotate by 90 degree counterclockwise
    StreamRotationCCW180    = 2,    ///< Rotate by 180 degree counterclockwise
    StreamRotationCCW270    = 3     ///< Rotate by 270 degree counterclockwise
};

/// @brief The current status of a single stream buffer. This structure redefines the camera3_buffer_status_t enum defined in
///        hardware/camera3.h. Additional information can be found there.
enum Camera3BufferStatus
{
    BufferStatusOK      = 0,    ///< The buffer is in a normal state, and can be used after waiting on its sync fence.
    BufferStatusError   = 1     ///< The buffer does not contain valid data, and the data in it should not be used. The sync
                                ///  fence must still be waited on before reusing the buffer.
};

/// @brief Transport header for compressed JPEG buffers in output streams. This structure redefines the camera3_jpeg_blob
///        defined in hardware/camera3.h. Additional information can be found there.
struct Camera3JPEGBlob
{
    UINT16  JPEGBlobId;     ///< The JPEG blob ID field must be set to CAMERA3_JPEG_BLOB_ID
    UINT32  JPEGBlobSize;   ///< The JPEG sizein bytes
};

/// @brief Indicates the type of message sent, which specifies which member of the message union is valid. This structure
///        redefines the camera3_msg_type_t enum defined in hardware/camera3.h. Additional information can be found there.
enum Camera3MessageType
{
    MessageTypeError    = 1,    ///< An error has occurred
    MessageTypeShutter  = 2     ///< The exposure of a given request or processing a reprocess request has begun
};

/// @brief Error message codes. This structure redefines the camera3_error_msg_code_t enum defined in hardware/camera3.h.
///        Additional information can be found there.
enum Camera3ErrorMessageCode
{
    MessageCodeDevice    = 1,    ///< A serious failure occurred and no further frames will be produced by the device
    MessageCodeRequest   = 2,    ///< An error has occurred in processing a request and no output will be produced for this
                                 ///  request.
    MessageCodeResult    = 3,    ///< An error has occurred in producing an output result metadata buffer for a request, but
                                 ///  output stream buffers for it will still be available.
    MessageCodeBuffer    = 4,    ///< An error has occurred in placing an output buffer into a stream for a request. The frame
                                 ///  metadata and other buffers may still be available.
    MessageCodeRecovery  = 5,    ///< Recovery event from KMD
    MessageCodeSOFFreeze = 6     ///< The Kernel has raised an SOF Freeze notification
};

/// @brief Camera3 Hal Stream param for override pixel format and usage flags
struct Camera3HalStream
{
    INT32 id;                           ///<Stream ID - a nonnegative integer identifier for a stream.
    HALPixelFormat overrideFormat;      ///< An override pixel format for the buffers in this stream
    GrallocProducerUsage producerUsage; ///<The gralloc producer usage flags for this stream, as needed by the HAL.
    GrallocConsumerUsage consumerUsage; ///<The gralloc consumer usage flags for this stream, as needed by the HAL.
    UINT32 maxBuffers;
};

/// @brief A handle to a single camera input or output stream. This structure redefines the camera3_stream_t structure defined
///        in hardware/camera3.h. Additional information can be found there.
struct Camera3Stream
{
    // Set by framework before configure_streams()
    Camera3StreamType       streamType;          ///< The type of the stream
    UINT32                  width;               ///< The width in pixels of the buffers in this stream
    UINT32                  height;              ///< The height in pixels of the buffers in this stream
    HALPixelFormat          format;              ///< The pixel format for the buffers in this stream. Format is a value from the
                                                 ///< HALPixelFormat* list or from device-specific headers.

    // Set by HAL during configure_streams().
    GrallocUsage            grallocUsage;        ///< The gralloc usage (GRALLOC_USAGE_*) flags defined in gralloc.h for this stream,
                                                 ///  as needed by the HAL
    UINT32                  maxNumBuffers;       ///< The maximum number of buffers the HAL device may need to have dequeued at the
                                                 ///  same time.
    VOID*                   pPrivateInfo;        ///< A handle to HAL-private information for the stream that will not be inspected
                                                 ///  by the framework code.

    // Set by framework before configure_streams()
    AndroidDataspace        dataspace;           ///< A field that describes the contents of the buffer
    Camera3StreamRotation   rotation;            ///< The output rotation of the stream to be done in the HAL
    const CHAR*             pPhysicalCameraId;   ///< The physical camera id this stream belongs to
    Camera3HalStream*       pHalStream;          ///< Pointer to the HAL stream
    VOID*                   pReserved[5];        ///< Padding reserved for future use
};

/// @brief Encapsulates a single buffer from a camera3 stream. This structure redefines the camera3_stream_buffer structure
///        defined in hardware/camera3.h. Additional information can be found there.
struct Camera3StreamBuffer
{
    Camera3Stream*      pStream;        ///< The handle of the stream this buffer is associated with
    BufferHandle*       phBuffer;       ///< The native handle to the buffer
    Camera3BufferStatus bufferStatus;   ///< Current state of the buffer
    INT                 acquireFence;   ///< A handle to the acquire sync fence for this buffer. The HAL must wait on this
                                        ///  fence before attempting to read from or write to this buffer.
    INT                 releaseFence;   ///< A handle to the release sync fence for this buffer. The HAL must set this fence
                                        ///  when returning buffers to the framework, or write - 1 to indicate that no waiting
                                        ///  is required for this buffer.
};

/// @brief The result of a single capture or reprocess by the camera HAL device. This structure redefines the
///        camera3_capture_result structure defined in hardware/camera3.h. Additional information can be found there.
struct Camera3CaptureResult
{
    UINT32                      frameworkFrameNum;    ///< The unique frame number
    const Metadata*             pResultMetadata;      ///< The result metadata for this capture. Can be NULL.
    UINT32                      numOutputBuffers;     ///< The number of output buffers returned in this result structure
    const Camera3StreamBuffer*  pOutputBuffers;       ///< The handles for the output stream buffers for this capture
    const Camera3StreamBuffer*  pInputBuffer;         ///< The handle for the input stream buffer for this capture. Can be NULL.
    UINT32                      numPartialMetadata;   ///< The number of partial metadata results
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) //Android-P or better
    UINT32                      numPhyscamMetadata;   ///< The number of physical camera metadata this capture result contains
    const CHAR**                pPhyscamIds;          ///< The array of strings containing the physical camera ids
    const Metadata**            pPhyscamMetadata;     ///< The array of physical camera metadata for the physical cameras requested
#endif
};

/// @brief Encapsulates the error message information. This structure redefines the camera3_error_msg_t structure defined in
///        hardware/camera3.h. Additional information can be found there.
struct Camera3ErrorMessage
{
    UINT32                  frameworkFrameNum;  ///< Frame number of the request the error applies to. 0 if the frame number
                                                ///  isn't applicable to the error.
    Camera3Stream*          pErrorStream;       ///< Pointer to the stream that had a failure. NULL if the stream isn't
                                                ///  applicable to the error.
    Camera3ErrorMessageCode errorMessageCode;   ///< The code for this error
};

/// @brief Encapsulates the shutter message information. This structure redefines the camera3_shutter_msg_t structure defined in
///        hardware/camera3.h. Additional information can be found there.
struct Camera3ShutterMessage
{
    UINT32 frameworkFrameNum;       ///< Frame number of the request that has begun exposure or reprocessing.
    UINT64 timestamp;               ///< Timestamp for the start of capture
};

/// @brief The message structure sent to camera3_callback_ops_t.notify(). This structure redefines the camera3_notify_msg_t
///        structure defined in hardware/camera3.h. Additional information can be found there.
struct Camera3NotifyMessage
{
    Camera3MessageType messageType;             ///< The discriminator for the message union type below

    union
    {
        Camera3ErrorMessage     errorMessage;   ///< Error message contents. Valid if messageType is
                                                ///  Camera3MessageType::Error
        Camera3ShutterMessage   shutterMessage; ///< Shutter message contents. Valid if messageType is
                                                ///  Camera3MessageType::Shutter
        UINT8                   generic[32];    ///< Generic message contents used to ensure a minimum size for custom
                                                ///  message types.
    } message;
};

/// @brief Encapsulates input and output stream definitions. This structure redefines the camera3_stream_configuration_t
///        structure defined in hardware/camera3.h. Additional information can be found there.
struct Camera3StreamConfig
{
    UINT32                  numStreams;     ///< The total number of input and output streams in the streams list.
                                            ///  There must be at least one output-capable stream.
    Camera3Stream**         ppStreams;      ///< An array of camera stream pointers, defining the input/output
                                            ///  configuration for the camera HAL device.
    Camera3StreamConfigMode operationMode;  ///< The operation mode of streams in this configuration
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) //Android-P or better
    CameraMetadata*         pSessionParameters;    ///< Session metadata buffer
#endif
};

/// @brief A single request for image capture/buffer reprocessing, sent to the Camera HAL device by the framework. The request
///        contains the settings to be used for this capture, and the set of output buffers to write the resulting image data
///        in. It may optionally contain an input buffer, in which case the request is for reprocessing that input buffer
///        instead of capturing a new image with the camera sensor. The capture is identified by the frame number. This
///        structure redefines the camera3_capture_request_t structure defined in hardware/camera3.h. Additional information
///        can be found there.
struct Camera3CaptureRequest
{
    UINT32                      frameworkFrameNum;    ///< The unique frame number for this capture request
    const Metadata*             pMetadata;            ///< The metadata buffer that contains the capture and processing parameters
                                                      ///  for the request
    Camera3StreamBuffer*        pInputBuffer;         ///< The input stream buffer to use for this request, if any.
    UINT32                      numOutputBuffers;     ///< The number of output buffers for this capture request. Must be at
                                                      ///  least 1.
    const Camera3StreamBuffer*  pOutputBuffers;       ///< An array of num_output_buffers stream buffers, to be filled with image
                                                      ///  data from this capture / reprocess.The HAL must wait on the acquire
                                                      ///  fences of each stream buffer before writing to them.
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) //Android-P or better
    UINT32                      numPhyscamSettings;   ///< The number of physical camera settings to be applied
    const CHAR**                pPhyscamId;           ///< The physical camera ids
    const Metadata**            pPhyscamSettings;     ///< The capture settings for the physical cameras
#endif
};

/// @brief Encapsulates the function pointer type definitions that define the camera device operations. This structure redefines
///        the camera3_device_ops_t enum defined in hardware/camera3.h. Additional information can be found there.
struct Camera3DeviceOps
{
    /// @brief Defines the prototype for the COMMON API initialize method
    INT (*Initialize)(const Camera3Device*, const Camera3CbOps*);

    /// @brief Defines the prototype for the COMMON API configure_streams method
    INT (*ConfigureStreams)(const Camera3Device*, Camera3StreamConfig*);

    /// @brief Defines the prototype for the COMMON API register_stream_buffers method
    INT (*RegisterStreamBuffers)(const Camera3Device*, const Camera3StreamBufferSet*);

    /// @brief Defines the prototype for the COMMON API construct_default_request_settings method
    const Metadata* (*ConstructDefaultRequestSettings)(const Camera3Device*, INT);

    /// @brief Defines the prototype for the COMMON API construct_default_request_settings method
    INT (*ProcessCaptureRequest)(const Camera3Device*, Camera3CaptureRequest*);

    /// @brief Defines the prototype for the COMMON API get_metadata_vendor_tag_ops method
    VOID (*GetMetadataVendorTagOps)(const Camera3Device*, VendorTagQueryOps*);

    /// @brief Defines the prototype for the COMMON API dump method
    VOID (*Dump)(const Camera3Device*, INT);

    /// @brief Defines the prototype for the COMMON API flush method
    INT (*Flush)(const Camera3Device*);

    VOID* pReserved[8];  ///< Padding reserved for future use
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Encapsulates generic hardware module information. This structure redefines the hw_module_t structure defined in
///        hardware/hardware.h. Additional information can be found there.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct HwModule
{
    UINT32              tag;                ///< Must be initialized to HARDWARE_MODULE_TAG
    UINT16              moduleAPIVersion;   ///< The API version of the implemented module
    UINT16              HALAPIVersion;      ///< The API version of the HAL module interface to version the hw_module_t,
                                            ///  hw_module_methods_t, and hw_device_t structures and definitions.
    const CHAR*         pId;                ///< Identifier of module
    const CHAR*         pName;              ///< Name of this module
    const CHAR*         pAuthor;            ///< Author/owner/implementer of the module
    HwModuleMethods*    pMethods;           ///< Table of modules methods
    VOID*               pDSO;               ///< A module's DSO

#ifdef __LP64__
    UINT64              reserved[32 - 7];   ///< Padding reserved for future use
#else // __LP64__
    UINT32              reserved[32 - 7];   ///< Padding reserved for future use
#endif // __LP64__
};

/// @brief Encapsulates generic hardware device information. This structure redefines the hw_device_t structure defined in
///        hardware/hardware.h. Additional information can be found there.
struct HwDevice
{
    UINT32      tag;            ///< Must be initialized to HARDWARE_DEVICE_TAG
    UINT32      version;        ///< Version of the module-specific device API used to manage different device implementations.
    HwModule*   pModule;        ///< Reference to the module this device belongs to

#ifdef __LP64__
    UINT64      reserved[12];   ///< Padding reserved for future use
#else // __LP64__
    UINT32      reserved[12];   ///< Padding reserved for future use
#endif // __LP64__

    CloseFunc   close;          ///< Device close function
};

/// @brief Encapsulates the data associated with a camera device. This structure redefines the camera3_device_t structure
///        defined in hardware/camera3.h. Additional information can be found there.
struct Camera3Device
{
    HwDevice            hwDevice;       ///< Hardware device information for the
    Camera3DeviceOps*   pDeviceOps;     ///< Pointer to the camera device operations structure of function pointers
    VOID*               pPrivateData;   ///< Opaque pointer to HAL-private data that the framework will return to HAL
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// system/camera.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Enumerates the directions a image sensor can face. This enum redefines the unnamed enum defined in system/camera.h.
///        Additional information can be found there.
enum ImageSensorFacing
{
    ImageSensorFacingBack       = 0,    ///< The facing of the image sensor is opposite to that of the screen.
    ImageSensorFacingFront      = 1,    ///< The facing of the image sensor is the same as that of the screen.
    ImageSensorFacingExternal   = 2     ///< The facing of the image sensor is not fixed relative to the screen.
};

/// @brief Enumerates the orientation of the image. This enum defines the allowable integer values for orientation.
enum ImageOrientation
{
    ImageOrientationCW0     = 0,    ///< 0 degrees clockwise
    ImageOrientationCW90    = 90,   ///< 90 degrees clockwise
    ImageOrientationCW180   = 180,  ///< 180 degrees clockwise
    ImageOrientationCW270   = 270   ///< 270 degrees clockwise
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// hardware/camera_common.h Wrappers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Enumerates the camera device status. This enum redefines the camera_device_status_t enum defined in
///        hardware/camera_common.h. Additional information can be found there.
enum CameraDeviceStatus
{
    CameraDeviceStatusNotPresent    = 0,    ///< The camera device is not currently connected, and opening it will fail
    CameraDeviceStatusPresent       = 1,    ///< The camera device is connected, and opening it will succeed
    CameraDeviceStatusEnumerating   = 2,    ///< The camera device is connected, but it is undergoing an enumeration and so
                                            ///  opening the device will return -EBUSY.
};

/// @brief Enumerates the torch mode status. This enum redefines the torch_mode_status_t enum defined in
///        hardware/camera_common.h. Additional information can be found there.
enum TorchModeStatus
{
    TorchModeStatusNotAvailable = 0,    ///< The flash unit is no longer available and the torch mode can not be turned on
    TorchModeStatusAvailableOff = 1,    ///< The torch mode is now off and available to be turned on via set_torch_mode().
    TorchModeStatusAvailableOn  = 2     ///< The torch mode is now on and available to be turned off via set_torch_mode().
};

/// @brief Encapsulates static camera information. This structure redefines the camera_info_t structure defined in
///        hardware/camera_common.h. Additional information can be found there.
struct CameraInfo
{
    ImageSensorFacing       imageSensorFacing;          ///< The direction that the camera faces. See system/camera.h for
                                                        ///  definitions.
    ImageOrientation        imageOrientation;           ///< The orientation of the camera image. The value is the angle that
                                                        ///  the camera image needs to be rotated clockwise so it shows
                                                        ///  correctly on the display in its natural orientation.
    UINT32                  deviceVersion;              ///< Version of camera device API
    const Metadata*         pStaticCameraInfo;          ///< The camera's fixed characteristics, which include all static camera
                                                        ///  metadata.
    INT                     resourceCost;               ///< The total resource "cost" of using this camera, represented as an
                                                        ///  integer value in the range[0, 100] where 100 represents total usage
                                                        ///  of the shared resource that is the limiting bottleneck of the
                                                        ///  camera subsystem.
    CHAR**                  ppConflictingDevices;       ///< An array of camera device IDs represented as NULL-terminated
                                                        ///  strings indicating other devices that cannot be simultaneously
                                                        ///  opened while this camera device is in use.
    SIZE_T                  conflictingDevicesLength;   ///< The length of the array given in the ppConflictingDevices field.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Types for data exchange between HAL and Topology
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT MaxInputBuffers           = 40;
static const UINT MaxOutputBuffers          = 40;
static const UINT MaxMetadataBuffers        = 2;

/// @brief Opaque handle to a COMMONStream, to be used in Topology callbacks
typedef UINT64 StreamHandle;

/// @brief Different callback types from the Pipeline
enum class CbType
{
    Error,           ///< Error case
    Async,           ///< Async notify
    Metadata,        ///< Metadata available
    PartialMetadata, ///< Partial Metadata available
    EarlyMetadata,   ///< Early metadata available
    Buffer,          ///< Buffer done
    SOF,             ///< SOF event
    MetaBufferDone,  ///< Metadata buffer is written and ready to be recirculated
    Invalid          ///< Max out
};

/// @brief Error callback
struct CbPayloadError
{
    Camera3ErrorMessageCode code;               ///< Error code
    UINT32                  sequenceId;         ///< Sequence ID of the error
    UINT                    streamId;           ///< Stream Id, if applicable
    ChiBufferInfo           bufferInfo;         ///< Actual buffer if buffer error
};

/// @brief Async notification callback
struct CbPayloadAsync
{
    UINT64  requestID;                          ///< RequestID for tracking
    UINT32  sequenceId;                         ///< Sequence ID of the async notification
    UINT64  timestamp;                          ///< Timestamp
};

/// @brief SOF notification callback
struct CbPayloadSof
{
    UINT32  frameNum;                           ///< Frame number of the SOF notification
    UINT32  sequenceId;                         ///< Sequence ID of the async notification
    UINT64  timestamp;                          ///< Timestamp
    UINT32  bIsSequenceIdValid;                 ///< True if sequenceId is valid
};

/// @brief Payload for metadata callback
struct MetadataPayload
{
    MetaBuffer* pMetaBuffer[MaxMetadataBuffers];  ///< Array of metadata buffers
    UINT32      requestId;                        ///< Request Id for the metadata
};

/// @brief Payload for partial metadata callback
struct PartialMetadataPayload
{
    MetaBuffer* pMetaBuffer;  ///< MetadataBuffer
    UINT32      requestId;    ///< Request Id for the metadata
};

/// @brief Metadata available callback
struct CbPayloadMetadata
{
    UINT32          sequenceId;                     ///< Sequence ID of the callback
    MetadataPayload metaPayload;                    ///< Metadata payload
};

/// @brief Partial Metadata available callback
struct CbPayloadPartialMetadata
{
    UINT32                 sequenceId;                     ///< Sequence ID of the callback
    PartialMetadataPayload partialMetaPayload;             ///< Metadata payload
};

/// @brief Buffer done callback
struct CbPayloadBuffer
{
    UINT32          sequenceId;                 ///< Sequence ID for the callback
    ChiBufferInfo   bufferInfo;                 ///< Actual buffer payload
    UINT            streamId;                   ///< Stream Id
};

/// @brief Metabuffer done callback
struct CbPayloadMetaBufferDone
{
    UINT32          sequenceId;          ///< Sequence ID of the callback
    MetaBuffer*     pInputMetabuffer;    ///< Metadata buffer for input
    MetaBuffer*     pOutputMetabuffer;   ///< Metadata buffer for output
};

/// @brief Input buffer information
struct StreamInputBufferInfo
{
    ChiStreamBuffer      inputBuffer;   ///< Input buffer
    UINT                 portId;        ///< Port id which the input buffer is meant for
    CSLFence             fence;         ///< (CSL) Fence handle to be used for the input buffer
    BOOL                 isChiFence;    ///< boolean flag to check if chi allocated or Camx allocated fence
    volatile UINT        fenceRefCount; ///< refcount for fence if it is shared.
};

/// @brief Buffer information in the stream
struct StreamBufferInfo
{
    UINT32                  sequenceId;                        ///< ID assigned for the request
    UINT64                  originalFrameworkNumber;           ///< Original framework frame number for mapping
    UINT32                  numOutputBuffers;                  ///< Number of output buffers of the (possibly batched) request
    ChiStreamBuffer         outputBuffers[MaxOutputBuffers];   ///< Output buffers
    UINT32                  numInputBuffers;                   ///< Number of input buffers of the request
    StreamInputBufferInfo   inputBufferInfo[MaxInputBuffers];  ///< Input buffers
};

/// @brief multi request combo
struct MultiRequestSyncCombo
{
    BOOL    isMultiRequest;                         ///< The flag to indicate if it is multi requet
    UINT64  requestID[MaxPipelinesPerSession];      ///< Request ID of the pipeline in the session
    BOOL    isActive[MaxPipelinesPerSession];       ///< Indicate if the camera in the pipeline is active
    BOOL    isMaster[MaxPipelinesPerSession];       ///< Indicate if the camera in the pipeline is master w.r.t 3A processing
};

/// @brief multi request sync information
struct MultiRequestSyncData
{
    UINT64                  syncSequenceId;         ///< Multi request sync sequence ID
    UINT                    numPipelines;           ///< The number of pipelines in the session
    MultiRequestSyncCombo   currReq;                ///< The record of current Multi request
    MultiRequestSyncCombo   prevReq;                ///< The record of previous Multi request
};

/// @brief Peer request sync information
struct PeerSyncInfo
{
    BOOL    needSync;                       ///< The flag that indicates if we need to sync information from peer pipeline
    BOOL    isMaster;                       ///< The flag that indicates if it is Master or Slave
    UINT    peerPipelineID;                 ///< The peer pipeline ID that we need to sync information from
    INT64   requestDelta;                   ///< The request delta between own pipeline and peer pipeline which we need to sync
                                            ///  information from
};

/// @brief Capture request
struct CaptureRequest
{
    UINT                   numBatchedFrames;                        ///< Number of frames batched in this request
    BOOL                   HALOutputBufferCombined;                 ///< Is the HAL output buffer combined for batch mode
    UINT64                 requestId;                               ///< RequestId of the (possibly batched) request
    UINT64                 CSLSyncID;                               ///< syncID to synchronize csl Links
    const MetaBuffer*      pInputMetaBuffer;                        ///< Input Metadata of the (possibly batched) request
    MetaBuffer*            pOutputMetaBuffer;                       ///< Output Metadata of the (possibly batched) request
    UINT                   pipelineIndex;                           ///< Index of pipeline which this PCR belongs to
    VOID*                  pPrivData;                               ///< Private data for callback
    MultiRequestSyncData*  pMultiRequestData;                       ///< Multi request sync data pointer
    PeerSyncInfo           peerSyncInfo;                            ///< Holds peer sync information
    StreamBufferInfo*      pStreamBuffers;                          ///< Input buffer of the (possibly batched) request
    UINT32                 expectedExposureTime;                    ///< How long do we expect this request to take

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PartialDeepCopy
    ///
    /// @brief  Copies pSrc to pDst, but also does a deep copy of the contents of pStreamBuffers[pSrc->numBatchedFrames]
    ///         The other structures like pInputMetaBuffer etc are NOT deep copied.
    ///
    /// @param  pDst   Destination structure to copy into
    /// @param  pSrc   Source structure to copy from
    ///
    /// @return CamxResultSuccess   if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult PartialDeepCopy(
       struct CaptureRequest* pDst,
       struct CaptureRequest* pSrc)
    {
        CamxResult result = CamxResultSuccess;

        if ((NULL != pDst) && (NULL != pSrc) && (NULL != pDst->pStreamBuffers) && (NULL != pSrc->pStreamBuffers))
        {
            StreamBufferInfo* pDstSaveStreamBufferPtr = pDst->pStreamBuffers;
            UINT numBatchedFrames                     = GetBatchedHALOutputNum(pSrc);
            Utils::Memcpy(pDst, pSrc, sizeof(CaptureRequest));
            pDst->pStreamBuffers = pDstSaveStreamBufferPtr;
            Utils::Memcpy(pDst->pStreamBuffers, pSrc->pStreamBuffers, (sizeof(StreamBufferInfo) * numBatchedFrames));
        }
        else
        {
            result = CamxResultEInvalidArg;
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PartialClearData
    ///
    /// @brief  Clears the contents of the structure including the contents of the pStreamBuffers array,
    ///         but does not clear the pStreamBuffers pointer itself.
    ///
    /// @param  pData  Structure to clear
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID PartialClearData(
         struct CaptureRequest* pData)
    {
        if (NULL != pData)
        {
            StreamBufferInfo* pSaveStreamBufferPtr = pData->pStreamBuffers;
            UINT32            numBatchedFrames     = GetBatchedHALOutputNum(pData);

            Utils::Memset(pData, 0, sizeof(CaptureRequest));
            pData->pStreamBuffers = pSaveStreamBufferPtr;
            if (NULL != pSaveStreamBufferPtr)
            {
                Utils::Memset(pSaveStreamBufferPtr, 0, sizeof(StreamBufferInfo) * numBatchedFrames);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBatchedHALOutputNum
    ///
    /// @brief  Get the number of HAL output buffer for batch mode.
    ///         If the flag HALOutputBufferCombined is TRUE, the HAL output buffer will be combined into one single buffer.
    ///         Otherwise, one video output buffer maps to one HAL output buffer, so the output buffer number equal to
    ///         the numBatchedFrames.
    ///
    /// @param  pData  The CaptureRequest structure
    ///
    /// @return The number of HAL output buffer for batch mode.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT GetBatchedHALOutputNum(
        const struct CaptureRequest* pData)
    {
        UINT  numBatchedFrames = 1;
        if (NULL != pData && FALSE == pData->HALOutputBufferCombined)
        {
            numBatchedFrames = pData->numBatchedFrames;
        }

        return numBatchedFrames;
    }
};

/// @brief Session capture request
struct SessionCaptureRequest
{
    CaptureRequest    requests[MaxPipelinesPerSession]; ///< Capture request for all pipeline
    UINT32            numRequests;                      ///< NUmber of capture request
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Types for data to handle gralloc formats with HAL formats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Format info used to initialize format params
struct FormatParamInfo
{
    BOOL            isHALBuffer;                         ///< Is the buffer that uses this format a HAL buffer
    BOOL            isImplDefined;                       ///< Is the underlying HAL format Implementation Defined
    GrallocUsage64  grallocUsage;                        ///< The gralloc usage flags for this buffer
    UINT32          maxJPEGSize;                         ///< JPEGsize for max resolution in bytes
    UINT32          planeStride;                         ///< The number of bytes between the first byte of two sequential lines on plane 1. It may be
                                                         ///  greater than nWidth * nDepth / 8 if the line includes padding.
                                                         ///  Macro-tile width aligned for UBWC
    UINT32          sliceHeight;                         ///< The number of lines in the plane which can be equal to or larger than actual frame height.
                                                         ///  Tile height aligned for UBWC
    UINT32          UBWCVersionProducerUsage;            ///< producer usage for ubwc usage
    UINT32          UBWCVersionConsumerUsage;            ///< producer usage for ubwc usage
    UINT32          LossyUBWCProducerUsage;              ///< producer usage for ubwc usage
    UINT32          LossyUBWCConsumerUsage;              ///< producer usage for ubwc usage
    UINT32          bitWidth;                            ///< bits per pixel
    ChiStreamFormat pixelFormat;                         ///< StreamFormat enum (mandatory for external formats)
    UINT32          outputFormat;                        ///< outputFormat (mandatory for external formats)
    UINT32          yuvPlaneAlign;                       ///< Plane alignment for YUV formats from camxsettings.xml
    UINT32          debugDataSize;                       ///< Debug Data size to add in MaxJpegSize
};

CAMX_NAMESPACE_END

#endif // CAMXCOMMONTYPES_H
