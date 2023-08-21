/*!
 * @file test_ip_c2d.c
 *
 * @cr
 * Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services    Tests C2D IP block related routines
 */

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>

#include "buf_pool.h"
#include "test_utils.h"

#include "dvpTest.h"
#include "dvpTest_tb.h"

#define VPP_LOG_TAG     VPP_LOG_UT_C2D_TAG
#define VPP_LOG_MODULE  VPP_LOG_UT_C2D
#include "vpp_dbg.h"
#include "vpp.h"
#include "vpp_reg.h"
#include "vpp_ip.h"
#include "vpp_ip_c2d.h"
#include "vpp_utils.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/
#define C2D_DUMP_BUF            0

#define INPUT_FLUSH_DONE        (1<<0)
#define OUTPUT_FLUSH_DONE       (1<<1)
#define DRAIN_DONE              (1<<2)

#define C2D_PROC_CNT            20

#define C2D_OUT_NV12_EXTN       "_NV12.yuv"
#define C2D_OUT_UBWC_NV12_EXTN  "_ubwcNV12.yuv"
#define C2D_OUT_RGBA8_EXTN      "_RGBA8.yuv"
#define C2D_OUT_UBWC_RGBA8_EXTN "_ubwcRGB8.yuv"
#define C2D_MAX_W               4096
#define C2D_MAX_H               2160

enum c2d_format {
    C2D_FORMAT_NV12,
    C2D_FORMAT_RGB,
    C2D_FORMAT_MAX,
};

typedef enum {
    C2DTEST_STATE_INTERMEDIATE, // Store OBD buffers in temp pool to convert back for CRC
    C2DTEST_STATE_FINAL,        // Return OBD buffers to original pool
} t_EVppC2DTest_State;

struct c2d_test_ctx {
    void *c2d_ctx; // context returned from gpu init
    uint32_t u32ExpExtraLen;
    uint32_t au32BufCrcIn[C2D_PROC_CNT];
    uint32_t au32BufCrcOut[C2D_PROC_CNT];
    struct vpp_port_param stParamLinear;
    struct vpp_port_param stParamUbwc;
    struct vpp_port_param stParamRgb;
    uint32_t u32CrcOutCnt;
    t_EVppC2DTest_State eState;
    t_StVppBufPool stPoolTemp;
    uint32_t bSkipObdCrc;
};

typedef struct {
    enum clip_reg eClip;
    char cOutFileName[MAX_FILE_SEG_LEN];
} t_StC2DClipInfo;

/************************************************************************
 * Local static variables
 ***********************************************************************/
static struct test_ctx stTestCtx;
static struct c2d_test_ctx stCtxC2D;

static const t_StC2DClipInfo astClipReg[C2D_FORMAT_MAX][VPP_RESOLUTION_MAX] =
{
    [C2D_FORMAT_NV12] =
    {
        [VPP_RESOLUTION_SD] =
        {
            .eClip = CLIP_CANYON_720x480,
            .cOutFileName = "C2D_out_Canyon_720x480",
        },
        [VPP_RESOLUTION_HD] =
        {
            .eClip = CLIP_BEACH_1280x720,
            .cOutFileName = "C2D_out_Beach_1280x720",
        },
        [VPP_RESOLUTION_FHD] =
        {
            .eClip = CLIP_FLOWER_1920x1080,
            .cOutFileName = "C2D_out_Flower_1920x1080",
        },
        [VPP_RESOLUTION_UHD] =
        {
            .eClip = CLIP_NEW_YORK_3840x2160,
            .cOutFileName = "C2D_out_NewYork_3840x2160",
        },
    },
    [C2D_FORMAT_RGB] =
    {
        [VPP_RESOLUTION_SD] =
        {
            .eClip = CLIP_UBWC_RGBA8_SD,
            .cOutFileName = "C2D_SD_in_RGB8_out",
        },
        [VPP_RESOLUTION_HD] =
        {
            .eClip = CLIP_UBWC_RGBA8_HD,
            .cOutFileName = "C2D_HD_in_RGB8_out",
        },
        [VPP_RESOLUTION_FHD] =
        {
            .eClip = CLIP_UBWC_RGBA8_FHD,
            .cOutFileName = "C2D_FHD_in_RGB8_out",
        },
        [VPP_RESOLUTION_UHD] =
        {
            .eClip = CLIP_UBWC_RGBA8_UHD,
            .cOutFileName = "C2D_UHD_in_RGB8_out",
        },
    },
};

/************************************************************************
 * Forward Declarations
 ************************************************************************/

/************************************************************************
 * Local Functions
 ***********************************************************************/
static void test_c2d_input_buffer_done(void *pv, t_StVppBuf *pstBuf)
{
    struct test_ctx *pCtx;
    struct bufnode *pstNode;

    if (!pv || !pstBuf)
    {
        DVP_ASSERT_FAIL();
        LOGE("Null ptrs given. pv=%p, pstBuf=%p", pv, pstBuf);
        return;
    }

    pCtx = (struct test_ctx *)pv;
    pstNode = pstBuf->pBuf->cookie;

    DVP_ASSERT_TRUE(pstNode->owner == BUF_OWNER_LIBRARY);
    pstNode->owner = BUF_OWNER_CLIENT;

    pthread_mutex_lock(&stTestCtx.mutex);
    put_buf(pCtx->buf_pool, pstNode);
    stTestCtx.u32InRxCnt++;
    stTestCtx.u32InHoldCnt--;
    pthread_mutex_unlock(&stTestCtx.mutex);

    pthread_cond_signal(&pCtx->cond);
}

