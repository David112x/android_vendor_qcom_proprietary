//-------------------------------------------------------------------------
// Copyright (c) 2015-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// @file  ipe_data.h
// @brief IPE (Image Processing Engine) data types
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// @Edit History
// when              who           what,where,why
// -----             ---           -------------
//
// 10/29/2015        opolonsk      Initial version
//------------------------------------------------------------------------
#ifndef _IPE_DATA_H_
#define _IPE_DATA_H_

#include "ipdefs.h"
#include "fwdefs.h"

// \deprecated will be removed in future versions
#define fullpassParameters  parameters[PASS_NAME_FULL]

// \deprecated will be removed in future versions
#define dc4Parameters       parameters[PASS_NAME_DC_4]

// \deprecated will be removed in future versions
#define dc16Parameters      parameters[PASS_NAME_DC_16]

// \deprecated will be removed in future versions
#define dc64Parameters      parameters[PASS_NAME_DC_64]

// \deprecated will be removed in future versions
#define dc4     dc[PASS_NAME_DC_4 -PASS_NAME_DC_BASE]

// \deprecated will be removed in future versions
#define dc16    dc[PASS_NAME_DC_16 -PASS_NAME_DC_BASE]

// \deprecated will be removed in future versions
#define dc64    dc[PASS_NAME_DC_64 -PASS_NAME_DC_BASE]

//------------------------------------------------------------------------
// Type Declarations
//------------------------------------------------------------------------
#pragma pack(push)
#pragma pack(1)

typedef enum _IPE_IO_IMAGES
{
    IPE_INPUT_IMAGE_FULL,
    IPE_INPUT_IMAGE_DS4,
    IPE_INPUT_IMAGE_DS16,
    IPE_INPUT_IMAGE_DS64,

    IPE_INPUT_IMAGE_FULL_REF,
    IPE_INPUT_IMAGE_DS4_REF,
    IPE_INPUT_IMAGE_DS16_REF,
    IPE_INPUT_IMAGE_DS64_REF,

    IPE_OUTPUT_IMAGE_DISPLAY,
    IPE_OUTPUT_IMAGE_VIDEO,

    IPE_OUTPUT_IMAGE_FULL_REF,
    IPE_OUTPUT_IMAGE_DS4_REF,
    IPE_OUTPUT_IMAGE_DS16_REF,
    IPE_OUTPUT_IMAGE_DS64_REF,

    // Moving to bottom for backward compatibility
    IPE_INPUT2_IMAGE_FULL,
    IPE_INPUT2_IMAGE_DSX,

    IPE_INPUT_OUTPUT_SCRATCHBUFFER,
    IPE_OUTPUT_IMAGE_HDR10_STATS,
    /// utility definitions

    IPE_INPUT_IMAGE_FIRST = IPE_INPUT_IMAGE_FULL,
    IPE_INPUT_IMAGE_LAST = IPE_INPUT_IMAGE_DS64_REF,

    IPE_INPUT_IMAGE_REF_FIRST = IPE_INPUT_IMAGE_FULL_REF,
    IPE_INPUT_IMAGE_REF_LAST = IPE_INPUT_IMAGE_DS64_REF,

    IPE_OUTPUT_IMAGE_FIRST = IPE_OUTPUT_IMAGE_DISPLAY,
    IPE_OUTPUT_IMAGE_LAST = IPE_OUTPUT_IMAGE_DS64_REF,

    IPE_OUTPUT_IMAGE_REF_FIRST = IPE_OUTPUT_IMAGE_FULL_REF,
    IPE_OUTPUT_IMAGE_REF_LAST = IPE_OUTPUT_IMAGE_DS64_REF,

    IPE_INPUT2_IMAGE_FIRST = IPE_INPUT2_IMAGE_FULL,
    IPE_INPUT2_IMAGE_LAST = IPE_INPUT2_IMAGE_DSX,

    IPE_INPUT_OUTPUT_IMAGE_LAST = IPE_INPUT_OUTPUT_SCRATCHBUFFER,

    IPE_IO_IMAGES_MAX // Must be the last member

} IPE_IO_IMAGES, IpeIoImage;

typedef enum PassName
{
    PASS_NAME_FULL  = 0,    // FULL
    PASS_NAME_DC_4  = 1,    // ANR/TF 1:4
    PASS_NAME_DC_16 = 2,    // ANR/TF 1:16
    PASS_NAME_DC_64 = 3,    // ANR/TF 1:64
    PASS_NAME_DS_FULL = 4,    // Downscale FULL
    PASS_NAME_DS_DC_4 = 5,    // Downscale 1:4
    PASS_NAME_DS_DC_16 = 6,   // Downscale 1:16
    PASS_NAME_MAX_NOISE_PROC = 7,

    PASS_NAME_MAX = PASS_NAME_DC_64 + 1,

    PASS_NAME_DC_BASE = PASS_NAME_DC_4,
    PASS_NAME_DS_BASE = PASS_NAME_DS_FULL,
} PassName;

typedef enum ExposureName
{
    SHORT_EXPOSURE = 0,
    MID_EXPOSURE = 1,
    LONG_EXPOSURE = 2,

    MFHDR_EXPOSURE_MAX,
} ExposureName;

typedef enum ExposureSelect
{
    EXPOSURE_SELECT_INVALID = 0,
    EXPOSURE_SELECT_MAIN = 1,
    EXPOSURE_SELECT_REF = 2,
    EXPOSURE_SELECT_MFHDR = 3,

    EXPOSURE_SELECT_MAX,
} ExposureSelect;

/**
* Function to convert pass input from IFE, to string that can be printed.
*
*
* @param[in]  inputName
*               pass input from IFE name to convert.
* @return C string of the name of the pass input from IFE.
*/
const char* PassInputFromOutsideToString(IPE_IO_IMAGES inputName);

/**
* Function to convert pass name to string that can be printed.
*
*
* @param[in]  passName
*               pass name to convert.
* @return C string of the name of the pass.
*/
const char* IpePassNameToString(PassName passName);

typedef enum _IpeOperationMode
{
    REAL_TIME_STREAM,
    NON_REAL_TIME_STREAM,
    SEMI_REAL_TIME_STREAM,
} IpeOperationMode;

typedef enum _IpeTopologyType
{
    TOPOLOGY_TYPE_DEFAULT = 0,    // invalid, for backwards compatibility
    TOPOLOGY_TYPE_MFSR_PREFILTER,
    TOPOLOGY_TYPE_MFSR_BLEND,
    TOPOLOGY_TYPE_NO_NPS_LTM,
    TOPOLOGY_TYPE_MFHDR,
    TOPOLOGY_TYPE_NPS_ONLY,
    TOPOLOGY_TYPE_MAX            // Must be the last member
} IpeTopologyType;

