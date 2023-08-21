/*!
 ******************************************************************************
 * @file            immotion_api.h
 *
 * @services        ImMotion interface
 *
 * @description     Provides the definition for the ImMotion API
 *
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 ******************************************************************************
 */

/*!
 * Note: This header should not be modified.
 */

/*!
 * ImMotion is a cinemograph application that takes a sequence of frames and
 * blends regions of interest in these frames together to add motion effects
 * to an otherwise still "photo".
 *
 * ImMotion is an asynchronous application; the main processing functions do
 * not return frames when received but rather at a later time via a callback.
 * Callbacks are received in the same order frame process calls are made. The
 * callback thread is the same as the thread which handles ImMotion calls
 * asynchronously, and so the callback should not perform any intensive tasks.
 * ImMotion is a dual pass solution consisting of an Analysis stage and a Blend
 * stage. The two passes may share information via a persistent memory buffer.
 * The following restrictions currently apply:
 *     1) Only one ImMotion session may be active at any time.
 *     2) The input and output resolution must be the same.
 *     3) Frame dimensions must be divisible by 8.
 */

#ifndef _IMMOTION_API_H_
#define _IMMOTION_API_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMMOTION_MAX_CLOSE_ARR_ALLOC 128

/*!
 * ImMotion boolean type
 */
typedef int32_t immotion_bool_t;

/*!
 * ImMotion context
 */
typedef void* immotion_context_t;

/*!
 * Process Callback definition
 */
typedef struct immotion_analysis_process_callback_info_t
               immotion_analysis_process_callback_info_t;
typedef void (*immotion_analysis_process_callback_t)
             (immotion_analysis_process_callback_info_t callback_info);

typedef struct immotion_blend_process_callback_info_t
               immotion_blend_process_callback_info_t;
typedef void (*immotion_blend_process_callback_t)
             (immotion_blend_process_callback_info_t callback_info);

/*!
 * ImMotion return codes
 */
typedef enum {
    /*! Successful run, no errors */
    IMMOTION_OK,
    /*! Unclassified error */
    IMMOTION_ERR,
    /*! Out of memory */
    IMMOTION_ERR_MEMORY,
    /*! OS specific error */
    IMMOTION_ERR_OS,
    /*! Invalid context */
    IMMOTION_ERR_INVALID_CONTEXT,
    /*! Invalid ROI boxes */
    IMMOTION_ERR_INVALID_ROI_BOXES,
    /*! An input buffer is invalid */
    IMMOTION_ERR_INVALID_INPUT_FRAME,
    /*! An output buffer is invalid */
    IMMOTION_ERR_INVALID_OUTPUT_FRAME,
    /*! The mask buffer is invalid */
    IMMOTION_ERR_INVALID_MASK_BUF,
    /*! The segment blob buffer is invalid */
    IMMOTION_ERR_INVALID_SEG_BLOB_BUF,
    /*! The selected segment blob buffer is invalid */
    IMMOTION_ERR_INVALID_SEL_SEG_BLOB_BUF,
    /*! The scratch buffer is invalid */
    IMMOTION_ERR_INVALID_SCRATCH_BUF,
    /*! The persist buffer is invalid */
    IMMOTION_ERR_INVALID_PERSIST_BUF,
    /*! An output infomation structure is invalid */
    IMMOTION_ERR_INVALID_INFO_ADDRESS,
    /*! ImMotion functions called in an incorrect order */
    IMMOTION_ERR_INVALID_CALL_ORDER,
    /*! The provided frame format is invalid */
    IMMOTION_ERR_INVALID_FRAME_FORMAT,
    /*! The provided frame format is unsupported */
    IMMOTION_ERR_UNSUPPORTED_FRAME_FORMAT,
    /*! The provided frame resolution is unsupported */
    IMMOTION_ERR_UNSUPPORTED_FRAME_RESOLUTION,
    /*! The provided mask format is invalid */
    IMMOTION_ERR_INVALID_MASK_FORMAT,
    /*! The provided mask format is unsupported */
    IMMOTION_ERR_UNSUPPORTED_MASK_FORMAT,
    /*! The provided mask resolution is unsupported */
    IMMOTION_ERR_UNSUPPORTED_MASK_RESOLUTION,
    /*! The provided number of scanlines is invalid */
    IMMOTION_ERR_INVALID_NUM_SCANLINES,
    /*! The provided output frame parameters and input frame parameters do not match */
    IMMOTION_ERR_OUTPUT_INPUT_FRAME_PARAMS_MISMATCH,
    /*! The provided mask frame parameters and input frame parameters do not match */
    IMMOTION_ERR_MASK_INPUT_FRAME_PARAMS_MISMATCH,
    /*! The frame parameters do not match with the frame parameters used in Analysis */
    IMMOTION_ERR_FRAME_PARAMS_MISMATCH_ANALYSIS,
    /*! The Analysis mode is invalid */
    IMMOTION_ERR_INVALID_ANALYSIS_MODE,
    /*! The Analysis mode is unsupported */
    IMMOTION_ERR_UNSUPPORTED_ANALYSIS_MODE,
    /*! The Blend mode is invalid */
    IMMOTION_ERR_INVALID_BLEND_MODE,
    /*! The Blend mode is unsupported */
    IMMOTION_ERR_UNSUPPORTED_BLEND_MODE,
    /*! The Mask mode is invalid */
    IMMOTION_ERR_INVALID_MASK_MODE,
    /*! The Mask mode is unsupported */
    IMMOTION_ERR_UNSUPPORTED_MASK_MODE,
    /*! The initial display region is not within frame bounds */
    IMMOTION_ERR_INVALID_INITIAL_DISPLAY_REGION,
    /*! The provided callback is invalid */
    IMMOTION_ERR_INVALID_CALLBACK,
    /*! No reference frame is set */
    IMMOTION_ERR_NO_REFERENCE,
    /*! The provided process queue full policy is invalid */
    IMMOTION_ERR_INVALID_QUEUE_FULL_POLICY,
    /*! The close policy is invalid */
    IMMOTION_ERR_INVALID_CLOSE_POLICY,
    /*! Returned in non-blocking mode to indicate the internal queue is full */
    IMMOTION_INTERNAL_QUEUE_FULL,
    /*! Returned for frames interrupted during close */
    IMMOTION_INTERRUPTED_FRAME,
    /*! Number of return codes */
    IMMOTION_NUM_RETURN_CODES,
    _32BIT_PLACEHOLDER_immotion_return_code_t = 0x7fffffff
} immotion_return_code_t;

