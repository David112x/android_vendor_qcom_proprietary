/**=============================================================================

@file
   cvpDme.h

@brief
   API Definitions for LRME based on Descriptor-based Motion Estimation (DME)

Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================**/

//=============================================================================
///@details CVP DME APIs using Computer Vision Processor acceleration
///@ingroup cvp_motion_estimation
//=============================================================================

#ifndef CVP_DME_H
#define CVP_DME_H

#include "cvpTypes.h"
#include "cvpMem.h"
#include "cvpNcc.h"
#define CVP_ICA_MAX_GRID_DATASIZE 945
#define CVP_ICA_MAX_PRSP_PRAMS    128
#define CVP_ECU_GAMMA_MAX_DATASIZE    65
#define CVP_ECU_IGAMMA_MAX_DATASIZE    65
#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
/// @brief
///    Structure for ICA configuration
/// @param sSrcImageInfo
///    Structure for source image information.
/// @param sDstImageInfo
///    Structure for destination image information.
/// @param bCtcPerspectiveEnable
///    Flag to enable perspective transform
/// @param bCtcGridEnable
///    Flag to enable grid transform
/// @param bCtcTranslationOnly
///    Flag to enable translation only
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
typedef struct _cvpDmeConfigIca
{
   cvpImageInfo sSrcImageInfo;
   cvpImageInfo sDstImageInfo;
   bool         bCtcPerspectiveEnable;
   bool         bCtcGridEnable;
   bool         bCtcTranslationOnly;

   INLINE _cvpDmeConfigIca() {}

} cvpDmeConfigIca;

