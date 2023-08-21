////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __IS_INTERFACE_ERROR_CODES_H__
#define __IS_INTERFACE_ERROR_CODES_H__


/**  @addtogroup Return_Values
  *  @brief     IS algorithm return values
  *  @{
  */
#define IS_RET_BASE                 0x08000000                  /**< @return_val Base value for all IS related errors and warnings */
#define IS_RET_SUCCESS              (0)                         /**< @return_val Success return value */

#define IS_RET_ERROR_BASE           (IS_RET_BASE)               /**< @return_val Error base value - algorithm cannot recover */

#define IS_RET_FRAME_NOT_PROCESSES  (IS_RET_ERROR_BASE + 1)     /**< @return_val Frame was not processed */
#define IS_RET_GENERAL_ERROR        (IS_RET_ERROR_BASE + 2)     /**< @return_val General error return value */
#define IS_RET_INVALID_INPUT        (IS_RET_ERROR_BASE + 3)     /**< @return_val Invalid input */
#define IS_RET_OUT_OF_MEMORY        (IS_RET_ERROR_BASE + 4)     /**< @return_val Out of memory error */

#define IS_RET_WARN_BASE            (IS_RET_BASE | 0x100)       /**< @return_val Warning base value - algorithm can recover but
                                                                  *  results could be sub-optimal
                                                                  */

#define IS_RET_WARN_INVALID_INPUT   (IS_RET_WARN_BASE + 1)      /**< @return_val Invalid input */
/** @} */

/**  @addtogroup    Errors_Information
  *  @brief         IS algorithm output errors and warnings enumerators and information
  *  @{
  */

/**  @addtogroup    Critical_Errors
 *  @brief          Critical errors. Algorithm cannot recover. API will return with the appropriate error code.
 *                  Values range: 0000-0511
 *  @{
 */

 /** \error ERROR IS0001: Internal assertion failed */
#define IS_ERR_ASSERTION                    "IS0001"

 /** \error ERROR IS0002: General error occurred */
#define IS_ERR_GENERAL_ERROR                "IS0002"

 /** \error ERROR IS0003: Invalid input was detected */
#define IS_ERR_INVALID_INPUT                "IS0003"

/** \error ERROR IS0004: Memory allocation failed */
#define IS_ERR_OUT_OF_MEMORY                "IS0004"

/** \error ERROR IS0005: Invalid filter status was detected */
#define IS_ERR_INVALID_FILTER_STATUS        "IS0005"

/** \error ERROR IS0006: Invalid initial FPS values */
#define IS_ERR_INVALID_INPUT_FPS            "IS0006"

/** \error ERROR IS0007: Out of Order Start for Frame timestamp was detected */
#define IS_ERR_OOO_SOF                      "IS0007"

/** \error ERROR IS0008: Duplicated frame was detected */
#define IS_ERR_DUPLICATED_FRAMES            "IS0008"

/** \error ERROR IS0009: Actual sample rate exceeds the requested sample rate, could cause buffer overflow */
#define IS_ERR_SAMPLE_RATE_EXCEEDS          "IS0009"

/** \error ERROR IS0010: Invalid IFE input data, struct might be un-initialized */
#define IS_ERR_IFE_INVALID_INIT             "IS0010"

/** \error ERROR IS0011: Invalid IFE input data. Region of interest might be outside of the image */
#define IS_ERR_IFE_INVALID_ROI              "IS0011"

/** \error ERROR IS0012: Out of Order frame ID was detected */
#define IS_ERR_OOO_FRAME_ID                 "IS0012"

/** \error ERROR IS0013: Invalid SAT matrix was detected */
#define IS_ERR_SAT_INVALID                  "IS0013"

/** \error ERROR IS0014: Invalid SAT matrix was detected */
#define IS_ERR_SENSOR_ID_INVALID            "IS0014"

/** \error ERROR IS0015: Invalid input, configuration mismatch. Gyro based MCTF is enabled but output structs are not passed as input */
#define IS_ERR_INVALID_INPUT_MCTF           "IS0015"

/** \error ERROR IS0016: Invalid input, configuration mismatch. Pointer structs are NULL */
#define IS_ERR_INVALID_INPUT_STAB_TRANSFORM "IS0016"

/** \error ERROR IS0017: Invalid input, configuration mismatch. Pointer structs are NULL */
#define IS_ERR_INVALID_INIT_INPUT           "IS0017"

