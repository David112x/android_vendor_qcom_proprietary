/*!
 * @file test_ip_hvx.c
 *
 * @cr
 * Copyright (c) 2015-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @services
 *
 *
 */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "dvpTest.h"
#include "dvpTest_tb.h"

#define VPP_LOG_TAG     VPP_LOG_MODULE_HVX_TAG
#define VPP_LOG_MODULE  VPP_LOG_MODULE_HVX

#include "vpp_dbg.h"
#include "vpp_def.h"
#include "vpp_ctx.h"
#include "vpp_core.h"
#include "vpp.h"
#include "vpp_uc.h"
#include "vpp_utils.h"

#include "test_mock_registry.h"
#include "vpp_ip_hvx.h"
#include "test_utils.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/
struct StHvxTestCtx {
    void *pstHvxCb; // context returned from hvx init
    uint32_t u32ExpExtraLen;
    uint32_t u32ExpOBD;
    uint32_t u32DrainOBD;
    uint32_t u32BypassExpected;
    uint32_t* au32BypSeq;
    uint32_t* au32OutFlgSeq;
    uint32_t* au32MarkerSeq;
    // function to validate output buffers @ obd
    void (*pfnc_obd_cust_check)(t_StVppBuf*);
};

#define INPUT_FLUSH_DONE        (1<<0)
#define OUTPUT_FLUSH_DONE       (1<<1)
#define DRAIN_DONE              (1<<2)

#define ROTATETEST_INNAME                       "beach_1080p_track1.yuv"
#define ROTATETEST_INVAL_INNAME                 "beach_1080p_INVALID_track1.yuv"

#define ROTATETEST_CNR_OUTNAME                  "beach_1080p_CNR_track1.yuv"
#define ROTATETEST_AIE_OUTNAME                  "beach_1080p_AIE_track1.yuv"
#define ROTATETEST_DI_OUTNAME                   "beach_1080p_DI_track1.yuv"
#define ROTATETEST_GLOBAL_DEMO_OUTNAME          "beach_1080p_GLOBAL_DEMO_track1.yuv"
#define ROTATETEST_DEFAULT_OUTNAME              "beach_1080p_DEFAULT_track1.yuv"
#define ROTATETEST_INVAL_OUTNAME                "beach_1080p_INVALID_track1.yuv"
#define ROTATETEST_INVAL_OUTNAME                "beach_1080p_INVALID_track1.yuv"
#define ROTATETEST_CNR_AIE_OUTNAME              "beach_1080p_CNR_AIE_track1.yuv"
#define ROTATETEST_AIE_VALRECONFIG_OUTNAME      "beach_1080p_AIE_valid_track1.yuv"
#define ROTATETEST_AIE_INVALRECONFIG_OUTNAME    "beach_1080p_AIE_invalid_track1.yuv"

#define ROTATETEST2_INNAME                      "bridge_480p_track1.yuv"
#define ROTATETEST2_AIEVALID_OUTNAME            "bridge_480p_AIEVALID_track1.yuv"
#define ROTATETEST2_AIEINVALID_OUTNAME          "bridge_480p_AIEINVALID_track1.yuv"

#define SET_CTRL_CADE(ctrl,md,lvl,cont,sat)       ctrl.mode = md; ctrl.cade_level = lvl; ctrl.contrast = cont; ctrl.saturation = sat;
#define SET_CTRL_CNR(ctrl,md,lvl)                 ctrl.mode = md; ctrl.level = lvl;
#define SET_CTRL_AIE(ctrl,md,hmd,c_lvl,lsg,lso,las,labl,labh)     \
    ctrl.mode = md; ctrl.hue_mode = hmd; ctrl.cade_level = c_lvl; ctrl.ltm_sat_gain = lsg; ctrl.ltm_sat_offset = lso; ctrl.ltm_ace_str = las; ctrl.ltm_ace_bright_l = labl; ctrl.ltm_ace_bright_h = labh;
#define SET_CTRL_DI(ctrl,md)                      ctrl.mode = md;
#define SET_CTRL_DEMO(ctrl,perc,dir)              ctrl.process_percent = perc; ctrl.process_direction = dir;

#define SET_FILENAMES(inname,outname,maxlen) { \
    strlcpy(pstTestCtx->params.cInputName, inname, maxlen); \
    strlcpy(pstTestCtx->params.cOutputName, outname, maxlen); \
}
#define SET_BUF_PARAMS(w,h,fmt) { \
    pstTestCtx->params.u32Width = w; \
    pstTestCtx->params.u32Height = h; \
    pstTestCtx->params.eInputBufFmt = fmt; \
    pstTestCtx->params.eOutputBufFmt = fmt; \
}

/************************************************************************
 * Local static variables
 ***********************************************************************/
static struct test_ctx stTestCtx;
static struct test_ctx *pstTestCtx = &stTestCtx;
static struct StHvxTestCtx stCtxHvx;
static const uint32 cu32_valid_buf_flags = VPP_BUFFER_FLAG_EOS |
    VPP_BUFFER_FLAG_DATACORRUPT | VPP_BUFFER_FLAG_SYNCFRAME |
    VPP_BUFFER_FLAG_READONLY;

/************************************************************************
 * Forward Declarations
 ************************************************************************/

/************************************************************************
 * Local Functions
 ***********************************************************************/
static void obd_buf_check(t_StVppBuf *pBuf)
{
    t_EVppBufType eType;

    if (pBuf->eQueuedPort==VPP_PORT_INPUT)
        LOGI("BYPASS_DETECTED!\n");

    eType = eVppBuf_GetFrameType(pBuf);
    DVP_ASSERT_EQUAL(eType, eVppBufType_Progressive);

    if (!stCtxHvx.u32BypassExpected)
        DVP_ASSERT_EQUAL(pBuf->eQueuedPort,VPP_PORT_OUTPUT);

    return;
}

static void obd_di_buf_check(t_StVppBuf *pBuf)
{
    t_EVppBufType eType;
    uint32_t val, u32;

    u32 = u32VppBuf_GetFrameTypeExtradata(pBuf, VPP_EXTERNAL_EXTRADATA_TYPE, &eType);

    if (stTestCtx.u32OutRxCnt < stCtxHvx.u32ExpOBD)
    {
        if (stCtxHvx.au32BypSeq != NULL)
        {
            if (!stCtxHvx.au32BypSeq[stTestCtx.u32OutRxCnt])
            {
                DVP_ASSERT_EQUAL(eType, eVppBufType_Progressive);
                DVP_ASSERT_EQUAL(pBuf->eQueuedPort,VPP_PORT_OUTPUT);
            }
            else
            {
                DVP_ASSERT_EQUAL(pBuf->eQueuedPort,VPP_PORT_INPUT);
            }
        }
        else
        {
            DVP_ASSERT_EQUAL(eType, eVppBufType_Progressive);
            if (!stCtxHvx.u32BypassExpected)
                DVP_ASSERT_EQUAL(pBuf->eQueuedPort,VPP_PORT_OUTPUT);
        }

        if (stCtxHvx.au32OutFlgSeq != NULL)
        {
            DVP_ASSERT_TRUE(stTestCtx.u32OutRxCnt < stCtxHvx.u32ExpOBD);
            DVP_ASSERT_TRUE((pBuf->pBuf->flags & stCtxHvx.au32OutFlgSeq[stTestCtx.u32OutRxCnt]) ==
                    stCtxHvx.au32OutFlgSeq[stTestCtx.u32OutRxCnt]);
            LOGD("obd buf flags = %d", pBuf->pBuf->flags);
        }

        if (stCtxHvx.au32MarkerSeq != NULL)
        {
            get_extra_data_marker_int_buf(pBuf, &val);
            DVP_ASSERT_TRUE(stCtxHvx.au32MarkerSeq[stTestCtx.u32OutRxCnt] == val);
            LOGD("obd buf marker = %d", val);
        }
    }
    else
    {
        DVP_ASSERT_TRUE(pBuf->stPixel.u32FilledLen == 0);
    }

    return;
}

