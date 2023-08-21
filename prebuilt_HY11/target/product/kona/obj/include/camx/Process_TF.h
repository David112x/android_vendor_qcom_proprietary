// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

#ifndef __PROCESS_TF_H__
#define __PROCESS_TF_H__

#include "TF_Chromatix.h"
#include "NcLibContext.h"
#include "NcLibChipInfo.h"
#include <math.h>
#include <string.h>
#include "ipe_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------
*       API Functions
* ----------------------------------------------------------------------- */


/**
*  @brief   Validate TF SW Context
*
*
*  @param [in]      nclibContext              Contains SW information which is relevant for activation
*  @param [in]      curPassScalingRatioLog4   Define which pass is used
*                                             0 - full pass
*                                             1 - DC4  pass
*                                             2 - DC16 pass
*                                             3 - DC64 pass
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t ValidateTfContext(NCLIB_CONTEXT_TF* nclibContext, uint32_t curPassScalingRatioLog4);

/**
*  @brief   Validate TF Refinement calculation result
*
*
*  @param [in]      fwStructTf                TF FW struct
*  @param [in]      fwStructRefinement        Refinement FW struct
*  @param [in]      curPassScalingRatioLog4   Define which pass is used
*                                             0 - full pass
*                                             1 - DC4  pass
*                                             2 - DC16 pass
*                                             3 - DC64 pass
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t ValidateTfCalc(TfPassParameters* fwStructTf, RefinementPassParameters* fwStructRefinement, uint32_t curPassScalingRatioLog4);

/**
*  @brief   Set default FW structs settings
*           Should be called immediately after TfPassParameters, RefinementPassParameters and LmcPassParameters creation
*
*  @param [out]     fwStructTf                TF FW struct
*  @param [out]     fwStructRefinement        Refinement FW struct
*  @param [out]     fwStructLmc               LMC FW struct
*/
void SetDefaultsForTFStructs(TfPassParameters* fwStructTf, RefinementPassParameters* fwStructRefinement, LmcPassParameters* fwStructLmc);

/**
*  @brief   Function that validates that relationship between different Chromatix settings are valid.
*
*
*  @param [in]      chromatixStruct           TF Chromatix struct
*  @param [in]      nclibContext              Contains SW information which is relevant for activation
*  @param [in]      curPassScalingRatioLog4   Define which pass is used
*                                             0 - full pass
*                                             1 - DC4  pass
*                                             2 - DC16 pass
*                                             3 - DC64 pass
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t Validate_TF_ChromatixParamRelations(
    TF_Chromatix* chromatixStruct,
    NCLIB_CONTEXT_TF* nclibContext,
    uint32_t curPassScalingRatioLog4);

uint32_t floorLog2( uint32_t uNumber, uint32_t minBit );

#ifndef __forceinline
#ifdef __GNUC__
#define __forceinline static inline __attribute__((always_inline))
#else
#define __forceinline static inline
#endif
#endif

__forceinline int32_t linInterpQ8(int32_t v1, int32_t v2, int32_t dxQ8) // dxQ8 can be in [0,256] inclusive
{
    return v1 + (((v2 - v1)*dxQ8 + 128) >> 8);
}


#ifdef __cplusplus
}
#endif


#endif //__PROCESS_TF_H__

