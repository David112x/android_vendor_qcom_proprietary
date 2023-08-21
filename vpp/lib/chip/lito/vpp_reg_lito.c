/*!
 * @file vpp_reg_lito.c
 *
 * @cr
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <sys/types.h>

#include "vpp.h"
#include "vpp_reg.h"
#include "vpp_utils.h"
#include "vpp_dbg.h"

#ifdef VPP_TARGET_USES_HCP
#include "vpp_ip_hcp.h"
#endif

#if defined(VPP_TARGET_USES_FRC_MC)
#define VPP_TARGET_USES_FRC
#include "vpp_ip_frc_mc.h"
#endif

#ifdef VPP_TARGET_USES_HVX
#include "vpp_ip_hvx.h"
#endif

#ifndef OMX_EXTRADATA_DOES_NOT_EXIST
#include "OMX_QCOMExtns.h"
#endif

/************************************************************************
 * HCP Definition
 ***********************************************************************/
#ifdef VPP_TARGET_USES_HCP
static t_EVppColorFmt color_fmt_hcp[] = {
    VPP_COLOR_FORMAT_NV12_VENUS,
    VPP_COLOR_FORMAT_NV21_VENUS,
    VPP_COLOR_FORMAT_P010,
    VPP_COLOR_FORMAT_UBWC_NV12,
    VPP_COLOR_FORMAT_UBWC_NV21,
    VPP_COLOR_FORMAT_UBWC_TP10,
};