//------------------------------------------------------------------------------
/// @brief
///    Structure for per-Frame ICA configuration
/// @param nGridDataSize
///    Size of valid Grid data in the grid buffer. in words (UInt32).
/// @param nGridData
///    Grid paramter data in the grid buffer.  Currently always 945*2 words.
/// @param nPerspectiveDataSize
///    Size of valid perspective data.   in words (UInt32).
/// @param nPerspectiveData
///    Perspective parameter data in the perspective buffer.
/// @param nCTCHalfOutputFrameWidth
///    Defines half of the output frame height. Used for shifting the output pixel y coordinates
///    from the output domain to the shifted output domain = nDestWidth/2
/// @param nCTCHalfOutputFrameHeight
///    Defines half of the output frame width. Used for shifting the output pixel x coordinates
///    from the output domain to the shifted output domain = nDestHeight/2
/// @param nCTCPerspTransformGeomM
///    Defines the width of the perspective transform partitioning geometry.
///    Relevant only when CTC_TRANSFORM.mode is equal to 0 or 1.
///    The height is defined by CTC_PERSP_TRANSFORM_GEOMETRY_N
///    The geometry of the perspective transform partitioning is described
///    in terms of MxN, when M means number of columns and N means
///    number of rows. Valid combinations are those in which MxN.
/// @param nCTCPerspTransformGeomN
///    Defines the height of the perspective transform partitioning geometry.
///    Relevant only when CTC_TRANSFORM.mode is equal to 0 or 1.
///    The width is defined by CTC_PERSP_TRANSFORM_GEOMETRY_M.
///    The geometry of the perspective transform partitioning is described
///    in terms of MxN, when M means number of columns and N means
///    number of rows. Valid combinations are those in which MxN
/// @param nOpgInterpLUT0
///    OPG LUT0 - This LUT holds the Y interpolation coefficients
/// @param nOpgInterpLUT1
///    OPG LUT1 - This LUT holds the Y interpolation coefficients
/// @param nOpgInterpLUT2
///    OPG LUT2 - This LUT holds the Y interpolation coefficients
/// @param nCtcInputCoordPrecision
///    Defines how the 17 bits allocated for coordinate representation are divided between the integer
///    and the fractional part during CTC inverse transform calculations.
///    "Number of bits allocated for the integer part and the fractional part in the input pixel coordinate.
///    The configuration is based on the input image resolution.
///    0: 1.13.3
///    1: 1.12.4
///    2: 1.11.5
///    3: 1.10.6
///    4: 1.9.7
///    5: 1.8.8
///    6: 1.7.9
///    7: 1.6.10"
/// @param nCtcO2vScaleFactor_x
///    Defines the scale factor used to convert the x output coordinates from the output
///    resolution domain to the virtual resolution domain.
///    Scale factor of the x coordinate in relation to virtual resolution.">
///    Bits 0-15 - Mantissa
///    Bits 16-21 - Exponent
/// @param nCtcO2vScaleFactor_y
///    Defines the scale factor used to convert the y output coordinates from the output
///    resolution domain to the virtual resolution domain.
///    Scale factor of the y coordinate in relation to virtual resolution."
///    Bits 0-15 - Mantissa
///    Bits 16-21 - Exponent
/// @param nCtcV2iInvScaleFactor_x
///    Defines the inverse scale factor used to convert the x coordinate result
///    of the inverse transform calculation from the virtual resolution domain to the input resolution domain.
///    Inverse scale factor of the x coordinates in relation to input resolution"
///    Bits 0-15 - Mantissa
///    Bits 16-21 - Exponent
/// @param nCtcV2iInvScaleFactor_y
///    Defines the inverse scale factor used to convert the y coordinate result
///    of the inverse transform calculation from the virtual resolution domain to the input resolution domain.
///    Inverse scale factor of the y coordinates in relation to input resolution"
///    Bits 0-15 - Mantissa
///    Bits 16-21 - Exponent
/// @param nControllerValidWidthMinus1
///    Defines the width of the valid region minus 1 inside the input frame.
///    The valid region starts at x=0. Coordinates outside of the valid region will be marked invalid
/// @param nControllerValidHeightMinus1
///    Defines the height of the valid region minus 1 inside the input frame.
///    The valid region starts at y=0. Coordinates outside of the valid region will be marked invalid
/// @param nCtcO2vOffset_x
///    Defines the x offset used to convert the output coordinates from the output
///    resolution domain to the virtual resolution domain.
///    Offset of the x coordinate in relation to virtual resolution"
///    Bits 0-15 - Mantissa
///    Bits 16-21 - Exponent
///    Default is 0 for firmware
/// @param nCtcO2vOffset_y
///    Defines the y offset used to convert the output coordinates from the output
///    resolution domain to the virtual resolution domain.
///    Offset of the y coordinate in relation to virtual resolution"
///    Bits 0-15 - Mantissa
///    Bits 16-21 - Exponent
///    Default is 0 for firmware
/// @param nCtcV2iOffset_x
///    Defines the offset used to convert the x coordinate result of the inverse transform
///    calculation from the virtual resolution domain to the input resolution domain.
///    Offset of the x coordinate in relation to input resolution."
///    Bits 0-15 - Mantissa
///    Bits 16-21 - Exponent
///    Default is 0 for firmware
/// @param nCtcV2iOffset_y
///    Defines the offset used to convert the y coordinate result of the inverse transform
///    calculation from the virtual resolution domain to the input resolution domain.
///    Offset of the y coordinate in relation to input resolution."
///    Bits 0-15 - Mantissa
///    Bits 16-21 - Exponent
///    Default is 0 for firmware
/// @param nCtcXTranslation
///    Relevant only in case MODE = 0 (Warping On) and CTC_TRANSFORM.translation_only is equal to 1.
///    Defines the translation that should be induced on the x coordinate by CTC."
///    Firmware defaults this value to 0
/// @param nCtcYTranslation
///    Relevant only in case MODE = 0 (Warping On) and CTC_TRANSFORM.translation_only is equal to 1.
///    Defines the translation that should be induced on the y coordinate by CTC."
///    Firmware defaults this value to 0
/// @ingroup cvp_image_transform
//------------------------------------------------------------------------------
typedef struct _cvpDmeFrameConfigIca
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

   INLINE _cvpDmeFrameConfigIca() {}
} cvpDmeFrameConfigIca;

