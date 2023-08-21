/*!
 * @file vpp_ip_hvx.c
 *
 * @cr
 * Copyright (c) 2015-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <sys/types.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define VPP_LOG_TAG     VPP_LOG_MODULE_HVX_TAG
#define VPP_LOG_MODULE  VPP_LOG_MODULE_HVX
#include "vpp_dbg.h"

#include "vpp.h"
#include "vpp_reg.h"
#include "vpp_ip.h"
#include "vpp_ip_hvx.h"
#include "hvx_debug.h"
#include "vpp_utils.h"
#include "vpp_stats.h"
#include "vpp_tunings.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/
// Out-Of-Bounds
#define OOB(x, min, max)    ((x) < (min) || (x) > (max))

#define HVX_DI_IN_BUF_CNT       4
#define HVX_OTHER_IN_BUF_CNT    1
#define HVX_DI_OUT_BUF_CNT      2
#define HVX_OTHER_OUT_BUF_CNT   1

#define HVX_FW_DEFAULT_IN_BUF_NEED      1
#define HVX_FW_DEFAULT_IN_BUF_RELEASE   1
#define HVX_FW_DEFAULT_OUT_BUF_NEED     1
#define HVX_FW_DEFAULT_OUT_BUF_RELEASE  1

// #define HVX_SINGLE_THREAD
// #define HVX_REG_BUFFER_ON_QUEUE
// #define HVX_DO_NOT_REGISTER_BUFFER

// #define HVX_FORCE_DUMP_BUF
#define HVX_DUMP_BUF_IN_NM                  "hvx_%p_in.yuv"
#define HVX_DUMP_BUF_OUT_NM                 "hvx_%p_out.yuv"
#define HVX_TUNINGS_FILE_NAME               "/mnt/vendor/persist/vpp/tunings.txt"

// uncomment to update gralloc framerate whenever the DI mask is enabled
//#define UPDATE_GRALLOC_FRAMERATE_WHEN_DI_MASK_ENABLED

// uncomment to update gralloc framerate whenever the DI mask is enabled
// and only if DI has actually been run
//#define UPDATE_GRALLOC_FRAMERATE_WHEN_DI_RUN

#ifdef HVX_LOG_FRAMEDESC
#define HVX_DUMP_FRAME_DESC(pstCb, pstBuf, str, port) vVppIpHvx_DumpFrameDesc(pstCb, pstBuf, str, port)
#else
#define HVX_DUMP_FRAME_DESC(pstCb, pstBuf, str, port)
#endif

enum {
    HVX_STAT_PROC,
    HVX_STAT_WORKER,
    HVX_STAT_WORKER_SLEEP,
};

typedef enum {
    eHvxDiCfgLut_TFF,
    eHvxDiCfgLut_BFF,
    eHvxDiCfgLut_Max,
} t_EHvxDiCfgLut;

static const uint8_t acu8DiBufCfgLut[][HVX_DI_IN_BUF_CNT-1][8] = {
    // eHvxDICfgLut_TFF,
    {
        // BufIdx, UseDelta, BufIdx, UseDelta, BufIdx, UseDelta, BufIdx, UseDelta
        {0, 1, 0, 1, 0, 1, 0, 1}, // 1 Input Frame
        {0, 1, 1, 0, 1, 1, 1, 1}, // 2 Input Frame
        {0, 1, 1, 0, 1, 1, 2, 0}, // 3 Input Frame
    },
    // eHvxDICfgLut_BFF,
    {
        // BufIdx, UseDelta, BufIdx, UseDelta, BufIdx, UseDelta, BufIdx, UseDelta
        {0, 1, 0, 0, 0, 0, 0, 0}, // 1 Input Frame
        {0, 1, 0, 0, 1, 1, 1, 0}, // 2 Input Frame
        {0, 1, 0, 0, 1, 1, 1, 0}, // 3 Input Frame
    },
};

static const int8_t acs8DiExtCfgLut[][HVX_DI_IN_BUF_CNT-1][5] = {
    // eHvxDICfgLut_TFF
    {
        // DC oBuf, IDR oBuf, IDR Chk[1Buf], IDR Chk[2Bufs], IDR Chk[3Bufs]
        {0,  0, 0, 1, 1}, // in frame 0 DC or IDR flagged
        {0,  1, 0, 0, 1}, // in frame 1 DC or IDR flagged
        {1, -1, 0, 0, 0}, // in frame 2 DC or IDR flagged
    },
    // eHvxDICfgLut_BFF
    {
        // DC oBuf, IDR oBuf, IDR Chk[1Buf], IDR Chk[2Bufs], IDR Chk[3Bufs]
        { 0,  0, 0, 1, 1}, // in frame 0 DC or IDR flagged
        { 0, -1, 0, 0, 0}, // in frame 1 DC or IDR flagged
        {-1, -1, 0, 0, 0}, // in frame 2 DC or IDR flagged
    },
};

static const t_EVppUbwcStatField aeDiUbwcField[HVX_DI_IN_BUF_CNT] = {
    eVppUbwcStatField_BF, eVppUbwcStatField_TF, eVppUbwcStatField_BF, eVppUbwcStatField_TF,
};

#define HVX_DI_BUF_CFG_LUT(BufType, NumInputs) \
    acu8DiBufCfgLut[ \
    (BufType == eVppBufType_Interleaved_BFF) ?  eHvxDiCfgLut_BFF : eHvxDiCfgLut_TFF] \
    [NumInputs > 0 && NumInputs < HVX_DI_IN_BUF_CNT ? NumInputs - 1 : 0]

#define HVX_DI_DC_CFG_LUT(BufType, Idx) \
    acs8DiExtCfgLut[ \
    (BufType == eVppBufType_Interleaved_BFF) ?  eHvxDiCfgLut_BFF : eHvxDiCfgLut_TFF] \
    [Idx < HVX_DI_IN_BUF_CNT - 1 ? Idx : 0][0]

#define HVX_DI_IDR_CFG_LUT(BufType, Idx) \
    acs8DiExtCfgLut[ \
    (BufType == eVppBufType_Interleaved_BFF) ?  eHvxDiCfgLut_BFF : eHvxDiCfgLut_TFF] \
    [Idx < HVX_DI_IN_BUF_CNT - 1 ? Idx : 0][1]

#define HVX_DI_IDR_CHK_LUT(BufType, Idx, NumInputs) \
    acs8DiExtCfgLut[ \
    (BufType == eVppBufType_Interleaved_BFF) ?  eHvxDiCfgLut_BFF : eHvxDiCfgLut_TFF] \
    [Idx < HVX_DI_IN_BUF_CNT - 1 ? Idx : 0] \
    [NumInputs > 0 && NumInputs < HVX_DI_IN_BUF_CNT ? 1 + NumInputs : 2]

#define HVX_DI_EXT_CPY_LUT(BufType, Idx, NumInputs) \
    (BufType == eVppBufType_Interleaved_TFF && NumInputs > 1 && Idx > 0) ? 1 : 0

static const t_StVppStatsConfig astHvxStatsCfg[] = {
    VPP_PROF_DECL(HVX_STAT_PROC, 10, 1),
    VPP_PROF_DECL(HVX_STAT_WORKER, 10, 0),
    VPP_PROF_DECL(HVX_STAT_WORKER_SLEEP, 10, 0),
};

static const uint32_t u32HvxStatCnt = VPP_STATS_CNT(astHvxStatsCfg);

#define HVX_PORT_PARAM_WIDTH_MIN    256
#define HVX_PORT_PARAM_HEIGHT_MIN   128

#define HVX_TUNING_LOAD(pstCb, eAlgoId) \
    if (bVppIpHvxCore_IsAlgoSupported(eAlgoId)) \
    { \
        uint32_t u32Err; \
        u32Err = u32VppIpHvx_TuningsAlgoLoad((pstCb), (eAlgoId)); \
        LOGE_IF(u32Err != VPP_OK, "Error opening tunings for " #eAlgoId "(%d), u32Err=%u", \
                (eAlgoId), u32Err); \
    }

#define HVX_TUNING_UNREGISTER(pstCb, eAlgoId) \
    if (bVppIpHvxCore_IsAlgoSupported(eAlgoId)) \
    { \
        uint32_t u32Err; \
        u32Err = u32VppIpHvxCore_TuningsAlgoUnregister((pstCb)->pstHvxCore, (eAlgoId)); \
        LOGE_IF(u32Err != VPP_OK, "Error unregistering tunings for " #eAlgoId "(%d), u32Err=%u", \
                (eAlgoId), u32Err); \
    }

/************************************************************************
 * Local static variables
 ***********************************************************************/

static const t_StCustomHvxParams stGlobalHvxParams =
{
    .stCustomAieParams = {
        .stAieDe = {
            .de_fDeGain0 = 1.00,
            .de_fDeOff0 = 0.00,
            .de_nDeMin0 = 4.00,
            .de_nDeMax0 = 32.00,
            .de_fDeGain1 = 0.50,
            .de_fDeOff1 = 0.00,
            .de_nDeMin1 = 6.00,
            .de_nDeMax1 = 32.00,
            .de_fDeGain2 = 1.50,
            .de_fDeOff2 = 0.00,
            .de_nDeMin2 = 2.00,
            .de_nDeMax2 = 32.00,
            .de_fDeGain3 = 0.25,
            .de_fDeOff3 = 0.00,
            .de_nDeMin3 = 10.00,
            .de_nDeMax3 = 32.00,
            .de_nDePower = 4.00,
            .de_fDeSlope = 1.00,
        },
        .stAieLtmCa = {
            .ltm_nCA_EN_LPF = 1,
            .stAieLtmCa_Y = {
                .ltm_CA_params_EN = 1,
                .ltm_CA_params_LMODE = 0,
                .ltm_CA_params_TMODE0 = 0,
                .ltm_CA_params_TMODE1 = 1,
                .ltm_CA_params_TSIGN0 = 1,
                .ltm_CA_params_TSIGN1 = 0,
                .ltm_CA_params_HMIN = 59,
                .ltm_CA_params_HMAX = 193,
                .ltm_CA_params_YMIN = 3,
                .ltm_CA_params_YMAX = 248,
                .ltm_CA_params_SMIN = 16,
                .ltm_CA_params_SMAX = 7946,
            },
            .stAieLtmCa_U = {
                .ltm_CA_params_EN = 1,
                .ltm_CA_params_LMODE = 0,
                .ltm_CA_params_TMODE0 = 1,
                .ltm_CA_params_TMODE1 = 1,
                .ltm_CA_params_TSIGN0 = 1,
                .ltm_CA_params_TSIGN1 = 0,
                .ltm_CA_params_HMIN = 0,
                .ltm_CA_params_HMAX = -215,
                .ltm_CA_params_YMIN = 3,
                .ltm_CA_params_YMAX = 248,
                .ltm_CA_params_SMIN = 16,
                .ltm_CA_params_SMAX = 7946,
            },
            .stAieLtmCa_V = {
                .ltm_CA_params_EN = 1,
                .ltm_CA_params_LMODE = 0,
                .ltm_CA_params_TMODE0 = 0,
                .ltm_CA_params_TMODE1 = 1,
                .ltm_CA_params_TSIGN0 = 0,
                .ltm_CA_params_TSIGN1 = 1,
                .ltm_CA_params_HMIN = 179,
                .ltm_CA_params_HMAX = 69,
                .ltm_CA_params_YMIN = 3,
                .ltm_CA_params_YMAX = 248,
                .ltm_CA_params_SMIN = 16,
                .ltm_CA_params_SMAX = 7946,
            },
        },
        .stAieLtm = {
            .ltm_nLTM_EN_MAP = 1,
            .ltm_nLTM_EN_SAT = 1,
            .ltm_nLTM_NUMX = 0,
            .ltm_nLTM_NUMY = 0,
            .ltm_nLTM_BITS_SIZEX = 8,
            .ltm_nLTM_BITS_SIZEY = 8,
            .ltm_nLTM_SAT_GAIN = 256,
            .ltm_nLTM_SAT_OFF = 256,
            .ltm_nLTM_SAT_THR = 12,
            .ltm_nCoefIIR = 64,
            .ltm_nThrDif = 115,
            .ltm_nLutCntBlend = 16,
            .ltm_nLutCoeffLPF = 255,
            .ltm_fTmCoeff = 0.85,
            .ltm_fLutCoeffIIR = 0.5,
            .ltm_fLutCoeffMinIIR = 0.1,
            .ltm_fLutThreshIIR = 0.25,
        },
        .stAieAce = {
            .ace_nStrCon = 128,
            .ace_nStrBriL = 0,
            .ace_nStrBriH = 0,
        },
    },
};

/************************************************************************
 * Forward Declarations
 ************************************************************************/
static uint32_t u32VppIpHvx_CmdPut(t_StVppIpHvxCb *pstCb, t_StVppIpCmd stCmd);
static void vVppIpHvx_InitParam(t_StVppIpHvxCb *pstCb);

#ifdef VPP_HVX_CRC_BUF_ENABLE
#define VPP_HVX_CRC_BUF(_pstCb,_pBuf) u32Vpp_CrcBuffer(_pBuf, eVppBuf_Pixel,\
                                 (_pBuf->stPixel.u32FilledLen / 2),\
                                 (_pBuf->stPixel.u32FilledLen / 8),\
                                 _pstCb->stats.u32InQueuedCnt, "VPP_FRC_HVX_CRC_BUF")
#else
#define VPP_HVX_CRC_BUF(_pstCb,_pBuf)
#endif

/************************************************************************
 * Local Functions
 ***********************************************************************/

static inline uint32_t HVX_MAX(uint32_t a, uint32_t b)
{
    return (a > b) ? a : b;
}

#define HVX_DUMP_NM_LEN 256
void vVppIpHvx_DumpFrame(t_StVppIpHvxCb *pstCb, t_StVppBuf *pstBuf,
                         enum vpp_port port)
{
    char cPath[HVX_DUMP_NM_LEN];
    struct vpp_port_param *pstParam;

    if (bVppIpHvxCore_IsSecure(pstCb->pstHvxCore))
        return;

    if (port == VPP_PORT_INPUT)
    {
        snprintf(cPath, HVX_DUMP_NM_LEN, HVX_DUMP_BUF_IN_NM, pstCb);
        pstParam = &pstCb->stInput.stParam;
    }
    else
    {
        snprintf(cPath, HVX_DUMP_NM_LEN, HVX_DUMP_BUF_OUT_NM, pstCb);
        pstParam = &pstCb->stOutput.stParam;
    }

    u32VppBuf_Dump(pstCb->stBase.pstCtx, pstBuf, *pstParam, cPath);
}

void vVppIpHvx_DumpFrameDesc(t_StVppIpHvxCb *pstCb, t_StVppBuf *pstBuf, char *str,
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
         "fd=%d, base=%p, virt=%p, uvOff=%u, sz=%u",
         str, pstParam->fmt, pstParam->width, pstParam->height,
         u32VppUtils_GetStride(pstParam), pstBuf->stPixel.fd,
         pstBuf->stPixel.priv.pvPa, pstBuf->stPixel.pvBase,
         u32VppUtils_GetUVOffset(pstParam), pstBuf->stPixel.u32AllocLen);
}

static void vVppIpHvx_ProcStateSet(t_StVppIpHvxCb *pstCb, t_EVppHvxState eState)
{
    t_EVppHvxState eStatePrev;

    if (!pstCb)
        return;

    if (pstCb->eHvxProcState != eState)
    {
        eStatePrev = pstCb->eHvxProcState;
        pstCb->eHvxProcState = eState;
        LOGI("HVX Proc state change from %u to %u", eStatePrev, eState);
    }
}

static uint32_t u32VppIpHvx_ReturnBuffer(t_StVppIpHvxCb *pstCb,
                                         enum vpp_port ePort,
                                         t_StVppBuf *pstBuf)
{
#ifdef HVX_REG_BUFFER_ON_QUEUE
    vVppIpHvxCore_UnregisterBuffer(pstCb->pstHvxCore, &pstBuf->stPixel);
#endif
    return u32VppIp_CbBufDone(&pstCb->stBase.stCb, ePort, pstBuf);
}

