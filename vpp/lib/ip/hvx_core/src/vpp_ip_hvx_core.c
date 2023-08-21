/*!
 * @file vpp_ip_hvx_core.c
 *
 * @cr
 * Copyright (c) 2015-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <sys/types.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "AEEStdErr.h"

#define VPP_LOG_TAG     VPP_LOG_MODULE_HVX_CORE_TAG
#define VPP_LOG_MODULE  VPP_LOG_MODULE_HVX_CORE
#include "vpp_dbg.h"

#include "vpp.h"
#include "vpp_ion.h"
#include "vpp_reg.h"
#include "vpp_ip.h"
#include "vpp_utils.h"
#include "vpp_stats.h"
#include "vpp_ip_hvx_core.h"


/************************************************************************
 * Local definitions
 ***********************************************************************/
#define HVX_MIGRATE_CNT_MAX                 1000
#define HVX_VERSION_LEN                     128
#define HVX_LIB_NAME_LEN                    64
#define HVX_PROPERTY_LOGFLAGS               "vendor.media.vpp.hvx.logflags"
#define HVX_PROPERTY_STATSFLAGS             "vendor.media.vpp.hvx.statsflags"
#define HVX_PROPERTY_STATSPERIOD            "vendor.media.vpp.hvx.statsperiod"
#define HVX_HANDLE_INVALID                  ((remote_handle64)(-1))
#define HVX_TUNING_HEADER_SIZE              (sizeof(uint32_t))
#define HVX_TUNING_PARAM_HEADER_SIZE        (sizeof(vpp_svc_hdr_t))
#define HVX_TUNING_ELEMENTS_SIZE            (sizeof(uint32_t))
#define HVX_TUNING_TUNING_COUNT_MAX         256

enum {
    HVX_CORE_STAT_REGBUF,
    HVX_CORE_STAT_UNREGBUF,

    HVX_CORE_STAT_SVC_OPEN,
    HVX_CORE_STAT_SVC_CLOSE,
    HVX_CORE_STAT_SVC_INIT,
    HVX_CORE_STAT_SVC_DEINIT,
    HVX_CORE_STAT_SVC_SEND_CTX,
    HVX_CORE_STAT_SVC_RETRIEVE_CTX,
    HVX_CORE_STAT_SVC_PREPARE_CTX,
    HVX_CORE_STAT_SVC_GET_CTX,
    HVX_CORE_STAT_SVC_GET_DIAG_CTX,
    HVX_CORE_STAT_SVC_SEND_SCRATCH,
    HVX_CORE_STAT_SVC_RETRIEVE_SCRATCH,
    HVX_CORE_STAT_SVC_SEND_TUNINGS,
    HVX_CORE_STAT_SVC_GET_TUNING_PARAMS,

    HVX_CORE_STAT_MIGRATE,
    HVX_CORE_STAT_ALLOC_CTX,
    HVX_CORE_STAT_FREE_CTX,
    HVX_CORE_STAT_CORE_OPEN,
    HVX_CORE_STAT_ALLOC_SCRATCH,

    HVX_CORE_STAT_SET_DBG_LVL,
    HVX_CORE_STAT_GET_VER_INFO,
};

/************************************************************************
 * Local static variables
 ***********************************************************************/
static t_StVppIpHvxCoreGlobalCb stGlobalCb = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
};

static const t_StVppStatsConfig astHvxCoreStatsCfg[] = {
    VPP_PROF_DECL(HVX_CORE_STAT_REGBUF, 100, 0),
    VPP_PROF_DECL(HVX_CORE_STAT_UNREGBUF, 100, 0),

    VPP_PROF_DECL(HVX_CORE_STAT_SVC_OPEN, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_SVC_CLOSE, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_SVC_INIT, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_SVC_DEINIT, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_SVC_SEND_CTX, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_SVC_RETRIEVE_CTX, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_SVC_PREPARE_CTX, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_SVC_GET_CTX, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_SVC_GET_DIAG_CTX, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_SVC_SEND_SCRATCH, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_SVC_RETRIEVE_SCRATCH, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_SVC_SEND_TUNINGS, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_SVC_GET_TUNING_PARAMS, 1, 1),

    VPP_PROF_DECL(HVX_CORE_STAT_MIGRATE, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_ALLOC_CTX, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_FREE_CTX, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_CORE_OPEN, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_ALLOC_SCRATCH, 1, 1),

    VPP_PROF_DECL(HVX_CORE_STAT_SET_DBG_LVL, 1, 1),
    VPP_PROF_DECL(HVX_CORE_STAT_GET_VER_INFO, 1, 1),
};

static const uint32_t u32HvxCoreStatCnt = VPP_STATS_CNT(astHvxCoreStatsCfg);

/************************************************************************
 * Forward Declarations
 ************************************************************************/
extern void *remote_register_fd(int fd, int size);
#pragma weak remote_register_fd

extern void remote_register_buf(void* buf, int size, int fd);
#pragma weak remote_register_buf

/************************************************************************
 * Local Functions
 ***********************************************************************/

static void vVppIpHvxCore_GlobalMutexWaitStateSet(t_EVppIpHvxCoreGlobalState eState)
{
    t_StVppIpHvxCoreGlobalCb *pstGlobal = &stGlobalCb;

    if ((eState != HVX_CORE_GLOBAL_STATE_BOOT) && (eState != HVX_CORE_GLOBAL_STATE_SHUTDOWN))
    {
        LOGE("Trying to set to invalid state=%d", eState);
        return;
    }

    pthread_mutex_lock(&pstGlobal->mutex);
    while (pstGlobal->eState == HVX_CORE_GLOBAL_STATE_BOOT ||
           pstGlobal->eState == HVX_CORE_GLOBAL_STATE_SHUTDOWN)
    {
        LOGD("Waiting for another client to finish, eState=%u", pstGlobal->eState);
        pthread_cond_wait(&pstGlobal->cond, &pstGlobal->mutex);
    }
    pstGlobal->eState = eState;
    pthread_mutex_unlock(&pstGlobal->mutex);
}

static void vVppIpHvxCore_SetDebugLevels(t_StVppIpHvxCoreCb *pstCb)
{
    int32_t s32Ret;
    vpp_svc_statlogs_cfg_t stDebugCfg;

    stDebugCfg.log_flag = pstCb->debug_cfg.u32LogFlags;
    stDebugCfg.stats_flag = pstCb->debug_cfg.u32StatsFlags;
    stDebugCfg.stats_period = pstCb->debug_cfg.u32StatsPeriod;

    s32Ret = vpp_svc_set_config(pstCb->hvx_handle, CFG_TYPE_STATSLOGS, (unsigned char*)&stDebugCfg,
                                sizeof(vpp_svc_statlogs_cfg_t));
    if (s32Ret != 0)
    {
        LOGE("unable to set stats config on firmware, s32Ret=%d", s32Ret);
    }
}

static void vVppIpHvxCore_ReadProperties(t_StVppIpHvxCoreCb *pstCb)
{
    VPP_RET_VOID_IF_NULL(pstCb);

    u32VppUtils_ReadPropertyU32(HVX_PROPERTY_LOGFLAGS,
                                &pstCb->debug_cfg.u32LogFlags, "0");
    u32VppUtils_ReadPropertyU32(HVX_PROPERTY_STATSFLAGS,
                                &pstCb->debug_cfg.u32StatsFlags, "0");
    u32VppUtils_ReadPropertyU32(HVX_PROPERTY_STATSPERIOD,
                                &pstCb->debug_cfg.u32StatsPeriod, "0");

    LOGI("HVX debug levels: log_flag=0x%x, stats_flag=0x%x, stats_period=0x%x",
         pstCb->debug_cfg.u32LogFlags, pstCb->debug_cfg.u32StatsFlags,
         pstCb->debug_cfg.u32StatsPeriod);
}

static void vVppIpHvxCore_GetVersionInfo(t_StVppIpHvxCoreCb *pstCb)
{
    int32_t s32Ret, i;
    vpp_svc_cap_resource_list_t* pstCapRes;

    VPP_RET_VOID_IF_NULL(pstCb);

    if (pstCb->once.u32TsRead)
        return;
    pstCb->once.u32TsRead = VPP_TRUE;

    pstCapRes = pstCb->pstCapabilityResources;

    for (i = 0; i < pstCapRes->resourceLen; i++)
    {
        s32Ret = vpp_svc_get_buildts_id(pstCb->hvx_handle, pstCapRes->resource[i].vpp_func_id,
                                        (unsigned char *)pstCapRes->resource[i].build_ts,
                                        MAX_TS_LEN);
        if (s32Ret == AEE_SUCCESS)
        {
            LOGI("HVX func_id[%d] version: %s",
                 pstCapRes->resource[i].vpp_func_id,
                 pstCapRes->resource[i].build_ts);
        }
        else if (s32Ret == AEE_EREADONLY)
            LOGI("HVX in protected mode. Unable to get HVX version for func_id[%d]", i);
        else
            LOGE("Unable to get HVX version for func_id[%d], s32Ret=%d", i, s32Ret);
    }
}

static uint32_t u32VppIpHvxCore_HandleSessionMigration(t_StVppIpHvxCoreCb *pstCb)
{
    int rc;
    uint32_t u32Heap, i;
    uint32_t u32Ret = VPP_ERR_HW;

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);

    VPP_IP_PROF_START(&pstCb->stBase, HVX_CORE_STAT_MIGRATE);

    if (!pstCb->stBase.bSecureSession)
        return VPP_OK;

    u32Heap = u32VppIon_GetHeapId(pstCb->stBase.pstCtx,
                                  pstCb->stBase.bSecureSession);

    for (i = 0; i < HVX_MIGRATE_CNT_MAX; i++)
    {
        rc = vpp_svc_migrate_pd(pstCb->hvx_handle, u32Heap);
        if (rc == AEE_SUCCESS)
        {
            u32Ret = VPP_OK;
            LOGI("vpp_svc_migrate_pd(), i=%u, rc=%d", i, rc);
            break;
        }
        else if (rc != AEE_EINTERRUPTED)
        {
            LOGE("ERROR: vpp_svc_migrate_pd(), rc=%d", rc);
            break;
        }
    }

    LOGE_IF(i == HVX_MIGRATE_CNT_MAX, "migrate count reached max, max=%u", i);

    VPP_IP_PROF_STOP(&pstCb->stBase, HVX_CORE_STAT_MIGRATE);
    return u32Ret;
}

static void vVppIpHvxCore_RegisterIon(t_StVppIonBuf *pstIon, void **ppv)
{
    if (!ppv)
    {
        LOGE("ppv null");
        return;
    }

    *ppv = MAP_FAILED;
    if (!pstIon)
    {
        LOGE("pstIon is null");
        return;
    }

    if (remote_register_fd)
    {
        *ppv = remote_register_fd(pstIon->fd_ion_mem, pstIon->len);
        if (*ppv == (void *)-1 || *ppv == NULL)
        {
            LOGE("unable to remote_register_fd, fd=%d, dummy_ptr=%p",
                 pstIon->fd_ion_mem, *ppv);
            *ppv = MAP_FAILED;
        }
        else
        {
            LOGI("registering buffer, fd=%d, sz=%u, new_ptr=%p",
                 pstIon->fd_ion_mem, pstIon->len, *ppv);
        }
    }
    else
        LOGE("unable to register buffer, fd=%d ptr=%p, sz=%u",
             pstIon->fd_ion_mem, pstIon->buf, pstIon->len);
}

static void vVppIpHvxCore_UnregisterIon(t_StVppIonBuf *pstIon, void *pv)
{
    if (!pstIon || !pv)
    {
        LOGE("NULL input: pstIon=%p, pv=%p", pstIon, pv);
        return;
    }

    if (remote_register_buf)
    {
        LOGI("unregistering buffer, secure=%u, fd=%d, ptr=%p, sz=%u",
             pstIon->bSecure, pstIon->fd_ion_mem, pv, pstIon->len);
        remote_register_buf(pv, pstIon->len, -1);
    }
    else
    {
        LOGI("unable to unregister buffer, fd=%d, ptr=%p, sz=%u",
             pstIon->fd_ion_mem, pstIon->buf, pstIon->len);
    }
}

static uint32_t u32VppIpHvxCore_SendScratchBufs(t_StVppIpHvxCoreCb *pstHvxCore,
                                                uint32_t u32Flags)
{
    t_StHvxCoreScratchBuf *pstScratchBufs;
    _dmahandle1_t astScratchBufInfo[MAX_SCRATCH_BUFS];
    uint32_t u32ScratchBufCnt, i;
    int rc;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    u32ScratchBufCnt = pstHvxCore->u32ScratchBufCnt;
    if (u32ScratchBufCnt > MAX_SCRATCH_BUFS)
    {
        LOGE("Buffer count %u exceeds max %u", u32ScratchBufCnt, MAX_SCRATCH_BUFS);
        return VPP_ERR;
    }

    pstScratchBufs = pstHvxCore->pstScratchBufs;
    if (!pstScratchBufs)
    {
        if (u32ScratchBufCnt)
        {
            LOGE("u32ScratchBufCnt=%u but array is NULL!", u32ScratchBufCnt);
            return VPP_ERR;
        }
        return VPP_OK;
    }

    for (i = 0; i < u32ScratchBufCnt; i++)
    {
        astScratchBufInfo[i].fd = pstScratchBufs[i].stIonBuf.fd_ion_mem;
        astScratchBufInfo[i].len = pstScratchBufs[i].u32Size;
        astScratchBufInfo[i].offset = pstScratchBufs[i].u32Offset;
    }

    if (u32ScratchBufCnt)
    {
        VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_SEND_SCRATCH);
        rc = vpp_svc_set_scratch_bufs(pstHvxCore->hvx_handle, u32Flags,
                                      astScratchBufInfo, u32ScratchBufCnt);
        VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_SEND_SCRATCH);

        if (rc != AEE_SUCCESS)
        {
            LOGE("Error: Send scratch bufs failed, rc=%d", rc);
            return VPP_ERR_HW;
        }
    }

    return VPP_OK;
}

static uint32_t u32VppIpHvxCore_RetrieveScratchBufs(t_StVppIpHvxCoreCb *pstHvxCore)
{
    int rc;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_RETRIEVE_SCRATCH);
    rc = vpp_svc_retrieve_scratch_bufs(pstHvxCore->hvx_handle);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_RETRIEVE_SCRATCH);

    if (rc != AEE_SUCCESS)
    {
        LOGE("Error: Retrieve scratch bufs failed, rc=%d", rc);
        return VPP_ERR_HW;
    }

    return VPP_OK;
}