//------------------------------------------------------------------------------
/// @brief
///    Structure for per-Frame Exposure Compensation Unit (ECU) configuration
/// @param bICAEnable
///    Enable the ICA. ECU only is a valid use case as well.
///    0 - Disable  1-Enable. Default set to 0.
/// @param bECUEnable
///    Enable the ECU module
///    0 - Disable  1-Enable. Default set to 0.
/// @param nECUGain
///    Gain value for ECU. Default: 1024
/// @param bECUGammaEnable
///    Enable Gamma correction.
///    0 - Disable  1-Enable. Default is 0
/// @param bECUIGammaEnable
///    Enable Inverse Gamma correction.
///    0 - Disable  1-Enable. Default is 0
/// @param nECULUTBankSelect
///    Select the corresponding LUT for ECU
///    0 or 1 (Default is 0)
/// @param nOpgInterpLUT2
///    OPG LUT2 - This LUT holds the Y interpolation coefficients
/// @param nECUGammaDataSize
///    ECU Gamma parameters size.
/// @param nECUGammaData
///    ECU Gamma parameters.  Currently max is 64 uint32_ts.
/// @param nECUInverseGammaDataSize
///    ECU Inverse Gamma parameters size.
/// @param sECUInverseGammaData
///    ECU Inverse Gamma parameters.  Currently max is 64 uint32_ts.
//------------------------------------------------------------------------------
typedef struct _cvpDmeFrameConfigEcu
{
   uint32_t     bICAEnable;
   uint32_t     bECUEnable;
   uint32_t     nECUGain;
   uint32_t     bECUGammaEnable;
   uint32_t     bECUIGammaEnable;
   uint32_t     nECULUTBankSelect;
   uint32_t     nECUGammaDataSize;
   uint32_t     nECUGammaData[CVP_ECU_GAMMA_MAX_DATASIZE];
   uint32_t     nECUInverseGammaDataSize;
   uint32_t     nECUInverseGammaData[CVP_ECU_IGAMMA_MAX_DATASIZE];

   INLINE _cvpDmeFrameConfigEcu() : bICAEnable(0), bECUEnable(0), nECUGain(1024), bECUGammaEnable(0), bECUIGammaEnable(0), nECULUTBankSelect(0) {}
} cvpDmeFrameConfigEcu;

//------------------------------------------------------------------------------
/// @brief
///    Structure for HCD configuration
///    All members will have default values.
/// @param nNumHorizZones
///    Total number of zones in Horizontal Direction
///    Range: 1 to 16. In units of 8x8 pixels. Default: 8
/// @param nNumVertZones
///    Total number of zones in Vertical Direction
///    Range: 1 to 16. In units of 8x8 pixels. Default: 8
/// @param nZoneWidth
///    valid if nMode below is 0x2. In units of 8x8 pixels (TODO: Need default value)
/// @param nZoneHeight
///    valid if nMode below is 0x2. In units of 8x8 pixels, should be even (TODO: Need default value)
/// @param nCmShift
///    cam_shift (TODO: Need default value)
/// @param nMode
///    Harris corner detector writeout modes
///    0x0: writeout the peak corner measure every 8x8 (Default)
///    0x1: writeout corner measure for every pixel(16-bits)
///    0x2: writeout corner measures for each of the zones specified
/// @param nNMSTap
///    Harris window size
///    0: 3x3 nms; 1: 5x5 nms (default)
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpDmeConfigHcd
{
   uint32_t     nNumHorizZones;
   uint32_t     nNumVertZones;
   uint32_t     nZoneWidth;
   uint32_t     nZoneHeight;
   uint32_t     nCmShift;
   uint32_t     nMode;
   uint32_t     nNMSTap;

   INLINE _cvpDmeConfigHcd() : nNumHorizZones(8), nNumVertZones(8), nZoneWidth(0), nZoneHeight(0), nCmShift(0), nMode(0), nNMSTap(1) { }

} cvpDmeConfigHcd;

