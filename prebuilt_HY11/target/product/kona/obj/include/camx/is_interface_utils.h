////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __IS_INTERFACE_UTILS_H__
#define __IS_INTERFACE_UTILS_H__

/*------------------------------------------------------------------------
*       Include Files
* ----------------------------------------------------------------------- */

#include "is_interface.h"
#include "ipe_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------
*       Definitions and Macros
* ----------------------------------------------------------------------- */

#define EIS_UTIL_MAX_FILE_PREFIX_LENGTH     128  /**< Maximal length of file prefix, including terminating null char */

/*------------------------------------------------------------------------
*       Data structures
* ----------------------------------------------------------------------- */
typedef enum is_utils_log_outputs_e
{
    IS_UTILS_FLAG_NONE          = 0x00000000,   /**< No data will be written.
                                                 *   @warning if eis_utils_log_init() is called with this flag it will return with error.
                                                 */
    IS_UTILS_FLAG_WRITE_INPUT   = 0x00000001,   /**< Write input data of EIS algorithm to file */
    IS_UTILS_FLAG_WRITE_OUTPUT  = 0x00000002,   /**< Write output data of EIS algorithm to file */
    IS_UTILS_FLAG_LOGCAT_INPUT  = 0x00000004,   /**< Write output data of EIS algorithm to logcat */
    IS_UTILS_FLAG_LOGCAT_OUTPUT = 0x00000008,   /**< Write output data of EIS algorithm to logcat */

    IS_UTILS_FLAG_DEFAULT       = IS_UTILS_FLAG_WRITE_INPUT,   /**< Default flags */

    IS_UTILS_FLAG_ALL           = IS_UTILS_FLAG_WRITE_INPUT |
                                  IS_UTILS_FLAG_WRITE_OUTPUT |
                                  IS_UTILS_FLAG_LOGCAT_INPUT |
                                  IS_UTILS_FLAG_LOGCAT_OUTPUT,
}is_utils_log_flags;

/** log utility initial common configurations data structure */
typedef struct is_utils_log_init_s
{
    is_init_data_common*    init_common;                                    /**< Common configuration, as passed to eis3_initialize() or eis2_initialize() */
    is_init_data_sensor*    init_sensors;                                   /**< Per sensor configuration, as passed to eis3_initialize() or eis2_initialize() */
    uint32_t                num_sensors;                                    /**< init_sensors array size */
    is_utils_log_flags      flags;                                          /**< Defines the logging system flags */
    char                    file_prefix[EIS_UTIL_MAX_FILE_PREFIX_LENGTH];   /**< file prefix, as it will be written to file system */
}is_utils_log_init;

/** log utility common configurations data structure */
typedef struct is_utils_log_write_data_s
{
    is_input_t*         is_input;                   /**< EIS algorithm input struct */
    is_output_type*     is_output;                  /**< EIS algorithm output struct */
    is_time_intervals*  is_output_time_intervals;   /**< EIS algorithm output struct */
    char*               buffer;                     /**< Additional per frame buffer to be saved */
}is_utils_log_write_data;

/*------------------------------------------------------------------------
*       API Declarations
* ----------------------------------------------------------------------- */

/**
* @deprecated Use eis_utility_convert_to_window_regions_deployment() instead, will be removed in future version
*
*  @brief   Utility function that coverts window regions in Hana format to Kona (GeoLib)
*
*  @note    Use only in case GeoLib is not present in the system
*
*  @param [in]  ife                         IFE crop window
*  @param [in]  ipe                         IPE digital zoom window
*  @param [in]  stabilization_crop_ratio_x  stabilization crop ratio for x-axis, as taken from
*                                           eis3_get_stabilization_crop_ratio_ex() or eis2_get_stabilization_crop_ratio_ex()
*  @param [in]  stabilization_crop_ratio_y  stabilization crop ratio for y-axis, as taken from
*                                           eis3_get_stabilization_crop_ratio_ex() or eis2_get_stabilization_crop_ratio_ex()
*  @param [in]  stabilization_input_size_x  Actual stabilization input image size == IPE input size at DZ x1
*  @param [in]  stabilization_input_size_y  Actual stabilization input image size == IPE input size at DZ x1
*  @param [out] eisWindowRegions    Window regions as taken by EIS algorithm
*
*  @return 0 in case of success, otherwise failed.
**/
EIS_VISIBILITY_PUBLIC
int32_t eis_utility_convert_to_window_regions(
    const WindowRegion* ife,
    const WindowRegion* ipe,
    double              stabilization_crop_ratio_x,
    double              stabilization_crop_ratio_y,
    uint32_t            stabilization_input_size_x,
    uint32_t            stabilization_input_size_y,
    eis_roi_windows*    eisWindowRegions);


