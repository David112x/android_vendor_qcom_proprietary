// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

#ifndef __NCLIB_CONTEXT_H__
#define __NCLIB_CONTEXT_H__

#include <stdint.h>

#include "CommonDefs.h"
#include "ipe_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Unset optical center */
#define CONTEXT_OPTICAL_CENTER_NOT_SET (-1)

/** ANR, TF optical center is provided in logical values [0,1]
    format: 15uQ14
    top left is [0,0], bottom right is [1, 1] - which is translated in fixed point to:
    [OPTICAL_CENTER_LOGICAL_MAX, OPTICAL_CENTER_LOGICAL_MAX] */
#define OPTICAL_CENTER_LOGICAL_MAX_SHIFT (14)
#define OPTICAL_CENTER_LOGICAL_MAX (1<<OPTICAL_CENTER_LOGICAL_MAX_SHIFT)


/** Flow modes */
enum EIPEConfigOption
{
    IPE_CONFIG_VIDEO,        /**< normal video processing */
    IPE_CONFIG_STILL,        /**< normal still-image processing */
    IPE_CONFIG_MF_PRE,       /**< multi-frame processing; pre-filtering */
    IPE_CONFIG_MF_TEMPORAL,  /**< multi-frame processing; temporal blending */

    IPE_CONFIG_NUM           /**< only for SW purpose */
};

/** ICA minimal output restriction */
#define ICA_MIN_WIDTH_PIXELS (30)
#define ICA_MIN_HEIGHT_PIXELS (26)

/** ICA Array dimensions*/
enum EICAArraySizes
{
    ICA_MAX_PERSPECTIVE_TRANSFORMS = 9,                                                                                 /**< max number of perspective transforms */
    ICA_PARAMETERS_PER_PERSPECTIVE_TRANSFORM = 9,                                                                       /**< number of parameters per perspective transform */
    ICA_V10_GRID_TRANSFORM_WIDTH = 33,                                                                                  /**< ICA v1.0 grid width */
    ICA_V10_GRID_TRANSFORM_HEIGHT = 25,                                                                                 /**< ICA v1.0 grid height */
    ICA_V20_GRID_TRANSFORM_WIDTH = 35,                                                                                  /**< ICA v2.0 grid width */
    ICA_V20_GRID_TRANSFORM_HEIGHT = 27,                                                                                 /**< ICA v2.0 grid height */
    ICA_V20_GRID_TRANSFORM_SIZE = ICA_V20_GRID_TRANSFORM_WIDTH * ICA_V20_GRID_TRANSFORM_HEIGHT,                         /**< ICA v2.0 grid size */
    ICA_V30_GRID_TRANSFORM_LOW_RES_WIDTH = 35,                                                                          /**< ICA v3.0 low resolution grid width */
    ICA_V30_GRID_TRANSFORM_LOW_RES_HEIGHT = 27,                                                                         /**< ICA v3.0 low resolution grid height */
    ICA_V30_GRID_TRANSFORM_LOW_RES_SIZE = ICA_V30_GRID_TRANSFORM_LOW_RES_WIDTH * ICA_V30_GRID_TRANSFORM_LOW_RES_HEIGHT, /**< ICA v3.0 low resolution grid size */
    ICA_V30_GRID_TRANSFORM_HI_RES_WIDTH = 67,                                                                           /**< ICA v3.0 high resolution grid width */
    ICA_V30_GRID_TRANSFORM_HI_RES_HEIGHT = 51,                                                                          /**< ICA v3.0 high resolution grid height */
    ICA_V30_GRID_TRANSFORM_HI_RES_SIZE = ICA_V30_GRID_TRANSFORM_HI_RES_WIDTH * ICA_V30_GRID_TRANSFORM_HI_RES_HEIGHT     /**< ICA v3.0 high resolution grid size */
};

/** Struct that describes up to five faces */
typedef struct FD_CONFIG_CONTEXT_TAG
{
    uint32_t faceNum;            /**< Number of faces. Valid range is 0 - 5 */
    uint32_t faceCenterX[5];     /**< array that contain the X axis of faces center  */
    uint32_t faceCenterY[5];     /**< array that contain the Y axis of faces center  */
    uint32_t faceRadius[5];      /**< array that contain the radius of faces */
} FD_CONFIG_CONTEXT;