static void test_hvx_input_buffer_done(void *pv, t_StVppBuf *pBuf)
{
    LOGI("Running test_hvx_input_buffer_done");

    struct test_ctx *pCtx = (struct test_ctx*) pv;
    struct bufnode *pNode = pBuf->pBuf->cookie;
    uint32_t u32Ret;

    LOGI("IBD_owner:%d\n", pNode->port_owner);

    DVP_ASSERT_TRUE(pNode->owner == BUF_OWNER_LIBRARY);
    pNode->owner = BUF_OWNER_CLIENT;
    u32Ret = restore_bufnode_internal_buf(pNode);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    put_buf(pCtx->buf_pool, pNode);
    pCtx->u32InRxCnt += 1;
    LOGI("-> Done: %d IBDs processed\n", pCtx->u32InRxCnt);
    pthread_cond_signal(&pCtx->cond);
}

static void test_hvx_output_buffer_done(void *pv, t_StVppBuf *pBuf)
{
    LOGI("Running test_hvx_output_buffer_done");
    struct test_ctx *pCtx = (struct test_ctx*) pv;
    struct bufnode *pNode = pBuf->pBuf->cookie;
    uint32_t u32Ret;

    LOGI("INJECT_OBD:internal_buf_owner=%d\n", pBuf->eQueuedPort);

    DVP_ASSERT_TRUE(pNode->owner == BUF_OWNER_LIBRARY);
    pNode->owner = BUF_OWNER_CLIENT;

    LOGI("OBD_owner:%d\n", pNode->port_owner);

    if (pNode->pIntBuf->stPixel.u32FilledLen)
    {
        LOGI("Running dump_buf");
        dump_buf(pNode);
        DVP_ASSERT_EQUAL(pNode->pIntBuf->stExtra.u32FilledLen, stCtxHvx.u32ExpExtraLen);
        LOGI("extradata: expFillLen=%u, act_fill_len=%u", stCtxHvx.u32ExpExtraLen,
             pNode->pIntBuf->stExtra.u32FilledLen);
        validate_extradata_integrity(pNode);
    }

    // Run odb custom check callback
    if (stCtxHvx.pfnc_obd_cust_check != NULL)
        (*stCtxHvx.pfnc_obd_cust_check) (pBuf);

    u32Ret = restore_bufnode_internal_buf(pNode);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    put_buf(pCtx->buf_pool, pNode);
    pCtx->u32OutRxCnt += 1;
    LOGI("-> Done: %d OBDs processed\n", pCtx->u32OutRxCnt);
    pthread_cond_signal(&pCtx->cond);
}

