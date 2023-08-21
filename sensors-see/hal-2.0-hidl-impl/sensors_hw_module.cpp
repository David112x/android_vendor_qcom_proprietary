/*-------------------------------------------------------------------
Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

* Not a Contribution.

*Copyright (C) 2016 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
--------------------------------------------------------------------*/

#include <errno.h>
#include <hardware/sensors.h>
#include <math.h>
#include <sys/types.h>
#include <stdint.h>
#include "sensors_hw_module.h"
#include "wakelock_utils.h"

using Result = ::android::hardware::sensors::V1_0::Result;
using RateLevel = ::android::hardware::sensors::V1_0::RateLevel;
using SharedMemInfo = ::android::hardware::sensors::V1_0::SharedMemInfo;
using SharedMemType = ::android::hardware::sensors::V1_0::SharedMemType;
using SharedMemFormat = ::android::hardware::sensors::V1_0::SharedMemFormat;
using SensorType = ::android::hardware::sensors::V1_0::SensorType;
using SensorInfo = ::android::hardware::sensors::V1_0::SensorInfo;

#define SEC_TO_NS_CONVERSION 1000000000

/*implementation of HAL 2.0 APIs*/
namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

/*helper functions*/
static Result ResultFromStatus(android::status_t err) {
    switch (err) {
        case android::OK:
            return Result::OK;
        case android::PERMISSION_DENIED:
            return Result::PERMISSION_DENIED;
        case android::NO_MEMORY:
            return Result::NO_MEMORY;
        case android::BAD_VALUE:
            return Result::BAD_VALUE;
        default:
            return Result::INVALID_OPERATION;
    }
}

bool convertFromSharedMemInfo(const SharedMemInfo& memIn, sensors_direct_mem_t *memOut) {
    if (memOut == nullptr) {
        return false;
    }

    switch(memIn.type) {
        case SharedMemType::ASHMEM:
            memOut->type = SENSOR_DIRECT_MEM_TYPE_ASHMEM;
            break;
        case SharedMemType::GRALLOC:
            memOut->type = SENSOR_DIRECT_MEM_TYPE_GRALLOC;
            break;
        default:
            return false;
    }

    switch(memIn.format) {
        case SharedMemFormat::SENSORS_EVENT:
            memOut->format = SENSOR_DIRECT_FMT_SENSORS_EVENT;
            break;
        default:
            return false;
    }

    if (memIn.memoryHandle == nullptr) {
        return false;
    }

    memOut->size = memIn.size;
    memOut->handle = memIn.memoryHandle;
    return true;
}

int convertFromRateLevel(RateLevel rate) {
    switch(rate) {
        case RateLevel::STOP:
            return SENSOR_DIRECT_RATE_STOP;
        case RateLevel::NORMAL:
            return SENSOR_DIRECT_RATE_NORMAL;
        case RateLevel::FAST:
            return SENSOR_DIRECT_RATE_FAST;
        case RateLevel::VERY_FAST:
            return SENSOR_DIRECT_RATE_VERY_FAST;
        default:
            return -1;
    }
}

void convertFromSensor(const sensor_t &src, SensorInfo *dst) {
    if (nullptr != dst) {
        dst->name = src.name;
        dst->vendor = src.vendor;
        dst->version = src.version;
        dst->sensorHandle = src.handle;
        dst->type = (SensorType)src.type;
        dst->maxRange = src.maxRange;
        dst->resolution = src.resolution;
        dst->power = src.power;
        dst->minDelay = src.minDelay;
        dst->fifoReservedEventCount = src.fifoReservedEventCount;
        dst->fifoMaxEventCount = src.fifoMaxEventCount;
        dst->typeAsString = src.stringType;
        dst->requiredPermission = src.requiredPermission;
        dst->maxDelay = src.maxDelay;
        dst->flags = src.flags;
    }
}


/*implementation start of hal 2.0 class methods */
sensors_hw_module::sensors_hw_module()
{
    _hal = sensors_hal::get_instance();
    _is_hal_configured = false;
}
sensors_hw_module::~sensors_hw_module()
{
}

