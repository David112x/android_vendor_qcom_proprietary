/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef SEC_CAM_ENC_H
#define SEC_CAM_ENC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

/******************************************************************************
 * Macros and Structure definitions
 *****************************************************************************/

typedef void *sce_handle;

/* Max no. of buffers allowed to register, OEMs can change this as needed */
#define MAX_NUM_BUFFS 30

/*--------------------------------------------------------------------------
  Errors returned by TA or libseccamenc
  --------------------------------------------------------------------------*/
typedef enum sce_result {
    SCE_SUCCESS = 0,
    SCE_ERROR_INIT_FAILED = 100,
    SCE_ERROR_TERMINATE_FAILED = 101,
    SCE_ERROR_FEATURE_NOT_SUPPORTED = 102,
    SCE_ERROR_INVALID_PARAMS = 103,
    SCE_ERROR_BUFFER_TOO_SHORT = 104,
    SCE_ERROR_REG_BUF_FAILED = 105,
    SCE_ERROR_PREPARE_BUF_FAILED = 106,
    SCE_ERROR_COPY_FAILED = 107,
    SCE_ERROR_CRYPTO_FAILED = 108,
    SCE_FATAL_ERROR = 109,
    SCE_FAILURE = 0x7FFFFFFF
} sce_result_t;

/*--------------------------------------------------------------------------
  SCE buf types
  --------------------------------------------------------------------------*/
typedef enum sce_buf_type {
    SCE_BUF_FULL_FRAME = 10,
    SCE_BUF_DOWN_SCALE_FRAME = 11,
    SCE_BUF_RAW_FRAME = 12,
    SCE_BUF_ENCODE_FRAME = 13,
    SCE_BUF_NON_SECURE = 14,
    SCE_BUF_TYPE_MAX,
    SCE_BUF_INVALID_TYPE = 0x7FFFFFFF
} sce_buf_type_t;

/*--------------------------------------------------------------------------
  Generic SCE buffer structure
  --------------------------------------------------------------------------*/
typedef struct sce_buffer {
    sce_buf_type_t buf_type;
    uint32_t buf_size;
    uint64_t buf_handle;
} __attribute__((packed)) sce_buf_t;

/*--------------------------------------------------------------------------
  Structure for sce buffer tuple
  --------------------------------------------------------------------------*/
typedef struct sce_buf_tuple {
    sce_buf_t sce_buf1;
    sce_buf_t sce_buf2;
} __attribute__((packed)) sce_buf_tuple_t;


/******************************************************************************
 * APIs exposed by libseccamenc
 *****************************************************************************/

/*
  This API loads the secure app and returns the handle to the client.

  @return
  SCE_SUCCESS                  - Success.
  SCE_ERROR_INIT_FAILED        - Failure.

  @param[out]       l_handle     pointer to the handle

  @dependencies
  This function should be invoked by the client before starting any secure
  session using sce_copy().

*/
sce_result_t sce_init(sce_handle *l_handle, const char *app_name);

/*
  This API registers buffers with TA. Only registered buffers will be allowed
  to use in Prepare, Copy and Encrypt APIs. All Full frame, Downscale and
  Encoder buffers must be registered at the beginning of the usecase, new
  buffers can't be registered during usecase.

  @return
  SCE_SUCCESS               - Success.
  SCE_ERROR_REG_BUF_FAILED  - Failure.

  @param[in]    l_handle       Pointer to the handle returned by sce_init().
  @param[in]    buf_list       Pointer to array of sce_buf_t struct which
                               contain buffers to be registered.
  @param[in]    list_size      Size of buf_list array in bytes.

  @dependencies
  This function must be called at the beginning of use case before using any
  of these APIs - Prepare, Copy, Encrypt. Only registred buffers will be
  processed by TA.

*/
sce_result_t sce_register_buffers(sce_handle l_handle,
                                  const sce_buf_t *sce_buf_list,
                                  const uint32_t num_bufs);

/*
  This API prepares full frame and it's associated downscaled buffers for
  further processing. Only prepared buffers will be allowed to use in Copy API.
  Full frame : downscaled frame association should be strictly maintained.

  @return
  SCE_SUCCESS                  - Success.
  SCE_ERROR_PREPARE_BUF_FAILED - Failure.

  @param[in]    l_handle       Pointer to the handle returned by sce_init().
  @param[in]    buf_tuple      Reference to a tuple of sce_buf_t containing
                               full frame and it's corresponding downscaled
                               frame buffer to be prepared.

  @dependencies
  This function must be called after calling sce_init() and before calling
  sce_copy() API.
*/
sce_result_t sce_prepare_buffers(sce_handle l_handle,
                                 const sce_buf_tuple_t &buf_tuple);

/*
  This API copies frame data from secure downscaled(or raw) frame into the
  non-secure out buffer if allowed.

  @return
  SCE_SUCCESS                - Success.
  SCE_ERROR_COPY_FAILED      - Failure.

  @param[in]    l_handle       Pointer to the handle returned by sce_init().
  @param[in]    buf_tuple      Reference to a tuple of sce_buf_t containing
                               downscaled frame and a non secure output buffer.
  @param[out]   out_buf_size   Number of bytes written into the out buffer.

  @dependencies
  This function must only be invoked after using sce_init() &
  sce_prepare_buffers(). All registered downscaled buffers must be sent for
  copy in order.
*/
sce_result_t sce_copy_frame_data(sce_handle l_handle,
                                 const sce_buf_tuple_t &buf_tuple,
                                 uint32_t &out_buf_size);

/*
  This function signs and encrypts data pointed by enc_buf and puts signed +
  encrypted data into non secure out_buf from struct app_buf_desc. Input buffer
  must be a registered encoder buffer.

  @return
  SCE_SUCCESS                   - Success.
  SCE_ERROR_SIGN_ENCRYPT_FAILED - Failure.

  @param[in]    l_handle       Pointer to the handle returned by sce_init().
  @param[in]    buf_tuple      Reference to a tuple of sce_buf_t containing
                               encoded frame and a non secure output buffer.
  @param[out]   out_buf_size   Number of bytes written into the out buffer.

  @dependencies
  This API must be called after using sce_init() and sce_copy_frame_data().

*/
sce_result_t sce_sign_encrypt(sce_handle l_handle,
                              const sce_buf_tuple_t &buf_tuple,
                              uint32_t &out_buf_size);

/*
  This API unloads the secure app corresponding to the client handle.

  @return
  SCE_SUCCESS                  - Success.
  SCE_ERROR_TERMINATE_FAILED   - Failure.

  @param[in]    l_handle       Pointer to the handle returned by sce_init().

  @dependencies
  This function should be invoked by the client on properly initialized sessoin
  and before closing secure camera encode usecase application.

*/
sce_result_t sce_terminate(sce_handle *l_handle);

#ifdef __cplusplus
}
#endif

#endif /* SEC_CAM_ENC_H */