static void vVppIpHvx_InitParam(t_StVppIpHvxCb *pstCb)
{
    t_StHvxParam *pstHvxParams = &pstCb->stHvxParams;

    LOGI("%s()", __func__);

    memset(pstHvxParams, 0, sizeof(t_StHvxParam));

    // INIT AIE
    t_StCustomAieParams *pstCustomAieParams = &pstCb->stLocalHvxParams.stCustomAieParams;
    vpp_svc_aie_params_t *pstAieParams = &pstHvxParams->stAieParams;
    pstAieParams->ltm_nEN_DE             = 0;
    pstAieParams->ltm_nEN_CA             = 0;
    pstAieParams->de_fDeGain0            = pstCustomAieParams->stAieDe.de_fDeGain0;
    pstAieParams->de_fDeOff0             = pstCustomAieParams->stAieDe.de_fDeOff0;
    pstAieParams->de_nDeMin0             = pstCustomAieParams->stAieDe.de_nDeMin0;
    pstAieParams->de_nDeMax0             = pstCustomAieParams->stAieDe.de_nDeMax0;
    pstAieParams->de_fDeGain1            = pstCustomAieParams->stAieDe.de_fDeGain1;
    pstAieParams->de_fDeOff1             = pstCustomAieParams->stAieDe.de_fDeOff1;
    pstAieParams->de_nDeMin1             = pstCustomAieParams->stAieDe.de_nDeMin1;
    pstAieParams->de_nDeMax1             = pstCustomAieParams->stAieDe.de_nDeMax1;
    pstAieParams->de_fDeGain2            = pstCustomAieParams->stAieDe.de_fDeGain2;
    pstAieParams->de_fDeOff2             = pstCustomAieParams->stAieDe.de_fDeOff2;
    pstAieParams->de_nDeMin2             = pstCustomAieParams->stAieDe.de_nDeMin2;
    pstAieParams->de_nDeMax2             = pstCustomAieParams->stAieDe.de_nDeMax2;
    pstAieParams->de_fDeGain3            = pstCustomAieParams->stAieDe.de_fDeGain3;
    pstAieParams->de_fDeOff3             = pstCustomAieParams->stAieDe.de_fDeOff3;
    pstAieParams->de_nDeMin3             = pstCustomAieParams->stAieDe.de_nDeMin3;
    pstAieParams->de_nDeMax3             = pstCustomAieParams->stAieDe.de_nDeMax3;
    pstAieParams->de_nDePower            = pstCustomAieParams->stAieDe.de_nDePower;
    pstAieParams->de_fDeSlope            = pstCustomAieParams->stAieDe.de_fDeSlope;
    pstAieParams->ltm_nCA_EN_LPF         = pstCustomAieParams->stAieLtmCa.ltm_nCA_EN_LPF;
    pstAieParams->ltm_CA_params_EN_Y     = pstCustomAieParams->stAieLtmCa.stAieLtmCa_Y.ltm_CA_params_EN;
    pstAieParams->ltm_CA_params_LMODE_Y  = pstCustomAieParams->stAieLtmCa.stAieLtmCa_Y.ltm_CA_params_LMODE;
    pstAieParams->ltm_CA_params_TMODE0_Y = pstCustomAieParams->stAieLtmCa.stAieLtmCa_Y.ltm_CA_params_TMODE0;
    pstAieParams->ltm_CA_params_TMODE1_Y = pstCustomAieParams->stAieLtmCa.stAieLtmCa_Y.ltm_CA_params_TMODE1;
    pstAieParams->ltm_CA_params_TSIGN0_Y = pstCustomAieParams->stAieLtmCa.stAieLtmCa_Y.ltm_CA_params_TSIGN0;
    pstAieParams->ltm_CA_params_TSIGN1_Y = pstCustomAieParams->stAieLtmCa.stAieLtmCa_Y.ltm_CA_params_TSIGN1;
    pstAieParams->ltm_CA_params_HMIN_Y   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_Y.ltm_CA_params_HMIN;
    pstAieParams->ltm_CA_params_HMAX_Y   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_Y.ltm_CA_params_HMAX;
    pstAieParams->ltm_CA_params_YMIN_Y   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_Y.ltm_CA_params_YMIN;
    pstAieParams->ltm_CA_params_YMAX_Y   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_Y.ltm_CA_params_YMAX;
    pstAieParams->ltm_CA_params_SMIN_Y   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_Y.ltm_CA_params_SMIN;
    pstAieParams->ltm_CA_params_SMAX_Y   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_Y.ltm_CA_params_SMAX;
    pstAieParams->ltm_CA_params_EN_U     = pstCustomAieParams->stAieLtmCa.stAieLtmCa_U.ltm_CA_params_EN;
    pstAieParams->ltm_CA_params_LMODE_U  = pstCustomAieParams->stAieLtmCa.stAieLtmCa_U.ltm_CA_params_LMODE;
    pstAieParams->ltm_CA_params_TMODE0_U = pstCustomAieParams->stAieLtmCa.stAieLtmCa_U.ltm_CA_params_TMODE0;
    pstAieParams->ltm_CA_params_TMODE1_U = pstCustomAieParams->stAieLtmCa.stAieLtmCa_U.ltm_CA_params_TMODE1;
    pstAieParams->ltm_CA_params_TSIGN0_U = pstCustomAieParams->stAieLtmCa.stAieLtmCa_U.ltm_CA_params_TSIGN0;
    pstAieParams->ltm_CA_params_TSIGN1_U = pstCustomAieParams->stAieLtmCa.stAieLtmCa_U.ltm_CA_params_TSIGN1;
    pstAieParams->ltm_CA_params_HMIN_U   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_U.ltm_CA_params_HMIN;
    pstAieParams->ltm_CA_params_HMAX_U   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_U.ltm_CA_params_HMAX;
    pstAieParams->ltm_CA_params_YMIN_U   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_U.ltm_CA_params_YMIN;
    pstAieParams->ltm_CA_params_YMAX_U   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_U.ltm_CA_params_YMAX;
    pstAieParams->ltm_CA_params_SMIN_U   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_U.ltm_CA_params_SMIN;
    pstAieParams->ltm_CA_params_SMAX_U   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_U.ltm_CA_params_SMAX;
    pstAieParams->ltm_CA_params_EN_V     = pstCustomAieParams->stAieLtmCa.stAieLtmCa_V.ltm_CA_params_EN;
    pstAieParams->ltm_CA_params_LMODE_V  = pstCustomAieParams->stAieLtmCa.stAieLtmCa_V.ltm_CA_params_LMODE;
    pstAieParams->ltm_CA_params_TMODE0_V = pstCustomAieParams->stAieLtmCa.stAieLtmCa_V.ltm_CA_params_TMODE0;
    pstAieParams->ltm_CA_params_TMODE1_V = pstCustomAieParams->stAieLtmCa.stAieLtmCa_V.ltm_CA_params_TMODE1;
    pstAieParams->ltm_CA_params_TSIGN0_V = pstCustomAieParams->stAieLtmCa.stAieLtmCa_V.ltm_CA_params_TSIGN0;
    pstAieParams->ltm_CA_params_TSIGN1_V = pstCustomAieParams->stAieLtmCa.stAieLtmCa_V.ltm_CA_params_TSIGN1;
    pstAieParams->ltm_CA_params_HMIN_V   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_V.ltm_CA_params_HMIN;
    pstAieParams->ltm_CA_params_HMAX_V   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_V.ltm_CA_params_HMAX;
    pstAieParams->ltm_CA_params_YMIN_V   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_V.ltm_CA_params_YMIN;
    pstAieParams->ltm_CA_params_YMAX_V   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_V.ltm_CA_params_YMAX;
    pstAieParams->ltm_CA_params_SMIN_V   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_V.ltm_CA_params_SMIN;
    pstAieParams->ltm_CA_params_SMAX_V   = pstCustomAieParams->stAieLtmCa.stAieLtmCa_V.ltm_CA_params_SMAX;
    pstAieParams->ltm_nLTM_EN_MAP        = pstCustomAieParams->stAieLtm.ltm_nLTM_EN_MAP;
    pstAieParams->ltm_nLTM_EN_SAT        = pstCustomAieParams->stAieLtm.ltm_nLTM_EN_SAT;
    pstAieParams->ltm_nLTM_NUMX          = pstCustomAieParams->stAieLtm.ltm_nLTM_NUMX;
    pstAieParams->ltm_nLTM_NUMY          = pstCustomAieParams->stAieLtm.ltm_nLTM_NUMY;
    pstAieParams->ltm_nLTM_BITS_SIZEX    = pstCustomAieParams->stAieLtm.ltm_nLTM_BITS_SIZEX;
    pstAieParams->ltm_nLTM_BITS_SIZEY    = pstCustomAieParams->stAieLtm.ltm_nLTM_BITS_SIZEY;
    pstAieParams->ltm_nLTM_SAT_GAIN      = pstCustomAieParams->stAieLtm.ltm_nLTM_SAT_GAIN;
    pstAieParams->ltm_nLTM_SAT_OFF       = pstCustomAieParams->stAieLtm.ltm_nLTM_SAT_OFF;
    pstAieParams->ltm_nLTM_SAT_THR       = pstCustomAieParams->stAieLtm.ltm_nLTM_SAT_THR;
    pstAieParams->ltm_nCoefIIR           = pstCustomAieParams->stAieLtm.ltm_nCoefIIR;
    pstAieParams->ltm_nThrDif            = pstCustomAieParams->stAieLtm.ltm_nThrDif;
    pstAieParams->ltm_nLutCntBlend       = pstCustomAieParams->stAieLtm.ltm_nLutCntBlend;
    pstAieParams->ltm_nLutCoeffLPF       = pstCustomAieParams->stAieLtm.ltm_nLutCoeffLPF;
    pstAieParams->ltm_fTmCoeff           = pstCustomAieParams->stAieLtm.ltm_fTmCoeff;
    pstAieParams->ltm_fLutCoeffIIR       = pstCustomAieParams->stAieLtm.ltm_fLutCoeffIIR;
    pstAieParams->ltm_fLutCoeffMinIIR    = pstCustomAieParams->stAieLtm.ltm_fLutCoeffMinIIR;
    pstAieParams->ltm_fLutThreshIIR      = pstCustomAieParams->stAieLtm.ltm_fLutThreshIIR;
    pstAieParams->ace_nStrCon            = pstCustomAieParams->stAieAce.ace_nStrCon;
    pstAieParams->ace_nStrBriL           = pstCustomAieParams->stAieAce.ace_nStrBriL;
    pstAieParams->ace_nStrBriH           = pstCustomAieParams->stAieAce.ace_nStrBriH;

    return;
}

static void vVppIpHvx_InitCapabilityResources(t_StVppIpHvxCb *pstCb)
{
    uint32_t u32Cnt = 0;

    if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        u32VppIpHvxCore_AlgoInit(pstCb->pstHvxCore, u32Cnt++, VPP_FUNC_ID_MVP, NULL);

    if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_NR))
        u32VppIpHvxCore_AlgoInit(pstCb->pstHvxCore, u32Cnt++, VPP_FUNC_ID_NR, NULL);

    if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_IE))
        u32VppIpHvxCore_AlgoInit(pstCb->pstHvxCore, u32Cnt++, VPP_FUNC_ID_IE, NULL);
}

