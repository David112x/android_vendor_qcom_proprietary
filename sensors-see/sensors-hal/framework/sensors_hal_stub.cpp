/*
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#include <functional>
#include <cutils/properties.h>
#include <cassert>
#include "sensors_hal.h"
#include "sensor_factory.h"
#include "ssc_utils.h"
#include "sensors_timeutil.h"

#ifdef SNS_DIRECT_REPORT_SUPPORT
#include"sns_low_lat_stream.h"
#include"sns_low_lat.h"

#endif


using namespace std;

const char SENSORS_HAL_PROP_DEBUG[] = "persist.vendor.debug.sensors.hal";

/* map debug property value to log_level */
static const unordered_map<char, sensors_log::log_level> log_level_map = {
    { '0', sensors_log::SILENT },
    { '1', sensors_log::INFO },
    { 'e', sensors_log::ERROR },
    { 'E', sensors_log::ERROR },
    { 'i', sensors_log::INFO },
    { 'I', sensors_log::INFO },
    { 'd', sensors_log::DEBUG },
    { 'D', sensors_log::DEBUG },
    { 'v', sensors_log::VERBOSE },
    { 'V', sensors_log::VERBOSE },
};

void sensors_hal::get_system_config()
{
    char debug_prop[PROPERTY_VALUE_MAX];
    int len;
    len = property_get(SENSORS_HAL_PROP_DEBUG, debug_prop, "i");
    if (len > 0) {
        if (log_level_map.find(debug_prop[0]) != log_level_map.end()) {
            _sysconfig.log_level = log_level_map.at(debug_prop[0]);
        }
    }
    sns_logi("log_level: %d", _sysconfig.log_level);
}

sensors_hal::sensors_hal()
{
    sensors_log::set_tag("sensors-hal");
    get_system_config();
    sensors_log::set_level(_sysconfig.log_level);
    sns_logi("initializing the stub version of sensors_hal");
}

int sensors_hal::activate(int handle, int enable)
{
    return 0;
}

int sensors_hal::batch(int handle, int flags, int64_t sampling_period_ns,
                       int64_t max_report_latency_ns)
{
    return 0;
}

int sensors_hal::flush(int handle)
{
    return 0;
}

int sensors_hal::poll(sensors_event_t* data, int count)
{
    int num_events = 0;
    /* basically, this function call will be blocked forever
       at _event_queue.pop() because no sensor event will be
       pushed to the queue in this stub version of HAL */
    for (int i = 0; i < count; i++) {
        if (num_events == 0 || _event_queue.size() > 0) {
            data[num_events++] = _event_queue.pop();
        }
    }
    return num_events;
}

void sensors_hal::init_sensors()
{
}

int sensors_hal::get_sensors_list(const sensor_t **s_list)
{
    sns_logi("no sensor is listed");
    *s_list = NULL;
    return 0;
}

#ifdef SNS_DIRECT_REPORT_SUPPORT
int sensors_hal::register_direct_channel(const struct sensors_direct_mem_t* mem)
{
    return -EINVAL;
}

void sensors_hal::unregister_direct_channel(int channel_handle)
{
}

int sensors_hal::config_direct_report(int sensor_handle, int channel_handle,
    const struct sensors_direct_cfg_t *config)
{
   return -EINVAL;
}

std::list<direct_channel*>::const_iterator sensors_hal::find_direct_channel(int channel_handle)
{
    return open_direct_channels.cend();
}

bool sensors_hal::is_used_buffer_fd(int buffer_fd)
{
    return false;
}

bool sensors_hal::is_used_memory(const struct native_handle *mem_handle)
{
    return false;
}

unsigned int sensors_hal::direct_rate_level_to_sample_period(int rate_level)
{
    return 0;
}

void sensors_hal::start_offset_update_timer()
{
}

void sensors_hal::stop_offset_update_timer()
{
}

void sensors_hal::handle_offset_timer()
{
}

void sensors_hal::on_offset_timer_update(union sigval param)
{
}
#endif