/*!
 * ImMotion frame formats
 * @caveats Not all are necessarily supported
 */
typedef enum {
    /*! NV12 Linear */
    IMMOTION_NV12_LINEAR,
    /*! P010 Linear */
    IMMOTION_P010_LINEAR,
    /*! TP10 Linear */
    IMMOTION_TP10_LINEAR,
    /*! NV12 UBWC */
    IMMOTION_NV12_UBWC,
    /*! P010 UBWC */
    IMMOTION_P010_UBWC,
    /*! TP10 UBWC */
    IMMOTION_TP10_UBWC,
    /*! Number of frame formats */
    IMMOTION_NUM_FRAME_FORMATS,
    _32BIT_PLACEHOLDER_immotion_frame_format_t = 0x7fffffff
} immotion_frame_format_t;

/*!
 * ImMotion Analysis modes
 */
typedef enum {
    /*! ImMotion will only stabilize the frames. */
    IMMOTION_STABILIZE_ONLY,
    /*!
     * ImMotion will apply extra processing which will be used in Blend if
     * mask mode SEGMENT_MASK is also enabled.
     */
    IMMOTION_FULL_ANALYSIS,

    IMMOTION_NUM_ANALYSIS_MODES,
    _32BIT_PLACEHOLDER_immotion_analysis_mode_t = 0x7fffffff
} immotion_analysis_mode_t;

/*!
 * ImMotion Blend modes
 */
typedef enum {
    /*! Freeze outside the mask */
    IMMOTION_MOVE,
    /*! Freeze inside the mask */
    IMMOTION_FREEZE,

    IMMOTION_NUM_BLEND_MODES,
    _32BIT_PLACEHOLDER_immotion_blend_mode_t = 0x7fffffff
} immotion_blend_mode_t;

/*!
 * ImMotion Mask modes
 */
typedef enum {
    /*!
     * ImMotion will expect the segmentation mask during the reference frame
     * BlendProcess call as well as the persist buffer on a BlendOpen call.
     */
    IMMOTION_SEGMENT_MASK,
    /* ImMotion will expect an external mask on each BlendProcess call. */
    IMMOTION_CONTINUOUS_MASKS,

    IMMOTION_NUM_MASK_MODES,
    _32BIT_PLACEHOLDER_immotion_mask_mode_t = 0x7fffffff
} immotion_mask_mode_t;


/*!
 * ImMotion internal queue full policies
 */
typedef enum {
    /*! ImMotion will block the calling thread when the internal queue is full */
    IMMOTION_QUEUE_FULL_POLICY_BLOCK,
    /*! ImMotion will immediately return if the internal queue is full with no action */
    IMMOTION_QUEUE_FULL_POLICY_RETURN,
    /*! Number of queue full policies */
    IMMOTION_NUM_QUEUE_FULL_POLICIES,
    _32BIT_PLACEHOLDER_immotion_queue_full_policy_t = 0x7fffffff
} immotion_queue_full_policy_t;

/*!
 * ImMotion Close policies
 */
typedef enum {
    /*! ImMotion will kill all queued unprocessed frames before closing */
    IMMOTION_CLOSE_POLICY_KILL_UNPROCESSED,
    /*! ImMotion will wait until all queued unprocessed are finished before closing */
    IMMOTION_CLOSE_POLICY_KILL_NONE,
    /*! Number of ImMotion Close policies */
    IMMOTION_NUM_CLOSE_POLICIES,
    _32BIT_PLACEHOLDER_immotion_close_policy_t = 0x7fffffff
} immotion_close_policy_t;