//------------------------------------------------------------------------------
/// @brief
///    Structure for NCC configuration
///    All members will have default values.
/// @param eScoreType
///    NCC output score type.
///     0: best result, 1: All NCC costs
/// @param nSearchWindowWidth
///    Width of search window in pixels, Default set to 18
/// @param nSearchWindowHeight
///    Height of search window in pixels, Default set to 18
/// @param nTemplateWidth
///    Width of template in pixels, Default set to 8
/// @param nTemplateHeight
///    Height of template in pixels, Default set to 8
/// @param nNCCMode
///    NCC Mode: 0:Reserved, 1:Reserved, 2:Default
/// @param nNCCThreshold
///    NCC Threshold, If NCC exceeds this threshold the block will be
///    marked invalid. Range:TBD, Default:800
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpDmeConfigNcc
{
    cvpNccScoreType eScoreType;
    uint32_t        nSearchWindowWidth;
    uint32_t        nSearchWindowHeight;
    uint32_t        nTemplateWidth;
    uint32_t        nTemplateHeight;
    uint32_t        nNCCMode;
    uint32_t        nNCCThreshold;

    INLINE _cvpDmeConfigNcc() : eScoreType(CVP_NCC_BEST_SCORE), nSearchWindowWidth(18), nSearchWindowHeight(18),  nTemplateWidth(8), nTemplateHeight(8), nNCCMode(2), nNCCThreshold(800) { }

} cvpDmeConfigNcc;

//------------------------------------------------------------------------------
/// @brief
///    Structure for RANSAC configuration
///    All members will have default values.
/// @param nRansacModel
///    RANSAC Transform Model to use
///    0:projective, 1:affine, 2:rigid, 3:rigid with post-processing
///    Default: 2
/// @param bRansacOptimEnable
///     Enable Ransac optimization
/// @param nRansacThreshold
///    RANSAC Threshold (TODO: Need default value)
/// @ingroup cvp_object_detection
//------------------------------------------------------------------------------
typedef struct _cvpDmeConfigRansac
{
    uint32_t     nRansacModel;
    bool         bRansacOptimEnable;
    float        nRansacThreshold;

    INLINE _cvpDmeConfigRansac() : nRansacModel(2), bRansacOptimEnable(true), nRansacThreshold(0) { }

} cvpDmeConfigRansac;

