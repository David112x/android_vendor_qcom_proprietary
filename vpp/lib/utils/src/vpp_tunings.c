/*!
 * @file vpp_tunings.c
 *
 * @cr
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define VPP_LOG_TAG     VPP_LOG_MODULE_TUNINGS_TAG
#define VPP_LOG_MODULE  VPP_LOG_MODULE_TUNINGS
#include "vpp_def.h"
#include "vpp_dbg.h"
#include "vpp_utils.h"
#include "vpp_tunings.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/

#define MAX_CHAR_PER_ROW 256
#define TUNINGS_FILE_DELIM ","

/************************************************************************
 * Local Functions
 ***********************************************************************/

static t_StTuning *pstVppTunings_AllocTuning(const t_StTuningDef *pstDef,
                                             t_StTuningBlock *pstTuningBlock)
{
    if (!pstDef || !pstTuningBlock)
        return NULL;

    t_StTuning *pstTuning = calloc(1, sizeof(t_StTuning));
    if (!pstTuning)
        goto err_alloc_top;

    pstTuning->pstDef = pstDef;
    pstTuning->puVal = calloc(pstDef->u32Count, sizeof(t_UTuningValue));
    if (!pstTuning->puVal)
        goto err_alloc_val;

    pstTuning->pstNext = pstTuningBlock->pstTuningList;
    pstTuningBlock->pstTuningList = pstTuning;
    pstTuningBlock->u32TuningListCnt++;

    return pstTuning;

err_alloc_val:
    free(pstTuning);

err_alloc_top:
    return NULL;
}

static void vVppTunings_FreeTuning(t_StTuning *pstTuning)
{
    if (!pstTuning)
        return;

    if (pstTuning->puVal)
        free(pstTuning->puVal);

    free(pstTuning);
}

static void vVppTunings_FreeAllTunings(t_StTuningBlock *pstTuningBlock)
{
    uint32_t u32 = 0;
    t_StTuning *pstTuning;

    if (!pstTuningBlock)
        return;

    while (pstTuningBlock->pstTuningList)
    {
        pstTuning = pstTuningBlock->pstTuningList;
        pstTuningBlock->pstTuningList = pstTuning->pstNext;
        vVppTunings_FreeTuning(pstTuning);
        u32++;
        pstTuningBlock->u32TuningListCnt--;
    }

    LOGD("free'd %u tunings", u32);
    LOGE_IF(pstTuningBlock->u32TuningListCnt,
            "u32TuningListCnt=%u, should be 0 after free!",
            pstTuningBlock->u32TuningListCnt);

    pstTuningBlock->u32TuningListCnt = 0;
    pstTuningBlock->u32TuningListValidCnt = 0;
}

static t_StTuning *pstVppTunings_Find(const t_StTuningDef *pstDef,
                                      t_StTuningBlock *pstTuningBlock)
{
    if (!pstDef || !pstTuningBlock)
        return NULL;

    t_StTuning *pstTuning = pstTuningBlock->pstTuningList;

    while (pstTuning)
    {
        if (pstTuning->pstDef == pstDef)
        {
            // LOGI("found existing tuning at %u", u32);
            return pstTuning;
        }
        pstTuning = pstTuning->pstNext;
    }

    // LOGI("unable to find existing tuning");
    return NULL;
}

