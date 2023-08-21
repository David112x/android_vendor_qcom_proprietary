// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

#ifndef __PROCESS_ANR_PASS_H__
#define __PROCESS_ANR_PASS_H__

#include <string.h>
#include "ANR_Chromatix.h"
#include "NcLibContext.h"
#include "ipe_data.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
*  @brief   Set default values for ANR FW struct
*
*  @param [in]      fwStruct               The FW struct that is set.
*
*  @return void
*/
void SetDefaultsForAnrStruct(AnrParameters* fwStruct);

/**
*  @brief   Validate ANR SW Context
*
*  @param [in]      nclibContext           ANR SW context struct to validate
*  @param [in]      fullImageWidthPixels   ANR Full image width in pixels
*  @param [in]      fullImageHeightPixels  ANR Full image height in pixels
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t ValidateAnrContext(
    NCLIB_CONTEXT_ANR* nclibContext,
    uint32_t fullImageWidthPixels,
    uint32_t fullImageHeightPixels);

/**
*  @brief   Check whether noise reduction is enabled in Chromatix for specific pass
*
*  @param [in]      chromatix           ANR chromatix structure for the tested pass
*  @param [out]     yEnabled            Return value for Luma channel
*  @param [out]     cEnabled            Return value for Chroma channel
*
*  @return void
*/
void ANR_IsPassEnabled(ANR_Chromatix* chromatix, NCLIB_BOOLEAN* yEnabled, NCLIB_BOOLEAN* cEnabled);

#ifdef __cplusplus
}
#endif


#endif //__PROCESS_ANR_PASS_H__
