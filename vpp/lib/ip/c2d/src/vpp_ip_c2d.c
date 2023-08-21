/*!
 * @file vpp_ip_c2d.c
 *
 * @cr
 * Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include <linux/msm_kgsl.h>
#include <c2d2.h>

#define VPP_LOG_TAG     VPP_LOG_MODULE_C2D_TAG
#define VPP_LOG_MODULE  VPP_LOG_MODULE_C2D
#include "vpp_dbg.h"

#include "vpp.h"
#include "vpp_reg.h"
#include "vpp_queue.h"
#include "vpp_ip.h"
#include "vpp_ip_c2d.h"
#include "vpp_utils.h"
#include "vpp_stats.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/
#define C2D_Y_PLANE_LINEAR                  0
#define C2D_UV_PLANE_LINEAR                 1
#define C2D_Y_PLANE_UBWC                    1
#define C2D_UV_PLANE_UBWC                   3
#define C2D_RGBA_PLANE_UBWC                 1
#define C2D_INIT_MAX_OBJECT                 4
#define C2D_INIT_MAX_TEMPLATE               4

#define C2D_CMD_Q_SZ                        30
#define C2D_PORT_BUF_Q_SZ                   VPP_INTERNAL_BUF_MAX
#define C2D_COMPUTE_SURFACE                 (1 << 0)

#define C2D_DUMP_NM_LEN                     256
#define C2D_DUMP_BUF_IN_NM                  "c2d_%p_in.yuv"
#define C2D_DUMP_BUF_OUT_NM                 "c2d_%p_out.yuv"

#define C2D_PORT_PARAM_WIDTH_MIN            32
#define C2D_PORT_PARAM_HEIGHT_MIN           32

#define C2D_CB_GET(ctx) (ctx ? (t_StVppIpC2DCb *)ctx : NULL)

#define C2D_STATS(cb, stat)                 (cb)->stats.u32##stat++

#define C2D_DUMP_FRAME(cb, buf, port)       vVppIpC2D_DumpFrame(cb, buf, port)

#define C2D_STATS_PROF(_E) \
    _E(C2D_STAT_PROC, 10, 1) \
    _E(C2D_STAT_DRIVER_INIT, 1, 1) \
    _E(C2D_STAT_CREATE_SURFACE, 1, 1)

VPP_STAT_ENUM(C2D_STATS_PROF);

typedef struct StVppIpC2DGlobalCb {
    pthread_mutex_t mutex;
    uint32_t bC2DDriverInited;
    uint32_t u32DriverSessCnt;
} t_StVppIpC2DGlobalCb;

typedef struct {
    uint32_t u32EnableMask;
    uint32_t u32ComputeMask;
} t_StVppIpC2DCfg;

typedef union {
    C2D_YUV_SURFACE_DEF stYuvSurface;
    C2D_RGB_SURFACE_DEF stRgbSurface;
} t_UStSurface;

typedef struct {
    uint32_t u32SurfaceId;
    t_UStSurface stSurface;
} t_StVppIpC2DPort;

typedef struct {
    t_StVppIpBase stBase;
    t_StVppIpC2DGlobalCb *pstGlobal;
    uint32_t u32InternalFlags;
    uint32_t bInlineProcOnly;

    sem_t sem;
    pthread_t thread;
    pthread_cond_t cond;
    pthread_mutex_t mutex;

    t_StVppIpPort stInput;
    t_StVppIpPort stOutput;
    t_StVppIpCmdQueue stCmdQ;

    uint32_t eState;

    t_StVppIpC2DCfg stCfg;
    uint32_t u32InputMin;
    uint32_t u32OutputMin;

    struct {
        t_StVppIpC2DPort stIn;
        t_StVppIpC2DPort stOut;
        C2D_OBJECT stBlit;
    } c2d;

    struct {
        STAT_DECL(InQCnt);          // Number of frames queued to input
        STAT_DECL(OutQCnt);         // Number of frames queued to output
        STAT_DECL(InProcCnt);       // Number of input frames processed
        STAT_DECL(OutProcCnt);      // Number of output frames processed into
        STAT_DECL(IBDCnt);          // Number of IBD issued
        STAT_DECL(OBDCnt);          // Number of OBD issued
    } stats;

    struct {
        uint32_t u32OpenRet;
        uint32_t u32CloseRet;
    } async_res;
} t_StVppIpC2DCb;

/************************************************************************
 * Local static variables
 ***********************************************************************/
static t_StVppIpC2DGlobalCb stGlobalCb = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
};

VPP_STAT_DECL(C2D_STATS_PROF, C2DStats);

/************************************************************************
 * Forward Declarations
 ************************************************************************/

/************************************************************************
 * Local Functions
 ***********************************************************************/
static void vVppIpC2D_DumpFrame(t_StVppIpC2DCb *pstCb, t_StVppBuf *pstBuf,
                                enum vpp_port port)
{
    char cPath[C2D_DUMP_NM_LEN];
    struct vpp_port_param *pstParam;

    if (pstCb->stBase.bSecureSession)
        return;

    if (port == VPP_PORT_INPUT)
    {
        snprintf(cPath, C2D_DUMP_NM_LEN, C2D_DUMP_BUF_IN_NM, pstCb);
        pstParam = &pstCb->stInput.stParam;
    }
    else
    {
        snprintf(cPath, C2D_DUMP_NM_LEN, C2D_DUMP_BUF_OUT_NM, pstCb);
        pstParam = &pstCb->stOutput.stParam;
    }

    u32VppBuf_Dump(pstCb->stBase.pstCtx, pstBuf, *pstParam, cPath);
}

static uint32_t u32VppIpC2D_GetFormat(enum vpp_color_format eFmt,
                                      uint32 bIsSource)
{
    uint32_t u32C2DFmt;

    switch (eFmt)
    {
        case VPP_COLOR_FORMAT_NV12_VENUS:
            u32C2DFmt = C2D_COLOR_FORMAT_420_NV12;
            break;
        case VPP_COLOR_FORMAT_UBWC_NV12:
            u32C2DFmt = C2D_COLOR_FORMAT_420_NV12 | C2D_FORMAT_UBWC_COMPRESSED;
            break;
// Linear P010 not supported in C2D, so don't support UBWC TP10 for now
#if 0
        case VPP_COLOR_FORMAT_UBWC_TP10:
            u32C2DFmt = C2D_COLOR_FORMAT_420_TP10 | C2D_FORMAT_UBWC_COMPRESSED;
            break;
#endif
        case VPP_COLOR_FORMAT_RGBA8:
            u32C2DFmt = C2D_COLOR_FORMAT_8888_RGBA | C2D_FORMAT_SWAP_ENDIANNESS;
            if (bIsSource)
                u32C2DFmt |= C2D_FORMAT_PREMULTIPLIED;
            break;
        case VPP_COLOR_FORMAT_UBWC_RGBA8:
            u32C2DFmt = C2D_COLOR_FORMAT_8888_RGBA | C2D_FORMAT_SWAP_ENDIANNESS |
                C2D_FORMAT_UBWC_COMPRESSED;
            if (bIsSource)
                u32C2DFmt |= C2D_FORMAT_PREMULTIPLIED;
            break;
        case VPP_COLOR_FORMAT_BGRA8:
            u32C2DFmt = C2D_COLOR_FORMAT_8888_RGBA | C2D_FORMAT_SWAP_RB |
                C2D_FORMAT_SWAP_ENDIANNESS;
            if (bIsSource)
                u32C2DFmt |= C2D_FORMAT_PREMULTIPLIED;
            break;
        case VPP_COLOR_FORMAT_UBWC_BGRA8:
            u32C2DFmt = C2D_COLOR_FORMAT_8888_RGBA | C2D_FORMAT_SWAP_RB |
                C2D_FORMAT_SWAP_ENDIANNESS | C2D_FORMAT_UBWC_COMPRESSED;
            if (bIsSource)
                u32C2DFmt |= C2D_FORMAT_PREMULTIPLIED;
            break;
        case VPP_COLOR_FORMAT_UBWC_RGB565:
            u32C2DFmt = C2D_COLOR_FORMAT_565_RGB | C2D_FORMAT_UBWC_COMPRESSED;
            if (bIsSource)
                u32C2DFmt |= C2D_FORMAT_PREMULTIPLIED;
            break;
        case VPP_COLOR_FORMAT_UBWC_BGR565:
            u32C2DFmt = C2D_COLOR_FORMAT_565_RGB | C2D_FORMAT_SWAP_RB |
                C2D_FORMAT_UBWC_COMPRESSED;
            if (bIsSource)
                u32C2DFmt |= C2D_FORMAT_PREMULTIPLIED;
            break;
        default:
            // Should not get here since port config validated
            LOGE("eFmt=%d not supported!", eFmt);
            u32C2DFmt = (uint32_t)-1;
    }

    return u32C2DFmt;
}

