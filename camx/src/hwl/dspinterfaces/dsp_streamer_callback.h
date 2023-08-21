// NOWHINE // NOWHINE ENTIRE FILE - Auto Gen by IDL Hexagon sdk
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _DSP_STREAMER_CALLBACK_H
#define _DSP_STREAMER_CALLBACK_H
#include "AEEStdDef.h"
#include "remote.h"
#include "dsp_streamer.h"
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
enum hvx_cb_error_type_t {
   HVX_OK,
   HVX_ERROR_GENERAL,
   HVX_ERROR_BUS_UNDER_RUN,
   HVX_ERROR_BUS_RCV_OVER_RUN,
   HVX_ERROR_BUS_OVERFLOW,
   _32BIT_PLACEHOLDER_hvx_cb_error_type_t = 0x7fffffff
};
typedef enum hvx_cb_error_type_t hvx_cb_error_type_t;
__QAIC_HEADER_EXPORT int __QAIC_HEADER(dsp_streamer_callback_data)(int handle, int ife_id, int buf_label) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT int __QAIC_HEADER(dsp_streamer_callback_error)(int handle, int ife_id, int frame_id, hvx_cb_error_type_t error, const char* error_msg, int error_msgLen) __QAIC_HEADER_ATTRIBUTE;
#ifdef __cplusplus
}
#endif
#endif //_DSP_STREAMER_CALLBACK_H