static t_StVppPortCfg port_cfg_hcp[] = {
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

static t_StVppAlgo algo_hcp[] = {
    {
        .ctrl = HQV_CONTROL_AIE,
    },
    {
        .ctrl = HQV_CONTROL_EAR,
#ifndef OMX_EXTRADATA_DOES_NOT_EXIST
        .u32MetaCnt = 2,
        .meta = (uint32_t []) {
            OMX_QTI_ExtraDataCategory_Basic,
            OMX_QTI_ExtraDataCategory_Advanced,
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
            color_fmt_hcp, // supported input port color formats
            color_fmt_hcp, // supported output port color formats
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
static t_EVppColorFmt color_fmt_frc[] = {
    VPP_COLOR_FORMAT_UBWC_NV12,
};

static t_StVppPortCfg port_cfg_frc[] = {
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
 * HVX Definition
 ***********************************************************************/
#ifdef VPP_TARGET_USES_HVX
static t_EVppColorFmt color_fmt_hvx[] = {
    VPP_COLOR_FORMAT_UBWC_NV12,
    VPP_COLOR_FORMAT_UBWC_NV21,
};

static t_StVppPortCfg port_cfg_hvx[] = {
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
        .ctrl = HQV_CONTROL_CNR,
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
            RES_FHD_MAX_W,     // max supported width
            RES_FHD_MAX_H,     // max supported height
            256,               // min supported width
            128,                // min supported height
            color_fmt_hvx,     // supported input port color formats
            color_fmt_hvx,     // supported output port color formats
            u32VppIpHvx_Boot,
            u32VppIpHvx_Shutdown,
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
 * Multi-IP Definition
 ***********************************************************************/
#if defined(VPP_TARGET_USES_HVX) && defined(VPP_TARGET_USES_HCP)
static t_StVppPortCfg port_cfg_hvx_hcp[] = {
    {
        VPP_UC_INTERNAL_REQ(2)
        {
            VPP_UC_PORT(VPP_COLOR_FORMAT_UBWC_NV12, VPP_COLOR_FORMAT_UBWC_NV12, \
                        10, eVppBufPxDataType_Raw)
            VPP_UC_PORT(VPP_COLOR_FORMAT_UBWC_NV12, VPP_COLOR_FORMAT_UBWC_NV12, \
                        0, eVppBufPxDataType_Raw)
        },
    },
    {
        VPP_UC_INTERNAL_REQ(2)
        {
            VPP_UC_PORT(VPP_COLOR_FORMAT_UBWC_NV21, VPP_COLOR_FORMAT_UBWC_NV21, \
                        10, eVppBufPxDataType_Raw)
            VPP_UC_PORT(VPP_COLOR_FORMAT_UBWC_NV21, VPP_COLOR_FORMAT_UBWC_NV21, \
                        0, eVppBufPxDataType_Raw)
        },
    },
};
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
    VPP_UC_PORT_CFG(port_cfg_hcp)
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
    VPP_UC_PORT_CFG(port_cfg_hcp)
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
    VPP_UC_PORT_CFG(port_cfg_hcp)
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
        VPP_UC_CREDITS(VPP_IP_FRC, FRC, 30, 30, 60, NOT_SUPPORTED)
    },
    VPP_UC_PORT_CFG(port_cfg_frc)
    VPP_UC_FACTOR(1, 1, 0, 3)
    VPP_UC_DEFAULT_FPS(7)
    VPP_UC_FALLBACK(NULL, NULL, NULL, NULL)
};
#endif

/************************************************************************
 * MCNR Usecase Definition
 ***********************************************************************/
#ifdef VPP_TARGET_USES_HVX
static t_StVppUsecase uc_mcnr = {
    .name = "MCNR",
    .algos = {
        .ctrls = (enum hqv_control_type[]) {
            HQV_CONTROL_CNR,
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
    VPP_UC_PORT_CFG(port_cfg_hvx)
    VPP_UC_FACTOR(0, 1, 0, 1)
    VPP_UC_DEFAULT_FPS(30)
    VPP_UC_FALLBACK(NULL, NULL, NULL, NULL)
};
#endif

/************************************************************************
 * Multi-IP Usecase Definition
 ***********************************************************************/
#if (defined(VPP_TARGET_USES_HVX) && defined(VPP_TARGET_USES_HCP))
static t_StVppUsecase uc_mcnr_aie = {
    .name = "MCNR + AIE",
    .algos = {
        .ctrls = (enum hqv_control_type[]) {
            HQV_CONTROL_CNR,
            HQV_CONTROL_AIE,
        },
        .u32CtrlCnt = 2,
    },
    .composition = (t_StVppUsecaseIp[]) {
        {
            VPP_UC_COMP(HVX, 2, VPP_FALSE)
        },
        {
            VPP_UC_COMP(HCP, 8, VPP_FALSE)
        },
    },
    .u32IpCnt = 2,
    .credits = {
        VPP_UC_CREDITS(VPP_IP_HVX, HVX, 15, 15, 15, NOT_SUPPORTED)
        VPP_UC_CREDITS(VPP_IP_HCP, HCP, 2, 2, 8, NOT_SUPPORTED)
    },
    VPP_UC_PORT_CFG(port_cfg_hvx_hcp)
    VPP_UC_FACTOR(0, 1, 0, 1)
    VPP_UC_DEFAULT_FPS(30)
    VPP_UC_FALLBACK(NULL, NULL, NULL, &uc_aie)
};

static t_StVppUsecase uc_mcnr_ear = {
    .name = "MCNR + EAR",
    .algos = {
        .ctrls = (enum hqv_control_type[]) {
            HQV_CONTROL_CNR,
            HQV_CONTROL_EAR,
        },
        .u32CtrlCnt = 2,
    },
    .composition = (t_StVppUsecaseIp[]) {
        {
            VPP_UC_COMP(HVX, 2, VPP_FALSE)
        },
        {
            VPP_UC_COMP(HCP, 8, VPP_FALSE)
        },
    },
    .u32IpCnt = 2,
    .credits = {
        VPP_UC_CREDITS(VPP_IP_HVX, HVX, 15, 15, 15, NOT_SUPPORTED)
        VPP_UC_CREDITS(VPP_IP_HCP, HCP, 2, 2, 8, NOT_SUPPORTED)
    },
    VPP_UC_PORT_CFG(port_cfg_hvx_hcp)
    VPP_UC_FACTOR(0, 1, 0, 1)
    VPP_UC_DEFAULT_FPS(30)
    VPP_UC_FALLBACK(NULL, NULL, NULL, &uc_ear)
};

static t_StVppUsecase uc_mcnr_ear_aie = {
    .name = "MCNR + EAR + AIE",
    .algos = {
        .ctrls = (enum hqv_control_type[]) {
            HQV_CONTROL_CNR,
            HQV_CONTROL_EAR,
            HQV_CONTROL_AIE,
        },
        .u32CtrlCnt = 3,
    },
    .composition = (t_StVppUsecaseIp[]) {
        {
            VPP_UC_COMP(HVX, 2, VPP_FALSE)
        },
        {
            VPP_UC_COMP(HCP, 8, VPP_FALSE)
        },
    },
    .u32IpCnt = 2,
    .credits = {
        VPP_UC_CREDITS(VPP_IP_HVX, HVX, 15, 15, 15, NOT_SUPPORTED)
        VPP_UC_CREDITS(VPP_IP_HCP, HCP, 2, 2, 8, NOT_SUPPORTED)
    },
    VPP_UC_PORT_CFG(port_cfg_hvx_hcp)
    VPP_UC_FACTOR(0, 1, 0, 1)
    VPP_UC_DEFAULT_FPS(30)
    VPP_UC_FALLBACK(NULL, NULL, NULL, &uc_ear_aie)
};
#endif

/************************************************************************
 * Auto Usecase Definition
 ***********************************************************************/
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
    VPP_UC_PORT_CFG(port_cfg_hcp)
    VPP_UC_FACTOR(0, 1, 0, 1)
    VPP_UC_DEFAULT_FPS(30)
    VPP_UC_FALLBACK(NULL, NULL, NULL, NULL)
};
#endif

static t_StVppUsecase * apstUcLito[] = {
#if defined(VPP_TARGET_USES_HCP)
    &uc_aie,
    &uc_ear,
    &uc_ear_aie,
#endif
#if defined(VPP_TARGET_USES_FRC)
    &uc_frc,
#endif
#if defined(VPP_TARGET_USES_HVX)
    &uc_mcnr,
#endif
#if (defined(VPP_TARGET_USES_HVX) && defined(VPP_TARGET_USES_HCP))
    &uc_mcnr_aie,
    &uc_mcnr_ear,
    &uc_mcnr_ear_aie,
#endif
};

t_StVppUsecase ** ppstVppUsecase_GetRegistry(uint32_t *o_pu32Cnt)
{
    VPP_RET_IF_NULL(o_pu32Cnt, NULL);

    if (u32VppUtils_IsSoc(SAIPAN) == VPP_TRUE)
    {
        *o_pu32Cnt = sizeof(apstUcLito) / sizeof(t_StVppUsecase *);
        return apstUcLito;
    }
    *o_pu32Cnt = 0;
    return NULL;
}

t_StVppUsecase * pstVppUsecase_GetAuto()
{
    if (u32VppUtils_IsSoc(SAIPAN) == VPP_TRUE)
    {
#ifdef VPP_TARGET_USES_HCP
        return &uc_auto;
#else
        return NULL;
#endif
    }
    return NULL;
}