static void vVppIpC2D_ComputeC2DSurface(t_UStSurface *pstC2dSurface,
                                        struct vpp_port_param stVppParam,
                                        uint32 bIsSource)
{
    uint32_t u32Width, u32Height, u32Stride0, u32Stride1, u32Fmt;
    C2D_YUV_SURFACE_DEF *pstYuvSurface;
    C2D_RGB_SURFACE_DEF *pstRgbSurface;

    VPP_RET_VOID_IF_NULL(pstC2dSurface);

    u32Width = stVppParam.width;
    u32Height = stVppParam.height;
    u32Fmt = u32VppIpC2D_GetFormat(stVppParam.fmt, bIsSource);

    if (u32VppUtils_IsFmtRgb(stVppParam.fmt))
    {
        pstRgbSurface = &pstC2dSurface->stRgbSurface;
        pstRgbSurface->format = u32Fmt;
        pstRgbSurface->width = u32Width;
        pstRgbSurface->height = u32Height;
        if (u32VppUtils_IsFmtUbwc(stVppParam.fmt))
        {
            u32Stride0 = u32VppUtils_CalcStrideForPlane(u32Width,
                                                         stVppParam.fmt,
                                                         C2D_RGBA_PLANE_UBWC);
        }
        else
        {
            u32Stride0 = stVppParam.stride;
        }
        pstRgbSurface->stride = u32Stride0;
    }
    else
    {
        pstYuvSurface = &pstC2dSurface->stYuvSurface;
        pstYuvSurface->format = u32Fmt;
        pstYuvSurface->width = u32Width;
        pstYuvSurface->height = u32Height;

        if (u32VppUtils_IsFmtUbwc(stVppParam.fmt))
        {
            u32Stride0  = u32VppUtils_CalcStrideForPlane(u32Width,
                                                         stVppParam.fmt,
                                                         C2D_Y_PLANE_UBWC);
            u32Stride1 = u32VppUtils_CalcStrideForPlane(u32Width,
                                                        stVppParam.fmt,
                                                        C2D_UV_PLANE_UBWC);
        }
        else
        {
            u32Stride0 = stVppParam.stride;
            u32Stride1 = stVppParam.stride;
        }

        pstYuvSurface->stride0 = u32Stride0;
        pstYuvSurface->stride1 = u32Stride1;
    }
}

static void vVppIpC2D_Compute(t_StVppIpC2DCb *pstCb)
{
    C2D_OBJECT *pstC2dBlit;

    VPP_RET_VOID_IF_NULL(pstCb);

    if (pstCb->stCfg.u32ComputeMask)
    {
        pthread_mutex_lock(&pstCb->mutex);

        vVppIpC2D_ComputeC2DSurface(&pstCb->c2d.stIn.stSurface,
                                    pstCb->stInput.stParam, VPP_TRUE);

        vVppIpC2D_ComputeC2DSurface(&pstCb->c2d.stOut.stSurface,
                                    pstCb->stOutput.stParam, VPP_FALSE);

        pstC2dBlit = &pstCb->c2d.stBlit;
        pstC2dBlit->source_rect.x = 0 << 16;
        pstC2dBlit->source_rect.y = 0 << 16;
        pstC2dBlit->source_rect.width = pstCb->stInput.stParam.width << 16;
        pstC2dBlit->source_rect.height = pstCb->stInput.stParam.height << 16;
        pstC2dBlit->target_rect.x = 0 << 16;
        pstC2dBlit->target_rect.y = 0 << 16;
        pstC2dBlit->target_rect.width = pstCb->stOutput.stParam.width << 16;
        pstC2dBlit->target_rect.height = pstCb->stOutput.stParam.height << 16;

        pstCb->stCfg.u32ComputeMask = 0;

        pthread_mutex_unlock(&pstCb->mutex);
    }
}

static void vVppIpC2D_SignalWorkerStart(t_StVppIpC2DCb *pstCb)
{
    pthread_mutex_lock(&pstCb->mutex);

    VPP_FLAG_SET(pstCb->u32InternalFlags, IP_WORKER_STARTED);

    pthread_cond_signal(&pstCb->cond);

    pthread_mutex_unlock(&pstCb->mutex);
}

static void vVppIpC2D_WaitWorkerStart(t_StVppIpC2DCb *pstCb)
{
    pthread_mutex_lock(&pstCb->mutex);

    while (!VPP_FLAG_IS_SET(pstCb->u32InternalFlags, IP_WORKER_STARTED))
        pthread_cond_wait(&pstCb->cond, &pstCb->mutex);

    pthread_mutex_unlock(&pstCb->mutex);
}

static uint32_t bVppIpC2D_ShouldBypass(t_StVppIpC2DCb *pstCb)
{
    uint32_t u32InQSz, u32BufReqSz;
    t_StVppBuf *pstBuf;

    VPP_RET_IF_NULL(pstCb, VPP_FALSE);

    u32InQSz = u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ);
    u32BufReqSz = u32VppUtils_GetPxBufferSize(&pstCb->stInput.stParam);

    if (u32InQSz)
    {
        if (pstCb->stInput.stParam.fmt == pstCb->stOutput.stParam.fmt &&
            pstCb->stInput.stParam.width == pstCb->stOutput.stParam.width &&
            pstCb->stInput.stParam.height == pstCb->stOutput.stParam.height)
        {
            LOGI("Input/Output match: W=%u, H=%u, fmt=%d. No processing needed",
                 pstCb->stInput.stParam.width, pstCb->stInput.stParam.height,
                 pstCb->stInput.stParam.fmt);
            return VPP_TRUE;
        }

        pstBuf = pstVppIp_PortBufPeek(&pstCb->stInput, 0, NULL);
        if (!pstBuf)
        {
            LOGE("u32InQSz != 0 but peek returned NULL buffer!");
            return VPP_FALSE;
        }

        if (VPP_FLAG_IS_SET(pstBuf->u32InternalFlags, VPP_BUF_FLAG_BYPASS))
            return VPP_TRUE;

        if (pstBuf->stPixel.u32FilledLen < u32BufReqSz ||
            pstBuf->stPixel.u32ValidLen < u32BufReqSz)
        {
            LOGE("Buffer u32FilledLen=%u, u32ValidLen=%u, Required=%u",
                 pstBuf->stPixel.u32FilledLen, pstBuf->stPixel.u32ValidLen,
                 u32BufReqSz);
            return VPP_TRUE;
        }
    }

    return VPP_FALSE;
}

static uint32_t u32VppIpC2D_ProcBufReqMet(t_StVppIpC2DCb *pstCb)
{
    // Determine if the buffers in the ports satisfy the requirements
    // to trigger processing
    uint32_t u32InQSz, u32OutQSz;

    // This function requires that the caller has already locked the mutex
    // which guards these two queues.
    u32InQSz = u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ);
    u32OutQSz = u32VppBufPool_Cnt(&pstCb->stOutput.stPendingQ);

    LOGI("CHECK: u32InQSz=%u, u32OutQSz=%u, u32InputMin=%u, u32OutputMin=%u",
         u32InQSz, u32OutQSz, pstCb->u32InputMin, pstCb->u32OutputMin);

    if (u32InQSz == 0)
        return VPP_FALSE;

    if (bVppIpC2D_ShouldBypass(pstCb))
        return VPP_TRUE;

    if (u32InQSz >= pstCb->u32InputMin && u32OutQSz >= pstCb->u32OutputMin)
    {
        return VPP_TRUE;
    }

    return VPP_FALSE;
}

static uint32_t u32VppIpC2D_WorkerThreadShouldSleep(t_StVppIpC2DCb *pstCb)
{
    uint32_t u32Ret = VPP_TRUE;
    uint32_t u32CmdQSz, u32ProcMet;

    // This is a predicate function used for determining if the function worker
    // thread should sleep or not. Worker thread uses a condition variable to
    // wake itself and the mutex which is used is the same as that which guards
    // these functions. Therefore, there is no need to lock a mutex prior to
    // checking the command queues within this context.
    u32CmdQSz = u32VppIp_NtsCmdCnt(&pstCb->stCmdQ, NULL);
    u32ProcMet = u32VppIpC2D_ProcBufReqMet(pstCb);

    if (u32CmdQSz)
    {
        u32Ret = VPP_FALSE;
    }
    else if (pstCb->eState == VPP_IP_STATE_ACTIVE && u32ProcMet)
    {
        u32Ret = VPP_FALSE;
    }

    LOGI("CHECK: shouldSleep=%u, u32CmdQSz=%u, u32ProcMet=%u", u32Ret,
         u32CmdQSz, u32ProcMet);

    return u32Ret;
}

static uint32_t u32VppIpC2D_ValidatePortParam(struct vpp_port_param stParam)
{
    if (!u32VppUtils_IsFmtUbwc(stParam.fmt))
    {
        if (stParam.width > stParam.stride)
        {
            LOGE("validation failed: width=%u > stride=%u for fmt=%d",
                 stParam.width, stParam.stride, stParam.fmt);
            return VPP_ERR;
        }
        if (stParam.height > stParam.scanlines)
        {
            LOGE("validation failed: height=%u > scanlines=%u for fmt=%d",
                 stParam.height, stParam.scanlines, stParam.fmt);
            return VPP_ERR;
        }
    }
    if (stParam.width < C2D_PORT_PARAM_WIDTH_MIN)
    {
        LOGE("validation failed: width=%u, min=%u", stParam.width,
             C2D_PORT_PARAM_WIDTH_MIN);
        return VPP_ERR;
    }
    if (stParam.height < C2D_PORT_PARAM_HEIGHT_MIN)
    {
        LOGE("validation failed: height=%u, min=%u", stParam.height,
             C2D_PORT_PARAM_HEIGHT_MIN);
        return VPP_ERR;
    }
    if (stParam.fmt != VPP_COLOR_FORMAT_NV12_VENUS &&
        stParam.fmt != VPP_COLOR_FORMAT_RGBA8 &&
        stParam.fmt != VPP_COLOR_FORMAT_BGRA8 &&
        stParam.fmt != VPP_COLOR_FORMAT_UBWC_NV12 &&
        stParam.fmt != VPP_COLOR_FORMAT_UBWC_RGBA8 &&
        stParam.fmt != VPP_COLOR_FORMAT_UBWC_BGRA8 &&
        stParam.fmt != VPP_COLOR_FORMAT_UBWC_RGB565 &&
        stParam.fmt != VPP_COLOR_FORMAT_UBWC_BGR565)
    {
        LOGE("validation failed: fmt=%u", stParam.fmt);
        return VPP_ERR;
    }

    return VPP_OK;
}

