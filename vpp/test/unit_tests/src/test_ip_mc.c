/*!
 * @file test_ip_mc.c
 *
 * @cr
 * Copyright (c) 2015-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services    Implements tests for the mc (Motion Compensation) submodule
 */

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
//#define __STDC_FORMAT_MACROS
//#include <inttypes.h>

#include "buf_pool.h"
#include "test_utils.h"

#include "dvpTest.h"
#include "dvpTest_tb.h"

#define VPP_LOG_TAG     VPP_LOG_UT_MC_TAG
#define VPP_LOG_MODULE  VPP_LOG_UT_MC
#include "vpp_dbg.h"
#include "vpp.h"
// #include "vpp_core.h"
// #include "vpp_ctx.h"
#include "vpp_reg.h"
#include "vpp_ip.h"
#include "vpp_ip_frc_mc.h"
#include "vpp_utils.h"

// #include "vpp_uc.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/
#define INPUT_FLUSH_DONE        (1<<0)
#define OUTPUT_FLUSH_DONE       (1<<1)
#define DRAIN_DONE              (1<<2)

#define TIMESTAMP_FREQ          7
#define TIMESTAMP_MS            (1000000 / TIMESTAMP_FREQ)

#define IN_PATH "/data/vendor/vpp/input/"
#define IN_YUV_LINEAR_FILE "Slomo_Pitching_nv12_1280x720.yuv"
#define IN_YUV_UBWC_FILE "Slomo_Pitching_nv12_UBWC_1280x720.yuv"
#define IN_MBI_FILE "Slomo_TME_HD.mbin"
#define OUT_PATH "/data/vendor/vpp/output/"
#define OUT_YUV_LINEAR_FILE "FRCMC_Slomo_Pitching_nv12_1280x720"
#define OUT_YUV_UBWC_FILE "FRCMC_Slomo_Pitching_nv12_UBWC_1280x720"
#define OUT_YUV_FILE_EXT ".yuv"

typedef struct mc_test_ctx {
    void *mc_ctx; // context returned from mc init
    t_StVppCtx *pstCtx;
    uint32_t u32FlagCorrupt;
    uint32_t u32Inited;
    uint32_t u32FrameCopyInput;
    uint32_t u32TestTimeStamp; //do not test time stamp as default
} t_StMcTestCtx;

/************************************************************************
 * Local static variables
 ***********************************************************************/
static struct test_ctx tctx;
static t_StMcTestCtx stCtxMc;
const uint32_t u32InBufCnt = 20, u32OutBufCnt = 60;
uint32_t u32MbiBufCnt = 20;

/************************************************************************
 * Forward Declarations
 ************************************************************************/

/************************************************************************
 * Local Functions
 ***********************************************************************/
void test_mc_input_buffer_done(void *pv, t_StVppBuf *pBuf)
{
    struct test_ctx *pCtx = (struct test_ctx *)pv;
    struct bufnode *pNode;

    if (!pv || !pBuf)
    {
        DVP_ASSERT_FAIL();
        LOGE("Null pointers given on IBD. pv=%p, pBuf=%p", pv, pBuf);
        return;
    }

    LOGI("%s() with buffer=%p", __func__, pBuf);

    pNode = pBuf->pBuf->cookie;
    DVP_ASSERT_TRUE(pNode->owner == BUF_OWNER_LIBRARY);
    pNode->owner = BUF_OWNER_CLIENT;
    pthread_mutex_lock(&tctx.mutex);

    //Note: When flush, input pixel buffer may be returned to input port.
    if (pBuf->eBufPxType == eVppBufPxDataType_Compressed)
        put_buf(pCtx->buf_pool_ext, pNode);
    else
    {
        put_buf(pCtx->buf_pool, pNode);
        tctx.u32InRxCnt++;
        tctx.u32InHoldCnt--;
    }
    pthread_mutex_unlock(&tctx.mutex);

    LOGD("pBuf->eBufPxType=%d", pBuf->eBufPxType);
    pthread_cond_signal(&pCtx->cond);
}

