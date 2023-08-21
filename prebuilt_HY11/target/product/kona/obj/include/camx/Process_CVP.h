// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

#ifndef __PROCESS_CVP_H__
#define __PROCESS_CVP_H__

#include "CVP_Chromatix.h"
#include "ImageTransform.h"
#include "NcLibWarpCommonDef.h"
#include "GeoLib.h"

/**
* CVP ICA-related declarations
*/
#define CVP_ICA_MAX_GRID_DATASIZE 945
#define CVP_ICA_MAX_PRSP_PRAMS    128
/* Note: must be binary identical to cvpDmeFrameConfigIca from cvpDme.h */
typedef struct _NcLibcvpDmeFrameConfigIca
{
    uint64_t     nGridDataSize;
    uint64_t     nGridData[CVP_ICA_MAX_GRID_DATASIZE];
    uint32_t     nPerspectiveDataSize;
    uint32_t     nPerspectiveData[CVP_ICA_MAX_PRSP_PRAMS];
    uint32_t     nCTCPerspTransformGeomM;
    uint32_t     nCTCPerspTransformGeomN;
    int32_t      nOpgInterpLUT0[16];
    int32_t      nOpgInterpLUT1[16];
    int32_t      nOpgInterpLUT2[16];
    uint32_t     nCTCHalfOutputFrameWidth;
    uint32_t     nCTCHalfOutputFrameHeight;
    uint32_t     nCtcInputCoordPrecision;
    uint32_t     nCtcO2vScaleFactor_x;
    uint32_t     nCtcO2vScaleFactor_y;
    uint32_t     nCtcV2iInvScaleFactor_x;
    uint32_t     nCtcV2iInvScaleFactor_y;
    uint32_t     nControllerValidWidthMinus1;
    uint32_t     nControllerValidHeightMinus1;
    uint32_t     nCtcO2vOffset_x;
    uint32_t     nCtcO2vOffset_y;
    uint32_t     nCtcV2iOffset_x;
    uint32_t     nCtcV2iOffset_y;
    uint32_t     nCtcXTranslation;
    uint32_t     nCtcYTranslation;

    inline _NcLibcvpDmeFrameConfigIca() {}
} NcLibcvpDmeFrameConfigIca;

/**
*  @brief   Convert ICA v3.0 Grid to ICA v2.0 grid
*
*  @param [in]      inGridX                     pointer to input Grid X coordinates.
*  @param [in]      inGridY                     pointer to input Grid Y coordinates.
*  @param [in]      inGridTransformGeometry     defines the number of active grid samples. 0: 35x27 samples; 1: 67x51 samples.
*  @param [out]     outGridY                    pointer to output Grid X coordinates, 35x27 samples
*  @param [out]     outGridX                    pointer to output Grid Y coordinates, 35x27 samples.
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t NcLibConvertV30GridToV20(
    const int32_t* inGridX,
    const int32_t* inGridY,
    uint32_t inGridTransformGeometry,
    int32_t* outGridX,
    int32_t* outGridY);


/**
*  @brief   Convert ICA v3.0 matrices to ICA v2.0 matrices
*
*  @param [in]      numberOfMatrices            Number of matrices to convert (up to 9 are valid)
*  @param [in]      inExponent                  pointer to ICA v3.0 exponent Lut of perspective (the size of LUT is 8 * numberOfMatrices).
*  @param [in]      inMantissa                  pointer to ICA v3.0 mantissa Lut of perspective (the size of LUT is 8 * numberOfMatrices).
*  @param [out]     outExponent                 pointer to ICA v2.0 exponent Lut of perspective (the size of LUT is 8 * numberOfMatrices).
*  @param [out]     outMantissa                 pointer to ICA v2.0 mantissa Lut of perspective (the size of LUT is 8 * numberOfMatrices).
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t NcLibConvertV30MatricesToV20(
    uint32_t numberOfMatrices,
    const int32_t* inExponent,
    const int32_t* inMantissa,
    int32_t* outExponent,
    int32_t* outMantissa);

/**
*  @brief   Calculate ICA transform based on DME processing.
*
*  @param [in]   coarseCenterW        Image center X
*  @param [in]   coarseCenterH        Image center Y
*  @param [in]   upscaleFactorW       Transform upscale factor X
*  @param [in]   upscaleFactorH       Transform upscale factor Y
*  @param [in]   cvpResult            CVP DME results
*  @param [out]  out                  DME transform
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t ProcessCvpMeResult(
    float coarseCenterW,
    float coarseCenterH,
    float upscaleFactorW,
    float upscaleFactorH,
    const float cvpResult[3][3],
    CPerspectiveTransform* transform);

/**
*  @brief   Calculate TF confidence based on DME transform confidence
*
*  @param [in]      chromatixStruct              Chromatix configuration
*  @param [in]      transformConfidence          The transform confidence of DME results
*  @param [out]     enableTransformConfidence    Will be set to 1 if transform confidence is enabled.
*                                                Should be passed to FW in TfPassParameters::enableTransformConfidence.
*  @param [out]     transformConfidenceVal       Will receive the updated transform confidence.
*                                                Should be passed to FW in TfPassParameters::transformConfidenceVal.
*  @param [in,out]  isTransformForcedToIdentity  optional parameter (pass NULL to disable it).
*                                                if enabled (not NULL) state of "transform forced to identity" for
*                                                hysteresis implementation.
*                                                By default should be set to 0.
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t ConfigureCvpConfidenceParameter(
    const CVP_Chromatix* chromatixStruct,
    uint32_t transformConfidence,
    uint32_t* enableTransformConfidence,
    int32_t* transformConfidenceVal,
    uint8_t* isTransformForcedToIdentity);

#define NCLIB_PROCESS_CVP_ICA_PROC_BUFFER_SIZE (ICA_V20_GRID_TRANSFORM_SIZE * 16)

/**
*  @brief   Calculate CVP pre-DME ICA registers
*
*  @param [in]      grid               input grid. Geometry has to be either NcLibIcaGrid35x27 or NcLibIcaGrid67x51.
*  @param [in]      persp              input perspective transform matrices.
*  @param [in]      geo                input ICA geometrical mapping struct.
*  @param [in,out]  procBuffer         Pointer to intermediate buffer of at least NCLIB_PROCESS_CVP_ICA_PROC_BUFFER_SIZE
*                                      bytes. The function will use this buffer for intermediate calculations.
*  @param [in]      translationOnly    Whther CTC transform mode is TRANSLATION_ONLY. If 1, grid and perspective are not filled.
*  @param [in,out]  cvpStruct          Output struct filled by the function.
*                                      nOpgInterpLUT<n>, nCtcXTranslation and nCtcYTranslation are not populated.
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t NcLibProcessCvpIca(
    const NcLibWarpGrid* grid,       // Identical to IPEICAGridTransform
    const NcLibWarpMatrices* persp,  // Identical to IPEICAPerspectiveTransform
    const GeoLibIcaMapping* geo,
    uint32_t translationOnly,
    void* procBuffer,
    NcLibcvpDmeFrameConfigIca* cvpStruct);

#endif // __PROCESS_CVP_H__