static void test_hvx_event(void *pv, t_StVppEvt stEvt)
{
    LOGI("%s() got event: %u", __func__, stEvt.eType);
    struct test_ctx *pCtx = (struct test_ctx *) pv;
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

/************************************************************************
 * Helper Functions
 ***********************************************************************/
static void queue_single_buf(int port_owner, t_EVppBufType eBufType,
                             uint32_t buf_flags, uint32_t buf_fill,
                             uint32_t buf_marker)
{
    struct bufnode *pNode;
    uint32_t u32;

    // Run get_buf() to retrieve a buffer from the buffer pool
    pNode = get_buf(stTestCtx.buf_pool);
    if (pNode)
    {
        if (port_owner == VPP_PORT_INPUT)
        {
            if (buf_fill == VPP_TRUE)
                fill_buf(pNode);
            else
            {
                pNode->pIntBuf->pBuf->pixel.filled_len = 0;
                pNode->pIntBuf->stPixel.u32FilledLen = 0;
            }

            pNode->owner = BUF_OWNER_LIBRARY;

            // Only support Progressive, Interleaved TFF, and Interleaved BFF
            DVP_ASSERT_TRUE(eBufType == eVppBufType_Progressive ||
                            eBufType == eVppBufType_Interleaved_TFF ||
                            eBufType == eVppBufType_Interleaved_BFF);
            fill_extra_data(pNode, eBufType, buf_marker);
            pNode->pIntBuf->eBufType = eBufType;
            DVP_ASSERT_FALSE(buf_flags & ~cu32_valid_buf_flags);
            pNode->pIntBuf->pBuf->flags = buf_flags | pNode->pExtBuf->flags;
            stCtxHvx.u32ExpExtraLen = pNode->pExtBuf->extradata.filled_len;

            LOGI("Starting u32VppIpHvx_QueueBuf for VPP_PORT_INPUT");
        }
        else
        {
            pNode->owner = BUF_OWNER_LIBRARY;

            DVP_ASSERT_FALSE(stTestCtx.u32Flags & DRAIN_DONE);

            LOGI("Starting u32VppIpHvx_QueueBuf for VPP_PORT_OUTPUT");
        }
        // 8996 requires a mapped pixel buffer since register fd doesn't work for non-secure
        if (u32VppUtils_IsSoc(MSM8996) &&
            (stTestCtx.buf_pool->params.eProtection == PROTECTION_ZONE_NONSECURE))
            pNode->pIntBuf->stPixel.eMapping = eVppBuf_MappedInternal;
        else
            pNode->pIntBuf->stPixel.eMapping= eVppBuf_Unmapped;

        pNode->pIntBuf->eQueuedPort = port_owner;
        pNode->pIntBuf->eCurrentIpPort = port_owner;
        u32 = u32VppIpHvx_QueueBuf(stCtxHvx.pstHvxCb, port_owner, pNode->pIntBuf);
        DVP_ASSERT_EQUAL(u32, VPP_OK);
    }
    else
    {
        DVP_ASSERT_FAIL();
    }
}

static void set_default_hqv_control(struct hqv_control *ctrl, enum hqv_mode mode,
                                    enum hqv_control_type ctrl_type)
{
    if (ctrl == NULL)
    {
        LOGE("ctrl was null");
        DVP_ASSERT_FAIL();
        return;
    }

    ctrl->mode = mode;
    ctrl->ctrl_type = ctrl_type;

    // Exit early if not manual mode or auto mode
    if (ctrl->mode != HQV_MODE_MANUAL && ctrl->mode != HQV_MODE_AUTO)
    {
        ctrl->ctrl_type = HQV_CONTROL_NONE;
        return;
    }

    ctrl->ctrl_type = ctrl_type;
    switch(ctrl->ctrl_type)
    {
        case HQV_CONTROL_CADE:
            /*! Arg 1: Valid values: HQV_MODE_OFF, HQV_MODE_AUTO, HQV_MODE_MANUAL */
            /*! Arg 2: Range: 0 - 100 */
            /*! Arg 3: Range: (-50) - 50 */
            /*! Arg 4: Range: (-50) - 50 */
            SET_CTRL_CADE(ctrl->cade, HQV_MODE_MANUAL, 50, 0, 0);
            break;
        case HQV_CONTROL_CNR:
            /*! Arg 1: Valid values: HQV_MODE_OFF, HQV_MODE_AUTO, HQV_MODE_MANUAL */
            /*! Arg 2: Range: 0 - 100 */
            SET_CTRL_CNR(ctrl->cnr, HQV_MODE_MANUAL, 100);
            break;
        case HQV_CONTROL_AIE:
            /*! Arg 1: Valid values: HQV_MODE_OFF, HQV_MODE_AUTO, HQV_MODE_MANUAL */
            /*! Arg 2: Valid values: HQV_HUE_MODE_ON, HQV_HUE_MODE_OFF */
            /*! Arg 3: Range: 0 - 100 */
            /*! Arg 4: Range: 0 - 100 */
            /*! Arg 5: Range: 0 - 100 */
            /*! Arg 6: Range: 0 - 100 */
            /*! Arg 7: Range: 0 - 100 */
            /*! Arg 8: Range: 0 - 100 */
            SET_CTRL_AIE(ctrl->aie, HQV_MODE_MANUAL, HQV_HUE_MODE_OFF, 0, 30,
                         50, 50, 50, 50);
            break;
        case HQV_CONTROL_DI:
            /*! Arg 1: Valid values: HQV_DI_MODE_OFF, HQV_DI_MODE_VIDEO_1F, HQV_DI_MODE_VIDEO_3F, HQV_DI_MODE_AUTO */
            SET_CTRL_DI(ctrl->di, HQV_DI_MODE_AUTO);
            break;
        case HQV_CONTROL_GLOBAL_DEMO:
            /*! Arg 1: Range: 0 - 100 */
            /*! Arg 2: HQV_SPLIT_LEFT_TO_RIGHT, HQV_SPLIT_RIGHT_TO_LEFT,
            HQV_SPLIT_TOP_TO_BOTTOM, HQV_SPLIT_BOTTOM_TO_TOP, HQV_SPLIT_MAX */
            SET_CTRL_DEMO(ctrl->demo, 100, HQV_SPLIT_LEFT_TO_RIGHT);
            break;
        default:
            ctrl->ctrl_type = HQV_CONTROL_AIE;
            SET_CTRL_AIE(ctrl->aie, HQV_MODE_MANUAL, HQV_HUE_MODE_OFF, 0, 30,
                         50, 50, 50, 50);
            break;
    }
}

// Generalize the DI Tests using this helper
static void IpHvx_DI_Test(uint32 u32InBufCnt, t_EVppBufType* aeInBufSeq,
                          uint32* au32InBufFlags, uint32* au32FillSeq)
{
    uint32_t u32, i, buf_fill;
    struct hqv_control ctrl;

    SET_FILENAMES(ROTATETEST_INNAME, ROTATETEST_DI_OUTNAME, MAX_FILE_SEG_LEN);

    // Check config error
    DVP_ASSERT_TRUE(stCtxHvx.u32ExpOBD - stCtxHvx.u32DrainOBD >= 0);

    // 1. Run u32VppIpHvx_SetCtrl (see inside set_hqv_control for algo settings)
    set_default_hqv_control(&ctrl, HQV_MODE_MANUAL, HQV_CONTROL_DI);
    u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
    DVP_ASSERT_TRUE(u32 == VPP_OK);

    // 2. Running init_buf_pool
    uint32_t u32OutBufCnt, u32BufTotal;
    u32OutBufCnt = u32InBufCnt * 2; // 2 for every interleaved frame
    u32BufTotal = u32InBufCnt + u32OutBufCnt;
    u32 = init_buf_pool(&stTestCtx.stVppCtx, &stTestCtx.buf_pool,
                        &stTestCtx.params, u32BufTotal, VPP_FALSE);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 3. Running u32VppIpHvx_Open
    u32 = u32VppIpHvx_Open(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 4. Running queue_single_buf
    for (i = 0; i < u32InBufCnt; i++)
    {
        buf_fill = (au32FillSeq == NULL) ? VPP_TRUE : au32FillSeq[i];
        queue_single_buf(VPP_PORT_INPUT, aeInBufSeq[i], au32InBufFlags[i],
                         buf_fill, i);
        queue_single_buf(VPP_PORT_OUTPUT, eVppBufType_Max, 0, VPP_TRUE, 0);
        queue_single_buf(VPP_PORT_OUTPUT, eVppBufType_Max, 0, VPP_TRUE, 0);
    }

    // 5. Wait for the buffers to complete processing before continuing
    pthread_mutex_lock(&pstTestCtx->mutex);
    while (pstTestCtx->u32OutRxCnt < stCtxHvx.u32ExpOBD - stCtxHvx.u32DrainOBD)
        pthread_cond_wait(&pstTestCtx->cond, &pstTestCtx->mutex);
    pthread_mutex_unlock(&pstTestCtx->mutex);

    // 6. Call Drain if requested
    if (stCtxHvx.u32DrainOBD)
    {
        u32 = u32VppIpHvx_Drain(stCtxHvx.pstHvxCb);
        DVP_ASSERT_EQUAL(u32, VPP_OK);
        // Continue once drain completed
        while (!(stTestCtx.u32Flags & DRAIN_DONE))
            pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
        pthread_mutex_unlock(&stTestCtx.mutex);
        stTestCtx.u32Flags &= ~DRAIN_DONE;
    }

    // 7. Run u32VppIpHvx_Flush for both ports
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_INPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_OUTPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 8. Run u32VppIpHvx_Close
    u32 = u32VppIpHvx_Close(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 9. Running free_buf_pool
    free_buf_pool(stTestCtx.buf_pool, VPP_FALSE);
}

/************************************************************************
 * Test Functions
 ***********************************************************************/
TEST_SUITE_INIT(IpHvxSuiteInit)
{
    init_test_registry(TEST_SUITE_VPP_CLIENT);
}

TEST_SUITE_TERM(IpHvxSuiteTerm)
{
    term_test_registry();
}

TEST_SETUP(IpHvxTestInit)
{
    memset(&stCtxHvx, 0, sizeof(struct StHvxTestCtx));
    // Set default obd callback to use
    stCtxHvx.pfnc_obd_cust_check = &obd_buf_check;
    uint32_t u32Ret;
    u32Ret = tctx_common_init(pstTestCtx);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    // Set the callbacks
    stTestCtx.cb.input_buffer_done = test_hvx_input_buffer_done;
    stTestCtx.cb.output_buffer_done = test_hvx_output_buffer_done;
    stTestCtx.cb.event = test_hvx_event;
    stTestCtx.cb.pv = &stTestCtx;

    tctx_set_port_params(&stTestCtx, 1920, 1080, VPP_COLOR_FORMAT_NV12_VENUS);

    u32Ret = u32VppIon_Init(&stTestCtx.stVppCtx);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    stCtxHvx.pstHvxCb = vpVppIpHvx_Init(&stTestCtx.stVppCtx, 0, stTestCtx.cb);
    DVP_ASSERT_PTR_NNULL(stCtxHvx.pstHvxCb);

    u32Ret = u32VppIpHvx_SetParam(stCtxHvx.pstHvxCb, VPP_PORT_INPUT,
                                  stTestCtx.port_param);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
    u32Ret = u32VppIpHvx_SetParam(stCtxHvx.pstHvxCb, VPP_PORT_OUTPUT,
                                  stTestCtx.port_param);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    buf_params_init_default(&stTestCtx.params, &stTestCtx.port_param);
}

TEST_CLEANUP(IpHvxTestTerm)
{
    uint32_t u32Ret;

    vVppIpHvx_Term(stCtxHvx.pstHvxCb);

    u32Ret = u32VppIon_Term(&stTestCtx.stVppCtx);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    tctx_common_destroy(&stTestCtx);
}

TEST(IpHvx_InvalidControls)
{
    uint32_t u32;
    struct hqv_control ctrl = {};

    // Invalid mode
    ctrl.mode = HQV_MODE_MAX;
    u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
    DVP_ASSERT_NEQUAL(u32, VPP_OK);

    // Invalid ctrl type
    ctrl.mode = HQV_MODE_MANUAL;
    ctrl.ctrl_type = HQV_CONTROL_NONE;
    u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
    DVP_ASSERT_NEQUAL(u32, VPP_OK);

    // Global demo params
    {
        // Invalid percentage
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_GLOBAL_DEMO;
        ctrl.demo.process_direction = HQV_SPLIT_LEFT_TO_RIGHT;
        ctrl.demo.process_percent = 101;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);

        // Invalid direction
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_GLOBAL_DEMO;
        ctrl.demo.process_direction = HQV_SPLIT_MAX;
        ctrl.demo.process_percent = 50;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);
    }

    // AIE params
    if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_IE))
    {
        // Invalid aie mode
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_AIE;
        ctrl.aie.mode = HQV_MODE_MAX;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);

        // Invalid hue_mode
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_AIE;
        ctrl.aie.mode = HQV_MODE_MANUAL;
        ctrl.aie.hue_mode = HQV_HUE_MODE_MAX;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);

        // Invalid cade_level
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_AIE;
        ctrl.aie.mode = HQV_MODE_MANUAL;
        ctrl.aie.cade_level = 101;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);

        // Invalid ltm_sat_gain
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_AIE;
        ctrl.aie.mode = HQV_MODE_MANUAL;
        ctrl.aie.ltm_sat_gain = 101;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);

        // Invalid ltm_sat_offset
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_AIE;
        ctrl.aie.mode = HQV_MODE_MANUAL;
        ctrl.aie.ltm_sat_offset = 101;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);

        // Invalid ltm_ace_str
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_AIE;
        ctrl.aie.mode = HQV_MODE_MANUAL;
        ctrl.aie.ltm_ace_str = 101;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);

        // Invalid ltm_ace_bright_l
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_AIE;
        ctrl.aie.mode = HQV_MODE_MANUAL;
        ctrl.aie.ltm_ace_bright_l = 101;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);

        // Invalid ltm_ace_bright_h
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_AIE;
        ctrl.aie.mode = HQV_MODE_MANUAL;
        ctrl.aie.ltm_ace_bright_h = 101;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);
    }
    else
    {
        // Invalid AIE ctrl_type for target that doesn't support CR
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_AIE;
        ctrl.aie.mode = HQV_MODE_AUTO;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);
    }

    // CNR params
    if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_NR))
    {
        // Invalid cnr mode
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_CNR;
        ctrl.cnr.mode = HQV_MODE_MAX;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);

        // Invalid cnr level
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_CNR;
        ctrl.cnr.level = 101;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);
    }
    else
    {
        // Invalid CNR ctrl_type for target that doesn't support it
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_CNR;
        ctrl.cnr.mode = HQV_MODE_AUTO;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);
    }

    // DI params
    if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
    {
        // Invalid di mode
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_DI;
        ctrl.di.mode = HQV_DI_MODE_MAX;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);
    }
    else
    {
        // Invalid DI ctrl_type for target that doesn't support it
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.mode = HQV_MODE_MANUAL;
        ctrl.ctrl_type = HQV_CONTROL_DI;
        ctrl.di.mode = HQV_DI_MODE_AUTO;
        u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
        DVP_ASSERT_NEQUAL(u32, VPP_OK);
    }
}

