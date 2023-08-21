/*!
 * @file HidlVppUtils.cpp
 *
 * @cr
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @services
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include <string.h>
#include <gr_priv_handle.h>

#include "qdMetaData.h"

#include "vpp_dbg.h"
#include "vpp.h"
#include "vpp_def.h"
#include <cutils/native_handle.h>
#include <vendor/qti/hardware/vpp/1.1/types.h>
#include <vendor/qti/hardware/vpp/1.2/types.h>
#include <vendor/qti/hardware/vpp/1.3/types.h>
#include "HidlVpp.h"

uint64_t u64LogLevelIpc;

/************************************************************************
 * Local definitions
 ***********************************************************************/
#define CLEAR_PRIV_HDL_BASE_PARAMS(pH)    {\
                                              ((private_handle_t *)pH)->base = 0;\
                                              ((private_handle_t *)pH)->base_metadata = 0;\
                                          }

#define VPP_PARAM_MAP(_var, _this, _that) \
    case _this: _var = _that; break

namespace qti_vpp {

/*********** From vpp to hidl *******************************************/

uint32_t HidlVpp::HidlVppUtils::vppReqToHidl(const struct vpp_requirements *pstReq, VppRequirements& stReq)
{
    if (!pstReq)
        return VPP_ERR_PARAM;

    stReq.inColorFmtMask = pstReq->in_color_fmt_mask;

    stReq.metadata.cnt = pstReq->metadata.cnt;

    for (size_t i = 0; i < META_MAX_CNT; ++i)
    {
        stReq.metadata.meta[i] = pstReq->metadata.meta[i];
    }

    for (size_t i = 0; i < VPP_RESOLUTION_MAX; ++i)
    {
        stReq.inFactor[i].add = pstReq->in_factor[i].add;
        stReq.inFactor[i].mul = pstReq->in_factor[i].mul;

        stReq.outFactor[i].add = pstReq->out_factor[i].add;
        stReq.outFactor[i].mul = pstReq->out_factor[i].mul;
    }
    return VPP_OK;
}


uint32_t HidlVpp::HidlVppUtils::vppMemBufToHidl(const struct vpp_mem_buffer memBuf,
                                                VppMemBuffer& stVppMemBuf)
{
    native_handle_t *nativeHandle = nullptr;
    if (memBuf.fd >= 0)
         close(memBuf.fd);

    stVppMemBuf.handleFd.setTo(nativeHandle);
    stVppMemBuf.allocLen = memBuf.alloc_len;

    stVppMemBuf.filledLen = memBuf.filled_len;
    stVppMemBuf.offset = memBuf.offset;
    stVppMemBuf.validDataLen = memBuf.valid_data_len;

    return VPP_OK;
}

uint32_t HidlVpp::HidlVppUtils::vppBufferToHidl(const struct vpp_buffer *buf, VppBuffer& stVppBuf)
{
    uint32_t u32Ret;

    if (!buf)
        return VPP_ERR_PARAM;

    stVppBuf.flags = buf->flags;
    stVppBuf.timestamp = buf->timestamp;
    stVppBuf.bufferId = (uint64_t)buf->cookie;
    stVppBuf.cookieInToOut = (uint64_t)buf->cookie_in_to_out;

    if (buf->pvGralloc)
    {
        // This will force the service to mmap the gralloc Fds

        // Workaround for bug where copyMetaData maps the buffer
        // into the address space but doesn't unmap it. This
        // prevents a memory leak.
        private_handle_t *p = (private_handle_t *)buf->pvGralloc;
        if (p && p->base_metadata)
        {
            munmap(reinterpret_cast<void *>(p->base_metadata),
                   getMetaDataSize());
            p->base_metadata = (uintptr_t)nullptr;
        }
        releaseNativeHandle((const native_handle_t *)buf->pvGralloc);
        stVppBuf.pvGralloc = nullptr;
    }
    else
        stVppBuf.pvGralloc = nullptr;

    u32Ret = vppMemBufToHidl(buf->pixel, stVppBuf.pixel);
    if (u32Ret != VPP_OK)
        return VPP_ERR_PARAM;

    u32Ret = vppMemBufToHidl(buf->extradata, stVppBuf.extradata);
    if (u32Ret != VPP_OK)
        return VPP_ERR_PARAM;

    return VPP_OK;
}

uint32_t HidlVpp::HidlVppUtils::vppEventToHidl(const struct vpp_event e, VppEvent& stEvt)
{
    using vendor::qti::hardware::vpp::V1_1::VppEventType;

    uint32_t u32Ret = VPP_OK;

    stEvt.type = (VppEventType)e.type;

    switch (e.type)
    {
        case VPP_EVENT_FLUSH_DONE:
            stEvt.u.flushDone.port = (VppPort)e.flush_done.port;
            break;
        case VPP_EVENT_RECONFIG_DONE:
            stEvt.u.reconfigDone.reconfigStatus = e.port_reconfig_done.reconfig_status;
            vppReqToHidl(&e.port_reconfig_done.req, stEvt.u.reconfigDone.req);
            break;
        case VPP_EVENT_ERROR:
            break;
        default:
            LOGE("Unsupported Event type %u", e.type);
            u32Ret = VPP_ERR_PARAM;
            break;
    }
    return u32Ret;
}

/*********** From hidl to vpp *******************************************/
void HidlVpp::HidlVppUtils::hidlToVppCtrl(struct hqv_ctrl_cade& v, const V1_1::HqvCtrlCade h)
{
    v.mode              = (hqv_mode)h.mode;
    v.cade_level        = h.cadeLevel;
    v.contrast          = h.contrast;
    v.saturation        = h.saturation;
}

void HidlVpp::HidlVppUtils::hidlToVppCtrl(struct hqv_ctrl_aie& v, const V1_1::HqvCtrlAie h)
{
    v.mode              = (hqv_mode)h.mode;
    v.hue_mode          = (hqv_hue_mode)h.hueMode;
    v.cade_level        = h.cadeLevel;
    v.ltm_level         = h.ltmLevel;
    v.ltm_sat_gain      = h.ltmSatGain;
    v.ltm_sat_offset    = h.ltmSatOffset;
    v.ltm_ace_str       = h.ltmAceStr;
    v.ltm_ace_bright_l  = h.ltmAceBriL;
    v.ltm_ace_bright_h  = h.ltmAceBriH;
}

void HidlVpp::HidlVppUtils::hidlToVppCtrl(struct hqv_ctrl_di& v, const V1_1::HqvCtrlDi h)
{
    v.mode              = (hqv_di_mode)h.mode;
}

void HidlVpp::HidlVppUtils::hidlToVppCtrl(struct hqv_ctrl_tnr& v, const V1_1::HqvCtrlTnr h)
{
    v.mode              = (hqv_mode)h.mode;
    v.level             = h.level;
}

void HidlVpp::HidlVppUtils::hidlToVppCtrl(struct hqv_ctrl_cnr& v, const V1_1::HqvCtrlCnr h)
{
    v.mode              = (hqv_mode)h.mode;
    v.level             = h.level;
}

void HidlVpp::HidlVppUtils::hidlToVppCtrl(struct hqv_ctrl_global_demo& v, const V1_1::HqvCtrlGlobalDemo h)
{
    v.process_percent   = h.processPercent;
    v.process_direction = (hqv_split_direction)h.processDirection;
}

void HidlVpp::HidlVppUtils::hidlToVppCtrl(struct hqv_ctrl_ear& v, const V1_1::HqvCtrlEar h)
{
    v.mode              = (hqv_ear_mode)h.mode;
}

void HidlVpp::HidlVppUtils::hidlToVppCtrl(struct hqv_ctrl_qbr& v, const V1_1::HqvCtrlQbr h)
{
    v.mode              = (hqv_qbr_mode)h.mode;
}

void HidlVpp::HidlVppUtils::hidlToVppCtrl(struct hqv_ctrl_meas& v, const V1_1::HqvCtrlMeas h)
{
    v.enable            = h.enable;
}

void HidlVpp::HidlVppUtils::hidlToVppCtrl(struct hqv_ctrl_frc& v, const V1_1::HqvCtrlFrc h)
{
    if (!v.segments || !v.num_segments)
        return;

    v.segments[0].ts_start                  = 0;
    v.segments[0].frame_copy_input          = VPP_FALSE;
    v.segments[0].frame_copy_on_fallback    = VPP_FALSE;

    v.segments[0].mode      = (hqv_frc_mode)h.mode;
    v.segments[0].level     = (hqv_frc_level)h.level;
    v.segments[0].interp    = (hqv_frc_interp)h.interp;
}

void HidlVpp::HidlVppUtils::hidlToVppCtrl(struct hqv_ctrl_frc& v, const V1_2::VppCtrlFrc h)
{
    uint32_t i;

    if (!v.segments)
    {
        // no memory to write this into
        return;
    }

    for (i = 0; i < h.segments.size() && i < v.num_segments; i++)
    {
        v.segments[i].ts_start = h.segments[i].ts_start;
        v.segments[i].frame_copy_input = h.segments[i].frame_copy_input;
        v.segments[i].frame_copy_on_fallback = h.segments[i].frame_copy_on_fallback;

        v.segments[i].mode = (hqv_frc_mode)h.segments[i].mode;
        v.segments[i].level = (hqv_frc_level)h.segments[i].level;
        v.segments[i].interp = (hqv_frc_interp)h.segments[i].interp;
    }

    v.num_segments = i;
}

uint32_t HidlVpp::HidlVppUtils::hidlToHqvCtrl(struct hqv_control &v, const HqvControl h)
{
    uint32_t u32Ret = VPP_OK;

    v.mode = (hqv_mode)h.mode;
    v.ctrl_type = (hqv_control_type)h.ctrlType;

    switch (v.ctrl_type)
    {
        case HQV_CONTROL_NONE:          /* do nothing */                    break;
        case HQV_CONTROL_CADE:          hidlToVppCtrl(v.cade, h.u.cade);    break;
        case HQV_CONTROL_AIE:           hidlToVppCtrl(v.aie, h.u.aie);      break;
        case HQV_CONTROL_FRC:           hidlToVppCtrl(v.frc, h.u.frc);      break;
        case HQV_CONTROL_DI:            hidlToVppCtrl(v.di, h.u.di);        break;
        case HQV_CONTROL_TNR:           hidlToVppCtrl(v.tnr, h.u.tnr);      break;
        case HQV_CONTROL_CNR:           hidlToVppCtrl(v.cnr, h.u.cnr);      break;
        case HQV_CONTROL_GLOBAL_DEMO:   hidlToVppCtrl(v.demo, h.u.demo);    break;
        case HQV_CONTROL_EAR:           hidlToVppCtrl(v.ear, h.u.ear);      break;
        case HQV_CONTROL_QBR:           hidlToVppCtrl(v.qbr, h.u.qbr);      break;
        case HQV_CONTROL_MEAS:          hidlToVppCtrl(v.meas, h.u.meas);    break;
        case HQV_CONTROL_CUST:          // fallthrough
        case HQV_CONTROL_MAX:           // fallthrough
        default:
            LOGE("Unsupported Control Type %u", (uint32_t)v.ctrl_type);
            u32Ret = VPP_ERR_PARAM;
            break;
    }
    return u32Ret;
}

uint32_t HidlVpp::HidlVppUtils::hidlToHqvCtrl(struct hqv_control &v, const V1_2::VppControl_1_2 h)
{
    uint32_t u32Ret = VPP_OK;

    v.mode = (hqv_mode)h.mode;
    v.ctrl_type = (hqv_control_type)h.ctrlType;

    switch (v.ctrl_type)
    {
        case HQV_CONTROL_NONE:          /* do nothing */                    break;
        case HQV_CONTROL_CADE:          hidlToVppCtrl(v.cade, h.u.cade);    break;
        case HQV_CONTROL_AIE:           hidlToVppCtrl(v.aie, h.u.aie);      break;
        case HQV_CONTROL_FRC:           hidlToVppCtrl(v.frc, h.frc);        break;
        case HQV_CONTROL_DI:            hidlToVppCtrl(v.di, h.u.di);        break;
        case HQV_CONTROL_TNR:           hidlToVppCtrl(v.tnr, h.u.tnr);      break;
        case HQV_CONTROL_CNR:           hidlToVppCtrl(v.cnr, h.u.cnr);      break;
        case HQV_CONTROL_GLOBAL_DEMO:   hidlToVppCtrl(v.demo, h.u.demo);    break;
        case HQV_CONTROL_EAR:           hidlToVppCtrl(v.ear, h.u.ear);      break;
        case HQV_CONTROL_QBR:           hidlToVppCtrl(v.qbr, h.u.qbr);      break;
        case HQV_CONTROL_MEAS:          hidlToVppCtrl(v.meas, h.u.meas);    break;
        case HQV_CONTROL_CUST:          // fallthrough
        case HQV_CONTROL_MAX:           // fallthrough
        default:
            LOGE("Unsupported Control Type %u", (uint32_t)v.ctrl_type);
            u32Ret = VPP_ERR_PARAM;
            break;
    }

    return u32Ret;
}

uint32_t HidlVpp::HidlVppUtils::hidlToVppReq(struct vpp_requirements *pstReq,
                                             const VppRequirements& stReq)
{
    if (!pstReq)
        return VPP_ERR_PARAM;

    pstReq->in_color_fmt_mask = stReq.inColorFmtMask;

    pstReq->metadata.cnt = stReq.metadata.cnt;

    for (size_t i = 0; i < META_MAX_CNT; ++i)
    {
        pstReq->metadata.meta[i] = stReq.metadata.meta[i];
    }

    for (size_t i = 0; i < VPP_RESOLUTION_MAX; ++i)
    {
        pstReq->in_factor[i].add = stReq.inFactor[i].add;
        pstReq->in_factor[i].mul = stReq.inFactor[i].mul;

        pstReq->out_factor[i].add = stReq.outFactor[i].add;
        pstReq->out_factor[i].mul = stReq.outFactor[i].mul;
    }
    return VPP_OK;
}

void HidlVpp::HidlVppUtils::hidlToVppPortParam(struct vpp_port_param& param,
                                               const VppPortParam& stParam)
{
    using vendor::qti::hardware::vpp::V1_1::VppColorFormat;

    switch (stParam.fmt)
    {
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_NV12_VENUS,
                      VPP_COLOR_FORMAT_NV12_VENUS);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_NV21_VENUS,
                      VPP_COLOR_FORMAT_NV21_VENUS);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_P010,
                      VPP_COLOR_FORMAT_P010);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_UBWC_NV12,
                      VPP_COLOR_FORMAT_UBWC_NV12);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_UBWC_NV21,
                      VPP_COLOR_FORMAT_UBWC_NV21);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_UBWC_TP10,
                      VPP_COLOR_FORMAT_UBWC_TP10);
        default:
            LOGE("Setting to unknown color format=%d", stParam.fmt);
            param.fmt = (vpp_color_format)stParam.fmt;
            break;
    }
    param.height = stParam.height;
    param.width = stParam.width;
    param.stride = stParam.stride;
    param.scanlines = stParam.scanlines;
}