static void test_c2d_output_buffer_done(void *pv, t_StVppBuf *pstBuf)
{
    struct test_ctx *pCtx;
    struct c2d_test_ctx *pC2DCtx;
    struct bufnode *pstNode;
    uint32_t u32Crc;
    t_EVppBufType eType;

    if (!pv || !pstBuf)
    {
        DVP_ASSERT_FAIL();
        LOGE("Null ptrs given. pv=%p, pstBuf=%p", pv, pstBuf);
        return;
    }

    pCtx = (struct test_ctx *)pv;
    pC2DCtx = (struct c2d_test_ctx *)pCtx->pPrivateCtx;
    pstNode = pstBuf->pBuf->cookie;

    DVP_ASSERT_TRUE(pstNode->owner == BUF_OWNER_LIBRARY);
    pstNode->owner = BUF_OWNER_CLIENT;

    eType = eVppBuf_GetFrameType(pstBuf);
    DVP_ASSERT_EQUAL(eType, eVppBufType_Progressive);

    pthread_mutex_lock(&stTestCtx.mutex);
    if (pstNode->pIntBuf->stPixel.u32FilledLen)
    {
#if C2D_DUMP_BUF
        dump_buf(pstNode);
#endif
        DVP_ASSERT_EQUAL(pstNode->pIntBuf->stExtra.u32FilledLen,
                         pC2DCtx->u32ExpExtraLen);
        LOGI("extradata: expFillLen=%u, act_fill_len=%u\n",
             pC2DCtx->u32ExpExtraLen,
             pstNode->pIntBuf->stExtra.u32FilledLen);
        validate_extradata_integrity(pstNode);
    }

    if (pstBuf->eQueuedPort == VPP_PORT_INPUT)
    {
        LOGE("Bypassed buffer received!");
        DVP_ASSERT_FAIL();
        stTestCtx.u32OutRxCnt++;
        stTestCtx.u32InHoldCnt--;
    }
    else if (pstBuf->eQueuedPort == VPP_PORT_OUTPUT)
    {
        DVP_ASSERT_PASS();
        // Bypassed (or flushed)
        if (pstNode->pIntBuf->stPixel.u32FilledLen == 0)
        {
            LOGD("given filled_len == 0 on output buffer.");
        }
        stTestCtx.u32OutRxCnt++;
        stTestCtx.u32OutHoldCnt--;
    }
    else
    {
        LOGE("OBD on buffer queued to unknown port=%d", pstBuf->eQueuedPort);
        DVP_ASSERT_FAIL();
    }

    if (stCtxC2D.eState == C2DTEST_STATE_INTERMEDIATE)
    {
        u32VppBufPool_Put(&stCtxC2D.stPoolTemp, pstNode->pIntBuf);
    }
    else
    {
        if (!stCtxC2D.bSkipObdCrc)
        {
            u32Crc = u32Vpp_CrcBuffer(pstNode->pIntBuf,
                                      eVppBuf_Pixel,
                                      pstNode->pIntBuf->stPixel.u32FilledLen / 2,
                                      pstNode->pIntBuf->stPixel.u32FilledLen / 8,
                                      stCtxC2D.u32CrcOutCnt, "C2D_Test_CRC_out");
            stCtxC2D.au32BufCrcOut[stCtxC2D.u32CrcOutCnt] = u32Crc;
            stCtxC2D.u32CrcOutCnt++;
        }
        put_buf(pCtx->buf_pool, pstNode);
    }
    pthread_mutex_unlock(&stTestCtx.mutex);

    pthread_cond_signal(&pCtx->cond);
}

static void test_c2d_event(void *pv, t_StVppEvt stEvt)
{
    LOGI("%s() got event: %u", __func__, stEvt.eType);
    struct test_ctx *pCtx = (struct test_ctx *)pv;
    if (!pCtx)
    {
        LOGE("context was null");
        DVP_ASSERT_FAIL();
        return;
    }

    pthread_mutex_lock(&pCtx->mutex);

    if (stEvt.eType == VPP_EVT_FLUSH_DONE)
    {
        if (stEvt.flush.ePort == VPP_PORT_INPUT)
            pCtx->u32Flags |= INPUT_FLUSH_DONE;
        else if (stEvt.flush.ePort == VPP_PORT_OUTPUT)
            pCtx->u32Flags |= OUTPUT_FLUSH_DONE;
    }
    else if (stEvt.eType == VPP_EVT_DRAIN_DONE)
    {
        pCtx->u32Flags |= DRAIN_DONE;
    }

    pthread_mutex_unlock(&pCtx->mutex);

    pthread_cond_signal(&pCtx->cond);
}

static void vC2DTest_SetPoolParams(struct test_ctx *pCtx,
                                   enum vpp_resolution eRes,
                                   enum c2d_format eClipFmt,
                                   enum vpp_color_format eOutFmt)
{
    const t_StC2DClipInfo *pstClip;
    uint32_t u32Width, u32Height, u32Ret;

    if (eRes >= VPP_RESOLUTION_MAX)
        return;

    if (eClipFmt >= C2D_FORMAT_MAX)
        return;

    if (eOutFmt != VPP_COLOR_FORMAT_NV12_VENUS &&
        eOutFmt != VPP_COLOR_FORMAT_UBWC_NV12 &&
        eOutFmt != VPP_COLOR_FORMAT_RGBA8 &&
        eOutFmt != VPP_COLOR_FORMAT_UBWC_RGBA8)
        return;

    pstClip = &astClipReg[eClipFmt][eRes];

    u32Ret = get_res_for_clip(pstClip->eClip, &u32Width, &u32Height, NULL);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    pCtx->buf_pool->params.u32Width = u32Width;
    pCtx->buf_pool->params.u32Height = u32Height;
    pCtx->buf_pool->u32RdIdx = 0;
    pCtx->buf_pool->u32WrIdx = 0;

    u32Ret = populate_pool_params(pstClip->eClip, &pCtx->buf_pool->params);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    u32Ret = populate_port_params(pstClip->eClip, VPP_PORT_INPUT,
                                  &pCtx->buf_pool->stInputPort);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    pCtx->buf_pool->params.eOutputBufFmt = eOutFmt;
    if (eOutFmt == VPP_COLOR_FORMAT_NV12_VENUS)
    {
        pCtx->buf_pool->params.eOutputFileFormat = FILE_FORMAT_NV12;
        snprintf(pCtx->buf_pool->params.cOutputName,
                 sizeof(pCtx->buf_pool->params.cOutputName),
                 "%s%s", pstClip->cOutFileName, C2D_OUT_NV12_EXTN);
    }
    else if (eOutFmt == VPP_COLOR_FORMAT_UBWC_NV12)
    {
        pCtx->buf_pool->params.eOutputFileFormat = FILE_FORMAT_NV12_UBWC;
        snprintf(pCtx->buf_pool->params.cOutputName,
                 sizeof(pCtx->buf_pool->params.cOutputName),
                 "%s%s", pstClip->cOutFileName, C2D_OUT_UBWC_NV12_EXTN);
    }
    else if (eOutFmt == VPP_COLOR_FORMAT_RGBA8)
    {
        pCtx->buf_pool->params.eOutputFileFormat = FILE_FORMAT_RGBA8;
        snprintf(pCtx->buf_pool->params.cOutputName,
                 sizeof(pCtx->buf_pool->params.cOutputName),
                 "%s%s", pstClip->cOutFileName, C2D_OUT_RGBA8_EXTN);
    }
    else
    {
        pCtx->buf_pool->params.eOutputFileFormat = FILE_FORMAT_RGBA8_UBWC;
        snprintf(pCtx->buf_pool->params.cOutputName,
                 sizeof(pCtx->buf_pool->params.cOutputName),
                 "%s%s", pstClip->cOutFileName, C2D_OUT_UBWC_RGBA8_EXTN);
    }

