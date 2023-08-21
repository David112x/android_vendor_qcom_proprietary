////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  s5k2x5sp_stats_high_level_struct.h
/// @brief The header that defines high level struct for s5k2x5sp stats data.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _S5K2X5SP_STATS_HIGH_LEVEL_STRUCT_H_
#define _S5K2X5SP_STATS_HIGH_LEVEL_STRUCT_H_

#include "s5k2x5sp_stats_common.h"
#include "s5k2x5sp_stats_low_level_offset.h"

/** split_hist_hdr_mode_t
 *
 *  This enumeration represents what 3D HDR mode stats are collected for
 */
typedef enum {
    EXPOSURE_SHORT = 0,
    EXPOSURE_LONG,
    EXPOSURE_MEDIUM,
} split_hdr_mode_t;

/** hist_grid_type
 *
 *  This enumeration represents how histograms are collected
 */
typedef enum {
    HIST_WINDOW = 0,
    HIST_WEIGHTED_GRID
}hist_grid_type;

/** st_split_hist_conf_t
 *    @num_of_active_mem:   Num of valid memory containing the valid histogram data.
 *    @num_bins:            Num of bins containing valid data.
 *    @grid_en:             Grid mode; 0 is window, 1 is weighted grid.
 *    @hdr_mode:            HDR mode; 0 is SHORT, 1 is LONG, 2 is MEDIUM.
 *    @hist_data:           pointer of histogram buffers.
 **/
typedef struct {
    UINT8            num_of_active_mem;
    UINT16           num_bins[MAX_NUM_OF_SPLIT_HIST_CHANNEL_MEMORY];
    hist_grid_type   grid_en[MAX_NUM_OF_SPLIT_HIST_CHANNEL_MEMORY];
    split_hdr_mode_t hdr_mode[MAX_NUM_OF_SPLIT_HIST_CHANNEL_MEMORY];
    UINT32*          hist_data[MAX_NUM_OF_SPLIT_HIST_CHANNEL_MEMORY];
}st_split_hist_conf_t;

/** st_split_hist_stat_data_t
 *    @split_r_hist:         pointer containing the red histogram buffers and configuration info
 *    @split_g_hist:         pointer containing the green histogram buffers and configuration info
 *    @split_b_hist:         pointer containing the blue histogram buffers and configuration info
 *    @split_y_hist:         pointer containing the y histogram buffers and configuration info
 *    @split_r_hist_valid:   true if split_r_hist contains valid data
 *    @split_g_hist_valid:   true if split_g_hist contains valid data
 *    @split_b_hist_valid:   true if split_b_hist contains valid data
 *    @split_y_hist_valid:   true if split_y_hist contains valid data
 *
 *  This structure is used to pass the split exposure histogram statistics from the sensor.
 **/
typedef struct {
    st_split_hist_conf_t split_r_hist;
    st_split_hist_conf_t split_g_hist;
    st_split_hist_conf_t split_b_hist;
    st_split_hist_conf_t split_y_hist;
    bool                 split_r_hist_valid;
    bool                 split_g_hist_valid;
    bool                 split_b_hist_valid;
    bool                 split_y_hist_valid;
}st_split_hist_stat_t;

/** st_merged_hist_conf_t
 *    @num_of_active_mem:       Num of enabled memory containing histogram data.
 *    @grid_en:                 Grid mode; 0 is window, 1 is weighted grid.
 *    @hist_data:               pointer of histogram buffers.
 **/
typedef struct {
    UINT8          num_of_active_mem;
    hist_grid_type grid_en[MAX_NUM_OF_MERGED_HIST_CHANNEL_MEMORY];
    UINT32*        hist_data[MAX_NUM_OF_MERGED_HIST_CHANNEL_MEMORY];
}st_merged_hist_conf_t;

/** st_merged_hist_stat_data_t
 *    @merged_r_hist:           pointer containing the red histogram buffers and configuration info
 *    @merged_g_hist:           pointer containing the green histogram buffers and configuration info
 *    @merged_b_hist:           pointer containing the blue histogram buffers and configuration info
 *    @merged_y_hist:           pointer containing the y histogram buffers and configuration info
 *    @merged_r_hist_valid:     true if merged_r_hist contains valid data
 *    @merged_g_hist_valid:     true if merged_g_hist contains valid data
 *    @merged_b_hist_valid:     true if merged_b_hist contains valid data
 *    @merged_y_hist_valid:     true if merged_y_hist contains valid data
 *
 *  This structure is used to pass the merged exposure histogram statistics from the sensor.
 **/
