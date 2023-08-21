/*!
 * @file HidlVpp.h
 *
 * @cr
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @services
 */

#ifndef _HIDL_VPP_H_
#define _HIDL_VPP_H_

#include <vendor/qti/hardware/vpp/1.1/IHidlVpp.h>
#include <vendor/qti/hardware/vpp/1.2/IHidlVpp.h>
#include <vendor/qti/hardware/vpp/1.3/IHidlVpp.h>
#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <list>
#include <utils/Mutex.h>

#include "vpp.h"
#include "vpp_stats.h"
#include "vpp_def.h"

#define VPP_BUF_POOL_MAXSIZE 256

namespace qti_vpp {

using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::vendor::qti::hardware::vpp::V1_1::HqvControl;
using ::vendor::qti::hardware::vpp::V1_1::IHidlVpp;
using ::vendor::qti::hardware::vpp::V1_1::IHidlVppCallbacks;
using ::vendor::qti::hardware::vpp::V1_1::VideoProperty;
using ::vendor::qti::hardware::vpp::V1_1::VppBuffer;
using ::vendor::qti::hardware::vpp::V1_1::VppPort;
using ::vendor::qti::hardware::vpp::V1_1::VppPortParam;
using ::vendor::qti::hardware::vpp::V1_1::VppRequirements;
using ::vendor::qti::hardware::vpp::V1_1::VppMemBuffer;
using ::vendor::qti::hardware::vpp::V1_1::VppEvent;
using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::android::wp;
using ::android::Mutex;

using namespace android;
using namespace ::vendor::qti::hardware::vpp;

struct HidlVpp : public V1_3::IHidlVpp {
     HidlVpp();
    ~HidlVpp();

    /***************************************************************************
     * ::vendor::qti::hardware::vpp::V1_1::IHidlVppService follow.
     ****************************************************************************/
    Return<uint32_t> vppInit(uint32_t flags, const ::android::sp<IHidlVppCallbacks>& cb) override;
    Return<void> vppTerm() override;
    Return<void> vppSetCtrl(uint64_t cookie, const HqvControl& ctrl, vppSetCtrl_cb _hidl_cb) override;
    Return<uint32_t> vppSetParameter(VppPort port, const VppPortParam& param) override;
    Return<uint32_t> vppQueueBuf(VppPort port, const VppBuffer& buf) override;
    Return<uint32_t> vppReconfigure(const VppPortParam& inputParam, const VppPortParam& outputParam) override;
    Return<uint32_t> vppFlush(VppPort port) override;
    Return<uint32_t> vppSetVidProp(const VideoProperty& prop) override;

    /***************************************************************************
     * ::vendor::qti::hardware::vpp::V1_2::IHidlVppService follow.
     ****************************************************************************/
    Return<void> vppSetCtrl_1_2(const V1_2::VppControl_1_2 &ctrl, vppSetCtrl_1_2_cb _hidl_cb) override;

     /***************************************************************************
     * ::vendor::qti::hardware::vpp::V1_3::IHidlVppService follow.
     ****************************************************************************/
    Return<uint32_t> vppSetCtrl_1_3(const V1_2::VppControl_1_2 &ctrl) override;
    Return<uint32_t> vppSetParameter_1_3(VppPort port, const V1_3::VppPortParam_1_3& param) override;
    Return<uint32_t> vppReconfigure_1_3(const V1_3::VppPortParam_1_3& inputParam,
                                        const V1_3::VppPortParam_1_3& outputParam) override;
    Return<void> vppGetBufRequirements(vppGetBufRequirements_cb _hidl_cb) override;
    Return<uint32_t> vppOpen() override;
    Return<uint32_t> vppClose() override;

    static uint32_t vppBufferDone(void *pv, struct vpp_buffer *buf, enum vpp_port ePort);
    static void vppIBD(void *pv, struct vpp_buffer *buf);
    static void vppOBD(void *pv, struct vpp_buffer *buf);
    static void vppEVT(void *pv, struct vpp_event e);

    class HidlVppDeathRecipient : public hidl_death_recipient {
    public:
        HidlVppDeathRecipient(HidlVpp *hidlVpp) : mHidlVpp(hidlVpp) { }
        virtual void serviceDied(uint64_t cookie, const wp<IBase>& who);
    private:
        HidlVpp *mHidlVpp;
    };

