/*!
 * @file HidlVppService.h
 *
 * @cr
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @services
 */

#ifndef _HIDL_VPP_SERVICE_H_
#define _HIDL_VPP_SERVICE_H_

#include <vendor/qti/hardware/vpp/1.1/IHidlVppService.h>
#include <vendor/qti/hardware/vpp/1.2/IHidlVppService.h>
#include <vendor/qti/hardware/vpp/1.3/IHidlVppService.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace qti_vpp {

using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

using namespace ::vendor::qti::hardware::vpp;

struct HidlVppService : public V1_3::IHidlVppService
{
    Return<sp<V1_1::IHidlVpp>> getNewVppSession(uint32_t flags) override;
    Return<sp<V1_2::IHidlVpp>> getNewVppSession_1_2(uint32_t flags) override;
    Return<sp<V1_3::IHidlVpp>> getNewVppSession_1_3(uint32_t flags) override;
};

}  // namespace qti_vpp

#endif  // _HIDL_VPP_SERVICE_H_
