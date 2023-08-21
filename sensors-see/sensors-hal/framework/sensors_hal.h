/*
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#pragma once

#include <signal.h>
#include <time.h>
#include <queue>
#include <unordered_map>
#include "sensor.h"
#include "concurrent_queue.h"
#include "sensors_log.h"
#ifdef SNS_DIRECT_REPORT_SUPPORT
#include "direct_channel.h"
#endif

#ifdef SNS_DIRECT_REPORT_SUPPORT
#include<hardware/gralloc.h>
#include<hardware/hardware.h>
#include<list>
#endif

/* global system configuration (set via setprop command)*/
struct sensors_hal_sysconfig
{
    sensors_log::log_level log_level = sensors_log::INFO;
};

/**
 * @brief class to represent entire sensors HAL. This class is
 *        the single entry-point for all HAL API calls.
 *
 */
class sensors_hal
{
public:

    /**
     * @brief returns the singleton instance of the sensors_hal
     *        object
     * @return sensors_hal&  instance
     */
    static sensors_hal& get_instance()
    {
        static sensors_hal hal;
        return hal;
    }

    /**
     * @brief activate a sensor by its handle
     *
     * @param handle
     * @param en
     *     1: activate a sensor, start streaming
     *     0: deactivate a sensor, stop streaming
     *
     * @return int 0 on success, negative on failure
     */
    int activate(int handle, int en);

    /**
     * @brief set timing parameters for a sensor.
     *
     * @param handle
     * @param flags
     * @param sampling_period_ns
     * @param max_report_latency_ns
     *
     * @return int 0 on success, negative on failure
     */
    int batch(int handle, int flags, int64_t sampling_period_ns,
              int64_t max_report_latency_ns);

    /**
     * @brief Asynchronously flushes the hardware FIFO of a sensor
     *        and generates flush_complete event when done.
     *
     * @param handle handle of the sensor to flush
     *
     * @return int 0 on success, negative on failure
     */
    int flush(int handle);

    /**
     * @brief poll for new events
     *
     * This function blocks until at least one event is generated by
     * one of the sensors. Muliple events can be retrieved using a
     * single call to poll().
     *
     * @param data allocated buffer for storing events
     * @param count maximum number of events to receive
     *
     * @return int number of events generated
     */
    int poll(sensors_event_t* data, int count);

    /**
     * @brief get list of available sensors
     *
     * @param s_list populates the list of sensors
     *
     * @return int number of sensors available
     */
    int get_sensors_list(const sensor_t **s_list);

#ifdef SNS_DIRECT_REPORT_SUPPORT
    /**
     * @brief Register direct report channel.
     *
     * Register a channel using supplied shared memory information. By the
     * time this function returns, shared memory content will be initilized
     * to zero.
     * Support gralloc memory (SENSOR_DIRECT_MEM_TYPE_GRALLOC) only.
     *
     * @param mem points to a valid struct sensors_direct_mem_t.
     *
     * @return int A handle of channel (>0, <INT32_MAX) when success,
     *               which later can be referred in unregister or config_direct_report
     *               call, or error code (<0) when failed.
     */
    int register_direct_channel(const struct sensors_direct_mem_t* mem);

    /**
     * @brief Unregister direct report channel.
     *
     * @param channel_handle contains handle of channel to be unregistered
     */
    void unregister_direct_channel(int channel_handle);
    /**
     * @brief  Configure direct sensor event report in direct channel.
     *
     * Start, modify rate or stop direct report of a sensor in a certain direct
     * channel. A special case is setting sensor handle -1 to stop means
     * to stop all active sensor report on the channel specified.
     *
     * @param sensor_handle Sensor to be configured. The sensor has to
     *               support direct report mode by setting flags of sensor_t.
     *               Also, direct report mode is only defined for continuous
     *               reporting mode sensors.
     * @param channel_handle Channel handle to be configured.
     * @param config Direct report parameters, see sensor_direct_cfg_t.
     *
     * @return int when sensor is started or sensor rate level is changed:
     *               return positive identifier of sensor in specified channel
     *               if successful, otherwise return negative error code.
     *               when sensor is stopped: return 0 for success or negative
     *               error code for failure.
     */
    int config_direct_report(int sensor_handle, int channel_handle,
                 const struct sensors_direct_cfg_t * config);
#endif

private:
    /*
     * creates sensors_hal object. When this function returns,
     * a list of available sensors is populated and sensors are
     * ready to be activated
     */
    sensors_hal();

    /*
     * discovers suids of sensors present on SSC and creates sensor
     * objects for all available sensors. These sensor objects are
     * put inside a map container for future retrival using handles.
     */
    void init_sensors();

    /*
     * reads system configuration and stores in _sysconfig
     */
    void get_system_config();

    bool is_rt_thread_enabled();
    bool sensors_restricted();
    uint32_t get_atrace_delay_checktime_ms();

    bool _is_rt_thread_enable;
    uint32_t _atrace_delay_checktime_ms = 0;
    /* global system configuration */
    sensors_hal_sysconfig _sysconfig;

    /* list of registered hal sensors */
    std::vector<sensor_t> _hal_sensors;
    /* map from sensor handle to sensor object pointers */
    std::unordered_map<int, std::unique_ptr<sensor>> _sensors;
    /* event queue for all sensor events */
    concurrent_queue<sensors_event_t> _event_queue;

#ifdef SNS_DIRECT_REPORT_SUPPORT
    std::list<direct_channel*> open_direct_channels;
    pthread_mutex_t open_direct_channels_mutex;
    timer_t _offset_update_timer;

    /**
     * @brief Search through the list of direct channels
     *
     * @param channel_handle the direct channel handle to be found
     *
     * @return iterator point to matching direct channel pointer if found,
     *             iterator point to end of the direct channels list if NOT found
     */
    std::list<direct_channel*>::const_iterator find_direct_channel(int channel_handle);

    /**
     * @brief Search through the list of direct channels to see if the fd is in use
     *
     * @param buffer_fd the buffer fd to be found
     *
     * @return true if the buffer fd is found within the list of direct channels,
     *             false otherwise
     */
    bool is_used_buffer_fd(int buffer_fd);

    /**
     * @brief Search through the list of direct channels to see if the memory is in use
     *
     * @param native handle pointer.
     *
     * @return true if the memory is using by any other direct channels,
     *         false otherwise
     */

    bool is_used_memory(const struct native_handle *mem_handle);
    /**
     * @brief Convert the SENSOR_DIRECT_RATE_* level to an equivalent sample period (in us)
     *
     * @param rate_level the rate level (enum SENSOR_DIRECT_RATE_*)
     *
     * @return sample period in us, 0 if no available sample period
     */
    unsigned int direct_rate_level_to_sample_period(int rate_level);

    /**
     * Starts a timer to update the AP/SLPI timestamp offset on all open direct
     * channels to adjust for drift.
     */
    void start_offset_update_timer();

    /**
     * Stops the AP/SLPI timestamp offset update timer.
     */
    void stop_offset_update_timer();

    /**
     * Invoked in timer thread when AP/SLPI timestamp offset update timer fires.
     */
    void handle_offset_timer();
    static void on_offset_timer_update(union sigval param);
#endif
};