    set_port_params(&pCtx->buf_pool->stOutputPort,
                    u32Width, u32Height, eOutFmt);
}

static uint32_t u32C2DTest_BufPoolInit(struct test_ctx *pCtx,
                                       uint32_t u32BufCnt)
{
    uint32_t i, u32BufSzTmp, u32Ret;
    uint32_t u32BufSz = 0;
    uint32_t u32Width = C2D_MAX_W;
    uint32_t u32Height = C2D_MAX_H;
    struct vpp_port_param stParam;
    enum vpp_color_format eFmt = 0;

    for (i = 0; i < VPP_COLOR_FORMAT_MAX; i++)
    {
        // Skip redundant formats
        if (i == VPP_COLOR_FORMAT_NV21_VENUS || i == VPP_COLOR_FORMAT_UBWC_NV21 ||
            i == VPP_COLOR_FORMAT_BGRA8 || i == VPP_COLOR_FORMAT_UBWC_BGRA8 ||
            i == VPP_COLOR_FORMAT_UBWC_BGR565)
            continue;

        set_port_params(&stParam, u32Width, u32Height, i);
        u32BufSzTmp = u32VppUtils_GetPxBufferSize(&stParam);

        if (u32BufSzTmp > u32BufSz)
        {
            eFmt = i;
            u32BufSz = u32BufSzTmp;
        }
    }

    // Create buffer pool with largest buffers to support all formats
    tctx_set_port_params(pCtx, u32Width, u32Height, eFmt);

    buf_params_init_default(&pCtx->params, &pCtx->port_param);

    u32Ret = init_buf_pool(&pCtx->stVppCtx, &pCtx->buf_pool, &pCtx->params,
                           u32BufCnt, VPP_TRUE);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    pCtx->buf_pool->params.eInputFileType = FILE_TYPE_MULTI_FRAMES;
    pCtx->buf_pool->params.eOutputFileType = FILE_TYPE_MULTI_FRAMES;
    pCtx->buf_pool->params.u32MaxInputFrames = u32Height / 2;

    return u32Ret;
}

static uint32_t u32QueueBuf(void *pvCtx, enum vpp_port ePort, t_StVppBuf *pBuf)
{
    uint32_t u32Ret;
    uint32_t *pu32HoldCnt = NULL;
    uint32_t *pu32PutCnt = NULL;

    if (pBuf != NULL)
    {
        // Set to let buffer_done()'s know where this buffer came from.
        pBuf->eQueuedPort = ePort;
    }

    if (ePort == VPP_PORT_INPUT)
    {
        pu32HoldCnt = &stTestCtx.u32InHoldCnt;
        pu32PutCnt = &stTestCtx.u32InPutCnt;
    }
    else if (ePort == VPP_PORT_OUTPUT)
    {
        pu32HoldCnt = &stTestCtx.u32OutHoldCnt;
        pu32PutCnt = &stTestCtx.u32OutPutCnt;
    }

    if (pu32HoldCnt != NULL && pu32PutCnt != NULL)
    {
        pthread_mutex_lock(&stTestCtx.mutex);
        (*pu32HoldCnt)++;
        (*pu32PutCnt)++;
        pthread_mutex_unlock(&stTestCtx.mutex);
    }
    u32Ret = u32VppIpC2D_QueueBuf(pvCtx, ePort, pBuf);
    if (u32Ret != VPP_OK && pu32HoldCnt != NULL && pu32PutCnt != NULL)
    {
        pthread_mutex_lock(&stTestCtx.mutex);
        (*pu32HoldCnt)--;
        (*pu32PutCnt)--;
        pthread_mutex_unlock(&stTestCtx.mutex);
    }

    return u32Ret;
}

// This function will queue a set of linear buffers and store the CRCs. Then
// it will convert the buffers to UBWC and back, and then compare the output
// CRCs to make sure they match the original.
static void vC2DTest_RotateBuffers(enum vpp_resolution eRes)
{
    uint32_t u32Crc, u32Cnt, u32Ret;
    uint32_t i;
    struct bufnode *pstNode;
    t_StVppBuf *pstBuf;

    stCtxC2D.eState = C2DTEST_STATE_INTERMEDIATE;
    stCtxC2D.u32CrcOutCnt = 0;
    // Queue to input pool
    for (i = 0; i < C2D_PROC_CNT; i++)
    {
        pthread_mutex_lock(&stTestCtx.mutex);
        while (NULL == (pstNode = get_buf(stTestCtx.buf_pool)))
            pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
        pthread_mutex_unlock(&stTestCtx.mutex);

        vVppBuf_Clean(pstNode->pIntBuf, 0);
        fill_buf(pstNode);

        pstNode->owner = BUF_OWNER_LIBRARY;

        fill_extra_data(pstNode, eVppBufType_Progressive, 0);
        stCtxC2D.u32ExpExtraLen = pstNode->pIntBuf->stExtra.u32FilledLen;
        u32Crc = u32Vpp_CrcBuffer(pstNode->pIntBuf,
                                  eVppBuf_Pixel,
                                  pstNode->pIntBuf->stPixel.u32FilledLen / 2,
                                  pstNode->pIntBuf->stPixel.u32FilledLen / 8,
                                  i, "C2D_Test_CRC_in");
        stCtxC2D.au32BufCrcIn[i] = u32Crc;

        u32Ret = u32QueueBuf(stCtxC2D.c2d_ctx, VPP_PORT_INPUT,
                             pstNode->pIntBuf);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    }

    for (i = 0; i < C2D_PROC_CNT; i++)
    {
        pthread_mutex_lock(&stTestCtx.mutex);
        while (NULL == (pstNode = get_buf(stTestCtx.buf_pool)))
            pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
        pthread_mutex_unlock(&stTestCtx.mutex);

        vVppBuf_Clean(pstNode->pIntBuf, 0);
        pstNode->owner = BUF_OWNER_LIBRARY;

        u32Ret = u32QueueBuf(stCtxC2D.c2d_ctx, VPP_PORT_OUTPUT,
                             pstNode->pIntBuf);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    }

    pthread_mutex_lock(&stTestCtx.mutex);
    LOGI("entering wait loop");
    while (stTestCtx.buf_pool->u32ListSz +
           u32VppBufPool_Cnt(&stCtxC2D.stPoolTemp) != (C2D_PROC_CNT * 2))
        pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
    LOGI("breaking out of wait loop");
    pthread_mutex_unlock(&stTestCtx.mutex);

    u32Cnt = u32VppBufPool_Cnt(&stCtxC2D.stPoolTemp);
    if (u32Cnt != C2D_PROC_CNT)
    {
        LOGE("Error, processed ubwc pool count=%u, expected=%u",
             u32Cnt, C2D_PROC_CNT);
        DVP_ASSERT_FAIL();
        return;
    }
    u32Ret = u32VppIpC2D_Reconfigure(stCtxC2D.c2d_ctx, stCtxC2D.stParamUbwc,
                                     stCtxC2D.stParamLinear);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    pthread_mutex_lock(&stTestCtx.mutex);
    stCtxC2D.eState = C2DTEST_STATE_FINAL;
    pthread_mutex_unlock(&stTestCtx.mutex);

    vC2DTest_SetPoolParams(&stTestCtx, eRes, C2D_FORMAT_NV12,
                           VPP_COLOR_FORMAT_NV12_VENUS);
    for (i = 0; i < C2D_PROC_CNT; i++)
    {
        pthread_mutex_lock(&stTestCtx.mutex);
        while (NULL == (pstBuf = pstVppBufPool_Get(&stCtxC2D.stPoolTemp)))
            pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
        pthread_mutex_unlock(&stTestCtx.mutex);

        pstNode = pstBuf->pBuf->cookie;
        pstNode->owner = BUF_OWNER_LIBRARY;

        u32Ret = u32QueueBuf(stCtxC2D.c2d_ctx, VPP_PORT_INPUT, pstBuf);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    }

    for (i = 0; i < C2D_PROC_CNT; i++)
    {
        pthread_mutex_lock(&stTestCtx.mutex);
        while (NULL == (pstNode = get_buf(stTestCtx.buf_pool)))
            pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
        pthread_mutex_unlock(&stTestCtx.mutex);

        vVppBuf_Clean(pstNode->pIntBuf, 0);
        pstNode->owner = BUF_OWNER_LIBRARY;

        u32Ret = u32QueueBuf(stCtxC2D.c2d_ctx, VPP_PORT_OUTPUT,
                             pstNode->pIntBuf);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    }
    pthread_mutex_lock(&stTestCtx.mutex);
    LOGI("entering wait loop");
    while (stTestCtx.buf_pool->u32ListSz != (C2D_PROC_CNT * 2))
        pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
    LOGI("breaking out of wait loop");
    pthread_mutex_unlock(&stTestCtx.mutex);

    for (i = 0; i < C2D_PROC_CNT; i++)
    {
        DVP_ASSERT_EQUAL(stCtxC2D.au32BufCrcIn[i], stCtxC2D.au32BufCrcOut[i]);
    }
}

