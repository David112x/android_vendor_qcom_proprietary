/*!
 * @file test_metadata.c
 *
 * @cr
 * Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <errno.h>
#ifndef VPP_GRALLOC_DOES_NOT_EXIST
#include <qdMetaData.h>
#endif

#include "vpp_def.h"
#include "vpp_dbg.h"
#include "vpp_ion.h"
#include "vpp.h"
#include "vpp_buf.h"
#include "test_metadata.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/
#define RET_IF_ERR(u32)     if (u32 != VPP_OK) \
                                return u32;

#define TOK_DYN_RANGE       "DYN_RANGE"
#define TOK_QP_SUM          "QP_SUM"
#define TOK_QP_SUM_CNT      "QP_SUM_CNT"
#define TOK_SKIP_QP_SUM     "SKIP_QP_SUM"
#define TOK_SKIP_QP_SUM_CNT "SKIP_QP_SUM_CNT"

/************************************************************************
 * Local static variables
 ***********************************************************************/

/************************************************************************
 * Forward Declarations
 ************************************************************************/

/************************************************************************
 * Local Functions
 ***********************************************************************/
// u32Sz = size of buffer in bytes
static void u32VppTestMetadata_StripLineEndings(char *pc, uint32_t u32Sz)
{
    if (!pc)
        return;

    uint32_t i = strcspn(pc, "\r\n");
    if (i < u32Sz)
        pc[i] = '\0';
}

static uint32_t u32VppTestMetadata_GetVal(t_StMetaData *pstMetaData,
                                          const char *pcKey, uint32_t *u32Val)
{
     uint32_t u32Cnt, u32Ret = VPP_ERR;

     for (u32Cnt = 0; u32Cnt < pstMetaData->u32MapSize; u32Cnt++)
     {
         if (strncmp(pcKey, pstMetaData->astMetaDataMap[u32Cnt].cHeaderTok,
                     MAX_SIZE_HEADER_TOKEN) == 0)
         {
            *u32Val = pstMetaData->astMetaDataMap[u32Cnt].val;
            u32Ret = VPP_OK;
            break;
         }
     }

     return u32Ret;
}

static uint32_t u32VppTestMetadata_ReadHeader(t_StMetaData *pstMetaData)
{
    static const uint32_t u32HeaderLen = MAX_SIZE_HEADER_TOKEN * MAX_NUM_OF_TOKENS;
    char header[u32HeaderLen];
    char *headerTok, *pcSave;

    if (fgets(header, u32HeaderLen, pstMetaData->fpMetaData) != NULL)
    {
        u32VppTestMetadata_StripLineEndings(header, u32HeaderLen);
        headerTok = strtok_r(header, ",", &pcSave);

        while ((headerTok != NULL) && (pstMetaData->u32MapSize < MAX_NUM_OF_TOKENS))
        {
           strlcpy(pstMetaData->astMetaDataMap[pstMetaData->u32MapSize].cHeaderTok,
                   headerTok, MAX_SIZE_HEADER_TOKEN);
           pstMetaData->u32MapSize++;
           headerTok = strtok_r(NULL, ",", &pcSave);
        }
    }

    if (!pstMetaData->u32MapSize)
        return VPP_ERR;
    else
        return VPP_OK;
}

static uint32_t u32VppTestMetadata_ReadData(t_StMetaData *pstMetaData)
{
    const uint32_t u32DataLen = MAX_SIZE_HEADER_TOKEN * MAX_NUM_OF_TOKENS;
    char metaData[u32DataLen];
    char *dataTok, *pcSave;
    uint32_t bRewindDone = VPP_FALSE, bContinueReading = VPP_TRUE;
    uint32_t u32Ret = VPP_OK;

    pstMetaData->bValid = VPP_FALSE;

    while (bContinueReading)
    {
        if (fgets(metaData, u32DataLen, pstMetaData->fpMetaData) != NULL)
        {
            uint32_t u32TokCnt = 0;

            u32VppTestMetadata_StripLineEndings(metaData, u32DataLen);
            dataTok = strtok_r(metaData, ",", &pcSave);

            while ((dataTok != NULL) && (u32TokCnt < pstMetaData->u32MapSize))
            {
                pstMetaData->astMetaDataMap[u32TokCnt].val = strtol(dataTok, NULL, 0);
                LOGI("Metadata: [%s]=%u", pstMetaData->astMetaDataMap[u32TokCnt].cHeaderTok,
                     pstMetaData->astMetaDataMap[u32TokCnt].val);
                u32TokCnt++;
                dataTok = strtok_r(NULL, ",", &pcSave);
            }
            pstMetaData->bValid = VPP_TRUE;
            bContinueReading = VPP_FALSE;
        }
        else
        {
            if (bRewindDone == VPP_FALSE)
            {
                fseek(pstMetaData->fpMetaData, 0, SEEK_SET);
                // Read the first line, as it contains the header info
                fgets(metaData, u32DataLen, pstMetaData->fpMetaData);
                bRewindDone = VPP_TRUE;
            }
            else
            {
                u32Ret = VPP_ERR;
                // This might happen if the file contain only the header
                bContinueReading = VPP_FALSE;
            }
        }
    }

    return u32Ret;
}

