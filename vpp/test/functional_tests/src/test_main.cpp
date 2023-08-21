/*!
 * @file test_main.c
 *
 * @cr
 * Copyright (c) 2015-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>

#include "dvpTest.h"
#include "dvpTest_tb.h"
#include "test_utils.h"
#include "buf_pool.h"
#include "vpp_utils.h"
#include "vpp_buf.h"

#define VPP_LOG_TAG     VPP_LOG_FT_MAIN_TAG
#define VPP_LOG_MODULE  VPP_LOG_FT_MAIN
#include "vpp_dbg.h"
#ifdef VPP_SERVICE
    #include <hidl/LegacySupport.h>
    using android::hardware::configureRpcThreadpool;
    using android::hardware::joinRpcThreadpool;
#endif
/************************************************************************
 * Local definitions
 ***********************************************************************/
struct test_ctx stTestCtx;
struct test_ctx *pstTestCtx = &stTestCtx;
struct functional_test_ctx FuncTestCtx;

#define VPP_LOG_LVL_BUFFER_EXCHANGE     VPP_LOG_LVL_INFO
#define VPP_LOG_LVL_CONCURRENCY         VPP_LOG_LVL_INFO
#define VPP_LOG_LVL_MAIN                VPP_LOG_LVL_INFO

#define VPP_LOG(which) (VPP_LOG_LVL_##which << (VPP_LOG_FLD_WID * VPP_LOG_FT_##which))

enum
{
    PARAM_TESTS = 1,
    PARAM_IN_PATH,
    PARAM_OUT_PATH,
    PARAM_IN_FILE_NAME,
    PARAM_OUT_FILE_NAME,
    PARAM_IN_FILE_TYPE,
    PARAM_OUT_FILE_TYPE,
    PARAM_IN_PIX_FMT,
    PARAM_OUT_PIX_FMT,
    PARAM_IN_BUF_TYPE,
    PARAM_IN_FRAME_WIDTH,
    PARAM_IN_FRAME_HEIGHT,
    PARAM_OUT_FRAME_WIDTH,
    PARAM_OUT_FRAME_HEIGHT,
    PARAM_IN_NUM_FRAMES,
    PARAM_OUT_NUM_FRAMES,
    PARAM_HQV_MODE,
    PARAM_CADE_MODE,
    PARAM_CADE_LEVEL,
    PARAM_CADE_CONTRAST_LEVEL,
    PARAM_CADE_SATURATION_LEVEL,
    PARAM_AIE_MODE,
    PARAM_AIE_HUE_MODE,
    PARAM_AIE_CADE_LEVEL,
    PARAM_AIE_LTM_LEVEL,
    PARAM_AIE_LTM_SAT_GAIN,
    PARAM_AIE_LTM_SAT_OFFSET,
    PARAM_AIE_LTM_ACE_STR,
    PARAM_AIE_LTM_ACE_BRIL,
    PARAM_AIE_LTM_ACE_BRIH,
    PARAM_QBR_MODE,
    PARAM_EAR_MODE,
    PARAM_MEAS_ENABLE,
    PARAM_DI_MODE,
    PARAM_TNR_MODE,
    PARAM_TNR_LEVEL,
    PARAM_CNR_MODE,
    PARAM_CNR_LEVEL,
    PARAM_FRC_SEGMENT,
    PARAM_FRC_MODE,
    PARAM_FRC_LEVEL,
    PARAM_FRC_INTERP,
    PARAM_FRC_TIMESTAMP_START,
    PARAM_FRC_FRM_CP_ON_FALLBACK,
    PARAM_FRC_FRM_CP_INPUT,
    PARAM_GLOBAL_SPLITSCREEN_PERCENT,
    PARAM_GLOBAL_SPLITSCREEN_DIRECTION,
    PARAM_NUM_RECONFIG_STREAMS,
    PARAM_RECONFIG0_IN_NAME,
    PARAM_RECONFIG0_OUT_NAME,
    PARAM_RECONFIG0_IN_WIDTH,
    PARAM_RECONFIG0_IN_HEIGHT,
    PARAM_RECONFIG0_IN_FILE_TYPE,
    PARAM_RECONFIG0_OUT_FILE_TYPE,
    PARAM_RECONFIG0_IN_PIX_FMT,
    PARAM_RECONFIG0_OUT_PIX_FMT,
    PARAM_RECONFIG0_IN_BUF_TYPE,
    PARAM_RECONFIG1_IN_NAME,
    PARAM_RECONFIG1_OUT_NAME,
    PARAM_RECONFIG1_IN_WIDTH,
    PARAM_RECONFIG1_IN_HEIGHT,
    PARAM_RECONFIG1_IN_FILE_TYPE,
    PARAM_RECONFIG1_OUT_FILE_TYPE,
    PARAM_RECONFIG1_IN_PIX_FMT,
    PARAM_RECONFIG1_OUT_PIX_FMT,
    PARAM_RECONFIG1_IN_BUF_TYPE,
    PARAM_RECONFIG2_IN_NAME,
    PARAM_RECONFIG2_OUT_NAME,
    PARAM_RECONFIG2_IN_WIDTH,
    PARAM_RECONFIG2_IN_HEIGHT,
    PARAM_RECONFIG2_IN_FILE_TYPE,
    PARAM_RECONFIG2_OUT_FILE_TYPE,
    PARAM_RECONFIG2_IN_PIX_FMT,
    PARAM_RECONFIG2_OUT_PIX_FMT,
    PARAM_RECONFIG2_IN_BUF_TYPE,
    PARAM_RECONFIG3_IN_NAME,
    PARAM_RECONFIG3_OUT_NAME,
    PARAM_RECONFIG3_IN_WIDTH,
    PARAM_RECONFIG3_IN_HEIGHT,
    PARAM_RECONFIG3_IN_FILE_TYPE,
    PARAM_RECONFIG3_OUT_FILE_TYPE,
    PARAM_RECONFIG3_IN_PIX_FMT,
    PARAM_RECONFIG3_OUT_PIX_FMT,
    PARAM_RECONFIG3_IN_BUF_TYPE,
    PARAM_RECONFIG4_IN_NAME,
    PARAM_RECONFIG4_OUT_NAME,
    PARAM_RECONFIG4_IN_WIDTH,
    PARAM_RECONFIG4_IN_HEIGHT,
    PARAM_RECONFIG4_IN_FILE_TYPE,
    PARAM_RECONFIG4_OUT_FILE_TYPE,
    PARAM_RECONFIG4_IN_PIX_FMT,
    PARAM_RECONFIG4_OUT_PIX_FMT,
    PARAM_RECONFIG4_IN_BUF_TYPE,
    PARAM_SECURE,
    PARAM_DISABLE_GRALLOC,
    PARAM_DISABLE_EXTRA_DATA,
    PARAM_META_DATA_FILE_NAME,
};
/************************************************************************
 * Local static variables
 ***********************************************************************/
uint64_t u64LogLevelFunc = VPP_LOG(BUFFER_EXCHANGE) | VPP_LOG(CONCURRENCY) |
                           VPP_LOG(MAIN);

/************************************************************************
 * Forward Declarations
 ************************************************************************/
TEST_SUITE_DECLARE(BufferExchangeSuite);
TEST_SUITE_DECLARE(ConcurrencySuite);

/************************************************************************
 * Local Functions
 ***********************************************************************/

