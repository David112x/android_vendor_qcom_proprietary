// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------
#ifndef __GEOLIB_H__
#define __GEOLIB_H__

//------------------------------------------------------------------------
// @file  GeoLib.h
// @brief GeoLib API functions
//------------------------------------------------------------------------

#include <stdint.h>
#include "NcLibWarp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**< Video/Preview supported flows */
enum GeoLibVideoFlowMode
{
    GEOLIB_VIDEO_WITH_INLINE_EIS,           /**< Video with inline EIS, undistorted IPE output (V_IE_U)  */
    GEOLIB_VIDEO_EIS_POST_IPE_DISTORTED,    /**< Video with EIS/DIS post-IPE, distorted IPE output (V_EPI_D) */
    GEOLIB_VIDEO_EIS_POST_IPE_UNDISTORTED,  /**< Video with EIS/DIS post-IPE, undistorted IPE output (V_EPI_U) */
    GEOLIB_VIDEO_NO_EIS_IPE_DISTORTED,      /**< Video without EIS/DIS, distorted IPE output (V_WOE_D) */
    GEOLIB_VIDEO_NO_EIS_IPE_UNDISTORTED,    /**< Video without EIS/DIS, undistorted IPE output (V_WOE_U) */
    GEOLIB_STILL_PREVIEW_IPE_DISTORTED,     /**< Preview, distorted IPE output (P_D) */
    GEOLIB_STILL_PREVIEW_IPE_UNDISTORTED,   /**< Preview, undistorted IPE output (P_U) */

    GEOLIB_FLOW_NUM                         /**< only for SW purpose */
};

/**< Snapshot/MFNR/MFSR flows */
enum GeoLibStillFlowMode
{
    GEOLIB_STILL_SNAPSHOT,          /**< Snapshot */
    GEOLIB_STILL_MFNR,              /**< MFNR (MFSR is permanently disabled) */
    GEOLIB_STILL_MFSR,              /**< Enable MFSR when possible, if all necessary conditions are held, otherwise fall through to MFNR mode (e.g., when MFSR is not possible due to insufficient upscaling). Depends on Chromatix (MF:Max_SR_sensor_gain) */
    GEOLIB_STILL_FLOW_MODE_NUM      /**< only for SW purpose */
};

/**< Snapshot/MFNR/MFSR flows */
enum GeoLibMultiFrameMode
{
    GEOLIB_MF_ANCHOR,               /**< Multi-frame pre-filter   */
    GEOLIB_MF_BLEND,                /**< Multi-frame blend stage  */
    GEOLIB_MF_FINAL,                /**< Multi-frame post-filter  */
    GEOLIB_MF_MODE_NUM              /**< only for SW purpose */
};

typedef NcLibFloat2 GeoLibFloat2;
typedef NcLibROI GeoLibROI;
typedef NcLibImageSize GeoLibImageSize;
typedef NcLibFloat2 GeoLibWindowSize;

/**< Crop and scale configuration for cropping fractional ROI on input image, then
  scaling it to a fractional ROI on the output. The exact scaling factor is determined
  by the ratio between the crop ROI in the input and ROI in the output.

  NOTE: Current implementation will always set the offset of the output ROI to zero,
  meaning that the top and left of the output image will always be aligned with the
  top and left (fractional) offset in the input ROI.

                  ___________________________
       Input     |  |xxxxxxxxxxxxxx|         |
       Image     |  |x   IN ROI   x|         |
                 |__|xxxxxxxxxxxxxx|_________|
                        |
                     ___V______
       Output       |xxxxxxx   |
       Image        |x ROI x   | <-- padding
                    |xxxxxxx___|
 */
struct GeoLibCropAndScale
{
    GeoLibROI                       inputImageRoi;          /**< Input crop ROI (fractional) relative to sensor dimensions */
    GeoLibImageSize                 inputImageSize;         /**< Input size in pixels */
    GeoLibROI                       outputImageRoi;         /**< Scaled and cropped fractional ROI relative to the output; it can be smaller than the output due to output buffer padding. */
    GeoLibImageSize                 outputImageSize;        /**< Output size in pixels, including optional padding according to output ROI */

};

