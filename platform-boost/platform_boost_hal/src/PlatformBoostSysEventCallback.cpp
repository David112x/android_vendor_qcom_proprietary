/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "PlatformBoostSysEventCallback.h"

namespace vendor {
namespace qti {
namespace platform_boost {
namespace V1_0 {
namespace implementation {

Return<void> PlatformBoostSysEventCallback::onEvent(const hidl_string& event, const hidl_string& value){
    return Void();
}

IPlatformBoostSysEventCallback* HIDL_FETCH_IPlatformBoostSysEventCallback(const char* /* name */) {
    ALOGI("start to HIDL_FETCH_IPlatformBoostSysEventCallback");
    return new PlatformBoostSysEventCallback();
}

}
}  // namespace V1_0
}  // namespace platform_boost
}  // namespace qti
}  // namespace vendor

