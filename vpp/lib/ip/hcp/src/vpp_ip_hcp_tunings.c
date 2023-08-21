/*!
 * @file vpp_ip_hcp_tunings.c
 *
 * @cr
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>

#define VPP_LOG_TAG     VPP_LOG_MODULE_HCP_TAG
#define VPP_LOG_MODULE  VPP_LOG_MODULE_HCP

#include "vpp_dbg.h"
#include "vpp_utils.h"

#include "hcp_hfi.h"
#include "hcp_rpc.h"

#include "vpp_tunings.h"
#include "vpp_ip_hcp_dbg.h"
#include "vpp_ip_hcp_priv.h"
#include "vpp_ip_hcp_tunings.h"
#include "vpp_ip_hcp_hfi.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/

#define GET_SYSTUNE_HEADER_SIZE()       (sizeof(uint32_t))
#define GET_SYSTUNEPARAM_HEADER_SIZE()  (sizeof(uint32_t) * 2)
#define GET_TUNING_ELEMENTS_SIZE()      (sizeof(uint32_t))

/************************************************************************
 * Forward Declarations
 ************************************************************************/
static uint32_t u32VppIpHcp_Validate_LTMTUNE_ANLTMZONEMAP(t_StTuning *pstTuning);

/************************************************************************
 * Local static variables
 ***********************************************************************/