/*!
 * Frame resolution
 */
typedef struct {
    /*! Frame width */
    uint32_t width;
    /*! Frame height */
    uint32_t height;
} immotion_frame_resolution_t;

/*!
 * Frame parameters including format, resolution, strides and scanlines
 */
typedef struct {
    /*! Frame format */
    immotion_frame_format_t format;
    /*! Frame resolution */
    immotion_frame_resolution_t resolution;
    /*! Frame stride, ignored on UBWC frames */
    uint32_t stride;
    /*!
     * Number of scanlines between Luma start and Chroma start. The stride of
     * each scanline is the same as the Luma stride. If there is no offset
     * between Luma and Chroma, then this should equal the frame height.
     * Ignored on UBWC frames.
     */
    uint32_t num_scanlines;
} immotion_frame_params_t;

/*!
 * Generic rectangle structure
 */
typedef struct immotion_rect_t {
    /*! Top left x coordinate */
    uint32_t x;
    /*! Top left y coordinate */
    uint32_t y;
    /*! Width of the rectangle */
    uint32_t width;
    /*! Height of the rectangle */
    uint32_t height;
} immotion_rect_t;

/*!
 * Buffer structure specifically for ROI rectangles.
 */
typedef struct immotion_roi_boxes_t {
    /*! Immotion_rect_t buffer pointer */
    immotion_rect_t* buf;
    /*! Number of ROI boxes */
    uint32_t size;
} immotion_roi_boxes_t;

/*!
 * Generic buffer structure. May be expressed in terms of fd or buffer pointer.
 * The fd is used if non-negative, the buffer is used otherwise. The buffer may
 * not be NULL if it is to be used.
 */
typedef struct {
    /*! Buffer file descriptor */
    int32_t fd;
    /*! Buffer pointer */
    uint8_t* ptr;
    /*! Allocated buffer size */
    uint32_t size;
    /*! Offset from the buffer pointer to true buffer start */
    uint32_t offset;
    /*!
     * Information to be passed along with the buffer object. Not used by
     * ImMotion.
     */
    void* cookie;
} immotion_buf_t;

/*!
 * Allocation information
 */
typedef struct {
    /*! The minimum required buffer size */
    uint32_t size;
    /*! The minimum required buffer alignment. It will be a power of 2. */
    uint32_t alignment;
} immotion_alloc_info_t;

typedef struct {
    struct {
        /*! Array of frame formats to be filled by ImMotion */
        immotion_frame_format_t* frame_formats;
        /*!
         * Number of entries in the frame formats array
         * @caveats Only up to this number of entries is filled
         */
        uint32_t frame_formats_size;
        /*!
         * Number of formats actually supported by ImMotion. This may be more
         * than the frame_formats_size if it was not large enough.
         */
        uint32_t num_frame_formats_supported;
    };

    // For Analysis mode, both STABILIZE_ONLY and FULL_ANALYSIS are supported.

    /*! The minimum supported frame dimension */
    uint32_t min_dimension;
    /*! The maximum supported frame dimension */
    uint32_t max_dimension;
} immotion_analysis_supported_info_t;

/*!
 * ImMotion supported information
 */
typedef struct {
    struct {
        /*! Array of frame formats to be filled by ImMotion */
        immotion_frame_format_t* frame_formats;
        /*!
         * Number of entries in the frame formats array
         * @caveats Only up to this number of entries is filled
         */
        uint32_t frame_formats_size;
        /*!
         * Number of formats actually supported by ImMotion. This may be more
         * than the frame_formats_size if it was not large enough.
         */
        uint32_t num_frame_formats_supported;
    };

    // For Blend modes, both FREEZE and MOVE are supported.

    // For Mask modes, both SEGMENT_MASK and CONTINUOUS_MASKS are supported.

    /*! The minimum supported frame dimension */
    uint32_t min_dimension;
    /*! The maximum supported frame dimension */
    uint32_t max_dimension;
} immotion_blend_supported_info_t;

/*!
 * ImMotion Init parameters
 */
typedef struct {
    uint32_t reserved;
} immotion_init_params_t;

/*!
 * ImMotion AnalysisOpen parameters. These parameters configure a run of
 * ImMotion Analysis appropriately.
 */
typedef struct {
    /*! Input frame parameters */
    immotion_frame_params_t frame_params_in;
    /*! Output frame parameters */
    immotion_frame_params_t frame_params_out;
    /*! ImMotion Analysis mode */
    immotion_analysis_mode_t analysis_mode;
    /*!
     * The initial region that ImMotion will display. The area outside this
     * region will have black bars inserted. The display region parameters may
     * be set to 0 to indicate that the whole frame should be active. This can
     * be used to avoid border creep up to this specified region.
     */
    immotion_rect_t initial_display_region;
    /*!
     * The queue management policy that ImMotion will use when the internal
     * queue is full.
     */
    immotion_queue_full_policy_t queue_full_policy;
    /*! The callback to be invoked after each process call completes. */
    immotion_analysis_process_callback_t callback;
} immotion_analysis_open_params_t;

