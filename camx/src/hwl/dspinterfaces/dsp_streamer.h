////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE // NOWHINE ENTIRE FILE - Auto Gen by IDL Hexagon sdk


#ifndef DSP_STREAMER_H
#define DSP_STREAMER_H
#include "AEEStdDef.h"
#include "remote.h"
#ifndef __QAIC_HEADER
#define __QAIC_HEADER(ff) ff
#endif // __QAIC_HEADER

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
enum hvx_event_type_t {
   HVX_EVENT_QUERY_CAPS,
   HVX_EVENT_OPEN,
   HVX_EVENT_STATIC_CONFIG,
   HVX_EVENT_DYNAMIC_CONFIG,
   HVX_EVENT_START,
   HVX_EVENT_RESET,
   HVX_EVENT_STOP,
   HVX_EVENT_CLOSE,
   HVX_EVENT_MAX,
   _32BIT_PLACEHOLDER_hvx_event_type_t = 0x7fffffff
};
typedef enum hvx_event_type_t hvx_event_type_t;
enum hvx_IFE_mode_t {
   HVX_IFE_NULL,
   HVX_IFE0,
   HVX_IFE1,
   HVX_IFE_BOTH,
   HVX_IFE_MAX,
   _32BIT_PLACEHOLDER_hvx_IFE_mode_t = 0x7fffffff
};
typedef enum hvx_IFE_mode_t hvx_IFE_mode_t;
enum hvx_vector_mode_t {
   HVX_VECTOR_NULL,
   HVX_VECTOR_128,
   HVX_VECTOR_MAX,
   _32BIT_PLACEHOLDER_hvx_vector_mode_t = 0x7fffffff
};
typedef enum hvx_vector_mode_t hvx_vector_mode_t;
enum hvx_pixel_format_t {
   HVX_BAYER_RGGB,
   HVX_BAYER_BGGR,
   HVX_BAYER_GRBG,
   HVX_BAYER_GBRG,
   HVX_PIXEL_UYVY,
   HVX_PIXEL_VYUY,
   HVX_PIXEL_YUYV,
   HVX_PIXEL_YVYU,
   HVX_PIXEL_MAX,
   _32BIT_PLACEHOLDER_hvx_pixel_format_t = 0x7fffffff
};
typedef enum hvx_pixel_format_t hvx_pixel_format_t;
enum hvx_request_buffer_type_t {
   HVX_BUFFER_STATS,
   HVX_BUFFER_FRAME,
   HVX_BUFFER_MAX,
   _32BIT_PLACEHOLDER_hvx_request_buffer_type_t = 0x7fffffff
};
typedef enum hvx_request_buffer_type_t hvx_request_buffer_type_t;
typedef struct request_buffer_t request_buffer_t;
struct request_buffer_t {
   hvx_request_buffer_type_t type;
   int num_buffers;
   int buffer_size;
   uint64* buffer_addrs;
   int buffer_addrsLen;
   int* buffer_label;
   int buffer_labelLen;
};
enum buf_request_mode_t {
   BUF_REQUEST_STATS_ONLY,
   BUF_REQUEST_FRAME_DUMP_ONLY,
   BUF_REQUEST_BOTH,
   BUF_REQUEST_MODE_MAX,
   _32BIT_PLACEHOLDER_buf_request_mode_t = 0x7fffffff
};
typedef enum buf_request_mode_t buf_request_mode_t;
enum tapping_point_select_t {
   TAPPING_POINT_1ST,
   TAPPING_POINT_2ND,
   TAPPING_POINT_3RD,
   TAPPING_POINT_4TH,
   TAPPING_POINT_MAX,
   _32BIT_PLACEHOLDER_tapping_point_select_t = 0x7fffffff
};
typedef enum tapping_point_select_t tapping_point_select_t;
typedef struct hvx_static_config_t hvx_static_config_t;
struct hvx_static_config_t {
   tapping_point_select_t tapping_point;
   hvx_IFE_mode_t IFE_mode;
   int hvx_unit_no[2];
   int in_width;
   int in_height;
   int image_overlap;
   int right_image_offset;
   int out_width;
   int out_height;
   hvx_pixel_format_t pixel_format;
   int bits_per_pixel;
   unsigned int ife_clk_freq[2];
   int IFE_id;
   hvx_vector_mode_t hvx_vector_mode;
   request_buffer_t req_buf[2];
   buf_request_mode_t buf_request_mode;
};
typedef struct hvx_query_caps_t hvx_query_caps_t;
struct hvx_query_caps_t {
   hvx_vector_mode_t hvx_vector_mode;
   int max_hvx_unit;
};
typedef struct send_buffer_t send_buffer_t;
struct send_buffer_t {
   hvx_request_buffer_type_t type;
   uint64* buffer_addrs;
   int buffer_addrsLen;
   int buffer_size;
   hvx_IFE_mode_t IFE_mode;
   int buffer_label;
};
typedef struct hvx_dynamic_config_t hvx_dynamic_config_t;
struct hvx_dynamic_config_t {
   char* user_data;
   int user_dataLen;
   send_buffer_t buffer0;
   send_buffer_t buffer1;
};
typedef struct hvx_open_t hvx_open_t;
struct hvx_open_t {
   char name[32];
   hvx_IFE_mode_t IFE_mode;
   int dsp_clock;
   int bus_clock;
   int dsp_latency;
};
enum evt_return_type_t {
   HVX_SUCCESS,
   HVX_ERROR_DSP_CLOCK_REQ_FAILURE,
   HVX_ERROR_HVX_POWER_REQ_FAILURE,
   HVX_ERROR_HVX_POWER_REL_FAILURE,
   HVX_ERROR_NO_CONTEXT_CONTROLLER,
   HVX_ERROR_STREAMER_MAPPING_FAILURE,
   HVX_ERROR_L2_LINE_LOCK_FAILURE,
   HVX_ERROR_NOT_ENOUGH_L2_MEM,
   HVX_ERROR_NOT_ENOUGH_STACK_MEM,
   HVX_ERROR_THREAD_CREATE_FAILURE,
   HVX_ERROR_INPUT_STRIDE_NOT_ALIGNED,
   HVX_ERROR_LOAD_CUSTOMER_LIB_FAILURE,
   HVX_ERROR_INVALID_HANDLE,
   HVX_ERROR_INVALID_PAYLOAD,
   HVX_ERROR_BUFFER_ALLOCATION_FAILURE,
   HVX_ERROR_INVALID_BUFFER_PARAMETER,
   HVX_FAILURE,
   _32BIT_PLACEHOLDER_evt_return_type_t = 0x7fffffff
};
typedef enum evt_return_type_t evt_return_type_t;

