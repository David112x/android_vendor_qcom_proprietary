////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __IS_INTERFACE_H__
#define __IS_INTERFACE_H__

#include <stdint.h>
#include <stdbool.h>
#include "ipdefs.h"
#include "NcLibWarpCommonDef.h"
#include "is_interface_error_codes.h"

/*****************************************************************************/
/*         Definitions and Macros                                            */
/*****************************************************************************/

#define IS_API_VERSION_MAJOR        1   /**< Major API version number */
#define IS_API_VERSION_MINOR        2   /**< Minor API version number */
#define IS_API_VERSION_STEP         0   /**< Step API version number */

#if defined(__GNUC__)
#define EIS_VISIBILITY_PUBLIC __attribute__ ((visibility ("default")))
#define EIS_VISIBILITY_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define EIS_VISIBILITY_PUBLIC __declspec(dllexport)
#define EIS_VISIBILITY_LOCAL
#endif // defined(__GNUC__)


#if (1 == IS_API_VERSION_MAJOR) && (2 == IS_API_VERSION_MINOR) && (0 == IS_API_VERSION_STEP)

/**
*   @addtogroup     OIS_Chromatix_translation
*
*   @brief          Translate existing reserved parameters to OIS Chromatix parameters
*
*  @{
*/

#ifndef ois__elf
#define ois__elf                     general.res_lut_param_1[0]
#endif /* ois__elf */

#ifndef ois__unit
#define ois__unit                    general.res_lut_param_1[1]
#endif /* ois__unit */

#ifndef ois__scale_x
#define ois__scale_x                 general.res_lut_param_1[2]
#endif /* ois__scale_x */

#ifndef ois__scale_y
#define ois__scale_y                 general.res_lut_param_1[3]
#endif /* ois__scale_y */

#ifndef ois__center_command_enable
#define ois__center_command_enable   general.res_lut_param_1[4]  /**< Enable OIS centering mechanism (option to center \ automatic modes) */
#endif /* ois__center_command_enable */

#ifndef ois__center_exposure_time_th
#define ois__center_exposure_time_th general.res_lut_param_1[5]    /**< If (exp_time > exposure_time_th) then OIS is set to automatic mode, otherwise OIS is fixed to center.
                                                                    *   Units are in microseconds.
                                                                    */
#endif /* ois__center_exposure_time_th */

#ifndef ois__center_response_time_ms
#define ois__center_response_time_ms general.res_lut_param_1[6]    /**< Response time of OIS centering mechanism between centering \ automatic
                                                                    *   operation mode.
                                                                    *   Units are in milliseconds.
                                                                    */
#endif /* ois_center_response_time_ms */

#ifndef ois__noise_floor
#define ois__noise_floor            general.res_lut_param_1[7]    /**< Place holder for ois_noise_floor for older C7 XML version */
#endif /* ois__noise_floor */

#ifndef top__lens_pos_orientation_axis_x
#define top__lens_pos_orientation_axis_x            top.res_lut_param_0[0]    /**< top__lens_pos_orientation_axis_x: 1,2,3 is what axis to pick and minus means minus signal */
#endif /* top__lens_pos_orientation_axis_x */

#ifndef top__lens_pos_orientation_axis_y
#define top__lens_pos_orientation_axis_y            top.res_lut_param_0[1]    /**< top__lens_pos_orientation_axis_y: 1,2,3 is what axis to pick and minus means minus signal */
#endif /* top__lens_pos_orientation_axis_y */

#ifndef top__lens_pos_orientation_axis_z
#define top__lens_pos_orientation_axis_z            top.res_lut_param_0[2]    /**< top__lens_pos_orientation_axis_z: 1,2,3 is what axis to pick and minus means minus signal */
#endif /* top__lens_pos_orientation_axis_z */

/** @} */

/**
*   @addtogroup     iFOVC_Chromatix_translation
*
*   @brief          Translate existing reserved parameters to iFOVC Chromatix parameters
*
*   @note           All other iFOVC calibration/tunning parameters will be placed here
*
*  @{
*/

#ifndef top__ifovc_frequency
#define top__ifovc_frequency            general.res_lut_param_3[0]    /**< Requested iFOVC sampling frequency in Hz. Has to match actual iFOVC sampling frequency. */
#endif /* top__ifovc_frequency */

#ifndef timing__ifovc_offset
#define timing__ifovc_offset            general.res_lut_param_1[9]    /**< Offset between iFOVC timing and SOF timing in microseconds */
#endif /* timing__ifovc_offset */

/** @} */

#endif /* API version is 1.2.0 */

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/*         Chromatix data structures                                         */
/*****************************************************************************/