/** Struct that describes ANR SW context */
typedef struct NCLIB_CONTEXT_ANR_TAG
{
    uint32_t frameNumber;          /**< used for MF and may be used also for seed */
    FD_CONFIG_CONTEXT fdConfig;    /**< Face Detection for ANR */
    PARAM_INT optical_center_x; /**< Optical Center X.
                                 * logic value 0 is start of the image, OPTICAL_CENTER_LOGICAL_MAX is the end of
                                 * the image. format: 15uQ14.
                                 * If value == CONTEXT_OPTICAL_CENTER_NOT_SET then it is not
                                 * SET and we take from Chromatix. */
    PARAM_INT optical_center_y; /**< Optical Center Y.
                                 * logic value 0 is start of the image, OPTICAL_CENTER_LOGICAL_MAX is the end of
                                 * the image. format: 15uQ14.
                                 * If value == CONTEXT_OPTICAL_CENTER_NOT_SET then it is not
                                 * SET and we take from Chromatix. */

} NCLIB_CONTEXT_ANR;

/** Struct that describes TF SW context */
typedef struct NCLIB_CONTEXT_TF_TAG
{
    uint32_t frameNumber; /**< used for MF and may be used also for seed */

    uint32_t numberOfFrames;                 /**< used for MF */

    uint8_t tfHasReferenceInput;             /**< indication if TF has reference input */
    enum EIPEConfigOption tfConfigOption;    /**< flow mode */

    float upscalingFactorMFSR_ICA;           /**<  used for MFSR calculations (affects size of ICA out,
                                              *    thus affects refinement calculations etc) */

    /**< digital zoom window (at full resolution) - affects refinement settings */
    uint8_t isDigitalZoomEnabled;            /**< 1 if Digital zoom is enabled, else 0 */
    uint32_t dzStartX;                       /**< Digital zoom start X */
    uint32_t dzStartY;                       /**< Digital zoom start Y */
    uint32_t dzSizeX;                        /**< Digital zoom width */
    uint32_t dzSizeY;                        /**< Digital zoom height */
    uint8_t preferPaddingByReflection;       /**< FW settings, indicate how to handle missing data.
                                              *   if 1 padding, else reflection*/
    uint32_t fullPassIcaOutputFrameWidth;    /**<  Calculated full pass ICA out width at ICA output of full pass,
                                              *    assuming no scaling  (e.g. may account for EIS margins but not
                                              *    for MFSR scaling) */
    uint32_t fullPassIcaOutputFrameHeight;   /**<  Calculated full pass ICA out height at ICA output of full pass,
                                              *    assuming no scaling  (e.g. may account for EIS margins but not
                                              *    for MFSR scaling) */
    uint32_t testMode;                       /**<  Reserved for developer options, must be 0. */
} NCLIB_CONTEXT_TF;

/** Struct that describes GRA SW context */
typedef struct NCLIB_CONTEXT_GRA_TAG
{
    uint32_t frameNumber; /**< used for MF and may be used also for seed */

    uint8_t isIpeOut8bit; /**< if set to 1, IPE output is 8bit, else it is 10bit */
} NCLIB_CONTEXT_GRA;

/** Struct that describes DSX SW context */
typedef struct NCLIB_CONTEXT_DSX_TAG
{
    uint32_t inW;            /**< Input width [16, 8190] */
    uint32_t inH;            /**< Input hight [16, 16384] */
    uint32_t outW;           /**< Output width [2, 1696] */
    uint32_t outH;           /**< Output width [2, 13108] */
    float downscaleX;       /**< Step size X [1.25, 4] */
    float downscaleY;       /**< Step size Y [1.25, 4] */
    float offsetX;          /**< input offset X [0, 0.8*inW] */
    float offsetY;          /**< input offset Y [0, 0.8*inH] */

} NCLIB_CONTEXT_DSX;

