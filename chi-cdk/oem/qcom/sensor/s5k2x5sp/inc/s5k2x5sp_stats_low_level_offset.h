////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  s5k2x5sp_stats_low_level_offset.h
/// @brief The header that defines offset for s5k2x5sp low level stats data.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _S5K2X5SP_STATS_LOW_LEVEL_OFFSET_H_
#define _S5K2X5SP_STATS_LOW_LEVEL_OFFSET_H_

#define FOOTER_DSC_OFFSET   0x80
#define FOOTER_DATA_OFFSET  0x280

//#define MAX_NUM_OF_SPLIT_HIST_CHANNELS 4
#define MAX_NUM_OF_SPLIT_HIST_CHANNEL_MEMORY  6
#define MAX_NUM_OF_MERGED_HIST_CHANNEL_MEMORY 2
#define MAX_NUM_OF_YSUM_MEMORY                3
#define MAX_NUM_OF_THUMB_NAIL_MEMORY          5
#define MAX_YSUM_STATS_STRIP_NUM              128 /* 1~128*/
#define MAX_THUMBNAIL_STATS_PATCH_NUM         6240  /* 80x78*/

//#define HIST_MERGED_MEMORY_NUM 8

/*Below define offset values are relative offset of Footer description*/
#define DATA_SIZE_REG_ADDR         2 //0x2
#define DATA_START_OFFSET_REG_ADDR 4 //0x4

/*Gyro*/
#define GYRO_META_SIZE   64
#define GYRO_META_OFFSET 64 //0x40

/*Hist. Split*/
#define HIST_SPLIT_META_SIZE             128
#define HIST_SPLIT_META_OFFSET           128 //0x80
#define HIST_SPLIT_META_G_DATA_OFFSET    108
#define HIST_SPLIT_META_B_DATA_OFFSET    110
#define HIST_SPLIT_META_Y_DATA_OFFSET    112
#define HIST_SPLIT_META_R_MEM_DSC_OFFSET 8 //0x8
#define HIST_SPLIT_META_G_MEM_DSC_OFFSET 32 //0x20
#define HIST_SPLIT_META_B_MEM_DSC_OFFSET 56 //0x38
#define HIST_SPLIT_META_Y_MEM_DSC_OFFSET 80 //0x50

/*Hist. Merged*/
#define HIST_MERGED_META_SIZE             64
#define HIST_MERGED_META_OFFSET           256 //0x100
#define HIST_MERGED_META_G_DATA_OFFSET    44
#define HIST_MERGED_META_B_DATA_OFFSET    46
#define HIST_MERGED_META_Y_DATA_OFFSET    48
#define HIST_MERGED_META_R_MEM_DSC_OFFSET 8 //0x8
#define HIST_MERGED_META_G_MEM_DSC_OFFSET 16 //0x10
#define HIST_MERGED_META_B_MEM_DSC_OFFSET 24 //0x18
#define HIST_MERGED_META_Y_MEM_DSC_OFFSET 32 //0x20

/*Y-Sum*/
#define Y_SUM_META_SIZE                 64
#define Y_SUM_META_OFFSET               320 //0x140
#define Y_SUM_META_NUM_OF_STRIPS_OFFSET 20
#define Y_SUM_META_MEM_DSC_OFFSET       8
#define Y_SUM_META_MEM_1_DATA_OFFSET    22 //0x16
#define Y_SUM_META_MEM_2_DATA_OFFSET    24 //0x18


/*Thumb*/
#define THUMB_META_SIZE                64
#define THUMB_META_OFFSET              384 //0x180
#define THUMB_META_H_NUM_OFFSET        18
#define THUMB_META_V_NUM_OFFSET        20
#define THUMB_META_PW_OFFSET           14 //patch width offset
#define THUMB_META_PH_OFFSET           16 //patch height offset
#define THUMB_META_MEM_DSC_OFFSET      8
#define THUMB_META_G_DATA_OFFSET       24 //0x18
#define THUMB_META_B_DATA_OFFSET       26 //0x1A
#define THUMB_META_Y_DATA_OFFSET       28 //0x1C
#define THUMB_META_COUNTER_DATA_OFFSET 30 //0x1E


/*DRC counter*/
#define DRC_META_SIZE   64
#define DRC_META_OFFSET 0x1C0
 

 #endif