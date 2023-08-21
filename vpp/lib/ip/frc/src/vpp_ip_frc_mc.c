/*!
 * @file vpp_ip_frc_mc.c
 *
 * @cr
 * Copyright (c) 2015-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 *
 *
 */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <inttypes.h>

#include <media/msm_vidc_utils.h>

#include "AEEStdErr.h"

#include "vpp.h"
#include "vpp_buf.h"
#include "vpp_callback.h"
#include "vpp_utils.h"
#include "vpp_tunings.h"
#include "vpp_ip_frc_mc.h"
#include "vpp_stats.h"
#ifdef VPP_TARGET_USES_C2D
#include "vpp_ip_c2d.h"
#endif
#define VPP_LOG_TAG     VPP_LOG_MODULE_FRC_MC_TAG
#define VPP_LOG_MODULE  VPP_LOG_MODULE_FRC_MC
#include "vpp_dbg.h"
 #include "hvx_debug.h"

/************************************************************************
 * Local definitions
 ************************************************************************/
// #define DUMP_PROCESSING_PARAMS

//#define FRCMC_FORCE_DUMP_BUF
#define FRCMC_DUMP_BUF_IN_NM            "frcmc_%p_in.yuv"
#define FRCMC_DUMP_BUF_OUT_NM           "frcmc_%p_out.yuv"
#define FRCMC_TUNINGS_FILE_NAME         "/mnt/vendor/persist/vpp/tunings.txt"

// If controls result in bypass, keeping ME running results in lower startup lag but higher power
#define FRCMC_KEEP_ME_RUNNING           0

#ifdef FRCMC_LOG_FRAMEDESC
#define FRCMC_DUMP_FRAME_DESC(pstCb, pstBuf, str, port) vVppIpFrcMc_DumpFrameDesc(pstCb, pstBuf, str, port)
#else
#define FRCMC_DUMP_FRAME_DESC(pstCb, pstBuf, str, port)
#endif
#define FRCMC_CBLOG(pstCb, buf, logid) if (buf) vVppIpCbLog(pstCb, buf, logid);
#define FRCMC_REG_BUF(pstCb, buf) \
    if (buf) \
    { \
        vVppIpHvxCore_RegisterBuffer(pstCb, &(buf)->stPixel); \
    }
#define FRCMC_UNREG_BUF(pstCb, buf) \
    if (buf && VPP_FLAG_IS_SET(buf->stPixel.u32IntBufFlags, VPP_BUF_FLAG_HVX_REGISTERED)) \
    { \
        vVppIpHvxCore_UnregisterBuffer(pstCb, &(buf)->stPixel); \
    }
//When a session has updated global parameters using set_custom_ctrl,
//other running sessions also need to be informed and update global parameters
#define MAX_SESSIONS                    32
#define FRCMC_PORT_PARAM_WIDTH_MAX      RES_FHD_MAX_W
#define FRCMC_PORT_PARAM_WIDTH_MIN      324
#define FRCMC_PORT_PARAM_HEIGHT_MAX     RES_FHD_MAX_H
#define FRCMC_PORT_PARAM_HEIGHT_MIN     128

#define FRCMC_MC_QUALITY_DEFAULT        4
#define FRCMC_INTERP_FRAMES_DEFAULT     1
#define FRCMC_INTERP_FRAMES_MAX         3
#define FRCMC_CTRL_SEG_CNT_DEFAULT      42
// #define VPP_FRCMC_DEBUG_MBI

enum
{
    FRCMC_BLOCK_MC,
    FRCMC_BLOCK_MAX,
};

enum {
    FRCMC_STAT_PROC,
    FRCMC_STAT_WORKER,
    FRCMC_STAT_WORKER_SLEEP,
#ifdef VPP_TARGET_USES_C2D
    FRCMC_STAT_C2D_IN_PROC,
    FRCMC_STAT_C2D_OUT_PROC,
#endif
    FRCMC_STAT_TOTAL_PROC,
};

/************************************************************************
 * Local static variables
 ************************************************************************/
static t_StCustomFrcMcParams stGlobalFrcMcParams =
{
    .mode = FRC_MODE_HFR,
    .mc_quality = FRCMC_MC_QUALITY_DEFAULT,
    .NUM_INTERP_FRAMES = FRCMC_INTERP_FRAMES_DEFAULT,
    .interp_cnt = 0,
    .RepeatMode_repeatPeriod = 5,
    .TH_MOTION = 60,
    .TH_MOTION_LOW = 60,
    .TH_MVOUTLIER_COUNT = 30,
    .TH_MVOUTLIER_COUNT_LOW = 25,
    .TH_OCCLUSION = 10,
    .TH_OCCLUSION_LOW = 10,
    .TH_MOTION00 = 63,
    .TH_MOTION00_LOW = 63,
    .TH_MVOUTLIER_VARIANCE_COUNT = 30,
    .TH_MVOUTLIER_VARIANCE_COUNT_LOW = 25,
    .TH_SCENECUT = 75,
    .TH_VARIANCE = 1,
    .TH_SAD_FR_RATIO = 9,
};

static const t_StVppStatsConfig astFrcMcStatsCfg[] = {
    VPP_PROF_DECL(FRCMC_STAT_PROC, 10, 1),
    VPP_PROF_DECL(FRCMC_STAT_WORKER, 1, 0),
    VPP_PROF_DECL(FRCMC_STAT_WORKER_SLEEP, 1, 0),
#ifdef VPP_TARGET_USES_C2D
    VPP_PROF_DECL(FRCMC_STAT_C2D_IN_PROC, 10, 1),
    VPP_PROF_DECL(FRCMC_STAT_C2D_OUT_PROC, 10, 1),
#endif
    VPP_PROF_DECL(FRCMC_STAT_TOTAL_PROC, 10, 1),
};

static const uint32_t u32FrcMcStatCnt = VPP_STATS_CNT(astFrcMcStatsCfg);

static const uint32_t au32FrcMcMaxSloMoOutRate[VPP_RESOLUTION_MAX] = {33, 33, 33, 0};
static const uint32_t au32FrcMcMaxSloMoInterp[VPP_RESOLUTION_MAX] = {4, 4, 2, 1};

/************************************************************************
 * Forward Declarations
 *************************************************************************/
static uint32_t u32VppIpFrcMc_CmdPut(t_StVppIpFrcMcCb *pstCb, t_StVppIpCmd stCmd);
static void vVppIpFrcMc_InitParam(t_StVppIpFrcMcCb *pstCb);

/************************************************************************
 * Local Functions
 ************************************************************************/
#ifdef FRCMC_DUMP_FRAME_ENABLE
#define FRCMC_DUMP_NM_LEN 256
void vVppIpFrcMc_DumpFrame(t_StVppIpFrcMcCb *pstCb, t_StVppBuf *pstBuf,
                           enum vpp_port port)
{
    char cPath[FRCMC_DUMP_NM_LEN];
    struct vpp_port_param *pstParam;

    if (bVppIpHvxCore_IsSecure(pstCb->pstHvxCore))
        return;

    if (port == VPP_PORT_INPUT)
    {
        snprintf(cPath, FRCMC_DUMP_NM_LEN, FRCMC_DUMP_BUF_IN_NM, pstCb);
        pstParam = &pstCb->stInput.stParam;
    }
    else
    {
        snprintf(cPath, FRCMC_DUMP_NM_LEN, FRCMC_DUMP_BUF_OUT_NM, pstCb);
        pstParam = &pstCb->stOutput.stParam;
    }

    u32VppBuf_Dump(pstCb->stBase.pstCtx, pstBuf, *pstParam, cPath);
}
#endif

void vVppIpFrcMc_DumpFrameDesc(t_StVppIpFrcMcCb *pstCb, t_StVppBuf *pstBuf, char *str,
                               enum vpp_port port)
{
    struct vpp_port_param *pstParam;

    if (port == VPP_PORT_INPUT)
    {
        pstParam = &pstCb->stInput.stParam;
    }
    else
    {
        pstParam = &pstCb->stOutput.stParam;
    }

    LOGI("%s, fmt=%u, w=%u, h=%u, stride=%u, "
         "fd=%d, pvPa=%p, pvBase=%p, uvOff=%u, sz=%u",
         str, pstParam->fmt, pstParam->width, pstParam->height,
         u32VppUtils_GetStride(pstParam), pstBuf->stPixel.fd,
         pstBuf->stPixel.priv.pvPa, pstBuf->stPixel.pvBase,
         u32VppUtils_GetUVOffset(pstParam), pstBuf->stPixel.u32ValidLen);
}

static void vVppIpFrcMc_StateSet(t_StVppIpFrcMcCb *pstCb, t_EVppFrcMcState eState)
{
    t_EVppFrcMcState eStatePrev;

    if (!pstCb)
        return;

    if (pstCb->eFrcMcState != eState)
    {
        eStatePrev = pstCb->eFrcMcState;
        pstCb->eFrcMcState = eState;
        LOGI("MC state change from %u to %u", eStatePrev, eState);
    }
}

static uint32_t u32VppIpFrcMc_ReturnBuffer(t_StVppIpFrcMcCb *pstCb,
                                           enum vpp_port ePort,
                                           t_StVppBuf *pstBuf)
{
    if (ePort == VPP_PORT_OUTPUT)
        pstCb->stats.u32OBDYuvCnt++;
    else
    {
        if (pstBuf->eBufPxType == eVppBufPxDataType_Raw)
            pstCb->stats.u32IBDYuvCnt++;
        else
            pstCb->stats.u32IBDMbiCnt++;
    }

    return u32VppIp_CbBufDone(&pstCb->stBase.stCb, ePort, pstBuf);
}

#ifdef VPP_TARGET_USES_C2D
static uint32_t u32VppIpFrcMc_UnassociateInternalBuffers(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t i;
    t_StVppBuf *pstBuf;
    struct vpp_buffer *pstExtBuf;

    if (!pstCb)
        return VPP_ERR_PARAM;

    for (i = 0; i < pstCb->stC2DCb.u32AllocCnt; i++)
    {
        pstBuf = &pstCb->stC2DCb.stC2DBufs.pstVppBuf[i];
        pstExtBuf = &pstCb->stC2DCb.stC2DBufs.pstVppExtBuf[i];

        // External buffer
        memset(&pstExtBuf->pixel, 0, sizeof(pstExtBuf->pixel));

        // Internal buffer
        pstBuf->pBuf = NULL;
        memset(&pstBuf->stPixel, 0, sizeof(t_StVppMemBuf));
    }

    return VPP_OK;
}

static uint32_t u32VppIpFrcMc_AssociateInternalBuffers(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t i;
    t_StVppBuf *pstBuf;
    struct vpp_buffer *pstExtBuf;
    t_StVppIonBuf *pstIonPx;

    if (!pstCb)
        return VPP_ERR_PARAM;

    for (i = 0; i < pstCb->stC2DCb.u32AllocCnt; i++)
    {
        pstBuf = &pstCb->stC2DCb.stC2DBufs.pstVppBuf[i];
        pstExtBuf = &pstCb->stC2DCb.stC2DBufs.pstVppExtBuf[i];
        pstIonPx = &pstCb->stC2DCb.stC2DBufs.pstIonBuf[i];

        // Fill out the external buffer structure
        pstExtBuf->pixel.fd = pstIonPx->fd_ion_mem;
        pstExtBuf->pixel.offset = 0;
        pstExtBuf->pixel.alloc_len = pstIonPx->len;
        pstExtBuf->pixel.filled_len = 0;
        pstExtBuf->pixel.valid_data_len = pstIonPx->len;

        // Internal buffer structure
        pstBuf->pBuf = pstExtBuf;
        pstBuf->eBufPxType = eVppBufPxDataType_Raw;
        pstBuf->u32InternalFlags = VPP_BUF_FLAG_INTERNAL;

        pstBuf->stPixel.pvBase = pstIonPx->buf;
        pstBuf->stPixel.eMapping = eVppBuf_MappedInternal;
        pstBuf->stPixel.fd = pstExtBuf->pixel.fd;
        pstBuf->stPixel.u32AllocLen = pstExtBuf->pixel.alloc_len;
        pstBuf->stPixel.u32FilledLen = pstExtBuf->pixel.filled_len;
        pstBuf->stPixel.u32Offset = pstExtBuf->pixel.offset;
        pstBuf->stPixel.u32ValidLen = pstExtBuf->pixel.valid_data_len;
        pstBuf->stPixel.u32MappedLen = pstExtBuf->pixel.alloc_len;

        if (pstCb->stBase.bSecureSession)
        {
            pstBuf->stPixel.pvBase = VPP_BUF_UNMAPPED_BUF_VAL;
            pstBuf->stPixel.eMapping = eVppBuf_Unmapped;
            pstBuf->stPixel.u32MappedLen = 0;
        }
    }

    return VPP_OK;
}

static uint32_t u32VppIpFrcMc_FreeInternalBufferMem(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t i, u32Ret, u32Err = VPP_OK;

    if (!pstCb)
        return VPP_ERR_PARAM;

    for (i = 0; i < pstCb->stC2DCb.u32AllocCnt; i++)
    {
        u32Ret = u32VppIon_Free(pstCb->stBase.pstCtx,
                                &pstCb->stC2DCb.stC2DBufs.pstIonBuf[i]);
        if (u32Ret != VPP_OK)
        {
            LOGE("unable to ion free ion px at i=%u, u32Ret=%u", i, u32Ret);
            u32Err = VPP_ERR;
        }
    }
    pstCb->stC2DCb.u32AllocPxSize = 0;

    return u32Err;
}

static uint32_t u32VppIpFrcMc_AllocInternalBufferMem(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t u32BufSz, u32Cnt, u32Ret, i;
    uint32_t bSecure = VPP_FALSE;

    if (!pstCb)
        return VPP_ERR_PARAM;

    if (pstCb->stBase.bSecureSession)
        bSecure = VPP_TRUE;

    u32BufSz = pstCb->stC2DCb.u32AllocPxSize;
    for (u32Cnt = 0; u32Cnt < pstCb->stC2DCb.u32AllocCnt; u32Cnt++)
    {
        LOGI("pstCtx=%p, pstIonBuf=%p", pstCb->stBase.pstCtx,
             &pstCb->stC2DCb.stC2DBufs.pstIonBuf[u32Cnt]);
        u32Ret = u32VppIon_Alloc(pstCb->stBase.pstCtx,
                                 u32BufSz,
                                 bSecure,
                                 &pstCb->stC2DCb.stC2DBufs.pstIonBuf[u32Cnt]);
        if (u32Ret != VPP_OK)
        {
            LOGE("unable to ion allocate C2D px buffer[%u], u32Ret=%u",
                 u32Cnt, u32Ret);
            for (; u32Cnt; u32Cnt--)
            {
                i = u32Cnt - 1;
                u32Ret = u32VppIon_Free(pstCb->stBase.pstCtx,
                                        &pstCb->stC2DCb.stC2DBufs.pstIonBuf[i]);
                if (!u32Ret)
                    LOGE("unable to ion free internal C2D px buffer[%u], u32Ret=%u",
                         i, u32Ret);
            }
            return VPP_ERR_NO_MEM;
        }
    }

    return VPP_OK;
}

static uint32_t u32VppIpFrcMc_FreeInternalBufStructs(t_StVppIpFrcMcCb *pstCb)
{

    if (!pstCb)
        return VPP_ERR_PARAM;

    if (pstCb->stC2DCb.stC2DBufs.pstVppBuf)
    {
        free(pstCb->stC2DCb.stC2DBufs.pstVppBuf);
        pstCb->stC2DCb.stC2DBufs.pstVppBuf = NULL;
    }

    if (pstCb->stC2DCb.stC2DBufs.pstVppExtBuf)
    {
        free(pstCb->stC2DCb.stC2DBufs.pstVppExtBuf);
        pstCb->stC2DCb.stC2DBufs.pstVppExtBuf = NULL;
    }

    if (pstCb->stC2DCb.stC2DBufs.pstIonBuf)
    {
        free(pstCb->stC2DCb.stC2DBufs.pstIonBuf);
        pstCb->stC2DCb.stC2DBufs.pstIonBuf = NULL;
    }

    return VPP_OK;
}

static uint32_t u32VppIpFrcMc_AllocInternalBufStructs(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t u32BufCnt;

    if (!pstCb)
        return VPP_ERR_PARAM;

    u32BufCnt = pstCb->stC2DCb.u32AllocCnt;

    pstCb->stC2DCb.stC2DBufs.pstVppBuf = calloc(u32BufCnt, sizeof(t_StVppBuf));
    if (!pstCb->stC2DCb.stC2DBufs.pstVppBuf)
    {
        LOGE("unable to allocate pstVppBuf");
        goto ERR_ALLOC;
    }

    pstCb->stC2DCb.stC2DBufs.pstVppExtBuf = calloc(u32BufCnt, sizeof(struct vpp_buffer));
    if (!pstCb->stC2DCb.stC2DBufs.pstVppExtBuf)
    {
        LOGE("unable to allocate pstVppExtBuf");
        goto ERR_ALLOC;
    }

    pstCb->stC2DCb.stC2DBufs.pstIonBuf = calloc(u32BufCnt, sizeof(t_StVppIonBuf));
    if (!pstCb->stC2DCb.stC2DBufs.pstIonBuf)
    {
        LOGE("unable to allocate pstIonBuf");
        goto ERR_ALLOC;
    }

    return VPP_OK;

ERR_ALLOC:
    u32VppIpFrcMc_FreeInternalBufStructs(pstCb);
    return VPP_ERR_NO_MEM;
}

static uint32_t u32VppIpFrcMc_FreeInternalC2DBuffers(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t u32Ret;
    uint32_t u32Err = VPP_OK;

    if (!pstCb)
        return VPP_ERR_PARAM;

    u32Ret = u32VppIpFrcMc_UnassociateInternalBuffers(pstCb);
    if (u32Ret != VPP_OK)
    {
        u32Err = VPP_ERR;
        LOGE("unable to unassociate internal buffers u32Ret=%u", u32Ret);
    }

    u32Ret = u32VppIpFrcMc_FreeInternalBufferMem(pstCb);
    if (u32Ret != VPP_OK)
    {
        u32Err = VPP_ERR;
        LOGE("unable to free internal ion buffers u32Ret=%u", u32Ret);
    }

    u32Ret = u32VppIpFrcMc_FreeInternalBufStructs(pstCb);
    if (u32Ret != VPP_OK)
    {
        u32Err = VPP_ERR;
        LOGE("unable to free internal buffer structures u32Ret=%u", u32Ret);
    }

    pstCb->stC2DCb.u32AllocPxSize = 0;
    pstCb->stC2DCb.u32AllocCnt =0;

    return u32Err;
}