static void vC2DTest_InlineProc(struct c2d_test_ctx *pstC2DTestCtx)
{
    uint32_t i, j, u32Width, u32Height, u32Crc, u32Ret;
    void *pCtx;
    const t_StC2DClipInfo *pstClip;
    struct bufnode *pstNodeIn, *pstNodeOut;
    t_StVppBuf *pstBuf;
    t_StVppMemBuf *pstPixel;

    VPP_RET_VOID_IF_NULL(pstC2DTestCtx);
    VPP_RET_VOID_IF_NULL(pstC2DTestCtx->c2d_ctx);

    pCtx = pstC2DTestCtx->c2d_ctx;

    for (i = VPP_RESOLUTION_SD; i <= VPP_RESOLUTION_UHD; i++)
    {
        pstClip = &astClipReg[C2D_FORMAT_NV12][i];
        u32Ret = get_res_for_clip(pstClip->eClip, &u32Width,
                                  &u32Height, NULL);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
        set_port_params(&pstC2DTestCtx->stParamLinear, u32Width, u32Height,
                        VPP_COLOR_FORMAT_NV12_VENUS);
        set_port_params(&pstC2DTestCtx->stParamUbwc, u32Width, u32Height,
                        VPP_COLOR_FORMAT_UBWC_NV12);
        vC2DTest_SetPoolParams(&stTestCtx, i, C2D_FORMAT_NV12,
                               VPP_COLOR_FORMAT_UBWC_NV12);

        u32Ret = u32VppIpC2D_InlineReconfigure(pCtx,
                                               pstC2DTestCtx->stParamLinear,
                                               pstC2DTestCtx->stParamUbwc);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        for (j = 0; j < C2D_PROC_CNT; j++)
        {
            pstNodeIn = get_buf(stTestCtx.buf_pool);
            pstNodeOut = get_buf(stTestCtx.buf_pool);
            if (pstNodeIn && pstNodeOut)
            {
                vVppBuf_Clean(pstNodeIn->pIntBuf, 0);
                fill_buf(pstNodeIn);
                pstPixel = &pstNodeIn->pIntBuf->stPixel;
                u32Crc = u32Vpp_CrcBuffer(pstNodeIn->pIntBuf,
                                          eVppBuf_Pixel,
                                          pstPixel->u32FilledLen / 2,
                                          pstPixel->u32FilledLen / 8,
                                          j, "C2D_Test_Inline_CRC_in");
                pstC2DTestCtx->au32BufCrcIn[j] = u32Crc;
                vVppBuf_Clean(pstNodeOut->pIntBuf, 0);
                u32Ret = u32VppIpC2D_InlineProcess(pCtx,
                                                   pstNodeIn->pIntBuf,
                                                   pstNodeOut->pIntBuf);
                DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
                put_buf(stTestCtx.buf_pool, pstNodeIn);
                u32VppBufPool_Put(&pstC2DTestCtx->stPoolTemp,
                                  pstNodeOut->pIntBuf);
            }
            else
            {
                LOGE("got NULL buffers: pstNodeIn=%p, pstNodeOut=%p",
                     pstNodeIn, pstNodeOut);
                DVP_ASSERT_FAIL();
                if (pstNodeIn)
                    put_buf(stTestCtx.buf_pool, pstNodeIn);
                if (pstNodeOut)
                    put_buf(stTestCtx.buf_pool, pstNodeOut);
            }
        }
        u32Ret = u32VppIpC2D_InlineReconfigure(pCtx,
                                               pstC2DTestCtx->stParamUbwc,
                                               pstC2DTestCtx->stParamLinear);
        for (j = 0; j < C2D_PROC_CNT; j++)
        {
            pstBuf = pstVppBufPool_Get(&pstC2DTestCtx->stPoolTemp);
            pstNodeOut = get_buf(stTestCtx.buf_pool);
            if (pstBuf && pstNodeOut)
            {
                vVppBuf_Clean(pstNodeOut->pIntBuf, 0);

                u32Ret = u32VppIpC2D_InlineProcess(pCtx,
                                                   pstBuf,
                                                   pstNodeOut->pIntBuf);
                DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
                pstPixel = &pstNodeOut->pIntBuf->stPixel;
                u32Crc = u32Vpp_CrcBuffer(pstNodeOut->pIntBuf,
                                          eVppBuf_Pixel,
                                          pstPixel->u32FilledLen / 2,
                                          pstPixel->u32FilledLen / 8,
                                          j, "C2D_Test_Inline_CRC_out");
                DVP_ASSERT_EQUAL(pstC2DTestCtx->au32BufCrcIn[j], u32Crc);
                put_buf(stTestCtx.buf_pool, pstNodeOut);
                put_buf(stTestCtx.buf_pool, pstBuf->pBuf->cookie);
            }
            else
            {
                LOGE("got NULL buffers: pstBuf=%p, pstNodeOut=%p",
                     pstBuf, pstNodeOut);
                DVP_ASSERT_FAIL();
                if (pstBuf)
                    put_buf(stTestCtx.buf_pool, pstBuf->pBuf->cookie);
                if (pstNodeOut)
                    put_buf(stTestCtx.buf_pool, pstNodeOut);
            }
        }
    }
}

