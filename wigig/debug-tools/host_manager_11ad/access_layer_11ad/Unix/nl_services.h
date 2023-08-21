/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <cstdint>
#include "OperationStatus.h"

enum wil_nl_60g_evt_type
{
    DRIVER_EVENT_DRIVER_ERROR = 0,          // NL_60G_EVT_DRIVER_ERROR
    DRIVER_EVENT_FW_ERROR = 1,              // NL_60G_EVT_FW_ERROR
    DRIVER_EVENT_FW_WMI_EVENT = 2,          // NL_60G_EVT_FW_WMI
    DRIVER_EVENT_DRIVER_SHUTDOWN = 3,       // NL_60G_EVT_DRIVER_SHUTOWN
    DRIVER_EVENT_DRIVER_DEBUG_EVENT = 4,    // NL_60G_EVT_DRIVER_DEBUG_EVENT
    DRIVER_EVENT_DRIVER_GENERIC_EVENT = 5,  // NL_60G_EVT_DRIVER_GENERIC
    DRIVER_EVENT_FW_RMI_EVENT = 6,          // NL_60G_EVT_FW_RMI
};

/* Structure definition for the driver event report */
#define DRIVER_MSG_MAX_LEN 2048 // buffer size for driver events, contract with the Driver - do NOT change!
struct driver_event_report {
    uint32_t evt_type;      /* wil_nl_60g_evt_type */
    uint32_t buf_len;
    uint32_t evt_id;        /* dummy, for compatibility with other drivers (event index, keeping track of events in the queue) */
    uint8_t  reserved1;     /* more events in queue, not in use, for compatibility with other drivers */
    uint32_t listener_id;   /* dummy, for compatibility with other drivers */
    unsigned char buf[DRIVER_MSG_MAX_LEN];
} __attribute__((packed));

/* Abstract definition of the netlink state */
typedef struct nl_state* nl_state_ptr;

/* Initialize the netlink interface and test if driver supports driver events
 * interfaceName is the interface name without transport prefix
 * ppState is initialized with a handler to the allocated state
 */
OperationStatus nl_initialize(const char* interfaceName, nl_state_ptr* ppState);

/* Release the structures for NL communication */
void nl_release(nl_state_ptr pState);

/* Get bitmask of the driver capabilities */
OperationStatus nl_get_capabilities(nl_state_ptr pState, uint32_t& driverCapabilities);

/* Request to Enable/Disable events publishing to the user space */
OperationStatus nl_enable_driver_events_transport(nl_state_ptr pState, bool enable);

/* FW Reset through generic driver command */
OperationStatus nl_fw_reset(nl_state_ptr pState);

/*
 * Send command to the Driver
 * Notes:
 * Id represents driver command type which is a contract between the Driver and the command initiator.
 * Response is updated only for DRIVER_CMD_GENERIC_COMMAND.
 */
OperationStatus nl_send_driver_command(nl_state_ptr pState, uint32_t id, uint32_t bufLen, const void* pBuffer, uint32_t* pResponse);

/* Blocking call to get the next driver event */
OperationStatus nl_get_driver_event(nl_state_ptr pState, int cancelationFd, struct driver_event_report* pMessageBuf);