void HidlVpp::HidlVppUtils::hidl_1_3ToVppPortParam(struct vpp_port_param& param,
                                                   const V1_3::VppPortParam_1_3& stParam)
{
    using vendor::qti::hardware::vpp::V1_3::VppColorFormat;

    switch (stParam.fmt)
    {
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_NV12_VENUS,
                      VPP_COLOR_FORMAT_NV12_VENUS);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_NV21_VENUS,
                      VPP_COLOR_FORMAT_NV21_VENUS);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_P010,
                      VPP_COLOR_FORMAT_P010);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_UBWC_NV12,
                      VPP_COLOR_FORMAT_UBWC_NV12);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_UBWC_NV21,
                      VPP_COLOR_FORMAT_UBWC_NV21);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_UBWC_TP10,
                      VPP_COLOR_FORMAT_UBWC_TP10);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_RGBA8,
                      VPP_COLOR_FORMAT_RGBA8);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_BGRA8,
                      VPP_COLOR_FORMAT_BGRA8);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_UBWC_RGBA8,
                      VPP_COLOR_FORMAT_UBWC_RGBA8);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_UBWC_BGRA8,
                      VPP_COLOR_FORMAT_UBWC_BGRA8);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_UBWC_RGB565,
                      VPP_COLOR_FORMAT_UBWC_RGB565);
        VPP_PARAM_MAP(param.fmt, VppColorFormat::VPP_COLOR_FORMAT_UBWC_BGR565,
                      VPP_COLOR_FORMAT_UBWC_BGR565);
        default:
            LOGE("Setting to unknown color format=%d", stParam.fmt);
            param.fmt = (vpp_color_format)stParam.fmt;
            break;
    }
    param.height = stParam.height;
    param.width = stParam.width;
    param.stride = stParam.stride;
    param.scanlines = stParam.scanlines;
}