static const t_StTuningDef astTuningDef[] = {
    // AIE, LTM, DE
    TUNING_DEF(DDTUNE_SAT_THR, U32, 1, 0, 63, NULL),
    TUNING_DEF(LTMTUNE_NLTMZONELEVEL, U32, 1, 0, 1, NULL),
    TUNING_DEF(LTMTUNE_ANLTMZONEMAP, U32, 20, 0, 0,
               u32VppIpHcp_Validate_LTMTUNE_ANLTMZONEMAP),
    TUNING_DEF(LTMTUNE_ANNYQDEGAINFACTOR, FLOAT, 9, 0, 2.00, NULL),
    TUNING_DEF(LTMTUNE_ANEARDEGAINFACTOR, FLOAT, 9, 0, 2.00, NULL),
    TUNING_DEF(LTMTUNE_FDEOFF, FLOAT, 1, -1.00, 7.00, NULL),
    TUNING_DEF(LTMTUNE_NDEMIN, U32, 1, 0, 63, NULL),
    TUNING_DEF(LTMTUNE_NDEMAX, U32, 1, 0, 63, NULL),
    TUNING_DEF(LTMTUNE_NDEPOW1, U32, 1, 0, 8, NULL),
    TUNING_DEF(LTMTUNE_NDEPOW2, U32, 1, 0, 8, NULL),
    TUNING_DEF(LTMTUNE_NDENYQTHRESH, U32, 1, 0, 1023, NULL),
    TUNING_DEF(LTMTUNE_NDENYQGAIN, U32, 1, 0, 255, NULL),
    TUNING_DEF(LTMTUNE_NDELPFBLEND, U32, 1, 0, 31, NULL),
    TUNING_DEF(LTMTUNE_FLUTCOEFFIIR, FLOAT, 1, 0.00, 1.00, NULL),
    TUNING_DEF(LTMTUNE_NLUTCNTBLEND, U32, 1, 0, 63, NULL),
    TUNING_DEF(LTMTUNE_FHSTTHRDIFF, FLOAT, 1, 0.00, 1.00, NULL),
    TUNING_DEF(LTMTUNE_FHSTCOEFFIIR, FLOAT, 1, 0.00, 1.00, NULL),
    TUNING_DEF(LTMTUNE_NBLACKMAX, U32, 1, 0, 255, NULL),
    TUNING_DEF(LTMTUNE_NWHITEMIN, U32, 1, 0, 255, NULL),
    TUNING_DEF(LTMTUNE_NQUANTILE, U32, 1, 0, 255, NULL),
    TUNING_DEF(LTMTUNE_NRNGLIM, U32, 1, 0, 255, NULL),
    TUNING_DEF(LTMTUNE_NHSTLIM, U32, 1, 0, 255, NULL),
    TUNING_DEF(LTMTUNE_NCLPGAP, U32, 1, 0, 31, NULL),
    TUNING_DEF(LTMTUNE_FDECOEFFIIR, FLOAT, 1, 0.00, 1.00, NULL),
    TUNING_DEF(LTMTUNE_NHSTUPDDEL, U32, 1, 0, 128, NULL),
    TUNING_DEF(LTMTUNE_FLUTCOEFFMINIIR, FLOAT, 1, 0.00, 1.00, NULL),
    TUNING_DEF(LTMTUNE_FLUTTHRESHIIR, FLOAT, 1, 0.00, 1.00, NULL),
    TUNING_DEF(LTMTUNE_NLUTCOEFFLPF, U32, 1, 0, 256, NULL),
    TUNING_DEF(LTMTUNE_NLUTUPDDEL, U32, 1, 0, 128, NULL),
    TUNING_DEF(LTMTUNE_FTMCOEFF, FLOAT, 1, 0.00, 10.00, NULL),
    TUNING_DEF(LTMTUNE_NTMORDER, U32, 1, 0, 1, NULL),
    TUNING_DEF(LTMTUNE_AFAUTOACECON, FLOAT, 4, 0.00, 1.00, NULL),
    TUNING_DEF(LTMTUNE_AFAUTOACEBRIL, FLOAT, 4, 0.00, 1.00, NULL),
    TUNING_DEF(LTMTUNE_AFAUTOACEBRIH, FLOAT, 4, 0.00, 1.00, NULL),
    TUNING_DEF(LTMTUNE_AFAUTOSATSAT, FLOAT, 4, 0.00, 2.00, NULL),
    TUNING_DEF(LTMTUNE_AFAUTOSATOFF, FLOAT, 4, 0.00, 2.00, NULL),
    TUNING_DEF(LTMTUNE_AFAUTODEGAIN, FLOAT, 4, -1.00, 7.00, NULL),
    TUNING_DEF(LTMTUNE_AFAUTODEOFF, FLOAT, 4, -1.00, 7.00, NULL),
    TUNING_DEF(APPTUNE_NLTMMAPEN, U32, 1, 0, 1, NULL),
    TUNING_DEF(APPTUNE_NLTMSATEN, U32, 1, 0, 1, NULL),

    // EAR
    TUNING_DEF(EARTUNE_REG_MAP_H263, U32, 3, 0, 7, NULL),
    TUNING_DEF(EARTUNE_REG_MAP_H264, U32, 3, 0, 7, NULL),
    TUNING_DEF(EARTUNE_REG_MAP_HEVC, U32, 3, 0, 7, NULL),
    TUNING_DEF(EARTUNE_REG_MAP_MPEG1, U32, 3, 0, 7, NULL),
    TUNING_DEF(EARTUNE_REG_MAP_MPEG2, U32, 3, 0, 7, NULL),
    TUNING_DEF(EARTUNE_REG_MAP_MPEG4, U32, 3, 0, 7, NULL),
    TUNING_DEF(EARTUNE_REG_MAP_DIVX_311, U32, 3, 0, 7, NULL),
    TUNING_DEF(EARTUNE_REG_MAP_DIVX, U32, 3, 0, 7, NULL),
    TUNING_DEF(EARTUNE_REG_MAP_VC1, U32, 3, 0, 7, NULL),
    TUNING_DEF(EARTUNE_REG_MAP_SPARK, U32, 3, 0, 7, NULL),
    TUNING_DEF(EARTUNE_REG_MAP_VP8, U32, 3, 0, 7, NULL),
    TUNING_DEF(EARTUNE_REG_MAP_VP9, U32, 3, 0, 7, NULL),
    TUNING_DEF(EARTUNE_NBARDETEN, U32, 1, 0, 1, NULL),

    // Split Screen
    TUNING_DEF(DDTUNE_PPSPLIT_LINE_Y, U32, 1, 0, 15, NULL),
    TUNING_DEF(DDTUNE_PPSPLIT_LINE_CB, U32, 1, 0, 15, NULL),
    TUNING_DEF(DDTUNE_PPSPLIT_LINE_CR, U32, 1, 0, 15, NULL),
    TUNING_DEF(APPTUNE_ANPPSPLITWIDTHMAP, U32, 3, 0, 3, NULL),

    // Dithering
    TUNING_DEF(APPTUNE_NDITMODE, U32, 1, 1, 3, NULL),

    // Custom
    TUNING_DEF(LTMTUNE_NCUST_SDR_LUT_EN, U32, 1, 0, 1, NULL),
    TUNING_DEF(LTMTUNE_NCUST_HDR_LUT_EN, U32, 1, 0, 1, NULL),
};

