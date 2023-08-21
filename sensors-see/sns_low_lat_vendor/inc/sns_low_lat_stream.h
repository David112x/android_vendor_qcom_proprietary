/*============================================================================
  @file sns_low_lat_stream.h

  @brief
  API functions, data structures, and constants used by the Low Latency Stream
  Library

  @note
  File line length should generally be limited to <= 80 columns.

  Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ===========================================================================*/

#ifndef _SNS_LOW_LAT_STREAM_H
#define _SNS_LOW_LAT_STREAM_H
#include "AEEStdDef.h"
#include "remote.h"
#ifndef __QAIC_HEADER
#define __QAIC_HEADER(ff) ff
#endif //__QAIC_HEADER

#ifndef __QAIC_HEADER_EXPORT
#define __QAIC_HEADER_EXPORT
#endif // __QAIC_HEADER_EXPORT

#ifndef __QAIC_HEADER_ATTRIBUTE
#define __QAIC_HEADER_ATTRIBUTE
#endif // __QAIC_HEADER_ATTRIBUTE

#ifndef __QAIC_IMPL
#define __QAIC_IMPL(ff) ff
#endif //__QAIC_IMPL

#ifndef __QAIC_IMPL_EXPORT
#define __QAIC_IMPL_EXPORT
#endif // __QAIC_IMPL_EXPORT

#ifndef __QAIC_IMPL_ATTRIBUTE
#define __QAIC_IMPL_ATTRIBUTE
#endif // __QAIC_IMPL_ATTRIBUTE
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sns_std_suid_t sns_std_suid_t;

/*Funtion pointer declarations to the open , close and invoke methods*/
 typedef int (*remote_handle_open_t)(const char* name, remote_handle *ph);
 typedef int (*remote_handle_close_t)(remote_handle h);
 typedef int (*remote_handle_invoke_t)(remote_handle h, uint32_t dwScalars, remote_arg *pra);

/*sns_remote_handles contains handle for open, close and invoke methods*/
typedef struct _sns_remote_handlers
{
   remote_handle_open_t open;
  remote_handle_close_t close;
  remote_handle_invoke_t invoke;
}_sns_remote_handlers;

void sns_init_remote_handles(_sns_remote_handlers handlers);

struct sns_std_suid_t {
   uint64 suid_low;
   uint64 suid_high;
};
__QAIC_HEADER_EXPORT int __QAIC_HEADER(sns_low_lat_stream_init)(int fd, unsigned int size, unsigned int offset, int* handle) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT int __QAIC_HEADER(sns_low_lat_stream_config)(int handle, int64 timestamp_offset, const sns_std_suid_t* sensor_suid, unsigned int sample_period_us, unsigned int flags, int sensor_handle) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT int __QAIC_HEADER(sns_low_lat_stream_stop)(int handle) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT int __QAIC_HEADER(sns_low_lat_stream_deinit)(int handle) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT int __QAIC_HEADER(sns_low_lat_stream_poll)(int handle) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT int __QAIC_HEADER(sns_low_lat_stream_forward_block)(int sleep_in_ms) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT int __QAIC_HEADER(sns_low_lat_stream_offset_update)(int64 timestamp_offset, int64 slope) __QAIC_HEADER_ATTRIBUTE;
#ifdef __cplusplus
}
#endif
#endif //_SNS_LOW_LAT_STREAM_H
