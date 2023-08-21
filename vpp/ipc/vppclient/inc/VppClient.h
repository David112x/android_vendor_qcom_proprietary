/*!
 * @file VppClient.h
 *
 * @cr
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @services
 */

#ifndef _VPP_CLIENT_H_
#define _VPP_CLIENT_H_

#include <utils/RefBase.h>
#include <pthread.h>
#include <utils/Mutex.h>
#include <vendor/qti/hardware/vpp/1.2/IHidlVppService.h>
#include <vendor/qti/hardware/vpp/1.2/IHidlVpp.h>
#include <vendor/qti/hardware/vpp/1.1/IHidlVppCallbacks.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include "vpp_stats.h"

namespace android {

// Namespaces for Hidl interface
using namespace vendor::qti::hardware::vpp;
using ::vendor::qti::hardware::vpp::V1_2::IHidlVppService;
using ::vendor::qti::hardware::vpp::V1_2::IHidlVpp;
using ::vendor::qti::hardware::vpp::V1_1::IHidlVppCallbacks;
using ::vendor::qti::hardware::vpp::V1_1::HqvControl;
using ::vendor::qti::hardware::vpp::V1_2::VppControl_1_2;
using ::vendor::qti::hardware::vpp::V1_1::VideoProperty;
using ::vendor::qti::hardware::vpp::V1_1::VppBuffer;
using ::vendor::qti::hardware::vpp::V1_1::VppMemBuffer;
using ::vendor::qti::hardware::vpp::V1_1::VppPort;
using ::vendor::qti::hardware::vpp::V1_1::VppPortParam;
using ::vendor::qti::hardware::vpp::V1_1::VppRequirements;
using ::vendor::qti::hardware::vpp::V1_1::VppEvent;

typedef struct VppClientBuf {
    enum vpp_port ePortType;
    uint32_t bufferId;
    struct vpp_buffer *pstBuffer;
} t_StVppClientBuf;

class VppClient : public RefBase
{
public:

    class HidlVppUtils {
    public:
        // From vpp to hidl
        static uint32_t hqvCtrlToHidl(const struct hqv_control v, HqvControl& h);
        static uint32_t hqvCtrlToHidl(const struct hqv_control v, V1_2::VppControl_1_2& h);
        static void vppPortParamToHidl(const struct vpp_port_param param, VppPortParam& stParam);
        static uint32_t vppPropToHidl(const struct video_property prop, VideoProperty& stProp);
        static uint32_t vppBufferToHidl(const struct vpp_buffer *buf, VppBuffer& stVppBuf);

        // From hidl to vpp
        static uint32_t hidlToVppReq(struct vpp_requirements *pstReq,
                                     const VppRequirements& stReq);
        static uint32_t hidlToVppEvent(struct vpp_event& e, const VppEvent& stEvt);

    private:
        static uint32_t vppMemBufToHidl(const struct vpp_mem_buffer memBuf,
                                        VppMemBuffer& stVppMemBuf);

        static void vppCtrlToHidl(const struct hqv_ctrl_cade v, V1_1::HqvCtrlCade& h);
        static void vppCtrlToHidl(const struct hqv_ctrl_aie v, V1_1::HqvCtrlAie& h);
        static void vppCtrlToHidl(const struct hqv_ctrl_di v, V1_1::HqvCtrlDi& h);
        static void vppCtrlToHidl(const struct hqv_ctrl_tnr v, V1_1::HqvCtrlTnr& h);
        static void vppCtrlToHidl(const struct hqv_ctrl_cnr v, V1_1::HqvCtrlCnr& h);
        static void vppCtrlToHidl(const struct hqv_ctrl_global_demo v, V1_1::HqvCtrlGlobalDemo& h);
        static void vppCtrlToHidl(const struct hqv_ctrl_ear v, V1_1::HqvCtrlEar& h);
        static void vppCtrlToHidl(const struct hqv_ctrl_qbr v, V1_1::HqvCtrlQbr& h);
        static void vppCtrlToHidl(const struct hqv_ctrl_meas v, V1_1::HqvCtrlMeas& h);
        static void vppCtrlToHidl(const struct hqv_ctrl_frc v, V1_1::HqvCtrlFrc& h);
        static void vppCtrlToHidl(const struct hqv_ctrl_frc v, V1_2::VppCtrlFrc& h);
    };

    /*
    * class VppClientDeathRecipient : implements the serviceDied(). This method will
    * be called whenever the vppService dies/exits unexpectedly
    */
    class VppClientDeathRecipient : public hardware::hidl_death_recipient
    {
    public:
        VppClientDeathRecipient(VppClient *client) : mClient(client){};
        ~VppClientDeathRecipient() {};

        // DeathRecipient callback
        virtual void serviceDied(uint64_t cookie,
                   const ::android::wp<::android::hidl::base::V1_0::IBase>& who);
    private:
        VppClient *mClient;
    };

    // Public VppClient methods
    VppClient();
    ~VppClient();

    void* init(uint32_t flags, struct vpp_callbacks cb);
    void term();
    uint32_t setCtrl(struct hqv_control ctrl, struct vpp_requirements *req);
    uint32_t setParameter(enum vpp_port port, struct vpp_port_param param);
    uint32_t queueBuf(enum vpp_port port, struct vpp_buffer *buf);
    uint32_t reconfigure(struct vpp_port_param input_param,
                         struct vpp_port_param output_param);
    uint32_t flush(enum vpp_port ePort);
    uint32_t setVidProp(struct video_property prop);

    void releaseNativeHandle(const native_handle_t *nh);
    void readProperties(uint32_t *pu32StatsEn);

private:
    // Hidl interface instances
    sp<IHidlVppService> mVppService;
    sp<IHidlVpp> mVppSession;
    sp<IHidlVppCallbacks> mCallback;
    sp<VppClientDeathRecipient> mDeathRecipient;

    /*
    * vector to hold the VppClientBufs,used for tracking the buffers submitted
    * to Vpp for processing (Input and output buffers).
    * Incase the client becomes inaccessible, the remaining buffers are returned back.
    */
    std::vector<VppClientBuf> mVppClientBufs;

    /*
    * vector to hold the VppClientBufs, ref to the queued OUTPUT-PORT
    * buffers at the instance when the service exits unexpectedly.
    * These buffers will be returned during the 'flush()'
    */
    std::vector<t_StVppClientBuf> mVppClientFlushBufs;

    // Context for holding the vpp client's stats
    t_StVppCtx mStCtx;
    void *mStatHandle;

    // Client buffer Id
    uint32_t mVppClientBufId;

    // vpp_requirements returned during setCtrl() cb
    VppRequirements mVppReq;

    // Register the VppClientBuf to the pool
    bool registerVppBuf(VppClientBuf buf);

    // Get the next buffer id
    uint32_t getNextBufferId(void);

public:
    Mutex mVppClientMutex;
    // Real stuff needed to interact with a client
    struct vpp_callbacks mCb;
    // if TRUE then the vppClient needs to bypass all the incoming buffers
    bool mVppClientBypass;

    // Deregister the VppClientBuf from the pool
    struct vpp_buffer* deregisterVppBuf(uint32_t bufId);
    // De-Register and return all the VppClientBufs from the pool
    bool clearVppBufs(void);

};

};
#endif // _VPP_CLIENT_H_