/** \error ERROR IS0018: Sampler detected that many samples are missing */
#define IS_ERR_SAMPLER_MISSING_MANY_SAMPLES "IS0018"

/** \error ERROR IS0019: One of the sensors is missing many sample with respect to other sensor */
#define IS_ERR_SENSOR_MISSING_MANY_SAMPLES  "IS0019"

/** @} */

/**  @addtogroup    ISQ_Warnings
*  @brief           Image Stabilization Quality warnings. Algorithm can recover, however degradation in stabilization quality is expected.
*                   Values range: 0512-1023
*  @{
*/

/** \error ERROR IS0512: Invalid rolling shutter skew value, is equal to zero */
#define IS_ERR_RSS_ZERO                     "IS0512"

/** \error ERROR IS0513: Invalid rolling shutter skew value, exceed frame time */
#define IS_ERR_RSS_EXCEEDS                  "IS0513"

/** \error ERROR IS0514: Invalid filter values are detected, sub-optimal stabilization could occur */
#define IS_ERR_INVALID_FILTER_VALUES        "IS0514"

/** \error ERROR IS0515: Invalid SOF value */
#define IS_ERR_SOF_INVALID                  "IS0515"

/** \error ERROR IS0516: Invalid frame time value */
#define IS_ERR_FRAME_TIME_INVALID           "IS0516"

/** \error ERROR IS0517: Invalid exposure time value */
#define IS_ERR_EXPOSURE_TIME_INVALID        "IS0517"

/** \error ERROR IS0518: Estimated sample rate is not close to mean sample rate. Might indicate on missing samples or false time-stamps */
#define IS_ERR_SAMPLE_RATE_EST_VS_AVG       "IS0518"

/** \error ERROR IS0519: Estimated sample rate is not close to initial value. Might indicate on false HW configuration */
#define IS_ERR_SAMPLE_RATE_EST_VS_INIT      "IS0519"

/** \error ERROR IS0521: Sampler has detected a missing sample */
#define IS_ERR_SAMPLER_MISSING_SAMPLE       "IS0521"

/** \error ERROR IS0522: Sampler has detected missing samples from beginning of frame */
#define IS_ERR_SAMPLER_FIRST_MISSING_SAMPLE "IS0522"

/** \error ERROR IS0523: Sampler has detected missing samples from end of stream */
#define IS_ERR_SAMPLER_LAST_MISSING_SAMPLE  "IS0523"

/** \error ERROR IS0524: Out of Order sample */
#define IS_ERR_SAMPLE_OOO                   "IS0524"

/** \error ERROR IS0525: Duplicated sample */
#define IS_ERR_SAMPLE_DUPLICATED            "IS0525"

/** \error ERROR IS0526: Gyro samples spacing is too big with respect to estimated frequency. Might indicate on missing gyro samples or false gyro time-stamps */
#define IS_ERR_SAMPLE_SPACING               "IS0526"

/** \error ERROR IS0527: IFE Region of Interest is not centered */
#define IS_ERR_IFE_ROI_NOT_CENTERED         "IS0527"

/** \error ERROR IS0528: IPE Region of Interest is not centered */
#define IS_ERR_IPE_ROI_NOT_CENTERED         "IS0528"

/** \error ERROR IS0529: corrupted CSV file was read */
#define IS_ERR_DUMP_FILE_CORRUPTED          "IS0529"

/** \error ERROR IS0530: General warning occurred */
#define IS_ERR_GENERAL_WARNING              "IS0530"

/** @} */


/**  @addtogroup    Verbose_Information
*  @brief           Additional information that might indicate on use-cases that might affect the stabilized video
*                   Values range: 1024-1535
*  @{
*/

/** \error VERBOSE IS1024: Pause-Resume use-case was detected */
#define IS_VRB_PAUSE_RESUME                 "IS1024"

/** \error VERBOSE IS1025: Invalid input was passed to algorithm destroy function. Could indicate on invalid SW flow */
#define IS_VRB_DESTROY_INVALID_INPUT        "IS1025"

/** \error VERBOSE IS1026: Input has some configurations which are sub-optimal */
#define IS_VRB_INPUT_INFO                   "IS1026"

/** \error VERBOSE IS1027: General debug information */
#define IS_VRB_DEBUG_INFO                   "IS1027"

/** \error VERBOSE IS1028: Frame drop was detected, stabilized video could look not smooth */
#define IS_VRB_FRAME_DROP                   "IS1028"


/** @} */

/** @} */

#endif /* __IS_INTERFACE_ERROR_CODES_H__ */
