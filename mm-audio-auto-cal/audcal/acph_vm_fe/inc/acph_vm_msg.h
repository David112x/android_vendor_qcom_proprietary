#ifndef _ACPH_VM_MSG_H_
#define _ACPH_VM_MSG_H_
/*===========================================================================
    @file   acph_vm_msg.h

    The message struct between acph frontend and backend in hypervisor.

    This file will provide message struct definitions for fe and be.

                    Copyright (c) 2017 Qualcomm Technologies, Inc.
                    All Rights Reserved.
                    Confidential and Proprietary - Qualcomm Technologies, Inc.
========================================================================== */
/*===========================================================================
    EDIT HISTORY FOR MODULE

    This section contains comments describing changes made to the module.
    Notice that changes are listed in reverse chronological order. Please
    use ISO format for dates.

    when        who     what, where, why
    ----------  ---     -----------------------------------------------------
    2017-08-29  ct      Enable ACPH VM Frontend for GVM Calibration.
===========================================================================*/

/* ---------------------------------------------------------------------------
 * Include Files
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *--------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Type Declarations
 *--------------------------------------------------------------------------- */
enum acph_vm_msg_type
{
    ACPH_VM_MSG_BUFFER_SHARED,
    ACPH_VM_MSG_BUFFER_SHARED_RESP,
    ACPH_VM_MSG_BUFFER_REQ,
    ACPH_VM_MSG_BUFFER_RESP,
    ACPH_VM_MSG_ONLINE_PING,
    ACPH_VM_MSG_ONLINE_ACK,
    ACPH_VM_MSG_MAX,
};

struct acph_vm_buffer_shared_payload {
    uint32_t req_buf_export_id;
    uint32_t req_buf_length;
    uint32_t resp_buf_export_id;
    uint32_t resp_buf_length;
};

struct acph_vm_buffer_req_payload {
    uint32_t real_req_len;
    uint32_t reserved;
};

struct acph_vm_buffer_resp_payload{
    uint32_t real_resp_len;
    uint32_t status;
    uint32_t reserved;
};

struct acph_vm_buffer_shared_resp_payload {
    uint32_t status;
    uint32_t reserved;
};

struct acph_vm_online_ping_payload {
    uint32_t reserved;
};

struct acph_vm_online_ack_payload {
    uint32_t reserved;
};

typedef struct acph_vm_payload {
    enum acph_vm_msg_type msg_type;
    union {
        struct acph_vm_buffer_shared_payload        shared_payload;
        struct acph_vm_buffer_shared_resp_payload   shared_resp_payload;
        struct acph_vm_buffer_req_payload           req_payload;
        struct acph_vm_buffer_resp_payload          resp_payload;
        struct acph_vm_online_ping_payload          ping_payload;
        struct acph_vm_online_ack_payload           ack_payload;
    };
}acph_vm_payload_t;

#endif  //#ifndef _ACPH_VM_MSG_H_
