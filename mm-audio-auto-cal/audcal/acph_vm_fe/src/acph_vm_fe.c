/*===========================================================================
    FILE:           acph_vm_fe.c

    OVERVIEW:       This file contains the implementaion of the acph vm frontend
                    API and its helper methods.

    DEPENDENCIES:   Depends on sub modules of audcal and habmm module

                    Copyright (c) 2017 Qualcomm Technologies, Inc.
                    All Rights Reserved.
                    Confidential and Proprietary - Qualcomm Technologies, Inc.
========================================================================== */

/*===========================================================================
    EDIT HISTORY FOR MODULE

    This section contains comments describing changes made to the module.
    Notice that changes are listed in reverse chronological order. Please
    use ISO format for dates.

    $ $Header:

    when        who     what, where, why
    ----------  ---     -----------------------------------------------------
    2017-08-22  ct  Enable ACPH VM Frontend for GVM Calibration.
========================================================================== */

/* ---------------------------------------------------------------------------
 * Include Files
 *--------------------------------------------------------------------------- */
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include "habmm.h"
#include "acph_vm_msg.h"
#include "acph_vm_fe.h"

/* ---------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *--------------------------------------------------------------------------- */
#define HAB_TIMEOUT_IN_MS       0xFFFFFFFF
#define HAB_API_FLAG            0
#define ACPH_HAB_CH_SUB_ID      4

#define ACPH_VM_LOG_LEVEL_INFO  1
#define ACPH_VM_LOG_LEVEL_ERROR 2

#define ACPH_VM_LOG(level, format, ...) \
    if (level >= ACPH_VM_LOG_LEVEL_INFO) { \
        fprintf(stdout, format, ##__VA_ARGS__);\
    }

/* ---------------------------------------------------------------------------
 * Type Declarations
 *--------------------------------------------------------------------------- */
typedef struct acph_vm_fe_global_info
{
    int32_t     hab_handle;
    uint8_t    *acph_req_buf;
    int32_t     acph_req_buf_len;
    uint32_t    acph_req_export_id;
    uint8_t    *acph_resp_buf;
    int32_t     acph_resp_buf_len;
    uint32_t    acph_resp_export_id;
    pfn_Callback callback;
    pthread_t thread;
} acph_vm_fe_global_into_t;

/* ---------------------------------------------------------------------------
 * Global Data Definitions
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Static Variable Definitions
 *--------------------------------------------------------------------------- */
static acph_vm_fe_global_into_t g_acph_vm_fe;

/* ---------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Externalized Function Definitions
 *--------------------------------------------------------------------------- */

/*****************************************************************************
 * Core Routine Implementations                                              *
 ****************************************************************************/

static int32_t acph_vm_fe_import_id(uint32_t handle,
                                    uint8_t     **buffer_shared,
                                    uint32_t length_shared,
                                    uint32_t export_id
                                    )
{
    int32_t rc = 0;

    rc = habmm_import(handle, (void **)buffer_shared, length_shared, export_id, HAB_API_FLAG);
    if (0 != rc)
    {
        ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_ERROR, "acph_vm_fe unable to import id 0x%x handle 0x%x", export_id, handle);
        return ACPH_VM_FAILURE;
    }

    return ACPH_VM_SUCCESS;
}

static int8_t acph_vm_fe_unimport_id(uint32_t handle,
                                    uint32_t export_id,
                                    uint8_t *buff_shared
                                    )
{
    int32_t rc = 0;

    rc = habmm_unimport(handle, export_id, buff_shared, HAB_API_FLAG);

    if (0 != rc)
    {
        ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_ERROR, "acph_vm_fe unable to unimport id %x handle %x buff %p",
                export_id, handle, buff_shared);
        return ACPH_VM_FAILURE;
    }

    return ACPH_VM_SUCCESS;
}

