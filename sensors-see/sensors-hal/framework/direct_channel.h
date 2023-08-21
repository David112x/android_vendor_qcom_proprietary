/*============================================================================
  @file direct_channel.h

  @brief
  direct_channel class definition.

  Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
============================================================================*/

#ifndef SNS_LE_QCS605
#pragma once


#include <cstdlib>
#include <cutils/native_handle.h>
#include <hardware/gralloc1.h>

#include "sensor.h"
#include "sns_low_lat_stream.h"

typedef int (*direct_channel_stream_init_t)(int fd, unsigned int size, unsigned int offset, int* handle);
typedef int (*direct_channel_stream_deinit_t)(int handle);
typedef int (*direct_channel_stream_config_t)(int handle, int64 timestamp_offset,
        const sns_std_suid_t* sensor_suid, unsigned int sample_period_us,
        unsigned int flags, int sensor_handle);
typedef int (*direct_channel_stream_stop_t)(int handle);
typedef int (*direct_channel_offset_update_t)(int64 timestamp_offset, int64 slope);
/*============================================================================
 * Class direct_channel
 * Direct channel has two work modes: Android mode and VR mode. Android mode is default mode which sensor data in
 * shared memory compliant with Android requirement. VR mode is customize mode which:
 * #1 Event timestamp will be Qtimer (in nanosecond) .
 * #2 Sensor data will come from sensor driver directly and not go through resampler.
 * #3 Data ready interrupt will be triggered to CDSP for each sensor event.
 * Direct channel can only work in one of the modes, there is global switch "qti_vr_mode" in sensor registry to select
 * between these two modes, Android mode is default mode and will be used if registry is not set.
 *=============================================================================*/
class direct_channel {
public:
    /**
     * @brief constructor
     *
     * @param[in] mem_handle handle of share memory
     * @param[in] mem_size size of share memory block
     */

    direct_channel(const struct native_handle *mem_handle,
        const size_t mem_size);
    /**
     * @brief copy constructor
     *
     * @param[in] direct channel object to copy
     */
    direct_channel(const direct_channel &copy);

    /**
     * @brief destructor
     */
    ~direct_channel();

    /**
     * @brief get channel handle on remote side
     *
     * @return int handle
     */
    int get_low_lat_handle();

    /**
     * @brief get channel handle on Android side
     *
     * @return int handle
     */
    int get_client_channel_handle();

    /**
     * @brief get file descriptor of share memory
     *
     * @return int fd
     */
    int get_buffer_fd();

    /**
     * @brief check if memory buffer is already register to current channel object
     *
     * @param[in] handle of memory block to be validated
     *
     * @return bool true if this block of memory is already registered, false otherwise
     */
    bool is_same_memory(const struct native_handle *mem_handle);

    /**
     * @brief configure sensor in particular direct report channel.
     *
     * @param[in] handle Handle of direct report channel
     * @param[in] timestamp_offset Timestamp offset between AP and SSC.
     * @param[in] sensor_suid Sensor to configure
     * @param[in] sample_period_us Sensor sampling period, set this to zero will stop the sensor.
     * @param[in] flags Configure flag of sensor refer to "SNS_LLCM_FLAG_X"
     * @param[in] sensor_handle Sensor identifier used in Android.
     *
     * @return int 0 on success, negative on failure
     */
    int config_channel(int handle, int64 timestamp_offset,
        const sns_std_suid_t* sensor_suid, unsigned int sample_period_us,
        unsigned int flags, int sensor_handle);

    /**
     * @brief Disable all sensors in particular channel.
     *
     * @param[in] handle Handle of direct report channel
     *
     * @return int 0 on success, negative on failure
     */
    int stop_channel(int handle);

    /**
     * @brief Update the timestamp offset between AP and SSC
     *
     * @param[in] new_offset Timestamp offset
     */
    void update_offset(int64_t new_offset);

private:
    struct native_handle* mem_native_handle;
    void* buffer_ptr;
    size_t buffer_size;
    int sns_low_lat_handle;

    /* FastRPC symbol pointers */
    void *llcmhandler = NULL;
    direct_channel_stream_init_t direct_channel_stream_init;
    direct_channel_stream_deinit_t direct_channel_stream_deinit;
    direct_channel_stream_config_t direct_channel_stream_config;
    direct_channel_stream_stop_t   direct_channel_stream_stop;
    direct_channel_offset_update_t direct_channel_offset_update;

    /* Assigned by the constructor. If this field is 0, then the object is invalid */
    int client_channel_handle;

    /* Note: valid range for Android channel handles is (>0, <INT32_MAX) */
    static int client_channel_handle_counter;

    /* Gralloc1 device singleton */
    static gralloc1_device_t* gralloc1_device;
    static GRALLOC1_PFN_RETAIN gralloc1_retain;
    static GRALLOC1_PFN_RELEASE gralloc1_release;
    static GRALLOC1_PFN_LOCK gralloc1_lock;
    static GRALLOC1_PFN_UNLOCK gralloc1_unlock;
    static GRALLOC1_PFN_GET_BACKING_STORE gralloc1_GetBackingStore;

    /**
     * @brief Load gralloc1 function symbols
     *
     * @return int 0 on success, negative on failure
     */
    int init_gralloc1_dev(void);

    /**
     * @brief Load direct channel fasrRPC remote symbols
     *
     * @return int 0 on success, negative on failure
     */
    int init_fastRPC_symbols(void);
};
#endif //SNS_LE_QCS605