static uint32_t u32VppIpFrcMc_AllocInternalC2DBuffers(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t u32BufSz, u32BufSzTmp, u32Ret;
    struct vpp_port_param stParam;

    if (!pstCb)
        return VPP_ERR_PARAM;

    pstCb->stC2DCb.u32AllocCnt = pstCb->u32FwBufInPixReq + pstCb->u32FwBufOutPixReq;

    stParam = pstCb->stInput.stParam;
    u32BufSz = u32VppUtils_GetPxBufferSize(&stParam);
    stParam.fmt = VPP_COLOR_FORMAT_NV12_VENUS;
    u32BufSzTmp = u32VppUtils_GetPxBufferSize(&stParam);
    if (u32BufSzTmp > u32BufSz)
        u32BufSz = u32BufSzTmp;
    pstCb->stC2DCb.u32AllocPxSize = u32BufSz;

    u32Ret = u32VppIpFrcMc_AllocInternalBufStructs(pstCb);
    if (u32Ret != VPP_OK)
    {
        LOGE("unable to allocate internal buffer structures u32Ret=%u", u32Ret);
        goto ERR_ALLOC;
    }

    u32Ret = u32VppIpFrcMc_AllocInternalBufferMem(pstCb);
    if (u32Ret != VPP_OK)
    {
        LOGE("unable to allocate internal ion buffers u32Ret=%u", u32Ret);
        goto ERR_ALLOC;
    }

    u32Ret = u32VppIpFrcMc_AssociateInternalBuffers(pstCb);
    if (u32Ret != VPP_OK)
    {
        LOGE("unable link associate buffers u32Ret=%u", u32Ret);
        goto ERR_ALLOC;
    }

    return VPP_OK;

ERR_ALLOC:
    u32VppIpFrcMc_FreeInternalC2DBuffers(pstCb);
    pstCb->stC2DCb.u32AllocPxSize = 0;
    pstCb->stC2DCb.u32AllocCnt =0;

    return VPP_ERR;
}

static void vVppIpFrcMc_CalcPortParam(uint32_t u32Width, uint32_t u32Height,
                                      enum vpp_color_format eFmt,
                                      struct vpp_port_param *o_pstParamLinear)
{
    VPP_RET_VOID_IF_NULL(o_pstParamLinear);

    o_pstParamLinear->width = u32Width;
    o_pstParamLinear->height = u32Height;
    o_pstParamLinear->fmt = eFmt;
    o_pstParamLinear->stride =
        u32VppUtils_CalcStrideForPlane(u32Width, eFmt, 0);
    o_pstParamLinear->scanlines =
        u32VppUtils_CalcScanlinesForPlane(u32Height, eFmt, 0);
}

static uint32_t u32VppIpFrcMc_InitC2D(t_StVppIpFrcMcCb *pstCb)
{
    struct vpp_port_param stParamLinear, stParamUbwc;
    uint32_t u32Ret = VPP_OK;

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);

    stParamUbwc = pstCb->stInput.stParam;
    vVppIpFrcMc_CalcPortParam(stParamUbwc.width, stParamUbwc.height,
                              VPP_COLOR_FORMAT_NV12_VENUS, &stParamLinear);

    pstCb->stC2DCb.vpCtxUbwcToLin = vpVppIpC2D_InlineInit(pstCb->stBase.pstCtx,
                                                          stParamUbwc,
                                                          stParamLinear);
    if (!pstCb->stC2DCb.vpCtxUbwcToLin)
    {
        LOGE("Error initializing C2D UBWC to Linear context!");
        u32Ret = VPP_ERR_HW;
        goto ERR_C2D_UBWC_TO_LIN;
    }
    pstCb->stC2DCb.vpCtxLinToUbwc = vpVppIpC2D_InlineInit(pstCb->stBase.pstCtx,
                                                          stParamLinear,
                                                          stParamUbwc);
    if (!pstCb->stC2DCb.vpCtxLinToUbwc)
    {
        LOGE("Error initializing C2D Linear to UBWC context!");
        u32Ret = VPP_ERR_HW;
        goto ERR_C2D_LIN_TO_UBWC;
    }

    u32Ret = u32VppIpFrcMc_AllocInternalC2DBuffers(pstCb);
    if (u32Ret != VPP_OK)
    {
        LOGE("Error allocating internal C2D buffers u32Ret=%u", u32Ret);
        goto ERR_ALLOC_C2D_BUFS;
    }

    return u32Ret;

ERR_ALLOC_C2D_BUFS:
    vVppIpC2D_InlineTerm(pstCb->stC2DCb.vpCtxLinToUbwc);
    pstCb->stC2DCb.vpCtxLinToUbwc = NULL;

ERR_C2D_LIN_TO_UBWC:
    vVppIpC2D_InlineTerm(pstCb->stC2DCb.vpCtxUbwcToLin);
    pstCb->stC2DCb.vpCtxUbwcToLin = NULL;

ERR_C2D_UBWC_TO_LIN:
    return u32Ret;
}

static void vVppIpFrcMc_TermC2D(t_StVppIpFrcMcCb *pstCb)
{
    VPP_RET_VOID_IF_NULL(pstCb);

    if (pstCb->stC2DCb.vpCtxLinToUbwc)
    {
        vVppIpC2D_InlineTerm(pstCb->stC2DCb.vpCtxLinToUbwc);
        pstCb->stC2DCb.vpCtxLinToUbwc = NULL;
    }
    if (pstCb->stC2DCb.vpCtxUbwcToLin)
    {
        vVppIpC2D_InlineTerm(pstCb->stC2DCb.vpCtxUbwcToLin);
        pstCb->stC2DCb.vpCtxUbwcToLin = NULL;
    }
}

static uint32_t u32VppIpFrcMc_ReconfigureC2D(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t u32 = VPP_OK;
    uint32_t u32BufSz, u32BufSzTmp;
    struct vpp_port_param stParamUbwc, stParamLinear;

    if (!pstCb)
        return VPP_ERR_PARAM;

    if (pstCb->bNeedsC2D)
    {
        if (!pstCb->stC2DCb.vpCtxUbwcToLin || !pstCb->stC2DCb.vpCtxLinToUbwc)
        {
            vVppIpFrcMc_TermC2D(pstCb);
            u32 = u32VppIpFrcMc_InitC2D(pstCb);
            if (u32 != VPP_OK)
            {
                LOGE("Error initializing C2D, u32=%u", u32);
                return u32;
            }
        }
        else
        {
            stParamUbwc = pstCb->stInput.stParam;
            vVppIpFrcMc_CalcPortParam(stParamUbwc.width, stParamUbwc.height,
                                      VPP_COLOR_FORMAT_NV12_VENUS, &stParamLinear);
            u32 = u32VppIpC2D_InlineReconfigure(pstCb->stC2DCb.vpCtxUbwcToLin,
                                                stParamUbwc, stParamLinear);
            if (u32 != VPP_OK)
            {
                LOGE("Error reconfiguring vpCtxUbwcToLin, u32=%u", u32);
                return u32;
            }
            u32 = u32VppIpC2D_InlineReconfigure(pstCb->stC2DCb.vpCtxLinToUbwc,
                                                stParamLinear, stParamUbwc);
            if (u32 != VPP_OK)
            {
                LOGE("Error reconfiguring vpCtxLinToUbwc, u32=%u", u32);
                return u32;
            }

            u32BufSz = u32VppUtils_GetPxBufferSize(&stParamLinear);
            u32BufSzTmp = u32VppUtils_GetPxBufferSize(&stParamUbwc);
            if (u32BufSzTmp > u32BufSz)
                u32BufSz = u32BufSzTmp;

            if (pstCb->stC2DCb.u32AllocCnt)
            {
                if (u32BufSz > pstCb->stC2DCb.u32AllocPxSize)
                {
                    u32 = u32VppIpFrcMc_FreeInternalC2DBuffers(pstCb);
                    if (u32 != VPP_OK)
                    {
                        LOGE("Error freeing internal C2D buffers, u32=%u", u32);
                        return u32;
                    }
                    u32 = u32VppIpFrcMc_AllocInternalC2DBuffers(pstCb);
                    if (u32 != VPP_OK)
                    {
                        LOGE("Error allocating internal C2D buffers, u32=%u", u32);
                        return u32;
                    }
                }
            }
            else
            {
                u32 = u32VppIpFrcMc_AllocInternalC2DBuffers(pstCb);
                if (u32 != VPP_OK)
                {
                    LOGE("Error allocating internal C2D buffers, u32=%u", u32);
                    return u32;
                }
            }
        }
    }

    return VPP_OK;
}
#endif

static void vVppIpFrcMc_InitParam(t_StVppIpFrcMcCb *pstCb)
{
    t_StFrcMcParam *pstFrcMcParams = &pstCb->stFrcMcParams;
    t_StCustomFrcMcParams *pstLocalParams = &pstCb->stLocalFrcMcParams;
    LOGI("%s()", __func__);

    memset(pstFrcMcParams, 0, sizeof(t_StFrcMcParam));

    vpp_svc_frc_params_t *frc_params_p = &pstCb->stFrcMcParams.stFrcParams;

    memset((void*)frc_params_p, 0, sizeof(vpp_svc_frc_params_t));

    frc_params_p->update_flags = 1;
    //Init pstCb->stFrcMcParams using values stored in pstCb->stLocalFrcMcParams
    frc_params_p->mode = pstLocalParams->mode;
    frc_params_p->mc_quality = pstLocalParams->mc_quality;
    frc_params_p->RepeatMode_repeatPeriod = pstLocalParams->RepeatMode_repeatPeriod;
    frc_params_p->TH_MOTION = pstLocalParams->TH_MOTION;
    frc_params_p->TH_MOTION_LOW = pstLocalParams->TH_MOTION_LOW;
    frc_params_p->TH_MVOUTLIER_COUNT = pstLocalParams->TH_MVOUTLIER_COUNT;
    frc_params_p->TH_MVOUTLIER_COUNT_LOW = pstLocalParams->TH_MVOUTLIER_COUNT_LOW;
    frc_params_p->TH_OCCLUSION = pstLocalParams->TH_OCCLUSION;
    frc_params_p->TH_OCCLUSION_LOW = pstLocalParams->TH_OCCLUSION_LOW;
    frc_params_p->TH_MOTION00 = pstLocalParams->TH_MOTION00;
    frc_params_p->TH_MOTION00_LOW = pstLocalParams->TH_MOTION00_LOW;
    frc_params_p->TH_MVOUTLIER_VARIANCE_COUNT = pstLocalParams->TH_MVOUTLIER_VARIANCE_COUNT;
    frc_params_p->TH_MVOUTLIER_VARIANCE_COUNT_LOW = pstLocalParams->TH_MVOUTLIER_VARIANCE_COUNT_LOW;
    frc_params_p->TH_SCENECUT = pstLocalParams->TH_SCENECUT;
    frc_params_p->TH_VARIANCE = pstLocalParams->TH_VARIANCE;
    frc_params_p->TH_SAD_FR_RATIO = pstLocalParams->TH_SAD_FR_RATIO;

    print_vpp_svc_frc_params(frc_params_p);
}

static void vVppIpFrcMc_InitCapabilityResources(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t u32Index = 0;

    //FRC
    u32VppIpHvxCore_AlgoInit(pstCb->pstHvxCore, u32Index, VPP_FUNC_ID_FRC, NULL);
}

static void vVppIpFrcMc_Compute(t_StVppIpFrcMcCb *pstCb)
{
    t_StFrcMcParam *pstFrcMcParams;
    uint32_t u32Length, u32Index, u32Width, u32Height, u32Planes;
    uint32_t au32Stride[MAX_NUM_PLANES];
    enum vpp_color_format eFmt;
    uint32_t u32Computed = 0;

    LOGI("%s()", __func__);

    if (!pstCb || !pstCb->pstHvxCore)
    {
        LOGE("%s() Error. FRCMC or HVX Core control block is NULL.", __func__);
        return;
    }

    u32Computed = pstCb->stCfg.u32ComputeMask;
    pstFrcMcParams = &pstCb->stFrcMcParams;
    u32Width = pstCb->stInput.stParam.width;
    u32Height = pstCb->stInput.stParam.height;
    eFmt = pstCb->stInput.stParam.fmt;
#ifdef VPP_TARGET_USES_C2D
    if (pstCb->bNeedsC2D)
    {
        if (pstCb->stInput.stParam.fmt == VPP_COLOR_FORMAT_UBWC_NV12)
            eFmt = VPP_COLOR_FORMAT_NV12_VENUS;
        else if (pstCb->stInput.stParam.fmt == VPP_COLOR_FORMAT_UBWC_NV21)
            eFmt = VPP_COLOR_FORMAT_NV21_VENUS;
    }
#endif
    u32Planes = u32VppUtils_GetNumPlanes(eFmt);

    if (pstCb->stCfg.u32ComputeMask & FRCMC_PARAM)
    {

        u32VppIpHvxCore_BufParamInit(pstCb->pstHvxCore, u32Width, u32Height, eFmt);

        if (pstCb->stCfg.u32EnableMask & FRC_ALGO_MC)
            pstCb->stCfg.u32ComputeMask |= FRC_ALGO_MC;

        pstCb->stCfg.u32ComputeMask &= ~FRCMC_PARAM;
    }

    u32Index = 0;
    u32Length = 0;
    if (pstCb->stCfg.u32EnableMask & FRC_ALGO_MC)
    {
        LOGI("Set pstParams->header");
        u32VppIpHvxCore_SvcParamSetHeaderIdxAlgo(pstCb->pstHvxCore, u32Index, VPP_FUNC_ID_FRC);
        u32Index++;
        pstCb->pstFrcParams = (vpp_svc_frc_params_t*)
            (pvVppIpHvxCore_SvcParamGetDataOffsetAddr(pstCb->pstHvxCore, u32Length));

        vpp_svc_frc_params_t* frc_params_p = &pstFrcMcParams->stFrcParams;
        frc_params_p->update_flags = 1;

        if (pstCb->stCfg.u32ComputeMask & FRC_ALGO_MC)
        {
            struct vpp_ctrl_frc_segment *pstFrcSeg = NULL;
            int32_t s32Idx = pstCb->stCfg.stFrcCtrlSegs.s32SegIdx;

            frc_params_p->in_frame_width = u32Width;
            frc_params_p->in_frame_height = u32Height;

            if ((s32Idx >= 0) && ((uint32_t)s32Idx < pstCb->stCfg.stFrcCtrlSegs.u32AllocCnt) &&
                ((uint32_t)s32Idx < pstCb->stCfg.stFrcCtrlSegs.u32SegCnt))
            {
                pstFrcSeg = &pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments[s32Idx];
                LOGI("Seg[%d]: Mode=%u, Level=%u, Interp=%u, CopyIn=%u, CopyFb=%u, Ts=%"PRIu64,
                     s32Idx, pstFrcSeg->mode, pstFrcSeg->level, pstFrcSeg->interp,
                     pstFrcSeg->frame_copy_input, pstFrcSeg->frame_copy_on_fallback,
                     pstFrcSeg->ts_start);
                frc_params_p->input_copy = pstFrcSeg->frame_copy_input;
                frc_params_p->fallback_copy = pstFrcSeg->frame_copy_on_fallback;
                switch (pstFrcSeg->mode)
                {
                    case HQV_FRC_MODE_OFF:
                        frc_params_p->mode = FRC_MODE_PASSTHROUGH;
                        break;
                    case HQV_FRC_MODE_SLOMO:
                        frc_params_p->mode = FRC_MODE_SLOMO;
                        break;
                    case HQV_FRC_MODE_SMOOTH_MOTION:
                    default:
                        frc_params_p->mode = FRC_MODE_HFR;
                        break;
                }

                switch (pstFrcSeg->level)
                {
                    case HQV_FRC_LEVEL_OFF:
                        frc_params_p->mc_quality = FRC_MC_QUAL_REPEAT;
                        break;
                    case HQV_FRC_LEVEL_LOW:
                        frc_params_p->mc_quality = FRC_MC_QUAL_WEAK;
                        break;
                    case HQV_FRC_LEVEL_MED:
                        frc_params_p->mc_quality = FRC_MC_QUAL_MED;
                        break;
                    case HQV_FRC_LEVEL_HIGH:
                    default:
                        frc_params_p->mc_quality = FRC_MC_QUAL_STRONG;
                        break;
                }

                frc_params_p->NUM_INTERP_FRAMES = FRCMC_INTERP_FRAMES_DEFAULT;
                frc_params_p->interp_cnt = 0;
                if (pstFrcSeg->mode == HQV_FRC_MODE_SLOMO)
                {
                    switch (pstFrcSeg->interp)
                    {
                        case HQV_FRC_INTERP_2X:
                            frc_params_p->NUM_INTERP_FRAMES = 1;
                            break;
                        case HQV_FRC_INTERP_3X:
                            frc_params_p->NUM_INTERP_FRAMES = 2;
                            break;
                        case HQV_FRC_INTERP_4X:
                            frc_params_p->NUM_INTERP_FRAMES = 3;
                            break;
                        case HQV_FRC_INTERP_1X:
                        default:
                            frc_params_p->NUM_INTERP_FRAMES = 0;
                            break;
                    }
                }
            }
            else
            {
                // Program for DSP bypass
                LOGD("Segment=%d, programming for bypass", s32Idx);
                frc_params_p->mode = FRC_MODE_SLOMO;
                frc_params_p->mc_quality = FRC_MC_QUAL_REPEAT;
                frc_params_p->NUM_INTERP_FRAMES = 0;
                frc_params_p->input_copy = 0;
                frc_params_p->fallback_copy = 0;
            }
            pstCb->stInfo.u32InterpFrames = frc_params_p->NUM_INTERP_FRAMES;
            pstCb->stInfo.u32FrcFactorReq = frc_params_p->NUM_INTERP_FRAMES + 1;
            pstCb->stInfo.u32FrcFactorActual = frc_params_p->NUM_INTERP_FRAMES + 1;
            pstCb->stInfo.u32InterpCnt = frc_params_p->interp_cnt;

            pstCb->stCfg.u32ComputeMask &= ~FRC_ALGO_MC;
        }

        memcpy(pstCb->pstFrcParams, frc_params_p, sizeof(vpp_svc_frc_params_t));
        u32Length += sizeof(vpp_svc_frc_params_t);
    }

    u32VppIpHvxCore_SvcParamSetDataSize(pstCb->pstHvxCore, u32Index, u32Length);

    if (u32Computed)
    {
        u32VppIpHvxCore_BuffInCompute(pstCb->pstHvxCore);
        u32VppIpHvxCore_BuffOutCompute(pstCb->pstHvxCore);

        if (pstCb->u32FwBufInMbiReq)
        {
            // MBI buffer doesn't use stride, init to 0
            memset(au32Stride, 0, sizeof(au32Stride));
            u32VppIpHvxCore_BufInSetAttrStride(pstCb->pstHvxCore, pstCb->u32FwBufInPixReq,
                                               au32Stride, u32Planes);
        }
    }

    return;
}

static uint32_t u32VppIpFrcMc_CmdGet(t_StVppIpFrcMcCb *pstCb, t_StVppIpCmd *pstCmd)
{
    LOGI("%s() pstCmd->eCmd=%d", __func__, pstCmd->eCmd);
    int32_t idx;
    idx = vpp_queue_dequeue(&pstCb->stCmdQ);

    if (idx < 0)
    {
        LOGI("GetCmd=%d, idx=%d", pstCmd->eCmd, idx);
        return VPP_ERR;
    }
    else
    {
        *pstCmd = pstCb->astCmdNode[idx];
        LOG_CMD("GetCmd", pstCmd->eCmd);
        LOGI("GetCmd idx=%d pstCmd->eCmd=%d", idx, pstCmd->eCmd);
    }

    return VPP_OK;
}