static int32_t acph_vm_fe_task_fn(void* param)
{
    int32_t rc;
    acph_vm_payload_t input;
    acph_vm_payload_t output;
    uint32_t payload_size = sizeof(acph_vm_payload_t);
    uint32_t rsize = sizeof(acph_vm_payload_t);
    uint8_t *resp_buf;
    uint32_t resp_buf_len;

    ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_INFO,"acph_vm_fe thread enter\n");
    rc = habmm_socket_open(&g_acph_vm_fe.hab_handle,
                            HAB_MMID_CREATE(MM_AUD_4, ACPH_HAB_CH_SUB_ID),
                            HAB_TIMEOUT_IN_MS,
                            0);
    if (0 != rc ) {
        ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_ERROR, "acph_vm_fe fail to open hab channel,rc %d", rc);
        goto end;
    }

    /* Blockin loop to receive PVM callbacks */
    while (1)
    {
        /* Wait for callback message */
        memset(&input, 0, payload_size);
        memset(&output, 0, payload_size);

        rc = habmm_socket_recv(g_acph_vm_fe.hab_handle,
                            (void *)&input,
                              &rsize,
                              HAB_TIMEOUT_IN_MS,
                              0);
        if (rc) {
            ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_ERROR, "acph_vm_fe hammm_socet_recv receive fail: %d\n", rc);
            /* TODO: depends on the HAB error code, may need to implement
             * a retry mechanism.
             */
            goto end;
        }

        if (ACPH_VM_MSG_BUFFER_SHARED == input.msg_type) {
            rc = acph_vm_fe_import_id(g_acph_vm_fe.hab_handle,
                    &g_acph_vm_fe.acph_req_buf,
                    input.shared_payload.req_buf_length,
                    input.shared_payload.req_buf_export_id);
            if (ACPH_VM_SUCCESS != rc){
                ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_ERROR, "acph_vm_fe import req buffer fail");
                goto end;
            }

            rc = acph_vm_fe_import_id(g_acph_vm_fe.hab_handle,
                    &g_acph_vm_fe.acph_resp_buf,
                    input.shared_payload.resp_buf_length,
                    input.shared_payload.resp_buf_export_id);
            if (ACPH_VM_SUCCESS != rc){
                ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_ERROR, "acph_vm_fe import resp buffer fail");
                goto end;
            }

            g_acph_vm_fe.acph_req_buf_len = input.shared_payload.req_buf_length;
            g_acph_vm_fe.acph_resp_buf_len = input.shared_payload.resp_buf_length;
            g_acph_vm_fe.acph_req_export_id = input.shared_payload.req_buf_export_id;
            g_acph_vm_fe.acph_resp_export_id = input.shared_payload.resp_buf_export_id;
            ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_INFO, "req buf %p len %x id %x resp buf %p len %x id %x\n",
                g_acph_vm_fe.acph_req_buf,
                g_acph_vm_fe.acph_req_buf_len,
                g_acph_vm_fe.acph_req_export_id,
                g_acph_vm_fe.acph_resp_buf,
                g_acph_vm_fe.acph_resp_buf_len,
                g_acph_vm_fe.acph_resp_export_id);

            output.msg_type = ACPH_VM_MSG_BUFFER_SHARED_RESP;
            output.shared_resp_payload.status  = 1;
        } else if (ACPH_VM_MSG_BUFFER_REQ == input.msg_type) {
            g_acph_vm_fe.acph_req_buf_len = input.req_payload.real_req_len;
            if (g_acph_vm_fe.callback) {
                g_acph_vm_fe.callback((uint8_t *)g_acph_vm_fe.acph_req_buf,
                        g_acph_vm_fe.acph_req_buf_len,
                        &resp_buf,
                        &resp_buf_len);
                memcpy(g_acph_vm_fe.acph_resp_buf, resp_buf, resp_buf_len);
                output.msg_type = ACPH_VM_MSG_BUFFER_RESP;
                output.resp_payload.real_resp_len = resp_buf_len;
            } else {
                ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_ERROR, "acph_vm_fe callback not initialized, thread exit!\n");
                goto end;
            }
        } else if (ACPH_VM_MSG_ONLINE_PING == input.msg_type) {
                output.msg_type = ACPH_VM_MSG_ONLINE_ACK;
                ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_INFO, "acph_vm_fe receive ping message\n");
        } else {
            ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_ERROR, "acph_vm_fe receive unknown message\n");
                output.msg_type = ACPH_VM_MSG_BUFFER_RESP;
                output.resp_payload.status = 0xFFFFFFFF;
        }

        rc = habmm_socket_send(g_acph_vm_fe.hab_handle,
                (void *)&output,
                sizeof(output),
                0);
        if (0 != rc) {
            ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_ERROR, "acph sendback shared payload resp failure");
            goto end;
        }
    }

end:
    ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_INFO, "acph_vm_fe thread exit\n");
    pthread_exit(NULL);
}

int32_t acph_vm_fe_init (pfn_Callback callback)
{
    int32_t rc = 0;

    ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_INFO, "%s Enter\n", __FUNCTION__);
    memset(&g_acph_vm_fe, 0, sizeof(g_acph_vm_fe));

    /* Create thread to communicate with ACPH VM BE service in PVM */
    rc = pthread_create(&g_acph_vm_fe.thread, NULL,
                         (void *)acph_vm_fe_task_fn, NULL);
    if ( rc )
        ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_ERROR, "Failed to create vapm fe service: %d", rc);
    g_acph_vm_fe.callback = callback;
    ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_INFO, "%s Leave rc %d\n", __FUNCTION__, rc);
    return rc;
}

int32_t acph_vm_fe_deinit (void)
{
    int rc, *ret_val;

    ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_INFO, "%s Enter\n", __FUNCTION__);

    if (g_acph_vm_fe.hab_handle && g_acph_vm_fe.acph_req_export_id)
        acph_vm_fe_unimport_id(g_acph_vm_fe.hab_handle, g_acph_vm_fe.acph_req_export_id,
                g_acph_vm_fe.acph_req_buf);

    if (g_acph_vm_fe.hab_handle && g_acph_vm_fe.acph_resp_export_id)
         acph_vm_fe_unimport_id(g_acph_vm_fe.hab_handle, g_acph_vm_fe.acph_resp_export_id,
                g_acph_vm_fe.acph_resp_buf);

    if (g_acph_vm_fe.hab_handle)
        habmm_socket_close(g_acph_vm_fe.hab_handle);

    rc = pthread_join(g_acph_vm_fe.thread, (void **)&ret_val);
    if (rc)
        ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_ERROR, "pthread join acph vm fe thread failure");

    ACPH_VM_LOG(ACPH_VM_LOG_LEVEL_INFO, "%s Leave rc %d\n", __FUNCTION__, rc);
    return rc;
}
