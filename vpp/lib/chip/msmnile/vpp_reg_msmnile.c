/*!
 * @file vpp_reg_msmnile.c
 *
 * @cr
 * Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
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

#ifdef VPP_TARGET_USES_HCP
#include "vpp_ip_hcp.h"
#endif

#if defined(VPP_TARGET_USES_FRC_MC)
#define VPP_TARGET_USES_FRC
#include "vpp_ip_frc_mc.h"
#endif

#ifndef OMX_EXTRADATA_DOES_NOT_EXIST
#include "OMX_QCOMExtns.h"
#endif

/************************************************************************
 * Local functions
 ***********************************************************************/
static uint32_t u32VppReg_ValidateUcFrcMsmnile(t_StVppUcSessCfg *pstSessCfg)
{
    struct StVppParam *pstParam;

    VPP_RET_IF_NULL(pstSessCfg, VPP_ERR_PARAM);
    VPP_RET_IF_NULL(pstSessCfg->pstParam, VPP_ERR_PARAM);

    pstParam = pstSessCfg->pstParam;

    // Max dimensions are fixed, can't be rotated
    if (pstParam->input_port.width > RES_FHD_MAX_W ||
        pstParam->input_port.height > RES_FHD_MAX_H)
    {
        LOGE("chipset validation failed: Input: W:%u H:%u, Max: W:%u H:%u",
             pstParam->input_port.width, pstParam->input_port.height,
             RES_FHD_MAX_W, RES_FHD_MAX_H);
        return VPP_ERR_INVALID_CFG;
    }

    return VPP_OK;
}

/************************************************************************
 * Common Definition
 ***********************************************************************/
static t_EVppColorFmt color_fmt_common[] = {
    VPP_COLOR_FORMAT_NV12_VENUS,
    VPP_COLOR_FORMAT_NV21_VENUS,
    VPP_COLOR_FORMAT_P010,
    VPP_COLOR_FORMAT_UBWC_NV12,
    VPP_COLOR_FORMAT_UBWC_NV21,
    VPP_COLOR_FORMAT_UBWC_TP10,
};

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
#endif

static t_StVppPortCfg port_cfg_hcp_common[] = {
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
            VPP_UC_PORT(VPP_COLOR_FORMAT_P010, VPP_COLOR_FORMAT_P010, \
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
    {
        VPP_UC_INTERNAL_REQ(1)
        {
            VPP_UC_PORT(VPP_COLOR_FORMAT_UBWC_TP10, VPP_COLOR_FORMAT_UBWC_TP10, \
                        0, eVppBufPxDataType_Raw)
        },
    },
};

/************************************************************************
 * HCP Definition
 ***********************************************************************/
#ifdef VPP_TARGET_USES_HCP
static t_StVppAlgo algo_hcp[] = {
    {
        .ctrl = HQV_CONTROL_AIE,
    },
    {
        .ctrl = HQV_CONTROL_EAR,
#ifndef OMX_EXTRADATA_DOES_NOT_EXIST
        .u32MetaCnt = 2,
        .meta = (uint32_t []) {
            OMX_QcomIndexParamVideoQPExtraData,
            OMX_QcomIndexParamFrameInfoExtraData,
        },
#endif
    },
    {
        .ctrl = HQV_CONTROL_GLOBAL_DEMO,
    },
};

VPP_IP_DECL(HCP,
            VPP_IP_HCP,
            algo_hcp,
            960,            // Max credits
            VPP_QUIRK_NONE, // Quirks
            0,              // stride multiple
            0,              // scanline multiple
            8,              // max concurrent sessions
            VPP_TRUE,       // supports cpz?
            VPP_FALSE,      // requires mmap'd buffers?
            RES_UHD_MAX_W,  // max supported width
            RES_UHD_MAX_H,  // max supported height
            128,            // min supported width
            96,             // min supported height
            color_fmt_common, // supported input port color formats
            color_fmt_common, // supported output port color formats
            u32VppIpHcp_Boot,
            u32VppIpHcp_Shutdown,
            vpVppIpHcp_Init,
            vVppIpHcp_Term,
            u32VppIpHcp_Open,
            u32VppIpHcp_Close,
            u32VppIpHcp_SetParam,
            u32VppIpHcp_SetCtrl,
            u32VppIpHcp_GetBufferRequirements,
            u32VppIpHcp_QueueBuf,
            u32VppIpHcp_Flush,
            u32VppIpHcp_Drain,
            u32VppIpHcp_Reconfigure);
#endif

/************************************************************************
 * FRC Definition
 ***********************************************************************/
#ifdef VPP_TARGET_USES_FRC
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
            2,              // max concurrent sessions
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
            vpVppIpFrcMc_Init,
            vVppIpFrcMc_Term,
            u32VppIpFrcMc_Open,
            u32VppIpFrcMc_Close,
            u32VppIpFrcMc_SetParam,
            u32VppIpFrcMc_SetCtrl,
            u32VppIpFrcMc_GetBufferRequirements,
            u32VppIpFrcMc_QueueBuf,
            u32VppIpFrcMc_Flush,
            u32VppIpFrcMc_Drain,
            u32VppIpFrcMc_Reconfigure);
