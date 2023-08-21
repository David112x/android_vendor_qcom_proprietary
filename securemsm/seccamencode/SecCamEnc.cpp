/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "SecCamEnc.h"
#include "QSEEComAPI.h"
#include "SecCamEncCommon.h"
#include "common_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 *                    MACRO & Global definitions
 *****************************************************************************/

#define SCE_BUF_SIZE (100 * 1024)

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static const char *const app_paths[] = {"/vendor/firmware_mnt/image",
                                        "/vendor/firmware", "/firmware/image"};
static uint32_t nr_app_paths = NUM_ELEMS(app_paths);

/******************************************************************************
 *                    Local helper APIs
 *****************************************************************************/

/*
Description: This API prepares an array of two sce_buf_t from input
sce_buf_tuple_t. Buf_tuple must have at least one buffer of type 'buf_type'.

@return
SCE_SUCCESS              -   Success.
SCE_ERROR_INVALID_PARAMS -   Failure.

@param[in]    buf_tuple      Reference to a sce_buf_tuple_t containing two bufs.
@param[out]   buf_list       Reference to an array of sce_buf_t of size two.
@param[in]    buf_type       Type of buffer to be checked against buf_tuple.
*/
static inline sce_result_t sce_prepare_list(const sce_buf_tuple_t &buf_tuple,
                                            sce_buf_t (&buf_list)[2],
                                            sce_buf_type_t buf_type)
{
    sce_result_t ret = SCE_SUCCESS;

    if(buf_type == buf_tuple.sce_buf1.buf_type) {
        buf_list[0] = buf_tuple.sce_buf1;
        buf_list[1] = buf_tuple.sce_buf2;
    } else if(buf_type == buf_tuple.sce_buf2.buf_type) {
        buf_list[0] = buf_tuple.sce_buf2;
        buf_list[1] = buf_tuple.sce_buf1;
    } else {
        SCE_LOG_MSG_DBG("Buffer not of type : %d", buf_type);
        ret = SCE_ERROR_INVALID_PARAMS;
    }

exit:
    return ret;
}

/*
Description: This API send sce buffers and a command id to TA for process.

@return
SCE_SUCCESS     -            Success.
SCE_FAILURE     -            Failure.

@param[in]    l_handle       Pointer to the SCE handle passed by application.
@param[in]    sce_buf_list   Pointer to an array of sce_buf_t containing
                             num_bufs sce buffers passed by application.
@param[in]    num_bufs       Number of buffers from sce_buf_list to pass to TZ.
@param[out]   out_buf_size   Number of bytes written into out NS buffer.
@param[in]    cmd_id         Command to send to TZ.
*/
static sce_result_t sce_send_command(sce_handle l_handle,
                                     const sce_buf_t *sce_buf_list,
                                     uint32_t num_bufs,
                                     uint32_t &out_buf_size,
                                     sce_cmd_type_t cmd_id)
{
    long ret = SCE_SUCCESS;
    uint32_t cmd_len = 0;
    uint32_t rsp_len = 0;
    size_t i = 0;
    size_t buf_to_send = 0;
    size_t buf_remaining = 0;
    size_t ion_fd_info_offset = 0;
    tz_sce_buf_req_t *cmd = NULL;
    tz_sce_buf_rsp_t *resp = NULL;
    sce_buf_t l_sce_buf = {};
    struct QSEECom_handle *l_QSEEComHandle = NULL;
    struct QSEECom_ion_fd_info ion_fd_info = {};

    ENTER();

    /* Sanity check */
    CHECK_COND_MSG(l_handle == NULL, SCE_ERROR_INVALID_PARAMS,
                   "SCE handle is NULL");
    CHECK_COND_MSG(sce_buf_list == NULL, SCE_ERROR_INVALID_PARAMS,
                   "Buf list is NULL");
    CHECK_COND_MSG(num_bufs < 1, SCE_FAILURE, "Invalid num_bufs:%d", num_bufs);
    /*------------------------------------------------------------------------
      Prepare, Copy and Sign_Encrypt APIs request must have only 2 num_bufs.
      ------------------------------------------------------------------------*/
    CHECK_COND_MSG((SCE_REGISTER_BUFFERS != cmd_id) && (2 != num_bufs),
            SCE_FAILURE, "Invalid num_bufs:%d for cmd:%d", num_bufs, cmd_id);

    l_QSEEComHandle = (struct QSEECom_handle *)l_handle;

    SCE_PREPARE_REQ_RSP_BUF(cmd_id);

    ion_fd_info_offset = offsetof(tz_sce_buf_req_t, sce_bufs) +
                         offsetof(sce_buf_t, buf_handle);

    buf_remaining = num_bufs;

    /*--------------------------------------------------------------------------
      As we can send only SCE_MAX_QSEECOM_ION_BUF no. of buffers in single
      QSEECom_send_modified_cmd_64() call, make multiple calls to TZ if no.
      of buffers to register are more than SCE_MAX_QSEECOM_ION_BUF.
      --------------------------------------------------------------------------*/
    for (i = 0; i < num_bufs;) {
        memset((void *)cmd, 0, SCE_BUF_SIZE);
        memset((void *)&ion_fd_info, 0, sizeof(QSEECom_ion_fd_info));

        buf_to_send = buf_remaining < SCE_MAX_QSEECOM_ION_BUF
                         ? buf_remaining
                         : SCE_MAX_QSEECOM_ION_BUF;

        /* Fill in request structure fields. */
        cmd->cmd_id = cmd_id;
        cmd->num_buf = buf_to_send;

        for (auto j = 0; j < buf_to_send; j++) {
            l_sce_buf = sce_buf_list[i + j];

            SCE_VALIDATE_BUFFER(l_sce_buf);

            cmd->sce_bufs[j].buf_type = l_sce_buf.buf_type;
            cmd->sce_bufs[j].buf_size = l_sce_buf.buf_size;

            /* Fill in ion handle info */
            ion_fd_info.data[j].fd = l_sce_buf.buf_handle;
            ion_fd_info.data[j].cmd_buf_offset =
                ion_fd_info_offset + j * sizeof(sce_buf_t);
        }

        ret = QSEECom_send_modified_cmd_64(l_QSEEComHandle, cmd, cmd_len, resp,
                                           rsp_len, &ion_fd_info);
        CHECK_COND_MSG(ret != 0, SCE_FAILURE, "ioctl failed, ret :%ld", ret);

        ret = (sce_result_t)resp->ret;
        CHECK_COND_MSG(SCE_SUCCESS != ret, ret, "Error from TZ, ret :%ld", ret);

        buf_remaining -= buf_to_send;
        i += buf_to_send;
    }

    /* This should never happen */
    CHECK_COND_MSG((i != num_bufs) || buf_remaining,
                   SCE_FAILURE, "error in logic ret:%ld", ret);

    out_buf_size = resp->out_buf_size;

exit:

    EXIT();

    return (sce_result_t)ret;
}