static uint32_t u32ConvertParams(char *cpInput, uint32_t *u32pOutput)
{
    *u32pOutput = 0;
    uint32_t u32Ret = VPP_OK;

    /* NV12 */
    if(strcasecmp(cpInput, "0") == 0)
    {
        *u32pOutput = 0;
    }
    else if(strcasecmp(cpInput, "1") == 0)
    {
        *u32pOutput = 1;
    }
    else if(strcasecmp(cpInput, "2") == 0)
    {
        *u32pOutput = 2;
    }
    else if(strcasecmp(cpInput, "3") == 0)
    {
        *u32pOutput = 3;
    }
    else if(strcasecmp(cpInput, "NV12") == 0)
    {
        *u32pOutput = FILE_FORMAT_NV12;
    }
    else if(strcasecmp(cpInput, "P010") == 0)
    {
        *u32pOutput = FILE_FORMAT_P010;
    }
    else if(strcasecmp(cpInput, "NV12_UBWC") == 0)
    {
        *u32pOutput = FILE_FORMAT_NV12_UBWC;
    }
    else if(strcasecmp(cpInput, "TP10_UBWC") == 0)
    {
        *u32pOutput = FILE_FORMAT_TP10_UBWC;
    }
    else if(strcasecmp(cpInput, "RGBA8") == 0)
    {
        *u32pOutput = FILE_FORMAT_RGBA8;
    }
    else if(strcasecmp(cpInput, "BGRA8") == 0)
    {
        *u32pOutput = FILE_FORMAT_BGRA8;
    }
    else if(strcasecmp(cpInput, "RGBA8_UBWC") == 0)
    {
        *u32pOutput = FILE_FORMAT_RGBA8_UBWC;
    }
    else if(strcasecmp(cpInput, "BGRA8_UBWC") == 0)
    {
        *u32pOutput = FILE_FORMAT_BGRA8_UBWC;
    }
    else if(strcasecmp(cpInput, "RGB565_UBWC") == 0)
    {
        *u32pOutput = FILE_FORMAT_RGB565_UBWC;
    }
    else if(strcasecmp(cpInput, "BGR565_UBWC") == 0)
    {
        *u32pOutput = FILE_FORMAT_BGR565_UBWC;
    }
    else if((strcasecmp(cpInput, "singleframe") == 0) || (strcasecmp(cpInput, "single") == 0))
    {
        *u32pOutput = FILE_TYPE_SINGLE_FRAMES;
    }
    else if((strcasecmp(cpInput, "multiframe") == 0) || (strcasecmp(cpInput, "multi") == 0))
    {
        *u32pOutput = FILE_TYPE_MULTI_FRAMES;
    }
    else if(strcasecmp(cpInput, "none") == 0)
    {
        *u32pOutput = FILE_TYPE_NONE;
    }
    else if(strcasecmp(cpInput, "off") == 0)
    {
        *u32pOutput = HQV_MODE_OFF;
    }
    else if(strcasecmp(cpInput, "on") == 0)
    {
        *u32pOutput = 1;
    }
    else if((strcasecmp(cpInput, "low") == 0) || (strcasecmp(cpInput, "frc_low") == 0))
    {
        *u32pOutput = HQV_FRC_LEVEL_LOW;
    }
    else if((strcasecmp(cpInput, "med") == 0) || (strcasecmp(cpInput, "frc_med") == 0))
    {
        *u32pOutput = HQV_FRC_LEVEL_MED;
    }
    else if((strcasecmp(cpInput, "high") == 0) || (strcasecmp(cpInput, "frc_high") == 0))
    {
        *u32pOutput = HQV_FRC_LEVEL_HIGH;
    }
    else if(strcasecmp(cpInput, "smooth") == 0)
    {
        *u32pOutput = HQV_FRC_MODE_SMOOTH_MOTION;
    }
    else if(strcasecmp(cpInput, "slomo") == 0)
    {
        *u32pOutput = HQV_FRC_MODE_SLOMO;
    }
    else if(strcasecmp(cpInput, "1x") == 0)
    {
        *u32pOutput = HQV_FRC_INTERP_1X;
    }
    else if(strcasecmp(cpInput, "2x") == 0)
    {
        *u32pOutput = HQV_FRC_INTERP_2X;
    }
    else if(strcasecmp(cpInput, "3x") == 0)
    {
        *u32pOutput = HQV_FRC_INTERP_3X;
    }
    else if(strcasecmp(cpInput, "4x") == 0)
    {
        *u32pOutput = HQV_FRC_INTERP_4X;
    }
    else if(strcasecmp(cpInput, "auto") == 0)
    {
        *u32pOutput = HQV_MODE_AUTO;
    }
    else if((strcasecmp(cpInput, "man") == 0) || (strcasecmp(cpInput, "manual") == 0))
    {
        *u32pOutput = HQV_MODE_MANUAL;
    }
    else if(strcasecmp(cpInput, "1f") == 0)
    {
        *u32pOutput = HQV_DI_MODE_VIDEO_1F;
    }
    else if(strcasecmp(cpInput, "3f") == 0)
    {
        *u32pOutput = HQV_DI_MODE_VIDEO_3F;
    }
    else if(strcasecmp(cpInput, "di_auto") == 0)
    {
        *u32pOutput = HQV_DI_MODE_AUTO;
    }
    else if (strcasecmp(cpInput, "ear_bypass") == 0)
    {
        *u32pOutput = HQV_EAR_MODE_BYPASS;
    }
    else if (strcasecmp(cpInput, "ear_low") == 0)
    {
        *u32pOutput = HQV_EAR_MODE_LOW;
    }
    else if (strcasecmp(cpInput, "ear_med") == 0)
    {
        *u32pOutput = HQV_EAR_MODE_MEDIUM;
    }
    else if (strcasecmp(cpInput, "ear_high") == 0)
    {
        *u32pOutput = HQV_EAR_MODE_HIGH;
    }
    else if ((strcasecmp(cpInput, "ear_adaptive_stream") == 0) ||
             (strcasecmp(cpInput, "ear_stream") == 0))
    {
        *u32pOutput = HQV_EAR_MODE_STREAM_ADAPTIVE;
    }
    else if ((strcasecmp(cpInput, "ear_adaptive_frame") == 0) ||
             (strcasecmp(cpInput, "ear_frame") == 0))
    {
        *u32pOutput = HQV_EAR_MODE_FRAME_ADAPTIVE;
    }
    else if((strcasecmp(cpInput, "prog") == 0) || (strcasecmp(cpInput, "progressive") == 0))
    {
        *u32pOutput = eVppBufType_Progressive;
    }
    else if(strcasecmp(cpInput, "tff") == 0)
    {
        *u32pOutput = eVppBufType_Interleaved_TFF;
    }
    else if(strcasecmp(cpInput, "bff") == 0)
    {
        *u32pOutput = eVppBufType_Interleaved_BFF;
    }
    else if(strcasecmp(cpInput, "right") == 0)
    {
        *u32pOutput = HQV_SPLIT_LEFT_TO_RIGHT;
    }
    else if(strcasecmp(cpInput, "left") == 0)
    {
        *u32pOutput = HQV_SPLIT_RIGHT_TO_LEFT;
    }
    else if(strcasecmp(cpInput, "down") == 0)
    {
        *u32pOutput = HQV_SPLIT_TOP_TO_BOTTOM;
    }
    else if(strcasecmp(cpInput, "up") == 0)
    {
        *u32pOutput = HQV_SPLIT_BOTTOM_TO_TOP;
    }
    else
    {
        LOGE("%s: ERROR: Input parameter %s is not supported!\n", __func__, cpInput);
        u32Ret = VPP_ERR;
    }

    return u32Ret;
}

#define VPP_MAP(_var, _this, _that) \
    case _this: _var = _that; break
static enum vpp_color_format eGetColorFmtFromFileFmt(enum buffer_format eFmt)
{
    enum vpp_color_format eRet = VPP_COLOR_FORMAT_MAX;
    switch (eFmt)
    {
        VPP_MAP(eRet, FILE_FORMAT_NV12, VPP_COLOR_FORMAT_NV12_VENUS);
        VPP_MAP(eRet, FILE_FORMAT_P010, VPP_COLOR_FORMAT_P010);
        VPP_MAP(eRet, FILE_FORMAT_NV12_UBWC, VPP_COLOR_FORMAT_UBWC_NV12);
        VPP_MAP(eRet, FILE_FORMAT_TP10_UBWC, VPP_COLOR_FORMAT_UBWC_TP10);
        VPP_MAP(eRet, FILE_FORMAT_RGBA8, VPP_COLOR_FORMAT_RGBA8);
        VPP_MAP(eRet, FILE_FORMAT_BGRA8, VPP_COLOR_FORMAT_BGRA8);
        VPP_MAP(eRet, FILE_FORMAT_RGBA8_UBWC, VPP_COLOR_FORMAT_UBWC_RGBA8);
        VPP_MAP(eRet, FILE_FORMAT_BGRA8_UBWC, VPP_COLOR_FORMAT_UBWC_BGRA8);
        VPP_MAP(eRet, FILE_FORMAT_RGB565_UBWC, VPP_COLOR_FORMAT_UBWC_RGB565);
        VPP_MAP(eRet, FILE_FORMAT_BGR565_UBWC, VPP_COLOR_FORMAT_UBWC_BGR565);
        default:
            eRet = VPP_COLOR_FORMAT_MAX;
    }