/************************************************************************
 * Test Functions
 ***********************************************************************/
TEST_SUITE_INIT(IpC2DSuiteInit)
{
}

TEST_SUITE_TERM(IpC2DSuiteTerm)
{
}

TEST_SETUP(IpC2DTestInit)
{
    uint32_t u32Ret;

    tctx_common_init(&stTestCtx);

    stTestCtx.cb.input_buffer_done = test_c2d_input_buffer_done;
    stTestCtx.cb.output_buffer_done = test_c2d_output_buffer_done;
    stTestCtx.cb.event = test_c2d_event;
    stTestCtx.cb.pv = &stTestCtx;
    stTestCtx.u32Flags = 0;
    stTestCtx.pPrivateCtx = &stCtxC2D;
    stCtxC2D.u32CrcOutCnt = 0;
    stCtxC2D.bSkipObdCrc = 0;
    stCtxC2D.eState = C2DTEST_STATE_INTERMEDIATE;

    u32Ret = u32VppBufPool_Init(&stCtxC2D.stPoolTemp);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
}

TEST_CLEANUP(IpC2DTestTerm)
{
    uint32_t u32Ret;

    u32Ret = u32VppBufPool_Term(&stCtxC2D.stPoolTemp);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    tctx_common_destroy(&stTestCtx);
}

TEST(IpC2D_TestBasicConfig)
{
    uint32_t u32PixBufSzIn, u32PixBufSzOut, u32Ret;
    void *pCtx;
    struct vpp_port_param stParamIn, stParamOut;
    t_StVppIpBufReq stInputBufReq, stOutputBufReq;

    set_port_params(&stParamIn, 1920, 1080, VPP_COLOR_FORMAT_NV12_VENUS);
    set_port_params(&stParamOut, 1920, 1080, VPP_COLOR_FORMAT_UBWC_NV12);
    u32PixBufSzIn = u32VppUtils_GetPxBufferSize(&stParamIn);
    u32PixBufSzOut = u32VppUtils_GetPxBufferSize(&stParamOut);

    stCtxC2D.c2d_ctx = vpVppIpC2D_Init(&stTestCtx.stVppCtx, 0, stTestCtx.cb);
    DVP_ASSERT_PTR_NNULL(stCtxC2D.c2d_ctx);
    if (stCtxC2D.c2d_ctx)
    {
        pCtx = stCtxC2D.c2d_ctx;

        u32Ret = u32VppIpC2D_SetParam(pCtx, VPP_PORT_INPUT, stParamIn);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_SetParam(pCtx, VPP_PORT_OUTPUT, stParamOut);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_Open(pCtx);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_GetBufferRequirements(pCtx, &stInputBufReq,
                                                   &stOutputBufReq);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
        DVP_ASSERT_EQUAL(stInputBufReq.u32PxSz, u32PixBufSzIn);
        DVP_ASSERT_EQUAL(stOutputBufReq.u32PxSz, u32PixBufSzOut);

        set_port_params(&stParamIn, 1280, 720, VPP_COLOR_FORMAT_NV12_VENUS);
        set_port_params(&stParamOut, 1280, 720, VPP_COLOR_FORMAT_UBWC_NV12);
        u32PixBufSzIn = u32VppUtils_GetPxBufferSize(&stParamIn);
        u32PixBufSzOut = u32VppUtils_GetPxBufferSize(&stParamOut);

        u32Ret = u32VppIpC2D_Reconfigure(pCtx, stParamIn, stParamOut);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_GetBufferRequirements(pCtx, &stInputBufReq,
                                                   &stOutputBufReq);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
        DVP_ASSERT_EQUAL(stInputBufReq.u32PxSz, u32PixBufSzIn);
        DVP_ASSERT_EQUAL(stOutputBufReq.u32PxSz, u32PixBufSzOut);

        u32Ret = u32VppIpC2D_Close(pCtx);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        vVppIpC2D_Term(pCtx);
    }

    stCtxC2D.c2d_ctx = vpVppIpC2D_InlineInit(&stTestCtx.stVppCtx, stParamIn,
                                             stParamOut);
    DVP_ASSERT_PTR_NNULL(stCtxC2D.c2d_ctx);
    if (stCtxC2D.c2d_ctx)
    {
        pCtx = stCtxC2D.c2d_ctx;

        set_port_params(&stParamIn, 1920, 1080, VPP_COLOR_FORMAT_NV12_VENUS);
        set_port_params(&stParamOut, 1920, 1080, VPP_COLOR_FORMAT_UBWC_NV12);
        u32Ret = u32VppIpC2D_InlineReconfigure(pCtx, stParamIn, stParamOut);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        vVppIpC2D_InlineTerm(pCtx);
    }
}