/************************************************************************
 * Global Functions
 ***********************************************************************/
uint32_t u32VppTestMetaData_Init(t_StMetaData *pstMetaData, char *pcInputPath)
{
    char cName[MAX_SIZE_FILE_NAME];
    uint32_t u32 = VPP_OK;

    VPP_RET_IF_NULL(pstMetaData, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pcInputPath, VPP_ERR_PARAM);

    pstMetaData->bValid = VPP_FALSE;

    if (strlen(pstMetaData->cMetaDataInputName))
    {
        strlcpy(cName, pcInputPath, MAX_SIZE_FILE_NAME);
        strlcat(cName, pstMetaData->cMetaDataInputName, MAX_SIZE_FILE_NAME);
        pstMetaData->fpMetaData = fopen(cName, "r");

        if (pstMetaData->fpMetaData)
        {
            uint32_t u32 = u32VppTestMetadata_ReadHeader(pstMetaData);
            if (u32)
            {
                // Looks like a empty file
                LOGE("u32VppTestMetadata_ReadHeader failed u32=%u", u32);
                vVppTestMetaData_Term(pstMetaData);
            }
        }
        else
        {
            LOGE("Cannot open file [%s]", cName);
            u32 = VPP_ERR;
        }
    }
    else
    {
        LOGD("Metadata filename NOT specified");
        u32 = VPP_ERR;
    }

    return u32;
}

void vVppTestMetaData_Term(t_StMetaData *pstMetaData)
{
    VPP_RET_VOID_IF_NULL(pstMetaData);

    if (pstMetaData->fpMetaData)
    {
       fclose(pstMetaData->fpMetaData);
       pstMetaData->fpMetaData = NULL;
    }
}

uint32_t u32VppTestMetaData_UpdateNextData(t_StMetaData *pstMetaData)
{
    uint32_t u32Ret;

    VPP_RET_IF_NULL(pstMetaData, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstMetaData->fpMetaData, VPP_ERR_PARAM);

    u32Ret = u32VppTestMetadata_ReadData(pstMetaData);

    return u32Ret;
}

uint32_t u32VppTestMetaData_GetDynamicRange(t_StMetaData *pstMetaData,
                                            uint32_t *pu32DynRange)
{
    VPP_RET_IF_NULL(pstMetaData, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pu32DynRange, VPP_ERR_PARAM);

    if (pstMetaData->bValid != VPP_TRUE)
        return VPP_ERR;

    return u32VppTestMetadata_GetVal(pstMetaData, TOK_DYN_RANGE, pu32DynRange);
}

uint32_t u32VppTestMetaData_GetQpData(t_StMetaData *pstMetaData,
                                      t_StMetaDataQp *pstQpData)
{
    VPP_RET_IF_NULL(pstMetaData, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstQpData, VPP_ERR_PARAM);

    if (pstMetaData->bValid != VPP_TRUE)
        return VPP_ERR;

    RET_IF_ERR(u32VppTestMetadata_GetVal(pstMetaData, TOK_QP_SUM,
                                         &pstQpData->u32QPSum));
    RET_IF_ERR(u32VppTestMetadata_GetVal(pstMetaData, TOK_QP_SUM_CNT,
                                         &pstQpData->u32QPSumCnt));
    RET_IF_ERR(u32VppTestMetadata_GetVal(pstMetaData, TOK_SKIP_QP_SUM,
                                         &pstQpData->u32SkipQPSum));
    RET_IF_ERR(u32VppTestMetadata_GetVal(pstMetaData, TOK_SKIP_QP_SUM_CNT,
                                         &pstQpData->u32SkipQPSumCnt));

    return VPP_OK;
}

uint32_t u32VppTestMetaData_SetColorPrimaries(void *pvBuf, uint32_t bIsHdr)
{
    uint32_t u32;
    ColorMetaData stColorMetaData;
    VPP_RET_IF_NULL(pvBuf, VPP_ERR_PARAM);

    if (bIsHdr)
    {
        stColorMetaData.colorPrimaries = ColorPrimaries_BT2020;
        stColorMetaData.transfer = Transfer_SMPTE_ST2084;
    }
    else
    {
        stColorMetaData.colorPrimaries = ColorPrimaries_Max;
        stColorMetaData.transfer = Transfer_Max;
    }

    u32 = u32VppBuf_GrallocMetaDataSet(pvBuf, COLOR_METADATA, &stColorMetaData);
    if (u32)
    {
        LOGE("u32VppBuf_GrallocMetaDataSet failed u32=%u", u32);
        return u32;
    }

    return VPP_OK;
}