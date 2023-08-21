////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchinodegpu.h
/// @brief Chi node for GPU Functionality
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCHINODEGPU_H
#define CAMXCHINODEGPU_H

#include <CL/opencl.h>
#include "camxchinodeutil.h"
#include "chinode.h"
#include "camxosutils.h"
#include <iterator>
#include <map>

typedef struct
{
    cl_mem     YUVImage;
    cl_mem     YImage;
    cl_mem     UVImage;
    INT        fd;
    VOID*      pAddr;
} MapList;

enum RotationAngle
{
    Rotate0Degrees,   ///< Rotation of 0 Degrees in the Clockwise Direction
    Rotate90Degrees,  ///< Rotation of 90 Degrees in the Clockwise Direction
    Rotate180Degrees, ///< Rotation of 180 Degress in the Clockwise Direction
    Rotate270Degrees  ///< Rotation of 270 Degrees in the Clockwise Direction
};

enum FlipDirection
{
    FlipNone,         ///< No Flip
    FlipLeftRight,    ///< Left-Right Flip (Vertical mirror)
    FlipTopBottom,    ///< Top-Bottom Flip (Horizontal mirror)
    FlipBoth          ///< Left-Right and Top-Bottom Flip (Vertical and Horizontal mirror)
};

enum CLImageFormats
{
    CLNV12Linear, ///< index to CL_QCOM_NV12 of supported CL format structure
    CLTP10Linear, ///< index to CL_QCOM_TP10 of supported CL format structure
    CLP010Linear, ///< index to CL_QCOM_P010 of supported CL format structure
    CLNV12UBWC,   ///< index to CL_QCOM_COMPRESSED_NV12 of supported CL format structure
    CLNV124RUBWC, ///< index to CL_QCOM_COMPRESSED_NV12_4R of supported CL format structure
    CLTP10UBWC    ///< index to CL_QCOM_COMPRESSED_TP10 of supported CL format structure
};

enum DownscaleOutputPortId
{
    FULL = 0,
    DS4  = 1,
    DS16 = 2,
    DS64 = 3,
    DS   = 4,
    DSb  = 5
};

/// @brief OpenCL image format info
struct CLImageFormatInfo
{
    UINT channelOrder;      ///< Channel Order Type
    UINT channelOrderY;     ///< Channel Order Type for Y Plane
    UINT channelOrderUV;    ///< Channel Order Type for UV Plane
    UINT channelDataType;   ///< Channel Data Type
    UINT channelDataTypeY;  ///< Channel Data Type for Y Plane
    UINT channelDataTypeUV; ///< Chanell Data Type for UV Plane
};

/// @brief Camera id bypass info
struct CameraBypassInfo
{
    UINT cameraId;    ///< The physical camera id belonging to this mapping
    UINT inputPortId; ///< The input port bound to cameraId
};