//------------------------------------------------------------------------------
/// @brief
///    Structure for DME configuration that will be sent once per session
/// @param nActualFps
///    Input frames per second. Default value is 30.
/// @param nOperationalFps
///    Desired output frames per second. nOperationalFps should be equal to
///    or greater than nActualFps. nOperationalFps >= nActualFps. Default value is 30.
/// @param sFullResImageInfo
///    Structure for Full-resolution/distorted source image information.
/// @param nScaledSrcWidth
///    ICA corrected OR Downscaled Image Width in pixels.
/// @param nScaledSrcHeight
///    ICA corrected OR Downscaled Image Height in Pixels.
/// @param sIcaConfigParam
///    Ica configuration
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param sHcdConfigParam
///    Hcd configuration
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param sNccConfigParam
///    NCC configuration
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param sRansacConfigParam
///    Ransac configuration
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param bICAEnable
///    Flag to enable ICA
///    Default: false
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param bDSEnable
///    Flag to enable downscaling for non camera usecases
///    Default: true
/// @param bCalcFinalTransform
///    Flag to enable calculation of the final Transform in firmware
///    If this flag is enabled, FW will return final transform in Q16 format.
///    Default: false
/// @param bEnableLRMERobustness
///    Flag to enable lrme_robustness
///    Default: true
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param bFtextEnable
///    Flag to enable Ftexture compute
///    Default: false
/// @param nFtextMode
///    FTexture Mode
///    0: Frame Level Stats, 1: MB level Stats
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param bEnableInlierTracking
///    Flag to enable Inlier Tracking. This should be enabled for Temporal Alignment Only
///    Default: false
/// @param nInlierStep
///    Inlier step
///    Range:0 to 120, Default:26
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param nOutlierStep
///    Outlier step
///    Range:-120 to 0, Default:-26
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param nFollowGlobalMotionStep
///    Global motion step
///    Range:0 to 120, Default:4
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param nNoMvConveyedInfoStep
///    Step size for no MV
///    Range:0 to 120, Default:12
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param nInvalidTransformStep
///    Invalid transform step
///    Range:0 to 120, Default:8
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param nValidTransformMinConfidenceForUpdates
///    Valid transform step
///    Range:255, Default:50
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param minInlierWeightTh
///    Minimum inlier width threshold
///    Range:0 to 15, Default:6
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param nMinAllowedTarVar
///    Minimum allowed tar variance value. If below this value
///    (non-informative block) - the block will be marked invalid
///    Range:TBD, Default:10
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param nMeaningfulNCCDiff
///    Locations with NCC difference below this threshold are
///    considered to have exactly the same similarity measure
///    Range:TBD, Default:5
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param nRobustnessDistMap
///    Provides a requirement on minimal normalized difference of
///    SADs at distance 1-9 pixels
///    Default values: [0, 128, 128, 128, 128, 128, 128, 128]
///    (no penalty to single pixel distance; afterwards same penalty
///    for all other distances)
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @ingroup cvp_motion_estimation
//------------------------------------------------------------------------------
typedef struct _cvpConfigDme
{
   uint32_t            nActualFps;
   uint32_t            nOperationalFps;
   cvpImageInfo        sFullResImageInfo;
   uint32_t            nScaledSrcWidth;
   uint32_t            nScaledSrcHeight;
   cvpDmeConfigIca     sIcaConfigParam;
   cvpDmeConfigHcd     sHcdConfigParam;
   cvpDmeConfigNcc     sNccConfigParam;
   cvpDmeConfigRansac  sRansacConfigParam;
   bool                bICAEnable;
   bool                bDSEnable;
   bool                bCalcFinalTransform;
   bool                bEnableLRMERobustness;
   bool                bFtextEnable;
   uint32_t            nFtextMode;
   bool                bEnableInlierTracking;
   int32_t             nInlierStep;
   int32_t             nOutlierStep;
   int32_t             nFollowGlobalMotionStep;
   int32_t             nNoMvConveyedInfoStep;
   int32_t             nInvalidTransformStep;
   int32_t             nValidTransformMinConfidenceForUpdates;
   uint32_t            minInlierWeightTh;
   uint32_t            nMinAllowedTarVar;
   uint32_t            nMeaningfulNCCDiff;
   uint32_t            nRobustnessDistMap[8];

   INLINE _cvpConfigDme() : nActualFps(30), nOperationalFps(30), bICAEnable(false), bDSEnable(false),
                            bCalcFinalTransform(false), bEnableLRMERobustness(true), bFtextEnable(false),
                            nFtextMode(0), bEnableInlierTracking(false), nInlierStep(26), nOutlierStep(-26),
                            nFollowGlobalMotionStep(4), nNoMvConveyedInfoStep(12), nInvalidTransformStep(8),
                            nValidTransformMinConfidenceForUpdates(50), minInlierWeightTh(6),
                            nMinAllowedTarVar(10), nMeaningfulNCCDiff(5)
   {
      nRobustnessDistMap[0] = 0;
      nRobustnessDistMap[1] = 128;
      nRobustnessDistMap[2] = 128;
      nRobustnessDistMap[3] = 128;
      nRobustnessDistMap[4] = 128;
      nRobustnessDistMap[5] = 128;
      nRobustnessDistMap[6] = 128;
      nRobustnessDistMap[7] = 128;
   }

} cvpConfigDme;