/**
     * Opens the handle in the specified domain.  If this is the first
     * handle, this creates the session.  Typically this means opening
     * the device, aka open("/dev/adsprpc-smd"), then calling ioctl
     * device APIs to create a PD on the DSP to execute our code in,
     * then asking that PD to dlopen the .so and dlsym the skel function.
     *
     * @param uri, <interface>_URI"&_dom=aDSP"
     *    <interface>_URI is a QAIC generated uri, or
     *    "file:///<sofilename>?<interface>_skel_handle_invoke&_modver=1.0"
     *    If the _dom parameter is not present, _dom=DEFAULT is assumed
     *    but not forwarded.
     *    Reserved uri keys:
     *      [0]: first unamed argument is the skel invoke function
     *      _dom: execution domain name, _dom=mDSP/aDSP/DEFAULT
     *      _modver: module version, _modver=1.0
     *      _*: any other key name starting with an _ is reserved
     *    Unknown uri keys/values are forwarded as is.
     * @param h, resulting handle
     * @retval, 0 on success
     */
__QAIC_HEADER_EXPORT int __QAIC_HEADER(dsp_streamer_open)(const char* uri, remote_handle64* h) __QAIC_HEADER_ATTRIBUTE;

/**
     * Closes a handle.  If this is the last handle to close, the session
     * is closed as well, releasing all the allocated resources.
     * @param h, the handle to close
     * @retval, 0 on success, should always succeed
     */
__QAIC_HEADER_EXPORT int __QAIC_HEADER(dsp_streamer_close)(remote_handle64 h) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT int __QAIC_HEADER(dsp_streamer_event)(remote_handle64 _h, int* session_id, hvx_event_type_t type, const char* data, int dataLen) __QAIC_HEADER_ATTRIBUTE;
#ifndef dsp_streamer_URI
#define dsp_streamer_URI "file:///libdsp_streamer_skel.so?dsp_streamer_skel_handle_invoke&_modver=1.0"
#endif /*dsp_streamer_URI*/
#ifdef __cplusplus
}
#endif
#endif //DSP_STREAMER_H