static const uint32_t su32TuningCnt = sizeof(astTuningDef) / sizeof(t_StTuningDef);

/************************************************************************
 * Local Functions
 ***********************************************************************/

#define IS_ODD(x)   ((x) & 0x1)
static uint32_t u32VppIpHcp_Validate_LTMTUNE_ANLTMZONEMAP(t_StTuning *pstTuning)
{
    uint32_t u32;
    uint32_t u32Min, u32Max;

    RET_ERR_IF_NULL_TUNING(pstTuning);
    RET_ERR_IF_WRONG_TUNING(pstTuning, LTMTUNE_ANLTMZONEMAP);

    for (u32 = 0; u32 < pstTuning->pstDef->u32Count; u32++)
    {
        u32Min = 0;

        if (IS_ODD(u32))
            u32Max = 5;
        else
            u32Max = 7;

        RET_ERR_IF_U32_OUT_OF_RANGE(pstTuning, u32, u32Min, u32Max);
    }

    return VPP_OK;
}

static uint32_t u32VppIpHcp_GetTuningsBufferSize(t_StVppIpHcpGlobalCb *pstGlobal)
{
    uint32_t u32TuningCnt, u32Size = 0;
    t_StHcpTuningCb *pstCb;
    t_StTuning *pstTuning;

    VPP_RET_IF_NULL(pstGlobal, VPP_ERR_PARAM);

    pstCb = (t_StHcpTuningCb *)pstGlobal->pvTunings;

    u32TuningCnt = u32VppTunings_GetValidTuningsCount(pstCb->pvTuningBlock);
    if (u32TuningCnt > 0)
    {
        uint32_t u32Idx;
        u32Size = GET_SYSTUNE_HEADER_SIZE();

        for (u32Idx = 0; u32Idx < u32TuningCnt; u32Idx++)
        {
            pstTuning = pstVppTunings_GetTuningByIndex(pstCb->pvTuningBlock,
                                                       u32Idx);
            if (!pstTuning)
            {
                LOGE("Index %u returned null tuning but should be valid!",
                     u32Idx);
                continue;
            }

            u32Size += GET_SYSTUNEPARAM_HEADER_SIZE();
            u32Size += u32VppTunings_GetTuningCount(pstTuning) *
                GET_TUNING_ELEMENTS_SIZE();
        }
    }

    return u32Size;
}

static uint32_t u32VppIpHcp_AllocIonBuffer(t_StVppIpHcpGlobalCb *pstGlobal,
                                           uint32_t u32Size)
{
    uint32_t u32Ret;
    t_StHcpTuningCb *pstCb;

    if (!pstGlobal || !u32Size)
        return VPP_ERR_PARAM;

    pstCb = (t_StHcpTuningCb *)pstGlobal->pvTunings;

    u32Ret = u32VppIon_Alloc(&pstGlobal->stVppCtx, u32Size, VPP_FALSE,
                             &pstCb->stSysBuf.stIon);
    if (u32Ret)
    {
        LOGE("failed to init ion memory. ret=%u", u32Ret);
        return VPP_ERR;
    }
    pstCb->stSysBuf.u32Sz = u32Size;
    pstCb->stSysBuf.eSysBufType = HCP_BUF_TYPE_TUNING;

    return VPP_OK;
}