typedef struct _ScratchIntermediate
{
    uint32_t            offset[PASS_NAME_MAX];
    uint32_t            size[PASS_NAME_MAX];
} ScratchIntermediate;


/**< Two dimensional coordinate or window */
typedef struct _Float2
{
    float                           x;                      /**< can be coordinate or window width   */
    float                           y;                      /**< can be coordinate or window height  */
}Float2;

/**< Fractional ROI or FOV rectangle on 1x1 (0..1) */
typedef struct _WindowROI
{
    Float2                    offset;                 /**< on domain 1x1 */
    Float2                    size;                   /**< on domain 1x1 */
}WindowROI;

typedef struct _IpeConfigIoData
{
    // Image dimensions and layouts
    ImageDescriptor     images[IPE_IO_IMAGES_MAX];

    // Stabilization margins, zero by default.
    ImageDimensions     stabilizationMargins;

    // An indication whether the IPE is working in secure mode
    uint32_t            secureMode;

    // super-resolution still/video (disabled by default)
    uint32_t            icaMaxUpscalingQ16;

    // Used to define maximal supported batch size in current stream
    // should be set to 1 if not in HFR mode
    uint32_t            maxBatchSize;

    // Talos specific register
    // 0: detiler output will connect to ANR ; ICA2 output will connect to TF (default)
    // 1: detiler output will connect to TF  ; ICA2 output will connect to ANR
    uint32_t            muxMode;

    IpeTopologyType     topologyType;
} IpeConfigIoData;

typedef struct _IpeConfigIoResult
{
    uint32_t            scratchMemSize;
    uint32_t            cdmBufferSize;

    ScratchIntermediate scratchBufferPdi;
    ScratchIntermediate scratchBufferTfi;
    ScratchIntermediate scratchBufferTfr;
    ScratchIntermediate scratchBufferLmci;
} IpeConfigIoResult;

// The following block is specified explicitly since the round and clamp
// are coupled with the crop parameters which are striping dependent
typedef struct _IpeChannelClampParameters
{
    uint32_t    clampMin;
    uint32_t    clampMax;
} IpeChannelClampParameters;

// Programmable rounding pattern (2x2 quad)
typedef enum _IpeRoundingPattern
{
    // 0x0: Regular rounding (1111)
    ROUNDING_PATTERN_REGULAR            = 0,
    // 0x1: Checkerboard, odd (1001)
    ROUNDING_PATTERN_CHECKERBOARD_ODD   = 1,
    // 0x2: Checkerboard, even (0110)
    ROUNDING_PATTERN_CHECKERBOARD_EVEN  = 2,
    // 0x3: Truncation (0000)
    ROUNDING_PATTERN_TRANCATION         = 3,
} IpeRoundingPattern;

typedef struct _IpeRoundingParameters
{
    IpeRoundingPattern lumaRoundingPattern;
    IpeRoundingPattern chromaRoundingPattern;
} IpeRoundingParameters;

typedef struct _IpeClampParameters
{
    IpeChannelClampParameters lumaClamp;
    IpeChannelClampParameters chromaClamp;
} IpeClampParameters;

typedef struct _IpeDS4PassParameters
{
    uint32_t                coefficient7;
    uint32_t                coefficient16;
    uint32_t                coefficient25;
    IpeRoundingParameters   roundingParams;
} IpeDS4PassParameters;

typedef struct _IpeDS4Parameters
{
    IpeDS4PassParameters parameters[PASS_NAME_MAX];
} IpeDS4Parameters;

typedef struct _IpeDsxModuleParameters
{
    uint32_t mode;
    uint32_t paddingWeightsEn;
    uint32_t scaleRatioX;
    uint32_t scaleRatioY;
    uint32_t startLocationX;
    uint32_t startLocationY;
    uint32_t outputWidthX;
    uint32_t outputWidthY;
} IpeDsxModuleParameters;

typedef struct _IpeDsxPassParameters
{
    IpeDsxModuleParameters dsxLuma;
    IpeDsxModuleParameters dsxChroma;
} IpeDsxPassParameters;

typedef struct _IpeDsxParameters
{
    IpeDsxPassParameters parameters[PASS_NAME_MAX];
} IpeDsxParameters;

typedef struct _ScalerParameters
{
    uint32_t horizontalRoundingOption;
    uint32_t verticalRoundingOption;
} ScalerParameters;

typedef enum _YInterpolationMode
{
    // 0: 4x4 kernel
    LUT_INTERPOLATION_TYPE = 0,
    // Bi-linear (2x2)
    BILINEAR_INTERPOLATION_TYPE = 1
} YInterpolationMode;

typedef struct _WindowRegion IpeZoomWindow;

typedef enum _GridTransformGeometry
{
    // 0x0: 35x27 samples (1:256 grid hop)
    GRID_TRANSFORM_GEOMETRY_0  = 0,
    // 0x1: 67x51 samples (1:128 grid hop)
    GRID_TRANSOFMR_GEOMETRY_1  = 1,

} GridTransformGeometry;

