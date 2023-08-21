/*
 * Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include <hardware/sensors.h>
#include <android/hardware/sensors/2.0/ISensors.h>
#include <android/hardware/sensors/1.0/types.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <log/log.h>
#include <thread>
#include "sensors_hal.h"


using ::android::sp;
using ::android::hardware::EventFlag;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::MessageQueue;
using ::android::hardware::MQDescriptor;
using ::android::hardware::Return;
using ::android::hardware::Void;

using ::android::hardware::sensors::V1_0::SensorFlagBits;
using ::android::hardware::sensors::V1_0::SensorStatus;
using ::android::hardware::sensors::V2_0::ISensors;
using ::android::hardware::sensors::V2_0::ISensorsCallback;



namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

class sensors_hw_module : public ISensors {

using Event = ::android::hardware::sensors::V1_0::Event;
using OperationMode = ::android::hardware::sensors::V1_0::OperationMode;
using RateLevel = ::android::hardware::sensors::V1_0::RateLevel;
using Result = ::android::hardware::sensors::V1_0::Result;
using SharedMemInfo = ::android::hardware::sensors::V1_0::SharedMemInfo;
using SharedMemFormat = android::hardware::sensors::V1_0::SharedMemFormat;


public:

sensors_hw_module();
~sensors_hw_module();
/**
 * Enumerate all available (static) sensors.
 */
Return<void> getSensorsList(getSensorsList_cb _hidl_cb) override;

/**
 * Place the module in a specific mode. The following modes are defined
 *
 *  SENSOR_HAL_NORMAL_MODE - Normal operation. Default state of the module.
 *
 *  SENSOR_HAL_DATA_INJECTION_MODE - Loopback mode.
 *    Data is injected for the supported sensors by the sensor service in
 *    this mode.
 *
 * @return OK on success
 *  BAD_VALUE if requested mode is not supported
 *  PERMISSION_DENIED if operation is not allowed
 */
Return<Result> setOperationMode(OperationMode mode) override;

/**
 * Activate/de-activate one sensor.
 *
 * After sensor de-activation, existing sensor events that have not
 * been picked up by poll() must be abandoned immediately so that
 * subsequent activation will not get stale sensor events (events
 * that are generated prior to the latter activation).
 *
 * @param  sensorHandle is the handle of the sensor to change.
 * @param  enabled set to true to enable, or false to disable the sensor.
 *
 * @return result OK on success, BAD_VALUE if sensorHandle is invalid.
 */
Return<Result> activate(int32_t sensorHandle, bool enabled) override;

/**
 * Initialize the Sensors HAL's Fast Message Queues (FMQ) and callback.
 *
 * The Fast Message Queues (FMQ) that are used to send data between the
 * framework and the HAL. The callback is used by the HAL to notify the
 * framework of asynchronous events, such as a dynamic sensor connection.

* initialize must be thread safe and prevent concurrent calls
* to initialize from simultaneously modifying state.
*
* @param eventQueueDescriptor Fast Message Queue descriptor that is used to
*	  create the Event FMQ which is where sensor events are written. The
*   descriptor is obtained from the framework's FMQ that is used to read
*   sensor events.
* @param wakeLockDescriptor Fast Message Queue descriptor that is used to
*   create the Wake Lock FMQ which is where wake_lock events are read
*   from. The descriptor is obtained from the framework's FMQ that is
*   used to write wake_lock events.
* @param sensorsCallback sensors callback that receives asynchronous data
*   from the Sensors HAL.
* @return result OK on success; BAD_VALUE if descriptor is invalid (such
*   as null)
*/
Return<Result> initialize(
    const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
    const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
    const sp<ISensorsCallback>& sensorsCallback) override;

/**
 * Sets a sensorâs parameters, including sampling frequency and maximum
 * report latency. This function can be called while the sensor is
 * activated, in which case it must not cause any sensor measurements to
 * be lost: transitioning from one sampling rate to the other cannot cause
 * lost events, nor can transitioning from a high maximum report latency to
 * a low maximum report latency.
 *
 * @param sensorHandle handle of sensor to be changed.
 * @param samplingPeriodNs specifies sensor sample period in nanoseconds.
 * @param maxReportLatencyNs allowed delay time before an event is sampled
 *     to time of report.
 * @return result OK on success, BAD_VALUE if any parameters are invalid.
 */

Return<Result> batch(int32_t sensorHandle, int64_t samplingPeriodNs,
                     int64_t maxReportLatencyNs) override;

/**
 * Trigger a flush of internal FIFO.
 *
 * Flush adds a FLUSH_COMPLETE metadata event to the end of the "batch mode"
 * FIFO for the specified sensor and flushes the FIFO.	If the FIFO is empty
 * or if the sensor doesn't support batching (FIFO size zero), return
 * SUCCESS and add a trivial FLUSH_COMPLETE event added to the event stream.
 * This applies to all sensors other than one-shot sensors. If the sensor
 * is a one-shot sensor, flush must return BAD_VALUE and not generate any
 * flush complete metadata.  If the sensor is not active at the time flush()
 * is called, flush() return BAD_VALUE.
 *
 * @param sensorHandle handle of sensor to be flushed.
 * @return result OK on success and BAD_VALUE if sensorHandle is invalid.
 */