    class HidlVppUtils {
    public:
        // From vpp to hidl
        static uint32_t vppReqToHidl(const struct vpp_requirements *pstReq,
                                     VppRequirements& stReq);
        static uint32_t vppBufferToHidl(const struct vpp_buffer *buf, VppBuffer& stVppBuf);
        static uint32_t vppEventToHidl(const struct vpp_event e, VppEvent& stEvt);

        // From hidl to vpp
        static uint32_t hidlToHqvCtrl(struct hqv_control &v, const HqvControl h);
        static uint32_t hidlToHqvCtrl(struct hqv_control &v, const V1_2::VppControl_1_2 h);
        static uint32_t hidlToVppReq(struct vpp_requirements *pstReq,
                                     const VppRequirements& stReq);
        static void hidlToVppPortParam(struct vpp_port_param& param,
                                       const VppPortParam& stParam);
        static void hidl_1_3ToVppPortParam(struct vpp_port_param& param,
                                          const V1_3::VppPortParam_1_3& stParam);
        static uint32_t hidlToVppProp(struct video_property& prop,
                                      const VideoProperty& stProp);
        static uint32_t hidlToVppBuffer(struct vpp_buffer *buf, const VppBuffer& stVppBuf);
        static void hidl_1_1ToHidl_1_3VppPortParam(const VppPortParam& stParam_1_1,
                                                   V1_3::VppPortParam_1_3& stParam_1_3);
        static void hidl_1_3ToHidl_1_1VppPortParam(VppPortParam& stParam_1_1,
                                                   const V1_3::VppPortParam_1_3& stParam_1_3);

        static void readProperties(uint32_t *pu32StatsEn);
    private:
        static uint32_t vppMemBufToHidl(const struct vpp_mem_buffer memBuf,
                                        VppMemBuffer& stVppMemBuf);

        static void hidlToVppMemBuf(struct vpp_mem_buffer& memBuf,
                                    const VppMemBuffer& stVppMemBuf);
        static void hidlToVppCtrl(struct hqv_ctrl_cade& v, const V1_1::HqvCtrlCade h);
        static void hidlToVppCtrl(struct hqv_ctrl_aie& v, const V1_1::HqvCtrlAie h);
        static void hidlToVppCtrl(struct hqv_ctrl_di& v, const V1_1::HqvCtrlDi h);
        static void hidlToVppCtrl(struct hqv_ctrl_tnr& v, const V1_1::HqvCtrlTnr h);
        static void hidlToVppCtrl(struct hqv_ctrl_cnr& v, const V1_1::HqvCtrlCnr h);
        static void hidlToVppCtrl(struct hqv_ctrl_global_demo& v,
                                  const V1_1::HqvCtrlGlobalDemo h);
        static void hidlToVppCtrl(struct hqv_ctrl_ear& v, const V1_1::HqvCtrlEar h);
        static void hidlToVppCtrl(struct hqv_ctrl_qbr& v, const V1_1::HqvCtrlQbr h);
        static void hidlToVppCtrl(struct hqv_ctrl_meas& v, const V1_1::HqvCtrlMeas h);
        static void hidlToVppCtrl(struct hqv_ctrl_frc& v, const V1_1::HqvCtrlFrc h);
        static void hidlToVppCtrl(struct hqv_ctrl_frc& v, const V1_2::VppCtrlFrc h);

        static void releaseNativeHandle(const native_handle_t *nh);
    };

    sp<IHidlVppCallbacks> mHidlCb;
    void* pvCtx;
    struct vpp_buffer mVppBuffers[VPP_BUF_POOL_MAXSIZE];
    std::list<struct vpp_buffer *> mBufPool;

    Mutex mMutex;
    // Context for holding the vpp client's stats
    t_StVppCtx mStCtx;
    void *mStatHandle;

    bool mClientDied;
    Mutex mDeathMutex;
    sp<HidlVppDeathRecipient> mClientDeathNotification;

    // Internal flags, VPP_FLAG_INPUT_PORT_SET, VPP_FLAG_OUTPUT_PORT_SET
    uint32_t mVppFlags;
};

}  // namespace qti_vpp

#endif  // _HIDL_VPP_H_