/** Struct that describes LRME SW context */
typedef struct NCLIB_CONTEXT_LRME_TAG
{
    uint8_t lrmeHasReferenceInput;        /**< 1 if LRME had reference input, else 0 */
    uint8_t ifeAreDsImagesPD;             /**< 1 if LRME inputs are PD format, else 0 */
    uint8_t ifeAreDsImages10Bps;          /**< 1 if is 10bit, else 0 */
    uint16_t lrmeTransformConfidenceQ8;  /**< calculated LRME processed result confidence [0-256] */

    uint16_t fps;                        /**< Frames per second - required to be able to configure the
                                          *   search range automatically */
    uint32_t ifeOutFrameWidth;           /**< IFE output frame width. For LRME configuration these parameters
                                          *   are assumed to be equal to icaInputFrame size. */
    uint32_t ifeOutFrameHeight;          /**< IFE output frame height. For LRME configuration these parameters
                                          *   are assumed to be equal to icaInputFrame size. */
    uint8_t isTransformForcedToIdentity; /**< input parameter (should be initialized to 0): state
                                          *   of "transform forced to identity" for hysteresis implementation */

} NCLIB_CONTEXT_LRME;

/** Struct that describes LRME meta data, that will be used by SW to configure the HW */
typedef struct NCLIB_LRME_REG_METADATA_TAG
{
    uint8_t lrmeIsUseDC4;                /**< whether to use DC4 or DC16 image */
    uint16_t lrmeTarCropWindowCoordLeft; /**< in bytes - cropping: start reading from
                                          *    (lrmeTarCropWindowCoordLeft, lrmeTarCropWindowCoordTop) address in Tar
                                          *    buffer */
    uint16_t lrmeTarCropWindowCoordTop;  /**< in rows */
    uint16_t lrmeRefCropWindowCoordLeft; /**< in bytes - cropping: start reading from (lrmeRefCropWindowCoordLeft,
                                          *   lrmeRefCropWindowCoordTop) address in Ref buffer */
    uint16_t lrmeRefCropWindowCoordTop;  /**< in rows */

} NCLIB_LRME_REG_METADATA;

/** Struct that describes MF SW context */
typedef struct NCLIB_CONTEXT_MULTI_FRAME_TAG
{
    uint32_t placeHolder;              /**< place holder. not used right now. */
} NCLIB_CONTEXT_MULTI_FRAME;

/** Struct that describes EIS SW context */
typedef struct NCLIB_CONTEXT_EIS_TAG
{
    uint32_t placeHolder;              /**< place holder. not used right now. */
} NCLIB_CONTEXT_EIS;


#ifdef FW_LMC_STRUCT_MISSING

typedef struct _LmcPassParameters
{
    //TF LM parameters
    //One of the parameters that helps calculate the irregularity threshold
    //5 bits
    uint32_t lmIrregMantissa;

    //One of the parameters that helps calculate the irregularity threshold
    //5 bits
    uint32_t lmIrregExp;

    //One of the parameters that helps calculate confidence threshold
    //5 bits
    uint32_t lmConfMantisaa;

    //One of the parameters that helps calculate confidence threshold
    //5 bits
    uint32_t lmConfExp;

    //One of the parameters that helps calculate SAD threshold in the im_sad_0 check
    //8 bits
    uint32_t lmSadC2;

    //One of the parameters that helps calculate SAD threshold in the im_sad_0 check
    //8 bits
    uint32_t lmSadC1;

    //One of the parameters that helps calculate SAD threshold in the im_sad_0 check
    //8 bits
    uint32_t lmDefaultSadC2;

    //One of the parameters that helps calculate SAD threshold in the im_sad_0 check
    //8 bits
    uint32_t lmDefaultSadC1;

    //Maximum value for motion vector in y direction
    //6 bits
    uint32_t lmMaxY;

    //Maximum value for motion vector in x direction
    //6 bits
    uint32_t lmMaxX;

    //One of the parameters that helps calculate the smoothness factor
    //5 bits
    uint32_t lmSmoothnessA;

    //One of the parameters that helps calculate the smoothness factor
    //5 bits
    uint32_t lmSmoothnessM;

    //ICA parameter for LM
    //Motion vector difference threshold for LMC in ICA
    //5 bits
    uint32_t ctc_lmc_mv_diff_thr;
} LmcPassParameters;

typedef struct _LmcParameters
{
    uint32_t enableLMC;
    LmcPassParameters    parameters[PASS_NAME_MAX];
} LmcParameters;

#endif // FW_LMC_STRUCT_MISSING


#ifdef __cplusplus
}
#endif


#endif //__NCLIB_CONTEXT_H__