// NOWHINE FILE NC004c: Things outside the Camx namespace should be prefixed with Camx/CSL

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class containing functions to use OpenCL as part of the GPU Node.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GPUOpenCL
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyImage
    ///
    /// @brief  Copy image from input to output, it's a simple memory copy
    ///
    /// @param  hOutput Handle to the output buffer.
    /// @param  hInput  Handle to the input buffer.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CopyImage(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyBuffer
    ///
    /// @brief  Copy buffer from input to output
    ///
    /// @param  hOutput Handle to the output buffer.
    /// @param  hInput  Handle to the input buffer.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CopyBuffer(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RotateImage
    ///
    /// @brief  Read input and write the output according to the given rotation
    ///
    /// @param  hOutput        Handle to the output buffer.
    /// @param  hInput         Handle to the input buffer.
    /// @param  targetRotation The rotation angle in degrees
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult RotateImage(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput,
        RotationAngle       targetRotation);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlipImage
    ///
    /// @brief  Does a flip (mirror) to the image
    ///
    /// @param  hOutput            Handle to the output buffer.
    /// @param  hInput             Handle to the input buffer.
    /// @param  targetFlip         The direction of the flip.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FlipImage(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput,
        FlipDirection       targetFlip);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DownscaleBy4Image
    ///
    /// @brief  Downscale input by 4 and write to output image, using ds4 for Y plane and box filter for UV plane
    ///
    /// @param  hOutput        The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput         The CHINODEBUFFERHANDLE to output image buffer
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult DownscaleBy4Image(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DownscaleImage
    ///
    /// @brief  Downscale input to output image, using box filter for Y plane and UV plane
    ///
    /// @param  hOutput        The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput         The CHINODEBUFFERHANDLE to output image buffer
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult DownscaleImage(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteDownscaleBy4SinglePlane
    ///
    /// @brief  Downscale input by 4 and write to output  using ds4 algorithm
    ///
    /// @param  dst        Memory for destination plane
    /// @param  src        Memory for source plane
    /// @param  dstWidth   Width of the destination plane
    /// @param  dstHeight  Height of the destination plane;
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteDownscaleBy4SinglePlane(
        cl_mem          dst,
        cl_mem          src,
        UINT32          dstWidth,
        UINT32          dstHeight,
        FLOAT           srcYMax);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteBoxFilterSinglePlane
    ///
    /// @brief  Downscale input by scaleX and scaleY and write to output using box filter
    ///
    /// @param  dst        Memory for destination plane
    /// @param  src        Memory for source plane
    /// @param  scaleX     X scale for downscaling
    /// @param  scaleY     Y scale for downscaling
    /// @param  dstWidth   Width of the destination plane
    /// @param  dstHeight  Height of the destination plane;
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteBoxFilterSinglePlane(
        cl_mem          dst,
        cl_mem          src,
        FLOAT           scaleX,
        FLOAT           scaleY,
        UINT32          dstWidth,
        UINT32          dstHeight,
        FLOAT           srcYMax);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertP010ImageToPD10
    ///
    /// @brief  Convert Image from P010 format to PD10
    ///
    /// @param  hOutput        The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput         The CHINODEBUFFERHANDLE to input image buffer
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ConvertP010ImageToPD10(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertP010ImageToUBWCTP10
    ///
    /// @brief  Convert Image from P010 format to UBWCTP10
    ///
    /// @param  hOutput        The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput         The CHINODEBUFFERHANDLE to input image buffer
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ConvertP010ImageToUBWCTP10(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyUBWCTP10ImageToUBWCTP10
    ///
    /// @brief  Convert Image from P010 format to UBWCTP10
    ///
    /// @param  hOutput        The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput         The CHINODEBUFFERHANDLE to input image buffer
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CopyUBWCTP10ImageToUBWCTP10(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    enum CLInitStatus
    {
        CLInitInvalid = 0,
        CLInitRunning = 1,
        CLInitDone    = 2,
    };

    CLInitStatus m_initStatus;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize the resources required to use OpenCL with the GPU Node
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Uninitialize
    ///
    /// @brief  Cleans up the resources allocated to use OpenCL with the GPU Node
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Uninitialize();



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GPUOpenCL
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    GPUOpenCL();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~GPUOpenCL
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~GPUOpenCL();

    GPUOpenCL(const GPUOpenCL&) = delete;               ///< Disallow the copy constructor
    GPUOpenCL& operator=(const GPUOpenCL&) = delete;    ///< Disallow assignment operator
    BOOL m_bIsBufferMappingEnabled;                     ///< Tells if Buffer mapping enabled

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeFuncPtrs
    ///
    /// @brief  Loads the OpenCL implementation and initializes the required function pointers
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeFuncPtrs();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateProgram
    ///
    /// @brief  Create a OpenCL program object
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateProgram();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateProgramFromBinary
    ///
    /// @brief  Create a OpenCL program object from cached binary
    ///
    /// @param  context         An OpenCL context
    /// @param  device          Device id for which to create program
    /// @param  pFileName       Program binary file name
    ///
    /// @return An OpenCL program object if success or NULL for failure.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    cl_program CreateProgramFromBinary(
        cl_context   context,
        cl_device_id device,
        const CHAR*  pFileName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SaveProgramBinary
    ///
    /// @brief  Save compiled OpenCL program binary to a file
    ///
    /// @param  program          An OpenCL program object
    /// @param  device           Device id for the program object
    /// @param  pFileName        Program binary file name
    ///
    /// @return CDKResultSuccess if save binary success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SaveProgramBinary(
        cl_program   program,
        cl_device_id device,
        const CHAR*  pFileName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeKernel
    ///
    /// @brief  Initialize the OpenCL kernels for the supported use cases
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeKernel();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeResources
    ///
    /// @brief  Initialize the OpenCL resources for the supported use cases
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteCopyImage
    ///
    /// @brief  Execute the selected kernel
    ///
    /// @param  dst     Destination image for the copy.
    /// @param  src     Source image for the copy.
    /// @param  width   Image width in pixels.
    /// @param  height  Image height in pixels.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteCopyImage(
        cl_mem dst,
        cl_mem src,
        UINT32 width,
        UINT32 height);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteRotateImage
    ///
    /// @brief  Execute the selected kernel
    ///
    /// @param  dst        Destination image for the copy.
    /// @param  src        Source image for the copy.
    /// @param  dstWidth   Destination image width in pixels.
    /// @param  dstHeight  Destination image height in pixels.
    /// @param  srcWidth   Source image width in pixels.
    /// @param  srcHeight  Source image height in pixels.
    /// @param  rotation   Rotation angle
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteRotateImage(
        cl_mem          dst,
        cl_mem          src,
        UINT32          dstWidth,
        UINT32          dstHeight,
        UINT32          srcWidth,
        UINT32          srcHeight,
        RotationAngle   rotation);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteFlipImage
    ///
    /// @brief  Execute the selected kernel
    ///
    /// @param  dst         Destination image for the flip.
    /// @param  src         Source image for the flip.
    /// @param  width       Image width in pixels.
    /// @param  height      Image height in pixels.
    /// @param  inStride    Stride of the input buffer
    /// @param  outStride   Stride of the output buffer
    /// @param  inUVOffset  Offset of the input buffer to where the UV starts, 0 if flipping Luma plane
    /// @param  outUVOffset Offset of the output buffer to where the UV starts, 0 if flipping Luma plane
    /// @param  direction   Direction of the flip operation
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteFlipImage(
        cl_mem          dst,
        cl_mem          src,
        UINT32          width,
        UINT32          height,
        UINT32          inStride,
        UINT32          outStride,
        UINT32          inUVOffset,
        UINT32          outUVOffset,
        FlipDirection   direction);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteConvertP010ImageToPD10
    ///
    /// @brief  Convert input image format from P010 to PD10 format
    ///
    /// @param  dst         Destination image for the conversion.
    /// @param  src         Source image..
    /// @param  width       Image width in pixels.
    /// @param  height      Image height in pixels.
    /// @param  inStride    Stride of the input buffer
    /// @param  inScanlines Scanlines of the input buffer
    /// @param  outStride   Stride of the output buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteConvertP010ImageToPD10(
        cl_mem          dst,
        cl_mem          src,
        UINT32          width,
        UINT32          height,
        UINT32          inStride,
        UINT32          inScanlines,
        UINT32          outStride);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteConvertP010ImageToUBWCTP10
    ///
    /// @brief  Execute the selected kernel
    ///
    /// @param  dst     Destination Y image for the conversion.
    /// @param  dst     Destination UV image for the conversion.
    /// @param  src     Source image for the conversion.
    /// @param  width   Image width in pixels.
    /// @param  height  Image height in pixels.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteConvertP010ImageToUBWCTP10(
        cl_mem          dstY,
        cl_mem          dstUV,
        cl_mem          srcYUV,
        UINT32          width,
        UINT32          height);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteCopyUBWCTP10ImageToUBWCTP10
    ///
    /// @brief  Execute the selected kernel
    ///
    /// @param  dst     Destination Y image for the conversion.
    /// @param  dst     Destination UV image for the conversion.
    /// @param  src     Source image for the conversion.
    /// @param  width   Image width in pixels.
    /// @param  height  Image height in pixels.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteCopyUBWCTP10ImageToUBWCTP10(
        cl_mem          dstY,
        cl_mem          dstUV,
        cl_mem          srcYUV,
        UINT32          width,
        UINT32          height);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateSeparateYUVImagePlanes
    ///
    /// @brief  Create independent memory objects referring to the YUV image, and the Y and UV planes.
    ///
    /// @param  hBuffer             Handle to the buffer being mapped.
    /// @param  imageIndex          Image index within the buffer.
    /// @param  memoryAccessFlags   R/W Access flags.
    /// @param  pYUVImage           Pointer that contains the YUV image memory object.
    /// @param  pYImage             Pointer that contains the Y image memory object.
    /// @param  pUVImage            Pointer that contains the UV image memory object.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateSeparateYUVImagePlanes(
        CHINODEBUFFERHANDLE hBuffer,
        UINT                imageIndex,
        cl_mem_flags        memoryAccessFlags,
        cl_mem*             pYUVImage,
        cl_mem*             pYImage,
        cl_mem*             pUVImage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseSeparateYUVImagePlanes
    ///
    /// @brief  Release independent memory objects referring to the YUV image, and the Y and UV planes.
    ///
    /// @param  yuvImage    Pointer that contains the YUV image memory object.
    /// @param  yImage      Pointer that contains the Y image memory object.
    /// @param  uvImage     Pointer that contains the UV image memory object.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ReleaseSeparateYUVImagePlanes(
        cl_mem yuvImage,
        cl_mem yImage,
        cl_mem uvImage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseYUVImagePlanes
    ///
    /// @brief  Release independent memory objects referring to the YUV image, and the Y and UV planes.
    ///
    /// @return none
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseYUVImagePlanes();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertChiFormat
    ///
    /// @brief  Translate the input Chi Format to the equivalent OpenCL format
    ///
    /// @param  chiFormat Chi Format to be converted
    ///
    /// @return CLImageFormats image format type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CLImageFormats ConvertChiFormat(
        ChiFormat chiFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCompressedFormat
    ///
    /// @brief  Determine if the given format is compressed or not
    ///
    /// @param  format The format to check if compressed
    ///
    /// @return BOOL if given format is a compressed format
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsCompressedFormat(
        CLImageFormats format);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FindEntryinMapTable
    ///
    /// @brief  Find memory objects referring to the YUV image, Y and UV planes based on fd and virtual addr in map table.
    ///
    /// @param  fd                  file descriptor.
    /// @param  pAddr               Virtual address of Image buffer.
    /// @param  pYUVImage           Pointer that contains the YUV image memory object.
    /// @param  pYImage             Pointer that contains the Y image memory object.
    /// @param  pUVImage            Pointer that contains the UV image memory object.
    ///
    /// @return TRUE if entry found else FALSE.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL FindEntryinMapTable(
        INT      fd,
        VOID*    pAddr,
        cl_mem*  pYUVImage,
        cl_mem*  pYImage,
        cl_mem*  pUVImage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddEntryinMapTable
    ///
    /// @brief  Add a new entry in map including fd, virtual addr, memory objects referring to the YUV image, Y and UV planes.
    ///
    /// @param  fd                  file descriptor.
    /// @param  pAddr               Virtual address of Image buffer.
    /// @param  pYUVImage           Pointer that contains the YUV image memory object.
    /// @param  pYImage             Pointer that contains the Y image memory object.
    /// @param  pUVImage            Pointer that contains the UV image memory object.
    ///
    /// @return none.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID AddEntryinMapTable(
        INT      fd,
        VOID*    pAddr,
        cl_mem*  pYUVImage,
        cl_mem*  pYImage,
        cl_mem*  pUVImage);

    // Typedefs for OpenCL functions. Refer to the OpenCL documentation on Khronos website for parameter definitions

    /// @todo (CAMX-2286) Improvements to GPU Node Support: Add all the OpenCL 2.0 functions, not just the ones used.
    typedef CL_API_ENTRY cl_int (CL_API_CALL* PFNCLGETPLATFORMIDS)(cl_uint, cl_platform_id*, cl_uint*);
    typedef CL_API_ENTRY cl_int (CL_API_CALL* PFNCLGETDEVICEIDS)
                                    (cl_platform_id, cl_device_type, cl_uint, cl_device_id*, cl_uint*);

    typedef CL_API_ENTRY cl_context (CL_API_CALL* PFNCLCREATECONTEXT)
                                        (const cl_context_properties*,
                                         cl_uint,
                                         const cl_device_id*,
                                         void (CL_CALLBACK*)(const CHAR*, const VOID*, size_t, void *),
                                         VOID*,
                                         cl_int*);

    typedef CL_API_ENTRY cl_int (CL_API_CALL* PFNCLRELEASECONTEXT)(cl_context);


    typedef CL_API_ENTRY cl_command_queue (CL_API_CALL* PFNCLCREATECOMMANDQUEUE)(cl_context,
                                                                                 cl_device_id,
                                                                                 cl_command_queue_properties,
                                                                                 cl_int*);

    typedef CL_API_ENTRY cl_int (CL_API_CALL* PFNCLRELEASECOMMANDQUEUE)(cl_command_queue);

    typedef CL_API_ENTRY cl_mem (CL_API_CALL* PFNCLCREATEIMAGE2D)(cl_context,
                                                                  cl_mem_flags,
                                                                  const cl_image_format*,
                                                                  size_t,
                                                                  size_t,
                                                                  size_t,
                                                                  VOID*,
                                                                  cl_int*);


    typedef CL_API_ENTRY cl_mem (CL_API_CALL* PFNCLCREATEIMAGE)(cl_context,
                                                                cl_mem_flags,
                                                                const cl_image_format*,
                                                                const cl_image_desc*,
                                                                VOID*,
                                                                cl_int*);

    typedef CL_API_ENTRY cl_int (CL_API_CALL* PFNCLRELEASEMEMOBJECT)(cl_mem);

    typedef CL_API_ENTRY cl_program (CL_API_CALL* PFNCLCREATEPROGRAMWITHSOURCE)(cl_context,
                                                                                cl_uint,
                                                                                const CHAR**,
                                                                                const size_t*,
                                                                                cl_int*);

    typedef CL_API_ENTRY cl_program (CL_API_CALL* PFNCLCREATEPROGRAMWITHBINARY)(cl_context,
                                                                                cl_uint,
                                                                                const cl_device_id*,
                                                                                const size_t*,
                                                                                const UCHAR**,
                                                                                cl_int*,
                                                                                cl_int*);

    typedef CL_API_ENTRY cl_int (CL_API_CALL* PFNCLBUILDPROGRAM)(cl_program,
                                    cl_uint,
                                    const cl_device_id*,
                                    const CHAR*,
                                    void (CL_CALLBACK*)(cl_program, VOID*),
                                    VOID*);

    typedef CL_API_ENTRY cl_int     (CL_API_CALL* PFNCLRELEASEPROGRAM)(cl_program);

    typedef CL_API_ENTRY cl_int     (CL_API_CALL* PFNCLGETPROGRAMINFO)(cl_program,
                                                                       cl_program_info,
                                                                       size_t,
                                                                       VOID*,
                                                                       size_t*);

    typedef CL_API_ENTRY cl_int     (CL_API_CALL* PFNCLGETPROGRAMBUILDINFO)(cl_program,
                                                                            cl_device_id,
                                                                            cl_program_build_info,
                                                                            size_t,
                                                                            VOID*,
                                                                            size_t*);

    typedef CL_API_ENTRY cl_kernel  (CL_API_CALL* PFNCLCREATEKERNEL)(cl_program, const CHAR*, cl_int*);
    typedef CL_API_ENTRY cl_int     (CL_API_CALL* PFNRELEASEKERNEL)(cl_kernel);
    typedef CL_API_ENTRY cl_int     (CL_API_CALL* PFNCLSETKERNELARG)(cl_kernel, cl_uint, size_t, const VOID*);

    typedef CL_API_ENTRY cl_int     (CL_API_CALL* PFNCLENQUEUENDRANGEKERNEL)(cl_command_queue,
                                                                             cl_kernel,
                                                                             cl_uint,
                                                                             const size_t*,
                                                                             const size_t*,
                                                                             const size_t*,
                                                                             cl_uint,
                                                                             const cl_event*,
                                                                             cl_event*);

    typedef CL_API_ENTRY cl_int (CL_API_CALL* PFNCLENQUEUECOPYBUFFER)(cl_command_queue command_queue,
                                                                      cl_mem src_buffer,
                                                                      cl_mem dst_buffer,
                                                                      size_t src_offset,
                                                                      size_t dst_offset,
                                                                      size_t cb,
                                                                      cl_uint num_events_in_wait_list,
                                                                      const cl_event *event_wait_list,
                                                                      cl_event *event);

    typedef CL_API_ENTRY cl_int (CL_API_CALL* PFNCLFLUSH)(cl_command_queue);
    typedef CL_API_ENTRY cl_int (CL_API_CALL* PFNCLFINISH)(cl_command_queue);

    typedef CL_API_ENTRY cl_int (CL_API_CALL* PFNCLGETDEVICEINFO)(cl_device_id, cl_device_info, size_t, VOID*, size_t*);

    typedef CL_API_ENTRY cl_mem (CL_API_CALL* PFNCLCREATEBUFFER)(cl_context, cl_mem_flags, size_t, VOID*, cl_int *);

    typedef CL_API_ENTRY cl_sampler (CL_API_CALL* PFNCLCREATESAMPLER)(cl_context,
                                                                      cl_bool,
                                                                      cl_addressing_mode,
                                                                      cl_filter_mode,
                                                                      cl_int*);

    typedef CL_API_ENTRY cl_int (CL_API_CALL* PFNCLRELEASESAMPLER)(cl_sampler);

    CHILIBRARYHANDLE                m_hOpenCLLib;                   ///< handle for OpenCL library.

    PFNCLGETPLATFORMIDS             m_pfnCLGetPlatformIDs;          ///< Function pointer for clGetPlatformIDs
    PFNCLGETDEVICEIDS               m_pfnCLGetDeviceIDs;            ///< Function pointer for clGetDeviceIDs
    PFNCLCREATECONTEXT              m_pfnCLCreateContext;           ///< Function pointer for clCreateContext
    PFNCLRELEASECONTEXT             m_pfnCLReleaseContext;          ///< Function pointer for clReleaseContext
    PFNCLCREATECOMMANDQUEUE         m_pfnCLCreateCommandQueue;      ///< Function pointer for clCreateCommandQueue
    PFNCLRELEASECOMMANDQUEUE        m_pfnCLReleaseCommandQueue;     ///< Function pointer for clReleaseCommandQueue
    PFNCLCREATEIMAGE2D              m_pfnCLCreateImage2D;           ///< Function pointer for clCreateImage2D
    PFNCLCREATEIMAGE                m_pfnCLCreateImage;             ///< Function pointer for clCreateImage
    PFNCLRELEASEMEMOBJECT           m_pfnCLReleaseMemObject;        ///< Function pointer for clReleaseMemObject
    PFNCLCREATEPROGRAMWITHSOURCE    m_pfnCLCreateProgramWithSource; ///< Function pointer for clCreateProgramWithSource
    PFNCLCREATEPROGRAMWITHBINARY    m_pfnCLCreateProgramWithBinary; ///< Function pointer for clCreateProgramWithBinary
    PFNCLBUILDPROGRAM               m_pfnCLBuildProgram;            ///< Function pointer for clBuildProgram
    PFNCLRELEASEPROGRAM             m_pfnCLReleaseProgram;          ///< Function pointer for clReleaseProgram
    PFNCLGETPROGRAMINFO             m_pfnCLGetProgramInfo;          ///< Function pointer for clGetProgramInfo
    PFNCLGETPROGRAMBUILDINFO        m_pfnCLGetProgramBuildInfo;     ///< Function pointer for clGetProgramBuildInfo
    PFNCLCREATEKERNEL               m_pfnCLCreateKernel;            ///< Function pointer for clCreateKernel
    PFNRELEASEKERNEL                m_pfnCLReleaseKernel;           ///< Function pointer for clReleaseKernel
    PFNCLSETKERNELARG               m_pfnCLSetKernelArg;            ///< Function pointer for clSetKernelArg
    PFNCLENQUEUENDRANGEKERNEL       m_pfnCLEnqueueNDRangeKernel;    ///< Function pointer for clEnqueueNDRangeKernel
    PFNCLFLUSH                      m_pfnCLFlush;                   ///< Function pointer for clFlush
    PFNCLFINISH                     m_pfnCLFinish;                  ///< Function pointer for clFinish
    PFNCLGETDEVICEINFO              m_pfnCLGetDeviceInfo;           ///< Function pointer for clGetDeviceInfo
    PFNCLCREATEBUFFER               m_pfnCLCreateBuffer;            ///< Function pointer for clCreateBuffer
    PFNCLCREATESAMPLER              m_pfnCLCreateSampler;           ///< Function pointer for clCreateSampler
    PFNCLRELEASESAMPLER             m_pfnCLReleaseSampler;          ///< Function pointer for clReleaseSampler
    PFNCLENQUEUECOPYBUFFER          m_pfnCLEnqueueCopyBuffer;       ///< Function pointer for clEnqueueCopyBuffer

    cl_device_id                    m_device;                       ///< OpenCL GPU Device
    cl_context                      m_context;                      ///< OpenCL GPU Context
    cl_command_queue                m_queue;                        ///< OpenCL GPU Queue for Submissions
    cl_program                      m_program;                      ///< OpenCL Program with multiple Kernels
    cl_kernel                       m_copyImageKernel;              ///< Image Copy Kernel
    cl_kernel                       m_rotateImageKernel;            ///< Image Rotate Kernel
    cl_kernel                       m_flipImageKernel;              ///< Image Flip Kernel
    cl_kernel                       m_ds4SinglePlaneKernel;         ///< DS4 Single Plane Kernel
    cl_kernel                       m_boxFilterSinglePlaneKernel;   ///< Box Filter Single Plane Kernel
    cl_kernel                       m_p010topd10Kernel;             ///< Image convert from P010 to PD10 format
    cl_kernel                       m_p010totp10Kernel;             ///< Image convert from P010 to UBWCTP10 format
    cl_kernel                       m_tp10totp10Kernel;             ///< Image copy from UBWCTP10 to UBWCTP10 format
    cl_mem                          m_ds4WeightsImage;              ///< DS4 Weights Image
    cl_sampler                      m_ds4Sampler;                   ///< DS4 Sampler

    CamX::Mutex*                    m_pOpenCLMutex;                 ///< Mutex to protect the opecl buffer creation
    std::map<UINT32, MapList >      m_mapList;                      ///< Holds handle to YUV, UV and V plane.

    /// @todo (CAMX-2286) Improvements to GPU Node Support: Need to optimize these kernels and dispatch size.
    ///                   Also, allow loading program source from a file.
    // Program Source containing multiple kernels
    const CHAR* m_pProgramSource =
        "static const int Y_COMPONENT = 0;\n"
        "static const int U_COMPONENT = 1;\n"
        "static const int V_COMPONENT = 2;\n"
        "__kernel void tp10_to_tp10(__read_only  image2d_t src_tp10,\n"
        "                           __write_only image2d_t dest_tp10_y,\n"
        "                           __write_only image2d_t dest_tp10_uv,\n"
        "                           sampler_t sampler)\n"
        "{\n"
        "    const int    wid_x = get_global_id(0);\n"
        "    const int    wid_y = get_global_id(1);\n"
        "    const int2   read_coord = (int2)(6 * wid_x, wid_y);\n"
        "    const int2   y_write_coord = (int2)(6 * wid_x, wid_y);\n"
        "    const int2   uv_write_coord = (int2)(3 * wid_x, wid_y / 2);\n"
        "    const float4 pixels_in[] = {\n"
        "        read_imagef(src_tp10, sampler, read_coord),\n"
        "        read_imagef(src_tp10, sampler, read_coord + (int2)(1, 0)),\n"
        "        read_imagef(src_tp10, sampler, read_coord + (int2)(2, 0)),\n"
        "        read_imagef(src_tp10, sampler, read_coord + (int2)(3, 0)),\n"
        "        read_imagef(src_tp10, sampler, read_coord + (int2)(4, 0)),\n"
        "        read_imagef(src_tp10, sampler, read_coord + (int2)(5, 0)),\n"
        "        };\n"
        "    float        y_pixels_out[2][3] = {\n"
        "        { pixels_in[0].s0, pixels_in[1].s0, pixels_in[2].s0 },\n"
        "        { pixels_in[3].s0, pixels_in[4].s0, pixels_in[5].s0 },\n"
        "        };\n"
        "    float2       uv_pixels_out[3] = {\n"
        "        { pixels_in[0].s1, pixels_in[0].s2 },\n"
        "        { pixels_in[2].s1, pixels_in[2].s2 },\n"
        "        { pixels_in[4].s1, pixels_in[4].s2 },\n"
        "        };\n"
        "    qcom_write_imagefv_3x1_n10t00(dest_tp10_y, y_write_coord, y_pixels_out[0]);\n"
        "    qcom_write_imagefv_3x1_n10t00(dest_tp10_y, y_write_coord + (int2)(3, 0), y_pixels_out[1]);\n"
        "    if (wid_y % 2 == 0) qcom_write_imagefv_3x1_n10t01(dest_tp10_uv, uv_write_coord, uv_pixels_out);\n"
        "}\n"

        "__kernel void p010_to_tp10(__read_only  image2d_t src_p010,\n"
        "                           __write_only image2d_t dest_tp10_y,\n"
        "                           __write_only image2d_t dest_tp10_uv,\n"
        "                                        sampler_t sampler)\n"
        "{\n"
        "    const int    wid_x          = get_global_id(0);\n"
        "    const int    wid_y          = get_global_id(1);\n"
        "    const int2   read_coord     = (int2)(6 * wid_x, wid_y);\n"
        "    const int2   y_write_coord  = (int2)(6 * wid_x, wid_y);\n"
        "    const int2   uv_write_coord = (int2)(3 * wid_x, wid_y / 2);\n"
        "    const float4 pixels_in[]    = {\n"
        "        read_imagef(src_p010, sampler, read_coord               ),\n"
        "        read_imagef(src_p010, sampler, read_coord + (int2)(1, 0)),\n"
        "        read_imagef(src_p010, sampler, read_coord + (int2)(2, 0)),\n"
        "        read_imagef(src_p010, sampler, read_coord + (int2)(3, 0)),\n"
        "        read_imagef(src_p010, sampler, read_coord + (int2)(4, 0)),\n"
        "        read_imagef(src_p010, sampler, read_coord + (int2)(5, 0)),\n"
        "        };\n"
        "    float        y_pixels_out[2][3] = {\n"
        "        {pixels_in[0].s0, pixels_in[1].s0, pixels_in[2].s0},\n"
        "        {pixels_in[3].s0, pixels_in[4].s0, pixels_in[5].s0},\n"
        "        };\n"
        "    float2       uv_pixels_out[3]   = {\n"
        "        {pixels_in[0].s1, pixels_in[0].s2},\n"
        "        {pixels_in[2].s1, pixels_in[2].s2},\n"
        "        {pixels_in[4].s1, pixels_in[4].s2},\n"
        "        };\n"
        "    qcom_write_imagefv_3x1_n10t00(dest_tp10_y, y_write_coord,                y_pixels_out[0]);\n"
        "    qcom_write_imagefv_3x1_n10t00(dest_tp10_y, y_write_coord + (int2)(3, 0), y_pixels_out[1]);\n"
        "    if (wid_y % 2 == 0) qcom_write_imagefv_3x1_n10t01(dest_tp10_uv, uv_write_coord, uv_pixels_out);\n"
        "}\n"

        "__kernel void tp10_to_p010(__read_only  image2d_t src_image,\n"
        "                           __write_only image2d_t dest_p010_y,\n"
        "                           __write_only image2d_t dest_p010_uv,\n"
        "                                        sampler_t sampler)\n"
        "{\n"
        "    const int    wid_x              = get_global_id(0);\n"
        "    const int    wid_y              = get_global_id(1);\n"
        "    const float2 read_coord         = (float2)(4 * wid_x, 4 * wid_y) + 0.5;\n"
        "    const int2   y_write_coord      = (int2)(4 * wid_x, 4 * wid_y);\n"
        "    const int2   uv_write_coord     = (int2)(2 * wid_x, 2 * wid_y);\n"
        "    const float4 y_pixels_in[]      = {\n"
        "        qcom_read_imagef_2x2(src_image, sampler, read_coord,                    Y_COMPONENT),\n"
        "        qcom_read_imagef_2x2(src_image, sampler, read_coord + (float2)(2., 0.), Y_COMPONENT),\n"
        "        qcom_read_imagef_2x2(src_image, sampler, read_coord + (float2)(0., 2.), Y_COMPONENT),\n"
        "        qcom_read_imagef_2x2(src_image, sampler, read_coord + (float2)(2., 2.), Y_COMPONENT),\n"
        "        };\n"
        "    float        y_pixels_out[4][4] = {\n"
        "       {y_pixels_in[0].s3, y_pixels_in[0].s2, y_pixels_in[1].s3, y_pixels_in[1].s2},\n"
        "       {y_pixels_in[0].s0, y_pixels_in[0].s1, y_pixels_in[1].s0, y_pixels_in[1].s1},\n"
        "       {y_pixels_in[2].s3, y_pixels_in[2].s2, y_pixels_in[3].s3, y_pixels_in[3].s2},\n"
        "       {y_pixels_in[2].s0, y_pixels_in[2].s1, y_pixels_in[3].s0, y_pixels_in[3].s1},\n"
        "       };\n"
        "    qcom_write_imagefv_4x1_n10p00(dest_p010_y, y_write_coord,                y_pixels_out[0]);\n"
        "    qcom_write_imagefv_4x1_n10p00(dest_p010_y, y_write_coord + (int2)(0, 1), y_pixels_out[1]);\n"
        "    qcom_write_imagefv_4x1_n10p00(dest_p010_y, y_write_coord + (int2)(0, 2), y_pixels_out[2]);\n"
        "    qcom_write_imagefv_4x1_n10p00(dest_p010_y, y_write_coord + (int2)(0, 3), y_pixels_out[3]);\n"
        "    const float4 u_pixels_in       = qcom_read_imagef_2x2(src_image, sampler, read_coord, U_COMPONENT);\n"
        "    const float4 v_pixels_in       = qcom_read_imagef_2x2(src_image, sampler, read_coord, V_COMPONENT);\n"
        "    float2       uv_pixels_out[2][2] = {\n"
        "       {{u_pixels_in.s3, v_pixels_in.s3}, {u_pixels_in.s2, v_pixels_in.s2}},\n"
        "       {{u_pixels_in.s0, v_pixels_in.s0}, {u_pixels_in.s1, v_pixels_in.s1}},\n"
        "       };\n"
        "    qcom_write_imagefv_2x1_n10p01(dest_p010_uv, uv_write_coord,                uv_pixels_out[0]);\n"
        "    qcom_write_imagefv_2x1_n10p01(dest_p010_uv, uv_write_coord + (int2)(0, 1), uv_pixels_out[1]);\n"
        "}\n"

        "__kernel void copyImage(__write_only image2d_t dst, __read_only image2d_t src)\n"
        "{\n"
        "   int2   coord;\n"
        "   float4 color; \n"
        "   coord.x = get_global_id(0);  \n"
        "   coord.y = get_global_id(1);  \n"
        "   const sampler_t sampler1 = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST; \n"
        "   color = read_imagef(src, sampler1, coord);\n"
        "   write_imagef(dst, coord, color);\n"
        "}\n"

        "__kernel void rotateImage(__write_only image2d_t dst,"
        "                          __read_only image2d_t src,"
        "                          int dstWidth,"
        "                          int dstHeight,"
        "                          int srcWidth,"
        "                          int srcHeight,"
        "                          float sinTheta,"
        "                          float cosTheta)\n"
        "{\n"
        "    const int ix = get_global_id(0);\n"
        "    const int iy = get_global_id(1);\n"
        "    const int halfWidth = dstWidth / 2;\n"
        "    const int halfHeight = dstHeight / 2;\n"
        "    int2 dstCoord = (int2)(ix, iy);\n"
        "    int2 srcCoord;\n"
        "    int translatedX = ix - halfWidth;\n"
        "    int translatedY = iy - halfHeight;\n"
        "    float rotatedX = (((float) translatedX) * cosTheta + ((float) translatedY) * sinTheta);\n"
        "    float rotatedY = (((float) translatedX) * -sinTheta + ((float) translatedY) * cosTheta);\n"
        "    srcCoord.x = rotatedX + (srcWidth / 2);\n"
        "    srcCoord.y = rotatedY + (srcHeight / 2);\n"
        "    const sampler_t sampler1 = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST; \n"
        "    float4 color; \n"
        "    color = read_imagef(src, sampler1, srcCoord);\n"
        "    write_imagef(dst, dstCoord, color);\n"
        "}\n"

        "__kernel void flipImage(__global unsigned char* dst,\n"
        "                        __global unsigned char* src,\n"
        "                        int width,\n"
        "                        int height,\n"
        "                        unsigned int inStride,\n"
        "                        unsigned int outStride,\n"
        "                        unsigned int inUVOffset,\n"
        "                        unsigned int outUVOffset,\n"
        "                        int direction)\n"
        "{\n"
        "    int2   srcCoord;\n"
        "    int2   dstCoord;\n"
        "    float4 color; \n"
        "    dstCoord.x = get_global_id(0);  \n"
        "    dstCoord.y = get_global_id(1);  \n"
        "    const sampler_t sampler1 = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST; \n"
        "   if (0 == direction)\n"
        "   {\n"
        "       srcCoord.x = dstCoord.x;\n"
        "       srcCoord.y = dstCoord.y;\n"
        "   }\n"
        "   else if (1 == direction)\n"
        "   {\n"
        "       srcCoord.x = width - dstCoord.x - 1;\n"
        "        srcCoord.y = dstCoord.y;\n"
        "    }\n"
        "    else if (2 == direction)\n"
        "    {\n"
        "        srcCoord.x = dstCoord.x;\n"
        "        srcCoord.y = height - dstCoord.y - 1;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        srcCoord.x = width - dstCoord.x - 1;\n"
        "        srcCoord.y = height - dstCoord.y - 1;\n"
        "    }\n"
        "   if (0 == inUVOffset)\n"
        "   {\n"
        "        dst[(dstCoord.y * outStride) + dstCoord.x] = src[(srcCoord.y * inStride) + srcCoord.x];\n"
        "   }\n"
        "   else\n"
        "   {\n"
        "       dst[(dstCoord.y * outStride) + (dstCoord.x * 2) + outUVOffset] = \n"
        "           src[(srcCoord.y * inStride) + (srcCoord.x * 2) + inUVOffset];\n"
        "       dst[(dstCoord.y * outStride) + (dstCoord.x * 2) + 1 + outUVOffset] = \n"
        "           src[(srcCoord.y * inStride) + (srcCoord.x * 2) + 1 + inUVOffset];\n"
        "   }\n"
        "}\n"

        "__kernel void ds4_single_plane (  __read_only     image2d_t       src_image,\n"
        "                                  __write_only    image2d_t       downscaled_image,"
        "                                  __read_only     qcom_weight_image_t Kvalues,\n"
        "                                  sampler_t       sampler,\n"
        "                                  float           src_y_max\n"
        "                                                                                  )\n"
        "{\n"
        "  int     wid_x               = get_global_id(0);\n"
        "  int     wid_y               = get_global_id(1);\n"
        "  int2    dst_coord  = (int2)(wid_x, wid_y); \n"
        "  float4  result; \n"
        "  float2  src_coord            = convert_float2(dst_coord) * 4.0f + 0.5 + 0.5f;\n"
        "  result = qcom_convolve_imagef(src_image, sampler, src_coord, Kvalues);\n"
        "  src_coord.y = fmin(src_coord.y, src_y_max);\n"
        "  write_imagef(downscaled_image, dst_coord, result); \n"
        "}\n"

        "__kernel void boxfilter_single_plane(  __read_only     image2d_t               src_image,\n"
        "                                       __write_only    image2d_t               downscaled_image,\n"
        "                                                       sampler_t               sampler,\n"
        "                                                       qcom_box_size_t         scale,\n"
        "                                                       float2                  unpacked_scale,\n"
        "                                                       float                   src_y_max\n"
        "                                                                                              )\n"
        "{\n"
        "  int     wid_x           = get_global_id(0);\n"
        "  int     wid_y           = get_global_id(1);\n"
        "  float2  src_coord       = (float2)(((float)wid_x+0.5f)* unpacked_scale.x , ((float)wid_y+0.5f) * unpacked_scale.y);\n"
        "  int2    downscaled_coord= (int2)(wid_x, wid_y);\n"
        "  float4  downscaled_pixel= (float4)(0.0f, 0.0f, 0.0f, 0.0f);\n"
        "  src_coord.y = fmin(src_coord.y, src_y_max);\n"
        "  // Do down_scaling\n"
        "  downscaled_pixel = qcom_box_filter_imagef(src_image, sampler, src_coord, scale);\n"
        "  // Writeout the down_scaled pixel\n"
        "  write_imagef(downscaled_image, downscaled_coord, downscaled_pixel);\n"
        "}\n"

        "__kernel void p010_to_pd10(  __global unsigned char* dst,                             \n"
        "                             __global unsigned char* src,                             \n"
        "                             uint                    in_stride_bytes,                 \n"
        "                             uint                    in_scanline_bytes,               \n"
        "                             uint                    out_stride_bytes)                \n"
        "{                                                                                     \n"
        "  int           wid_x                   = get_global_id(0);                           \n"
        "  int           wid_y                   = get_global_id(1);                           \n"
        "  const ushort  in_stride_pixels        = (in_stride_bytes / 2);                      \n"
        "  const ushort  *src_luma16             = src;                                        \n"
        "  const ushort  *src_chroma16           = src + (in_stride_bytes * in_scanline_bytes);\n"
        "  const ushort  *src_luma_row           = src_luma16 + (in_stride_bytes * wid_y);     \n"
        "  const ushort  *src_chroma_row         = src_chroma16 + (in_stride_pixels * wid_y);  \n"
        "  uchar         *dst_row                = dst + (out_stride_bytes * wid_y);           \n"
        "  ushort        src_x0                  = (wid_x * 2);                                \n"
        "  ushort        src_x1                  = (src_x0 + 1);                               \n"
        "  ushort        dst_idx                 = (wid_x * 8);                                \n"
        "  ushort        y0, y1, y2, y3, cb, cr;                                               \n"
        "                                                \n"
        "  y0 = src_luma_row[src_x0]>>6;                 \n"
        "  y1 = src_luma_row[src_x1]>>6;                 \n"
        "  y2 = src_luma_row[in_stride_pixels+src_x0]>>6;\n"
        "  y3 = src_luma_row[in_stride_pixels+src_x1]>>6;\n"
        "  cb = src_chroma_row[src_x0]>>6;               \n"
        "  cr = src_chroma_row[src_x1]>>6;               \n"
        "                                                \n"
        "  dst_row[dst_idx++] = (uchar) (y0>>2);         \n"
        "  dst_row[dst_idx++] = (uchar) (y2>>2);         \n"
        "  dst_row[dst_idx++] = (uchar) (y1>>2);         \n"
        "  dst_row[dst_idx++] = (uchar) (y3>>2);         \n"
        "  dst_row[dst_idx++] = (uchar) (cb>>2);         \n"
        "  dst_row[dst_idx++] = (uchar) (cr>>2);         \n"
        "  dst_row[dst_idx++] = (uchar) ((y0 & 0x03) + ((y2 & 0x03) << 2) + ((y1 & 0x03) << 4) + ((y3 & 0x03) << 6));\n"
        "  dst_row[dst_idx++] = (uchar) (((cb & 0x03) + ((cr & 0x03) << 2)));\n"
        "        \n"
        "        \n"
        "        \n"
        "}\n";
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Chi GPU node class for Chi interface.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiGPUNode
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialization required to create a node
    ///
    /// @param  pCreateInfo Pointer to a structure that defines create session information for the node.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        CHINODECREATEINFO* pCreateInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryBufferInfo
    ///
    /// @brief  Implementation of PFNNODEQUERYBUFFERINFO defined in chinode.h
    ///
    /// @param  pQueryBufferInfo    Pointer to a structure to query the input buffer resolution and type.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult QueryBufferInfo(
        CHINODEQUERYBUFFERINFO* pQueryBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetBufferInfo
    ///
    /// @brief  Implementation of PFNNODESETBUFFERINFO defined in chinode.h
    ///
    /// @param  pSetBufferInfo  Pointer to a structure with information to set the output buffer resolution and type.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetBufferInfo(
        CHINODESETBUFFERPROPERTIESINFO* pSetBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessRequest
    ///
    /// @brief  Implementation of PFNNODEPROCREQUEST defined in chinode.h
    ///
    /// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessRequest(
        CHINODEPROCESSREQUESTINFO* pProcessRequestInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlushRequest
    ///
    /// @brief  Clear up node's internal resources for a request
    ///
    /// @param  pFlushRequestInfo Pointer to a structure that defines the information required for flushing a request.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FlushRequest(
        CHINODEFLUSHREQUESTINFO* pFlushRequestInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ComputeResponseTime
    ///
    /// @brief  Compute flush response time. Return worst-case flush response time in ms in CHINODERESPONSEINFO
    ///
    /// @param  pInfo Pointer to a structure that defines the information required for calculating response time.
    ///
    /// @return CDKResultSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ComputeResponseTime(
        CHINODERESPONSEINFO* pInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiGPUNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiGPUNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiGPUNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiGPUNode();