typedef struct _IcaParameters
{
    // Output zoom window.
    IpeZoomWindow          zoomWindow;

    // IFE output zoom window.
    IpeZoomWindow          ifeZoomWindow;

    YInterpolationMode yInterpolationMode;

    // 0: Out-of-frame pixel is populated with a predefined value.
    // 1: Out-of-frame pixel is populated using duplication.
    // Applicable only for the full pass
    // format: 1u
    uint32_t           invalidPixelModeInterpolationEnabled;

    // Y Output sample values for out-of-input-frame pixels, in case calculate (bit 0) is equal to 0
    // format: 10u
    uint32_t           invalidPixelModeConstY;

    // Cb Output sample values for out-of-input-frame pixels, in case calculate (bit 0) is equal to 0
    // format: 10u
    uint32_t           invalidPixelModeConstCb;

    // Cr Output sample values for out-of-input-frame pixels, in case calculate (bit 0) is equal to 0
    // format: 10u
    uint32_t           invalidPixelModeConstCr;

    // In case of 8 bit formats (as defined by INPUT_FORMAT), this parameter defines whether the 8-bit sample is aligned to the MSB or LSB within the 10-bit output.
    // For Titan PT/HT, this parameter is expected to be set to 1.
    // 0: LSB aligned
    // 1: MSB aligned
    // format: 1u
    uint32_t           eightBitOutputAlignment;

    ///////////////
    // When Y_INTERPOLATION_TYPE = 0, luma samples are interpolated using the coefficients in this LUT.
    // format: 14s
    int16_t            opgInterpolationCoefficients0[16];
    int16_t            opgInterpolationCoefficients1[16];
    int16_t            opgInterpolationCoefficients2[16];

    ///////////////
    // Relevant only in case MODE = 0 (‘Warping On’) and CTC_TRANSFORM.translation_only is equal to 1.
    // Defines the translation that should be induced on the x coordinate by CTC.
    // 32 bits allocated as 1.15.16
    // format: 32s16Q
    int32_t            shiftOnlyXQ16;
    // Defines the translation that should be induced on the y coordinate by CTC.
    // 32 bits allocated as 1.15.16
    // format: 32s16Q
    int32_t            shiftOnlyYQ16;


    // 0: Disabled, 1: Enabled
    // format: 1u
    uint32_t           isGridEnable;

    // 0: Disabled, 1: Enabled
    // format: 1u
    uint32_t           isPerspectiveEnable;

    // The geometry of the perspective transform partitioning is described in terms of MxN, when
    // M means number of columns and N means number of rows.
    // Valid combinations are those in which MxN<=9.
    uint32_t           perspGeomM; // Partition columns
    uint32_t           perspGeomN; // Partition rows

   // The geometry of the Grid transform
   // 0: 35x27,  1: 67x51 samples
    GridTransformGeometry gridTransformGeometry;

    uint32_t forceWarpOn; //icaGeoParameters.ica1[PASS_NAME_FULL].forceWarpOn at first frame in a batch

} IcaParameters;

typedef struct _IcaPassParameters
{
    uint32_t INPUT_FRAME_WIDTH_MINUS_1;         // 14u
    uint32_t INPUT_FRAME_HEIGHT_MINUS_1;        // 14u
    uint32_t OUTPUT_STRIP_HEIGHT_MINUS_1;       // 14u
    uint32_t CONTROLLER_VALID_WIDTH_MINUS_1;    // 19uQ5 as 0.14.5
    uint32_t CONTROLLER_VALID_HEIGHT_MINUS_1;   // 19uQ5 as 0.14.5
    uint32_t CTC_HALF_OUTPUT_FRAME_WIDTH;       // 14u
    uint32_t CTC_HALF_OUTPUT_FRAME_HEIGHT;      // 14u
    uint32_t CTC_O2V_SCALE_FACTOR_X;            // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_O2V_SCALE_FACTOR_Y;            // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_O2V_OFFSET_X;                  // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_O2V_OFFSET_Y;                  // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_V2I_INV_SCALE_FACTOR_X;        // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_V2I_INV_SCALE_FACTOR_Y;        // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_V2I_OFFSET_X;                  // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_V2I_OFFSET_Y;                  // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_INPUT_COORD_PRECISION;         // 3u

    uint32_t outputWidthPixels;
    uint32_t outputHeightPixels;
    uint32_t forceWarpOn;                       // if 1 must set MODE register to WARP_ON (0), CTC cannot be bypassed
} IcaPassParameters;

typedef struct _GeoIcaParameters
{
    IcaPassParameters ica1[PASS_NAME_MAX];
    IcaPassParameters ica2[PASS_NAME_MAX];
}  GeoIcaParameters;


typedef struct _ANR_RNF_BYPASS
{
    uint32_t  RNF_BYPASS__LUMA; /* 0:0 */
    uint32_t  RNF_BYPASS__CHROMA; /* 1:1 */

} ANR_RNF_BYPASS;

typedef struct _RnfParameters
{
    ANR_RNF_BYPASS rnfBypass;
} RnfParameters;

typedef struct _ANR_CYLPF_FILTER2_BYPASS
{
    uint32_t  CYLPF_FILTER2_BYPASS; /* 0:0 */
} ANR_CYLPF_FILTER2_BYPASS;

typedef struct _Filter2Parameters
{
    ANR_CYLPF_FILTER2_BYPASS filter2Bypass;
} Filter2Parameters;

typedef struct _ANR_CYLPF_BYPASS
{
    uint32_t  CYLPF_BYPASS; /* 0:0 */
} ANR_CYLPF_BYPASS;

typedef struct _CylpfParameters
{
    ANR_CYLPF_BYPASS cylpfBypass;

    // X coordinate of the first pixel in the strip relative to the optical center. (1,8,15)
    // format: 24s
    uint32_t lnrXStart;

    // The normalized relative increment in the X axis when moving two pixel to the left.
    // Can be negative when using straight lines. (1,1,15)
    // format: 17sQ15
    uint32_t lnrXRelInc;

    // Normalized Y coordinate of the first pixel relative to the optical center. (1,8,15)
    // format: 24s
    uint32_t lnrYStart;

    // Normalized cross element value of the ellipse in the first pixel relative to the optical center (1,8,15)
    // format: 24s
    uint32_t lnrYExtStart;

    // The normalized relative increment in the Y axis when moving one pixel downwards.
    // Can be negative when using straight lines. (1,1,15)
    // format: 17sQ15
    uint32_t lnrYRelInc;

    // The normalized relative increment in the ellipse cross element when moving one pixel downwards (1,1,15)
    // format: 17sQ15
    uint32_t lnrYExtRelInc;

    // This register describes the closest power of 2, which result in XrelInc
    // or YrelInc that are smaller than 1 (in absolute value).
    // format: 3u
    // Todo: SW should provide this value in CDMPayload.
    uint32_t lnrRadiusRange;

    // Determines the distance function for the radius dependent blend factor calculation.
    // 0: ELLIPSE; 1 is deprecated
    // format: 1u
    // Todo: SW should provide this value in CDMPayload.
    uint32_t lnrRadialShape;

    uint32_t ditheringSeedFrame;
} CylpfParameters;

typedef struct _ANR_MODULE_CFG
{
    uint32_t  EN; // 0-disabled, 1-enabled
} ANR_MODULE_CFG;

typedef struct _AnrPassParameters
{
    ANR_MODULE_CFG       moduleCfg;
    RnfParameters        rnfParameters;
    Filter2Parameters    filter2Parameters;
    CylpfParameters      cylpfParameters;
} AnrPassParameters;

typedef struct _AnrParameters // Advanced noise reduction
{
    AnrPassParameters   parameters[PASS_NAME_MAX];
} AnrParameters;