static uint32_t u32VppIpHcp_FreeIonBuffer(t_StVppIpHcpGlobalCb *pstGlobal)
{
    uint32_t u32Ret;
    t_StHcpTuningCb *pstCb;

    VPP_RET_IF_NULL(pstGlobal, VPP_ERR_PARAM);

    pstCb = (t_StHcpTuningCb *)pstGlobal->pvTunings;

    u32Ret = u32VppIon_Free(&pstGlobal->stVppCtx, &pstCb->stSysBuf.stIon);
    if (u32Ret)
    {
        LOGE("failed to free ion memory. ret=%u", u32Ret);
        return VPP_ERR;
    }
    pstCb->stSysBuf.u32Sz = 0;

    return VPP_OK;
}

#ifdef DUMP_BUFFER_ENABLED
static void DUMP_BUFFER(t_StVppIpHcpGlobalCb *pstGlobal)
{
    uint32_t *pu32;
    uint32_t u32;
    t_StHcpTuningCb *pstCb;

    if (!pstGlobal || !pstGlobal->pvTunings)
        return;

    pstCb = (t_StHcpTuningCb *)pstGlobal->pvTunings;
    pu32 = (uint32_t *)pstCb->stSysBuf.stIon.buf;

    for (u32 = 0;
         u32 < u32VppIpHcp_GetTuningsBufferSize(pstGlobal) / sizeof(uint32_t);
         u32++, pu32++)
    {
        LOGD("HCP_TUNING:DUMP_BUF: %u", *pu32);
    }
}
#endif

static int32_t s32VppIpHcp_ConvertFloatToFixed(t_UTuningValue uValue)
{
    int32_t s32Value = uValue.FLOAT * 65536.0f;
    LOGD("float=%f (0x%x) converted to fixed=0x%x", uValue.FLOAT, uValue.U32,
         s32Value);
    return s32Value;
}

static uint32_t u32VppIpHcp_PackTuningElement(t_ETuningType eType,
                                              t_UTuningValue uValue,
                                              uint32_t *pu32Dest)
{
    int32_t s32Value;
    VPP_RET_IF_NULL(pu32Dest, VPP_ERR_PARAM);

    switch (eType)
    {
        // Intentional fall-through.
        case TUNING_TYPE_U32:
        case TUNING_TYPE_S32:
            memcpy(pu32Dest, &uValue.U32, sizeof(uint32_t));
            return VPP_OK;
        case TUNING_TYPE_FLOAT:
            s32Value = s32VppIpHcp_ConvertFloatToFixed(uValue);
            memcpy(pu32Dest, &s32Value, sizeof(int32_t));
            return VPP_OK;
        default:
            LOGE("given invalid tuning element type to pack. u32=%u", eType);
            return VPP_ERR_PARAM;
    }
}

static uint32_t u32VppIpHcp_PackTuningParam(t_StTuning *pstTuning,
                                            t_StSysTuneParam *pstParam)
{
    uint32_t u32;
    uint32_t u32Ret;
    uint32_t *pu32TuneParamElement;

    VPP_RET_IF_NULL(pstTuning, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstParam, VPP_ERR_PARAM);

    pstParam->hdr.u32Id = pstTuning->pstDef->u32Id;
    pstParam->hdr.u32Len = pstTuning->pstDef->u32Count;

    pu32TuneParamElement = &pstParam->u32Bdy[0];

    for (u32 = 0; u32 < pstTuning->pstDef->u32Count; u32++)
    {
        u32Ret = u32VppIpHcp_PackTuningElement(pstTuning->pstDef->eType,
                                               pstTuning->puVal[u32],
                                               &pu32TuneParamElement[u32]);
        if (u32Ret != VPP_OK)
        {
            LOGE("failed to pack tuning element. ret=%u, type=%u, value=0x%x",
                 u32Ret, pstTuning->pstDef->eType, pstTuning->puVal[u32].U32);
        }
    }

    return VPP_OK;
}

