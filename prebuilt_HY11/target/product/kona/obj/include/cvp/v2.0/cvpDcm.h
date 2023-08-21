
/**=============================================================================

@file
   cvpDcm.h

@brief
   API Definitions for Descriptor Calculation and Match (DCM)

Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//=============================================================================
///@details CVP DCM APIs using Computer Vision Processor acceleration
///@ingroup cvp_descriptor_calculation_matching
//=============================================================================

#ifndef CVP_DCM_H
#define CVP_DCM_H

#include "cvpTypes.h"
#include "cvpMem.h"

#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
/// @brief
///    Descriptor Calculation and Match (DCM) mode
/// @param CVP_DCM_CAL
///    Descriptor Calculate only.
/// @param CVP_DCM_CAL_MATCH
///    Descriptor Calculate and Matching.
///
/// @ingroup cvp_descriptor_calculation_matching
//------------------------------------------------------------------------------
typedef enum
{
   CVP_DCM_CAL = 0,
   CVP_DCM_CAL_MATCH
} cvpDcmMode;

//------------------------------------------------------------------------------
/// @brief
///    Packed structure for DCM Search Position.
/// @param PosX
///    Represented by bit 15:0. 16 bit x-axis position.
/// @param PosY
///    Represented by bit 31:16. 16 bit y-axis position.
/// @ingroup cvp_descriptor_calculation_matching
//------------------------------------------------------------------------------
typedef struct _cvpDcmSearchPosition
{
   uint16_t PosX        : 16;  // 15:0
   uint16_t PosY        : 16;  // 31:16

   #ifdef __cplusplus
   INLINE _cvpDcmSearchPosition()
   {
      memset(this, 0, sizeof (*this));
   }
   #endif
} cvpDcmSearchPosition;

//------------------------------------------------------------------------------
/// @brief
///    Packed structure for DCM Matching Result.
/// @param nMatchScore
///    Represented by bit 5:0. 6 bit match score.
/// @param nMatchIndex
///    Represented by bit 15:6. 10 bit best match index (possible value
///    of 0: (nNumDCPoints-1) (max 1023)) taken from the stored best index.
/// @ingroup cvp_descriptor_calculation_matching
//------------------------------------------------------------------------------
typedef struct _cvpDcmMatchResult
{
   uint16_t nMatchScore   : 6;  // 5:0
   uint16_t nMatchIndex   : 10;  // 15:6

   #ifdef __cplusplus
   INLINE _cvpDcmMatchResult()
   {
      memset(this, 0, sizeof (*this));
   }
   #endif
} cvpDcmMatchResult;

//------------------------------------------------------------------------------
/// @brief
///    Structure representing DCM output
/// @param pDescriptorBuffer
///    Descriptor data structure. Each Descriptor is represented with 32 bytes.
///    The size of the buffer is nDescriptorSize * 32 bytes.
/// @param pResultMatchBuffer
///    Matching Result data structure. Each Matching result is represented with
///    cvpDcmMatchResult.
///    The size of the buffer is nResultMatchSize * 16 bits.
/// @param nDescriptorSize
///    Number of Descriptor Calculation
/// @param nResultMatchSize
///    Number of Descriptor Matching Result
///
/// @ingroup cvp_descriptor_calculation_matching
//------------------------------------------------------------------------------
typedef struct _cvpDcmOutput
{
   cvpMem           *pDescriptorBuffer;
   cvpMem           *pResultMatchBuffer;
   uint32_t           nDescriptorSize;
   uint32_t           nResultMatchSize;

   #ifdef __cplusplus
   INLINE _cvpDcmOutput() : pDescriptorBuffer(NULL), pResultMatchBuffer(NULL),
                            nDescriptorSize(0), nResultMatchSize(0) {}
   #endif
} cvpDcmOutput;

//------------------------------------------------------------------------------
/// @brief
///    The DCM has been processed by the CVP system, and the
///    output is ready to be consumed. This callback is used to notify the
///    application. The application sends the image using the
///    cvpDcm_Async call.
///
/// @param eStatus
///    CVP status for the current process.
/// @param pOutput
///    Pointer to the DCM ouput structure.
/// @param hDcm
///    Handle for the DCM that was passed in the cvpDcm_Async function.
/// @param pSessionUserData
///    User-data that was set in the cvpInitDcm structure.
/// @param pTaskUserData
///    User-data that was passed in the cvpDcm_Async function
///    which corresponds to this callback.
///
/// @ingroup cvp_descriptor_calculation_matching
//------------------------------------------------------------------------------
typedef void (*CvpDcmCb)(cvpStatus     eStatus,
                          cvpDcmOutput *pOutput,
                          cvpHandle     hDcm,
                          void          *pSessionUserData,
                          void          *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Structure for Descriptor Calculation and Match (DCM) configuration.
/// @param nActualFps
///    Input frames per second. Default value is 30.
/// @param nOperationalFps
///    Desired output frames per second. nOperationalFps should be equal to
///    or greater than nActualFps. nOperationalFps >= nActualFps. Default value is 30.
/// @param sImageInfo
///    Structure for the image information.
/// @param eDcmMode
///    Flag to enable Descriptor Matching.
///    When enabled, descriptor matching output will be available. Otherwise,
///    descriptor matching output will be skipped.
/// @param nMaxNumDCPoints
///    Max size of current Descriptor group.
///    Valid range is [1, 1024]
///    Default value is 1024.
/// @param nMaxNumDCMRefPoints
///    Max size of reference Descriptor group.
///    Valid range is [1, 1024]
///    Valid only if matching is enabled.
///
/// @ingroup cvp_descriptor_calculation_matching
//------------------------------------------------------------------------------
typedef struct _cvpConfigDcm
{
   uint32_t          nActualFps;
   uint32_t          nOperationalFps;
   cvpImageInfo      sImageInfo;
   cvpDcmMode        eDcmMode;
   uint32_t          nMaxNumDCPoints;
   uint32_t          nMaxNumDCRefPoints;

   #ifdef __cplusplus
   INLINE _cvpConfigDcm() : nActualFps(30),
                            nOperationalFps(30),
                            eDcmMode(CVP_DCM_CAL),
                            nMaxNumDCPoints(1024),
                            nMaxNumDCRefPoints(1024) {}
   #endif
} cvpConfigDcm;

//------------------------------------------------------------------------------
/// @brief
///    Structure for DCM output buffer requirement
/// @param nDescriptorBytes
///    The required size in bytes for descriptor results.
/// @param nResultMatchBytes
///    The required size in bytes for descriptor matching results.
///
/// @ingroup cvp_descriptor_calculation_matching
//------------------------------------------------------------------------------
typedef struct _cvpDcmOutBuffReq
{
   uint32_t nDescriptorBytes;
   uint32_t nResultMatchBytes;

   #ifdef __cplusplus
   INLINE _cvpDcmOutBuffReq() : nDescriptorBytes(0),
                                 nResultMatchBytes(0) {}
   #endif
} cvpDcmOutBuffReq;

//------------------------------------------------------------------------------
/// @brief
///    Initialize CVP - Descriptor Calculation and Match (DCM).
/// @param hSession
///    [Input] CVP session handle
/// @param pConfig
///    [Input] Pointer to the DCM configuration.
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
/// @retval CVP handle for DCM.
///    If successful.
/// @retval NULL
///    If initialization failed.
///
/// @ingroup cvp_descriptor_calculation_matching
//------------------------------------------------------------------------------
CVP_API cvpHandle cvpInitDcm(cvpSession          hSession,
                             const cvpConfigDcm *pConfig,
                             cvpDcmOutBuffReq   *pOutMemReq,
                             CvpDcmCb            fnCb,
                             void               *pSessionUserData);

//------------------------------------------------------------------------------
/// @brief
///    Deinitialize CVP - Descriptor Calculation and Match (DCM).
/// @param hDcm
///    [Input] CVP handle for DCM.
///
/// @retval CVP_SUCCESS
///    If deinit is successful.
///
/// @ingroup cvp_descriptor_calculation_matching
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeInitDcm(cvpHandle hDcm);

//------------------------------------------------------------------------------
/// @brief
///    Descriptor Calculation and Match (DCM).
/// @details
///    Synchronous (blocking) function that will do DCM.
/// @param hDcm
///    [Input] Handle for the DCM.
/// @param pSrcImage
///    [Input] Pointer to the CVP image.
/// @param pSearchPosition
///    [Input] Pointer to the Search Position buffer.
///    Each Matching result is represented with
///    cvpDcmSearchPosition.
/// @param pRefDescriptor
///    [Input] Pointer to the Reference Descriptor buffer.
///    Descriptor data structure. Each Descriptor is represented with 32 bytes.
///    The size of the buffer is nNumDCMRefPoints * 32 bytes.
///    Valid only when Descriptor Matching is enabled.
/// @param nNumDCPoints
///    [Input] Size of current Descriptor group.
///    Valid range is [1, 1024]
/// @param nNumDCMRefPoints
///    [Input] Size of reference Descriptor group.
///    Valid range is [1, 1024]
///    Valid only if matching is enabled.
/// @param bDescriptorLPF
///    [Input] Boolean to enable Descriptor Low Pass Filter.
///    Set to true by default.
/// @param pOutput
///    [Output] Pointer to the DCM ouput structure.
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_descriptor_calculation_matching
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDcm_Sync(cvpHandle       hDcm,
                                 const cvpImage *pSrcImage,
                                 const cvpMem   *pSearchPosition,
                                 const cvpMem   *pRefDescriptor,
                                 uint32_t         nNumDCPoints,
                                 uint32_t         nNumDCMRefPoints,
                                 bool             bDescriptorLPF,
                                 cvpDcmOutput   *pOutput);

//------------------------------------------------------------------------------
/// @brief
///    Descriptor Calculation and Match (DCM).
/// @details
///    Asynchronous (non-blocking) function that will do DCM.
/// @param hDcm
///    [Input] Handle for the DCM.
/// @param pSrcImage
///    [Input] Pointer to the CVP image.
/// @param pSearchPosition
///    [Input] Pointer to the Search Position buffer.
/// @param pRefDescriptor
///    [Input] Pointer to the Reference Descriptor buffer.
///    Descriptor data structure. Each Descriptor is represented with 32 bytes.
///    The size of the buffer is nNumDCMRefPoints * 32 bytes.
///    Valid only when Descriptor Matching is enabled.
/// @param nNumDCPoints
///    [Input] Size of current Descriptor group.
///    Valid range is [1, 1024]
/// @param nNumDCMRefPoints
///    [Input] Size of reference Descriptor group.
///    Valid range is [1, 1024]
///    Valid only if matching is enabled.
/// @param pOutput
///    [Output] Pointer to the DCM ouput structure.
/// @param eDescriptorLPF
///    [Input] Boolean to enable Descriptor Low Pass Filter.
///    Set to true by default.
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_descriptor_calculation_matching
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDcm_Async(cvpHandle       hDcm,
                                  const cvpImage *pSrcImage,
                                  const cvpMem *pSearchPosition,
                                  const cvpMem *pRefDescriptor,
                                  uint32_t         nNumDCPoints,
                                  uint32_t         nNumDCMRefPoints,
                                  bool             eDescriptorLPF,
                                  cvpDcmOutput   *pOutput,
                                  const void      *pTaskUserData);

#ifdef __cplusplus
}//extern "C"
#endif

#endif