typedef enum _TfBlendMode
{
    // regular blending
    TF_BLEND_MODE_REGULAR = 0,

    // copy IMG to TFO
    TF_BLEND_MODE_CUR_IMG_PASSTHRU = 1,

    // copy ANRO to TFO
    TF_BLEND_MODE_ANR_IMG_PASSTHRU = 2,

    // copy MCTFP to TFO
    TF_BLEND_MODE_PREV_IMG_PASSTHRU = 3,

    // use blending with constant factor between ANRO and
    // MCTFP (this mode can not be used when invalidMctfpImage is '1'
    TF_BLEND_MODE_CONSTANT_BLENDING = 4,
} TfBlendMode;

typedef enum _TfIndMorphology
{
    TF_IND_MORPHOLOGY_DISABLED  = 0, // default value
    TF_IND_MORPHOLOGY_5x5       = 1,
    TF_IND_MORPHOLOGY_3x3       = 2,
    TF_IND_MORPHOLOGY_NONE      = 3, // input pass-through (no filter)
} TfIndMorphology;

// Window size for SAD Y calculation
typedef enum _TfSadYCalcMode
{
    TF_SAD_Y_CALC_MODE_7x5  = 0, // default value
    TF_SAD_Y_CALC_MODE_5x5  = 1,
    TF_SAD_Y_CALC_MODE_3x3  = 2,
    TF_SAD_Y_CALC_MODE_1x1  = 3,
} TfSadYCalcMode;

typedef struct _TF_MODULE_CFG
{
    uint32_t  EN; // 0-disabled, 1-enabled
} TF_MODULE_CFG;

typedef struct _TfPassParameters
{
    TF_MODULE_CFG moduleCfg;

    uint32_t invalidMctfpImage; // 1u
    uint32_t disableOutputIndications; // 1u
    uint32_t disableUseIndications; // 1u

    // Whether ghost detection takes Luma into account.
    // When both disableLumaGhostDetection and disableChromaGhostDetection are ‘1’,
    // ghost detection should be bypassed (setting FS and a3 either
    // to indication values or to zeros – depending on indicationsDominateFsDecision flag).
    // format: 1u
    uint32_t disableLumaGhostDetection;

    // Whether ghost detection takes Chroma into account.
    // When both disableLumaGhostDetection and disableChromaGhostDetection are ‘1’,
    // ghost detection should be bypassed (setting FS and a3 either
    // to indication values or to zeros – depending on indicationsDominateFsDecision flag).
    // format: 1u
    uint32_t disableChromaGhostDetection;

    uint32_t smearInputsForDecisions; // 1u
    uint32_t useAnrForDecisions_Y; // 1u
    uint32_t useAnrForDecisions_C; // 1u
    uint32_t enableLNR; // 1u
    uint32_t enableNoiseEstByLuma; // 1u
    uint32_t enableNoiseEstByChroma; // 1u
    uint32_t paddingByReflection; // 1u
    TfSadYCalcMode sadYCalcMode;
    uint32_t isSadC5x3; // 1u
    uint32_t lnrLutShiftY; // 1u
    uint32_t lnrLutShiftC; // 1u
    uint32_t isDCI; // 1u
    uint32_t indicationsDominateFsDecision; // 1u
    uint32_t applyFsRankFilter; // 1u
    uint32_t applyFsLpf; // 1u
    uint32_t takeOofIndFrom; // 1u
    uint32_t isSameBlendingForAllFrequencies; // 1u
    uint32_t fsDecisionFreeParamShiftY; // 2u
    uint32_t fsDecisionFreeParamShiftC; // 2u
    uint32_t outOfFramePixelsConfidence; //4u

    // Note that all modes except TF_BLEND_MODE_REGULAR are trivial blending modes
    // (namely, pixel-wise constant blending or passthru of one of inputs). When
    // in these modes the blender does not need any information from other sub-blocks
    // of TF effectively allowing detached operation of Ghost Detection and Blending.
    // Note also that when blendingMode != 0 and output indication are disabled, all
    // TF blocks except blender (and morphology) can be switched off.
    TfBlendMode blendingMode;

    // if enableIndicationsDecreaseFactor is set to 0, indicationsDecreaseFactor
    // is disabled (set to 16), otherwise, inidcationDecreaseFactor is used
    // Obsolete item. FW is not using it. NCLib takes care of it.
    uint32_t enableIndicationsDecreaseFactor;

    // Multiplicative factor for decreasing the indication values (valid factor
    // range is [0-16]).
    // Value of 16 means no change to indications.
    // This feature may be useful for example when Global Motion
    // Compensation model was inaccurate.
    // format: 5u
    uint32_t indicationsDecreaseFactor;

    // Whether to take “1-a” instead of “a” for a1 and a4 blending weights.
    // The actual implementation is done more efficiently (using muxes), but the logical
    // meaning is as above – namely: take only MCTFP input as output (instead of spatial
    // blend result in regular mode) when FS is low and perform temporal blending when FS is high.
    // format: 1u
    uint32_t invertTemporalBlendingWeights;

    // Whether to apply dithering to Y channel of final TF output
    // format: 1u
    uint32_t ditherY;

    // Whether to apply dithering to Cb channel of final TF output
    // format: 1u
    uint32_t ditherCb;

    // Whether to apply dithering to Cr channel of final TF output
    // format: 1u
    uint32_t ditherCr;

    // Whether to override IMG input from ANRO - used for TF-Blend feature in Still use case
    // format: 1u
    uint32_t useAnroAsImgInput;

    // Used for TF-Blend feature in Still use case.
    // Whether to override MCTFP input from IMG. The “pixel valid” indications from ICA
    // are also routed together with the data, so when useImgAsMctfpInput is ‘1’ the
    // ICA1 “pixel valid” indications are routed to MCTFP.
    // Note also that IMG can be routed to MCTFP even when MCTFP input does not get
    // data from ICA2 (i.e. when only ANRO and IMG data are available) – in this
    // case the unit should behave as if it gets data on MCTFP as usual.
    // format: 1u
    uint32_t useImgAsMctfpInput;

    // TFIE module
    TfIndMorphology morphErode;

    // TFID module
    TfIndMorphology morphDilate;

    // Seed used for pseudo-random number generator - part of dithering mechanism
    // format: 30u
    int32_t  prngSeed;

    // Initialization of horizontal part of distance to optical center
    // format: 21sQ15
    int32_t  lnrStartIdxH;

    // Initialization of vertical part of distance to optical center
    // format: 21sQ15
    int32_t  lnrStartIdxV;

    // Horizontal step between neighbor pixels – this is the increment for LnrStartIdxH.
    // SW should configure this parameter so that LnrStartIdxH + LnrScaleH*frameW can
    // be represented by 21 bit signed.
    // format: 16uQ15
    int32_t  lnrScaleH;

    // Vertical step between neighbor rows – this is the increment for LnrStartIdxV.
    // SW should configure this parameter so that LnrStartIdxV + LnrScaleV*frameH
    // can be represented by 21 bit signed.
    // format: 16uQ15
    int32_t  lnrScaleV;

    // Whether to calculate and apply transform confidence (based on coverage
    // of the frame by LRME-calculated valid MVs).
    // Frames with higher coverage get higher confidence.
    // The transform confidence is only relevant in full pass
    // format: 1u
    uint32_t enableTransformConfidence;

    // Defines mapping function from calculated transform confidence to actually
    // used transform confidence. The calculated confidence is in the range 0:256 (8 bit fraction).
    // format: 9uQ8
    int32_t  transformConfidenceVal;

    // Sparse mapping between FS values and a1 blending factor for Luma
    // format: 8u
    int32_t  fsToA1MapY[9];

    // Sparse mapping between FS values and a1 blending factor for Chroma
    // format: 8u
    int32_t  fsToA1MapC[9];

    // Sparse mapping between FS values and a4 blending factor for Luma
    // format: 8u
    int32_t  fsToA4MapY[9];

    // Sparse mapping between FS values and a4 blending factor for Chroma
    // format: 8u
    int32_t  fsToA4MapC[9];

    // Lower limit for a2 blending factor calculation in Luma (this value may never be reached – depending on a2SlopeY)
    // format: 8u
    int32_t  a2MinY;

    // Lower limit for a2 blending factor calculation in Chroma (this value may never be reached – depending on a2SlopeC)
    // format: 8u
    int32_t  a2MinC;

    // Upper limit for a2 blending factor calculation (used where FS=0) in Luma
    // format: 8u
    int32_t  a2MaxY;

    // Slope as a function of FS (a2 starts from a2MaxC at FS=0 and goes down with slope a2SlopeC as FS increases). Used for Chroma. [8uQ5]
    // format: 8uQ5
    int32_t  a2MaxC;

    // Slope as a function of FS (a2 starts from a2MaxY at FS=0 and goes down with slope a2SlopeY as FS increases). Used for Luma. [8uQ5]
    // format: 8uQ5
    int32_t  a2SlopeY;

    // Slope as a function of FS (a2 starts from a2MaxC at FS=0 and goes down with slope a2SlopeC as FS increases). Used for Chroma. [8uQ5]
    // format: 8uQ5
    int32_t  a2SlopeC;
} TfPassParameters;