TEST(IpHvx_AIE_BasicTestcase)
{
    uint32_t u32,i;
    struct hqv_control ctrl;
    stCtxHvx.u32BypassExpected = 0;

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_IE))
        return;
    SET_FILENAMES(ROTATETEST_INNAME, ROTATETEST_AIE_OUTNAME, MAX_FILE_SEG_LEN);

    // 1. Run u32VppIpHvx_SetCtrl (see inside set_hqv_control for algo settings)
    set_default_hqv_control(&ctrl, HQV_MODE_MANUAL,HQV_CONTROL_AIE);
    u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
    DVP_ASSERT_TRUE(u32 == VPP_OK);

    // 2. Running init_buf_pool
    uint32_t u32InBufCnt, u32OutBufCnt, u32BufTotal;
    u32InBufCnt = 5;
    u32OutBufCnt = u32InBufCnt; // In most cases, equal to u32InBufCnt
    u32BufTotal = u32InBufCnt + u32OutBufCnt;
    u32 = init_buf_pool(&stTestCtx.stVppCtx, &stTestCtx.buf_pool, &stTestCtx.params,
                        u32BufTotal, VPP_FALSE);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 3. Running u32VppIpHvx_Open
    u32 = u32VppIpHvx_Open(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 4. Running queue_single_buf
    for (i = 0; i < u32InBufCnt; i++)
    {
        if (i == u32InBufCnt - 1)
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive,
                             VPP_BUFFER_FLAG_EOS, VPP_TRUE, 0);
        else
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive, 0, VPP_TRUE, 0);
        queue_single_buf(VPP_PORT_OUTPUT, eVppBufType_Max, 0, VPP_TRUE, 0);
    }

    // 5. Wait for the buffers to complete processing before continuing
    pthread_mutex_lock(&pstTestCtx->mutex);
    while (pstTestCtx->u32OutRxCnt < u32OutBufCnt)
        pthread_cond_wait(&pstTestCtx->cond, &pstTestCtx->mutex);
    pthread_mutex_unlock(&pstTestCtx->mutex);

    // 6. Run u32VppIpHvx_Flush for both ports
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_INPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_OUTPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 7. Run u32VppIpHvx_Close
    u32 = u32VppIpHvx_Close(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 8. Running free_buf_pool
    free_buf_pool(stTestCtx.buf_pool, VPP_FALSE);
}