typedef struct eis_top_s
{
    // minimal_total_margin == total margin_x == requested_total_margins_x
    // total margins ratio for x axis (physical + virtual), with respect to input image size and represent sum of both sides.
    // If physical margins are not sufficient, virtual margins will be used to get to the minimal_total_margin.
    // format: float
    float minimal_total_margin;

    // Requested total margins ratio for y axis, with respect to input image size and represent sum of both sides.
    // This is used for requesting physical margins and also to be sent to GeoLib.
    // Actual y margins would be between [minimal_total_margin, requested_total_margins_y]
    // format: float
    float requested_total_margins_y_fhd_30;

    // Requested total margins ratio for y axis, with respect to input image size and represent sum of both sides.
    // This is used for requesting physical margins and also to be sent to GeoLib.
    // Actual y margins would be between [minimal_total_margin, requested_total_margins_y]
    // format: float
    float requested_total_margins_y_fhd_60;

    // Requested total margins ratio for y axis, with respect to input image size and represent sum of both sides.
    // This is used for requesting physical margins and also to be sent to GeoLib.
    // Actual y margins would be between [minimal_total_margin, requested_total_margins_y]
    // format: float
    float requested_total_margins_y_fhd_120;

    // Requested total margins ratio for y axis, with respect to input image size and represent sum of both sides.
    // This is used for requesting physical margins and also to be sent to GeoLib.
    // Actual y margins would be between [minimal_total_margin, requested_total_margins_y]
    // format: float
    float requested_total_margins_y_4k_30;

    // Requested total margins ratio for y axis, with respect to input image size and represent sum of both sides.
    // This is used for requesting physical margins and also to be sent to GeoLib.
    // Actual y margins would be between [minimal_total_margin, requested_total_margins_y]
    // format: float
    float requested_total_margins_y_4k_60;

    // Requested total margins ratio for y axis, with respect to input image size and represent sum of both sides.
    // This is used for requesting physical margins and also to be sent to GeoLib.
    // Actual y margins would be between [minimal_total_margin, requested_total_margins_y]
    // format: float
    float requested_total_margins_y_4k_120;

    // Requested total margins ratio for y axis, with respect to input image size and represent sum of both sides.
    // This is used for requesting physical margins and also to be sent to GeoLib.
    // Actual y margins would be between [minimal_total_margin, requested_total_margins_y]
    // format: float
    float requested_total_margins_y_8k_30;

    // Requested total margins ratio for y axis, with respect to input image size and represent sum of both sides.
    // This is used for requesting physical margins and also to be sent to GeoLib.
    // Actual y margins would be between [minimal_total_margin, requested_total_margins_y]
    // format: float
    float requested_total_margins_y_8k_60;

    // Requested total margins ratio for y axis, with respect to input image size and represent sum of both sides.
    // This is used for requesting physical margins and also to be sent to GeoLib.
    // Actual y margins would be between [minimal_total_margin, requested_total_margins_y]
    // format: float
    float requested_total_margins_y_8k_120;

    // future looking buffer size
    // format: 16u
    uint32_t future_buffer_size_fhd_30;

    // future looking buffer size
    // format: 16u
    uint32_t future_buffer_size_fhd_60;

    // future looking buffer size
    // format: 16u
    uint32_t future_buffer_size_fhd_120;

    // future looking buffer size
    // format: 16u
    uint32_t future_buffer_size_4k_30;

    // future looking buffer size
    // format: 16u
    uint32_t future_buffer_size_4k_60;

    // future looking buffer size
    // format: 16u
    uint32_t future_buffer_size_4k_120;

    // future looking buffer size
    // format: 16u
    uint32_t future_buffer_size_8k_30;

    // future looking buffer size
    // format: 16u
    uint32_t future_buffer_size_8k_60;

    // future looking buffer size
    // format: 16u
    uint32_t future_buffer_size_8k_120;

    // Requested gyro sampling frequency in Hz. Has to match actual gyro sampling frequency.
    // format: 16u
    uint32_t gyro_frequency;

    // Requested OIS sampling frequency in Hz. Has to match actual OIS sampling frequency.
    // format: 16u
    uint32_t ois_frequency;

    // Requested accelerometer sampling frequency in Hz. Has to match actual accelerometer sampling frequency.
    // format: 16u
    uint32_t acc_frequency;

    // Requested magnetometer sampling frequency in Hz. Has to match actual magnetometer sampling frequency.
    // format: 16u
    uint32_t mag_frequency;

    // Top Reserved LUT parameter 0
    // format: float
    float res_lut_param_0[32];

} eis_top;