static uint32_t u32VppIpHcp_PackBuffer(t_StVppIpHcpGlobalCb *pstGlobal)
{
    uint32_t u32TuningCnt, u32Idx, u32Ret;
    char *pcParam;
    t_StHcpTuningCb *pstCb;
    t_StTuning *pstTuning;
    t_StSysTune *pstSysTune;
    t_StSysTuneParam *pstParam;

    VPP_RET_IF_NULL(pstGlobal, VPP_ERR_PARAM);

    pstCb = (t_StHcpTuningCb *)pstGlobal->pvTunings;

    memset(pstCb->stSysBuf.stIon.buf, 0, pstCb->stSysBuf.stIon.len);

    pstSysTune = (t_StSysTune *)pstCb->stSysBuf.stIon.buf;
    pstSysTune->u32NumTuneParams = 0;
    pcParam = (char *)&pstSysTune->u32Bdy[0];

    u32TuningCnt = u32VppTunings_GetValidTuningsCount(pstCb->pvTuningBlock);
    for (u32Idx = 0; u32Idx < u32TuningCnt; u32Idx++)
    {
        pstTuning = pstVppTunings_GetTuningByIndex(pstCb->pvTuningBlock, u32Idx);
        if (!pstTuning)
        {
            LOGE("Index %u returned null tuning but should be valid!", u32Idx);
            continue;
        }

        pstParam = (t_StSysTuneParam *)pcParam;

        u32Ret = u32VppIpHcp_PackTuningParam(pstTuning, pstParam);
        if (u32Ret != VPP_OK)
        {
            LOGE("failed to pack a tuning param. ret=%u", u32Ret);
            return VPP_ERR;
        }

        pstSysTune->u32NumTuneParams++;
        pcParam += sizeof(pstParam->hdr) +
                   pstParam->hdr.u32Len * sizeof(uint32_t);
    }
#ifdef DUMP_BUFFER_ENABLED
    DUMP_BUFFER(pstGlobal);
#endif

    return VPP_OK;
}

static uint32_t u32VppIpHcp_ValidateMessagePacket(t_StVppIpHcpGlobalCb *pstGlobal,
                                                  t_StHcpHfiMsgPkt *pstMsg)
{
    uint32_t u32Val;

    VPP_RET_IF_NULL(pstGlobal, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstGlobal->pvTunings, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstMsg, VPP_ERR_PARAM);

    u32Val = pstMsg->stHdr.eMsgId;
    if (u32Val != MSG_ID_BUFF_RELEASED)
    {
        LOGE("given invalid message id. id=%u", u32Val);
        return VPP_ERR_PARAM;
    }
    u32Val = pstMsg->stHdr.u32Size;
    if (u32Val == 0)
    {
        LOGE("given invalid message packet size. size=%u", u32Val);
        return VPP_ERR_PARAM;
    }
    u32Val = pstMsg->stMsgBuf.bufAttr.eBufType;
    if (u32Val != BUFF_TYPE_GENERIC)
    {
        LOGE("given invalid buffer type. type=%u", u32Val);
        return VPP_ERR_PARAM;
    }
    u32Val = pstMsg->stMsgBuf.bufAttr.u32Cookie_h;
    if (u32Val != HCP_SYS_TUNE_COOKIE_H)
    {
        LOGE("given invalid cookie_h. h=%u", u32Val);
        return VPP_ERR_PARAM;
    }
    u32Val = pstMsg->stMsgBuf.bufAttr.u32Cookie_l;
    if (u32Val != HCP_SYS_TUNE_COOKIE_L)
    {
        LOGE("given invalid cookie_l. l=%u", u32Val);
        return VPP_ERR_PARAM;
    }
    u32Val = pstMsg->stMsgBuf.bufAttr.stGenBufAttr.u32Size;
    if (u32Val != u32VppIpHcp_GetTuningsBufferSize(pstGlobal))
    {
        LOGE("given invalid buffer size. size=%u, expected=%u", u32Val,
             u32VppIpHcp_GetTuningsBufferSize(pstGlobal));
        return VPP_ERR_PARAM;
    }
    u32Val = pstMsg->stMsgBuf.bufAttr.stGenBufAttr.eUsage;
    if (u32Val != BUFF_USAGE_TUNE)
    {
        LOGE("given invalid buffer usage. usage=%u", u32Val);
        return VPP_ERR_PARAM;
    }

    return VPP_OK;
}

