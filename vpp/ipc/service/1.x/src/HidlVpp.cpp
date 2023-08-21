/*!
 * @file HidlVpp.cpp
 *
 * @cr
 * Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @services
 */

#include <sys/types.h>
#include <inttypes.h>

#define VPP_LOG_TAG     VPP_LOG_IPC_SERVICE_TAG
#define VPP_LOG_MODULE  VPP_LOG_IPC_SERVICE
#include "vpp_dbg.h"
#include "HidlVpp.h"

namespace qti_vpp {

using namespace ::vendor::qti::hardware::vpp;

enum {
    // The time taken for the vpp init
    PL_VPP_INIT,
    // The time taken for the vpp term
    PL_VPP_TERM,
    // The time taken for the vpp set_ctrl
    PL_VPP_SET_CTRL,
    // The time taken for the vpp set_param
    PL_VPP_SET_PARAM,
    // The time taken for the vpp get_buf_requirements
    PL_VPP_GET_BUF_REQUIREMENTS,
    // The time taken for the vpp open
    PL_VPP_OPEN,
    // The time taken for the vpp close
    PL_VPP_CLOSE,
    // The time taken for the vpp input quebuf
    PL_VPP_QUEUE_BUF_IN,
    // The time taken for the vpp output quebuf
    PL_VPP_QUEUE_BUF_OUT,
    // The time taken for the vpp quebuf
    PL_VPP_RECONFIG,
    // The time taken for the vpp flush
    PL_VPP_FLUSH,
    // The time taken for the vpp set_vid_prop
    PL_VPP_SET_VID_PROP,
    PL_VPP_STATS_MAX,
};

// Configure characteristics for each individual stat
static const t_StVppStatsConfig astVppLibStatsCfg[] = {
    VPP_PROF_DECL(PL_VPP_INIT, 1, 1),
    VPP_PROF_DECL(PL_VPP_TERM, 1, 1),
    VPP_PROF_DECL(PL_VPP_SET_CTRL, 1, 1),
    VPP_PROF_DECL(PL_VPP_SET_PARAM, 1, 1),
    VPP_PROF_DECL(PL_VPP_GET_BUF_REQUIREMENTS, 1, 1),
    VPP_PROF_DECL(PL_VPP_OPEN, 1, 1),
    VPP_PROF_DECL(PL_VPP_CLOSE, 1, 1),
    VPP_PROF_DECL(PL_VPP_QUEUE_BUF_IN, 1, 1),
    VPP_PROF_DECL(PL_VPP_QUEUE_BUF_OUT, 1, 1),
    VPP_PROF_DECL(PL_VPP_RECONFIG, 1, 1),
    VPP_PROF_DECL(PL_VPP_FLUSH, 1, 1),
    VPP_PROF_DECL(PL_VPP_SET_VID_PROP, 1, 1)
};

static const uint32_t u32VppLibStatsCnt = VPP_STATS_CNT(astVppLibStatsCfg);

void HidlVpp::HidlVppDeathRecipient::serviceDied(uint64_t cookie, const wp<IBase>& who)
{
    VPP_UNUSED(who);

    LOGE("callback client died, for c=%" PRIu64, cookie);

    if (mHidlVpp)
    {
        Mutex::Autolock l(mHidlVpp->mDeathMutex);
        mHidlVpp->mClientDied = true;
    }
}

HidlVpp::HidlVpp() : pvCtx(NULL),
    mClientDied(false),
    mClientDeathNotification(new HidlVppDeathRecipient(this))
{
    uint32_t i, u32StatsEn, u32Ret = VPP_OK;

    LOGI("HidlVpp constructor called, this=%p", this);

    memset(mVppBuffers, 0, sizeof(struct vpp_buffer) * VPP_BUF_POOL_MAXSIZE);

    for (i = 0; i < VPP_BUF_POOL_MAXSIZE; i++)
        mBufPool.push_front(&mVppBuffers[i]);

    // Read properties, debug-controls
    HidlVppUtils::readProperties(&u32StatsEn);

    mStatHandle = NULL;
    memset(&mStCtx, 0, sizeof(mStCtx));
    u32Ret = u32VppStats_Init(&mStCtx);
    if (u32Ret == VPP_OK)
    {
        u32Ret = u32VppStats_SetEnable(&mStCtx, u32StatsEn);
        LOGE_IF(u32Ret != VPP_OK, "u32VppStats_SetEnable returned error %u", u32Ret);

        // Register the calls or stats
        u32Ret = u32VppStats_Register(&mStCtx, astVppLibStatsCfg, u32VppLibStatsCnt, &mStatHandle);
        LOGE_IF(u32Ret != VPP_OK, "u32VppStats_Register returned error %u", u32Ret);
    }
    else
        LOGE("u32VppStats_Init returned error %u", u32Ret);
}

HidlVpp::~HidlVpp()
{
    uint32_t u32Ret = VPP_OK;

    {
        Mutex::Autolock l(mDeathMutex);
        mClientDied = true;
    }

    LOGI("HidlVpp destructor called for session w/ this=%p", this);
    if (pvCtx != NULL)
    {
        vpp_term(pvCtx);
        pvCtx = NULL;
    }

    if (mStatHandle)
    {
         // Unregister the stats from the module
         u32Ret = u32VppStats_Unregister(&mStCtx, mStatHandle);
         LOGE_IF(u32Ret != VPP_OK, "u32VppStats_Unregister failed: %u", u32Ret);

         // Terminate the vpp stats module
         u32Ret = u32VppStats_Term(&mStCtx);
         LOGE_IF(u32Ret != VPP_OK, "u32VppStats_Term failed: %u", u32Ret);
    }
}

// Methods from ::vendor::qti::hardware::vpp::V1_1::IHidlVpp follow.
Return<uint32_t> HidlVpp::vppInit(uint32_t flags, const sp<IHidlVppCallbacks>& cb)
{
    struct vpp_callbacks stCallbacks;

    stCallbacks.pv = static_cast<void *>(this);
    stCallbacks.input_buffer_done = vppIBD;
    stCallbacks.output_buffer_done = vppOBD;
    stCallbacks.vpp_event = vppEVT;

    mVppFlags = 0;

    mHidlCb = cb;
    mHidlCb->linkToDeath(mClientDeathNotification, 0);

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_INIT);
    pvCtx = vpp_init(flags, stCallbacks);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_INIT);

    if (!pvCtx)
    {
        LOGE("ERROR: vpp_init returned null");
        return VPP_ERR;
    }
    LOGI("this=%p, vpp_init returned ctx=%p", this, pvCtx);

    return VPP_OK;
}