    return eRet;
}
#undef VPP_MAP

static uint32_t u32UpdatePortParamsFromPool(struct buf_pool_params *pstPoolParam,
                                            struct vpp_port_param *pstPortParam,
                                            enum vpp_port ePort)
{
    if (!pstPoolParam || !pstPortParam)
        return VPP_ERR_PARAM;

    pstPortParam->width = pstPoolParam->u32Width;
    pstPortParam->height = pstPoolParam->u32Height;
    if (ePort == VPP_PORT_INPUT)
        pstPortParam->fmt = eGetColorFmtFromFileFmt(pstPoolParam->eInputFileFormat);
    else
        pstPortParam->fmt = eGetColorFmtFromFileFmt(pstPoolParam->eOutputFileFormat);
    pstPortParam->stride = u32VppUtils_CalculateStride(pstPortParam->width,
                                                       pstPortParam->fmt);
    pstPortParam->scanlines = u32VppUtils_CalculateScanlines(pstPortParam->height,
                                                             pstPortParam->fmt);

    return VPP_OK;
}

static uint32_t u32GetArgs(int argc, char **argv, struct test_ctx *pCtx)
{
    int c;
    uint32_t u32Ret = 0;
    uint32_t u32Param, i;
    struct buf_pool_params *buf_param = &pCtx->params;
    struct functional_test_ctx *pFuncTestCtx = (struct functional_test_ctx *)pCtx->pPrivateCtx;
    uint32_t u32FrcSegIdx = 0;

    while (1)
    {
        static struct option long_options[] =
        {
            /* tests to run (bitfield) */
            {"tests", required_argument, 0, PARAM_TESTS},
            /* input file path */
            {"ifp", required_argument, 0, PARAM_IN_PATH},
            /* output file path */
            {"ofp", required_argument, 0, PARAM_OUT_PATH},
            /* input file name */
            {"ifn", required_argument, 0, PARAM_IN_FILE_NAME},
            /* output file name */
            {"ofn", required_argument, 0, PARAM_OUT_FILE_NAME},
            /* input file type */
            {"ift", required_argument, 0, PARAM_IN_FILE_TYPE},
            /* output file type */
            {"oft", required_argument, 0, PARAM_OUT_FILE_TYPE},
            /* input pixel format */
            {"ipf", required_argument, 0, PARAM_IN_PIX_FMT},
            /* output pixel format */
            {"opf", required_argument, 0, PARAM_OUT_PIX_FMT},
            /* input buffer type */
            {"ibt", required_argument, 0, PARAM_IN_BUF_TYPE},
            /* input frame width */
            {"ifw", required_argument, 0, PARAM_IN_FRAME_WIDTH},
            /* input frame height */
            {"ifh", required_argument, 0, PARAM_IN_FRAME_HEIGHT},
#ifdef SCALAR_ENABLE
            /* output frame width */
            {"ofw", required_argument, 0, PARAM_OUT_FRAME_WIDTH},
            /* output frame height */
            {"ofh", required_argument, 0, PARAM_OUT_FRAME_HEIGHT},
#endif
            /* input source number of frames; 0=ALL */
            {"inf", required_argument, 0, PARAM_IN_NUM_FRAMES},
            /* number of frames to generate on output.
                Will loop input until frames generated */
            {"nf", required_argument, 0, PARAM_OUT_NUM_FRAMES},
            /* HQV mode */
            {"hqv", required_argument, 0, PARAM_HQV_MODE},
            /* CADE mode */
            {"cadem", required_argument, 0, PARAM_CADE_MODE},
            /* CADE level */
            {"cadel", required_argument, 0, PARAM_CADE_LEVEL},
            /* CADE contrast */
            {"cadec", required_argument, 0, PARAM_CADE_CONTRAST_LEVEL},
            /* CADE saturation */
            {"cades", required_argument, 0, PARAM_CADE_SATURATION_LEVEL},
            /* AIE mode */
            {"aiem", required_argument, 0, PARAM_AIE_MODE},
            /* AIE hue mode */
            {"aiehm", required_argument, 0, PARAM_AIE_HUE_MODE},
            /* AIE CADE level */
            {"aiecl", required_argument, 0, PARAM_AIE_CADE_LEVEL},
            /* AIE LTM level */
            {"aiell", required_argument, 0, PARAM_AIE_LTM_LEVEL},
            /* AIE LTM Sat Gain */
            {"aielsg", required_argument, 0, PARAM_AIE_LTM_SAT_GAIN},
            /* AIE LTM Sat Offset */
            {"aielso", required_argument, 0, PARAM_AIE_LTM_SAT_OFFSET},
            /* AIE LTM Ace Strength */
            {"aielas", required_argument, 0, PARAM_AIE_LTM_ACE_STR},
            /* AIE LTM Ace Brightness Low */
            {"aielabl", required_argument, 0, PARAM_AIE_LTM_ACE_BRIL},
            /* AIE LTM Ace Brightness High */
            {"aielabh", required_argument, 0, PARAM_AIE_LTM_ACE_BRIH},
            /* QBR mode */
            {"qbrm", required_argument, 0, PARAM_QBR_MODE},
            /* EAR mode */
            {"earm", required_argument, 0, PARAM_EAR_MODE},
            /* Measurement enable */
            {"mease", required_argument, 0, PARAM_MEAS_ENABLE},
            /* DI mode */
            {"dim", required_argument, 0, PARAM_DI_MODE},
            /* TNR mode */
            {"tnrm", required_argument, 0, PARAM_TNR_MODE},
            /* TNR level */
            {"tnrl", required_argument, 0, PARAM_TNR_LEVEL},
            /* CNR mode */
            {"cnrm", required_argument, 0, PARAM_CNR_MODE},
            /* CNR level */
            {"cnrl", required_argument, 0, PARAM_CNR_LEVEL},
            /* FRC segment index */
            {"frcsegment", no_argument, 0, PARAM_FRC_SEGMENT},
            /* FRC mode */
            {"frcm", required_argument, 0, PARAM_FRC_MODE},
            /* FRC level */
            {"frcl", required_argument, 0, PARAM_FRC_LEVEL},
            /* FRC interp */
            {"frci", required_argument, 0, PARAM_FRC_INTERP},
            /* FRC timestamp start */
            {"frcts", required_argument, 0, PARAM_FRC_TIMESTAMP_START},
            /* FRC frame copy on fallback */
            {"frccpfb", required_argument, 0, PARAM_FRC_FRM_CP_ON_FALLBACK},
            /* FRC frame copy input */
            {"frccpi", required_argument, 0, PARAM_FRC_FRM_CP_INPUT},
            /* Split screen percentage */
            {"ssp", required_argument, 0, PARAM_GLOBAL_SPLITSCREEN_PERCENT},
            /* Split screen direction */
            {"ssd", required_argument, 0, PARAM_GLOBAL_SPLITSCREEN_DIRECTION},
            /* Number of reconfig streams */
            {"nrs", required_argument, 0, PARAM_NUM_RECONFIG_STREAMS},
            /* Reconfig stream 0 input file name */
            {"r0ifn", required_argument, 0, PARAM_RECONFIG0_IN_NAME},
            /* Reconfig stream 0 output file name */
            {"r0ofn", required_argument, 0, PARAM_RECONFIG0_OUT_NAME},
            /* Reconfig stream 0 input frame width */
            {"r0ifw", required_argument, 0, PARAM_RECONFIG0_IN_WIDTH},
            /* Reconfig stream 0 input frame height */
            {"r0ifh", required_argument, 0, PARAM_RECONFIG0_IN_HEIGHT},
            /* Reconfig stream 0 input file type */
            {"r0ift", required_argument, 0, PARAM_RECONFIG0_IN_FILE_TYPE},
            /* Reconfig stream 0 output file type */
//            {"r0oft", required_argument, 0, PARAM_RECONFIG0_OUT_FILE_TYPE},
            /* Reconfig stream 0 input pixel format */
            {"r0ipf", required_argument, 0, PARAM_RECONFIG0_IN_PIX_FMT},
            /* Reconfig stream 0 output pixel format */
            {"r0opf", required_argument, 0, PARAM_RECONFIG0_OUT_PIX_FMT},
            /* Reconfig stream 0 input buffer type */
            {"r0ibt", required_argument, 0, PARAM_RECONFIG0_IN_BUF_TYPE},
            /* Reconfig stream 1 input file name */
            {"r1ifn", required_argument, 0, PARAM_RECONFIG1_IN_NAME},
            /* Reconfig stream 1 output file name */
            {"r1ofn", required_argument, 0, PARAM_RECONFIG1_OUT_NAME},
            /* Reconfig stream 1 input frame width */
            {"r1ifw", required_argument, 0, PARAM_RECONFIG1_IN_WIDTH},
            /* Reconfig stream 1 input frame height */
            {"r1ifh", required_argument, 0, PARAM_RECONFIG1_IN_HEIGHT},
            /* Reconfig stream 1 input file type */
            {"r1ift", required_argument, 0, PARAM_RECONFIG1_IN_FILE_TYPE},
            /* Reconfig stream 1 output file type */
//            {"r1oft", required_argument, 0, PARAM_RECONFIG1_OUT_FILE_TYPE},
            /* Reconfig stream 1 input pixel format */
            {"r1ipf", required_argument, 0, PARAM_RECONFIG1_IN_PIX_FMT},
            /* Reconfig stream 1 output pixel format */
            {"r1opf", required_argument, 0, PARAM_RECONFIG1_OUT_PIX_FMT},
            /* Reconfig stream 1 input buffer type */
            {"r1ibt", required_argument, 0, PARAM_RECONFIG1_IN_BUF_TYPE},
            /* Reconfig stream 2 input file name */
            {"r2ifn", required_argument, 0, PARAM_RECONFIG2_IN_NAME},
            /* Reconfig stream 2 output file name */
            {"r2ofn", required_argument, 0, PARAM_RECONFIG2_OUT_NAME},
            /* Reconfig stream 2 input frame width */
            {"r2ifw", required_argument, 0, PARAM_RECONFIG2_IN_WIDTH},
            /* Reconfig stream 2 input frame height */
            {"r2ifh", required_argument, 0, PARAM_RECONFIG2_IN_HEIGHT},
            /* Reconfig stream 2 input file type */
            {"r2ift", required_argument, 0, PARAM_RECONFIG2_IN_FILE_TYPE},
            /* Reconfig stream 2 output file type */
//            {"r2oft", required_argument, 0, PARAM_RECONFIG2_OUT_FILE_TYPE},
            /* Reconfig stream 2 input pixel format */
            {"r2ipf", required_argument, 0, PARAM_RECONFIG2_IN_PIX_FMT},
            /* Reconfig stream 2 output pixel format */
            {"r2opf", required_argument, 0, PARAM_RECONFIG2_OUT_PIX_FMT},
            /* Reconfig stream 2 input buffer type */
            {"r2ibt", required_argument, 0, PARAM_RECONFIG2_IN_BUF_TYPE},
            /* Reconfig stream 3 input file name */
            {"r3ifn", required_argument, 0, PARAM_RECONFIG3_IN_NAME},
            /* Reconfig stream 3 output file name */
            {"r3ofn", required_argument, 0, PARAM_RECONFIG3_OUT_NAME},
            /* Reconfig stream 3 input frame width */
            {"r3ifw", required_argument, 0, PARAM_RECONFIG3_IN_WIDTH},
            /* Reconfig stream 3 input frame height */
            {"r3ifh", required_argument, 0, PARAM_RECONFIG3_IN_HEIGHT},
            /* Reconfig stream 3 input file type */
            {"r3ift", required_argument, 0, PARAM_RECONFIG3_IN_FILE_TYPE},
            /* Reconfig stream 3 output file type */
//            {"r3oft", required_argument, 0, PARAM_RECONFIG3_OUT_FILE_TYPE},
            /* Reconfig stream 3 input pixel format */
            {"r3ipf", required_argument, 0, PARAM_RECONFIG3_IN_PIX_FMT},
            /* Reconfig stream 3 output pixel format */
            {"r3opf", required_argument, 0, PARAM_RECONFIG3_OUT_PIX_FMT},
            /* Reconfig stream 3 input buffer type */
            {"r3ibt", required_argument, 0, PARAM_RECONFIG3_IN_BUF_TYPE},
            /* Reconfig stream 4 input file name */
            {"r4ifn", required_argument, 0, PARAM_RECONFIG4_IN_NAME},
            /* Reconfig stream 4 output file name */
            {"r4ofn", required_argument, 0, PARAM_RECONFIG4_OUT_NAME},
            /* Reconfig stream 4 input frame width */
            {"r4ifw", required_argument, 0, PARAM_RECONFIG4_IN_WIDTH},
            /* Reconfig stream 4 input frame height */
            {"r4ifh", required_argument, 0, PARAM_RECONFIG4_IN_HEIGHT},
            /* Reconfig stream 4 input file type */
            {"r4ift", required_argument, 0, PARAM_RECONFIG4_IN_FILE_TYPE},
            /* Reconfig stream 4 output file type */
//            {"r4oft", required_argument, 0, PARAM_RECONFIG4_OUT_FILE_TYPE},
            /* Reconfig stream 4 input pixel format */
            {"r4ipf", required_argument, 0, PARAM_RECONFIG4_IN_PIX_FMT},
            /* Reconfig stream 4 output pixel format */
            {"r4opf", required_argument, 0, PARAM_RECONFIG4_OUT_PIX_FMT},
            /* Reconfig stream 4 input buffer type */
            {"r4ibt", required_argument, 0, PARAM_RECONFIG4_IN_BUF_TYPE},
            /* Indicates that the test should allocate buffers in cpz */
            {"secure", required_argument, 0, PARAM_SECURE},
            /* Disable extradata */
            {"disexd", required_argument, 0, PARAM_DISABLE_EXTRA_DATA},
            /* Disable gralloc data */
            {"disgr", required_argument, 0, PARAM_DISABLE_GRALLOC},
            /* meta-data file name */
            {"mdfn", required_argument, 0, PARAM_META_DATA_FILE_NAME},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long_only (argc, argv, "",
                              long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case PARAM_TESTS:
                pFuncTestCtx->u32TestMask = strtoul(optarg, NULL, 0);
                LOGI ("Test Mask 0x%x\n", pFuncTestCtx->u32TestMask);
                break;
            case PARAM_IN_PATH:
                strlcpy(buf_param->cInputPath, optarg, sizeof(buf_param->cInputPath));
                for(i = 0; i < MAX_RECONFIG_STREAMS; i++)
                {
                    strlcpy(pFuncTestCtx->reconfig_streams[i].reconfig_pool_params.cInputPath,
                            optarg, sizeof(buf_param->cInputPath));
                }
                LOGI ("new input file path %s \n", buf_param->cInputPath);
                break;
            case PARAM_OUT_PATH:
                strlcpy(buf_param->cOutputPath, optarg,
                        sizeof(buf_param->cOutputPath));
                for(i = 0; i < MAX_RECONFIG_STREAMS; i++)
                {
                    strlcpy(pFuncTestCtx->reconfig_streams[i].reconfig_pool_params.cOutputPath,
                            optarg, sizeof(buf_param->cOutputPath));
                }
                LOGI ("new output file path %s \n", buf_param->cOutputPath);
                break;
            case PARAM_IN_FILE_NAME:
                strlcpy(buf_param->cInputName, optarg, sizeof(buf_param->cInputName));
                LOGI ("new input file name %s \n", buf_param->cInputName);
                break;
            case PARAM_OUT_FILE_NAME:
                strlcpy(buf_param->cOutputName, optarg,
                        sizeof(buf_param->cOutputName));
                LOGI ("new output file name %s \n", buf_param->cOutputName);
                break;
            case PARAM_RECONFIG0_IN_NAME:
                strlcpy(pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.cInputName, optarg,
                        sizeof(buf_param->cInputName));
                LOGI ("Reconfig 0 new input file name %s \n", pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.cInputName);
                break;
            case PARAM_RECONFIG0_OUT_NAME:
                strlcpy(pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.cOutputName, optarg,
                        sizeof(buf_param->cOutputName));
                LOGI ("Reconfig 0 new output file name %s \n", pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.cOutputName);
                break;
            case PARAM_RECONFIG1_IN_NAME:
                strlcpy(pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.cInputName, optarg,
                        sizeof(buf_param->cInputName));
                LOGI ("Reconfig 1 new input file name %s \n", pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.cInputName);
                break;
            case PARAM_RECONFIG1_OUT_NAME:
                strlcpy(pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.cOutputName, optarg,
                        sizeof(buf_param->cOutputName));
                LOGI ("Reconfig 1 new output file name %s \n", pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.cOutputName);
                break;
            case PARAM_RECONFIG2_IN_NAME:
                strlcpy(pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.cInputName, optarg,
                        sizeof(buf_param->cInputName));
                LOGI ("Reconfig 2 new input file name %s \n", pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.cInputName);
                break;
            case PARAM_RECONFIG2_OUT_NAME:
                strlcpy(pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.cOutputName, optarg,
                        sizeof(buf_param->cOutputName));
                LOGI ("Reconfig 2 new output file name %s \n", pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.cOutputName);
                break;
            case PARAM_RECONFIG3_IN_NAME:
                strlcpy(pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.cInputName, optarg,
                        sizeof(buf_param->cInputName));
                LOGI ("Reconfig 3 new input file name %s \n", pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.cInputName);
                break;
            case PARAM_RECONFIG3_OUT_NAME:
                strlcpy(pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.cOutputName, optarg,
                        sizeof(buf_param->cOutputName));
                LOGI ("Reconfig 3 new output file name %s \n", pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.cOutputName);
                break;
            case PARAM_RECONFIG4_IN_NAME:
                strlcpy(pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.cInputName, optarg,
                        sizeof(buf_param->cInputName));
                LOGI ("Reconfig 4 new input file name %s \n", pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.cInputName);
                break;
            case PARAM_RECONFIG4_OUT_NAME:
                strlcpy(pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.cOutputName, optarg,
                        sizeof(buf_param->cOutputName));
                LOGI ("Reconfig 4 new output file name %s \n", pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.cOutputName);
                break;
            case PARAM_NUM_RECONFIG_STREAMS:
                pFuncTestCtx->u32StreamCount = atoi(optarg);
                if(pFuncTestCtx->u32StreamCount > MAX_RECONFIG_STREAMS)
                    pFuncTestCtx->u32StreamCount = MAX_RECONFIG_STREAMS;
                LOGI ("Reconfiguration stream count %d\n", pFuncTestCtx->u32StreamCount);
                break;
            case PARAM_IN_FILE_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    buf_param->eInputFileType = (enum file_type)u32Param;
                }
                LOGI ("Input File type %d \n", buf_param->eInputFileType);
                break;
            case PARAM_OUT_FILE_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    buf_param->eOutputFileType = (enum file_type)u32Param;
                    for(i = 0; i < MAX_RECONFIG_STREAMS; i++)
                    {
                        pFuncTestCtx->reconfig_streams[i].reconfig_pool_params.eOutputFileType = (enum file_type)u32Param;
                    }
                }
                LOGI ("Output file type %d \n", buf_param->eOutputFileType);
                break;
            case PARAM_RECONFIG0_IN_FILE_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.eInputFileType = (enum file_type)u32Param;
                }
                LOGI ("Reconfig0 Input File type %d \n", u32Param);
                break;
/*
            case PARAM_RECONFIG0_OUT_FILE_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.eOutputFileType = u32Param;
                }
                LOGI ("Reconfig0 Output file type %d \n", u32Param);
                break;
*/
            case PARAM_RECONFIG1_IN_FILE_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.eInputFileType = (enum file_type)u32Param;
                }
                LOGI ("Reconfig1 Input File type %d \n", u32Param);
                break;