static uint32_t u32VppIpFrcMc_CmdPut(t_StVppIpFrcMcCb *pstCb, t_StVppIpCmd stCmd)
{
    int32_t idx;
    uint32_t u32Ret = VPP_OK;

    pthread_mutex_lock(&pstCb->mutex);

    LOG_CMD("InsertCmd", stCmd.eCmd);
    idx = vpp_queue_enqueue(&pstCb->stCmdQ);
    if (idx < 0)
    {
        u32Ret = VPP_ERR;
    }
    else
    {
        pstCb->astCmdNode[idx] = stCmd;
        pthread_cond_signal(&pstCb->cond);
    }

    pthread_mutex_unlock(&pstCb->mutex);

    return u32Ret;
}

static void vVppIpFrcMc_SignalWorkerStart(t_StVppIpFrcMcCb *pstCb)
{
    pthread_mutex_lock(&pstCb->mutex);

    pstCb->u32InternalFlags |= IP_WORKER_STARTED;

    pthread_cond_signal(&pstCb->cond);

    pthread_mutex_unlock(&pstCb->mutex);
}

static void vVppIpFrcMc_WaitWorkerStart(t_StVppIpFrcMcCb *pstCb)
{
    pthread_mutex_lock(&pstCb->mutex);

    while (!(pstCb->u32InternalFlags & IP_WORKER_STARTED))
        pthread_cond_wait(&pstCb->cond, &pstCb->mutex);

    pthread_mutex_unlock(&pstCb->mutex);
}

static uint32_t bVppIpFrcMc_NeedFrameCopyInput(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t bNeedCopyInput = VPP_FALSE;
    struct vpp_ctrl_frc_segment *pstFrcCtrl;
    t_StVppIpFrcCtrlSegs *pstFrcSegs;

    pstFrcSegs = &pstCb->stCfg.stFrcCtrlSegs;

    if (pstFrcSegs->s32SegIdx >= 0)
    {
        pstFrcCtrl = &pstFrcSegs->pstFrcSegments[pstFrcSegs->s32SegIdx];

        if (pstFrcCtrl->mode != HQV_FRC_MODE_OFF && pstFrcCtrl->frame_copy_input == VPP_TRUE)
            bNeedCopyInput = VPP_TRUE;
    }

    return bNeedCopyInput;
}

static uint32_t bVppIpFrcMc_CanReduceInBufReq(t_StVppIpFrcMcCb *pstCb, uint32_t *pu32BufProcCnt)
{
    uint32_t i;
    t_StVppBuf **ppstBufIn;
    uint32_t u32InQSz, u32OutQSz, u32MbiQSz, u32BufReqSz, bNeedCopyInput;

    VPP_RET_IF_NULL(pstCb, VPP_FALSE);
    VPP_RET_IF_NULL(pu32BufProcCnt, VPP_FALSE);

    u32InQSz = u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ);
    u32OutQSz = u32VppBufPool_Cnt(&pstCb->stOutput.stPendingQ);
    u32MbiQSz = u32VppBufPool_Cnt(&pstCb->stMbi.stPendingQ);
    u32BufReqSz = u32VppUtils_GetPxBufferSize(&pstCb->stInput.stParam);
    ppstBufIn = pstCb->ppstBufPeekPix;
    bNeedCopyInput = bVppIpFrcMc_NeedFrameCopyInput(pstCb);

    *pu32BufProcCnt = pstCb->u32FwBufInPixReq;
    if (u32InQSz && u32OutQSz)
    {
        if (VPP_FLAG_IS_SET(pstCb->u32InternalFlags, IP_DRAIN_PENDING) &&
            u32InQSz < pstCb->u32FwBufInPixReq &&
            ((u32MbiQSz >= u32InQSz - 1) || pstCb->u32FwBufInMbiReq == 0))
        {
            if (bNeedCopyInput && (u32InQSz > 1) && (u32OutQSz < pstCb->u32FwBufOutPixReq))
            {
                return VPP_FALSE;
            }
            *pu32BufProcCnt = u32InQSz;
            LOGD("Pending drain, process with %u buffers", u32InQSz);
            return VPP_TRUE;
        }
        if (u32InQSz == 1)
        {
            if (bNeedCopyInput)
            {
                ppstBufIn[0] = pstVppIp_PortBufPeek(&pstCb->stInput, 0, NULL);

                if (VPP_FLAG_IS_SET(ppstBufIn[0]->pBuf->flags, VPP_BUFFER_FLAG_EOS))
                {
                    *pu32BufProcCnt = 1;
                    LOGD("Pending EOS with 1 buffer remaining and copy enabled; copy to output");
                    return VPP_TRUE;
                }
            }
            return VPP_FALSE;
        }

        for (i = 0; i < (pstCb->u32FwBufInPixReq - 1) && i < (u32InQSz - 1); i++)
        {
            ppstBufIn[i] = pstVppIp_PortBufPeek(&pstCb->stInput, i + 1, NULL);
            if (ppstBufIn[i] == NULL)
            {
                LOGE("%u buffers in queue, but peeked buffer index %u is NULL!",
                     u32InQSz, i + 1);
                return VPP_FALSE;
            }
            if (ppstBufIn[i]->stPixel.u32FilledLen < u32BufReqSz ||
                ppstBufIn[i]->stPixel.u32ValidLen < u32BufReqSz)
            {
                // Do not process improperly filled buffer
                if ((u32MbiQSz >= i) || pstCb->u32FwBufInMbiReq == 0)
                {
                    // Need to interp and copy, but not enough output buffers
                    if (i && bNeedCopyInput && (u32OutQSz < pstCb->u32FwBufOutPixReq))
                        return VPP_FALSE;

                    *pu32BufProcCnt = i + 1;
                    if (VPP_FLAG_IS_SET(ppstBufIn[i]->pBuf->flags, VPP_BUFFER_FLAG_EOS))
                        LOGD("Empty EOS, using %u buffers", i + 1);
                    else
                        LOGE("Buf[%u]: FilledLen=%u ValidLen=%u Required=%u! Using %u buffers",
                             i + 1, ppstBufIn[i]->stPixel.u32FilledLen,
                             ppstBufIn[i]->stPixel.u32ValidLen, u32BufReqSz, i + 1);
                    return VPP_TRUE;
                }
            }
            if (VPP_FLAG_IS_SET(ppstBufIn[i]->pBuf->flags, VPP_BUFFER_FLAG_EOS))
            {
                // Need to interp and copy, but not enough output buffers
                if (bNeedCopyInput && (u32OutQSz < pstCb->u32FwBufOutPixReq))
                    return VPP_FALSE;

                // Include filled EOS buffer in process
                if ((u32MbiQSz >= (i + 1)) || pstCb->u32FwBufInMbiReq == 0)
                {
                    *pu32BufProcCnt = i + 2;
                    LOGD("Pending EOS, using %u buffers", i + 2);
                    return VPP_TRUE;
                }
            }
        }
    }

    return VPP_FALSE;
}

static uint32_t bVppIpFrcMc_BypassBufCheck(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t u32BufReqSz = u32VppUtils_GetPxBufferSize(&pstCb->stInput.stParam);
    uint32_t u32InQSz = u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ);
    uint32_t bNeedCopyInput = bVppIpFrcMc_NeedFrameCopyInput(pstCb);
    t_StVppBuf* pstBufIn0 = pstVppIp_PortBufPeek(&pstCb->stInput, 0, NULL);
    t_StVppBuf* pstBufIn1 = pstVppIp_PortBufPeek(&pstCb->stInput, 1, NULL);

    if (u32InQSz)
    {
        if (pstBufIn0 == NULL)
        {
            LOGE("u32InQSz != 0 but pstBufIn_0 == NULL. u32InQSz=%u", u32InQSz);
            return VPP_FALSE;
        }

        if (((VPP_FLAG_IS_SET(pstBufIn0->pBuf->flags, VPP_BUFFER_FLAG_EOS)) && !bNeedCopyInput) ||
            (VPP_FLAG_IS_SET(pstBufIn0->u32InternalFlags, VPP_BUF_FLAG_BYPASS)) ||
            (VPP_FLAG_IS_SET(pstBufIn0->u32InternalFlags, VPP_BUF_FLAG_INTERNAL_BYPASS)) ||
            (pstBufIn0->eBufType != eVppBufType_Progressive))
        {
            LOGD("%s() First buffer EOS=0x%x, bypass=0x%x, internal_bypass=0x%x, type=%u. Bypass",
                 __func__, (VPP_FLAG_IS_SET(pstBufIn0->pBuf->flags, VPP_BUFFER_FLAG_EOS)),
                 (VPP_FLAG_IS_SET(pstBufIn0->u32InternalFlags, VPP_BUF_FLAG_BYPASS)),
                 (VPP_FLAG_IS_SET(pstBufIn0->u32InternalFlags, VPP_BUF_FLAG_INTERNAL_BYPASS)),
                 pstBufIn0->eBufType);
            return VPP_TRUE;
        }

        if (pstBufIn0->stPixel.u32FilledLen < u32BufReqSz ||
            pstBufIn0->stPixel.u32ValidLen < u32BufReqSz)
        {
            LOGE("First buffer u32FilledLen=%u, u32ValidLen=%u, Required=%u",
                 pstBufIn0->stPixel.u32FilledLen, pstBufIn0->stPixel.u32ValidLen, u32BufReqSz);
            return VPP_TRUE;
        }

        // In frame copy input case, bypass conditions will never depend on the second buffer
        if (bNeedCopyInput)
            return VPP_FALSE;

        if (!pstBufIn1)
        {
            // Only one input buffer in queue, check if drain is pending
            if (VPP_FLAG_IS_SET(pstCb->u32InternalFlags, IP_DRAIN_PENDING))
            {
                LOGD("%s() One buffer left to drain for FRC_MC. Can bypass it", __func__);
                return VPP_TRUE;
            }
        }
        else
        {
            if ((VPP_FLAG_IS_SET(pstBufIn1->u32InternalFlags, VPP_BUF_FLAG_BYPASS)) ||
                (VPP_FLAG_IS_SET(pstBufIn1->u32InternalFlags, VPP_BUF_FLAG_INTERNAL_BYPASS)) ||
                (pstBufIn1->eBufType != eVppBufType_Progressive))
            {
                LOGD("%s(): Second buffer bypass=0x%x, internal bypass=0x%x type=%u. Bypass first",
                     __func__, VPP_FLAG_IS_SET(pstBufIn1->u32InternalFlags, VPP_BUF_FLAG_BYPASS),
                     VPP_FLAG_IS_SET(pstBufIn1->u32InternalFlags, VPP_BUF_FLAG_INTERNAL_BYPASS),
                     pstBufIn1->eBufType);
                return VPP_TRUE;
            }

            if (pstBufIn1->stPixel.u32FilledLen < u32BufReqSz ||
                pstBufIn1->stPixel.u32ValidLen < u32BufReqSz)
            {
                LOGE("Second buffer u32FilledLen=%u, u32ValidLen=%u, Required=%u",
                     pstBufIn1->stPixel.u32FilledLen, pstBufIn1->stPixel.u32ValidLen, u32BufReqSz);
                return VPP_TRUE;
            }

            if (pstBufIn1->pBuf->timestamp < pstBufIn0->pBuf->timestamp)
            {
                LOGE("Second buf ts=%" PRIu64 " < first buf ts=%" PRIu64 " Bypass!",
                     pstBufIn1->pBuf->timestamp, pstBufIn0->pBuf->timestamp);
                return VPP_TRUE;
            }
        }
    }

    return VPP_FALSE;
}

static uint32_t bVppIpFrcMc_AlgoCtrlsCanBypass(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t bCanBypass = VPP_TRUE;
    t_StVppIpFrcMcCfg *pstCfg;
    t_StVppIpFrcCtrlSegs *pstFrcSegs;
    struct vpp_ctrl_frc_segment *pstFrcCtrl;

    VPP_RET_IF_NULL(pstCb, VPP_FALSE);

    pstCfg = &pstCb->stCfg;
    pstFrcSegs = &pstCfg->stFrcCtrlSegs;

#if FRCMC_KEEP_ME_RUNNING
    if (pstCb->u32FwBufInMbiReq == 0 && VPP_FLAG_IS_SET(pstCfg->u32EnableMask, FRC_ALGO_MC))
    {
        // In the case where FW doesn't require external MBI buffers, assume
        // that MBI is generated by FW, so keep ME running to avoid start
        // up lag.
        return VPP_FALSE;
    }
#endif

    if (VPP_FLAG_IS_SET(pstCfg->u32EnableMask, FRC_ALGO_MC))
    {
        if (pstFrcSegs->s32SegIdx < 0)
            return VPP_TRUE;

        pstFrcCtrl = &pstFrcSegs->pstFrcSegments[pstFrcSegs->s32SegIdx];
        if (pstFrcCtrl->mode == HQV_FRC_MODE_SLOMO)
        {
            if (pstFrcCtrl->level != HQV_FRC_LEVEL_OFF &&
                pstFrcCtrl->interp != HQV_FRC_INTERP_1X)
                bCanBypass = VPP_FALSE;
            if (pstFrcCtrl->frame_copy_input == VPP_TRUE)
                bCanBypass = VPP_FALSE;
        }
        else if (pstFrcCtrl->mode != HQV_FRC_MODE_OFF &&
                 (pstFrcCtrl->level != HQV_FRC_LEVEL_OFF ||
                  pstFrcCtrl->frame_copy_input == VPP_TRUE))
        {
            bCanBypass = VPP_FALSE;
        }
    }

    return bCanBypass;
}

static uint32_t bVppIpFrcMc_ProcBufReqMet(t_StVppIpFrcMcCb *pstCb)
{
    // Determine if the buffers in the ports satisfy the requirements
    // to trigger processing
    uint32_t u32InQSz, u32OutQSz, u32MbiQSz, u32Temp, u32OutReq, bNeedCopyInput;

    // This function requires that the caller has already locked the mutex
    // which guards these queues.
    u32InQSz = u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ);
    u32MbiQSz = u32VppBufPool_Cnt(&pstCb->stMbi.stPendingQ);
    u32OutQSz = u32VppBufPool_Cnt(&pstCb->stOutput.stPendingQ);
    bNeedCopyInput = bVppIpFrcMc_NeedFrameCopyInput(pstCb);
    u32OutReq = pstCb->u32FwBufOutPixReq;
    if ((u32OutReq > 1) && !bNeedCopyInput)
        u32OutReq--;

    LOGI("CHECK: u32InQSz=%u, u32MbiQSz=%u u32OutQSz=%u, InNeed=%u, MbiNeed=%u, OutNeed=%u",
         u32InQSz, u32MbiQSz, u32OutQSz, pstCb->u32FwBufInPixReq, pstCb->u32FwBufInMbiReq,
         u32OutReq);

    if (u32InQSz == 0)
        return VPP_FALSE;

    // If in interp state, only need to peek one input buffer and have output to write to
    // If no output buffer, need to wait to finish processing input pair already in flight
    if (pstCb->eFrcMcState == FRCMC_STATE_ACTIVE_INTERP)
    {
        if (u32OutQSz)
            return VPP_TRUE;
        else
            return VPP_FALSE;
    }

    if (bVppIpFrcMc_BypassBufCheck(pstCb) == VPP_TRUE)
        return VPP_TRUE;

    // In this case, want to return associated MBI buffer as well to keep in sync
    if (bVppIpFrcMc_AlgoCtrlsCanBypass(pstCb) == VPP_TRUE &&
        u32MbiQSz >= pstCb->u32FwBufInMbiReq)
        return VPP_TRUE;

    if (u32InQSz >= pstCb->u32FwBufInPixReq && u32OutQSz >= u32OutReq &&
        u32MbiQSz >= pstCb->u32FwBufInMbiReq)
        return VPP_TRUE;

    if (bVppIpFrcMc_CanReduceInBufReq(pstCb, &u32Temp) == VPP_TRUE)
        return VPP_TRUE;

    LOGI("CHECK: ReqMet return VPP_FALSE\n");
    return VPP_FALSE;
}

static uint32_t u32WorkerThreadShouldSleep(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t u32Ret = VPP_TRUE;
    uint32_t u32CmdQSz, bProcMet;

    // This is a predicate function used for determining if the function worker
    // thread should sleep or not. Worker thread uses a condition variable to
    // wake itself and the mutex which is used is the same as that which guards
    // these functions. Therefore, there is no need to lock a mutex prior to
    // checking the command queues within this context.
    u32CmdQSz = vpp_queue_count(&pstCb->stCmdQ);
    if (u32CmdQSz)
    {
        u32Ret = VPP_FALSE;
        LOGI("CHECK: shouldSleep=VPP_FALSE, u32CmdQSz=%u", u32CmdQSz);
        return u32Ret;
    }

    bProcMet = bVppIpFrcMc_ProcBufReqMet(pstCb);
    if ((VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE)) && bProcMet)
    {
        u32Ret = VPP_FALSE;
        LOGI("CHECK: shouldSleep=VPP_FALSE, u32ProcMet=%u", bProcMet);
        return u32Ret;
    }

    LOGI("CHECK: shouldSleep=%u, u32CmdQSz=%u, u32ProcMet=%u", u32Ret,
         u32CmdQSz, bProcMet);

    return u32Ret;
}

static uint32_t u32VppIpFrcMc_ValidatePortParam(struct vpp_port_param stParam)
{
    uint32_t u32LargerDim, u32SmallerDim;

    // Actual width and height need to be greater than the min
    if (stParam.width < FRCMC_PORT_PARAM_WIDTH_MIN)
    {
        LOGE("validation failed: width=%u, min=%u", stParam.width,
             FRCMC_PORT_PARAM_WIDTH_MIN);
        return VPP_ERR;
    }
    if (stParam.height < FRCMC_PORT_PARAM_HEIGHT_MIN)
    {
        LOGE("validation failed: height=%u, min=%u", stParam.height,
             FRCMC_PORT_PARAM_HEIGHT_MIN);
        return VPP_ERR;
    }

    // Use larger/smaller dimension to compare against max width/height respectively.
    // Generic check; chipset specific registry validation may be performed as well.
    if (stParam.height > stParam.width)
    {
        u32LargerDim = stParam.height;
        u32SmallerDim = stParam.width;
    }
    else
    {
        u32LargerDim = stParam.width;
        u32SmallerDim = stParam.height;
    }

    if (u32LargerDim > FRCMC_PORT_PARAM_WIDTH_MAX)
    {
        LOGE("validation failed: Larger Dimension=%u, max=%u", u32LargerDim,
             FRCMC_PORT_PARAM_WIDTH_MAX);
        return VPP_ERR;
    }
    if (u32SmallerDim > FRCMC_PORT_PARAM_HEIGHT_MAX)
    {
        LOGE("validation failed: Smaller Dimension=%u, max=%u", u32SmallerDim,
             FRCMC_PORT_PARAM_HEIGHT_MAX);
        return VPP_ERR;
    }

    if (stParam.fmt != VPP_COLOR_FORMAT_NV12_VENUS &&
        stParam.fmt != VPP_COLOR_FORMAT_NV21_VENUS &&
        stParam.fmt != VPP_COLOR_FORMAT_UBWC_NV12 &&
        stParam.fmt != VPP_COLOR_FORMAT_UBWC_NV21)
    {
        LOGE("validation failed: fmt=%u", stParam.fmt);
        return VPP_ERR;
    }
    if (!u32VppUtils_IsFmtUbwc(stParam.fmt))
    {
        if (stParam.width > stParam.stride)
        {
            LOGE("validation failed: width=%u, stride=%u", stParam.width,
                 stParam.stride);
            return VPP_ERR;
        }
        if (stParam.height > stParam.scanlines)
        {
            LOGE("validation failed: height=%u, scanlines=%u", stParam.height,
                 stParam.scanlines);
            return VPP_ERR;
        }
    }

    return VPP_OK;
}