static uint32_t u32VppIpHvxCore_AllocScratchBufs(t_StVppIpHvxCoreCb *pstHvxCore,
                                                 const vpp_svc_scratch_buf_req_t *pstScratchReq)
{
    uint32_t u32Ret = VPP_OK;
    t_StHvxCoreScratchBuf *pstScratchBufs;
    uint32_t u32BufCnt, i;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstScratchReq, VPP_ERR_PARAM);

    if (pstHvxCore->u32ScratchBufCnt || pstHvxCore->pstScratchBufs)
    {
        LOGE("Scratch buffers not empty u32ScratchBufCnt=%u, pstScratchBufs=%p!",
             pstHvxCore->u32ScratchBufCnt, pstHvxCore->pstScratchBufs);
        return VPP_ERR;
    }

    u32BufCnt = pstScratchReq->numbufs;
    if (!u32BufCnt)
        return VPP_OK;

    if (u32BufCnt > MAX_SCRATCH_BUFS)
    {
        LOGE("Requested %u scratch buffers greater than MAX %u",
             u32BufCnt, MAX_SCRATCH_BUFS);
        return VPP_ERR_PARAM;
    }

    LOGI("Allocating %u scratch buffers", u32BufCnt);
    pstHvxCore->pstScratchBufs = calloc(u32BufCnt, sizeof(t_StHvxCoreScratchBuf));
    if (!pstHvxCore->pstScratchBufs)
    {
        LOGE("Error allocating scratch buffer array");
        return VPP_ERR_NO_MEM;
    }
    pstScratchBufs = pstHvxCore->pstScratchBufs;
    for (i = 0; i < u32BufCnt; i++)
    {
        if (pstScratchReq->buflen[i])
        {
            pstScratchBufs[i].u32Size = pstScratchReq->buflen[i];
            pstScratchBufs[i].u32Offset = 0;
            pstScratchBufs[i].bSecure = pstHvxCore->stBase.bSecureSession;
            u32Ret = u32VppIon_Alloc(pstHvxCore->stBase.pstCtx, pstScratchBufs[i].u32Size,
                                     pstScratchBufs[i].bSecure, &pstScratchBufs[i].stIonBuf);
            if (u32Ret != VPP_OK)
            {
                LOGE("Error allocating ion mem for scratch buffer %u, u32=%u",
                     i, u32Ret);
                u32Ret = VPP_ERR_NO_MEM;
                goto ERR_ALLOC_ION;
            }
            pstHvxCore->u32ScratchBufCnt++;
            vVppIpHvxCore_RegisterIon(&pstScratchBufs[i].stIonBuf,
                                      (void *)&pstScratchBufs[i].pvRemote);
            LOGD("allocated ion mem for scratch buffer %u, fd=%d, sz=%u",
                 i, pstScratchBufs[i].stIonBuf.fd_ion_mem, pstScratchBufs[i].u32Size);
        }
    }

    return u32Ret;

ERR_ALLOC_ION:
    for (i = 0; i < pstHvxCore->u32ScratchBufCnt; i++)
    {
        if (pstScratchBufs[i].u32Size)
        {
            vVppIpHvxCore_UnregisterIon(&pstScratchBufs[i].stIonBuf, pstScratchBufs[i].pvRemote);
            u32VppIon_Free(pstHvxCore->stBase.pstCtx, &pstScratchBufs[i].stIonBuf);
        }
    }
    free(pstHvxCore->pstScratchBufs);
    pstHvxCore->pstScratchBufs = NULL;
    pstHvxCore->u32ScratchBufCnt = 0;

    return u32Ret;
}

static void vVppIpHvxCore_FreeScratchBufs(t_StVppIpHvxCoreCb *pstHvxCore)
{
    VPP_RET_VOID_IF_NULL(pstHvxCore);
    t_StHvxCoreScratchBuf *pstScratchBufs;
    uint32_t i;

    VPP_RET_VOID_IF_NULL(pstHvxCore);

    pstScratchBufs = pstHvxCore->pstScratchBufs;
    if (!pstScratchBufs)
    {
        pstHvxCore->u32ScratchBufCnt = 0;
        return;
    }
    for (i = 0; i < pstHvxCore->u32ScratchBufCnt; i++)
    {
        if (pstScratchBufs[i].u32Size)
        {
            vVppIpHvxCore_UnregisterIon(&pstScratchBufs[i].stIonBuf, pstScratchBufs[i].pvRemote);
            u32VppIon_Free(pstHvxCore->stBase.pstCtx, &pstScratchBufs[i].stIonBuf);
        }
    }
    free(pstScratchBufs);
    pstHvxCore->pstScratchBufs = NULL;
    pstHvxCore->u32ScratchBufCnt = 0;
}

static uint32_t u32VppIpHvxCore_AllocateContext(t_StVppIpHvxCoreCb *pstHvxCore,
                                                uint32_t u32ReqLen)
{
    uint32_t u32Ret = VPP_OK;
    t_StVppIonBuf *pstIonBuf;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstIonBuf = &pstHvxCore->ctx.stIonCtxBuf;

    LOGD("allocating context buffer, sz=%u", u32ReqLen);

    uint32_t u32;
    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_ALLOC_CTX);
    u32 = u32VppIon_Alloc(pstHvxCore->stBase.pstCtx, u32ReqLen,
                          pstHvxCore->stBase.bSecureSession, pstIonBuf);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_ALLOC_CTX);
    if (u32 != VPP_OK)
    {
        LOGE("Error: context ion alloc failed, secure=%u, fd=%d, addr=%p, sz=%u",
             pstHvxCore->stBase.bSecureSession, pstIonBuf->fd_ion_mem,
             pstIonBuf->buf, u32ReqLen);

        return VPP_ERR_NO_MEM;
    }
    else
    {
        pstHvxCore->ctx.bAllocated = VPP_TRUE;
        pstHvxCore->ctx.u32AllocatedSize = u32ReqLen;
        pstHvxCore->ctx.u32Offset = 0;
    }

    LOGD("allocated ion mem fd=%d, addr=%p, sz=%u", pstIonBuf->fd_ion_mem,
         pstIonBuf->buf, u32ReqLen);

    vVppIpHvxCore_RegisterIon(pstIonBuf, (void *)&pstHvxCore->ctx.pvRemote);

    return u32Ret;

}

static uint32_t u32VppIpHvxCore_FreeContext(t_StVppIpHvxCoreCb *pstHvxCore)
{
    t_StVppIonBuf *pstIonBuf;

    if (!pstHvxCore)
        return VPP_ERR_PARAM;

    if (!pstHvxCore->ctx.bAllocated)
        return VPP_OK;

    pstIonBuf = &pstHvxCore->ctx.stIonCtxBuf;

    LOGD("freeing context buffer");

    vVppIpHvxCore_UnregisterIon(pstIonBuf, pstHvxCore->ctx.pvRemote);

    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_FREE_CTX);
    u32VppIon_Free(pstHvxCore->stBase.pstCtx, pstIonBuf);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_FREE_CTX);

    pstHvxCore->ctx.bAllocated = VPP_FALSE;
    pstHvxCore->ctx.u32AllocatedSize = 0;

    return VPP_OK;
}

static uint32_t u32VppIpHvxCore_GetTuningsBufferSize(t_StHvxCoreTunings *pstFuncTunings)
{
    uint32_t u32TuningCnt, u32Size = 0;
    t_StTuning *pstTuning;

    VPP_RET_IF_NULL(pstFuncTunings, 0);

    if (pstFuncTunings->pvTuningBlock)
    {
        u32TuningCnt = u32VppTunings_GetValidTuningsCount(pstFuncTunings->pvTuningBlock);
        if (u32TuningCnt > 0)
        {
            uint32_t u32Idx;
            u32Size = HVX_TUNING_HEADER_SIZE;

            for (u32Idx = 0; u32Idx < u32TuningCnt; u32Idx++)
            {
                pstTuning = pstVppTunings_GetTuningByIndex(pstFuncTunings->pvTuningBlock,
                                                           u32Idx);
                if (!pstTuning)
                {
                    LOGE("Index %u returned null tuning but should be valid!", u32Idx);
                    continue;
                }

                u32Size += HVX_TUNING_PARAM_HEADER_SIZE;
                u32Size += u32VppTunings_GetTuningCount(pstTuning) * HVX_TUNING_ELEMENTS_SIZE;
            }
        }
    }
    return u32Size;
}

static uint32_t u32VppIpHvxCore_TuningElementPack(t_ETuningType eType,
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
            s32Value = float_to_fixed16p16(uValue.FLOAT);
            memcpy(pu32Dest, &s32Value, sizeof(int32_t));
            return VPP_OK;
        default:
            LOGE("given invalid tuning element type to pack. u32=%u", eType);
            return VPP_ERR_PARAM;
    }
}

static uint32_t u32VppIpHvxCore_TuningParamPack(t_StTuning *pstTuning,
                                                vpp_svc_systuneparam_t *pstParam)
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
        u32Ret = u32VppIpHvxCore_TuningElementPack(pstTuning->pstDef->eType,
                                                   pstTuning->puVal[u32],
                                                   &pu32TuneParamElement[u32]);
        LOGE_IF(u32Ret != VPP_OK, "failed to pack tuning element. ret=%u, type=%u, value=0x%x",
                u32Ret, pstTuning->pstDef->eType, pstTuning->puVal[u32].U32);
    }

    return VPP_OK;
}

#ifdef DUMP_TUNING_BUFFER_ENABLED
static void DUMP_TUNING_BUFFER(t_StHvxCoreTunings *pstFuncTunings)
{
    uint32_t *pu32;
    uint32_t u32;

    VPP_RET_VOID_IF_NULL(pstFuncTunings);

    if (pstFuncTunings->pvTuningBlock)
    {
        pu32 = (uint32_t *)pstFuncTunings->stTuningBuf.buf;

        for (u32 = 0;
             u32 < u32VppIpHvxCore_GetTuningsBufferSize(pstFuncTunings) / sizeof(uint32_t);
             u32++, pu32++)
        {
            LOGD("TUNING:DUMP_BUF[%u]: %x", u32, *pu32);
        }
    }
}
#endif

static uint32_t u32VppIpHvxCore_TuningBufferPack(t_StHvxCoreTunings *pstFuncTunings)
{
    uint32_t u32TuningCnt, u32Idx, u32Ret;
    char *pcParam;
    t_StTuning *pstTuning;
    vpp_svc_systune_t *pstSysTune;
    vpp_svc_systuneparam_t *pstParam;

    VPP_RET_IF_NULL(pstFuncTunings, VPP_ERR_PARAM);

    memset(pstFuncTunings->stTuningBuf.buf, 0, pstFuncTunings->stTuningBuf.len);
    pstSysTune = (vpp_svc_systune_t *)pstFuncTunings->stTuningBuf.buf;
    pstSysTune->u32NumTuneParams = 0;
    pcParam = (char *)&pstSysTune->u32Bdy[0];

    u32TuningCnt = u32VppTunings_GetValidTuningsCount(pstFuncTunings->pvTuningBlock);
    for (u32Idx = 0; u32Idx < u32TuningCnt; u32Idx++)
    {
        pstTuning = pstVppTunings_GetTuningByIndex(pstFuncTunings->pvTuningBlock, u32Idx);
        if (!pstTuning)
        {
            LOGE("Index %u returned null tuning but should be valid!", u32Idx);
            continue;
        }

        pstParam = (vpp_svc_systuneparam_t *)pcParam;

        u32Ret = u32VppIpHvxCore_TuningParamPack(pstTuning, pstParam);
        if (u32Ret != VPP_OK)
        {
            LOGE("failed to pack a tuning param. ret=%u", u32Ret);
            return VPP_ERR;
        }

        pstSysTune->u32NumTuneParams++;
        pcParam += HVX_TUNING_PARAM_HEADER_SIZE +
            u32VppTunings_GetTuningCount(pstTuning) * HVX_TUNING_ELEMENTS_SIZE;
    }
#ifdef DUMP_TUNING_BUFFER_ENABLED
    DUMP_TUNING_BUFFER(pstFuncTunings);
#endif
    return VPP_OK;
}

static uint32_t u32VppIpHvxCore_TuningsSend(t_StVppIpHvxCoreCb *pstHvxCore,
                                            vpp_svc_vpp_func_id_t eAlgoId,
                                            uint32_t *pBuf,
                                            uint32_t u32BufSize)
{
    int rc;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pBuf, VPP_ERR_PARAM);
    if (eAlgoId >= VPP_FUNC_ID_NUM)
    {
        LOGE("eAlgoId=%d >= Max=%d", eAlgoId, VPP_FUNC_ID_NUM);
        return VPP_ERR_PARAM;
    }

    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_SEND_TUNINGS);
    rc = vpp_svc_apply_tuning_params_frombuff(pstHvxCore->hvx_handle,
                                              eAlgoId,
                                              pBuf, u32BufSize);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_SEND_TUNINGS);

    if (rc != AEE_SUCCESS)
    {
        LOGE("Error: Send tunings pBuf=%p for eAlgoId=%d failed, rc=%d", pBuf, eAlgoId, rc);
        return VPP_ERR_HW;
    }

    LOGI("Send tunings for eAlgoId=%d success, rc=%d", eAlgoId, rc);
    return VPP_OK;
}

