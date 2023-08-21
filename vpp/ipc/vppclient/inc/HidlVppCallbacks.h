/*!
 * @file HidlVppCallbacks.h
 *
 * @cr
 * Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @services
 */

#ifndef _VPP_HIDL_CALLBACKS_H_
#define _VPP_HIDL_CALLBACKS_H_

#include <vendor/qti/hardware/vpp/1.1/IHidlVppCallbacks.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include "VppClient.h"

namespace qti_vpp {

using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::vendor::qti::hardware::vpp::V1_1::IHidlVppCallbacks;
using ::vendor::qti::hardware::vpp::V1_1::VppBuffer;
using ::vendor::qti::hardware::vpp::V1_1::VppEvent;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::android::VppClient;

struct HidlVppCallbacks : public IHidlVppCallbacks
{
    HidlVppCallbacks(VppClient *client);
    ~HidlVppCallbacks();
    // Methods from ::vendor::qti::hardware::vpp::V1_1::IHidlVppCallbacks follow.
    Return<uint32_t> inputBufferDone(const VppBuffer& buf) override;
    Return<uint32_t> outputBufferDone(const VppBuffer& buf) override;
    Return<uint32_t> vppEvent(const VppEvent& e) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    VppClient *mClient;
};

}  // namespace qti_vpp

#endif  // _VPP_HIDL_CALLBACKS_H_
