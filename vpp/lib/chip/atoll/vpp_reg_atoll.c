/*!
 * @file vpp_reg_lito.c
 *
 * @cr
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <sys/types.h>

#include "vpp.h"
#include "vpp_dbg.h"
#include "vpp_utils.h"
#include "vpp_reg.h"
#include "vpp_core.h"

#if defined(VPP_TARGET_USES_FRC_CORE) && defined(VPP_TARGET_USES_FRC_ME) && defined(VPP_TARGET_USES_FRC_MC)
#define VPP_TARGET_USES_FRC
#include "vpp_ip_frc_core.h"
#endif

#ifdef VPP_TARGET_USES_HVX
#include "vpp_ip_hvx.h"
#endif

#ifndef OMX_EXTRADATA_DOES_NOT_EXIST
#include "OMX_QCOMExtns.h"
#endif

/************************************************************************
 * Local functions
 ***********************************************************************/
static uint32_t u32VppReg_ValidateUcFrc(t_StVppUcSessCfg *pstSessCfg)
{
    struct StVppHqvCtrl *pstVppCtrl;
    struct vpp_ctrl_frc_segment *pstSegment;
    struct hqv_control stCtrl;
    uint32_t i;

    VPP_RET_IF_NULL(pstSessCfg, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstSessCfg->pstVppCtrl, VPP_ERR_PARAM);

    pstVppCtrl = pstSessCfg->pstVppCtrl;
    stCtrl.frc = pstVppCtrl->frc;

    if (pstVppCtrl->mode == HQV_MODE_OFF || pstVppCtrl->mode == HQV_MODE_AUTO)
        return VPP_OK;

    if (stCtrl.frc.num_segments == 0 || stCtrl.frc.segments == NULL)
        return VPP_OK;

    for (i = 0; i < stCtrl.frc.num_segments; i++)
    {
        pstSegment = &stCtrl.frc.segments[i];
        if (pstSegment->mode == HQV_FRC_MODE_SLOMO &&
            pstSegment->interp != HQV_FRC_INTERP_1X &&
            pstSegment->interp != HQV_FRC_INTERP_2X)
        {
            LOGE("SloMo ctrl seg[%u] not supported. Interp=%u (max=%u) Validate failed",
                 i, pstSegment->interp, HQV_FRC_INTERP_2X);
            return VPP_ERR_INVALID_CFG;
        }
    }

    return VPP_OK;
}

/************************************************************************
 * Common Definition
 ***********************************************************************/
static t_EVppColorFmt color_fmt_common[] = {
    VPP_COLOR_FORMAT_NV12_VENUS,
    VPP_COLOR_FORMAT_NV21_VENUS,
    VPP_COLOR_FORMAT_UBWC_NV12,
    VPP_COLOR_FORMAT_UBWC_NV21,
};

/************************************************************************
 * FRC Definition
 ***********************************************************************/
#ifdef VPP_TARGET_USES_FRC
static t_EVppColorFmt color_fmt_frc[] = {
    VPP_COLOR_FORMAT_UBWC_NV12,
};

static t_StVppPortCfg port_cfg_frc_common[] = {
    {
        VPP_UC_INTERNAL_REQ(1)
        {
            VPP_UC_PORT(VPP_COLOR_FORMAT_UBWC_NV12, VPP_COLOR_FORMAT_UBWC_NV12, \
                        0, eVppBufPxDataType_Raw)
        },
    },
};

static t_StVppAlgo algo_frc[] = {
    {
        .ctrl = HQV_CONTROL_FRC,
    },
};

VPP_IP_DECL(FRC,
            VPP_IP_FRC,
            algo_frc,
            1000,           // Max credits
            VPP_QUIRK_NONE, // Quirks
            0,              // stride multiple
            0,              // scanline multiple
            1,              // max concurrent sessions
            VPP_FALSE,      // supports cpz?
            VPP_FALSE,      // requires mmap'd buffers?
            RES_FHD_MAX_W,  // max supported width
            RES_FHD_MAX_H,  // max supported height
            324,            // min supported width
            128,            // min supported height
            color_fmt_frc,  // supported input port color formats
            color_fmt_frc,  // supported output port color formats
            NULL,
            NULL,
            vpVppIpFrc_Init,
            vVppIpFrc_Term,
            u32VppIpFrc_Open,
            u32VppIpFrc_Close,
            u32VppIpFrc_SetParam,
            u32VppIpFrc_SetCtrl,
            u32VppIpFrc_GetBufferRequirements,
            u32VppIpFrc_QueueBuf,
            u32VppIpFrc_Flush,
            u32VppIpFrc_Drain,
            u32VppIpFrc_Reconfigure);
#endif

/************************************************************************
 * HVX Definition
 ***********************************************************************/
#ifdef VPP_TARGET_USES_HVX
static t_StVppPortCfg port_cfg_hvx_common[] = {
    {
        VPP_UC_INTERNAL_REQ(1)
        {
            VPP_UC_PORT(VPP_COLOR_FORMAT_NV12_VENUS, VPP_COLOR_FORMAT_NV12_VENUS, \
                        0, eVppBufPxDataType_Raw)
        },
    },
    {
        VPP_UC_INTERNAL_REQ(1)
        {
            VPP_UC_PORT(VPP_COLOR_FORMAT_NV21_VENUS, VPP_COLOR_FORMAT_NV21_VENUS, \
                        0, eVppBufPxDataType_Raw)
        },
    },
    {
        VPP_UC_INTERNAL_REQ(1)
        {
            VPP_UC_PORT(VPP_COLOR_FORMAT_UBWC_NV12, VPP_COLOR_FORMAT_UBWC_NV12, \
                        0, eVppBufPxDataType_Raw)
        },
    },
    {
        VPP_UC_INTERNAL_REQ(1)
        {
            VPP_UC_PORT(VPP_COLOR_FORMAT_UBWC_NV21, VPP_COLOR_FORMAT_UBWC_NV21, \
                        0, eVppBufPxDataType_Raw)
        },
    },
};