//------------------------------------------------------------------------------
/// @brief
///    Structure for DME configuration that will be sent for every frame
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param nICAScaledInputWidth
///    Input frame is downscaled to this width prior to sending it to ICA.
///    Default val is 0. Both bICAEnable and bDSEnable in cvpConfigDme needs to be set to 1.
/// @param nICAScaledInputHeight
///    Input frame is downscaled to this height prior to sending it to ICA.
///    Default val is 0. Both bICAEnable and bDSEnable in cvpConfigDme needs to be set to 1.
///    Both bICAEnable and bDSEnable in cvpConfigDme needs to be set to 1.
/// @param nCoarseCenterW
///    Coarse Center width.
///    Default set to 0. CVP FW will calculate this if not set.
/// @param nCoarseCenterH
///    Coarse Center height.
///    Default set to 0. CVP FW will calculate this if not set.
/// @param nCoarse2FullTrasformScale
///    Coarse to Full resolution upscale ratio
///    Default set to 0. CVP FW will calculate this if not set.
/// @param bSkipMVCalc
///    Flag to skip MV Calculation & Transform Model for current frame.
///    This flag is only set for 1st frame in temporal alignment
/// @param bEnableDescriptorLPF
///    Flag to enable 5x5 filtering
///    0: Disable, 1: Enable (default)
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param bEnableNCCSubPel
///    0: Disable Subpel
///    1: Enable Subpel (default)
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param nMinFpxThreshold
///    Min Threashold for HCD FPX; Default:2, Range:TBD
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param nDescMatchThreshold
///    Descriptor Match threshold; Range 4:253, Default=52
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @param nNCCRobustnessThreshold
///    Threshold above which NCC result is valid; Range= -3:4, Default=0
///    [Internal] Stand-alone video usecase (WFD) will not need to initialize
/// @ingroup cvp_motion_estimation
//------------------------------------------------------------------------------
typedef struct _cvpFrameConfigDme // Video would not have to set anything
{
   uint32_t  nICAScaledInputWidth;
   uint32_t  nICAScaledInputHeight;
   uint32_t  nCoarseCenterW;
   uint32_t  nCoarseCenterH;
   uint32_t  nCoarse2FullTrasformScale;
   bool      bSkipMVCalc;
   bool      bEnableDescriptorLPF;
   bool      bEnableNCCSubPel;
   uint32_t  nMinFpxThreshold;
   uint32_t  nDescMatchThreshold;
   int32_t   nNCCRobustnessThreshold;

   INLINE _cvpFrameConfigDme() : nICAScaledInputWidth(0), nICAScaledInputHeight(0), nCoarseCenterW(0), nCoarseCenterH(0), nCoarse2FullTrasformScale(0), bSkipMVCalc(false), bEnableDescriptorLPF(true), bEnableNCCSubPel(true), nMinFpxThreshold(2), nDescMatchThreshold(52), nNCCRobustnessThreshold(0) { }

} cvpFrameConfigDme;

//------------------------------------------------------------------------------
/// @brief
///    Structure representing DME output
/// @param nFrameSumGradient
///    Frame-Level Sum Gradient
/// @param nFrameSumSquareGradient1
///    Frame-Level Sum Square Gradient1
/// @param nFrameSumSquareGradient2
///    Frame-Level Sum Square Gradient2
/// @param nLumaHist
///    Luma Histogram
/// @param nMVxSum
///    cumulative sum of MVx
/// @param nMVySum
///    cumulative sum of MVy
/// @param nNumMVs
///    NumMVs
/// @param FinalTransform
///    Final 3x3 Transform in Q16 format (Will be used by Video stand-alone usecases, e.g., WFD)
/// @param Transform
///    Intermediate 3x3 Transform generated by RANSAC (Will be used by Camera)
/// @ingroup cvp_motion_estimation
//------------------------------------------------------------------------------
typedef struct _cvpDmeOutput
{
   double    FullResTransform[9];
   double    CoarseTransform[9];
   int32_t   FullResTransformQ16[9];
   int32_t   CoarseTransformQ16[9];
   uint32_t  FullResWidth;
   uint32_t  FullResHeight;
   uint32_t  ProcessedWidth;
   uint32_t  ProcessedHeight;
   uint32_t  nIsValid;
   uint32_t  nFrameSumGradient;
   uint32_t  nFrameSumSquareGradient1;
   uint32_t  nFrameSumSquareGradient2;
   uint16_t  nLumaHist[8];
   int32_t   nMVxSum;
   int32_t   nMVySum;
   uint32_t  nNumMVs;
   uint32_t  nVersionNum;
    int32_t   nTransformConfidence;
    uint32_t  reserved[185];
} cvpDmeOutput;

//------------------------------------------------------------------------------
/// @brief
///    Structure for DME reference/downscaled output buffer requirement
/// @param nRefBufferBytes
///    The required size in bytes for src/reference buffer.
/// @param nRefFrameContextBytes
///    The required size in bytes for src/reference frame context.
/// @param nDMEOutputBytes
///    The required size in bytes for cvpDmeOutput.
///
/// @ingroup cvp_motion_estimation
//------------------------------------------------------------------------------
typedef struct _cvpDmeRefBuffReq
{
   uint32_t nRefBufferBytes;
   uint32_t nRefFrameContextBytes;
   uint32_t nDMEOutputBytes;
} cvpDmeRefBuffReq;