TEST(IpC2D_TestInvalidConfig)
{
    uint32_t u32Ret;
    void *pCtx;
    struct vpp_port_param stParamIn, stParamOut;
    t_StVppBuf stBuf;
    t_StVppCallback stCb;

    set_port_params(&stParamIn, 1920, 1080, VPP_COLOR_FORMAT_NV12_VENUS);
    set_port_params(&stParamOut, 1280, 720, VPP_COLOR_FORMAT_UBWC_NV12);

    // Null VPP ctx
    stCtxC2D.c2d_ctx = vpVppIpC2D_Init(NULL, 0, stTestCtx.cb);
    DVP_ASSERT_PTR_NULL(stCtxC2D.c2d_ctx);

    // Null callback functions
    memset(&stCb, 0, sizeof(t_StVppCallback));
    stCtxC2D.c2d_ctx = vpVppIpC2D_Init(&stTestCtx.stVppCtx, 0, stCb);
    DVP_ASSERT_PTR_NULL(stCtxC2D.c2d_ctx);

    stCtxC2D.c2d_ctx = vpVppIpC2D_Init(&stTestCtx.stVppCtx, 0, stTestCtx.cb);
    DVP_ASSERT_PTR_NNULL(stCtxC2D.c2d_ctx);

    if (stCtxC2D.c2d_ctx)
    {
        pCtx = stCtxC2D.c2d_ctx;

        // NULL context
        u32Ret = u32VppIpC2D_SetParam(NULL, VPP_PORT_INPUT, stParamIn);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Invalid port
        u32Ret = u32VppIpC2D_SetParam(pCtx, 0xFFFF, stParamIn);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_SetParam(pCtx, VPP_PORT_INPUT, stParamIn);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_SetParam(pCtx, VPP_PORT_OUTPUT, stParamOut);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        // Queue buf before open is not allowed
        u32Ret = u32VppIpC2D_QueueBuf(pCtx, VPP_PORT_INPUT, &stBuf);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // In/out different resolutions should fail
        u32Ret = u32VppIpC2D_Open(pCtx);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Valid Open
        set_port_params(&stParamIn, 1280, 720, VPP_COLOR_FORMAT_NV12_VENUS);
        u32Ret = u32VppIpC2D_SetParam(pCtx, VPP_PORT_INPUT, stParamIn);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
        u32Ret = u32VppIpC2D_Open(pCtx);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        // NULL context
        u32Ret = u32VppIpC2D_QueueBuf(NULL, VPP_PORT_INPUT, &stBuf);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // NULL buffer
        u32Ret = u32VppIpC2D_QueueBuf(pCtx, VPP_PORT_INPUT, NULL);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Invalid port
        u32Ret = u32VppIpC2D_QueueBuf(pCtx, 0xFFFF, &stBuf);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // NULL context
        u32Ret = u32VppIpC2D_Reconfigure(NULL, stParamIn, stParamOut);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Reconfigure to different resolutions should fail
        set_port_params(&stParamOut, 1920, 1080, VPP_COLOR_FORMAT_UBWC_NV12);
        u32Ret = u32VppIpC2D_Reconfigure(pCtx, stParamIn, stParamOut);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // NULL context
        u32Ret = u32VppIpC2D_Flush(NULL, VPP_PORT_INPUT);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Invalid port
        u32Ret = u32VppIpC2D_Flush(pCtx, 0xFFFF);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // NULL context
        u32Ret = u32VppIpC2D_Drain(NULL);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // NULL context
        u32Ret = u32VppIpC2D_Close(NULL);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_Close(pCtx);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        vVppIpC2D_Term(pCtx);
    }

    // In/out different resolutions should fail
    set_port_params(&stParamIn, 1920, 1080, VPP_COLOR_FORMAT_NV12_VENUS);
    set_port_params(&stParamOut, 1280, 720, VPP_COLOR_FORMAT_UBWC_NV12);
    stCtxC2D.c2d_ctx = vpVppIpC2D_InlineInit(&stTestCtx.stVppCtx, stParamIn,
                                             stParamOut);
    DVP_ASSERT_PTR_NULL(stCtxC2D.c2d_ctx);

    // Null VPP ctx
    set_port_params(&stParamOut, 1920, 1080, VPP_COLOR_FORMAT_UBWC_NV12);
    stCtxC2D.c2d_ctx = vpVppIpC2D_InlineInit(NULL, stParamIn,
                                             stParamOut);
    DVP_ASSERT_PTR_NULL(stCtxC2D.c2d_ctx);

    // Valid Init
    stCtxC2D.c2d_ctx = vpVppIpC2D_InlineInit(&stTestCtx.stVppCtx, stParamIn,
                                             stParamOut);
    DVP_ASSERT_PTR_NNULL(stCtxC2D.c2d_ctx);

    if (stCtxC2D.c2d_ctx)
    {
        pCtx = stCtxC2D.c2d_ctx;

        // Inline session calling SetParam not accepted
        u32Ret = u32VppIpC2D_SetParam(pCtx, VPP_PORT_OUTPUT, stParamOut);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Inline session calling Open not accepted
        u32Ret = u32VppIpC2D_Open(pCtx);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Inline session cannot queue buffers
        u32Ret = u32VppIpC2D_QueueBuf(pCtx, VPP_PORT_INPUT, &stBuf);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Inline session calling Reconfigure not accepted
        u32Ret = u32VppIpC2D_Reconfigure(pCtx, stParamIn, stParamOut);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Inline session calling Flush not accepted
        u32Ret = u32VppIpC2D_Flush(pCtx, VPP_PORT_INPUT);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Inline session calling Drain not accepted
        u32Ret = u32VppIpC2D_Drain(pCtx);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Inline session calling Close not accepted
        u32Ret = u32VppIpC2D_Close(pCtx);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // NULL context
        u32Ret = u32VppIpC2D_InlineReconfigure(NULL, stParamIn, stParamOut);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Reconfigure to different resolutions should fail
        set_port_params(&stParamOut, 1280, 720, VPP_COLOR_FORMAT_UBWC_NV12);
        u32Ret = u32VppIpC2D_InlineReconfigure(pCtx, stParamIn, stParamOut);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // NULL context
        u32Ret = u32VppIpC2D_InlineProcess(NULL, &stBuf, &stBuf);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // NULL input buffer
        u32Ret = u32VppIpC2D_InlineProcess(pCtx, NULL, &stBuf);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // NULL output buffer
        u32Ret = u32VppIpC2D_InlineProcess(pCtx, &stBuf, NULL);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // NULL context; void function, so just test for segfault
        vVppIpC2D_InlineTerm(NULL);

        vVppIpC2D_InlineTerm(pCtx);
    }
}

