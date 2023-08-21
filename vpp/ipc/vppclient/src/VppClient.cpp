/*!
 * @file VppClient.cpp
 *
 * @cr
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @services
 */

#include <sys/types.h>
#include <inttypes.h>
#include <cutils/native_handle.h>
#include <cutils/properties.h>
#include <time.h>

#define VPP_LOG_TAG     VPP_LOG_IPC_CLIENT_TAG
#define VPP_LOG_MODULE  VPP_LOG_IPC_CLIENT
#include "vpp_dbg.h"
#include "vpp.h"
#include "vpp_def.h"
#include "vpp_ion.h"
#include "buf_pool.h"
#include "VppClient.h"
#include "HidlVppCallbacks.h"

#define VPP_PARAM_MAP(_var, _this, _that) \
    case _this: _var = _that; break

uint64_t u64LogLevelIpc;

namespace android {

// Hidl VppCallbacks Implementation
using qti_vpp::HidlVppCallbacks;

#define VPP_SVC_GET_RETRY_ATTEMPTS  1000
#define VPP_SVC_RETRY_INTERVAL_NS   (1 * 10e6)

enum
{
    // The time taken for the vpp constructor
    PL_VPPCLIENT_CONSTRUCTOR,
    // The time taken for the vpp init
    PL_VPPCLIENT_INIT,
    // The time taken for the vpp term
    PL_VPPCLIENT_TERM,
    // The time taken for the vpp set_ctrl
    PL_VPPCLIENT_SETCTRL,
    // The time taken for the vpp set_param
    PL_VPPCLIENT_SETPARAM,
    // The time taken for the vpp quebuf
    PL_VPPCLIENT_QUEUE_BUF_IN,
    // The time taken for the vpp quebuf
    PL_VPPCLIENT_QUEUE_BUF_OUT,
    // The time taken for the vpp quebuf
    PL_VPPCLIENT_RECONFIG,
    // The time taken for the vpp flush
    PL_VPPCLIENT_FLUSH,
    // The time taken for the vpp set_vid_prop
    PL_VPPCLIENT_SETVIDPROP,
    PL_VPPCLIENT_STATS_MAX,
};

// Configure characteristics for each individual stat
static const t_StVppStatsConfig astVppStatsCfg[] =
{
    VPP_PROF_DECL(PL_VPPCLIENT_CONSTRUCTOR, 1, 1),
    VPP_PROF_DECL(PL_VPPCLIENT_INIT, 1, 1),
    VPP_PROF_DECL(PL_VPPCLIENT_TERM, 1, 1),
    VPP_PROF_DECL(PL_VPPCLIENT_SETCTRL, 1, 1),
    VPP_PROF_DECL(PL_VPPCLIENT_SETPARAM, 1, 1),
    VPP_PROF_DECL(PL_VPPCLIENT_QUEUE_BUF_IN, 1, 1),
    VPP_PROF_DECL(PL_VPPCLIENT_QUEUE_BUF_OUT, 1, 1),
    VPP_PROF_DECL(PL_VPPCLIENT_RECONFIG, 1, 1),
    VPP_PROF_DECL(PL_VPPCLIENT_FLUSH, 1, 1),
    VPP_PROF_DECL(PL_VPPCLIENT_SETVIDPROP, 1, 1)
};

static const uint32_t u32VppStatsCnt = VPP_STATS_CNT(astVppStatsCfg);

void VppClient::VppClientDeathRecipient::serviceDied(uint64_t cookie,
                  const ::android::wp<::android::hidl::base::V1_0::IBase>& who)
{
    struct vpp_event e;

    LOGE("vpp service died. cookie: %llu, who: %p",
         static_cast<unsigned long long>(cookie), &who);

    /*
    * Set the vpp client bypass flag to true
    * The queuebuf can use this to directly route the INPUT buffer
    * to OUTPUT_CALLBACK.
    */
    {
        Mutex::Autolock a(mClient->mVppClientMutex);
        mClient->mVppClientBypass = true;
    }

    e.type = VPP_EVENT_ERROR;
    // Trigger an ERROR event to the OMX module.
    if (mClient->mCb.vpp_event)
        mClient->mCb.vpp_event(mClient->mCb.pv, e);

    /* clear/flush all the buffers and return it back to the client*/
    mClient->clearVppBufs();
}

VppClient::VppClient() :
    mDeathRecipient(new VppClientDeathRecipient(this)),
    mVppClientBypass(false)
{
    // Initialize the vpp stats module
    uint32_t u32Ret = VPP_OK;
    uint32_t u32StatsEn;

    // Read properties, debug-controls
    readProperties(&u32StatsEn);

    memset(&mStCtx, 0, sizeof(mStCtx));

    // Set mStatHandle to NULL, in case the init returns error, stop will also retun error
    mStatHandle = NULL;
    u32Ret = u32VppStats_Init(&mStCtx);
    if (u32Ret == VPP_OK)
    {
        u32Ret = u32VppStats_SetEnable(&mStCtx, u32StatsEn);
        LOGE_IF(u32Ret != VPP_OK, "u32VppStats_SetEnable returned error %u", u32Ret);

        // Register the calls or stats
        u32Ret = u32VppStats_Register(&mStCtx, astVppStatsCfg, u32VppStatsCnt, &mStatHandle);
        LOGE_IF(u32Ret != VPP_OK, "u32VppStats_Register returned error %u", u32Ret);

        u32Ret = u32VppStats_Start(&mStCtx, mStatHandle, PL_VPPCLIENT_CONSTRUCTOR);
        LOGE_IF(u32Ret != VPP_OK, "u32VppStats_Start returned error %u", u32Ret);
    }
    else
        LOGE("u32VppStats_Init returned error %u", u32Ret);

    // Get the Vpp Service from the Service manager
    uint32_t u32Attempts = 0;
    while (mVppService == NULL && u32Attempts < VPP_SVC_GET_RETRY_ATTEMPTS)
    {
        mVppService = IHidlVppService::tryGetService("vppService");
        if (mVppService == NULL)
        {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = VPP_SVC_RETRY_INTERVAL_NS;
            nanosleep(&ts, NULL);
            u32Attempts++;
        }
    }

    if (mVppService == NULL)
    {
        LOGE("ERROR: unable to acquire vpp service after %u attempts...", u32Attempts);
        goto ERR_VPPCLIENT;
    }

    LOGD("acquired vppservice after %u attempts", u32Attempts);

    // Get the Vpp Service from the Service manager
    mVppSession = mVppService->getNewVppSession_1_2(0);
    if (mVppSession == NULL)
    {
        LOGE("vppSession is null");
        goto ERR_VPPCLIENT;
    }

    // This callback object handle is sent to the Service during the 'init()' call
    mCallback = new HidlVppCallbacks(this);
    if (mCallback == NULL)
    {
        LOGE("mCallback is null");
        goto ERR_VPPCLIENT;
    }

    // The 'HidlVppDeathRecipient::servicedied()' method gets called whenever the service dies.
    mVppService->linkToDeath(mDeathRecipient, 0 /*cookie*/);

ERR_VPPCLIENT:
    u32Ret = u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPPCLIENT_CONSTRUCTOR);
    LOGE_IF(u32Ret != VPP_OK, "u32VppStats_Stop returned error %u", u32Ret);
}