static uint32_t u32VppIpC2D_ValidatePortParams(t_StVppIpC2DCb *pstCb,
                                               struct vpp_port_param stInput,
                                               struct vpp_port_param stOutput)
{
    VPP_UNUSED(pstCb);

    if (stInput.height != stOutput.height)
    {
        LOGE("validation failed: height, input: %u, output: %u",
             stInput.height, stOutput.height);
        return VPP_ERR;
    }
    if (stInput.width != stOutput.width)
    {
        LOGE("validation failed: width, input: %u, output: %u",
             stInput.width, stOutput.width);
        return VPP_ERR;
    }
    if (u32VppIpC2D_ValidatePortParam(stInput) != VPP_OK)
    {
        LOGE("validation failed: input port params.");
        return VPP_ERR;
    }
    if (u32VppIpC2D_ValidatePortParam(stOutput) != VPP_OK)
    {
        LOGE("validation failed: output port params.");
        return VPP_ERR;
    }
    if (u32VppUtils_IsFmtRgb(stOutput.fmt) &&
        !u32VppUtils_IsFmtRgb(stInput.fmt))
    {
        // C2D can do this, but disabling because we don't know how to
        // fill the graphics gralloc metadata
        LOGE("validation failed: converting from YUV to RGB not supported");
        return VPP_ERR;
    }

    return VPP_OK;
}

static uint32_t u32VppIpC2D_ValidateConfig(t_StVppIpC2DCb *pstCb)
{
    return u32VppIpC2D_ValidatePortParams(pstCb, pstCb->stInput.stParam,
                                          pstCb->stOutput.stParam);
}

static uint32_t u32VppIpC2D_DriverSessCntDecrement(t_StVppIpC2DCb *pstCb)
{
    uint32_t u32Ret = VPP_OK;
    C2D_STATUS rc;

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);

    pthread_mutex_lock(&pstCb->pstGlobal->mutex);
    if (pstCb->pstGlobal->u32DriverSessCnt == 0)
    {
        LOGE("C2D driver session count already 0! Can't decrement");
        u32Ret = VPP_ERR_INVALID_CFG;
    }
    else
    {
        pstCb->pstGlobal->u32DriverSessCnt--;
        if (pstCb->pstGlobal->u32DriverSessCnt == 0)
        {
            rc = c2dDriverDeInit();
            if (rc != C2D_STATUS_OK)
            {
                LOGE("c2dDriverDeInit failed, rc=%d", rc);
                u32Ret = VPP_ERR_HW;
            }
            pstCb->pstGlobal->bC2DDriverInited = VPP_FALSE;
        }
        else
        {
            LOGD("Not deinitializing C2D driver, still %u session(s) open",
                 pstCb->pstGlobal->u32DriverSessCnt);
        }
    }
    pthread_mutex_unlock(&pstCb->pstGlobal->mutex);

    return u32Ret;
}

static uint32_t u32VppIpC2D_InitC2DSurfaces(t_StVppIpC2DCb *pstCb)
{
    uint32_t u32SurfaceType, u32Ret = VPP_OK;
    C2D_STATUS rc;
    C2D_DRIVER_SETUP_INFO stC2dSetup;
    C2D_OBJECT *pstC2dBlit;
    static const uint32_t u32SurfaceRgb = C2D_SURFACE_RGB_HOST |
        C2D_SURFACE_WITH_PHYS | C2D_SURFACE_WITH_PHYS_DUMMY;
    static const uint32_t u32SurfaceYuv = C2D_SURFACE_YUV_HOST |
        C2D_SURFACE_WITH_PHYS | C2D_SURFACE_WITH_PHYS_DUMMY;

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);

    stC2dSetup.max_object_list_needed = C2D_INIT_MAX_OBJECT;
    stC2dSetup.max_surface_template_needed = C2D_INIT_MAX_TEMPLATE;

    pthread_mutex_lock(&pstCb->pstGlobal->mutex);
    VPP_IP_PROF_START(&pstCb->stBase, C2D_STAT_DRIVER_INIT);
    if (pstCb->pstGlobal->bC2DDriverInited == VPP_FALSE)
    {
        rc = c2dDriverInit(NULL);
        VPP_IP_PROF_STOP(&pstCb->stBase, C2D_STAT_DRIVER_INIT);
        if (rc != C2D_STATUS_OK)
        {
            LOGE("Error initializing C2D driver! rc=%d", rc);
            u32Ret = VPP_ERR_HW;
            pthread_mutex_unlock(&pstCb->pstGlobal->mutex);
            goto ERR_C2D_DRIVER_INIT;
        }
        pstCb->pstGlobal->bC2DDriverInited = VPP_TRUE;
    }
    pstCb->pstGlobal->u32DriverSessCnt++;
    pthread_mutex_unlock(&pstCb->pstGlobal->mutex);

    pstCb->stCfg.u32ComputeMask = C2D_COMPUTE_SURFACE;
    vVppIpC2D_Compute(pstCb);

    u32SurfaceType = u32VppUtils_IsFmtRgb(pstCb->stInput.stParam.fmt) ?
        u32SurfaceRgb : u32SurfaceYuv;
    VPP_IP_PROF_START(&pstCb->stBase, C2D_STAT_CREATE_SURFACE);
    rc = c2dCreateSurface(&pstCb->c2d.stIn.u32SurfaceId, C2D_SOURCE,
                          u32SurfaceType, &pstCb->c2d.stIn.stSurface);
    VPP_IP_PROF_STOP(&pstCb->stBase, C2D_STAT_CREATE_SURFACE);

    if (rc != C2D_STATUS_OK)
    {
        LOGE("Error c2dCreateSurface source, rc=%d", rc);
        u32Ret = VPP_ERR_HW;
        goto ERR_C2D_CREATE_SOURCE;
    }
    LOGI("Created source surface ID=%u", pstCb->c2d.stIn.u32SurfaceId);

    u32SurfaceType = u32VppUtils_IsFmtRgb(pstCb->stOutput.stParam.fmt) ?
        u32SurfaceRgb : u32SurfaceYuv;
    VPP_IP_PROF_START(&pstCb->stBase, C2D_STAT_CREATE_SURFACE);
    rc = c2dCreateSurface(&pstCb->c2d.stOut.u32SurfaceId, C2D_TARGET,
                          u32SurfaceType, &pstCb->c2d.stOut.stSurface);
    VPP_IP_PROF_STOP(&pstCb->stBase, C2D_STAT_CREATE_SURFACE);

    if (rc != C2D_STATUS_OK)
    {
        LOGE("Error c2dCreateSurface target, rc=%d", rc);
        u32Ret = VPP_ERR_HW;
        goto ERR_C2D_CREATE_TARGET;
    }
    LOGI("Created target surface ID=%u", pstCb->c2d.stOut.u32SurfaceId);

    pstC2dBlit = &pstCb->c2d.stBlit;

    pstC2dBlit->config_mask = (C2D_TARGET_RECT_BIT | C2D_NO_BILINEAR_BIT |
                               C2D_NO_ANTIALIASING_BIT | C2D_NO_PIXEL_ALPHA_BIT |
                               C2D_ALPHA_BLEND_NONE);
    pstC2dBlit->surface_id = pstCb->c2d.stIn.u32SurfaceId;

    return u32Ret;

ERR_C2D_CREATE_TARGET:
    rc = c2dDestroySurface(pstCb->c2d.stIn.u32SurfaceId);
    LOGE_IF(rc != C2D_STATUS_OK, "c2dDestroySurface source failed, rc=%d", rc);

ERR_C2D_CREATE_SOURCE:
    u32VppIpC2D_DriverSessCntDecrement(pstCb);

ERR_C2D_DRIVER_INIT:
    return u32Ret;
}

static uint32_t u32VppIpC2D_ProcCmdOpen(t_StVppIpC2DCb *pstCb)
{
    uint32_t u32Ret;

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);

    u32Ret = u32VppIpC2D_InitC2DSurfaces(pstCb);

    if (u32Ret == VPP_OK)
        VPP_IP_STATE_SET(pstCb, VPP_IP_STATE_ACTIVE);

    pstCb->async_res.u32OpenRet = u32Ret;

    LOGI("u32VppIpC2D_ProcCmdOpen() posting semaphore, u32Ret=%u", u32Ret);
    sem_post(&pstCb->sem);

    return u32Ret;
}

