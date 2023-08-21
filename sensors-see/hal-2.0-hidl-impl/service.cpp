/*
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#define LOG_TAG "android.hardware.sensors@2.0-service"

#include <android/hardware/sensors/2.0/ISensors.h>
#include <hidl/HidlTransportSupport.h>
#include <log/log.h>
#include <utils/StrongPointer.h>
#include "sensors_hw_module.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::sensors::V2_0::ISensors;
using android::hardware::sensors::V2_0::implementation::sensors_hw_module;

int main(int /* argc */, char** /* argv */) {
    configureRpcThreadpool(1, true);
    android::sp<ISensors> sensor_module = new sensors_hw_module;
    if (sensor_module->registerAsService() != ::android::OK) {
        ALOGE("Failed to register Sensors HAL instance");
        return -1;
    }
    joinRpcThreadpool();
    return 1;  // joinRpcThreadpool shouldn't exit
}

