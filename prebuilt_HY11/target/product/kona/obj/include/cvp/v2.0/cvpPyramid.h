/**=============================================================================

@file
   cvpPyramid.h

@brief
   API Definitions for Pyramid functionality

Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//=============================================================================
///@details CVP Pyramid APIs using Computer Vision Processor acceleration
///@ingroup cvp_image_transform, cvp_feature_detection
//=============================================================================

#ifndef CVP_PYRAMID_H
#define CVP_PYRAMID_H

#include "cvpTypes.h"
#include "cvpFeaturePoint.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define CVP_MAX_PYRAMID_FPX_OCTAVES           (5)
#define CVP_MAX_PYRAMID_FPX_SCALES_PER_OCTAVE (4)
#define CVP_MAX_PYRAMID_FPX_LEVELS            (CVP_MAX_PYRAMID_FPX_OCTAVES * CVP_MAX_PYRAMID_FPX_SCALES_PER_OCTAVE)

// -----------------------------------------------------------------------------
/// @brief
///    Structure representing a pyramid image
/// @param pImage
///    Pointer to the image structure array.
/// @param nOctaves
///    Number of octaves. Image size is halved between adjacent octaves.
/// @param nScalesPerOctave
///    Number of scales per octave. This includes the octave.
/// @param nLevels
///    Number of levels which is the number of pImages.
///    It is equal to nOctaves * nScalesPerOctave.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
typedef struct _cvpPyramidImage
{
   cvpImage *pImage;
   uint32_t  nOctaves;
   uint32_t  nScalesPerOctave;
   uint32_t  nLevels;

#ifdef __cplusplus
   INLINE _cvpPyramidImage() : pImage(NULL),
                               nOctaves(0),
                               nScalesPerOctave(0),
                               nLevels(0) { }
#endif

} cvpPyramidImage;

//------------------------------------------------------------------------------
/// @brief
///    Structure for pyramid configuration.
/// @param nActualFps
///    Input frames per second. Default value is 30.
/// @param nOperationalFps
///    Desired output frames per second. nOperationalFps should be equal to
///    or greater than nActualFps. nOperationalFps >= nActualFps. Default value is 30.
/// @param sSrcImageInfo
///    Structure for source image information. Supported color formats are
///    CVP_COLORFORMAT_GRAY_8BIT and CVP_COLORFORMAT_GRAY_UBWC.
/// @param nFilterDiv2Coeff
///    Div by 2 filter coefficients. Range is from 0 (sharpest) to 4 (flattest).
///    Default value is 3.
/// @param nOctaves
///    Number of octaves. Image size is halved between adjacent octaves.
/// @param nScalesPerOctave
///    Number of scales per octave. This includes the octave.
///    Number of levels is equal to nScalesPerOctave * nOctaves.
/// @param eOutFormat
///    Pyramid ouptut color format. Supported color formats are
///    CVP_COLORFORMAT_GRAY_8BIT and CVP_COLORFORMAT_GRAY_UBWC.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
typedef struct _cvpConfigPyramidImage
{
   uint32_t       nActualFps;
   uint32_t       nOperationalFps;
   cvpImageInfo   sSrcImageInfo;
   uint32_t       nFilterDiv2Coeff[CVP_MAX_PYRAMID_FPX_OCTAVES];
   uint32_t       nOctaves;
   uint32_t       nScalesPerOctave;
   cvpColorFormat eOutFormat;

#ifdef __cplusplus
   INLINE _cvpConfigPyramidImage() : nActualFps(30),
                                     nOperationalFps(30),
                                     nOctaves(0),
                                     nScalesPerOctave(0),
                                     eOutFormat(CVP_COLORFORMAT_GRAY_8BIT)
   {
      for (uint32_t i=0; i<CVP_MAX_PYRAMID_FPX_OCTAVES; i++)
      {
         nFilterDiv2Coeff[i] = 3;
      }
   }
#endif

} cvpConfigPyramidImage;

//------------------------------------------------------------------------------
/// @brief
///    Structure for Pyramid Image output buffer requirement
/// @param nImageBytes
///    The required size in bytes for pyramid image (sImage.pImage[i])
///    where i = level index).
///    Must not allocate buffer for the base image (sImage.pImage[0]). It will set the
///    input image information into pImage[0].
/// @param nLevels
///    Number of pyramid levels.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpPyramidImageOutBuffReq
{
   uint32_t nImageBytes[CVP_MAX_PYRAMID_FPX_LEVELS];
   uint32_t nLevels;

#ifdef __cplusplus
   INLINE _cvpPyramidImageOutBuffReq() : nLevels(0)
   {
      memset(nImageBytes, 0, sizeof(nImageBytes));
   }
#endif

} cvpPyramidImageOutBuffReq;

//------------------------------------------------------------------------------
/// @brief
///    The pyramid image has been processed by the CVP system, and the
///    output is ready to be consumed. This callback is used to notify the
///    application. The application sends the image using the
///    cvpPyramidImage_Async call.
///
/// @param eStatus
///    CVP status for the current process.
/// @param pOutput
///    Pointer to the pyramid output structure.
/// @param hPyramid
///    Handle for the pyramid that was passed in the cvpPyramidImage_Async function.
/// @param pSessionUserData
///    User-data that was set in the cvpInitPyramid structure.
/// @param pTaskUserData
///    User-data that was passed in the cvpPyramidImage_Async function
///    which corresponds to this callback.
///
/// @ingroup cvp_image_transform, cvp_feature_detection
//------------------------------------------------------------------------------
typedef void (*CvpPyramidImageCb)(cvpStatus        eStatus,
                                  cvpPyramidImage *pOutput,
                                  cvpHandle        hPyramid,
                                  void            *pSessionUserData,
                                  void            *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Initialize pyramid image.
/// @param hSession
///    [Input] CVP session handle
/// @param pConfig
///    [Input] Pointer to the pyramid image configuration.
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
/// @retval CVP_SUCCESS.
///    If successful.
/// @retval CVP_ENORES
///    If there is no enough memory.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpHandle cvpInitPyramidImage(cvpSession                   hSession,
                                      const cvpConfigPyramidImage *pConfig,
                                      cvpPyramidImageOutBuffReq   *pOutMemReq,
                                      CvpPyramidImageCb            fnCb,
                                      void                        *pSessionUserData);

//------------------------------------------------------------------------------
/// @brief
///    Deinitialize CVP - Pyramid Image
/// @param hPyramidImage
///    [Input] CVP handle for pyramid image.
///
/// @retval CVP_SUCCESS
///    If deinit is successful.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeInitPyramidImage(cvpHandle hPyramidImage);

//------------------------------------------------------------------------------
/// @brief
///    Pyramid Image.
/// @details
///    Synchronous (blocking) function that will do pyramid image.
/// @param hPyramiImage
///    [Input] Handle for the pyramid image.
/// @param pSrcImage
///    [Input] Pointer to the source image.
/// @param pOutput
///    [Output] Pointer to the Pyramid Image output structure.
///    pOutput.sImage.pImage[0] is not used. There is no data in this pointer.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpPyramidImage_Sync(cvpHandle        hPyramidImage,
                                       const cvpImage  *pSrcImage,
                                       cvpPyramidImage *pOutput);

//------------------------------------------------------------------------------
/// @brief
///    Pyramid Image
/// @details
///    Asynchronous function that will queue the image and return almost
///    immediately. In the background, it will do pyramid image. Once the output
///    is ready, it will notify the user through the callback function and
///    user can queue another image using the same buffer.
/// @param hPyramiImage
///    [Input] Handle for the pyramid image.
/// @param pSrcImage
///    [Input] Pointer to the source image.
/// @param pOutput
///    [Output] Pointer to the Pyramid Image output structure.
///    pOutput.sImage.pImage[0] is not used. There is no data in this pointer.
/// @param pTaskUserData
///    [Input] User-data which will correspond to the callback.
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
CVP_API cvpStatus cvpPyramidImage_Async(cvpHandle        hPyramidImage,
                                        const cvpImage  *pSrcImage,
                                        cvpPyramidImage *pOutput,
                                        const void      *pTaskUserData);

// -----------------------------------------------------------------------------
/// @brief
///    Structure representing Feature Point pyramid. Max is CVP_MAX_PYRAMID_FPX_OCTAVES.
///    Feature point extraction (FPX) can be done at the octave level only.
/// @param sFeaturePoint
///    List of Feature Point output structure. The sFeaturePoint[i] value will be invalid
///    if bFpxEnable[i] is set to false.
/// @param nOctaves
///    Number of octaves that has Feature Point Extraction output.
///
/// @ingroup cvp_feature_detection
//------------------------------------------------------------------------------
typedef struct _cvpPyramidFpx
{
   cvpFeaturePointOutput  sFeaturePoint[CVP_MAX_PYRAMID_FPX_OCTAVES];
   uint32_t               nOctaves;

#ifdef __cplusplus
   INLINE _cvpPyramidFpx() : nOctaves(0) { }
#endif

} cvpPyramidFpx;

// -----------------------------------------------------------------------------
/// @brief
///    Structure representing pyramid image and pyramid Feature Point Extraction output
/// @param sImage
///    Image pyramid output
/// @param sFpx
///    Feature Point Extraction (FPX) pyramid output
///
/// @ingroup cvp_image_transform, cvp_feature_detection
//------------------------------------------------------------------------------
typedef struct _cvpPyramidOutput
{
   cvpPyramidImage  sImage;
   cvpPyramidFpx    sFpx;
} cvpPyramidOutput;

//------------------------------------------------------------------------------
/// @brief
///    The pyramid has been processed by the CVP system, and the
///    output is ready to be consumed. This callback is used to notify the
///    application. The application sends the image using the
///    cvpPyramid_Async call.
///
/// @param eStatus
///    CVP status for the current process.
/// @param pOutput
///    Pointer to the pyramid output structure.
/// @param hPyramid
///    Handle for the pyramid that was passed in the cvpPyramid_Async function.
/// @param pSessionUserData
///    User-data that was set in the cvpInitPyramid structure.
/// @param pTaskUserData
///    User-data that was passed in the cvpPyramid_Async function
///    which corresponds to this callback.
///
/// @ingroup cvp_image_transform, cvp_feature_detection
//------------------------------------------------------------------------------
typedef void (*CvpPyramidCb)(cvpStatus         eStatus,
                             cvpPyramidOutput *pOutput,
                             cvpHandle         hPyramid,
                             void             *pSessionUserData,
                             void             *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Structure for pyramid configuration.
/// @param nActualFps
///    Input frames per second. Default value is 30.
/// @param nOperationalFps
///    Desired output frames per second. nOperationalFps should be equal to
///    or greater than nActualFps. nOperationalFps >= nActualFps. Default value is 30.
/// @param sSrcImageInfo
///    Structure for source image information. Supported color formats are
///    CVP_COLORFORMAT_GRAY_8BIT and CVP_COLORFORMAT_GRAY_UBWC.
/// @param nFilterDiv2Coeff
///    Div by 2 filter coefficients. Range is from 0 (sharpest) to 4 (flattest).
///    Default value is 3.
/// @param nOctaves
///    Number of octaves. Image size is halved between adjacent octaves.
/// @param nScalesPerOctave
///    Number of scales per octave. This includes the octave.
///    Number of levels is equal to nScalesPerOctave * nOctaves.
/// @param eOutFormat
///    Pyramid ouptut color format. Supported color formats are
///    CVP_COLORFORMAT_GRAY_8BIT and CVP_COLORFORMAT_GRAY_UBWC.
/// @param eFpxMode
///    Enum representing the Feature Point Extraction mode.
/// @param nFpxGradFilterCoeffsX[3]
///    3x1 FPX Gradient filter coefficients in X direction. Default value is {1,2,1}.
/// @param nFpxGradFilterCoeffsY[3]
///    3x1 FPX Gradient filter coefficients in Y direction. Default value is {1,2,1}.
/// @param nFpxGradFilterShift
///    FPX Gradient filter shift. Default value is 3.
/// @param nLowPassFilterCoeffs[3]
///    3x1 FPX low pass filter coefficients. Default value is {1,4,6}.
/// @param sFpxZoneConfig
///    Configuration for FPX Zone mode. It will be used only if eMode is
///    CVP_FPX_ZONE.
/// @param bFpxEnable
///    List of flags (max is CVP_MAX_PYRAMID_FPX_OCTAVES) to enable Feature Point
///    Extraction for each octave.
///
/// @ingroup cvp_image_transform, cvp_feature_detection
//------------------------------------------------------------------------------
typedef struct _cvpConfigPyramid
{
   uint32_t          nActualFps;
   uint32_t          nOperationalFps;
   cvpImageInfo      sSrcImageInfo;
   uint32_t          nFilterDiv2Coeff[CVP_MAX_PYRAMID_FPX_OCTAVES];
   uint32_t          nOctaves;
   uint32_t          nScalesPerOctave;
   cvpColorFormat    eOutFormat;
   cvpFpxMode        eFpxMode;
   uint32_t          nFpxGradFilterCoeffsX[3];
   uint32_t          nFpxGradFilterCoeffsY[3];
   uint32_t          nFpxGradFilterShift;
   uint32_t          nLowPassFilterCoeffs[3];
   cvpConfigFpxZone  sFpxZoneConfig[CVP_MAX_PYRAMID_FPX_OCTAVES];
   bool              bFpxEnable[CVP_MAX_PYRAMID_FPX_OCTAVES];

#ifdef __cplusplus
   INLINE _cvpConfigPyramid() : nActualFps(30),
                                nOperationalFps(30),
                                nOctaves(0),
                                nScalesPerOctave(0),
                                eOutFormat(CVP_COLORFORMAT_GRAY_8BIT),
                                eFpxMode(CVP_FPX_PEAK_8x8),
                                nFpxGradFilterShift(3)
   {
      for (uint32_t i=0; i<CVP_MAX_PYRAMID_FPX_OCTAVES; i++)
      {
         nFilterDiv2Coeff[i] = 3;
      }
      nFpxGradFilterCoeffsX[0] = 1;
      nFpxGradFilterCoeffsX[1] = 2;
      nFpxGradFilterCoeffsX[2] = 1;

      nFpxGradFilterCoeffsY[0] = 1;
      nFpxGradFilterCoeffsY[1] = 2;
      nFpxGradFilterCoeffsY[2] = 1;

      nLowPassFilterCoeffs[0] = 1;
      nLowPassFilterCoeffs[1] = 4;
      nLowPassFilterCoeffs[2] = 6;

      memset(bFpxEnable, 0, sizeof(bFpxEnable));
   }
#endif

} cvpConfigPyramid;

//------------------------------------------------------------------------------
/// @brief
///    Structure for Pyramid Feature Point Extraction output buffer requirement
/// @param nFeaturePointBytes
///    The required size in bytes for pyramid FPX output
///    (sFpx.sFeaturePoint[i].pPoints where i = level index)
/// @param nOctaves
///    Number of octaves.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpPyramidFpxOutBuffReq
{
   uint32_t nFeaturePointBytes[CVP_MAX_PYRAMID_FPX_OCTAVES];
   uint32_t nOctaves;

#ifdef __cplusplus
   INLINE _cvpPyramidFpxOutBuffReq() : nOctaves(0)
   {
      memset(nFeaturePointBytes, 0, sizeof(nFeaturePointBytes));
   }
#endif

} cvpPyramidFpxOutBuffReq;

//------------------------------------------------------------------------------
/// @brief
///    Structure for Pyramid output buffer requirement
/// @param sImage
///    The output buffer requirement for pyramid image.
/// @param sFpx
///    The output buffer requirement for pyramid Feature Point Extraction.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpPyramidOutBuffReq
{
   cvpPyramidImageOutBuffReq  sImage;
   cvpPyramidFpxOutBuffReq    sFpx;
} cvpPyramidOutBuffReq;

//------------------------------------------------------------------------------
/// @brief
///    Initialize pyramid image and pyramid Feature Point Extraction.
/// @param hSession
///    [Input] CVP session handle
/// @param pConfig
///    [Input] Pointer to the pyramid configuration struct
///    [Output] sZoneConfig (nZoneWidth and nZoneHeight) will be adjusted based
///    on the valid range.
/// @param pOutMemReq
///    [Output] Pointer to the output memory requirement.
/// @param fnCb
///    [Input] Callback function for the asynchronous API
///    Setting to NULL will result in initializing for the synchronous API
/// @param pSessionUserData
///    [Input] A private pointer that user can set with this session, and this
///    pointer will be provided as parameter to all callback functions
///    originated from the current session. This could be used to associate a
///    callback to this session. This can only be set once while initializing
///    the handle. This value will not/cannot-be changed for life
///    of a session.
///
/// @retval pyramid handle
///    If successful.
/// @retval NULL
///    If initialization failed.
///
/// @ingroup cvp_image_transform, cvp_feature_detection
//------------------------------------------------------------------------------
CVP_API cvpHandle cvpInitPyramid(cvpSession              hSession,
                                 cvpConfigPyramid       *pConfig,
                                 cvpPyramidOutBuffReq   *pOutMemReq,
                                 CvpPyramidCb            fnCb,
                                 void                   *pSessionUserData);

//------------------------------------------------------------------------------
/// @brief
///    Deinitialize CVP - Pyramid Image and Pyramid Feature Point Extraction
/// @param hPyramid
///    [Input] CVP handle for pyramid.
///
/// @retval CVP_SUCCESS
///    If deinit is successful.
///
/// @ingroup cvp_image_transform, cvp_feature_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeInitPyramid(cvpHandle hPyramid);

//------------------------------------------------------------------------------
/// @brief
///    Pyramid Image and Pyramid Feature Point Extraction.
/// @details
///    Synchronous (blocking) function that will do pyramid image and/or
///    feature point extraction.
/// @param hPyramid
///    [Input] Handle for the pyramid image and pyramid Feature Point Extraction.
/// @param pSrcImage
///    [Input] Pointer to the source image.
/// @param nFpxScoreShift
///    [Input] FPX score shift value. This value can be calculated by calling
///    cvpGetFpxScoreShift() using the previous result in the pyramid base line of pOutput->sFpx.nMaxFpxScore.
///    It's suggested to set to 8 for the first call.
/// @param nFpxThreshold
///    [Input] Threshold to filter the Feature Point Extraction scores.
/// @param nFpxRobustness
///    [Input] This parameter controls the robustness of the feature point detector.
///    Lower robustness setting produces more candidate feature points and vice versa.
///    Valid range is 0 to 127. Default value is 10.
/// @param eFpxNmsTap
///    [Input] Non-maximal suppression filter type. Set to CVP_FPX_5_TAP_NMS by
///    default.
/// @param pOutput
///    [Output] Pointer to the pyramid ouput structure.
///    pOutput.sImage.pImage[0] is not used. There is no data in this pointer.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_image_transform, cvp_feature_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpPyramid_Sync(cvpHandle         hPyramid,
                                  const cvpImage   *pSrcImage,
                                  uint32_t          nFpxScoreShift,
                                  uint32_t          nFpxThreshold,
                                  uint32_t          nFpxRobustness,
                                  cvpFpxNmsTap      eFpxNmsTap,
                                  cvpPyramidOutput *pOutput);

//------------------------------------------------------------------------------
/// @brief
///    Pyramid Image and Pyramid Feature Point Extraction.
/// @details
///    Asynchronous function that will queue the image and return almost
///    immediately. In the background, it will do pyramid image and/or pyramid
///    fpx. Once the output is ready, it will notify the user through the
///    callback function and user can queue another image using the same buffer.
/// @param hPyramid
///    [Input] Handle for the pyramid image and pyramid Feature Point Extraction.
/// @param pSrcImage
///    [Input] Pointer to the source image.
/// @param nFpxScoreShift
///    [Input] FPX score shift value. This value can be calculated by calling
///    cvpGetFpxScoreShift() using the previous result in the pyramid base line of
///    pOutput->sFpx.nMaxFpxScore.
///    It's suggested to set to 8 for the first call.
/// @param nFpxThreshold
///    [Input] Threshold to filter the Feature Point Extraction scores.
/// @param nFpxRobustness
///    [Input] This parameter controls the robustness of the feature point detector.
///    Lower robustness setting produces more candidate feature points and vice versa.
///    Valid range is 0 to 127. Default value is 10.
/// @param eFpxNmsTap
///    [Input] Non-maximal suppression filter type. Set to CVP_FPX_5_TAP_NMS by
///    default.
/// @param pOutput
///    [Output] Pointer to the pyramid ouput structure.
///    pOutput.sImage.pImage[0] is not used. There is no data in this pointer.
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
/// @ingroup cvp_image_transform, cvp_feature_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpPyramid_Async(cvpHandle         hPyramid,
                                   const cvpImage   *pSrcImage,
                                   uint32_t          nFpxScoreShift,
                                   uint32_t          nFpxThreshold,
                                   uint32_t          nFpxRobustness,
                                   cvpFpxNmsTap      eFpxNmsTap,
                                   cvpPyramidOutput *pOutput,
                                   const void       *pTaskUserData);

#ifdef __cplusplus
} //extern "C"
#endif

#endif
