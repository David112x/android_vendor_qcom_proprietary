/*-------------------------------------------------------------------
Copyright (c) 2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
--------------------------------------------------------------------*/

#include <errno.h>
#include <hardware/sensors.h>
#include <math.h>
#include <sys/types.h>
#include <stdint.h>
#include "sensors_hw_module.h"

using Result = ::android::hardware::sensors::V1_0::Result;
using RateLevel = ::android::hardware::sensors::V1_0::RateLevel;
using SharedMemInfo = ::android::hardware::sensors::V1_0::SharedMemInfo;
using SensorInfo = ::android::hardware::sensors::V1_0::SensorInfo;

/*implementation of HAL 2.0 APIs*/
namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {


/*implementation start of hal 2.0 class methods */
sensors_hw_module::sensors_hw_module()
{

}
sensors_hw_module::~sensors_hw_module()
{

}

/*start of public APIs implementation*/
Return<Result> sensors_hw_module::initialize(
    const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
    const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
    const sp<ISensorsCallback>& sensorsCallback) {
    return Result::OK;
}


Return<Result> sensors_hw_module::activate(int32_t handle, bool en)
{
    return Result::OK;
}

Return<Result> sensors_hw_module::batch(int32_t handle,
                     int64_t samplingPeriodNs,
                     int64_t maxReportLatencyNs)
{
    return Result::OK;
}

Return<Result> sensors_hw_module::flush(int32_t handle)
{
    return Result::OK;
}

Return<Result> sensors_hw_module::injectSensorData(const Event &data)
{
    return Result::INVALID_OPERATION;
}

Return<void> sensors_hw_module::registerDirectChannel(const SharedMemInfo& mem,
                                   registerDirectChannel_cb _hidl_cb)
{
    return Void();
}

Return<Result> sensors_hw_module::unregisterDirectChannel(int32_t channelHandle)
{
#ifndef SNS_DIRECT_REPORT_SUPPORT
    // HAL does not support
    return Result::INVALID_OPERATION;
#else
    return Result::OK;
#endif
}


Return<void> sensors_hw_module::configDirectReport(
                int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
                configDirectReport_cb _hidl_cb)
{
    return Void();
}

Return<void> sensors_hw_module::getSensorsList( getSensorsList_cb _hidl_cb)
{

    hidl_vec<SensorInfo> out;
    out.resize(0);
    _hidl_cb(out);
    return Void();

}

Return<Result> sensors_hw_module::setOperationMode(OperationMode mode)
{
    return Result::INVALID_OPERATION;
}

}
}
}
}
}