/************************************************************************
 * Global Functions
 ***********************************************************************/

uint32_t u32VppIpHcp_TuningInit(t_StVppIpHcpGlobalCb *pstGlobal)
{
    int rc;
    uint32_t u32;
    t_StHcpTuningCb *pstCb;

    VPP_RET_IF_NULL(pstGlobal, VPP_ERR_PARAM);
    if (pstGlobal->pvTunings)
    {
        LOGE("tunings already initialized in global cb");
        return VPP_ERR_STATE;
    }

    pstCb = calloc(1, sizeof(t_StHcpTuningCb));
    if (!pstCb)
    {
        LOGE("unable to allocate tuning cb");
        u32 = VPP_ERR_NO_MEM;
        goto err_calloc_cb;
    }

    rc = pthread_mutex_init(&pstCb->mutex, NULL);
    if (rc)
    {
        LOGE("unable to init mutex. rc=%d (%s)", rc, strerror(rc));
        u32 = VPP_ERR_RESOURCES;
        goto err_mutex_init;
    }

    pstCb->pvTuningBlock = vpVppTunings_Init(HCP_TUNINGS_FILE_NAME, astTuningDef,
                                             su32TuningCnt);
    if (!pstCb->pvTuningBlock)
    {
        LOGD("unable to init tunings, will use default.");
        u32 = VPP_ERR_RESOURCES;
        goto err_read_tunings;
    }

    pstGlobal->pvTunings = pstCb;
    pstCb->eState = HCP_TUNING_STATE_INITED;

    return VPP_OK;

err_read_tunings:
    rc = pthread_mutex_destroy(&pstCb->mutex);
    LOGE_IF(rc, "failed to destroy mutex. rc=%d (%s)", rc, strerror(rc));
err_mutex_init:
    free(pstCb);
err_calloc_cb:
    return u32;
}

uint32_t u32VppIpHcp_TuningTerm(t_StVppIpHcpGlobalCb *pstGlobal)
{
    uint32_t u32;
    t_StHcpTuningCb *pstCb;

    VPP_RET_IF_NULL(pstGlobal, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstGlobal->pvTunings, VPP_OK);

    pstCb = pstGlobal->pvTunings;

    if (pstCb->eState != HCP_TUNING_STATE_INITED &&
        pstCb->eState != HCP_TUNING_STATE_BUFFER_SENT)
    {
        LOGE("unable to term tuning due to invalid state. state=%u",
             pstCb->eState);
        return VPP_ERR_STATE;
    }

    if (pstCb->eState == HCP_TUNING_STATE_BUFFER_SENT)
    {
        LOGE("called tuning term() without receiving tuning buffer back.");
        u32 = u32VppIpHcp_FreeIonBuffer(pstGlobal);
        LOGE_IF(u32 != VPP_OK, "failed to free tuning buffer. ret=%u", u32);
    }

    vVppTunings_Term(pstCb->pvTuningBlock);

    u32 = pthread_mutex_destroy(&pstCb->mutex);
    LOGE_IF(u32, "failed to destroy mutex. u32=%u", u32);

    free(pstCb);
    pstGlobal->pvTunings = NULL;

    return VPP_OK;
}

