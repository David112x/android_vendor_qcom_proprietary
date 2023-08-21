/*============================================================================
  @file direct_channel.cpp

  @brief
  direct_channel class implementation.

  Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
============================================================================*/

#include "direct_channel.h"
#include <string>
#include <cutils/native_handle.h>
#include <hardware/hardware.h>
#include <hardware/gralloc1.h>
#include <dlfcn.h>

#include "sensors_hal.h"
#include "sensors_timeutil.h"
#include "rpcmem.h"

/* Initialize the android_handle_counter */
int direct_channel::client_channel_handle_counter = 0;

/* Initialize the gralloc1 device and functions */
gralloc1_device_t* direct_channel::gralloc1_device = nullptr;
GRALLOC1_PFN_RETAIN direct_channel::gralloc1_retain = nullptr;
GRALLOC1_PFN_RELEASE direct_channel::gralloc1_release = nullptr;
GRALLOC1_PFN_LOCK direct_channel::gralloc1_lock = nullptr;
GRALLOC1_PFN_UNLOCK direct_channel::gralloc1_unlock = nullptr;
GRALLOC1_PFN_GET_BACKING_STORE direct_channel::gralloc1_GetBackingStore = nullptr;

#define SNS_LOW_LAT_STUB_NAME "libsns_low_lat_stream_stub.so"
extern "C" {
 void register_buf(void* buf, int size, int fd, int attr);
}
direct_channel::direct_channel(const struct native_handle *mem_handle,
    const size_t mem_size)
    :sns_low_lat_handle(-1),
     client_channel_handle(0)
{
    int ret = 0;
    /* Load fastRPC symbols, always load symbols for each inctance to restore from SSR */
    llcmhandler = NULL;
    if (init_fastRPC_symbols()) {
      sns_loge("%s: init fastRPC symbols failed!", __FUNCTION__);
      return;
    }

    /* If the gralloc1 module singleton hasn't been started, bring it up */
    if (gralloc1_device == NULL) {
      if (init_gralloc1_dev()) {
        /* Kick out if gralloc1_device initialization fails */
        sns_loge("%s: initGralloc1Dev failed!", __FUNCTION__);
        return;
      }
    }

    /* Check the native_handle for validity */
    if (mem_handle->numFds < 1) {
      sns_loge("%s: Unexpected numFds. Expected at least 1. Received %d.", __FUNCTION__,
          mem_handle->numFds);
      return;
    }


    /* Copy the native_handle */
    mem_native_handle = native_handle_clone(mem_handle);
    if(NULL == mem_native_handle){
        sns_loge("%s: mem_native_handle is NULL", __FUNCTION__);
        return;
    }

    sns_logd("(mem_handle/):hnd %p (cloned handle/):local_hnd %p",
               (void *)mem_handle, mem_native_handle);

    buffer_size = mem_size;

    /* Register the buffer */
    ret = gralloc1_retain(gralloc1_device, mem_native_handle);
    if (ret != 0) {
      native_handle_close(mem_native_handle);
      mem_native_handle = NULL;
      sns_loge("%s: gralloc1_retain FAILED ret %d", __FUNCTION__, ret);
      return;
    }

    /* Lock the shared memory to get the virtual address */
    gralloc1_rect_t accessRegion = {
        .left = 0,
        .top = 0,
        .width = static_cast<int32_t>(buffer_size),
        .height = 1
    };
    ret = gralloc1_lock(gralloc1_device, mem_native_handle,
        GRALLOC1_PRODUCER_USAGE_SENSOR_DIRECT_DATA,
        GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN, &accessRegion, &buffer_ptr, -1);
    if (ret != 0) {
      gralloc1_release(gralloc1_device, mem_native_handle);
      native_handle_close(mem_native_handle);
      mem_native_handle = NULL;
      sns_loge("%s: gralloc1_lock FAILED ret %d", __FUNCTION__, ret);
      return;
    }

    /* Increment the Android handle counter and check it for validity. Then
        assign the resulting Android handle to this Direct Channel */
    ++client_channel_handle_counter;
    if (client_channel_handle_counter <= 0) {
      client_channel_handle_counter = 1;
    }
    client_channel_handle = client_channel_handle_counter;

    /* Clear out the buffer */
    memset(buffer_ptr, 0, buffer_size);

    int buf_size = (int)buffer_size;

    /* Map the shared memory to a fastRPC addressable file descriptor */
    register_buf(buffer_ptr, buf_size, this->get_buffer_fd(), 0);

    /* Initialize the buffer with the Low Latency library */
    ret = direct_channel_stream_init(this->get_buffer_fd(), buffer_size, 0, &sns_low_lat_handle);

    if (ret != 0) {
      sns_loge("%s: sns_low_lat_stream_init failed! ret %d", __FUNCTION__, ret);
      sns_low_lat_handle = -1;
      gralloc1_release(gralloc1_device, mem_native_handle);
      native_handle_close(mem_native_handle);
      mem_native_handle = NULL;
      client_channel_handle = 0;
    }

    sns_logv("%s: complete ret=%d", __FUNCTION__, ret);
    return;
}