TEST(IpHvx_DI_TFF_BasicTestcase)
{
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF};
    uint32 au32InBufFlags[] = {0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_BFF_BasicTestcase)
{
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF};
    uint32 au32InBufFlags[] = {0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_BypassTestcase)
{
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Progressive,
        eVppBufType_Progressive,
        eVppBufType_Progressive,
        eVppBufType_Progressive,
        eVppBufType_Progressive};
    uint32 au32InBufFlags[] = {0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 5;
    stCtxHvx.u32BypassExpected = 1;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_FieldMixTestcase)
{
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_TFF};
    uint32 au32InBufFlags[] = {0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_ProgressiveMixTestcase)
{
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Progressive,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF};
    uint32 au32InBufFlags[] = {0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, 0, 0, 0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32BypSeq[] = {0, 0, 0, 0, 1, 0, 0, 0, 0};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 9;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.au32BypSeq = au32BypSeq;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_MultiMixTestcase)
{
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Progressive,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Progressive};
    uint32 au32InBufFlags[] = {0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, 0, 0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32BypSeq[] = {0, 0, 0, 0, 1, 0, 0, 1};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 8;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.au32BypSeq = au32BypSeq;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_TFF_FlagPropagationTestcase)
{
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF};
    uint32 au32InBufFlags[] = {0, VPP_BUFFER_FLAG_DATACORRUPT, 0,
        VPP_BUFFER_FLAG_SYNCFRAME, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {VPP_BUFFER_FLAG_DATACORRUPT, 0, 0, 0, 0,
        VPP_BUFFER_FLAG_SYNCFRAME, 0, 0, 0, VPP_BUFFER_FLAG_EOS};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_BFF_FlagPropagationTestcase)
{
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF};
    uint32 au32InBufFlags[] = {0, 0, VPP_BUFFER_FLAG_DATACORRUPT,
        VPP_BUFFER_FLAG_SYNCFRAME, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, VPP_BUFFER_FLAG_DATACORRUPT, 0, 0,
        0, VPP_BUFFER_FLAG_SYNCFRAME, 0, 0, VPP_BUFFER_FLAG_EOS};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_TFF_LoseIDRFlagPropagationTestcase)
{
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF};
    uint32 au32InBufFlags[] = {VPP_BUFFER_FLAG_DATACORRUPT, 0, VPP_BUFFER_FLAG_SYNCFRAME,
        VPP_BUFFER_FLAG_DATACORRUPT, VPP_BUFFER_FLAG_SYNCFRAME | VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {VPP_BUFFER_FLAG_DATACORRUPT, 0, 0, VPP_BUFFER_FLAG_DATACORRUPT,
        0, 0, 0, VPP_BUFFER_FLAG_SYNCFRAME, 0, VPP_BUFFER_FLAG_EOS};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_BFF_LoseIDRFlagPropagationTestcase)
{
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF};
    uint32 au32InBufFlags[] = {VPP_BUFFER_FLAG_DATACORRUPT, 0, VPP_BUFFER_FLAG_SYNCFRAME,
        VPP_BUFFER_FLAG_DATACORRUPT, VPP_BUFFER_FLAG_SYNCFRAME | VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {VPP_BUFFER_FLAG_DATACORRUPT, 0, 0, 0,
        VPP_BUFFER_FLAG_DATACORRUPT, 0, 0, 0, VPP_BUFFER_FLAG_SYNCFRAME, VPP_BUFFER_FLAG_EOS};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_TFF_DrainTestcase)
{
    // For the Drain Testcase we won't use an EOS
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF};
    uint32 au32InBufFlags[] = {VPP_BUFFER_FLAG_DATACORRUPT, 0, VPP_BUFFER_FLAG_SYNCFRAME,
        0, 0};
    uint32 au32OutBufFlags[] = {VPP_BUFFER_FLAG_DATACORRUPT, 0, 0, VPP_BUFFER_FLAG_SYNCFRAME,
        0, 0, 0, 0, 0, 0};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32DrainOBD = 4;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_BFF_DrainTestcase)
{
    // For the Drain Testcase we won't use an EOS
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF};
    uint32 au32InBufFlags[] = {VPP_BUFFER_FLAG_DATACORRUPT, 0, VPP_BUFFER_FLAG_SYNCFRAME,
        0, 0};
    uint32 au32OutBufFlags[] = {VPP_BUFFER_FLAG_DATACORRUPT, 0, 0, 0,
        VPP_BUFFER_FLAG_SYNCFRAME, 0, 0, 0, 0, 0};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32DrainOBD = 4;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_TFF_MidstreamEOSTestcase)
{
    // For the Drain Testcase we won't use an EOS
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF};
    uint32 au32InBufFlags[] = {0, 0, VPP_BUFFER_FLAG_EOS, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, 0, 0, 0, VPP_BUFFER_FLAG_EOS, 0, 0, 0,
        VPP_BUFFER_FLAG_EOS};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_BFF_MidstreamEOSTestcase)
{
    // For the Drain Testcase we won't use an EOS
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF};
    uint32 au32InBufFlags[] = {0, 0, VPP_BUFFER_FLAG_EOS, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, 0, 0, 0, VPP_BUFFER_FLAG_EOS, 0, 0, 0,
        VPP_BUFFER_FLAG_EOS};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_TFF_EmptyEOSTestcase)
{
    // For the Drain Testcase we won't use an EOS
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF};
    uint32 au32InBufFlags[] = {0, 0, VPP_BUFFER_FLAG_EOS, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, 0, 0, VPP_BUFFER_FLAG_EOS, 0, 0, 0,
        VPP_BUFFER_FLAG_EOS};
    uint32 au32FillBufSeq[] = {VPP_TRUE, VPP_TRUE, VPP_FALSE, VPP_TRUE, VPP_TRUE};
    uint32 au32BypSeq[] = {0, 0, 0, 0, 1, 0, 0, 0, 0};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 9;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.au32BypSeq = au32BypSeq;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, au32FillBufSeq);
}

TEST(IpHvx_DI_BFF_EmptyEOSTestcase)
{
    // For the Drain Testcase we won't use an EOS
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF};
    uint32 au32InBufFlags[] = {0, 0, VPP_BUFFER_FLAG_EOS, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, 0, 0, VPP_BUFFER_FLAG_EOS, 0, 0, 0,
        VPP_BUFFER_FLAG_EOS};
    uint32 au32FillBufSeq[] = {VPP_TRUE, VPP_TRUE, VPP_FALSE, VPP_TRUE, VPP_TRUE};
    uint32 au32BypSeq[] = {0, 0, 0, 0, 1, 0, 0, 0, 0};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 9;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.au32BypSeq = au32BypSeq;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, au32FillBufSeq);
}