/*!
 * ImMotion BlendOpen parameters. These parameters configure a run of ImMotion
 * Blend appropriately.
 */
typedef struct {
    /*! Input frame parameters */
    immotion_frame_params_t frame_params_in;
    /*! Output frame parameters */
    immotion_frame_params_t frame_params_out;
    /*!
     * Mask frame parameters. On either mask mode, there will be at least one
     * mask for ImMotion.
     */
    immotion_frame_params_t frame_params_mask;
    /*! ImMotion Blend mode */
    immotion_blend_mode_t blend_mode;
    /*! ImMotion Mask mode */
    immotion_mask_mode_t mask_mode;
    /*!
     * The initial region that ImMotion will display. The area outside this
     * region will have black bars inserted. The display region parameters may
     * be set to 0 to indicate that the whole frame should be active. This can
     * be used to avoid border creep up to this specified region.
     *
     * @caveats If mask mode is SEGMENT_MASK, this is ignored. The display
     * region determined at the end of the Analysis pass will be used instead.
     */
    immotion_rect_t initial_display_region;
    /*!
     * The queue management policy that ImMotion will use when the internal
     * queue is full.
     */
    immotion_queue_full_policy_t queue_full_policy;
    /*! The callback to be invoked after each process call completes. */
    immotion_blend_process_callback_t callback;
} immotion_blend_open_params_t;

/*!
 * ImMotion AnalysisProcess parameters.
 */
typedef struct {
    /*!
     * If true, causes the process call to use the this frame as the reference
     * frame for subsequent ImMotion AnalyisProcess calls.
     */
    immotion_bool_t ref;
    /*!
     * Possible extra arguments available during the callback. These are simply
     * copied over and provided to the callback through this structure.
     */
    void* callback_args;
} immotion_analysis_process_params_t;

/*!
 * ImMotion BlendProcess parameters.
 */
typedef struct {
    /*!
     * If true, causes the process call to use the this frame as the reference
     * frame for subsequent ImMotion blend process calls.
     *
     * @caveats If mask mode is SEGMENT_MASK, another frame in the sub-sequence
     *          of frames processed during Analysis should be used. ImMotion
     *          will not return an error if a non-sub-sequence frame is used,
     *          but the results may not be sane.
     */
    immotion_bool_t ref;
    /*!
     * Mask information provided to ImMotion.
     *
     * @caveats If mask mode is SEGMENT_MASK, this mask is only used when the
     *          ref flag is set. Otherwise it is ignored. If mask mode is
     *          CONTINUOUS_MASKS, this mask is required every frame.
     */
    immotion_buf_t mask;

    /*
     * Regions of interest for blending.
     *
     * @caveats This is only used on the reference frame of a BlendProcess with
     *          SEGMENT_MASK mode, and is ignored for all non-reference frames
     *          and all CONTINUOUS_MASK mode frames.
     */
    immotion_roi_boxes_t roi_boxes;
    /*!
     * Possible extra arguments available during the callback. These are simply
     * copied over and provided to the callback through this structure.
     */
    void* callback_args;
} immotion_blend_process_params_t;

/*!
 * ImMotion fallback indicators from Analysis.
 */
typedef struct {
    /*!
     * Indicates that the fallback indicators are strong enough so that
     * the current frame was not processed in an acceptable manner. It
     * may be entirely invalid so that the current frame's result should
     * be discarded and processing discontinued for future frames without
     * a new reference.
     */
    immotion_bool_t error;
    /*! Indicates that the motion is too large */
    immotion_bool_t large_motion;
    /*! Indicates that stabilization has not performed well */
    immotion_bool_t poor_stabilization;
    /*! Indicates that exposure change is too excessive */
    immotion_bool_t large_exposure_change;
    /*! Indicates that features were tracked poorly */
    immotion_bool_t poor_features;
    /*! Indicates that there are no reliable features to track */
    immotion_bool_t no_motion_vectors;
    /*!
     * This is set when the motion is such that the initial display region
     * (which would have been hidden) is about to be exceeded. This indicates
     * that the black bar region has shrunk beyond what was originally specified.
     * This never results in the error parameter being set but can be used to
     * inform app policy.
     */
    immotion_bool_t initial_display_region_exceeded;
} immotion_fallback_t;

/*
 * ImMotion AnalysisProcess returned information
 */
typedef struct {
    /*!
     * The return code from the Process call. If valid, the next two fields
     * contain valid information.
     */
    int32_t ret;
    /*! The current display region */
    immotion_rect_t active_display_region;
    /*! Fallback indicators */
    immotion_fallback_t fallback;
} immotion_analysis_process_info_t;

/*
 * ImMotion BlendProcess returned information
 */