/*
            case PARAM_RECONFIG1_OUT_FILE_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.eOutputFileType = u32Param;
                }
                LOGI ("Reconfig1 Output file type %d \n", u32Param);
                break;
*/
            case PARAM_RECONFIG2_IN_FILE_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.eInputFileType = (enum file_type)u32Param;
                }
                LOGI ("Reconfig2 Input File type %d \n", u32Param);
                break;
/*
            case PARAM_RECONFIG2_OUT_FILE_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.eOutputFileType = u32Param;
                }
                LOGI ("Reconfig2 Output file type %d \n", u32Param);
                break;
*/
            case PARAM_RECONFIG3_IN_FILE_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.eInputFileType = (enum file_type)u32Param;
                }
                LOGI ("Reconfig3 Input File type %d \n", u32Param);
                break;
/*
            case PARAM_RECONFIG3_OUT_FILE_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.eOutputFileType = u32Param;
                }
                LOGI ("Reconfig3 Output file type %d \n", u32Param);
                break;
*/
            case PARAM_RECONFIG4_IN_FILE_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.eInputFileType = (enum file_type)u32Param;
                }
                LOGI ("Reconfig4 Input File type %d \n", u32Param);
                break;
/*
            case PARAM_RECONFIG4_OUT_FILE_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.eOutputFileType = u32Param;
                }
                LOGI ("Reconfig4 Output file type %d \n", u32Param);
                break;
*/
            case PARAM_IN_PIX_FMT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    buf_param->eInputFileFormat = (enum buffer_format)u32Param;
                }
                LOGI ("Input File format %d \n", buf_param->eInputFileFormat);
                break;
            case PARAM_OUT_PIX_FMT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    buf_param->eOutputFileFormat = (enum buffer_format)u32Param;
                }
                LOGI ("Output file format %d \n", buf_param->eOutputFileFormat);
                break;
            case PARAM_RECONFIG0_IN_PIX_FMT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.eInputFileFormat = (enum buffer_format)u32Param;
                }
                LOGI ("Reconfig0 Input File format %d \n", u32Param);
                break;
            case PARAM_RECONFIG0_OUT_PIX_FMT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.eOutputFileFormat = (enum buffer_format)u32Param;
                }
                LOGI ("Reconfig0 Output file format %d \n", u32Param);
                break;
            case PARAM_RECONFIG1_IN_PIX_FMT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.eInputFileFormat = (enum buffer_format)u32Param;
                }
                LOGI ("Reconfig1 Input File format %d \n", u32Param);
                break;
            case PARAM_RECONFIG1_OUT_PIX_FMT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.eOutputFileFormat = (enum buffer_format)u32Param;
                }
                LOGI ("Reconfig1 Output file format %d \n", u32Param);
                break;
            case PARAM_RECONFIG2_IN_PIX_FMT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.eInputFileFormat = (enum buffer_format)u32Param;
                }
                LOGI ("Reconfig2 Input File format %d \n", u32Param);
                break;
            case PARAM_RECONFIG2_OUT_PIX_FMT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.eOutputFileFormat = (enum buffer_format)u32Param;
                }
                LOGI ("Reconfig2 Output file format %d \n", u32Param);
                break;
            case PARAM_RECONFIG3_IN_PIX_FMT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.eInputFileFormat = (enum buffer_format)u32Param;
                }
                LOGI ("Reconfig3 Input File format %d \n", u32Param);
                break;
            case PARAM_RECONFIG3_OUT_PIX_FMT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.eOutputFileFormat = (enum buffer_format)u32Param;
                }
                LOGI ("Reconfig3 Output file format %d \n", u32Param);
                break;
            case PARAM_RECONFIG4_IN_PIX_FMT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.eInputFileFormat = (enum buffer_format)u32Param;
                }
                LOGI ("Reconfig4 Input File format %d \n", u32Param);
                break;
            case PARAM_RECONFIG4_OUT_PIX_FMT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.eOutputFileFormat = (enum buffer_format)u32Param;
                }
                LOGI ("Reconfig4 Output file format %d \n", u32Param);
                break;
            case PARAM_IN_BUF_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    buf_param->eBufferType= (t_EVppBufType)u32Param;
                }
                LOGI ("Input Buffer Type %d \n", buf_param->eBufferType);
                break;
            case PARAM_RECONFIG0_IN_BUF_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.eBufferType = (t_EVppBufType)u32Param;
                }
                LOGI ("Reconfig0 Input Buffer Type %d \n", u32Param);
                break;
            case PARAM_RECONFIG1_IN_BUF_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.eBufferType = (t_EVppBufType)u32Param;
                }
                LOGI ("Reconfig1 Input Buffer Type %d \n", u32Param);
                break;
            case PARAM_RECONFIG2_IN_BUF_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.eBufferType = (t_EVppBufType)u32Param;
                }
                LOGI ("Reconfig2 Input Buffer Type %d \n", u32Param);
                break;
            case PARAM_RECONFIG3_IN_BUF_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.eBufferType = (t_EVppBufType)u32Param;
                }
                LOGI ("Reconfig3 Input Buffer Type %d \n", u32Param);
                break;
            case PARAM_RECONFIG4_IN_BUF_TYPE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.eBufferType = (t_EVppBufType)u32Param;
                }
                LOGI ("Reconfig4 Input Buffer Type %d \n", u32Param);
                break;
            case PARAM_SECURE:
                if (atoi(optarg))
                {
                    pCtx->eProtection = PROTECTION_ZONE_SECURE;
                }
                else
                {
                    pCtx->eProtection = PROTECTION_ZONE_NONSECURE;
                }
                pCtx->params.eProtection = pCtx->eProtection;

                for (i = 0; i < MAX_RECONFIG_STREAMS; i++)
                {
                    pFuncTestCtx->reconfig_streams[i].reconfig_pool_params.eProtection = pstTestCtx->eProtection;
                }

                LOGI ("%s session requested",
                      pCtx->eProtection == PROTECTION_ZONE_SECURE ?
                      "secure" : "non-secure");
                break;
            case PARAM_IN_FRAME_WIDTH:
                buf_param->u32Width = atoi(optarg);
                LOGI ("Input width %d \n", buf_param->u32Width);
                break;
            case PARAM_IN_FRAME_HEIGHT:
                buf_param->u32Height = atoi(optarg);
                LOGI ("Input height %d \n", buf_param->u32Height);
                break;
            case PARAM_RECONFIG0_IN_WIDTH:
                pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.u32Width = atoi(optarg);
                LOGI ("Reconfig 0 input width %d \n", pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.u32Width);
                break;
            case PARAM_RECONFIG0_IN_HEIGHT:
                pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.u32Height = atoi(optarg);
                LOGI ("Reconfig 0 input height %d \n", pFuncTestCtx->reconfig_streams[0].reconfig_pool_params.u32Height);
                break;
            case PARAM_RECONFIG1_IN_WIDTH:
                pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.u32Width = atoi(optarg);
                LOGI ("Reconfig 1 input width %d \n", pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.u32Width);
                break;
            case PARAM_RECONFIG1_IN_HEIGHT:
                pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.u32Height = atoi(optarg);
                LOGI ("Reconfig 1 input height %d \n", pFuncTestCtx->reconfig_streams[1].reconfig_pool_params.u32Height);
                break;
            case PARAM_RECONFIG2_IN_WIDTH:
                pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.u32Width = atoi(optarg);
                LOGI ("Reconfig 2 input width %d \n", pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.u32Width);
                break;
            case PARAM_RECONFIG2_IN_HEIGHT:
                pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.u32Height = atoi(optarg);
                LOGI ("Reconfig 2 input height %d \n", pFuncTestCtx->reconfig_streams[2].reconfig_pool_params.u32Height);
                break;
            case PARAM_RECONFIG3_IN_WIDTH:
                pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.u32Width = atoi(optarg);
                LOGI ("Reconfig 3 input width %d \n", pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.u32Width);
                break;
            case PARAM_RECONFIG3_IN_HEIGHT:
                pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.u32Height = atoi(optarg);
                LOGI ("Reconfig 3 input height %d \n", pFuncTestCtx->reconfig_streams[3].reconfig_pool_params.u32Height);
                break;
            case PARAM_RECONFIG4_IN_WIDTH:
                pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.u32Width = atoi(optarg);
                LOGI ("Reconfig 4 input width %d \n", pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.u32Width);
                break;
            case PARAM_RECONFIG4_IN_HEIGHT:
                pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.u32Height = atoi(optarg);
                LOGI ("Reconfig 4 input height %d \n", pFuncTestCtx->reconfig_streams[4].reconfig_pool_params.u32Height);
                break;
            case PARAM_OUT_FRAME_WIDTH:
            #ifdef SCALAR_ENABLE
                // port_param->OutWidth = atoi(optarg);
                // buf_param->u32OutWidth = port_param->OutWidth;
                // LOGI ("Output width %d \n", port_param->Outwidth);
            #else
                LOGI ("Cannot specify output width, scalar not supported\n");
            #endif
                break;
            case PARAM_OUT_FRAME_HEIGHT:
            #ifdef SCALAR_ENABLE
                // port_param->OutHeight = atoi(optarg);
                // buf_param->u32OutHeight = port_param->OutHeight;
                // LOGI ("Output height %d \n", port_param->OutHeight);
            #else
                LOGI ("Cannot specify output height, scalar not supported\n");
            #endif
                break;
            case PARAM_IN_NUM_FRAMES:
                buf_param->u32MaxInputFrames = atoi(optarg);
                LOGI ("Max source input frames %d \n", buf_param->u32MaxInputFrames);
                break;
            case PARAM_OUT_NUM_FRAMES:
                pCtx->u32FramesOut = atoi(optarg);
                LOGI ("Frames to produce %d \n", pCtx->u32FramesOut);
                break;
            case PARAM_HQV_MODE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                    pCtx->hqvCtrl.mode = (enum hqv_mode)u32Param;
                LOGI ("HQV mode %d \n", pCtx->hqvCtrl.mode);
                break;
            case PARAM_CADE_MODE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pCtx->hqvCtrl.cade.mode = (enum hqv_mode)u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_CADE);
                }
                LOGI ("CADE mode %d\n", pCtx->hqvCtrl.cade.mode);
                break;
            case PARAM_CADE_LEVEL:
                pCtx->hqvCtrl.cade.cade_level = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_CADE);
                LOGI ("CADE level %d\n", pCtx->hqvCtrl.cade.cade_level);
                break;
            case PARAM_CADE_CONTRAST_LEVEL:
                pCtx->hqvCtrl.cade.contrast = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_CADE);
                LOGI ("CADE contrast level %d\n", pCtx->hqvCtrl.cade.contrast);
                break;
            case PARAM_CADE_SATURATION_LEVEL:
                pCtx->hqvCtrl.cade.saturation = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_CADE);
                LOGI ("CADE saturation level %d\n", pCtx->hqvCtrl.cade.saturation);
                break;
            case PARAM_AIE_MODE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pCtx->hqvCtrl.aie.mode = (enum hqv_mode)u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_AIE);
                }
                LOGI ("AIE mode %d\n", pCtx->hqvCtrl.aie.mode);
                break;
            case PARAM_AIE_HUE_MODE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pCtx->hqvCtrl.aie.hue_mode = (enum hqv_hue_mode)u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_AIE);
                }
                LOGI ("AIE HUE mode %d\n", pCtx->hqvCtrl.aie.hue_mode);
                break;
            case PARAM_AIE_CADE_LEVEL:
                pCtx->hqvCtrl.aie.cade_level = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_AIE);
                LOGI ("AIE CADE level %d\n", pCtx->hqvCtrl.aie.cade_level);
                break;
            case PARAM_AIE_LTM_LEVEL:
                pCtx->hqvCtrl.aie.ltm_level = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_AIE);
                LOGI ("AIE LTM level %d\n", pCtx->hqvCtrl.aie.ltm_level);
                break;
            case PARAM_AIE_LTM_SAT_GAIN:
                pCtx->hqvCtrl.aie.ltm_sat_gain = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_AIE);
                LOGI ("AIE LTM Sat Gain %d\n", pCtx->hqvCtrl.aie.ltm_sat_gain);
                break;
            case PARAM_AIE_LTM_SAT_OFFSET:
                pCtx->hqvCtrl.aie.ltm_sat_offset = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_AIE);
                LOGI ("AIE LTM Sat Offset %d\n", pCtx->hqvCtrl.aie.ltm_sat_offset);
                break;
            case PARAM_AIE_LTM_ACE_STR:
                pCtx->hqvCtrl.aie.ltm_ace_str = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_AIE);
                LOGI ("AIE LTM Ace Strength %d\n", pCtx->hqvCtrl.aie.ltm_ace_str);
                break;
            case PARAM_AIE_LTM_ACE_BRIL:
                pCtx->hqvCtrl.aie.ltm_ace_bright_l = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_AIE);
                LOGI ("AIE LTM Ace Brightness Low %d\n", pCtx->hqvCtrl.aie.ltm_ace_bright_l);
                break;
            case PARAM_AIE_LTM_ACE_BRIH:
                pCtx->hqvCtrl.aie.ltm_ace_bright_h = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_AIE);
                LOGI ("AIE LTM Ace Brightness High %d\n", pCtx->hqvCtrl.aie.ltm_ace_bright_h);
                break;
            case PARAM_QBR_MODE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pCtx->hqvCtrl.qbr.mode = (enum hqv_qbr_mode)u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_QBR);
                }
                LOGI ("QBR mode %d\n", pCtx->hqvCtrl.qbr.mode);
                break;
            case PARAM_EAR_MODE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pCtx->hqvCtrl.ear.mode = (enum hqv_ear_mode)u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_EAR);
                }
                LOGI ("EAR mode %d\n", pCtx->hqvCtrl.ear.mode);
                break;
            case PARAM_MEAS_ENABLE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pCtx->hqvCtrl.meas.enable = u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_MEAS);
                }
                LOGI ("MEAS enable %d\n", pCtx->hqvCtrl.meas.enable);
                break;
            case PARAM_DI_MODE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pCtx->hqvCtrl.di.mode = (enum hqv_di_mode)u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_DI);
                }
                LOGI ("DI mode %d\n", pCtx->hqvCtrl.di.mode);
                break;
            case PARAM_TNR_MODE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pCtx->hqvCtrl.tnr.mode = (enum hqv_mode)u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_TNR);
                }
                LOGI ("TNR mode %d\n", pCtx->hqvCtrl.tnr.mode);
                break;
            case PARAM_TNR_LEVEL:
                pCtx->hqvCtrl.tnr.level = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_TNR);
                LOGI ("TNR level %d\n", pCtx->hqvCtrl.tnr.level);
                break;
            case PARAM_CNR_MODE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pCtx->hqvCtrl.cnr.mode = (enum hqv_mode)u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_CNR);
                }
                LOGI ("CNR mode %d\n", pCtx->hqvCtrl.cnr.mode);
                break;
            case PARAM_CNR_LEVEL:
                pCtx->hqvCtrl.cnr.level = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_CNR);
                LOGI ("CNR level %d\n", pCtx->hqvCtrl.cnr.level);
                break;
            case PARAM_FRC_SEGMENT:
            {
                struct vpp_ctrl_frc_segment *pstFrcSegs;

                uint32_t u32CurSegCnt = pCtx->hqvCtrl.frc.num_segments;
                uint32_t u32NewSegCnt = u32CurSegCnt + 1;

                LOGI("reallocating segments, current=%u, required=%u",
                     u32CurSegCnt, u32NewSegCnt);

                pstFrcSegs = (struct vpp_ctrl_frc_segment *)
                    calloc(u32NewSegCnt, sizeof(struct vpp_ctrl_frc_segment));
                if (!pstFrcSegs)
                {
                    LOGE("failed to allocate segments, u32CurSegCnt=%u, u32NewSegCnt=%u",
                         u32CurSegCnt, u32NewSegCnt);
                    break;
                }

                if (u32CurSegCnt > 0)
                {
                    memcpy(pstFrcSegs, pCtx->hqvCtrl.frc.segments,
                           sizeof(struct vpp_ctrl_frc_segment) * u32CurSegCnt);
                    free(pCtx->hqvCtrl.frc.segments);
                }

                pCtx->hqvCtrl.frc.segments = pstFrcSegs;
                pCtx->hqvCtrl.frc.num_segments = u32NewSegCnt;

                u32FrcSegIdx = u32NewSegCnt - 1;
                LOGI("FRC segment index = %u", u32FrcSegIdx);
                break;
            }
            case PARAM_FRC_MODE:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if ((u32Ret == VPP_OK) && (u32FrcSegIdx < pCtx->hqvCtrl.frc.num_segments) &&
                    (pCtx->hqvCtrl.frc.segments))
                {
                    pCtx->hqvCtrl.frc.segments[u32FrcSegIdx].mode = (enum hqv_frc_mode)u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_FRC);
                    LOGI ("FRC mode[%u]: %d", u32FrcSegIdx,
                          pCtx->hqvCtrl.frc.segments[u32FrcSegIdx].mode);
                }
                else
                    LOGE ("PARAM_FRC_MODE failed index=%u", u32FrcSegIdx);
                break;
            case PARAM_FRC_LEVEL:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if ((u32Ret == VPP_OK) && (u32FrcSegIdx < pCtx->hqvCtrl.frc.num_segments) &&
                    (pCtx->hqvCtrl.frc.segments))
                {
                    pCtx->hqvCtrl.frc.segments[u32FrcSegIdx].level = (enum hqv_frc_level)u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_FRC);
                    LOGI ("FRC level[%u]: %d", u32FrcSegIdx,
                          pCtx->hqvCtrl.frc.segments[u32FrcSegIdx].level);
                }
                else
                    LOGE ("PARAM_FRC_LEVEL failed index=%u", u32FrcSegIdx);
                break;
            case PARAM_FRC_INTERP:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if ((u32Ret == VPP_OK) && (u32FrcSegIdx < pCtx->hqvCtrl.frc.num_segments) &&
                    (pCtx->hqvCtrl.frc.segments))
                {
                    pCtx->hqvCtrl.frc.segments[u32FrcSegIdx].interp = (enum hqv_frc_interp)u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_FRC);
                    LOGI ("FRC interp[%u]: %d", u32FrcSegIdx,
                          pCtx->hqvCtrl.frc.segments[u32FrcSegIdx].interp);
                }
                else
                    LOGE ("PARAM_FRC_INTERP failed index=%u", u32FrcSegIdx);
                break;
            case PARAM_FRC_TIMESTAMP_START:
                if ((u32FrcSegIdx < pCtx->hqvCtrl.frc.num_segments) &&
                    (pCtx->hqvCtrl.frc.segments))
                {
                    pCtx->hqvCtrl.frc.segments[u32FrcSegIdx].ts_start = atoi(optarg);
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_FRC);
                    LOGI ("FRC timestamp start[%u]: %lld", u32FrcSegIdx,
                          pCtx->hqvCtrl.frc.segments[u32FrcSegIdx].ts_start);
                }
                else
                    LOGE ("PARAM_FRC_TIMESTAMP_START failed index=%u", u32FrcSegIdx);
                break;
            case PARAM_FRC_FRM_CP_ON_FALLBACK:
                if ((u32FrcSegIdx < pCtx->hqvCtrl.frc.num_segments) &&
                    (pCtx->hqvCtrl.frc.segments))
                {
                    pCtx->hqvCtrl.frc.segments[u32FrcSegIdx].frame_copy_on_fallback = atoi(optarg);
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_FRC);
                    LOGI ("FRC frame copy on fallback[%u]: %u", u32FrcSegIdx,
                          pCtx->hqvCtrl.frc.segments[u32FrcSegIdx].frame_copy_on_fallback);
                }
                else
                    LOGE ("PARAM_FRC_FR_CP_ON_FALLBACK failed index=%u", u32FrcSegIdx);
                break;
            case PARAM_FRC_FRM_CP_INPUT:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if ((u32Ret == VPP_OK) && (u32FrcSegIdx < pCtx->hqvCtrl.frc.num_segments) &&
                    (pCtx->hqvCtrl.frc.segments))
                {
                    pCtx->hqvCtrl.frc.segments[u32FrcSegIdx].frame_copy_input = atoi(optarg);
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_FRC);
                    LOGI ("FRC frame copy input[%u]: %u", u32FrcSegIdx,
                          pCtx->hqvCtrl.frc.segments[u32FrcSegIdx].frame_copy_input);
                }
                else
                    LOGE ("PARAM_FRC_FR_CP_INPUT failed index=%u", u32FrcSegIdx);
                break;
            case PARAM_GLOBAL_SPLITSCREEN_PERCENT:
                pCtx->hqvCtrl.demo.process_percent = atoi(optarg);
                pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_GLOBAL_DEMO);
                LOGI ("Split screen process percent %d\n", pCtx->hqvCtrl.demo.process_percent);
                break;
            case PARAM_GLOBAL_SPLITSCREEN_DIRECTION:
                u32Ret |= u32ConvertParams(optarg, &u32Param);
                if(u32Ret == VPP_OK)
                {
                    pCtx->hqvCtrl.demo.process_direction = (enum hqv_split_direction)u32Param;
                    pCtx->u32hqvCtrlSetFlag |= (1 << HQV_CONTROL_GLOBAL_DEMO);
                }
                LOGI ("Split screen process direction %d\n", pCtx->hqvCtrl.demo.process_direction);
                break;
            case PARAM_DISABLE_EXTRA_DATA:
                buf_param->bDisableExtraData = atoi(optarg);
                LOGI ("Extra-Data disable %u", buf_param->bDisableExtraData);
                break;
            case PARAM_DISABLE_GRALLOC:
                buf_param->bDisableGralloc = atoi(optarg);
                LOGI ("Gralloc buf disable %u", buf_param->bDisableGralloc);
                break;
            case PARAM_META_DATA_FILE_NAME:
                strlcpy(buf_param->stMetaData.cMetaDataInputName, optarg,
                        sizeof(buf_param->stMetaData.cMetaDataInputName));
                LOGI ("Meta-Data input file name %s", buf_param->stMetaData.cMetaDataInputName);
                break;
            default:
                LOGE("%s: ERROR: Parameter undefined in test app!\n", __func__);
                u32Ret = VPP_ERR;
        }
    }

    u32Ret |= u32UpdatePortParamsFromPool(buf_param, &pCtx->port_param, VPP_PORT_INPUT);
    buf_param->eInputBufFmt = pCtx->port_param.fmt;

    u32Ret |= u32UpdatePortParamsFromPool(buf_param, &pCtx->port_param_out, VPP_PORT_OUTPUT);
    buf_param->eOutputBufFmt = pCtx->port_param_out.fmt;

    for (i = 0; i < MAX_RECONFIG_STREAMS; i++)
    {
        u32Ret |= u32UpdatePortParamsFromPool(&pFuncTestCtx->reconfig_streams[i].reconfig_pool_params,
                                              &pFuncTestCtx->reconfig_streams[i].port_param,
                                              VPP_PORT_INPUT);
        pFuncTestCtx->reconfig_streams[i].reconfig_pool_params.eInputBufFmt =
            pFuncTestCtx->reconfig_streams[i].port_param.fmt;

        u32Ret |= u32UpdatePortParamsFromPool(&pFuncTestCtx->reconfig_streams[i].reconfig_pool_params,
                                              &pFuncTestCtx->reconfig_streams[i].port_param_out,
                                              VPP_PORT_OUTPUT);
        pFuncTestCtx->reconfig_streams[i].reconfig_pool_params.eOutputBufFmt =
            pFuncTestCtx->reconfig_streams[i].port_param_out.fmt;
    }


    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        LOGI ("non-option ARGV-elements: ");
        while (optind < argc)
            LOGI ("%s ", argv[optind++]);
    }

    return u32Ret;
}