static uint32_t u32VppIpFrcMc_ValidateConfig(struct vpp_port_param stInput,
                                             struct vpp_port_param stOutput)
{
    if (stInput.height != stOutput.height)
    {
        LOGD("validation failed: height, input: %u, output: %u",
             stInput.height, stOutput.height);
        return VPP_ERR;
    }
    if (stInput.width != stOutput.width)
    {
        LOGD("validation failed: width, input: %u, output: %u",
             stInput.width, stOutput.width);
        return VPP_ERR;
    }
    if (stInput.fmt != stOutput.fmt)
    {
        LOGD("validation failed: fmt, input: %u, output: %u",
             stInput.fmt, stOutput.fmt);
        return VPP_ERR;
    }
    if (u32VppIpFrcMc_ValidatePortParam(stInput))
    {
        LOGE("Input port param validation failed");
        return VPP_ERR;
    }
    if (u32VppIpFrcMc_ValidatePortParam(stOutput))
    {
        LOGE("Output port param validation failed");
        return VPP_ERR;
    }

    return VPP_OK;
}

static uint32_t u32VppIpFrcMc_ValidateCtrl(struct hqv_control stCtrl)
{
    struct hqv_ctrl_frc stFrc;
    uint32_t u32Idx;

    if (stCtrl.mode != HQV_MODE_OFF &&
        stCtrl.mode != HQV_MODE_AUTO &&
        stCtrl.mode != HQV_MODE_MANUAL)
    {
        LOGE("Invalid stCtrl.mode=%u", stCtrl.mode);
        return VPP_ERR_PARAM;
    }

    if (stCtrl.mode == HQV_MODE_MANUAL)
    {
        if (stCtrl.ctrl_type != HQV_CONTROL_FRC &&
            stCtrl.ctrl_type != HQV_CONTROL_CUST &&
            stCtrl.ctrl_type != HQV_CONTROL_GLOBAL_DEMO)
        {
            LOGE("Invalid stCtrl.ctrl_type=%u", stCtrl.ctrl_type);
            return VPP_ERR_PARAM;
        }

        if (stCtrl.ctrl_type == HQV_CONTROL_FRC)
        {
            stFrc = stCtrl.frc;
            for (u32Idx = 0; u32Idx < stFrc.num_segments; u32Idx++)
            {
                struct vpp_ctrl_frc_segment *pstFrcSeg = &stFrc.segments[u32Idx];
                if (pstFrcSeg->mode >= HQV_FRC_MODE_MAX)
                {
                    LOGE("Invalid pstFrcSeg->mode=%u at idx=%u", pstFrcSeg->mode, u32Idx);
                    return VPP_ERR_PARAM;
                }

                if (pstFrcSeg->mode != HQV_FRC_MODE_OFF)
                {
                    if (pstFrcSeg->level >= HQV_FRC_LEVEL_MAX)
                    {
                        LOGE("Invalid pstFrcSeg->level=%u at idx=%u", pstFrcSeg->level, u32Idx);
                        return VPP_ERR_PARAM;
                    }

                    if (pstFrcSeg->mode == HQV_FRC_MODE_SLOMO &&
                        pstFrcSeg->interp >= HQV_FRC_INTERP_MAX)
                    {
                        LOGE("Invalid pstFrcSeg->interp=%u at idx=%u", pstFrcSeg->interp, u32Idx);
                        return VPP_ERR_PARAM;
                    }
                }
            }
        }
    }
    return VPP_OK;
}


static uint32_t u32VppIpFrcMc_ProcCmdOpen(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t u32Ret, u32VppProcFlags, u32TotalInBufCnt;
    vpp_svc_inout_buf_req_t stBufReq;
    enum vpp_color_format eFmt;

    LOG_ENTER_ARGS("pstCb->stBase.pstCtx = %p", pstCb->stBase.pstCtx);

    pthread_mutex_lock(&pstCb->mutex);

    eFmt = pstCb->stInput.stParam.fmt;
    if (u32VppUtils_IsFmtUbwc(eFmt) && pstCb->bHvxLinearOnly)
#ifdef VPP_TARGET_USES_C2D
    {
        pstCb->bNeedsC2D = VPP_TRUE;
        pstCb->stCfg.u32ComputeMask |= FRCMC_PARAM;
        if (pstCb->stInput.stParam.fmt == VPP_COLOR_FORMAT_UBWC_NV12)
            eFmt = VPP_COLOR_FORMAT_NV12_VENUS;
        else if (pstCb->stInput.stParam.fmt == VPP_COLOR_FORMAT_UBWC_NV21)
            eFmt = VPP_COLOR_FORMAT_NV21_VENUS;
        else
        {
            // Should never get here since config should be validated
            LOGE("eFmt=%d not supported!", eFmt);
            u32Ret = VPP_ERR_PARAM;
            goto ERR_FORMAT;
        }
    }
#else
    {
        LOGE("eFmt=%d is UBWC. Not supported!", eFmt);
        u32Ret = VPP_ERR_PARAM;
        goto ERR_FORMAT;
    }
#endif

    u32Ret = u32VppIpHvxCore_BufParamInit(pstCb->pstHvxCore, pstCb->stInput.stParam.width,
                                          pstCb->stInput.stParam.height,
                                          eFmt);
    if (u32Ret != VPP_OK)
    {
        LOGE("Error initializing buffer parameters u32Ret=%u", u32Ret);
        goto ERR_BUF_PARAM_INIT;
    }

    // ********* vpp_svc_context_t ****
    //Determine which VPP Functions to enable
    u32VppProcFlags = (1 << (VPP_FUNC_ID_FRC & FUNC_ID_MASK));

    u32Ret = u32VppIpHvxCore_Open(pstCb->pstHvxCore, u32VppProcFlags, 0);
    if (u32Ret != VPP_OK)
    {
        LOGE("HVX Open failed u32Ret=%u", u32Ret);
        goto ERR_HVX_OPEN;
    }

    u32Ret = u32_VppIpHvxCore_GetBufReqs(pstCb->pstHvxCore, u32VppProcFlags, &stBufReq);
    if ((u32Ret != VPP_OK) || !stBufReq.input || !stBufReq.output)
    {
        LOGE("Error getting pixel buffer requirements, u32Ret=%u, InPixReq=%u, OutPixReq=%u",
             u32Ret, stBufReq.input, stBufReq.output);
        u32Ret = VPP_ERR_HW;
        goto ERR_GET_BUF_REQ;
    }
    pstCb->u32FwBufInPixReq = stBufReq.input;
    pstCb->u32FwBufInMbiReq = stBufReq.input_meta;
    pstCb->u32FwBufOutPixReq = stBufReq.output;
    LOGI("FW buffer requirements: In=%u, MBI=%u, Out=%u", pstCb->u32FwBufInPixReq,
         pstCb->u32FwBufInMbiReq, pstCb->u32FwBufOutPixReq);

    u32TotalInBufCnt = pstCb->u32FwBufInPixReq + pstCb->u32FwBufInMbiReq;

    u32Ret = u32VppIpHvxCore_BuffInInit(pstCb->pstHvxCore, u32TotalInBufCnt);
    if (u32Ret != VPP_OK)
    {
        LOGE("Error allocating bufferIn pixel_data u32Ret=%u", u32Ret);
        goto ERR_ALLOC_INPUT;
    }

    u32Ret = u32VppIpHvxCore_BuffOutInit(pstCb->pstHvxCore, pstCb->u32FwBufOutPixReq);
    if (u32Ret != VPP_OK)
    {
        LOGE("Error allocating bufferOut pixel_data u32Ret=%u", u32Ret);
        goto ERR_ALLOC_OUTPUT;
    }

    if (pstCb->stCfg.u32ComputeMask)
        vVppIpFrcMc_Compute(pstCb);
    u32VppIpHvxCore_SvcParamSetROI(pstCb->pstHvxCore, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (pstCb->u32FwBufInPixReq > 1)
    {
        pstCb->ppstBufPeekPix = calloc((pstCb->u32FwBufInPixReq - 1), sizeof(t_StVppBuf*));
        if (!pstCb->ppstBufPeekPix)
        {
            LOGE("Error allocating peek pixel buffer memory!");
            u32Ret = VPP_ERR_NO_MEM;
            goto ERR_BUF_PIX_ALLOC;
        }
    }

    if (pstCb->u32FwBufInMbiReq > 1)
    {
        pstCb->ppstBufPeekMbi = calloc((pstCb->u32FwBufInMbiReq - 1), sizeof(t_StVppBuf*));
        if (!pstCb->ppstBufPeekMbi)
        {
            LOGE("Error allocating peek mbi buffer memory!");
            u32Ret = VPP_ERR_NO_MEM;
            goto ERR_BUF_MBI_ALLOC;
        }
    }

    pstCb->ppstBufOut = calloc((pstCb->u32FwBufOutPixReq), sizeof(t_StVppBuf*));
    if (!pstCb->ppstBufOut)
    {
        LOGE("Error allocating output buffer memory!");
        u32Ret = VPP_ERR_NO_MEM;
        goto ERR_BUF_OUT_ALLOC;
    }

    pstCb->ppstBufFwIn = calloc((pstCb->u32FwBufInPixReq), sizeof(t_StVppBuf*));
    if (!pstCb->ppstBufFwIn)
    {
        LOGE("Error allocating firmware input buffer memory!");
        u32Ret = VPP_ERR_NO_MEM;
        goto ERR_BUF_FW_IN_ALLOC;
    }

    pstCb->ppstBufFwOut = calloc((pstCb->u32FwBufOutPixReq), sizeof(t_StVppBuf*));
    if (!pstCb->ppstBufFwOut)
    {
        LOGE("Error allocating firmware output buffer memory!");
        u32Ret = VPP_ERR_NO_MEM;
        goto ERR_BUF_FW_OUT_ALLOC;
    }

#ifdef VPP_TARGET_USES_C2D
    if (pstCb->bNeedsC2D)
    {
        u32Ret = u32VppIpFrcMc_InitC2D(pstCb);
        if (u32Ret != VPP_OK)
        {
            LOGE("Error Initializing C2D u32Ret=%u", u32Ret);
            goto ERR_C2D_INIT;
        }
    }
#endif
    // Not fatal if tunings fail; Log and continue
    u32Ret = u32VppIpHvxCore_TuningsBoot(pstCb->pstHvxCore, FRCMC_TUNINGS_FILE_NAME,
                                         u32VppProcFlags, VPP_FALSE);
    if (u32Ret != VPP_OK)
    {
        LOGE("Could not boot FRC tunings, u32Ret=%u", u32Ret);
    }
    else
    {
        u32Ret = u32VppIpHvxCore_TuningsAlgoLoad(pstCb->pstHvxCore, VPP_FUNC_ID_FRC);
        LOGE_IF(u32Ret != VPP_OK, "No FRC tunings loaded, u32Ret=%u", u32Ret);
    }

    u32Ret = u32VppIpHvxCore_TuningsAlgoRegister(pstCb->pstHvxCore, VPP_FUNC_ID_FRC);
    LOGE_IF(u32Ret != VPP_OK, "Error registering FRC tunings session, u32Ret=%u", u32Ret);

    pthread_mutex_unlock(&pstCb->mutex);

    VPP_IP_STATE_SET(pstCb, VPP_IP_STATE_ACTIVE);

    LOGI("u32VppIpFrcMc_ProcCmdOpen() posting semaphore");
    sem_post(&pstCb->sem);

    pstCb->async_res.u32OpenRet = VPP_OK;
    return pstCb->async_res.u32OpenRet;

#ifdef VPP_TARGET_USES_C2D
ERR_C2D_INIT:
    if (pstCb->ppstBufFwOut)
    {
        free(pstCb->ppstBufFwOut);
        pstCb->ppstBufFwOut = NULL;
    }
#endif

ERR_BUF_FW_OUT_ALLOC:
    if (pstCb->ppstBufFwIn)
    {
        free(pstCb->ppstBufFwIn);
        pstCb->ppstBufFwIn = NULL;
    }

ERR_BUF_FW_IN_ALLOC:
    if (pstCb->ppstBufOut)
    {
        free(pstCb->ppstBufOut);
        pstCb->ppstBufOut = NULL;
    }

ERR_BUF_OUT_ALLOC:
    if (pstCb->ppstBufPeekMbi)
    {
        free(pstCb->ppstBufPeekMbi);
        pstCb->ppstBufPeekMbi = NULL;
    }

ERR_BUF_MBI_ALLOC:
    if (pstCb->ppstBufPeekPix)
    {
        free(pstCb->ppstBufPeekPix);
        pstCb->ppstBufPeekPix = NULL;
    }

ERR_BUF_PIX_ALLOC:
    vVppIpHvxCore_BuffOutTerm(pstCb->pstHvxCore);

ERR_ALLOC_OUTPUT:
    vVppIpHvxCore_BuffInTerm(pstCb->pstHvxCore);

ERR_ALLOC_INPUT:
ERR_GET_BUF_REQ:
    u32VppIpHvxCore_Close(pstCb->pstHvxCore);

ERR_HVX_OPEN:
ERR_BUF_PARAM_INIT:
ERR_FORMAT:
    pthread_mutex_unlock(&pstCb->mutex);

    pstCb->async_res.u32OpenRet = u32Ret;

    LOGI("%s() posting semaphore (error)", __func__);
    sem_post(&pstCb->sem);

    return pstCb->async_res.u32OpenRet;
}

static uint32_t u32VppIpFrcMc_ProcCmdClose(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t u32Ret = VPP_OK;
    const uint32_t u32VppProcFlags = (0x01 << (VPP_FUNC_ID_FRC & FUNC_ID_MASK));

    LOGD("%s()", __func__);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        u32Ret = VPP_ERR_STATE;
    }

    u32Ret = u32VppIpHvxCore_TuningsAlgoUnregister(pstCb->pstHvxCore, VPP_FUNC_ID_FRC);
    LOGE_IF(u32Ret != VPP_OK, "Error unregistering FRC tunings session, u32Ret=%u", u32Ret);

    u32Ret = u32VppIpHvxCore_TuningsShutdown(pstCb->pstHvxCore, u32VppProcFlags);
    LOGD_IF(u32Ret != VPP_OK, "Could not shut down FRC tunings, u32Ret=%u", u32Ret);

#ifdef VPP_TARGET_USES_C2D
    if (pstCb->stC2DCb.u32AllocCnt)
    {
        u32Ret = u32VppIpFrcMc_FreeInternalC2DBuffers(pstCb);
        LOGE_IF(u32Ret != VPP_OK, "Error freeing internal C2D buffers u32Ret=%u", u32Ret);
    }
    vVppIpFrcMc_TermC2D(pstCb);
    pstCb->bNeedsC2D = VPP_FALSE;
#endif

    if (pstCb->ppstBufFwOut)
    {
        free(pstCb->ppstBufFwOut);
        pstCb->ppstBufFwOut = NULL;
    }

    if (pstCb->ppstBufFwIn)
    {
        free(pstCb->ppstBufFwIn);
        pstCb->ppstBufFwIn = NULL;
    }

    if (pstCb->ppstBufOut)
    {
        free(pstCb->ppstBufOut);
        pstCb->ppstBufOut = NULL;
    }

    if (pstCb->ppstBufPeekPix)
    {
        free(pstCb->ppstBufPeekPix);
        pstCb->ppstBufPeekPix = NULL;
    }

    if (pstCb->ppstBufPeekMbi)
    {
        free(pstCb->ppstBufPeekMbi);
        pstCb->ppstBufPeekMbi = NULL;
    }

    vVppIpHvxCore_BuffInTerm(pstCb->pstHvxCore);
    vVppIpHvxCore_BuffOutTerm(pstCb->pstHvxCore);

    u32Ret = u32VppIpHvxCore_Close(pstCb->pstHvxCore);
    if (u32Ret != VPP_OK)
        LOGE("HVX Core Close Fail u32Ret=%u", u32Ret);

    VPP_IP_STATE_SET(pstCb, VPP_IP_STATE_INITED);

    pstCb->async_res.u32CloseRet = u32Ret;

    LOGI("%s() posting semaphore", __func__);
    sem_post(&pstCb->sem);

    return pstCb->async_res.u32CloseRet;
}


static void vVppIpFrcMc_FlushMcPort(t_StVppIpFrcMcCb *pstCb, t_StVppIpPort *pstMcPort,
                                    enum vpp_port ePort)
{
    t_StVppBuf *pBuf;

    while (u32VppIp_PortBufGet(pstMcPort, &pBuf, &pstCb->mutex) == VPP_OK)
    {
        VPP_FLAG_SET(pBuf->u32InternalFlags, VPP_BUF_FLAG_FLUSHED);
        if (ePort == VPP_PORT_OUTPUT)
            pBuf->stPixel.u32FilledLen = 0;
        vVppIpCbLog(&pstCb->stBase.stCb, pBuf, eVppLogId_IpBufDone);
        u32VppIpFrcMc_ReturnBuffer(pstCb, ePort, pBuf);
    }

}

static uint32_t u32VppIpFrcMc_FlushPort(t_StVppIpFrcMcCb *pstCb, enum vpp_port ePort)
{
    if (ePort == VPP_PORT_INPUT)
    {
        // Flush MBI buf and input buffers
        vVppIpFrcMc_FlushMcPort(pstCb, &pstCb->stMbi, ePort);
        vVppIpFrcMc_FlushMcPort(pstCb, &pstCb->stInput, ePort);

        LOGI("Resetting state after input flush");
        vVppIpFrcMc_StateSet(pstCb, FRCMC_STATE_ACTIVE_START);
        return VPP_OK;
    }
    else if (ePort == VPP_PORT_OUTPUT)
    {
        vVppIpFrcMc_FlushMcPort(pstCb, &pstCb->stOutput, ePort);
        return VPP_OK;
    }
    else
    {
        LOGE("%s(): ePort Not correct", __func__);
        return VPP_ERR_PARAM;
    }
}