TEST(IpC2D_AsyncProcess)
{
    uint32_t i, u32Width, u32Height, u32Ret;
    void *pCtx;
    const t_StC2DClipInfo *pstClip;

    u32C2DTest_BufPoolInit(&stTestCtx, C2D_PROC_CNT * 2);

    stCtxC2D.c2d_ctx = vpVppIpC2D_Init(&stTestCtx.stVppCtx, 0, stTestCtx.cb);
    DVP_ASSERT_PTR_NNULL(stCtxC2D.c2d_ctx);
    if (stCtxC2D.c2d_ctx)
    {
        pCtx = stCtxC2D.c2d_ctx;

        pstClip = &astClipReg[C2D_FORMAT_NV12][VPP_RESOLUTION_UHD];
        u32Ret = get_res_for_clip(pstClip->eClip, &u32Width, &u32Height, NULL);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        set_port_params(&stCtxC2D.stParamLinear, u32Width, u32Height,
                        VPP_COLOR_FORMAT_NV12_VENUS);
        set_port_params(&stCtxC2D.stParamUbwc, u32Width, u32Height,
                        VPP_COLOR_FORMAT_UBWC_NV12);
        vC2DTest_SetPoolParams(&stTestCtx, VPP_RESOLUTION_UHD, C2D_FORMAT_NV12,
                               VPP_COLOR_FORMAT_UBWC_NV12);

        u32Ret = u32VppIpC2D_SetParam(pCtx, VPP_PORT_INPUT,
                                      stCtxC2D.stParamLinear);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_SetParam(pCtx, VPP_PORT_OUTPUT,
                                      stCtxC2D.stParamUbwc);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_Open(pCtx);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        vC2DTest_RotateBuffers(VPP_RESOLUTION_UHD);

        for (i = VPP_RESOLUTION_SD; i < VPP_RESOLUTION_UHD; i++)
        {
            VPP_FLAG_CLR(stTestCtx.u32Flags, DRAIN_DONE);
            u32Ret = u32VppIpC2D_Drain(pCtx);
            DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

            pthread_mutex_lock(&stTestCtx.mutex);
            while (!(VPP_FLAG_IS_SET(stTestCtx.u32Flags, DRAIN_DONE)))
                pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
            pthread_mutex_unlock(&stTestCtx.mutex);

            pstClip = &astClipReg[C2D_FORMAT_NV12][i];
            u32Ret = get_res_for_clip(pstClip->eClip, &u32Width,
                                      &u32Height, NULL);
            set_port_params(&stCtxC2D.stParamLinear, u32Width, u32Height,
                            VPP_COLOR_FORMAT_NV12_VENUS);
            set_port_params(&stCtxC2D.stParamUbwc, u32Width, u32Height,
                            VPP_COLOR_FORMAT_UBWC_NV12);
            vC2DTest_SetPoolParams(&stTestCtx, i, C2D_FORMAT_NV12,
                                   VPP_COLOR_FORMAT_UBWC_NV12);

            u32Ret = u32VppIpC2D_Reconfigure(pCtx, stCtxC2D.stParamLinear,
                                             stCtxC2D.stParamUbwc);
            vC2DTest_RotateBuffers(i);
        }

        VPP_FLAG_CLR(stTestCtx.u32Flags, INPUT_FLUSH_DONE);
        VPP_FLAG_CLR(stTestCtx.u32Flags, OUTPUT_FLUSH_DONE);

        u32Ret = u32VppIpC2D_Flush(pCtx, VPP_PORT_INPUT);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
        u32Ret = u32VppIpC2D_Flush(pCtx, VPP_PORT_OUTPUT);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        pthread_mutex_lock(&stTestCtx.mutex);
        while (!(VPP_FLAG_IS_SET(stTestCtx.u32Flags, INPUT_FLUSH_DONE)) ||
               !(VPP_FLAG_IS_SET(stTestCtx.u32Flags, OUTPUT_FLUSH_DONE)))
            pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
        pthread_mutex_unlock(&stTestCtx.mutex);

        u32Ret = u32VppIpC2D_Close(pCtx);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        vVppIpC2D_Term(pCtx);
    }
    LOGI("freeing buffer pools");
    free_buf_pool(stTestCtx.buf_pool, VPP_TRUE);
}

TEST(IpC2D_InlineProcess)
{
    uint32_t u32Width, u32Height, u32Ret;
    const t_StC2DClipInfo *pstClip;

    u32C2DTest_BufPoolInit(&stTestCtx, C2D_PROC_CNT * 2);
    pstClip = &astClipReg[C2D_FORMAT_NV12][VPP_RESOLUTION_SD];
    u32Ret = get_res_for_clip(pstClip->eClip, &u32Width, &u32Height, NULL);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    set_port_params(&stCtxC2D.stParamLinear, u32Width, u32Height,
                    VPP_COLOR_FORMAT_NV12_VENUS);
    set_port_params(&stCtxC2D.stParamUbwc, u32Width, u32Height,
                    VPP_COLOR_FORMAT_UBWC_NV12);
    vC2DTest_SetPoolParams(&stTestCtx, VPP_RESOLUTION_SD, C2D_FORMAT_NV12,
                           VPP_COLOR_FORMAT_UBWC_NV12);

    stCtxC2D.c2d_ctx = vpVppIpC2D_InlineInit(&stTestCtx.stVppCtx,
                                             stCtxC2D.stParamLinear,
                                             stCtxC2D.stParamUbwc);
    DVP_ASSERT_PTR_NNULL(stCtxC2D.c2d_ctx);
    if (stCtxC2D.c2d_ctx)
    {
        vC2DTest_InlineProc(&stCtxC2D);
        vVppIpC2D_InlineTerm(stCtxC2D.c2d_ctx);
    }
    LOGI("freeing buffer pools");
    free_buf_pool(stTestCtx.buf_pool, VPP_TRUE);
}

TEST(IpC2D_AsyncSessionInlineProcess)
{
    uint32_t u32Width, u32Height, u32Ret;
    const t_StC2DClipInfo *pstClip;
    void *pCtx;

    u32C2DTest_BufPoolInit(&stTestCtx, C2D_PROC_CNT * 2);
    pstClip = &astClipReg[C2D_FORMAT_NV12][VPP_RESOLUTION_SD];
    u32Ret = get_res_for_clip(pstClip->eClip, &u32Width, &u32Height, NULL);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    set_port_params(&stCtxC2D.stParamLinear, u32Width, u32Height,
                    VPP_COLOR_FORMAT_NV12_VENUS);
    set_port_params(&stCtxC2D.stParamUbwc, u32Width, u32Height,
                    VPP_COLOR_FORMAT_UBWC_NV12);
    vC2DTest_SetPoolParams(&stTestCtx, VPP_RESOLUTION_SD, C2D_FORMAT_NV12,
                           VPP_COLOR_FORMAT_UBWC_NV12);

    stCtxC2D.c2d_ctx = vpVppIpC2D_Init(&stTestCtx.stVppCtx, 0, stTestCtx.cb);
    DVP_ASSERT_PTR_NNULL(stCtxC2D.c2d_ctx);

    pCtx = stCtxC2D.c2d_ctx;
    if (pCtx)
    {
        u32Ret = u32VppIpC2D_SetParam(pCtx, VPP_PORT_INPUT,
                                      stCtxC2D.stParamLinear);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_SetParam(pCtx, VPP_PORT_OUTPUT,
                                      stCtxC2D.stParamUbwc);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_Open(pCtx);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        if (u32Ret == VPP_OK)
        {
            vC2DTest_InlineProc(&stCtxC2D);
        }

        vVppIpC2D_Term(pCtx);
    }
    LOGI("freeing buffer pools");
    free_buf_pool(stTestCtx.buf_pool, VPP_TRUE);
}