private:
    ChiGPUNode(const ChiGPUNode&) = delete;               ///< Disallow the copy constructor
    ChiGPUNode& operator=(const ChiGPUNode&) = delete;    ///< Disallow assignment operator

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDependencies
    ///
    /// @brief  Setting dependencies needed for the node
    ///
    /// @param  pProcessRequestInfo Pointer to a structure that defines the information required for processing a request.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDependencies(
        CHINODEPROCESSREQUESTINFO* pProcessRequestInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyImage
    ///
    /// @brief  Copy image from input to output, it's a simple memory copy
    ///
    /// @param  hOutput  The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput   The CHINODEBUFFERHANDLE to output image buffer
    ///
    /// @return CDKResultSuccess if success or CDKResultEFailed if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CopyImage(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyBuffer
    ///
    /// @brief  Copy buffer from input to output
    ///
    /// @param  hOutput  The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput   The CHINODEBUFFERHANDLE to output image buffer
    ///
    /// @return CDKResultSuccess if success or CDKResultEFailed if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CopyBuffer(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RotateImage
    ///
    /// @brief  Read input and write the output according to the given rotation
    ///
    /// @param  hOutput        The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput         The CHINODEBUFFERHANDLE to output image buffer
    /// @param  targetRotation The rotation amound in degrees
    ///
    /// @return CDKResultSuccess if success or CDKResultEFailed if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult RotateImage(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput,
        RotationAngle       targetRotation);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlipImage
    ///
    /// @brief  Read input and write the output according to the flip direction
    ///
    /// @param  hOutput            The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput             The CHINODEBUFFERHANDLE to output image buffer
    /// @param  targetFlip         The direction of the flip
    /// @param  currentOrientation Current orientation of the device
    ///
    /// @return CDKResultSuccess if success or CDKResultEFailed if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FlipImage(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput,
        FlipDirection       targetFlip,
        RotationAngle       currentOrientation);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DownscaleBy4Image
    ///
    /// @brief  Downscale input by 4 and write to output image, using ds4 for Y plane and box filter for UV plane
    ///
    /// @param  hOutput        The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput         The CHINODEBUFFERHANDLE to output image buffer
    ///
    /// @return CDKResultSuccess if success or CDKResultEFailed if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult DownscaleBy4Image(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DownscaleImage
    ///
    /// @brief  Downscale input to output output image, using box filter for Y plane and UV plane
    ///
    /// @param  hOutput        The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput         The CHINODEBUFFERHANDLE to output image buffer
    ///
    /// @return CDKResultSuccess if success or CDKResultEFailed if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult DownscaleImage(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertP010ImageToPD10
    ///
    /// @brief  Convert input image format from P010 to PD10 format
    ///
    /// @param  hOutput        The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput         The CHINODEBUFFERHANDLE to input image buffer
    ///
    /// @return CDKResultSuccess if success or CDKResultEFailed if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ConvertP010ImageToPD10(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertP010ImageToUBWCTP10
    ///
    /// @brief  Convert input image format from P010 to UBWCTP10 format
    ///
    /// @param  hOutput        The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput         The CHINODEBUFFERHANDLE to input image buffer
    ///
    /// @return CDKResultSuccess if success or CDKResultEFailed if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ConvertP010ImageToUBWCTP10(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyUBWCTP10ImageToUBWCTP10
    ///
    /// @brief  Copy input image format from UBWCTP10 to UBWCTP10 format
    ///
    /// @param  hOutput        The CHINODEBUFFERHANDLE to output image buffer
    /// @param  hInput         The CHINODEBUFFERHANDLE to input image buffer
    ///
    /// @return CDKResultSuccess if success or CDKResultEFailed if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CopyUBWCTP10ImageToUBWCTP10(
        CHINODEBUFFERHANDLE hOutput,
        CHINODEBUFFERHANDLE hInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadSkipVendorTag
    ///
    /// @brief  Read FDSkip Propertyneeded for the node
    ///
    /// @param  requestId                 Current requestid
    /// @param  bIsSkipForFDFromProperty  FD frame skip flag to be read from property
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReadSkipVendorTag(
        UINT64 requestId,
        BOOL* bIsSkipForFDFromProperty);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateOrientation
    ///
    /// @brief  Update the orientation data from metadata in the pipeline
    ///
    /// @param requestId    The request Id to update orientation metadata from
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateOrientation(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateFlip
    ///
    /// @brief  Update the flip data from metadata in the pipeline
    ///
    /// @param requestId    The request Id to update flip metadata from
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateFlip(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateMetaData
    ///
    /// @brief  Update the metadata in the pipeline
    ///
    /// @param  requestId   The request id for current request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateMetaData(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetaData
    ///
    /// @brief  Get the needed metadata from camera system
    ///
    /// @param  requestId   The request id for current request
    /// @param  tagId       The id of tag to get from camera system
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetMetaData(
        UINT64 requestId,
        UINT32 tagId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeDownscaleBypassMap
    ///
    /// @brief  Initializes bypass mapping for downscale mode
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeDownscaleBypassMap();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeDownscaleBufferManagers
    ///
    /// @brief  Initializes bypass mapping for downscale mode
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeDownscaleBufferManagers(
        UINT previewWidth,
        UINT previewHeight);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessDownscaleRequest
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessDownscaleRequest(
        CHINODEPROCESSREQUESTINFO* pProcessRequestInfo);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSupportedFormat
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSupportedFormat(
        ChiFormat format)
    {
        // Blacklist unsupported formats
        BOOL result;
        switch (format)
        {
            case ChiFormat::PD10:
                result = FALSE;
                break;
            default:
                result = TRUE;
                break;
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSupportedFormat
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsArbitraryDownscalerOnly(
        CHINODEQUERYBUFFERINFO *pQueryBufferInfo)
    {
        // Check if only arbitrary downscaling is executed - loop over all the ports
        // and if any are not DS/DSb, we are not in arbitrary downcaling only scenario.
        BOOL result = TRUE;
        for (UINT port = 0; port < pQueryBufferInfo->numOutputPorts; port++)
        {
            if (DS != pQueryBufferInfo->pOutputPortQueryInfo[port].outputPortId &&
                DSb != pQueryBufferInfo->pOutputPortQueryInfo[port].outputPortId)
            {
                result = FALSE;
                break;
            }
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DownscaleDivide
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT DownscaleDivide(
        UINT dividend,
        UINT divisor)
    {
        return ChiNodeUtils::AlignGeneric32(ChiNodeUtils::DivideAndCeil(dividend, divisor), 2);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsBypassableNode
    ///
    /// @brief  Get whether a node is bypassable or not
    ///
    /// @param  requestId   The request id for current request
    ///
    /// @return True if the node is bypassable otherwise false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsBypassableNode() const
    {
        return m_nodeFlags.isBypassable;
    }

    CHIHANDLE           m_hChiSession;      ///< The CHI session handle
    UINT32              m_nodeId;           ///< The node's Id
    UINT32              m_nodeInstanceId;   ///< The node's instance Id
    UINT32              m_nodeCaps;         ///< The selected node caps
    CHINODEIMAGEFORMAT  m_format;           ///< The selected format

    UINT64              m_processedFrame;   ///< The count for processed frame
    RotationAngle       m_currentRotation;  ///< If gpu rotation, use this to pass as metadata to dependant nodes
    FlipDirection       m_currentFlip;      ///< If gpu flip, use this to pass as metadata to dependant nodes
    UINT32              m_inDimensions[2];  ///< The input width x height dimension
    UINT32              m_fullOutputDimensions[2]; ///< The output width x height dimension

    CamX::Mutex*        m_pGpuNodeMutex;    ///< GPU node mutex

    GPUOpenCL*          m_pOpenCL;          ///< Local CL class instance pointer
    GPUOpenCL           m_openCL;           ///< CL class instance
    CHINODEFLAGS        m_nodeFlags;        ///< Node flags

    UINT                m_numPortMapping;                       ///< The number of input port mappings
    CameraBypassInfo    m_portMapDS4[MaxNumImageSensors];       ///< DS4 input port mapping
    CameraBypassInfo    m_portMapDS16[MaxNumImageSensors];      ///< DS16 input port mapping
    BOOL                m_bIsSkipProcessing;                    ///< Tells if EPR needs to skip
    UINT32              m_vendorTagPreviewStreamPresent;        ///< Holds vendor Tag base
    UINT32              m_vendorTagSkipGPUprocessingbasedonFD;  ///< Process skip based on FD Tag Id
    CHIBUFFERMANAGERHANDLE m_hBufferManagerDS4;
    CHIBUFFERMANAGERHANDLE m_hBufferManagerDS16;
    CHINODEBUFFERHANDLE    m_hTempBufferDS4;
    CHINODEBUFFERHANDLE    m_hTempBufferDS16;

};

#endif // CAMXCHINODEGPU_H