uint32_t HidlVpp::HidlVppUtils::hidlToVppProp(struct video_property& prop, const VideoProperty& stProp)
{
    uint32_t u32Ret = VPP_OK;

    prop.property_type = (vid_prop_type)stProp.propertyType;

    switch (prop.property_type)
    {
        case VID_PROP_CODEC:
            prop.codec.eCodec = (vpp_codec_type)stProp.u.codec.eCodec;
            break;
        case VID_PROP_NON_REALTIME:
            prop.non_realtime.bNonRealtime = stProp.u.nonRealtime.bNonRealtime;
            break;
        case VID_PROP_OPERATING_RATE:
            prop.operating_rate.u32OperatingRate = stProp.u.operatingRate.u32OperatingRate;
            break;
        case VID_PROP_MAX:
        default:
            LOGE("Unsupported propterty_type %u", prop.property_type);
            u32Ret = VPP_ERR_PARAM;
            break;
    };
    return u32Ret;
}

void HidlVpp::HidlVppUtils::hidlToVppMemBuf(struct vpp_mem_buffer& memBuf,
                                            const VppMemBuffer& stVppMemBuf)
{
    auto nh = stVppMemBuf.handleFd.getNativeHandle();
    if (nh == nullptr)
        memBuf.fd = -1;
    else
        memBuf.fd = dup(nh->data[0]);

    memBuf.alloc_len = stVppMemBuf.allocLen;
    memBuf.filled_len = stVppMemBuf.filledLen;
    memBuf.offset = stVppMemBuf.offset;
    memBuf.pvMapped = nullptr;
    memBuf.valid_data_len = stVppMemBuf.validDataLen;
}

