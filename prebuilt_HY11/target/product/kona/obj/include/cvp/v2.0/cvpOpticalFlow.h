/**=============================================================================

@file
   cvpOpticalFlow.h

@brief
   API Definitions for Optical Flow.

Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//=============================================================================
///@details CVP Optical Flow APIs using Computer Vision Processor acceleration
///@ingroup cvp_object_detection
//=============================================================================

#ifndef CVP_OPTICAL_FLOW_H
#define CVP_OPTICAL_FLOW_H

#include "cvpTypes.h"

#define CVP_OPTICALFLOW_MV_WEIGHTS_SIZE           (7)

#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
/// @brief
///    Mode option for Optical Flow.
/// @param CVP_OPTICALFLOW_ONE_PASS
///    One Pass Optical Flow
/// @param CVP_OPTICALFLOW_SEVEN_PASS
///    Seven Pass Optical Flow
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef enum
{
   CVP_OPTICALFLOW_ONE_PASS = 0,
   CVP_OPTICALFLOW_SEVEN_PASS
} cvpOpticalFlowMode;

//------------------------------------------------------------------------------
/// @brief
///    Structure representing Motion Vector
///
/// @param nMVX_L0
///    Represented by bit 8:0 MVX_L0 - Signed Horizontal motion vector for block 0.
/// @param nMVY_L0
///    Represented by bit 15:9. MVY_L0 - Signed Vertical motion vector for block 0.
/// @param nReserved
///    Represented by bit 27:16. Unused reserved bit.
/// @param nConf
///    Represented by bit 31:28. Motion vector confidence.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpMotionVector {
   int32_t nMVX_L0   : 9;   //   8:  0
   int32_t nMVY_L0   : 7;   //  15:  9
   int32_t nReserved : 12;  //  27: 16
   int32_t nConf     : 4;   //  31: 28

   #ifdef __cplusplus
   INLINE _cvpMotionVector()
   {
      memset(this, 0, sizeof (*this));
   }
   #endif

} cvpMotionVector;

//------------------------------------------------------------------------------
/// @brief
///    Structure representing optical flow statistic packets.
///
/// @param nVariance
///    8x8 block variance.
/// @param nMean
///    8x8 block mean.
/// @param nReserved
///    Unused reserved bit.
/// @param nBestMVSad
///    SAD of the best MV of 8x8 block.
/// @param nSad
///    SAD of the (0,0) MVs.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpOFStats {
   uint16_t nVariance;
   uint8_t  nMean;
   uint8_t  nReserved;
   uint16_t nBestMVSad;
   uint16_t nSad;

   #ifdef __cplusplus
   INLINE _cvpOFStats() : nVariance(0), nMean(0), nReserved(0),
                          nBestMVSad(0), nSad(0){}

   #endif

} cvpOFStats;

//------------------------------------------------------------------------------
/// @brief
///    Structure representing Optical Flow output (Refer to Programming Guide
///    for details).
/// @param pMotionVector
///    Pointer to motion vectors. The data type for pMotionVector->pAddress is a
///    pointer to cvpMotionVector structure.
/// @param pStats
///    Pointer to stats packets. The data type for pStats->pAddress is a pointer
///    to cvpOFStats structure.
/// @param nMVSize
///    Size of the motion vector array.
/// @param nStatsSize
///    Size of the stats packet array.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpOpticalFlowOutput
{
   cvpMem  *pMotionVector;
   cvpMem  *pStats;
   uint32_t nMVSize;
   uint32_t nStatsSize;

   #ifdef __cplusplus
   INLINE _cvpOpticalFlowOutput() : pMotionVector(NULL),
                                    pStats(NULL),
                                    nMVSize(0),
                                    nStatsSize(0){}
   #endif
} cvpOpticalFlowOutput;

//------------------------------------------------------------------------------
/// @brief
///    The optical flow has been processed by the CVP system, and the
///    output is ready to be consumed. This callback is used to notify the
///    application. The application sends the image using the
///    cvpOpticalFlow_Async call.
///
/// @param eStatus
///    CVP status for the current process.
/// @param pOutput
///    Pointer to the motion engine output structure.
/// @param hOpticFlow
///    Handle for the optical flow that was passed in the
///    cvpOpticalFlow_Async function.
/// @param pSessionUserData
///    User-data that was set in cvpInitOpticalFlow.
/// @param pTaskUserData
///    User-data that was passed in the cvpOpticalFlow_Async function
///    which corresponds to this callback.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef void (*CvpOpticalFlowCb)(cvpStatus             eStatus,
                                 cvpOpticalFlowOutput *pOutput,
                                 cvpHandle             hOpticFlow,
                                 void                 *pSessionUserData,
                                 void                 *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Structure for optical flow Enhance In-loop Confidence (EIC) configuration.
/// @param nLambda
///    Weight for Activity confidence towards final Confidence score. Range [0 32].
///    Final Confidence = (nLambda * ActivityConfidenceScore +
///    (32-nLambda) * ClusterConfidenceScore ) >> 5
/// @param nConfidenceS1
///    Confidence when Best MV is in Max Candidate Cluster pool.
///    Range [0-3]. Suggested [0]. Smaller value is better.
/// @param nConfidenceS2
///    Confidence when MV candidates are spread into two equally likely Cluster/Regions.
///    Range [7-10]. Suggested [7]. Smaller value is better.
/// @param nConfidenceS3
///    Confidence when MV candidates are spread into three equally likely Cluster/Regions.
///    Range [10-12]. Suggested [10]. Smaller value is better.
/// @param nConfidenceS4
///    Confidence when Best MV is not in Max Candidate Cluster pool.
///    Range [7-15]. Suggested [7]. Smaller value is better.
/// @param nBestCandidateRepeat
///    Number of times same MV was predicted. If this condition is met, Clustering
///    is not applied. Range [0-56]. Suggested [6].
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpOFConfidenceInfo
{
   uint32_t nLambda;
   uint32_t nConfidenceS1;
   uint32_t nConfidenceS2;
   uint32_t nConfidenceS3;
   uint32_t nConfidenceS4;
   uint32_t nBestCandidateRepeat;

   #ifdef __cplusplus
   INLINE _cvpOFConfidenceInfo() : nLambda(16),
                                   nConfidenceS1(0),
                                   nConfidenceS2(7),
                                   nConfidenceS3(10),
                                   nConfidenceS4(7),
                                   nBestCandidateRepeat(6){}
   #endif
} cvpOFConfidenceInfo;

//------------------------------------------------------------------------------
/// @brief
///    Structure for optical flow configuration.
/// @param nActualFps
///    Input frames per second. Default value is 30.
/// @param nOperationalFps
///    Desired output frames per second. nOperationalFps should be equal to
///    or greater than nActualFps. nOperationalFps >= nActualFps. Default value is 30.
/// @param sImageInfo
///    Structure for image information.
/// @param bStatsEnable
///    Flag to enable or disable the stats packet.
/// @param eMode
///    Enum representing the Optical Flow mode
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpConfigOpticalFlow
{
   uint32_t            nActualFps;
   uint32_t            nOperationalFps;
   cvpImageInfo        sImageInfo;
   bool                bStatsEnable;
   cvpOpticalFlowMode  eMode;

   #ifdef __cplusplus
   INLINE _cvpConfigOpticalFlow() : nActualFps(30),
                                    nOperationalFps(30),
                                    bStatsEnable(false),
                                    eMode(CVP_OPTICALFLOW_SEVEN_PASS){}
   #endif
} cvpConfigOpticalFlow;

//------------------------------------------------------------------------------
/// @brief
///    Structure for optical flow configuration - Advanced.
/// @param nMvDist
///    MV Distance threshold. Higher the value, sparser MVs are searched.
///    Valid values are 1,2,3 and 4. Default value is 2.
/// @param nMvWeights
///    Smoothness for different candidates. It is an array of CVP_OPTICALFLOW_MV_WEIGHTS_SIZE
///    elements. Assign higher values to 1st 5 for more global motion,
///    6th for smoothness, 7th for local motion.
///    Each element can take values 1 to 20. Default values are {10,2,2,1,1,7,20}
/// @param nMedianFiltType
///    Type of median filtering. Valid values are {0,1,2,5,6}. Default value is 5.
/// @param nThresholdMedFilt
///    Median filtering threshold.
///    Valid values are 700,900,1000,1200,1400,1444 and 1600. Default value is 900.
/// @param nSmoothnessPenaltyThresh
///    Cost Threshold.
///    Valid values are 200 to 2000 (in steps of 50). Default value is 500.
/// @param nSearchRangeX
///    Search range in X direction. Range is +/- of the value.
///    Valid values are 128, 96 and 64. Default value is 96.
/// @param nSearchRangeY
///    Search range in Y direction. Range is +/- of the value.
///    Valid values are 64 and 48. Default value is 48.
/// @param bEnableEic
///    Flag to enable Enhance In-loop Confidence (EIC) parameters.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpAdvConfigOpticalFlow
{
   uint32_t            nMvDist;
   uint32_t            nMvWeights[CVP_OPTICALFLOW_MV_WEIGHTS_SIZE];
   uint32_t            nMedianFiltType;
   uint32_t            nThresholdMedFilt;
   uint16_t            nSmoothnessPenaltyThresh;
   int16_t             nSearchRangeX;
   int16_t             nSearchRangeY;
   bool                bEnableEic;
   cvpOFConfidenceInfo sEicInfo;
   #ifdef __cplusplus
   INLINE _cvpAdvConfigOpticalFlow() : nMvDist(2),
                                       nMedianFiltType(5), nThresholdMedFilt(900),
                                       nSmoothnessPenaltyThresh(500), nSearchRangeX(96),
                                       nSearchRangeY(48), bEnableEic(false)
   {
      nMvWeights[0] = 10;
      nMvWeights[1] = 2;
      nMvWeights[2] = 2;
      nMvWeights[3] = 1;
      nMvWeights[4] = 1;
      nMvWeights[5] = 7;
      nMvWeights[6] = 20;
   }
   #endif

} cvpAdvConfigOpticalFlow;

//------------------------------------------------------------------------------
/// @brief
///    Structure for Optical Flow output buffer requirement
/// @param nMotionVectorBytes
///    The required size in bytes for motion vectors.
/// @param nStatsBytes
///    The required size in bytes for stats packets.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpOpticalFlowOutBuffReq
{
   uint32_t nMotionVectorBytes;
   uint32_t nStatsBytes;

   #ifdef __cplusplus
   INLINE _cvpOpticalFlowOutBuffReq() : nMotionVectorBytes(0), nStatsBytes(0){}
   #endif

} cvpOpticalFlowOutBuffReq;

//------------------------------------------------------------------------------
/// @brief
///    Initialize optical flow using CVP
/// @param hSession
///    [Input] CVP session handle
/// @param pConfig
///    [Input] Pointer to the optical flow configuration.
/// @param pAdvConfig
///    [Input] Pointer to the optical flow configuration - Advanced. This is
///    optional and can be set to NULL. When set to NULL, the constituent elements
///     will be assigned default values.
/// @param pOutMemReq
///    [Output] Pointer to the output memory requirement.
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
/// @retval optical flow handle
///    If successful.
/// @retval NULL
///    If initialization failed.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpHandle cvpInitOpticalFlow(cvpSession                     hSession,
                                     const cvpConfigOpticalFlow    *pConfig,
                                     const cvpAdvConfigOpticalFlow *pAdvConfig,
                                     cvpOpticalFlowOutBuffReq      *pOutMemReq,
                                     CvpOpticalFlowCb               fnCb,
                                     void                          *pSessionUserData);

//------------------------------------------------------------------------------
/// @brief
///    Deinitialize optical flow
/// @param hOpticalFlow
///    [Input] optical flow handle.
///
/// @retval CVP_SUCCESS
///    If deinit is successful.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeInitOpticalFlow(cvpHandle hOpticalFlow);

//------------------------------------------------------------------------------
/// @brief
///    Register optical flow image buffer to the optical flow handle.
/// @details
///    This API will register the image buffer and prepare any necessary scratch
///    buffer associated with this image.
/// @param hOpticalFlow
///    [Input] optical flow handle.
/// @param pImage
///    [Input] Pointer to the image.
///
/// @retval CVP_SUCCESS
///    If registering is successful.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpRegisterOpticalFlowImageBuf(cvpHandle       hOpticalFlow,
                                                 const cvpImage *pImage);

//------------------------------------------------------------------------------
/// @brief
///    Deregister optical flow image buffer from the optical flow handle.
/// @details
///    This API will deregister the image buffer and free any necessary scratch
///    buffer associated with this image.
/// @param hOpticalFlow
///    [Input] optical flow handle.
/// @param pImage
///    [Input] Pointer to the image.
///
/// @retval CVP_SUCCESS
///    If deregistering is successful.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeregisterOpticalFlowImageBuf(cvpHandle       hOpticalFlow,
                                                   const cvpImage *pImage);

//------------------------------------------------------------------------------
/// @brief
///    Optical Flow.
/// @details
///    Synchronous (blocking) function that will do optical flow.
/// @param hOpticalFlow
///    [Input] Handle for the optical flow.
/// @param pRefImage
///    [Input] Pointer to the reference image.
/// @param pCurImage
///    [Input] Pointer to the current image.
/// @param bNewRefImg
///    [Input] Flag to indicate if the reference image is new and has never been passed to
///    this API previously. Setting it to true will trigger cvp to prepare necessary
///    preprocessing step in calculating optical flow. Setting it to false will notify
///    cvp to reuse the previous preprocessing associated with this image.
/// @param bNewCurImg
///    [Input] Flag to indicate if the current image is new and has never been passed to
///    this API previously. Setting it to true will trigger cvp to prepare necessary
///    preprocessing step in calculating optical flow. Setting it to false will notify
///    cvp to reuse the previous preprocessing associated with this image.
/// @param pOutput
///    [Output] Pointer to the optical flow ouput structure.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpOpticalFlow_Sync(cvpHandle                  hOpticalFlow,
                                      const cvpImage            *pRefImage,
                                      const cvpImage            *pCurImage,
                                      bool                       bNewRefImg,
                                      bool                       bNewCurImg,
                                      cvpOpticalFlowOutput      *pOutput);

//------------------------------------------------------------------------------
/// @brief
///    optical flow.
/// @details
///    Asynchronous function that will queue the image and return almost
///    immediately. In the background, it will do optical flow.
///    Once the output is ready, it will notify the user through the callback
///    function and user can queue another image using the same buffer.
/// @param hOpticalFlow
///    [Input] Handle for the optical flow.
/// @param pRefImage
///    [Input] Pointer to the reference image.
/// @param pCurImage
///    [Input] Pointer to the current image.
/// @param bNewRefImg
///    [Input] Flag to indicate if the reference image is new and has never been passed to
///    this API previously. Setting it to true will trigger cvp to prepare necessary
///    preprocessing step in calculating optical flow. Setting it to false will notify
///    cvp to reuse the previous preprocessing associated with this image.
/// @param bNewCurImg
///    [Input] Flag to indicate if the current image is new and has never been passed to
///    this API previously. Setting it to true will trigger cvp to prepare necessary
///    preprocessing step in calculating optical flow. Setting it to false will notify
///    cvp to reuse the previous preprocessing associated with this image.
/// @param pOutput
///    [Output] Pointer to the optical flow ouput structure which corresponds to the
///    callback.
/// @param pTaskUserData
///    [Input] User-data which will correspond to the callback.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is initialized for synchronous API.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpOpticalFlow_Async(cvpHandle                  hOpticalFlow,
                                       const cvpImage            *pRefImage,
                                       const cvpImage            *pCurImage,
                                       bool                       bNewRefImg,
                                       bool                       bNewCurImg,
                                       cvpOpticalFlowOutput      *pOutput,
                                       const void                *pTaskUserData);

#ifdef __cplusplus
}//extern "C"
#endif

#endif
