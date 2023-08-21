/*!
 * @file test_metadata.h
 *
 * @cr
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */
#ifndef _TEST_METADATA_H_
#define _TEST_METADATA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define MAX_SIZE_FILE_NAME      256
#define MAX_SIZE_HEADER_TOKEN   64
#define MAX_NUM_OF_TOKENS       32

typedef struct {
    uint32_t u32QPSum;
    uint32_t u32QPSumCnt;
    uint32_t u32SkipQPSum;
    uint32_t u32SkipQPSumCnt;
} t_StMetaDataQp;

typedef struct {
    char cHeaderTok[MAX_SIZE_HEADER_TOKEN];
    uint32_t val;
} t_StMetaDataMap;

typedef struct {
    char cMetaDataInputName[MAX_SIZE_FILE_NAME];
    FILE *fpMetaData;
    uint32_t u32MapSize;
    uint32_t bValid;
    t_StMetaDataMap astMetaDataMap[MAX_NUM_OF_TOKENS];
} t_StMetaData;

/***************************************************************************
 * Function Prototypes
 ***************************************************************************/
uint32_t u32VppTestMetaData_Init(t_StMetaData *pstMetaData, char *pcInputPath);
void vVppTestMetaData_Term(t_StMetaData *pstMetaData);
uint32_t u32VppTestMetaData_UpdateNextData(t_StMetaData *pstMetaData);
uint32_t u32VppTestMetaData_GetDynamicRange(t_StMetaData *pstMetaData,
                                            uint32_t *pu32DynRange);
uint32_t u32VppTestMetaData_GetQpData(t_StMetaData *pstMetaData, t_StMetaDataQp *pstQpData);
uint32_t u32VppTestMetaData_SetColorPrimaries(void *pvBuf, uint32_t bIsHdr);

#ifdef __cplusplus
 }
#endif

#endif /* _TEST_METADATA_H_ */