Return<void> HidlVpp::vppTerm()
{
    if (pvCtx != NULL)
    {
        u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_TERM);
        vpp_term(pvCtx);
        u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_TERM);

        pvCtx = NULL;
    }
    if (mHidlCb != NULL) {
        mHidlCb->unlinkToDeath(mClientDeathNotification);
        mHidlCb = NULL;
    }
    return Void();
}

Return<void> HidlVpp::vppSetCtrl(uint64_t cookie, const HqvControl& ctrl, vppSetCtrl_cb _hidl_cb)
{
    struct hqv_control stHqvCtrl;
    struct vpp_requirements stVppReq;
    uint32_t u32 = VPP_OK;
    VppRequirements retReq;
    struct vpp_ctrl_frc_segment stFrcSegs;

    if (ctrl.mode == V1_1::HqvMode::HQV_MODE_MANUAL &&
        ctrl.ctrlType == V1_1::HqvControlType::HQV_CONTROL_FRC)
    {
        // HqvControl can contain only one set of FRC controls,
        // treat this as 1 segment received from the client.
        stHqvCtrl.frc.segments = &stFrcSegs;
        stHqvCtrl.frc.num_segments = 1;
    }

    u32 = HidlVppUtils::hidlToHqvCtrl(stHqvCtrl, ctrl);
    if (u32 != VPP_OK)
        LOGE("ERROR: from hidlToHqvCtrl Err=%u", u32);
    else
    {
        u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_SET_CTRL);
        u32 = vpp_set_ctrl(pvCtx, stHqvCtrl);
        u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_SET_CTRL);
        LOGE_IF(u32 != VPP_OK, "vpp_set_ctrl returned error, u32=%u", u32);

        // This is to allow backward compatibility with the 1.1 and 1.2 Hidl versions.
        u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_GET_BUF_REQUIREMENTS);
        u32 |= vpp_get_buf_requirements(pvCtx, &stVppReq);
        u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_GET_BUF_REQUIREMENTS);
        LOGE_IF(u32 != VPP_OK, "error after vpp_get_buf_requirements, u32=%u", u32);

        if (u32 == VPP_OK)
        {
            u32 = HidlVppUtils::vppReqToHidl(&stVppReq, retReq);
            LOGE_IF(u32 != VPP_OK, "ERROR: from vppReqToHidl Err=%u", u32);
        }
    }

    retReq.retStatus = u32;
    retReq.cookie = cookie;
    // For Hidl, multiple params need to returned using the callback
    _hidl_cb(retReq);

    return Void();
}

