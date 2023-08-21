/**=============================================================================

@file
   cvpNcc.h

@brief
   API Definitions for Normalized Cross Correlation (NCC)

Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//=============================================================================
///@details CVP NCC APIs using Computer Vision Processor acceleration
///@ingroup cvp_object_detection
//=============================================================================

#ifndef CVP_NCC_H
#define CVP_NCC_H

#include "cvpTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
/// @brief
///    NCC operation mode.
/// @param CVP_NCC_PATCH_MODE
///    NCC patch based mode.
/// @param CVP_NCC_FRAME_MODE
///    NCC frame based mode.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef enum
{
   CVP_NCC_PATCH_MODE = 0x00,
   CVP_NCC_FRAME_MODE = 0x02
} cvpNccMode;

//------------------------------------------------------------------------------
/// @brief
///    NCC score mode
/// @param CVP_NCC_BEST_SCORE
///    Only give the best result output.
/// @param CVP_NCC_ALL_SCORE
///    Give all NCC score output.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef enum
{
   CVP_NCC_BEST_SCORE = 0,
   CVP_NCC_ALL_SCORE
} cvpNccScoreType;

//------------------------------------------------------------------------------
/// @brief
///    Packed structure for NCC search windows.
/// @param nLocX_Ref
///    Represented by bit 15:0. Top left of 18x18 window position x in uQ12.4 format.
/// @param nLocY_Ref
///    Represented by bit 31:16. Top left of 18x18 window position y in uQ12.4 format.
/// @param nLocX_Src
///    Represented by bit 45:32. For CVP_NCC_FRAME_MODE only. In CVP_NCC_PATCH_MODE,
///    it will be ignored. Top left of current 8x8 window position x in integer format.
/// @param nLocY_Src
///    Represented by bit 59:46. For CVP_NCC_FRAME_MODE only. In CVP_NCC_PATCH_MODE,
///    it will be ignored. Top left of current 8x8 window position y in integer format.
/// @param nReserved_0
///    Represented by bit 62:60. Unused reserved bit.
/// @param nReserved_1
///    Represented by bit 63. Unused reserved bit.
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpNccSearchWindow
{
   uint64_t nLocX_Ref   : 16;  // 15:0
   uint64_t nLocY_Ref   : 16;  // 31:16
   uint64_t nLocX_Src   : 14;  // 45:32
   uint64_t nLocY_Src   : 14;  // 59:46
   uint64_t nReserved_0 : 3;   // 62:60
   uint64_t nReserved_1 : 1;   // 63

   #ifdef __cplusplus
   INLINE _cvpNccSearchWindow()
   {
      memset(this, 0, sizeof (*this));
   }
   #endif
} cvpNccSearchWindow;

//------------------------------------------------------------------------------
/// @brief
///    Packed structure for NCC result windows.
/// @param nLocX_Match
///    Represented by bit 15:0. Top left of optimized 8x8 best matched window
///    position x in uQ12.4 format.
/// @param nLocY_Match
///    Represented by bit 31:16. Top left of optimized 8x8 best matched window
///    position y in uQ12.4 format.
/// @param nLocX_Src
///    Represented by bit 45:32. For CVP_NCC_FRAME_MODE only. In CVP_NCC_PATCH_MODE,
///    it should be ignored. Top left of current 8x8 window position x in integer
///    format. The value was copied from the input cvpNccSearchWindows.nLocX_Src.
/// @param nLocY_Src
///    Represented by bit 59:46. For CVP_NCC_FRAME_MODE only. In CVP_NCC_PATCH_MODE,
///    it should be ignored. Top left of current 8x8 window position y in integer
///    format. The value was copied from the input cvpNccSearchWindows.nLocY_Src.
/// @param nRobustnes
///    Represented by bit 62:60. Robustness value of NCC.
/// @param nSubpel
///    Represented by bit 63. A flag that inidicates if subpel was enabled (from
///    config).
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpNccResultWindow
{
   uint64_t nLocX_Match : 16;  // 15:0
   uint64_t nLocY_Match : 16;  // 31:16
   uint64_t nLocX_Src   : 14;  // 45:32
   uint64_t nLocY_Src   : 14;  // 59:46
   uint64_t nRobustnes  : 3;   // 62:60
   uint64_t nSubpel     : 1;   // 63

   #ifdef __cplusplus
   INLINE _cvpNccResultWindow()
   {
      memset(this, 0, sizeof (*this));
   }
   #endif
} cvpNccResultWindow;

//------------------------------------------------------------------------------
/// @brief
///    Structure for NCC output.
/// @param pNccScore
///    Pointer to the NCC score. Min buffer size is nNccScoreSize * 2.
///    The score memory layout is depending on eScoreType.
///    If eScoreType == CVP_NCC_BEST_SCORE, then size of buffer is
///    (nNccRequests + 7) / 8 * 8 * 2 bytes. Each template generates one best score which has 16 bit.
///    If eScoreType == CVP_NCC_ALL_SCORE, then size of buffer is
///    nNccRequests * 12 * 12 * 2 bytes. All score results will be saved as 4x1
///    pixels in vertical raster scan order.
///    Refer to Programming Guide for details
/// @param pBestPosition
///    Pointer to the structure for the best position for each search window.
///    Min buffer size is nBestPositionSize * sizeof(cvpNccResultWindow) bytes.
///    Structure cvpNccResultWindow is used to describe each position.
/// @param sUBWCRefImage
///    Structure to the converted UBWC reference image. This will be generated
///    only if the input reference image is not in UBWC format.
/// @param nNccScoreSize
///    Number of pNccScore.
/// @param nBestPositionSize
///    Number of pBestPosition which is equal to nNccRequests.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct cvpNccOutput
{
   cvpMem             *pNccScore;
   cvpMem             *pBestPosition;
   cvpImage            sUBWCRefImage;
   uint32_t            nNccScoreSize;
   uint32_t            nBestPositionSize;

   #ifdef __cplusplus
   INLINE cvpNccOutput() : pNccScore(NULL),
                           pBestPosition(NULL),
                           nNccScoreSize(0),
                           nBestPositionSize(0) {}
   #endif

} cvpNccOutput;

//------------------------------------------------------------------------------
/// @brief
///    The NCC has been processed by the CVP system, and the
///    output is ready to be consumed. This callback is used to notify the
///    application. The application sends the image using the
///    cvpNcc_Async call.
///
/// @param eStatus
///    CVP status for the current process.
/// @param pOutput
///    Pointer to the NCC output structure.
/// @param hNcc
///    Handle for the NCC that was passed in the cvpNcc_Async function.
/// @param pSessionUserData
///    User-data that was set in the cvpInitNcc structure.
/// @param pTaskUserData
///    User-data that was passed in the cvpNcc_Async function
///    which corresponds to this callback.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef void (*CvpNccCb)(cvpStatus     eStatus,
                         cvpNccOutput *pOutput,
                         cvpHandle     hNcc,
                         void         *pSessionUserData,
                         void         *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Structure for NCC configuration.
/// @param nActualFps
///    Input frames per second. Default value is 30.
/// @param nOperationalFps
///    Desired output frames per second. nOperationalFps should be equal to
///    or greater than nActualFps. nOperationalFps >= nActualFps. Default value is 30.
/// @param sImageInfo
///    Structure for image information.
/// @param eScoreType
///    NCC output score type.
/// @param eMode
///    NCC operation mode.
/// @param nMaxTemplates
///    Maximum number of supported templates. This is used to calculate NCC
///    buffer size. Max value is 750.
/// @param bRobustness
///    Flag to enable robustness. By default, robustness is disable (false).
/// @param bSubPel
///    Flag to enable subpel. By default, subpel is enable (true).
/// @param bRightShiftRounding
///    Flag to enable right shift rounding control. By default, it is enable (true).
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpConfigNcc
{
   uint32_t        nActualFps;
   uint32_t        nOperationalFps;
   cvpImageInfo    sImageInfo;
   cvpNccScoreType eScoreType;
   cvpNccMode      eMode;
   uint32_t        nMaxTemplates;
   bool            bRobustness;
   bool            bSubPel;
   bool            bRightShiftRounding;

   #ifdef __cplusplus
   INLINE _cvpConfigNcc() : nActualFps(30),
                            nOperationalFps(30),
                            eScoreType(CVP_NCC_BEST_SCORE),
                            eMode(CVP_NCC_PATCH_MODE),
                            nMaxTemplates(750),
                            bRobustness(false),
                            bSubPel(true),
                            bRightShiftRounding(true)
                            {}
   #endif

} cvpConfigNcc;

//------------------------------------------------------------------------------
/// @brief
///    Structure for NCC output buffer Requirement
/// @param nNccScoreBytes
///    The required size in bytes for NCC score buffer.
/// @param nBestPositionBytes
///    The required size in bytes for best position buffer.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpNccOutBuffReq
{
   uint32_t nNccScoreBytes;
   uint32_t nBestPositionBytes;
   uint32_t nUBWCRefImageBytes;

   #ifdef __cplusplus
   INLINE _cvpNccOutBuffReq() : nNccScoreBytes(0), nBestPositionBytes(0), nUBWCRefImageBytes(0){}
   #endif

} cvpNccOutBuffReq;

//------------------------------------------------------------------------------
/// @brief
///    Initialize CVP - NCC.
/// @param hSession
///    [Input] CVP session handle
/// @param pConfig
///    [Input] Pointer to the configuration.
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
/// @retval CVP handle for NCC.
///    If successful.
/// @retval NULL
///    If initialization failed.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpHandle cvpInitNcc(cvpSession          hSession,
                             const cvpConfigNcc *pConfig,
                             cvpNccOutBuffReq   *pOutMemReq,
                             CvpNccCb            fnCb,
                             void               *pSessionUserData);

//------------------------------------------------------------------------------
/// @brief
///    Deinitialize CVP - NCC.
/// @param hNcc
///    [Input] CVP handle for NCC.
///
/// @retval CVP_SUCCESS
///    If deinit is successful.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeInitNcc(cvpHandle hNcc);

//------------------------------------------------------------------------------
/// @brief
///    NCC using patch based mode.
/// @details
///    Synchronous (blocking) function that will do NCC using patch based mode.
/// @param hNcc
///    [Input] Handle for the NCC.
/// @param pRefImage
///    [Input] Pointer to the reference image. The image can be in linear or
///    UBWC format. If it's in linear format, the converted UBWC format image
///    will be generated as part of the cvpNccOutput. Max resolution is 1920x1080.
/// @param pTemplateBuffer
///    [Input] Pointer to 8x8 templates. It's 1 byte per pixel.
///    Size of buffer is cvpAlignmentu32(nNccRequests,16) * 8 * 8 bytes.
/// @param pSearchWindows
///    [Input] Pointer to the search windows structure.
///    Size of buffer is cvpAlignmentu32(nNccRequests,16) * sizeof(cvpNccSearchWindow)
///    bytes. For each template and each search window, there will be 11*11 searches.
///    Structure cvpNccSearchWindow is used to describe each position.
/// @param nNccRequests
///    [Input] Number of NCC requests. Max is 750.
/// @param pOutput
///    [Output] Pointer to the NCC ouput structure.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is initialized for the asynchronous API or CVP_NCC_FRAME_MODE.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpNccPatch_Sync(cvpHandle            hNcc,
                                   const cvpImage      *pRefImage,
                                   cvpMem              *pTemplateBuffer,
                                   cvpMem              *pSearchWindows,
                                   uint32_t             nNccRequests,
                                   cvpNccOutput        *pOutput);

//------------------------------------------------------------------------------
/// @brief
///    NCC using patch based mode.
/// @details
///    Asynchronous function that will queue the image and return almost
///    immediately. In the background, it will do NCC using patch based mode.
///    Once the output is ready, it will notify the user through the callback
///    function and user can queue another image using the same buffer.
/// @param hNcc
///    [Input] Handle for the NCC.
/// @param pRefImage
///    [Input] Pointer to the reference image. The image can be in linear or
///    UBWC format. If it's in linear format, the converted UBWC format image
///    will be generated as part of the cvpNccOutput. Max resolution is 1920x1080.
/// @param pTemplateBuffer
///    [Input] Pointer to 8x8 templates. It's 1 byte per pixel.
///    Size of buffer is cvpAlignmentu32(nNccRequests,16) * 8 * 8 bytes.
/// @param pSearchWindows
///    [Input] Pointer to the search windows structure.
///    Size of buffer is cvpAlignmentu32(nNccRequests,16) * sizeof(cvpNccSearchWindow)
///    bytes. For each template and each search window, there will be 11*11 searches.
///    Structure cvpNccSearchWindow is used to describe each position.
/// @param nNccRequests
///    [Input] Number of NCC requests. Max is 750.
/// @param pOutput
///    [Output] Pointer to the NCC ouput structure which  corresponds to
///    the callback.
/// @param pTaskUserData
///    [Input] User-data which corresponds to the callback.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is initialized for the synchronous API or CVP_NCC_FRAME_MODE.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpNccPatch_Async(cvpHandle            hNcc,
                                    const cvpImage      *pRefImage,
                                    cvpMem              *pTemplateBuffer,
                                    cvpMem              *pSearchWindows,
                                    uint32_t             nNccRequests,
                                    cvpNccOutput        *pOutput,
                                    const void          *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    NCC using frame based mode.
/// @details
///    Synchronous (blocking) function that will do NCC using frame based mode.
/// @param hNcc
///    [Input] Handle for the NCC.
/// @param pRefImage
///    [Input] Pointer to the reference image. The image can be in linear or
///    UBWC format. If it's in linear format, the converted UBWC format image
///    will be generated as part of the cvpNccOutput. Max resolution is 1920x1080.
/// @param pSrcImage
///    [Input] Pointer to the source image. The source image might be from
///    the previous image. Must be in UBWC format. Max resolution is 1920x1080.
/// @param pSearchWindows
///    [Input] Pointer to the search windows structure.
///    Size of buffer is cvpAlignmentu32(nNccRequests,16) * sizeof(cvpNccSearchWindow)
///    bytes. For each template and each search window, there will be 11*11 searches.
///    Structure cvpNccSearchWindow is used to describe each position.
/// @param nNccRequests
///    [Input] Number of NCC requests. Max is 750.
/// @param pOutput
///    [Output] Pointer to the NCC ouput structure.
/// @param pTaskUserData
///    [Input] User-data which corresponds to the callback.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is initialized for the asynchronous API or CVP_NCC_PATCH_MODE.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpNccFrame_Sync(cvpHandle            hNcc,
                                   const cvpImage      *pRefImage,
                                   const cvpImage      *pSrcImage,
                                   cvpMem              *pSearchWindows,
                                   uint32_t             nNccRequests,
                                   cvpNccOutput        *pOutput);

//------------------------------------------------------------------------------
/// @brief
///    NCC using frame based mode.
/// @details
///    Synchronous (blocking) function that will do NCC using frame based mode.
/// @param hNcc
///    [Input] Handle for the NCC.
/// @param pRefImage
///    [Input] Pointer to the reference image. The image can be in linear or
///    UBWC format. If it's in linear format, the converted UBWC format image
///    will be generated as part of the cvpNccOutput. Max resolution is 1920x1080.
/// @param pSrcImage
///    [Input] Pointer to the source image. The source image might be from
///    the previous image. Must be in UBWC format. Max resolution is 1920x1080.
/// @param pSearchWindows
///    [Input] Pointer to the search windows structure.
///    Size of buffer is cvpAlignmentu32(nNccRequests,16) * sizeof(cvpNccSearchWindow)
///    bytes. For each template and each search window, there will be 11*11 searches.
///    Structure cvpNccSearchWindow is used to describe each position.
/// @param nNccRequests
///    [Input] Number of NCC requests. Max is 750.
/// @param pOutput
///    [Output] Pointer to the NCC ouput structure.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is initialized for the asynchronous API or CVP_NCC_PATCH_MODE.
///
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpNccFrame_Async(cvpHandle            hNcc,
                                    const cvpImage      *pRefImage,
                                    const cvpImage      *pSrcImage,
                                    cvpMem              *pSearchWindows,
                                    uint32_t             nNccRequests,
                                    cvpNccOutput        *pOutput,
                                    const void          *pTaskUserData);
#ifdef __cplusplus
}//extern "C"
#endif

#endif