static uint32_t u32VppIpC2D_TermC2DSurfaces(t_StVppIpC2DCb *pstCb)
{
    uint32_t u32, u32Ret = VPP_OK;
    C2D_STATUS rc;

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);

    rc = c2dDestroySurface(pstCb->c2d.stOut.u32SurfaceId);
    if (rc != C2D_STATUS_OK)
    {
        LOGE("Error destroying target C2D surface! rc=%d", rc);
        u32Ret = VPP_ERR_HW;
    }

    rc = c2dDestroySurface(pstCb->c2d.stIn.u32SurfaceId);
    if (rc != C2D_STATUS_OK)
    {
        LOGE("Error destroying source C2D surface! rc=%d", rc);
        u32Ret = VPP_ERR_HW;
    }

    u32 = u32VppIpC2D_DriverSessCntDecrement(pstCb);
    if (u32 != VPP_OK)
    {
        LOGE("Error decrementing C2D driver session count u32=%u", u32);
        u32Ret = u32;
    }

    return u32Ret;
}

static uint32_t u32VppIpC2D_ProcCmdClose(t_StVppIpC2DCb *pstCb)
{
    uint32_t u32Ret;

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);

    u32Ret = u32VppIpC2D_TermC2DSurfaces(pstCb);

    pstCb->async_res.u32CloseRet = u32Ret;

    LOGI("u32VppIpC2D_ProcCmdClose() posting semaphore, u32Ret=%u", u32Ret);
    sem_post(&pstCb->sem);

    return u32Ret;
}

static uint32_t u32VppIpC2D_FlushPort(t_StVppIpC2DCb *pstCb, enum vpp_port ePort)
{
    t_StVppBuf *pBuf;
    t_StVppIpPort *pstPort;

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);
    if (ePort == VPP_PORT_INPUT)
    {
        pstPort = &pstCb->stInput;
    }
    else if (ePort == VPP_PORT_OUTPUT)
    {
        pstPort = &pstCb->stOutput;
    }
    else
    {
        return VPP_ERR_PARAM;
    }

    while (u32VppIp_PortBufGet(pstPort, &pBuf, &pstCb->mutex) == VPP_OK)
    {
        VPP_FLAG_SET(pBuf->u32InternalFlags, VPP_BUF_FLAG_FLUSHED);
        if (ePort == VPP_PORT_OUTPUT)
            pBuf->stPixel.u32FilledLen = 0;
        vVppIpCbLog(&pstCb->stBase.stCb, pBuf, eVppLogId_IpBufDone);
        u32VppIp_CbBufDone(&pstCb->stBase.stCb, ePort, pBuf);
    }

    return VPP_OK;
}

static uint32_t u32VppIpC2D_ProcCmdFlush(t_StVppIpC2DCb *pstCb,
                                         t_StVppIpCmd *pstCmd)
{
    uint32_t u32;
    t_StVppEvt stEvt;

    // Flush Port
    u32 = u32VppIpC2D_FlushPort(pstCb, pstCmd->flush.ePort);

    if (u32 == VPP_OK)
    {
        stEvt.eType = VPP_EVT_FLUSH_DONE;
        stEvt.flush.ePort = pstCmd->flush.ePort;
        u32VppIpCbEvent(&pstCb->stBase.stCb, stEvt);
    }

    return u32;
}

static uint32_t u32VppIpC2D_ProcCmdDrain(t_StVppIpC2DCb *pstCb)
{
    t_StVppEvt stEvt;

    pthread_mutex_lock(&pstCb->mutex);

    if (!u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ))
    {
        // No more input buffers in the queue, drain has already been
        // completed.
        stEvt.eType = VPP_EVT_DRAIN_DONE;
        u32VppIpCbEvent(&pstCb->stBase.stCb, stEvt);
    }
    else
    {
        VPP_FLAG_SET(pstCb->u32InternalFlags, IP_DRAIN_PENDING);
    }

    pthread_mutex_unlock(&pstCb->mutex);

    return VPP_OK;
}

static uint32_t u32VppIpC2D_ProcessBuffers(t_StVppIpC2DCb *pstCb,
                                           t_StVppBuf *pstBufIn,
                                           t_StVppBuf *pstBufOut)
{
    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstBufIn, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstBufOut, VPP_ERR_PARAM);

    uint32_t u32Ret = VPP_OK;
    void *pvGpuAddrBufIn, *pvGpuAddrBufOut;
    uint32_t u32BufSize, u32UVOffset;
    C2D_YUV_SURFACE_DEF *pstYuvSurface;
    C2D_RGB_SURFACE_DEF *pstRgbSurface;
    C2D_STATUS rc;

    u32BufSize = u32VppUtils_GetBufferSize(&pstCb->stInput.stParam);
    rc = c2dMapAddr(pstBufIn->pBuf->pixel.fd, NULL,
                    u32BufSize, 0, KGSL_USER_MEM_TYPE_ION, &pvGpuAddrBufIn);
    if (rc != C2D_STATUS_OK)
    {
        LOGE("Error c2dMapAddr IN fd=%d pvBase=%p, BufSz=%u, rc=%d, ",
             pstBufIn->pBuf->pixel.fd, pstBufIn->stPixel.pvBase,
             u32BufSize, rc);
        u32Ret = VPP_ERR_HW;
        goto ERR_C2D_MAP_IN;
    }

    if (u32VppUtils_IsFmtRgb(pstCb->stInput.stParam.fmt))
    {
        pstRgbSurface = &pstCb->c2d.stIn.stSurface.stRgbSurface;
        pstRgbSurface->buffer = pvGpuAddrBufIn;
        pstRgbSurface->phys = pvGpuAddrBufIn;

        rc = c2dUpdateSurface(pstCb->c2d.stIn.u32SurfaceId, C2D_SOURCE,
                              C2D_SURFACE_RGB_HOST | C2D_SURFACE_WITH_PHYS,
                              pstRgbSurface);
    }
    else
    {
        u32UVOffset  = u32VppUtils_GetUVOffset(&pstCb->stInput.stParam);
        pstYuvSurface = &pstCb->c2d.stIn.stSurface.stYuvSurface;
        pstYuvSurface->plane0 = pvGpuAddrBufIn;
        pstYuvSurface->phys0 = pvGpuAddrBufIn;
        pstYuvSurface->plane1 = (uint8_t *)pvGpuAddrBufIn + u32UVOffset;
        pstYuvSurface->phys1 = (uint8_t *)pvGpuAddrBufIn + u32UVOffset;

        rc = c2dUpdateSurface(pstCb->c2d.stIn.u32SurfaceId, C2D_SOURCE,
                              C2D_SURFACE_YUV_HOST | C2D_SURFACE_WITH_PHYS,
                              pstYuvSurface);
    }

    if (rc != C2D_STATUS_OK)
    {
        LOGE("Error c2dUpdateSurface stIn.u32SurfaceId=%u, rc=%d",
             pstCb->c2d.stIn.u32SurfaceId, rc);
        u32Ret = VPP_ERR_HW;
        goto ERR_C2D_UPDATE_SURFACE_IN;
    }

    u32BufSize = u32VppUtils_GetBufferSize(&pstCb->stOutput.stParam);
    rc = c2dMapAddr(pstBufOut->pBuf->pixel.fd, NULL,
                    u32BufSize, 0, KGSL_USER_MEM_TYPE_ION, &pvGpuAddrBufOut);
    if (rc != C2D_STATUS_OK)
    {
        LOGE("Error c2dMapAddr OUT fd=%d pvBase=%p, BufSz=%u, rc=%d",
             pstBufOut->pBuf->pixel.fd, pstBufOut->stPixel.pvBase,
             u32BufSize, rc);
        u32Ret = VPP_ERR_HW;
        goto ERR_C2D_MAP_OUT;
    }

    if (u32VppUtils_IsFmtRgb(pstCb->stOutput.stParam.fmt))
    {
        pstRgbSurface = &pstCb->c2d.stOut.stSurface.stRgbSurface;
        pstRgbSurface->buffer = pvGpuAddrBufOut;
        pstRgbSurface->phys = pvGpuAddrBufOut;

        rc = c2dUpdateSurface(pstCb->c2d.stOut.u32SurfaceId, C2D_TARGET,
                              C2D_SURFACE_RGB_HOST | C2D_SURFACE_WITH_PHYS,
                              pstRgbSurface);
    }
    else
    {
        u32UVOffset = u32VppUtils_GetUVOffset(&pstCb->stOutput.stParam);
        pstYuvSurface = &pstCb->c2d.stOut.stSurface.stYuvSurface;
        pstYuvSurface->plane0 = pvGpuAddrBufOut;
        pstYuvSurface->phys0 = pvGpuAddrBufOut;
        pstYuvSurface->plane1 = (uint8_t *)pvGpuAddrBufOut + u32UVOffset;
        pstYuvSurface->phys1 = (uint8_t *)pvGpuAddrBufOut + u32UVOffset;

        rc = c2dUpdateSurface(pstCb->c2d.stOut.u32SurfaceId, C2D_TARGET,
                              C2D_SURFACE_YUV_HOST | C2D_SURFACE_WITH_PHYS,
                              pstYuvSurface);
    }
    if (rc != C2D_STATUS_OK)
    {
        LOGE("Error c2dUpdateSurface stOut.u32SurfaceId=%u,  rc=%d",
             pstCb->c2d.stOut.u32SurfaceId, rc);
        u32Ret = VPP_ERR_HW;
        goto ERR_C2D_UPDATE_SURFACE_OUT;
    }

    LOGI("About to call c2dDraw, pvGpuAddrBufIn=%p, pvGpuAddrBufOut=%p",
         pvGpuAddrBufIn, pvGpuAddrBufOut);
    VPP_IP_PROF_START(&pstCb->stBase, C2D_STAT_PROC);
    rc = c2dDraw(pstCb->c2d.stOut.u32SurfaceId, C2D_TARGET_ROTATE_0,
                 NULL, 0, 0, &pstCb->c2d.stBlit, 1);
    if (rc != C2D_STATUS_OK)
    {
        VPP_IP_PROF_STOP(&pstCb->stBase, C2D_STAT_PROC);
        LOGE("Error c2dDraw rc=%d", rc);
        u32Ret = VPP_ERR;
        goto ERR_C2D_DRAW;
    }

    rc = c2dFinish(pstCb->c2d.stOut.u32SurfaceId);
    VPP_IP_PROF_STOP(&pstCb->stBase, C2D_STAT_PROC);
    if (rc != C2D_STATUS_OK)
    {
        pstBufOut->stPixel.u32FilledLen = 0;
        LOGE("Error c2dFinish rc=%d", rc);
        u32Ret = VPP_ERR;
    }
    else
    {
        u32BufSize = u32VppUtils_GetPxBufferSize(&pstCb->stOutput.stParam);
        pstBufOut->stPixel.u32FilledLen = u32BufSize;
        pstBufOut->pBuf->timestamp = pstBufIn->pBuf->timestamp;
        pstBufOut->pBuf->flags = pstBufIn->pBuf->flags;
        pstBufOut->pBuf->cookie_in_to_out = pstBufIn->pBuf->cookie_in_to_out;
        pstBufOut->u32TimestampFrameRate = pstBufIn->u32TimestampFrameRate;
        pstBufOut->u32OperatingRate = pstBufIn->u32OperatingRate;
        u32VppBuf_PropagateExtradata(pstBufIn, pstBufOut, pstBufIn->eBufType,
                                     VPP_EXTERNAL_EXTRADATA_TYPE);
        u32VppBuf_GrallocMetadataCopy(pstBufIn, pstBufOut);
        if (u32VppUtils_IsFmtRgb(pstCb->stInput.stParam.fmt) !=
            u32VppUtils_IsFmtRgb(pstCb->stOutput.stParam.fmt))
        {
            u32VppBuf_GrallocMetadataColorFmtSet(pstBufOut,
                                                 pstCb->stOutput.stParam.fmt);
        }
        LOGI("c2dDraw successful, rc=%d", rc);
    }