#define VPP_INPUT_PORT_PARAM_SET         (1 << 0)
#define VPP_OUTPUT_PORT_PARAM_SET        (1 << 1)
Return<uint32_t> HidlVpp::vppSetParameter(VppPort port, const VppPortParam& param)
{
    uint32_t u32 = VPP_OK;
    vpp_port ePort = static_cast<vpp_port>(port);
    V1_3::VppPortParam_1_3 param_1_3;

    if (ePort >= VPP_PORT_MAX)
        return VPP_ERR_PARAM;

    if (param.fmt >= V1_1::VppColorFormat::VPP_COLOR_FORMAT_MAX)
    {
        LOGE("Color format=%d >= max=%d", param.fmt, V1_1::VppColorFormat::VPP_COLOR_FORMAT_MAX);
        return VPP_ERR_PARAM;
    }

    HidlVppUtils::hidl_1_1ToHidl_1_3VppPortParam(param, param_1_3);
    u32 = vppSetParameter_1_3(port, param_1_3);

    // This is to allow backward compatibility with the 1.1 and 1.2 Hidl versions.
    // VPP_ERR_INVALID_CFG will be returned if the controls are invalid
    if ((u32 == VPP_OK) || (u32 == VPP_ERR_INVALID_CFG))
    {
        mVppFlags |= (ePort == VPP_PORT_INPUT) ? VPP_INPUT_PORT_PARAM_SET :
                     ((ePort == VPP_PORT_OUTPUT) ? VPP_OUTPUT_PORT_PARAM_SET : 0);

        if (mVppFlags & VPP_INPUT_PORT_PARAM_SET &&
            mVppFlags & VPP_OUTPUT_PORT_PARAM_SET)
        {
            u32 = vpp_open(pvCtx);
            LOGE_IF(u32 != VPP_OK, "vpp_open() failed u32=%u", u32);
        }
    }

    return u32;
}

Return<uint32_t> HidlVpp::vppQueueBuf(VppPort port, const VppBuffer& buf)
{
    struct vpp_buffer *pstExtBuf = NULL;
    uint32_t u32 = VPP_OK, u32Key;

    Mutex::Autolock a(mMutex);
    if (mBufPool.size() > 0)
    {
        pstExtBuf = mBufPool.back();
        mBufPool.pop_back();
    }

    VPP_RET_IF_NULL(pstExtBuf, VPP_ERR);

    u32 = HidlVppUtils::hidlToVppBuffer(pstExtBuf, buf);
    if (u32 != VPP_OK)
    {
        mBufPool.push_front(pstExtBuf);
        LOGE("hidlToVppBuffer returned error %u", u32);
        return u32;
    }

    u32Key = (port == VppPort::VPP_PORT_INPUT) ? PL_VPP_QUEUE_BUF_IN : PL_VPP_QUEUE_BUF_OUT;

    u32VppStats_Start(&mStCtx, mStatHandle, u32Key);
    u32 = vpp_queue_buf(pvCtx, (enum vpp_port)port, pstExtBuf);
    u32VppStats_Stop(&mStCtx, mStatHandle, u32Key);

    LOGE_IF(u32 != VPP_OK, "vpp_queue_buf(%u) returned error, u32=%u", port, u32);

    return u32;
}