/** EIS general configurations */
typedef struct eis_general_s
{
    // Focal length in pixel units for horizontal size of 1920. This value is scaled for other resolutions.
    // format: 16u
    uint32_t focal_length;

    // Gyro noise floor [rad/sec]
    // format: float
    float gyro_noise_floor;

    // OIS noise floor [pixel normalized to 1920]
    // format: float
    float ois_noise_floor;

    // Accelerometer noise floor [m/s2]
    // format: float
    float acc_noise_floor;

    // Magnetometer noise floor [rad/sec]
    // format: float
    float mag_noise_floor;

    // Precision of the EIS output grid (will be overridden by 0 if HW not supports grid precision 1).
    //     0: 35x27 samples (945 samples are used).
    //     1: 67x51 samples (3417 samples are used).
    // format: 1u
    uint32_t output_grid_precision;

    // Reserved parameter 1
    // format: float
    float res_param_1;

    // Reserved parameter 2
    // format: float
    float res_param_2;

    // Reserved parameter 3
    // format: float
    float res_param_3;

    // Reserved parameter 4
    // format: float
    float res_param_4;

    // Reserved parameter 5
    // format: float
    float res_param_5;

    // Reserved parameter 6
    // format: float
    float res_param_6;

    // Reserved parameter 7
    // format: float
    float res_param_7;

    // Reserved parameter 8
    // format: float
    float res_param_8;

    // Reserved parameter 9
    // format: float
    float res_param_9;

    // Reserved parameter 10
    // format: float
    float res_param_10;

    // Reserved LUT parameter 1
    // format: float
    float res_lut_param_1[32];

    // Reserved LUT parameter 2
    // format: float
    float res_lut_param_2[32];

    // Reserved LUT parameter 3
    // format: 32u
    uint32_t res_lut_param_3[16];

} eis_general;

/** EIS timing related configurations */
typedef struct eis_timing_s
{
    // Offset between gyro timing and SOF timing in microseconds
    // format: 32s
    int32_t s3d_offset;

    // Offset between OIS angles timing and SOF timing in microseconds
    // format: 32s
    int32_t ois_offset;

    // Offset between accelerometer timing and SOF timing in microseconds
    // format: 32s
    int32_t acc_offset;

    // Offset between magnetometer timing and SOF timing in microseconds
    // format: 32s
    int32_t mag_offset;

} eis_timing;

/** EIS blur masking configurations */
typedef struct eis_blur_masking_s
{
    // Enable blur masking mechanism
    // format: 1u
    uint32_t enable;

    // Minimal stabilization strength for strong blur ( estimated_blur >= end_decrease_at_blur )
    // format: float
    float min_strength;

    // if (exp_time > exposure_time_th) then blur masking feature is enabled.
    // Otherwise disabled
    // Units are seconds
    // format: float
    float exposure_time_th;

    // Blur below this point won't decrease strength. Units are pixels out of 1920 resolutions.
    // Blur above this point will cause min_strength stabilization (in between start/end interpolate) Units are pixels out of 1920 resolutions.
    // If (end_decrease_at_blur>start_decrease_at_blur) then feature will be disabled.
    // format: float
    float start_decrease_at_blur;

    // Blur below this point won't decrease strength. Units are pixels out of 1920 resolutions.
    // Blur above this point will cause min_strength stabilization (in between start/end interpolate) Units are pixels out of 1920 resolutions.
    // If (end_decrease_at_blur>start_decrease_at_blur) then feature will be disabled.
    // format: float
    float end_decrease_at_blur;

    // Threshold for pan detection on the filter result [rad/sec].
    // Lower than pan_min_threshold is not a pan. Higher than pan_max_threshold is a full pan. In between the weight is linear.
    // Blur masking mechanism is only active when no pan is detected.
    // format: float
    float pan_min_threshold;

    // Threshold for pan detection on the filter result [rad/sec].
    // Lower than pan_min_threshold is not a pan. Higher than pan_max_threshold is a full pan. In between the weight is linear.
    // Blur masking mechanism is only active when no pan is detected.
    // format: float
    float pan_max_threshold;

    // blur_masking_res1
    // format: float
    float blur_masking_res1;

    // blur_masking_res2
    // format: float
    float blur_masking_res2;

} eis_blur_masking;