TEST(IpHvx_DI_TFF_ExtraDataPropagationTestcase)
{
    // For the Drain Testcase we won't use an EOS
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF,
        eVppBufType_Interleaved_TFF};
    uint32 au32InBufFlags[] = {0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32MarkerSeq[] = {0, 1, 1, 2, 2, 3, 3, 4, 4, 4};
    uint32 au32BypSeq[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.au32MarkerSeq = au32MarkerSeq;
    stCtxHvx.au32BypSeq = au32BypSeq;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_DI_BFF_ExtraDataPropagationTestcase)
{
    // For the Drain Testcase we won't use an EOS
    uint32 u32InBufCnt = 5;
    t_EVppBufType aeInBufSeq[] = {
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF,
        eVppBufType_Interleaved_BFF};
    uint32 au32InBufFlags[] = {0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32OutBufFlags[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, VPP_BUFFER_FLAG_EOS};
    uint32 au32MarkerSeq[] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4};
    uint32 au32BypSeq[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        return;

    stCtxHvx.u32ExpOBD = 10;
    stCtxHvx.u32BypassExpected = 0;
    stCtxHvx.au32OutFlgSeq = au32OutBufFlags;
    stCtxHvx.au32MarkerSeq = au32MarkerSeq;
    stCtxHvx.au32BypSeq = au32BypSeq;
    stCtxHvx.pfnc_obd_cust_check = &obd_di_buf_check;

    IpHvx_DI_Test(u32InBufCnt, aeInBufSeq, au32InBufFlags, NULL);
}

TEST(IpHvx_Unsupported_BasicTestcase)
{
    uint32_t u32,i;
    struct hqv_control ctrl;
    stCtxHvx.u32BypassExpected = 1;

    SET_FILENAMES(ROTATETEST_INNAME, ROTATETEST_INVAL_OUTNAME, MAX_FILE_SEG_LEN);

    // 1. Run u32VppIpHvx_SetCtrl (see inside set_hqv_control for algo settings)
    set_default_hqv_control(&ctrl,HQV_MODE_MANUAL,HQV_CONTROL_CADE);
    u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
    DVP_ASSERT_TRUE(u32 != VPP_OK);

    // 2. Running init_buf_pool
    uint32_t u32InBufCnt, u32OutBufCnt, u32BufTotal;
    u32InBufCnt = 5;
    u32OutBufCnt = u32InBufCnt; // In most cases, equal to u32InBufCnt
    u32BufTotal = u32InBufCnt + u32OutBufCnt;
    u32 = init_buf_pool(&stTestCtx.stVppCtx, &stTestCtx.buf_pool, &stTestCtx.params,
                        u32BufTotal, VPP_FALSE);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 3. Running u32VppIpHvx_Open
    u32 = u32VppIpHvx_Open(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 4. Running queue_single_buf
    for (i = 0; i < u32InBufCnt; i++)
    {
        if (i == u32InBufCnt -1)
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive,
                             VPP_BUFFER_FLAG_EOS, VPP_TRUE, 0);
        else
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive, 0, VPP_TRUE, 0);
        queue_single_buf(VPP_PORT_OUTPUT, eVppBufType_Max, 0, VPP_TRUE, 0);
    }

    // 5. Wait for the buffers to complete processing before continuing
    pthread_mutex_lock(&pstTestCtx->mutex);
    while (pstTestCtx->u32OutRxCnt < u32OutBufCnt)
        pthread_cond_wait(&pstTestCtx->cond, &pstTestCtx->mutex);
    pthread_mutex_unlock(&pstTestCtx->mutex);

    // 6. Run u32VppIpHvx_Flush for both ports
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_INPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_OUTPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 7. Run u32VppIpHvx_Close
    u32 = u32VppIpHvx_Close(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 8. Running free_buf_pool
    free_buf_pool(stTestCtx.buf_pool, VPP_FALSE);
}

TEST(IpHvx_CNR_BasicTestcase)
{
    uint32_t u32,i;
    struct hqv_control ctrl;
    stCtxHvx.u32BypassExpected = 0;

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_NR))
        return;

    SET_FILENAMES(ROTATETEST_INNAME, ROTATETEST_CNR_OUTNAME, MAX_FILE_SEG_LEN);

    // 1. Run u32VppIpHvx_SetCtrl (see inside set_hqv_control for algo settings)
    set_default_hqv_control(&ctrl,HQV_MODE_MANUAL,HQV_CONTROL_CNR);
    u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
    DVP_ASSERT_TRUE(u32 == VPP_OK);

    // 2. Running init_buf_pool
    uint32_t u32InBufCnt, u32OutBufCnt, u32BufTotal;
    u32InBufCnt = 5;
    u32OutBufCnt = u32InBufCnt; // In most cases, equal to u32InBufCnt
    u32BufTotal = u32InBufCnt + u32OutBufCnt;
    u32 = init_buf_pool(&stTestCtx.stVppCtx, &stTestCtx.buf_pool, &stTestCtx.params,
                        u32BufTotal, VPP_FALSE);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 3. Running u32VppIpHvx_Open
    u32 = u32VppIpHvx_Open(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 4. Running queue_single_buf
    for (i = 0; i < u32InBufCnt; i++)
    {
        if (i == u32InBufCnt -1)
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive,
                             VPP_BUFFER_FLAG_EOS, VPP_TRUE, 0);
        else
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive, 0, VPP_TRUE, 0);
        queue_single_buf(VPP_PORT_OUTPUT, eVppBufType_Max, 0, VPP_TRUE, 0);
    }

    // 5. Wait for the buffers to complete processing before continuing
    pthread_mutex_lock(&pstTestCtx->mutex);
    while (pstTestCtx->u32OutRxCnt < u32OutBufCnt)
        pthread_cond_wait(&pstTestCtx->cond, &pstTestCtx->mutex);
    pthread_mutex_unlock(&pstTestCtx->mutex);

    // 6. Run u32VppIpHvx_Flush for both ports
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_INPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_OUTPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 7. Run u32VppIpHvx_Close
    u32 = u32VppIpHvx_Close(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 8. Running free_buf_pool
    free_buf_pool(stTestCtx.buf_pool, VPP_FALSE);
}

TEST(IpHvx_AUTO_BasicTestcase)
{
    uint32_t u32,i;
    struct hqv_control ctrl;
    stCtxHvx.u32BypassExpected = 0;

    SET_FILENAMES(ROTATETEST_INNAME, ROTATETEST_DEFAULT_OUTNAME, MAX_FILE_SEG_LEN);

    // 1. Run u32VppIpHvx_SetCtrl (see inside set_hqv_control for algo settings)
    set_default_hqv_control(&ctrl,HQV_MODE_AUTO,HQV_CONTROL_NONE);
    u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
    DVP_ASSERT_TRUE(u32 == VPP_OK);

    // 2. Running init_buf_pool
    uint32_t u32InBufCnt, u32OutBufCnt, u32BufTotal;
    u32InBufCnt = 5;
    u32OutBufCnt = u32InBufCnt; // In most cases, equal to u32InBufCnt
    u32BufTotal = u32InBufCnt + u32OutBufCnt;
    u32 = init_buf_pool(&stTestCtx.stVppCtx, &stTestCtx.buf_pool, &stTestCtx.params,
                        u32BufTotal, VPP_FALSE);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 3. Running u32VppIpHvx_Open
    u32 = u32VppIpHvx_Open(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 4. Running queue_single_buf
    for (i = 0; i < u32InBufCnt; i++)
    {
        if (i == u32InBufCnt -1)
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive,
                             VPP_BUFFER_FLAG_EOS, VPP_TRUE, 0);
        else
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive, 0, VPP_TRUE, 0);
        queue_single_buf(VPP_PORT_OUTPUT, eVppBufType_Max, 0, VPP_TRUE, 0);
    }

    // 5. Wait for the buffers to complete processing before continuing
    pthread_mutex_lock(&pstTestCtx->mutex);
    while (pstTestCtx->u32OutRxCnt < u32OutBufCnt)
        pthread_cond_wait(&pstTestCtx->cond, &pstTestCtx->mutex);
    pthread_mutex_unlock(&pstTestCtx->mutex);

    // 6. Run u32VppIpHvx_Flush for both ports
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_INPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_OUTPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 7. Run u32VppIpHvx_Close
    u32 = u32VppIpHvx_Close(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 8. Running free_buf_pool
    free_buf_pool(stTestCtx.buf_pool, VPP_FALSE);
}

TEST(IpHvx_MultiAlgo_BasicTestcase)
{
    uint32_t u32,i;
    struct hqv_control ctrl;
    stCtxHvx.u32BypassExpected = 0;

    SET_FILENAMES(ROTATETEST_INNAME, ROTATETEST_CNR_AIE_OUTNAME, MAX_FILE_SEG_LEN);

    if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_NR) ||
        !bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_IE))
        return;

    // 1. Run u32VppIpHvx_SetCtrl (see inside set_hqv_control for algo settings)
    set_default_hqv_control(&ctrl,HQV_MODE_MANUAL,HQV_CONTROL_CNR);
    u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
    DVP_ASSERT_TRUE(u32 == VPP_OK);
    set_default_hqv_control(&ctrl,HQV_MODE_MANUAL,HQV_CONTROL_AIE);
    u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
    DVP_ASSERT_TRUE(u32 == VPP_OK);

    // 2. Running init_buf_pool
    uint32_t u32InBufCnt, u32OutBufCnt, u32BufTotal;
    u32InBufCnt = 5;
    u32OutBufCnt = u32InBufCnt; // In most cases, equal to u32InBufCnt
    u32BufTotal = u32InBufCnt + u32OutBufCnt;
    u32 = init_buf_pool(&stTestCtx.stVppCtx, &stTestCtx.buf_pool, &stTestCtx.params,
                        u32BufTotal, VPP_FALSE);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 3. Running u32VppIpHvx_Open
    u32 = u32VppIpHvx_Open(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 4. Running queue_single_buf
    for (i = 0; i < u32InBufCnt; i++)
    {
        if (i == u32InBufCnt -1)
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive,
                             VPP_BUFFER_FLAG_EOS, VPP_TRUE, 0);
        else
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive, 0, VPP_TRUE, 0);
        queue_single_buf(VPP_PORT_OUTPUT, eVppBufType_Max, 0, VPP_TRUE, 0);
    }

    // 5. Wait for the buffers to complete processing before continuing
    pthread_mutex_lock(&pstTestCtx->mutex);
    while (pstTestCtx->u32OutRxCnt < u32OutBufCnt)
        pthread_cond_wait(&pstTestCtx->cond, &pstTestCtx->mutex);
    pthread_mutex_unlock(&pstTestCtx->mutex);

    // 6. Run u32VppIpHvx_Flush for both ports
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_INPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_OUTPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 7. Run u32VppIpHvx_Close
    u32 = u32VppIpHvx_Close(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 8. Running free_buf_pool
    free_buf_pool(stTestCtx.buf_pool, VPP_FALSE);
}