Return<uint32_t> HidlVpp::vppReconfigure(const VppPortParam& inputParam, const VppPortParam& outputParam)
{
    V1_3::VppPortParam_1_3 inputParam_1_3, outputParam_1_3;
    uint32_t u32 = VPP_OK;

    if (inputParam.fmt >= V1_1::VppColorFormat::VPP_COLOR_FORMAT_MAX ||
        outputParam.fmt >= V1_1::VppColorFormat::VPP_COLOR_FORMAT_MAX)
    {
        LOGE("Input format=%d and/or output format=%d >= max=%d",
             inputParam.fmt, outputParam.fmt, V1_1::VppColorFormat::VPP_COLOR_FORMAT_MAX);
        return VPP_ERR_PARAM;
    }

    HidlVppUtils::hidl_1_1ToHidl_1_3VppPortParam(inputParam, inputParam_1_3);
    HidlVppUtils::hidl_1_1ToHidl_1_3VppPortParam(outputParam, outputParam_1_3);
    u32 = vppReconfigure_1_3(inputParam_1_3, outputParam_1_3);

    LOGE_IF(u32 != VPP_OK, "vpp_reconfigure returned error, u32=%u", u32);

    return u32;
}

Return<uint32_t> HidlVpp::vppFlush(VppPort port)
{
    uint32_t u32 = VPP_OK;

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_FLUSH);
    u32 = vpp_flush(pvCtx, (enum vpp_port)port);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_FLUSH);

    LOGE_IF(u32 != VPP_OK, "vpp_flush returned error, u32=%u", u32);

    return u32;
}

Return<uint32_t> HidlVpp::vppSetVidProp(const VideoProperty& prop)
{
    struct video_property stProp;
    uint32_t u32 = VPP_OK;

    u32 = HidlVppUtils::hidlToVppProp(stProp, prop);
    if (u32 != VPP_OK)
    {
        LOGE("hidlToVppProp returned error %u", u32);
        return u32;
    }

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_SET_VID_PROP);
    u32 = vpp_set_vid_prop(pvCtx, stProp);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_SET_VID_PROP);

    LOGE_IF(u32 != VPP_OK, "vpp_set_vid_prop returned error, u32=%u", u32);

    return u32;
}

Return<void> HidlVpp::vppSetCtrl_1_2(const V1_2::VppControl_1_2 &ctrl, vppSetCtrl_1_2_cb _hidl_cb)
{
    uint32_t u32;
    struct vpp_requirements stReq;
    VppRequirements ret;
    ret.cookie = 0;

    u32 = vppSetCtrl_1_3(ctrl);
    LOGE_IF(u32 != VPP_OK, "vppSetCtrl_1_3 returned error, u32=%u", u32);

    // This is to allow backward compatibility with the 1.1 and 1.2 Hidl versions.
    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_GET_BUF_REQUIREMENTS);
    u32 |= vpp_get_buf_requirements(pvCtx, &stReq);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_GET_BUF_REQUIREMENTS);
    LOGE_IF(u32 != VPP_OK, "error after vpp_get_buf_requirements, u32=%u", u32);

    if (u32 == VPP_OK)
    {
        u32 = HidlVppUtils::vppReqToHidl(&stReq, ret);
        LOGE_IF(u32 != VPP_OK, "ERROR: from vppReqToHidl Err=%u", u32);
    }

    ret.retStatus = u32;
    _hidl_cb(ret);
    return Void();
}