//------------------------------------------------------------------------------
/// @brief
///    The DME has been processed by the CVP system, and the
///    output is ready to be consumed. This callback is used to notify the
///    application. The application sends the input using the
///    cvpDme_Async call.
///
/// @param eStatus
///    CVP status for the current process.
/// @param pScaledRefImage
///    Pointer to ref buffer.
/// @param pScaledRefFrameCtx
///    Pointer to ref frame context.
/// @param dDmeOutput
///    Structure containing DME output
/// @param hDme
///    Handle for DME that was passed in the cvpDme_Async function.
/// @param pSessionUserData
///    User-data that was set in the cvpInitDme structure.
/// @param pTaskUserData
///    User-data that was passed in the cvpDme_Async function
///    which corresponds to this callback.
///
/// @ingroup cvp_motion_estimation
//------------------------------------------------------------------------------
typedef void (*CvpDmeCb)(cvpStatus      eStatus,
                         cvpImage      *pScaledRefImage,
                         cvpMem        *pScaledRefFrameCtx,
                         cvpDmeOutput  *pDmeOutput,
                         cvpHandle      hDme,
                         void          *pSessionUserData,
                         void          *pTaskUserData);

//------------------------------------------------------------------------------
/// @brief
///    Initialize CVP - DME.
/// @param hSession
///    [Input] CVP session handle
/// @param pConfig
///    [Input] Pointer to the DME configuration.
/// @param pOutMemReq
///    [Output] Pointer to the output memory requirement.
/// @param fnCb
///    [Input] Callback function for the asynchronous API
///    Setting to NULL will result in initializing for synchronous API
/// @param pSessionUserData
///    [Input] A private pointer the user can set with this session, and this
///    pointer will be provided as parameter to all callback functions
///    originated from the current session. This could be used to associate a
///    callback to this session. This can only be set once while initializing
///    the handle. This value will not/cannot-be changed for life
///    of a session.
///
/// @retval CVP handle for DME.
///    If successful.
/// @retval NULL
///    If initialization failed.
///
/// @ingroup cvp_motion_estimation
//------------------------------------------------------------------------------
CVP_API cvpHandle cvpInitDme(cvpSession           hSession,
                             const cvpConfigDme  *pConfig,
                             cvpDmeRefBuffReq    *pOutMemReq,
                             CvpDmeCb             fnCb,
                             void                *pSessionUserData);

//------------------------------------------------------------------------------
/// @brief
///    Deinitialize CVP - DME.
/// @param hDme
///    [Input] CVP handle for DME.
///
/// @retval CVP_SUCCESS
///    If deinit is successful.
///
/// @ingroup cvp_motion_estimation
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDeInitDme(cvpHandle hDme);

//------------------------------------------------------------------------------
/// @brief
///    DME Process call
/// @details
///    Synchronous (blocking) function that will do DME.
///    Illustration for use of Frame parameters in the API
///          Frame param               Time t0        Time t1        Time t2
///    1. Full res/dist ICA image        P0             P1              P2
///    2. Scaled src image               Ps0            Ps0`            Ps1`
///    3. Scaled ref image               NULL           Ps1`            Ps2`
/// @param hDme
///    [Input] Handle for DME.
/// @param pFrameConfigDme
///    [Input] Pointer to set of per-frame DME configurations.
/// @param pFullResImage
///    [Input] Pointer to either the full resolution or ICA distorted image.
/// @param pScaledSrcImage
///    [Input] Pointer to downscaled source frame buffer.
/// @param pScaledSrcFrameCtx
///    [Output] Pointer to downscaled source frame context.
/// @param pEcuBuffer
///    [Input] Pointer to exposure compensation unit frame config buffer.
///    [Internal] Buffer content filled using struct cvpDmeFrameConfigEcu
/// @param pGridBuffer
///    [Input] Pointer to grid transform buffer.
///    [Internal] Buffer content filled using struct cvpDmeFrameConfigIca
/// @param pScaledRefImage
///    [Input] Pointer to downscaled frame buffer.
///    [Note] This would be the reference buffer for the next frame processing.
/// @param pScaledRefFrameCtx
///    [Input] Pointer to downscaled frame context.
///    [Note] This would be the reference context for the next frame processing.
/// @param pDmeOutput
///    [Output] pointer to DME output structure.
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
///
/// @ingroup cvp_motion_estimation
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDme_Sync(cvpHandle           hDme,
                              cvpFrameConfigDme  *pFrameConfigDme,
                              const cvpImage     *pFullResImage,
                              cvpImage           *pScaledSrcImage,
                              cvpMem             *pScaledSrcFrameCtx,
                              cvpMem             *pEcuBuffer,
                              cvpMem             *pGridBuffer,
                              cvpImage           *pScaledRefImage,
                              cvpMem             *pScaledRefFrameCtx,
                              cvpMem             *pDmeOutput);

