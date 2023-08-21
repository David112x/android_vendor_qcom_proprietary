// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

#ifndef __PROCESS_LRME_H__
#define __PROCESS_LRME_H__

#include "LRME_Chromatix.h"
#include "LRME_Registers.h"
#include "NcLibContext.h"
#include "ipe_data.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
*  @brief   Calculate LRME registers which are set directly by Chromatix.
*
*  @param [in]      chromatixStruct             pointer to the pass Chromatix struct
*  @param [out]     regStruct                   pointer to the pass register struct
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t LRME_ProcessNcLib(LRME_Chromatix* chromatixStruct, LRME_REG* regStruct);


/**
*  @brief   Calculate LRME non Chromatix registers and meta data.
*
*  @param [in]      chromatixStruct             pointer to the pass Chromatix struct
*  @param [in]      context                     LRME SW context struct
*  @param [out]     regStruct                   pointer to the pass register struct
*  @param [out]     regMetaData                 Contains meta data information that can be used by
*                                               SW to configure HW correctly
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t LRME_ConcludeRegistersConfiguration(
    LRME_Chromatix* chromatixStruct,
    NCLIB_CONTEXT_LRME* context,
    LRME_REG* regStruct,
    NCLIB_LRME_REG_METADATA* regMetaData);


/**
*  @brief   Calculate TF confidence based on LRME post processing (e.g Ransac) transform confidence
*
*  @param [in]      chromatixStruct             pointer to the pass Chromatix struct
*  @param [in]      transformConfidence         The transform confidence of LRME results (e.g. from Ransac)
*  @param [out]     enableTransformConfidence   Will be set to 1 if transform confidence is enabled.
*                                               Should be passed to FW in TfPassParameters::enableTransformConfidence.
*  @param [out]     transformConfidenceVal      Will receive the updated transform confidence.
*                                               Should be passed to FW in TfPassParameters::transformConfidenceVal.
*  @param [in,out]  isTransformForcedToIdentity optional parameter (NULL disable it).
*                                               if enabled (not zero) state of "transform forced to identity" for
*                                               hysteresis implementation.
*                                               By default should be set to 0.
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t LRME_ConfigureConfidenceParameter(
    const LRME_Chromatix* chromatixStruct,
    uint32_t transformConfidence,
    uint32_t* enableTransformConfidence,
    int32_t* transformConfidenceVal,
    uint8_t* isTransformForcedToIdentity );

/**
*  @brief   Validate LRME SW Context
*
*  @param [in]      nclibContext           LRME SW context struct to validate
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t ValidateLrmeContext(NCLIB_CONTEXT_LRME* nclibContext);

/**
*  @brief   Validate relation between LRME Chromatix parameters
*
*  @param [in]      chromatixStruct       LRME Chromatix struct
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t LRME_CrossValidateChromatix(const LRME_Chromatix* chromatixStruct);

/**
*  @brief   Validate calculations - LRME meta data and TF confidence.
*
*  @param [in]      regMetaData       LRME meta data
*  @param [in]      fwStructTf        TF FW struct (for confidence)
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t ValidateLrmeCalc(NCLIB_LRME_REG_METADATA* regMetaData, TfPassParameters* fwStructTf);

#ifdef __cplusplus
}
#endif


#endif //__PROCESS_LRME_H__