static void vVppIpHvx_Compute(t_StVppIpHvxCb *pstCb)
{
    t_StHvxParam *pstHvxParams;
    uint32_t u32Length, u32Index, u32Width, u32Height, u32Resolution;
    float fLevel;
    enum vpp_color_format eFmt;
    uint32_t u32Computed = 0;

    LOG_ENTER();

    if(!pstCb || !pstCb->pstHvxCore)
    {
        LOGE("Error. HVX or HVX Core control block is NULL.");
        return;
    }

    u32Computed = pstCb->stCfg.u32ComputeMask;
    pstHvxParams = &pstCb->stHvxParams;
    u32Width = pstCb->stInput.stParam.width;
    u32Height = pstCb->stInput.stParam.height;
    eFmt = pstCb->stInput.stParam.fmt;
    u32Resolution = u32VppUtils_GetVppResolution(&pstCb->stInput.stParam);

    if (pstCb->stCfg.u32ComputeMask & HVX_PARAM)
    {
        u32VppIpHvxCore_BufParamInit(pstCb->pstHvxCore, u32Width, u32Height, eFmt);

        if (pstCb->stCfg.u32AutoHqvEnable)
        {
            if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
            {
                pstCb->stCfg.u32EnableMask |= HVX_ALGO_DI;
                pstCb->stCfg.stDi.mode = HQV_DI_MODE_AUTO;
            }

            if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_NR))
            {
                pstCb->stCfg.u32EnableMask |= HVX_ALGO_CNR;
                pstCb->stCfg.stCnr.mode = HQV_MODE_AUTO;
            }

            if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_IE))
            {
                pstCb->stCfg.u32EnableMask |= HVX_ALGO_AIE;
                pstCb->stCfg.stAie.mode = HQV_MODE_AUTO;
            }
            pstCb->stCfg.u32EnableMask &= ~HVX_ALGO_TNR;


            if (u32Resolution > VPP_RESOLUTION_FHD)
            {
                pstCb->stCfg.u32EnableMask &= ~HVX_ALGO_DI;
                pstCb->stCfg.u32EnableMask &= ~HVX_ALGO_CNR;
            }
        }

        if (pstCb->stCfg.u32EnableMask & HVX_ALGO_DI)
            pstCb->stCfg.u32ComputeMask |= HVX_ALGO_DI;
        if (pstCb->stCfg.u32EnableMask & HVX_ALGO_CNR)
            pstCb->stCfg.u32ComputeMask |= HVX_ALGO_CNR;
        if (pstCb->stCfg.u32EnableMask & HVX_ALGO_TNR)
            pstCb->stCfg.u32ComputeMask |= HVX_ALGO_TNR;
        if (pstCb->stCfg.u32EnableMask & HVX_ALGO_AIE)
            pstCb->stCfg.u32ComputeMask |= HVX_ALGO_AIE;
        if (pstCb->stCfg.u32EnableMask & HVX_ALGO_ROI)
            pstCb->stCfg.u32ComputeMask |= HVX_ALGO_ROI;

        pstCb->stCfg.u32ComputeMask &= ~HVX_PARAM;
    }

    if (pstCb->stCfg.u32EnableMask & HVX_ALGO_ROI)
    {
        u32VppIpHvxCore_SvcParamSetROI(pstCb->pstHvxCore, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        if (pstCb->stCfg.u32ComputeMask & HVX_ALGO_ROI)
        {
            uint32_t u32XStart, u32YStart, u32XEnd, u32YEnd, u32LineWidth, u32Luma, u32Cr, u32Cb;

            if (u32Resolution > VPP_RESOLUTION_FHD)
                u32LineWidth = 16;
            else if (u32Resolution > VPP_RESOLUTION_HD)
                u32LineWidth = 8;
            else if (u32Resolution > VPP_RESOLUTION_SD)
                u32LineWidth = 4;
            else
                u32LineWidth = 2;
            u32Luma = 0x00;  // Green
            u32Cr = 0x00;
            u32Cb = 0x00;

            switch(pstCb->stCfg.stDemo.process_direction)
            {
                case HQV_SPLIT_RIGHT_TO_LEFT:
                    u32XStart = u32Width - ((pstCb->stCfg.stDemo.process_percent * u32Width) /
                        HVX_ROI_PERCENTAGE_MAX);
                    u32YStart = 0;
                    u32XEnd = u32Width;
                    u32YEnd = u32Height;
                    break;
                case HQV_SPLIT_TOP_TO_BOTTOM:
                    u32XStart = 0;
                    u32YStart = 0;
                    u32XEnd = u32Width;
                    u32YEnd = ((pstCb->stCfg.stDemo.process_percent * u32Height) /
                        HVX_ROI_PERCENTAGE_MAX);
                    break;
                case HQV_SPLIT_BOTTOM_TO_TOP:
                    u32XStart = 0;
                    u32YStart = u32Height - ((pstCb->stCfg.stDemo.process_percent * u32Height) /
                        HVX_ROI_PERCENTAGE_MAX);
                    u32XEnd = u32Width;
                    u32YEnd = u32Height;
                    break;
                case HQV_SPLIT_LEFT_TO_RIGHT:
                default:
                    u32XStart = 0;
                    u32YStart = 0;
                    u32XEnd = ((pstCb->stCfg.stDemo.process_percent * u32Width) /
                        HVX_ROI_PERCENTAGE_MAX);
                    u32YEnd = u32Height;
                    break;
            }
            u32VppIpHvxCore_SvcParamSetROI(pstCb->pstHvxCore, 1, 1, u32XStart, u32YStart,
                                           u32XEnd, u32YEnd, u32LineWidth, u32Luma, u32Cr, u32Cb);
            pstCb->stCfg.u32ComputeMask &= ~HVX_ALGO_ROI;
            LOGD("ROI X0:%d, Y0:%d, X1:%d, Y1:%d", u32XStart, u32YStart, u32XEnd, u32YEnd);
        }
    }
    else
    {
        u32VppIpHvxCore_SvcParamSetROI(pstCb->pstHvxCore, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    u32Index = 0;
    u32Length = 0;
    if (pstCb->stCfg.u32EnableMask & HVX_ALGO_DI)
    {
        u32VppIpHvxCore_SvcParamSetHeaderIdxAlgo(pstCb->pstHvxCore, u32Index, VPP_FUNC_ID_MVP);
        pstCb->pstMvpHeader = pstVppIpHvxCore_SvcParamGetHeaderIdxAddr(pstCb->pstHvxCore, u32Index);
        u32Index++;
        pstCb->pstMvpParams = (vpp_svc_mvp_params_t*)
            (pvVppIpHvxCore_SvcParamGetDataOffsetAddr(pstCb->pstHvxCore, u32Length));
        pstHvxParams->u32MvpEn = 1;

        if (pstCb->stCfg.u32ComputeMask & HVX_ALGO_DI)
        {
            pstHvxParams->stMvpParams.in_frame_width = u32Width;
            pstHvxParams->stMvpParams.in_frame_height = u32Height / 2;
            pstHvxParams->stMvpParams.update_flags = 1;

            if (pstCb->stCfg.stDi.mode == HQV_DI_MODE_AUTO)
                pstHvxParams->stMvpParams.mode = MVP_MODE_AUTO;
            else if (pstCb->stCfg.stDi.mode == HQV_DI_MODE_VIDEO_3F)
                pstHvxParams->stMvpParams.mode = MVP_MODE_3FIELD;
            else //HQV_DI_MODE_VIDEO_1F
                pstHvxParams->stMvpParams.mode = MVP_MODE_1FIELD;

            pstCb->stCfg.u32ComputeMask &= ~HVX_ALGO_DI;
            LOGD("Set MVP input parameters: mode: %d", pstHvxParams->stMvpParams.mode);
        }
        memcpy(pstCb->pstMvpParams, &pstHvxParams->stMvpParams, sizeof(vpp_svc_mvp_params_t));
        u32Length += sizeof(vpp_svc_mvp_params_t);
    }
    else
    {
        pstHvxParams->u32MvpEn = 0;
        pstCb->u32InternalFlags &= ~IP_FLAG_DI_ALGO_RUN; // Clear flag (non-DI uc)
    }

    if (pstCb->stCfg.u32EnableMask & HVX_ALGO_CNR)
    {
        u32VppIpHvxCore_SvcParamSetHeaderIdxAlgo(pstCb->pstHvxCore, u32Index, VPP_FUNC_ID_NR);
        pstCb->pstNrHeader = pstVppIpHvxCore_SvcParamGetHeaderIdxAddr(pstCb->pstHvxCore, u32Index);
        u32Index++;
        pstCb->pstNrParams = (vpp_svc_nr_params_t*)
            (pvVppIpHvxCore_SvcParamGetDataOffsetAddr(pstCb->pstHvxCore, u32Length));

        if (pstCb->stCfg.u32ComputeMask & HVX_ALGO_CNR)
        {
            pstHvxParams->stNrParams.update_flags = 1;
            pstHvxParams->stNrParams.in_frame_width = u32Width;
            pstHvxParams->stNrParams.in_frame_height = u32Height;
            if (pstCb->stCfg.stCnr.mode == HQV_MODE_AUTO)
            {
                pstHvxParams->stNrParams.mode = NR_MODE_AUTO;
            }
            else
            {
                pstHvxParams->stNrParams.mode = NR_MODE_MANUAL;
                pstHvxParams->stNrParams.level = pstCb->stCfg.stCnr.level;
            }
            pstCb->stCfg.u32ComputeMask &= ~HVX_ALGO_CNR;
            LOGD("Set NR input parameters: mode: %d", pstHvxParams->stNrParams.mode);
        }

        memcpy(pstCb->pstNrParams, &pstHvxParams->stNrParams, sizeof(vpp_svc_nr_params_t));
        u32Length += sizeof(vpp_svc_nr_params_t);
    }

    if (pstCb->stCfg.u32EnableMask & HVX_ALGO_AIE)
    {
        u32VppIpHvxCore_SvcParamSetHeaderIdxAlgo(pstCb->pstHvxCore, u32Index, VPP_FUNC_ID_IE);
        pstCb->pstAieHeader = pstVppIpHvxCore_SvcParamGetHeaderIdxAddr(pstCb->pstHvxCore, u32Index);
        u32Index++;
        pstCb->pstAieParams = (vpp_svc_aie_params_t*)
            (pvVppIpHvxCore_SvcParamGetDataOffsetAddr(pstCb->pstHvxCore, u32Length));

        if (pstCb->stCfg.u32ComputeMask & HVX_ALGO_AIE)
        {
            pstHvxParams->stAieParams.update_flags = 1;
            pstHvxParams->stAieParams.in_frame_width = u32Width;
            pstHvxParams->stAieParams.in_frame_height = u32Height;

            // Need to always set to manual, since this is not handled in the
            // firmware.
            pstHvxParams->stAieParams.mode = AIE_MODE_MANUAL;

            if (pstCb->stCfg.stAie.mode == HQV_MODE_AUTO)
            {
                t_StCustomAieParams *pstLocalAieParams = &pstCb->stLocalHvxParams.stCustomAieParams;
                // CADE
                pstHvxParams->stAieParams.ltm_nEN_DE = 1;
                pstHvxParams->stAieParams.de_fDeGain0 = pstLocalAieParams->stAieDe.de_fDeGain0;
                pstHvxParams->stAieParams.de_fDeGain1 = pstLocalAieParams->stAieDe.de_fDeGain1;
                pstHvxParams->stAieParams.de_fDeGain2 = pstLocalAieParams->stAieDe.de_fDeGain2;
                pstHvxParams->stAieParams.de_fDeGain3 = pstLocalAieParams->stAieDe.de_fDeGain3;

                // Hue
                pstHvxParams->stAieParams.ltm_nEN_CA = 1;

                // LTM
                pstHvxParams->stAieParams.ltm_nLTM_EN_MAP = pstLocalAieParams->stAieLtm.ltm_nLTM_EN_MAP;
                pstHvxParams->stAieParams.ace_nStrCon = pstLocalAieParams->stAieAce.ace_nStrCon;
                pstHvxParams->stAieParams.ace_nStrBriL = pstLocalAieParams->stAieAce.ace_nStrBriL;
                pstHvxParams->stAieParams.ace_nStrBriH = pstLocalAieParams->stAieAce.ace_nStrBriH;
                pstHvxParams->stAieParams.ltm_nLTM_SAT_GAIN = pstLocalAieParams->stAieLtm.ltm_nLTM_SAT_GAIN;
                pstHvxParams->stAieParams.ltm_nLTM_SAT_OFF = pstLocalAieParams->stAieLtm.ltm_nLTM_SAT_OFF;
            }
            else
            {
                // CADE + HUE
                fLevel = fVppUtils_ScaleFloat(AIE_CADE_LEVEL_MIN, AIE_CADE_LEVEL_MAX,
                                              HVX_AIE_CADE_DE0_LEVEL_MIN, HVX_AIE_CADE_DE0_LEVEL_MAX,
                                              (float)pstCb->stCfg.stAie.cade_level);
                pstHvxParams->stAieParams.de_fDeGain0 = fLevel;
                fLevel = fVppUtils_ScaleFloat(AIE_CADE_LEVEL_MIN, AIE_CADE_LEVEL_MAX,
                                              HVX_AIE_CADE_DE1_LEVEL_MIN, HVX_AIE_CADE_DE1_LEVEL_MAX,
                                              (float)pstCb->stCfg.stAie.cade_level);
                pstHvxParams->stAieParams.de_fDeGain1 = fLevel;
                fLevel = fVppUtils_ScaleFloat(AIE_CADE_LEVEL_MIN, AIE_CADE_LEVEL_MAX,
                                              HVX_AIE_CADE_DE2_LEVEL_MIN, HVX_AIE_CADE_DE2_LEVEL_MAX,
                                              (float)pstCb->stCfg.stAie.cade_level);
                pstHvxParams->stAieParams.de_fDeGain2 = fLevel;
                fLevel = fVppUtils_ScaleFloat(AIE_CADE_LEVEL_MIN, AIE_CADE_LEVEL_MAX,
                                              HVX_AIE_CADE_DE3_LEVEL_MIN, HVX_AIE_CADE_DE3_LEVEL_MAX,
                                              (float)pstCb->stCfg.stAie.cade_level);
                pstHvxParams->stAieParams.de_fDeGain3 = fLevel;
                if (pstCb->stCfg.stAie.cade_level != AIE_CADE_LEVEL_MIN)
                {
                    pstHvxParams->stAieParams.ltm_nEN_DE = 1;
                    if (pstCb->stCfg.stAie.hue_mode == HQV_HUE_MODE_ON)
                        pstHvxParams->stAieParams.ltm_nEN_CA = 1;
                    else
                        pstHvxParams->stAieParams.ltm_nEN_CA = 0;
                }
                else
                {
                    pstHvxParams->stAieParams.ltm_nEN_DE = 0;
                    pstHvxParams->stAieParams.ltm_nEN_CA = 0;
                }

                // LTM
                pstHvxParams->stAieParams.ltm_nLTM_EN_MAP = 1;
                pstHvxParams->stAieParams.ace_nStrCon =
                    (pstCb->stCfg.stAie.ltm_ace_str * HVX_AIE_LTM_CON_LEVEL_MAX) / AIE_LTM_ACE_STR_MAX;
                pstHvxParams->stAieParams.ace_nStrBriL =
                    (pstCb->stCfg.stAie.ltm_ace_bright_l * HVX_AIE_LTM_BRI_LEVEL_MAX) / AIE_LTM_ACE_BRI_L_MAX;
                pstHvxParams->stAieParams.ace_nStrBriH =
                    (pstCb->stCfg.stAie.ltm_ace_bright_h * HVX_AIE_LTM_BRI_LEVEL_MAX) / AIE_LTM_ACE_BRI_H_MAX;
                pstHvxParams->stAieParams.ltm_nLTM_SAT_GAIN =
                    (pstCb->stCfg.stAie.ltm_sat_gain * HVX_AIE_LTM_SAT_GAIN_MAX) / AIE_LTM_SAT_GAIN_MAX;
                pstHvxParams->stAieParams.ltm_nLTM_SAT_OFF =
                    (pstCb->stCfg.stAie.ltm_sat_offset * HVX_AIE_LTM_SAT_OFFSET_MAX) / AIE_LTM_SAT_OFFSET_MAX;
            }
            pstCb->stCfg.u32ComputeMask &= ~HVX_ALGO_AIE;

            LOGD("Set AIE input parameters: mode: %d",
                  (int)pstHvxParams->stAieParams.mode);
        }

        memcpy(pstCb->pstAieParams, &pstHvxParams->stAieParams, sizeof(vpp_svc_aie_params_t));
        u32Length += sizeof(vpp_svc_aie_params_t);
    }

    u32VppIpHvxCore_SvcParamSetDataSize(pstCb->pstHvxCore, u32Index, u32Length);

    if (u32Computed)
    {
        u32VppIpHvxCore_BuffInCompute(pstCb->pstHvxCore);
        u32VppIpHvxCore_BuffOutCompute(pstCb->pstHvxCore);
    }

    return;
}

static uint32_t u32VppIpHvx_CmdGet(t_StVppIpHvxCb *pstCb, t_StVppIpCmd *pstCmd)
{
    LOGI("%s()", __func__);
    int32_t idx;
    idx = vpp_queue_dequeue(&pstCb->stCmdQ);

    if (idx < 0)
    {
        LOGD("%s(): idx=%d not correct.", __func__, idx);
        return VPP_ERR;
    }
    else
    {
        *pstCmd = pstCb->astCmdNode[idx];
        LOG_CMD("GetCmd", pstCmd->eCmd);
    }

    return VPP_OK;
}

static uint32_t u32VppIpHvx_CmdPut(t_StVppIpHvxCb *pstCb, t_StVppIpCmd stCmd)
{
    int32_t idx;
    uint32_t u32Ret = VPP_OK;

    pthread_mutex_lock(&pstCb->mutex);

    LOG_CMD("InsertCmd", stCmd.eCmd);
    idx = vpp_queue_enqueue(&pstCb->stCmdQ);
    if (idx < 0)
    {
        LOGE("%s(): idx=%d not correct.", __func__, idx);
        u32Ret = VPP_ERR;
    } else
    {
        pstCb->astCmdNode[idx] = stCmd;
        pthread_cond_signal(&pstCb->cond);
    }

    pthread_mutex_unlock(&pstCb->mutex);

    return u32Ret;
}

static void vVppIpHvx_SignalWorkerStart(t_StVppIpHvxCb *pstCb)
{
    pthread_mutex_lock(&pstCb->mutex);

    pstCb->u32InternalFlags |= IP_WORKER_STARTED;

    pthread_cond_signal(&pstCb->cond);

    pthread_mutex_unlock(&pstCb->mutex);
}

static void vVppIpHvx_WaitWorkerStart(t_StVppIpHvxCb *pstCb)
{
    pthread_mutex_lock(&pstCb->mutex);

    while (!(pstCb->u32InternalFlags & IP_WORKER_STARTED))
        pthread_cond_wait(&pstCb->cond, &pstCb->mutex);

    pthread_mutex_unlock(&pstCb->mutex);
}

static uint32_t u32ProcBufReqMet(t_StVppIpHvxCb *pstCb)
{
    // Determine if the buffers in the ports satisfy the requirements
    // to trigger processing
    uint32_t i, u32InQSz, u32OutQSz;

    // This function requires that the caller has already locked the mutex
    // which guards these two queues.
    u32InQSz  = u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ);
    if (u32InQSz == 0)
        return VPP_FALSE;

    //code to check buffer type of interlaced or progressive and decide if DI
    //and other algo need to be bypassed
    t_StVppBuf* pstBufIn[HVX_DI_IN_BUF_CNT]; // DI is worst case in req bufs
    for (i = 0; i < HVX_DI_IN_BUF_CNT; i++)
        pstBufIn[i] = pstVppBufPool_Peek(&pstCb->stInput.stPendingQ, i);

    if (!pstBufIn[0])
        return VPP_FALSE;

    pstCb->u32InputIntoProc = pstCb->u32InputMaxIntoProc;
    pstCb->u32InputOutofProc = HVX_FW_DEFAULT_IN_BUF_RELEASE;
    pstCb->u32OutputIntoProc = pstCb->u32OutputMaxIntoProc;
    pstCb->u32OutputOutofProc = pstCb->u32OutputMaxIntoProc;

    //when pstBufIn[0] is progressive and DI and one or more other
    //algo(CNR, AIE) are enabled, we disable DI but still run other algo.
    if (VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, (HVX_ALGO_CNR | HVX_ALGO_AIE)) &&
        VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, HVX_ALGO_DI) &&
        (pstBufIn[0]->eBufType == eVppBufType_Progressive))
    {
        pstCb->u32InputIntoProc  = HVX_FW_DEFAULT_IN_BUF_NEED;
        pstCb->u32InputOutofProc = HVX_FW_DEFAULT_IN_BUF_RELEASE;
        pstCb->u32OutputIntoProc = HVX_FW_DEFAULT_OUT_BUF_NEED;
        pstCb->u32OutputOutofProc = HVX_FW_DEFAULT_OUT_BUF_RELEASE;

        //LOGD("DI+Other algo and In frame progressive. nmb_buffers set to 1_1_1_1");
    }

    // Bypass cases
    if (VPP_FLAG_IS_SET(pstBufIn[0]->u32InternalFlags, VPP_BUF_FLAG_BYPASS) ||
        VPP_FLAG_IS_SET(pstBufIn[0]->u32InternalFlags, VPP_BUF_FLAG_INTERNAL_BYPASS))
    {
        LOGI("CHECK: ReqMet due to bypass flag or EOS as first buffer");
        return VPP_TRUE;
    }

    if (pstBufIn[0]->eBufType != eVppBufType_Progressive &&
        pstBufIn[0]->eBufType != eVppBufType_Interleaved_TFF &&
        pstBufIn[0]->eBufType != eVppBufType_Interleaved_BFF)
    {
        LOGI("CHECK: ReqMet due to unsupported buffer type");
        return VPP_TRUE;
    }

    if (!VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, (HVX_ALGO_CNR | HVX_ALGO_AIE)) &&
        VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, HVX_ALGO_DI) &&
        (pstBufIn[0]->eBufType == eVppBufType_Progressive))
    {
        LOGI("CHECK: ReqMet progressive bypass");
        return VPP_TRUE;
    }

    if (!VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, HVX_ALGO_DI) &&
        (pstBufIn[0]->eBufType == eVppBufType_Interleaved_TFF ||
        pstBufIn[0]->eBufType == eVppBufType_Interleaved_BFF))
    {
        LOGI("CHECK: ReqMet interleaved bypass");
        return VPP_TRUE;
    }

    u32OutQSz = u32VppBufPool_Cnt(&pstCb->stOutput.stPendingQ);
    if(u32OutQSz == 0)
        return VPP_FALSE;

    LOGI("CHECK: u32InQSz=%u, u32OutQSz=%u, u32InputIntoProc=%u, u32OutputIntoProc=%u",
         u32InQSz, u32OutQSz, pstCb->u32InputIntoProc, pstCb->u32OutputIntoProc);

    if (pstCb->u32InputIntoProc && pstCb->u32OutputIntoProc &&
        u32InQSz >= pstCb->u32InputIntoProc && u32OutQSz >= pstCb->u32OutputIntoProc)
    {
        LOGI("CHECK: ReqMet due to queue'd buffers");
        return VPP_TRUE;
    }

    if((pstCb->u32InternalFlags & IP_DRAIN_PENDING) &&
       (u32InQSz >= 1) && (u32OutQSz >= pstCb->u32OutputIntoProc))
    {
        LOGI("CHECK: ReqMet due to DRAIN");
        return VPP_TRUE;
    }

    for (i = 0; i < pstCb->u32InputIntoProc; i++)
    {
        if (pstBufIn[i] != NULL &&
            VPP_FLAG_IS_SET(pstBufIn[i]->pBuf->flags, VPP_BUFFER_FLAG_EOS) &&
            u32OutQSz >= pstCb->u32OutputIntoProc)
        {
            LOGI("CHECK: ReqMet due to EOS in buffer %d", i+1);
            return VPP_TRUE;
        }

        if (pstBufIn[i] != NULL &&
            VPP_FLAG_IS_SET(pstBufIn[i]->u32InternalFlags,
                            VPP_BUF_FLAG_BYPASS | VPP_BUF_FLAG_INTERNAL_BYPASS) &&
            u32OutQSz >= pstCb->u32OutputIntoProc)
        {
            LOGI("CHECK: ReqMet due to BYPASS in buffer %d", i+1);
            return VPP_TRUE;
        }
    }

    LOGI("CHECK: ReqMet return VPP_FALSE");
    return VPP_FALSE;
}