Return<uint32_t> HidlVpp::vppSetCtrl_1_3(const V1_2::VppControl_1_2 &ctrl)
{
    uint32_t u32;
    struct hqv_control stCtrl;
    struct vpp_ctrl_frc_segment *pstFrcSegs = NULL;

    stCtrl.frc.segments = NULL;
    stCtrl.frc.num_segments = 0;

    if (ctrl.mode == V1_1::HqvMode::HQV_MODE_MANUAL &&
        ctrl.ctrlType == V1_1::HqvControlType::HQV_CONTROL_FRC)
    {
        uint32_t u32Segs = ctrl.frc.segments.size();
        if (u32Segs)
        {
            LOGD("allocating %u frc segments", u32Segs);
            pstFrcSegs = (struct vpp_ctrl_frc_segment *)
                calloc(u32Segs, sizeof(struct vpp_ctrl_frc_segment));

            if (!pstFrcSegs)
            {
                LOGE("failed to allocate %u segments", u32Segs);
                return VPP_ERR_NO_MEM;
            }

            stCtrl.frc.segments = pstFrcSegs;
            stCtrl.frc.num_segments = u32Segs;
        }
    }

    HidlVppUtils::hidlToHqvCtrl(stCtrl, ctrl);

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_SET_CTRL);
    u32 = vpp_set_ctrl(pvCtx, stCtrl);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_SET_CTRL);
    LOGE_IF(u32 != VPP_OK, "failed to set_ctrl, u32=%u", u32);

    if (pstFrcSegs)
    {
        free(pstFrcSegs);
        pstFrcSegs = NULL;
    }

    return u32;
}

Return<uint32_t> HidlVpp::vppSetParameter_1_3(VppPort port, const V1_3::VppPortParam_1_3& param)
{
    struct vpp_port_param stParam;
    uint32_t u32 = VPP_OK;

    HidlVppUtils::hidl_1_3ToVppPortParam(stParam, param);

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_SET_PARAM);
    u32 = vpp_set_parameter(pvCtx, (vpp_port)port, stParam);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_SET_PARAM);
    LOGE_IF(u32 != VPP_OK, "vpp_set_parameter returned error, u32=%u", u32);

    return u32;
}

Return<uint32_t> HidlVpp::vppReconfigure_1_3(const V1_3::VppPortParam_1_3& inputParam,
                                             const V1_3::VppPortParam_1_3& outputParam)
{
    vpp_port_param stInParam, stOutParam;
    uint32_t u32 = VPP_OK;

    HidlVppUtils::hidl_1_3ToVppPortParam(stInParam, inputParam);
    HidlVppUtils::hidl_1_3ToVppPortParam(stOutParam, outputParam);

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_RECONFIG);
    u32 = vpp_reconfigure(pvCtx, stInParam, stOutParam);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_RECONFIG);

    LOGE_IF(u32 != VPP_OK, "vpp_reconfigure returned error, u32=%u", u32);

    return u32;
}

Return<void> HidlVpp::vppGetBufRequirements(vppGetBufRequirements_cb _hidl_cb)
{
    uint32_t u32;
    struct vpp_requirements stReq;

    VppRequirements ret;
    ret.cookie = 0;

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_GET_BUF_REQUIREMENTS);
    u32 = vpp_get_buf_requirements(pvCtx, &stReq);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_GET_BUF_REQUIREMENTS);
    if (u32 == VPP_OK)
        HidlVppUtils::vppReqToHidl(&stReq, ret);
    else
        LOGE("vpp_get_buf_requirements() failed =%u", u32);

    ret.retStatus = u32;
    _hidl_cb(ret);
    return Void();
}

Return<uint32_t> HidlVpp::vppOpen()
{
    uint32_t u32;

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_OPEN);
    u32 = vpp_open(pvCtx);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_OPEN);

    LOGE_IF(u32 != VPP_OK, "vpp_open() failed = %u", u32);

    return u32;
}

