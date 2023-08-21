/**=============================================================================

@file
   cvpOdt.h

@brief
   API Definitions for Object Detect Track (ODT)

Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//=============================================================================
///@details CVP ODT APIs using Computer Vision Processor acceleration
///@ingroup cvp_object_detection_tracking
//=============================================================================

#ifndef CVP_ODT_H
#define CVP_ODT_H

#include "cvpTypes.h"
#include "cvpObjectDetection.h"

#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
/// @brief
///    Structure for object detection and tracking (ODT) configuration.
/// @param nActualFps
///    Input frames per second. Default value is 30.
/// @param nOperationalFps
///    Desired output frames per second. nOperationalFps should be equal to
///    or greater than nActualFps. nOperationalFps >= nActualFps. Default value is 30.
/// @param sImageInfo
///    Structure for image information.
/// @param nOctaves
///    Number of octaves. Image size is halved between ajacent octaves.
/// @param nScalesPerOctave
///    Number of scales per octave. Total scales = num_oct*num_scales+1
/// @param nHorFlip
///    [Input] Bit flags to enable horizontal flip result.
///    Bit [i] corresponds to ith model (0-31).
/// @param nVerFlip
///    [Input] Bit flags to enable vertical flip result.
///    Bit [i] corresponds to ith model (0-31).
/// @param nHorVerFlip
///    [Input] Bit flags to enable horizontal and vertical flip result.
///    Bit [i] corresponds to ith model (0-31).
/// @param nNMSThreshold
///    Non-maximum suppression threshold. The value range is 0-255.
///    By default is 102 which mean 0.4 of 256. To get the integer value from
///    floating point non-maximum suppression threshold value,
///    <floating point value> * 256.
///
/// @ingroup cvp_object_detection_tracking
//------------------------------------------------------------------------------

typedef struct _cvpConfigOdt
{
   uint32_t     nActualFps;
   uint32_t     nOperationalFps;
   cvpImageInfo sImageInfo;
   uint32_t     nOctaves;
   uint32_t     nScalesPerOctave;
   uint32_t     nHorFlip;
   uint32_t     nVerFlip;
   uint32_t     nHorVerFlip;
   uint32_t     nNMSThreshold;

   #ifdef __cplusplus
   INLINE _cvpConfigOdt() : nActualFps(30),
                            nOperationalFps(30),
                            nOctaves(0),
                            nScalesPerOctave(0),
                            nHorFlip(0),
                            nVerFlip(0),
                            nHorVerFlip(0),
                            nNMSThreshold(102){}
   #endif
} cvpConfigOdt;


//------------------------------------------------------------------------------
/// @brief
///    Structure for ODT output buffer requirement.
/// @param nOdBytes
///    The required size in bytes for OD output.
/// @param nOdtBytes
///    The required size in bytes for ODT output.
///
/// @ingroup cvp_object_detection_tracking
//------------------------------------------------------------------------------
typedef struct _cvpOdtOutBuffReq
{
   uint32_t nOdBytes;
   uint32_t nOdtBytes;

#ifdef __cplusplus
   INLINE _cvpOdtOutBuffReq() : nOdBytes(0), nOdtBytes(0) {}
#endif

} cvpOdtOutBuffReq;

//------------------------------------------------------------------------------
/// @brief
///    Struct representing Object Detection Tracking (ODT) output
/// @param pOdOutput
///    OD output.  The data type for pOdOutput.pAddress
///    is a pointer to cvpOdBoundingBoxes structure.
/// @param pOdtOutput
///    ODT output. The data type for pOdtOutput.pAddress
///    is a pointer to cvpOdBoundingBoxes structure.
///
/// @ingroup cvp_object_detection_tracking
//------------------------------------------------------------------------------
typedef struct _cvpOdtOutput
{
   cvpMem *pOdOutput;                // nODResultBuffer
   cvpMem *pOdtOutput;               // nODTResultBuffer

   #ifdef __cplusplus
   INLINE _cvpOdtOutput() : pOdOutput(NULL),
                            pOdtOutput(NULL) {}
   #endif
} cvpOdtOutput;


//------------------------------------------------------------------------------
/// @brief
///    The ODT has been processed by the CVP system, and the
///    output is ready to be consumed. This callback is used to notify the
///    application. The application sends the input using the
///    cvpOdt_Async call.
///
/// @param eStatus
///    CVP status for the current process.
/// @param pOutput
///    Pointer to the ODT ouput structure.
/// @param hOdt
///    Handle for the ODT that was passed in the
///    cvpOdt_Async function.
/// @param pSessionUserData
///    User-data that was set in the cvpInitOdt structure.
/// @param pTaskUserData
///    User-data that was passed in the cvpOdt_Async function
///    which corresponds to this callback.
///
/// @ingroup cvp_object_detection_tracking
//------------------------------------------------------------------------------
typedef void(*CvpOdtCb)(cvpStatus     eStatus,
                        cvpOdtOutput *pOutput,
                        cvpHandle     hOdt,
                        void         *pSessionUserData,
                        void         *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Initialize CVP - Object Detection Tracking (ODT).
/// @param hSession
///    [Input] CVP session handle
/// @param pConfig
///    [Input] Pointer to the configuration structure
/// @param pOutMemReq
///    [Output] Pointer to the output memory requirement structure
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
/// @retval CVP handle for Object Detection Tracking (ODT).
///    If successful.
/// @retval NULL
///    If initialization failed.
///
/// @ingroup cvp_object_detection_tracking
//------------------------------------------------------------------------------
CVP_API cvpHandle cvpInitOdt(cvpSession          hSession,
                             const cvpConfigOdt *pConfig,
                             cvpOdtOutBuffReq   *pOutMemReq,
                             CvpOdtCb            fnCb,
                             void               *pSessionUserData);


//------------------------------------------------------------------------------
/// @brief
///    Deinitialize CVP - Object Detection Tracking (ODT).
/// @param hOdt
///    [Input] CVP handle for Object Detection Tracking (ODT).
///
/// @retval CVP_SUCCESS
///    If deinit is successful.
///
/// @ingroup cvp_object_detection_tracking
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeInitOdt(cvpHandle hOdt);

//------------------------------------------------------------------------------
/// @brief
///    Register ODT image buffer to the ODT handle.
/// @details
///    This API will register the image buffer and prepare any necessary scratch
///    buffer associated with this image.
/// @param hOdt
///    [Input] ODT handle.
/// @param pImage
///    [Input] Pointer to the image.
///
/// @retval CVP_SUCCESS
///    If registering is successful.
///
/// @ingroup cvp_object_detection_tracking
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpRegisterOdtImageBuf(cvpHandle       hOdt,
                                         const cvpImage *pImage);

//------------------------------------------------------------------------------
/// @brief
///    Deregister ODT image buffer from the ODT handle.
/// @details
///    This API will deregister the image buffer and free any necessary scratch
///    buffer associated with this image.
/// @param hOdt
///    [Input] ODT handle.
/// @param pImage
///    [Input] Pointer to the image.
///
/// @retval CVP_SUCCESS
///    If deregistering is successful.
///
/// @ingroup cvp_object_detection_tracking
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeregisterOdtImageBuf(cvpHandle       hOdt,
                                           const cvpImage *pImage);

//------------------------------------------------------------------------------
/// @brief
///    Object Detection Tracking (ODT).
/// @details
///    Synchronous (blocking) function that will do Object Detection Tracking (ODT).
/// @param hOdt
///    [Input] Handle for the Object Detection Tracking (ODT).
/// @param pRefImage
///    [Input] Pointer to the reference image.
/// @param pCurImage
///    [Input] Pointer to the current image.
/// @param pModel
///    [Input] Pretrained Object Detection (OD) model
/// @param pRefOdt
///    [Input] Pointer to the ODT Reference. The data type for
///    pRefOdt.pAddress is a pointer to cvpOdBoundingBoxes structure.
/// @param bNewRefImg
///    [Input] Flag to indicate if the reference image is new and has never been passed to
///    this API previously. Setting it to true will trigger cvp to prepare necessary
///    preprocessing step in calculating ODT. Setting it to false will notify
///    cvp to reuse the previous preprocessing associated with this image.
/// @param pOutput
///    [Output] Pointer to the ODT ouput structure.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_object_detection_tracking
//------------------------------------------------------------------------------

CVP_API cvpStatus cvpOdt_Sync(cvpHandle       hOdt,
                              const cvpImage *pRefImage,
                              const cvpImage *pCurImage,
                              const cvpMem   *pModel,
                              const cvpMem   *pRefOdt,
                              bool            bNewRefImg,
                              cvpOdtOutput   *pOutput);


//------------------------------------------------------------------------------
/// @brief
///    Object Detection Tracking (ODT).
/// @details
///    Asynchronous function that will queue the image and return almost
///    immediately. In the background, it will do Object Detection Tracking (ODT).
///    Once the output is ready, it will notify the user through the callback
///    function and user can queue another image using the same buffer.
/// @param hOdt
///    [Input] Handle for the Object Detection Tracking (ODT).
/// @param pRefImage
///    [Input] Pointer to the reference image.
/// @param pCurImage
///    [Input] Pointer to the current image.
/// @param pModel
///    [Input] Pretrained Object Detection (OD) model
/// @param pRefOdt
///    [Input] Pointer to the ODT Reference. The data type for
///    pRefOdt.pAddress is a pointer to cvpOdBoundingBoxes structure.
/// @param bNewRefImg
///    [Input] Flag to indicate if the reference image is new and has never been passed to
///    this API previously. Setting it to true will trigger cvp to prepare necessary
///    preprocessing step in calculating ODT. Setting it to false will notify
///    cvp to reuse the previous preprocessing associated with this image.
/// @param pOutput
///    [Output] Pointer to the ODT ouput structure.
/// @param pTaskUserData
///    [Input] User-data which corresponds to the callback.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter
/// @retval CVP_EUNSUPPORTED
///    If the handle is initialized for synchronous API.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpOdt_Async(cvpHandle       hOdt,
                               const cvpImage *pRefImage,
                               const cvpImage *pCurImage,
                               const cvpMem   *pModel,
                               const cvpMem   *pRefOdt,
                               bool            bNewRefImg,
                               cvpOdtOutput   *pOutput,
                               const void     *pTaskUserData);


#ifdef __cplusplus
}//extern "C"
#endif

#endif