static uint32_t u32VppIpHvxCore_TuningsAlgoBoot(t_StVppIpHvxCoreCb *pstHvxCore,
                                                vpp_svc_vpp_func_id_t eAlgoId,
                                                uint32_t bBootLock)
{
    int rc;
    uint32_t i, u32TuningCnt;
    uint32_t u32Ret = VPP_OK;
    t_StVppSvcTuningDef *pstSvcTuningDef = NULL;
    t_StTuningDef *pstTuningDefSrc = NULL;
    t_StVppIpHvxCoreGlobalCb *pstGlobal;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstHvxCore->pstGlobal, VPP_ERR_PARAM);
    if (eAlgoId >= VPP_FUNC_ID_NUM)
    {
        LOGE("eAlgoId=%d >= Max=%d", eAlgoId, VPP_FUNC_ID_NUM);
        return VPP_ERR_PARAM;
    }

    pstGlobal = pstHvxCore->pstGlobal;

    pthread_mutex_lock(&pstGlobal->mutex);
    if (!pstGlobal->apstTunings[eAlgoId])
    {
        LOGE("apstTunings for eAlgoId=%d is NULL", eAlgoId);
        pthread_mutex_unlock(&pstGlobal->mutex);
        return VPP_ERR_PARAM;
    }
    if (pstGlobal->apstTunings[eAlgoId]->bBooted)
    {
        LOGI("eAlgoId=%d already booted", eAlgoId);
        pthread_mutex_unlock(&pstGlobal->mutex);
        return VPP_OK;
    }

    pstGlobal->apstTunings[eAlgoId]->u32TuningDefCnt = 0;

    rc = vpp_svc_get_num_tuning_params(pstHvxCore->hvx_handle, eAlgoId, &u32TuningCnt);
    if (rc != AEE_SUCCESS)
    {
        LOGE("Error: get number of tuning params for eAlgoId=%u failed, rc=%d", eAlgoId, rc);
        pthread_mutex_unlock(&pstGlobal->mutex);
        return VPP_ERR;
    }
    if (u32TuningCnt > HVX_TUNING_TUNING_COUNT_MAX)
    {
        LOGE("u32TuningCnt=%u is above MAX(%u)", u32TuningCnt, HVX_TUNING_TUNING_COUNT_MAX);
        pthread_mutex_unlock(&pstGlobal->mutex);
        return VPP_ERR_PARAM;
    }
    if (u32TuningCnt == 0)
    {
        LOGI("No tunings supported for eAlgoId=%u", eAlgoId);
        pstGlobal->apstTunings[eAlgoId]->bBooted = VPP_TRUE;
        pstGlobal->apstTunings[eAlgoId]->bBootLock = bBootLock;
        pthread_mutex_unlock(&pstGlobal->mutex);
        return VPP_OK;
    }

    LOGI("Allocating %u svc tuning defs", u32TuningCnt);
    pstSvcTuningDef = calloc(u32TuningCnt, sizeof(t_StVppSvcTuningDef));
    if (!pstSvcTuningDef)
    {
        LOGE("Error allocating memory for pstSvcTuningDef");
        pthread_mutex_unlock(&pstGlobal->mutex);
        return VPP_ERR_NO_MEM;
    }

    rc = vpp_svc_get_tuning_params(pstHvxCore->hvx_handle, eAlgoId,
                                   (unsigned char *)pstSvcTuningDef,
                                   (u32TuningCnt * sizeof(t_StVppSvcTuningDef)));
    if (rc != AEE_SUCCESS)
    {
        LOGE("Error: getting tuning params for eAlgoId=%u, rc=%d", eAlgoId, rc);
        u32Ret = VPP_ERR_HW;
        goto ERR_GET_TUNING_PARAMS;
    }

    pstGlobal->apstTunings[eAlgoId]->pstTuningDefSrc = calloc(u32TuningCnt, sizeof(t_StTuningDef));
    if (!pstGlobal->apstTunings[eAlgoId]->pstTuningDefSrc)
    {
        LOGE("Error allocating memory apstTuningDefSrc");
        u32Ret = VPP_ERR_NO_MEM;
        goto ERR_ALLOC_TUNING_DEF;
    }
    pstTuningDefSrc = pstGlobal->apstTunings[eAlgoId]->pstTuningDefSrc;

    for (i = 0; i < u32TuningCnt; i++)
    {
        strlcpy(pstTuningDefSrc[i].acId, pstSvcTuningDef[i].pcId, VPP_TUNINGS_MAX_TUNING_NAME_SIZE);
        pstTuningDefSrc[i].u32Id = pstSvcTuningDef[i].u32Id;
        pstTuningDefSrc[i].u32Count = pstSvcTuningDef[i].u32Count;
        switch (pstSvcTuningDef[i].eType)
        {
            case VPP_SVC_TUNING_TYPE_FIXED16P16:
                pstTuningDefSrc[i].eType = TUNING_TYPE_FLOAT;
                pstTuningDefSrc[i].min.FLOAT = fixed16p16_to_float(pstSvcTuningDef[i].min.FIXED16P16);
                pstTuningDefSrc[i].max.FLOAT = fixed16p16_to_float(pstSvcTuningDef[i].max.FIXED16P16);
                break;
            case VPP_SVC_TUNING_TYPE_U32:
                pstTuningDefSrc[i].eType = TUNING_TYPE_U32;
                pstTuningDefSrc[i].min.U32 = pstSvcTuningDef[i].min.U32;
                pstTuningDefSrc[i].max.U32 = pstSvcTuningDef[i].max.U32;
                break;
            case VPP_SVC_TUNING_TYPE_S32:
                pstTuningDefSrc[i].eType = TUNING_TYPE_S32;
                pstTuningDefSrc[i].min.S32 = pstSvcTuningDef[i].min.S32;
                pstTuningDefSrc[i].max.S32 = pstSvcTuningDef[i].max.S32;
                break;
            default:
                LOGE("SVC Tuning Type=%d not supported for Tuning ID=%u (%s)",
                     pstSvcTuningDef[i].eType, pstSvcTuningDef[i].u32Id, pstSvcTuningDef[i].pcId);
                u32Ret = VPP_ERR_PARAM;
                break;
        }
        if (u32Ret != VPP_OK)
            break;
    }

    if (u32Ret == VPP_OK)
    {
        pstGlobal->apstTunings[eAlgoId]->u32TuningDefCnt = u32TuningCnt;
        pstGlobal->apstTunings[eAlgoId]->bBooted = VPP_TRUE;
        pstGlobal->apstTunings[eAlgoId]->bBootLock = bBootLock;
    }
    else
    {
        free(pstTuningDefSrc);
        pstGlobal->apstTunings[eAlgoId]->pstTuningDefSrc = NULL;
    }

ERR_ALLOC_TUNING_DEF:
ERR_GET_TUNING_PARAMS:
    free(pstSvcTuningDef);
    pthread_mutex_unlock(&pstGlobal->mutex);

    return u32Ret;
}

static uint32_t u32VppIpHvxCore_TuningsAlgoShutdown(t_StVppIpHvxCoreCb *pstHvxCore,
                                                    vpp_svc_vpp_func_id_t eAlgoId,
                                                    uint32_t bBootLock)
{
    t_StTuningDef *pstTuningDefSrc;
    t_StVppIpHvxCoreGlobalCb *pstGlobal;
    uint32_t u32Ret = VPP_OK;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstHvxCore->pstGlobal, VPP_ERR_PARAM);
    if (eAlgoId >= VPP_FUNC_ID_NUM)
    {
        LOGE("eAlgoId=%d >= Max=%d", eAlgoId, VPP_FUNC_ID_NUM);
        return VPP_ERR_PARAM;
    }

    pstGlobal = pstHvxCore->pstGlobal;
    pthread_mutex_lock(&pstGlobal->mutex);

    if (!pstGlobal->apstTunings[eAlgoId])
    {
        LOGE("apstTunings for eAlgoId=%d is NULL", eAlgoId);
        u32Ret = VPP_ERR_PARAM;
        goto SHUTDOWN_END;
    }

    if (!pstGlobal->apstTunings[eAlgoId]->bBooted)
    {
        LOGE("eAlgoId=%d not booted. Can't shutdown", eAlgoId);
        u32Ret = VPP_ERR_STATE;
        goto SHUTDOWN_END;
    }

    if (pstGlobal->apstTunings[eAlgoId]->bBootLock && !bBootLock)
    {
        LOGD("eAlgoId=%d bootlocked, but bootlock flag not given. Skipping shutdown", eAlgoId);
        u32Ret = VPP_ERR_STATE;
        goto SHUTDOWN_END;
    }

    if (pstHvxCore->pstGlobal->apstTunings[eAlgoId]->stSession.u32Cnt)
    {
        LOGD("eAlgoId=%d has %u other sessions open. Can't shutdown",
             eAlgoId, pstHvxCore->pstGlobal->apstTunings[eAlgoId]->stSession.u32Cnt);
        u32Ret = VPP_ERR_STATE;
        goto SHUTDOWN_END;
    }

    pstTuningDefSrc = pstGlobal->apstTunings[eAlgoId]->pstTuningDefSrc;
    if (pstTuningDefSrc)
    {
        free(pstTuningDefSrc);
        pstGlobal->apstTunings[eAlgoId]->pstTuningDefSrc = NULL;
    }

    pstGlobal->apstTunings[eAlgoId]->bBooted = VPP_FALSE;
    pstGlobal->apstTunings[eAlgoId]->bBootLock = VPP_FALSE;

SHUTDOWN_END:
    pthread_mutex_unlock(&pstGlobal->mutex);
    return u32Ret;
}


static uint32_t u32VppIpHvxCore_TuningsAlgoInit(t_StVppIpHvxCoreCb *pstHvxCore,
                                                const char *pcFileName,
                                                vpp_svc_vpp_func_id_t eAlgoId,
                                                uint32_t bForceInit)
{
    void *pvTuningBlock;
    uint32_t u32Ret = VPP_OK;
    uint32_t u32TuningCnt;
    t_StTuningDef *pstTuningDefSrc;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstHvxCore->pstGlobal, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pcFileName, VPP_ERR_PARAM);
    if (eAlgoId >= VPP_FUNC_ID_NUM)
    {
        LOGE("eAlgoId=%d >= Max=%d", eAlgoId, VPP_FUNC_ID_NUM);
        return VPP_ERR_PARAM;
    }

    pthread_mutex_lock(&pstHvxCore->pstGlobal->mutex);
    if (!pstHvxCore->pstGlobal->apstTunings[eAlgoId]->bBooted)
    {
        LOGE("Tunings block for eAlgoId=%d not booted", eAlgoId);
        pthread_mutex_unlock(&pstHvxCore->pstGlobal->mutex);
        return VPP_ERR_STATE;
    }
    if (pstHvxCore->pstGlobal->apstTunings[eAlgoId]->pvTuningBlock)
    {
        if (!bForceInit)
        {
            LOGI("Tunings block for eAlgoId=%d already inited, pvTuningBlock=%p",
                 eAlgoId, pstHvxCore->pstGlobal->apstTunings[eAlgoId]->pvTuningBlock);
            pthread_mutex_unlock(&pstHvxCore->pstGlobal->mutex);
            return VPP_OK;
        }
        LOGI("Re-initing tunings block for eAlgoId=%d", eAlgoId);
        vVppTunings_Term(pstHvxCore->pstGlobal->apstTunings[eAlgoId]->pvTuningBlock);
        pstHvxCore->pstGlobal->apstTunings[eAlgoId]->pvTuningBlock = NULL;
    }

    pstTuningDefSrc = pstHvxCore->pstGlobal->apstTunings[eAlgoId]->pstTuningDefSrc;
    u32TuningCnt = pstHvxCore->pstGlobal->apstTunings[eAlgoId]->u32TuningDefCnt;

    pvTuningBlock = vpVppTunings_Init(pcFileName, pstTuningDefSrc, u32TuningCnt);
    pstHvxCore->pstGlobal->apstTunings[eAlgoId]->pvTuningBlock = pvTuningBlock;
    pthread_mutex_unlock(&pstHvxCore->pstGlobal->mutex);

    return u32Ret;
}

static void vVppIpHvxCore_TuningsAlgoTerm(t_StVppIpHvxCoreCb *pstHvxCore,
                                          vpp_svc_vpp_func_id_t eAlgoId,
                                          uint32_t bBootLock)
{
    VPP_RET_VOID_IF_NULL(pstHvxCore);
    VPP_RET_VOID_IF_NULL(pstHvxCore->pstGlobal);
    if (eAlgoId >= VPP_FUNC_ID_NUM)
    {
        LOGE("eAlgoId=%d >= Max=%d", eAlgoId, VPP_FUNC_ID_NUM);
        return;
    }

    pthread_mutex_lock(&pstHvxCore->pstGlobal->mutex);
    if (!pstHvxCore->pstGlobal->apstTunings[eAlgoId]->pvTuningBlock)
    {
        LOGE("pvTuningBlock for eAlgoId=%d is already NULL", eAlgoId);
        pthread_mutex_unlock(&pstHvxCore->pstGlobal->mutex);
        return;
    }

    if (pstHvxCore->pstGlobal->apstTunings[eAlgoId]->stSession.u32Cnt)
    {
        LOGD("eAlgoId=%d has %u other sessions open. Don't need to term",
             eAlgoId, pstHvxCore->pstGlobal->apstTunings[eAlgoId]->stSession.u32Cnt);
        pthread_mutex_unlock(&pstHvxCore->pstGlobal->mutex);
        return;
    }

    if (pstHvxCore->pstGlobal->apstTunings[eAlgoId]->bBootLock && !bBootLock)
    {
        LOGD("eAlgoId=%d bootlocked, but bootlock flag not given. Don't need to term",
             eAlgoId);
        pthread_mutex_unlock(&pstHvxCore->pstGlobal->mutex);
        return;
    }

    vVppTunings_Term(pstHvxCore->pstGlobal->apstTunings[eAlgoId]->pvTuningBlock);
    pstHvxCore->pstGlobal->apstTunings[eAlgoId]->pvTuningBlock = NULL;
    pthread_mutex_unlock(&pstHvxCore->pstGlobal->mutex);
}

static uint32_t u32VppIpHvxCore_TuningsBootInternal(t_StVppIpHvxCoreCb *pstHvxCore,
                                                    const char *pcFileName,
                                                    uint32_t u32VppProcFlags,
                                                    uint32_t bBootLock,
                                                    uint32_t bForceInit,
                                                    uint32_t bCoreBoot)
{
    uint32_t u32Err;
    vpp_svc_vpp_func_id_t eAlgoId;
    t_StVppIpHvxCoreGlobalCb *pstGlobal;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pcFileName, VPP_ERR_PARAM);
    if (!u32VppProcFlags)
    {
        LOGD("No processing flags set, nothing to boot");
        return VPP_OK;
    }
    LOG_ENTER();

    pstGlobal = pstHvxCore->pstGlobal;
    if (!bCoreBoot)
    {
        vVppIpHvxCore_GlobalMutexWaitStateSet(HVX_CORE_GLOBAL_STATE_BOOT);
    }

    for (eAlgoId = 0; eAlgoId < VPP_FUNC_ID_NUM; eAlgoId++)
    {
        if (u32VppProcFlags & (0x01 << eAlgoId))
        {
            // Not fatal if error, continue to try to boot the others
            pthread_mutex_lock(&pstGlobal->mutex);
            if (!pstGlobal->apstTunings[eAlgoId])
            {
                pstGlobal->apstTunings[eAlgoId] = calloc(1, sizeof(t_StHvxCoreTunings));
                if (!pstGlobal->apstTunings[eAlgoId])
                {
                    LOGE("Error allocating memory for tunings struct for eAlgoId=%u", eAlgoId);
                    pthread_mutex_unlock(&pstGlobal->mutex);
                    continue;
                }
            }
            pthread_mutex_unlock(&pstGlobal->mutex);
            u32Err = u32VppIpHvxCore_TuningsAlgoBoot(pstHvxCore, eAlgoId, bBootLock);
            if (u32Err != VPP_OK)
            {
                LOGE("Error booting tunings for eAlgoId=%d, u32Err=%u", eAlgoId, u32Err);
                continue;
            }
            u32Err = u32VppIpHvxCore_TuningsAlgoInit(pstHvxCore, pcFileName, eAlgoId, bForceInit);
            LOGE_IF(u32Err != VPP_OK, "Error initing tunings for eAlgoId=%d, u32Err=%u",
                    eAlgoId, u32Err);
        }
    }

    if (!bCoreBoot)
    {
        pthread_mutex_lock(&pstGlobal->mutex);
        pstGlobal->eState = HVX_CORE_GLOBAL_STATE_READY;
        pthread_mutex_unlock(&pstGlobal->mutex);
        pthread_cond_broadcast(&pstGlobal->cond);
    }
    LOG_EXIT_RET(VPP_OK);
}

