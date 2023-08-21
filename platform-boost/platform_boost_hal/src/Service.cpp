/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#include <vendor/qti/platform_boost/1.0/IPlatformBoost.h>
#include <hidl/LegacySupport.h>
#include <cutils/log.h>

#include "PlatformBoost.h"

using ::vendor::qti::platform_boost::V1_0::IPlatformBoost;
using ::vendor::qti::platform_boost::V1_0::implementation::PlatformBoost;

using android::sp;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::status_t;
using android::OK;

int main() {
    android::sp<IPlatformBoost> service = new PlatformBoost();
    ALOGI("start to register PlatformBoost HAL service");
    configureRpcThreadpool(1, true /*callerWillJoin*/);
    if (service->registerAsService() != OK) {
        ALOGE("Can not register PlatformBoost HAL service");
        return -1;
    }

    ALOGI("registerd PlatformBoost HAL service");
    joinRpcThreadpool();

    return 1; // should never get here
}