static uint32_t u32WorkerThreadShouldSleep(t_StVppIpHvxCb *pstCb)
{
    uint32_t u32Ret = VPP_TRUE;
    uint32_t u32CmdQSz, u32ProcMet;

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

    u32ProcMet = u32ProcBufReqMet(pstCb);
    if (pstCb->eState == VPP_IP_STATE_ACTIVE && u32ProcMet)
    {
        u32Ret = VPP_FALSE;
        LOGI("CHECK: shouldSleep=VPP_FALSE, u32ProcMet=%u", u32ProcMet);
        return u32Ret;
    }

    LOGI("CHECK: shouldSleep=%u, u32CmdQSz=%u, u32ProcMet=%u", u32Ret,
         u32CmdQSz, u32ProcMet);

    return u32Ret;
}

static uint32_t u32VppIpHvx_ValidateCtrl(struct hqv_control stCtrl)
{
    uint32_t u32Ret = VPP_OK;
    struct hqv_ctrl_aie aie;
    struct hqv_ctrl_cnr cnr;
    struct hqv_ctrl_di di;
    struct hqv_ctrl_global_demo demo;

    if (stCtrl.mode != HQV_MODE_OFF &&
        stCtrl.mode != HQV_MODE_AUTO &&
        stCtrl.mode != HQV_MODE_MANUAL)
    {
        LOGE("Invalid stCtrl.mode=%u", stCtrl.mode);
        return VPP_ERR_PARAM;
    }

    if (stCtrl.mode == HQV_MODE_MANUAL)
    {
        if (stCtrl.ctrl_type == HQV_CONTROL_AIE)
        {
            aie = stCtrl.aie;
            if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_IE))
            {
                LOGE("ctrl_type unsupported, type=%u", stCtrl.ctrl_type);
                return VPP_ERR_PARAM;
            }
            if (aie.mode != HQV_MODE_OFF && aie.mode != HQV_MODE_AUTO &&
                aie.mode != HQV_MODE_MANUAL)
            {
                LOGE("invalid AIE mode=%u", aie.mode);
                return VPP_ERR_PARAM;
            }
            if (aie.mode == HQV_MODE_MANUAL)
            {
                if (aie.hue_mode != HQV_HUE_MODE_OFF &&
                    aie.hue_mode != HQV_HUE_MODE_ON)
                {
                    LOGE("invalid AIE hue mode=%u", aie.hue_mode);
                    return VPP_ERR_PARAM;
                }
                if (OOB(aie.cade_level, AIE_CADE_LEVEL_MIN, AIE_CADE_LEVEL_MAX) ||
                    OOB(aie.ltm_sat_gain, AIE_LTM_SAT_GAIN_MIN, AIE_LTM_SAT_GAIN_MAX) ||
                    OOB(aie.ltm_sat_offset, AIE_LTM_SAT_OFFSET_MIN, AIE_LTM_SAT_OFFSET_MAX) ||
                    OOB(aie.ltm_ace_str, AIE_LTM_ACE_STR_MIN, AIE_LTM_ACE_STR_MAX) ||
                    OOB(aie.ltm_ace_bright_l, AIE_LTM_ACE_BRI_L_MIN, AIE_LTM_ACE_BRI_L_MAX) ||
                    OOB(aie.ltm_ace_bright_h, AIE_LTM_ACE_BRI_H_MIN, AIE_LTM_ACE_BRI_H_MAX))
                {
                    LOGE("invalid AIE params, cade_level=%u, ltm_sat_gain=%u, "
                         "ltm_sat_offset=%u, ltm_ace_str=%u, "
                         "ltm_ace_bright_l=%u, ltm_ace_bright_h=%u",
                         aie.cade_level, aie.ltm_sat_gain, aie.ltm_sat_offset,
                         aie.ltm_ace_str, aie.ltm_ace_bright_l,
                         aie.ltm_ace_bright_h);
                    return VPP_ERR_PARAM;
                }
            }
        }
        else if (stCtrl.ctrl_type == HQV_CONTROL_CNR)
        {
            cnr = stCtrl.cnr;
            if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_NR))
            {
                LOGE("ctrl_type unsupported, type=%u", stCtrl.ctrl_type);
                return VPP_ERR_PARAM;
            }
            if (cnr.mode != HQV_MODE_OFF && cnr.mode != HQV_MODE_AUTO &&
                cnr.mode != HQV_MODE_MANUAL)
            {
                LOGE("invalid cnr mode=%u", cnr.mode);
                return VPP_ERR_PARAM;
            }
            if (OOB(cnr.level, CNR_LEVEL_MIN, CNR_LEVEL_MAX))
            {
                LOGE("invalid cnr params, level=%u", cnr.level);
                return VPP_ERR_PARAM;
            }
        }
        else if (stCtrl.ctrl_type == HQV_CONTROL_DI)
        {
            di = stCtrl.di;
            if (!bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
            {
                LOGE("ctrl_type unsupported, type=%u", stCtrl.ctrl_type);
                return VPP_ERR_PARAM;
            }
            if (di.mode != HQV_DI_MODE_OFF && di.mode != HQV_DI_MODE_VIDEO_1F &&
                di.mode != HQV_DI_MODE_VIDEO_3F && di.mode != HQV_DI_MODE_AUTO)
            {
                LOGE("invalid di mode=%u", di.mode);
                return VPP_ERR_PARAM;
            }
        }
        else if (stCtrl.ctrl_type == HQV_CONTROL_GLOBAL_DEMO)
        {
            demo = stCtrl.demo;
            if (demo.process_direction != HQV_SPLIT_LEFT_TO_RIGHT &&
                demo.process_direction != HQV_SPLIT_RIGHT_TO_LEFT &&
                demo.process_direction != HQV_SPLIT_TOP_TO_BOTTOM &&
                demo.process_direction != HQV_SPLIT_BOTTOM_TO_TOP)
            {
                LOGE("invalid demo direction=%u", demo.process_direction);
                return VPP_ERR_PARAM;
            }
            if (OOB(demo.process_percent, SPLIT_PERCENT_MIN, SPLIT_PERCENT_MAX))
            {
                LOGE("invalid demo params, percent=%u", demo.process_percent);
                return VPP_ERR_PARAM;
            }
        }
        else
        {
            LOGE("ctrl_type unsupported, type=%u", stCtrl.ctrl_type);
            return VPP_ERR_PARAM;
        }
    }

    return u32Ret;
}

static uint32_t u32VppIpHvx_ValidatePortParam(struct vpp_port_param stParam)
{
    if (stParam.width < HVX_PORT_PARAM_WIDTH_MIN)
    {
        LOGE("validation failed: width=%u, min=%u", stParam.width,
             HVX_PORT_PARAM_WIDTH_MIN);
        return VPP_ERR;
    }
    if (stParam.height < HVX_PORT_PARAM_HEIGHT_MIN)
    {
        LOGE("validation failed: height=%u, min=%u", stParam.height,
             HVX_PORT_PARAM_HEIGHT_MIN);
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

    if (stParam.fmt == VPP_COLOR_FORMAT_NV12_VENUS ||
        stParam.fmt == VPP_COLOR_FORMAT_NV21_VENUS)
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

static uint32_t u32VppIpHvx_ValidateConfig(struct vpp_port_param stInput,
                                           struct vpp_port_param stOutput)
{
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
    if (stInput.fmt != stOutput.fmt)
    {
        LOGE("validation failed: fmt, input: %u, output: %u",
             stInput.fmt, stOutput.fmt);
        return VPP_ERR;
    }
    if (u32VppIpHvx_ValidatePortParam(stInput))
        return VPP_ERR;
    if (u32VppIpHvx_ValidatePortParam(stOutput))
        return VPP_ERR;

    return VPP_OK;
}

static uint32_t u32VppIpHvx_GetSupportedProcFlags()
{
    uint32_t u32VppProcFlags = 0;

    //Determine which VPP Functions to enable
    if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
        u32VppProcFlags |= (1 << VPP_FUNC_ID_MVP);
    if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_NR))
        u32VppProcFlags |= (1 << VPP_FUNC_ID_NR);
    if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_IE))
        u32VppProcFlags |= (1 << VPP_FUNC_ID_IE);

    return u32VppProcFlags;
}

static uint32_t u32VppIpHvx_TuningsAlgoLoad(t_StVppIpHvxCb *pstCb, vpp_svc_vpp_func_id_t eAlgoId)
{
    uint32_t u32Err, u32Ret = VPP_OK;

    u32Err = u32VppIpHvxCore_TuningsAlgoLoad(pstCb->pstHvxCore, eAlgoId);
    if (u32Err != VPP_OK)
    {
        LOGE("No tunings loaded for eAlgoId=%d, u32Err=%u", eAlgoId, u32Err);
        u32Ret = u32Err;
    }

    u32Err = u32VppIpHvxCore_TuningsAlgoRegister(pstCb->pstHvxCore, eAlgoId);
    if (u32Err != VPP_OK)
    {
        LOGE("Error registering tunings for eAlgoId=%d, u32Err=%u", eAlgoId, u32Err);
        u32Ret = u32Err;
    }

    return u32Ret;
}

//#define USE_MALLOC //for debug only
static uint32_t u32VppIpHvx_ProcCmdOpen(t_StVppIpHvxCb *pstCb)
{
    uint32_t u32Ret;
    uint32_t u32VppProcFlags;
    uint32_t u32FrameSizeBytes = 0;
    uint32_t u32NumBuffers;
    vpp_svc_inout_buf_req_t stBufReq;

    LOG_ENTER_ARGS("pstCb->stBase.pstCtx = %p", pstCb->stBase.pstCtx);

    pthread_mutex_lock(&pstCb->mutex);

    u32NumBuffers = HVX_MAX(HVX_DI_IN_BUF_CNT, HVX_OTHER_IN_BUF_CNT);
    u32Ret = u32VppIpHvxCore_BuffInInit(pstCb->pstHvxCore, u32NumBuffers);
    if (u32Ret != VPP_OK)
    {
        LOGE("Error allocating bufferIn pixel_data");
        goto ERR_ALLOC_INPUT;
    }

    u32NumBuffers = HVX_MAX(HVX_DI_OUT_BUF_CNT, HVX_OTHER_OUT_BUF_CNT);
    u32Ret = u32VppIpHvxCore_BuffOutInit(pstCb->pstHvxCore, u32NumBuffers);
    if (u32Ret != VPP_OK)
    {
        LOGE("Error allocating bufferOut pixel_data");
        goto ERR_ALLOC_OUTPUT;
    }

    if (pstCb->stCfg.u32ComputeMask)
        vVppIpHvx_Compute(pstCb);

    u32Ret = u32VppIpHvxCore_BufParamInit(pstCb->pstHvxCore, pstCb->stInput.stParam.width,
                                          pstCb->stInput.stParam.height,
                                          pstCb->stInput.stParam.fmt);
    if (u32Ret != VPP_OK)
    {
        LOGE("Error initializing buffer parameters u32Ret=%u", u32Ret);
        goto ERR_BUF_PARAM_INIT;
    }

    // FW can support dynamic enablement within a session, so open all supported algos.
    u32VppProcFlags = u32VppIpHvx_GetSupportedProcFlags();
    u32Ret = u32VppIpHvxCore_Open(pstCb->pstHvxCore, u32VppProcFlags, u32FrameSizeBytes);
    if (u32Ret != VPP_OK)
    {
        LOGE("HVX Open failed ret %d", u32Ret);
        goto ERR_HVX_OPEN;
    }

    u32Ret = u32_VppIpHvxCore_GetBufReqs(pstCb->pstHvxCore, u32VppProcFlags, &stBufReq);
    if ((u32Ret != VPP_OK) || !stBufReq.input || !stBufReq.output)
    {
        LOGE("Error getting pixel buffer requirements, u32Ret=%u, InPixReq=%u, OutPixReq=%u",
             u32Ret, stBufReq.input, stBufReq.output);
        goto ERR_GET_BUF_REQ;
    }
    pstCb->u32InputMaxIntoProc = stBufReq.input;
    pstCb->u32OutputMaxIntoProc = stBufReq.output;
    LOGI("FW buffer requirements: In=%u, Out=%u", pstCb->u32InputMaxIntoProc,
         pstCb->u32OutputMaxIntoProc);

    // Not fatal if tunings fail; Log and continue
    u32Ret = u32VppIpHvxCore_TuningsBoot(pstCb->pstHvxCore, HVX_TUNINGS_FILE_NAME,
                                         u32VppProcFlags, VPP_FALSE);
    if (u32Ret != VPP_OK)
    {
        LOGE("Could not boot HVX tunings, u32Ret=%u", u32Ret);
    }
    else
    {
        HVX_TUNING_LOAD(pstCb, VPP_FUNC_ID_MVP);
        HVX_TUNING_LOAD(pstCb, VPP_FUNC_ID_NR);
        HVX_TUNING_LOAD(pstCb, VPP_FUNC_ID_IE);
    }
    pthread_mutex_unlock(&pstCb->mutex);

    // Reset proc stats
    VPP_IP_PROF_RESET_SINGLE(&pstCb->stBase, HVX_STAT_PROC);

    //// Ready to do DSP Executions ////////////////////////////////
    VPP_IP_STATE_SET(pstCb, VPP_IP_STATE_ACTIVE);

    LOGI("%s() posting semaphore", __func__);
    sem_post(&pstCb->sem);

    pstCb->async_res.u32OpenRet = VPP_OK;
    return pstCb->async_res.u32OpenRet;

ERR_GET_BUF_REQ:
ERR_HVX_OPEN:
ERR_BUF_PARAM_INIT:
    vVppIpHvxCore_BuffOutTerm(pstCb->pstHvxCore);

ERR_ALLOC_OUTPUT:
    vVppIpHvxCore_BuffInTerm(pstCb->pstHvxCore);

ERR_ALLOC_INPUT:
    pthread_mutex_unlock(&pstCb->mutex);

    pstCb->async_res.u32OpenRet = u32Ret;

    LOGI("%s() posting semaphore (error)", __func__);
    sem_post(&pstCb->sem);

    return pstCb->async_res.u32OpenRet;
}

static uint32_t u32VppIpHvx_ProcCmdClose(t_StVppIpHvxCb *pstCb)
{
    uint32_t u32VppProcFlags, u32Err, u32Ret = VPP_OK;

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        u32Ret = VPP_ERR_STATE;
    }

    LOGD("%s()", __func__);

    HVX_TUNING_UNREGISTER(pstCb, VPP_FUNC_ID_MVP);
    HVX_TUNING_UNREGISTER(pstCb, VPP_FUNC_ID_NR);
    HVX_TUNING_UNREGISTER(pstCb, VPP_FUNC_ID_IE);

    u32VppProcFlags = u32VppIpHvx_GetSupportedProcFlags();
    u32Err = u32VppIpHvxCore_TuningsShutdown(pstCb->pstHvxCore, u32VppProcFlags);
    LOGD_IF(u32Err != VPP_OK, "Could not shut down HVX tunings, u32Err=%u", u32Err);

    u32Ret = u32VppIpHvxCore_Close(pstCb->pstHvxCore);
    if (u32Ret != VPP_OK)
        LOGE("HVX Core Close Fail u32Ret=%u", u32Ret);

    vVppIpHvxCore_BuffInTerm(pstCb->pstHvxCore);
    vVppIpHvxCore_BuffOutTerm(pstCb->pstHvxCore);

    vVppIpHvx_ProcStateSet(pstCb, HVX_STATE_INITED);
    VPP_IP_STATE_SET(pstCb, VPP_IP_STATE_INITED);

    pstCb->async_res.u32CloseRet = u32Ret;

    LOGI("%s() posting semaphore", __func__);
    sem_post(&pstCb->sem);

    return pstCb->async_res.u32CloseRet;
}