/**< ICA logical mapping parameters (one-to-one correspondence to ICA O2V and V2I registers) */
struct GeoLibIcaMapping
{
    GeoLibROI                       inputImageRoi;          /**< Input image ROI (on distorted domain) */
    GeoLibImageSize                 inputImageSize;         /**< Input image size in pixels (can be padded) */
    GeoLibImageSize                 inputValidSize;         /**< Input valid size in pixels */
    GeoLibROI                       outputImageRoi;         /**< Output image ROI (on undistorted domain) */
    GeoLibImageSize                 outputImageSize;        /**< Output image size in pixels */
};

/**< Multi-pass ICA1/ICA2 mapping configuration */
struct GeoLibIcaPassMapping
{
    uint32_t                        numPasses;
    GeoLibIcaMapping                icaMappingFull;         /**< IPE 1:1 pass mapping */
    GeoLibIcaMapping                icaMappingDS4;          /**< IPE 1:4 pass mapping */
    GeoLibIcaMapping                icaMappingDS16;         /**< IPE 1:16 pass mapping */
    GeoLibIcaMapping                icaMappingDS64;         /**< IPE 1:64 pass (still only) */
    GeoLibROI                       dzFOV;                  /**< Cropped zoom FOV */
};

/**< Specifies if LDC grid resampling is needed and the input for NcLibResampleGrid() */
struct GeoLibGridRoi
{
    bool                            enabled;                /**< Disabled if resample is not needed */
    GeoLibROI                       cropRoi;                /**< ROI for grid resampling (NcLibResampleGrid) */
    GeoLibROI                       outRoi;                 /**< ROI for grid resampling (NcLibResampleGrid) */
};

/**< EIS-related parameters, as defined in EIS Chromatix */
struct GeoLibEisParams
{
    GeoLibFloat2                    minMarginRatio;         /**< Minimal EIS total margin ratio. Defined in Chromatix (EIS:minimal_total_margin) */
    GeoLibFloat2                    reqMarginRatio;         /**< EIS requested total margin ratio. Defined in Chromatix (EIS:requested_total_margins) */
};

/**< Inputs to EIS algorithm */
struct GeoLibInternalEisConfig
{
    GeoLibROI                       preCropDistorted;       /**< Pre-EIS crop window on distorted sensor (full IFE input and FOV) */
    GeoLibROI                       preCropUndistorted;     /**< Pre-EIS crop window on undistorted sensor (after LDC) */
    GeoLibROI                       outCropFOV;             /**< Final EIS output FOV, relative to undistorted pre-EIS crop window */
};

/**< Inputs to SAT algorithm */
struct GeoLibInternalSatConfig
{
    GeoLibROI                       satROI;                 /**< SAT ROI including margins in IFE out domain */
    GeoLibROI                       outFOV;                 /**< SAT transform output FOV in IFE Out domain */
};

/**< Inputs to ERS algorithm */
struct GeoLibInternalErsConfig
{
    GeoLibROI                       preCropDistorted;       /**< Pre-ERS crop window on distorted sensor (full IFE input and FOV) */
    GeoLibROI                       preCropUndistorted;     /**< Pre-ERS crop window on undistorted sensor (after LDC) */
};

/**< Frame-level inputs to IPE striping library */
struct GeoLibIpeStripingConfig
{
    GeoLibImageSize                 mainInputSize;          /**< Full size of Main (NPS) input window */
    GeoLibROI                       tfCropWindow;           /**< TF crop ROI on mainInput, aligned to 8 px */
    GeoLibROI                       videoOutFOV;            /**< FOV of IPE video output on mainInput */
    GeoLibROI                       displayOutFOV;          /**< FOV of IPE display output on mainInput */
};

/**< Each IFE output port is configured separately. */
enum GeoLibVideoStreamIfePort
{
    GEOLIB_IFE_PORT_VIDEO_OUT,                              /**< Video out, supports VSR */
    GEOLIB_IFE_PORT_DISPLAY_OUT,                            /**< Display out, no VSR */
    GEOLIB_IFE_PORT_OUT_NUM                                 /**< only for SW purpose */
};

