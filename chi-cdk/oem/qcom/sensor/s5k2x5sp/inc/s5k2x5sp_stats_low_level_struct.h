////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  s5k2x5sp_stats_low_level_struct.h
/// @brief The header that defines low level detailed struct for s5k2x5sp stats data.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _S5K2X5SP_STATS_LOW_LEVEL_STRUCT_H_
#define _S5K2X5SP_STATS_LOW_LEVEL_STRUCT_H_

#include "s5k2x5sp_stats_common.h"

struct general_information {
    UINT16 sanity_constatnt_value;
    UINT16 frame_counter;
    UINT8  footer_version;
    UINT8  footer_size;
    UINT32 number_of_sections;
    UINT8  reserved[54];
} __attribute__((packed));

struct base_meta_head {
    UINT8  enable;
    UINT8  reserved;
    UINT16 data_total_size;
    UINT32 data_start_offset;
} __attribute__((packed));

struct gyro_synchronizer_metadata {
    struct base_meta_head base;
    UINT16 start_footer_pilot;
    UINT8  full_scale_sel;
    UINT8  sel_axis;
    UINT8  all_ts_mode;
    UINT8  num_internal_ram_chunks;
    UINT8  mem_intr_cntr;
    UINT8  sample_rate_enum;
    UINT8  gyro_sen_dlpf_cfg;
    UINT8  gyro_sen_fchoice_b;
    UINT8  gyro_sen_vendor;
    UINT8  reserved_1;
    UINT16 gyro_err_enum_bite;
    UINT16 max_cnt;
    UINT16 extra_partial_chunk_size;
    UINT16 internal_mem_one_chunk_size_bytes;
    UINT16 time_stamp_intrpt_cntr;
    UINT8  reserved_2;
    UINT8  reserved_3;
    UINT32 ts_clk_khz;
    UINT32 spi_clk_khz;
    UINT32 frame_counter;
    UINT32 ts0_reg;
    UINT32 ts1_reg;
    UINT32 ts2_reg;
    UINT32 ts3_reg;
    UINT32 ts4_reg;
} __attribute__((packed));

typedef UINT32 memory_descriptor;
struct histogram_meta_tail {
    UINT8  data_bytes_per_bin;
    UINT8  data_integer_bits;
    UINT8  data_precision_bits;
    UINT8  reserved_0;
    UINT16 g_data_ofs;
    UINT16 b_data_ofs;
    UINT16 y_data_ofs;
    UINT8  reserved_1[14];
} __attribute__((packed));

struct split_exposure_histogram_statistices_metadata {
    struct base_meta_head       base;
    memory_descriptor           red_ram[6];
    memory_descriptor           green_ram[6];
    memory_descriptor           blue_ram[6];
    memory_descriptor           y_ram[6];
    struct histogram_meta_tail  tail;
} __attribute__((packed));

struct merged_exposure_histogram_statistices_metadata {
    struct base_meta_head       base;
    memory_descriptor           red_ram[2];
    memory_descriptor           green_ram[2];
    memory_descriptor           blue_ram[2];
    memory_descriptor           y_ram[2];
    struct histogram_meta_tail  tail;
} __attribute__((packed));

struct ysum_statistices_metadata {
    struct base_meta_head   base;
    UINT8  type_of_data_mem[3];
    UINT8  reserved_0;
    UINT16 start_x;
    UINT16 start_y;
    UINT16 strip_width;
    UINT16 strip_size;
    UINT16 num_of_stripes;
    UINT16 offset_to_mem_1;
    UINT16 offset_to_mem_2;
    UINT8  reserved_1[38];
} __attribute__((packed));

struct thumbnail_statistices_metadata {
    struct base_meta_head   base;
    UINT8  type_of_data;
    UINT8  average_val_bits_per_entry;
    UINT16 patch_array_start_x;
    UINT16 patch_array_start_y;
    UINT16 patch_array_patch_width;
    UINT16 patch_array_patch_height;
    UINT16 number_of_patches_in_strip;
    UINT16 number_of_strips;
    UINT16 number_of_patches;
    UINT16 green_offset_bytes;
    UINT16 blue_offset_bytes;
    UINT16 y_offset_bytes;
    UINT16 counter_offset_bytes;
    UINT8  reserved[32];
} __attribute__((packed));

struct s5k2x5_embedded_metadata {
    UINT8 debug_section[128];
    struct general_information general_info;
    struct gyro_synchronizer_metadata gyro_syncnzr_meta;
    struct split_exposure_histogram_statistices_metadata split_exp_hist_stats_meta;
    struct merged_exposure_histogram_statistices_metadata merged_exp_hist_stats_meta;
    struct ysum_statistices_metadata ysum_stats_meta;
    struct thumbnail_statistices_metadata thumb_stats_meta;
    UINT8 drc_counters_metadata[64];
} __attribute__((packed));


#endif