static uint32_t u32VppIpHvx_FlushPort(t_StVppIpHvxCb *pstCb, enum vpp_port ePort)
{
    t_StVppBuf *pBuf;
    t_StVppIpPort *pstPort;

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

    while (u32VppIp_PortBufGet(pstPort, &pBuf, NULL) == VPP_OK)
    {
        VPP_FLAG_SET(pBuf->u32InternalFlags, VPP_BUF_FLAG_FLUSHED);
        pBuf->stPixel.u32FilledLen = 0;
        vVppIpCbLog(&pstCb->stBase.stCb, pBuf, eVppLogId_IpBufDone);
        u32VppIpHvx_ReturnBuffer(pstCb, ePort, pBuf);
    }

    return VPP_OK;
}

static uint32_t u32VppIpHvx_ProcCmdFlush(t_StVppIpHvxCb *pstCb,
                                         t_StVppIpCmd *pstCmd)
{
    uint32_t u32;
    t_StVppEvt stEvt;

    // Flush Port
    u32 = u32VppIpHvx_FlushPort(pstCb, pstCmd->flush.ePort);

    if (u32 == VPP_OK)
    {
        stEvt.eType = VPP_EVT_FLUSH_DONE;
        stEvt.flush.ePort = pstCmd->flush.ePort;
        u32VppIpCbEvent(&pstCb->stBase.stCb, stEvt);
    }

    return u32;
}

static uint32_t u32VppIpHvx_ProcCmdDrain(t_StVppIpHvxCb *pstCb)
{
    uint32_t u32;
    t_StVppEvt stEvt;

    pthread_mutex_lock(&pstCb->mutex);

    u32 = u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ);
    LOGI("drain requested, input queue count = %u", u32);

    if(u32 == 0)
    {
        // No more input buffers in the queue, drain has already been
        // completed.
        stEvt.eType = VPP_EVT_DRAIN_DONE;
        u32VppIpCbEvent(&pstCb->stBase.stCb, stEvt);
    }
    else
    {
        pstCb->u32InternalFlags |= IP_DRAIN_PENDING;
    }

    pthread_mutex_unlock(&pstCb->mutex);

    return VPP_OK;
}

static uint32_t u32VppIpHvx_ProcCmdReconfigure(t_StVppIpHvxCb *pstCb)
{
    uint32_t u32;

    if (!pstCb)
        return VPP_ERR_PARAM;

    u32 = u32VppIpHvxCore_PrepareCtx(pstCb->pstHvxCore);
    if (u32 != VPP_OK)
    {
        LOGE("ERROR: PrepareCtx returned err, u32=%u", u32);
    }

    pstCb->async_res.u32ReconfigRet = u32;
    LOGI("%s() posting semaphore (reconfig)", __func__);
    sem_post(&pstCb->sem);

    return u32;
}

static uint32_t bVppIpHvx_AlgoCtrlsCanBypass(t_StVppIpHvxCb *pstCb)
{
    uint32_t bCanBypass = VPP_TRUE;
    t_StVppIpHvxCfg *stCfg = &pstCb->stCfg;

    if (VPP_FLAG_IS_SET(stCfg->u32EnableMask, HVX_ALGO_ROI))
    {
        if (stCfg->stDemo.process_percent == 0)
            return VPP_TRUE;
    }
    if (VPP_FLAG_IS_SET(stCfg->u32EnableMask, HVX_ALGO_AIE))
    {
        if ((stCfg->stAie.mode == HQV_MODE_AUTO) ||
            ((stCfg->stAie.mode == HQV_MODE_MANUAL) &&
             ((stCfg->stAie.cade_level != 0) ||
              (stCfg->stAie.hue_mode != HQV_HUE_MODE_OFF) ||
              (stCfg->stAie.ltm_sat_gain != 0) ||
              (stCfg->stAie.ltm_sat_offset != AIE_LTM_SAT_OFFSET_MAX/2) ||
              (stCfg->stAie.ltm_ace_str != 0) ||
              (stCfg->stAie.ltm_ace_bright_l != 0) ||
              (stCfg->stAie.ltm_ace_bright_h != 0))))
            bCanBypass = VPP_FALSE;
    }
    if (VPP_FLAG_IS_SET(stCfg->u32EnableMask, HVX_ALGO_CNR))
    {
        if ((stCfg->stCnr.mode == HQV_MODE_AUTO) || ((stCfg->stCnr.mode == HQV_MODE_MANUAL) &&
            (stCfg->stCnr.level != 0)))
            bCanBypass = VPP_FALSE;
    }
    if (VPP_FLAG_IS_SET(stCfg->u32EnableMask, HVX_ALGO_DI))
    {
        if (stCfg->stDi.mode != HQV_DI_MODE_OFF)
            bCanBypass = VPP_FALSE;
    }
    return bCanBypass;
}

static void vVppIpHvx_ReturnBuffers(t_StVppIpHvxCb *pstCb, t_StVppBuf **ppstBufIn,
                                    uint32_t bBypassInput, t_StVppBuf **ppstBufOut,
                                    uint32_t u32BufOutFilledLength)
{
    uint32_t i;

    LOGI("%s()", __func__);

    for (i = 0; i < pstCb->u32InputOutofProc; i++)
    {
        if (ppstBufIn[i])
        {
            HVX_DUMP_FRAME_DESC(pstCb, ppstBufIn[i], "in", VPP_PORT_INPUT);
            if (VPP_FLAG_IS_SET(ppstBufIn[i]->u32InternalFlags, VPP_BUF_FLAG_DUMP))
            {
                vVppIpHvx_DumpFrame(pstCb, ppstBufIn[i],VPP_PORT_INPUT);
            }
            pstCb->stats.u32InProcCnt++;
            vVppIpCbLog(&pstCb->stBase.stCb, ppstBufIn[i], eVppLogId_IpBufDone);
            if (bBypassInput)
            {
                u32VppIpHvx_ReturnBuffer(pstCb, VPP_PORT_OUTPUT, ppstBufIn[i]);
            }
            else
            {
                u32VppIpHvx_ReturnBuffer(pstCb, VPP_PORT_INPUT, ppstBufIn[i]);
            }
        }
    }
    for (i = 0; i < pstCb->u32OutputOutofProc; i++)
    {
        if (ppstBufOut[i])
        {
            ppstBufOut[i]->stPixel.u32FilledLen = u32BufOutFilledLength;

            HVX_DUMP_FRAME_DESC(pstCb, ppstBufOut[i], "out", VPP_PORT_OUTPUT);
            if (VPP_FLAG_IS_SET(ppstBufOut[i]->u32InternalFlags, VPP_BUF_FLAG_DUMP))
            {
                vVppIpHvx_DumpFrame(pstCb, ppstBufOut[i],VPP_PORT_OUTPUT);
            }
            VPP_FLAG_CLR(ppstBufOut[i]->u32InternalFlags, VPP_BUF_FLAG_FLUSHED);
            pstCb->stats.u32OutProcCnt++;
            vVppIpCbLog(&pstCb->stBase.stCb, ppstBufOut[i], eVppLogId_IpBufDone);

            u32VppIpHvx_ReturnBuffer(pstCb, VPP_PORT_OUTPUT, ppstBufOut[i]);
        }
    }
}

static uint32_t u32VppIpHvx_CheckBufCfg(t_StVppIpHvxCb *pstCb, t_StVppBuf **ppstBufIn,
                                        uint8_t *pu8EnBypass, uint8_t *pu8SkipDi,
                                        uint32_t *pu32BufInCnt)
{
    uint32_t u32Ret = VPP_OK;
    uint32_t u32InBufSz;
    uint32_t u32OutBufSz;
    uint32_t i = 0;

    if (pstCb == NULL || ppstBufIn[0] == NULL || pu8EnBypass == NULL ||
        pu8SkipDi == NULL || pu32BufInCnt == NULL)
        return VPP_ERR;

    u32InBufSz = u32VppUtils_GetPxBufferSize(&pstCb->stInput.stParam);
    u32OutBufSz = u32VppUtils_GetPxBufferSize(&pstCb->stOutput.stParam);
    *pu8EnBypass = *pu8SkipDi = 0;

    // Check all InputOutofProc buffer types against each other to ensure they
    // match (assumed that they must match in order to run algo because they
    // have to be returned).
    // The rest of the InputIntoProc buffers don't necessarily have to match.
    for (i = 1; i < pstCb->u32InputOutofProc; i++)
    {
        if (ppstBufIn[i] == NULL)
            return VPP_ERR;

        if (ppstBufIn[i]->eBufType != ppstBufIn[i-1]->eBufType)
        {
            LOGE("Input buffer types (InputOutofProc) don't match");
            return VPP_ERR;
        }
    }

    if (!VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, HVX_ALGO_DI))
        *pu8SkipDi = 1;

    // When ppstBufIn[0] is progressive and DI and one or more other algos
    // are enabled, disable DI but still run other algo.
    if (VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, (HVX_ALGO_CNR | HVX_ALGO_AIE)) &&
        VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, HVX_ALGO_DI) &&
        (ppstBufIn[0]->eBufType == eVppBufType_Progressive)) //P frame
    {
        *pu8SkipDi = 1;
    }

    // Check configuration against input buffer
    if (!(VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask,
        (HVX_ALGO_DI | HVX_ALGO_CNR | HVX_ALGO_AIE))))
    {
        //No algo enabled, i.e. HQV_MODE_OFF
        LOGI("enable_bypass set to 1 for there is no algo enabled");
        *pu8EnBypass = 1;
    }
    else if (bVppIpHvx_AlgoCtrlsCanBypass(pstCb))
    {
        LOGI("enable_bypass set to 1. All enabled algos settings are disabled");
        *pu8EnBypass = 1;
    }
    else if (!(*pu8SkipDi) && (pstCb->stCfg.u32EnableMask & HVX_ALGO_DI) &&
             ppstBufIn[0]->eBufType != eVppBufType_Interleaved_TFF &&
             ppstBufIn[0]->eBufType != eVppBufType_Interleaved_BFF)
    {
        // Unsupported interlace format if DI is enabled
        LOGE("Unsupported interlaced field format %d", ppstBufIn[0]->eBufType);
        *pu8EnBypass = 1;
        u32Ret = VPP_ERR_INVALID_CFG;
    }
    else if (!(pstCb->stCfg.u32EnableMask & HVX_ALGO_DI) &&
             (ppstBufIn[0]->eBufType != eVppBufType_Progressive))
    {
        // Input is interlaced and DI not enabled
        LOGI("enable_bypass set to 1 for non-DI algo and non-progressive frame");
        *pu8EnBypass = 1;
    }
    else if ((pstCb->stCfg.u32EnableMask & HVX_ALGO_DI) &&
             !(pstCb->stCfg.u32EnableMask & HVX_ALGO_CNR) &&
             !(pstCb->stCfg.u32EnableMask & HVX_ALGO_AIE) &&
             (ppstBufIn[0]->eBufType == eVppBufType_Progressive))
    {
        // Only DI enabled and input frame type is progressive
        LOGI("enable_bypass set to 1 for DI-only and progressive frame");
        *pu8EnBypass = 1;
    }
    else
    {
        // Let any internal bypass within u32InputOutofProc take precedence
        // to bypass the algo. This should be revisted when
        // u32InputOutofProc > 1 because it's really dependent on how the algo
        // behaves
        for (i = 0; i < pstCb->u32InputOutofProc; i++)
        {
            if (VPP_FLAG_IS_SET(ppstBufIn[i]->u32InternalFlags, VPP_BUF_FLAG_BYPASS) ||
                VPP_FLAG_IS_SET(ppstBufIn[i]->u32InternalFlags, VPP_BUF_FLAG_INTERNAL_BYPASS))
            {
                LOGI("enable_bypass set to 1 for VPP_BUF_FLAG_BYPASS set");
                *pu8EnBypass = 1;
                break;
            }
        }
    }

    //to check buffer size.If too small put them back
    LOGI("u32InBufSz=%d u32OutBufSz=%d u32AllocLen=%d",
         u32InBufSz, u32OutBufSz, ppstBufIn[0]->stPixel.u32AllocLen);

    if (u32Ret == VPP_OK && *pu8EnBypass != 1)
    {
        // Check buffer sizes of inputs that are to be returned
        for (i = 0; i < pstCb->u32InputOutofProc; i++)
        {
            if (ppstBufIn[i]->stPixel.u32ValidLen < u32InBufSz ||
                ppstBufIn[i]->stPixel.u32FilledLen < u32InBufSz)
            {
                u32Ret = VPP_ERR;
                LOGD("Input buf size too small. Expect %u but valid=%u, filled=%u",
                     u32InBufSz, ppstBufIn[i]->stPixel.u32ValidLen,
                     ppstBufIn[i]->stPixel.u32FilledLen);
            }
        }

        // Check the size rest of the input buffers
        for (i = pstCb->u32InputOutofProc; i < pstCb->u32InputIntoProc; i++)
        {
            if (ppstBufIn[i] == NULL)
            {
                if (!(pstCb->u32InternalFlags & IP_DRAIN_PENDING) &&
                    !VPP_FLAG_IS_SET(ppstBufIn[i-1]->pBuf->flags, VPP_BUFFER_FLAG_EOS) &&
                    !VPP_FLAG_IS_SET(ppstBufIn[i-1]->u32InternalFlags,
                                     VPP_BUF_FLAG_BYPASS | VPP_BUF_FLAG_INTERNAL_BYPASS))
                {
                    u32Ret = VPP_ERR;
                    LOGE("peek at input buffer at idx=%u failed", i);
                }
                break;
            }
            if (VPP_FLAG_IS_SET(ppstBufIn[i]->u32InternalFlags,
                                VPP_BUF_FLAG_BYPASS | VPP_BUF_FLAG_INTERNAL_BYPASS))
            {
                *pu32BufInCnt = i;
                LOGD("Bypass on buf[%u], reduce proc to %u buffers", i, *pu32BufInCnt);
                break;
            }
            if ((*pu8SkipDi) && ppstBufIn[i]->eBufType != eVppBufType_Progressive)
            {
                *pu32BufInCnt = i;
                LOGD("Non-progressive on buf[%u], reduce proc to %u buffers", i, *pu32BufInCnt);
                break;
            }
            if (ppstBufIn[i]->stPixel.u32ValidLen < u32InBufSz ||
                ppstBufIn[i]->stPixel.u32FilledLen < u32InBufSz)
            {
                if (VPP_FLAG_IS_SET(ppstBufIn[i]->pBuf->flags, VPP_BUFFER_FLAG_EOS))
                {
                    LOGD("Empty EOS on buf[%u]", i);
                    *pu8EnBypass = 1;
                    break;
                }
                else
                {
                    u32Ret = VPP_ERR;
                    LOGD("Input buf[%u] size too small. Expect %u but valid=%u, filled=%u",
                         i, u32InBufSz, ppstBufIn[i]->stPixel.u32ValidLen,
                         ppstBufIn[i]->stPixel.u32FilledLen);
                }
            }
        }

        // Check output buffer sizes
        for (i = 0; i < pstCb->u32OutputIntoProc; i++)
        {
            t_StVppBuf* pbuf = pstVppBufPool_Peek(&pstCb->stOutput.stPendingQ, i);

            if (!pbuf)
            {
                u32Ret = VPP_ERR;
                LOGE("peek at output buffer at idx=%u failed", i);
                break;
            }
            if (pbuf->stPixel.u32ValidLen < u32OutBufSz)
            {
                u32Ret = VPP_ERR;
                LOGD("Output buf%d size too small. Expect %d and actual %d",
                     i, u32OutBufSz, pbuf->stPixel.u32ValidLen);
                break;
            }
        }
    }

    return u32Ret;
}