void vFuncTestInit(struct test_ctx *pstTestCtx, struct functional_test_ctx *pFuncTestCtx)
{
    uint32_t u32Ret, i;

    // Initialize global test context
    u32Ret = tctx_common_init(pstTestCtx);
    pstTestCtx->pPrivateCtx = pFuncTestCtx;
    if(u32Ret)
    {
        LOGE("Error initializing test context!");
        return;
    }
    tctx_set_port_params(pstTestCtx, 1920, 1080, VPP_COLOR_FORMAT_NV12_VENUS);

    u32Ret = buf_params_init_default(&pstTestCtx->params, &pstTestCtx->port_param);
    if(u32Ret)
    {
        LOGE("Error initializing buffer parameters!");
        return;
    }

    memset(pFuncTestCtx, 0, sizeof(struct functional_test_ctx));
    for(i = 0; i < MAX_RECONFIG_STREAMS; i++)
    {
        struct vpp_port_param *pp = &pFuncTestCtx->reconfig_streams[i].port_param;
        set_port_params(pp, 1920, 1080, VPP_COLOR_FORMAT_NV12_VENUS);
        u32Ret = buf_params_init_default(&pFuncTestCtx->reconfig_streams[i].reconfig_pool_params, pp);
        if(u32Ret)
        {
            LOGE("Error initializing buffer parameters!");
            return;
        }
    }

    populate_port_params(CLIP_CANYON_720x480, VPP_PORT_INPUT,
                         &pFuncTestCtx->reconfig_streams[0].stNewParams.input_port);
    populate_port_params(CLIP_CANYON_720x480, VPP_PORT_OUTPUT,
                         &pFuncTestCtx->reconfig_streams[0].stNewParams.output_port);

    populate_pool_params(CLIP_CANYON_720x480, &pFuncTestCtx->reconfig_streams[0].reconfig_pool_params);

    populate_port_params(CLIP_FLOWER_1920x1080, VPP_PORT_INPUT,
                         &pFuncTestCtx->reconfig_streams[1].stNewParams.input_port);
    populate_port_params(CLIP_FLOWER_1920x1080, VPP_PORT_OUTPUT,
                         &pFuncTestCtx->reconfig_streams[1].stNewParams.output_port);

    populate_pool_params(CLIP_FLOWER_1920x1080, &pFuncTestCtx->reconfig_streams[1].reconfig_pool_params);

    populate_port_params(CLIP_NEW_YORK_3840x2160, VPP_PORT_INPUT,
                         &pFuncTestCtx->reconfig_streams[2].stNewParams.input_port);
    populate_port_params(CLIP_NEW_YORK_3840x2160, VPP_PORT_OUTPUT,
                         &pFuncTestCtx->reconfig_streams[2].stNewParams.output_port);

    populate_pool_params(CLIP_NEW_YORK_3840x2160, &pFuncTestCtx->reconfig_streams[2].reconfig_pool_params);

    pFuncTestCtx->u32StreamCount = 3;
    pFuncTestCtx->u32TestMask = 0xFFFFFFFF;

    pstTestCtx->hqvCtrl.cade.contrast = 50;
    pstTestCtx->hqvCtrl.cade.saturation = 50;
}