ERR_C2D_DRAW:
ERR_C2D_UPDATE_SURFACE_OUT:
    rc = c2dUnMapAddr(pvGpuAddrBufOut);
    LOGE_IF(rc != C2D_STATUS_OK, "Error c2dUnMapAddr OUT, rc=%d", rc);

ERR_C2D_MAP_OUT:
ERR_C2D_UPDATE_SURFACE_IN:
    rc = c2dUnMapAddr(pvGpuAddrBufIn);
    LOGE_IF(rc != C2D_STATUS_OK, "Error c2dUnMapAddr IN, rc=%d", rc);

ERR_C2D_MAP_IN:
    return u32Ret;
}

static uint32_t u32VppIpC2D_Process(t_StVppIpC2DCb *pstCb)
{
    uint32_t u32Ret, u32BufSz, u32AvailSz, u32OutMin, bBypass;
    t_StVppMemBuf *pstIntBuf;
    t_StVppBuf *pstBufIn = NULL, *pstBufOut = NULL;

    LOG_ENTER();

    pthread_mutex_lock(&pstCb->mutex);

    u32OutMin = pstCb->u32OutputMin;

    bBypass = bVppIpC2D_ShouldBypass(pstCb);
    u32VppIp_PortBufGet(&pstCb->stInput, &pstBufIn, NULL);
    if (!pstBufIn)
    {
        LOGE("port_buf_get returned null buffer!");
        pthread_mutex_unlock(&pstCb->mutex);
        LOG_EXIT_RET(VPP_ERR);
    }

    if (!bBypass)
        u32VppIp_PortBufGet(&pstCb->stOutput, &pstBufOut, NULL);

    pthread_mutex_unlock(&pstCb->mutex);

    if (pstBufIn && pstBufOut)
    {
        // Make sure that output buffer size is sufficient for input buffer
        pstIntBuf = &pstBufOut->stPixel;

        u32BufSz = u32VppUtils_GetPxBufferSize(&pstCb->stOutput.stParam);
        u32AvailSz = pstIntBuf->u32AllocLen - pstIntBuf->u32Offset;
        if (u32AvailSz < u32BufSz)
        {
            LOGE("required buffer size exceeds available, bypassing: req_sz=%u, "
                 "buf:{fd=%d, u32Offset=%u, u32AllocLen=%u, u32FilledLen=%u}",
                 u32BufSz, pstIntBuf->fd, pstIntBuf->u32Offset,
                 pstIntBuf->u32AllocLen, pstIntBuf->u32FilledLen);

            // Send input buffer to output, return output buffer
            C2D_STATS(pstCb, OBDCnt);
            vVppIpCbLog(&pstCb->stBase.stCb, pstBufIn, eVppLogId_IpBufDone);
            u32VppIp_CbBufDone(&pstCb->stBase.stCb, VPP_PORT_OUTPUT, pstBufIn);

            pstBufOut->stPixel.u32FilledLen = 0;
            C2D_STATS(pstCb, OBDCnt);
            vVppIpCbLog(&pstCb->stBase.stCb, pstBufOut, eVppLogId_IpBufDone);
            u32VppIp_CbBufDone(&pstCb->stBase.stCb, VPP_PORT_OUTPUT, pstBufOut);

            LOG_EXIT_RET(VPP_ERR);
        }

        // Normal processing
        C2D_STATS(pstCb, InProcCnt);
        C2D_STATS(pstCb, OutProcCnt);

        if(VPP_FLAG_IS_SET(pstBufIn->u32InternalFlags, VPP_BUF_FLAG_DUMP))
        {
            C2D_DUMP_FRAME(pstCb, pstBufIn, VPP_PORT_INPUT);
        }

        vVppIpCbLog(&pstCb->stBase.stCb, pstBufIn, eVppLogId_IpProcStart);
        vVppIpCbLog(&pstCb->stBase.stCb, pstBufOut, eVppLogId_IpProcStart);

        u32Ret = u32VppIpC2D_ProcessBuffers(pstCb, pstBufIn, pstBufOut);

        vVppIpCbLog(&pstCb->stBase.stCb, pstBufIn, eVppLogId_IpProcDone);
        vVppIpCbLog(&pstCb->stBase.stCb, pstBufOut, eVppLogId_IpProcDone);

        if(VPP_FLAG_IS_SET(pstBufOut->u32InternalFlags, VPP_BUF_FLAG_DUMP))
        {
            C2D_DUMP_FRAME(pstCb, pstBufOut, VPP_PORT_OUTPUT);
        }

        if (u32Ret != VPP_OK)
        {
            LOGE("error processing buffer, bypassing, u32Ret=%u", u32Ret);
            C2D_STATS(pstCb, OBDCnt);
            vVppIpCbLog(&pstCb->stBase.stCb, pstBufIn, eVppLogId_IpBufDone);
            u32VppIp_CbBufDone(&pstCb->stBase.stCb, VPP_PORT_OUTPUT, pstBufIn);
        }
        else
        {
            C2D_STATS(pstCb, IBDCnt);
            vVppIpCbLog(&pstCb->stBase.stCb, pstBufIn, eVppLogId_IpBufDone);
            u32VppIp_CbBufDone(&pstCb->stBase.stCb, VPP_PORT_INPUT, pstBufIn);
        }

        C2D_STATS(pstCb, OBDCnt);
        vVppIpCbLog(&pstCb->stBase.stCb, pstBufOut, eVppLogId_IpBufDone);
        u32VppIp_CbBufDone(&pstCb->stBase.stCb, VPP_PORT_OUTPUT, pstBufOut);
    }
    else
    {
        // Bypass
        C2D_STATS(pstCb, OBDCnt);
        LOGI("bypassing input buffer");
        vVppIpCbLog(&pstCb->stBase.stCb, pstBufIn, eVppLogId_IpBufDone);
        u32VppIp_CbBufDone(&pstCb->stBase.stCb, VPP_PORT_OUTPUT, pstBufIn);
    }

    LOG_EXIT_RET(VPP_OK);
}

static void vVppIpC2D_HandlePendingDrain(t_StVppIpC2DCb *pstCb)
{
    t_StVppEvt stEvt;

    pthread_mutex_lock(&pstCb->mutex);

    if(VPP_FLAG_IS_SET(pstCb->u32InternalFlags, IP_DRAIN_PENDING) &&
       !u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ))
    {
        VPP_FLAG_CLR(pstCb->u32InternalFlags, IP_DRAIN_PENDING);
        stEvt.eType = VPP_EVT_DRAIN_DONE;
        u32VppIpCbEvent(&pstCb->stBase.stCb, stEvt);
    }

    pthread_mutex_unlock(&pstCb->mutex);
}