typedef struct eis_lens_distortion_correction_s
{
    // 0 = use grids defined here (local grid, only if ldc_grid_enable == 1, , otherwise no LDC)
    // 1 = use grids defined in ICA/LDC Chromatix (only if its valid, otherwise no LDC)
    // Comment: Usually take grids from ICA Chromatix unless the input FOV is different between Video with EIS or video without EIS. Or if the chip doesn't contains ICA Chromatix.
    // format: 1u
    uint32_t ldc_grid_source;

    // Enable/validity for the grids below (defined under EIS Chromatix)
    // format: 1u
    uint32_t ldc_grid_enable;

    // 0 = Invalid configuration for EIS local grid ( in ICA Chromatix it is calibration on full sensor FOV )
    // 1 = EIS local grid is calibrated on IFE Input  @ DZ X1 domain (FOV)
    // 2 = EIS local grid is calibrated on IFE Output @ DZ X1 domain (FOV)
    // Comment: EIS local grid means the grid defined below and not the one coming from ICA Chromatix
    // format: 2u
    uint32_t ldc_calib_domain;

    // LDC grid in2out. Grid is defined on IFE output at digital zoom X1. FOV includes the EIS margins (image could be 4:3 or any different ratio than 16:9).
    // format: 18sQ4
    int32_t distorted_input_to_undistorted_ldc_grid_x[3417];

    // LDC grid in2out. Grid is defined on IFE output at digital zoom X1. FOV includes the EIS margins (image could be 4:3 or any different ratio than 16:9).
    // format: 18sQ4
    int32_t distorted_input_to_undistorted_ldc_grid_y[3417];

    // Precision of the LDC in2out grid.
    //     0: 35x27 samples (945 samples are used).
    //     1: 67x51 samples (3417 samples are used).
    // format: 1u
    uint32_t distorted_input_to_undistorted_ldc_grid_geometry;

    // LDC grid out2in. Grid is defined on IFE output at digital zoom X1. FOV includes the EIS margins (image could be 4:3 or any different ratio than 16:9).
    // format: 18sQ4
    int32_t undistorted_to_distorted_input_ldc_grid_x[3417];

    // LDC grid out2in. Grid is defined on IFE output at digital zoom X1. FOV includes the EIS margins (image could be 4:3 or any different ratio than 16:9).
    // format: 18sQ4
    int32_t undistorted_to_distorted_input_ldc_grid_y[3417];

    // Precision of the LDC out2in grid.
    //     0: 35x27 samples (945 samples are used).
    //     1: 67x51 samples (3417 samples are used).
    // format: 1u
    uint32_t undistorted_to_distorted_input_ldc_grid_geometry;

    // 0 = custom/no model (grid is not by any of the following models model ), 1 = regular, 2= fisheye, 3 = reserved for a new model
    // format: 3u
    uint32_t ldc_model_type;

    // Model parameters to be used with the selected model.
    // Parameters interpretation per model:
    // For model 1: [k1 k2 p1 p2 k3]
    // For model 2: [k1 k2 k3 k4]
    // format: float
    float model_parameters[32];

    // Camera matrix \ Focal length that was extracted during camera calibration for LDC. Image sizes are as defined below and are not normalized and according to calibration images.
    // format: float
    float focal_length_x;

    // Camera matrix \ Focal length that was extracted during camera calibration for LDC. Image sizes are as defined below and are not normalized and according to calibration images.
    // format: float
    float focal_length_y;

    // Camera matrix \ Optical center that was extracted during camera calibration for LDC. Image sizes are as defined below and are not normalized and according to calibration images.
    // format: float
    float optical_center_x;

    // Camera matrix \ Optical center that was extracted during camera calibration for LDC. Image sizes are as defined below and are not normalized and according to calibration images.
    // format: float
    float optical_center_y;

    // Distorted image size, image used for LDC calibration
    // format: 16u
    uint32_t image_size_distorted_x;

    // Distorted image size, image used for LDC calibration
    // format: 16u
    uint32_t image_size_distorted_y;

    // Un-Distorted image size, image used for LDC calibration
    // format: 16u
    uint32_t image_size_undistorted_x;

    // Un-Distorted image size, image used for LDC calibration
    // format: 16u
    uint32_t image_size_undistorted_y;

} eis_lens_distortion_correction;

/** Chromatix configuration data structure */
typedef struct is_chromatix_info_s
{
    eis_top                         top;                        /**< EIS top level parameters configurations struct */
    eis_general                     general;                    /**< General Chromatix configuration struct */
    eis_timing                      timing;                     /**< Timing related Chromatix configuration struct */
    eis_blur_masking                blur_masking;               /**< Blur masking related Chromatix configuration struct */
    eis_lens_distortion_correction  lens_distortion_correction; /**< Lens distortion correction configuration struct */
} is_chromatix_info_t;

/*****************************************************************************/
/*         EIS Algorithm data structures                                     */
/*****************************************************************************/