static uint32_t u32VppIpHvxCore_TuningsShutdownInternal(t_StVppIpHvxCoreCb *pstHvxCore,
                                                        uint32_t u32VppProcFlags,
                                                        uint32_t bBootLock,
                                                        uint32_t bCoreShutdown)
{
    uint32_t u32Err, u32Ret = VPP_OK;
    vpp_svc_vpp_func_id_t eAlgoId;
    t_StVppIpHvxCoreGlobalCb *pstGlobal;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    if (!u32VppProcFlags)
    {
        LOGD("No processing flags set, nothing to shutdown");
        return VPP_OK;
    }
    LOG_ENTER();

    pstGlobal = pstHvxCore->pstGlobal;
    if (!bCoreShutdown)
    {
        vVppIpHvxCore_GlobalMutexWaitStateSet(HVX_CORE_GLOBAL_STATE_SHUTDOWN);
    }

    for (eAlgoId = 0; eAlgoId < VPP_FUNC_ID_NUM; eAlgoId++)
    {
        if (u32VppProcFlags & (0x01 << eAlgoId))
        {
            if (!pstGlobal->apstTunings[eAlgoId])
            {
                LOGE("apstTunings for eAlgoId=%d is NULL, can't shutdown", eAlgoId);
                u32Ret = VPP_ERR_STATE;
                continue;
            }
            vVppIpHvxCore_TuningsAlgoTerm(pstHvxCore, eAlgoId, bBootLock);
            u32Err = u32VppIpHvxCore_TuningsAlgoShutdown(pstHvxCore, eAlgoId, bBootLock);
            if (u32Err != VPP_OK)
            {
                LOGD("Can't shutdown tunings for eAlgoId=%d, u32Err=%u", eAlgoId, u32Err);
                u32Ret = u32Err;
                continue;
            }
            pthread_mutex_lock(&pstGlobal->mutex);
            free(pstGlobal->apstTunings[eAlgoId]);
            pstGlobal->apstTunings[eAlgoId] = NULL;
            pthread_mutex_unlock(&pstGlobal->mutex);
        }
    }

    if (!bCoreShutdown)
    {
        pthread_mutex_lock(&pstGlobal->mutex);
        pstGlobal->eState = HVX_CORE_GLOBAL_STATE_READY;
        pthread_mutex_unlock(&pstGlobal->mutex);
        pthread_cond_broadcast(&pstGlobal->cond);
    }
    LOG_EXIT_RET(u32Ret);
}

static uint32_t u32VppIpHvxCore_FirmwareSessionOpen(t_StVppIpHvxCoreCb *pstHvxCore,
                                                    uint32_t u32VppProcFlags)
{
    int rc;
    uint32_t u32Length, u32CtrlCnt = 0, u32Index = 0;
    uint32_t u32Ret = VPP_OK;
    vpp_svc_vpp_func_id_t eAlgoId;
    const char *pcUri = vpp_svc_URI CDSP_DOMAIN;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    if (!u32VppProcFlags)
    {
        LOGD("No processing flags to open session with");
        return VPP_ERR_PARAM;
    }

    rc = vpp_svc_open(pcUri, &pstHvxCore->hvx_handle);
    if ((rc != AEE_SUCCESS) || (pstHvxCore->hvx_handle == HVX_HANDLE_INVALID))
    {
        LOGE("ERROR: vpp_svc_open failed! rc=%d, handle=0x%"PRIx64, rc, pstHvxCore->hvx_handle);
        return VPP_ERR_HW;
    }

    pstHvxCore->pstCapabilityResources = (vpp_svc_cap_resource_list_t *)
        HVX_ALLOC(sizeof(vpp_svc_cap_resource_list_t));
    if (pstHvxCore->pstCapabilityResources == NULL)
    {
        LOGE("Error: pstCapabilityResources alloc failed");
        u32Ret = VPP_ERR_NO_MEM;
        goto ERR_CAPABILITY;
    }

    for (eAlgoId = 0; eAlgoId < VPP_FUNC_ID_NUM; eAlgoId++)
    {
        if (u32VppProcFlags & (0x01 << eAlgoId))
        {
            u32CtrlCnt++;
        }
    }

    LOGI("Boot u32CtrlCnt=%u", u32CtrlCnt);
    pstHvxCore->pstCapabilityResources->resourceLen = u32CtrlCnt;
    u32Length = sizeof(vpp_svc_cap_resource_t) * u32CtrlCnt;
    pstHvxCore->pstCapabilityResources->resource =
        (vpp_svc_cap_resource_t*)HVX_ALLOC(u32Length);
    if (pstHvxCore->pstCapabilityResources->resource == NULL)
    {
        LOGE("Error: pstCapabilityResources->resource alloc failed");
        u32Ret = VPP_ERR_NO_MEM;
        goto ERR_CAPABILITY_RES;
    }

    for (eAlgoId = 0; eAlgoId < VPP_FUNC_ID_NUM; eAlgoId++)
    {
        if (u32VppProcFlags & (0x01 << eAlgoId))
        {
            u32VppIpHvxCore_AlgoInit(pstHvxCore, u32Index, eAlgoId, NULL);
            u32Index++;
        }
    }

    rc = vpp_svc_init(pstHvxCore->hvx_handle, pstHvxCore->pstCapabilityResources);
    if (rc != AEE_SUCCESS)
    {
        LOGE("vpp_svc_init() error, rc=%d", rc);
        u32Ret = VPP_ERR_HW;
        goto ERR_VPP_SVC_INIT;
    }
    LOGI("vpp_svc_init() successful");

    return u32Ret;

ERR_VPP_SVC_INIT:
    free(pstHvxCore->pstCapabilityResources->resource);

ERR_CAPABILITY_RES:
    free(pstHvxCore->pstCapabilityResources);
    pstHvxCore->pstCapabilityResources = NULL;

ERR_CAPABILITY:
    rc = vpp_svc_close(pstHvxCore->hvx_handle);
    LOGE_IF(rc != AEE_SUCCESS, "ERROR: vpp_svc_close failed rc=%d", rc);
    pstHvxCore->hvx_handle = HVX_HANDLE_INVALID;

    return u32Ret;
}

static void vVppIpHvxCore_FirmwareSessionClose(t_StVppIpHvxCoreCb *pstHvxCore)
{
    int rc;

    VPP_RET_VOID_IF_NULL(pstHvxCore);

    rc = vpp_svc_deinit(pstHvxCore->hvx_handle, pstHvxCore->pstCapabilityResources);
    LOGE_IF(rc != AEE_SUCCESS, "vpp_svc_deinit() failed, rc=%d", rc);

    if (pstHvxCore->pstCapabilityResources)
    {
        if (pstHvxCore->pstCapabilityResources->resource)
            free(pstHvxCore->pstCapabilityResources->resource);

        free(pstHvxCore->pstCapabilityResources);
        pstHvxCore->pstCapabilityResources = NULL;
    }

    rc = vpp_svc_close(pstHvxCore->hvx_handle);
    LOGE_IF(rc != AEE_SUCCESS, "ERROR: vpp_svc_close failed rc=%d", rc);
    pstHvxCore->hvx_handle = HVX_HANDLE_INVALID;
}

/************************************************************************
 * Global Functions
 ***********************************************************************/

vpp_svc_field_fmt_t eVppIpHvxCore_SvcFieldFormatGet(t_EVppBufType eBufType)
{
    vpp_svc_field_fmt_t eFmt;
    switch (eBufType)
    {
        case eVppBufType_Interleaved_TFF:
            eFmt = FIELD_FMT_INTERLEAVED_TFF;
            break;

        case eVppBufType_Interleaved_BFF:
            eFmt = FIELD_FMT_INTERLEAVED_BFF;
            break;
        case eVppBufType_Frame_TFF:
            eFmt = FIELD_FMT_TFF;
            break;

        case eVppBufType_Frame_BFF:
            eFmt = FIELD_FMT_BFF;
            break;

        case eVppBufType_Progressive:
            eFmt = FIELD_FMT_PROGRESSIVE;
            break;

        case eVppBufType_Max:
        default:
            LOGE("Requested invalid format: eBufType=%d", eBufType);
            eFmt = FIELD_FMT_PROGRESSIVE;
            break;
    }
    return eFmt;
}

vpp_svc_pixel_fmt_t eVppIpHvxCore_SvcPixelFormatGet(enum vpp_color_format fmt)
{
    switch (fmt)
    {
        case VPP_COLOR_FORMAT_NV12_VENUS:
            return PIXEL_FMT_NV12;
        case VPP_COLOR_FORMAT_NV21_VENUS:
            return PIXEL_FMT_NV21;
        case VPP_COLOR_FORMAT_P010:
            return PIXEL_FMT_P010;
        case VPP_COLOR_FORMAT_UBWC_NV12:
            return PIXEL_FMT_NV12_UBWC;
        case VPP_COLOR_FORMAT_UBWC_NV21:
            return PIXEL_FMT_NV21_UBWC;
        case VPP_COLOR_FORMAT_UBWC_TP10:
            return PIXEL_FMT_TP10_UBWC;
        default:
            LOGE("unsupported VPP color format: %u, returning default HVX format %u",
                 fmt, PIXEL_FMT_NV12);
            return PIXEL_FMT_NV12;
    }
}

inline void *hvx_alloc_int(uint32_t sz, const char *str)
{
    LOGI("-- allocating %d bytes for: %s", sz, str);
    return memalign(128, sz);
}

void vVppIpHvxCore_RegisterBuffer(t_StVppIpHvxCoreCb *pstHvxCore, t_StVppMemBuf *pstIntMemBuf)
{
    if (!pstHvxCore || !pstHvxCore->stBase.pstCtx || !pstIntMemBuf)
        return;

    if (VPP_FLAG_IS_SET(pstIntMemBuf->u32IntBufFlags, VPP_BUF_FLAG_HVX_REGISTERED))
        return;

    // In a secure session, buffer management logic can not map this into
    // virtual address space for us, but the interface to HVX requires that the
    // buffer have a virtual address. Thus, we need to ask the adsp driver to
    // give us a virtual address to use. We will replace the vaddr here in the
    // buffer, and when we do the unregister call, we will replace it with
    // NULL. Must take care here not to modify the u32MappedLen or anything
    // like that.
    if (pstIntMemBuf->eMapping == eVppBuf_Unmapped)
    {
        void *pv = NULL;
        if (remote_register_fd)
        {
            VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_REGBUF);
            pv = remote_register_fd(pstIntMemBuf->fd, pstIntMemBuf->u32AllocLen);
            VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_REGBUF);

            if (pv == (void *)-1 || pv == NULL)
            {
                LOGE("unable to remote_register_fd, fd=%d, alloc_len=%u, "
                     "dummy_ptr=%p", pstIntMemBuf->fd, pstIntMemBuf->u32AllocLen, pv);
            }
            else
            {
                pstIntMemBuf->pvBase = pv;
                VPP_FLAG_SET(pstIntMemBuf->u32IntBufFlags, VPP_BUF_FLAG_HVX_REGISTERED);
                LOGI("remote_register_fd, fd=%d, sz=%u, new_ptr=%p",
                     pstIntMemBuf->fd, pstIntMemBuf->u32AllocLen, pv);
            }
        }
        else
            LOGE("unable to remote_register_fd, fd=%d, ptr=%p, sz=%u",
                 pstIntMemBuf->fd, pv, pstIntMemBuf->u32AllocLen);

    }
    else
    {
        if (remote_register_buf)
        {
            VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_REGBUF);
            remote_register_buf(pstIntMemBuf->pvBase, pstIntMemBuf->u32MappedLen,
                                pstIntMemBuf->fd);
            VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_REGBUF);

            VPP_FLAG_SET(pstIntMemBuf->u32IntBufFlags, VPP_BUF_FLAG_HVX_REGISTERED);

            LOGI("remote_register_buf, fd=%d, ptr=%p, sz=%u", pstIntMemBuf->fd,
                 pstIntMemBuf->pvBase, pstIntMemBuf->u32MappedLen);
        }
        else
            LOGE("unable to remote_register_buf, fd=%d, ptr=%p, sz=%u",
                 pstIntMemBuf->fd, pstIntMemBuf->pvBase, pstIntMemBuf->u32MappedLen);
    }
}

void vVppIpHvxCore_UnregisterBuffer(t_StVppIpHvxCoreCb *pstHvxCore, t_StVppMemBuf *pstIntMemBuf)
{
    if (!pstHvxCore || !pstHvxCore->stBase.pstCtx || !pstIntMemBuf)
        return;

    if (!VPP_FLAG_IS_SET(pstIntMemBuf->u32IntBufFlags, VPP_BUF_FLAG_HVX_REGISTERED))
        return;

    if (remote_register_buf)
    {
        uint32_t u32MappedLen = pstIntMemBuf->u32MappedLen;
        if (pstIntMemBuf->eMapping == eVppBuf_Unmapped)
        {
            u32MappedLen = pstIntMemBuf->u32AllocLen;
        }

        VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_UNREGBUF);
        remote_register_buf(pstIntMemBuf->pvBase, u32MappedLen, -1);
        VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_UNREGBUF);

        if (pstIntMemBuf->eMapping == eVppBuf_Unmapped)
        {
            pstIntMemBuf->pvBase = VPP_BUF_UNMAPPED_BUF_VAL;
        }

        VPP_FLAG_CLR(pstIntMemBuf->u32IntBufFlags, VPP_BUF_FLAG_HVX_REGISTERED);

        LOGI("unregistering buffer, fd=%d, ptr=%p, sz=%u", pstIntMemBuf->fd,
             pstIntMemBuf->pvBase, u32MappedLen);
    }
}

uint32_t bVppIpHvxCore_IsSecure(t_StVppIpHvxCoreCb *pstHvxCore)
{
    if (!pstHvxCore)
        return VPP_FALSE;

    return pstHvxCore->stBase.bSecureSession;
}