static uint32_t u32VppIpHvx_ProcessBuffer(t_StVppIpHvxCb *pstCb)
{
    uint32_t i=0;
    uint32_t u32Ret = VPP_OK;
    t_StVppBuf *pstBufIn[HVX_MAX_NUM_BUFS] = {NULL};
    t_StVppBuf *pstBufOut[HVX_MAX_NUM_BUFS] = {NULL};
    uint32_t u32InputBuffers = 0;
    uint32_t u32OutputBuffers = 0;
    t_StVppIpHvxCoreCb *pstHvxCore = pstCb->pstHvxCore;
    uint32_t u32Length;
    t_EVppBufType eCurBufType;
    vpp_svc_field_fmt_t eFieldFmt;
    uint32_t u32Width, u32Height, u32Planes;
    uint32_t u32OutBufSz = u32VppUtils_GetPxBufferSize(&pstCb->stOutput.stParam);
    uint32_t au32Stride[MAX_NUM_PLANES];
    uint32_t au32StrideDi[MAX_NUM_PLANES];
    uint8_t di_skipped = 0;
    uint8_t enable_bypass = 0;
    enum vpp_color_format eFmt;

    pthread_mutex_lock(&pstCb->mutex);

    LOGI("In %s, pstCb->u32InputIntoProc: %d, pstCb->u32OutputIntoProc(%d)", __func__,
         pstCb->u32InputIntoProc, pstCb->u32OutputIntoProc);

    if (pstCb->stCfg.u32ComputeMask)
        vVppIpHvx_Compute(pstCb);

    // Get inputs buffers to be consumed and removed from the queue
    for (i = 0; i < pstCb->u32InputOutofProc; i++)
    {
        //These buffers will be consumed and removed from Queue
        uint32_t ret = u32VppIp_PortBufGet(&pstCb->stInput, &pstBufIn[i], NULL);
        if (ret != VPP_OK || pstBufIn[i] == NULL)
        {
            LOGE("unable to remove buffer from input queue");
            u32Ret = VPP_ERR;
            goto PROCESS_BUFFER_BYPASS;
        }
        else
            u32InputBuffers++;
    }
    //Get other input buffers to be used for processing
    for (i = pstCb->u32InputOutofProc; i < pstCb->u32InputIntoProc; i++)
    {
        //These buffers will be used but will remain Queue for next time using
        pstBufIn[i] = pstVppBufPool_Peek(&pstCb->stInput.stPendingQ, (i - pstCb->u32InputOutofProc));

        if (pstBufIn[i] == NULL)
        {
            if (!(pstCb->u32InternalFlags & IP_DRAIN_PENDING) &&
                !VPP_FLAG_IS_SET(pstBufIn[i-1]->pBuf->flags, VPP_BUFFER_FLAG_EOS) &&
                !VPP_FLAG_IS_SET(pstBufIn[i-1]->u32InternalFlags,
                                 VPP_BUF_FLAG_BYPASS | VPP_BUF_FLAG_INTERNAL_BYPASS))
            {
                LOGE("unable to remove buffer from input queue");
                u32Ret = VPP_ERR;
                goto PROCESS_BUFFER_BYPASS;
            }
            break;
        }
        else
            u32InputBuffers++;
    }

    // Check the HVX configuration against the input buffer which will be
    // removed from the Queue
    u32Ret = u32VppIpHvx_CheckBufCfg(pstCb, pstBufIn, &enable_bypass, &di_skipped, &u32InputBuffers);
    if (u32Ret != VPP_OK)
    {
        goto PROCESS_BUFFER_BYPASS;
    }

    if (enable_bypass)
    {
        // CNR is 1 buffer ahead, so need to call process with peeked buffer from previous call
        // before we can go back to active_start state and begin bypass
        if (VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, HVX_ALGO_CNR) &&
            pstCb->eHvxProcState == HVX_STATE_ACTIVE)
        {
            LOGD("Process with %u buffers before changing to bypass", pstCb->u32InputOutofProc);
            u32InputBuffers = pstCb->u32InputOutofProc;
        }
        else
        {
            goto PROCESS_BUFFER_BYPASS;
        }
    }

    eCurBufType = pstBufIn[0]->eBufType;
    eFieldFmt = eVppIpHvxCore_SvcFieldFormatGet(eCurBufType);
    eFmt = pstCb->stInput.stParam.fmt;
    u32Planes = u32VppUtils_GetNumPlanes(eFmt);
    u32VppIpHvxCore_BufParamSetFldFmt(pstHvxCore, eFieldFmt);
    u32VppIpHvxCore_BufInSetFldFmt(pstHvxCore, eFieldFmt);
    u32VppIpHvxCore_BufParamGetPlaneTotalSize(pstHvxCore, &u32Length);
    u32VppIpHvxCore_BufParamGetPlaneStride(pstHvxCore, au32Stride, u32Planes);
    u32VppIpHvxCore_BufParamGetSize(pstHvxCore, &u32Width, &u32Height);

    if (di_skipped) //P frame
    {
        // Disable DI, adjust strides and height
        if (pstCb->pstMvpHeader)
            pstCb->pstMvpHeader->process_flags = 0;
        u32VppIpHvxCore_BufInSetSize(pstHvxCore, u32Width, u32Height);
        u32VppIpHvxCore_BufInSetNumBuffers(pstHvxCore, u32InputBuffers);
        for (i = 0; i < u32InputBuffers; i++)
        {
            u32VppIpHvxCore_BufInSetAttrStride(pstHvxCore, i, au32Stride, u32Planes);
        }
    }
    else
    {
        //DI+Other algo and In frame interlaced
        //When P to I, we need to change the following back to enable DI
        if (pstCb->pstMvpHeader)
            pstCb->pstMvpHeader->process_flags = 1;
        u32VppIpHvxCore_BufInSetSize(pstHvxCore, u32Width, (u32Height / 2));
        u32VppIpHvxCore_BufInSetNumBuffers(pstHvxCore, HVX_DI_IN_BUF_CNT);
        for (i = 0; i < u32Planes; i++)
        {
            if (u32VppUtils_IsFmtUbwc(eFmt))
                au32StrideDi[i] = au32Stride[i];
            else
                au32StrideDi[i] = au32Stride[i] * 2;
        }
        for (i = 0; i < HVX_DI_IN_BUF_CNT; i++)
        {
            u32VppIpHvxCore_BufInSetAttrStride(pstHvxCore, i, au32StrideDi, u32Planes);
        }
    }

    LOGI("DI %s. HVX_ALGO_CNR=%d HVX_ALGO_AIE=%d pstBufIn[0]->eBufType=%d",
         di_skipped ? "skipped" : "enabled",
         VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, HVX_ALGO_CNR),
         VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, HVX_ALGO_AIE),
         eCurBufType);

    //Get output buffers to be used for processing
    for (i = 0; i < pstCb->u32OutputOutofProc; i++)
    {
        //These buffers will be consumed and removed from Queue
        uint32_t ret = u32VppIp_PortBufGet(&pstCb->stOutput, &pstBufOut[i], NULL);
        if (ret != VPP_OK || pstBufOut[i] == NULL)
        {
            LOGE("%s(): u32VppIp_PortBufGet() error", __func__);
            u32Ret = VPP_ERR;
            goto PROCESS_BUFFER_BYPASS;
        }
        else
            u32OutputBuffers++;
    }
    for (i = pstCb->u32OutputOutofProc; i < pstCb->u32OutputIntoProc ;i++)
    {
        //These buffers will be used but will remain Queue for next time using
        pstBufOut[i] = pstVppBufPool_Peek(&pstCb->stOutput.stPendingQ, (i - pstCb->u32OutputOutofProc));
        if ((pstBufOut[i]) == NULL)
        {
            LOGE("%s(): pstVppBufPool_Peek() error", __func__);
            u32Ret = VPP_ERR;
            goto PROCESS_BUFFER_BYPASS;
        }
        else
            u32OutputBuffers++;
    }
    u32VppIpHvxCore_BufOutSetNumBuffers(pstHvxCore, u32OutputBuffers);
    pthread_mutex_unlock(&pstCb->mutex);

#ifndef HVX_REG_BUFFER_ON_QUEUE
    for (i = 0; i < u32InputBuffers; i++)
    {
        vVppIpHvxCore_RegisterBuffer(pstCb->pstHvxCore, &pstBufIn[i]->stPixel);
    }
    for (i = 0; i < u32OutputBuffers; i++)
    {
        vVppIpHvxCore_RegisterBuffer(pstCb->pstHvxCore, &pstBufOut[i]->stPixel);
    }
#endif

    if ((pstCb->stCfg.u32EnableMask & HVX_ALGO_DI) && !di_skipped )
    {
        uint32_t u32InBufsToUse = 1;
        const uint8_t* pu8DiBufCfgLut;
        t_EVppUbwcStatField eUbwcFld;

        for (i = 1; i < u32InputBuffers; i++)
        {
            if ((pstBufIn[i]->eBufType != pstBufIn[i-1]->eBufType) ||
                VPP_FLAG_IS_SET(pstBufIn[i-1]->pBuf->flags, VPP_BUFFER_FLAG_EOS) ||
                VPP_FLAG_IS_SET(pstBufIn[i]->u32InternalFlags,
                                VPP_BUF_FLAG_BYPASS | VPP_BUF_FLAG_INTERNAL_BYPASS))
                break;
            if (pstBufIn[i]->stPixel.u32FilledLen == 0)
                break;
            u32InBufsToUse++;
        }

        LOGD("DI: BufType=%d, InBufsToUse=%d EOS=%d", pstBufIn[0]->eBufType,
             u32InBufsToUse, pstBufIn[0]->pBuf->flags & VPP_BUFFER_FLAG_EOS);

        pu8DiBufCfgLut = HVX_DI_BUF_CFG_LUT(pstBufIn[0]->eBufType, u32InBufsToUse);
        for (i = 0; i < HVX_DI_IN_BUF_CNT; i++)
        {
            uint32_t u32BufIdx, u32Delta;
            u32BufIdx = pu8DiBufCfgLut[i*2];

            if (u32VppUtils_IsFmtUbwc(eFmt))
            {
                // TODO: Need to confirm this calculation. Bottom field may not be at u32Length/2
                u32Delta = pu8DiBufCfgLut[i * 2 + 1] * (u32Length / 2);
                eUbwcFld = aeDiUbwcField[i];
                u32VppIpHvxCore_BufInSetAttrUbwc(pstHvxCore, i,
                                                 &pstBufIn[u32BufIdx]->stUbwcStats[eUbwcFld]);
            }
            else
            {
                u32Delta = pu8DiBufCfgLut[i * 2 + 1] * au32Stride[0];
                u32VppIpHvxCore_BufInSetAttrUbwc(pstHvxCore, i, NULL);
            }

            LOGD("\tBufIdx=%u Delta=%u", u32BufIdx, u32Delta);
            u32VppIpHvxCore_BufInSetUserDataAddr(pstHvxCore, i,
                                                 ((void*)((char*)
                                                          (pstBufIn[u32BufIdx]->stPixel.pvBase) +
                                                          u32Delta)));
            u32VppIpHvxCore_BufInSetUserDataLen(pstHvxCore, i, (u32Length - u32Delta));
        }
    }
    else
    {
        for (i = 0; i < pstCb->u32InputIntoProc; i++)
        {
            if (i < u32InputBuffers)
            {
                u32VppIpHvxCore_BufInSetUserDataAddr(pstHvxCore, i,
                                                     ((void*)(pstBufIn[i]->stPixel.pvBase)));
                u32VppIpHvxCore_BufInSetUserDataLen(pstHvxCore, i, u32Length);
                if (u32VppUtils_IsFmtUbwc(eFmt))
                {
                    u32VppIpHvxCore_BufInSetAttrUbwc(pstHvxCore, i,
                                                     &pstBufIn[i]->stUbwcStats[eVppUbwcStatField_P]);
                }
                else
                {
                    u32VppIpHvxCore_BufInSetAttrUbwc(pstHvxCore, i, NULL);
                }
            }
            else
            {
                u32VppIpHvxCore_BufInSetUserDataAddr(pstHvxCore, i, NULL);
                u32VppIpHvxCore_BufInSetUserDataLen(pstHvxCore, i, 0);
                u32VppIpHvxCore_BufInSetAttrUbwc(pstHvxCore, i, NULL);
            }

        }
    }

    for (i = 0; i < pstCb->u32OutputIntoProc; i++)
    {
        LOGI("ProcessBuffer: output, i=%u, pvBase=%p", i, pstBufOut[i]->stPixel.pvBase);
        //To set buffer pointer
        u32VppIpHvxCore_BufOutSetUserDataAddr(pstHvxCore, i,
                                              (void *)(pstBufOut[i]->stPixel.pvBase));
    }

    // These log time start calls are intentionally separated from the
    // RegisterBuffer calls, so that the RegisterBuffer calls do not skew the
    // times for the buffers. The time it takes to register a buffer should be
    // included in the overall processing time.
    for (i = 0; i < u32InputBuffers; i++)
    {
        vVppIpCbLog(&pstCb->stBase.stCb, pstBufIn[i], eVppLogId_IpProcStart);
    }
    for (i = 0; i < u32OutputBuffers; i++)
    {
        vVppIpCbLog(&pstCb->stBase.stCb, pstBufOut[i], eVppLogId_IpProcStart);
    }

#ifdef DUMP_PROCESSING_PARAMS
    if (VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, HVX_ALGO_DI))
        print_vpp_svc_mvp_params(pstCb->pstMvpParams);

    if (VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, HVX_ALGO_CNR))
        print_vpp_svc_nr_params(pstCb->pstNrParams);

    if (VPP_FLAG_IS_SET(pstCb->stCfg.u32EnableMask, HVX_ALGO_AIE))
        print_vpp_svc_ie_params(pstCb->pstAieParams);
#endif

    LOGI("about to call vpp_svc_process, InProcCnt=%u, OutProcCnt=%u",
         pstCb->stats.u32InProcCnt, pstCb->stats.u32OutProcCnt);

    int rc;
    VPP_IP_PROF_START(&pstCb->stBase, HVX_STAT_PROC);
    rc = VppIpHvxCore_Process(pstHvxCore);
    VPP_IP_PROF_STOP(&pstCb->stBase, HVX_STAT_PROC);

    LOGI("vpp_svc_process returned, rc=%d", rc);

#ifndef HVX_REG_BUFFER_ON_QUEUE
    for (i = 0; i < u32InputBuffers; i++)
    {
        vVppIpHvxCore_UnregisterBuffer(pstCb->pstHvxCore, &pstBufIn[i]->stPixel);
    }
    for (i = 0; i < u32OutputBuffers; i++)
    {
        vVppIpHvxCore_UnregisterBuffer(pstCb->pstHvxCore, &pstBufOut[i]->stPixel);
    }