VppClient::~VppClient()
{
    uint32_t u32Ret = VPP_OK;

    LOGI("destructor called, this=%p", this);

    // Terminate the session in case the destructor gets called before the term()
    this->term();
    if (mVppService != NULL)
    {
        mVppService->unlinkToDeath(mDeathRecipient);
        mVppService = NULL;
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

void *VppClient::init(uint32_t flags, struct vpp_callbacks cb)
{
    uint32_t u32Ret = VPP_OK;

    // Check if the Vpp session is available
    if (mVppSession == NULL)
        return NULL;

    // Reset the Client Buffer Ids
    mVppClientBufId = 0;

    /*
    * Callbacks from the calling module needs to be stored
    * VppClientCallback methods will use it trigger the callback
    * to the calling module.
    */
    mCb = cb;

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPPCLIENT_INIT);
    u32Ret = mVppSession->vppInit(flags, mCallback);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPPCLIENT_INIT);

    return ((u32Ret == VPP_OK) ? this : NULL);
}

void VppClient::term()
{
    // Check if the Vpp session is available
    if (mVppSession == NULL)
        return;

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPPCLIENT_TERM);
    mVppSession->vppTerm();
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPPCLIENT_TERM);
}

uint32_t VppClient::setCtrl(struct hqv_control ctrl, struct vpp_requirements *req)
{
    uint32_t u32Ret = VPP_OK;
    VppControl_1_2 stVppCtrl;

    if (mVppSession == NULL)
        return VPP_ERR_STATE;

    if (req == NULL)
        return VPP_ERR_PARAM;

    // Convert the data structures to Hidl compatible data
    u32Ret = HidlVppUtils::hqvCtrlToHidl(ctrl, stVppCtrl);
    if (u32Ret != VPP_OK)
    {
        LOGE("VppClient::setCtrl hqvCtrlToHidl returned Err=%u", u32Ret);
        return u32Ret;
    }

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPPCLIENT_SETCTRL);
    mVppSession->vppSetCtrl_1_2(stVppCtrl, [this] (auto req) {
        mVppReq = req;
        LOGD("vppReq_cb %u, fmt_mask=0x%llx",
             mVppReq.metadata.cnt, (unsigned long long)mVppReq.inColorFmtMask);
    });

    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPPCLIENT_SETCTRL);

    u32Ret = HidlVppUtils::hidlToVppReq(req, mVppReq);
    if (u32Ret != VPP_OK)
    {
        LOGE("VppClient::setCtrl hidlToVppReq returned Err=%u", u32Ret);
        return u32Ret;
    }

    return mVppReq.retStatus;
}