static t_StVppAlgo algo_hvx[] = {
    {
        .ctrl = HQV_CONTROL_AIE,
    },
    {
        .ctrl = HQV_CONTROL_GLOBAL_DEMO,
    },
};

VPP_IP_DECL(HVX,
            VPP_IP_HVX,
            algo_hvx,
            1000,              // Max credits
            VPP_QUIRK_NONE,    // Quirks
            0,                 // stride multiple
            0,                 // scanline multiple
            2,                 // max concurrent sessions
            VPP_FALSE,         // supports cpz?
            VPP_FALSE,         // requires mmap'd buffers?
            RES_UHD_MAX_W,     // max supported width
            RES_UHD_MAX_H,     // max supported height
            256,               // min supported width
            128,               // min supported height
            color_fmt_common,  // supported input port color formats
            color_fmt_common,  // supported output port color formats
            NULL,
            NULL,
            vpVppIpHvx_Init,
            vVppIpHvx_Term,
            u32VppIpHvx_Open,
            u32VppIpHvx_Close,
            u32VppIpHvx_SetParam,
            u32VppIpHvx_SetCtrl,
            u32VppIpHvx_GetBufferRequirements,
            u32VppIpHvx_QueueBuf,
            u32VppIpHvx_Flush,
            u32VppIpHvx_Drain,
            u32VppIpHvx_Reconfigure);
#endif

/************************************************************************
 * Usecase Definition
 ***********************************************************************/

/************************************************************************
 * FRC Usecase Definition
 ***********************************************************************/
#ifdef VPP_TARGET_USES_FRC
static t_StVppUsecase uc_frc = {
    .name = "FRC",
    .algos = {
        .ctrls = (enum hqv_control_type[]) {
            HQV_CONTROL_FRC,
        },
        .u32CtrlCnt = 1,
    },
    .composition = (t_StVppUsecaseIp[]) {
        {
            VPP_UC_COMP(FRC, 1, VPP_FALSE)
        },
    },
    .u32IpCnt = 1,
    .credits = {
        VPP_UC_CREDITS(VPP_IP_FRC, FRC, 60, 60, NOT_SUPPORTED, NOT_SUPPORTED)
    },
    VPP_UC_PORT_CFG(port_cfg_frc_common)
    VPP_UC_FACTOR(1, 1, 0, 2)
    VPP_UC_DEFAULT_FPS(15)
    VPP_UC_FALLBACK(NULL, NULL, NULL, NULL)
    .pu32ChipsetValidate = u32VppReg_ValidateUcFrc,
};
#endif

/************************************************************************
 * HVX Usecase Definition
 ***********************************************************************/
#if defined(VPP_TARGET_USES_HVX)
static t_StVppUsecase uc_aie = {
    .name = "AIE",
    .algos = {
        .ctrls = (enum hqv_control_type[]) {
            HQV_CONTROL_AIE,
        },
        .u32CtrlCnt = 1,
    },
    .composition = (t_StVppUsecaseIp[]) {
        {
            VPP_UC_COMP(HVX, 2, VPP_FALSE)
        },
    },
    .u32IpCnt = 1,
    .credits = {
        VPP_UC_CREDITS(VPP_IP_HVX, HVX, 15, 15, 15, NOT_SUPPORTED)
    },
    VPP_UC_PORT_CFG(port_cfg_hvx_common)
    VPP_UC_FACTOR(0, 1, 0, 1)
    VPP_UC_DEFAULT_FPS(30)
    VPP_UC_FALLBACK(NULL, NULL, NULL, NULL)
};

static t_StVppUsecase uc_auto = {
    .name = "AUTO",
    .algos = {
        .ctrls = (enum hqv_control_type[]) {
            HQV_CONTROL_AIE,
        },
        .u32CtrlCnt = 1,
    },
    .composition = (t_StVppUsecaseIp[]) {
        {
            VPP_UC_COMP(HVX, 2, VPP_FALSE)
        },
    },
    .u32IpCnt = 1,
    .credits = {
        VPP_UC_CREDITS(VPP_IP_HVX, HVX, 15, 15, 15, NOT_SUPPORTED)
    },
    VPP_UC_PORT_CFG(port_cfg_hvx_common)
    VPP_UC_FACTOR(0, 1, 0, 1)
    VPP_UC_DEFAULT_FPS(30)
    VPP_UC_FALLBACK(NULL, NULL, NULL, NULL)
};
#endif

static t_StVppUsecase * apstUcAtoll[] = {
#if defined(VPP_TARGET_USES_HVX)
    &uc_aie,
#endif
#if defined(VPP_TARGET_USES_FRC)
    &uc_frc,
#endif
};

t_StVppUsecase ** ppstVppUsecase_GetRegistry(uint32_t *o_pu32Cnt)
{
    *o_pu32Cnt = sizeof(apstUcAtoll) / sizeof(t_StVppUsecase *);
    return apstUcAtoll;
}

t_StVppUsecase * pstVppUsecase_GetAuto()
{
#ifdef VPP_TARGET_USES_HVX
    return &uc_auto;
#else
    return NULL;
#endif
}