typedef struct {
    /*!
     * The return code from the Process call.
     */
    int32_t ret;
} immotion_blend_process_info_t;

/*!
 * Information that AnalysisProcess will provide to the callback when it is
 * invoked.
 */
struct immotion_analysis_process_callback_info_t {
    /*! Input frame, this is a copy of what was passed to the Process call */
    immotion_buf_t input;
    /*! Process parameters, this is a copy of what was passed to the Process call */
    immotion_analysis_process_params_t params;
    /*! Output frame, this is a copy of what was passed to the Process call */
    immotion_buf_t output;
    /*! Process return information, this is provided by ImMotion */
    immotion_analysis_process_info_t process_info;
};

/*!
 * Information that BlendProcess will provide to the callback when it is
 * invoked.
 */
struct immotion_blend_process_callback_info_t {
    /*! Input frame, this is a copy of what was passed to the Process call */
    immotion_buf_t input;
    /*! Process parameters, this is a copy of what was passed to the Process call */
    immotion_blend_process_params_t params;
    /*! Output frame, this is a copy of what was passed to the Process call */
    immotion_buf_t output;
    /*! Process return information, this is provided by ImMotion */
    immotion_blend_process_info_t process_info;
};

/*!
 * Parameters for AnalysisClose.
 */
typedef struct {
    /*! Close policy */
    immotion_close_policy_t close_policy;
} immotion_analysis_close_params_t;

/*!
 * Parameters for BlendClose.
 */
typedef struct {
    /*! Close policy */
    immotion_close_policy_t close_policy;
} immotion_blend_close_params_t;

/*!
 * Information that AnalysisClose will provide to the calling thread when done.
 */
typedef struct {
    /*! This is the final display region from the last successfully processed frame */
    immotion_rect_t final_display_region;
    struct {
        /*!
         * This is a user-allocated array whose entries are the frames
         * interrupted due to calling Close. Frames can be interrupted if the
         * IMMOTION_CLOSE_POLICY_KILL_UNPROCESSED is used. May be set to NULL
         * if no information is required.
         */
        immotion_analysis_process_callback_info_t* interrupted_arr;
        /*!
         * Number of entries provided in the interrupted information array.
         * Only up to this many entries are filled. The maximum size needed is
         * IMMOTION_MAX_CLOSE_ARR_ALLOC.
         */
        uint32_t interrupted_arr_size;
        /*!
         * Number of frames actually interrupted by ImMotion. This may be more
         * than the interrupted array size if it was not large enough.
         */
        uint32_t num_interrupted;
    };
} immotion_analysis_close_info_t;

/*!
 * Information that BlendClose will provide to the calling thread when done.
 */
typedef struct {
    struct {
        /*!
         * This is a user-allocated array whose entries are the frames
         * interrupted due to calling Close. Frames can be interrupted if the
         * IMMOTION_CLOSE_POLICY_KILL_UNPROCESSED is used. May be set to NULL
         * if no information is required.
         */
        immotion_blend_process_callback_info_t* interrupted_arr;
        /*!
         * Number of entries provided in the interrupted information array.
         * Only up to this many entries are filled. The maximum size needed is
         * IMMOTION_MAX_CLOSE_ARR_ALLOC.
         */
        uint32_t interrupted_arr_size;
        /*!
         * Number of frames actually interrupted by ImMotion. This may be more
         * than the interrupted array size if it was not large enough.
         */
        uint32_t num_interrupted;
    };
} immotion_blend_close_info_t;

/*!
 * ImMotion Term parameters
 */
typedef struct {
    uint32_t reserved;
} immotion_term_params_t;

/***************************************************************************
 * Function Prototypes
 ***************************************************************************/

/*!
 * @brief       Get supported parameters for ImMotion Analysis
 *
 * @description Provides all the parameters supported by ImMotion as defined in
 *              the immotion_supported_info_t structure. This does not require
 *              a session of ImMotion to be initialized.
 *
 * @output      info           The ImMotion supported parameters structure to be
 *                             populated for Analysis.
 *
 * @return      IMMOTION_OK    if parameters filled successfully, else a code
 *                             defined in immotion_return_code_t.
 */
int32_t immotion_GetAnalysisSupportedParameters(immotion_analysis_supported_info_t* info);

/*!
 * @brief       Get supported parameters for ImMotion Blend
 *
 * @description Provides all the parameters supported by ImMotion as defined in
 *              the immotion_supported_info_t structure. This does not require
 *              a session of ImMotion to be initialized.
 *
 * @output      info           The ImMotion supported parameters structure to be
 *                             populated for Blend.
 *
 * @return      IMMOTION_OK    if parameters filled successfully, else a code
 *                             defined in immotion_return_code_t.
 */
int32_t immotion_GetBlendSupportedParameters(immotion_blend_supported_info_t* info);