uint32_t u32VppIpHvxCore_SvcParamSetROI(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t bEnable,
                                        uint32_t bCompute, uint32_t u32XStart, uint32_t u32YStart,
                                        uint32_t u32XEnd, uint32_t u32YEnd, uint32_t u32LineWidth,
                                        uint32_t u32LineY, uint32_t u32LineCr, uint32_t u32LineCb)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstHvxCore->pstParams->gen_params.roi_en = bEnable;
    if (bCompute) {
        pstHvxCore->pstParams->gen_params.roi_x_tl = u32XStart;
        pstHvxCore->pstParams->gen_params.roi_y_tl = u32YStart;
        pstHvxCore->pstParams->gen_params.roi_x_br = u32XEnd;
        pstHvxCore->pstParams->gen_params.roi_y_br = u32YEnd;
        pstHvxCore->pstParams->gen_params.transit_reg_wid = u32LineWidth;
        pstHvxCore->pstParams->gen_params.transit_reg_yval = u32LineY;
        pstHvxCore->pstParams->gen_params.transit_reg_c1val = u32LineCr;
        pstHvxCore->pstParams->gen_params.transit_reg_c2val = u32LineCb;
    }
    return VPP_OK;
}

uint32_t u32VppIpHvxCore_SvcParamSetHeaderIdxAlgo(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32Index,
                                                  uint32_t u32Algo)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    (pstHvxCore->pstParams->header + u32Index)->vpp_func_id = u32Algo;
    (pstHvxCore->pstParams->header + u32Index)->process_flags = 1;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_SvcParamSetDataSize(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32AlgoCnt,
                                             uint32_t u32DataLength)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstHvxCore->pstParams->headerLen = u32AlgoCnt;
    pstHvxCore->pstParams->user_dataLen = u32DataLength;

    return VPP_OK;
}

vpp_svc_config_hdr_t *pstVppIpHvxCore_SvcParamGetHeaderIdxAddr(t_StVppIpHvxCoreCb *pstHvxCore,
                                                               uint32_t u32Index)
{
    if (!pstHvxCore)
        return NULL;

    return (vpp_svc_config_hdr_t*)(pstHvxCore->pstParams->header + u32Index);
}

void *pvVppIpHvxCore_SvcParamGetDataOffsetAddr(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32Offset)
{
    if (!pstHvxCore)
        return NULL;

    return (void *)(pstHvxCore->pstParams->user_data + u32Offset);
}

uint32_t u32VppIpHvxCore_BufParamSetSize(t_StVppIpHvxCoreCb *pstHvxCore,
                                         uint32_t u32Width, uint32_t u32Height)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstHvxCore->stHvxBufParams.u32InWidth = u32Width;
    pstHvxCore->stHvxBufParams.u32InHeight = u32Height;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufParamGetSize(t_StVppIpHvxCoreCb *pstHvxCore,
                                         uint32_t *pu32Width, uint32_t *pu32Height)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pu32Width, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pu32Height, VPP_ERR_PARAM);

    *pu32Width = pstHvxCore->stHvxBufParams.u32InWidth;
    *pu32Height = pstHvxCore->stHvxBufParams.u32InHeight;

    return VPP_OK;

}

uint32_t u32VppIpHvxCore_BufParamSetPixFmt(t_StVppIpHvxCoreCb *pstHvxCore,
                                           enum vpp_svc_pixel_fmt_t eFmt)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstHvxCore->stHvxBufParams.ePixFmt = eFmt;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufParamGetPixFmt(t_StVppIpHvxCoreCb *pstHvxCore,
                                           enum vpp_svc_pixel_fmt_t *peFmt)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(peFmt, VPP_ERR_PARAM);

    *peFmt = pstHvxCore->stHvxBufParams.ePixFmt;

    return VPP_OK;

}