void test_mc_output_buffer_done(void *pv, t_StVppBuf *pBuf)
{
    struct test_ctx *pCtx = (struct test_ctx *)pv;
    struct bufnode *pNode;
    t_EVppBufType eType;
    static uint32_t u32InterpCnt;
    static uint32_t u32FrcFactorOld;
    uint32_t u32FrcFactor;
    uint64_t u64OutTimestamp;
    t_StMcTestCtx *pstMcCtx;
    t_StVppIpFrcMcCb *pstCb;

    if (!pv || !pBuf)
    {
        DVP_ASSERT_FAIL();
        LOGE("Null pointers given on OBD. pv=%p, pBuf=%p", pv, pBuf);
        return;
    }

    LOGI("%s() with buffer=0x%p", __func__, pBuf);

    pstMcCtx = (t_StMcTestCtx *)pCtx->pPrivateCtx;
    pstCb = (t_StVppIpFrcMcCb *)pstMcCtx->mc_ctx;
    pNode = pBuf->pBuf->cookie;
    DVP_ASSERT_TRUE(pNode->owner == BUF_OWNER_LIBRARY);
    pNode->owner = BUF_OWNER_CLIENT;

    eType = eVppBuf_GetFrameType(pBuf);
    DVP_ASSERT_EQUAL(eType, eVppBufType_Progressive);

    pthread_mutex_lock(&tctx.mutex);
    if (VPP_FLAG_IS_SET(pBuf->pBuf->flags,VPP_BUFFER_FLAG_DATACORRUPT))
        pstMcCtx->u32FlagCorrupt = 1;
    //test timestamp
    if (pstMcCtx->u32TestTimeStamp &&
        pNode->port_owner == VPP_PORT_OUTPUT && pBuf->stPixel.u32FilledLen != 0 &&
        tctx.u32InRxCnt)
    {
        u32FrcFactor = pstCb->stInfo.u32FrcFactorActual;
        if (u32FrcFactor != u32FrcFactorOld)
        {
            u32InterpCnt = 0;
            u32FrcFactorOld = u32FrcFactor;
        }

        u64OutTimestamp = ((tctx.u32InRxCnt - 1) * TIMESTAMP_MS) +
            (TIMESTAMP_MS / u32FrcFactor * (u32InterpCnt + 1));
        if (pBuf->pBuf->timestamp != u64OutTimestamp)
        {
            LOGE("pBuf->timestamp = %llu Expected = %llu, InRx=%u, Factor=%u, InterpCnt=%u",
                 pBuf->pBuf->timestamp, u64OutTimestamp, tctx.u32InRxCnt, u32FrcFactor,
                 u32InterpCnt);
        }
        DVP_ASSERT_TRUE(pBuf->pBuf->timestamp == u64OutTimestamp);

        u32InterpCnt++;
        if (u32InterpCnt >= (u32FrcFactor - 1))
        {
            u32InterpCnt = 0;
        }
    }
    if (pNode->pIntBuf->stPixel.u32FilledLen)
    {
        dump_buf(pNode);
    }
    if (pNode->port_owner == VPP_PORT_INPUT)
    {
        if (pstMcCtx->u32FrameCopyInput)
        {
            LOGE("Frame copy input on, but received input buffer on OBD!");
            DVP_ASSERT_FAIL();
        }
        if (tctx.u32InRxCnt == 0)
            u32InterpCnt = 0;
        tctx.u32InRxCnt++;
        tctx.u32InHoldCnt--;
    }
    else
    {
        tctx.u32OutRxCnt++;
        tctx.u32OutHoldCnt--;
    }

    put_buf(pCtx->buf_pool, pNode);

    pthread_mutex_unlock(&tctx.mutex);
    pthread_cond_signal(&pCtx->cond);

}