//------------------------------------------------------------------------------
/// @brief
///    DME Process Call in Async
/// @details
///    Asynchronous function that will queue the image and return almost
///    immediately. In the background, it will do DME.
///    Once the output is ready, it will notify the user through the callback
///    function and user can queue another image using the same buffer.
///    DME output will be available in the output structure returned in the callback.
///    Illustration for use of Frame parameters in the API
///          Frame param               Time t0        Time t1        Time t2
///    1. Full res/dist ICA image        P0             P1              P2
///    2. Scaled src image               Ps0            Ps0`            Ps1`
///    3. Scaled ref image               NULL           Ps1`            Ps2`
/// @param hDme
///    [Input] Handle for DME.
/// @param pFrameConfigDme
///    [Input] Pointer to set of per-frame DME configurations.
/// @param pFullResImage
///    [Input] Pointer to either the full resolution or ICA distorted image.
/// @param pScaledSrcImage
///    [Input] Pointer to downscaled source frame buffer.
///    [Note] This would be the reference buffer from the previous frame processing.
/// @param pScaledSrcFrameCtx
///    [Output] Pointer to downscaled source frame context.
/// @param pEcuBuffer
///    [Input] Pointer to exposure compensation unit frame config buffer.
///    [Internal] Buffer content filled using struct cvpDmeFrameConfigEcu
/// @param pGridBuffer
///    [Input] Pointer to grid transform buffer.
///    [Internal] Buffer content filled using struct cvpDmeFrameConfigIca
/// @param pScaledRefImage
///    [Input] Pointer to downscaled ref frame buffer.
/// @param pScaledRefFrameCtx
///    [Input] Pointer to downscaled ref frame context.
/// @param pDmeOutput
///    [Output] pointer to DME output structure.
/// @param pFences
///    [Output] pointer to fence objects.
/// @param nNumFences
///    [Output] Number of fences in the pFences.
/// @param pTaskUserData
///    [Input] Pointer to user-data buffer which corresponds to the callback.
///
/// @retval CVP_SUCCESS
///    If the image is successfully pushed to the queue. It will be processed
///    immediately.
/// @retval CVP_EBADPARAM
///    If there is any bad parameter.
/// @retval CVP_EUNSUPPORTED
///    If the handle is initialized for the synchronous API.
///
/// @ingroup cvp_motion_estimation
//------------------------------------------------------------------------------
CVP_API cvpStatus cvpDme_Async(cvpHandle           hDme,
                               cvpFrameConfigDme  *pFrameConfigDme,
                               const cvpImage     *pFullResImage,
                               cvpImage           *pScaledSrcImage,
                               cvpMem             *pScaledSrcFrameCtx,
                               cvpMem             *pEcuBuffer,
                               cvpMem             *pGridBuffer,
                               cvpImage           *pScaledRefImage,
                               cvpMem             *pScaledRefFrameCtx,
                               cvpMem             *pDmeOutput,
                               cvpFence           *pFences,
                               uint32_t           nNumFences,
                               const void         *pTaskUserData);

#ifdef __cplusplus
}//extern "C"
#endif

#endif