#define GEOLIB_VIDEO_INIT_CTX_SIZE    132

/**< GeoLib Video one-time outputs (init) per stream (either IFE video out or IFE display out) */
struct GeoLibVideoStreamConfig
{
    /*
    * PER-STREAM INITIALIZATION OUTPUTS
    */

    /* IFE BUFFER SIZES */

    GeoLibImageSize                 ifeOutFullSize;         /**< Minimal buffer size required for IFE_out_1:1 (IFE video full output or IFE display full output) */
    GeoLibImageSize                 ifeOutDS4Size;          /**< Minimal buffer size required for IFE_out_1:4 (DSX for IFE video output or DS4 for IFE display output) */
    GeoLibImageSize                 ifeOutDS16Size;         /**< Minimal buffer size required for IFE_out_1:16 (IFE video DS16 output or IFE display DS16 output) */

    GeoLibImageSize                 tfOutFullSize;          /**< Minimal buffer size required for TF Ref out 1:1, in pixels.*/
    GeoLibImageSize                 tfOutDS4Size;           /**< Minimal buffer size required for TF Ref out 1:4, in pixels.*/
    GeoLibImageSize                 tfOutDS16Size;          /**< Minimal buffer size required for TF Ref out 1:16, in pixels.*/


    /* HORIZONTAL OFF-CENTER SHIFT IN SENSOR DOMAIN  */
    float                           ifeInCenterH;

    /* LDC RESAMPLED ON PROPER INPUT DOMAIN */
    NcLibIcaGrid                    ldCropOutToIn;              /**< Output-to-input LDC grid transform, resampled for per-frame calculations */
    NcLibIcaGrid                    ldCropInToOut;              /**< Input-to-output LDC grid transform, resampled for per-frame calculations */

    /* VIDEO ENCODER GMO DOWNSCALE GRID FACTOR */
    GeoLibFloat2                    gmoScale;                   /**< GMO grid scaling, less or equal 1.0 */

    /*
    * INTERNAL CONTEXT FOR PER-FRAME CALCULATIONS
    */
    uint8_t                         initCtx[GEOLIB_VIDEO_INIT_CTX_SIZE];
};

/**< GeoLib Video per-frame outputs */
struct GeoLibVideoFrameConfig
{
    GeoLibCropAndScale              ifeFullCropAndScale;        /**< IFE pre-crop and M/N DS settings (IFE video full output or IFE display full output) */
    GeoLibCropAndScale              ifeDSXCropAndScale;         /**< IFE DSX settings (IFE video out) */
    GeoLibCropAndScale              ifeFdCropAndScale;          /**< IFE face detection output settings (FD_OUT) */
    GeoLibInternalEisConfig         eisConfig;                  /**< EIS algorithm configuration */
    GeoLibInternalErsConfig         ersConfig;                  /**< ERS algorithm configuration */
    GeoLibInternalSatConfig         satConfig;                  /**< SAT algorithm configuration */
    GeoLibIcaPassMapping            ica1Mapping;                /**< ICA1 mapping configuration */
    GeoLibIcaPassMapping            ica2Mapping;                /**< ICA2 mapping configuration */
    GeoLibImageSize                 cvpDSoutSize;               /**  CVP-DS output size, if 0 DS should be disabled */
    GeoLibIcaMapping                cvpIcaMapping;              /**< CVP-ICA mapping configuration */
    GeoLibGridRoi                   ica1GridRoi;                /**< ICA1 grid resampling, if needed */
    GeoLibGridRoi                   ica2GridRoi;                /**< ICA2 grid resampling, if needed */
    GeoLibGridRoi                   cvpIcaGridRoi;              /**< CVP-ICA grid resampling, if needed */
    GeoLibIpeStripingConfig         ipeStripingInputs;          /**< Frame-level inputs to IPE striping library */
    GeoLibCropAndScale              ppsUsCropAndScale;          /**< PPS pre-crop and Upscaler settings */
    GeoLibCropAndScale              videoOutCropAndScale;       /**< Video out pre-crop and M/N DS config */
    GeoLibCropAndScale              displayOutCropAndScale;     /**< Display out pre-crop and M/N DS config */
    GeoLibGridRoi                   gmoGridRoi;                 /**< GMO transform resampling windows */
    uint64_t                        requestID;                  /**< Request ID for debugging purposes */
};