static uint32_t u32VppIpFrcMc_ProcCmdFlush(t_StVppIpFrcMcCb *pstCb,
                                           t_StVppIpCmd *pstCmd)
{
    uint32_t u32;
    t_StVppEvt stEvt;

    // Flush Port
    u32 = u32VppIpFrcMc_FlushPort(pstCb, pstCmd->flush.ePort);

    LOGI("%s() u32VppIpFrcMc_FlushPort ret = %d", __func__, u32);

    if (u32 == VPP_OK)
    {
        pthread_mutex_lock(&pstCb->mutex);
        // If drain was pending, return drain done event first
        if (pstCmd->flush.ePort == VPP_PORT_INPUT &&
            VPP_FLAG_IS_SET(pstCb->u32InternalFlags, IP_DRAIN_PENDING))
        {
            VPP_FLAG_CLR(pstCb->u32InternalFlags, IP_DRAIN_PENDING);
            pthread_mutex_unlock(&pstCb->mutex);
            stEvt.eType = VPP_EVT_DRAIN_DONE;
            u32VppIpCbEvent(&pstCb->stBase.stCb, stEvt);
        }
        else
        {
            pthread_mutex_unlock(&pstCb->mutex);
        }

        stEvt.eType = VPP_EVT_FLUSH_DONE;
        stEvt.flush.ePort = pstCmd->flush.ePort;
        u32VppIpCbEvent(&pstCb->stBase.stCb, stEvt);
    }

    return u32;
}

static uint32_t u32VppIpFrcMc_ProcCmdDrain(t_StVppIpFrcMcCb *pstCb)
{
    t_StVppEvt stEvt;

    pthread_mutex_lock(&pstCb->mutex);

    if (u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ) == 0)
    {
        pthread_mutex_unlock(&pstCb->mutex);
        // No more input pixel buffers.
        // There shouldn't be any, but flush any remaining MBI buffers
        u32VppIpFrcMc_FlushPort(pstCb, VPP_PORT_INPUT);
        // Drain complete
        stEvt.eType = VPP_EVT_DRAIN_DONE;
        u32VppIpCbEvent(&pstCb->stBase.stCb, stEvt);
    }
    else
    {
        VPP_FLAG_SET(pstCb->u32InternalFlags, IP_DRAIN_PENDING);
        pthread_mutex_unlock(&pstCb->mutex);
    }

    return VPP_OK;
}

static uint32_t u32VppIpFrcMc_ProcCmdReconfigure(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t u32;
    uint32_t u32Ret = VPP_OK;

    if (!pstCb)
        return VPP_ERR_PARAM;

#ifdef VPP_TARGET_USES_C2D
    u32 = u32VppIpFrcMc_ReconfigureC2D(pstCb);
    if (u32 != VPP_OK)
    {
        LOGE("ERROR: ReconfigureC2D returned err, u32=%u", u32);
        u32Ret = VPP_ERR;
        goto ERR_RECONFIG;
    }
#endif

    u32 = u32VppIpHvxCore_PrepareCtx(pstCb->pstHvxCore);
    if (u32 != VPP_OK)
    {
        LOGE("ERROR: PrepareCtx returned err, u32=%u", u32);
        u32Ret = VPP_ERR;
    }

#ifdef VPP_TARGET_USES_C2D
ERR_RECONFIG:
#endif

    pstCb->async_res.u32ReconfigRet = u32Ret;
    LOGI("%s() posting semaphore (reconfig)", __func__);
    sem_post(&pstCb->sem);

    return u32Ret;
}

static void vVppIpFrcMc_ReturnOutputBuffer(t_StVppIpFrcMcCb *pstCb, t_StVppBuf *pstBufOut)
{
    LOG_ENTER();
    if (pstBufOut)
    {
        if (pstBufOut->eQueuedPort == VPP_PORT_OUTPUT)
        {
            FRCMC_DUMP_FRAME_DESC(pstCb, pstBufOut, "out", VPP_PORT_OUTPUT);
        }
        else
        {
            FRCMC_DUMP_FRAME_DESC(pstCb, pstBufOut, "in", VPP_PORT_OUTPUT);
        }
#ifdef FRCMC_DUMP_FRAME_ENABLE
        if ((VPP_FLAG_IS_SET(pstBufOut->u32InternalFlags, VPP_BUF_FLAG_DUMP)) &&
            (pstBufOut->stPixel.u32FilledLen != 0))
        {
            vVppIpFrcMc_DumpFrame(pstCb, pstBufOut, VPP_PORT_OUTPUT);
        }
#endif
        VPP_FLAG_CLR(pstBufOut->u32InternalFlags, VPP_BUF_FLAG_FLUSHED);
        vVppIpCbLog(&pstCb->stBase.stCb, pstBufOut, eVppLogId_IpBufDone);

        u32VppIpFrcMc_ReturnBuffer(pstCb, VPP_PORT_OUTPUT, pstBufOut);
    }
}

//Utility function for u32VppIpFrcMc_ProcessBuffer()
static void vVppIpFrcMc_ReturnBuffers(t_StVppIpFrcMcCb *pstCb,
                                      t_StVppBuf **ppstBufIn,
                                      uint32_t u32InCnt,
                                      t_StVppBuf **ppstBufMbi,
                                      uint32_t u32MbiCnt,
                                      t_StVppBuf **ppstBufOut,
                                      uint32_t u32OutCnt,
                                      uint32_t bBypassInput,
                                      uint32_t u32BufOutFilledLength)
{
    LOG_ENTER();
    uint32_t i;

    if (ppstBufIn && u32InCnt)
    {
        for (i = 0; i < u32InCnt; i++)
        {
            if (!ppstBufIn[i])
                continue;
            if (bBypassInput)
                vVppIpFrcMc_ReturnOutputBuffer(pstCb, ppstBufIn[i]);
            else
                u32VppIpFrcMc_ReturnBuffer(pstCb, VPP_PORT_INPUT, ppstBufIn[i]);
        }
    }
    if (ppstBufMbi && u32MbiCnt)
    {
        for (i = 0; i < u32MbiCnt; i++)
        {
            if (!ppstBufMbi[i])
                continue;
            vVppIpCbLog(&pstCb->stBase.stCb, ppstBufMbi[i], eVppLogId_IpBufDone);
            u32VppIpFrcMc_ReturnBuffer(pstCb, VPP_PORT_INPUT, ppstBufMbi[i]);
        }
    }
    if (ppstBufOut && u32OutCnt)
    {
        for (i = 0; i < u32OutCnt; i++)
        {
            if (!ppstBufOut[i])
                continue;
            ppstBufOut[i]->stPixel.u32FilledLen = u32BufOutFilledLength;
            vVppIpFrcMc_ReturnOutputBuffer(pstCb, ppstBufOut[i]);
        }
    }
}

static void vVppIpFrcMc_PrepareInBuffer(t_StVppIpHvxCoreCb *pstHvxCore, t_StVppBuf *pstBufIn,
                                        uint32_t u32Idx, uint32_t u32BufSize,
                                        uint32_t bIsUbwc)
{
    VPP_RET_VOID_IF_NULL(pstHvxCore);
    VPP_RET_VOID_IF_NULL(pstBufIn);

    FRCMC_REG_BUF(pstHvxCore, pstBufIn);
    u32VppIpHvxCore_BufInSetUserDataLen(pstHvxCore, u32Idx, u32BufSize);
    u32VppIpHvxCore_BufInSetUserDataAddr(pstHvxCore, u32Idx, pstBufIn->stPixel.pvBase);
    if (bIsUbwc)
    {
        u32VppIpHvxCore_BufInSetAttrUbwc(pstHvxCore, u32Idx,
                                         &pstBufIn->stUbwcStats[eVppUbwcStatField_P]);
    }
}

static uint32_t u32VppIpFrcMc_UpdateInterpFactor(t_StVppIpFrcMcCb *pstCb, uint32_t u32OperatingRate)
{
    uint32_t u32Res, u32MaxOutRate, u32MaxInterp;
    t_StVppIpFrcCtrlSegs *pstFrcSegs;
    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);

    u32Res = u32VppUtils_GetVppResolution(&pstCb->stInput.stParam);
    if (u32Res >= VPP_RESOLUTION_MAX)
    {
        LOGE("Error getting resolution, u32Res=%u", u32Res);
        return VPP_ERR;
    }

    pstFrcSegs = &pstCb->stCfg.stFrcCtrlSegs;
    if (pstFrcSegs->s32SegIdx < 0)
    {
        LOGE("Invalid segment index, s32SegIdx=%d", pstFrcSegs->s32SegIdx);
        return VPP_ERR;
    }

    u32MaxOutRate = au32FrcMcMaxSloMoOutRate[u32Res];
    u32MaxInterp = au32FrcMcMaxSloMoInterp[u32Res] <= pstCb->stInfo.u32FrcFactorReq ?
        au32FrcMcMaxSloMoInterp[u32Res] : pstCb->stInfo.u32FrcFactorReq;

    if (pstFrcSegs->pstFrcSegments[pstFrcSegs->s32SegIdx].mode == HQV_FRC_MODE_SLOMO)
    {
        if (pstCb->bNonRealtime)
        {
            // In non-realtime use case, don't need to downgrade the interp factor
            pstCb->stInfo.u32FrcFactorActual = pstCb->stInfo.u32FrcFactorReq;
            LOGD("FRC Factor=%u (Requested=%u). Non-realtime, allow requested interp factor",
                 pstCb->stInfo.u32FrcFactorActual, pstCb->stInfo.u32FrcFactorReq);
        }
        else
        {
            if (u32OperatingRate > u32MaxOutRate || u32OperatingRate == 0)
            {
                pstCb->stInfo.u32FrcFactorActual = 1;
                LOGD("Operating Rate=%u is invalid or exceeds Max=%u! Can't interpolate",
                     u32OperatingRate, u32MaxOutRate);
            }
            else
            {
                pstCb->stInfo.u32FrcFactorActual = u32MaxOutRate / u32OperatingRate;
                if (pstCb->stInfo.u32FrcFactorActual > u32MaxInterp)
                    pstCb->stInfo.u32FrcFactorActual = u32MaxInterp;
            }
            LOGD("FRC Factor=%u (Requested=%u Max=%u). Operating Rate=%u, Max Output=%u",
                 pstCb->stInfo.u32FrcFactorActual, pstCb->stInfo.u32FrcFactorReq,
                 au32FrcMcMaxSloMoInterp[u32Res], u32OperatingRate, u32MaxOutRate);
        }
        pstCb->stInfo.u32InterpFrames = pstCb->stInfo.u32FrcFactorActual - 1;
        pstCb->pstFrcParams->NUM_INTERP_FRAMES = pstCb->stInfo.u32InterpFrames;
        pstCb->pstFrcParams->update_flags = 1;
    }

    return VPP_OK;
}

static int32_t s32VppIpFrcMc_GetCtrlSegIdx(t_StVppIpFrcMcCb *pstCb)
{
    t_StVppBuf *pstBufInPeek;
    uint64_t u64Ts;
    int32_t s32SegIdx = -1;
    t_StVppIpFrcCtrlSegs *pstCtrlSegs;
    uint32_t i;

    VPP_RET_IF_NULL(pstCb, s32SegIdx);

    pstBufInPeek = pstVppIp_PortBufPeek(&pstCb->stInput, 0, NULL);
    if (!pstBufInPeek)
    {
        LOGE("Error peeking input buffer!");
        return s32SegIdx;
    }
    u64Ts = pstBufInPeek->pBuf->timestamp;

    pstCtrlSegs = &pstCb->stCfg.stFrcCtrlSegs;
    for (i = 0; i < pstCtrlSegs->u32SegCnt; i++)
    {
        if (u64Ts < (pstCtrlSegs->pstFrcSegments[i].ts_start))
        {
            if (i == 0)
            {
                LOGE("Buffer ts=%"PRIu64" lower than first segment ts=%"PRIu64", will bypass",
                     u64Ts, (pstCtrlSegs->pstFrcSegments[i].ts_start));
            }
            break;
        }
        else
        {
            s32SegIdx = (int32_t)i;
        }
    }
    LOGI("Using FRC control segment %d for buffer ts=%"PRIu64, s32SegIdx, u64Ts);
    return s32SegIdx;
}

static void vVppIpFrcMc_FillOutputBufData(t_StVppIpFrcMcCb *pstCb, t_StVppBuf *pstBufSrc,
                                          t_StVppBuf *pstBufDest, uint64_t u64Timestamp)
{
    VPP_RET_VOID_IF_NULL(pstBufDest);

    if (!pstBufSrc)
    {
        LOGE("No source buffer to propagate gralloc and extradata, skipping!");
    }
    else
    {
        u32VppBuf_PropagateExtradata(pstBufSrc, pstBufDest,
                                     eVppBufType_Progressive,
                                     VPP_EXTERNAL_EXTRADATA_TYPE);
        u32VppBuf_GrallocMetadataCopy(pstBufSrc, pstBufDest);
        // In interp state, source buffer framerate hasn't been updated yet
        if (pstCb->eFrcMcState == FRCMC_STATE_ACTIVE_INTERP)
            u32VppBuf_GrallocFramerateMultiply(pstBufDest,
                                               pstCb->stInfo.u32FrcFactorActual);
        pstBufDest->pBuf->cookie_in_to_out = pstBufSrc->pBuf->cookie_in_to_out;
    }
    pstBufDest->pBuf->timestamp = u64Timestamp;
    pstBufDest->u32TimestampFrameRate = pstCb->stInfo.u32TimestampFrameRate;
    pstBufDest->u32OperatingRate = pstCb->stInfo.u32OperatingRate;
}

static uint32_t u32VppIpFrcMc_ProcessBuffer(t_StVppIpFrcMcCb *pstCb)
{
    uint32_t u32Ret = VPP_OK;
    t_StVppBuf *pstBufInPull, *pstBufMbi, *pstBufTemp;
    t_StVppBuf **ppstBufInPeek, **ppstBufMbiPeek, **ppstBufFwIn, **ppstBufFwOut, **ppstBufOut;
    t_StVppIpHvxCoreCb *pstHvxCore;
    t_StVppIpFrcCtrlSegs *pstCtrlSegs;
    struct video_property stVidProp;
    uint32_t u32BufSz, u32PlSize, u32InBufReq, u32InPeekReq, u32InMbiReq, u32;
    uint32_t bIsFmtUbwc, bNonRealtime;
    uint32_t u32OutBufSz, u32Res, u32BufPeekIdx, u32MbiBufIndex, i;
    uint32_t u32MaxOutRate = 0, u32MaxInterp = 0, u32OutBufReq = 0, u32BufCount = 0;
    uint32_t bEnableBypass = 0;
    uint32_t bFrameCopyInput = 0;
    uint64_t u64TimestampDelta;
    int32_t s32CtrlSeg;
#ifdef VPP_TARGET_USES_C2D
    uint32_t bNeedsC2D;
#endif

    LOGI("%s(), Context: %p", __func__, pstCb);

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);

    pstHvxCore = pstCb->pstHvxCore;
    u32OutBufSz = u32VppUtils_GetPxBufferSize(&pstCb->stOutput.stParam);
    pstBufInPull = pstBufMbi = NULL;
#ifdef VPP_TARGET_USES_C2D
    bNeedsC2D = pstCb->bNeedsC2D;
    bIsFmtUbwc = bNeedsC2D ? VPP_FALSE : u32VppUtils_IsFmtUbwc(pstCb->stInput.stParam.fmt);
#else
    bIsFmtUbwc = u32VppUtils_IsFmtUbwc(pstCb->stInput.stParam.fmt);