Return<uint32_t> HidlVpp::vppClose()
{
    uint32_t u32;

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPP_CLOSE);
    u32 = vpp_close(pvCtx);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPP_CLOSE);

    LOGE_IF(u32 != VPP_OK, "vpp_close() failed = %u", u32);

    return u32;
}

uint32_t HidlVpp::vppBufferDone(void *pv, struct vpp_buffer *buf, enum vpp_port ePort)
{
    VppBuffer stVppBuf;
    uint32_t u32 = VPP_OK;
    Return<uint32_t> r = VPP_OK;
    const char *w = ePort == VPP_PORT_INPUT ? "i" : "o";

    if (!pv || !buf || (ePort >= VPP_PORT_MAX))
    {
        LOGE("ERROR: pv=%p, buf=%p port=%u", pv, buf, (uint32_t)ePort);
        return VPP_ERR_PARAM;
    }

    HidlVpp *vpp = static_cast<HidlVpp *>(pv);

    u32 = HidlVppUtils::vppBufferToHidl(buf, stVppBuf);

    {
        Mutex::Autolock a(vpp->mMutex);
        vpp->mBufPool.push_front(buf);
    }

    if (u32 != VPP_OK)
    {
        LOGE("vppBufferToHidl failed, u32=%u", u32);
        return u32;
    }

    Mutex::Autolock l(vpp->mDeathMutex);

    if (vpp->mClientDied)
    {
        LOGE("client is dead, not issuing callback!");
        return VPP_ERR;
    }

    if (ePort == VPP_PORT_INPUT)
        r = vpp->mHidlCb->inputBufferDone(stVppBuf);
    else
        r = vpp->mHidlCb->outputBufferDone(stVppBuf);

    u32 = r.isOk() ? static_cast<uint32_t>(r) : VPP_ERR;
    LOGE_IF(!r.isOk(), "Binder callback failed for %sbd", w);
    LOGE_IF(u32 != VPP_OK, "%sbd failed, u32=%u", w, u32);

    return u32;
}

void HidlVpp::vppIBD(void *pv, struct vpp_buffer *buf)
{
    if (!pv || !buf)
    {
        LOGE("ERROR: pv=%p buf=%p", pv, buf);
        return;
    }

    LOGI("got ibd for HidlVpp instance = %p, ts=%" PRIu64, pv, buf->timestamp);

    if (vppBufferDone(pv, buf, VPP_PORT_INPUT) != VPP_OK)
        LOGE("ERROR: from callback for port VPP_PORT_INPUT");
}

void HidlVpp::vppOBD(void *pv, struct vpp_buffer *buf)
{
    if (!pv || !buf)
    {
        LOGE("ERROR: pv=%p buf=%p", pv, buf);
        return;
    }

    LOGI("got obd for HidlVpp instance = %p, ts=%" PRIu64, pv, buf->timestamp);

    if (vppBufferDone(pv, buf, VPP_PORT_OUTPUT) != VPP_OK)
        LOGE("ERROR: from callback for port VPP_PORT_OUTPUT");
}

void HidlVpp::vppEVT(void *pv, struct vpp_event e)
{
    VppEvent stEvt;
    uint32_t u32 = VPP_OK;
    Return<uint32_t> r = VPP_OK;

    if (!pv)
    {
        LOGE("ERROR: pv=%p", pv);
        return;
    }
    HidlVpp *vpp = static_cast<HidlVpp *>(pv);
    LOGI("got evt for HidlVpp instance = %p", vpp);

    u32 = HidlVppUtils::vppEventToHidl(e, stEvt);
    if (u32 != VPP_OK)
    {
        LOGE("ERROR: from vppEventToHidl Err=%u", u32);
        return;
    }

    r = vpp->mHidlCb->vppEvent(stEvt);
    u32 = r.isOk() ? static_cast<uint32_t>(r) : VPP_ERR;
    LOGE_IF(!r.isOk(), "Binder callback failed for evt");
    LOGE_IF(u32 != VPP_OK, "vppEvent failed, u32=%u", u32);
}

}  // namespace qti_vpp