#endif

    for (i = 0; i < u32InputBuffers; i++)
    {
        vVppIpCbLog(&pstCb->stBase.stCb, pstBufIn[i], eVppLogId_IpProcDone);
    }
    for (i = 0; i < u32OutputBuffers; i++)
    {
        vVppIpCbLog(&pstCb->stBase.stCb, pstBufOut[i], eVppLogId_IpProcDone);
    }

    if (0 != rc)
    {
        u32Ret = VPP_ERR;
        LOGE("Error: compute on aDSP failed");
        vVppIpHvx_ProcStateSet(pstCb, HVX_STATE_ACTIVE_START);
    }
    else
    {
        LOGD("vpp_svc_process successfully!");
        vVppIpHvx_ProcStateSet(pstCb, HVX_STATE_ACTIVE);
        if (u32InputBuffers < pstCb->u32InputIntoProc)
        {
            // Reset state if processed with reduced buffers
            vVppIpHvx_ProcStateSet(pstCb, HVX_STATE_ACTIVE_START);
        }
    }

    //update_flags
    if (pstCb->stCfg.u32EnableMask & HVX_ALGO_DI)
    {
        pstCb->pstMvpParams->update_flags = 0;
    }
    if (pstCb->stCfg.u32EnableMask & HVX_ALGO_CNR)
    {
        pstCb->pstNrParams->update_flags = 0;
    }
    if (pstCb->stCfg.u32EnableMask & HVX_ALGO_AIE)
    {
        pstCb->pstAieParams->update_flags = 0;
    }

    //timestamp and pBuf->flags
    if ((pstCb->stCfg.u32EnableMask & HVX_ALGO_DI) && !di_skipped)
    {
        uint32_t u32InBufsToUse = 0;
        int8_t s8ExtIBuf, s8DcOBuf, s8IdrOBuf, bIdrChk;

        for (i = 0; i < u32InputBuffers; i++)
        {
            if (i > 0 && ((pstBufIn[i]->eBufType != pstBufIn[i-1]->eBufType) ||
                VPP_FLAG_IS_SET(pstBufIn[i-1]->pBuf->flags, VPP_BUFFER_FLAG_EOS) ||
                VPP_FLAG_IS_SET(pstBufIn[i]->u32InternalFlags,
                                VPP_BUF_FLAG_BYPASS | VPP_BUF_FLAG_INTERNAL_BYPASS)))
                break;
            if (pstBufIn[i]->stPixel.u32FilledLen == 0)
                break;
            u32InBufsToUse++;
        }

        pstBufOut[0]->pBuf->timestamp = pstBufIn[0]->pBuf->timestamp;
        if (u32InputBuffers == 1 || pstBufIn[1] == NULL ||
            (pstBufIn[1] != NULL && pstBufIn[1]->stPixel.u32FilledLen == 0))
            pstBufOut[1]->pBuf->timestamp = pstBufOut[0]->pBuf->timestamp + 1;
        else
            pstBufOut[1]->pBuf->timestamp = (pstBufIn[0]->pBuf->timestamp / 2) +
                (pstBufIn[1]->pBuf->timestamp / 2);
        pstBufOut[0]->u32TimestampFrameRate = pstBufIn[0]->u32TimestampFrameRate * 2;
        pstBufOut[0]->u32OperatingRate = pstBufIn[0]->u32OperatingRate * 2;
        pstBufOut[1]->u32TimestampFrameRate = pstBufIn[0]->u32TimestampFrameRate * 2;
        pstBufOut[1]->u32OperatingRate = pstBufIn[0]->u32OperatingRate * 2;

        for (i = 0; i < u32InBufsToUse; i++)
        {
            s8DcOBuf = HVX_DI_DC_CFG_LUT(pstBufIn[0]->eBufType, i);
            s8IdrOBuf = HVX_DI_IDR_CFG_LUT(pstBufIn[0]->eBufType, i);
            bIdrChk = HVX_DI_IDR_CHK_LUT(pstBufIn[0]->eBufType, i, u32InBufsToUse);
            if ((VPP_FLAG_IS_SET(pstBufIn[i]->pBuf->flags, VPP_BUFFER_FLAG_DATACORRUPT)) &&
                !(VPP_FLAG_IS_SET(pstBufIn[i]->u32InternalFlags, VPP_BUF_FLAG_DC_PROCESSED)) &&
                s8DcOBuf != -1)
            {
                VPP_FLAG_SET(pstBufOut[s8DcOBuf]->pBuf->flags, VPP_BUFFER_FLAG_DATACORRUPT);
                VPP_FLAG_SET(pstBufIn[i]->u32InternalFlags, VPP_BUF_FLAG_DC_PROCESSED);
                pstCb->u32InternalFlags |= IP_CORRUPT_BUFFERS;
            }

            if ((VPP_FLAG_IS_SET(pstBufIn[i]->pBuf->flags, VPP_BUFFER_FLAG_SYNCFRAME)) &&
                !(VPP_FLAG_IS_SET(pstBufIn[i]->u32InternalFlags, VPP_BUF_FLAG_IDR_PROCESSED)) &&
                s8IdrOBuf != -1)
            {
                if (!bIdrChk ||
                    !(VPP_FLAG_IS_SET(pstBufIn[i + 1]->pBuf->flags, VPP_BUFFER_FLAG_DATACORRUPT)))
                {
                    VPP_FLAG_SET(pstBufOut[s8IdrOBuf]->pBuf->flags, VPP_BUFFER_FLAG_SYNCFRAME);
                    pstCb->u32InternalFlags &= ~IP_CORRUPT_BUFFERS;
                }
                VPP_FLAG_SET(pstBufIn[i]->u32InternalFlags, VPP_BUF_FLAG_IDR_PROCESSED);
            }
        }

        if (VPP_FLAG_IS_SET(pstBufIn[0]->pBuf->flags, VPP_BUFFER_FLAG_EOS))
        {
            VPP_FLAG_SET(pstBufOut[1]->pBuf->flags, VPP_BUFFER_FLAG_EOS);
            VPP_FLAG_SET(pstBufIn[0]->u32InternalFlags, VPP_BUF_FLAG_EOS_PROCESSED);
            pstCb->u32InternalFlags &= ~IP_CORRUPT_BUFFERS;
        }

        for (i = 0; i < pstCb->u32OutputOutofProc; i++)
        {
            s8ExtIBuf = HVX_DI_EXT_CPY_LUT(pstBufIn[0]->eBufType, i, u32InBufsToUse);

            u32VppBuf_PropagateExtradata(pstBufIn[s8ExtIBuf],
                                         pstBufOut[i],
                                         eVppBufType_Progressive,
                                         VPP_EXTERNAL_EXTRADATA_TYPE);
            pstBufOut[i]->pBuf->cookie_in_to_out =
                pstBufIn[s8ExtIBuf]->pBuf->cookie_in_to_out;
            u32VppBuf_GrallocMetadataCopy(pstBufIn[s8ExtIBuf], pstBufOut[i]);
            u32VppBuf_GrallocFrameTypeSet(pstBufOut[i], eVppBufType_Progressive);
            u32VppBuf_GrallocFramerateMultiply(pstBufOut[i], 2);
            if (u32VppUtils_IsFmtUbwc(eFmt))
            {
                u32VppIpHvxCore_BufOutGetAttrUbwc(pstHvxCore, i,
                                                  &pstBufOut[i]->stUbwcStats[eVppUbwcStatField_P]);
            }

        }
        pstCb->u32InternalFlags |= IP_FLAG_DI_ALGO_RUN; // Set flag (DI uc)
    }
    else
    {   // Copy from input to output:
        for (i = 0; i < pstCb->u32OutputOutofProc; i++)
        {
            pstBufOut[i]->pBuf->timestamp = pstBufIn[i]->pBuf->timestamp;
            pstBufOut[i]->pBuf->flags = pstBufIn[i]->pBuf->flags;
            pstBufOut[i]->pBuf->cookie_in_to_out = pstBufIn[i]->pBuf->cookie_in_to_out;
            pstBufOut[i]->u32TimestampFrameRate = pstBufIn[i]->u32TimestampFrameRate;
            pstBufOut[i]->u32OperatingRate = pstBufIn[i]->u32OperatingRate;
            u32VppBuf_PropagateExtradata(pstBufIn[i], pstBufOut[i],
                                         eVppBufType_Progressive,
                                         VPP_EXTERNAL_EXTRADATA_TYPE);

            u32VppBuf_GrallocMetadataCopy(pstBufIn[i], pstBufOut[i]);
#if defined(UPDATE_GRALLOC_FRAMERATE_WHEN_DI_MASK_ENABLED) || defined(UPDATE_GRALLOC_FRAMERATE_WHEN_DI_RUN)
            if (pstCb->stCfg.u32EnableMask & HVX_ALGO_DI
#ifdef UPDATE_GRALLOC_FRAMERATE_WHEN_DI_RUN
                && pstCb->u32InternalFlags & IP_FLAG_DI_ALGO_RUN
#endif
                )
                u32VppBuf_GrallocFramerateMultiply(pstBufOut[i], 2);
#endif
            if (u32VppUtils_IsFmtUbwc(eFmt))
            {
                u32VppIpHvxCore_BufOutGetAttrUbwc(pstHvxCore, i,
                                                  &pstBufOut[i]->stUbwcStats[eVppUbwcStatField_P]);
            }

            if (VPP_FLAG_IS_SET(pstBufIn[i]->pBuf->flags, VPP_BUFFER_FLAG_SYNCFRAME))
            {
                VPP_FLAG_SET(pstBufIn[i]->u32InternalFlags, VPP_BUF_FLAG_IDR_PROCESSED);

                pstCb->u32InternalFlags &= ~IP_CORRUPT_BUFFERS;
            }
            if (VPP_FLAG_IS_SET(pstBufIn[i]->pBuf->flags, VPP_BUFFER_FLAG_DATACORRUPT))
            {
                VPP_FLAG_SET(pstBufIn[i]->u32InternalFlags, VPP_BUF_FLAG_DC_PROCESSED);

                pstCb->u32InternalFlags |= IP_CORRUPT_BUFFERS;
            }
            if (VPP_FLAG_IS_SET(pstBufIn[i]->pBuf->flags, VPP_BUFFER_FLAG_EOS))
            {
                VPP_FLAG_SET(pstBufIn[i]->u32InternalFlags, VPP_BUF_FLAG_EOS_PROCESSED);

                pstCb->u32InternalFlags &= ~IP_CORRUPT_BUFFERS;
            }
        }
    }

    if (rc == 0)
        vVppIpHvx_ReturnBuffers(pstCb, pstBufIn, VPP_FALSE, pstBufOut, u32OutBufSz);
    else
    {
#if defined(UPDATE_GRALLOC_FRAMERATE_WHEN_DI_MASK_ENABLED) || defined(UPDATE_GRALLOC_FRAMERATE_WHEN_DI_RUN)
        for (i = 0; i < pstCb->u32InputOutofProc; i++)
        {
            if (pstCb->stCfg.u32EnableMask & HVX_ALGO_DI
#ifdef UPDATE_GRALLOC_FRAMERATE_WHEN_DI_RUN
                && pstCb->u32InternalFlags & IP_FLAG_DI_ALGO_RUN
#endif
                )
                u32VppBuf_GrallocFramerateMultiply(pstBufIn[i], 2);
        }
#endif
        vVppIpHvx_ReturnBuffers(pstCb, pstBufIn, VPP_TRUE, pstBufOut, 0);
    }

    return u32Ret;

PROCESS_BUFFER_BYPASS:
    for (i = 0; i < pstCb->u32InputOutofProc; i++)
    {
        if (pstBufIn[i] != NULL)
        {
            VPP_FLAG_CLR(pstBufIn[i]->u32InternalFlags, VPP_BUF_FLAG_INTERNAL_BYPASS);
#if defined(UPDATE_GRALLOC_FRAMERATE_WHEN_DI_MASK_ENABLED) || defined(UPDATE_GRALLOC_FRAMERATE_WHEN_DI_RUN)
            if (pstCb->stCfg.u32EnableMask & HVX_ALGO_DI
#ifdef UPDATE_GRALLOC_FRAMERATE_WHEN_DI_RUN
                && pstCb->u32InternalFlags & IP_FLAG_DI_ALGO_RUN
#endif
                )
                u32VppBuf_GrallocFramerateMultiply(pstBufIn[i], 2);
#endif
        }
    }

    vVppIpHvx_ProcStateSet(pstCb, HVX_STATE_ACTIVE_START);
    pthread_mutex_unlock(&pstCb->mutex);
    // Return buffers
    vVppIpHvx_ReturnBuffers(pstCb, pstBufIn, VPP_TRUE, pstBufOut, 0);

    return u32Ret;
}

static void vVppIpHvx_HandlePendingDrain(t_StVppIpHvxCb *pstCb)
{
    t_StVppEvt stEvt;

    pthread_mutex_lock(&pstCb->mutex);

    if (pstCb->u32InternalFlags & IP_DRAIN_PENDING &&
        u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ) ==0)
    {
        pstCb->u32InternalFlags &= ~IP_DRAIN_PENDING;
        stEvt.eType = VPP_EVT_DRAIN_DONE;
        u32VppIpCbEvent(&pstCb->stBase.stCb, stEvt);
    }

    pthread_mutex_unlock(&pstCb->mutex);
}

static void *vpVppIpHvx_Worker(void *pv)
{
    LOGI("%s started", __func__);

    t_StVppIpHvxCb *pstCb = (t_StVppIpHvxCb *)pv;

    // Signal back to main thread that we've launched and are ready to go
    vVppIpHvx_SignalWorkerStart(pstCb);

    while (1)
    {
        pthread_mutex_lock(&pstCb->mutex);
        while (u32WorkerThreadShouldSleep(pstCb))
        {
            VPP_IP_PROF_START(&pstCb->stBase, HVX_STAT_WORKER_SLEEP);
            pthread_cond_wait(&pstCb->cond, &pstCb->mutex);
            VPP_IP_PROF_STOP(&pstCb->stBase, HVX_STAT_WORKER_SLEEP);
        }

        VPP_IP_PROF_START(&pstCb->stBase, HVX_STAT_WORKER);

        uint32_t u32Ret;
        t_StVppIpCmd stCmd;
        u32Ret = u32VppIpHvx_CmdGet(pstCb, &stCmd);
        if (u32Ret == VPP_OK)
        {
            pthread_mutex_unlock(&pstCb->mutex);

            // Process the command
            LOG_CMD("ProcessCmd", stCmd.eCmd);

            if (stCmd.eCmd == VPP_IP_CMD_THREAD_EXIT)
                break;

            else if (stCmd.eCmd == VPP_IP_CMD_OPEN)
                u32VppIpHvx_ProcCmdOpen(pstCb);

            else if (stCmd.eCmd == VPP_IP_CMD_CLOSE)
                u32VppIpHvx_ProcCmdClose(pstCb);

            else if (stCmd.eCmd == VPP_IP_CMD_FLUSH)
                u32VppIpHvx_ProcCmdFlush(pstCb, &stCmd);

            else if (stCmd.eCmd == VPP_IP_CMD_DRAIN)
                u32VppIpHvx_ProcCmdDrain(pstCb);

            else if (stCmd.eCmd == VPP_IP_CMD_RECONFIGURE)
                u32VppIpHvx_ProcCmdReconfigure(pstCb);

            else
                LOGE("unknown command in queue");

            VPP_IP_PROF_STOP(&pstCb->stBase, HVX_STAT_WORKER);
            continue;
        }

        if (pstCb->eState != VPP_IP_STATE_ACTIVE)
        {
            LOGD("got buffer, but state is not active");
            pthread_mutex_unlock(&pstCb->mutex);
            VPP_IP_PROF_STOP(&pstCb->stBase, HVX_STAT_WORKER);
            continue;
        }

        if (u32ProcBufReqMet(pstCb))
        {
            pthread_mutex_unlock(&pstCb->mutex);
            u32VppIpHvx_ProcessBuffer(pstCb);
            vVppIpHvx_HandlePendingDrain(pstCb);
            VPP_IP_PROF_STOP(&pstCb->stBase, HVX_STAT_WORKER);
            continue;
        }

        pthread_mutex_unlock(&pstCb->mutex);
    } //while (1)

    LOGI("%s exited", __func__);

    return NULL;
}

/************************************************************************
 * Global Functions
 ***********************************************************************/
uint32_t u32VppIpHvx_Boot()
{
    uint32_t u32Ret = VPP_OK;
    uint32_t u32VppProcFlags;

    LOG_ENTER();

    u32VppProcFlags = u32VppIpHvx_GetSupportedProcFlags();
    u32Ret = u32VppIpHvxCore_Boot(HVX_TUNINGS_FILE_NAME, u32VppProcFlags);
    LOGE_IF(u32Ret != VPP_OK, "u32VppIpHvxCore_Boot returned u32Ret=%u", u32Ret);

    LOG_EXIT_RET(u32Ret);
}

uint32_t u32VppIpHvx_Shutdown()
{
    uint32_t u32Ret = VPP_OK;
    uint32_t u32VppProcFlags;

    LOG_ENTER();

    u32VppProcFlags = u32VppIpHvx_GetSupportedProcFlags();
    u32Ret = u32VppIpHvxCore_Shutdown(u32VppProcFlags);
    LOGE_IF(u32Ret != VPP_OK, "u32VppIpHvxCore_Shutdown returned u32Ret=%u", u32Ret);

    LOG_EXIT_RET(u32Ret);
}

void * vpVppIpHvx_Init(t_StVppCtx *pstCtx, uint32_t u32Flags, t_StVppCallback cbs)
{
    LOGI("%s", __func__);

    int rc;
    t_StVppIpHvxCb *pstCb;
    uint32_t u32Length = 0, u32CtrlCnt = 0;
    uint32_t u32;

    VPP_RET_IF_NULL(pstCtx, NULL);

    if (u32VppIp_ValidateCallbacks(&cbs) != VPP_OK)
    {
        LOGE("given invalid callbacks.");
        goto ERROR_CALLBACKS;
    }

    pstCb = calloc(sizeof(t_StVppIpHvxCb), 1);
    if (!pstCb)
    {
        LOGE("calloc failed for hvx context");
        goto ERROR_MALLOC_CONTEXT;
    }
    LOGD("%s pstCb=%p", __func__,pstCb);

    u32VppIp_SetBase(pstCtx, u32Flags, cbs, &pstCb->stBase);

    vVppIpHvx_ProcStateSet(pstCb, HVX_STATE_INITED);

    u32 = VPP_IP_PROF_REGISTER(&pstCb->stBase, astHvxStatsCfg, u32HvxStatCnt);
    LOGE_IF(u32 != VPP_OK, "ERROR: unable to register stats, u32=%u", u32);

    if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP))
    {
        u32Length += sizeof(vpp_svc_mvp_params_t);
        u32CtrlCnt++;
    }

    if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_NR))
    {
        u32Length += sizeof(vpp_svc_nr_params_t);
        u32CtrlCnt++;
    }

    if (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_IE))
    {
        u32Length += sizeof(vpp_svc_aie_params_t);
        u32CtrlCnt++;
    }
    LOGD("u32CtrlCnt =%u, u32Length = %u", u32CtrlCnt, u32Length);

    pstCb->pstHvxCore = pstVppIpHvxCore_Init(pstCb->stBase.pstCtx, u32Flags,
                                             u32CtrlCnt, u32Length);
    if (!pstCb->pstHvxCore)
    {
        LOGE("Failed to init HVX Core.");
        goto ERROR_CORE_INIT;
    }

    if (u32VppIp_PortInit(&pstCb->stInput) != VPP_OK)
    {
        LOGE("unable to u32VppIp_PortInit() input port");
        goto ERROR_INPUT_PORT_INIT;
    }

    if (u32VppIp_PortInit(&pstCb->stOutput) != VPP_OK)
    {
        LOGE("unable to u32VppIp_PortInit() output port");
        goto ERROR_OUTPUT_PORT_INIT;
    }

    if (vpp_queue_init(&pstCb->stCmdQ, HVX_CMD_Q_SZ) != VPP_OK)
    {
        LOGE("unable to vpp_queue_init");
        goto ERROR_CMD_Q_INIT;
    }

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

#ifndef HVX_SINGLE_THREAD
    rc = pthread_create(&pstCb->thread, NULL, vpVppIpHvx_Worker, pstCb);
    if (rc)
    {
        LOGE("unable to spawn hvx worker thread");
        goto ERROR_THREAD_CREATE;
    }
#endif

    vVppIpHvx_InitCapabilityResources(pstCb);

    memcpy((void*) &pstCb->stLocalHvxParams, (void*) &stGlobalHvxParams, sizeof(t_StCustomHvxParams));

    vVppIpHvx_InitParam(pstCb);

