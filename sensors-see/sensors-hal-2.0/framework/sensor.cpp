/*
 * Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <algorithm>
#include <vector>
#include "sensors_log.h"
#include "sensor.h"
#include "sensors_hal_common.h"
#include <unistd.h>
using namespace std;
using ::android::hardware::sensors::V1_0::SensorStatus;
typedef ::android::hardware::sensors::V1_0::SensorType SensorType;
typedef ::android::hardware::sensors::V1_0::MetaDataEventType MetaDataEventType;
using EventMessageQueue = MessageQueue<Event, android::hardware::kSynchronizedReadWrite>;

std::mutex sensor::_mutex;

static const uint32_t SNS_DIRECT_MIN_VERY_FAST_RATE_HZ = 440U;
static const uint32_t SNS_DIRECT_MIN_FAST_RATE_HZ = 110U;
static const uint32_t SNS_DIRECT_MIN_NORMAL_RATE_HZ = 28U;

#define SEC_PER_HOUR (60*60)
#define MSEC_TO_NS_CONVERSION 1000000
#define FMQ_WRITE_BLOCKING_TIMEOUT_MS 100
static int64_t FMQ_WRITE_BLOCKING_TIMEOUT_NS = FMQ_WRITE_BLOCKING_TIMEOUT_MS*MSEC_TO_NS_CONVERSION;

sensor::sensor(sensor_wakeup_type wakeup_type):_bwakeup(false)
{
    memset(&_sensor_info, 0x00, sizeof(sensor_t));
    set_required_permission("");
    if (wakeup_type == SENSOR_WAKEUP)
      _bwakeup = true;
    _previous_ts = 0;
    _wakelock_inst = sns_wakelock::get_instance();
    _is_deactivate_in_progress = false;
    is_client_alive = true;
    _event_count = 0;
}

void sensor::set_config(sensor_params params)
{
    /* clamp the sample period according to the min and max delays supported by
       sensor.*/
    uint64_t min_delay_ns = _sensor_info.minDelay < 0 ? NSEC_PER_MSEC:
                                _sensor_info.minDelay * NSEC_PER_USEC;
    uint64_t max_delay_ns = _sensor_info.maxDelay < 0 ?
                                SEC_PER_HOUR * NSEC_PER_SEC:
                                _sensor_info.maxDelay * NSEC_PER_USEC;

    sns_logd("sp_requested = %" PRIu64, params.sample_period_ns);
    sns_logd("min_delay=%" PRIu64 "ns max_delay=%" PRIu64 "ns",
             min_delay_ns, max_delay_ns);

    if (params.sample_period_ns < min_delay_ns) {
        params.sample_period_ns = max(min_delay_ns, (uint64_t) NSEC_PER_MSEC);
    } else if (params.sample_period_ns > max_delay_ns) {
        params.sample_period_ns = max_delay_ns;
    }

    sns_logd("sp_set = %" PRIu64 "ns, active = %d",
             params.sample_period_ns, is_active());

    /* if new params are same as existing params, no update required. */
    if (!(params == _params)) {
        _params = params;
        /* if sensor is active, update configuration */
        if (is_active()) {
            update_config();
        }
    }
}

/* set direct channel flags */
void sensor::set_direct_channel_flag(bool en)
{
    if (en && _min_low_lat_delay != 0) {
        _sensor_info.flags |= SENSOR_FLAG_DIRECT_CHANNEL_GRALLOC;
    } else {
        _sensor_info.flags &=
            ~(SENSOR_FLAG_DIRECT_CHANNEL_GRALLOC | SENSOR_DIRECT_MEM_TYPE_ASHMEM);
        return;
    }


    /* Set rate flag according sensor minimum delay */
    if ( (unsigned long long)_min_low_lat_delay <=
            USEC_PER_SEC / SNS_DIRECT_MIN_VERY_FAST_RATE_HZ ) {
        _sensor_info.flags |=
            SENSOR_DIRECT_RATE_VERY_FAST << SENSOR_FLAG_SHIFT_DIRECT_REPORT;
    } else if ((unsigned long long)_min_low_lat_delay <=
            USEC_PER_SEC / SNS_DIRECT_MIN_FAST_RATE_HZ ) {
        _sensor_info.flags |=
            SENSOR_DIRECT_RATE_FAST << SENSOR_FLAG_SHIFT_DIRECT_REPORT;
    } else if ((unsigned long long)_min_low_lat_delay <=
            USEC_PER_SEC / SNS_DIRECT_MIN_NORMAL_RATE_HZ ) {
        _sensor_info.flags |=
            SENSOR_DIRECT_RATE_NORMAL << SENSOR_FLAG_SHIFT_DIRECT_REPORT;
    } else {
        _sensor_info.flags &=
            ~(SENSOR_FLAG_DIRECT_CHANNEL_GRALLOC | SENSOR_DIRECT_MEM_TYPE_ASHMEM);
        sns_loge("_min_low_lat_delay=(%d) not meet requirement, direct channel disabled.",
            _min_low_lat_delay);
    }
}

/* function to submit a new event to sensor HAL clients */
void sensor::submit_sensors_hal_event()
{
  if(true == _is_deactivate_in_progress) {
    events.clear();
    return;
  }
  if (events.size()) {
    submit();
  } else {
    sns_logi("%s: trying to push empty Events, count = %zu", get_sensor_info().stringType, _event_count);
  }
}

void sensor::submit()
{
  std::vector<Event>::size_type count = events.size();
  while(count) {
    sns_logd("submitting: (SensorType:%d/count:%d)" , (int)events[0].sensorType,(int)count);
    if( true == _is_deactivate_in_progress || false == is_client_alive) {
      sns_logi("deactivated do not submit to system server (SensorType:%d)" ,
            (int)events[0].sensorType);
      events.clear();
      break;
    }

    if(nullptr != _mEventQueue && nullptr!= _mEventQueueFlag) {
      if( false == _is_deactivate_in_progress) {
        size_t available_to_write = _mEventQueue->availableToWrite();
        if(available_to_write != 0) {
          size_t write_count = available_to_write >= count ? count : available_to_write;
          if(SENSOR_WAKEUP == _bwakeup && nullptr != _wakelock_inst) {
            sns_logd("Acquire %zu locks", write_count);
            _wakelock_inst->get_n_locks(write_count);
          }
          std::unique_lock<std::mutex> lock(_mutex);
          bool written = _mEventQueue->write(events.data(), write_count);
          lock.unlock();
          if (written) {
            _mEventQueueFlag->wake(static_cast<uint32_t>(EventQueueFlagBits::READ_AND_PROCESS));
            events.erase(events.begin(), events.begin()+write_count);
            count = events.size();
          } else {
            if(SENSOR_WAKEUP == _bwakeup && nullptr != _wakelock_inst) {
              sns_logd("Release %zu locks because of writing failure", write_count);
              _wakelock_inst->put_n_locks(write_count);
            }
            sns_logd("fmq write failed ");
          }
        } else {
          uint32_t event_flag_state = 0;
          /* Below are the cases where we need to come out of wait API
           * 1. Wait for EVENTS_READ flag when available_to_write is 0.
           * 2. Wait for internal wake which triggers during deactivate and submit already in wake API.
           * 3. Wait for 100msec to other corner cases along with system_server restart.
           * */
          _mEventQueueFlag->wait((EventQueueFlagBits::EVENTS_READ) |
              hal_EventQueueFlagBits::EVENT_INTERNAL_WAKE,
              &event_flag_state,
              100000000);
        }
      }
    } else {
      sns_logi("system_server restarted ");
      events.clear();
      break;
    }
  }
}