Return<Result> flush(int32_t sensorHandle) override;

/**
 * Inject a single sensor event or push operation environment parameters to
 * device.
 *
 * When device is in NORMAL mode, this function is called to push operation
 * environment data to device. In this operation, Event is always of
 * SensorType::AdditionalInfo type. See operation evironment parameters
 * section in AdditionalInfoType.
 *
 * When device is in DATA_INJECTION mode, this function is also used for
 * injecting sensor events.
 *
 * Regardless of OperationMode, injected SensorType::ADDITIONAL_INFO
 * type events should not be routed back to the sensor event queue.
 *
 * @see AdditionalInfoType
 * @see OperationMode
 * @param event sensor event to be injected
 * @return result OK on success; PERMISSION_DENIED if operation is not
 *     allowed; INVALID_OPERATION, if this functionality is unsupported;
 *     BAD_VALUE if sensor event cannot be injected.
 */
Return<Result> injectSensorData(const Event& event) override;

/**
 * Register direct report channel.
 *
 * Register a direct channel with supplied shared memory information. Upon
 * return, the sensor hardware is responsible for resetting the memory
 * content to initial value (depending on memory format settings).
 *
 * @param mem shared memory info data structure.
 * @return result OK on success; BAD_VALUE if shared memory information is
 *     not consistent; NO_MEMORY if shared memory cannot be used by sensor
 *     system; INVALID_OPERATION if functionality is not supported.
 * @return channelHandle a positive integer used for referencing registered
 *     direct channel (>0) in configureDirectReport and
 *     unregisterDirectChannel if result is OK, -1 otherwise.
 */
Return<void> registerDirectChannel(const SharedMemInfo& mem,
                                   registerDirectChannel_cb _hidl_cb) override;

/**
 * Unregister direct report channel.
 *
 * Unregister a direct channel previously registered using
 * registerDirectChannel, and remove all active sensor report configured in
 * still active sensor report configured in the direct channel.
 *
 * @param channelHandle handle of direct channel to be unregistered.
 * @return result OK if direct report is supported; INVALID_OPERATION
 *     otherwise.
 */
Return<Result> unregisterDirectChannel(int32_t channelHandle) override;

/**
 * Configure direct sensor event report in direct channel.
 *
 * This function start, modify rate or stop direct report of a sensor in a
 * certain direct channel.
 *
 * @param sensorHandle handle of sensor to be configured. When combined
 *     with STOP rate, sensorHandle can be -1 to denote all active sensors
 *     in the direct channel specified by channel Handle.
 * @param channelHandle handle of direct channel to be configured.
 * @param rate rate level, see RateLevel enum.
 * @return result OK on success; BAD_VALUE if parameter is invalid (such as
 *     rate level is not supported by sensor, channelHandle does not exist,
 *     etc); INVALID_OPERATION if functionality is not supported.
 * @return reportToken positive integer to identify multiple sensors of
 *     the same type in a single direct channel. Ignored if rate is STOP.
 *     See SharedMemFormat.
 */
Return<void> configDirectReport(int32_t sensorHandle, int32_t channelHandle,
                        RateLevel rate,
                        configDirectReport_cb _hidl_cb) override;

private:
    /**
     * sensors hal module handle
     */
    sensors_hal* _hal;

    using EventMessageQueue = MessageQueue<Event, android::hardware::kSynchronizedReadWrite>;
    using WakeLockMessageQueue = MessageQueue<uint32_t, android::hardware::kSynchronizedReadWrite>;
    /**
     * The Event FMQ where sensor events are written
     */
    std::shared_ptr<EventMessageQueue> _mEventQueue;

    /**
     * The Wake Lock FMQ that is read to determine when the framework has handled WAKE_UP events
     */
    std::unique_ptr<WakeLockMessageQueue> _mWakeLockQueue;

    /**
     * Event Flag to signal to the framework when sensor events are available to be read
     */
    EventFlag* _mEventQueueFlag;

    std::atomic<bool> _is_hal_configured;

    /**
     * wakelock fmq status read thread to get
     *  number of wakeup events processed by framework
     */
    static void* wakeLock_fmq_status_read(void *in_params);

    std::thread _fmq_status_th;

    Return<Result> updateFMQDescriptor(
        const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
        const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor);
    void clearFMQDescriptor();

    /*Clear All existing clients when System_server dies*/
    void clearActiveConnections();
    std::vector<int32_t> active_drm_list;

    /* Initialize must be thread safe and prevent concurrent calls
     * to initialize from simultaneously modifying state. */
    std::mutex _mutex;

};
}
}
}
}
}