typedef struct {
    st_merged_hist_conf_t merged_r_hist;
    st_merged_hist_conf_t merged_g_hist;
    st_merged_hist_conf_t merged_b_hist;
    st_merged_hist_conf_t merged_y_hist;
    bool                  merged_r_hist_valid;
    bool                  merged_g_hist_valid;
    bool                  merged_b_hist_valid;
    bool                  merged_y_hist_valid;
}st_merged_hist_stat_t;

/** ysum_hdr_mode_t
 *
 *  This enumeration represents what Y SUM stats are collected for
 */
typedef enum {
    YSUM_SHORT = 0,
    YSUM_MEDIUM,
    YSUM_LONG,
    YSUM_MIXED,
    YSUM_NOT_APPLICABLE
} ysum_hdr_mode_t;

/** st_ysum_conf_t
 *    @num_of_active_mem:       Num of valid memory containing y sum data.
 *    @num_of_strips:           Num of strips(1-128).
 *    @hdr_mode:                HDR mode; 0 is SHORT, 1 is MEDIUM, 2 is LONG, 3 is MIXED, 4 is not applicable.
 *    @ysum_data:               Array containing y sum values.
 **/
typedef struct {
    UINT8              num_of_active_mem;
    UINT16             num_of_strips;
    ysum_hdr_mode_t hdr_mode[MAX_NUM_OF_YSUM_MEMORY];
    UINT32*            ysum_data[MAX_NUM_OF_YSUM_MEMORY];
}st_ysum_conf_t;

/** st_y_sum_stat_data_t
 *    @y_sum_data:  pointer containing y sum buffers and configuration info
 *    @y_sum_valid: true if y_sum contains valid data
 *
 *  This structure is used to pass the y sum statistics from the sensor.
 **/
typedef struct {
    st_ysum_conf_t y_sum_data;
    bool           y_sum_valid;
}st_y_sum_stat_t;

/** thumb_hdr_mode_t
 *
 *  This enumeration represents what THUMB stats are collected for
 */
typedef enum {
    THUMB_SHORT = 0,
    THUMB_MEDIUM,
    THUMB_LONG,
    THUMB_MERGED
}thumb_hdr_mode_t;

/** st_thumb_nail_buf_t
 *    @r_avg:   pointer containing the average for the red pixels
 *    @g_avg:   pointer containing the average for the green pixels
 *    @b_avg:   pointer containing the average for the blue pixels
 *    @y_avg:   pointer containing the average for the y
 *    @counter:	pointer containing the counter for the y???
 *
 **/
typedef struct {
    UINT16* r_avg;
    UINT16* g_avg;
    UINT16* b_avg;
    UINT16* y_avg;
    UINT16* counter;
}st_thumb_nail_buf_t;

/** st_thumb_stat_data_t
 *    @thumb_nail_data:     pointer containing y sum buffers and configuration info.
 *    @region_h_num:        Horizontal number of regions for the thumb nail.
 *    @region_v_num:        Vertical number of regions for the thumb nail.
 *    @region_pixel_cnt:    The count of the region pixels.(region_width x region_height)
 *    @region_width:        Horizontal number of pixels in the each region.
 *    @region_height:       Vertical number of pixels in the each region.
 *    @thumb_nail_valid:    true if y_sum contains valid data
 *
 *  This structure is used to pass the thumb nail statistics from the sensor.
 **/
typedef struct {
    st_thumb_nail_buf_t thumb_nail_data;
    UINT16              region_h_num; /* max 80 */
    UINT16              region_v_num; /* max 78 */
    UINT32              region_pixel_cnt;
    UINT16              region_width;
    UINT16              region_height;
    UINT16              num_of_patch;
    thumb_hdr_mode_t    hdr_mode;
    bool                thumb_nail_valid;
}st_thumb_stat_t;

typedef struct{
    st_split_hist_stat_t  split_hist;
    st_merged_hist_stat_t merged_hist;
    st_y_sum_stat_t       y_sum;
    st_thumb_stat_t       thumb_nail;
}st_ae_stat_t;

typedef struct{
    UINT8* gyro_header;
    UINT8* gyro_data;
    UINT16 data_size;
}st_gyro_stat_t;

#endif