uint32_t u32VppIpHcp_TuningLoad(t_StVppIpHcpGlobalCb *pstGlobal)
{
    uint32_t u32Ret;
    uint32_t u32FuncRet;
    uint32_t u32Size;
    t_StHcpTuningCb *pstCb;

    VPP_RET_IF_NULL(pstGlobal, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstGlobal->pvTunings, VPP_OK);

    pstCb = pstGlobal->pvTunings;

    pthread_mutex_lock(&pstCb->mutex);

    if (pstCb->eState != HCP_TUNING_STATE_INITED)
    {
        LOGE("unable to load tunings due to invalid state. state=%u",
             pstCb->eState);
        u32FuncRet = VPP_ERR_STATE;
        pthread_mutex_unlock(&pstCb->mutex);
        goto err_wrong_state;
    }

    pstCb->eState = HCP_TUNING_STATE_BUFFER_SEND_PENDING;

    pthread_mutex_unlock(&pstCb->mutex);

    u32Size = u32VppIpHcp_GetTuningsBufferSize(pstGlobal);
    LOGD("received size for tunings buffer. size=%u", u32Size);

    if (u32Size > 0)
    {
        u32Ret = u32VppIpHcp_AllocIonBuffer(pstGlobal, u32Size);
        if (u32Ret)
        {
            LOGE("failed to allocate tunings buffer. ret=%u", u32Ret);
            u32FuncRet = VPP_ERR;
            goto err_alloc_ion_buffer;
        }

        u32Ret = u32VppIpHcp_PackBuffer(pstGlobal);
        if (u32Ret != VPP_OK)
        {
            LOGE("failed to pack tuning buffer. ret=%u", u32Ret);
            u32FuncRet = VPP_ERR;
            goto err_pack_buffer;
        }

        pthread_mutex_lock(&pstCb->mutex);

        u32Ret = u32VppIpHcp_HwGlobalSendSysBuf(pstGlobal, &pstCb->stSysBuf);
        if (u32Ret != VPP_OK)
        {
            LOGE("failed to send tuning buffer to firmware. ret=%u", u32Ret);
            u32FuncRet = VPP_ERR;
            pthread_mutex_unlock(&pstCb->mutex);
            goto err_send_buffer;
        }

        pstCb->eState = HCP_TUNING_STATE_BUFFER_SENT;
    }
    else
    {
        pthread_mutex_lock(&pstCb->mutex);
        pstCb->eState = HCP_TUNING_STATE_INITED;
    }

    pthread_mutex_unlock(&pstCb->mutex);

    return VPP_OK;

err_send_buffer:
err_pack_buffer:
    u32Ret = u32VppIpHcp_FreeIonBuffer(pstGlobal);
    LOGE_IF(u32Ret != VPP_OK, "failed to free tuning buffer. ret=%u", u32Ret);
err_alloc_ion_buffer:
    pthread_mutex_lock(&pstCb->mutex);
    pstCb->eState = HCP_TUNING_STATE_INITED;
    pthread_mutex_unlock(&pstCb->mutex);
err_wrong_state:
    return u32FuncRet;
}

