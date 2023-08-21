#ifndef _HCP_RPC_H
#define _HCP_RPC_H
/**
 Copyright (c) 2017 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
**/
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
/******* Defines ******************************/
/******* HCP Interface ******************/
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
__QAIC_HEADER_EXPORT int __QAIC_HEADER(hcp_rpc_open)(const char* uri, remote_handle64* h) __QAIC_HEADER_ATTRIBUTE;
/** 
    * Closes a handle.  If this is the last handle to close, the session
    * is closed as well, releasing all the allocated resources.

    * @param h, the handle to close
    * @retval, 0 on success, should always succeed
    */
__QAIC_HEADER_EXPORT int __QAIC_HEADER(hcp_rpc_close)(remote_handle64 h) __QAIC_HEADER_ATTRIBUTE;
/**
    * @enum     lock_action_t
    * @brief
    */
enum hcp_rpc_lock_action_t {
   HCP_HOST_LOCK,
   HCP_HOST_UNLOCK,
   HCP_HOST_NOOP,
   HCP_HOST_MAX,
   _32BIT_PLACEHOLDER_hcp_rpc_lock_action_t = 0x7fffffff
};
typedef enum hcp_rpc_lock_action_t hcp_rpc_lock_action_t;
/**
     * @struct ts_strings_t
     * @brief Container for timestamp strings
     */
typedef struct hcp_rpc_ts_strings_t hcp_rpc_ts_strings_t;
struct hcp_rpc_ts_strings_t {
   unsigned char* dyn_img_ts;
   int dyn_img_tsLen;
   unsigned char* static_img_ts;
   int static_img_tsLen;
};
/**
     * @struct     raw_buffer_t
     * @brief     Raw buffer type
     */
typedef struct hcp_rpc_raw_buffer_t hcp_rpc_raw_buffer_t;
struct hcp_rpc_raw_buffer_t {
   unsigned char* buf_content;
   int buf_contentLen;
};
/**
    * @struct     pkt_t
    * @brief     Raw packet
    */
typedef struct hcp_rpc_pkt_t hcp_rpc_pkt_t;
struct hcp_rpc_pkt_t {
   unsigned char* content;
   int contentLen;
};
/**
     * @struct     buffer_t
     * @brief     Raw buffer with attributes. .
     */