typedef struct _TfParameters
{
    TfPassParameters    parameters[PASS_NAME_MAX];
} TfParameters;

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

typedef struct _TF_REF_CFG
{
    uint32_t  TRENABLE; /* 0:0 */
    uint32_t  TRFSTHRESHOLD; /* 7:2 */
    uint32_t  TRDEADZONE; /* 12:11 */
} TF_REF_CFG;

typedef struct _RefinementPassParameters
{
    TF_REF_CFG refinementCfg;

    // X coordinate of first block
    // format: in pixels
    uint32_t offsetX;

    // X number of blocks
    uint32_t blockNumX;

    // Width of blocks (>=59)
    // format: 9u
    uint32_t blockSizeX;

    // Y coordinate of first block
    // format: 14u
    uint32_t offsetY;

    // Y number of blocks (1…31)
    // format: 5u
    uint32_t blockNumY;

    // Height of blocks (>=32; must be even).
    // format: 9u
    uint32_t blockSizeY;

} RefinementPassParameters;

typedef struct _RefinementParameters
{
    RefinementPassParameters dc[PASS_NAME_MAX - 1];
    uint32_t                 icaNum; // 0 (For MFNR) or 1 (For Video).
} RefinementParameters;

typedef struct _CAC_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */

} CAC_MODULE_CFG;

typedef struct _CacParameters
{
    CAC_MODULE_CFG moduleCfg;
    uint32_t cac2Enable;
    uint32_t cacSnrEnable;
} CacParameters;

typedef struct _ChromaUpScalerParameters
{
    uint32_t chromaUpCosited;
    uint32_t chromaUpEven;
} ChromaUpScalerParameters;

typedef struct _IPE_COLOR_XFORM_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
} IPE_COLOR_XFORM_MODULE_CFG;

typedef struct _ColorTransformParameters
{
    IPE_COLOR_XFORM_MODULE_CFG moduleCfg;
} ColorTransformParameters;

typedef struct _LTM_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
    uint32_t  RGAMMA_EN; /* 8:8 */
    uint32_t  LA_EN; /* 11:11 */
    uint32_t  MASK_EN; /* 13:13 */
} LTM_MODULE_CFG;

typedef struct _LtmParameters // Local tone mapping
{
    LTM_MODULE_CFG moduleCfg;

    uint32_t dcBinInitCnt;

    uint32_t rgb2yC1;
    uint32_t rgb2yC2;
    uint32_t rgb2yC3;
    uint32_t rgb2yC4;

    uint32_t ipLceThd;
    uint32_t ipYRatioMax;

    uint32_t maskD0;
    uint32_t maskD1;
    uint32_t maskD2;
    uint32_t maskD3;
    uint32_t maskD4;
    uint32_t maskD5;

    uint32_t maskShift;
    uint32_t maskScale;

    uint32_t downscaleMNHorTermination;
    uint32_t downscaleMNVerTermination;

    uint32_t dataCollectionEnabled;

    uint32_t ltmImgProcessEn;

} LtmParameters;

typedef struct _LTM_V16_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
    uint32_t  GAMMA_EN; /* 8:8 */
    uint32_t  RGAMMA_EN; /* 9:9 */
    uint32_t  LA_EN; /* 11:11 */
    uint32_t  MASK_EN; /* 13:13 */
    uint32_t  NONLINEAR_IN; /* 20:20 */
    uint32_t  TABLE_SEL; /* 23:21 */
} LTM_V16_MODULE_CFG;

typedef struct _LtmV16Parameters // Local tone mapping
{
    LTM_V16_MODULE_CFG moduleCfg;

    uint32_t dcBinInitCnt;

    uint32_t rgb2yC1;
    uint32_t rgb2yC2;
    uint32_t rgb2yC3;
    uint32_t rgb2yC4;

    uint32_t ipLceThd;
    uint32_t ipYRatioMax;

    uint32_t maskD0;
    uint32_t maskD1;
    uint32_t maskD2;
    uint32_t maskD3;
    uint32_t maskD4;
    uint32_t maskD5;

    uint32_t maskShift;
    uint32_t maskScale;

    uint32_t downscaleMNHorTermination;
    uint32_t downscaleMNVerTermination;

    uint32_t dataCollectionEnabled;

    uint32_t ltmImgProcessEn;

} LtmV16Parameters;