#endif
    u32Res = u32VppUtils_GetVppResolution(&pstCb->stInput.stParam);
    u32InBufReq = pstCb->u32FwBufInPixReq;
    u32InPeekReq = pstCb->u32FwBufInPixReq - 1;
    u32InMbiReq = pstCb->u32FwBufInMbiReq;
    ppstBufInPeek = pstCb->ppstBufPeekPix;
    ppstBufMbiPeek = pstCb->ppstBufPeekMbi;
    ppstBufOut = pstCb->ppstBufOut;
    ppstBufFwIn = pstCb->ppstBufFwIn;
    ppstBufFwOut = pstCb->ppstBufFwOut;
    u32MbiBufIndex = pstCb->u32FwBufInPixReq;
    pstCtrlSegs = &pstCb->stCfg.stFrcCtrlSegs;

    stVidProp.property_type = VID_PROP_NON_REALTIME;
    u32 = u32VppUtils_GetVidProp(pstCb->stBase.pstCtx, &stVidProp);
    if (u32 == VPP_OK)
    {
        bNonRealtime = stVidProp.non_realtime.bNonRealtime;
    }
    else
    {
        // Not fatal, just assume realtime if call failed
        bNonRealtime = VPP_FALSE;
        LOGE("Error getting VID_PROP_NON_REALTIME property, using realtime, u32=%u", u32);
    }

    if (pstCb->bNonRealtime != bNonRealtime)
    {
        LOGI("Changing HVX clock for bNonRealtime=%u", bNonRealtime);
        if (bNonRealtime)
            u32 = u32VppIpHvxCore_SetClock(pstHvxCore, HVX_CORE_CLK_TURBO);
        else
            u32 = u32VppIpHvxCore_SetClock(pstHvxCore, HVX_CORE_CLK_FW_DEFAULT);

        if (u32 == VPP_OK)
            pstCb->bNonRealtime = bNonRealtime;
        else
            LOGE("Error setting HVX clock, u32=%u", u32);
    }

    if (u32Res >= VPP_RESOLUTION_MAX)
    {
        LOGE("Error getting resolution, u32Res=%u. Enabling bypass", u32Res);
        bEnableBypass = 1;
    }
    else
    {
        u32MaxOutRate = au32FrcMcMaxSloMoOutRate[u32Res];
        u32MaxInterp = au32FrcMcMaxSloMoInterp[u32Res];
    }

    pthread_mutex_lock(&pstCb->mutex);
    if (pstCb->eFrcMcState == FRCMC_STATE_NULL)
        vVppIpFrcMc_StateSet(pstCb, FRCMC_STATE_ACTIVE_START);

    // Skip bypass and error buffer checks if in middle of interpolation since no new buffers needed
    if (pstCb->eFrcMcState != FRCMC_STATE_ACTIVE_INTERP)
    {
        if (bVppIpFrcMc_CanReduceInBufReq(pstCb, &u32InBufReq) == VPP_TRUE)
        {
            if (u32InBufReq)
            {
                u32InPeekReq = u32InBufReq - 1;
            }
            else
            {
                LOGE("Processing state needs at least 1 buffers, but u32InBufReq=%u", u32InBufReq);
                pthread_mutex_unlock(&pstCb->mutex);
                return VPP_ERR;
            }
            if (u32InMbiReq)
            {
                u32InMbiReq = u32InBufReq - 1;
            }
        }

        s32CtrlSeg = s32VppIpFrcMc_GetCtrlSegIdx(pstCb);
        if (s32CtrlSeg != pstCtrlSegs->s32SegIdx)
        {
            pstCb->stCfg.u32ComputeMask |= FRC_ALGO_MC;
            pstCtrlSegs->s32SegIdx = s32CtrlSeg;
        }

        if (pstCb->stCfg.u32ComputeMask)
            vVppIpFrcMc_Compute(pstCb);

        if (bVppIpFrcMc_BypassBufCheck(pstCb) == VPP_TRUE)
        {
            LOGI("bVppIpFrcMc_BypassBufCheck returned true. Bypassing input buffer to output");
            bEnableBypass = 1;
        }
        else if (u32InMbiReq)
        {
            // If not bypass, peek at Input and MBI buffers to make sure timestamps match
            pstBufInPull = pstVppIp_PortBufPeek(&pstCb->stInput, 0, NULL);
            pstBufMbi = pstVppIp_PortBufPeek(&pstCb->stMbi, 0, NULL);
            if (pstBufInPull == NULL || pstBufMbi == NULL)
            {
                LOGE("Error peeking at Input %p and MBI %p buffers", pstBufInPull, pstBufMbi);
                pthread_mutex_unlock(&pstCb->mutex);
                return VPP_ERR;
            }
            if (pstBufInPull->pBuf->timestamp != pstBufMbi->pBuf->timestamp)
            {
                // Error case, return the lower timestamp to try to catch up
                if (pstBufInPull->pBuf->timestamp > pstBufMbi->pBuf->timestamp)
                {
                    LOGE("Input timestamp %"PRIu64" greater than MBI %"PRIu64", returning MBI",
                         pstBufInPull->pBuf->timestamp, pstBufMbi->pBuf->timestamp);
                    pstBufInPull = NULL;
                    u32Ret = u32VppIp_PortBufGet(&pstCb->stMbi, &pstBufMbi, NULL);
                    if ((pstBufMbi == NULL) || (u32Ret != VPP_OK))
                    {
                        LOGE("Error getting MBI buffer. pstBufMbi=%p, u32Ret=%u",
                             pstBufMbi, u32Ret);
                        pthread_mutex_unlock(&pstCb->mutex);
                        return VPP_ERR;
                    }
                }
                else
                {
                    LOGE("Input timestamp %"PRIu64" less than MBI %"PRIu64", bypassing input",
                         pstBufInPull->pBuf->timestamp, pstBufMbi->pBuf->timestamp);
                    pstBufMbi = NULL;
                    u32Ret = u32VppIp_PortBufGet(&pstCb->stInput, &pstBufInPull, NULL);
                    if (pstBufInPull == NULL || u32Ret != VPP_OK)
                    {
                        LOGE("u32VppIp_PortBufGet() error. pstBufIn=%p, u32Ret=%u",
                             pstBufInPull, u32Ret);
                        pthread_mutex_unlock(&pstCb->mutex);
                        return VPP_ERR;
                    }
                    if (pstBufInPull->eBufType == eVppBufType_Progressive)
                        u32VppBuf_GrallocFramerateMultiply(pstBufInPull,
                                                           pstCb->stInfo.u32FrcFactorActual);
                }
                goto ERROR_PROCESS;
            }
        }
    }

    if (bEnableBypass)
    {
        // Get buffer to bypass
        u32Ret = u32VppIp_PortBufGet(&pstCb->stInput, &pstBufInPull, NULL);
        if (pstBufInPull == NULL || u32Ret != VPP_OK)
        {
            pthread_mutex_unlock(&pstCb->mutex);

            LOGE("u32VppIp_PortBufGet() error. pstBufIn=%p, u32Ret=%u", pstBufInPull, u32Ret);
            return VPP_ERR;
        }

        vVppIpFrcMc_StateSet(pstCb, FRCMC_STATE_ACTIVE_START);

        if (pstBufInPull->eBufType == eVppBufType_Progressive)
            u32VppBuf_GrallocFramerateMultiply(pstBufInPull, pstCb->stInfo.u32FrcFactorActual);

        pthread_mutex_unlock(&pstCb->mutex);
    }
    else
    {
        VPP_IP_PROF_START(&pstCb->stBase, FRCMC_STAT_TOTAL_PROC);

        u32VppIpHvxCore_BufParamGetPlaneTotalSize(pstHvxCore, &u32BufSz);
        for (i = 0; i < (pstCb->u32FwBufInPixReq + pstCb->u32FwBufInMbiReq); i++)
        {
            u32VppIpHvxCore_BufInSetUserDataLen(pstHvxCore, i, 0);
            u32VppIpHvxCore_BufInSetUserDataAddr(pstHvxCore, i, NULL);
            u32VppIpHvxCore_BufInSetAttrUbwc(pstHvxCore, i, NULL);
        }

        bEnableBypass = bVppIpFrcMc_AlgoCtrlsCanBypass(pstCb);
        pstBufInPull = pstBufMbi = NULL;
        memset(ppstBufOut, 0, sizeof(t_StVppBuf*) * (pstCb->u32FwBufOutPixReq));
        if (pstCb->eFrcMcState == FRCMC_STATE_ACTIVE ||
            pstCb->eFrcMcState == FRCMC_STATE_ACTIVE_START)
        {
            if (pstCb->u32FwBufInPixReq > 1)
                memset(ppstBufInPeek, 0, sizeof(t_StVppBuf*) * (pstCb->u32FwBufInPixReq - 1));
            if (pstCb->u32FwBufInMbiReq > 1)
                memset(ppstBufMbiPeek, 0, sizeof(t_StVppBuf*) * (pstCb->u32FwBufInMbiReq - 1));

            u32Ret = u32VppIp_PortBufGet(&pstCb->stInput, &pstBufInPull, NULL);
            if (pstBufInPull == NULL || u32Ret != VPP_OK)
            {
                VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_TOTAL_PROC);
                pthread_mutex_unlock(&pstCb->mutex);

                LOGE("u32VppIp_PortBufGet() error. pstBufIn=%p, u32Ret=%u", pstBufInPull, u32Ret);
                return VPP_ERR;
            }

            if (!bEnableBypass)
            {
                for (i = 0; i < u32InPeekReq; i++)
                {
                    ppstBufInPeek[i] = pstVppIp_PortBufPeek(&pstCb->stInput, i, NULL);
                    if (!ppstBufInPeek[i])
                    {
                        LOGE("Error peeking input buffer %u, bypass", i);
                        goto ERROR_PROCESS_PROF_STOP;
                    }
                }
            }

            if (u32InMbiReq)
            {
                // Get MBI buffer
                u32Ret = u32VppIp_PortBufGet(&pstCb->stMbi, &pstBufMbi, NULL);
                if (u32Ret != VPP_OK || pstBufMbi == NULL)
                {
                    LOGE("Error getting MBI buffers, bypass input");
                    goto ERROR_PROCESS_PROF_STOP;
                }
                if (u32InMbiReq > 1 && !bEnableBypass)
                {
                    for (i = 0; i < u32InMbiReq - 1; i++)
                    {
                        ppstBufMbiPeek[i] = pstVppIp_PortBufPeek(&pstCb->stMbi, i, NULL);
                        if (!ppstBufMbiPeek[i])
                        {
                            LOGE("Error peeking MBI buffer %u, bypass", i);
                            goto ERROR_PROCESS_PROF_STOP;
                        }
                    }
                }
            }
            // If FRCMC not enabled, bypass pixel and return MBI buffers
            if (bEnableBypass || pstCtrlSegs->s32SegIdx < 0)
            {
                LOGD("Settings result in no processing: bypass input and return MBI");
                goto ERROR_PROCESS_PROF_STOP;
            }
            if (pstCtrlSegs->pstFrcSegments[pstCtrlSegs->s32SegIdx].mode ==
                HQV_FRC_MODE_SLOMO &&
                (pstCb->stInfo.u32FrcFactorActual != pstCb->stInfo.u32FrcFactorReq ||
                 (pstCb->bNonRealtime == VPP_FALSE &&
                  (pstCb->stInfo.u32FrcFactorActual > u32MaxInterp ||
                   pstCb->stInfo.u32FrcFactorActual * pstBufInPull->u32OperatingRate >
                   u32MaxOutRate))))
            {
                // Update interp rate in SloMo when current setting doesn't equal requested setting
                // OR in realtime processing, if output rate exceeds max supported
                u32Ret = u32VppIpFrcMc_UpdateInterpFactor(pstCb, pstBufInPull->u32OperatingRate);
                if (u32Ret != VPP_OK)
                {
                    LOGE("Error updating interpolation rate, bypass input");
                    goto ERROR_PROCESS_PROF_STOP;
                }
                if (pstCb->stInfo.u32FrcFactorActual <= 1)
                {
                    LOGD("Operating Rate %u too high to interpolate (Max output rate = %u)",
                         pstBufInPull->u32OperatingRate, u32MaxOutRate);
                    goto ERROR_PROCESS_PROF_STOP;
                }
            }

            if (pstCb->eFrcMcState == FRCMC_STATE_ACTIVE_START)
            {
#ifdef VPP_TARGET_USES_C2D
                if (bNeedsC2D)
                {
                    ppstBufFwIn[u32BufCount] = &pstCb->stC2DCb.stC2DBufs.pstVppBuf[u32BufCount];

                    VPP_IP_PROF_START(&pstCb->stBase, FRCMC_STAT_C2D_IN_PROC);
                    u32Ret = u32VppIpC2D_InlineProcess(pstCb->stC2DCb.vpCtxUbwcToLin,
                                                       pstBufInPull, ppstBufFwIn[u32BufCount]);
                    VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_C2D_IN_PROC);
                    if (u32Ret != VPP_OK)
                    {
                        LOGE("C2D error ppstBufFwIn[%u], u32Ret=%u", u32BufCount, u32Ret);
                        goto ERROR_PROCESS_PROF_STOP;
                    }
                }
                else
#endif
                {
                    ppstBufFwIn[u32BufCount] = pstBufInPull;
                }

                vVppIpFrcMc_PrepareInBuffer(pstHvxCore, ppstBufFwIn[u32BufCount], u32BufCount,
                                            u32BufSz, bIsFmtUbwc);
                u32BufCount++;
                if (u32InPeekReq)
                {
                    for (u32BufPeekIdx = 0; u32BufPeekIdx < u32InPeekReq - 1; u32BufPeekIdx++)
                    {
#ifdef VPP_TARGET_USES_C2D
                        if (bNeedsC2D)
                        {
                            ppstBufFwIn[u32BufCount] =
                                &pstCb->stC2DCb.stC2DBufs.pstVppBuf[u32BufCount];
                            VPP_IP_PROF_START(&pstCb->stBase, FRCMC_STAT_C2D_IN_PROC);
                            u32Ret = u32VppIpC2D_InlineProcess(pstCb->stC2DCb.vpCtxUbwcToLin,
                                                               ppstBufInPeek[u32BufPeekIdx],
                                                               ppstBufFwIn[u32BufCount]);
                            VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_C2D_IN_PROC);
                            if (u32Ret != VPP_OK)
                            {
                                LOGE("C2D error ppstBufFwIn[%u], u32Ret=%u", u32BufCount, u32Ret);
                                goto ERROR_PROCESS_PROF_STOP;
                            }
                        }
                        else
#endif
                        {
                            ppstBufFwIn[u32BufCount] = ppstBufInPeek[u32BufPeekIdx];
                        }
                        vVppIpFrcMc_PrepareInBuffer(pstHvxCore, ppstBufFwIn[u32BufCount],
                                                    u32BufCount, u32BufSz, bIsFmtUbwc);
                        u32BufCount++;
                    }
                }
            }

            if ((u32InPeekReq == pstCb->u32FwBufInPixReq - 1) ||
                (u32InPeekReq && pstCb->eFrcMcState == FRCMC_STATE_ACTIVE_START))
            {
                // In normal active state, only need to send the newest buffer since other buffers
                // have been sent previously. In reduced buffer processing (drain/EOS/invalid
                // buffers), firmware will already have all needed buffers unless in active
                // start state.
#ifdef VPP_TARGET_USES_C2D
                if (bNeedsC2D)
                {
                    ppstBufFwIn[u32BufCount] = &pstCb->stC2DCb.stC2DBufs.pstVppBuf[u32BufCount];
                    VPP_IP_PROF_START(&pstCb->stBase, FRCMC_STAT_C2D_IN_PROC);
                    u32Ret = u32VppIpC2D_InlineProcess(pstCb->stC2DCb.vpCtxUbwcToLin,
                                                       ppstBufInPeek[u32InPeekReq - 1],
                                                       ppstBufFwIn[u32BufCount]);
                    VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_C2D_IN_PROC);
                    if (u32Ret != VPP_OK)
                    {
                        LOGE("C2D error ppstBufFwIn[%u], u32Ret=%u", u32BufCount, u32Ret);
                        goto ERROR_PROCESS_PROF_STOP;
                    }
                }
                else
#endif
                {
                    ppstBufFwIn[u32BufCount] = ppstBufInPeek[u32InPeekReq - 1];
                }
                vVppIpFrcMc_PrepareInBuffer(pstHvxCore, ppstBufFwIn[u32BufCount],
                                            u32BufCount, u32BufSz, bIsFmtUbwc);
                u32BufCount++;
            }

            if (u32InMbiReq)
            {
                if (u32BufCount > u32MbiBufIndex)
                {
                    LOGE("Error MBI buffer index %u already used by pixel buffer!",
                         u32MbiBufIndex);
                    goto ERROR_PROCESS_PROF_STOP;
                }

                u32PlSize = pstBufMbi->stPixel.u32FilledLen;
                LOGI("MBI buf plane size = %u", u32PlSize);
                if (pstCb->eFrcMcState == FRCMC_STATE_ACTIVE_START)
                {
                    vVppIpFrcMc_PrepareInBuffer(pstHvxCore, pstBufMbi, u32MbiBufIndex,
                                                u32PlSize, VPP_FALSE);
                    u32VppIpHvxCore_BufInSetAttrSize(pstHvxCore, u32MbiBufIndex, &u32PlSize, 1);
                    if (u32InMbiReq > 1)
                    {
                        for (i = 0; i < u32InMbiReq - 1; i++)
                        {
                            vVppIpFrcMc_PrepareInBuffer(pstHvxCore, ppstBufMbiPeek[i],
                                                        u32MbiBufIndex + 1 + i,
                                                        u32PlSize, VPP_FALSE);
                            u32VppIpHvxCore_BufInSetAttrSize(pstHvxCore, u32MbiBufIndex + i + 1,
                                                             &u32PlSize, 1);
                        }
                    }
                }
                else if (u32InMbiReq == pstCb->u32FwBufInMbiReq)
                {
                    // Only need to send a new MBI if current requirements equal to the max
                    // required by firmware. In reduced buffer processing, firmware will
                    // already have all needed buffers.
                    if (u32InMbiReq == 1)
                    {
                        vVppIpFrcMc_PrepareInBuffer(pstHvxCore, pstBufMbi, u32MbiBufIndex,
                                                    u32PlSize, VPP_FALSE);
                        u32VppIpHvxCore_BufInSetAttrSize(pstHvxCore, u32MbiBufIndex,
                                                         &u32PlSize, 1);
                    }
                    else
                    {
                        // In normal active state, only need to send the newest MBI buffer
                        // since other buffers have been sent previously
                        vVppIpFrcMc_PrepareInBuffer(pstHvxCore,
                                                    ppstBufMbiPeek[u32InMbiReq - 2],
                                                    u32MbiBufIndex,
                                                    u32PlSize, VPP_FALSE);
                        u32VppIpHvxCore_BufInSetAttrSize(pstHvxCore, u32MbiBufIndex,
                                                         &u32PlSize, 1);
                    }
                }
            }

            pstCb->stInfo.u32InterpCnt = 0;
            pstCb->pstFrcParams->interp_cnt = 0;
        }
        // Get Output buffers
        for (i = 0; i < pstCb->u32FwBufOutPixReq; i++)
        {
            u32VppIpHvxCore_BufOutSetUserDataAddr(pstHvxCore, i, NULL);
            u32VppIpHvxCore_BufOutSetUserDataLen(pstHvxCore, i, 0);
        }

        u32OutBufReq = 1;
        s32CtrlSeg = pstCtrlSegs->s32SegIdx;
        if ((pstCb->eFrcMcState == FRCMC_STATE_ACTIVE ||
             pstCb->eFrcMcState == FRCMC_STATE_ACTIVE_START) &&
            (s32CtrlSeg >= 0) && (pstCtrlSegs->pstFrcSegments[s32CtrlSeg].frame_copy_input))
        {
            if (pstCb->u32FwBufOutPixReq > 1)
            {
                bFrameCopyInput = VPP_TRUE;
                if (u32InBufReq > 1)
                    u32OutBufReq = 2;
            }
            else
            {
                LOGE("Not copying input to output since FW only supports %u out buffers!",
                     pstCb->u32FwBufOutPixReq);
            }
        }
        for (i = 0; i < u32OutBufReq; i++)
        {
            u32Ret = u32VppIp_PortBufGet(&pstCb->stOutput, &ppstBufOut[i], NULL);
            if (u32Ret != VPP_OK || ppstBufOut[i] == NULL)
            {
                LOGE("Error getting output buffer %u, bypassing input, returning MBI", i);
                goto ERROR_PROCESS_PROF_STOP;
            }

            if (ppstBufOut[i]->stPixel.u32ValidLen < u32OutBufSz)
            {
                LOGE("Error out buffer %u size %u too small, need %u", i,
                     ppstBufOut[i]->stPixel.u32ValidLen, u32OutBufSz);
                goto ERROR_PROCESS_PROF_STOP;
            }
#ifdef VPP_TARGET_USES_C2D
            if (bNeedsC2D)
            {
                ppstBufFwOut[i] = &pstCb->stC2DCb.stC2DBufs.pstVppBuf[u32BufCount++];
            }
            else
#endif
            {
                ppstBufFwOut[i] = ppstBufOut[i];
            }
            FRCMC_REG_BUF(pstCb->pstHvxCore, ppstBufFwOut[i]);
            u32VppIpHvxCore_BufOutSetUserDataAddr(pstHvxCore, i,
                                                  (void *)(ppstBufFwOut[i]->stPixel.pvBase));
            u32VppIpHvxCore_BufOutSetUserDataLen(pstHvxCore, i, u32BufSz);
        }
        pthread_mutex_unlock(&pstCb->mutex);

        // These log time start calls are intentionally separated from the
        // RegisterBuffer calls, so that the RegisterBuffer calls do not skew the
        // times for the buffers. The time it takes to register a buffer should be
        // included in the overall processing time.
        FRCMC_CBLOG(&pstCb->stBase.stCb, pstBufInPull, eVppLogId_IpProcStart);
        FRCMC_CBLOG(&pstCb->stBase.stCb, ppstBufInPeek[0], eVppLogId_IpProcStart);
        for (i = 0; i < u32OutBufReq; i++)
        {
            FRCMC_CBLOG(&pstCb->stBase.stCb, ppstBufOut[i], eVppLogId_IpProcStart);
        }

#ifdef DUMP_PROCESSING_PARAMS
        if (VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, FRC_ALGO_MC))
            print_vpp_svc_frc_params(pstCb->pstFrcParams);