/** Frame time data structure */
typedef struct frame_times_s
{
    uint64_t sof;                          /**< Start of frame in microseconds */
    uint32_t frame_time;                   /**< Total frame time in microseconds */
    uint32_t exposure_time;                /**< Exposure time in microseconds */
    uint32_t sensor_rolling_shutter_skew;  /**< sensor_rolling_shutter_skew in microseconds */
} frame_times_t;

/** Location of camera enumerator */
typedef enum CameraPosition_e
{
    CAM_REAR      = 0,        /**< Rear camera */
    CAM_FRONT     = 1,        /**< Front camera */
} CameraPosition;

/** EIS algorithm type */
typedef enum cam_is_type_e
{
    IS_TYPE_EIS_2_0 = 4,                    /**< EIS 2.x */
    IS_TYPE_EIS_NFL = IS_TYPE_EIS_2_0,      /**< EIS Non-Future looking */
    IS_TYPE_EIS_3_0 = 5,                    /**< EIS 3.x */
    IS_TYPE_EIS_FL = IS_TYPE_EIS_3_0        /**< EIS Future looking */
} cam_is_type_t;

/** EIS deployment type */
typedef enum cam_is_deployment_type_e
{
    DEP_TYPE_ICA_V20 = 0,   /**< EIS warp is done in ICA_V20 */
    DEP_TYPE_ICA_V30,       /**< EIS warp is done in ICA_V30 */
    DEP_TYPE_GPU_PRE,       /**< EIS warp is done in GPU pre IPE */
    DEP_TYPE_GPU_POST       /**< EIS warp is done in GPU post IPE/OPE */
} cam_is_deployment_type_t;

/** EIS algorithm initialization type */
typedef enum cam_is_operation_mode_e
{
    IS_OPT_REGULAR                          = 0,    /**< Regular operation mode, Stabilizer works as expected. */
    IS_OPT_CALIBRATION_SYMMETRIC            = 1,    /**< @deprecated */
    IS_OPT_CALIBRATION_ASYMMETRIC           = 2,    /**< Calibration mode (asymmetric Downscale) - Stabilizer won't act but instead downscale
                                                     *   Input image to output image size for later tuning or simulations.
                                                     *   Downscale is asymmetric for both axes. In case of different margins
                                                     *   per axis there will be need for different upscale per axis before simulations or tuning.
                                                     */

    IS_OPT_CALIBRATION_ASYMMETRIC_ROBUST    = 3,    /**< Same as IS_OPT_CALIBRATION_ASYMMETRIC but no return even if have
                                                     *   internal fail due to invalid input etc.
                                                     */

    IS_OPT_UNDISTORTION_ONLY                = 10,   /**< Un-Distortion only mode, no stabilization will be applied.
                                                     *   Outputs Grid will correct only rolling shutter skew and lens distortion.
                                                     */
    IS_OPT_IDENTITY_ONLY_ROBUST = 11,                  /**< Output identity matrix. Will also output the OIS center command and CVP alignment.
                                                     */

    IS_OPT_TEST_SAT_PASS_THROUGH            = 101,   /**< Test Mode : SAT matrix pass through */
    IS_OPT_TEST_MARGINS_LIMITS              = 102,   /**< Test Mode : Margin limit */
} cam_is_operation_mode_t;

typedef enum cam_is_ois_mode_e
{
    IS_OIS_MODE_INVALID     = 0,                /**< Output OIS mode is invalid and should be ignored by CamX node */
    IS_OIS_MODE_CENTER      = 1,                /**< OIS mode should be switched to center */
    IS_OIS_MODE_AUTOMATIC   = 2                 /**< OIS mode should be switched to automatic operation mode */
}cam_is_ois_mode;


/** single sample structure, contains information per axis and time-stamp */
typedef struct _sample_data_s
{
    double      data[4];    /**< 0==x, 1==y, 2==z, 3==w */
    uint64_t    ts;         /**< time stamp in microseconds */
} sample_data_t;

/** Samples array structures */
typedef struct _samples_data_s
{
    sample_data_t*  samples;        /**< Per axis data array */
    uint32_t        num_elements;   /**< Number of elements in the buffer */
} samples_data_t;

typedef samples_data_t  gyro_data_t;            /**< Gyro samples array. Measures device angular velocity [Rad/sec] */
typedef samples_data_t  acc_data_t;             /**< Accelerometer samples array. Measures device acceleration [meter/sec^2] */
typedef samples_data_t  mag_data_t;             /**< Magnetometer samples array. Measures device acceleration [Volt * sec/meter^2] */
typedef samples_data_t  ois_data_t;             /**< OIS samples array. */
typedef samples_data_t  ifovc_data_t;           /**< iFOVC samples array. */

