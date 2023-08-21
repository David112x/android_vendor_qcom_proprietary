////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  s5k2x5sp_stats_paeser.h
/// @brief The header that defines stats parse function for s5k2x5sp stats data.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _CAMX_2X5_STAT_PARSER_H_
#define _CAMX_2X5_STAT_PARSER_H_

#include "s5k2x5sp_stats_low_level_struct.h"
#include "s5k2x5sp_stats_high_level_struct.h"

#define DUMP_DIR
#ifdef  _DEBUG_
#ifdef  _ANDROID_
#undef  DUMP_DIR
#define DUMP_DIR      "/data/misc/camera/"
#endif
#endif

#define GYRO_DSC            DUMP_DIR "./gyro_dsc.bin"
#define GYRO_DATA           DUMP_DIR "./gyro_data_%d.bin"
#define HIST_SPLIT_DSC      DUMP_DIR "./hist_split_dsc.bin"
#define HIST_SPLIT_R_DATA   DUMP_DIR "./hist_split_r_data_%d_%d.bin"
#define HIST_SPLIT_G_DATA   DUMP_DIR "./hist_split_g_data_%d_%d.bin"
#define HIST_SPLIT_B_DATA   DUMP_DIR "./hist_split_b_data_%d_%d.bin"
#define HIST_SPLIT_Y_DATA   DUMP_DIR "./hist_split_y_data_%d_%d.bin"
#define HIST_MERGED_DSC     DUMP_DIR "./hist_merged_dsc.bin"
#define HIST_MERGED_R_DATA  DUMP_DIR "./hist_merged_r_data_%d_%d.bin"
#define HIST_MERGED_G_DATA  DUMP_DIR "./hist_merged_g_data_%d_%d.bin"
#define HIST_MERGED_B_DATA  DUMP_DIR "./hist_merged_b_data_%d_%d.bin"
#define HIST_MERGED_Y_DATA  DUMP_DIR "./hist_merged_y_data_%d_%d.bin"
#define Y_SUM_DSC           DUMP_DIR "./y_sum_dsc.bin"
#define Y_SUM_DATA          DUMP_DIR "./y_sum_data_mem_%d_%d.bin"
#define THUMB_DSC           DUMP_DIR "./thumb_dsc.bin"
#define THUMB_R_DATA        DUMP_DIR "./thumb_data_r_%d.bin"
#define THUMB_G_DATA        DUMP_DIR "./thumb_data_g_%d.bin"
#define THUMB_B_DATA        DUMP_DIR "./thumb_data_b_%d.bin"
#define THUMB_Y_DATA        DUMP_DIR "./thumb_data_y_%d.bin"
#define THUMB_COUNTER_DATA  DUMP_DIR "./thumb_data_counter_%d.bin"

template <typename dataType>
inline static UINT32 big2Little(
     dataType x)
{
    UINT32 size = sizeof(x);

    if (size == 1)
    {
        return x;
    }

    if (size == 2)
    {
        return ((x & 0xff) << 8) | ((x & 0xff00) >> 8);
    }

    if (size == 4)
    {
        return ((x & 0x000000ff) << 24)
                | ((x & 0x0000ff00) << 8)
                | ((x & 0x00ff0000) >> 8)
                | ((x & 0xff000000) >> 24);
    }

    return 0;
}

#ifdef __cplusplus
extern "C"
{
BOOL isValidStat(UINT8* pBase);
BOOL parseGyroStat(UINT8* pBase, st_gyro_stat_t* pGyroStat);
BOOL parseAEStatHistSplit(UINT8* pBase, st_split_hist_stat_t* pSplitHist);
BOOL parseAEStatHistMerged(UINT8* pBase, st_merged_hist_stat_t* pMergedHist);
BOOL parseAEStatYSum(UINT8* pBase, st_y_sum_stat_t* pYSum);
BOOL parseAEStatThumbnail(UINT8* pBase, st_thumb_stat_t* pThumbnail);
}
#endif
#endif
