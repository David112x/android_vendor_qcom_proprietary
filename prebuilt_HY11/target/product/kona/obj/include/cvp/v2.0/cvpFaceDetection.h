/**=============================================================================

@file
   cvpFaceDetection.h

@brief
   API Definitions for Face Detection

Copyright (c) 2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//=============================================================================
///@details CVP Face Detection (FD) APIs using Computer Vision Processor acceleration
///@ingroup cvp_face_detection
//=============================================================================

#ifndef CVP_FACE_DETECTION_H
#define CVP_FACE_DETECTION_H

#include "cvpTypes.h"

#define CVP_FD_MAX_DETECT_FACE_NUM   100

#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
/// @brief
///    Enumeration defining minimum detection face size.
/// @details
///    Increasing the minimum face size will speed up face detection.
///    Reducing the minimum face size will slow down face detection.
/// @param CVP_FD_MIN_FACE_SIZE_25X25
///    25x25 minimum face size.
/// @param CVP_FD_MIN_FACE_SIZE_32X32
///    32x32 minimum face size.
/// @param CVP_FD_MIN_FACE_SIZE_40X40
///    40x40 minimum face size.
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
typedef enum {
   CVP_FD_MIN_FACE_SIZE_25X25 = 1,
   CVP_FD_MIN_FACE_SIZE_32X32,
   CVP_FD_MIN_FACE_SIZE_40X40
} cvpFdMinFaceSize;

//------------------------------------------------------------------------------
/// @brief
///    Enumeration defining supported detection direction.
/// @param CVP_FD_DETECT_UP_45
///    Up direction +/- 45 degree.
/// @param CVP_FD_DETECT_UP_75
///    Up direction +/- 75 degree.
/// @param CVP_FD_DETECT_UP_135
///    Up direction +/- 135 degree.
/// @param CVP_FD_DETECT_DOWN_45
///    Down direction +/- 45 degree.
/// @param CVP_FD_DETECT_DOWN_75
///    Down direction +/- 75 degree.
/// @param CVP_FD_DETECT_DOWN_135
///    Down direction +/- 135 degree.
/// @param CVP_FD_DETECT_LEFT_45
///    Left direction +/- 45 degree.
/// @param CVP_FD_DETECT_LEFT_75
///    Left direction +/- 75 degree.
/// @param CVP_FD_DETECT_LEFT_135
///    Left direction +/- 135 degree.
/// @param CVP_FD_DETECT_RIGHT_45
///    Right direction +/- 45 degree.
/// @param CVP_FD_DETECT_RIGHT_75
///    Right direction +/- 75 degree.
/// @param CVP_FD_DETECT_RIGHT_135
///    Right direction +/- 135 degree.
/// @param CVP_FD_DETECT_UP_DOWN_LEFT_RIGHT
///    Faces in all direction.
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
typedef enum {
   CVP_FD_DETECT_UP_45 = 0,
   CVP_FD_DETECT_UP_75,
   CVP_FD_DETECT_UP_135,
   CVP_FD_DETECT_DOWN_45,
   CVP_FD_DETECT_DOWN_75,
   CVP_FD_DETECT_DOWN_135,
   CVP_FD_DETECT_LEFT_45,
   CVP_FD_DETECT_LEFT_75,
   CVP_FD_DETECT_LEFT_135,
   CVP_FD_DETECT_RIGHT_45,
   CVP_FD_DETECT_RIGHT_75,
   CVP_FD_DETECT_RIGHT_135,
   CVP_FD_DETECT_UP_DOWN_LEFT_RIGHT
} cvpFdDetectDirection;

//------------------------------------------------------------------------------
/// @brief
///    Enumeration defining supported detection facial pose (yaw angle).
/// @param CVP_FD_DETECT_POSE_FRONT
///    Frontal face detection.
/// @param CVP_FD_DETECT_POSE_FRONT_HALF_PROFILE
///    Frontal and half profile face detection.
/// @param CVP_FD_DETECT_POSE_FRONT_HALF_FULL_PROFILE
///    Frontal, half, and full profile face detection.
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
typedef enum {
   CVP_FD_DETECT_POSE_FRONT = 0,
   CVP_FD_DETECT_POSE_FRONT_HALF_PROFILE,
   CVP_FD_DETECT_POSE_FRONT_HALF_FULL_PROFILE
} cvpFdDetectPose;

//------------------------------------------------------------------------------
/// @brief
///    Structure representing face information
///    that contains bounding box locations, score, and angles.
/// @param nCenterX
///    Center point X coordinate.
/// @param nCenterY
///    Center point Y coordinate.
/// @param nSize
///    Face size. The width and the height of face bounding box.
/// @param nScore
///    Confidence score. Range is between 0 and 999.
/// @param nAngle
///    Roll angle in degree. Range is between 0 and 360. It has +/-15 degree accuracy.
///    The value will be multiply of 30, i.e. 0, 30, 60, etc.
/// @param nPose
///    Yaw angle in degree. Possible values are -90 (left side), -45, 0, 45,
///    and 90 (right side).
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
typedef struct _cvpFdFaceInfo
{
   int16_t nCenterX;
   int16_t nCenterY;
   int16_t nSize;
   int16_t nScore;
   int16_t nAngle;
   int16_t nPose;

   #ifdef __cplusplus
   INLINE _cvpFdFaceInfo() : nCenterX(0),
                             nCenterY(0),
                             nSize(0),
                             nScore(0),
                             nAngle(0),
                             nPose(0){}
   #endif
} cvpFdFaceInfo;


//------------------------------------------------------------------------------
/// @brief
///    Structure representing face detection output
/// @param nCount
///    Number of valid faces.
/// @param sFace
///    Faces information. The first nCount is the valid results.
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
typedef struct _cvpFdOutput
{
   uint32_t      nCount;
   cvpFdFaceInfo sFace[CVP_FD_MAX_DETECT_FACE_NUM];

   #ifdef __cplusplus
   INLINE _cvpFdOutput() : nCount(0){}
   #endif
} cvpFdOutput;

//------------------------------------------------------------------------------
/// @brief
///    Structure for Face Detection buffer requirement.
/// @param nOutputBytes
///    The required size in bytes for Face Detection Output.
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
typedef struct _cvpFdBuffReq
{
   uint32_t   nOutputBytes;

   #ifdef __cplusplus
   INLINE _cvpFdBuffReq()
   {
      nOutputBytes = 0;
   }
   #endif

} cvpFdBuffReq;

//------------------------------------------------------------------------------
/// @brief
///    Structure for face detection (FD) configuration.
/// @param nActualFps
///    Input frames per second. Default value is 30.
/// @param nOperationalFps
///    Desired output frames per second. nOperationalFps should be equal to
///    or greater than nActualFps. nOperationalFps >= nActualFps. Default value is 30.
/// @param sImageInfo
///    Structure for image information. Supported color format are
///    CVP_COLORFORMAT_NV12 and CVP_COLORFORMAT_NV12_UBWC.
/// @param eMinDetectionSize
///    Minimum detection face size.
/// @param eDetectDirection
///    Detection direction.
/// @param eDetectPose
///    Detection facial pose (yaw angle).
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
typedef struct _cvpConfigFaceDetection
{
   uint32_t             nActualFps;
   uint32_t             nOperationalFps;
   cvpImageInfo         sImageInfo;
   cvpFdMinFaceSize     eMinDetectionSize;
   cvpFdDetectDirection eDetectDirection;
   cvpFdDetectPose      eDetectPose;

   #ifdef __cplusplus
   INLINE _cvpConfigFaceDetection() : nActualFps(30),
                                      nOperationalFps(30),
                                      eMinDetectionSize(CVP_FD_MIN_FACE_SIZE_25X25),
                                      eDetectDirection(CVP_FD_DETECT_UP_45),
                                      eDetectPose(CVP_FD_DETECT_POSE_FRONT){}
   #endif
} cvpConfigFaceDetection;

//------------------------------------------------------------------------------
/// @brief
///    Structure for Face Detection threshold.
/// @param nFront
///    Frontal pose (yaw) confidence threshold. Default: TBD.
///    Range value is between 0 and 999.
/// @param nHalf
///    Half profile pose (yaw) confidence threshold. Default: TBD.
///    Range value is between 0 and 999.
/// @param nFull
///    Full profile pose (yaw) confidence threshold. Default: TBD.
///    Range value is between 0 and 999.
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
typedef struct _cvpFdThresh
{
   uint32_t nFront;
   uint32_t nHalf;
   uint32_t nFull;

   #ifdef __cplusplus
   INLINE _cvpFdThresh() : nFront(0),
                           nHalf(0),
                           nFull(0) {}
   #endif
} cvpFdThresh;

//------------------------------------------------------------------------------
/// @brief
///    Structure for face detection (FD) frame configuration.
/// @param sThresh
///    Structure for Face Detection threshold.
/// @param bEnableRoi
///    Flag to enable region of interest (ROI).
/// @param sRoi
///    Structure for the region of interest (ROI).
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
typedef struct _cvpFdFrameConfig
{
   cvpFdThresh  sThresh;
   bool         bEnableRoi;
   cvpRoi       sRoi;

   INLINE _cvpFdFrameConfig() : bEnableRoi(false) {}
} cvpFdFrameConfig;

//------------------------------------------------------------------------------
/// @brief
///    The Face Detection has been processed by the CVP system, and the
///    output is ready to be consumed. This callback is used to notify the
///    application. The application sends the image using the
///    cvpFaceDetection_Async call.
///
/// @param eStatus
///    CVP status for the current process.
/// @param pOutput
///    Pointer to the Face Detection ouput structure.
/// @param hFaceDetection
///    Handle for the Face Detection that was passed in the
///    cvpFaceDetection_Async function.
/// @param pSessionUserData
///    User-data that was set in the cvpInitFaceDetection.
/// @param pTaskUserData
///    User-data that was passed in the cvpFaceDetection_Async function
///    which corresponds to this callback.
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
typedef void (*CvpFaceDetectionCb)(cvpStatus  eStatus,
                                   cvpMem    *pOutput,
                                   cvpHandle  hFaceDetection,
                                   void      *pSessionUserData,
                                   void      *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Initialize CVP - Face Detection (FD).
/// @param hSession
///    [Input] CVP session handle
/// @param pConfig
///    [Input] Pointer to the configuration.
/// @param pMemReq
///    [Output] Pointer to the memory requirement.
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
/// @retval CVP handle for Face Detection (FD)
///    If successful.
/// @retval NULL
///    If initialization failed.
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
CVP_API cvpHandle cvpInitFaceDetection(cvpSession                    hSession,
                                       const cvpConfigFaceDetection *pConfig,
                                       cvpFdBuffReq                 *pMemReq,
                                       CvpFaceDetectionCb            fnCb,
                                       void                         *pSessionUserData);

//------------------------------------------------------------------------------
/// @brief
///    Deinitialize CVP - Face Detection (FD).
/// @param hFd
///    [Input] CVP handle for Face Detection (FD).
///
/// @retval CVP_SUCCESS
///    If deinit is successful.
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeInitFaceDetection(cvpHandle hFd);

//------------------------------------------------------------------------------
/// @brief
///    Set configuration. This function can be called after cvpSessionStart(),
///    but the config parameters should be lower/easier than or equal to the config
///    parameters set during cvpInitFaceDetection().
/// @param hFd
///    [Input] CVP handle for Face Detection (FD).
/// @param pConfig
///    [Input] Pointer to the configuration.
///
/// @retval CVP_SUCCESS
///    If setConfig is successful.
/// @retval CVP_EBADPARAM
///    If one of config parameter is unacceptable.
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpFaceDetectionSetConfig(cvpHandle                     hFd,
                                            const cvpConfigFaceDetection *pConfig);

//------------------------------------------------------------------------------
/// @brief
///    Face Detection (FD).
/// @details
///    Synchronous (blocking) function that will do Face Detection (FD).
/// @param hFd
///    [Input] Handle for the Face Detection (FD).
/// @param pImage
///    [Input] Pointer to the CVP image.
/// @param pOutput
///    [Output] Face Detection output. The data type for pOutput.pAddress
///    is a pointer to cvpFdOutput structure. The required memory size is
///    at least the size fo cvpFdOutput structure.
/// @param pFrameConfig
///    [Input] Pointer to the face detection frame configuration.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpFaceDetection_Sync(cvpHandle                hFd,
                                        const cvpImage          *pImage,
                                        const cvpMem            *pOutput,
                                        const cvpFdFrameConfig  *pFrameConfig);

//------------------------------------------------------------------------------
/// @brief
///    Face Detection (FD).
/// @details
///    Asynchronous function that will queue the image and return almost
///    immediately. In the background, it will do Face Detection (FD).
///    Once the output is ready, it will notify the user through the callback
///    function and user can queue another image using the same buffer.
/// @param hFd
///    [Input] Handle for the Face Detection (FD).
/// @param pImage
///    [Input] Pointer to the CVP image.
/// @param pOutput
///    [Output] Face Detection output. The data type for pOutput.pAddress
///    is a pointer to cvpFdOutput structure. The required memory size is
///    at least the size fo cvpFdOutput structure.
/// @param pFrameConfig
///    [Input] Pointer to the face detection frame configuration.
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
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpFaceDetection_Async(cvpHandle               hFd,
                                         const cvpImage         *pImage,
                                         const cvpMem           *pOutput,
                                         const cvpFdFrameConfig *pFrameConfig,
                                         const void             *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Face Detection (FD).
/// @details
///    Asynchronous function using fences that will queue the image and return
///    almost immediately. In the background, it will wait for all input fences
///    singal (pFences->pIn) to do Face Detection (FD). Once the output is ready,
///    it will notify the user through output fences (pFences->pOut).
/// @param hFd
///    [Input] Handle for the Face Detection (FD).
/// @param pImage
///    [Input] Pointer to the CVP image.
/// @param pOutput
///    [Output] Face Detection output. The data type for pOutput.pAddress
///    is a pointer to cvpFdOutput structure. The required memory size is
///    at least the size fo cvpFdOutput structure.
/// @param pFrameConfig
///    [Input] Pointer to the face detection frame configuration.
/// @param pFences
///    [Input] Pointer to the collection of CVP fence objects.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter
/// @retval CVP_EUNSUPPORTED
///    If the handle is initialized for synchronous API.
///
/// @ingroup cvp_face_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpFaceDetection_Fence(cvpHandle               hFd,
                                         const cvpImage         *pImage,
                                         const cvpMem           *pOutput,
                                         const cvpFdFrameConfig *pFrameConfig,
                                         const cvpFences        *pFences);

#ifdef __cplusplus
}//extern "C"
#endif

#endif
