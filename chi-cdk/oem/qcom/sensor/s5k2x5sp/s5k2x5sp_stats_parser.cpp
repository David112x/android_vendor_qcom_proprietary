////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  s5k2x5sp_stats_parser.cpp
/// @brief 2x5 statistics data parser
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE NC009: File names must start with camx
// NOWHINE FILE PR012: #endifs must reference the if statement

#include "s5k2x5sp_stats_parser.h"
#include "cdkutils.h"
#include "s5k2x5sp_dbglog.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#define MIN(a, b) ((a) < (b) ? (a) : (b))

UINT32 g_dumpCnt;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dumpBinary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL dumpBinary(
    const CHAR* pPath,
    VOID*       pData,
    size_t      size)
{
    if (!pData || size == 0)
    {
        return FALSE;
    }

    FILE* pFile = CdkUtils::FOpen(pPath, "wb");
    if (!pFile)
    {
        SENSOR_LOG_INFO("failed to open() : %s\n", pPath);
        return FALSE;
    }

    size_t __attribute__((unused)) written =
        CdkUtils::FWrite(pData, 1, size, pFile);
    CdkUtils::FClose(pFile);
    SENSOR_LOG_INFO("dump file : %s, size in bytes : %zu\n", pPath, written);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// fitRGBY2U16
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT16* fitRGBY2U16(
    UINT32* pChannel,
    UINT32  numOfPatch)
{
    UINT32 i;
    UINT32 tmp;
    UINT16* pOutb = reinterpret_cast<UINT16*>(pChannel);

    for (i = 0; i < numOfPatch >> 1; ++i)
    {
        tmp = pChannel[i];
        pOutb[i * 2]     = static_cast<UINT16>(tmp & 0x00000FFF);
        pOutb[i * 2 + 1] = static_cast<UINT16>((tmp >> 12) & 0x00000FFF);
    }

    return pOutb;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// fitCounter2U16
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT16* fitCounter2U16(
    UINT32* pChannel,
    UINT32 numOfPatch)
{
    UINT32 i;
    UINT32 tmp;
    UINT16* pOutb = reinterpret_cast<UINT16*>(pChannel);

    for (i = 0; i < numOfPatch >> 1; ++i)
    {
        tmp              = pChannel[i];
        pOutb[i * 2]     = static_cast<UINT16>(tmp & 0x00007FFF);
        pOutb[i * 2 + 1] = static_cast<UINT16>((tmp >> 15) & 0x00007FFF);
    }

    return pOutb;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// isValidStat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL isValidStat(
    UINT8* pBase)
{
    struct s5k2x5_embedded_metadata* pMeta = reinterpret_cast<struct s5k2x5_embedded_metadata*>(pBase);

    BOOL bIsValidStat = FALSE;

    if (big2Little(pMeta->general_info.sanity_constatnt_value) == 0xF00D)
    {
        bIsValidStat = TRUE;
    }
    SENSOR_LOG_INFO("bIsValidStat = %d", bIsValidStat);
    return bIsValidStat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// parseGyroStat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL parseGyroStat(
    UINT8*          pBase,
    st_gyro_stat_t* pGyroStat)
{
    struct s5k2x5_embedded_metadata* pMeta = reinterpret_cast<struct s5k2x5_embedded_metadata*>(pBase);
    struct gyro_synchronizer_metadata* pGyroHeader = &pMeta->gyro_syncnzr_meta;

    if (pGyroHeader->base.enable == FALSE)
    {
        return FALSE;
    }
    /* Gyro header and data address*/
    pGyroStat->gyro_header = reinterpret_cast<UINT8 *>(pGyroHeader);
    pGyroStat->gyro_data   = pBase + big2Little(pGyroHeader->base.data_start_offset);
    pGyroStat->data_size   = big2Little(pGyroHeader->base.data_total_size);

    SENSOR_LOG_INFO("gyro header addr : %p\n", pGyroStat->gyro_header);
    SENSOR_LOG_INFO("gyro data addr : %p\n", pGyroStat->gyro_data);
    SENSOR_LOG_INFO("gyro data size : %d\n", pGyroStat->data_size);

#ifdef _DEBUG_
    dumpBinary(GYRO_DSC, static_cast<void *>(pGyroStat->gyro_header), 64);
    dumpBinary(GYRO_DATA, static_cast<void *>(pGyroStat->gyro_data), pGyroStat->data_size);
#endif

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// parseAEStatHistSplit
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL parseAEStatHistSplit(
    UINT8*                pBase,
    st_split_hist_stat_t* pSplitHist)
{
    struct s5k2x5_embedded_metadata* pMeta = reinterpret_cast<struct s5k2x5_embedded_metadata*>(pBase);
    struct split_exposure_histogram_statistices_metadata* pHistHeader = &pMeta->split_exp_hist_stats_meta;

    if (pHistHeader->base.enable == FALSE)
    {
        return FALSE;
    }
    UINT8 r_mem_idx = 0;
    UINT8 g_mem_idx = 0;
    UINT8 b_mem_idx = 0;
    UINT8 y_mem_idx = 0;

    for (UINT32 i = 0; i < MAX_NUM_OF_SPLIT_HIST_CHANNEL_MEMORY; i++)
    {
        /* R histogram*/
        if ((big2Little(pHistHeader->red_ram[i]) & 0x1) == 1)
        {
            pSplitHist->split_r_hist_valid = TRUE;
            pSplitHist->split_r_hist.num_of_active_mem++;
            pSplitHist->split_r_hist.grid_en[r_mem_idx] =
                static_cast<hist_grid_type>(big2Little(pHistHeader->red_ram[i]) >> 2 & 0x1);
            pSplitHist->split_r_hist.hdr_mode[r_mem_idx] =
                static_cast<split_hdr_mode_t>(big2Little(pHistHeader->red_ram[i]) >> 3 & 0x3);
            pSplitHist->split_r_hist.num_bins[r_mem_idx] = 128 * ((big2Little(pHistHeader->red_ram[i]) >> 8 & 0x1) + 1);
            pSplitHist->split_r_hist.hist_data[r_mem_idx] = reinterpret_cast<UINT32 *>(r_mem_idx == 0 ?
                pBase + big2Little(pHistHeader->base.data_start_offset) :
                reinterpret_cast<UINT8 *>(pSplitHist->split_r_hist.hist_data[r_mem_idx-1]) +
                pSplitHist->split_r_hist.num_bins[r_mem_idx - 1] * 4);
            SENSOR_LOG_INFO("r_hist[%d] enabled\n", i);
            SENSOR_LOG_INFO("r_hist[%d] num bins : %d\n", i, pSplitHist->split_r_hist.num_bins[r_mem_idx]);
            SENSOR_LOG_INFO("r_hist[%d] grid_en  : %d\n", i, pSplitHist->split_r_hist.grid_en[r_mem_idx]);
            SENSOR_LOG_INFO("r_hist[%d] hdr_mode : %d\n", i, pSplitHist->split_r_hist.hdr_mode[r_mem_idx]);
            SENSOR_LOG_INFO("r_hist[%d] hist data addr : %p\n", i, pSplitHist->split_r_hist.hist_data[r_mem_idx]);
            r_mem_idx++;
        }
        /* G histogram*/
        if ((big2Little(pHistHeader->green_ram[i]) & 0x1) == 1)
        {
            pSplitHist->split_g_hist_valid = TRUE;
            pSplitHist->split_g_hist.num_of_active_mem++;
            pSplitHist->split_g_hist.grid_en[g_mem_idx] =
                static_cast<hist_grid_type>(big2Little(pHistHeader->green_ram[i]) >> 2 & 0x1);
            pSplitHist->split_g_hist.hdr_mode[g_mem_idx] =
                static_cast<split_hdr_mode_t>(big2Little(pHistHeader->green_ram[i]) >> 3 & 0x3);
            pSplitHist->split_g_hist.num_bins[g_mem_idx] = 128 * ((big2Little(pHistHeader->green_ram[i]) >> 8 & 0x1) + 1);
            pSplitHist->split_g_hist.hist_data[g_mem_idx] = reinterpret_cast<UINT32 *>(g_mem_idx == 0 ?
                pBase + big2Little(pHistHeader->base.data_start_offset) + big2Little(pHistHeader->tail.g_data_ofs) :
                reinterpret_cast<UINT8 *>(pSplitHist->split_g_hist.hist_data[g_mem_idx-1]) +
                pSplitHist->split_g_hist.num_bins[g_mem_idx - 1] * 4);
            SENSOR_LOG_INFO("g_hist[%d] enabled\n", i);
            SENSOR_LOG_INFO("g_hist[%d] num bins : %d\n", i, pSplitHist->split_g_hist.num_bins[g_mem_idx]);
            SENSOR_LOG_INFO("g_hist[%d] grid_en  : %d\n", i, pSplitHist->split_g_hist.grid_en[g_mem_idx]);
            SENSOR_LOG_INFO("g_hist[%d] hdr_mode : %d\n", i, pSplitHist->split_g_hist.hdr_mode[g_mem_idx]);
            SENSOR_LOG_INFO("g_hist[%d] hist data addr : %p\n", i, pSplitHist->split_g_hist.hist_data[g_mem_idx]);
            g_mem_idx++;
        }
        /* B histogram*/
        if ((big2Little(pHistHeader->blue_ram[i]) & 0x1) == 1)
        {
            pSplitHist->split_b_hist_valid = TRUE;
            pSplitHist->split_b_hist.num_of_active_mem++;
            pSplitHist->split_b_hist.grid_en[b_mem_idx] =
                static_cast<hist_grid_type>(big2Little(pHistHeader->blue_ram[i]) >> 2 & 0x1);
            pSplitHist->split_b_hist.hdr_mode[b_mem_idx] =
                static_cast<split_hdr_mode_t>(big2Little(pHistHeader->blue_ram[i]) >> 3 & 0x3);
            pSplitHist->split_b_hist.num_bins[b_mem_idx] = 128 * ((big2Little(pHistHeader->blue_ram[i]) >> 8 & 0x1) + 1);
            pSplitHist->split_b_hist.hist_data[b_mem_idx] = reinterpret_cast<UINT32 *>(b_mem_idx == 0 ?
                pBase + big2Little(pHistHeader->base.data_start_offset) + big2Little(pHistHeader->tail.b_data_ofs) :
                reinterpret_cast<UINT8 *>(pSplitHist->split_b_hist.hist_data[b_mem_idx-1]) +
                pSplitHist->split_b_hist.num_bins[b_mem_idx - 1] * 4);
            SENSOR_LOG_INFO("b_hist[%d] enabled\n", i);
            SENSOR_LOG_INFO("b_hist[%d] num bins : %d\n", i, pSplitHist->split_b_hist.num_bins[b_mem_idx]);
            SENSOR_LOG_INFO("b_hist[%d] grid_en  : %d\n", i, pSplitHist->split_b_hist.grid_en[b_mem_idx]);
            SENSOR_LOG_INFO("b_hist[%d] hdr_mode : %d\n", i, pSplitHist->split_b_hist.hdr_mode[b_mem_idx]);
            SENSOR_LOG_INFO("b_hist[%d] hist data addr : %p\n", i, pSplitHist->split_b_hist.hist_data[b_mem_idx]);
            b_mem_idx++;
        }
        /* Y histogram*/
        if ((big2Little(pHistHeader->y_ram[i]) & 0x1) == 1)
        {
            pSplitHist->split_y_hist_valid = TRUE;
            pSplitHist->split_y_hist.num_of_active_mem++;
            pSplitHist->split_y_hist.grid_en[y_mem_idx] =
                static_cast<hist_grid_type>(big2Little(pHistHeader->y_ram[i]) >> 2 & 0x1);
            pSplitHist->split_y_hist.hdr_mode[y_mem_idx] =
                static_cast<split_hdr_mode_t>(big2Little(pHistHeader->y_ram[i]) >> 3 & 0x3);
            pSplitHist->split_y_hist.num_bins[y_mem_idx] = 128 * ((big2Little(pHistHeader->y_ram[i]) >> 8 & 0x1) + 1);
            pSplitHist->split_y_hist.hist_data[y_mem_idx] = reinterpret_cast<UINT32 *>(y_mem_idx == 0 ?
                pBase + big2Little(pHistHeader->base.data_start_offset) + big2Little(pHistHeader->tail.y_data_ofs) :
                reinterpret_cast<UINT8 *>(pSplitHist->split_y_hist.hist_data[y_mem_idx-1]) +
                pSplitHist->split_y_hist.num_bins[y_mem_idx-1] * 4);
            SENSOR_LOG_INFO("y_hist[%d] enabled\n", i);
            SENSOR_LOG_INFO("y_hist[%d] num bins : %d\n", i, pSplitHist->split_y_hist.num_bins[y_mem_idx]);
            SENSOR_LOG_INFO("y_hist[%d] grid_en  : %d\n", i, pSplitHist->split_y_hist.grid_en[y_mem_idx]);
            SENSOR_LOG_INFO("y_hist[%d] hdr_mode : %d\n", i, pSplitHist->split_y_hist.hdr_mode[y_mem_idx]);
            SENSOR_LOG_INFO("y_hist[%d] hist data addr : %p\n", i, pSplitHist->split_y_hist.hist_data[y_mem_idx]);
            y_mem_idx++;
        }
    }

    SENSOR_LOG_INFO("split hist total data size : %d\n", big2Little(pHistHeader->base.data_total_size));

#ifdef _DEBUG_
    CHAR file_name[64];
    dumpBinary(HIST_SPLIT_DSC, static_cast<void *>(pHistHeader), 128);
    if (pSplitHist->split_r_hist_valid == TRUE)
    {
        for (UINT32 i = 0; i < pSplitHist->split_r_hist.num_of_active_mem; i++)
        {
            CdkUtils::SNPrintF(file_name, sizeof(file_name), HIST_SPLIT_R_DATA, i, g_dumpCnt);
            dumpBinary(file_name,
                       static_cast<void *>(pSplitHist->split_r_hist.hist_data[i]),
                       pSplitHist->split_r_hist.num_bins[i] * 4);
        }
    }
    if (pSplitHist->split_g_hist_valid == TRUE)
    {
        for (UINT32 i = 0; i < pSplitHist->split_g_hist.num_of_active_mem; i++)
        {
            CdkUtils::SNPrintF(file_name, sizeof(file_name), HIST_SPLIT_G_DATA, i, g_dumpCnt);
            dumpBinary(file_name,
                       static_cast<void *>(pSplitHist->split_g_hist.hist_data[i]),
                       pSplitHist->split_g_hist.num_bins[i] * 4);
        }
    }
    if (pSplitHist->split_b_hist_valid == TRUE)
    {
        for (UINT32 i = 0; i < pSplitHist->split_b_hist.num_of_active_mem; i++)
        {
            CdkUtils::SNPrintF(file_name, sizeof(file_name), HIST_SPLIT_B_DATA, i, g_dumpCnt);
            dumpBinary(file_name,
                       static_cast<void *>(pSplitHist->split_b_hist.hist_data[i]),
                       pSplitHist->split_b_hist.num_bins[i] * 4);
        }
    }
    if (pSplitHist->split_y_hist_valid == TRUE)
    {
        for (UINT32 i = 0; i < pSplitHist->split_y_hist.num_of_active_mem; i++)
        {
            CdkUtils::SNPrintF(file_name, sizeof(file_name), HIST_SPLIT_Y_DATA, i, g_dumpCnt);
            dumpBinary(file_name,
                       static_cast<void *>(pSplitHist->split_y_hist.hist_data[i]),
                       pSplitHist->split_y_hist.num_bins[i] * 4);
        }
    }
#endif

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// parseAEStatHistMerged
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL parseAEStatHistMerged(
    UINT8* pBase,
    st_merged_hist_stat_t* pMergedHist)
{
    struct s5k2x5_embedded_metadata* pMeta = reinterpret_cast<struct s5k2x5_embedded_metadata*>(pBase);
    struct merged_exposure_histogram_statistices_metadata* pHistMergedHeader = &pMeta->merged_exp_hist_stats_meta;

    if (pHistMergedHeader->base.enable == FALSE)
    {
        return FALSE;
    }

    UINT8 r_mem_idx = 0;
    UINT8 g_mem_idx = 0;
    UINT8 b_mem_idx = 0;
    UINT8 y_mem_idx = 0;

    for (UINT32 i = 0; i < MAX_NUM_OF_MERGED_HIST_CHANNEL_MEMORY; i++)
    {
        /* R histogram*/
        if ((big2Little(pHistMergedHeader->red_ram[i]) & 0x1) == 1)
        {
            pMergedHist->merged_r_hist_valid = TRUE;
            pMergedHist->merged_r_hist.num_of_active_mem++;
            pMergedHist->merged_r_hist.grid_en[r_mem_idx] =
                static_cast<hist_grid_type>((big2Little(pHistMergedHeader->red_ram[i]) >> 2) & 0x1);
            pMergedHist->merged_r_hist.hist_data[r_mem_idx] = reinterpret_cast<UINT32 *>(r_mem_idx == 0 ?
                pBase + big2Little(pHistMergedHeader->base.data_start_offset) :
                reinterpret_cast<UINT8 *>(pMergedHist->merged_r_hist.hist_data[r_mem_idx-1] + 256 * 4));
            SENSOR_LOG_INFO("r_hist[%d] enabled\n", i);
            SENSOR_LOG_INFO("r_hist[%d] grid_en  : %d\n", i, pMergedHist->merged_r_hist.grid_en[r_mem_idx]);
            SENSOR_LOG_INFO("r_hist[%d] hist data addr : %p\n", i, pMergedHist->merged_r_hist.hist_data[r_mem_idx]);
            r_mem_idx++;
        }
        /* G histogram*/
        if ((big2Little(pHistMergedHeader->green_ram[i]) & 0x1) == 1)
        {
            pMergedHist->merged_g_hist_valid = TRUE;
            pMergedHist->merged_g_hist.num_of_active_mem++;
            pMergedHist->merged_g_hist.grid_en[g_mem_idx] =
                static_cast<hist_grid_type>((big2Little(pHistMergedHeader->green_ram[i]) >> 2) & 0x1);
            pMergedHist->merged_g_hist.hist_data[g_mem_idx] = reinterpret_cast<UINT32 *>(g_mem_idx == 0 ?
                pBase + big2Little(pHistMergedHeader->base.data_start_offset) + big2Little(pHistMergedHeader->tail.g_data_ofs) :
                reinterpret_cast<UINT8 *>(pMergedHist->merged_g_hist.hist_data[g_mem_idx-1] + 256 * 4));
            SENSOR_LOG_INFO("g_hist[%d] enabled\n", i);
            SENSOR_LOG_INFO("g_hist[%d] grid_en  : %d\n", i, pMergedHist->merged_g_hist.grid_en[g_mem_idx]);
            SENSOR_LOG_INFO("g_hist[%d] hist data addr : %p\n", i, pMergedHist->merged_g_hist.hist_data[g_mem_idx]);
            g_mem_idx++;
        }
        /* B histogram*/
        if ((big2Little(pHistMergedHeader->blue_ram[i]) & 0x1) == 1)
        {
            pMergedHist->merged_b_hist_valid = TRUE;
            pMergedHist->merged_b_hist.num_of_active_mem++;
            pMergedHist->merged_b_hist.grid_en[b_mem_idx] =
                static_cast<hist_grid_type>((big2Little(pHistMergedHeader->blue_ram[i]) >> 2) & 0x1);
            pMergedHist->merged_b_hist.hist_data[b_mem_idx] = reinterpret_cast<UINT32 *>(b_mem_idx == 0 ?
                pBase + big2Little(pHistMergedHeader->base.data_start_offset) + big2Little(pHistMergedHeader->tail.b_data_ofs) :
                reinterpret_cast<UINT8 *>(pMergedHist->merged_b_hist.hist_data[b_mem_idx-1] + 256 * 4));
            SENSOR_LOG_INFO("b_hist[%d] enabled\n", i);
            SENSOR_LOG_INFO("b_hist[%d] grid_en  : %d\n", i, pMergedHist->merged_b_hist.grid_en[b_mem_idx]);
            SENSOR_LOG_INFO("b_hist[%d] hist data addr : %p\n", i, pMergedHist->merged_b_hist.hist_data[b_mem_idx]);
            b_mem_idx++;
        }
        /* Y histogram*/
        if ((big2Little(pHistMergedHeader->y_ram[i]) & 0x1) == 1)
        {
            pMergedHist->merged_y_hist_valid = TRUE;
            pMergedHist->merged_y_hist.num_of_active_mem++;
            pMergedHist->merged_y_hist.grid_en[y_mem_idx] =
                static_cast<hist_grid_type>((big2Little(pHistMergedHeader->y_ram[i]) >> 2) & 0x1);
            pMergedHist->merged_y_hist.hist_data[y_mem_idx] = reinterpret_cast<UINT32 *>(y_mem_idx == 0 ?
                pBase + big2Little(pHistMergedHeader->base.data_start_offset) + big2Little(pHistMergedHeader->tail.y_data_ofs) :
                reinterpret_cast<UINT8 *>(pMergedHist->merged_y_hist.hist_data[y_mem_idx-1] + 256 * 4));
            SENSOR_LOG_INFO("y_hist[%d] enabled\n", i);
            SENSOR_LOG_INFO("y_hist[%d] grid_en  : %d\n", i, pMergedHist->merged_y_hist.grid_en[y_mem_idx]);
            SENSOR_LOG_INFO("y_hist[%d] hist data addr : %p\n", i, pMergedHist->merged_y_hist.hist_data[y_mem_idx]);
            y_mem_idx++;
        }
    }

    SENSOR_LOG_INFO("merged hist total data size : %d\n", big2Little(pHistMergedHeader->base.data_total_size));

#ifdef _DEBUG_
    CHAR file_name[64];
    dumpBinary(HIST_MERGED_DSC, static_cast<void *>(pHistMergedHeader), 64);
    if (pMergedHist->merged_r_hist_valid == TRUE)
    {
        for (UINT32 i = 0; i < pMergedHist->merged_r_hist.num_of_active_mem; i++)
        {
            CdkUtils::SNPrintF(file_name, sizeof(file_name), HIST_MERGED_R_DATA, i, g_dumpCnt);
            dumpBinary(file_name, static_cast<void *>(pMergedHist->merged_r_hist.hist_data[i]), 1024);
        }
    }
    if (pMergedHist->merged_g_hist_valid == TRUE)
    {
        for (UINT32 i = 0; i < pMergedHist->merged_g_hist.num_of_active_mem; i++)
        {
            CdkUtils::SNPrintF(file_name, sizeof(file_name), HIST_MERGED_G_DATA, i, g_dumpCnt);
            dumpBinary(file_name, static_cast<void *>(pMergedHist->merged_g_hist.hist_data[i]), 1024);
        }
    }
    if (pMergedHist->merged_b_hist_valid == TRUE)
    {
        for (UINT32 i = 0; i < pMergedHist->merged_b_hist.num_of_active_mem; i++)
        {
            CdkUtils::SNPrintF(file_name, sizeof(file_name), HIST_MERGED_B_DATA, i, g_dumpCnt);
            dumpBinary(file_name, static_cast<void *>(pMergedHist->merged_b_hist.hist_data[i]), 1024);
        }
    }
    if (pMergedHist->merged_y_hist_valid == TRUE)
    {
        for (UINT32 i = 0; i < pMergedHist->merged_y_hist.num_of_active_mem; i++)
        {
            CdkUtils::SNPrintF(file_name, sizeof(file_name), HIST_MERGED_Y_DATA, i, g_dumpCnt);
            dumpBinary(file_name, static_cast<void *>(pMergedHist->merged_y_hist.hist_data[i]), 1024);
        }
    }
#endif

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// parseAEStatYSum
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL parseAEStatYSum(
    UINT8* pBase,
    st_y_sum_stat_t* pYSum)
{
    struct s5k2x5_embedded_metadata* pMeta = (struct s5k2x5_embedded_metadata *)pBase;
    struct ysum_statistices_metadata* pYsumHeader = &pMeta->ysum_stats_meta;

    if (pYsumHeader->base.enable == FALSE)
    {
        return FALSE;
    }

    /* Y sum Meta dump*/
    pYSum->y_sum_valid = TRUE;
    pYSum->y_sum_data.num_of_active_mem = 3;
    for (UINT32 i = 0; i < MAX_NUM_OF_YSUM_MEMORY; i++)
    {
        pYSum->y_sum_data.num_of_strips = big2Little(pYsumHeader->num_of_stripes);
        pYSum->y_sum_data.hdr_mode[i] = static_cast<ysum_hdr_mode_t>(pYsumHeader->type_of_data_mem[i]);
        pYSum->y_sum_data.ysum_data[i] = reinterpret_cast<UINT32 *>(pBase + big2Little(pYsumHeader->base.data_start_offset) +
            (i == 1 ? big2Little(pYsumHeader->offset_to_mem_1):
            (i == 2 ? big2Little(pYsumHeader->offset_to_mem_1) : 0)));
    }

    SENSOR_LOG_INFO("y sum num_of_stripes : %d\n", pYSum->y_sum_data.num_of_strips);
    SENSOR_LOG_INFO("y sum mem 0 hdr_mode : %d\n", pYSum->y_sum_data.hdr_mode[0]);
    SENSOR_LOG_INFO("y sum mem 1 hdr_mode : %d\n", pYSum->y_sum_data.hdr_mode[1]);
    SENSOR_LOG_INFO("y sum mem 2 hdr_mode : %d\n", pYSum->y_sum_data.hdr_mode[2]);
    SENSOR_LOG_INFO("y sum total data size : %d\n", big2Little(pYsumHeader->base.data_total_size));

#ifdef _DEBUG_
    CHAR file_name[64];
    dumpBinary(Y_SUM_DSC, static_cast<void *>(pYsumHeader), 64);
    if (pYSum->y_sum_valid == TRUE)
    {
        for (UINT32 i = 0; i < pYSum->y_sum_data.num_of_active_mem; i++)
        {
            CdkUtils::SNPrintF(file_name, sizeof(file_name), Y_SUM_DATA, i, g_dumpCnt);
            dumpBinary(file_name, static_cast<void *>(pYSum->y_sum_data.ysum_data[i]), pYSum->y_sum_data.num_of_strips * 4);
        }
    }
#endif

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// parseAEStatThumbnail
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL parseAEStatThumbnail(
    UINT8* pBase,
    st_thumb_stat_t* pThumbnail)
{
    struct s5k2x5_embedded_metadata* pMeta = (struct s5k2x5_embedded_metadata *)pBase;
    struct thumbnail_statistices_metadata* pThumbHeader = &pMeta->thumb_stats_meta;
    UINT32* pRData;
    UINT32* pGData;
    UINT32* pBData;
    UINT32* pYData;
    UINT32* pCounterData;

    if (pThumbHeader->base.enable == FALSE)
    {
        return FALSE;
    }

    /* Thumb Meta dump*/
    pThumbnail->thumb_nail_valid = TRUE;
    pThumbnail->region_h_num = big2Little(pThumbHeader->number_of_patches_in_strip);
    pThumbnail->region_v_num = big2Little(pThumbHeader->number_of_strips);
    pThumbnail->region_width = big2Little(pThumbHeader->patch_array_patch_width);
    pThumbnail->region_height = big2Little(pThumbHeader->patch_array_patch_height);
    pThumbnail->region_pixel_cnt = pThumbnail->region_width * pThumbnail->region_height;
    pThumbnail->hdr_mode = static_cast<thumb_hdr_mode_t>(pThumbHeader->type_of_data);
    pThumbnail->num_of_patch = big2Little(pThumbHeader->number_of_patches);

    pRData = reinterpret_cast<UINT32 *>(pBase + big2Little(pThumbHeader->base.data_start_offset));
    pGData = reinterpret_cast<UINT32 *>(reinterpret_cast<UINT8 *>(pRData) + big2Little(pThumbHeader->green_offset_bytes));
    pBData = reinterpret_cast<UINT32 *>(reinterpret_cast<UINT8 *>(pRData) + big2Little(pThumbHeader->blue_offset_bytes));
    pYData = reinterpret_cast<UINT32 *>(reinterpret_cast<UINT8 *>(pRData) + big2Little(pThumbHeader->y_offset_bytes));
    pCounterData = reinterpret_cast<UINT32 *>(reinterpret_cast<UINT8 *>(pRData) +
        big2Little(pThumbHeader->counter_offset_bytes));

    pThumbnail->thumb_nail_data.r_avg = fitRGBY2U16(pRData, pThumbnail->num_of_patch);
    pThumbnail->thumb_nail_data.g_avg = fitRGBY2U16(pGData, pThumbnail->num_of_patch);
    pThumbnail->thumb_nail_data.b_avg = fitRGBY2U16(pBData, pThumbnail->num_of_patch);
    pThumbnail->thumb_nail_data.y_avg = fitRGBY2U16(pYData, pThumbnail->num_of_patch);
    pThumbnail->thumb_nail_data.counter = fitCounter2U16(pCounterData, pThumbnail->num_of_patch);

    SENSOR_LOG_INFO("thumb patch h num : %d\n", pThumbnail->region_h_num);
    SENSOR_LOG_INFO("thumb patch v num : %d\n", pThumbnail->region_v_num);
    SENSOR_LOG_INFO("thumb patch num : %d\n", pThumbnail->num_of_patch);
    SENSOR_LOG_INFO("thumb hdr mode : %d\n", pThumbnail->hdr_mode);
    SENSOR_LOG_INFO("thumb patch width : %d\n", pThumbnail->region_width);
    SENSOR_LOG_INFO("thumb patch height : %d\n", pThumbnail->region_height);
    SENSOR_LOG_INFO("thumb total data size : %d\n", big2Little(pThumbHeader->base.data_total_size));

#ifdef _DEBUG_
    CHAR file_name[64];
    dumpBinary(THUMB_DSC, static_cast<void *>(pThumbHeader), 64);
    if (pThumbnail->thumb_nail_valid == TRUE)
    {
        CdkUtils::SNPrintF(file_name, sizeof(file_name), THUMB_R_DATA, g_dumpCnt);
        dumpBinary(file_name, static_cast<void *>(pThumbnail->thumb_nail_data.r_avg), pThumbnail->num_of_patch * 2);
        CdkUtils::SNPrintF(file_name, sizeof(file_name), THUMB_G_DATA, g_dumpCnt);
        dumpBinary(file_name, static_cast<void *>(pThumbnail->thumb_nail_data.g_avg), pThumbnail->num_of_patch * 2);
        CdkUtils::SNPrintF(file_name, sizeof(file_name), THUMB_B_DATA, g_dumpCnt);
        dumpBinary(file_name, static_cast<void *>(pThumbnail->thumb_nail_data.b_avg), pThumbnail->num_of_patch * 2);
        CdkUtils::SNPrintF(file_name, sizeof(file_name), THUMB_Y_DATA, g_dumpCnt);
        dumpBinary(file_name, static_cast<void *>(pThumbnail->thumb_nail_data.y_avg), pThumbnail->num_of_patch * 2);
        CdkUtils::SNPrintF(file_name, sizeof(file_name), THUMB_COUNTER_DATA, g_dumpCnt);
        dumpBinary(file_name, static_cast<void *>(pThumbnail->thumb_nail_data.counter), pThumbnail->num_of_patch * 2);
    }
    g_dumpCnt++;
#endif

    return TRUE;
}
#ifdef __cplusplus
}
#endif // __cplusplus