uint32_t u32VppIpHcp_TuningProcBuffReleasedMsg(t_StVppIpHcpGlobalCb *pstGlobal,
                                               t_StHcpHfiMsgPkt *pstMsg)
{
    uint32_t u32Ret;
    uint32_t u32FuncRet;
    t_StHcpTuningCb *pstCb;

    VPP_RET_IF_NULL(pstGlobal, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstMsg, VPP_ERR_PARAM);

    if (pstGlobal->pvTunings == NULL)
    {
        LOGE("ProcBuff() called, but not expecting a buffer.");
        return VPP_ERR;
    }

    pstCb = pstGlobal->pvTunings;

    pthread_mutex_lock(&pstCb->mutex);

    if (pstCb->eState != HCP_TUNING_STATE_BUFFER_SENT)
    {
        LOGE("unable to process buffer due to invalid state. state=%u",
             pstCb->eState);
        u32FuncRet = VPP_ERR_STATE;
        pthread_mutex_unlock(&pstCb->mutex);
        goto err_wrong_state;
    }
    pstCb->eState = HCP_TUNING_STATE_BUFFER_RECV_PENDING;

    pthread_mutex_unlock(&pstCb->mutex);

    u32Ret = u32VppIpHcp_ValidateMessagePacket(pstGlobal, pstMsg);
    if (u32Ret != VPP_OK)
    {
        LOGE("unable to process buffer due to invalid message packet. ret=%u",
             u32Ret);
        u32FuncRet = VPP_ERR_PARAM;
        goto err_validate_message_packet;
    }

    u32Ret = u32VppIpHcp_FreeIonBuffer(pstGlobal);
    if (u32Ret)
        LOGE("failed to free tuning buffer. ret=%u", u32Ret);

    LOGI("received tuning buffer back. err=%d, op=%u", pstMsg->stHdr.s32Err,
         pstMsg->stMsgBuf.bufAttr.u32Operation);

    pthread_mutex_lock(&pstCb->mutex);
    pstCb->eState = HCP_TUNING_STATE_INITED;
    pthread_mutex_unlock(&pstCb->mutex);

    return VPP_OK;

err_validate_message_packet:
    pthread_mutex_lock(&pstCb->mutex);
    pstCb->eState = HCP_TUNING_STATE_BUFFER_SENT;
    pthread_mutex_unlock(&pstCb->mutex);
err_wrong_state:
    return u32FuncRet;
}

uint32_t bVppIpHcp_IsTuningLoadComplete(t_StVppIpHcpGlobalCb *pstGlobal)
{
    uint32_t bIsComplete = VPP_FALSE;
    t_StHcpTuningCb *pstCb;

    VPP_RET_IF_NULL(pstGlobal, VPP_FALSE);

    VPP_RET_IF_NULL(pstGlobal->pvTunings, VPP_TRUE);

    pstCb = pstGlobal->pvTunings;

    pthread_mutex_lock(&pstCb->mutex);
    if (pstCb->eState == HCP_TUNING_STATE_INITED)
    {
        bIsComplete = VPP_TRUE;
    }
    pthread_mutex_unlock(&pstCb->mutex);

    return bIsComplete;
}

uint32_t u32VppIpHcp_GetTuningCount(t_StVppIpHcpGlobalCb *pstGlobal,
                                    uint32_t u32TuningId)
{
    t_StHcpTuningCb *pstCb = NULL;

    if (!pstGlobal || !pstGlobal->pvTunings)
        return 0;

    pstCb = pstGlobal->pvTunings;
    if (pstCb->eState != HCP_TUNING_STATE_INITED)
        return 0;

    return u32VppTunings_GetTuningCountById(pstCb->pvTuningBlock, u32TuningId);
}

uint32_t u32VppIpHcp_GetTuning(t_StVppIpHcpGlobalCb *pstGlobal, uint32_t u32TuningId,
                               t_UTuningValue *puTuning, uint32_t u32Len)
{
    t_StHcpTuningCb *pstCb = NULL;
    uint32_t u32Ret;

    VPP_RET_IF_NULL(pstGlobal, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstGlobal->pvTunings, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(puTuning, VPP_ERR_PARAM);

    pstCb = pstGlobal->pvTunings;
    if ((pstCb->eState != HCP_TUNING_STATE_INITED) || !u32Len)
        return VPP_ERR_PARAM;

    u32Ret = u32VppTunings_GetTuningValuesById(pstCb->pvTuningBlock,
                                               u32TuningId, puTuning,
                                               u32Len);
    LOGE_IF(u32Ret != VPP_OK, "Error getting tuning values, u32Ret=%u", u32Ret);

    return u32Ret;
}