/**
* @brief One-time video flow initialization per stream (either IFE video output or IFE display output).
*
* @param [in]  flowMode           Use case description
* @param [in]  sensorSize         Full sensor output size (CSID_out), distorted pixels in case of LD
* @param [in]  ifeOutPort         Either video or display IFE out port selection
* @param [in]  fdOutSize          Face ditection final output size or NULL if FD config is not needed
* @param [in]  ldFullOutToIn      Output-to-input LDC grid transform for this sensor. Defined in Chromatix (ICA:ctc_grid_x/ctc_grid_y)
* @param [in]  ldFullInToOut      Input-to-output LDC grid transform for this sensor. Defined in Chromatix (ICA:ld_i2u_grid_x/ld_i2u_grid_y)
* @param [in]  ldFullOutSize      Undistorted LDC output size, in square pixels. Should be zero when LDC is disabled. Defined in Chromatix (ICA:ld_full_out_width/ld_full_out_height)
* @param [in]  videoOutSize       Video/Preview final output size in pixels
* @param [in]  displayOutSize     Display output size (in IPE SIMO mode) in pixels, or NULL if IPE display output is disabled
* @param [in]  maxVsrScaling      Max VSR upscale ratio, in range [1.0, 3.0]; 1.0 means no VSR. Defined in Chromatix (VIDEO:max_vsr)
* @param [in]  eisParameters      EIS-related parameters, as defined in EIS Chromatix
* @param [out] streamConfig       Output stream configuration
*
* @return NC_LIB_SUCCESS or appropriate error code on failure.
*/
int32_t GeoLibVideoStreamInit(
    const GeoLibVideoFlowMode       flowMode,
    const GeoLibImageSize*          sensorSize,
    GeoLibVideoStreamIfePort        ifeOutPort,
    const GeoLibImageSize*          fdOutSize,
    const NcLibIcaGrid*             ldFullOutToIn,
    const NcLibIcaGrid*             ldFullInToOut,
    const GeoLibImageSize*          ldFullOutSize,
    const GeoLibImageSize*          videoOutSize,
    const GeoLibImageSize*          displayOutSize,
    const float                     maxVsrScaling,
    const GeoLibEisParams*          eisParameters,
    GeoLibVideoStreamConfig*        streamConfig
);

/**
* @brief Per-frame video flow calculation.
*
* @param [in]  streamConfig     Initial stream configuration (from GeoLibVideoStreamInit)
* @param [in]  zoomWindow       Digital zoom window on undistorted LDC domain, normalized
* @param [in]  satWindowSize    Crop window for SAT on undistorted LDC domain, normalized and centered at zoom window. SAT window size must be bigger or equal to zoom window size, or NULL if SAT is not enabled.
* @param [in]  cvpDsRatio       Extra downscaling in CVP-DS for VSR flows, in range [1.0, 6.0]; 1.0 means no downscaling is done before registration. Defined in Chromatix (CVP:video_registration_down_scale_ratio)
* @param [in]  motionIndication Motion indication (M_ind) for motion-dependent EIS margin, in range [0.0, 1.0] as calculated by EIS algo, or 1.0 otherwise. Motion indication is 0 for slow motion and 1 otherwise. If slow motion the EIS margin can be reduced for highest resolution, otherwise constant EIS margin is maintained.
* @param [in]  prevFrameConfig  Previous frame configuration (NULL for first frame)
* @param [out] frameConfig      Output frame configuration
*
* @return NC_LIB_SUCCESS or appropriate error code on failure.
*/
int32_t GeoLibVideoCalcFrame(
    const GeoLibVideoStreamConfig*  streamConfig,
    const GeoLibROI*                zoomWindow,
    const GeoLibWindowSize*         satWindowSize,
    const float                     cvpDsRatio,
    const float                     motionIndication,
    const GeoLibVideoFrameConfig*   prevFrameConfig,
    GeoLibVideoFrameConfig*         frameConfig
);