/*!
 * @brief       Initializes a session of ImMotion
 *
 * @description Initialize a session of ImMotion and outputs a context. This
 *              context must be used when calling subsequent ImMotion methods.
 *
 * @input       params         The parameters to configure the Init with.
 * @output      ctx            The context to be used for subsequent calls. The
 *                             context will be marked NULL if the method fails.
 *
 * @return      IMMOTION_OK    if successful, else a code defined in
 *                             ImMotion_return_code_t.
 */
int32_t immotion_Init(immotion_init_params_t params,
                      immotion_context_t* ctx);

/*!
 * @brief       Analysis scratch buffer requirements
 *
 * @description Determines the allocation requirements of the scratch buffer
 *              that ImMotion will need to use for a particular opened session
 *              of Analysis. This information depends on the input and output
 *              frame parameters provided in the Analysis Open call, and may be
 *              reused for the same context if they do not change.
 *
 * @input       ctx            A valid context that was returned via an Init
 *                             call.
 * @input       params         The analysis open parameters that ImMotion will
 *                             be configured with.
 * @output      info           The information to be used for allocation
 *                             purposes. The returned size may be 0.
 *
 * @return      IMMOTION_OK    if opened successfully, else a code defined in
 *                             ImMotion_return_code_t.
 */
int32_t immotion_GetAnalysisScratchBufRequirements(immotion_context_t ctx,
                                                   immotion_analysis_open_params_t params,
                                                   immotion_alloc_info_t* info);

/*!
 * @brief       Blend scratch buffer requirements
 *
 * @description Determines the allocation requirements of the scratch buffer
 *              that ImMotion will need to use for a particular opened session
 *              of Blend. This information depends on the input and output frame
 *              parameters provided in the Blend Open call, and may be reused
 *              for the same context if they do not change.
 *
 * @input       ctx            A valid context that was returned via an Init
 *                             call.
 * @input       params         The blend open parameters that ImMotion will be
 *                             configured with.
 * @output      info           The information to be used for allocation
 *                             purposes. The returned size may be 0.
 *
 * @return      IMMOTION_OK    if opened successfully, else a code defined in
 *                             ImMotion_return_code_t.
 */
int32_t immotion_GetBlendScratchBufRequirements(immotion_context_t ctx,
                                                immotion_blend_open_params_t params,
                                                immotion_alloc_info_t* info);

/*!
 * @brief       Persist buffer requirements
 *
 * @description Determines the allocation requirements of the persist buffer
 *              that ImMotion will need to use for a particular opened session.
 *              This information depends on the analysis mode and input frame
 *              resolution provided in the Analysis Open call, and may be reused
 *              for the same context if they do not change.
 *
 * @input       ctx            A valid context that was returned via an Init
 *                             call.
 * @input       params         The open parameters that ImMotion Analysis will
 *                             be configured with.
 * @output      info           The information to be used for allocation
 *                             purposes. The returned size may be 0.
 *
 * @return      IMMOTION_OK    if opened successfully, else a code defined in
 *                             ImMotion_return_code_t.
 */
int32_t immotion_GetPersistBufRequirements(immotion_context_t ctx,
                                           immotion_analysis_open_params_t params,
                                           immotion_alloc_info_t* info);

/*!
 * @brief       Configures a session of ImMotion for Analysis
 *
 * @description Configures a session of ImMotion with application specific
 *              parameters for the Analysis pass. This function must complete
 *              successfully before AnalysisProcess calls can be issued. It is
 *              an error to attempt to open the same context twice if it is not
 *              closed first.
 *
 * @input       ctx            A valid context that was returned via an Init
 *                             call.
 * @input       params         The parameters to configure ImMotion Analysis
 *                             with.
 * @input       scratch        A scratch buffer for use by ImMotion meeting the
 *                             requirements of
 *                             GetAnalysisScratchBufRequirements. ImMotion will
 *                             not use this buffer after Close. This may be NULL
 *                             if GetAnalysisScratchBufRequirements returns a
 *                             size of 0.
 * @input       persist        A persistent buffer for use by ImMotion meeting
 *                             the requirements of GetPersistBufRequirements.
 *                             This buffer must also be passed to the blend
 *                             call. This may be NULL if
 *                             GetPersistBufRequirements returns a size of 0.
 * @return      IMMOTION_OK    if opened successfully, else a code defined in
 *                             ImMotion_return_code_t.
 */
int32_t immotion_AnalysisOpen(immotion_context_t ctx,
                              immotion_analysis_open_params_t params,
                              immotion_buf_t* scratch,
                              immotion_buf_t* persist);

