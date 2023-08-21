/**=============================================================================

@file
   cvpScale.h

@brief
   API Definitions for Scale

Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//=============================================================================
///@details CVP Pyramid APIs using Computer Vision Processor acceleration
///@ingroup cvp_image_transform
//=============================================================================

#ifndef CVP_SCALE_H
#define CVP_SCALE_H

#include "cvpTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
/// @brief
///    Image downscaling has been processed by the CVP system, and the
///    output is ready to be consumed. This callback is used to notify the
///    application. The application sends the image using the
///    cvpScaledown_Async call.
///
/// @param eStatus
///    CVP status for the current process.
/// @param pDst
///    Pointer to the downscaled output image.
/// @param hScaledown
///    Handle for the downscaling that was passed in the
///    cvpScaledown_Async function.
/// @param pSessionUserData
///    User-data that was set in the cvpInitScaledown structure.
/// @param pTaskUserData
///    User-data that was passed in the cvpScaledown_Async function
///    which corresponds to this callback.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
typedef void (*CvpScaledownCb)(cvpStatus           eStatus,
                               cvpImage           *pDst,
                               cvpHandle           hScaledown,
                               void               *pSessionUserData,
                               void               *pTaskUserData);


//------------------------------------------------------------------------------
/// @brief
///    Structure for Scaledown configuration use for clock voting purpose.
/// @param nActualFps
///    Input frames per second. Default value is 30.
/// @param nOperationalFps
///    Desired output frames per second. nOperationalFps should be equal to
///    or greater than nActualFps. nOperationalFps >= nActualFps. Default value is 30.
/// @param nMaxSrcWidth
///    Max src image width buffers for clock voting purpose.
/// @param nMaxSrcHeight
///    Max src image height buffers for clock voting purpose.
/// @param nMinDstWidth
///    Min dst image width buffers for clock voting purpose.
/// @param nMinDstHeight
///    Min dst image height buffers for clock voting purpose.
/// @param eSrcFormat
///    Src image color format.
/// @param eDstFormat
///    Dst image color format.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
typedef struct _cvpConfigScaleDown
{
   uint32_t       nActualFps;
   uint32_t       nOperationalFps;
   uint32_t       nMaxSrcWidth;
   uint32_t       nMaxSrcHeight;
   uint32_t       nMinDstWidth;
   uint32_t       nMinDstHeight;
   cvpColorFormat eSrcFormat;
   cvpColorFormat eDstFormat;

   #ifdef __cplusplus
   INLINE _cvpConfigScaleDown() : nActualFps(30),
                                  nOperationalFps(30),
                                  nMaxSrcWidth(1920),
                                  nMaxSrcHeight(1080),
                                  nMinDstWidth(640),
                                  nMinDstHeight(360),
                                  eSrcFormat(CVP_COLORFORMAT_GRAY_8BIT),
                                  eDstFormat(CVP_COLORFORMAT_GRAY_8BIT) {}
   #endif
} cvpConfigScaleDown;

//------------------------------------------------------------------------------
/// @brief
///    Initialize CVP - Scaledown.
/// @param hSession
///    [Input] CVP session handle.
/// @param pConfig
///    [Input] Pointer to the Scaledown configuration to set clock voting.
/// @param fnCb
///    [Input] Callback function for the asynchronous API
///    Setting to NULL will result in initializing for synchronous API
/// @param pSessionUserData
///    [Input] A private pointer that user can set with this session, and this
///    pointer will be provided as parameter to all callback functions
///    originated from the current session. This could be used to associate a
///    callback to this session. This can only be set once while initializing
///    the handle. This value will not/cannot-be changed for life
///    of a session.
///
/// @retval CVP handle for Scaledown
///    If successful.
/// @retval NULL
///    If initialization failed.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpHandle cvpInitScaledown(cvpSession          hSession,
                                   cvpConfigScaleDown *pConfig,
                                   CvpScaledownCb      fnCb,
                                   void               *pSessionUserData);

//------------------------------------------------------------------------------
/// @brief
///    Deinitialize CVP - Scaledown.
/// @param hScaledown
///    [Input] CVP handle for Scaledown
///
/// @retval CVP_SUCCESS
///    If deinit is successful.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeInitScaledown(cvpHandle hScaledown);

//------------------------------------------------------------------------------
/// @brief
///    Scaledown
/// @details
///    Synchronous (blocking) function that will do image downscaling
/// @param hScaledown
///    [Input] Handle for the downscaling.
/// @param nScaleDownInterpolation
///    [Input] This parameter controls the scaler interpolation strength.
///    Lower values result in sharper output images. Valid range is 0 to 3.
/// @param pSrc
///    [Input] Pointer to the CVP input image.
/// @param pDst
///    [Output] Pointer to the CVP output image. The output width, height,
///    and stride are based on this structure.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpScaledown_Sync(cvpHandle       hScaledown,
                                    uint32_t        nScaleDownInterpolation,
                                    const cvpImage *pSrc,
                                    cvpImage       *pDst);

//------------------------------------------------------------------------------
/// @brief
///    Scaledown
/// @details
///    Asynchronous function that will queue the image and returns almost
///    immediately. In the background, it will do downscaling.
///    Once the output is ready, it will notify the user through the callback
///    function and user can queue another image using the same buffer.
/// @param hScaledown
///    [Input] Handle for the downscaling.
/// @param nScaleDownInterpolation
///    [Input] This parameter controls the scaler interpolation strength.
///    Lower values result in sharper output images. Valid range is 0 to 3.
/// @param pSrc
///    [Input] Pointer to the CVP input image.
/// @param pDst
///    [Output] Pointer to the CVP output image. The output width, height,
///    and stride are based on this structure.
/// @param pTaskUserData
///    [Input] User-data which corresponds to the callback.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is initialized for the synchronous API.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpScaledown_Async(cvpHandle       hScaledown,
                                     uint32_t        nScaleDownInterpolation,
                                     const cvpImage *pSrc,
                                     cvpImage       *pDst,
                                     const void     *pTaskUserData);

#ifdef __cplusplus
}//extern "C"
#endif

#endif