typedef samples_data_t  linear_acc_data_t;      /**< Linear accelerometer samples array. Measures device acceleration [meter/sec^2] */
typedef samples_data_t  abs_orientation_data_t; /**< Absolute orientation samples array. */
typedef samples_data_t  rel_orientation_data_t; /**< Relative orientation samples array. */


/** Samples interval data structure */
typedef struct samples_times_s
{
    uint64_t first_ts;  /**< First sample timestamps [microseconds] in a batch of samples */
    uint64_t last_ts;   /**< Last sample timestamps [microseconds] in a batch of samples */
} samples_times_t;

typedef samples_times_t  gyro_times_t;      /** Gyro interval data structure */
typedef samples_times_t  ois_times_t;       /** OIS interval data structure */
typedef samples_times_t  ifovc_times_t;     /** iFOVC interval data structure */

/** Time intervals data structure */
typedef struct is_time_intervals_s
{
    gyro_times_t    gyro;       /**< Gyro interval  */
    ois_times_t     ois;        /**< OIS interval   */
    ifovc_times_t   ifovc;      /**< iFOVC interval */
} is_time_intervals;

typedef enum LDCCalibDomian_e
{
    DOMAIN_FULL_SENSOR     = 0,
    DOMAIN_IFE_INPUT       = 1, /**< a.k.a DOMAIN_SENSOR_OUTPUT per sensor mode */
    DOMAIN_EIS_INPUT_DZX1  = 2, /**< a.k.a DOMAIN_EIS_INPUT_DZX1.*/
    DOMAIN_IFE_OUTPUT_DZX1 = 2  /**< a.k.a DOMAIN_IFE_OUTPUT_DZX1. Only supported configuration. Need to sample grid by GeoLib to get this. */
} LDCCalibDomian;

/** WindowRegionF - Crop region of interest*/
typedef struct WindowRegionF_s
{
    float    fullWidth;
    float    fullHeight;
    float    windowWidth;
    float    windowHeight;
    float    windowLeft;
    float    windowTop;
} WindowRegionF;

/** Per sensor configuration data structure */
typedef struct is_init_data_sensor_s
{
    /**  @addtogroup    LDC_grids
      *  @brief         Lens Distortion Correction grids from ICA Chromatix
      *                 Must be defined on FOV of EIS Input for zoom X1 (as sampled by GeoLib to that FOV at init).
      *                 FOV dimension: is_input_frame_width X is_input_frame_height
      *
      * @warning        This will be used only in case enable bit in EIS Chromatix lens_distortion_correction struct is set to 1 and ldc_grid_source is set to 1
      *
      *  @{
      */
    const NcLibWarpGrid* ldc_in2out;        /**< input to output lens distortion correction grid */
    const NcLibWarpGrid* ldc_out2in;        /**< output to input lens distortion correction grid */
    LDCCalibDomian       ldc_calib_domain;  /**< 0 = full sensor; 1 = ife input; 2 = ife output at digital zoom X1 */

    /** @} */

    WindowRegionF image_sensor_crop;            /**< Sensor crop, output FOV of sensor vs full FOV of sensor */

    uint32_t ife_input_frame_width;                 /**< IFE input frame width  == Sensor output width  */
    uint32_t ife_input_frame_height;                /**< IFE input frame height == Sensor output height */
    uint32_t ife_input_frame_undistorted_width;     /**< IFE input frame width in undistorted domain    */
    uint32_t ife_input_frame_undistorted_height;    /**< IFE input frame height in undistorted domain   */

    uint32_t is_input_frame_width;               /**< EIS input frame width  */
    uint32_t is_input_frame_height;              /**< EIS input frame height */

    uint32_t sensor_mount_angle;            /**< Sensor mount angle (0, 90, 180, 270) */
    CameraPosition camera_position;         /**< Camera position (front, back, etc.) */

    float optical_center_x;                 /**< Lens optical center shift w.r.t. input image origin defined on image center in x-axis direction [pixels of input image] */
    float optical_center_y;                 /**< Lens optical center shift w.r.t. input image origin defined on image center in y-axis direction [pixels of input image] */

    is_chromatix_info_t is_chromatix_info;  /**< Chromatix configurations */

} is_init_data_sensor;

