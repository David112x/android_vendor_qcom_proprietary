/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#ifndef VENDOR_PLATFORM_BOOST_SERVICE_V1_0_IPLATFORMBOOSTSYSEVENTCALLBACK_H
#define VENDOR_PLATFORM_BOOST_SERVICE_V1_0_IPLATFORMBOOSTSYSEVENTCALLBACK_H

#include "Common.h"

#include <vendor/qti/platform_boost/1.0/IPlatformBoostSysEventCallback.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace vendor {
namespace qti {
namespace platform_boost {
namespace V1_0 {
namespace implementation {

using ::vendor::qti::platform_boost::V1_0::IPlatformBoostSysEventCallback;

using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;

using ::std::string;

struct PlatformBoostSysEventCallback : public IPlatformBoostSysEventCallback {
    Return<void> onEvent(const hidl_string& event, const hidl_string& value) override;
};

extern "C" IPlatformBoostSysEventCallback* HIDL_FETCH_IPlatformBoostSysEventListener(const char* name);

}
}  // namespace V1_0
}  // namespace platform_boost
}  // namespace qti
}  // namespace vendor

#endif // VENDOR_PLATFORM_BOOST_SERVICE_V1_0_IPLATFORMBOOSTSYSEVENTCALLBACK_H