typedef struct _IPE_COLOR_CORRECT_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
} IPE_COLOR_CORRECT_MODULE_CFG;

typedef struct _IpeColorCorrectParameters
{
    IPE_COLOR_CORRECT_MODULE_CFG moduleCfg;
} IpeColorCorrectParameters;

typedef struct _IPE_GLUT_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */

} IPE_GLUT_MODULE_CFG;

typedef struct _IpeGlutParameters
{
    IPE_GLUT_MODULE_CFG moduleCfg;
} IpeGlutParameters;

typedef struct _LUT_2D_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */

} LUT_2D_MODULE_CFG;

typedef struct _Lut2dParameters
{
    LUT_2D_MODULE_CFG moduleCfg;
} Lut2dParameters;

typedef struct _HDR10_PLUS_MODULE_CFG
{
    uint32_t EN; /* 0:0 */
} HDR10_PLUS_MODULE_CFG;

typedef struct _Hdr10PlusParameters
{
    HDR10_PLUS_MODULE_CFG moduleCfg;
    uint32_t rgnHOffset;
    uint32_t rgnVOffset;
    uint32_t rgnHNum;
    uint32_t rgnVNum;
} Hdr10PlusParameters;

typedef struct _CHROMA_ENHAN_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */

} CHROMA_ENHAN_MODULE_CFG;

typedef struct _ChromaEnhancementParameters
{
    CHROMA_ENHAN_MODULE_CFG moduleCfg;
} ChromaEnhancementParameters;

typedef struct _CHROMA_SUP_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */

} CHROMA_SUP_MODULE_CFG;

typedef struct _ChromaSupressionParameters
{
    CHROMA_SUP_MODULE_CFG moduleCfg;
} ChromaSupressionParameters;

typedef struct _SKIN_ENHAN_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
} SKIN_ENHAN_MODULE_CFG;

typedef struct _SkinEnhancementParameters
{
    SKIN_ENHAN_MODULE_CFG moduleCfg;
} SkinEnhancementParameters;

typedef struct _ASF_MODULE_CFG
{
    uint32_t  EN;             /* 0:0 */
    uint32_t  SP_EFF_EN;      /* 8:8 */
    uint32_t  SP_EFF_ABS_EN;  /* 9:9 */
    uint32_t  LAYER_1_EN;     /* 10:10 */
    uint32_t  LAYER_2_EN;     /* 11:11 */
    uint32_t  CONTRAST_EN;    /* 12:12 */
    uint32_t  CHROMA_GRAD_EN; /* 13:13 */
    uint32_t  EDGE_ALIGN_EN;  /* 14:14 */
    uint32_t  SKIN_EN;        /* 15:15 */
    uint32_t  RNR_ENABLE;     /* 16:16 */
    uint32_t  NEG_ABS_Y1;     /* 17:17 */
    uint32_t  SP;             /* 28:24 */
} ASF_MODULE_CFG;

typedef struct _AsfParameters
{
    ASF_MODULE_CFG moduleCfg;
    //Obsolete item, no need to provide value from SW, striping takes care of it.
    uint32_t faceVerticalOffset;
} AsfParameters;

typedef struct _ASF_MODULE_CFG_680
{
    uint32_t  EN;             /* 0:0 */
    uint32_t  SP_EFF_EN;      /* 8:8 */
    uint32_t  SP_EFF_ABS_EN;  /* 9:9 */
    uint32_t  LAYER_1_EN;     /* 10:10 */
    uint32_t  LAYER_2_EN;     /* 11:11 */
    uint32_t  EDGE_ALIGN_EN;  /* 14:14 */
    uint32_t  SKIN_EN;        /* 15:15 */
    uint32_t  RNR_ENABLE;     /* 16:16 */
    uint32_t  NEG_ABS_Y1;     /* 17:17 */
    uint32_t  SP;             /* 28:24 */
    uint32_t  L1_CONTRAST_EN; /* 30:30 */
    uint32_t  L2_CONTRAST_EN; /* 31:31 */

} ASF_MODULE_CFG_680;

typedef struct _AsfParameters_680
{
    ASF_MODULE_CFG_680 moduleCfg;
} AsfParameters_680;

typedef enum _UpScaleCfg
{
    UPSCALE_FILTER_ED           = 0,
    UPSCALE_FILTER_CIRCULAR     = 1,
    UPSCALE_FILTER_SEPARABLE    = 2,
} UpScaleCfg;

typedef enum _UpScaleBlend
{
    UPSCALE_BLEND_CIRCULAR      = 0,
    UPSCALE_BLEND_SEPARABLE     = 1,
} UpScaleBlend;

typedef struct _UpScalerParameters
{
    uint32_t directionalUpscalingEnabled;
    uint32_t sharpeningEnabled;
    uint32_t lumaCfg;   //> UpScaleCfg
    uint32_t chromaCfg; //> UpScaleCfg
    uint32_t bitwidth;
    uint32_t blendCfg;  //> UpScaleBlend
    uint32_t weightGain;
    uint32_t sharpenLevel1;
    uint32_t sharpenLevel2;
    uint32_t sharpenPrecision;
    uint32_t sharpenClip;
    uint32_t shapeThreshQuiet;
    uint32_t shapeThreshDieout;
    uint32_t threshLow;
    uint32_t threshHigh;
    uint32_t adjustDataA0;
    uint32_t adjustDataA1;
    uint32_t adjustDataA2;
    uint32_t adjustDataB0;
    uint32_t adjustDataB1;
    uint32_t adjustDataB2;
    uint32_t adjustDataC0;
    uint32_t adjustDataC1;
    uint32_t adjustDataC2;
} UpScalerParameters;


typedef struct _UpscalerLiteParameters
{
    uint16_t lumaVScaleFirAlgorithm;        //0x0 BI_LINEAR, 0x1 BI_CUBIC, 0x2 NN
    uint16_t lumaHScaleFirAlgorithm;        //0x0 BI_LINEAR, 0x1 BI_CUBIC, 0x2 NN
    uint16_t lumaInputDitheringDisable;
    uint16_t lumaInputDitheringMode;

    uint16_t chromaVScaleFirAlgorithm;      //0x0 BI_LINEAR, 0x1 BI_CUBIC, 0x2 NN
    uint16_t chromaHScaleFirAlgorithm;      //0x0 BI_LINEAR, 0x1 BI_CUBIC, 0x2 NN
    uint16_t chromaInputDitheringDisable;
    uint16_t chromaInputDitheringMode;
    uint16_t chromaRoundingModeV;
    uint16_t chromaRoundingModeH;
} UpscalerLiteParameters;


