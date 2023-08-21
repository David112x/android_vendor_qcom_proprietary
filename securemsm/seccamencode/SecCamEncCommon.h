/**
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef SEC_CAM_ENC_COMMON_H
#define SEC_CAM_ENC_COMMON_H

#include <utils/Log.h>

#undef LOG_TAG
#define LOG_TAG "SecCamEnc"

/******************************************************************************
 * SCE lib specific Macros and Structure definitions
 *****************************************************************************/

//#define ENABLE_SCE_DEBUG // Uncomment this to enable debug logs

#ifdef ENABLE_SCE_DEBUG
    #define SCE_LOG_MSG_DBG(fmt, ...) LOGE(fmt, ##__VA_ARGS__);
#else
    #define SCE_LOG_MSG_DBG(fmt, ...) LOGD(fmt, ##__VA_ARGS__);
#endif // ENABLE_SCE_DEBUG

#define ENTER() SCE_LOG_MSG_DBG("%s: enter", __FUNCTION__)

#define EXIT() SCE_LOG_MSG_DBG("%s: exit ret %ld", __FUNCTION__, ret)

#define BAIL_OUT()                                                             \
    {                                                                          \
        LOGE("Error at %s : %d", __FUNCTION__, __LINE__);                      \
        goto exit;                                                             \
    }

#define CHECK_COND(cond)                                                       \
    if ((cond)) {                                                              \
        BAIL_OUT();                                                            \
    }

#define ERROR_EXIT(err_code)                                                   \
    ret = err_code;                                                            \
    BAIL_OUT();

#define CHECK_COND_ERROR(cond, err_code)                                       \
    if ((cond)) {                                                              \
        ret = err_code;                                                        \
        BAIL_OUT();                                                            \
    }

#define SCE_LOG_MSG_ERR(fmt, ...)                                              \
    LOGE("ERROR %s::%d " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define ERROR(rv, ...)                                                         \
    SCE_LOG_MSG_ERR(__VA_ARGS__);                                              \
    goto exit;

#define CHECK_COND_MSG(cond, error_code, ...)                                  \
    if (cond) {                                                                \
        ret = error_code;                                                      \
        ERROR(rv, __VA_ARGS__);                                                \
    }

#define CHECK_COND_LOG_MSG(cond, ...)                                          \
    if (cond) {                                                                \
        SCE_LOG_MSG_ERR(__VA_ARGS__);                                          \
    }

#define NUM_ELEMS(x) (sizeof(x) / sizeof((x)[0]))

#define SCE_LOCK_MUTEX(mutex)                                                  \
    {                                                                          \
        CHECK_COND_MSG(!mutex, SCE_ERROR_COPY_FAILED, "mutex is null");        \
        CHECK_COND_MSG(pthread_mutex_lock(mutex) != 0, SCE_ERROR_COPY_FAILED,  \
                       "locking mutex failed");                                \
    }

#define SCE_UNLOCK_MUTEX(mutex)                                                \
    {                                                                          \
        CHECK_COND_MSG(!mutex, SCE_ERROR_COPY_FAILED, "mutex is null");        \
        CHECK_COND_MSG(pthread_mutex_unlock(mutex) != 0,                       \
                       SCE_ERROR_COPY_FAILED, "unlocking mutex failed");       \
    }

#define SCE_VALIDATE_BUFFER(sce_buf)                                           \
    {                                                                          \
        CHECK_COND_MSG((sce_buf.buf_handle == 0 ||                             \
                        sce_buf.buf_size == 0 ||                               \
                        sce_buf.buf_type >= SCE_BUF_TYPE_MAX),                 \
                       SCE_ERROR_INVALID_PARAMS, "Invalid sce_buffer");        \
    }

#define SCE_PREPARE_REQ_RSP_BUF(cmd_name)                                      \
    {                                                                          \
        cmd_len = QSEECOM_ALIGN(sizeof(tz_sce_buf_req_t));                     \
        rsp_len = QSEECOM_ALIGN(sizeof(tz_sce_buf_rsp_t));                     \
        CHECK_COND_MSG((cmd_len + rsp_len) > SCE_BUF_SIZE, SCE_FAILURE,        \
                       "Insufficient buffer to accommodate cmd/resp");         \
        cmd = (tz_sce_buf_req_t *)l_QSEEComHandle->ion_sbuffer;                \
        CHECK_COND_MSG(NULL == cmd, SCE_FAILURE, "cmd is null");               \
        resp = (tz_sce_buf_rsp_t *)(l_QSEEComHandle->ion_sbuffer + cmd_len);   \
        CHECK_COND_MSG(NULL == resp, SCE_FAILURE, "resp is null");             \
        memset((void *)cmd, 0, SCE_BUF_SIZE);                                  \
        cmd->cmd_id = cmd_name;                                                \
    }

/*--------------------------------------------------------------------------
  QSEECOM can send max 4 ION buffers to TZ in single send_modified_cmd call.
  --------------------------------------------------------------------------*/
#define SCE_MAX_QSEECOM_ION_BUF 4

/******************************************************************************
 * Request/Response structures and commands used to communicate with TA
 *****************************************************************************/

/*--------------------------------------------------------------------------
  TZ App/Client commands
  --------------------------------------------------------------------------*/
typedef enum sce_cmd_type {
    SCE_CMD_START = 10,
    SCE_REGISTER_BUFFERS = 11,
    SCE_PREPARE_BUFFERS = 12,
    SCE_COPY_FRAME_DATA = 13,
    SCE_SIGN_ENCRYPT_BUF = 14,
    SCE_CMD_END,
    SCE_INVALID_CMD = 0x7FFFFFFF
} sce_cmd_type_t;

/*--------------------------------------------------------------------------
  Request structure to send upto 4 buffers to TZ
  --------------------------------------------------------------------------*/
typedef struct tz_sce_buf_req_s {
    /** First 4 bytes should always be command id */
    sce_cmd_type_t cmd_id;
    uint32_t num_buf;
    /** QSEECOM can send at max 4 ION buffers to TZ */
    sce_buf_t sce_bufs[SCE_MAX_QSEECOM_ION_BUF];
} __attribute__((packed)) tz_sce_buf_req_t;

/*--------------------------------------------------------------------------
  Response structure to get filled from TZ
  --------------------------------------------------------------------------*/
typedef struct tz_sce_buf_rsp_s {
    /** First 4 bytes should always be command id */
    sce_cmd_type_t rsp_id;
    long ret;
    uint32_t out_buf_size;
} __attribute__((packed)) tz_sce_buf_rsp_t;

#endif /* SEC_CAM_ENC_COMMON_H */