#endif

        LOGI("about to call vpp_svc_process, InProcCnt=%u, OutProcCnt=%u, eFrcMcState=%u",
             pstCb->stats.u32InProcCnt, pstCb->stats.u32OutProcCnt, pstCb->eFrcMcState);
        int rc = 0;
        VPP_IP_PROF_START(&pstCb->stBase, FRCMC_STAT_PROC);
        rc = VppIpHvxCore_Process(pstHvxCore);
        VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_PROC);

        LOGI("vpp_svc_process returned, rc=%d", rc);
        pstCb->stats.u32InProcCnt++;

        for (i = 0; i < u32InBufReq; i++)
        {
            FRCMC_UNREG_BUF(pstCb->pstHvxCore, ppstBufFwIn[i]);
        }
        FRCMC_UNREG_BUF(pstCb->pstHvxCore, pstBufMbi);
        if (u32InMbiReq > 1)
        {
            for (i = 0; i < u32InMbiReq - 1; i++)
            {
                FRCMC_UNREG_BUF(pstCb->pstHvxCore, ppstBufMbiPeek[i]);
            }
        }
        for (i = 0; i < u32OutBufReq; i++)
        {
            FRCMC_UNREG_BUF(pstCb->pstHvxCore, ppstBufFwOut[i]);
        }

        FRCMC_CBLOG(&pstCb->stBase.stCb, pstBufInPull, eVppLogId_IpProcDone);
        FRCMC_CBLOG(&pstCb->stBase.stCb, ppstBufInPeek[0], eVppLogId_IpProcDone);
        for (i = 0; i < u32OutBufReq; i++)
        {
            FRCMC_CBLOG(&pstCb->stBase.stCb, ppstBufOut[i], eVppLogId_IpProcDone);
        }
        if (pstBufInPull)
        {
            u32VppBuf_GrallocFramerateMultiply(pstBufInPull, pstCb->stInfo.u32FrcFactorActual);
            pstBufInPull->u32TimestampFrameRate *= pstCb->stInfo.u32FrcFactorActual;
            pstBufInPull->u32OperatingRate *= pstCb->stInfo.u32FrcFactorActual;
        }

        if (rc != AEE_SUCCESS)
        {
            pstCb->stInfo.u32InterpCnt = 0;
            if (rc == AEE_EVPP_FRMCPYOP)
            {
                // MC asks for frame repeat, can recycle unused output buffers
                LOGD("Frame repeat, no interp output generated");
                for (i = 0; i < u32OutBufReq; i++)
                {
                    if (bFrameCopyInput && (i == 0))
                    {
#ifdef VPP_TARGET_USES_C2D
                        if (bNeedsC2D)
                        {
                            VPP_IP_PROF_START(&pstCb->stBase, FRCMC_STAT_C2D_OUT_PROC);
                            u32Ret = u32VppIpC2D_InlineProcess(pstCb->stC2DCb.vpCtxLinToUbwc,
                                                               ppstBufFwOut[i], ppstBufOut[i]);
                            VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_C2D_OUT_PROC);
                        }
#endif
                        ppstBufOut[i]->pBuf->flags = pstBufInPull->pBuf->flags;
                        vVppIpFrcMc_FillOutputBufData(pstCb, pstBufInPull, ppstBufOut[i],
                                                      pstBufInPull->pBuf->timestamp);
                        continue;
                    }
                    u32VppIp_PortBufPut(&pstCb->stOutput, ppstBufOut[i],
                                        &pstCb->mutex, &pstCb->cond);
                    ppstBufOut[i] = NULL;
                }
                // Need to reset to start state if process was only for copying (EOS/drain case)
                if (bFrameCopyInput && (u32InBufReq == 1))
                    vVppIpFrcMc_StateSet(pstCb, FRCMC_STATE_ACTIVE_START);
                else
                    vVppIpFrcMc_StateSet(pstCb, FRCMC_STATE_ACTIVE);
            }
            else
            {
                u32Ret = VPP_ERR;
                LOGE("Error: compute on aDSP failed, return=%d\n",rc);
                vVppIpFrcMc_StateSet(pstCb, FRCMC_STATE_ACTIVE_START);
            }
        }
        else
        {
            LOGD("vpp_svc_process successful! u32InterpCnt=%u. Output buf size=%u",
                 pstCb->stInfo.u32InterpCnt, u32OutBufSz);
            pstCb->stats.u32OutProcCnt++;
            pstCb->stInfo.u32InterpCnt++;
            pstCb->pstFrcParams->update_flags = 0;

#ifdef VPP_TARGET_USES_C2D
            if (bNeedsC2D)
            {
                for (i = 0; i < u32OutBufReq; i++)
                {
                    VPP_IP_PROF_START(&pstCb->stBase, FRCMC_STAT_C2D_OUT_PROC);
                    u32Ret = u32VppIpC2D_InlineProcess(pstCb->stC2DCb.vpCtxLinToUbwc,
                                                       ppstBufFwOut[i], ppstBufOut[i]);
                    VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_C2D_OUT_PROC);
                    ppstBufOut[i]->pBuf->flags = 0;
                    if (u32Ret != VPP_OK)
                    {
                        LOGE("C2D error ppstBufFwOut[%u], u32Ret=%u", i, u32Ret);
                        goto ERROR_PROCESS_PROF_STOP;
                    }
                }
            }
#endif
            if (pstCb->eFrcMcState != FRCMC_STATE_ACTIVE_INTERP && ppstBufInPeek[0])
            {
                // Source buffer for gralloc, extradata, cookie
                pstBufTemp = pstBufInPull;

                if (ppstBufInPeek[0]->pBuf->timestamp >= pstBufInPull->pBuf->timestamp)
                {
                    u64TimestampDelta = ppstBufInPeek[0]->pBuf->timestamp -
                        pstBufInPull->pBuf->timestamp;
                }
                else
                {
                    // Should not get here since this should've resulted in bypass
                    u64TimestampDelta = 0;
                    LOGE("Can't calculate delta. Second buf ts=%" PRIu64 " < first buf ts=%" PRIu64,
                         ppstBufInPeek[0]->pBuf->timestamp, pstBufInPull->pBuf->timestamp);
                }
                u64TimestampDelta /= pstCb->stInfo.u32FrcFactorActual;
                pstCb->stInfo.u64TimestampDelta = u64TimestampDelta;
                pstCb->stInfo.u64TimestampBase = pstBufInPull->pBuf->timestamp;
                pstCb->stInfo.u32TimestampFrameRate = pstBufInPull->u32TimestampFrameRate;
                pstCb->stInfo.u32OperatingRate = pstBufInPull->u32OperatingRate;
                if (bFrameCopyInput)
                {
                    ppstBufOut[0]->pBuf->flags = pstBufInPull->pBuf->flags;
                    // Data corrupt flag logic handled below
                    VPP_FLAG_CLR(ppstBufOut[0]->pBuf->flags, VPP_BUFFER_FLAG_DATACORRUPT);
                }
                if (VPP_FLAG_IS_SET(ppstBufInPeek[0]->pBuf->flags, VPP_BUFFER_FLAG_DATACORRUPT))
                {
                    VPP_FLAG_SET(ppstBufOut[u32OutBufReq - 1]->pBuf->flags,
                                 VPP_BUFFER_FLAG_DATACORRUPT);
                    if (!bFrameCopyInput)
                        VPP_FLAG_CLR(ppstBufInPeek[0]->pBuf->flags, VPP_BUFFER_FLAG_DATACORRUPT);
                }
            }
            else
            {
                // In interp case, first buffer already released, so peek next buffer as source
                // for gralloc, extradata, cookie
                pstBufTemp = pstVppIp_PortBufPeek(&pstCb->stInput, 0, NULL);
            }

            for (i = 0; i < u32OutBufReq; i++)
            {
                uint64_t u64Timestamp;

                if (bFrameCopyInput && i == 0)
                    u64Timestamp = pstCb->stInfo.u64TimestampBase;
                else
                    u64Timestamp = pstCb->stInfo.u64TimestampBase +
                        (pstCb->stInfo.u64TimestampDelta * pstCb->stInfo.u32InterpCnt);
                vVppIpFrcMc_FillOutputBufData(pstCb, pstBufTemp, ppstBufOut[i], u64Timestamp);
                if (bIsFmtUbwc)
                {
                    u32VppIpHvxCore_BufOutGetAttrUbwc(pstHvxCore, i,
                                                      &ppstBufOut[i]->stUbwcStats[eVppUbwcStatField_P]);
                }
            }

            if (pstCb->stInfo.u32InterpCnt >= pstCb->stInfo.u32InterpFrames)
            {
                vVppIpFrcMc_StateSet(pstCb, FRCMC_STATE_ACTIVE);
                pstCb->stInfo.u32InterpCnt = 0;
                pstCb->pstFrcParams->interp_cnt = 0;
                if (pstCb->stInfo.u32InterpFrames > 1)
                    pstCb->pstFrcParams->update_flags = 1;
            }
            else
            {
                vVppIpFrcMc_StateSet(pstCb, FRCMC_STATE_ACTIVE_INTERP);
                pstCb->pstFrcParams->interp_cnt = pstCb->stInfo.u32InterpCnt;
                pstCb->pstFrcParams->update_flags = 1;
            }

        }

        VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_TOTAL_PROC);
    }

    if (pstBufInPull && VPP_FLAG_IS_SET(pstBufInPull->u32InternalFlags,
                                        VPP_BUF_FLAG_INTERNAL_BYPASS))
        VPP_FLAG_CLR(pstBufInPull->u32InternalFlags, VPP_BUF_FLAG_INTERNAL_BYPASS);

    if (u32Ret == VPP_OK)
    {
        vVppIpFrcMc_ReturnBuffers(pstCb, &pstBufInPull, 1, &pstBufMbi, 1,
                                  ppstBufOut, u32OutBufReq, !bFrameCopyInput, u32OutBufSz);
    }
    else
    {
        vVppIpFrcMc_ReturnBuffers(pstCb, &pstBufInPull, 1, &pstBufMbi, 1,
                                  ppstBufOut, u32OutBufReq, VPP_TRUE, 0);
    }
    return u32Ret;

ERROR_PROCESS_PROF_STOP:
    VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_TOTAL_PROC);

ERROR_PROCESS:
    for (i = 0; i < (pstCb->u32FwBufInPixReq); i++)
    {
        FRCMC_UNREG_BUF(pstCb->pstHvxCore, ppstBufFwIn[i]);
    }
    FRCMC_UNREG_BUF(pstCb->pstHvxCore, pstBufMbi);
    if (u32InMbiReq > 1)
    {
        for (i = 0; i < u32InMbiReq - 1; i++)
        {
            FRCMC_UNREG_BUF(pstCb->pstHvxCore, ppstBufMbiPeek[i]);
        }
    }
    vVppIpFrcMc_StateSet(pstCb, FRCMC_STATE_ACTIVE_START);
    pstCb->stInfo.u32InterpCnt = 0;
    pthread_mutex_unlock(&pstCb->mutex);
    vVppIpFrcMc_ReturnBuffers(pstCb, &pstBufInPull, 1, &pstBufMbi, 1,
                              ppstBufOut, u32OutBufReq, VPP_TRUE, 0);
    return VPP_ERR;
}

static void vVppIpFrcMc_HandlePendingDrain(t_StVppIpFrcMcCb *pstCb)
{
    t_StVppEvt stEvt;

    pthread_mutex_lock(&pstCb->mutex);

    if ((VPP_FLAG_IS_SET(pstCb->u32InternalFlags, IP_DRAIN_PENDING)) &&
        u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ) == 0)
    {
        VPP_FLAG_CLR(pstCb->u32InternalFlags, IP_DRAIN_PENDING);
        pthread_mutex_unlock(&pstCb->mutex);
        // No more input pixel buffers. There shouldn't be any, but flush any remaining MBI buffers
        u32VppIpFrcMc_FlushPort(pstCb, VPP_PORT_INPUT);
        // Drain complete
        stEvt.eType = VPP_EVT_DRAIN_DONE;
        u32VppIpCbEvent(&pstCb->stBase.stCb, stEvt);
    }
    else
        pthread_mutex_unlock(&pstCb->mutex);
}

static void *vpVppIpFrcMc_Worker(void *pv)
{
    LOGI("%s started", __func__);

    t_StVppIpFrcMcCb *pstCb = (t_StVppIpFrcMcCb *)pv;

    // Signal back to main thread that we've launched and are ready to go
    vVppIpFrcMc_SignalWorkerStart(pstCb);

    while (1)
    {
        pthread_mutex_lock(&pstCb->mutex);
        while (u32WorkerThreadShouldSleep(pstCb))
        {
            VPP_IP_PROF_START(&pstCb->stBase, FRCMC_STAT_WORKER_SLEEP);
            pthread_cond_wait(&pstCb->cond, &pstCb->mutex);
            VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_WORKER_SLEEP);
        }

        VPP_IP_PROF_START(&pstCb->stBase, FRCMC_STAT_WORKER);

        uint32_t u32Ret;
        t_StVppIpCmd stCmd;
        u32Ret = u32VppIpFrcMc_CmdGet(pstCb, &stCmd);
        if (u32Ret == VPP_OK)
        {
            pthread_mutex_unlock(&pstCb->mutex);

            // Process the command
            LOG_CMD("ProcessCmd", stCmd.eCmd);

            if (stCmd.eCmd == VPP_IP_CMD_THREAD_EXIT)
            {
                VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_WORKER);
                break;
            }

            else if (stCmd.eCmd == VPP_IP_CMD_OPEN)
                u32VppIpFrcMc_ProcCmdOpen(pstCb);

            else if (stCmd.eCmd == VPP_IP_CMD_CLOSE)
                u32VppIpFrcMc_ProcCmdClose(pstCb);

            else if (stCmd.eCmd == VPP_IP_CMD_FLUSH)
                u32VppIpFrcMc_ProcCmdFlush(pstCb, &stCmd);

            else if (stCmd.eCmd == VPP_IP_CMD_DRAIN)
                u32VppIpFrcMc_ProcCmdDrain(pstCb);

            else if (stCmd.eCmd == VPP_IP_CMD_RECONFIGURE)
                u32VppIpFrcMc_ProcCmdReconfigure(pstCb);

            else
                LOGE("unknown command in queue");

            VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_WORKER);
            continue;
        }

        if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
        {
            LOGD("got buffer, but state is not active");
            pthread_mutex_unlock(&pstCb->mutex);
            VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_WORKER);
            continue;
        }

        if (bVppIpFrcMc_ProcBufReqMet(pstCb))
        {
            pthread_mutex_unlock(&pstCb->mutex);
            u32VppIpFrcMc_ProcessBuffer(pstCb);
            vVppIpFrcMc_HandlePendingDrain(pstCb);
            VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_WORKER);
            continue;
        }

        pthread_mutex_unlock(&pstCb->mutex);
        VPP_IP_PROF_STOP(&pstCb->stBase, FRCMC_STAT_WORKER);
    } //while (1)

    LOGI("%s exited", __func__);

    return NULL;
}

/************************************************************************
 * Global Functions
 ************************************************************************/
uint32_t u32VppIpFrcMc_Boot()
{
    uint32_t u32Ret = VPP_OK;
    const uint32_t u32VppProcFlags = (0x01 << (VPP_FUNC_ID_FRC & FUNC_ID_MASK));

    LOG_ENTER();

    u32Ret = u32VppIpHvxCore_Boot(FRCMC_TUNINGS_FILE_NAME, u32VppProcFlags);
    LOGE_IF(u32Ret != VPP_OK, "u32VppIpHvxCore_Boot returned u32Ret=%u", u32Ret);

    LOG_EXIT_RET(u32Ret);
}

uint32_t u32VppIpFrcMc_Shutdown()
{
    uint32_t u32Ret = VPP_OK;
    const uint32_t u32VppProcFlags = (0x01 << (VPP_FUNC_ID_FRC & FUNC_ID_MASK));

    LOG_ENTER();

    u32Ret = u32VppIpHvxCore_Shutdown(u32VppProcFlags);
    LOGE_IF(u32Ret != VPP_OK, "u32VppIpHvxCore_Shutdown returned u32Ret=%u", u32Ret);

    LOG_EXIT_RET(u32Ret);
}

void *vpVppIpFrcMc_Init(t_StVppCtx *pstCtx, uint32_t u32Flags, t_StVppCallback cbs)
{
    LOGI("%s", __func__);

    int rc;
    uint32_t u32;
    t_StVppIpFrcMcCb *pstCb;
    uint32_t u32Length;

    VPP_RET_IF_NULL(pstCtx, NULL);

    if (u32VppIp_ValidateCallbacks(&cbs) != VPP_OK)
    {
        LOGE("given invalid callbacks.");
        goto ERROR_CALLBACKS;
    }

    pstCb = calloc(sizeof(t_StVppIpFrcMcCb), 1);
    if (!pstCb)
    {
        LOGE("calloc failed for frc context");
        goto ERROR_MALLOC_CONTEXT;
    }
    LOGD("%s pstCb=%p", __func__,pstCb);

    u32VppIp_SetBase(pstCtx, u32Flags, cbs, &pstCb->stBase);

    vVppIpFrcMc_StateSet(pstCb, FRCMC_STATE_NULL);

    u32 = VPP_IP_PROF_REGISTER(&pstCb->stBase, astFrcMcStatsCfg, u32FrcMcStatCnt);
    LOGE_IF(u32 != VPP_OK, "ERROR: unable to register stats, u32=%u", u32);

    u32Length = sizeof(vpp_svc_frc_params_t);

    pstCb->pstHvxCore = pstVppIpHvxCore_Init(pstCb->stBase.pstCtx, u32Flags,
                                             FRCMC_BLOCK_MAX, u32Length);
    if (!pstCb->pstHvxCore)
    {
        LOGE("Failed to init HVX Core.");
        goto ERROR_CORE_INIT;
    }

    if (u32VppBufPool_Init(&pstCb->stInput.stPendingQ) != VPP_OK)
    {
        LOGE("unable to u32VppBufPool_Init() input queue\n");
        goto ERROR_PENDING_INPUT_Q_INIT;
    }

    if (u32VppBufPool_Init(&pstCb->stMbi.stPendingQ) != VPP_OK)
    {
        LOGE("unable to u32VppBufPool_Init() input queue\n");
        goto ERROR_PENDING_MBI_Q_INIT;
    }

    if (u32VppBufPool_Init(&pstCb->stOutput.stPendingQ) != VPP_OK)
    {
        LOGE("unable to u32VppBufPool_Init() output queue\n");
        goto ERROR_PENDING_OUTPUT_Q_INIT;
    }

    if (vpp_queue_init(&pstCb->stCmdQ, FRCMC_CMD_Q_SZ) != VPP_OK)
    {
        LOGE("unable to vpp_queue_init");
        goto ERROR_CMD_Q_INIT;
    }

    pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments =
        calloc(FRCMC_CTRL_SEG_CNT_DEFAULT, sizeof(struct vpp_ctrl_frc_segment));
    if (!pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments)
    {
        LOGE("Error allocating control segment memory");
        goto ERR_ALLOC_FRC_CTRL_SEG;
    }
    pstCb->stCfg.stFrcCtrlSegs.u32AllocCnt = FRCMC_CTRL_SEG_CNT_DEFAULT;

    rc = sem_init(&pstCb->sem, 0, 0);
    if (rc)
    {
        LOGE("unable to initialize hvx mutex");
        goto ERROR_SEM_INIT;
    }

    rc = pthread_mutex_init(&pstCb->mutex, NULL);
    if (rc)
    {
        LOGE("unable to initialize hvx mutex");
        goto ERROR_MUTEX_INIT;
    }

    rc = pthread_cond_init(&pstCb->cond, NULL);
    if (rc)
    {
        LOGE("unable to init condition variable");
        goto ERROR_COND_INIT;
    }

    rc = pthread_create(&pstCb->thread, NULL, vpVppIpFrcMc_Worker, pstCb);
    if (rc)
    {
        LOGE("unable to spawn hvx worker thread");
        goto ERROR_THREAD_CREATE;
    }

    vVppIpFrcMc_InitCapabilityResources(pstCb);

    //copy from global to local
    memcpy((void*) &pstCb->stLocalFrcMcParams, (void*) &stGlobalFrcMcParams, sizeof(t_StCustomFrcMcParams));

    vVppIpFrcMc_InitParam(pstCb);

    if (u32VppUtils_IsSoc(MSMTALOS))
        pstCb->bHvxLinearOnly = VPP_TRUE;

    // Wait for the thread to launch before returning
    vVppIpFrcMc_WaitWorkerStart(pstCb);

    VPP_IP_STATE_SET(pstCb, VPP_IP_STATE_INITED);

    return pstCb;

ERROR_THREAD_CREATE:
    LOGI("destroying condition variable");
    pthread_cond_destroy(&pstCb->cond);

ERROR_COND_INIT:
    LOGI("destroying mutex");
    pthread_mutex_destroy(&pstCb->mutex);

ERROR_MUTEX_INIT:
    LOGI("destroying semaphore");
    sem_destroy(&pstCb->sem);

ERROR_SEM_INIT:
    if (pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments)
    {
        free(pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments);
        pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments = NULL;
        pstCb->stCfg.stFrcCtrlSegs.u32AllocCnt = 0;
    }

ERR_ALLOC_FRC_CTRL_SEG:
    vpp_queue_term(&pstCb->stCmdQ);

ERROR_CMD_Q_INIT:
    u32VppBufPool_Term(&pstCb->stOutput.stPendingQ);

ERROR_PENDING_OUTPUT_Q_INIT:
    u32VppBufPool_Term(&pstCb->stMbi.stPendingQ);

ERROR_PENDING_MBI_Q_INIT:
    u32VppBufPool_Term(&pstCb->stInput.stPendingQ);

ERROR_PENDING_INPUT_Q_INIT:
    vVppIpHvxCore_Term(pstCb->pstHvxCore);

ERROR_CORE_INIT:
    u32 = VPP_IP_PROF_UNREGISTER(&pstCb->stBase);
    LOGE_IF(u32 != VPP_OK, "ERROR: unable to unregister stats, u32=%u", u32);

    if (pstCb)
        free(pstCb);

ERROR_MALLOC_CONTEXT:
ERROR_CALLBACKS:
    return NULL;
}