void* sensors_hw_module::wakeLock_fmq_status_read(void *in_params)
{
  sensors_hw_module* self = (sensors_hw_module*)in_params;
  sns_wakelock* wakeLock_obj = sns_wakelock::get_instance();

  if(nullptr != wakeLock_obj && nullptr != self) {
    while(1) {
      if(false == wakeLock_obj->get_status()) {
        wakeLock_obj->wait_for_held();
      }
      bool is_hal_configured = self->_is_hal_configured;
      if(false == is_hal_configured) {
        wakeLock_obj->put_all();
        sns_logi("system_server restarted");
        continue;
      }

      unsigned int events_available = self->_mWakeLockQueue->availableToRead();
      unsigned int processed_wakeup_count = 0;
      if(0 == events_available) {
        events_available = 1;
      }
      std::vector<uint32_t> wakelock_fmq_data(events_available);
      self->_mWakeLockQueue->readBlocking(wakelock_fmq_data.data(),
          events_available,
          0 /* readNotification */,
          static_cast<uint32_t>(WakeLockQueueFlagBits::DATA_WRITTEN |
                                hal_EventQueueFlagBits::WAKELOCK_INTERNAL_WAKE),
          SEC_TO_NS_CONVERSION);
      is_hal_configured = self->_is_hal_configured;
      if(false == is_hal_configured) {
        wakeLock_obj->put_all();
        sns_logi("system_server restarted");
        continue;
      }
      for(unsigned int i = 0 ; i < wakelock_fmq_data.size() ; i++)
        processed_wakeup_count += wakelock_fmq_data[i];
      sns_logd("Release %d locks" , (int)processed_wakeup_count);
      wakeLock_obj->put_n_locks(processed_wakeup_count);
    }
  } else {
    sns_loge("Not able to get wakelock instance ");
  }

  return NULL;
}

Return<Result> sensors_hw_module::updateFMQDescriptor(
    const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
    const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor)
{

  _mEventQueue = std::make_shared<EventMessageQueue>(eventQueueDescriptor, true /* resetPointers */);
  if(nullptr == _mEventQueue){
    sns_loge("Invalid FMQ descriptor - _mEventQueue");
    return Result::BAD_VALUE;
  }

  if (EventFlag::createEventFlag(_mEventQueue->getEventFlagWord(), &_mEventQueueFlag) != android::OK) {
      sns_loge("createEventFlag failed ");
      return Result::BAD_VALUE;
  }

  _mWakeLockQueue =
      std::make_unique<WakeLockMessageQueue>(wakeLockDescriptor, true /* resetPointers */);

  if (nullptr == _mWakeLockQueue || nullptr == _mEventQueueFlag) {
      ALOGE("%s _mWakeLockQueue failed ", __func__);
      return Result::BAD_VALUE;
  }
  return Result::OK;
}

void sensors_hw_module::clearFMQDescriptor()
{
  if (_mEventQueueFlag != nullptr) {
    hardware::EventFlag::deleteEventFlag(&_mEventQueueFlag);
    _mEventQueueFlag = nullptr;
  }
  // reset All unique pointers
  if(nullptr != _mEventQueue) {
    _mEventQueue.reset();
    _mEventQueue = nullptr;
  }

  if(nullptr != _mWakeLockQueue) {
    _mWakeLockQueue.reset();
    _mWakeLockQueue = nullptr;
  }
}
void sensors_hw_module::clearActiveConnections()
{
  /*Clear All outstanding wakeup events and release the wakelock*/
  sns_logi(" in clearActiveConnections() ");
  sns_wakelock* wakeLock_obj = sns_wakelock::get_instance();
  if(nullptr != wakeLock_obj)
    wakeLock_obj->put_all();

  /*Deactivate all the sensors for cleanup*/
  sensor_t const *list;
  size_t count = _hal->get_sensors_list(&list);
  for (size_t i = 0; i < count; ++i) {
     sns_logi("handle = %d",(int)list[i].handle );
    _hal->activate((int)list[i].handle, 0 /*de-activate*/);
  }

#ifdef SNS_DIRECT_REPORT_SUPPORT
  /*Deactivate the Active DRM clients*/
  unsigned int active_drm_count = active_drm_list.size();
  for( unsigned int i = 0 ; i < active_drm_count; i++ ) {
    int32_t channelHandle = active_drm_list[i];
    _hal->unregister_direct_channel((int)channelHandle);
  }
  active_drm_list.clear();
#endif

}
/*start of public APIs implementation*/
Return<Result> sensors_hw_module::initialize(
    const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
    const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
    const sp<ISensorsCallback>& sensorsCallback) {
    std::lock_guard<mutex> lk(_mutex);
    Result result = Result::OK;
    bool is_thread_created = false;
    if(true == _is_hal_configured) {
        is_thread_created = true;
        _is_hal_configured = false;
        _hal->update_system_server_status(_is_hal_configured);
        _mEventQueueFlag->wake(static_cast<uint32_t>(hal_EventQueueFlagBits::EVENT_INTERNAL_WAKE));
        _mEventQueueFlag->wake(static_cast<uint32_t>(hal_EventQueueFlagBits::WAKELOCK_INTERNAL_WAKE));
        clearActiveConnections();
        clearFMQDescriptor();
        if(Result::OK != updateFMQDescriptor(eventQueueDescriptor, wakeLockDescriptor)) {
          return Result::BAD_VALUE;
        }
    }

    if(Result::OK != updateFMQDescriptor(eventQueueDescriptor, wakeLockDescriptor)) {
      return Result::BAD_VALUE;
    }

    if(nullptr == _hal) {
      sns_loge("Error in creating the HAL object ");
      return Result::BAD_VALUE;
    }

    _hal->register_EventQueue(_mEventQueue, _mEventQueueFlag);

    if(false == is_thread_created) {
      _fmq_status_th = std::thread(&sensors_hw_module::wakeLock_fmq_status_read, this);
    }
    _is_hal_configured = true ;
    _hal->update_system_server_status(_is_hal_configured);
    return result;
}