uint32_t HidlVpp::HidlVppUtils::hidlToVppBuffer(struct vpp_buffer *buf, const VppBuffer& stVppBuf)
{
    if (!buf)
        return VPP_ERR_PARAM;

    buf->flags = stVppBuf.flags;
    buf->timestamp = stVppBuf.timestamp;
    buf->cookie = reinterpret_cast<void *>(stVppBuf.bufferId);
    buf->cookie_in_to_out = (void *)stVppBuf.cookieInToOut;
    if (stVppBuf.pvGralloc)
    {
        buf->pvGralloc = (void *)native_handle_clone(stVppBuf.pvGralloc.getNativeHandle());
        // Set base/base_metadata to 0.
        // This will force the service to mmap the gralloc Fds
        if (buf->pvGralloc)
        {
            CLEAR_PRIV_HDL_BASE_PARAMS(buf->pvGralloc);
        }
    }
    else
        buf->pvGralloc = nullptr;

    hidlToVppMemBuf(buf->pixel, stVppBuf.pixel);
    hidlToVppMemBuf(buf->extradata, stVppBuf.extradata);

    return VPP_OK;
}

void HidlVpp::HidlVppUtils::hidl_1_1ToHidl_1_3VppPortParam(const VppPortParam& stParam_1_1,
                                                           V1_3::VppPortParam_1_3& stParam_1_3)
{
    stParam_1_3.height = stParam_1_1.height;
    stParam_1_3.width = stParam_1_1.width;
    stParam_1_3.stride = stParam_1_1.stride;
    stParam_1_3.scanlines = stParam_1_1.scanlines;
    stParam_1_3.fmt = (V1_3::VppColorFormat)stParam_1_1.fmt;
}