/*!
 * @brief       Processes a frame
 *
 * @description This function will process a frame of the ImMotion input
 *              sequence. It is an asynchronous function which will signal that
 *              the output is ready via a callback. The provided frame may
 *              either be taken as the reference or processed using a previous
 *              frame as the reference. This is controlled via the ref flag in
 *              params. If it is taken as the reference, it is returned to the
 *              output possibly with black bars inserted corresponding to the
 *              initial display region. Otherwise, it is processed with the
 *              ImMotion effect. There must always be a reference frame, so the
 *              very first call to this function must have the ref flag set or
 *              it is in error. Additionally, a call to this function which sets
 *              the ref flag must succeed. If it does not, no subsequent frames
 *              may be processed until a reference call succeeds. A failure in
 *              processing a single frame with the ref flag unset may cause
 *              subsequent frames to become poor and so should not be continued
 *              without establishing a new reference first although this is not
 *              enforced. If an error is returned directly from this function,
 *              no callback will be received and the output will be invalid. An
 *              error may also occur at the callback stage where the specific
 *              error is passed through the process info structure to the
 *              callback. If an error is received at the callback stage, the
 *              output frame will be invalid, and the fallback and process info
 *              received will also be invalid with the exception of the return
 *              code itself. This function may only be called on an Open
 *              ImMotion session.
 *
 * @input       ctx                 A valid context that was returned via an
 *                                  Init call.
 * @input       input               The input frame information.
 * @input       params              The parameters to configure the current
 *                                  Process call with.
 * @input       output              The output frame information.
 *
 * @return      IMMOTION_OK         if parameters filled succesfully, else a
 *                                  code defined in ImMotion_return_code_t.
 */
int32_t immotion_AnalysisProcess(immotion_context_t ctx,
                                 immotion_buf_t input,
                                 immotion_analysis_process_params_t params,
                                 immotion_buf_t output);

/*!
 * @brief       Closes a session of ImMotion Analysis.
 *
 * @description Closes the provided ImMotion Analysis session in order to allow
 *              it to be configured with different parameters or terminated.
 *              This function must be called before AnalysisOpen is called
 *              again. It is possible to call this function when the ImMotion
 *              session is already closed, in which case it will have no effect.
 *
 * @input       ctx            A valid context that was returned via an Init
 *                             call.
 * @input       params         The parameters configuring the Close call.
 * @inout       info           The information returned by the Close including
 *                             the final display region as well as any
 *                             interrupted frames as a result of the Close.
 *                             This pointer may not be NULL.
 *
 * @return      IMMOTION_OK    if successful, else a code defined in
 *                             immotion_return_code_t.
 */
int32_t immotion_AnalysisClose(immotion_context_t ctx,
                               immotion_analysis_close_params_t params,
                               immotion_analysis_close_info_t* info);

/*!
 * @brief       Generates a mask of blobs for the user to choose from.
 *
 * @description Generates a mask of blobs after finishing the Analysis pass
 *              using an external segmentation mask. This may be used to display
 *              suitable regions to freeze or keep in motion during the Blend
 *              stage. This must be called after Analysis close. The resolution
 *              of the external segmentation mask must match the output mask
 *              resolution and the resolution used during the Analysis pass.
 *
 * @input       ctx                  A valid context that was returned via an
 *                                   Init call.
 * @input       seg_mask             An external segmentation mask.
 * @input       seg_mask_params      The frame parameters for the mask.
 * @input       seg_blobs            The output mask. Pixels within the blob
 *                                   regions will be filled with a value of 255.
 *                                   Pixels outside these regions will be
 *                                   zeroed.
 * @input       seg_blobs_params     The frame parameters of the output mask.
 * @return      IMMOTION_OK          if successful, else a code defined in
 *                                   immotion_return_code_t.
 */
int32_t immotion_GetSegBlobs(immotion_context_t ctx,
                             immotion_buf_t seg_mask,
                             immotion_frame_params_t seg_mask_params,
                             immotion_buf_t seg_blobs,
                             immotion_frame_params_t seg_blobs_params,
                             immotion_buf_t persist);

/*!
 * @brief       Generates a mask of the selected blob.
 *
 * @description Generates a mask of the selected blob. The selected blobs are
 *              determined via the provided ROI rectangles. This must be called
 *              after immotion_GetSegBlobs. The selected_seg_blobs can then be
 *              used to run Blend in CONTINUOUS_MASKS mode in order to save on
 *              processing time. The resolution of the input blob mask must
 *              match the output mask resolution and the resolution used during
 *              the Analysis pass.
 *
 * @input       ctx                          A valid context that was returned
 *                                           via an Init call.
 * @input       seg_blobs                    The input blob mask. This is the
 *                                           blob mask generated by
 *                                           immotion_GetSegBlobs.
 * @input       seg_blobs_params             The frame parameters for the
 *                                           seg_blobs.
 * @input       selected_seg_blobs           The output mask. Pixels within the
 *                                           selected blob region will be filled
 *                                           with a value of 255. Pixels outside
 *                                           this region will be zeroed. The
 *                                           selected blob region will be
 *                                           slightly dilated.
 * @input       selected_seg_blob_params     The frame parameters of the output
 *                                           mask.
 * @input       rois                         User chosen ROI rectangles.
 * @input       persist                      The persist buffer that was used
 *                                           for Analysis in FULL_ANALYSIS mode.
 * @return      IMMOTION_OK                  if successful, else a code defined
 *                                           in immotion_return_code_t.
 */