#define VALIDATE_TUNING(_type, _pt) \
    if ((_pt)->pstDef->eType == TUNING_TYPE_##_type) \
    { \
        uint32_t i; \
        for (i = 0; i < (_pt)->pstDef->u32Count; i++) \
        { \
            if ((_pt)->puVal[i]._type > (_pt)->pstDef->max._type || \
                (_pt)->puVal[i]._type < (_pt)->pstDef->min._type) \
                return VPP_ERR; \
        } \
        return VPP_OK; \
    }

static uint32_t u32VppTunings_Validate(t_StTuning *pstTuning)
{
    uint32_t u32;

    if (!pstTuning || !pstTuning->pstDef || !pstTuning->puVal)
        return VPP_ERR;

    if (pstTuning->pstDef->u32Count != pstTuning->u32ExternalCnt)
    {
        LOGE("incorrect number of tuning params, ext_cnt=%u, exp=%u",
             pstTuning->pstDef->u32Count, pstTuning->u32ExternalCnt);
        return VPP_ERR;
    }

    if (pstTuning->pstDef->eType == TUNING_TYPE_FLOAT)
    {
        for (u32 = 0; u32 < pstTuning->pstDef->u32Count; u32++)
        {
            // Check for NaN values.
            if (isnan(pstTuning->puVal[u32].FLOAT))
            {
                return VPP_ERR;
            }
        }
    }

    if (pstTuning->pstDef->pfVal)
        return pstTuning->pstDef->pfVal(pstTuning);

    VALIDATE_TUNING(FLOAT, pstTuning);
    VALIDATE_TUNING(U32, pstTuning);
    VALIDATE_TUNING(S32, pstTuning);

    return VPP_ERR;
}

static uint32_t u32VppTunings_Invalidate(t_StTuning *pstTuning)
{
    VPP_RET_IF_NULL(pstTuning, VPP_ERR_PARAM);

    pstTuning->u32ExternalCnt = 0;
    pstTuning->bValid = VPP_FALSE;

    return VPP_OK;
}

static const t_StTuningDef *pstVppTunings_GetDefByStrId(t_StTuningBlock *pstTuningBlock,
                                                        char *pcId, uint32_t u32Sz)
{
    const t_StTuningDef *pstDef;
    uint32_t i, u32CmpLen;

    VPP_RET_IF_NULL(pstTuningBlock, NULL);

    LOGI("searching for: '%s', sz=%u", pcId, u32Sz);
    for (i = 0; i < pstTuningBlock->u32TuningDefCnt; i++)
    {
        pstDef = &pstTuningBlock->pstTuningDef[i];

        u32CmpLen = strlen(pstDef->acId);
        if (u32Sz != u32CmpLen ||
            strncmp(pstDef->acId, pcId, u32CmpLen))
        {
            // LOGI(".. [%u], def->id=%s, def->sz=%u", i, pstDef->pcId, u32CmpLen);
            continue;
        }

        LOGI(".. found at [%u], def->id=%s, def->sz=%u",
             i, pstDef->acId, u32CmpLen);
        return pstDef;
    }

    return NULL;
}

static const t_StTuningDef *pstVppTunings_GetDefById(t_StTuningBlock *pstTuningBlock,
                                                     uint32_t u32Id)
{
    const t_StTuningDef *pstDef;
    uint32_t i;

    VPP_RET_IF_NULL(pstTuningBlock, NULL);

    LOGI("searching for: id=%u", u32Id);
    for (i = 0; i < pstTuningBlock->u32TuningDefCnt; i++)
    {
        pstDef = &pstTuningBlock->pstTuningDef[i];

        if (u32Id != pstDef->u32Id)
            continue;

        LOGI("found at [%u], def->id=%s", i, pstDef->acId);
        return pstDef;
    }

    return NULL;
}

static void DUMP_DEF(const t_StTuningDef *pstDef)
{
    LOGD("DUMP_DEF: %s (%u) - type=%u, cnt=%u", pstDef->acId, pstDef->u32Id,
         pstDef->eType, pstDef->u32Count);
}

static void DUMP_TUNING(const t_StTuning *pstTuning)
{
#define SIZE 256
    char c[SIZE];
    int rem, len = 0;
    uint32_t i;
    t_ETuningType eType;

    if (!pstTuning || !pstTuning->bValid)
        return;

    c[0] = '\0';
    eType = pstTuning->pstDef->eType;

    for (i = 0, rem = SIZE;
         i < pstTuning->u32ExternalCnt && rem > 0;
         i++, rem = SIZE - len)
    {
        if (eType == TUNING_TYPE_U32)
            len += snprintf(&c[len], rem, "%u, ", pstTuning->puVal[i].U32);
        else if (eType == TUNING_TYPE_S32)
            len += snprintf(&c[len], rem, "%d, ", pstTuning->puVal[i].S32);
        else if (eType == TUNING_TYPE_FLOAT)
            len += snprintf(&c[len], rem, "%f, ", pstTuning->puVal[i].FLOAT);
    }

    if (len)
        c[len - 2] = '\0';

    LOGD("%s: %s", pstTuning->pstDef->acId, c);
#undef SIZE
}

static char *strip_inplace(char *pc)
{
    uint32_t u32;
    char *pe;

    if (!pc)
        return pc;

    // strip leading whitespace
    u32 = 0;
    while (isspace((unsigned char)*pc))
    {
        u32++;
        pc++;
    }
    //LOGI("removed %u leading whitespaces. current string=%s", u32, pc);

    // trailing whitespace
    u32 = 0;
    pe = pc + strlen(pc) - 1;
    while (pe > pc && isspace((unsigned char)*pe))
    {
        u32++;
        pe--;
    }
    *(pe + 1) = '\0';
    //LOGI("removed %u trailing whitespaces. current string=%s", u32, pc);

    return pc;
}

// u32Sz = size of buffer in bytes
static char *strip_line_endings_inplace(char *pc, uint32_t u32Sz)
{
    if (!pc)
        return pc;

    uint32_t i = strcspn(pc, "\r\n");
    if (i < u32Sz)
    {
        pc[i] = '\0';
    }

    return pc;
}

static uint32_t u32VppTunings_ReadExternalTunings(t_StTuningBlock *pstTuningBlock)
{
    t_StTuning *pstTuning = NULL;
    const t_StTuningDef *pstDef = NULL;
    char cBuf[MAX_CHAR_PER_ROW];
    char *pcToken, *pcSave;
    uint32_t i;
    FILE *fp = NULL;

    VPP_RET_IF_NULL(pstTuningBlock, VPP_ERR_PARAM);

    fp = fopen(pstTuningBlock->acFileName, "r");
    if (!fp)
    {
        LOGE("unable to open file: %s, err=%d (%s)", pstTuningBlock->acFileName,
             errno, strerror(errno));
        return VPP_ERR_RESOURCES;
    }
    else
    {
        LOGI("opened file: %s", pstTuningBlock->acFileName);
    }

    // if fgets is not null, it will always return the buffer with a null
    // terminated string, so it should be able to pass to functions which
    // require a null terminated buffer.
    while (fgets(cBuf, MAX_CHAR_PER_ROW, fp))
    {

        LOGI("-----");
        pstDef = NULL;

        strip_line_endings_inplace(cBuf, MAX_CHAR_PER_ROW);

        LOGD("read in: '%s'", cBuf);
        pcToken = strtok_r(cBuf, TUNINGS_FILE_DELIM, &pcSave);
        if (!pcToken)
        {
            LOGE("unable to find tuning id: '%s'", pcToken);
            continue;
        }

        pcToken = strip_inplace(pcToken);
        pstDef = pstVppTunings_GetDefByStrId(pstTuningBlock, pcToken, strlen(pcToken));
        if (!pstDef)
        {
            LOGI("unable to find tuning definition for id: '%s'", pcToken);
            continue;
        }
        DUMP_DEF(pstDef);

        pstTuning = pstVppTunings_Find(pstDef, pstTuningBlock);
        if (!pstTuning)
        {
            LOGI("unable to find existing tuning, allocating...");
            pstTuning = pstVppTunings_AllocTuning(pstDef, pstTuningBlock);
        }

        if (!pstTuning)
        {
            LOGE("unable to find or allocate tuning structure");
            break;
        }

        if (pstTuning->bValid)
            pstTuningBlock->u32TuningListValidCnt--;
        u32VppTunings_Invalidate(pstTuning);

        // load external values into internal structure
        for (i = 0; i < pstDef->u32Count; i++)
        {
            pcToken = strtok_r(NULL, TUNINGS_FILE_DELIM, &pcSave);
            if (!pcToken)
                break;
            pcToken = strip_inplace(pcToken);

            LOGD("token: '%s'", pcToken);
            if (pstDef->eType == TUNING_TYPE_FLOAT)
            {
                pstTuning->puVal[i].FLOAT = strtod(pcToken, NULL);
            }
            else if (pstDef->eType == TUNING_TYPE_U32)
            {
                pstTuning->puVal[i].U32 = strtoul(pcToken, NULL, 0);
            }
            else if (pstDef->eType == TUNING_TYPE_S32)
            {
                pstTuning->puVal[i].S32 = strtol(pcToken, NULL, 0);
            }
        }
        pstTuning->u32ExternalCnt = i;

        if (u32VppTunings_Validate(pstTuning) == VPP_OK)
        {
            LOGD("setting valid on tuning");
            pstTuning->bValid = VPP_TRUE;
            pstTuningBlock->u32TuningListValidCnt++;
        }
        DUMP_TUNING(pstTuning);
    }

    fclose(fp);

    return VPP_OK;
}

/************************************************************************
 * Global Functions
 ***********************************************************************/

void *vpVppTunings_Init(const char *pcFileName,
                        const t_StTuningDef *pstTuningDefSrc,
                        uint32_t u32TuningDefCnt)
{
    uint32_t u32;
    t_StTuningBlock *pstTuningBlock = NULL;
    t_StTuningDef *pstTuningDef = NULL;

    VPP_RET_IF_NULL(pcFileName, NULL);
    VPP_RET_IF_NULL(pstTuningDefSrc, NULL);
    VPP_RET_EQ(u32TuningDefCnt, 0, NULL);

    pstTuningBlock = calloc(1, sizeof(t_StTuningBlock));
    if (!pstTuningBlock)
    {
        LOGE("Error allocating pstTuningBlock");
        goto ERR_ALLOC_BLOCK;
    }

    pstTuningDef = calloc(u32TuningDefCnt, sizeof(t_StTuningDef));
    if (!pstTuningDef)
    {
        LOGE("Error allocating pstTuningDef");
        goto ERR_ALLOC_DEF;
    }

    memcpy(pstTuningDef, pstTuningDefSrc,
           u32TuningDefCnt * sizeof(t_StTuningDef));
    strlcpy(pstTuningBlock->acFileName, pcFileName,
            VPP_TUNINGS_MAX_FILE_NAME_SIZE);
    pstTuningBlock->u32TuningDefCnt = u32TuningDefCnt;
    pstTuningBlock->pstTuningDef = pstTuningDef;

    u32 = u32VppTunings_ReadExternalTunings(pstTuningBlock);
    if (u32 == VPP_ERR_RESOURCES)
    {
        LOGD("unable to read external tunings file.");
        goto ERR_READ_TUNINGS;
    }
    else if (u32 != VPP_OK)
    {
        LOGE("error reading external tunings, u32=%u", u32);
        goto ERR_READ_TUNINGS;
    }

    return pstTuningBlock;

ERR_READ_TUNINGS:
    free(pstTuningDef);

ERR_ALLOC_DEF:
    free(pstTuningBlock);

ERR_ALLOC_BLOCK:
    return NULL;
}

void vVppTunings_Term(void *pstCb)
{
    t_StTuningBlock *pstTuningBlock;

    VPP_RET_VOID_IF_NULL(pstCb);

    pstTuningBlock = pstCb;

    vVppTunings_FreeAllTunings(pstTuningBlock);

    if (pstTuningBlock->pstTuningDef)
        free(pstTuningBlock->pstTuningDef);

    free(pstTuningBlock);
}

uint32_t u32VppTunings_GetValidTuningsCount(void *pstCb)
{
    t_StTuningBlock *pstTuningBlock;

    VPP_RET_IF_NULL(pstCb, 0);

    pstTuningBlock = pstCb;

    return pstTuningBlock->u32TuningListValidCnt;
}

t_StTuning *pstVppTunings_GetTuningByIndex(void *pstCb, uint32_t u32Idx)
{
    t_StTuningBlock *pstTuningBlock;
    t_StTuning *pstTuning = NULL;
    uint32_t u32Cnt;

    VPP_RET_IF_NULL(pstCb, NULL);

    pstTuningBlock = pstCb;
    if (u32Idx >= pstTuningBlock->u32TuningListValidCnt)
    {
        LOGE("Index %u >= u32TuningListValidCnt %u", u32Idx,
             pstTuningBlock->u32TuningListValidCnt);
        return NULL;
    }
    u32Cnt = u32Idx;

    pstTuning = pstTuningBlock->pstTuningList;
    while (pstTuning)
    {
        if (pstTuning->bValid)
        {
            if (u32Cnt)
                u32Cnt--;
            else
                break;
        }
        pstTuning = pstTuning->pstNext;
    }

    LOGE_IF(pstTuning == NULL, "No valid tuning found at index=%u", u32Idx);
    return pstTuning;
}

t_StTuning *pstVppTunings_GetTuningById(void *pstCb, uint32_t u32TuningId)
{
    t_StTuningBlock *pstTuningBlock;
    const t_StTuningDef *pstDef = NULL;
    t_StTuning *pstTuning = NULL;

    VPP_RET_IF_NULL(pstCb, NULL);

    pstTuningBlock = pstCb;
    pstDef = pstVppTunings_GetDefById(pstTuningBlock, u32TuningId);
    if (!pstDef)
    {
        LOGE("unable to find tuning definition for id=%u", u32TuningId);
        return NULL;
    }

    pstTuning = pstVppTunings_Find(pstDef, pstTuningBlock);

    if (pstTuning == NULL)
    {
        LOGD("No tuning found for id=%u", u32TuningId);
        return NULL;
    }
    if (!pstTuning->bValid)
    {
        LOGD("Invalid tuning for id=%u", u32TuningId);
        return NULL;
    }

    return pstTuning;
}

uint32_t u32VppTunings_GetTuningCount(t_StTuning *pstTuning)
{
    VPP_RET_IF_NULL(pstTuning, 0);
    VPP_RET_IF_NULL(pstTuning->pstDef, 0);

    return pstTuning->pstDef->u32Count;
}

uint32_t u32VppTunings_GetTuningCountById(void *pstCb, uint32_t u32TuningId)
{
    t_StTuning *pstTuning;

    VPP_RET_IF_NULL(pstCb, 0);

    pstTuning = pstVppTunings_GetTuningById(pstCb, u32TuningId);
    if (!pstTuning)
    {
        LOGE("unable to find valid tuning for Id=%u", u32TuningId);
        return 0;
    }

    return u32VppTunings_GetTuningCount(pstTuning);
}

uint32_t u32VppTunings_GetTuningCountByIndex(void *pstCb, uint32_t u32Idx)
{
    t_StTuning *pstTuning = NULL;

    VPP_RET_IF_NULL(pstCb, 0);

    pstTuning = pstVppTunings_GetTuningByIndex(pstCb, u32Idx);
    if (!pstTuning)
    {
        LOGE("unable to find valid tuning for index=%u", u32Idx);
        return 0;
    }

    return u32VppTunings_GetTuningCount(pstTuning);
}

uint32_t u32VppTunings_GetTuningValues(t_StTuning *pstTuning,
                                       t_UTuningValue *puTuning,
                                       uint32_t u32Len)
{
    uint32_t u32Idx;
    const t_StTuningDef *pstDef = NULL;

    VPP_RET_IF_NULL(pstTuning, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(puTuning, VPP_ERR_PARAM);

    if (!pstTuning->bValid)
    {
        LOGE("Tuning not valid, bvalid=%u", pstTuning->bValid);
        return VPP_ERR_PARAM;
    }

    pstDef = pstTuning->pstDef;
    if (!pstDef)
    {
        LOGE("no tuning definition for tuning=%p", pstTuning);
        return VPP_ERR_PARAM;
    }

    DUMP_DEF(pstDef);
    if (u32Len < pstDef->u32Count)
    {
        LOGE("Required len=%u, len sent=%u", pstDef->u32Count, u32Len);
        return VPP_ERR_PARAM;
    }

    for (u32Idx = 0; u32Idx < pstDef->u32Count; u32Idx++)
         puTuning[u32Idx] = pstTuning->puVal[u32Idx];

    return VPP_OK;
}

uint32_t u32VppTunings_GetTuningValuesByIndex(void *pstCb, uint32_t u32Idx,
                                              t_UTuningValue *puTuning,
                                              uint32_t u32Len)
{
    uint32_t u32Ret;
    t_StTuning *pstTuning = NULL;

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(puTuning, VPP_ERR_PARAM);

    pstTuning = pstVppTunings_GetTuningByIndex(pstCb, u32Idx);
    if (!pstTuning)
    {
        LOGE("unable to find valid tuning for index=%u", u32Idx);
        return VPP_ERR_PARAM;
    }

    u32Ret = u32VppTunings_GetTuningValues(pstTuning, puTuning, u32Len);
    LOGE_IF(u32Ret != VPP_OK, "Error getting values for index=%u, u32Ret=%u",
            u32Idx, u32Ret);

    return u32Ret;
}

uint32_t u32VppTunings_GetTuningValuesById(void *pstCb, uint32_t u32TuningId,
                                           t_UTuningValue *puTuning,
                                           uint32_t u32Len)
{
    uint32_t u32Ret;
    t_StTuning *pstTuning = NULL;

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(puTuning, VPP_ERR_PARAM);

    pstTuning = pstVppTunings_GetTuningById(pstCb, u32TuningId);
    if (!pstTuning)
    {
        LOGD("unable to find valid tuning for id=%u", u32TuningId);
        return VPP_ERR_PARAM;
    }

    u32Ret = u32VppTunings_GetTuningValues(pstTuning, puTuning, u32Len);
    LOGE_IF(u32Ret != VPP_OK, "Error getting values for Id=%u, u32Ret=%u",
            u32TuningId, u32Ret);

    return u32Ret;
}