/******************************************************************************
 *                  External exposed APIs
 *****************************************************************************/

/*
Descriptoin: This API loads the secure app and returns the handle to the client.

@return
SCE_SUCCESS                  - Success.
SCE_ERROR_INIT_FAILED        - Failure, in case of init failed.
SCE_ERROR_INVALID_PARAMS     - Failure, in case of invalid parameters.

@param[out]       l_handle     Pointer to the handle
*/
sce_result_t sce_init(sce_handle *l_handle, const char *app_name)
{
    long ret = SCE_SUCCESS;
    long qsee_ret = 0;
    struct QSEECom_handle **l_QSEEComHandle = NULL;

    ENTER();

    /* Sanity check */
    CHECK_COND_MSG(l_handle == NULL || *l_handle != NULL,
                   SCE_ERROR_INVALID_PARAMS, "Invalid Handle");
    CHECK_COND_MSG(app_name == NULL, SCE_ERROR_INVALID_PARAMS,
                   "Invalid app name");

    l_QSEEComHandle = (struct QSEECom_handle **)l_handle;

    for (auto i = 0; i < nr_app_paths; i++) {
        ret = QSEECom_start_app(l_QSEEComHandle, app_paths[i], app_name,
                                SCE_BUF_SIZE);
        if (!ret) {
            break;
        }
    }
    CHECK_COND_MSG(ret != 0, SCE_ERROR_INIT_FAILED, "Failed to load app (%s).",
                   app_name);

exit:
    /* Unload TA incase of error, if it is loaded already */
    if (ret && l_QSEEComHandle && *l_QSEEComHandle) {
        qsee_ret = QSEECom_shutdown_app(l_QSEEComHandle);
        CHECK_COND_LOG_MSG(qsee_ret, "TA unloading failed, ret:%ld", qsee_ret);
    }

    EXIT();
    return (sce_result_t)ret;
}

