/**=============================================================================

@file
   cvpMem.h

@brief
   Memory definition for Computer Vision Processor.

Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//=============================================================================
///@details CVP memory management
///@ingroup cvp_mem_management
//=============================================================================

#ifndef CVP_MEM_H
#define CVP_MEM_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "cvpTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif


//------------------------------------------------------------------------------
/// @brief
///    Enumeration that represents the buffer type.
/// @param CVP_MEM_BUFFER_INOUT_IMAGE
///    Buffer for input or output image.
/// @param CVP_MEM_BUFFER_IN_NCC_SEARCH
///    Buffer for NCC search.
/// @param CVP_MEM_BUFFER_IN_NCC_TEMPLATE
///    Buffer for NCC template.
/// @param CVP_MEM_BUFFER_IN_DCM_POINTS
///    Buffer for DCM points.
/// @param CVP_MEM_BUFFER_IN_DCM_REF
///    Buffer for DCM reference.
/// @param CVP_MEM_BUFFER_IN_REF_FPX
///    Buffer for Feature Point eXtraction image.
/// @param CVP_MEM_BUFFER_IN_DME_REF_CTX
///    Buffer for DME reference context.
/// @param CVP_MEM_BUFFER_IN_OD_MODEL
///    Buffer for Object Detection model.
/// @param CVP_MEM_BUFFER_IN_WARP_PERSPECTIVE_FRAME
///    Buffer for Warp perspective frame.
/// @param CVP_MEM_BUFFER_IN_WARP_GRID_FRAME
///    Buffer for Warp grid frame.
/// @param CVP_MEM_BUFFER_IN_REF_OD
///    Buffer for Object Detection image.
/// @param CVP_MEM_BUFFER_IN_REF_ODT
///    Buffer for Object Detection and Tracking image.
/// @param CVP_MEM_BUFFER_OUT_FPX
///    Buffer for Feature Point eXtraction output.
/// @param CVP_MEM_BUFFER_OUT_OF_STATS
///    Buffer for Optical Flow stats output.
/// @param CVP_MEM_BUFFER_OUT_DFS_DEPTH
///    Buffer for DFS depth output.
/// @param CVP_MEM_BUFFER_OUT_DFS_OCCLUSION
///    Buffer for DFS occlusion output.
/// @param CVP_MEM_BUFFER_OUT_NCC_SEARCH
///    Buffer for NCC search output.
/// @param CVP_MEM_BUFFER_OUT_NCC_SCORE
///    Buffer for NCC score output.
/// @param CVP_MEM_BUFFER_OUT_DCM_DESC
///    Buffer for DCM descriptor output.
/// @param CVP_MEM_BUFFER_OUT_DCM_MATCH
///    Buffer for DCM match output.
/// @param CVP_MEM_BUFFER_OUT_DME_SRC_CTX
///    Buffer for DME source context output.
/// @param CVP_MEM_BUFFER_OUT_OD
///    Buffer for Object Detection output.
/// @param CVP_MEM_BUFFER_OUT_ODT
///    Buffer for Object Detection and Tracking output.
/// @param CVP_MEM_BUFFER_OUT_DME_STATS
///    Buffer for DME stats and ftexture.
/// @ingroup cvp_mem_management
//------------------------------------------------------------------------------
typedef enum
{
   CVP_MEM_BUFFER_INOUT_IMAGE = 0,
   CVP_MEM_BUFFER_IN_NCC_SEARCH,
   CVP_MEM_BUFFER_IN_NCC_TEMPLATE,
   CVP_MEM_BUFFER_IN_DCM_POINTS,
   CVP_MEM_BUFFER_IN_DCM_REF,
   CVP_MEM_BUFFER_IN_REF_FPX,
   CVP_MEM_BUFFER_IN_DME_REF_CTX,
   CVP_MEM_BUFFER_IN_OD_MODEL,
   CVP_MEM_BUFFER_IN_WARP_PERSPECTIVE_FRAME,
   CVP_MEM_BUFFER_IN_WARP_GRID_FRAME,
   CVP_MEM_BUFFER_IN_REF_OD,
   CVP_MEM_BUFFER_IN_REF_ODT,
   CVP_MEM_BUFFER_OUT_FPX,
   CVP_MEM_BUFFER_OUT_OF_STATS,
   CVP_MEM_BUFFER_OUT_DFS_DEPTH,
   CVP_MEM_BUFFER_OUT_DFS_OCCLUSION,
   CVP_MEM_BUFFER_OUT_NCC_SEARCH,
   CVP_MEM_BUFFER_OUT_NCC_SCORE,
   CVP_MEM_BUFFER_OUT_DCM_DESC,
   CVP_MEM_BUFFER_OUT_DCM_MATCH,
   CVP_MEM_BUFFER_OUT_DME_SRC_CTX,
   CVP_MEM_BUFFER_OUT_OD,
   CVP_MEM_BUFFER_OUT_ODT,
   CVP_MEM_BUFFER_OUT_DME_STATS
} cvpMemBufferType;

//------------------------------------------------------------------------------
/// @brief
///    Allocates aligned memory for CVP process.
/// @details
///    Allocates aligned memory that can be shared among sessions.
///    Once done, users need to call cvpMemFree().
/// @param hSession
///    [Input] CVP session handle.
/// @param nAlignedBytes
///    [Input] Number of aligned bytes. This should satisfy the alignment requirement.
/// @param eSecureType
///    [Input] Secure buffer type.
/// @param ppMem
///    [Output] Pointer to pointer to the cvpMem structure.
///
/// @retval CVP_SUCCESS
///    If successful.
/// @retval CVP_ENORES
///    If no enough memory or resources
///
/// @ingroup cvp_mem_management
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpMemAlloc(cvpSession       hSession,
                              uint32_t         nBytes,
                              cvpMemSecureType eSecureType,
                              cvpMem         **ppMem);

//------------------------------------------------------------------------------
/// @brief
///    Free memory allocated by cvpMemAlloc().
/// @param hSession
///    [Input] CVP session handle.
/// @param pMem
///    [Input/Output] Pointer to cvpMem structure. After function called, the
///    structure parameters will be cleared.
///
/// @retval CVP_SUCCESS
///    If successful.
/// @retval CVP_EINVALSTATE
///    If it was called before cvpMemDeregister() or at the inproper state.
/// @retval CVP_EFAIL
///    If there is failure in freeing the cvpMem.
///
/// @ingroup cvp_mem_management
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpMemFree(cvpSession hSession,
                             cvpMem    *pMem);

//------------------------------------------------------------------------------
/// @brief
///    Register memory to CVP session.
/// @details
///    The memory can be allocated using cvpMemAlloc() or can be pre-allocated by
///    other module and pass it to this function as long as it's coming from
///    acceptable memory.
/// @param hSession
///    [Input] CVP session handle.
/// @param pMem
///    [Input] Pointer to cvpMem structure.
///
/// @ingroup cvp_mem_management
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpMemRegister(cvpSession    hSession,
                                 const cvpMem *pMem);

//------------------------------------------------------------------------------
/// @brief
///    Deregister memory from CVP session.
/// @param hSession
///    [Input] CVP session handle.
/// @param pMem
///    [Input] Pointer to cvpMem structure.
///
/// @ingroup cvp_mem_management
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpMemDeregister(cvpSession    hSession,
                                   const cvpMem *pMem);

//------------------------------------------------------------------------------
/// @brief
///    Get memory type
/// @param eMemFeature
///    [Input] CVP memory feature.
/// @param bSecure
///    [Input] Flag to indicate if it's for secure memory.
/// @param bDspAccess
///    [Input] Flag to indicate if DSP has permission to read and write.
/// @param memType
///    [Output] CVP memory type
///
/// @ingroup cvp_mem_management
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpMemGetType(cvpMemBufferType  eMemFeature,
                                bool              bSecure,
                                bool              bDspAccess,
                                cvpMemSecureType &eSecureType);

#ifdef __cplusplus
}//extern "C"
#endif

#endif