static void *vpVppIpC2D_Worker(void *pv)
{
    t_StVppIpC2DCb *pstCb = (t_StVppIpC2DCb *)pv;

    LOGD("%s started", __func__);
    vVppIpC2D_SignalWorkerStart(pstCb);

    while (1)
    {
        pthread_mutex_lock(&pstCb->mutex);
        while (u32VppIpC2D_WorkerThreadShouldSleep(pstCb))
            pthread_cond_wait(&pstCb->cond, &pstCb->mutex);

        uint32_t u32Ret;
        t_StVppIpCmd stCmd;
        u32Ret = u32VppIp_NtsCmdGet(&pstCb->stCmdQ, &stCmd,
                                    NULL);
        if (u32Ret == VPP_OK)
        {
            pthread_mutex_unlock(&pstCb->mutex);

            // Process the command
            LOG_CMD("ProcessCmd", stCmd.eCmd);

            if (stCmd.eCmd == VPP_IP_CMD_THREAD_EXIT)
            {
                break;
            }

            else if (stCmd.eCmd == VPP_IP_CMD_OPEN)
                u32VppIpC2D_ProcCmdOpen(pstCb);

            else if (stCmd.eCmd == VPP_IP_CMD_CLOSE)
                u32VppIpC2D_ProcCmdClose(pstCb);

            else if (stCmd.eCmd == VPP_IP_CMD_FLUSH)
                u32VppIpC2D_ProcCmdFlush(pstCb, &stCmd);

            else if (stCmd.eCmd == VPP_IP_CMD_DRAIN)
                u32VppIpC2D_ProcCmdDrain(pstCb);

            else
                LOGE("unknown command in queue: %d", stCmd.eCmd);

            continue;
        }

        if (pstCb->eState == VPP_IP_STATE_ACTIVE)
        {
            pthread_mutex_unlock(&pstCb->mutex);
            u32VppIpC2D_Process(pstCb);
            vVppIpC2D_HandlePendingDrain(pstCb);
            continue;
        }

        LOGE("%s woke up, but did no work", __func__);
        pthread_mutex_unlock(&pstCb->mutex);
    }

    LOGD("%s exited", __func__);

    return NULL;
}

/************************************************************************
 * Global Functions
 ***********************************************************************/

void *vpVppIpC2D_Init(t_StVppCtx *pstCtx, uint32_t u32Flags,
                       t_StVppCallback cbs)
{
    int rc;
    uint32_t u32;
    t_StVppIpC2DCb *pstCb;

    LOG_ENTER_ARGS("flags=0x%x", u32Flags);

    VPP_RET_IF_NULL(pstCtx, NULL);

    u32 = u32VppIp_ValidateCallbacks(&cbs);
    if (u32 != VPP_OK)
    {
        LOGE("Invalid callbacks.");
        goto ERROR_CALLBACKS;
    }

    pstCb = calloc(1, sizeof(t_StVppIpC2DCb));
    if (!pstCb)
    {
        LOGE("calloc failed for c2d context");
        goto ERROR_MALLOC;
    }

    u32VppIp_SetBase(pstCtx, u32Flags, cbs, &pstCb->stBase);

    u32 = VPP_IP_PROF_REGISTER_BY_NM(&pstCb->stBase, C2DStats);
    LOGE_IF(u32 != VPP_OK, "unable to register stats, u32=%u", u32);

    u32 = u32VppIp_PortInit(&pstCb->stInput);
    if (u32 != VPP_OK)
    {
        LOGE("unable to u32VppIp_PortInit() input queue u32=%u", u32);
        goto ERROR_INPUT_Q_INIT;
    }

    u32 = u32VppIp_PortInit(&pstCb->stOutput);
    if (u32 != VPP_OK)
    {
        LOGE("unable to u32VppIp_PortInit() output queue u32=%u", u32);
        goto ERROR_OUTPUT_Q_INIT;
    }

    u32 = u32VppIp_NtsCmdQueueInit(&pstCb->stCmdQ, C2D_CMD_Q_SZ);
    if (u32 != VPP_OK)
    {
        LOGE("unable to initialize command queue u32=%u", u32);
        goto ERROR_CMD_Q_INIT;
    }

    rc = sem_init(&pstCb->sem, 0, 0);
    if (rc)
    {
        LOGE("sem_init failed: %d --> %s", rc, strerror(rc));
        goto ERROR_SEM_INIT;
    }

    rc = pthread_mutex_init(&pstCb->mutex, NULL);
    if (rc)
    {
        LOGE("pthread_mutex_init failed: %d --> %s", rc, strerror(rc));
        goto ERROR_MUTEX_INIT;
    }

    rc = pthread_cond_init(&pstCb->cond, NULL);
    if (rc)
    {
        LOGE("pthread_cond_init failed: %d --> %s", rc, strerror(rc));
        goto ERROR_COND_INIT;
    }

    rc = pthread_create(&pstCb->thread, NULL, vpVppIpC2D_Worker, pstCb);
    if (rc)
    {
        LOGE("pthread_create failed: %d --> %s", rc, strerror(rc));
        goto ERROR_THREAD_CREATE;
    }
    pstCb->pstGlobal = &stGlobalCb;

    // Set the default input to output requirement
    pstCb->u32InputMin = 1;
    pstCb->u32OutputMin = 1;

    // Wait for the thread to launch before returning
    vVppIpC2D_WaitWorkerStart(pstCb);

    VPP_IP_STATE_SET(pstCb, VPP_IP_STATE_INITED);

    LOG_EXIT_RET(pstCb);

ERROR_THREAD_CREATE:
    LOGI("destroying condition variable");
    rc = pthread_cond_destroy(&pstCb->cond);
    LOGE_IF(rc, "pthread_cond_destroy failed: %d --> %s", rc, strerror(rc));

ERROR_COND_INIT:
    LOGI("destroying mutex");
    rc = pthread_mutex_destroy(&pstCb->mutex);
    LOGE_IF(rc, "pthread_mutex_destroy failed: %d --> %s", rc, strerror(rc));

ERROR_MUTEX_INIT:
    LOGI("destroying semaphore");
    rc = sem_destroy(&pstCb->sem);
    LOGE_IF(rc, "sem_destroy failed: %d --> %s", rc, strerror(rc));

ERROR_SEM_INIT:
    LOGI("destroying command queue");
    u32 = u32VppIp_NtsCmdQueueTerm(&pstCb->stCmdQ);
    LOGE_IF(u32 != VPP_OK, "Error destroying command queue u32=%u", u32);

ERROR_CMD_Q_INIT:
    u32 = u32VppIp_PortTerm(&pstCb->stOutput);
    LOGE_IF(u32 != VPP_OK, "Error closing output queue u32=%u", u32);

ERROR_OUTPUT_Q_INIT:
    u32 = u32VppIp_PortTerm(&pstCb->stInput);
    LOGE_IF(u32 != VPP_OK, "Error closing input queue u32=%u", u32);

ERROR_INPUT_Q_INIT:
    u32 = VPP_IP_PROF_UNREGISTER(&pstCb->stBase);
    LOGE_IF(u32 != VPP_OK, "Error unregistering stats u32=%u", u32);

    free(pstCb);

ERROR_MALLOC:
ERROR_CALLBACKS:
    LOG_EXIT_RET(NULL);
}

void vVppIpC2D_Term(void *ctx)
{
    int rc;
    uint32_t u32;
    t_StVppIpC2DCb *pstCb;
    t_StVppIpCmd stCmd;

    LOG_ENTER();

    VPP_RET_VOID_IF_NULL(ctx);
    pstCb = C2D_CB_GET(ctx);

    if (pstCb->bInlineProcOnly)
    {
        LOGE("Session=%p initialized for inline processing only!", pstCb);
        return;
    }

    stCmd.eCmd = VPP_IP_CMD_THREAD_EXIT;
    u32 = u32VppIp_NtsCmdPut(&pstCb->stCmdQ, stCmd,
                             &pstCb->mutex, &pstCb->cond);
    if (u32 != VPP_OK)
    {
        LOGE("unable to queue THREAD_EXIT, u32=%u", u32);
    }
    else
    {
        rc = pthread_join(pstCb->thread, NULL);
        LOGE_IF(rc, "pthread_join failed: %d --> %s", rc, strerror(rc));
    }

    u32 = u32VppIpC2D_FlushPort(pstCb, VPP_PORT_INPUT);
    LOGE_IF(u32 != VPP_OK, "ERROR: flushing input, u32=%u", u32);

    u32 = u32VppIpC2D_FlushPort(pstCb, VPP_PORT_OUTPUT);
    LOGE_IF(u32 != VPP_OK, "ERROR: flushing output, u32=%u", u32);

    rc = pthread_cond_destroy(&pstCb->cond);
    LOGE_IF(rc, "pthread_cond_destroy failed: %d --> %s", rc, strerror(rc));

    rc = pthread_mutex_destroy(&pstCb->mutex);
    LOGE_IF(rc, "pthread_mutex_destroy failed: %d --> %s", rc, strerror(rc));

    rc = sem_destroy(&pstCb->sem);
    LOGE_IF(rc, "sem_destroy failed: %d --> %s", rc, strerror(rc));

    u32 = u32VppIp_NtsCmdQueueTerm(&pstCb->stCmdQ);
    LOGE_IF(u32 != VPP_OK, "ERROR: Command queue term, u32=%u", u32);

    u32 = u32VppIp_PortTerm(&pstCb->stInput);
    LOGE_IF(u32 != VPP_OK, "ERROR: Input port term, u32=%u", u32);

    u32 = u32VppIp_PortTerm(&pstCb->stOutput);
    LOGE_IF(u32 != VPP_OK, "ERROR: Output port term, u32=%u", u32);

    u32 = VPP_IP_PROF_UNREGISTER(&pstCb->stBase);
    LOGE_IF(u32 != VPP_OK, "ERROR: unable to unregister stats, u32=%u", u32);

    free(pstCb);

    LOG_EXIT();
}

