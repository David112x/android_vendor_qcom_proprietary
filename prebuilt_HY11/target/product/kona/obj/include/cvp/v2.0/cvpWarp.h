/**=============================================================================

@file
   cvpWarp.h

@brief
   API Definitions for image warping

Copyright (c) 2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//=============================================================================
///@details CVP Warp APIs using Computer Vision Processor acceleration
//=============================================================================

#ifndef CVP_WARP_H
#define CVP_WARP_H

#include "cvpTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define CVP_MAX_GRID_DATA            945
#define CVP_MAX_PRSP_PARAMS           128
#define CVP_MAX_GRID_DATA_SIZE       (CVP_MAX_GRID_DATA * sizeof(uint64_t))
#define CVP_MAX_PRSP_PARAMS_SIZE      (CVP_MAX_PRSP_PARAMS * sizeof(uint32_t))

//------------------------------------------------------------------------------
/// @brief
///    Warp method.
/// @param CVP_WARP_GRID
///    Warp grid.
/// @param CVP_WARP_PERSPECTIVE
///    Warp perspective.
/// @param CVP_WARP_PERSPECTIVE_GRID
///    Warp perspective and grid.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
typedef enum
{
   CVP_WARP_GRID = 0x1,
   CVP_WARP_PERSPECTIVE = 0x2,
   CVP_WARP_PERSPECTIVE_GRID = 0x3,
} cvpWarpMode;

//------------------------------------------------------------------------------
/// @brief
///    Interpolation method.
/// @param CVP_WARP_INTER_BILINEAR
///    Bilinear interpolation
/// @param CVP_WARP_INTER_BICUBIC
///    Bicubic interpolation
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
typedef enum
{
   CVP_WARP_INTER_BICUBIC,
   CVP_WARP_INTER_BILINEAR
} cvpWarpInterpolation;

//------------------------------------------------------------------------------
/// @brief
///    Pixel extrapolation method.
/// @param CVP_WARP_BORDER_CONSTANT
///    Border is replicated with a constant value, e.g. xxxxxx|abcdefgh|xxxxxx
/// @param CVP_WARP_BORDER_REPLICATE
///    Border is replicated with the border value, e.g. aaaaaa|abcdefgh|hhhhhh
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
typedef enum
{
   CVP_WARP_BORDER_CONSTANT = 0x0,
   CVP_WARP_BORDER_REPLICATE = 0x1
} cvpWarpBorderMode;

//------------------------------------------------------------------------------
/// @brief
///    Structure for Warp configuration.
/// @param nActualFps
///    Frame per second.
/// @param nOperationalFps
///    Operational frame per second.
/// @param srcImageInfo
///    Structure for the source image information. Only CVP_COLORFORMAT_NV12 is supported.
/// @param dstImageInfo
///    Structure for the destination image information. Only CVP_COLORFORMAT_NV12 is supported.
/// @param eMode
///    Warp mode.
/// @param eInter
///    Interpolation mode.
/// @param eInter
///    Interpolation mode.
/// @param eBorder
///    Pixel extrapolation method.
/// @param nBorderValue
///    Pixel extrapolation value.
/// @param nCtcPerspTransformGeomM
///    Width of the perspective transform partitioning geometry. Used in perspective mode only.
/// @param nCtcPerspTransformGeomN
///    Height of the perspective transform partitioning geometry. Used in perspective mode only.
/// @param nOpgInterpLUT0
///    Y interpolation coefficients look up table 0.
/// @param nOpgInterpLUT1
///    Y interpolation coefficients look up table 1.
/// @param nOpgInterpLUT2
///    Y interpolation coefficients look up table 2.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
typedef struct _cvpConfigWarp
{
   uint32_t             nActualFps;
   uint32_t             nOperationalFps;
   cvpImageInfo         srcImageInfo;
   cvpImageInfo         dstImageInfo;
   cvpWarpMode          eMode;
   cvpWarpInterpolation eInter;
   cvpWarpBorderMode    eBorder;
   uint32_t             nBorderValue;
   uint32_t             nCtcPerspTransformGeomM;
   uint32_t             nCtcPerspTransformGeomN;

   #ifdef __cplusplus
   INLINE _cvpConfigWarp() : nActualFps(30),
                             nOperationalFps(30),
                             eMode(CVP_WARP_PERSPECTIVE_GRID),
                             eInter(CVP_WARP_INTER_BICUBIC)
   {
   }
   #endif
} cvpConfigWarp;

//------------------------------------------------------------------------------
/// @brief
///    The Warp has been processed by the CVP system, and the
///    output is ready to be consumed. This callback is used to notify the
///    application. The application sends the image using the
///    cvpWarp_Async call.
///
/// @param eStatus
///    CVP status for the current process.
/// @param pSessionUserData
///    User-data that was set in the cvpInitWarp structure.
/// @param hWarp
///    Handle for the Warp that was passed in the cvpWarp_Async function.
/// @param pTaskUserData
///    User-data that was passed in the cvpWarp_Async function
///    which corresponds to this callback.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
typedef void (*CvpWarpCb)(cvpStatus    eStatus,
                          cvpImage    *pDstImage,
                          cvpHandle    hWarp,
                          void        *pSessionUserData,
                          void        *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Structure for Warp buffer requirement
/// @param nDstBufferSize
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
typedef struct _cvpWarpBuffReq
{
   uint32_t nDstBufferSize;

   #ifdef __cplusplus
   INLINE _cvpWarpBuffReq() : nDstBufferSize(0) {}
   #endif
} cvpWarpBuffReq;

//------------------------------------------------------------------------------
/// @brief
///    Initialize CVP - De-warping engine for distortion correction (Warp).
/// @param hSession
///    [Input] CVP session handle.
/// @param pConfig
///    [Input] Pointer to the Warp configuration.
/// @param pOutMemReq
///    [Output] Pointer to the output memory requirement.
/// @param fnCb
///    [Input] Callback function for the asynchronous API
///    Setting to NULL will result in initializing for synchronous API
/// @param pSessionUserData
///    [Input] A private pointer the user can set with this session, and this
///    pointer will be provided as parameter to all callback functions
///    originated from the current session. This could be used to associate a
///    callback to this session. This can only be set once while initializing
///    the handle. This value will not/cannot-be changed for life
///    of a session.
///
/// @retval CVP handle for Warp.
///    If successful.
/// @retval NULL
///    If initialization failed.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpHandle cvpInitWarp(cvpSession          hSession,
                              const cvpConfigWarp *pConfig,
                              cvpWarpBuffReq      *pOutMemReq,
                              CvpWarpCb           fnCb,
                              void                *pSessionUserData);

//------------------------------------------------------------------------------
/// @brief
///    Deinitialize CVP - De-warping engine for distortion correction (Warp).
/// @param hWarp
///    [Input] CVP handle for Warp.
///
/// @retval CVP_SUCCESS
///    If deinit is successful.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeInitWarp(cvpHandle hWarp);

//------------------------------------------------------------------------------
/// @brief
///    Warp Grid.
/// @details
///    Synchronous (blocking) function that will do warp grid.
/// @param hWarp
///    [Input] Handle for the Warp.
/// @param pSrcImage
///    [Input] Pointer to the source image.
/// @param pDstImage
///    [Output] Pointer to the destination image.
/// @param pGridBuffer
///    [Input] Pointer to the grid buffer.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is not initialized for CVP_WARP_GRID.
///    If the handle is initialized for the asynchronous API.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpWarpGrid_Sync(cvpHandle        hWarp,
                                   const cvpImage   *pSrcImage,
                                   cvpImage         *pDstImage,
                                   const cvpMem     *pGridBuffer);

//------------------------------------------------------------------------------
/// @brief
///    Warp Grid.
/// @details
///    Asynchronous function that will queue the image and return almost
///    immediately. In the background, it will do warp grid.
///    Once the output is ready, it will notify the user through the callback
///    function and user can queue another image using the same buffer.
/// @param hWarp
///    [Input] Handle for the Warp.
/// @param pSrcImage
///    [Input] Pointer to the source image.
/// @param pDstImage
///    [Output] Pointer to the destination image.
/// @param pGridBuffer
///    [Input] Pointer to the grid buffer.
/// @param pTaskUserData
///    [Input] Pointer to user-data buffer which corresponds to the callback.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is not initialized for CVP_WARP_GRID.
///    If the handle is initialized for the synchronous API.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpWarpGrid_Async(cvpHandle        hWarp,
                                    const cvpImage   *pSrcImage,
                                    cvpImage         *pDstImage,
                                    const cvpMem     *pGridBuffer,
                                    const void       *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Warp Perpective.
/// @details
///    Synchronous (blocking) function that will do warp perspective.
/// @param hWarp
///    [Input] Handle for the Warp.
/// @param pSrcImage
///    [Input] Pointer to the source image.
/// @param pDstImage
///    [Output] Pointer to the destination image.
/// @param pPrspBuffer
///    [Input] Pointer to the perspective buffer.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is not initialized for CVP_WARP_PERSPECTIVE.
///    If the handle is initialized for the asynchronous API.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpWarpPerspective_Sync(cvpHandle        hWarp,
                                          const cvpImage   *pSrcImage,
                                          cvpImage         *pDstImage,
                                          const cvpMem     *pPrspBuffer);

//------------------------------------------------------------------------------
/// @brief
///    Warp Perpective.
/// @details
///    Asynchronous function that will queue the image and return almost
///    immediately. In the background, it will do warp perspective.
///    Once the output is ready, it will notify the user through the callback
///    function and user can queue another image using the same buffer.
/// @param hWarp
///    [Input] Handle for the Warp.
/// @param pSrcImage
///    [Input] Pointer to the source image.
/// @param pDstImage
///    [Output] Pointer to the destination image.
/// @param pPrspBuffer
///    [Input] Pointer to the perspective buffer.
/// @param pTaskUserData
///    [Input] Pointer to user-data buffer which corresponds to the callback.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is not initialized for CVP_WARP_PERSPECTIVE.
///    If the handle is initialized for the synchronous API.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpWarpPerspective_Async(cvpHandle        hWarp,
                                           const cvpImage   *pSrcImage,
                                           cvpImage         *pDstImage,
                                           const cvpMem     *pPrspBuffer,
                                           const void       *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Warp Perpective and Grid.
/// @details
///    Synchronous (blocking) function that will do warp perspective and grid.
/// @param hWarp
///    [Input] Handle for the Warp.
/// @param pSrcImage
///    [Input] Pointer to the source image.
/// @param pDstImage
///    [Output] Pointer to the destination image.
/// @param pPrspBuffer
///    [Input] Pointer to the perspective buffer.
/// @param pGridBuffer
///    [Input] Pointer to the grid buffer.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is not initialized for CVP_WARP_PERSPECTIVE_GRID.
///    If the handle is initialized for the asynchronous API.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpWarpPerspectiveGrid_Sync(cvpHandle         hWarp,
                                              const cvpImage   *pSrcImage,
                                              cvpImage         *pDstImage,
                                              const cvpMem     *pPrspBuffer,
                                              const cvpMem     *pGridBuffer);

//------------------------------------------------------------------------------
/// @brief
///    Warp Perpective and Grid.
/// @details
///    Asynchronous function that will queue the image and return almost
///    immediately. In the background, it will do warp perspective and grid.
///    Once the output is ready, it will notify the user through the callback
///    function and user can queue another image using the same buffer.
/// @param hWarp
///    [Input] Handle for the Warp.
/// @param pSrcImage
///    [Input] Pointer to the source image.
/// @param pDstImage
///    [Output] Pointer to the destination image.
/// @param pPrspBuffer
///    [Input] Pointer to the perspective buffer.
/// @param pGridBuffer
///    [Input] Pointer to the grid buffer.
/// @param pTaskUserData
///    [Input] Pointer to user-data buffer which corresponds to the callback.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is not initialized for CVP_WARP_PERSPECTIVE_GRID.
///    If the handle is initialized for the synchronous API.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpWarpPerspectiveGrid_Async(cvpHandle         hWarp,
                                               const cvpImage   *pSrcImage,
                                               cvpImage         *pDstImage,
                                               const cvpMem     *pPrspBuffer,
                                               const cvpMem     *pGridBuffer,
                                               const void       *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Resample grid data for warp
/// @details
///    Resample grid from current grid resolution to Warp required virtual
///    domain grid resolution (945 = 35 x 27).
/// @param pRawGridX
///    [Input] Input grid mapping data in "x".
/// @param pRawGridY
///    [Input] Input grid mapping data in "y".
/// @param nGridWidth
///    [Input] Input grid width.
/// @param nGridHeight
///    [Input] Input grid height.
/// @param nImageWidth
///    [Input] Input image width.
/// @param nImageHeight
///    [Input] Input image height.
/// @param pWarpGrid
///    [Output] Warp requires grid mapping in 64bit binary format.
///    The memory has to be allocated by caller.
///
/// @retval None
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API void cvpWarpResampleGrid(float64_t  *pRawGridX,
                                 float64_t  *pRawGridY,
                                 uint32_t    nGridWidth,
                                 uint32_t    nGridHeight,
                                 uint32_t    nImageWidth,
                                 uint32_t    nImageHeight,
                                 int64_t    *pWarpGrid);

//------------------------------------------------------------------------------
/// @brief
///    Convert perspective transform parameters for Warp
/// @details
///    This function converts user warping matrix on input image resolution to
///    Warp required warping matrix.
/// @param pWarpMatrix
///    [Input] Input Warp matrix (3x3 floating point matrix).
/// @param nImageWidth
///    [Input] Input image width.
/// @param nImageHeight
///    [Input] Input image height.
/// @param pWarpPrsp
///    [Output] Warp expects perspective matrix in floating point in binary
///    format. Each element is 22 bit long with 16 bit mantissa and 6 bit
///    exponent. The output is 8 params. pWarpPrsp[2][2] = "1" is not included
///    in the result buffer.
///
/// @retval None
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API void  cvpWarpPerspectiveParams(float32_t *pWarpMatrix,
                                       uint32_t   nImageWidth,
                                       uint32_t   nImageHeight,
                                       int32_t   *pWarpPrsp);

#ifdef __cplusplus
}//extern "C"
#endif

#endif
