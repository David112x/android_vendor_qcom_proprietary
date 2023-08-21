// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

#ifndef __PROCESS_DSX_H__
#define __PROCESS_DSX_H__

#include "DSX_Chromatix.h"
#include "DSX_Registers.h"
#include "DS4to1_Chromatix.h"
#include "ICA_Chromatix.h"
#include "NcLibContext.h"

/** Struct that defines DSX processing memory requirements */
struct DSX_ProcessNcLibBuffer
{
    FIELD_UINT64 buffInt[97];
    double buffDbl[384];
};

#ifdef __cplusplus
extern "C" {
#endif

    /**
    *  @brief   Calculate DSX registers which are set directly by Chromatix for one pass.
    *
    *  @param [in]      chromatixStruct             pointer to the pass DSX Chromatix struct
    *  @param [in]      ds4ChromatixStruct          pointer to the pass DS4 Chromatix struct
    *  @param [in]      icaChromatixStruct          pointer to the pass ICA Chromatix struct
    *  @param [in]      context                     Contains SW information which is relevant for activation
    *  @param [out]     buffer                      pointer to a temporary memory buffer needed for processing
    *  @param [out]     regStruct                   pointer to the pass register struct
    *
    *  @return NC_LIB_SUCCESS in case of success, otherwise failed.
    */
    int32_t DSX_ProcessNcLib(
        const DSX_Chromatix* chromatixStruct,
        const DS4to1_Chromatix* ds4ChromatixStruct,
        const ICA_Chromatix* icaChromatixStruct,
        const NCLIB_CONTEXT_DSX* context,
        DSX_ProcessNcLibBuffer* buffer,
        DSX_REG* regStruct);

    /**
    *  @brief   Validate calculated kernel weights per HW constraints.
                This HW configuration should be restricted (due to HW limitation).
    *
    *  @param [in]      regs                   pointer to the pass register struct
    *
    *  @return 0 if weights are valid, number of invalid weights arrays otherwise.
    */
    uint32_t DSX_ValidateKernelWeights(DSX_REG* regs);

    /**
    *  @brief   Validate calculated padding weights.
                Usage of "valid" configuration is desired in order to get good Image Quality.
    *
    *  @param [in]      regs                   pointer to the pass register struct
    *
    *  @return 0 if weights are valid, number of invalid weights arrays otherwise.
    */
    uint32_t DSX_ValidatePaddingWeights(DSX_REG* regs);

    /**
    *  @brief   Validate relation between DSX Chromatix parameters
    *
    *  @param [in]      chromatixStruct             pointer to the pass DSX Chromatix struct
    *  @param [in]      ds4ChromatixStruct          pointer to the pass DS4 Chromatix struct
    *  @param [in]      icaChromatixStruct          pointer to the pass ICA Chromatix struct
    *
    *  @return NC_LIB_SUCCESS in case of success, otherwise failed.
    */
    int32_t DSX_CrossValidateChromatix(
        const DSX_Chromatix* chromatixStruct,
        const DS4to1_Chromatix* ds4ChromatixStruct,
        const ICA_Chromatix* icaChromatixStruct);

#ifdef __cplusplus
}
#endif


#endif //__PROCESS_DSX_H__