/**< GeoLib Still Configuration */
struct GeoLibStillFrameConfig
{
    GeoLibStillFlowMode             flowMode;                   /**< Flow mode */
    GeoLibMultiFrameMode            mfMode;                     /**< multi frame mode */

    GeoLibImageSize                 bpsOutSizeFull;             /**< BPS 1:1 out image size */
    GeoLibImageSize                 bpsOutSizeDS4;              /**< BPS 1:4 out image size */
    GeoLibImageSize                 bpsOutSizeDS16;             /**< BPS 1:16 out image size */
    GeoLibImageSize                 bpsOutSizeDS64;             /**< BPS 1:64 out image size */
    GeoLibCropAndScale              bpsOutFullCrop;             /**< BPS 1:1 crop-only (no scaling) */
    GeoLibCropAndScale              bpsRegCropAndScale;         /**< BPS reg out M/N DS configuration */
    float                           offCenterH;                 /**< Horizontal off-center shift in sensor domain */
    GeoLibIcaPassMapping            ica1Mapping;                /**< ICA1 mapping configuration */
    GeoLibIcaPassMapping            ica2Mapping;                /**< ICA2 mapping configuration */
    GeoLibIcaMapping                cvpIcaMapping;              /**< CVP-ICA mapping configuration */
    GeoLibIpeStripingConfig         ipeStripingInputs;          /**< Frame-level inputs to IPE striping library */
    GeoLibCropAndScale              ppsUsCropAndScale;          /**< PPS pre-crop and Upscaler settings */
    GeoLibCropAndScale              stillOutCropAndScale;       /**< Still out pre-crop and M/N DS config */
    GeoLibCropAndScale              dispOutCropAndScale;        /**< Display out pre-crop and M/N DS config */
    GeoLibFloat2                    coarse2fullScale;           /**< Scale factor from coarse (CVP DME) domain to full (IPE in) domain */
    GeoLibFloat2                    coarseCenter;               /**< Transform center in coarse (CVP DME) domain. */
};

/**
* @brief Still flow calculation.
*
* @param [in]  flowMode         Use case description
* @param [in]  mfMode           Multi-frame mode (N/A for snapshot)
* @param [in]  sensorSize       Full sensor output size (RDI), distorted pixels in case of LD
* @param [in]  ldFullOutToIn    Output-to-input LDC grid transform for this sensor. Defined in Chromatix (ICA:ctc_grid_x/ctc_grid_y)
* @param [in]  ldFullInToOut    Input-to-output LDC grid transform for this sensor. Defined in Chromatix (ICA:ld_i2u_grid_x/ld_i2u_grid_y)
* @param [in]  ldFullOutSize    Undistorted LDC output size, in square pixels. Should be zero when LDC is disabled. Defined in Chromatix (ICA:ld_full_out_width/ld_full_out_height)
* @param [in]  stillOutSize     Main (still) output size in pixels
* @param [in]  dispOutSize      Display output size (in IPE SIMO mode) in pixels, or NULL if IPE display output is disabled
* @param [in]  regOutSize       Registration image size, should be NULL in case of still snapshot or multi-frame post-processing (GEOLIB_MF_FINAL). Defined in Chromatix (CVP:multi_frame_input_resolution).
* @param [in]  zoomWindow       Digital zoom window on undistorted LDC domain, normalized
* @param [out] stillConfig      Output configuration
*
* @return NC_LIB_SUCCESS or appropriate error code on failure.
*/
int32_t GeoLibStillFrameCalc(
    const GeoLibStillFlowMode       flowMode,
    const GeoLibMultiFrameMode      mfMode,
    const GeoLibImageSize*          sensorSize,
    const NcLibIcaGrid*             ldFullOutToIn,
    const NcLibIcaGrid*             ldFullInToOut,
    const GeoLibImageSize*          ldFullOutSize,
    const GeoLibImageSize*          stillOutSize,
    const GeoLibImageSize*          dispOutSize,
    const GeoLibImageSize*          regOutSize,
    const GeoLibROI*                zoomWindow,
    GeoLibStillFrameConfig*         stillConfig
);

#ifdef __cplusplus
}
#endif

#endif // __GEOLIB_H__