uint32_t u32VppIpHvxCore_BufParamSetFldFmt(t_StVppIpHvxCoreCb *pstHvxCore,
                                           enum vpp_svc_field_fmt_t eFmt)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstHvxCore->stHvxBufParams.eFieldFmt = eFmt;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufParamGetFldFmt(t_StVppIpHvxCoreCb *pstHvxCore,
                                           enum vpp_svc_field_fmt_t *peFmt)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(peFmt, VPP_ERR_PARAM);

    *peFmt = pstHvxCore->stHvxBufParams.eFieldFmt;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufParamSetPlaneSize(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t *pu32Size,
                                              uint32 u32Cnt)
{
    uint32_t i;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pu32Size, VPP_ERR_PARAM);

    if (u32Cnt > MAX_NUM_PLANES)
    {
        LOGE("u32Cnt=%u higher than MAX_NUM_PLANES=%u", u32Cnt, MAX_NUM_PLANES);
        return VPP_ERR_PARAM;
    }

    for (i = 0; i < MAX_NUM_PLANES; i++)
    {
        if (i < u32Cnt)
            pstHvxCore->stHvxBufParams.au32PlSizeBytes[i] = pu32Size[i];
        else
            pstHvxCore->stHvxBufParams.au32PlSizeBytes[i] = 0;
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufParamGetPlaneSize(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t *pu32Size,
                                              uint32 u32Cnt)
{
    uint32_t i;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pu32Size, VPP_ERR_PARAM);

    if (u32Cnt > MAX_NUM_PLANES)
    {
        LOGE("u32Cnt=%u higher than MAX_NUM_PLANES=%u", u32Cnt, MAX_NUM_PLANES);
        return VPP_ERR_PARAM;
    }

    for (i = 0; i < u32Cnt; i++)
    {
        pu32Size[i] = pstHvxCore->stHvxBufParams.au32PlSizeBytes[i];
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufParamGetPlaneTotalSize(t_StVppIpHvxCoreCb *pstHvxCore,
                                                   uint32_t *pu32Size)
{
    uint32_t i;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pu32Size, VPP_ERR_PARAM);

    *pu32Size = 0;
    for (i = 0; i < MAX_NUM_PLANES; i++)
    {
        *pu32Size += pstHvxCore->stHvxBufParams.au32PlSizeBytes[i];
    }

    return VPP_OK;
}


uint32_t u32VppIpHvxCore_BufParamSetPlaneStride(t_StVppIpHvxCoreCb *pstHvxCore,
                                                uint32_t *pu32Stride, uint32 u32Cnt)
{
    uint32_t i;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pu32Stride, VPP_ERR_PARAM);

    if (u32Cnt > MAX_NUM_PLANES)
    {
        LOGE("u32Cnt=%u higher than MAX_NUM_PLANES=%u", u32Cnt, MAX_NUM_PLANES);
        return VPP_ERR_PARAM;
    }

    for (i = 0; i < MAX_NUM_PLANES; i++)
    {
        if (i < u32Cnt)
            pstHvxCore->stHvxBufParams.au32PlStride[i] = pu32Stride[i];
        else
            pstHvxCore->stHvxBufParams.au32PlStride[i] = 0;
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufParamGetPlaneStride(t_StVppIpHvxCoreCb *pstHvxCore,
                                                uint32_t *pu32Stride, uint32 u32Cnt)
{
    uint32_t i;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pu32Stride, VPP_ERR_PARAM);

    if (u32Cnt > MAX_NUM_PLANES)
    {
        LOGE("u32Cnt=%u higher than MAX_NUM_PLANES=%u", u32Cnt, MAX_NUM_PLANES);
        return VPP_ERR_PARAM;
    }

    for (i = 0; i < u32Cnt; i++)
    {
        pu32Stride[i] = pstHvxCore->stHvxBufParams.au32PlStride[i];
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufParamInit(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32Width,
                                      uint32_t u32Height, enum vpp_color_format eFmt)
{
    uint32_t u32Planes, i;
    uint32_t au32Size[MAX_NUM_PLANES], au32Stride[MAX_NUM_PLANES];
    vpp_svc_pixel_fmt_t pix_fmt = _32BIT_PLACEHOLDER_vpp_svc_pixel_fmt_t;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    memset(au32Size, 0, sizeof(au32Size));
    memset(au32Stride, 0, sizeof(au32Stride));

    u32Planes = u32VppUtils_GetNumPlanes(eFmt);
    if (u32Planes > MAX_NUM_PLANES)
    {
        LOGE("u32VppUtils_GetNumPlanes returned %u planes for color format %u, but max is %u",
             u32Planes, eFmt, MAX_NUM_PLANES);
        return VPP_ERR;
    }

    for (i = 0; i < u32Planes; i++)
    {
        au32Stride[i] = u32VppUtils_CalcStrideForPlane(u32Width, eFmt, i);
        au32Size[i] = u32VppUtils_GetPlaneSize(u32Width, u32Height, eFmt, i);
        LOGI("Plane[%u]: Width=%u Height=%u ePixFmt=%u Stride=%u PlaneSize=%u",
             i, u32Width, u32Height, pix_fmt, au32Stride[i], au32Size[i]);
    }

    pix_fmt = eVppIpHvxCore_SvcPixelFormatGet(eFmt);
    u32VppIpHvxCore_BufParamSetSize(pstHvxCore, u32Width, u32Height);
    u32VppIpHvxCore_BufParamSetPixFmt(pstHvxCore, pix_fmt);
    u32VppIpHvxCore_BufParamSetPlaneSize(pstHvxCore, au32Size, u32Planes);
    u32VppIpHvxCore_BufParamSetPlaneStride(pstHvxCore, au32Stride, u32Planes);
    u32VppIpHvxCore_BufParamSetFldFmt(pstHvxCore, FIELD_FMT_PROGRESSIVE);

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BuffInInit(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32NumBuffers)
{
    uint32_t u32PixDataAllocSize;
    uint32_t i;
    vpp_svc_frame_group_descriptor_t* pstBufferdataIn;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    if (u32NumBuffers > MAX_BUFFS_IN_GROUP)
    {
        LOGE("Trying to init %u buffers but max is %u", u32NumBuffers, MAX_BUFFS_IN_GROUP);
        return VPP_ERR_PARAM;
    }

    pstBufferdataIn = pstHvxCore->pstBufferdataIn;
    u32PixDataAllocSize = sizeof(vpp_svc_raw_buffer_t) * u32NumBuffers;

    memset((void*)pstBufferdataIn, 0, sizeof(vpp_svc_frame_group_descriptor_t));

    pstBufferdataIn->pixel_data = (vpp_svc_raw_buffer_t*)HVX_ALLOC(u32PixDataAllocSize);
    if (!pstBufferdataIn->pixel_data)
        return VPP_ERR_NO_MEM;

    pstBufferdataIn->numbuffers = u32NumBuffers;
    pstBufferdataIn->pixel_dataLen = u32NumBuffers;

    for (i = 0; i < pstBufferdataIn->numbuffers; i++)
    {
        pstBufferdataIn->bufferattributes[i].index = i;
    }

    return VPP_OK;
}

void vVppIpHvxCore_BuffInTerm(t_StVppIpHvxCoreCb *pstHvxCore)
{
    if ((pstHvxCore) && (pstHvxCore->pstBufferdataIn))
    {
        if (pstHvxCore->pstBufferdataIn->pixel_data)
            free(pstHvxCore->pstBufferdataIn->pixel_data);

        pstHvxCore->pstBufferdataIn->pixel_data = NULL;
    }
}

uint32_t u32VppIpHvxCore_BuffInCompute(t_StVppIpHvxCoreCb *pstHvxCore)
{
    t_StHvxCoreBufParams *pstBufParams;
    vpp_svc_frame_group_descriptor_t* pstBufferdataIn;
    uint32_t u32FrameSize = 0;
    uint32_t au32Stride[MAX_NUM_PLANES];
    uint32_t au32Size[MAX_NUM_PLANES];
    uint32_t i, j;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    memset(au32Size, 0, sizeof(au32Size));
    memset(au32Stride, 0, sizeof(au32Stride));
    pstBufParams = &pstHvxCore->stHvxBufParams;
    pstBufferdataIn = pstHvxCore->pstBufferdataIn;
    u32VppIpHvxCore_BufParamGetPlaneSize(pstHvxCore, au32Size, MAX_NUM_PLANES);
    u32VppIpHvxCore_BufParamGetPlaneTotalSize(pstHvxCore, &u32FrameSize);
    u32VppIpHvxCore_BufParamGetPlaneStride(pstHvxCore, au32Stride, MAX_NUM_PLANES);

    pstBufferdataIn->width = pstBufParams->u32InWidth;
    pstBufferdataIn->height = pstBufParams->u32InHeight;
    pstBufferdataIn->pixelformat = pstBufParams->ePixFmt;
    pstBufferdataIn->fieldformat = pstBufParams->eFieldFmt;

    for (i = 0; i < pstBufferdataIn->numbuffers; i++)
    {
        u32VppIpHvxCore_BufInSetAttrSize(pstHvxCore, i, au32Size, MAX_NUM_PLANES);
        u32VppIpHvxCore_BufInSetAttrStride(pstHvxCore, i, au32Stride, MAX_NUM_PLANES);
        u32VppIpHvxCore_BufInSetUserDataLen(pstHvxCore, i, u32FrameSize);

        LOGD("pstBufferdataIn[%u] len=%u", i, u32FrameSize);
        for (j = 0; j < MAX_NUM_PLANES; j++)
        {
            LOGD("Plane[%u] Stride=%u SizeBytes=%u", j, au32Stride[j], au32Size[j]);
        }
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufInSetSize(t_StVppIpHvxCoreCb *pstHvxCore,
                                         uint32_t u32Width, uint32_t u32Height)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstHvxCore->pstBufferdataIn->width = u32Width;
    pstHvxCore->pstBufferdataIn->height = u32Height;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufInSetPixFmt(t_StVppIpHvxCoreCb *pstHvxCore,
                                        enum vpp_svc_pixel_fmt_t eFmt)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstHvxCore->pstBufferdataIn->pixelformat = eFmt;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufInSetFldFmt(t_StVppIpHvxCoreCb *pstHvxCore,
                                        enum vpp_svc_field_fmt_t eFmt)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstHvxCore->pstBufferdataIn->fieldformat = eFmt;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufInSetNumBuffers(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32NumBuffers)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstHvxCore->pstBufferdataIn->numbuffers = u32NumBuffers;
    pstHvxCore->pstBufferdataIn->pixel_dataLen = u32NumBuffers;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufInSetAttrSize(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32Idx,
                                          uint32_t *pu32Size, uint32_t u32Cnt)
{
    uint32_t i;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pu32Size, VPP_ERR_PARAM);

    if (u32Idx >= pstHvxCore->pstBufferdataIn->numbuffers)
    {
        LOGE("u32Idx=%u but allocated=%u", u32Idx, pstHvxCore->pstBufferdataIn->numbuffers);
        return VPP_ERR_PARAM;
    }

    if (u32Cnt > MAX_NUM_PLANES)
    {
        LOGE("u32Cnt=%u higher than MAX_NUM_PLANES=%u", u32Cnt, MAX_NUM_PLANES);
        return VPP_ERR_PARAM;
    }

    for (i = 0; i < MAX_NUM_PLANES; i++)
    {
        if (i < u32Cnt)
            pstHvxCore->pstBufferdataIn->bufferattributes[u32Idx].plane_sizebytes[i] = pu32Size[i];
        else
            pstHvxCore->pstBufferdataIn->bufferattributes[u32Idx].plane_sizebytes[i] = 0;
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufInSetAttrStride(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32Idx,
                                            uint32_t *pu32Stride, uint32_t u32Cnt)
{
    uint32_t i;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pu32Stride, VPP_ERR_PARAM);

    if (u32Idx >= pstHvxCore->pstBufferdataIn->numbuffers)
    {
        LOGE("u32Idx=%u but allocated=%u", u32Idx, pstHvxCore->pstBufferdataIn->numbuffers);
        return VPP_ERR_PARAM;
    }

    if (u32Cnt > MAX_NUM_PLANES)
    {
        LOGE("u32Cnt=%u higher than MAX_NUM_PLANES=%u", u32Cnt, MAX_NUM_PLANES);
        return VPP_ERR_PARAM;
    }

    for (i = 0; i < MAX_NUM_PLANES; i++)
    {
        if (i < u32Cnt)
            pstHvxCore->pstBufferdataIn->bufferattributes[u32Idx].plane_stride[i] = pu32Stride[i];
        else
            pstHvxCore->pstBufferdataIn->bufferattributes[u32Idx].plane_stride[i] = 0;
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufInSetAttrUbwc(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32Idx,
                                          t_StVppUbwcStats *pstBufStats)
{
    vpp_svc_ubwc_stats_t *pstUbwcStats;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    if (u32Idx >= pstHvxCore->pstBufferdataIn->numbuffers)
    {
        LOGE("u32Idx=%u but allocated=%u", u32Idx, pstHvxCore->pstBufferdataIn->numbuffers);
        return VPP_ERR_PARAM;
    }

    pstUbwcStats = &pstHvxCore->pstBufferdataIn->bufferattributes[u32Idx].ubwc_stats;

    if (!pstBufStats)
    {
        memset(pstUbwcStats, 0, sizeof(vpp_svc_ubwc_stats_t));
    }
    else
    {
        if (pstBufStats->eVersion != eVppUbwcStatVer_2p0)
            return VPP_OK;

        pstUbwcStats->valid = pstBufStats->bValid ? 1 : 0;
        pstUbwcStats->num_32b_comp = pstBufStats->stats.st2p0.u32Stat32;
        pstUbwcStats->num_64b_comp = pstBufStats->stats.st2p0.u32Stat64;
        pstUbwcStats->num_96b_comp = pstBufStats->stats.st2p0.u32Stat96;
        pstUbwcStats->num_128b_comp = pstBufStats->stats.st2p0.u32Stat128;
        pstUbwcStats->num_160b_comp = pstBufStats->stats.st2p0.u32Stat160;
        pstUbwcStats->num_192b_comp = pstBufStats->stats.st2p0.u32Stat192;
        pstUbwcStats->num_256b_comp = pstBufStats->stats.st2p0.u32Stat256;
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufInSetUserDataLen(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32Idx,
                                             uint32_t u32Size)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    if (u32Idx >= pstHvxCore->pstBufferdataIn->numbuffers)
    {
        LOGE("u32Idx=%u but allocated=%u", u32Idx, pstHvxCore->pstBufferdataIn->numbuffers);
        return VPP_ERR_PARAM;
    }

    pstHvxCore->pstBufferdataIn->pixel_data[u32Idx].user_dataLen = u32Size;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufInSetUserDataAddr(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32Idx,
                                              void* vpAddr)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    if (u32Idx >= pstHvxCore->pstBufferdataIn->numbuffers)
    {
        LOGE("u32Idx=%u but allocated=%u", u32Idx, pstHvxCore->pstBufferdataIn->numbuffers);
        return VPP_ERR_PARAM;
    }

    pstHvxCore->pstBufferdataIn->pixel_data[u32Idx].user_data = vpAddr;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BuffOutInit(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32NumBuffers)
{
    uint32_t u32PixDataAllocSize;
    vpp_svc_frame_group_descriptor_t* pstBufferdataOut;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    if (u32NumBuffers > MAX_BUFFS_IN_GROUP)
    {
        LOGE("Trying to init %u buffers but max is %u", u32NumBuffers, MAX_BUFFS_IN_GROUP);
        return VPP_ERR_PARAM;
    }

    u32PixDataAllocSize = sizeof(vpp_svc_raw_buffer_t) * u32NumBuffers;
    pstBufferdataOut = pstHvxCore->pstBufferdataOut;

    memset((void*)pstBufferdataOut, 0, sizeof(vpp_svc_frame_group_descriptor_t));

    pstBufferdataOut->pixel_data = (vpp_svc_raw_buffer_t*)HVX_ALLOC(u32PixDataAllocSize);
    if (!pstBufferdataOut->pixel_data)
        return VPP_ERR_NO_MEM;

    pstBufferdataOut->numbuffers = u32NumBuffers;
    pstBufferdataOut->pixel_dataLen = u32NumBuffers;

    return VPP_OK;
}

void vVppIpHvxCore_BuffOutTerm(t_StVppIpHvxCoreCb *pstHvxCore)
{
    if ((pstHvxCore) && (pstHvxCore->pstBufferdataOut))
    {
        if (pstHvxCore->pstBufferdataOut->pixel_data)
            free(pstHvxCore->pstBufferdataOut->pixel_data);

        pstHvxCore->pstBufferdataOut->pixel_data = NULL;
    }
}

uint32_t u32VppIpHvxCore_BuffOutCompute(t_StVppIpHvxCoreCb *pstHvxCore)
{
    t_StHvxCoreBufParams *pstBufParams;
    vpp_svc_frame_group_descriptor_t* pstBufferdataOut;
    uint32_t u32FrameSize;
    uint32_t i;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstBufParams = &pstHvxCore->stHvxBufParams;
    pstBufferdataOut = pstHvxCore->pstBufferdataOut;
    u32VppIpHvxCore_BufParamGetPlaneTotalSize(pstHvxCore, &u32FrameSize);

    for (i = 0; i < pstBufferdataOut->numbuffers; i++)
    {
        u32VppIpHvxCore_BufOutSetUserDataLen(pstHvxCore, i, u32FrameSize);
        LOGD("pstBufferdataOut[%d] len=%d", i, u32FrameSize);
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufOutGetAttrUbwc(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32Idx,
                                           t_StVppUbwcStats *pstBufStats)
{
    vpp_svc_ubwc_stats_t *pstUbwcStats;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstBufStats, VPP_ERR_PARAM);

    if (u32Idx >= pstHvxCore->pstBufferdataOut->numbuffers)
    {
        LOGE("u32Idx=%u but allocated=%u", u32Idx, pstHvxCore->pstBufferdataOut->numbuffers);
        return VPP_ERR_PARAM;
    }

    pstUbwcStats = &pstHvxCore->pstBufferdataOut->bufferattributes[u32Idx].ubwc_stats;

    if (u32VppUtils_IsSoc(SDM845))
    {
        pstBufStats->eVersion = eVppUbwcStatVer_2p0;
        pstBufStats->bValid = pstUbwcStats->valid ? VPP_TRUE : VPP_FALSE;
        pstBufStats->stats.st2p0.u32Stat32 = pstUbwcStats->num_32b_comp;
        pstBufStats->stats.st2p0.u32Stat64 = pstUbwcStats->num_64b_comp;
        pstBufStats->stats.st2p0.u32Stat96 = pstUbwcStats->num_96b_comp;
        pstBufStats->stats.st2p0.u32Stat128 = pstUbwcStats->num_128b_comp;
        pstBufStats->stats.st2p0.u32Stat160 = pstUbwcStats->num_160b_comp;
        pstBufStats->stats.st2p0.u32Stat192 = pstUbwcStats->num_192b_comp;
        pstBufStats->stats.st2p0.u32Stat256 = pstUbwcStats->num_256b_comp;
    }
    else
    {
        pstBufStats->eVersion = eVppUbwcStatVer_Unknown;
        pstBufStats->bValid = VPP_FALSE;
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufOutSetNumBuffers(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32NumBuffers)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstHvxCore->pstBufferdataOut->numbuffers = u32NumBuffers;
    pstHvxCore->pstBufferdataOut->pixel_dataLen = u32NumBuffers;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufOutSetUserDataLen(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32Idx,
                                              uint32_t u32Size)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    if (u32Idx >= pstHvxCore->pstBufferdataOut->numbuffers)
    {
        LOGE("u32Idx=%u but allocated=%u", u32Idx, pstHvxCore->pstBufferdataOut->numbuffers);
        return VPP_ERR_PARAM;
    }

    pstHvxCore->pstBufferdataOut->pixel_data[u32Idx].user_dataLen = u32Size;

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_BufOutSetUserDataAddr(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32Idx,
                                               void* vpAddr)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    if (u32Idx >= pstHvxCore->pstBufferdataOut->numbuffers)
    {
        LOGE("u32Idx=%u but allocated=%u", u32Idx, pstHvxCore->pstBufferdataOut->numbuffers);
        return VPP_ERR_PARAM;
    }

    pstHvxCore->pstBufferdataOut->pixel_data[u32Idx].user_data = vpAddr;

    return VPP_OK;
}

uint32_t u32_VppIpHvxCore_GetBufReqs(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32VppProcFlags,
                                     vpp_svc_inout_buf_req_t *pstBufReq)
{
    int rc;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstBufReq, VPP_ERR_PARAM);

    rc = vpp_svc_get_inout_bufs_req(pstHvxCore->hvx_handle, u32VppProcFlags, pstBufReq);

    if (rc != AEE_SUCCESS)
    {
        LOGE("Error getting buffer requirements, rc=%d", rc);
        return VPP_ERR_HW;
    }

    return VPP_OK;
}

int VppIpHvxCore_Process(t_StVppIpHvxCoreCb *pstHvxCore)
{
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    return vpp_svc_process(pstHvxCore->hvx_handle, pstHvxCore->pstParams,
                           (const vpp_svc_frame_group_descriptor_t*)pstHvxCore->pstBufferdataIn,
                           pstHvxCore->pstBufferdataOut);
}

uint32_t u32VppIpHvxCore_AlgoInit(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32Index,
                                  uint32_t u32Algo, const char *cpAlgoLibName)
{
    vpp_svc_cap_resource_list_t* pstCapabilityResources;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    pstCapabilityResources = pstHvxCore->pstCapabilityResources;
    pstCapabilityResources->resource[u32Index].vpp_func_id = u32Algo;
    pstCapabilityResources->resource[u32Index].feature_flags = 1;
    if (cpAlgoLibName)
        strlcpy(pstCapabilityResources->resource[u32Index].name_loc, cpAlgoLibName,
                HVX_LIB_NAME_LEN);

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_SendCtx(t_StVppIpHvxCoreCb *pstHvxCore)
{
    int rc;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_SEND_CTX);
    rc = vpp_svc_send_ctx(pstHvxCore->hvx_handle,
                          pstHvxCore->ctx.stIonCtxBuf.fd_ion_mem,
                          pstHvxCore->ctx.u32Offset,
                          pstHvxCore->ctx.u32Length);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_SEND_CTX);

    if (rc != AEE_SUCCESS)
    {
        LOGE("Error: Send context failed, rc=%d", rc);
        return VPP_ERR_HW;
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_RetrieveCtx(t_StVppIpHvxCoreCb *pstHvxCore)
{
    int rc;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_RETRIEVE_CTX);
    rc = vpp_svc_retrieve_ctx(pstHvxCore->hvx_handle);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_RETRIEVE_CTX);

    if (rc != AEE_SUCCESS)
    {
        LOGE("Error: Retrieve context failed, rc=%d", rc);
        return VPP_ERR_HW;
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_PrepareCtx(t_StVppIpHvxCoreCb *pstHvxCore)
{
    uint32_t u32Ret, u32FrameSizeBytes;
    int rc;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    u32Ret = u32VppIpHvxCore_BufParamGetPlaneTotalSize(pstHvxCore, &u32FrameSizeBytes);
    if (u32Ret != VPP_OK)
    {
        LOGE("Could not get frame size, setting to 0, u32Ret=%u", u32Ret);
        u32FrameSizeBytes = 0;
    }
    pstHvxCore->ctx.stFrameProp.frame_size_bytes = u32FrameSizeBytes;
    pstHvxCore->ctx.stFrameProp.width = pstHvxCore->stHvxBufParams.u32InWidth;
    pstHvxCore->ctx.stFrameProp.height = pstHvxCore->stHvxBufParams.u32InHeight;
    pstHvxCore->ctx.stFrameProp.pixelformat = pstHvxCore->stHvxBufParams.ePixFmt;
    pstHvxCore->ctx.stFrameProp.fieldformat = pstHvxCore->stHvxBufParams.eFieldFmt;

    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_PREPARE_CTX);

    rc = vpp_svc_prepare_ctx(pstHvxCore->hvx_handle,
                             &pstHvxCore->ctx.stFrameProp,
                             pstHvxCore->ctx.u32ProcessingFlags,
                             pstHvxCore->ctx.u32CtxSz,
                             pstHvxCore->ctx.u32DiagCtxSz);

    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_PREPARE_CTX);

    if (rc != AEE_SUCCESS)
    {
        LOGE("Error: context config failed, rc=%d", rc);
        return VPP_ERR_HW;
    }

    return VPP_OK;
}

uint32_t u32VppIpHvxCore_TuningsAlgoRegister(t_StVppIpHvxCoreCb *pstSess,
                                             vpp_svc_vpp_func_id_t eAlgoId)
{
    uint32_t u32Ret = VPP_OK;
    uint32_t i, u32Free = HVX_CORE_TUNE_MAX_SESSIONS;
    t_StVppIpHvxCoreGlobalCb *pstGlobal;

    VPP_RET_IF_NULL(pstSess, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstSess->pstGlobal, VPP_ERR_PARAM);
    if (eAlgoId >= VPP_FUNC_ID_NUM)
    {
        LOGE("eAlgoId=%d >= Max=%d", eAlgoId, VPP_FUNC_ID_NUM);
        return VPP_ERR_PARAM;
    }

    pstGlobal = pstSess->pstGlobal;
    pthread_mutex_lock(&pstGlobal->mutex);

    if (!pstGlobal->apstTunings[eAlgoId])
    {
        LOGE("apstTunings for eAlgoId=%d is NULL", eAlgoId);
        pthread_mutex_unlock(&pstGlobal->mutex);
        return VPP_ERR_PARAM;
    }
    for (i = 0; i < HVX_CORE_TUNE_MAX_SESSIONS; i++)
    {
        if (u32Free == HVX_CORE_TUNE_MAX_SESSIONS &&
            pstGlobal->apstTunings[eAlgoId]->stSession.apstSessId[i] == NULL)
        {
            u32Free = i;
            continue;
        }
        else if (pstGlobal->apstTunings[eAlgoId]->stSession.apstSessId[i] == pstSess)
        {
            u32Ret = VPP_ERR_RESOURCES;
            LOGE("Session has already been registered at i=%u", i);
            break;
        }
    }

    if (u32Ret == VPP_OK && u32Free < HVX_CORE_TUNE_MAX_SESSIONS)
    {
        pstGlobal->apstTunings[eAlgoId]->stSession.apstSessId[u32Free] = pstSess;
        pstGlobal->apstTunings[eAlgoId]->stSession.u32Cnt += 1;
        LOGI("Registered session, at i=%u, pstSess=%p, u32Cnt=%u",
             u32Free, pstSess, pstGlobal->apstTunings[eAlgoId]->stSession.u32Cnt);
    }
    else
    {
        u32Ret = VPP_ERR_RESOURCES;
        LOGE_IF(u32Free >= HVX_CORE_TUNE_MAX_SESSIONS,
                "Can't register session, max sessions (%d) reached",
                HVX_CORE_TUNE_MAX_SESSIONS);
    }
    pthread_mutex_unlock(&pstGlobal->mutex);

    return u32Ret;
}

uint32_t u32VppIpHvxCore_TuningsAlgoUnregister(t_StVppIpHvxCoreCb *pstSess,
                                               vpp_svc_vpp_func_id_t eAlgoId)
{
    uint32_t i;
    uint32_t u32Ret = VPP_OK;
    t_StVppIpHvxCoreGlobalCb *pstGlobal;

    VPP_RET_IF_NULL(pstSess, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstSess->pstGlobal, VPP_ERR_PARAM);
    if (eAlgoId >= VPP_FUNC_ID_NUM)
    {
        LOGE("eAlgoId=%d >= Max=%d", eAlgoId, VPP_FUNC_ID_NUM);
        return VPP_ERR_PARAM;
    }

    pstGlobal = pstSess->pstGlobal;
    pthread_mutex_lock(&pstGlobal->mutex);

    if (!pstGlobal->apstTunings[eAlgoId])
    {
        LOGE("apstTunings for eAlgoId=%d is NULL", eAlgoId);
        pthread_mutex_unlock(&pstGlobal->mutex);
        return VPP_ERR_PARAM;
    }
    for (i = 0; i < HVX_CORE_TUNE_MAX_SESSIONS; i++)
    {
        if (pstGlobal->apstTunings[eAlgoId]->stSession.apstSessId[i] == pstSess)
        {
            pstGlobal->apstTunings[eAlgoId]->stSession.apstSessId[i] = NULL;
            pstGlobal->apstTunings[eAlgoId]->stSession.u32Cnt -= 1;
            LOGI("Unregistered session at i=%u, pstSess=%p u32Cnt=%u",
                 i, pstSess, pstGlobal->apstTunings[eAlgoId]->stSession.u32Cnt);
            break;
        }
    }
    pthread_mutex_unlock(&pstGlobal->mutex);

    if (i == HVX_CORE_TUNE_MAX_SESSIONS)
    {
        LOGE("pstSess=%p not found! Can't unregister", pstSess);
        u32Ret = VPP_ERR_STATE;
    }

    return u32Ret;
}

uint32_t u32VppIpHvxCore_TuningsAlgoLoad(t_StVppIpHvxCoreCb *pstHvxCore,
                                         vpp_svc_vpp_func_id_t eAlgoId)
{
    uint32_t u32Size, u32Err;
    uint32_t u32Ret = VPP_OK;
    t_StHvxCoreTunings *pstTunings;
    t_StVppIpHvxCoreGlobalCb *pstGlobal;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstHvxCore->pstGlobal, VPP_ERR_PARAM);
    if (eAlgoId >= VPP_FUNC_ID_NUM)
    {
        LOGE("eAlgoId=%d >= Max=%d", eAlgoId, VPP_FUNC_ID_NUM);
        return VPP_ERR_PARAM;
    }

    pstGlobal = pstHvxCore->pstGlobal;
    pthread_mutex_lock(&pstHvxCore->pstGlobal->mutex);
    if (!pstGlobal->apstTunings[eAlgoId])
    {
        LOGE("apstTunings for eAlgoId=%d is NULL", eAlgoId);
        pthread_mutex_unlock(&pstGlobal->mutex);
        return VPP_ERR_PARAM;
    }
    pstTunings = pstGlobal->apstTunings[eAlgoId];

    u32Size = u32VppIpHvxCore_GetTuningsBufferSize(pstTunings);
    if (pstTunings->pvTuningBlock && u32Size)
    {
        u32Err = u32VppIon_Alloc(pstHvxCore->stBase.pstCtx, u32Size, VPP_FALSE,
                                 &pstTunings->stTuningBuf);
        if (u32Err != VPP_OK)
        {
            LOGE("failed to allocate ion memory for tuning buffer u32Err=%u", u32Err);
            pthread_mutex_unlock(&pstHvxCore->pstGlobal->mutex);
            return VPP_ERR_NO_MEM;
        }

        u32Err = u32VppIpHvxCore_TuningBufferPack(pstTunings);
        if (u32Err != VPP_OK)
        {
            LOGE("failed to pack tuning buffer. u32Err=%u", u32Err);
            u32Ret = VPP_ERR;
        }
        else
        {
            u32Err = u32VppIpHvxCore_TuningsSend(pstHvxCore, eAlgoId,
                                                 (uint32_t *)pstTunings->stTuningBuf.buf,
                                                 u32Size);
            if (u32Err != VPP_OK)
            {
                LOGE("failed to send tunings. u32Err=%u", u32Err);
                u32Ret = VPP_ERR;
            }
        }

        u32Err = u32VppIon_Free(pstHvxCore->stBase.pstCtx, &pstTunings->stTuningBuf);
        if (u32Err)
        {
            LOGE("failed to free ion memory. u32Err=%u", u32Err);
            u32Ret = VPP_ERR;
        }
    }

    pthread_mutex_unlock(&pstHvxCore->pstGlobal->mutex);
    return u32Ret;
}

uint32_t u32VppIpHvxCore_TuningsBoot(t_StVppIpHvxCoreCb *pstHvxCore, const char *pcFileName,
                                     uint32_t u32VppProcFlags, uint32_t bForceInit)
{
    uint32_t u32Ret;
    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pcFileName, VPP_ERR_PARAM);
    if (!u32VppProcFlags)
    {
        LOGD("No processing flags set, nothing to boot");
        return VPP_OK;
    }
    LOG_ENTER();

    u32Ret = u32VppIpHvxCore_TuningsBootInternal(pstHvxCore, pcFileName, u32VppProcFlags,
                                                 VPP_FALSE, bForceInit, VPP_FALSE);
    LOGE_IF(u32Ret != VPP_OK, "Tunings boot failed, u32Ret=%u", u32Ret);
    LOG_EXIT_RET(u32Ret);
}

uint32_t u32VppIpHvxCore_TuningsShutdown(t_StVppIpHvxCoreCb *pstHvxCore, uint32_t u32VppProcFlags)
{
    uint32_t u32Ret;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);
    if (!u32VppProcFlags)
    {
        LOGD("No processing flags set, nothing to shutdown");
        return VPP_OK;
    }
    LOG_ENTER();

    u32Ret = u32VppIpHvxCore_TuningsShutdownInternal(pstHvxCore, u32VppProcFlags,
                                                     VPP_FALSE, VPP_FALSE);
    LOGE_IF(u32Ret != VPP_OK, "Tunings shutdown failed, u32Ret=%u", u32Ret);
    LOG_EXIT_RET(u32Ret);
}

uint32_t u32VppIpHvxCore_Boot(const char *pcTuningsFileName, uint32_t u32VppProcFlags)
{
    uint32_t u32Ret = VPP_OK;
    t_StVppIpHvxCoreCb stHvxCore;
    t_StVppIpHvxCoreCb *pstHvxCore = &stHvxCore;
    t_StVppIpHvxCoreGlobalCb *pstGlobal = &stGlobalCb;

    VPP_RET_IF_NULL(pcTuningsFileName, VPP_ERR_PARAM);
    if (!u32VppProcFlags)
    {
        LOGD("No processing flags set, nothing to boot");
        return VPP_OK;
    }
    LOG_ENTER();

    memset(&stHvxCore, 0, sizeof(t_StVppIpHvxCoreCb));
    pstHvxCore->pstGlobal = pstGlobal;

    vVppIpHvxCore_GlobalMutexWaitStateSet(HVX_CORE_GLOBAL_STATE_BOOT);

    u32Ret = u32VppIpHvxCore_FirmwareSessionOpen(pstHvxCore, u32VppProcFlags);
    if (u32Ret != VPP_OK)
    {
        LOGE("Error opening firmware session, u32Ret=%u", u32Ret);
        goto ERR_GET_HANDLE;
    }

    u32Ret = u32VppIpHvxCore_TuningsBootInternal(pstHvxCore, pcTuningsFileName, u32VppProcFlags,
                                                 VPP_TRUE, VPP_TRUE, VPP_TRUE);
    LOGE_IF(u32Ret != VPP_OK, "Could not boot tunings, u32Ret=%u", u32Ret);
    vVppIpHvxCore_FirmwareSessionClose(pstHvxCore);

ERR_GET_HANDLE:
    pthread_mutex_lock(&pstGlobal->mutex);
    pstGlobal->eState = HVX_CORE_GLOBAL_STATE_READY;
    pthread_mutex_unlock(&pstGlobal->mutex);
    pthread_cond_broadcast(&pstGlobal->cond);

    LOG_EXIT_RET(u32Ret);
}

uint32_t u32VppIpHvxCore_Shutdown(uint32_t u32VppProcFlags)
{
    uint32_t u32Ret;
    t_StVppIpHvxCoreCb stHvxCore;
    t_StVppIpHvxCoreCb *pstHvxCore = &stHvxCore;
    t_StVppIpHvxCoreGlobalCb *pstGlobal = &stGlobalCb;

    if (!u32VppProcFlags)
    {
        LOGD("No processing flags set, nothing to shutdown");
        return VPP_OK;
    }
    LOG_ENTER();

    memset(&stHvxCore, 0, sizeof(t_StVppIpHvxCoreCb));
    pstHvxCore->pstGlobal = pstGlobal;

    vVppIpHvxCore_GlobalMutexWaitStateSet(HVX_CORE_GLOBAL_STATE_SHUTDOWN);

    u32Ret = u32VppIpHvxCore_TuningsShutdownInternal(pstHvxCore, u32VppProcFlags,
                                                     VPP_TRUE, VPP_TRUE);
    LOGD_IF(u32Ret != VPP_OK, "Could not shutdown tunings, u32Err=%u", u32Ret);

    pthread_mutex_lock(&pstGlobal->mutex);
    pstGlobal->eState = HVX_CORE_GLOBAL_STATE_READY;
    pthread_mutex_unlock(&pstGlobal->mutex);
    pthread_cond_broadcast(&pstGlobal->cond);

    LOG_EXIT_RET(u32Ret);
}

uint32_t u32VppIpHvxCore_Open(t_StVppIpHvxCoreCb *pstHvxCore,
                              uint32_t u32VppProcFlags,
                              uint32_t u32FrameSizeBytes)
{
    int rc;
    uint32_t u32Ret = VPP_OK;
    uint32_t u32;
    uint32_t u32RequiredLength;
    uint32_t u32VppCtxSize = 0;
    const char *pcUri;
    vpp_svc_scratch_buf_req_t stScratchReq;

    vpp_svc_cap_resource_list_t* pstCapabilityResources;

    LOG_ENTER();

    if (!pstHvxCore || !pstHvxCore->stBase.pstCtx)
        return VPP_ERR_PARAM;

    pcUri = vpp_svc_URI CDSP_DOMAIN;
    if (pstHvxCore->stBase.bSecureSession)
        pcUri = vpp_svc_URI CDSP_DOMAIN VPP_HVX_SECURE_SESSION;

    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_CORE_OPEN);

    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_OPEN);
    rc = vpp_svc_open(pcUri, &pstHvxCore->hvx_handle);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_OPEN);

    if ((rc != AEE_SUCCESS) || (pstHvxCore->hvx_handle == HVX_HANDLE_INVALID))
    {
        LOGE("ERROR: vpp_svc_open failed! rc=%d, handle=0x%"PRIx64, rc, pstHvxCore->hvx_handle);
        u32Ret = VPP_ERR_HW;
        goto ERROR_SVC_OPEN;
    }
    LOGD("vpp_svc_open successful: handle=0x%"PRIx64, pstHvxCore->hvx_handle);

    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_SET_DBG_LVL);
    vVppIpHvxCore_SetDebugLevels(pstHvxCore);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_SET_DBG_LVL);

    LOGD("flags=0x%x", u32VppProcFlags);

    pstCapabilityResources = pstHvxCore->pstCapabilityResources;

    LOGI("calling vpp_svc_init");

    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_INIT);
    rc = vpp_svc_init(pstHvxCore->hvx_handle, pstCapabilityResources);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_INIT);
    if (rc != AEE_SUCCESS)
    {
        LOGE("vpp_svc_init() error, rc=%d", rc);
        u32Ret = VPP_ERR_HW;
        goto ERR_VPP_SVC_INIT;
    }
    LOGI("vpp_svc_init() successful");

    // Get the version information from the firmware. Must be called after vpp_svc_init()!
    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_GET_VER_INFO);
    vVppIpHvxCore_GetVersionInfo(pstHvxCore);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_GET_VER_INFO);

    // Calculate context size and allocate context
    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_GET_CTX);
    rc = vpp_svc_get_ctx_size(pstHvxCore->hvx_handle, u32VppProcFlags,
                              u32FrameSizeBytes, &u32VppCtxSize);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_GET_CTX);

    if (rc != AEE_SUCCESS)
    {
        LOGE("get_ctx_sz error, rc=%d ctx_sz=%u", rc, u32VppCtxSize);
        u32Ret = VPP_ERR_HW;
        goto ERR_GET_CTX_SZ;
    }

    u32RequiredLength = (u32VppCtxSize + 127) & (~127);

    LOGD("u32FrameSizeBytes=%d, u32VppCtxSize=%d, u32RequiredLength=%u",
         u32FrameSizeBytes, u32VppCtxSize, u32RequiredLength);

    // if needs to allocate ion buffer
    if (pstHvxCore->ctx.bAllocated &&
        pstHvxCore->ctx.u32AllocatedSize < u32RequiredLength)
    {
        LOGD("freeing previously allocated ctx (sz=%u) for new ctx (sz=%u)",
             pstHvxCore->ctx.u32AllocatedSize, u32RequiredLength);
        u32VppIpHvxCore_FreeContext(pstHvxCore);
    }

    if (!pstHvxCore->ctx.bAllocated)
    {
        u32 = u32VppIpHvxCore_AllocateContext(pstHvxCore, u32RequiredLength);
        if (u32 != VPP_OK)
        {
            LOGE("ERROR: AllocateContext failed, u32=%u", u32);
            u32Ret = VPP_ERR_NO_MEM;
            goto ERR_ALLOC;
        }
    }

    pstHvxCore->ctx.u32Length = u32RequiredLength;
    pstHvxCore->ctx.u32ProcessingFlags = u32VppProcFlags;
    pstHvxCore->ctx.u32CtxSz = u32VppCtxSize;
    pstHvxCore->ctx.u32DiagCtxSz = 0;

    //Send context
    u32 = u32VppIpHvxCore_SendCtx(pstHvxCore);
    if (u32 != VPP_OK)
    {
        LOGE("Error: context send failed, u32=%u", u32);
        u32Ret = VPP_ERR_HW;
        goto ERR_SEND_CTX;
    }

    rc = vpp_svc_get_scratch_bufs_req(pstHvxCore->hvx_handle, u32VppProcFlags,
                                      &stScratchReq);
    if (rc != AEE_SUCCESS)
    {
        LOGE("vpp_svc_get_scratch_bufs_req error, rc=%d", rc);
        u32Ret = VPP_ERR_HW;
        goto ERR_GET_SCRATCH_REQ;
    }

    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_ALLOC_SCRATCH);
    u32Ret = u32VppIpHvxCore_AllocScratchBufs(pstHvxCore, &stScratchReq);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_ALLOC_SCRATCH);
    if (u32Ret != VPP_OK)
    {
        LOGE("allocating scratch buffers failed, u32Ret=%u", u32Ret);
        u32Ret = VPP_ERR_HW;
        goto ERR_VPP_ALLOC_SCRATCH;
    }

    u32Ret = u32VppIpHvxCore_SendScratchBufs(pstHvxCore, u32VppProcFlags);
    if (u32Ret != VPP_OK)
    {
        LOGE("sending scratch buffers to FW failed, u32Ret=%u", u32Ret);
        u32Ret = VPP_ERR_HW;
        goto ERR_VPP_SEND_SCRATCH;
    }

    u32Ret = u32VppIpHvxCore_HandleSessionMigration(pstHvxCore);
    if (u32Ret != VPP_OK)
    {
        LOGE("session migration failed, u32Ret=%u", u32Ret);
        u32Ret = VPP_ERR_HW;
        goto ERR_VPP_MIGRATE_SESSION;
    }

    //Prepare context for use
    u32 = u32VppIpHvxCore_PrepareCtx(pstHvxCore);
    if (u32 != VPP_OK)
    {
        LOGE("Error: context config failed, u32=%u", u32);
        u32Ret = VPP_ERR_HW;
        goto ERR_PREPARE_CTX;
    }

    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_CORE_OPEN);
    return VPP_OK;

