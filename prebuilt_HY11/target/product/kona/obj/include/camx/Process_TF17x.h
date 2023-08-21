// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

#ifndef __PROCESS_TF17x_H__
#define __PROCESS_TF17x_H__

#include "TF_Chromatix.h"
#include "NcLibContext.h"
#include "TF_Registers17x.h"


#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------
*       API Functions
* ----------------------------------------------------------------------- */

/**
*  @brief   Calculate TF, Refinement and LMC FW structs and TF registers for all passes
*           Calls TF_ProcessNcLib, TF_ConcludeFrameLevelRegistersConfiguration and TF_CalcWarpDependedParams
*           for each pass that do almost all the work.
*
*
*  @param [in]      chromatixStruct           Array of TF Chromatix structs with at least numOfPasses elements
*  @param [in]      context                   Contains SW information which is relevant for activation
*  @param [in]      numOfPasses               Defines how many passes are used
*  @param [in]      transformConfidenceVal    Defines mapping function from calculated transform
*                                             confidence to actually used transform confidence.
*                                             The calculated confidence range is 0:256 (8 bit fraction). format: 9uQ8
*                                             If transformConfidenceVal is set to 256 confidence has no effect.
*  @param [out]     regStruct                 Output array of TF register structs with at least numOfPasses elements
*  @param [out]     fwRefinementParameters    Output Refinement FW struct
*  @param [out]     fwTfParameters            Output TF FW struct
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t TF_ProcessNcLibFull17x(
    TF_Chromatix          chromatixStruct[],
    NCLIB_CONTEXT_TF*     context,
    uint32_t              numOfPasses,
    uint32_t              transformConfidenceVal,
    IPETFRegCmd17x           regStruct[],
    RefinementParameters* fwRefinementParameters,
    TfParameters*         fwTfParameters);

void SetDefaultVal_TF_REG17x(IPETFRegCmd17x* regStruct, uint32_t numOfPasses);
int32_t Validate_TF_REG17x(IPETFRegCmd17x* regStruct, uint32_t numOfPasses);

#ifdef __cplusplus
}
#endif


#endif //__PROCESS_TF17x_H__