uint32_t u32VppIpC2D_Open(void *ctx)
{
    t_StVppIpC2DCb *pstCb;
    t_StVppIpCmd stCmd;
    uint32_t u32;

    LOG_ENTER();

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = C2D_CB_GET(ctx);

    if (pstCb->bInlineProcOnly)
    {
        LOGE("Session=%p initialized for inline processing only!", pstCb);
        return VPP_ERR_INVALID_CFG;
    }

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_INITED))
    {
        LOGE("Invalid state. state=%u, exp=%u", pstCb->eState,
             VPP_IP_STATE_INITED);
        return VPP_ERR_STATE;
    }

    // Validate that the port configuration is valid
    if (u32VppIpC2D_ValidateConfig(pstCb) != VPP_OK)
    {
        LOGE("Invalid config, can't Open");
        return VPP_ERR_PARAM;
    }
    stCmd.eCmd = VPP_IP_CMD_OPEN;
    u32 = u32VppIp_NtsCmdPut(&pstCb->stCmdQ, stCmd,
                             &pstCb->mutex, &pstCb->cond);

    if (u32 != VPP_OK)
    {
        LOGE("unable to queue OPEN, u32=%u", u32);
        return u32;
    }

    LOGI(">> waiting on semaphore");
    sem_wait(&pstCb->sem);
    LOGI(">> got semaphore");

    LOG_EXIT_RET(pstCb->async_res.u32OpenRet);
}

uint32_t u32VppIpC2D_Close(void *ctx)
{
    t_StVppIpC2DCb *pstCb;
    t_StVppIpCmd stCmd;
    uint32_t u32;

    LOG_ENTER();

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = C2D_CB_GET(ctx);

    if (pstCb->bInlineProcOnly)
    {
        LOGE("Session=%p initialized for inline processing only!", pstCb);
        return VPP_ERR_INVALID_CFG;
    }

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGE("Invalid state. state=%u, exp=%u", pstCb->eState,
             VPP_IP_STATE_ACTIVE);
        return VPP_ERR_STATE;
    }

    VPP_IP_STATE_SET(pstCb, VPP_IP_STATE_INITED);

    stCmd.eCmd = VPP_IP_CMD_CLOSE;
    u32 = u32VppIp_NtsCmdPut(&pstCb->stCmdQ, stCmd,
                             &pstCb->mutex, &pstCb->cond);

    if (u32 != VPP_OK)
    {
        LOGE("unable to queue CLOSE, u32=%u", u32);
        return u32;
    }

    LOGI(">> waiting on semaphore");
    sem_wait(&pstCb->sem);
    LOGI(">> got semaphore");

    LOG_EXIT_RET(pstCb->async_res.u32CloseRet);
}


uint32_t u32VppIpC2D_SetParam(void *ctx, enum vpp_port ePort,
                              struct vpp_port_param stParam)
{
    uint32_t u32Ret = VPP_OK;
    t_StVppIpC2DCb *pstCb;

    LOG_ENTER();

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = C2D_CB_GET(ctx);

    if (pstCb->bInlineProcOnly)
    {
        LOGE("Session=%p initialized for inline processing only!", pstCb);
        return VPP_ERR_INVALID_CFG;
    }

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_INITED))
    {
        LOGE("Invalid state. state=%u, exp=%u", pstCb->eState,
             VPP_IP_STATE_INITED);
        return VPP_ERR_STATE;
    }

    u32Ret = u32VppIpC2D_ValidatePortParam(stParam);
    if (u32Ret != VPP_OK)
    {
        LOGE("Invalid port params u32Ret=%u", u32Ret);
        return VPP_ERR_PARAM;
    }

    pthread_mutex_lock(&pstCb->mutex);

    if (ePort == VPP_PORT_INPUT)
    {
        pstCb->stInput.stParam = stParam;
    }
    else if (ePort == VPP_PORT_OUTPUT)
    {
        pstCb->stOutput.stParam = stParam;
    }
    else
    {
        LOGE("Invalid port=%d", ePort);
        u32Ret = VPP_ERR_PARAM;
    }

    LOG_PARAM_PTR(I, &pstCb->stInput.stParam, &pstCb->stOutput.stParam);

    pthread_mutex_unlock(&pstCb->mutex);

    LOG_EXIT_RET(u32Ret);
}

uint32_t u32VppIpC2D_SetCtrl(void *ctx, struct hqv_control stCtrl)
{
    LOG_ENTER();

    VPP_UNUSED(ctx);
    VPP_UNUSED(stCtrl);

    LOG_EXIT_RET(VPP_OK);
}

uint32_t u32VppIpC2D_GetBufferRequirements(void *ctx,
                                           t_StVppIpBufReq *pstInputBufReq,
                                           t_StVppIpBufReq *pstOutputBufReq)
{
    t_StVppIpC2DCb *pstCb;
    uint32_t u32PxSz;

    LOG_ENTER();

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = C2D_CB_GET(ctx);

    if (pstCb->bInlineProcOnly)
    {
        LOGE("Session=%p initialized for inline processing only!", pstCb);
        return VPP_ERR_INVALID_CFG;
    }

    if (pstInputBufReq)
    {
        u32PxSz = u32VppUtils_GetPxBufferSize(&pstCb->stInput.stParam);
        pstInputBufReq->u32MinCnt = pstCb->u32InputMin;
        pstInputBufReq->u32PxSz = u32PxSz;
        pstInputBufReq->u32ExSz = 0;
    }

    if (pstOutputBufReq)
    {
        u32PxSz = u32VppUtils_GetPxBufferSize(&pstCb->stOutput.stParam);
        pstOutputBufReq->u32MinCnt = pstCb->u32OutputMin;
        pstOutputBufReq->u32PxSz = u32PxSz;
        pstOutputBufReq->u32ExSz = 0;
    }

    LOG_EXIT_RET(VPP_OK);
}

uint32_t u32VppIpC2D_QueueBuf(void *ctx, enum vpp_port ePort,
                              t_StVppBuf *pBuf)
{
    uint32_t u32Ret = VPP_OK;
    t_StVppIpC2DCb *pstCb;

    LOG_ENTER();

    VPP_RET_IF_NULL(pBuf, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);

    pstCb = C2D_CB_GET(ctx);

    if (pstCb->bInlineProcOnly)
    {
        LOGE("Session=%p initialized for inline processing only!", pstCb);
        return VPP_ERR_INVALID_CFG;
    }

    if (ePort != VPP_PORT_INPUT && ePort != VPP_PORT_OUTPUT)
    {
        LOGE("Invalid port. port=%u", ePort);
        return VPP_ERR_PARAM;
    }

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGE("Invalid state. state=%u, exp=%u", pstCb->eState,
             VPP_IP_STATE_ACTIVE);
        return VPP_ERR_STATE;
    }

    vVppIpCbLog(&pstCb->stBase.stCb, pBuf, eVppLogId_IpQueueBuf);

    if (ePort == VPP_PORT_INPUT)
    {
        u32Ret = u32VppIp_PortBufPut(&pstCb->stInput, pBuf,
                                     &pstCb->mutex, &pstCb->cond);
        if (u32Ret == VPP_OK)
            C2D_STATS(pstCb, InQCnt);
    }
    else if (ePort == VPP_PORT_OUTPUT)
    {
        u32Ret = u32VppIp_PortBufPut(&pstCb->stOutput, pBuf,
                                     &pstCb->mutex, &pstCb->cond);
        if (u32Ret == VPP_OK)
            C2D_STATS(pstCb, OutQCnt);
    }

    LOG_EXIT_RET(u32Ret);
}

uint32_t u32VppIpC2D_Flush(void *ctx, enum vpp_port ePort)
{
    uint32_t u32Ret = VPP_OK;
    t_StVppIpC2DCb *pstCb;
    t_StVppIpCmd stCmd;

    LOG_ENTER();

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = C2D_CB_GET(ctx);

    if (pstCb->bInlineProcOnly)
    {
        LOGE("Session=%p initialized for inline processing only!", pstCb);
        return VPP_ERR_INVALID_CFG;
    }

    if (ePort != VPP_PORT_INPUT && ePort != VPP_PORT_OUTPUT)
    {
        LOGE("Invalid port. port=%u", ePort);
        return VPP_ERR_PARAM;
    }

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_INITED) &&
        !VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGE("Invalid state. state=%u, exp:%u or %u", pstCb->eState,
             VPP_IP_STATE_INITED, VPP_IP_STATE_ACTIVE);
        LOG_EXIT_RET(VPP_ERR_STATE);
    }

    stCmd.eCmd = VPP_IP_CMD_FLUSH;
    stCmd.flush.ePort = ePort;
    u32Ret = u32VppIp_NtsCmdPut(&pstCb->stCmdQ, stCmd,
                                &pstCb->mutex, &pstCb->cond);

    LOGE_IF(u32Ret != VPP_OK, "Flush failed, u32=%u", u32Ret);

    LOG_EXIT_RET(u32Ret);
}

