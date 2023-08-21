/**=============================================================================

@file
   cvpObjectDetection.h

@brief
   API Definitions for Object Detection

Copyright (c) 2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//=============================================================================
///@details CVP Object Detection (OD) APIs using Computer Vision Processor acceleration
///@ingroup cvp_object_detection
//=============================================================================

#ifndef CVP_OBJECT_DETECTION_H
#define CVP_OBJECT_DETECTION_H

#include "cvpTypes.h"

#define CVP_OD_MAX_OCTAVES           (4)
#define CVP_OD_MAX_SCALES_PER_OCTAVE (8)
#define CVP_OD_DEFAULT_OUTPUT_SIZE   (8004)

#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
/// @brief
///    Structure representing Object Detection (OD) bounding box information
///    that contains bounding box locations, ID, and score.
/// @param nStartX
///    Top left X location.
/// @param nStartY
///    Top left X location.
/// @param nEndX
///    Bottom right X location.
/// @param nEndY
///    Bottom right Y location.
/// @param nId
///    Model ID.
/// @param nReserved
///    Unused reseved for alignment.
/// @param nScore
///    Bounding box score.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpOdBoundingBox
{
   int16_t nStartX;
   int16_t nStartY;
   int16_t nEndX;
   int16_t nEndY;
   int16_t nId;
   int16_t nReserved;
   float   nScore;

   #ifdef __cplusplus
   INLINE _cvpOdBoundingBox() : nStartX(0),
                                 nStartY(0),
                                 nEndX(0),
                                 nEndY(0),
                                 nId(0),
                                 nReserved(0),
                                 nScore(0){}
   #endif
} cvpOdBoundingBox;

//------------------------------------------------------------------------------
/// @brief
///    Structure representing Object Detection (OD) bounding boxes.
/// @param nCount
///    Number of valid bounding box.
/// @param sBox
///    Bounding box information. The first nCount is the valid results.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpOdBoundingBoxes
{
   uint32_t           nCount;
   cvpOdBoundingBox *pBox;

   #ifdef __cplusplus
   INLINE _cvpOdBoundingBoxes() : nCount(0){}
   #endif
} cvpOdBoundingBoxes;

//------------------------------------------------------------------------------
/// @brief
///    Structure for Object Detection output buffer requirement.
/// @param nOutputBytes
///    The required size in bytes for Object Detection Output.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpObjectDetectionOutBuffReq
{
   uint32_t   nOutputBytes;

   #ifdef __cplusplus
   INLINE _cvpObjectDetectionOutBuffReq()
   {
      nOutputBytes = CVP_OD_DEFAULT_OUTPUT_SIZE;
   }
   #endif

} cvpObjectDetectionOutBuffReq;

//------------------------------------------------------------------------------
/// @brief
///    Structure for object detection (OD) configuration.
/// @param nActualFps
///    Input frames per second. Default value is 30.
/// @param nOperationalFps
///    Desired output frames per second. nOperationalFps should be equal to
///    or greater than nActualFps. nOperationalFps >= nActualFps. Default value is 30.
/// @param sImageInfo
///    Structure for image information. Only CVP_COLORFORMAT_NV12 and CVP_COLORFORMAT_NV12_UBWC
///    color format are supported.
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
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpConfigObjectDetection
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
   INLINE _cvpConfigObjectDetection() : nActualFps(30),
                                        nOperationalFps(30),
                                        nOctaves(0),
                                        nScalesPerOctave(0),
                                        nHorFlip(0),
                                        nVerFlip(0),
                                        nHorVerFlip(0),
                                        nNMSThreshold(102){}
   #endif
} cvpConfigObjectDetection;

//------------------------------------------------------------------------------
/// @brief
///    The Object Detection has been processed by the CVP system, and the
///    output is ready to be consumed. This callback is used to notify the
///    application. The application sends the image using the
///    cvpObjectDetection_Async call.
///
/// @param eStatus
///    CVP status for the current process.
/// @param pOutput
///    Pointer to the Object Detection ouput structure.
/// @param hObjectDetection
///    Handle for the Object Detection that was passed in the
///    cvpObject Detection_Async function.
/// @param pSessionUserData
///    User-data that was set in the cvpInitObjectDetection.
/// @param pTaskUserData
///    User-data that was passed in the cvpObjectDetection_Async function
///    which corresponds to this callback.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef void (*CvpObjectDetectionCb)(cvpStatus  eStatus,
                                      cvpMem    *pOutput,
                                      cvpHandle  hObjectDetection,
                                      void       *pSessionUserData,
                                      void       *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Initialize CVP - Object Detection (OD).
/// @param hSession
///    [Input] CVP session handle
/// @param pConfig
///    [Input] Pointer to the configuration.
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
/// @retval CVP handle for Object Detection (OD)
///    If successful.
/// @retval NULL
///    If initialization failed.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpHandle cvpInitObjectDetection(cvpSession                      hSession,
                                         const cvpConfigObjectDetection *pConfig,
                                         cvpObjectDetectionOutBuffReq   *pOutMemReq,
                                         CvpObjectDetectionCb            fnCb,
                                         void                           *pSessionUserData);

//------------------------------------------------------------------------------
/// @brief
///    Deinitialize CVP - Object Detection (OD).
/// @param ObjectDetection
///    [Input] CVP handle for Object Detection (OD).
///
/// @retval CVP_SUCCESS
///    If deinit is successful.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeInitObjectDetection(cvpHandle hObjectDetection);

//------------------------------------------------------------------------------
/// @brief
///    Object Detection (OD).
/// @details
///    Synchronous (blocking) function that will do Object Detection (OD).
/// @param hObjectDetection
///    [Input] Handle for the Object Detection (OD).
/// @param pImage
///    [Input] Pointer to the CVP image.
/// @param pModel
///    [Input] Pretrained Object Detection (OD) model
/// @param pOutput
///    [Output] Object Detection output. The data type for
///    pOutput.pAddress is a pointer to cvpOdBoundingBoxes structure.
///    The required memory size is at least the size fo cvpOdBoundingBoxes
///    structure.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpObjectDetection_Sync(cvpHandle       hObjectDetection,
                                             const cvpImage *pImage,
                                             const cvpMem   *pModel,
                                             const cvpMem   *pOutput);

//------------------------------------------------------------------------------
/// @brief
///    Object Detection (OD).
/// @details
///    Asynchronous function that will queue the image and return almost
///    immediately. In the background, it will do Object Detection (OD).
///    Once the output is ready, it will notify the user through the callback
///    function and user can queue another image using the same buffer.
/// @param hObjectDetection
///    [Input] Handle for the Object Detection (OD).
/// @param pImage
///    [Input] Pointer to the CVP image.
/// @param pModel
///    [Input] Pretrained Object Detection (OD) model
/// @param pOutput
///    [Output] Object Detection output. The data type for
///    pOutput.pAddress is a pointer to cvpOdBoundingBoxes structure.
///    The required memory size is at least the size fo cvpOdBoundingBoxes
///    structure.
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
CVP_API cvpStatus cvpObjectDetection_Async(cvpHandle       hObjectDetection,
                                              const cvpImage *pImage,
                                              const cvpMem   *pModel,
                                              const cvpMem   *pOutput,
                                              const void      *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Get Object Detection boxes information from the raw cvp memory
/// @details
///    Synchronous function that will translate raw cvpMem output into
///    cvpOdBoundingBoxes. This API is only working for the non-secure cvpMem output.
/// @param pOutput
///    [Input] Pointer to the raw cvp memory output.
/// @param pBoxes
///    [Output] Pointer to the Object Detection boxes information.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter, such as NULL pointer or pass secure pOutput
///    buffer.
/// @retval CVP_EFAIL
///    If pOutput can't be translated to pBoxes.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpObjectDetectionGetBoxes(const cvpMem       *pOutput,
                                                cvpOdBoundingBoxes *pBoxes);

#ifdef __cplusplus
}//extern "C"
#endif

#endif