uint32_t VppClient::setParameter(enum vpp_port port, struct vpp_port_param param)
{
    uint32_t u32Ret = VPP_OK;
    VppPortParam stParam;

    if (mVppSession == NULL)
        return VPP_ERR_STATE;

    // Convert the data structures to Hidl compatible data
    HidlVppUtils::vppPortParamToHidl(param, stParam);

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPPCLIENT_SETPARAM);
    u32Ret = mVppSession->vppSetParameter((VppPort)port, stParam);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPPCLIENT_SETPARAM);

    return u32Ret;
}

uint32_t VppClient::queueBuf(enum vpp_port ePort, struct vpp_buffer *buf)
{
    uint32_t u32Ret = VPP_OK, u32Key;
    VppBuffer stVppBuf;
    bool bVppBypass;

    if (mVppSession == NULL)
        return VPP_ERR_STATE;

    if (buf == NULL)
        return VPP_ERR_PARAM;

    /*
    * Register the queued buffer.In case of the Service crashes/dies the
    * available registered buffers will be returned to the client(caller module).
    * The buffers are deregistered in the IBD/OBD callbacks.
    */
    VppClientBuf sClientBuf = {
        .ePortType = ePort,
        .bufferId = getNextBufferId(),
        .pstBuffer = buf,
    };

    registerVppBuf(sClientBuf);

    {
        Mutex::Autolock a(mVppClientMutex);
        bVppBypass = mVppClientBypass;
    }
    LOGI("VppClient:queueBuf Id:%u bypass:%u port:%u ts=%" PRIu64 " ",
         sClientBuf.bufferId, bVppBypass, (uint32_t)ePort, buf->timestamp);

    // If in bypass then send this buffer directly to the OBD
    if (false == bVppBypass)
    {
        // Convert the data structures to Hidl compatible data
        u32Ret = HidlVppUtils::vppBufferToHidl(buf, stVppBuf);
        if (u32Ret != VPP_OK)
        {
            deregisterVppBuf(sClientBuf.bufferId);
            LOGE("VppClient::queueBuf vppBufferToHidl returned Err=%u", u32Ret);
            return u32Ret;
        }

        // Get the next available Buffer id
        stVppBuf.bufferId = sClientBuf.bufferId;

        u32Key = (ePort==VPP_PORT_INPUT) ? PL_VPPCLIENT_QUEUE_BUF_IN : PL_VPPCLIENT_QUEUE_BUF_OUT;

        u32VppStats_Start(&mStCtx, mStatHandle, u32Key);
        u32Ret = mVppSession->vppQueueBuf((VppPort)ePort, stVppBuf);
        if (u32Ret != VPP_OK)
        {
            deregisterVppBuf(sClientBuf.bufferId);
            LOGE("VppClient::queueBuf vppQueueBuf returned Err=%u", u32Ret);
        }
        u32VppStats_Stop(&mStCtx, mStatHandle, u32Key);

        releaseNativeHandle(stVppBuf.pixel.handleFd.getNativeHandle());
        releaseNativeHandle(stVppBuf.extradata.handleFd.getNativeHandle());
    }
    else
    {
        clearVppBufs();
    }

    return u32Ret;
}

