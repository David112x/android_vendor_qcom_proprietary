/**=============================================================================

@file
   cvpUtils.h

@brief
   API Definition for Computer Vision Processor.

Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//=============================================================================
///@details CVP Utility API that use Computer Vision Processor acceleration
///@ingroup cvp_utils
//=============================================================================

#ifndef CVP_UTILS_H
#define CVP_UTILS_H

#include "cvpTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
/// @brief
///    Alignment utility
/// @details
///    Specific API required alignment in allocating memory, passing certain
///    parameters.
/// @param nValue
///    [Input] Value to be aligned.
/// @param nAlignment
///    [Input] Alignment value.
///
/// @retval alignment value
///
/// @ingroup cvp_utils
//------------------------------------------------------------------------------
CVP_API uint32_t cvpAlignmentu32(uint32_t nValue,
                                 uint32_t nAlignment);

//------------------------------------------------------------------------------
/// @brief
///    Float to fix point conversion.
/// @param nIn
///    [Input] A floating point number.
/// @param pOut
///    [Output] A pointer to Q format fixed point number that has the fractional bit
///    specified by nQFracBit. If there is loss of precision, it will round to
///    the closest possible value. E.g. nIn = 4.5, nQFracBit = 0, then nOut = 5.
/// @param nQFracBit
///    [Input] Q fractional bit. For example, if the Q format is Q13.2 that has
///    13 integer bit and 2 fractional bit, the nQFracBit is equal to 2.
///
/// @retval CVP_SUCCESS
///    If successfully.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_utils
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpFloatToFixf32u16(float32_t  nIn,
                                      uint16_t  *pOut,
                                      uint8_t    nQFracBit);

//------------------------------------------------------------------------------
/// @brief
///    Float to fix point conversion.
/// @param nIn
///    [Input] A pointer to Q format fixed point number that has the fractional bit
///    specified by nQFracBit.
/// @param pOut
///    [Output] Pointer to a floating point number.
/// @param nQFracBit
///    [Input] Q fractional bit. For example, if the Q format is Q13.2 that has
///    13 integer bit and 2 fractional bit, the nQFracBit is equal to 2.
///
/// @retval CVP_SUCCESS
///    If successfully.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_utils
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpFixToFloatu16f32(uint16_t   nIn,
                                      float32_t *pOut,
                                      uint8_t    nQFracBit);

//------------------------------------------------------------------------------
/// @brief
///    Query image information based on the color format, width, and height.
/// @param eFormat
///    [Input] Enum representing the color format of the image.
/// @param nWidth
///    [Input] Width of the Image.
/// @param nHeight
///    [Input] Height of the Image.
/// @param sImageInfo
///    [Output] Pointer to the image information structure.
///
/// @retval CVP_SUCCESS
///    If successfully.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_utils
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpQueryImageInfo(cvpColorFormat eFormat,
                                    uint32_t       nWidth,
                                    uint32_t       nHeight,
                                    cvpImageInfo  *pImageInfo);

//------------------------------------------------------------------------------
/// @brief
///    ICA+DME specific Query image information based on the color format, width,
///    and height. This will be moved/renamed to a different API is future revision.
/// @param eFormat
///    [Input] Enum representing the color format of the image.
/// @param nWidth
///    [Input] Width of the Image.
/// @param nHeight
///    [Input] Height of the Image.
/// @param sImageInfo
///    [Output] Pointer to the image information structure.
/// @param isICA
///    [Input] ICA enable.
///
/// @retval CVP_SUCCESS
///    If successfully.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_utils
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpQueryImageInfo_dme(cvpColorFormat eFormat,
                                        uint32_t       nWidth,
                                        uint32_t       nHeight,
                                        cvpImageInfo  *pImageInfo,
                                        uint32_t       isICA);

#ifdef __cplusplus
}//extern "C"
#endif
#endif