/**
*  @brief   Utility function that coverts window regions in Hana format to Kona (GeoLib)
*
*  @note    Use only in case GeoLib is not present in the system
*
*  @param [in]  ife                         IFE crop window
*  @param [in]  ipe                         IPE digital zoom window
*  @param [in]  stabilization_crop_ratio_x  stabilization crop ratio for x-axis, as taken from
*                                           eis3_get_stabilization_crop_ratio_ex() or eis2_get_stabilization_crop_ratio_ex()
*  @param [in]  stabilization_crop_ratio_y  stabilization crop ratio for y-axis, as taken from
*                                           eis3_get_stabilization_crop_ratio_ex() or eis2_get_stabilization_crop_ratio_ex()
*  @param [in]  stabilization_input_size_x  Actual stabilization input image size == IPE input size at DZ x1
*  @param [in]  stabilization_input_size_y  Actual stabilization input image size == IPE input size at DZ x1
*  @param [in]  deployment_type             EIS Deployment type
*  @param [out] eisWindowRegions    Window regions as taken by EIS algorithm
*
*  @return 0 in case of success, otherwise failed.
**/
EIS_VISIBILITY_PUBLIC
int32_t eis_utility_convert_to_window_regions_deployment(
    const WindowRegion*         ife,
    const WindowRegion*         ipe,
    double                      stabilization_crop_ratio_x,
    double                      stabilization_crop_ratio_y,
    uint32_t                    stabilization_input_size_x,
    uint32_t                    stabilization_input_size_y,
    cam_is_deployment_type_t    deployment_type,
    eis_roi_windows*            eisWindowRegions);

/**
*  @brief   Utility function that coverts a grid to 9 matrices
*               A temporary patch for using GPU OpenGL matrices warper instead of GPU Grid warper
*
*  @warning Cannot support LDC at all !!!
*
*  @param [in]      inGrid                        Input grid for conversion
*  @param [out]     outMatrices                   Output 9 matrices
*
*  @return
**/
EIS_VISIBILITY_PUBLIC
int32_t eis_utility_convert_grid_to_matrices(const NcLibWarpGrid* inGrid, NcLibWarpMatrices* outMatrices);

/**
*  @brief   Initializes EIS log utility
*
*  @param [out] context                     EIS log utility context
*  @param [in]  init_data                   EIS algorithm init data
*
*  @return 0 in case of success, otherwise failed.
**/
EIS_VISIBILITY_PUBLIC
int32_t eis_utils_log_init(
    void**                      context,
    const is_utils_log_init*    init_data);

/**
*  @brief   Write frame information to file
*
*  @param [in]  context                     EIS log utility context
*  @param [in]  data                        frame data to be written
*
*  @return 0 in case of success, otherwise failed.
**/
EIS_VISIBILITY_PUBLIC
int32_t eis_utils_log_write(
    void*                           context,
    const is_utils_log_write_data*  data);

/**
*  @brief   Flush data to file from temporary memory buffer
*
*  @param [in]  context                     EIS log utility context
*
*  @return 0 in case of success, otherwise failed.
**/
EIS_VISIBILITY_PUBLIC
int32_t eis_utils_log_flush(void* _context);

/**
*  @brief   Flush data to file from temporary memory buffers to files, close them and open new files with
*           the new file prefix supplied
*
*  @param [in]  context                     EIS log utility context
*  @param [in]  file_prefix                 EIS log file prefix
*
*  @return 0 in case of success, otherwise failed.
**/
EIS_VISIBILITY_PUBLIC
int32_t eis_utils_log_open(void* _context, const char file_prefix[EIS_UTIL_MAX_FILE_PREFIX_LENGTH]);

/**
*  @brief   return status is true if eis_utils_log_open() was called and returned successfully, otherwise false
*
*  @param [in]  context                     EIS log utility context
*  @param [in]  result                      return status
*
*  @return 0 in case of success, otherwise failed.
**/
EIS_VISIBILITY_PUBLIC
int32_t eis_utils_log_is_opened(void* _context, bool* result);

/**
*  @brief   Flush data from temporary memory buffers to files, and closes them
*
*  @param [in]  context                     EIS log utility context
*
*  @return 0 in case of success, otherwise failed.
**/
EIS_VISIBILITY_PUBLIC
int32_t eis_utils_log_close(void* _context);

/**
*  @brief   Destroys EIS log utility context
*
*  @param [in,out]  context                 EIS log utility context
*
*  @return 0 in case of success, otherwise failed.
**/
EIS_VISIBILITY_PUBLIC
int32_t eis_utils_log_destroy(void** context);

#ifdef __cplusplus
}
#endif

#endif /* __IS_INTERFACE_UTILS_H__ */