#ifndef HVX_SINGLE_THREAD
    // Wait for the thread to launch before returning
    vVppIpHvx_WaitWorkerStart(pstCb);
#endif

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
    vpp_queue_term(&pstCb->stCmdQ);

ERROR_CMD_Q_INIT:
    u32VppIp_PortTerm(&pstCb->stOutput);

ERROR_OUTPUT_PORT_INIT:
    u32VppIp_PortTerm(&pstCb->stInput);

ERROR_INPUT_PORT_INIT:
    vVppIpHvxCore_Term(pstCb->pstHvxCore);

ERROR_CORE_INIT:
    u32 = VPP_IP_PROF_UNREGISTER(&pstCb->stBase);
    LOGE_IF(u32 != VPP_OK, "ERROR: unable to unregister stats, u32=%u", u32);

    if(pstCb)
        free(pstCb);

ERROR_MALLOC_CONTEXT:
ERROR_CALLBACKS:
    return NULL;
}

void vVppIpHvx_Term(void *ctx)
{
    int rc;
    uint32_t u32;
    t_StVppIpHvxCb *pstCb;
    t_StVppIpCmd stCmd;

    LOGI("%s", __func__);

    VPP_RET_VOID_IF_NULL(ctx);
    pstCb = HVX_CB_GET(ctx);

    if(!pstCb)
    {
        LOGD("Try to free NULL pstCb");
        return;
    }

    if (VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGD("%s() Still in active state. Closing.", __func__);
        u32VppIpHvx_Close(ctx);
    }

    stCmd.eCmd = VPP_IP_CMD_THREAD_EXIT;
    u32VppIpHvx_CmdPut(pstCb, stCmd);

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

    u32VppIpHvx_FlushPort(pstCb, VPP_PORT_INPUT);
    u32VppIpHvx_FlushPort(pstCb, VPP_PORT_OUTPUT);
    u32VppIp_PortTerm(&pstCb->stInput);
    u32VppIp_PortTerm(&pstCb->stOutput);

    vVppIpHvxCore_Term(pstCb->pstHvxCore);

    u32 = VPP_IP_PROF_UNREGISTER(&pstCb->stBase);
    LOGE_IF(u32 != VPP_OK, "ERROR: unable to unregister stats, u32=%u", u32);

    free(pstCb);
}

uint32_t u32VppIpHvx_Open(void *ctx)
{
    LOGI("%s", __func__);

    t_StVppIpHvxCb *pstCb;
    t_StVppIpCmd stCmd;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = HVX_CB_GET(ctx);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_INITED))
        return VPP_ERR_STATE;

    // Validate that the port configuration is valid
    if (u32VppIpHvx_ValidateConfig(pstCb->stInput.stParam,
                                   pstCb->stOutput.stParam) != VPP_OK)
    {
        LOGE("given invalid port configs.");
        return VPP_ERR_PARAM;
    }

    // Clear internal flags except IP_WORKER_STARTED
    pstCb->u32InternalFlags &= IP_WORKER_STARTED;

#ifndef HVX_SINGLE_THREAD
    stCmd.eCmd = VPP_IP_CMD_OPEN;
    u32VppIpHvx_CmdPut(pstCb, stCmd);
#else
    u32VppIpHvx_ProcCmdOpen(pstCb);
#endif

    LOGI(">> waiting on semaphore");
    sem_wait(&pstCb->sem);
    LOGI(">> got semaphore");

    return pstCb->async_res.u32OpenRet;
}

uint32_t u32VppIpHvx_Close(void *ctx)
{
    LOGI("%s", __func__);

    t_StVppIpHvxCb *pstCb;
    t_StVppIpCmd stCmd;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = HVX_CB_GET(ctx);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
        return VPP_ERR_STATE;

    stCmd.eCmd = VPP_IP_CMD_CLOSE;
    u32VppIpHvx_CmdPut(pstCb, stCmd);

    LOGI(">> waiting on semaphore");
    sem_wait(&pstCb->sem);
    LOGI(">> got semaphore");

    return pstCb->async_res.u32CloseRet;
}

uint32_t u32VppIpHvx_SetParam(void *ctx, enum vpp_port ePort,
                              struct vpp_port_param stParam)
{
    LOGI("%s", __func__);

    uint32_t u32Ret = VPP_OK;
    t_StVppIpHvxCb *pstCb;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = HVX_CB_GET(ctx);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_INITED))
        return VPP_ERR_STATE;

    if (u32VppIpHvx_ValidatePortParam(stParam) != VPP_OK)
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

    pstCb->stCfg.u32ComputeMask |= HVX_PARAM;

    pthread_mutex_unlock(&pstCb->mutex);

    return u32Ret;
}

uint32_t u32VppIpHvx_SetCtrl(void *ctx, struct hqv_control stCtrl)
{
    LOGD("%s ctrl.mode=%d ctrl.ctrl_type=%d", __func__, stCtrl.mode, stCtrl.ctrl_type);

    t_StVppIpHvxCb *pstCb;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);

    if (u32VppIpHvx_ValidateCtrl(stCtrl) != VPP_OK)
    {
        LOGE("given invalid ctrls");
        return VPP_ERR_PARAM;
    }

    pstCb = HVX_CB_GET(ctx);
    pthread_mutex_lock(&pstCb->mutex);

    if(stCtrl.ctrl_type == HQV_CONTROL_GLOBAL_DEMO &&
       (stCtrl.mode == HQV_MODE_AUTO || stCtrl.mode == HQV_MODE_MANUAL))
    {
        pstCb->stCfg.stDemo = stCtrl.demo;
        if(stCtrl.demo.process_percent < HVX_ROI_PERCENTAGE_MAX)
        {
            pstCb->stCfg.u32EnableMask |= HVX_ALGO_ROI;
            pstCb->stCfg.u32ComputeMask |= HVX_ALGO_ROI;
        }
        else
        {
            pstCb->stCfg.u32EnableMask &= ~HVX_ALGO_ROI;
            pstCb->stCfg.u32ComputeMask |= HVX_ALGO_ROI;
        }
    }
    else if(stCtrl.mode == HQV_MODE_AUTO)
    {
        pstCb->stCfg.u32AutoHqvEnable = 1;

        pstCb->stCfg.u32ComputeMask |= HVX_PARAM;
    }
    else if(stCtrl.mode == HQV_MODE_MANUAL)
    {
        if(pstCb->stCfg.u32AutoHqvEnable)
        {
            pstCb->stCfg.u32EnableMask &= ~HVX_ALGO_DI;
            pstCb->stCfg.u32EnableMask &= ~HVX_ALGO_CNR;
            pstCb->stCfg.u32EnableMask &= ~HVX_ALGO_AIE;

            pstCb->stCfg.u32AutoHqvEnable = 0;
        }

        if ((stCtrl.ctrl_type == HQV_CONTROL_AIE)
            && bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_IE))
        {
            pstCb->stCfg.stAie = stCtrl.aie;

            if (pstCb->stCfg.stAie.mode == HQV_MODE_AUTO ||
                pstCb->stCfg.stAie.mode == HQV_MODE_MANUAL)
            {
                pstCb->stCfg.u32EnableMask |= HVX_ALGO_AIE;
                pstCb->stCfg.u32ComputeMask |= HVX_ALGO_AIE;
            }
            else
            {
                pstCb->stCfg.u32EnableMask &= ~HVX_ALGO_AIE;
            }
        }
        else if ((stCtrl.ctrl_type == HQV_CONTROL_CNR)
                 && (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_NR)))
        {
            pstCb->stCfg.stCnr = stCtrl.cnr;

            if (pstCb->stCfg.stCnr.mode == HQV_MODE_AUTO ||
                pstCb->stCfg.stCnr.mode == HQV_MODE_MANUAL)
            {
                pstCb->stCfg.u32EnableMask |= HVX_ALGO_CNR;
                pstCb->stCfg.u32ComputeMask |= HVX_ALGO_CNR;
            }
            else
            {
                pstCb->stCfg.u32EnableMask &= ~HVX_ALGO_CNR;
            }
        }
        else if ((stCtrl.ctrl_type == HQV_CONTROL_DI)
                 && (bVppIpHvxCore_IsAlgoSupported(VPP_FUNC_ID_MVP)))
        {
            pstCb->stCfg.stDi = stCtrl.di;

            if (pstCb->stCfg.stDi.mode == HQV_DI_MODE_VIDEO_1F ||
                pstCb->stCfg.stDi.mode == HQV_DI_MODE_VIDEO_3F ||
                pstCb->stCfg.stDi.mode == HQV_DI_MODE_AUTO)
            {
                pstCb->stCfg.u32EnableMask |= HVX_ALGO_DI;
                pstCb->stCfg.u32ComputeMask |= HVX_ALGO_DI;
            }
            else
            {
                pstCb->stCfg.u32EnableMask &= ~HVX_ALGO_DI;
            }
        }
        else
        {
            LOGE("Unsupported HVX control: ctrl_type=%u", stCtrl.ctrl_type);
            pthread_mutex_unlock(&pstCb->mutex);
            return VPP_ERR_INVALID_CFG;
        }
    }
    else if (stCtrl.mode == HQV_MODE_OFF)
    {
        LOGD("%s(): ctrl.mode is HQV_MODE_OFF", __func__);
        pstCb->stCfg.u32EnableMask &= ~(HVX_ALGO_AIE | HQV_CONTROL_CNR | HQV_CONTROL_DI);
    }
    else
    {
        LOGE("Unsupported HVX mode: mode=%u", stCtrl.mode);
        pthread_mutex_unlock(&pstCb->mutex);
        return VPP_ERR_INVALID_CFG;
    }
    pthread_mutex_unlock(&pstCb->mutex);

    return VPP_OK;
}

uint32_t u32VppIpHvx_GetBufferRequirements(void *ctx,
                                           t_StVppIpBufReq *pstInputBufReq,
                                           t_StVppIpBufReq *pstOutputBufReq)
{
    LOGI("%s", __func__);

    t_StVppIpHvxCb *pstCb;
    uint32_t u32PxSz;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = HVX_CB_GET(ctx);

    u32PxSz = u32VppUtils_GetPxBufferSize(&pstCb->stInput.stParam);

    if (pstInputBufReq)
    {
        pstInputBufReq->u32MinCnt = pstCb->u32InputMaxIntoProc;
        pstInputBufReq->u32PxSz = u32PxSz;
        pstInputBufReq->u32ExSz = 0;
    }

    if (pstOutputBufReq)
    {
        pstOutputBufReq->u32MinCnt = pstCb->u32OutputMaxIntoProc;
        pstOutputBufReq->u32PxSz = u32PxSz;
        pstOutputBufReq->u32ExSz = 0;
    }

    return VPP_OK;
}

uint32_t u32VppIpHvx_QueueBuf(void *ctx, enum vpp_port ePort, t_StVppBuf *pBuf)
{
    uint32_t u32Ret = VPP_OK;
    t_StVppIpHvxCb *pstCb;

    if (ePort != VPP_PORT_INPUT && ePort != VPP_PORT_OUTPUT)
    {
        LOGE("Error, received buffer with invalid port, port=%u", ePort);
        return VPP_ERR_PARAM;
    }

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = HVX_CB_GET(ctx);

    VPP_RET_IF_NULL(pstCb, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pBuf, VPP_ERR_PARAM);

    LOG_ENTER_ARGS("ctx=%p, ePort=%u, pBuf=%p, pBuf->stPixel.pvBase=%p", ctx, ePort,
                   pBuf, pBuf->stPixel.pvBase);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGE("Error, received buffer in invalid state, expected=%u, actual=%u",
             VPP_IP_STATE_ACTIVE, pstCb->eState);
        return VPP_ERR_STATE;
    }

    vVppIpCbLog(&pstCb->stBase.stCb, pBuf, eVppLogId_IpQueueBuf);

#ifdef HVX_FORCE_DUMP_BUF
    VPP_FLAG_SET(pBuf->u32InternalFlags, VPP_BUF_FLAG_DUMP);
#endif

#ifdef HVX_REG_BUFFER_ON_QUEUE
    vVppIpHvxCore_RegisterBuffer(pstCb->pstHvxCore, &pBuf->stPixel);
#endif

    VPP_FLAG_CLR(pBuf->u32InternalFlags, VPP_BUF_FLAG_INTERNAL_BYPASS);

    if (ePort == VPP_PORT_INPUT)
    {
        VPP_FLAG_CLR(pBuf->u32InternalFlags, VPP_BUF_FLAG_EOS_PROCESSED);
        VPP_FLAG_CLR(pBuf->u32InternalFlags, VPP_BUF_FLAG_IDR_PROCESSED);
        VPP_FLAG_CLR(pBuf->u32InternalFlags, VPP_BUF_FLAG_DC_PROCESSED);

        VPP_HVX_CRC_BUF(pstCb, pBuf);

        if (pBuf->stPixel.u32FilledLen == 0)
        {
            VPP_FLAG_SET(pBuf->u32InternalFlags, VPP_BUF_FLAG_INTERNAL_BYPASS);
        }

        u32Ret = u32VppIp_PortBufPut(&pstCb->stInput, pBuf, &pstCb->mutex, &pstCb->cond);
        pstCb->stats.u32InQueuedCnt++;
    }
    else if (ePort == VPP_PORT_OUTPUT)
    {
        u32Ret = u32VppIp_PortBufPut(&pstCb->stOutput, pBuf, &pstCb->mutex, &pstCb->cond);
    }

#ifdef HVX_SINGLE_THREAD
    while (u32ProcBufReqMet(pstCb))
    {
        u32VppIpHvx_ProcessBuffer(pstCb);
    }
#endif

    return u32Ret;
}

uint32_t u32VppIpHvx_Flush(void *ctx, enum vpp_port ePort)
{
    LOGI("%s", __func__);

    uint32_t u32Ret = VPP_OK;
    t_StVppIpHvxCb *pstCb;
    t_StVppIpCmd stCmd;

    if (ePort != VPP_PORT_INPUT && ePort != VPP_PORT_OUTPUT)
    {
        LOGE("ERROR: calling flush with invalid port, port=%u", ePort);
        return VPP_ERR_PARAM;
    }

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = HVX_CB_GET(ctx);

    stCmd.eCmd = VPP_IP_CMD_FLUSH;
    stCmd.flush.ePort = ePort;
    u32VppIpHvx_CmdPut(pstCb, stCmd);

    return u32Ret;
}

uint32_t u32VppIpHvx_Drain(void *ctx)
{
    LOGI("%s", __func__);

    uint32_t u32Ret = VPP_OK;
    t_StVppIpHvxCb *pstCb;
    t_StVppIpCmd stCmd;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = HVX_CB_GET(ctx);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
    {
        LOGE("ERROR: calling drain in invalid state, expected=%u, actual=%u",
             VPP_IP_STATE_ACTIVE, pstCb->eState);
        return VPP_ERR_STATE;
    }

    stCmd.eCmd = VPP_IP_CMD_DRAIN;
    u32VppIpHvx_CmdPut(pstCb, stCmd);

    return u32Ret;
}

uint32_t u32VppIpHvx_Reconfigure(void *ctx,
                                 struct vpp_port_param in_param,
                                 struct vpp_port_param out_param)
{
    LOG_ENTER();

    uint32_t u32Ret = VPP_OK;
    t_StVppIpHvxCb *pstCb;
    t_StVppIpCmd stCmd;

    VPP_RET_IF_NULL(ctx, VPP_ERR_PARAM);
    pstCb = HVX_CB_GET(ctx);

    if (!VPP_IP_STATE_EQUAL(pstCb, VPP_IP_STATE_ACTIVE))
        return VPP_ERR_STATE;

    // Validate that the port configuration is valid
    if (u32VppIpHvx_ValidateConfig(in_param, out_param) != VPP_OK)
        return VPP_ERR_PARAM;

    pthread_mutex_lock(&pstCb->mutex);

    if(u32VppBufPool_Cnt(&pstCb->stInput.stPendingQ))
        u32Ret = VPP_ERR_STATE;
    else
    {
        pstCb->stInput.stParam = in_param;
        pstCb->stOutput.stParam = out_param;
        pstCb->stCfg.u32ComputeMask |= HVX_PARAM;
        vVppIpHvx_Compute(pstCb);
    }

    pthread_mutex_unlock(&pstCb->mutex);

    pstCb->async_res.u32ReconfigRet = VPP_OK;
    stCmd.eCmd = VPP_IP_CMD_RECONFIGURE;
    u32VppIpHvx_CmdPut(pstCb, stCmd);

    LOGI(">> waiting on semaphore (reconfig)");
    sem_wait(&pstCb->sem);
    LOGI(">> got semaphore (reconfig)");

    if (pstCb->async_res.u32ReconfigRet != VPP_OK)
    {
        u32Ret = pstCb->async_res.u32ReconfigRet;
        LOGE("ERROR: reconfigure failed, u32=%u", u32Ret);
    }

    // Reset proc stats
    VPP_IP_PROF_RESET_SINGLE(&pstCb->stBase, HVX_STAT_PROC);

    LOG_EXIT_RET(u32Ret);
}