Return<Result> sensors_hw_module::activate(int32_t handle, bool en)
{
    return ResultFromStatus(_hal->activate((int)handle,(int)en));
}

Return<Result> sensors_hw_module::batch(int32_t handle,
                     int64_t samplingPeriodNs,
                     int64_t maxReportLatencyNs)
{
    return ResultFromStatus(_hal->batch((int)handle, samplingPeriodNs,
                     maxReportLatencyNs));
}

Return<Result> sensors_hw_module::flush(int32_t handle)
{
    return ResultFromStatus(_hal->flush(handle));
}

Return<Result> sensors_hw_module::injectSensorData(const Event &data)
{
    return Result::INVALID_OPERATION;
}

Return<void> sensors_hw_module::registerDirectChannel(const SharedMemInfo& mem,
                                   registerDirectChannel_cb _hidl_cb)
{
#ifndef SNS_DIRECT_REPORT_SUPPORT
// HAL does not support
    _hidl_cb(Result::INVALID_OPERATION, -1);
    return Void();
#else
    sensors_direct_mem_t m;
    if (!convertFromSharedMemInfo(mem, &m)) {
      _hidl_cb(Result::BAD_VALUE, -1);
      return Void();
    }

    int err = _hal->register_direct_channel( &m);
    if (err < 0) {
        _hidl_cb(ResultFromStatus(err), -1);
    } else {
        int32_t channelHandle = static_cast<int32_t>(err);
        active_drm_list.push_back(channelHandle);
        _hidl_cb(Result::OK, channelHandle);
    }
    return Void();
#endif

}

Return<Result> sensors_hw_module::unregisterDirectChannel(int32_t channelHandle)
{
#ifndef SNS_DIRECT_REPORT_SUPPORT
    // HAL does not support
    return Result::INVALID_OPERATION;
#else
    _hal->unregister_direct_channel((int)channelHandle);
    std::vector<int32_t>::iterator it;
    it = std::find (active_drm_list.begin(), active_drm_list.end(), channelHandle);
    if(it != active_drm_list.end())
      active_drm_list.erase(it);
    return Result::OK;
#endif
}


Return<void> sensors_hw_module::configDirectReport(
                int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
                configDirectReport_cb _hidl_cb)
{
#ifndef SNS_DIRECT_REPORT_SUPPORT
    // HAL does not support
    _hidl_cb(Result::INVALID_OPERATION, -1);
    return Void();
#else
    sensors_direct_cfg_t cfg = {
        .rate_level = convertFromRateLevel(rate)
    };
    if (cfg.rate_level < 0) {
        _hidl_cb(Result::BAD_VALUE, -1);
        return Void();
    }

    int err = _hal->config_direct_report(sensorHandle, channelHandle, &cfg);

    if (rate == RateLevel::STOP) {
        _hidl_cb(ResultFromStatus(err), -1);
    } else {
        _hidl_cb(err > 0 ? Result::OK : ResultFromStatus(err), err);
    }
    return Void();
#endif
}

Return<void> sensors_hw_module::getSensorsList( getSensorsList_cb _hidl_cb)
{
    sensor_t const *list;
    size_t count = _hal->get_sensors_list(&list);
    hidl_vec<SensorInfo> out;
    out.resize(count);

    for (size_t i = 0; i < count; ++i) {
        const sensor_t *src = &list[i];
        SensorInfo *dst = &out[i];
        convertFromSensor(*src, dst);
    }
    _hidl_cb(out);
    return Void();

}

Return<Result> sensors_hw_module::setOperationMode(OperationMode mode)
{
    return Result::BAD_VALUE;
}

}
}
}
}
}