/* Copy constructor */
direct_channel::direct_channel(const direct_channel &copy)
{
    mem_native_handle = native_handle_clone(copy.mem_native_handle);
    buffer_ptr = copy.buffer_ptr;
    buffer_size = copy.buffer_size;
    sns_low_lat_handle = copy.sns_low_lat_handle;
    client_channel_handle = copy.client_channel_handle;
}

direct_channel::~direct_channel()
{
    int ret = 0;
    int32_t dummy_var; /* Not really used */

    sns_logd("%s: (cloned handle /):local_hnd %p", __FUNCTION__, mem_native_handle);

    /* Make sure the object is valid by checking the android_handle */
    if (client_channel_handle != 0) {
      ret = direct_channel_stream_deinit(sns_low_lat_handle);
    } else {
      sns_loge("%s: deinit FAILED. direct_channel object is in a bad state.", __FUNCTION__);
    }
    if((gralloc1_device != NULL) && (mem_native_handle != NULL)) {
      ret = gralloc1_unlock(gralloc1_device, mem_native_handle, &dummy_var);
      if (0 != ret)
          sns_loge("gralloc1_unlock failed ret:%d", ret);

      ret = gralloc1_release(gralloc1_device, mem_native_handle);
      if (0 != ret)
          sns_loge("gralloc1_release failed ret:%d", ret);

      mem_native_handle = NULL;
    }

    sns_logi("%s: unloading stub(%s) llcmhandler=%p", __FUNCTION__, SNS_LOW_LAT_STUB_NAME, llcmhandler);
    ret = dlclose(llcmhandler);
    if(0 != ret) {
        sns_loge("%s: dlclose failed ret: %d", __FUNCTION__, ret);
    }

    sns_logv("%s: complete ret=%d", __FUNCTION__, ret);
}

int direct_channel::config_channel(int handle, int64 timestamp_offset, const sns_std_suid_t* sensor_suid, unsigned int sample_period_us, unsigned int flags, int sensor_handle)
{
   sns_logv("%s: ", __FUNCTION__);
   return direct_channel_stream_config(handle, timestamp_offset, sensor_suid, sample_period_us, flags, sensor_handle);
}

int direct_channel::stop_channel(int handle)
{
   sns_logv("%s: ", __FUNCTION__);
   return direct_channel_stream_stop(handle);
}

int direct_channel::get_low_lat_handle()
{
    return sns_low_lat_handle;
}

int direct_channel::get_client_channel_handle()
{
    return client_channel_handle;
}

int direct_channel::get_buffer_fd()
{
    if (mem_native_handle) {
        return mem_native_handle->data[0];
    } else {
        return -1;
    }
}

bool direct_channel::is_same_memory(const struct native_handle *mem_handle)
{
  gralloc1_backing_store_t s1, s2;

  if ((gralloc1_GetBackingStore(gralloc1_device, mem_native_handle, &s1) != GRALLOC1_ERROR_NONE) ||
    (gralloc1_GetBackingStore(gralloc1_device, mem_handle, &s2) != GRALLOC1_ERROR_NONE)) {
    sns_loge("%s: get backing store FAILED, h1=%p, h2=%p", __FUNCTION__,
        mem_native_handle, mem_handle);
    return false;
  }

  sns_logv("Get backing store, s1=%p, s2=%p", (void *)s1, (void *)s2);

  return (s1 == s2);
}