void vVppIpFrcMc_Term(void *ctx)
{
    int rc;
    uint32_t u32;
    t_StVppIpFrcMcCb *pstCb;
    t_StVppIpCmd stCmd;

    LOGI("%s\n", __func__);

    VPP_RET_VOID_IF_NULL(ctx);
    pstCb = FRCMC_CB_GET(ctx);

    if (!pstCb)
    {
        LOGD("Try to free NULL pstCb\n");
        return;
    }

    if (VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGD("%s() Still in active state. Closing.", __func__);
        u32VppIpFrcMc_Close(ctx);
    }
    u32VppIpFrcMc_FlushPort(pstCb, VPP_PORT_INPUT);
    u32VppIpFrcMc_FlushPort(pstCb, VPP_PORT_OUTPUT);

    if (pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments)
    {
        free(pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments);
        pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments = NULL;
        pstCb->stCfg.stFrcCtrlSegs.u32AllocCnt = 0;
        pstCb->stCfg.stFrcCtrlSegs.u32SegCnt = 0;
        pstCb->stCfg.stFrcCtrlSegs.s32SegIdx = 0;
    }

    stCmd.eCmd = VPP_IP_CMD_THREAD_EXIT;
    u32VppIpFrcMc_CmdPut(pstCb, stCmd);

    rc = pthread_join(pstCb->thread, NULL);
    if (rc)
        LOGE("pthread_join failed: %d --> %s", rc, strerror(rc));

    rc = pthread_cond_destroy(&pstCb->cond);
    if (rc)
        LOGE("pthread_cond_destroy failed: %d --> %s", rc, strerror(rc));

    rc = pthread_mutex_destroy(&pstCb->mutex);
    if (rc)
        LOGE("pthread_mutex_destroy failed: %d --> %s", rc, strerror(rc));

    rc = sem_destroy(&pstCb->sem);
    if (rc)
        LOGE("sem_destroy failed: %d --> %s", rc, strerror(rc));

    vpp_queue_term(&pstCb->stCmdQ);

    u32VppBufPool_Term(&pstCb->stInput.stPendingQ);
    u32VppBufPool_Term(&pstCb->stMbi.stPendingQ);
    u32VppBufPool_Term(&pstCb->stOutput.stPendingQ);

    vVppIpHvxCore_Term(pstCb->pstHvxCore);

    u32 = VPP_IP_PROF_UNREGISTER(&pstCb->stBase);
    LOGE_IF(u32 != VPP_OK, "ERROR: unable to unregister stats, u32=%u", u32);

    free(pstCb);
}

uint32_t u32VppIpFrcMc_Open(void *ctx)
{
    LOGI("%s\n", __func__);

    t_StVppIpFrcMcCb *pstCb;
    t_StVppIpCmd stCmd;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = FRCMC_CB_GET(ctx);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_INITED))
        return VPP_ERR_STATE;

    // Validate that the port configuration is valid
    if (u32VppIpFrcMc_ValidateConfig(pstCb->stInput.stParam, pstCb->stOutput.stParam) != VPP_OK)
        return VPP_ERR_PARAM;

    stCmd.eCmd = VPP_IP_CMD_OPEN;
    u32VppIpFrcMc_CmdPut(pstCb, stCmd);

    LOGI(">> waiting on semaphore");
    sem_wait(&pstCb->sem);
    LOGI(">> got semaphore");

    return pstCb->async_res.u32OpenRet;
}

uint32_t u32VppIpFrcMc_Close(void *ctx)
{
    LOGI("%s\n", __func__);

    t_StVppIpFrcMcCb *pstCb;
    t_StVppIpCmd stCmd;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = FRCMC_CB_GET(ctx);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGD("%s() VPP_ERR_STATE. pstCb->eState=%d", __func__, pstCb->eState);
        return VPP_ERR_STATE;
    }

    stCmd.eCmd = VPP_IP_CMD_CLOSE;
    u32VppIpFrcMc_CmdPut(pstCb, stCmd);

    LOGI(">> waiting on semaphore");
    sem_wait(&pstCb->sem);
    LOGI(">> got semaphore");

    return pstCb->async_res.u32CloseRet;
}

uint32_t u32VppIpFrcMc_SetParam(void *ctx, enum vpp_port ePort,
                                struct vpp_port_param stParam)
{
    LOGI("%s\n", __func__);

    uint32_t u32Ret = VPP_OK;
    t_StVppIpFrcMcCb *pstCb;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = FRCMC_CB_GET(ctx);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_INITED))
    {
        LOGD("%s: state is not VPP_IP_STATE_INITED", __func__);
        return VPP_ERR_STATE;
    }

    if (u32VppIpFrcMc_ValidatePortParam(stParam) != VPP_OK)
    {
        LOGE("given invalid port params.");
        return VPP_ERR_PARAM;
    }

    pthread_mutex_lock(&pstCb->mutex);

    if (ePort == VPP_PORT_INPUT)
        pstCb->stInput.stParam = stParam;
    else if (ePort == VPP_PORT_OUTPUT)
        pstCb->stOutput.stParam = stParam;
    else
        u32Ret = VPP_ERR_PARAM;

    pstCb->stCfg.u32ComputeMask |= FRCMC_PARAM;

    pthread_mutex_unlock(&pstCb->mutex);

    return u32Ret;
}

uint32_t u32VppIpFrcMc_SetCtrl(void *ctx, struct hqv_control stCtrl)
{
    LOGI("%s\n", __func__);

    uint32_t u32Ret;
    t_StVppIpFrcMcCb *pstCb;
    struct vpp_ctrl_frc_segment stSegTemp;
    uint32_t i, j;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = FRCMC_CB_GET(ctx);

    u32Ret = u32VppIpFrcMc_ValidateCtrl(stCtrl);
    if (u32Ret != VPP_OK)
    {
        LOGE("Control validation failed u32Ret=%u", u32Ret);
        return VPP_ERR_INVALID_CFG;
    }

    pthread_mutex_lock(&pstCb->mutex);
    if (stCtrl.mode == HQV_MODE_AUTO)
    {
        pstCb->stCfg.u32AutoHqvEnable = 1;
        pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments[0].mode = HQV_FRC_MODE_SMOOTH_MOTION;
        pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments[0].level = HQV_FRC_LEVEL_HIGH;
        pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments[0].ts_start = 0;
        pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments[0].frame_copy_input = 0;
        pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments[0].frame_copy_on_fallback = 0;
        pstCb->stCfg.stFrcCtrlSegs.u32SegCnt = 1;

        pstCb->stCfg.u32EnableMask |= FRC_ALGO_MC;
        pstCb->stCfg.u32ComputeMask |= FRC_ALGO_MC;

        pstCb->stCfg.u32ComputeMask |= FRCMC_PARAM;
    }
    else if (stCtrl.mode == HQV_MODE_MANUAL)
    {
        pstCb->stCfg.u32AutoHqvEnable = 0;
        if (stCtrl.ctrl_type == HQV_CONTROL_FRC)
        {
            if (stCtrl.frc.num_segments > pstCb->stCfg.stFrcCtrlSegs.u32AllocCnt)
            {
                if (pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments)
                    free(pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments);
                pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments = NULL;
                pstCb->stCfg.stFrcCtrlSegs.u32AllocCnt = 0;
                pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments =
                    calloc(stCtrl.frc.num_segments, sizeof(struct vpp_ctrl_frc_segment));
                if (!pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments)
                {
                    LOGE("Error reallocating frc control segment memory!");
                    pthread_mutex_unlock(&pstCb->mutex);
                    return VPP_ERR_NO_MEM;
                }
                pstCb->stCfg.stFrcCtrlSegs.u32AllocCnt = stCtrl.frc.num_segments;
            }
            memcpy(pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments, stCtrl.frc.segments,
                   stCtrl.frc.num_segments * sizeof(struct vpp_ctrl_frc_segment));
            pstCb->stCfg.stFrcCtrlSegs.u32SegCnt = stCtrl.frc.num_segments;

            for (i = 0; i < stCtrl.frc.num_segments; i++)
            {
                for (j = i + 1; j < stCtrl.frc.num_segments; j++)
                {
                    if (pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments[i].ts_start >
                        pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments[j].ts_start)
                    {
                        stSegTemp = pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments[i];
                        pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments[i] =
                            pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments[j];
                        pstCb->stCfg.stFrcCtrlSegs.pstFrcSegments[j] = stSegTemp;
                    }
                }
            }
            pstCb->stCfg.stFrcCtrlSegs.s32SegIdx = 0;
            if (stCtrl.frc.num_segments)
            {
                pstCb->stCfg.u32EnableMask |= FRC_ALGO_MC;
                pstCb->stCfg.u32ComputeMask |= FRC_ALGO_MC;
            }
            else
            {
                pstCb->stCfg.u32EnableMask &= ~FRC_ALGO_MC;
            }
        }
    }
    else if (stCtrl.mode == HQV_MODE_OFF)
    {
        LOGD("%s(): ctrl.mode is HQV_MODE_OFF", __func__);
        pstCb->stCfg.u32EnableMask &= ~FRC_ALGO_MC;
    }
    pthread_mutex_unlock(&pstCb->mutex);

    return VPP_OK;
}

uint32_t u32VppIpFrcMc_GetBufferRequirements(void *ctx,
                                             t_StVppIpBufReq *pstInputBufReq,
                                             t_StVppIpBufReq *pstOutputBufReq)

{
    LOGI("%s\n", __func__);

    uint32_t u32PxSz;
    t_StVppIpFrcMcCb *pstCb;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = FRCMC_CB_GET(ctx);

    if (pstInputBufReq)
    {
        pstInputBufReq->u32MinCnt = pstCb->u32FwBufInPixReq;
    }
    if (pstOutputBufReq)
    {
        u32PxSz = u32VppUtils_GetPxBufferSize(&pstCb->stOutput.stParam);
        pstOutputBufReq->u32PxSz = u32PxSz;
        pstOutputBufReq->u32MinCnt = pstCb->u32FwBufOutPixReq;
    }

    return VPP_OK;
}

uint32_t u32VppIpFrcMc_QueueBuf(void *ctx, enum vpp_port ePort,
                                t_StVppBuf *pBuf)

{
    uint32_t u32Ret = VPP_OK;
    t_StVppIpFrcMcCb *pstCb;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = FRCMC_CB_GET(ctx);

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pBuf, VPP_ERR_PARAM);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGE("eState=%d should be VPP_IP_STATE_ACTIVE=%d", pstCb->eState, VPP_IP_STATE_ACTIVE);
        return VPP_ERR_STATE;
    }

    vVppIpCbLog(&pstCb->stBase.stCb, pBuf, eVppLogId_IpQueueBuf);

#ifdef FRCMC_FORCE_DUMP_BUF
    VPP_FLAG_SET(pBuf->u32InternalFlags, VPP_BUF_FLAG_DUMP);
#endif

    if (ePort == VPP_PORT_INPUT)
    {
        VPP_FLAG_CLR(pBuf->u32InternalFlags, VPP_BUF_FLAG_EOS_PROCESSED);
        VPP_FLAG_CLR(pBuf->u32InternalFlags, VPP_BUF_FLAG_IDR_PROCESSED);
        VPP_FLAG_CLR(pBuf->u32InternalFlags, VPP_BUF_FLAG_DC_PROCESSED);
        VPP_FLAG_CLR(pBuf->u32InternalFlags, VPP_BUF_FLAG_INTERNAL_BYPASS);

        if (pBuf->eBufPxType == eVppBufPxDataType_Raw)
        {
            if (pBuf->stPixel.u32FilledLen == 0)
            {
                VPP_FLAG_SET(pBuf->u32InternalFlags, VPP_BUF_FLAG_INTERNAL_BYPASS);
            }
#ifdef FRCMC_DUMP_FRAME_ENABLE
            if ((VPP_FLAG_IS_SET(pBuf->u32InternalFlags, VPP_BUF_FLAG_DUMP)) &&
                (pBuf->stPixel.u32FilledLen != 0))
            {
                vVppIpFrcMc_DumpFrame(pstCb, pBuf, VPP_PORT_INPUT);
            }
#endif
            u32Ret = u32VppIp_PortBufPut(&pstCb->stInput, pBuf,
                                              &pstCb->mutex, &pstCb->cond);
            pstCb->stats.u32InYuvQCnt++;
        }
        else
        {
            u32Ret = u32VppIp_PortBufPut(&pstCb->stMbi, pBuf,
                                         &pstCb->mutex, &pstCb->cond);
            pstCb->stats.u32InMbiQCnt++;
        }
    }
    else if (ePort == VPP_PORT_OUTPUT)
    {
        u32Ret = u32VppIp_PortBufPut(&pstCb->stOutput, pBuf,
                                     &pstCb->mutex, &pstCb->cond);
        pstCb->stats.u32OutYuvQCnt++;
    }
    else
    {
        LOGE("received buffer on invalid port, port=%u", ePort);
        u32Ret = VPP_ERR_PARAM;
    }

    return u32Ret;
}

uint32_t u32VppIpFrcMc_Flush(void *ctx, enum vpp_port ePort)
{
    LOGI("%s\n", __func__);

    uint32_t u32Ret = VPP_OK;
    t_StVppIpFrcMcCb *pstCb;
    t_StVppIpCmd stCmd;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = FRCMC_CB_GET(ctx);

    if (ePort != VPP_PORT_INPUT && ePort != VPP_PORT_OUTPUT)
    {
        LOGE("ERROR: calling flush with invalid port, port=%u", ePort);
        return VPP_ERR_PARAM;
    }

    stCmd.eCmd = VPP_IP_CMD_FLUSH;
    stCmd.flush.ePort = ePort;
    u32VppIpFrcMc_CmdPut(pstCb, stCmd);

    return u32Ret;
}

uint32_t u32VppIpFrcMc_Drain(void *ctx)
{
    LOGI("%s\n", __func__);

    uint32_t u32Ret = VPP_OK;
    t_StVppIpFrcMcCb *pstCb;
    t_StVppIpCmd stCmd;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = FRCMC_CB_GET(ctx);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGE("ERROR: calling drain in invalid state, expected=%u, actual=%u",
             VPP_IP_STATE_ACTIVE, pstCb->eState);
        return VPP_ERR_STATE;
    }

    stCmd.eCmd = VPP_IP_CMD_DRAIN;
    u32VppIpFrcMc_CmdPut(pstCb, stCmd);

    return u32Ret;
}

uint32_t u32VppIpFrcMc_Reconfigure(void *ctx,
                                   struct vpp_port_param in_param,
                                   struct vpp_port_param out_param)
{
    LOG_ENTER();

    uint32_t u32Ret = VPP_OK;
    t_StVppIpFrcMcCb *pstCb;
    t_StVppIpCmd stCmd;
    uint32_t u32InQCnt;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = FRCMC_CB_GET(ctx);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
        return VPP_ERR_STATE;

    // Validate that the port configuration is valid
    if (u32VppIpFrcMc_ValidateConfig(in_param, out_param) != VPP_OK)
        return VPP_ERR_PARAM;

    pthread_mutex_lock(&pstCb->mutex);

    u32InQCnt = u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ);
    if (u32InQCnt)
    {
        LOGE("Reconfig but still %u buffers in input queue", u32InQCnt);
        pthread_mutex_unlock(&pstCb->mutex);
        return VPP_ERR_STATE;
    }

    pstCb->stInput.stParam = in_param;
    pstCb->stOutput.stParam = out_param;
    if (u32VppUtils_IsFmtUbwc(pstCb->stInput.stParam.fmt) && pstCb->bHvxLinearOnly)
#ifdef VPP_TARGET_USES_C2D
    {
        pstCb->bNeedsC2D = VPP_TRUE;
    }
    else
    {
        pstCb->bNeedsC2D = VPP_FALSE;
    }
#else
    {
        LOGE("Reconfig fmt=%d is UBWC. Not supported!", pstCb->stInput.stParam.fmt);
        pthread_mutex_unlock(&pstCb->mutex);
        return VPP_ERR_PARAM;
    }
#endif
    pstCb->stCfg.u32ComputeMask |= FRCMC_PARAM;
    vVppIpFrcMc_Compute(pstCb);
    pthread_mutex_unlock(&pstCb->mutex);

    pstCb->async_res.u32ReconfigRet = VPP_OK;
    stCmd.eCmd = VPP_IP_CMD_RECONFIGURE;
    u32VppIpFrcMc_CmdPut(pstCb, stCmd);

    LOGI(">> waiting on semaphore (reconfig)");
    sem_wait(&pstCb->sem);
    LOGI(">> got semaphore (reconfig)");

    if (pstCb->async_res.u32ReconfigRet != VPP_OK)
    {
        u32Ret = pstCb->async_res.u32ReconfigRet;
        LOGE("ERROR: reconfigure failed, u32=%u", u32Ret);
    }

    // Reset proc stats
    VPP_IP_PROF_RESET_SINGLE(&pstCb->stBase, FRCMC_STAT_PROC);

    LOG_EXIT_RET(u32Ret);
}