TEST(IpC2D_UbwcRgbToUbwcNv12Process)
{
    uint32_t i, j, u32Width, u32Height, u32Ret;
    struct bufnode *pstNode;
    const t_StC2DClipInfo *pstClip;
    void *pCtx;

    u32C2DTest_BufPoolInit(&stTestCtx, C2D_PROC_CNT * 2);

    stCtxC2D.eState = C2DTEST_STATE_FINAL;
    stCtxC2D.bSkipObdCrc = VPP_TRUE;
    stCtxC2D.c2d_ctx = vpVppIpC2D_Init(&stTestCtx.stVppCtx, 0, stTestCtx.cb);
    DVP_ASSERT_PTR_NNULL(stCtxC2D.c2d_ctx);
    if (stCtxC2D.c2d_ctx)
    {
        pCtx = stCtxC2D.c2d_ctx;

        pstClip = &astClipReg[C2D_FORMAT_RGB][VPP_RESOLUTION_SD];
        u32Ret = get_res_for_clip(pstClip->eClip, &u32Width, &u32Height, NULL);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        set_port_params(&stCtxC2D.stParamRgb, u32Width, u32Height,
                        VPP_COLOR_FORMAT_UBWC_RGBA8);
        set_port_params(&stCtxC2D.stParamUbwc, u32Width, u32Height,
                        VPP_COLOR_FORMAT_UBWC_NV12);
        vC2DTest_SetPoolParams(&stTestCtx, VPP_RESOLUTION_SD, C2D_FORMAT_RGB,
                               VPP_COLOR_FORMAT_UBWC_NV12);

        u32Ret = u32VppIpC2D_SetParam(pCtx, VPP_PORT_INPUT,
                                      stCtxC2D.stParamRgb);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_SetParam(pCtx, VPP_PORT_OUTPUT,
                                      stCtxC2D.stParamUbwc);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppIpC2D_Open(pCtx);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        for (i = 0; i < VPP_RESOLUTION_MAX; i++)
        {
            if (i > 0)
            {
                VPP_FLAG_CLR(stTestCtx.u32Flags, DRAIN_DONE);
                u32Ret = u32VppIpC2D_Drain(pCtx);
                DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
                stCtxC2D.u32CrcOutCnt = 0;

                pthread_mutex_lock(&stTestCtx.mutex);
                while (!(VPP_FLAG_IS_SET(stTestCtx.u32Flags, DRAIN_DONE)))
                    pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
                pthread_mutex_unlock(&stTestCtx.mutex);

                pstClip = &astClipReg[C2D_FORMAT_RGB][i];
                u32Ret = get_res_for_clip(pstClip->eClip, &u32Width,
                                          &u32Height, NULL);
                set_port_params(&stCtxC2D.stParamRgb, u32Width, u32Height,
                                VPP_COLOR_FORMAT_UBWC_RGBA8);
                set_port_params(&stCtxC2D.stParamUbwc, u32Width, u32Height,
                                VPP_COLOR_FORMAT_UBWC_NV12);
                vC2DTest_SetPoolParams(&stTestCtx, i, C2D_FORMAT_RGB,
                                       VPP_COLOR_FORMAT_UBWC_NV12);

                u32Ret = u32VppIpC2D_Reconfigure(pCtx, stCtxC2D.stParamRgb,
                                                 stCtxC2D.stParamUbwc);
            }
            for (j = 0; j < C2D_PROC_CNT; j++)
            {
                pthread_mutex_lock(&stTestCtx.mutex);
                while (NULL == (pstNode = get_buf(stTestCtx.buf_pool)))
                    pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
                pthread_mutex_unlock(&stTestCtx.mutex);

                vVppBuf_Clean(pstNode->pIntBuf, 0);
                fill_buf(pstNode);

                pstNode->owner = BUF_OWNER_LIBRARY;

                fill_extra_data(pstNode, eVppBufType_Progressive, 0);
                stCtxC2D.u32ExpExtraLen = pstNode->pIntBuf->stExtra.u32FilledLen;
                u32Ret = u32QueueBuf(stCtxC2D.c2d_ctx, VPP_PORT_INPUT,
                                     pstNode->pIntBuf);
                DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
            }

            for (j = 0; j < C2D_PROC_CNT; j++)
            {
                pthread_mutex_lock(&stTestCtx.mutex);
                while (NULL == (pstNode = get_buf(stTestCtx.buf_pool)))
                    pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
                pthread_mutex_unlock(&stTestCtx.mutex);

                vVppBuf_Clean(pstNode->pIntBuf, 0);
                pstNode->owner = BUF_OWNER_LIBRARY;

                u32Ret = u32QueueBuf(stCtxC2D.c2d_ctx, VPP_PORT_OUTPUT,
                                     pstNode->pIntBuf);
                DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
            }

            pthread_mutex_lock(&stTestCtx.mutex);
            LOGI("entering wait loop");
            while (stTestCtx.buf_pool->u32ListSz != (C2D_PROC_CNT * 2))
               pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
            LOGI("breaking out of wait loop");
            pthread_mutex_unlock(&stTestCtx.mutex);
        }

        VPP_FLAG_CLR(stTestCtx.u32Flags, INPUT_FLUSH_DONE);
        VPP_FLAG_CLR(stTestCtx.u32Flags, OUTPUT_FLUSH_DONE);

        u32Ret = u32VppIpC2D_Flush(pCtx, VPP_PORT_INPUT);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
        u32Ret = u32VppIpC2D_Flush(pCtx, VPP_PORT_OUTPUT);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        pthread_mutex_lock(&stTestCtx.mutex);
        while (!(VPP_FLAG_IS_SET(stTestCtx.u32Flags, INPUT_FLUSH_DONE)) ||
               !(VPP_FLAG_IS_SET(stTestCtx.u32Flags, OUTPUT_FLUSH_DONE)))
            pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
        pthread_mutex_unlock(&stTestCtx.mutex);

        u32Ret = u32VppIpC2D_Close(pCtx);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        vVppIpC2D_Term(pCtx);
    }

    LOGI("freeing buffer pools");
    free_buf_pool(stTestCtx.buf_pool, VPP_TRUE);
}

/************************************************************************
 * Global Functions
 ***********************************************************************/
TEST_CASES IpC2DTests[] = {
    TEST_CASE(IpC2D_TestBasicConfig),
    TEST_CASE(IpC2D_TestInvalidConfig),
    TEST_CASE(IpC2D_AsyncProcess),
    TEST_CASE(IpC2D_InlineProcess),
    TEST_CASE(IpC2D_AsyncSessionInlineProcess),
    TEST_CASE(IpC2D_UbwcRgbToUbwcNv12Process),
    TEST_CASE_NULL(),
};

TEST_SUITE(IpC2DSuite,
        "IpC2DTests",
        IpC2DSuiteInit,
        IpC2DSuiteTerm,
        IpC2DTestInit,
        IpC2DTestTerm,
        IpC2DTests);