ERR_PREPARE_CTX:
ERR_VPP_MIGRATE_SESSION:
    u32VppIpHvxCore_RetrieveScratchBufs(pstHvxCore);

ERR_VPP_SEND_SCRATCH:
ERR_VPP_ALLOC_SCRATCH:
    vVppIpHvxCore_FreeScratchBufs(pstHvxCore);

ERR_GET_SCRATCH_REQ:
    u32VppIpHvxCore_RetrieveCtx(pstHvxCore);

ERR_SEND_CTX:
    u32VppIpHvxCore_FreeContext(pstHvxCore);

ERR_ALLOC:
ERR_GET_CTX_SZ:
    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_DEINIT);
    rc = vpp_svc_deinit(pstHvxCore->hvx_handle, pstCapabilityResources);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_DEINIT);
    LOGE_IF(rc != AEE_SUCCESS, "vpp_svc_deinit() failed, rc=%d", rc);

ERR_VPP_SVC_INIT:
    rc = vpp_svc_close(pstHvxCore->hvx_handle);
    LOGE_IF(rc != AEE_SUCCESS, "ERROR: vpp_svc_close failed rc=%d", rc);

ERROR_SVC_OPEN:
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_CORE_OPEN);

    return u32Ret;
}

uint32_t u32VppIpHvxCore_Close(t_StVppIpHvxCoreCb *pstHvxCore)
{
    uint32_t u32;
    uint32_t u32Ret = VPP_OK;
    int rc;
    vpp_svc_cap_resource_list_t* pstCapabilityResources;

    LOG_ENTER();

    if (!pstHvxCore || ! pstHvxCore->stBase.pstCtx)
        return VPP_ERR_PARAM;

    u32 = u32VppIpHvxCore_RetrieveScratchBufs(pstHvxCore);
    if (u32 != VPP_OK)
    {
        LOGE("ERROR: unable to retreive scratch bufs, u32=%u", u32);
        u32Ret = VPP_ERR_HW;
    }
    vVppIpHvxCore_FreeScratchBufs(pstHvxCore);

    u32 = u32VppIpHvxCore_RetrieveCtx(pstHvxCore);
    if (u32 != VPP_OK)
    {
        LOGE("ERROR: unable to retreive context, u32=%u", u32);
        u32Ret = VPP_ERR_HW;
    }

    pstCapabilityResources = pstHvxCore->pstCapabilityResources;
    rc = vpp_svc_deinit(pstHvxCore->hvx_handle, pstCapabilityResources);
    if (rc != AEE_SUCCESS)
    {
        LOGE("Error: HVX close failed rc=%d", rc);
        u32Ret = VPP_ERR_HW;
    }

    VPP_IP_PROF_START(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_CLOSE);
    rc = vpp_svc_close(pstHvxCore->hvx_handle);
    VPP_IP_PROF_STOP(&pstHvxCore->stBase, HVX_CORE_STAT_SVC_CLOSE);
    if (rc != AEE_SUCCESS)
    {
        LOGE("ERROR: vpp_svc_close failed rc=%d", rc);
        u32Ret = VPP_ERR_HW;
    }

    return u32Ret;
}