typedef struct hcp_rpc_buffer_t hcp_rpc_buffer_t;
struct hcp_rpc_buffer_t {
   hcp_rpc_pkt_t attr_pkt;
   hcp_rpc_raw_buffer_t raw_buf;
};
/*!
    * @brief: Obtain host information.
    *
    * @description: Return timestamp strings. Lock host for exclusive use by this client.
    *
    * @return: Error code (0 if successful).
    */
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(hcp_rpc_get_info)(remote_handle64 _h, hcp_rpc_ts_strings_t* timestamps, hcp_rpc_lock_action_t lock_host) __QAIC_HEADER_ATTRIBUTE;
/*!
    * @brief: Open session.
    *
    * @description: Allocate and initialize internal management resources.
    *
    * @out sess_id: Session ID (to be used in future interactions with this session).
    *
    * @return:Error code (0 if successful).
    */
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(hcp_rpc_sess_open)(remote_handle64 _h, int* sess_id) __QAIC_HEADER_ATTRIBUTE;
/*!
    * @brief: Close session.
    *
    * @description: De-allocate internal management resources.
    *
    * @in sess_id: Session ID
    *
    * @return: Error code.
    */
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(hcp_rpc_sess_close)(remote_handle64 _h, int sess_id) __QAIC_HEADER_ATTRIBUTE;
/*!
    * @brief: Send packet.
    *
    * @description: Send raw packet to the Host.
    *
    * @in pkt: pointer to the packet to be sent to the Host. Must be allocated and set by caller.
    *
    * @return: Error code.
    */
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(hcp_rpc_send_pkt)(remote_handle64 _h, const hcp_rpc_pkt_t* pkt) __QAIC_HEADER_ATTRIBUTE;
/*!
    * @brief: Send packet: blocking.
    *
    * @description: Send raw packet to the Host, and receive a response packet.
    *
    * @in pkt_in: pointer to the packet to be sent to the Host. Must be allocated and set by caller.
    *
    * @out pkt_out: pointer to packet beceived from the Host. Must be allocated by caller.
    *
    * @return: Error code.
    */
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(hcp_rpc_send_pkt_blk)(remote_handle64 _h, const hcp_rpc_pkt_t* pkt_in, hcp_rpc_pkt_t* pkt_out) __QAIC_HEADER_ATTRIBUTE;
/*!
    * @brief: Send buffer(s).
    *
    * @description: Send groups of input/output buffers to the Host.
    *
    * @in sess_id: Determines the session to which the contained buffers are addressed.
    *
    * @in in_bufs_attr: Attributes of the input buffers. Must be allocated and set by caller.
    *
    * @in in_bufs: Array of input buffers.
    *
    * @in out_bufs_attr: Attributes of the output buffers. Must be allocated and set by caller.
    *
    * @out out_bufs: Array of output buffers.
    *
    * @return: Error code.
    *
    TODO: in the future, all buffers to reside in one array, and all attributes in another array
    */
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(hcp_rpc_send_bufs)(remote_handle64 _h, int sess_id, const hcp_rpc_pkt_t* in_bufs_attr, int in_bufs_attrLen, const hcp_rpc_raw_buffer_t* in_bufs, int in_bufsLen, const hcp_rpc_pkt_t* out_bufs_attr, int out_bufs_attrLen, hcp_rpc_raw_buffer_t* out_bufs, int out_bufsLen) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(hcp_rpc_send_bufs_dma)(remote_handle64 _h, int sess_id, const hcp_rpc_pkt_t* in_bufs_attr, int in_bufs_attrLen, int in_bufs, uint32 in_bufsOffset, uint32 in_bufsLen, const hcp_rpc_pkt_t* out_bufs_attr, int out_bufs_attrLen, int out_bufs, uint32 out_bufsOffset, uint32 out_bufsLen) __QAIC_HEADER_ATTRIBUTE;
/*!
    * @brief: Send array of buffers to be used during the session.
    *
    * @description: Use this to send buffers that have been allocated in HLOS,
    *               but that will not be accessed by HLOS.
    *               These buffers are to be used for the duration of the session and are returned at SESS_CLOSE.
    *
    * @in sess_id: Determines the session to which the contained buffers are addressed.
    *
    * @in bufs_attr: Array of buffer attributes. Must be allocated and set by caller.
    *
    * @out bufs: Array of buffers. Must be allocated by caller.
    *
    * @return: Error code.
    *
    */
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(hcp_rpc_send_sess_buf)(remote_handle64 _h, int sess_id, const hcp_rpc_pkt_t* bufs_attr, int bufs_attrLen, hcp_rpc_raw_buffer_t* bufs, int bufsLen) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(hcp_rpc_send_sess_buf_dma)(remote_handle64 _h, int sess_id, const hcp_rpc_pkt_t* bufs_attr, int bufs_attrLen, int bufs, uint32 bufsOffset, uint32 bufsLen) __QAIC_HEADER_ATTRIBUTE;
/*!
    * @brief: Send array of global buffers.
    *
    * @description: The buffers are returned under different conditions which are based on system settings.
    *
    * @in bufs_attr: Array of buffer attributes. Must be allocated and set by caller.
    *
    * @out bufs: Array of buffers. Must be allocated by caller.
    *
    * @return: Error code.
    *
    */
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(hcp_rpc_send_sys_buf)(remote_handle64 _h, const hcp_rpc_pkt_t* bufs_attr, int bufs_attrLen, hcp_rpc_raw_buffer_t* bufs, int bufsLen) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(hcp_rpc_send_sys_buf_dma)(remote_handle64 _h, const hcp_rpc_pkt_t* bufs_attr, int bufs_attrLen, int bufs, uint32 bufsOffset, uint32 bufsLen) __QAIC_HEADER_ATTRIBUTE;
/*!
    * @brief: Receive message.
    *
    * @description: Send raw packet to the Host, and receive a response packet.
    *
    * @out out: pointer to packet beceived from the Host. Must be allocated by caller.
    *
    * @in block: If 1, block caller until a packet is available. If 0, return error if there are no packets in queue.
    *
    * @return: Error code.
    */
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(hcp_rpc_recv_pkt)(remote_handle64 _h, int sess_id, hcp_rpc_pkt_t* pkt, int block) __QAIC_HEADER_ATTRIBUTE;
#ifndef hcp_rpc_URI
#define hcp_rpc_URI "file:///libhcp_rpc_skel.so?hcp_rpc_skel_handle_invoke&_modver=1.0"
#endif /*hcp_rpc_URI*/
#ifdef __cplusplus
}
#endif
#endif //_HCP_RPC_H