/** Common configurations for all sensors data structure */
typedef struct is_init_data_common_s
{
    cam_is_type_t            is_type;          /**< EIS algorithm version */
    cam_is_operation_mode_t  operation_mode;   /**< EIS operation mode */
    cam_is_deployment_type_t deployment_type;  /**< EIS stabilization transform deployment type.*/

    uint32_t is_output_frame_width;            /**< EIS output frame width after virtual margin upscale */
    uint32_t is_output_frame_height;           /**< EIS output frame height after virtual margin upscale  */

    uint16_t frame_fps;                        /**< Frame rate */

    bool do_virtual_upscale_in_transform;  /**< If true then upscale is done by ICA transform, otherwise by SW chosen up-scaler
                                            *
                                            *   @warning In case this value is true, IQ and performance degradation and could occur.
                                            */

    bool force_split_grid_matrix;   /**< If true then transform will be separated to un-distortion grid and a single stabilization matrix.
                                     *   Otherwise, un-distortion + stabilization will be done using grid only.
                                     *
                                     *   @note If is_sat_enabled is true and cam_is_deployment_type_t supports splitting
                                     *         then this value will be overridden by algorithm to true.
                                     *
                                     *   @warning This value is configurable only in case cam_is_deployment_type_t is supported
                                     */

    bool unify_undistortion_grid;   /**< If true then the un-distortion grid part will be same between NFL / FL. i.e. not use future data for un-distortion,
                                     *   use true in order to get same un-distortion for future looking and non future looking.
                                     *
                                     *   @warning  Since future information is not utilized, distortion correction could be sub-optimal.
                                     */

    bool is_sat_enabled;            /**< If true, sensor alignment transform, SAT will be applied in EIS transform along with
                                     *   EIS transform. EIS output matrices are already combined with SAT matrix on this case.
                                     *   Otherwise it is assumed that there is no SAT in the system.
                                     *
                                     *  @warning In case SAT exists in the system and it is not passed to EIS algorithm, and therefore
                                     *           not merged into EIS transform, IQ / power performance degeneration is expected
                                     */

    bool is_mag_enabled;    /**< if true, then magnetometer support is enabled. Otherwise disabled */
    bool is_acc_enabled;    /**< if true, then accelerometer support is enabled. Otherwise disabled */
    bool is_ois_enabled;    /**< if true, then OIS support is enabled. Otherwise disabled */
    bool is_ifovc_enabled;  /**< if true, then iFOVC support is enabled. Otherwise disabled */

    bool is_linear_acc_enabled;         /**< if true, then linear accelerometer support is enabled. Otherwise disabled */
    bool is_abs_orientation_enabled;    /**< if true, then absolute orientation support is enabled. Otherwise disabled */
    bool is_rel_orientation_enabled;    /**< if true, then relative orientation support is enabled. Otherwise disabled */

    /*          EIS 2.x specific configurations             */

    /*          EIS 3.x specific configurations             */
    uint16_t buffer_delay;          /**< For EIS3 future looking only */

} is_init_data_common;

typedef struct eis_roi_windows_s
{
    WindowRegionF        eis_pre_crop_vIN;  /**< eis_pre_crop window at vIN. Relative to IFE input size and FOV, i.e.
                                             *   This is the actual crop window done in IFE for DSX
                                             *    - fullWidth,   fullHeight   == IFE input size
                                             *    - windowWidth, windowHeight == pre crop size (actual)
                                             *    - windowLeft,  windowTop    == pre crop left, top (actual)
                                             *
                                             * for Inline EIS      : eis_pre_crop == ife_crop_dsx                (at_CSID_out)
                                             * for postIPE/OPE EIS : eis_pre_crop == ife_crop_dsx * ipe_crop     (at_CSID_out)
                                             *
                                             * crop FOV of the DSX not full.
                                             * == DSX FOV vs IFE input FOV
                                             *
                                             * In case of EIS Post IPE/OPE: we fill here the total crop done prior to EIS.
                                             */


    WindowRegionF        eis_pre_crop_vOUT;  /**< eis_pre_crop window at vOUT.
                                              *   Same window as eis_pre_crop_vIN but in UnDistorted Domain (after LDC)
                                              */

    // output_crop_fov == dzFOV_vOUT  relative to eis_pre_digital_zoom_at_vOUT
    // output_crop_fov is ICA output ROI vs Virtual domain out ROI ???
    // todo(kona) : change description....
    WindowRegionF        output_crop_fov;       /* crop ROI that ica/GPU is actually doing
                                                * ?????? This is the ICA output FOV with regard to its input FOV.
                                                * Its the total crop ICA is doing.
                                                * The FOV reduction is : physical + virtual margins + IPE DZ + FOVC + any other crop in ICA.
                                                * This could include a changing margins.
                                                *
                                                *    - fullWidth,   fullHeight   == EIS Input size (ICA1  input for inline / GPU input for post)
                                                *    - windowWidth, windowHeight == ICA1 output size
                                                *    - windowLeft,  windowTop    == ICA1 output crop left, top
                                                */
} eis_roi_windows;