TEST(IpHvx_ValidReconfig_BasicTestcase)
{
    uint32_t u32,i;
    struct hqv_control ctrl;
    stCtxHvx.u32BypassExpected = 0;

    // --- Original run ---
    SET_FILENAMES(ROTATETEST_INNAME, ROTATETEST_AIE_VALRECONFIG_OUTNAME, MAX_FILE_SEG_LEN);

    // 1. Run u32VppIpHvx_SetCtrl (see inside set_hqv_control for algo settings)
    set_default_hqv_control(&ctrl,HQV_MODE_AUTO, HQV_CONTROL_NONE);
    u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
    DVP_ASSERT_TRUE(u32 == VPP_OK);

    // 2. Running init_buf_pool
    uint32_t u32InBufCnt, u32OutBufCnt, u32BufTotal;
    u32InBufCnt = 2;
    u32OutBufCnt = u32InBufCnt; // In most cases, equal to u32InBufCnt
    u32BufTotal = u32InBufCnt + u32OutBufCnt;
    u32 = init_buf_pool(&stTestCtx.stVppCtx, &stTestCtx.buf_pool, &stTestCtx.params,
                        u32BufTotal, VPP_FALSE);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 3. Running u32VppIpHvx_Open
    u32 = u32VppIpHvx_Open(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 4. Running queue_single_buf
    for (i = 0; i < u32InBufCnt; i++)
    {
        if (i == u32InBufCnt -1)
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive,
                             VPP_BUFFER_FLAG_EOS, VPP_TRUE, 0);
        else
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive, 0, VPP_TRUE, 0);
        queue_single_buf(VPP_PORT_OUTPUT, eVppBufType_Max, 0, VPP_TRUE, 0);
    }

    // 5. Running u32VppIpHvx_Drain
    u32 = u32VppIpHvx_Drain(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    // Continue once drain completed
    while (!(stTestCtx.u32Flags & DRAIN_DONE))
        pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
    pthread_mutex_unlock(&stTestCtx.mutex);
    stTestCtx.u32Flags &= ~DRAIN_DONE;

    // 6. Run u32VppIpHvx_Close
    u32 = u32VppIpHvx_Close(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 7. Running free_buf_pool
    free_buf_pool(stTestCtx.buf_pool, VPP_FALSE);

    // --- Reconfigure ---
    // 1. Change the parameters
    SET_FILENAMES(ROTATETEST2_INNAME, ROTATETEST2_AIEVALID_OUTNAME, MAX_FILE_SEG_LEN);
    tctx_set_port_params(pstTestCtx, 720, 480, VPP_COLOR_FORMAT_NV12_VENUS);
    u32InBufCnt = 2;
    u32OutBufCnt = u32InBufCnt; // In most cases, equal to u32InBufCnt
    u32BufTotal = u32InBufCnt + u32OutBufCnt;
    u32 = init_buf_pool(&stTestCtx.stVppCtx, &stTestCtx.buf_pool, &stTestCtx.params,
                        u32BufTotal, VPP_FALSE);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    u32 = u32VppIpHvx_SetParam(stCtxHvx.pstHvxCb, VPP_PORT_INPUT,
                               stTestCtx.port_param);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_SetParam(stCtxHvx.pstHvxCb, VPP_PORT_OUTPUT,
                               stTestCtx.port_param);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 2. Running u32VppIpHvx_Open
    u32 = u32VppIpHvx_Open(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 3. Continue queue_single_buf
    for (i = 0; i < u32InBufCnt; i++)
    {
        if (i == u32InBufCnt -1)
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive,
                             VPP_BUFFER_FLAG_EOS, VPP_TRUE, 0);
        else
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive, 0, VPP_TRUE, 0);
        queue_single_buf(VPP_PORT_OUTPUT, eVppBufType_Max, 0, VPP_TRUE, 0);
    }

    // 4. Wait for the buffers to complete processing before continuing
    pthread_mutex_lock(&pstTestCtx->mutex);
    while (pstTestCtx->u32OutRxCnt < u32OutBufCnt)
        pthread_cond_wait(&pstTestCtx->cond, &pstTestCtx->mutex);
    pthread_mutex_unlock(&pstTestCtx->mutex);

    // 5. Run u32VppIpHvx_Flush for both ports
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_INPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_OUTPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 6. Run u32VppIpHvx_Close
    u32 = u32VppIpHvx_Close(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 7. Running free_buf_pool
    free_buf_pool(stTestCtx.buf_pool, VPP_FALSE);
}

TEST(IpHvx_MemTooSmallReconfig_BasicTestcase)
{
    uint32_t u32,i;
    struct hqv_control ctrl;
    stCtxHvx.u32BypassExpected = 1;

    // --- Original run ---
    SET_FILENAMES(ROTATETEST_INNAME, ROTATETEST_AIE_INVALRECONFIG_OUTNAME, MAX_FILE_SEG_LEN);

    // 1. Run u32VppIpHvx_SetCtrl (see inside set_hqv_control for algo settings)
    set_default_hqv_control(&ctrl,HQV_MODE_AUTO, HQV_CONTROL_NONE);
    u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
    DVP_ASSERT_TRUE(u32 == VPP_OK);

    // 2. Running init_buf_pool
    uint32_t u32InBufCnt, u32OutBufCnt, u32BufTotal;
    u32InBufCnt = 2;
    u32OutBufCnt = u32InBufCnt; // In most cases, equal to u32InBufCnt
    u32BufTotal = u32InBufCnt + u32OutBufCnt;
    u32 = init_buf_pool(&stTestCtx.stVppCtx, &stTestCtx.buf_pool, &stTestCtx.params,
                        u32BufTotal, VPP_FALSE);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 3. Running u32VppIpHvx_Open
    u32 = u32VppIpHvx_Open(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 4. Running queue_single_buf
    for (i = 0; i < u32InBufCnt; i++)
    {
        if (i == u32InBufCnt -1)
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive,
                             VPP_BUFFER_FLAG_EOS, VPP_TRUE, 0);
        else
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive, 0, VPP_TRUE, 0);
        queue_single_buf(VPP_PORT_OUTPUT, eVppBufType_Max, 0, VPP_TRUE, 0);
    }

    // 5. Running u32VppIpHvx_Drain
    u32 = u32VppIpHvx_Drain(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    // Continue once drain completed
    while (!(stTestCtx.u32Flags & DRAIN_DONE))
        pthread_cond_wait(&stTestCtx.cond, &stTestCtx.mutex);
    pthread_mutex_unlock(&stTestCtx.mutex);
    stTestCtx.u32Flags &= ~DRAIN_DONE;

    // 6. Run u32VppIpHvx_Close
    u32 = u32VppIpHvx_Close(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 7. Running free_buf_pool
    free_buf_pool(stTestCtx.buf_pool, VPP_FALSE);

    // --- Reconfigure ---
    // 1. Change the parameters
    SET_FILENAMES(ROTATETEST2_INNAME, ROTATETEST2_AIEINVALID_OUTNAME, MAX_FILE_SEG_LEN);
    // Allocating memory for only 2x2 frames
    SET_BUF_PARAMS(2,2,VPP_COLOR_FORMAT_NV12_VENUS);
    // HVX to process for 720x480 frames
    tctx_set_port_params(pstTestCtx, 720, 480, VPP_COLOR_FORMAT_NV12_VENUS);
    u32InBufCnt = 2;
    u32OutBufCnt = u32InBufCnt; // In most cases, equal to u32InBufCnt
    u32BufTotal = u32InBufCnt + u32OutBufCnt;
    u32 = init_buf_pool(&stTestCtx.stVppCtx, &stTestCtx.buf_pool, &stTestCtx.params,
                        u32BufTotal, VPP_FALSE);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    u32 = u32VppIpHvx_SetParam(stCtxHvx.pstHvxCb, VPP_PORT_INPUT,
                               stTestCtx.port_param);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_SetParam(stCtxHvx.pstHvxCb, VPP_PORT_OUTPUT,
                               stTestCtx.port_param);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 2. Running u32VppIpHvx_Open
    u32 = u32VppIpHvx_Open(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 3. Continue queue_single_buf
    for (i = 0; i < u32InBufCnt; i++)
    {
        if (i == u32InBufCnt -1)
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive,
                             VPP_BUFFER_FLAG_EOS, VPP_TRUE, 0);
        else
            queue_single_buf(VPP_PORT_INPUT, eVppBufType_Progressive, 0, VPP_TRUE, 0);
        queue_single_buf(VPP_PORT_OUTPUT, eVppBufType_Max, 0, VPP_TRUE, 0);
    }

    // 4. Wait for the buffers to complete processing before continuing
    pthread_mutex_lock(&pstTestCtx->mutex);
    while (pstTestCtx->u32OutRxCnt < u32OutBufCnt)
        pthread_cond_wait(&pstTestCtx->cond, &pstTestCtx->mutex);
    pthread_mutex_unlock(&pstTestCtx->mutex);

    // 5. Run u32VppIpHvx_Flush for both ports
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_INPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Flush(stCtxHvx.pstHvxCb, VPP_PORT_OUTPUT);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 6. Run u32VppIpHvx_Close
    u32 = u32VppIpHvx_Close(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // 7. Running free_buf_pool
    free_buf_pool(stTestCtx.buf_pool, VPP_FALSE);
}

TEST(IpHvx_StartSequence)
{
    struct hqv_control ctrl;
    uint32_t u32;

    // Open without boot
    set_default_hqv_control(&ctrl, HQV_MODE_AUTO, HQV_CONTROL_NONE);
    u32 = u32VppIpHvx_SetCtrl(stCtxHvx.pstHvxCb, ctrl);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Open(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Close(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // Open after boot
    u32 = u32VppIpHvx_Boot();
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Open(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Close(stCtxHvx.pstHvxCb);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    u32 = u32VppIpHvx_Shutdown();
    DVP_ASSERT_EQUAL(u32, VPP_OK);
}

TEST(IpHvx_BasicBootShutdown)
{
    uint32_t u32;

    u32 = u32VppIpHvx_Boot();
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    u32 = u32VppIpHvx_Shutdown();
    DVP_ASSERT_EQUAL(u32, VPP_OK);
}

TEST(IpHvx_RepeatedBootShutdown)
{
    uint32_t u32, i;

    for (i = 0; i < 50; i++)
    {
        LOGI("%s, i=%u", __func__, i);

        u32 = u32VppIpHvx_Boot();
        DVP_ASSERT_EQUAL(u32, VPP_OK);

        u32 = u32VppIpHvx_Shutdown();
        DVP_ASSERT_EQUAL(u32, VPP_OK);
    }
}

TEST(IpHvx_RepeatedBoot)
{
    uint32_t u32, i;

    for (i = 0; i < 20; i++)
    {
        LOGI("%s, i=%u", __func__, i);

        // Boot currently only boots tunings and is expected to always return success unless
        // there is a fatal error since only certain calls are skipped if previously booted.
        u32 = u32VppIpHvx_Boot();
        DVP_ASSERT_EQUAL(u32, VPP_OK);
    }

    u32 = u32VppIpHvx_Shutdown();
    DVP_ASSERT_EQUAL(u32, VPP_OK);
}

TEST(IpHvx_RepeatedShutdown)
{
    uint32_t u32, i;

    u32 = u32VppIpHvx_Boot();
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    for (i = 0; i < 20; i++)
    {
        LOGI("%s, i=%u", __func__, i);
        u32 = u32VppIpHvx_Shutdown();

        if (!i)
            DVP_ASSERT_EQUAL(u32, VPP_OK);
        else
            DVP_ASSERT_NEQUAL(u32, VPP_OK);
    }
}
/************************************************************************
 * Global Functions
 ***********************************************************************/
TEST_CASES IpHvxTests[] = {
    TEST_CASE(IpHvx_InvalidControls),
    TEST_CASE(IpHvx_CNR_BasicTestcase),
    TEST_CASE(IpHvx_AUTO_BasicTestcase),
    TEST_CASE(IpHvx_AIE_BasicTestcase),
    TEST_CASE(IpHvx_DI_TFF_BasicTestcase),
    TEST_CASE(IpHvx_DI_BFF_BasicTestcase),
    TEST_CASE(IpHvx_DI_BypassTestcase),
    TEST_CASE(IpHvx_DI_FieldMixTestcase),
    TEST_CASE(IpHvx_DI_ProgressiveMixTestcase),
    TEST_CASE(IpHvx_DI_MultiMixTestcase),
    TEST_CASE(IpHvx_DI_TFF_FlagPropagationTestcase),
    TEST_CASE(IpHvx_DI_BFF_FlagPropagationTestcase),
    TEST_CASE(IpHvx_DI_TFF_LoseIDRFlagPropagationTestcase),
    TEST_CASE(IpHvx_DI_BFF_LoseIDRFlagPropagationTestcase),
    TEST_CASE(IpHvx_DI_TFF_DrainTestcase),
    TEST_CASE(IpHvx_DI_BFF_DrainTestcase),
    TEST_CASE(IpHvx_DI_TFF_MidstreamEOSTestcase),
    TEST_CASE(IpHvx_DI_BFF_MidstreamEOSTestcase),
    TEST_CASE(IpHvx_DI_TFF_EmptyEOSTestcase),
    TEST_CASE(IpHvx_DI_BFF_EmptyEOSTestcase),
    TEST_CASE(IpHvx_DI_TFF_ExtraDataPropagationTestcase),
    TEST_CASE(IpHvx_DI_BFF_ExtraDataPropagationTestcase),
    TEST_CASE(IpHvx_Unsupported_BasicTestcase),
    TEST_CASE(IpHvx_MultiAlgo_BasicTestcase),
    TEST_CASE(IpHvx_ValidReconfig_BasicTestcase),
    TEST_CASE(IpHvx_MemTooSmallReconfig_BasicTestcase),
    TEST_CASE(IpHvx_StartSequence),
    TEST_CASE(IpHvx_BasicBootShutdown),
    TEST_CASE(IpHvx_RepeatedBootShutdown),
    TEST_CASE(IpHvx_RepeatedBoot),
    TEST_CASE(IpHvx_RepeatedShutdown),

    TEST_CASE_NULL(),
};

TEST_SUITE(IpHvxSuite,
           "IpHvxTests",
           IpHvxSuiteInit,
           IpHvxSuiteTerm,
           IpHvxTestInit,
           IpHvxTestTerm,
           IpHvxTests);