#endif

/************************************************************************
 * Usecase Definition
 ***********************************************************************/

/************************************************************************
 * HCP Usecase Definition
 ***********************************************************************/
#ifdef VPP_TARGET_USES_HCP
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
            VPP_UC_COMP(HCP, 8, VPP_FALSE)
        },
    },
    .u32IpCnt = 1,
    .credits = {
        VPP_UC_CREDITS(VPP_IP_HCP, HCP, 2, 2, 8, 16)
    },
    VPP_UC_PORT_CFG(port_cfg_hcp_common)
    VPP_UC_FACTOR(0, 1, 0, 1)
    VPP_UC_DEFAULT_FPS(30)
    VPP_UC_FALLBACK(NULL, NULL, NULL, NULL)
};

static t_StVppUsecase uc_ear = {
    .name = "EAR",
    .algos = {
        .ctrls = (enum hqv_control_type[]) {
            HQV_CONTROL_EAR,
        },
        .u32CtrlCnt = 1,
    },
    .composition = (t_StVppUsecaseIp[]) {
        {
            VPP_UC_COMP(HCP, 8, VPP_FALSE)
        },
    },
    .u32IpCnt = 1,
    .credits = {
        VPP_UC_CREDITS(VPP_IP_HCP, HCP, 2, 2, 8, 16)
    },
    VPP_UC_PORT_CFG(port_cfg_hcp_common)
    VPP_UC_FACTOR(0, 1, 0, 1)
    VPP_UC_DEFAULT_FPS(30)
    VPP_UC_FALLBACK(NULL, NULL, NULL, NULL)
};

static t_StVppUsecase uc_ear_aie = {
    .name = "EAR+AIE",
    .algos = {
        .ctrls = (enum hqv_control_type[]) {
            HQV_CONTROL_EAR,
            HQV_CONTROL_AIE,
        },
        .u32CtrlCnt = 2,
    },
    .composition = (t_StVppUsecaseIp[]) {
        {
            VPP_UC_COMP(HCP, 8, VPP_FALSE)
        },
    },
    .u32IpCnt = 1,
    .credits = {
        VPP_UC_CREDITS(VPP_IP_HCP, HCP, 2, 2, 8, 16)
    },
    VPP_UC_PORT_CFG(port_cfg_hcp_common)
    VPP_UC_FACTOR(0, 1, 0, 1)
    VPP_UC_DEFAULT_FPS(30)
    VPP_UC_FALLBACK(NULL, NULL, NULL, NULL)
};
#endif

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
        VPP_UC_CREDITS(VPP_IP_FRC, FRC, 60, 60, 60, NOT_SUPPORTED)
    },
    VPP_UC_PORT_CFG(port_cfg_frc_common)
    VPP_UC_FACTOR(1, 1, 0, 3)
    VPP_UC_DEFAULT_FPS(7)
    VPP_UC_FALLBACK(NULL, NULL, NULL, NULL)
    .pu32ChipsetValidate = u32VppReg_ValidateUcFrcMsmnile,
};
#endif

#ifdef VPP_TARGET_USES_HCP
static t_StVppUsecase uc_auto = {
    .name = "AUTO",
    .algos = {
        .ctrls = (enum hqv_control_type[]) {
            HQV_CONTROL_EAR,
            HQV_CONTROL_AIE,
        },
        .u32CtrlCnt = 2,
    },
    .composition = (t_StVppUsecaseIp[]) {
        {
            VPP_UC_COMP(HCP, 8, VPP_FALSE)
        },
    },
    .u32IpCnt = 1,
    .credits = {
        VPP_UC_CREDITS(VPP_IP_HCP, HCP, 2, 2, 8, 16)
    },
    VPP_UC_PORT_CFG(port_cfg_hcp_common)
    VPP_UC_FACTOR(0, 1, 0, 1)
    VPP_UC_DEFAULT_FPS(30)
    VPP_UC_FALLBACK(NULL, NULL, NULL, NULL)
};
#endif

static t_StVppUsecase * apstUcmsmnile[] = {
#if defined(VPP_TARGET_USES_HCP)
    &uc_aie,
    &uc_ear,
    &uc_ear_aie,
#endif
#if defined(VPP_TARGET_USES_FRC)
    &uc_frc,
#endif
};

t_StVppUsecase ** ppstVppUsecase_GetRegistry(uint32_t *o_pu32Cnt)
{
    *o_pu32Cnt = sizeof(apstUcmsmnile) / sizeof(t_StVppUsecase *);
    return apstUcmsmnile;
}

t_StVppUsecase * pstVppUsecase_GetAuto()
{
#ifdef VPP_TARGET_USES_HCP
    return &uc_auto;
#else
    return NULL;
#endif
}