uint32_t VppClient::reconfigure(struct vpp_port_param input_param,
                                struct vpp_port_param output_param)
{
    uint32_t u32Ret = VPP_OK;
    VppPortParam stInPortParam, stOutPortParam;

    if (mVppSession == NULL)
        return VPP_ERR_STATE;

    // Convert the data structures to Hidl compatible data
    HidlVppUtils::vppPortParamToHidl(input_param, stInPortParam);
    HidlVppUtils::vppPortParamToHidl(output_param, stOutPortParam);

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPPCLIENT_RECONFIG);
    u32Ret = mVppSession->vppReconfigure(stInPortParam, stOutPortParam);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPPCLIENT_RECONFIG);

    return u32Ret;
}

uint32_t VppClient::flush(enum vpp_port ePort)
{
    uint32_t u32Ret = VPP_OK;
    bool bVppBypass;

    if (mVppService == NULL)
        return VPP_ERR_STATE;

    {
        Mutex::Autolock a(mVppClientMutex);
        bVppBypass = mVppClientBypass;
    }
    // Return all the OUTPUT-PORT buffers if in bypass.
    if (true == bVppBypass)
    {
        struct vpp_event e;
        if (VPP_PORT_OUTPUT == ePort)
        {
            /*
            * Copy the contents of the container to a temporary and clear its contents
            * The copied buffer references will be used for Callbacks.
            */
            std::vector<VppClientBuf> lVppBufs;
            {
                Mutex::Autolock a(mVppClientMutex);
                lVppBufs = mVppClientFlushBufs;
                mVppClientFlushBufs.clear();
            }

            for (VppClientBuf &stBuf : lVppBufs)
            {
                mCb.output_buffer_done(mCb.pv, stBuf.pstBuffer);
            }
        }
        /* Send the FLUSH_DONE event for the port being requested*/
        e.type = VPP_EVENT_FLUSH_DONE;
        e.flush_done.port = ePort;
        mCb.vpp_event(mCb.pv, e);
    }
    else
    {
        u32VppStats_Start(&mStCtx, mStatHandle, PL_VPPCLIENT_FLUSH);
        u32Ret = mVppSession->vppFlush((VppPort)ePort);
        u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPPCLIENT_FLUSH);
    }

    return u32Ret;
}

uint32_t VppClient::setVidProp(struct video_property prop)
{
    uint32_t u32Ret = VPP_OK;
    VideoProperty stProp;

    if (mVppSession == NULL)
        return VPP_ERR_STATE;

    // Convert the data structures to Hidl compatible data
    u32Ret = HidlVppUtils::vppPropToHidl(prop, stProp);
    if (u32Ret != VPP_OK)
        return VPP_ERR_STATE;

    u32VppStats_Start(&mStCtx, mStatHandle, PL_VPPCLIENT_SETVIDPROP);
    u32Ret = mVppSession->vppSetVidProp(stProp);
    u32VppStats_Stop(&mStCtx, mStatHandle, PL_VPPCLIENT_SETVIDPROP);

    return u32Ret;
}

// Register the VppClientBuf to the pool
bool VppClient::registerVppBuf(VppClientBuf buf)
{
    Mutex::Autolock a(mVppClientMutex);

    // Register the buffer to Vpp Client buffer container
    mVppClientBufs.push_back(buf);
    LOGI("VppClient::registerVppBuf buffer with size %u id %u ts=%" PRIu64 " flags:%u",
         (uint32_t)mVppClientBufs.size(), buf.bufferId,
         buf.pstBuffer->timestamp, buf.pstBuffer->flags);

    return true;
}