/* IS input data structure */
typedef struct is_input_s
{
    uint32_t                frame_id;               /**< Frame ID */
    uint32_t                active_sensor_idx;      /**< Active sensor index */

    frame_times_t           frame_times;            /**< Frame time */

    gyro_data_t             gyro_data;              /**< Gyro data */
    mag_data_t              mag_data;               /**< Magnetometer data */
    acc_data_t              acc_data;               /**< Accelerometer data */
    ois_data_t              ois_data;               /**< OIS data */
    ifovc_data_t            ifovc_data;             /**< iFOVC data : ts, data[4] = {z_lens_pos, ifovc_correction_factor, 0, 0} */

    linear_acc_data_t       linear_acc_data;        /**< Linear Accelerometer data */
    abs_orientation_data_t  abs_orientation_data;   /**< Absolute orientation data */
    rel_orientation_data_t  rel_orientation_data;   /**< Relative orientation data */

    eis_roi_windows         window_regions;         /**< Input and Output ROI's */

    float                   focus_distance;         /**< Focus distance in meters. Distance from object on which focus is locked on */

    const NcLibWarp*        sat;                    /**< Optional - if equal to NULL then EIS disregards this parameter.
                                                     *
                                                     * - Order of matrices operators from in->out is first SAT and then EIS.
                                                     * - In order to align current sensor to previous sensor,
                                                     *   use SAT matrix which is OUT2IN, thus the matrix direction is from
                                                     *   previous sensor geometry to current sensor geometry.
                                                     * - SAT transform shall be defined on resolution of input to EIS.
                                                     * - SAT transform shall define center of image as origin (0,0).
                                                     */
} is_input_t;

/** IS output data structure */
typedef struct is_output_type_s
{
    uint32_t            frame_id;                   /**< Processed frame index  */
    uint32_t            active_sensor_idx;          /**< Active sensor index, synced with frame_id */

    NcLibWarp           stabilizationTransform;     /**< Stabilization transform, as passed to NcLib.
                                                     *   Transforms are synced with frame_id
                                                     */

    NcLibWarpMatrices   alignment_matrix_domain_undistorted;    /**< Undistorted frame alignment matrix using gyro instead of
                                                                 *   CVP/LRME, a matrix between "undistorted current" domain to "undistorted previous" domain.
                                                                 *   Transforms are synced with frame_id.
                                                                 */

    NcLibWarpMatrices   alignment_matrix_domain_stabilized;     /**< Stabilized frame alignment matrix using gyro target, a final
                                                                 *   matrix for MCTF matrix between "stabilized current" domain to "stabilized previous" domain.
                                                                 *   Transforms are synced with frame_id.
                                                                 */

    float               Mind_margins_blender_geolib;            /**< blender between maximal and minimal margins for GeoLib
                                                                 *   0 is minimal margins (== only physical margins); 1 is maximal margins (== total margins including virtual and maybe more in case of zoom).
                                                                 *   TODO (rgaizman): set this to 1 for calibration mode so we wont get unwanted VSR/IFE zoom.
                                                                 */

    float               cvp_motion_index_raw;                   /**< Amount of movement from previous frame to current frame in undistorted domain. units are in image pixels */

    cam_is_ois_mode     ois_mode;                               /**< When OIS is active, this will contain recommended OIS operation mode. Otherwise will contain an invalid output */

    bool                has_output;                             /**< if true, a frame was processed. Otherwise frame was not processed */
} is_output_type;

/** IS get stabilization margin input struct. Used by eis3_get_stabilization_margin_ex() */
typedef struct is_get_stabilization_margin_s
{
    uint32_t common_is_output_frame_height;             /**< Output frame height - Common to all sensors */
    uint32_t common_is_output_frame_width;              /**< Output frame width - Common to all sensors */
    bool     common_do_virtual_upscale_in_transform;    /**< If true then upscale is done using transform. Common to all sensors */

    uint32_t sensor_is_input_frame_height;              /**< Sensor input frame height */
    uint32_t sensor_is_input_frame_width;               /**< Sensor input frame width */

    float    sensor_minimal_total_margin_x;             /**< Minimal total margins as supplied to chromatix struct */
    float    sensor_minimal_total_margin_y;             /**< Minimal total margins as supplied to chromatix struct */
} is_get_stabilization_margin;

#ifdef __cplusplus
}
#endif

#endif /* _IS_INTERFACE_H_ */