typedef struct _GRA_MODULE_CFG
{
    uint32_t  EN;           // 0-disabled, 1-enabled
    uint32_t  EN_DITHER_Y;  // 0-disabled, 1-enabled
    uint32_t  EN_DITHER_C;  // 0-disabled, 1-enabled
} GRA_MODULE_CFG;

typedef struct _GraParameters
{
    GRA_MODULE_CFG moduleCfg;
    uint32_t grainStrength;
    uint32_t grainSeed;
    uint32_t mcgA;
    uint32_t skipAheadJump;
} GraParameters;

typedef struct _LENR_MODULE_CFG
{
    uint32_t EN; /* 0:0 */
    uint32_t BLTR_EN; /* 8:8 */
    uint32_t LCE_EN; /* 9:9 */
    uint32_t DN4_BLTR_CLAMP_EN; /* 10:10 */
    uint32_t DN8_BLTR_CLAMP_EN; /* 11:11 */
    uint32_t DN16_BLTR_CLAMP_EN; /* 12:12 */
    uint32_t FD_SNR_EN; /* 13:13 */
    uint32_t SNR_EN; /* 14:14 */
    uint32_t RNR_EN; /* 15:15 */
    uint32_t FNR_EN; /* 16:16 */
    uint32_t LAYER_1_ONLY; /* 17:17 */
    uint32_t POST_CROP_EN; /* 18:18 */
} LENR_MODULE_CFG;

typedef struct _LenrParameters
{
    LENR_MODULE_CFG moduleCfg;
} LenrParameters;

typedef struct _MFHDR_MAC_MODULE_CFG
{
    uint32_t EN; /* 0:0 */
    uint32_t MAC1_MOTION_EN; /* 9:9 */
    uint32_t MAC2_MOTION_EN; /* 10:10 */
    uint32_t MAC1_GRAD_MOTION_EN; /* 11:11 */
    uint32_t MAC2_GRAD_MOTION_EN; /* 12:12 */
    uint32_t MAC1_HIGHLIGHT_BLENDING_EN; /* 13:13 */
    uint32_t MAC2_HIGHLIGHT_BLENDING_EN; /* 14:14 */
    uint32_t MAC1_LOWLIGHT_BLENDING_EN; /* 15:15 */
    uint32_t MAC2_LOWLIGHT_BLENDING_EN; /* 16:16 */
    uint32_t MAC1_HIGHLIGHT_MOTION_EN; /* 17:17 */
    uint32_t MAC2_HIGHLIGHT_MOTION_EN; /* 18:18 */
    uint32_t MAC1_BACKGROUND_REC_EN; /* 19:19 */
    uint32_t MAC2_BACKGROUND_REC_EN; /* 20:20 */
} MFHDR_MAC_MODULE_CFG;

typedef struct _MfhdrMacParameters
{
    MFHDR_MAC_MODULE_CFG moduleCfg;
    ExposureSelect       exposureMap[MFHDR_EXPOSURE_MAX];
    uint32_t             exposureSMRatioLargerthan16;
    uint32_t             exposureMLRatioLargerthan16;
} MfhdrMacParameters;

typedef struct _MfhdrParameters
{
    uint32_t                        numExposures;

    ChromaUpScalerParameters        chromaUpscalerParameters[MFHDR_EXPOSURE_MAX];
    ColorTransformParameters        colorTransformParameters[MFHDR_EXPOSURE_MAX];
    IpeGlutParameters               glutParameters[MFHDR_EXPOSURE_MAX];

    MfhdrMacParameters              macParameters;
    GtmParameters                   gtmParameters;
    LtmV16Parameters                ltmParameters;
    IpeGlutParameters               postMacGlutParameters;
    ColorTransformParameters        postMacColorTransformParameters;
    ScalerParameters                scalerParameters;
    IpeRoundingParameters           roundingParameters;
    IpeClampParameters              clampParameters;
} MfhdrParameters;

typedef struct _IpeIQSettings
{
    IcaParameters                   ica1Parameters;
    IcaParameters                   ica2Parameters;
    AnrParameters                   anrParameters;
    TfParameters                    tfParameters;
    IpeRoundingParameters           tfOutRoundingParameters;
    IpeDS4Parameters                ds4Parameters;
    CacParameters                   cacParameters;
    ChromaUpScalerParameters        chromaUpscalerParameters;
    ColorTransformParameters        colorTransformParameters;
    LtmParameters                   ltmParameters;
    IpeColorCorrectParameters       colorCorrectParameters;
    IpeGlutParameters               glutParameters;
    Lut2dParameters                 lut2dParameters;
    ChromaEnhancementParameters     chromaEnhancementParameters;
    ChromaSupressionParameters      chromaSupressionParameters;
    SkinEnhancementParameters       skinEnhancementParameters;
    AsfParameters                   asfParameters;
    AsfParameters_680               asfParameters_680;
    HnrParameters                   hnrParameters;
    // The following struct should be configured for any chip version that has the V20 upscaler in PPS.
    UpScalerParameters              upscalerParameters;
    // The following struct should be configured for any chip version that has the V12 upscaler in PPS.
    UpscalerLiteParameters          upscalerLiteParameters;
    GraParameters                   graParameters;
    ScalerParameters                videoScalerParameters;
    ScalerParameters                displayScalerParameters;
    IpeRoundingParameters           videoRoundingParameters;
    IpeRoundingParameters           displayRoundingParameters;
    IpeClampParameters              videoClampParameters;
    IpeClampParameters              displayClampParameters;
    RefinementParameters            refinementParameters;
    LenrParameters                  lenrParameters;
    LmcParameters                   lmcParameters;
    IpeDsxParameters                dsxParameters;
    MfhdrParameters                 mfhdrParameters;
    uint32_t                        useGeoLibOutput;
    ImageDimensions                 mainInputSize;          /**< Full size of Main (NPS) input window */
    WindowROI                       tfCropWindow;           /**< TF crop ROI on full TFout, aligned to 8 px */
    WindowROI                       videoOutFOV;            /**< FOV of IPE video output on full TFout */
    WindowROI                       displayOutFOV;          /**< FOV of IPE display output on full TFout */
    LtmV16Parameters                ltmV16Parameters;
    UVGammaParameters               uvGammaParameters;
    float                           minDownscaleRatio;
    Hdr10PlusParameters             hdr10PlusParameters;
} IpeIQSettings;