// Deregister the VppClientBuf from the pool
struct vpp_buffer* VppClient::deregisterVppBuf(uint32_t bufId)
{
    struct vpp_buffer *pstVppBuf = NULL;
    Mutex::Autolock a(mVppClientMutex);

    // Find the buffer corresponding to the cookie and deregister it
    for (unsigned i = 0; i < mVppClientBufs.size(); ++i)
    {
        if (mVppClientBufs[i].bufferId == bufId)
        {
            pstVppBuf = mVppClientBufs[i].pstBuffer;
            // Erase it from the container
            mVppClientBufs.erase(mVppClientBufs.begin()+i);
            LOGI("VppClient:de-registered buffer with size %u bufId %u ",
                 (uint32_t)mVppClientBufs.size(), bufId);
        }
    }

    return pstVppBuf;
}

// De-Register and return all the VppClientBufs from the pool
bool VppClient::clearVppBufs( void )
{
    /*
    * Copy the contents of the container to a temporary and clear its contents
    * The copied buffer references will be used for Callbacks.
    */
    std::vector<VppClientBuf> lVppBufs;
    {
        Mutex::Autolock a(mVppClientMutex);
        lVppBufs = mVppClientBufs;
        mVppClientBufs.clear();
    }

    LOGI("VppClient::clearVppBufs size %u",(uint32_t)lVppBufs.size());
    for (VppClientBuf &stBuf : lVppBufs)
    {
        /*
        * The output buffer is not yet filled up might contain garbage data.
        * The input buffer can still be used for displaying.
        */
        if(stBuf.ePortType == VPP_PORT_OUTPUT)
        {
            // Set the filled_len to 0, Omx will recycle the buffer
            stBuf.pstBuffer->pixel.filled_len = 0;
            mVppClientFlushBufs.push_back(stBuf);
        }
        else
        {
            mCb.output_buffer_done(mCb.pv, stBuf.pstBuffer);
        }
    }

    return true;
}

// Returns the next buffer id
uint32_t VppClient::getNextBufferId(void)
{
    return (mVppClientBufId++);
}

void VppClient::releaseNativeHandle(const native_handle_t *nh)
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

// HIDL utilities
void VppClient::HidlVppUtils::vppCtrlToHidl(const struct hqv_ctrl_cade v, V1_1::HqvCtrlCade& h)
{
    using vendor::qti::hardware::vpp::V1_1::HqvMode;

    h.mode = (HqvMode)v.mode;
    h.cadeLevel = v.cade_level;
    h.contrast = v.contrast;
    h.saturation = v.saturation;
}

void VppClient::HidlVppUtils::vppCtrlToHidl(const struct hqv_ctrl_aie v, V1_1::HqvCtrlAie& h)
{
    using vendor::qti::hardware::vpp::V1_1::HqvMode;
    using vendor::qti::hardware::vpp::V1_1::HqvHueMode;

    h.mode = (HqvMode)v.mode;
    h.hueMode = (HqvHueMode)v.hue_mode;
    h.cadeLevel = v.cade_level;
    h.ltmLevel = v.ltm_level;
    h.ltmSatGain = v.ltm_sat_gain;
    h.ltmSatOffset = v.ltm_sat_offset;
    h.ltmAceStr = v.ltm_ace_str;
    h.ltmAceBriL = v.ltm_ace_bright_l;
    h.ltmAceBriH = v.ltm_ace_bright_h;
}

void VppClient::HidlVppUtils::vppCtrlToHidl(const struct hqv_ctrl_di v, V1_1::HqvCtrlDi& h)
{
    using vendor::qti::hardware::vpp::V1_1::HqvDiMode;

    h.mode = (HqvDiMode)v.mode;
}

void VppClient::HidlVppUtils::vppCtrlToHidl(const struct hqv_ctrl_tnr v, V1_1::HqvCtrlTnr& h)
{
    using vendor::qti::hardware::vpp::V1_1::HqvMode;

    h.mode = (HqvMode)v.mode;
    h.level = v.level;
}