uint32_t u32VppIpC2D_Drain(void *ctx)
{
    uint32_t u32Ret = VPP_OK;
    t_StVppIpC2DCb *pstCb;
    t_StVppIpCmd stCmd;

    LOG_ENTER();

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = C2D_CB_GET(ctx);

    if (pstCb->bInlineProcOnly)
    {
        LOGE("Session=%p initialized for inline processing only!", pstCb);
        return VPP_ERR_INVALID_CFG;
    }

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGE("Invalid state. state=%u, exp=%u", pstCb->eState,
             VPP_IP_STATE_ACTIVE);
        return VPP_ERR_STATE;
    }

    stCmd.eCmd = VPP_IP_CMD_DRAIN;
    u32Ret = u32VppIp_NtsCmdPut(&pstCb->stCmdQ, stCmd,
                                &pstCb->mutex, &pstCb->cond);

    LOGE_IF(u32Ret != VPP_OK, "Drain failed, u32=%u", u32Ret);

    LOG_EXIT_RET(u32Ret);
}

uint32_t u32VppIpC2D_Reconfigure(void *ctx,
                                 struct vpp_port_param in_param,
                                 struct vpp_port_param out_param)
{
    uint32_t u32QSz;
    uint32_t u32Ret = VPP_OK;
    t_StVppIpC2DCb *pstCb;

    LOG_ENTER();

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = C2D_CB_GET(ctx);

    if (pstCb->bInlineProcOnly)
    {
        LOGE("Session=%p initialized for inline processing only!", pstCb);
        return VPP_ERR_INVALID_CFG;
    }

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGE("Invalid state. state=%u, exp=%u", pstCb->eState,
             VPP_IP_STATE_ACTIVE);
        return VPP_ERR_STATE;
    }

    pthread_mutex_lock(&pstCb->mutex);

    u32QSz = u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ);
    if (u32QSz)
    {
        u32Ret = VPP_ERR_STATE;
        LOGE("Reconfigure called with %u buffers in input port",
             u32QSz);
    }
    else
    {
        u32Ret = u32VppIpC2D_ValidatePortParams(pstCb, in_param, out_param);
        if (u32Ret != VPP_OK)
        {
            LOGE("Invalid new port params, u32Ret=%u", u32Ret);
        }
        else
        {
            pstCb->stInput.stParam = in_param;
            pstCb->stOutput.stParam = out_param;
            LOGI("reconfigured to new params");
            LOG_PARAM_PTR(I, &pstCb->stInput.stParam, &pstCb->stOutput.stParam);
        }
    }
    pthread_mutex_unlock(&pstCb->mutex);

    if (u32Ret == VPP_OK)
    {
        pstCb->stCfg.u32ComputeMask = C2D_COMPUTE_SURFACE;
        vVppIpC2D_Compute(pstCb);
    }

    LOG_EXIT_RET(u32Ret);
}

void *vpVppIpC2D_InlineInit(t_StVppCtx *pstCtx,
                            struct vpp_port_param stInParam,
                            struct vpp_port_param stOutParam)
{
    int rc;
    uint32_t u32Ret;
    t_StVppIpC2DCb *pstCb;
    t_StVppCallback stCbs = {0};

    LOG_ENTER();

    VPP_RET_IF_NULL(pstCtx, NULL);

    pstCb = calloc(1, sizeof(t_StVppIpC2DCb));
    if (!pstCb)
    {
        LOGE("calloc failed for c2d context");
        goto ERROR_MALLOC;
    }

    u32VppIp_SetBase(pstCtx, 0, stCbs, &pstCb->stBase);

    u32Ret = VPP_IP_PROF_REGISTER_BY_NM(&pstCb->stBase, C2DStats);
    LOGE_IF(u32Ret != VPP_OK, "Unable to register stats, u32=%u", u32Ret);

    pstCb->bInlineProcOnly = VPP_TRUE;
    pstCb->pstGlobal = &stGlobalCb;

    rc = pthread_mutex_init(&pstCb->mutex, NULL);
    if (rc)
    {
        LOGE("unable to initialize c2d mutex");
        goto ERROR_MUTEX_INIT;
    }

    u32Ret = u32VppIpC2D_ValidatePortParams(pstCb, stInParam, stOutParam);
    if (u32Ret != VPP_OK)
    {
        LOGE("Invalid port params.");
        goto ERROR_PORT_PARAMS;
    }
    else
    {
        pstCb->stInput.stParam = stInParam;
        pstCb->stOutput.stParam = stOutParam;
    }

    u32Ret = u32VppIpC2D_InitC2DSurfaces(pstCb);
    if (u32Ret != VPP_OK)
    {
        LOGE("Error creating C2D surfaces, u32=%u", u32Ret);
        goto ERROR_INIT_C2D;
    }

    VPP_IP_STATE_SET(pstCb, VPP_IP_STATE_ACTIVE);

    LOG_EXIT_RET(pstCb);

ERROR_INIT_C2D:
ERROR_PORT_PARAMS:
    LOGI("destroying mutex");
    pthread_mutex_destroy(&pstCb->mutex);

ERROR_MUTEX_INIT:
    free(pstCb);

ERROR_MALLOC:
    LOG_EXIT_RET(NULL);
}

void vVppIpC2D_InlineTerm(void *ctx)
{
    int rc;
    uint32_t u32Ret;
    t_StVppIpC2DCb *pstCb;

    LOG_ENTER();

    VPP_RET_VOID_IF_NULL(ctx);
    pstCb = C2D_CB_GET(ctx);

    if (!pstCb->bInlineProcOnly)
    {
        LOGE("Session=%p not initialized for inline. Can't term", pstCb);
        return;
    }

    u32Ret = u32VppIpC2D_TermC2DSurfaces(pstCb);
    LOGE_IF(u32Ret != VPP_OK, "Error destroying C2D surfaces, u32=%u", u32Ret);

    rc = pthread_mutex_destroy(&pstCb->mutex);
    LOGE_IF(rc, "pthread_mutex_destroy failed: %d --> %s", rc, strerror(rc));

    u32Ret = VPP_IP_PROF_UNREGISTER(&pstCb->stBase);
    LOGE_IF(u32Ret != VPP_OK, "Unable to unregister stats, u32=%u", u32Ret);

    free(pstCb);

    LOG_EXIT();
}

uint32_t u32VppIpC2D_InlineReconfigure(void *ctx,
                                       struct vpp_port_param stInParam,
                                       struct vpp_port_param stOutParam)
{
    uint32_t u32QSz, u32Ret;
    t_StVppIpC2DCb *pstCb;

    LOG_ENTER();

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = C2D_CB_GET(ctx);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGE("Invalid state. state=%u, exp=%u", pstCb->eState,
             VPP_IP_STATE_ACTIVE);
        return VPP_ERR_STATE;
    }

    if (!pstCb->bInlineProcOnly)
    {
        pthread_mutex_lock(&pstCb->mutex);
        u32QSz = u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ);
        pthread_mutex_unlock(&pstCb->mutex);
        if (u32QSz)
        {
            LOGE("Inline reconfig called with %u buffers in input port",
                 u32QSz);
            return VPP_ERR_STATE;
        }
    }

    u32Ret = u32VppIpC2D_ValidatePortParams(pstCb, stInParam, stOutParam);
    if (u32Ret != VPP_OK)
    {
        LOGE("Invalid new port params, u32Ret=%u", u32Ret);
        return VPP_ERR_PARAM;
    }

    pstCb->stInput.stParam = stInParam;
    pstCb->stOutput.stParam = stOutParam;
    pstCb->stCfg.u32ComputeMask = C2D_COMPUTE_SURFACE;
    vVppIpC2D_Compute(pstCb);

    LOG_EXIT_RET(VPP_OK);
}

uint32_t u32VppIpC2D_InlineProcess(void *ctx,
                                   t_StVppBuf *pstBufIn,
                                   t_StVppBuf *pstBufOut)
{
    uint32_t u32Ret;
    t_StVppIpC2DCb *pstCb;

    LOG_ENTER();

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstBufIn, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstBufOut, VPP_ERR_PARAM);

    pstCb = C2D_CB_GET(ctx);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGE("Invalid state. state=%u, exp=%u", pstCb->eState,
             VPP_IP_STATE_ACTIVE);
        return VPP_ERR_STATE;
    }

    u32Ret = u32VppIpC2D_ProcessBuffers(pstCb, pstBufIn,pstBufOut);
    LOGE_IF(u32Ret != VPP_OK, "Error Inline processing, u32Ret=%u", u32Ret);

    return u32Ret;
}