/*
  Description: This API registers buffer(s) with TA. Only registered buffers
  will be allowed to use in Prepare, Copy, Encrypt APIs. All buffers must be
  registered at the beginning of the usecase, new buffers can't be registered
  during usecase.

  @return
  SCE_SUCCESS                - Success.
  SCE_ERROR_REG_BUF_FAILED  - Failure.

  @param[in]    l_handle       Pointer to the handle returned by sce_init().
  @param[in]    buf_list       Pointer to an array of sce_buf_t struct which
                               contain buffers to be registered.
  @param[in]    list_size      Size of buf_list array in bytes.
*/
sce_result_t sce_register_buffers(sce_handle l_handle,
                                  const sce_buf_t *sce_buf_list,
                                  const uint32_t num_bufs)
{
    long ret = SCE_SUCCESS;
    uint32_t out_buf_size;

    ENTER();

    /* Sanity check */
    CHECK_COND_MSG((num_bufs < 1) || (num_bufs > MAX_NUM_BUFFS),
                    SCE_ERROR_REG_BUF_FAILED,
                    "Invalid number of buffers:%d",num_bufs);

    /* Lock mutex */
    SCE_LOCK_MUTEX(&g_mutex);

    ret = sce_send_command(l_handle, sce_buf_list, num_bufs, out_buf_size, SCE_REGISTER_BUFFERS);

    CHECK_COND_MSG(ret != SCE_SUCCESS, SCE_ERROR_REG_BUF_FAILED,
                   "sce_send_command failed, ret: %ld", ret);

exit:
    /* Unlock mutex */
    SCE_UNLOCK_MUTEX(&g_mutex);

    EXIT();

    return (sce_result_t)ret;
}

/*
Description: This API prepares full frame and it's associated downscaled
buffers for further processing. Only prepared buffers will be allowed to
use in Copy API. Full frame : downscaled frame association should be strictly
maintained.

@return
SCE_SUCCESS                  - Success.
SCE_ERROR_PREPARE_BUF_FAILED - Failure.

@param[in]    l_handle       Pointer to the handle returned by sce_init().
@param[in]    buf_tuple      Reference to a tuple of sce_buf_t containing
                             full frame and it's corresponding downscaled
                             frame buffer to be prepared.
*/
sce_result_t sce_prepare_buffers(sce_handle l_handle,
                                 const sce_buf_tuple_t &buf_tuple)
{
    long ret = SCE_SUCCESS;
    uint32_t out_buf_size = 0;
    sce_buf_t l_buf_list[2] = {};

    ENTER();

    /* Lock mutex */
    SCE_LOCK_MUTEX(&g_mutex);

    /*------------------------------------------------------------------------
      For Prepare API request sent to TA, first buffer of request must be FF
      and second buffer must be an associated DS buffer.
      ------------------------------------------------------------------------*/
    ret = sce_prepare_list(buf_tuple, l_buf_list, SCE_BUF_FULL_FRAME);
    CHECK_COND_MSG(ret != SCE_SUCCESS, SCE_ERROR_PREPARE_BUF_FAILED,
                   "Invalid input buf tuple, ret: %ld", ret);

    CHECK_COND_MSG(SCE_BUF_DOWN_SCALE_FRAME != l_buf_list[1].buf_type,
                   SCE_ERROR_PREPARE_BUF_FAILED,
                   "Invalid Downscaled buff type");

    ret = sce_send_command(l_handle, l_buf_list, 2, out_buf_size, SCE_PREPARE_BUFFERS);

    CHECK_COND_MSG(ret != SCE_SUCCESS, SCE_ERROR_PREPARE_BUF_FAILED,
                   "sce_send_command failed, ret: %ld", ret);

exit:
    /* Unlock mutex */
    SCE_UNLOCK_MUTEX(&g_mutex);

    EXIT();

    return (sce_result_t)ret;
}

/*
Description: If allowed this API copies the content present in the secure buffer
(ds_buf or raw_buf),into the non-secure buffer (out_buf).

@return
SCE_SUCCESS               -  Success.
SCE_ERROR_COPY_FAILED     -  Failure, in case copy is failed.
SCE_ERROR_INVALID_PARAMS  -  Failure, in case of invalid parameters.

@param[in]    l_handle       Pointer to the handle returned by sce_init().
@param[in]    buf_tuple      Reference to a tuple of sce_buf_t containing
                             downscaled frame and a non secure output buffer.
@param[out]   out_buf_size   Number of bytes written into the out buffer.
*/
sce_result_t sce_copy_frame_data(sce_handle l_handle,
                                 const sce_buf_tuple_t &buf_tuple,
                                 uint32_t &out_buf_size)
{
    long ret = SCE_SUCCESS;
    sce_buf_t l_buf_list[2] = {};

    ENTER();

    /* Lock mutex */
    SCE_LOCK_MUTEX(&g_mutex);

    /*------------------------------------------------------------------------
      For Copy API request sent to TA, first buffer of request must be
      DS (or RAW) and second buffer must be a Non Secure buffer.
      ------------------------------------------------------------------------*/
    ret = sce_prepare_list(buf_tuple, l_buf_list, SCE_BUF_DOWN_SCALE_FRAME);
    if (ret != SCE_SUCCESS) {
        ret = sce_prepare_list(buf_tuple, l_buf_list, SCE_BUF_RAW_FRAME);
    }
    CHECK_COND_MSG(ret != SCE_SUCCESS, SCE_ERROR_COPY_FAILED,
                   "Invalid input buf tuple, ret: %ld", ret);

    CHECK_COND_MSG(SCE_BUF_NON_SECURE != l_buf_list[1].buf_type,
                   SCE_ERROR_COPY_FAILED,
                   "Output buff type is not a Non-Secure buff");

    ret = sce_send_command(l_handle, l_buf_list, 2, out_buf_size, SCE_COPY_FRAME_DATA);

    CHECK_COND_MSG(ret != SCE_SUCCESS, SCE_ERROR_COPY_FAILED,
                   "sce_send_command failed, ret: %ld", ret);

exit:
    /* Unlock mutex */
    SCE_UNLOCK_MUTEX(&g_mutex);

    EXIT();
    return (sce_result_t)ret;
}