void VppClient::HidlVppUtils::vppCtrlToHidl(const struct hqv_ctrl_cnr v, V1_1::HqvCtrlCnr& h)
{
    using vendor::qti::hardware::vpp::V1_1::HqvMode;

    h.mode = (HqvMode)v.mode;
    h.level = v.level;
}

void VppClient::HidlVppUtils::vppCtrlToHidl(const struct hqv_ctrl_global_demo v, V1_1::HqvCtrlGlobalDemo& h)
{
    using vendor::qti::hardware::vpp::V1_1::HqvSplitDirection;

    h.processPercent = v.process_percent;
    h.processDirection = (HqvSplitDirection)v.process_direction;
}

void VppClient::HidlVppUtils::vppCtrlToHidl(const struct hqv_ctrl_ear v, V1_1::HqvCtrlEar& h)
{
    using vendor::qti::hardware::vpp::V1_1::HqvEarMode;

    h.mode = (HqvEarMode)v.mode;
}

void VppClient::HidlVppUtils::vppCtrlToHidl(const struct hqv_ctrl_qbr v, V1_1::HqvCtrlQbr& h)
{
    using vendor::qti::hardware::vpp::V1_1::HqvQbrMode;

    h.mode = (HqvQbrMode)v.mode;
}

void VppClient::HidlVppUtils::vppCtrlToHidl(const struct hqv_ctrl_meas v, V1_1::HqvCtrlMeas& h)
{
    h.enable = v.enable;
}

void VppClient::HidlVppUtils::vppCtrlToHidl(const struct hqv_ctrl_frc v, V1_1::HqvCtrlFrc& h)
{
    using vendor::qti::hardware::vpp::V1_1::HqvFrcMode;
    using vendor::qti::hardware::vpp::V1_1::HqvFrcLevel;
    using vendor::qti::hardware::vpp::V1_1::HqvFrcInterp;

    if (!v.segments || !v.num_segments)
        return;

    h.mode = (HqvFrcMode)v.segments[0].mode;
    h.level = (HqvFrcLevel)v.segments[0].level;
    h.interp = (HqvFrcInterp)v.segments[0].interp;
}

void VppClient::HidlVppUtils::vppCtrlToHidl(const struct hqv_ctrl_frc v, V1_2::VppCtrlFrc& h)
{
    using vendor::qti::hardware::vpp::V1_2::VppCtrlFrcSegment;
    using vendor::qti::hardware::vpp::V1_2::VppCtrlFrc;
    using vendor::qti::hardware::vpp::V1_1::HqvFrcMode;
    using vendor::qti::hardware::vpp::V1_1::HqvFrcLevel;
    using vendor::qti::hardware::vpp::V1_1::HqvFrcInterp;

    if (!v.segments || !v.num_segments)
        return;

    // If there is only one segment and its set to OFF, do not send any
    // frc controls over to the vppservice
    if (v.num_segments == 1 && v.segments[0].mode == HQV_FRC_MODE_OFF)
        return;

    h.segments.resize(v.num_segments);

    for (uint32_t count = 0; count < v.num_segments; count++)
    {
       VppCtrlFrcSegment& frcSeg = h.segments[count];

       frcSeg.mode = (HqvFrcMode)v.segments[count].mode;
       frcSeg.level = (HqvFrcLevel)v.segments[count].level;
       frcSeg.interp = (HqvFrcInterp)v.segments[count].interp;
       frcSeg.ts_start = v.segments[count].ts_start;
       frcSeg.frame_copy_on_fallback = v.segments[count].frame_copy_on_fallback;
       frcSeg.frame_copy_input = v.segments[count].frame_copy_input;
    }
}