// In HFR, a batch of multiple frames are processed using the same
// settings (except ICA). Below is the definition of all that are
// required for 1 frame processing: buffers and ICA settings.
typedef struct _FrameSet
{
    FrameBuffers           buffers[IPE_IO_IMAGES_MAX];

    // ICA CDM Program Offsets (offset to cdmProgramArrayBase in IpeFrameProcessData)
    uint32_t               cdmProgramArrayIca1Addr;
    uint32_t               cdmProgramArrayIca2Addr;
    GeoIcaParameters       icaGeoParameters;
} FrameSet;

typedef struct _IpeFrameProcessData
{
    // Image Quality setting. Configured by SW.

    // SMMU-mapped address for output struct of striping library
    SMMU_ADDR              stripingLibOutAddr;

    // SMMU-mapped address pointing to IpeIQSettings
    SMMU_ADDR              iqSettingsAddr;

    // UBWC stats buffer
    SMMU_ADDR              ubwcStatsBufferAddress;
    uint32_t               ubwcStatsBufferSize; // in bytes

    // BL memory buffer for generating CDM commands inside the FW.
    SMMU_ADDR              cdmBufferAddress;
    uint32_t               cdmBufferSize;

    // Maximal requested number of IPE cores to use. Configured by SW.
    uint32_t               maxNumOfCoresToUse;

    // target time (ns) for the frame to complete in hardware
    // 0 value disables dynamic QoS health calculation
    uint32_t               targetCompletionTimeInNs;

    //
    // Pass-through IQ settings left untouched by the FW. These are SMMU-mapped addresses
    // pointing to CDMProgramArray structures. See definition of CDMProgramArray type.
    //
    // Base address where the rest of the CdmProgramArray offset is based off of
    SMMU_ADDR              cdmProgramArrayBase;

    // Pre-LTM array holds all the programs related to the blocks before LTM, including LTM
    // LUTs, but excluding the modules which have explicit configurations in the iqSettings.
    uint32_t               cdmProgramArrayPreLtmAddr;
    // Post-LTM array holds all the modules after LTM module in PPS.
    uint32_t               cdmProgramArrayPostLtmAddr;

    // Anr Cdm Programs
    uint32_t               cdmProgramArrayAnrFullPassAddr;
    uint32_t               cdmProgramArrayAnrDc4Addr;
    uint32_t               cdmProgramArrayAnrDc16Addr;
    uint32_t               cdmProgramArrayAnrDc64Addr;

    // TF CdmProgram
    uint32_t               cdmProgramArrayTfFullPassAddr;
    uint32_t               cdmProgramArrayTfDc4Addr;
    uint32_t               cdmProgramArrayTfDc16Addr;
    uint32_t               cdmProgramArrayTfDc64Addr;

    //DSX CdmPrograms
    uint32_t               cdmProgramArrayDsxDc4Addr;
    uint32_t               cdmProgramArrayDsxDc16Addr;
    uint32_t               cdmProgramArrayDsxDc64Addr;

    //MFHDR CdmPrograms
    uint32_t               cdmProgramArrayMfhdrPreLtmAddr;
    uint32_t               cdmProgramArrayMfhdrPostLtmAddr;

    // unique frame process request ID, used for identifying specific frames to abort
    uint32_t               requestId;

    //MANDATORY Parameter to continue or override the stream priority for a specific frame.
    //Client must provide an OperationMode value different from the Stream priority
    //for an updated priority scheduling, otherwise provide the same value as Stream priority
    IpeOperationMode       frameOpMode;


    // Number of buffer sets (1 for single frame, more for HFR use-case)
    uint32_t               numFrameSetsInBatch;

    // Pointers to the buffers (main and scaled 1/4, 1/16, 1/64 and their respective
    // references). Their sizes and format should already be known. Configured by SW.
    FrameSet               frameSets[MAX_HFR_GROUP];

} IpeFrameProcessData;

typedef struct _IpeFrameProcessResult
{
    // Released scratch memory buffer.
    uint32_t            hangDumpBufferSize;
    SMMU_ADDR           hangDumpBufferAddr;

    ScratchIntermediate scratchBufferPdi;
    ScratchIntermediate scratchBufferTfi;
    ScratchIntermediate scratchBufferTfr;
    ScratchIntermediate scratchBufferLmci;
} IpeFrameProcessResult;

typedef struct _IpeAbortData
{
    uint32_t            numRequestIds;  // Number of request IDs to abort
    uint32_t*           requestIds;     // Array of request IDs to abort
} IpeAbortData;

#pragma pack(pop)

// IPE_CONFIG_IO command
typedef struct _IpeConfigIo
{
    IpeConfigIoData         cmdData;    // Input data
    FWUSERARG               userArg;    // FW-reserved argument
} IpeConfigIo;

// IPE_CONFIG_IO_DONE command response
typedef struct _IpeConfigIoDone
{
    FWSTATUS                rc;         // Async return code
    IpeConfigIoResult       cmdRes;     // Result data
    FWUSERARG               userArg;    // FW-reserved argument
} IpeConfigIoDone;

// IPE_FRAME_PROCESS command
typedef struct _IpeFrameProcess
{
    IpeFrameProcessData     cmdData;    // Input data
    FWUSERARG               userArg;    // FW-reserved argument
} IpeFrameProcess;

// IPE_FRAME_PROCESS_DONE command response
typedef struct _IpeFrameProcessDone
{
    FWSTATUS                rc;         // Async return code
    IpeFrameProcessResult   cmdRes;     // Result data
    FWUSERARG               userArg;    // FW-reserved argument
} IpeFrameProcessDone;

// IPE_ABORT command
typedef struct _IpeAbort
{
    IpeAbortData            cmdData;    // Input data
    FWUSERARG               userArg;    // FW-reserved argument
} IpeAbort;

// IPE_ABORT_DONE command response
typedef struct _IpeAbortDone
{
    FWSTATUS                rc;         // Async return code
    FWUSERARG               userArg;    // FW-reserved argument
} IpeAbortDone;

// IPE_DESTROY command
typedef struct _IpeDestroy
{
    FWUSERARG               userArg;    // FW-reserved argument
} IpeDestroy;

// IPE_DESTROY_DONE command response
typedef struct _IpeDestroyDone
{
    FWSTATUS                rc;         // Async return code
    FWUSERARG               userArg;    // FW-reserved argument
} IpeDestroyDone;

// A handle to IPE instance
typedef struct
{
    uint32_t                tagHandle;  // Opaque 32-bit value
} IpeHandle;

#endif // _IPE_DATA_H_