void HidlVpp::HidlVppUtils::hidl_1_3ToHidl_1_1VppPortParam(VppPortParam& stParam_1_1,
                                                           const V1_3::VppPortParam_1_3& stParam_1_3)
{
    stParam_1_1.height = stParam_1_3.height;
    stParam_1_1.width = stParam_1_3.width;
    stParam_1_1.stride = stParam_1_3.stride;
    stParam_1_1.scanlines = stParam_1_3.scanlines;
    stParam_1_1.fmt = (V1_1::VppColorFormat)stParam_1_3.fmt;
    if (stParam_1_1.fmt >= V1_1::VppColorFormat::VPP_COLOR_FORMAT_MAX)
    {
        LOGE("Color format=%d >= v1.1 max=%d", stParam_1_1.fmt,
             V1_1::VppColorFormat::VPP_COLOR_FORMAT_MAX);
        stParam_1_1.fmt = V1_1::VppColorFormat::VPP_COLOR_FORMAT_MAX;
    }
}

void HidlVpp::HidlVppUtils::releaseNativeHandle(const native_handle_t *nh)
{
    if (nh != nullptr)
    {
        uint32_t u32 = native_handle_close(nh);
        if (u32 != 0)
            LOGE("ERROR : native_handle_close returned err = %u",u32);
        else
        {
            u32 = native_handle_delete(const_cast<native_handle_t*>(nh));
            if (u32 != 0)
                LOGE("ERROR : native_handle_delete returned err = %u",u32);
        }
    }
}

void HidlVpp::HidlVppUtils::readProperties(uint32_t *pu32StatsEn)
{
    char property_value[256] = {0};

    property_get(VPP_PROPERTY_LOG_CORE, property_value, "0");
    u64LogLevelIpc = strtoull(property_value, NULL, 0);

    property_get(VPP_PROPERTY_STATS_LEVEL, property_value, "0");
    *pu32StatsEn = strtoul(property_value, NULL, 0);

    // Log after setting global (in case defined log level disables this log)
    LOGD("%s=0x%llx", VPP_PROPERTY_LOG_CORE, (long long unsigned int)u64LogLevelIpc);
}

}