uint32_t VppClient::HidlVppUtils::hqvCtrlToHidl(const struct hqv_control v, HqvControl& h)
{
    using vendor::qti::hardware::vpp::V1_1::HqvMode;
    using vendor::qti::hardware::vpp::V1_1::HqvControlType;

    uint32_t u32Ret = VPP_OK;

    h.mode = (HqvMode)v.mode;
    h.ctrlType = (HqvControlType)v.ctrl_type;

    switch (v.ctrl_type)
    {
        case HQV_CONTROL_NONE:          /* do nothing */                    break;
        case HQV_CONTROL_CADE:          vppCtrlToHidl(v.cade, h.u.cade);    break;
        case HQV_CONTROL_AIE:           vppCtrlToHidl(v.aie, h.u.aie);      break;
        case HQV_CONTROL_FRC:           vppCtrlToHidl(v.frc, h.u.frc);      break;
        case HQV_CONTROL_DI:            vppCtrlToHidl(v.di, h.u.di);        break;
        case HQV_CONTROL_TNR:           vppCtrlToHidl(v.tnr, h.u.tnr);      break;
        case HQV_CONTROL_CNR:           vppCtrlToHidl(v.cnr, h.u.cnr);      break;
        case HQV_CONTROL_GLOBAL_DEMO:   vppCtrlToHidl(v.demo, h.u.demo);    break;
        case HQV_CONTROL_EAR:           vppCtrlToHidl(v.ear, h.u.ear);      break;
        case HQV_CONTROL_QBR:           vppCtrlToHidl(v.qbr, h.u.qbr);      break;
        case HQV_CONTROL_MEAS:          vppCtrlToHidl(v.meas, h.u.meas);    break;
        case HQV_CONTROL_CUST:          // fallthrough
        case HQV_CONTROL_MAX:           // fallthrough
        default:
            LOGE("Unsupported Control Type %u", (uint32_t)v.ctrl_type);
            u32Ret = VPP_ERR_PARAM;
            break;
    }

    return u32Ret;
}

uint32_t VppClient::HidlVppUtils::hqvCtrlToHidl(const struct hqv_control v, V1_2::VppControl_1_2& h)
{
    using vendor::qti::hardware::vpp::V1_1::HqvMode;
    using vendor::qti::hardware::vpp::V1_1::HqvControlType;

    uint32_t u32Ret = VPP_OK;

    h.mode = (HqvMode)v.mode;
    h.ctrlType = (HqvControlType)v.ctrl_type;

    switch (v.ctrl_type)
    {
        case HQV_CONTROL_NONE:          /* do nothing */                    break;
        case HQV_CONTROL_CADE:          vppCtrlToHidl(v.cade, h.u.cade);    break;
        case HQV_CONTROL_AIE:           vppCtrlToHidl(v.aie, h.u.aie);      break;
        case HQV_CONTROL_FRC:           vppCtrlToHidl(v.frc, h.frc);        break;
        case HQV_CONTROL_DI:            vppCtrlToHidl(v.di, h.u.di);        break;
        case HQV_CONTROL_TNR:           vppCtrlToHidl(v.tnr, h.u.tnr);      break;
        case HQV_CONTROL_CNR:           vppCtrlToHidl(v.cnr, h.u.cnr);      break;
        case HQV_CONTROL_GLOBAL_DEMO:   vppCtrlToHidl(v.demo, h.u.demo);    break;
        case HQV_CONTROL_EAR:           vppCtrlToHidl(v.ear, h.u.ear);      break;
        case HQV_CONTROL_QBR:           vppCtrlToHidl(v.qbr, h.u.qbr);      break;
        case HQV_CONTROL_MEAS:          vppCtrlToHidl(v.meas, h.u.meas);    break;
        case HQV_CONTROL_CUST:          // fallthrough
        case HQV_CONTROL_MAX:           // fallthrough
        default:
            LOGE("Unsupported Control Type %u", (uint32_t)v.ctrl_type);
            u32Ret = VPP_ERR_PARAM;
            break;
    }

    return u32Ret;
}