/*
Description: This function signs and encrypts data pointed by enc_buf and puts
signed + encrypted data into non secure out_buf from struct app_buf_desc.
Input buffer must be a registered encoder buffer.

@return
SCE_SUCCESS                    - Success.
SCE_ERROR_SIGN_ENCRYPT_FAILED  - Failure.

@param[in]   l_handle       Pointer to the handle returned by sce_init().
@param[in]   buf_tuple      Reference to a tuple of sce_buf_t containing
                            encoded frame and a non secure output buffer.
@param[out]  out_buf_size   Number of bytes written into the out buffer.
*/
sce_result_t sce_sign_encrypt(sce_handle l_handle,
                              const sce_buf_tuple_t &buf_tuple,
                              uint32_t &out_buf_size)
{
    long ret = SCE_SUCCESS;
    sce_buf_t l_buf_list[2] = {};

    ENTER();

    /* Lock mutex */
    SCE_LOCK_MUTEX(&g_mutex);

    /*------------------------------------------------------------------------
      For Sign_Encrypt API request sent to TA, first buffer of request must be
      ENCODE buf and second buffer must be a Non Secure buffer.
      ------------------------------------------------------------------------*/
    ret = sce_prepare_list(buf_tuple, l_buf_list, SCE_BUF_ENCODE_FRAME);
    CHECK_COND_MSG(ret != SCE_SUCCESS, SCE_ERROR_CRYPTO_FAILED,
                   "Invalid input buf tuple, ret: %ld", ret);

    CHECK_COND_MSG(SCE_BUF_NON_SECURE != l_buf_list[1].buf_type,
                   SCE_ERROR_CRYPTO_FAILED,
                   "Output buff type is not a Non-Secure buff");

    ret = sce_send_command(l_handle, l_buf_list, 2, out_buf_size, SCE_SIGN_ENCRYPT_BUF);

    CHECK_COND_MSG(ret != SCE_SUCCESS, SCE_ERROR_CRYPTO_FAILED,
                   "sce_send_command failed, ret: %ld", ret);

exit:
    /* Unlock mutex */
    SCE_UNLOCK_MUTEX(&g_mutex);

    EXIT();
    return (sce_result_t)ret;
}

/*
Description: This API unloads the secure app corresponding to the client handle.

@return
SCE_SUCCESS                  - Success.
SCE_ERROR_TERMINATE_FAILED   - Failure, in case of termination failure.
SCE_ERROR_INVALID_PARAMS     - Failure, in case of invalid parameters.

@param[in]    l_handle      pointer to the handle
*/
sce_result_t sce_terminate(sce_handle *l_handle)
{
    long ret = 0;
    struct QSEECom_handle **l_QSEEComHandle = NULL;

    ENTER();

    /* Sanity check */
    CHECK_COND_MSG(l_handle == NULL || *l_handle == NULL,
                   SCE_ERROR_INVALID_PARAMS, "Invalid SCE Handle");
    l_QSEEComHandle = (struct QSEECom_handle **)l_handle;

    /* Lock mutex */
    SCE_LOCK_MUTEX(&g_mutex);

    /* Unload TA */
    ret = QSEECom_shutdown_app(l_QSEEComHandle);
    CHECK_COND_LOG_MSG(ret != 0, "Error: unloading app, returned = %ld", ret);

    /* Unlock mutex */
    SCE_UNLOCK_MUTEX(&g_mutex);

exit:
    EXIT();
    return (sce_result_t)ret;
}

#ifdef __cplusplus
}
#endif