static void vFuncTestTerm(struct test_ctx *pstTestCtx, struct functional_test_ctx *pFuncTestCtx)
{
    VPP_RET_VOID_IF_NULL(pstTestCtx);
    VPP_RET_VOID_IF_NULL(pFuncTestCtx);

    if (pstTestCtx->hqvCtrl.frc.segments)
    {
        free(pstTestCtx->hqvCtrl.frc.segments);
        pstTestCtx->hqvCtrl.frc.segments = NULL;
        pstTestCtx->hqvCtrl.frc.num_segments = 0;
    }
}

/************************************************************************
 * Global Functions
 ***********************************************************************/
int main(int argc, char **argv)
{
    // Initialize the test bench
    vDvpTb_Init();

    vFuncTestInit(pstTestCtx, &FuncTestCtx);

    if(argc > 1 && u32GetArgs(argc, argv, pstTestCtx) != VPP_OK)
    {
        LOGE("error parsing args");
        return -1;
    }

    // Add the test suites to the framework
    TEST_SUITE_INSTALL(BufferExchangeSuite);
    TEST_SUITE_INSTALL(ConcurrencySuite);

#ifdef VPP_SERVICE
    configureRpcThreadpool(10, false /* callerWillJoin */);
    // otherwise, incoming callbacks from the vppService would block the
    // main thread (we have to do this as we are a "Binder server").
#endif //_VPP_SERVICE_

    // Start running the test bench
    vDvpTb_RunTests();

    // Clean up
    vFuncTestTerm(pstTestCtx, &FuncTestCtx);

    // Destroy the test context
    tctx_common_destroy(pstTestCtx);

    // Terminate the test bench
    vDvpTb_Term();

    return 0;
}
