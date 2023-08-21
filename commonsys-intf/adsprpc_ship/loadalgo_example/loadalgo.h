/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc
 */

#ifndef _LOADALGO_H
#define _LOADALGO_H
/// @file loadalgo.idl
///
#include <AEEStdDef.h>
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
#if !defined(__QAIC_DMAHANDLE1_OBJECT_DEFINED__) && !defined(__DMAHANDLE1_OBJECT__)
#define __QAIC_DMAHANDLE1_OBJECT_DEFINED__
#define __DMAHANDLE1_OBJECT__
typedef struct _dmahandle1_s {
   int fd;
   uint32 offset;
   uint32 len;
} _dmahandle1_t;
#endif /* __QAIC_DMAHANDLE1_OBJECT_DEFINED__ */
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
__QAIC_HEADER_EXPORT int __QAIC_HEADER(loadalgo_open)(const char* uri, remote_handle64* h) __QAIC_HEADER_ATTRIBUTE;
/**
    * Closes a handle.  If this is the last handle to close, the session
    * is closed as well, releasing all the allocated resources.

    * @param h, the handle to close
    * @retval, 0 on success, should always succeed
    */
__QAIC_HEADER_EXPORT int __QAIC_HEADER(loadalgo_close)(remote_handle64 h) __QAIC_HEADER_ATTRIBUTE;
/**
    * Remote call that initializes proxyPD on QDSP.
    * @param _h, Handle associated with the interface for a given domain.
    * @retval, 0 on success.
    */
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(loadalgo_init)(remote_handle64 _h) __QAIC_HEADER_ATTRIBUTE;
/**
    * Remote call that maps a given buffer in securePD on QDSP.
    * @param _h, Handle associated with the interface for a given domain.
    * @param din, File descriptor of the secure buffer.
    * @param dinOffset, Offest for the secure buffer, if any.
    * @param dinLen, Size of the memory to mapped in securePD.
    * @param type, Type of buffer to mapped in securePD.
    * @retval, 0 on success.
    */
__QAIC_HEADER_EXPORT int __QAIC_HEADER(loadalgo_physbuffer)(remote_handle64 _h, int din, uint32 dinOffset, uint32 dinLen, int type) __QAIC_HEADER_ATTRIBUTE;
/**
    * Remote call that runs gaussian algorithm in securePD on QDSP.
    * @param _h, Handle associated with the interface for a given domain.
    * @param src, Secure input image buffer.
    * @param srcOffset, Source secure buffer offest, if any.
    * @param srcLen, Length of the input secure buffer.
    * @param srcWidth, Input width in number of pixels.
    * @param srcHeight, Input height in number of lines.
    * @param srcStride, Stride of input data (i.e., number of bytes between column 0 of row 0 and
                        column 0 of row 1).
    * @param dst, Secure output buffer.
    * @param dstOffset, Destination secure buffer offest, if any.
    * @param dstLen, Length of the output secure buffer.
    * @param dstStride, Stride of output buffer.
    * @param  heap, Heap buffer allocated for securePD.
    * @param heapOffset, Heap buffer offset.
    * @param heapLen, Heap buffer size.
    * @param modeStatic, Mode indicating static vs dynamic
    *                  gaussian algo
    * @retval, 0 on success.
    */
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(loadalgo_Gaussian7x7u8)(remote_handle64 _h, int src, uint32 srcOffset, uint32 srcLen, uint32 srcWidth, uint32 srcHeight, uint32 srcStride, int dst, uint32 dstOffset, uint32 dstLen, uint32 dstStride, int heap, uint32 heapOffset, uint32 heapLen, uint32 modeStatic) __QAIC_HEADER_ATTRIBUTE;
/**
    * Remote call that deinitializes proxyPD on QDSP.
    * @param _h, Handle associated with the interface for a given domain.
    * @retval, 0 on success.
    */
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(loadalgo_deinit)(remote_handle64 _h) __QAIC_HEADER_ATTRIBUTE;
#ifndef loadalgo_URI
#define loadalgo_URI "file:///libloadalgo_skel.so?loadalgo_skel_handle_invoke&_modver=1.0"
#endif /*loadalgo_URI*/
#ifdef __cplusplus
}
#endif
#endif //_LOADALGO_H