t_StVppIpHvxCoreCb *pstVppIpHvxCore_Init(t_StVppCtx *pstCtx, uint32_t u32Flags,
                                         uint32_t u32CtrlCnt, uint32_t u32UserDataSize)
{
    uint32_t u32;
    t_StVppCallback cbs;
    t_StVppIpHvxCoreCb *pstHvxCore = NULL;
    uint32_t u32Length;
    t_StVppIpHvxCoreGlobalCb *pstGlobal = &stGlobalCb;

    LOG_ENTER();

    VPP_RET_IF_NULL(pstCtx, NULL);

    memset(&cbs, 0, sizeof(t_StVppCallback));

    pstHvxCore = calloc(sizeof(t_StVppIpHvxCoreCb), 1);
    if (!pstHvxCore)
    {
        LOGE("calloc failed for hvx core");
        return NULL;
    }
    pstHvxCore->pstGlobal = pstGlobal;
    u32VppIp_SetBase(pstCtx, u32Flags, cbs, &pstHvxCore->stBase);

    u32 = VPP_IP_PROF_REGISTER(&pstHvxCore->stBase, astHvxCoreStatsCfg, u32HvxCoreStatCnt);
    LOGE_IF(u32 != VPP_OK, "ERROR: unable to register stats, u32=%u", u32);

    vVppIpHvxCore_ReadProperties(pstHvxCore);

    pstHvxCore->pstCapabilityResources = (vpp_svc_cap_resource_list_t *)
        HVX_ALLOC(sizeof(vpp_svc_cap_resource_list_t));
    if (pstHvxCore->pstCapabilityResources == NULL)
    {
        LOGE("Error: pstCapabilityResources alloc failed");
        goto ERROR_CAPABILITY;
    }

    pstHvxCore->pstCapabilityResources->resourceLen = u32CtrlCnt;
    u32Length = sizeof(vpp_svc_cap_resource_t) * u32CtrlCnt;
    pstHvxCore->pstCapabilityResources->resource =
        (vpp_svc_cap_resource_t*)HVX_ALLOC(u32Length);
    if (pstHvxCore->pstCapabilityResources->resource == NULL)
    {
        LOGE("Error: pstCapabilityResources->resource alloc faild");
        goto ERROR_CAPABILITY_RES;
    }

    pstHvxCore->pstBufferdataIn = (vpp_svc_frame_group_descriptor_t*)
        HVX_ALLOC(sizeof(vpp_svc_frame_group_descriptor_t));
    if (pstHvxCore->pstBufferdataIn == NULL)
    {
        LOGE("Error: pstBufferdataIn alloc failed");
        goto ERROR_BUFFER_IN;
    }

    pstHvxCore->pstBufferdataOut = (vpp_svc_frame_group_descriptor_t*)
        HVX_ALLOC(sizeof(vpp_svc_frame_group_descriptor_t));
    if (pstHvxCore->pstBufferdataOut == NULL)
    {
        LOGE("Error: pstBufferdataOut alloc failed");
        goto ERROR_BUFFER_OUT;
    }

    pstHvxCore->pstParams = (vpp_svc_params_t*)HVX_ALLOC(sizeof(vpp_svc_params_t));
    if (pstHvxCore->pstParams == NULL)
    {
        LOGE("Error: pstHvxCore->pstParams alloc failed");
        goto ERROR_PARAMS;
    }

    u32Length = sizeof(vpp_svc_config_hdr_t) * u32CtrlCnt;
    pstHvxCore->pstParams->header = (vpp_svc_config_hdr_t*)HVX_ALLOC(u32Length);
    if (pstHvxCore->pstParams->header == NULL)
    {
        LOGE("Error: pstParams->header alloc failed");
        goto ERROR_HEADER;
    }

    pstHvxCore->pstParams->user_data = (unsigned char*)HVX_ALLOC(u32UserDataSize);
    if (pstHvxCore->pstParams->user_data == NULL)
    {
        LOGE("Error: alloc pstParams->user_data failed");
        goto ERROR_USERDATA;
    }

    return pstHvxCore;

ERROR_USERDATA:
    if (pstHvxCore->pstParams->header)
        free(pstHvxCore->pstParams->header);

ERROR_HEADER:
    if (pstHvxCore->pstParams)
        free(pstHvxCore->pstParams);

ERROR_PARAMS:
    if (pstHvxCore->pstBufferdataOut)
        free(pstHvxCore->pstBufferdataOut);

ERROR_BUFFER_OUT:
    if (pstHvxCore->pstBufferdataIn)
        free(pstHvxCore->pstBufferdataIn);

ERROR_BUFFER_IN:
    if (pstHvxCore->pstCapabilityResources->resource)
        free(pstHvxCore->pstCapabilityResources->resource);

ERROR_CAPABILITY_RES:
    if (pstHvxCore->pstCapabilityResources)
        free(pstHvxCore->pstCapabilityResources);

ERROR_CAPABILITY:
    u32 = VPP_IP_PROF_UNREGISTER(&pstHvxCore->stBase);
    LOGE_IF(u32 != VPP_OK, "ERROR: unable to unregister stats, u32=%u", u32);

    if (pstHvxCore)
        free(pstHvxCore);

    return NULL;
}

void vVppIpHvxCore_Term(t_StVppIpHvxCoreCb *pstHvxCore)
{
    uint32_t u32;

    LOG_ENTER();

    VPP_RET_VOID_IF_NULL(pstHvxCore);

    u32 = u32VppIpHvxCore_FreeContext(pstHvxCore);
    LOGE_IF(u32 != VPP_OK, "ERROR: unable to free context, u32=%u", u32);

    u32 = VPP_IP_PROF_UNREGISTER(&pstHvxCore->stBase);
    LOGE_IF(u32 != VPP_OK, "ERROR: unable to unregister stats, u32=%u", u32);

    if (pstHvxCore->pstBufferdataOut)
    {
        free(pstHvxCore->pstBufferdataOut);
        pstHvxCore->pstBufferdataOut = NULL;
    }

    if (pstHvxCore->pstBufferdataIn)
    {
        free(pstHvxCore->pstBufferdataIn);
        pstHvxCore->pstBufferdataIn = NULL;
    }

    if (pstHvxCore->pstParams)
    {
        if (pstHvxCore->pstParams->user_data)
            free(pstHvxCore->pstParams->user_data);

        if (pstHvxCore->pstParams->header)
            free(pstHvxCore->pstParams->header);

        free(pstHvxCore->pstParams);
        pstHvxCore->pstParams = NULL;
    }

    if (pstHvxCore->pstCapabilityResources)
    {
        if (pstHvxCore->pstCapabilityResources->resource)
            free(pstHvxCore->pstCapabilityResources->resource);
        free(pstHvxCore->pstCapabilityResources);
        pstHvxCore->pstCapabilityResources = NULL;
    }

    free(pstHvxCore);
}

uint32_t u32VppIpHvxCore_SetClock(t_StVppIpHvxCoreCb *pstHvxCore,
                                  t_EVppHvxCoreClock eClk)
{
    int rc = 0;
    uint8_t u8Clk;

    VPP_RET_IF_NULL(pstHvxCore, VPP_ERR_PARAM);

    switch (eClk)
    {
        case HVX_CORE_CLK_TURBO:
            u8Clk = DCVS_VCORNER_TURBO;
            break;
        case HVX_CORE_CLK_FW_DEFAULT:
        default:
            u8Clk = DCVS_VCORNER_DISABLE;
            break;
    }

    rc = vpp_svc_set_config(pstHvxCore->hvx_handle, CFG_TYPE_VCORNER,
                            (unsigned char *)&u8Clk, 1);
    if (rc != 0)
    {
        LOGE("Unable to set HVX clock, u8Clk=%u, rc=%d", u8Clk, rc);
        return VPP_ERR_HW;
    }

    return VPP_OK;
}

uint32_t bVppIpHvxCore_IsAlgoSupported(vpp_svc_vpp_func_id_t eAlgoId)
{
    uint32_t bRet = VPP_FALSE;

    switch (eAlgoId)
    {
        case VPP_FUNC_ID_MVP:
#if defined(MVP_ENABLED) && (MVP_ENABLED == 1)
            bRet = VPP_TRUE;
#endif
            break;

        case VPP_FUNC_ID_NR:
#if defined(NR_ENABLED) && (NR_ENABLED == 1)
            bRet = VPP_TRUE;
#endif
            break;

        case VPP_FUNC_ID_IE:
#if defined(AIE_ENABLED) && (AIE_ENABLED == 1)
            bRet = VPP_TRUE;
#endif
            break;

        case VPP_FUNC_ID_FRC:
#if defined(FRC_ENABLED) && (FRC_ENABLED == 1)
            bRet = VPP_TRUE;
#endif
            break;

        default:
            LOGD("Func ID not supported %d", eAlgoId);
    }
    return bRet;
}