int direct_channel::init_gralloc1_dev(void)
{
  int ret = 0;

  const hw_module_t* hw_mod = NULL;
  ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_mod);
  if (ret != 0) {
    sns_loge("%s: hw_get_module FAILED ret %d", __FUNCTION__, ret);
    return -1;
  }

  /* Check the gralloc API version for gralloc1 */
  if (((hw_mod->module_api_version >> 8) & 0xFF) != 1) {
    sns_loge("%s: unsupported gralloc version 0x%X", __FUNCTION__,
        hw_mod->module_api_version);
    return -2;
  } else {
    /* Open the gralloc1 module */
    ret = gralloc1_open(hw_mod, &gralloc1_device);
    if (ret != 0) {
      sns_loge("%s: gralloc1_open FAILED ret %d", __FUNCTION__, ret);
      return -3;
    } else {
      /* Get the necessary function pointers */
      gralloc1_retain = (GRALLOC1_PFN_RETAIN)gralloc1_device->getFunction(gralloc1_device, GRALLOC1_FUNCTION_RETAIN);
      gralloc1_release = (GRALLOC1_PFN_RELEASE)gralloc1_device->getFunction(gralloc1_device, GRALLOC1_FUNCTION_RELEASE);
      gralloc1_lock = (GRALLOC1_PFN_LOCK)gralloc1_device->getFunction(gralloc1_device, GRALLOC1_FUNCTION_LOCK);
      gralloc1_unlock = (GRALLOC1_PFN_UNLOCK)gralloc1_device->getFunction(gralloc1_device, GRALLOC1_FUNCTION_UNLOCK);
      gralloc1_GetBackingStore = (GRALLOC1_PFN_GET_BACKING_STORE)(gralloc1_device->getFunction(gralloc1_device,GRALLOC1_FUNCTION_GET_BACKING_STORE));

      if (gralloc1_retain == NULL || gralloc1_release == NULL
        || gralloc1_lock == NULL || gralloc1_unlock == NULL
        || gralloc1_GetBackingStore == NULL)
      {
        sns_loge("Fail to get gralloc1 function pointer: retain=%p, release=%p, lock=%p, unlock=%p, getBackingStore=%p",
          gralloc1_retain, gralloc1_release, gralloc1_lock, gralloc1_unlock, gralloc1_GetBackingStore);
        return -4;
      }
    }
  }

  return 0;
}

int direct_channel::init_fastRPC_symbols(void)
{
  sns_logi("%s: loading stub(%s) llcmhandler=%p", __FUNCTION__, SNS_LOW_LAT_STUB_NAME, llcmhandler);
  if(NULL == llcmhandler)  {
      if(NULL == (llcmhandler = dlopen(SNS_LOW_LAT_STUB_NAME, RTLD_NOW))) {
        sns_loge("%s: load stub(%s) failed !", __FUNCTION__, SNS_LOW_LAT_STUB_NAME);
        return -EINVAL;
      }
  }
  direct_channel_stream_init = (direct_channel_stream_init_t)dlsym(llcmhandler, "sns_low_lat_stream_init");
  direct_channel_stream_deinit =(direct_channel_stream_deinit_t)dlsym(llcmhandler, "sns_low_lat_stream_deinit");
  direct_channel_stream_stop = (direct_channel_stream_stop_t)dlsym(llcmhandler, "sns_low_lat_stream_stop");
  direct_channel_stream_config = (direct_channel_stream_config_t)dlsym(llcmhandler, "sns_low_lat_stream_config");
  direct_channel_offset_update = (direct_channel_offset_update_t)dlsym(llcmhandler, "sns_low_lat_stream_offset_update");

  if (direct_channel_stream_init == NULL || direct_channel_stream_deinit == NULL
        || direct_channel_stream_stop == NULL || direct_channel_stream_config == NULL) {
    sns_loge("Init fastRPC symbol failed: init=%p, deinit=%p, stop=%p, config=%p",
          direct_channel_stream_init,
          direct_channel_stream_deinit,
          direct_channel_stream_stop,
          direct_channel_stream_config);

    dlclose(llcmhandler);
    return -EINVAL;
  }

  if (direct_channel_offset_update == NULL) {
    sns_loge("direct_channel_offset_update=NULL! Continue with timestamp offset update disabled.");
  }

  sns_logi("%s: all fastRPC symbols load success.", __FUNCTION__);
  return 0;
}

void direct_channel::update_offset(int64_t new_offset)
{
  direct_channel_offset_update(new_offset, 0);
}