void VppClient::HidlVppUtils::vppPortParamToHidl(const struct vpp_port_param param, VppPortParam& stParam)
{
    using vendor::qti::hardware::vpp::V1_1::VppColorFormat;

    switch (param.fmt)
    {
        VPP_PARAM_MAP(stParam.fmt, VPP_COLOR_FORMAT_NV12_VENUS,
                      VppColorFormat::VPP_COLOR_FORMAT_NV12_VENUS);
        VPP_PARAM_MAP(stParam.fmt, VPP_COLOR_FORMAT_NV21_VENUS,
                      VppColorFormat::VPP_COLOR_FORMAT_NV21_VENUS);
        VPP_PARAM_MAP(stParam.fmt, VPP_COLOR_FORMAT_P010,
                      VppColorFormat::VPP_COLOR_FORMAT_P010);
        VPP_PARAM_MAP(stParam.fmt, VPP_COLOR_FORMAT_UBWC_NV12,
                      VppColorFormat::VPP_COLOR_FORMAT_UBWC_NV12);
        VPP_PARAM_MAP(stParam.fmt, VPP_COLOR_FORMAT_UBWC_NV21,
                      VppColorFormat::VPP_COLOR_FORMAT_UBWC_NV21);
        VPP_PARAM_MAP(stParam.fmt, VPP_COLOR_FORMAT_UBWC_TP10,
                      VppColorFormat::VPP_COLOR_FORMAT_UBWC_TP10);
        default:
            LOGE("Setting to unknown color format=%d", param.fmt);
            stParam.fmt = (VppColorFormat)param.fmt;
            break;
    }
    stParam.height = param.height;
    stParam.width = param.width;
    stParam.stride = param.stride;
    stParam.scanlines = param.scanlines;
}

uint32_t VppClient::HidlVppUtils::vppPropToHidl(const struct video_property prop, VideoProperty& stProp)
{
    using vendor::qti::hardware::vpp::V1_1::VidPropType;
    using vendor::qti::hardware::vpp::V1_1::VppCodecType;

    uint32_t u32Ret = VPP_OK;

    stProp.propertyType = (VidPropType)prop.property_type;

    switch (prop.property_type)
    {
        case VID_PROP_CODEC:
            stProp.u.codec.eCodec = (VppCodecType)prop.codec.eCodec;
            break;
        case VID_PROP_NON_REALTIME:
            stProp.u.nonRealtime.bNonRealtime = (uint32_t)prop.non_realtime.bNonRealtime;
            break;
        case VID_PROP_OPERATING_RATE:
            stProp.u.operatingRate.u32OperatingRate = prop.operating_rate.u32OperatingRate;
            break;
        case VID_PROP_MAX:
        default:
            LOGE("Unsupported property_type %u", prop.property_type);
            u32Ret = VPP_ERR_PARAM;
            break;
    };
    return u32Ret;
}

uint32_t VppClient::HidlVppUtils::vppMemBufToHidl(const struct vpp_mem_buffer memBuf, VppMemBuffer& stVppMemBuf)
{
    native_handle_t *nativeHandle = nullptr;
    if (memBuf.fd >= 0)
    {
        nativeHandle = native_handle_create(1 /* numFds */, 0 /* numInts */);
        if (nativeHandle == nullptr)
        {
            LOGE("native_handle_create() return nullptr");
            return VPP_ERR;
        }
        nativeHandle->data[0] = dup(memBuf.fd);
    }

    stVppMemBuf.handleFd.setTo(nativeHandle);
    stVppMemBuf.allocLen = memBuf.alloc_len;

    stVppMemBuf.filledLen = memBuf.filled_len;
    stVppMemBuf.offset = memBuf.offset;
    stVppMemBuf.validDataLen = memBuf.valid_data_len;

    return VPP_OK;
}

uint32_t VppClient::HidlVppUtils::vppBufferToHidl(const struct vpp_buffer *buf, VppBuffer& stVppBuf)
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
        stVppBuf.pvGralloc.setTo((native_handle_t *)buf->pvGralloc);
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

uint32_t VppClient::HidlVppUtils::hidlToVppEvent(struct vpp_event& e, const VppEvent& stEvt)
{
    uint32_t u32Ret = VPP_OK;

    e.type = (vpp_event_type)stEvt.type;

    switch (e.type)
    {
        case VPP_EVENT_FLUSH_DONE:
            e.flush_done.port = (vpp_port)stEvt.u.flushDone.port;
            break;
        case VPP_EVENT_RECONFIG_DONE:
            e.port_reconfig_done.reconfig_status = stEvt.u.reconfigDone.reconfigStatus;
            hidlToVppReq(&e.port_reconfig_done.req, stEvt.u.reconfigDone.req);
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

uint32_t VppClient::HidlVppUtils::hidlToVppReq(struct vpp_requirements *pstReq,
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

void VppClient::readProperties(uint32_t *pu32StatsEn)
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