int32_t immotion_GetSelectedSegBlobs(immotion_context_t ctx,
                                     immotion_buf_t seg_blobs,
                                     immotion_frame_params_t seg_blobs_params,
                                     immotion_buf_t selected_seg_blobs,
                                     immotion_frame_params_t selected_seg_blobs_params,
                                     immotion_roi_boxes_t rois,
                                     immotion_buf_t persist);

/*!
 * @brief       Configures a session of ImMotion for Blend
 *
 * @description Configures a session of ImMotion with application specific
 *              parameters for the Blend pass. This function must complete
 *              successfully before BlendProcess calls can be issued. It is an
 *              error to attempt to open the same context twice if it is not
 *              closed first.
 *
 * @input       ctx            A valid context that was returned via an Init
 *                             call.
 * @input       params         The parameters to configure ImMotion Blend with.
 * @input       scratch        A scratch buffer for use by ImMotion meeting the
 *                             requirements of
 *                             GetAnalysisScratchBufRequirements. ImMotion will
 *                             not use this buffer after Close. This may be NULL
 *                             if GetAnalysisScratchBufRequirements returns a
 *                             size of 0.
 * @input       persist        A persistent buffer for use by ImMotion meeting
 *                             the requirements of GetPersistBufRequirements.
 *                             This is the same buffer used in the Analysis
 *                             pass. This may be NULL if
 *                             GetPersistBufRequirements returns a size of 0.
 * @return      IMMOTION_OK    if opened successfully, else a code defined in
 *                             ImMotion_return_code_t.
 */
int32_t immotion_BlendOpen(immotion_context_t ctx,
                           immotion_blend_open_params_t params,
                           immotion_buf_t* scratch,
                           immotion_buf_t* persist);

/*!
 * @brief       Processes a frame
 *
 * @description This function will process a frame of the ImMotion input
 *              sequence. It is an asynchronous function which will signal that
 *              the output is ready via a callback. The provided frame may
 *              either be taken as the reference or processed using a previous
 *              frame as the reference. This is controlled via the ref flag in
 *              params. If it is taken as the reference, it is returned to the
 *              output possibly with black bars inserted corresponding to the
 *              initial display region. Otherwise, it is processed with the
 *              ImMotion effect. There must always be a reference frame, so the
 *              very first call to this function must have the ref flag set or
 *              it is in error. If the mask mode is SEGMENT_MASK, the segment
 *              mask must be provided on the same frame as the reference frame.
 *              If the mask mode is CONTINUOUS_MASKS, the external masks must be
 *              provided on every frame. Additionally, a call to this function
 *              which sets the ref flag must succeed. If it does not, no
 *              subsequent frames may be processed until a reference call
 *              succeeds. If an error is returned directly from this function,
 *              no callback will be received and the output will be invalid. An
 *              error may also occur at the callback stage where the specific
 *              error is passed through the process info structure to the
 *              callback. This function may only be called on an Open ImMotion
 *              session.
 *
 * @input       ctx                 A valid context that was returned via an
 *                                  Init call.
 * @input       input               The input frame information.
 * @input       params              The parameters to configure the current
 *                                  Process call with.
 * @input       output              The output frame information.
 *
 * @return      IMMOTION_OK         if parameters filled succesfully, else a
 *                                  code defined in ImMotion_return_code_t.
 */
int32_t immotion_BlendProcess(immotion_context_t ctx,
                              immotion_buf_t input,
                              immotion_blend_process_params_t params,
                              immotion_buf_t output);

/*!
 * @brief       Closes a session of ImMotion Blend.
 *
 * @description Closes the provided ImMotion Blend session in order to allow it
 *              to be configured with different parameters or terminated. This
 *              function must be called before BlendOpen is called again. It is
 *              possible to call this function when the ImMotion session is
 *              already closed, in which case it will have no effect.
 *
 * @input       ctx            A valid context that was returned via an Init
 *                             call.
 * @input       params         The parameters configuring the Close call.
 * @inout       info           The information returned by the Close including
 *                             the final display region as well as any
 *                             interrupted frames as a result of the Close.
 *                             This pointer may not be NULL.
 *
 * @return      IMMOTION_OK    if successful, else a code defined in
 *                             immotion_return_code_t.
 */
int32_t immotion_BlendClose(immotion_context_t ctx,
                            immotion_blend_close_params_t params,
                            immotion_blend_close_info_t* info);

/*!
 * @brief       Terminates a session of ImMotion
 *
 * @description Terminates a session of ImMotion and destroys the context. The
 *              context may no longer be used after this method returns. An open
 *              context may not be terminated before a Close call is issued.
 *
 * @input       ctx            A valid context that was returned via an Init
 *                             call.
 * @input       params         The parameters to configure the Term with.
 *
 * @return      IMMOTION_OK    if successful, else a code defined in
 *                             ImMotion_return_code_t.
 */
int32_t immotion_Term(immotion_context_t ctx,
                      immotion_term_params_t params);

#ifdef __cplusplus
}
#endif
#endif //_IMMOTION_API_H_