void test_mc_event(void *pv, t_StVppEvt stEvt)
{
    LOGI("%s() got event: %u", __func__, stEvt.eType);
    struct test_ctx *pCtx = (struct test_ctx *)pv;

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

static uint32_t u32QueueMbiBuf(void *pvCtx, enum vpp_port ePort, t_StVppBuf *pBuf)
{
    return u32VppIpFrcMc_QueueBuf(pvCtx, ePort, pBuf);
}

static uint32_t u32QueuePixelBuf(void *pvCtx, enum vpp_port ePort, t_StVppBuf *pBuf)
{
    uint32_t u32Ret;
    uint32_t *pu32HoldCnt = NULL;
    uint32_t *pu32PutCnt = NULL;

    if (ePort == VPP_PORT_INPUT)
    {
        pu32HoldCnt = &tctx.u32InHoldCnt;
        pu32PutCnt = &tctx.u32InPutCnt;
    }
    else if (ePort == VPP_PORT_OUTPUT)
    {
        pu32HoldCnt = &tctx.u32OutHoldCnt;
        pu32PutCnt = &tctx.u32OutPutCnt;
    }

    if (pu32HoldCnt != NULL && pu32PutCnt != NULL)
    {
        pthread_mutex_lock(&tctx.mutex);
        (*pu32HoldCnt)++;
        (*pu32PutCnt)++;
        pthread_mutex_unlock(&tctx.mutex);
    }
    u32Ret = u32VppIpFrcMc_QueueBuf(pvCtx, ePort, pBuf);
    if (u32Ret != VPP_OK && pu32HoldCnt != NULL && pu32PutCnt != NULL)
    {
        pthread_mutex_lock(&tctx.mutex);
        (*pu32HoldCnt)--;
        (*pu32PutCnt)--;
        pthread_mutex_unlock(&tctx.mutex);
    }

    return u32Ret;
}

#define FLAG_BYPASS_BUFFER                  (1 << 0)
#define FLAG_BYPASS_NONPROGRESIVE_BUFFER    (1 << 1)
#define FLAG_BYPASS_ALGO_MC_DISABLE         (1 << 2)
#define FLAG_BUFFER_FLAG_DATACORRUPT        (1 << 3)
#define FLAG_BUFFER_FLAG_EOS                (1 << 4)
#define FLAG_CONTROL_FLAG_COPY_INPUT        (1 << 5)
#define FLAG_CONTROL_FLAG_COPY_FALLBACK     (1 << 6)
#define FLAG_BUFFER_TIMESTAMP_DECREMENT     (1 << 7)

static uint32_t u32QueueInput(uint32_t u32Flags, struct hqv_control *pstCtrl)
{
    uint32_t u32Ret;
    uint32_t i;
    struct bufnode *pNode;
    uint32_t u32BufTotal;
    struct hqv_control stCtrl;
    struct vpp_ctrl_frc_segment stFrcCtrlSeg;

    LOGI("%s()", __func__);

    u32BufTotal = u32InBufCnt + u32OutBufCnt;

    if (!tctx.buf_pool)
    {
        LOGI("%s() init_buf_pool()", __func__);
        u32Ret = init_buf_pool(&tctx.stVppCtx, &tctx.buf_pool, &tctx.params,
                               u32BufTotal, VPP_TRUE);
        if (u32Ret != VPP_OK)
            return u32Ret;
    }
    if (!tctx.buf_pool_ext)
    {
        LOGI("%s() init_buf_pool() buf_pool_ext", __func__);
        u32Ret = init_buf_pool(&tctx.stVppCtx, &tctx.buf_pool_ext, &tctx.params,
                               u32MbiBufCnt, VPP_FALSE);
        if (u32Ret != VPP_OK)
            return u32Ret;
    }

    t_StVppIpFrcMcCb *pstCb = (t_StVppIpFrcMcCb *)stCtxMc.mc_ctx;
    if (!(u32Flags & FLAG_BYPASS_ALGO_MC_DISABLE))
    {
        if (!pstCtrl)
        {
            // Set to manual and frc
            stCtrl.frc.segments = &stFrcCtrlSeg;
            stCtrl.mode = HQV_MODE_MANUAL;
            stCtrl.ctrl_type = HQV_CONTROL_FRC;
            stCtrl.frc.num_segments = 1;
            stCtrl.frc.segments->mode = HQV_FRC_MODE_SMOOTH_MOTION;
            stCtrl.frc.segments->level = HQV_FRC_LEVEL_HIGH;
            stCtrl.frc.segments->ts_start = 0;
            stCtrl.frc.segments->frame_copy_input = VPP_FALSE;
            stCtrl.frc.segments->frame_copy_on_fallback = VPP_FALSE;
            if (u32Flags & FLAG_CONTROL_FLAG_COPY_INPUT)
                stCtrl.frc.segments->frame_copy_input = VPP_TRUE;
            if (u32Flags & FLAG_CONTROL_FLAG_COPY_FALLBACK)
                stCtrl.frc.segments->frame_copy_on_fallback = VPP_TRUE;

            u32Ret = u32VppIpFrcMc_SetCtrl(pstCb, stCtrl);
        }
        else
        {
            u32Ret = u32VppIpFrcMc_SetCtrl(pstCb, *pstCtrl);
        }
        LOGD("u32VppIpFrcMc_SetCtrl ,return = %d ", u32Ret);
        if (u32Ret != VPP_OK)
            return u32Ret;
    }
    else
    {
        stCtrl.mode = HQV_MODE_OFF;
        u32Ret = u32VppIpFrcMc_SetCtrl(pstCb, stCtrl);
        LOGD("u32VppIpFrcMc_SetCtrl ,return = %d ", u32Ret);
        if (u32Ret != VPP_OK)
            return u32Ret;
    }

    // Queue to input pool
    LOGI("%s() Push buffers loop", __func__);
    for (i = 0; i < u32MbiBufCnt; i++)
    {
        //2. MBI buf
        pNode = get_buf(tctx.buf_pool_ext);
        DVP_ASSERT_PTR_NNULL(pNode);

        if(pNode)
        {
            u32Ret = fill_mbi_buf(pNode);
            if (u32Ret)
                LOGD("Unable to fill MBI buffer.  May contain garbage data");
            pNode->owner = BUF_OWNER_LIBRARY;

            pNode->pIntBuf->eBufPxType = eVppBufPxDataType_Compressed;
            pNode->pIntBuf->stPixel.eMapping = eVppBuf_MappedInternal;
            pNode->pIntBuf->pBuf->timestamp = i * TIMESTAMP_MS;  //Inc with 33ms every input buffer
            u32Ret = u32QueueMbiBuf(stCtxMc.mc_ctx, VPP_PORT_INPUT, pNode->pIntBuf);
            DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
        }
        else
        {
            DVP_ASSERT_FAIL();
            LOGE("%s(): pNode == NULL",__func__);
            return VPP_ERR;
        }

        //1. input pixel buf
        pNode = get_buf(tctx.buf_pool);

        if(pNode == NULL)
        {
            LOGE("%s(): pNode == NULL",__func__);
            return VPP_ERR;
        }

        fill_buf(pNode);

        pNode->owner = BUF_OWNER_LIBRARY;
        pNode->port_owner = VPP_PORT_INPUT;

        if(u32Flags & FLAG_BYPASS_BUFFER)
        {
            VPP_FLAG_SET(pNode->pIntBuf->u32InternalFlags, VPP_BUF_FLAG_BYPASS);
        }
        if(u32Flags & FLAG_BYPASS_NONPROGRESIVE_BUFFER)
        {
            pNode->pIntBuf->eBufType = eVppBufType_Interleaved_TFF;//!= eVppBufType_Progressive
        }

        if(u32Flags & FLAG_BUFFER_FLAG_DATACORRUPT)
        {
            VPP_FLAG_SET(pNode->pIntBuf->pBuf->flags, VPP_BUFFER_FLAG_DATACORRUPT);
        }

        if((u32Flags & FLAG_BUFFER_FLAG_EOS) && (u32MbiBufCnt - 1 == i))
        {
            VPP_FLAG_SET(pNode->pIntBuf->pBuf->flags, VPP_BUFFER_FLAG_EOS);
        }

        pNode->pIntBuf->eBufPxType = eVppBufPxDataType_Raw;
        pNode->pIntBuf->stPixel.eMapping = eVppBuf_MappedInternal;
        if (u32Flags & FLAG_BUFFER_TIMESTAMP_DECREMENT)
            pNode->pIntBuf->pBuf->timestamp = (u32MbiBufCnt - i) * TIMESTAMP_MS;
        else
            pNode->pIntBuf->pBuf->timestamp = i * TIMESTAMP_MS;
        pNode->pIntBuf->u32TimestampFrameRate = TIMESTAMP_FREQ;
        pNode->pIntBuf->u32OperatingRate = TIMESTAMP_FREQ;
        if (i > 5 && i < 10)
            pNode->pIntBuf->u32OperatingRate = 15;
        u32Ret = u32QueuePixelBuf(stCtxMc.mc_ctx, VPP_PORT_INPUT, pNode->pIntBuf);
    }
    return VPP_OK;
}

static uint32_t u32BypassTest(uint32_t u32Flags)
{
    uint32_t u32Ret;
    uint32_t i;
    struct bufnode *pNode;

    stCtxMc.u32TestTimeStamp = 0;//do not test time stamp in this test
    uint32_t u32BufTotal;

    u32BufTotal = u32InBufCnt + u32OutBufCnt;

    u32MbiBufCnt = 5;

    u32Ret = u32QueueInput(u32Flags, NULL);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    if (u32Ret != VPP_OK)
        return u32Ret;

    u32Ret = u32VppIpFrcMc_Drain(stCtxMc.mc_ctx);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    if (u32Ret != VPP_OK)
        return u32Ret;

    for (i = 0; i < u32OutBufCnt; i++)
    {
        //3. Output pixel buf
        pNode = get_buf(tctx.buf_pool);
        DVP_ASSERT_PTR_NNULL(pNode);

        if(pNode)
        {
            pNode->owner = BUF_OWNER_LIBRARY;
            pNode->port_owner = VPP_PORT_OUTPUT;
            pNode->pIntBuf->eBufPxType = eVppBufPxDataType_Raw;
            u32Ret = u32QueuePixelBuf(stCtxMc.mc_ctx, VPP_PORT_OUTPUT, pNode->pIntBuf);
            DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
        }
        else
        {
            DVP_ASSERT_FAIL();
            LOGE("%s(): pNode == NULL",__func__);
            return VPP_ERR;
        }
    }

    pthread_mutex_lock(&tctx.mutex);

    while (!(tctx.u32Flags & DRAIN_DONE))
        pthread_cond_wait(&tctx.cond, &tctx.mutex);
    pthread_mutex_unlock(&tctx.mutex);

    if(u32Flags & FLAG_BUFFER_FLAG_DATACORRUPT)
        DVP_ASSERT_TRUE(stCtxMc.u32FlagCorrupt);
    else
        DVP_ASSERT_FALSE(stCtxMc.u32FlagCorrupt);

    DVP_ASSERT_TRUE(tctx.u32Flags & DRAIN_DONE);

    return VPP_OK;
}

static void IpMc_Init(uint32_t u32Width, uint32_t u32Height, enum vpp_color_format eFmt,
                      uint32_t u32Flags)
{
    uint32_t u32Ret;
    char cFileNameOut[MAX_FILE_SEG_LEN];
    char cFileNameIn[MAX_FILE_SEG_LEN];

    tctx_common_init(&tctx);

    tctx.cb.input_buffer_done = test_mc_input_buffer_done;
    tctx.cb.output_buffer_done = test_mc_output_buffer_done;
    tctx.cb.event = test_mc_event;
    tctx.cb.pv = &tctx;

    stCtxMc.u32FrameCopyInput = 0;
    stCtxMc.u32FlagCorrupt = 0;
    tctx.pPrivateCtx = &stCtxMc;

    tctx_set_port_params(&tctx, u32Width, u32Height, eFmt);
    tctx.params.u32Width = u32Width;
    tctx.params.u32Height = u32Height;
    tctx.params.eBufferType = eVppBufType_Progressive;

    tctx.params.eInputFileType = FILE_TYPE_MULTI_FRAMES;
    tctx.params.eOutputFileType = FILE_TYPE_MULTI_FRAMES;
    if (VPP_FLAG_IS_SET(u32Flags, VPP_SESSION_SECURE))
        tctx.params.eProtection = PROTECTION_ZONE_SECURE;
    else
        tctx.params.eProtection = PROTECTION_ZONE_NONSECURE;
    if (eFmt == VPP_COLOR_FORMAT_UBWC_NV12)
    {
        tctx.params.eInputFileFormat = FILE_FORMAT_NV12_UBWC;
        tctx.params.eInputBufFmt = VPP_COLOR_FORMAT_UBWC_NV12;
        tctx.params.eOutputFileFormat = FILE_FORMAT_NV12_UBWC;
        tctx.params.eOutputBufFmt = VPP_COLOR_FORMAT_UBWC_NV12;
        strlcpy(cFileNameIn, IN_YUV_UBWC_FILE, sizeof(cFileNameIn));
        snprintf(cFileNameOut, MAX_FILE_SEG_LEN, "%s%s", OUT_YUV_UBWC_FILE, OUT_YUV_FILE_EXT);
    }
    else
    {
        tctx.params.eInputFileFormat = FILE_FORMAT_NV12;
        tctx.params.eInputBufFmt = VPP_COLOR_FORMAT_NV12_VENUS;
        tctx.params.eOutputFileFormat = FILE_FORMAT_NV12;
        tctx.params.eOutputBufFmt = VPP_COLOR_FORMAT_NV12_VENUS;
        strlcpy(cFileNameIn, IN_YUV_LINEAR_FILE, sizeof(cFileNameIn));
        snprintf(cFileNameOut, MAX_FILE_SEG_LEN, "%s%s", OUT_YUV_LINEAR_FILE, OUT_YUV_FILE_EXT);
    }
    char cDir[] = IN_PATH;
    strlcpy(tctx.params.cInputPath, cDir, sizeof(tctx.params.cInputPath));
    strlcpy(tctx.params.cInputName, cFileNameIn, sizeof(tctx.params.cInputName));

    char cMbiFileName[] = IN_MBI_FILE;
    strlcpy(tctx.params.cInputNameMbi, cMbiFileName, sizeof(tctx.params.cInputNameMbi));

    char cDirOut[] = OUT_PATH;
    strlcpy(tctx.params.cOutputPath, cDirOut, sizeof(tctx.params.cOutputPath));
    strlcpy(tctx.params.cOutputName, cFileNameOut, sizeof(tctx.params.cOutputName));

    struct vpp_callbacks cb;
    memset(&cb, 0, sizeof(cb));
    stCtxMc.pstCtx = vpp_init(0, cb);

    stCtxMc.mc_ctx = vpVppIpFrcMc_Init(stCtxMc.pstCtx, u32Flags, tctx.cb);
    DVP_ASSERT_PTR_NNULL(stCtxMc.mc_ctx);

    u32Ret = u32VppIpFrcMc_SetParam(stCtxMc.mc_ctx, VPP_PORT_INPUT, tctx.port_param);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    u32Ret = u32VppIpFrcMc_SetParam(stCtxMc.mc_ctx, VPP_PORT_OUTPUT, tctx.port_param);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    u32Ret = u32VppIpFrcMc_Open(stCtxMc.mc_ctx);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    stCtxMc.u32Inited = 1;
}

static void IpMc_Term(void)
{
    u32VppIpFrcMc_Flush(stCtxMc.mc_ctx, VPP_PORT_INPUT);
    u32VppIpFrcMc_Flush(stCtxMc.mc_ctx, VPP_PORT_OUTPUT);
    pthread_mutex_lock(&tctx.mutex);
    LOGI("Wait for flush, u32Flags=%u", tctx.u32Flags);
    while (!(tctx.u32Flags & INPUT_FLUSH_DONE) || !(tctx.u32Flags & OUTPUT_FLUSH_DONE))
    {
        pthread_cond_wait(&tctx.cond, &tctx.mutex);
    }
    tctx.u32Flags &= ~(INPUT_FLUSH_DONE | OUTPUT_FLUSH_DONE);
    LOGI("Flush Done");
    pthread_mutex_unlock(&tctx.mutex);

    u32VppIpFrcMc_Close(stCtxMc.mc_ctx);

    LOGD("freeing buffer pools");
    free_buf_pool(tctx.buf_pool, VPP_FALSE);
    free_buf_pool(tctx.buf_pool_ext, VPP_TRUE);
    tctx.buf_pool = NULL;
    tctx.buf_pool_ext = NULL;

    vVppIpFrcMc_Term(stCtxMc.mc_ctx);

    vpp_term(stCtxMc.pstCtx);
    stCtxMc.pstCtx = NULL;

    tctx_common_destroy(&tctx);
    stCtxMc.u32Inited = 0;
}

static void FrcMcAlgoTest(uint32_t u32Width, uint32_t u32Height, enum vpp_color_format eFmt,
                          uint32_t u32Flags, struct hqv_control *pstCtrl)
{
    uint32_t u32Ret;
    uint32_t i;
    struct bufnode *pNode;
    uint32_t u32BufTotal;
    char cFileNameOut[MAX_FILE_SEG_LEN];
    t_StVppIpBufReq stBufReqIn, stBufReqOut;

    u32BufTotal = u32InBufCnt + u32OutBufCnt;
    stCtxMc.u32TestTimeStamp = 1;//test time stamp in this test

    if (stCtxMc.u32Inited)
    {
        IpMc_Term();
    }
    IpMc_Init(u32Width, u32Height, eFmt, u32Flags);

    if (pstCtrl)
    {
        if (eFmt == VPP_COLOR_FORMAT_UBWC_NV12)
        {
            snprintf(cFileNameOut, MAX_FILE_SEG_LEN, "%s_Mode%u_Level%u_Interp%u%s",
                     OUT_YUV_UBWC_FILE, pstCtrl->frc.segments->mode, pstCtrl->frc.segments->level,
                     pstCtrl->frc.segments->interp, OUT_YUV_FILE_EXT);
        }
        else
        {
            snprintf(cFileNameOut, MAX_FILE_SEG_LEN, "%s_Mode%u_Level%u_Interp%u%s",
                     OUT_YUV_LINEAR_FILE, pstCtrl->frc.segments->mode, pstCtrl->frc.segments->level,
                     pstCtrl->frc.segments->interp, OUT_YUV_FILE_EXT);
        }
        strlcpy(tctx.params.cOutputName, cFileNameOut, sizeof(tctx.params.cOutputName));
    }
    u32Ret = u32VppIpFrcMc_GetBufferRequirements(stCtxMc.mc_ctx, &stBufReqIn, &stBufReqOut);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    if (u32Ret != VPP_OK)
        return;

    u32Ret = u32QueueInput(0, pstCtrl);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    if (u32Ret != VPP_OK)
        return;

    for (i = 0; i < u32OutBufCnt; i++)
    {
        //3. Output pixel buf
        pNode = get_buf(tctx.buf_pool);
        DVP_ASSERT_PTR_NNULL(pNode);

        if(pNode)
        {
            pNode->owner = BUF_OWNER_LIBRARY;
            pNode->port_owner = VPP_PORT_OUTPUT;
            pNode->pIntBuf->eBufPxType = eVppBufPxDataType_Raw;
            pNode->pIntBuf->stPixel.eMapping = eVppBuf_MappedInternal;
            u32Ret = u32QueuePixelBuf(stCtxMc.mc_ctx, VPP_PORT_OUTPUT, pNode->pIntBuf);
            DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
            LOGI("\nPush loop %d",i);
        }
        else
        {
            DVP_ASSERT_FAIL();
            LOGE("%s(): pNode == NULL",__func__);
            return;
        }
    }
    pthread_mutex_lock(&tctx.mutex);

    LOGI("Entering wait loop, InPutCnt=%u OutPutCnt=%u InHoldCnt=%u OutHoldCnt=%u",
         tctx.u32InPutCnt, tctx.u32OutPutCnt,tctx.u32InHoldCnt, tctx.u32OutHoldCnt);
    while (tctx.u32InHoldCnt >= stBufReqIn.u32MinCnt)
    {
        pthread_cond_wait(&tctx.cond, &tctx.mutex);
        LOGI("InPutCnt=%d OutPutCnt=%d InHoldCnt=%d OutHoldCnt=%d", tctx.u32InPutCnt,
             tctx.u32OutPutCnt,tctx.u32InHoldCnt, tctx.u32OutHoldCnt);
    }

    LOGI("breaking out of wait loop");

    pthread_mutex_unlock(&tctx.mutex);

    u32VppIpFrcMc_Flush(stCtxMc.mc_ctx, VPP_PORT_INPUT);
    u32VppIpFrcMc_Flush(stCtxMc.mc_ctx, VPP_PORT_OUTPUT);

    pthread_mutex_lock(&tctx.mutex);
    LOGI("Wait for flush, u32Flags=%u", tctx.u32Flags);
    while (!(tctx.u32Flags & INPUT_FLUSH_DONE) || !(tctx.u32Flags & OUTPUT_FLUSH_DONE))
    {
        pthread_cond_wait(&tctx.cond, &tctx.mutex);
    }
    tctx.u32Flags &= ~(INPUT_FLUSH_DONE | OUTPUT_FLUSH_DONE);
    LOGI("Flush Done, u32Flags=%u", tctx.u32Flags);
    pthread_mutex_unlock(&tctx.mutex);
}

/************************************************************************
 * Test Functions
 ***********************************************************************/
TEST_SUITE_INIT(IpMcSuiteInit)
{
}

TEST_SUITE_TERM(IpMcSuiteTerm)
{
}

TEST_SETUP(IpMcTestInit)
{

}
TEST_CLEANUP(IpMcTestTerm)
{
}

TEST(IpMc_InputBuffer_FrcMcAlgo_HD)
{
    uint32_t u32W, u32H, i;
    uint32_t u32Mode, u32Level, u32Interp;
    struct hqv_control stCtrl;
    struct vpp_ctrl_frc_segment stFrcCtrlSeg;
    enum vpp_color_format eFmt = VPP_COLOR_FORMAT_NV12_VENUS;

    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    u32W = 1280;
    u32H = 720;
    stCtrl.mode = HQV_MODE_MANUAL;
    stCtrl.ctrl_type = HQV_CONTROL_FRC;
    stCtrl.frc.segments = &stFrcCtrlSeg;
    stCtrl.frc.num_segments = 1;
    for (i = 0; i < 2; i++)
    {
        if (i == 1)
            eFmt = VPP_COLOR_FORMAT_UBWC_NV12;

        for (u32Mode = HQV_FRC_MODE_OFF; u32Mode < HQV_FRC_MODE_MAX; u32Mode++)
        {
            if (u32Mode == HQV_FRC_MODE_OFF)
                continue;
            stCtrl.frc.segments->mode = u32Mode;
            for (u32Level = HQV_FRC_LEVEL_OFF; u32Level < HQV_FRC_LEVEL_MAX; u32Level++)
            {
                stCtrl.frc.segments->level = u32Level;
                if (u32Mode == HQV_FRC_MODE_SLOMO)
                {
                    for (u32Interp = HQV_FRC_INTERP_1X; u32Interp < HQV_FRC_INTERP_MAX; u32Interp++)
                    {
                        stCtrl.frc.segments->interp = u32Interp;
                        FrcMcAlgoTest(u32W, u32H, eFmt, 0, &stCtrl);
                    }
                }
                else
                {
                    stCtrl.frc.segments->interp = 0;
                    FrcMcAlgoTest(u32W, u32H, eFmt, 0, &stCtrl);
                }
            }
        }
    }
    IpMc_Term();
}

/*!
 *  Make sure that if we queue an equal number of input and output buffers, we
 *  get a drain done callback once all of the buffers have been returned.
 */
TEST(IpMc_Drain)
{
    uint32_t u32Ret;
    uint32_t i;
    struct bufnode *pNode;

    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    stCtxMc.u32TestTimeStamp = 0;//do not test time stamp in this test
    uint32_t u32BufTotal;

    u32BufTotal = u32InBufCnt + u32OutBufCnt;

    u32Ret = u32QueueInput(0, NULL);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    u32Ret = u32VppIpFrcMc_Drain(stCtxMc.mc_ctx);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    for (i = 0; i < u32OutBufCnt; i++)
    {
        //3. Output pixel buf
        pNode = get_buf(tctx.buf_pool);
        DVP_ASSERT_PTR_NNULL(pNode);

        if(pNode)
        {
            pNode->owner = BUF_OWNER_LIBRARY;
            pNode->port_owner = VPP_PORT_OUTPUT;
            pNode->pIntBuf->eBufPxType = eVppBufPxDataType_Raw;
            u32Ret = u32QueuePixelBuf(stCtxMc.mc_ctx, VPP_PORT_OUTPUT, pNode->pIntBuf);
            DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
        }
        else
        {
            DVP_ASSERT_FAIL();
            LOGE("%s(): pNode == NULL",__func__);
            return;
        }
    }
    pthread_mutex_lock(&tctx.mutex);

    while (!(tctx.u32Flags & DRAIN_DONE))
        pthread_cond_wait(&tctx.cond, &tctx.mutex);
    pthread_mutex_unlock(&tctx.mutex);

    DVP_ASSERT_TRUE(tctx.u32Flags & DRAIN_DONE);
    IpMc_Term();
}

/*!
 * Basic drain test case. Make sure that if we have never enqueued any buffers,
 * if we request a drain, we get back a drain done right away.
 */
TEST(IpMc_DrainNoBuffers)
{
    uint32_t u32Ret;

    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    u32Ret = u32VppIpFrcMc_Drain(stCtxMc.mc_ctx);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    while (!(tctx.u32Flags & DRAIN_DONE))
        pthread_cond_wait(&tctx.cond, &tctx.mutex);
    pthread_mutex_unlock(&tctx.mutex);

    DVP_ASSERT_TRUE(tctx.u32Flags & DRAIN_DONE);
    IpMc_Term();
}

/*!
 * Test bypass of non-progressive buffer
 */
TEST(IpMc_BypassNonProgressiveBuffer)
{
    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    uint32_t u32Ret = u32BypassTest(FLAG_BYPASS_NONPROGRESIVE_BUFFER);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    IpMc_Term();
}

/*!
 * Test bypass of buffer with bypass flag
 */
TEST(IpMc_BypassBypassBuffer)
{
    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    uint32_t u32Ret = u32BypassTest(FLAG_BYPASS_BUFFER);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    IpMc_Term();
}

/*!
 * Test bypass of buffer without set and thus frc mc disabled
 */
TEST(IpMc_BypassMcDisable)
{
    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    uint32_t u32Ret = u32BypassTest(FLAG_BYPASS_ALGO_MC_DISABLE);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    IpMc_Term();
}

/*!
 * Test FlagDataCorrupt
 */
TEST(IpMc_FlagDataCorrupt)
{
    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    uint32_t u32Ret = u32BypassTest(FLAG_BUFFER_FLAG_DATACORRUPT);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    IpMc_Term();
}

/*!
 * Test FlagEos
 */
TEST(IpMc_FlagEos)
{
    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    uint32_t u32Ret = u32BypassTest(FLAG_BUFFER_FLAG_EOS);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    IpMc_Term();
}

/*!
 * Test FlagEos with Frame Copy
 */
TEST(IpMc_FlagEosFrameCopy)
{
    uint32_t u32Ret;

    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    stCtxMc.u32FrameCopyInput = 1;

    u32Ret = u32BypassTest(FLAG_BUFFER_FLAG_EOS | FLAG_CONTROL_FLAG_COPY_FALLBACK |
                           FLAG_CONTROL_FLAG_COPY_INPUT);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    IpMc_Term();
}

/*!
 * Test Timestamp Decrement
 */
TEST(IpMc_TimestampDecrement)
{
    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    uint32_t u32Ret = u32BypassTest(FLAG_BUFFER_TIMESTAMP_DECREMENT);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    IpMc_Term();
}

TEST(IpMc_Reconfig)
{
    LOGD("\n--------------\n1st mc_algo test\n---------------\n");
    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    uint32_t u32Ret = u32BypassTest(0);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    uint32_t i;
    struct bufnode *pNode;
    //struct vpp_port_param stParam = {1920, 1080, VPP_COLOR_FORMAT_NV12_VENUS};
    uint32_t u32Width = 1280;
    uint32_t u32Height = 720;

    stCtxMc.u32TestTimeStamp = 0;//do not test time stamp in this test
    uint32_t u32BufTotal;

    tctx.u32Flags &= ~DRAIN_DONE;

    LOGD("\n--------------\n2nd mc_algo test\n---------------\n");
    u32BufTotal = u32InBufCnt + u32OutBufCnt;

    u32Ret = u32VppIpFrcMc_Drain(stCtxMc.mc_ctx);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    pthread_mutex_lock(&tctx.mutex);
    while (!(tctx.u32Flags & DRAIN_DONE))
        pthread_cond_wait(&tctx.cond, &tctx.mutex);
    tctx.u32Flags &= ~DRAIN_DONE;
    pthread_mutex_unlock(&tctx.mutex);

    u32Width = 1280/2;
    u32Height = 720/2;

    tctx_set_port_params(&tctx, u32Width, u32Height, VPP_COLOR_FORMAT_NV12_VENUS);
    tctx.params.u32Width = u32Width;
    tctx.params.u32Height = u32Height;
    tctx.params.eBufferType=eVppBufType_Progressive;
    tctx.params.eInputFileFormat=FILE_FORMAT_NV12;
    tctx.params.eInputFileType=FILE_TYPE_MULTI_FRAMES;
    tctx.params.eInputBufFmt=VPP_COLOR_FORMAT_NV12_VENUS;

    u32Ret = u32VppIpFrcMc_Reconfigure(stCtxMc.mc_ctx, tctx.port_param, tctx.port_param);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    u32Ret = u32VppIpFrcMc_Flush(stCtxMc.mc_ctx, VPP_PORT_INPUT);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    u32Ret = u32VppIpFrcMc_Flush(stCtxMc.mc_ctx, VPP_PORT_OUTPUT);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    pthread_mutex_lock(&tctx.mutex);
    LOGI("Wait for flush, u32Flags=%u", tctx.u32Flags);
    while (!(tctx.u32Flags & INPUT_FLUSH_DONE) || !(tctx.u32Flags & OUTPUT_FLUSH_DONE))
    {
        pthread_cond_wait(&tctx.cond, &tctx.mutex);
    }
    tctx.u32Flags &= ~(INPUT_FLUSH_DONE | OUTPUT_FLUSH_DONE);
    LOGI("Flush Done");
    pthread_mutex_unlock(&tctx.mutex);

    free_buf_pool(tctx.buf_pool, VPP_TRUE);
    free_buf_pool(tctx.buf_pool_ext, VPP_TRUE);
    tctx.buf_pool = NULL;
    tctx.buf_pool_ext = NULL;

    u32Ret = u32QueueInput(0, NULL);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    u32Ret = u32VppIpFrcMc_Drain(stCtxMc.mc_ctx);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    for (i = 0; i < u32OutBufCnt; i++)
    {
        //3. Output pixel buf
        pNode = get_buf(tctx.buf_pool);
        DVP_ASSERT_PTR_NNULL(pNode);

        if(pNode)
        {
            pNode->owner = BUF_OWNER_LIBRARY;
            pNode->port_owner = VPP_PORT_OUTPUT;
            pNode->pIntBuf->eBufPxType = eVppBufPxDataType_Raw;
            u32Ret = u32QueuePixelBuf(stCtxMc.mc_ctx, VPP_PORT_OUTPUT, pNode->pIntBuf);
            DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
        }
        else
        {
            DVP_ASSERT_FAIL();
            LOGE("%s(): pNode == NULL",__func__);
            return;
        }
    }

    pthread_mutex_lock(&tctx.mutex);

    while (!(tctx.u32Flags & DRAIN_DONE))
        pthread_cond_wait(&tctx.cond, &tctx.mutex);
    pthread_mutex_unlock(&tctx.mutex);
    IpMc_Term();
}

TEST(IpMc_StartSequence)
{
    uint32_t u32;
    struct vpp_callbacks cb;

    tctx_common_init(&tctx);
    tctx.cb.input_buffer_done = test_mc_input_buffer_done;
    tctx.cb.output_buffer_done = test_mc_output_buffer_done;
    tctx.cb.event = test_mc_event;
    tctx.cb.pv = &tctx;

    memset(&cb, 0, sizeof(cb));
    stCtxMc.pstCtx = vpp_init(0, cb);
    DVP_ASSERT_PTR_NNULL(stCtxMc.pstCtx);

    // Init without boot
    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    IpMc_Term();

    // Init after boot
    u32 = u32VppIpFrcMc_Boot();
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    IpMc_Init(1280, 720, VPP_COLOR_FORMAT_NV12_VENUS, 0);
    IpMc_Term();

    u32 = u32VppIpFrcMc_Shutdown();
    DVP_ASSERT_EQUAL(u32, VPP_OK);
}

TEST(IpMc_BasicBootShutdown)
{
    uint32_t u32;

    u32 = u32VppIpFrcMc_Boot();
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    u32 = u32VppIpFrcMc_Shutdown();
    DVP_ASSERT_EQUAL(u32, VPP_OK);
}

TEST(IpMc_RepeatedBootShutdown)
{
    uint32_t u32, i;

    for (i = 0; i < 50; i++)
    {
        LOGI("%s, i=%u", __func__, i);

        u32 = u32VppIpFrcMc_Boot();
        DVP_ASSERT_EQUAL(u32, VPP_OK);

        u32 = u32VppIpFrcMc_Shutdown();
        DVP_ASSERT_EQUAL(u32, VPP_OK);
    }
}

TEST(IpMc_RepeatedBoot)
{
    uint32_t u32, i;

    for (i = 0; i < 20; i++)
    {
        LOGI("%s, i=%u", __func__, i);

        // Boot currently only boots tunings and is expected to always return success unless
        // there is a fatal error since only certain calls are skipped if previously booted.
        u32 = u32VppIpFrcMc_Boot();
        DVP_ASSERT_EQUAL(u32, VPP_OK);
    }

    u32 = u32VppIpFrcMc_Shutdown();
    DVP_ASSERT_EQUAL(u32, VPP_OK);
}

TEST(IpMc_RepeatedShutdown)
{
    uint32_t u32, i;

    u32 = u32VppIpFrcMc_Boot();
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    for (i = 0; i < 20; i++)
    {
        LOGI("%s, i=%u", __func__, i);
        u32 = u32VppIpFrcMc_Shutdown();

        if (!i)
            DVP_ASSERT_EQUAL(u32, VPP_OK);
        else
            DVP_ASSERT_NEQUAL(u32, VPP_OK);
    }
}

#if 0
TEST(IpMc_InputBuffer_FrcMcAlgo_Secure)
{
    uint32_t u32W, u32H;
    struct hqv_control stCtrl;
    enum vpp_color_format eFmt = VPP_COLOR_FORMAT_NV12_VENUS;

    u32W = 1280;
    u32H = 720;
    stCtrl.mode = HQV_MODE_MANUAL;
    stCtrl.ctrl_type = HQV_CONTROL_FRC;
    stCtrl.frc.mode = HQV_FRC_MODE_SMOOTH_MOTION;
    stCtrl.frc.level = HQV_FRC_LEVEL_HIGH;
    stCtrl.frc.interp = HQV_FRC_INTERP_2X;
    FrcMcAlgoTest(u32W, u32H, eFmt, VPP_SESSION_SECURE, &stCtrl);
}
#endif
/************************************************************************
 * Global Functions
 ***********************************************************************/
TEST_CASES IpMcTests[] = {
    TEST_CASE(IpMc_InputBuffer_FrcMcAlgo_HD),
    TEST_CASE(IpMc_DrainNoBuffers),
    TEST_CASE(IpMc_Drain),
    TEST_CASE(IpMc_BypassBypassBuffer),
    TEST_CASE(IpMc_BypassNonProgressiveBuffer),
    TEST_CASE(IpMc_BypassMcDisable),
    TEST_CASE(IpMc_FlagDataCorrupt),
    TEST_CASE(IpMc_FlagEos),
    TEST_CASE(IpMc_FlagEosFrameCopy),
    TEST_CASE(IpMc_TimestampDecrement),
    TEST_CASE(IpMc_Reconfig),
    TEST_CASE(IpMc_StartSequence),
    TEST_CASE(IpMc_BasicBootShutdown),
    TEST_CASE(IpMc_RepeatedBootShutdown),
    TEST_CASE(IpMc_RepeatedBoot),
    TEST_CASE(IpMc_RepeatedShutdown),
#if 0
    TEST_CASE(IpMc_InputBuffer_FrcMcAlgo_Secure),
#endif
    TEST_CASE_NULL(),
};

TEST_SUITE(IpMcSuite,
           "IpMcTests",
           IpMcSuiteInit,
           IpMcSuiteTerm,
           IpMcTestInit,
           IpMcTestTerm,
           IpMcTests);
