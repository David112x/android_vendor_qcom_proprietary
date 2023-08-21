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
/*
 * Driver interaction with Linux nl80211/cfg80211
 * Copyright (c) 2002-2015, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2003-2004, Instant802 Networks, Inc.
 * Copyright (c) 2005-2006, Devicescape Software, Inc.
 * Copyright (c) 2007, Johannes Berg <johannes@sipsolutions.net>
 * Copyright (c) 2009-2010, Atheros Communications
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "nl_services.h"
#include "nl80211_copy.h"
#include "DebugLogger.h"

#include <sstream>
#include <memory>

#include <stdarg.h>
#include <inttypes.h>
#include <stdbool.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#ifndef SOL_NETLINK
    #define SOL_NETLINK    270
#endif

#define OUI_QCA 0x001374
#define NL_MSG_SIZE_MAX (8 * 1024)

enum qca_nl80211_vendor_subcmds_ {
    QCA_NL80211_VENDOR_SUBCMD_UNSPEC = 0,
};

enum qca_wlan_vendor_attr {
    QCA_WLAN_VENDOR_ATTR_TYPE = 7,
    QCA_WLAN_VENDOR_ATTR_BUF = 8,
    QCA_WLAN_VENDOR_ATTR_MAX,
};

/* This should be removed and DriverCommandType enum class should be used instead
   when netlink calls are compiled as C++ */
enum wil_nl_60g_cmd_type {
    DRIVER_CMD_FW_WMI = 0,
    DRIVER_CMD_GENERIC_COMMAND = 1,
    DRIVER_CMD_GET_DRIVER_STATISTICS = 2,
    DRIVER_CMD_REGISTER = 3,
    DRIVER_CMD_FW_RMI = 4,
};

/* enumeration of generic commands supported by the Driver */
enum wil_nl_60g_generic_cmd {
    NL_60G_GEN_FORCE_WMI_SEND = 0,
    NL_60G_GEN_RADAR_ALLOC_BUFFER = 1,
    NL_60G_GEN_FW_RESET = 2,
    NL_60G_GEN_GET_DRIVER_CAPA = 3,
    NL_60G_GEN_GET_FW_STATE = 4,
    NL_60G_GEN_AUTO_RADAR_RX_CONFIG = 5
};

/* structure with global state, passed to callback handlers */
struct nl_state
{
    /* callbacks handle for synchronous NL commands */
    struct nl_cb *cb;
    /* nl socket handle for synchronous NL commands */
    struct nl_sock *nl;
    /* nl socket handler for events */
    struct nl_sock *nl_event;
    /* family id for nl80211 events */
    int nl80211_id;
    /* interface index of wigig driver */
    int ifindex;
    /* event answer buffer to be filled */
    struct driver_event_report *driver_event_report_buf;
    /* sent command response */
    uint32_t command_response;
    /* true if driver has ability to publish WMI events and receive wmi CMD */
    bool has_wmi_pub;
};

// custom macro definition, cannot use nla_for_each_nested having implicit conversion from void*
#define for_each_nested_attribute(attribute_iter, nested_attribute_ptr, bytes_remaining) \
        for (attribute_iter = (struct nlattr *)nla_data(nested_attribute_ptr), bytes_remaining = nla_len(nested_attribute_ptr); \
             nla_ok(attribute_iter, bytes_remaining); \
             attribute_iter = nla_next(attribute_iter, &(bytes_remaining)))

/**
 * nl callback handler for disabling sequence number checking
 */
static int no_seq_check(struct nl_msg *msg, void *arg)
{
    (void)msg;
    (void)arg;
    return NL_OK;
}

/**
 * nl callback handler called on error
 */
static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
    (void)nla;

    if ( !(err && arg) ) // shouldn't happen
    {
        LOG_ERROR << "invalid arguments in nl error handler" << std::endl;
        return NL_SKIP;
    }

    int *ret = (int *)arg;
    *ret = err->error;

    LOG_DEBUG << "nl error handler with error: " << *ret << std::endl;

    return NL_SKIP;
}

/**
 * nl callback handler called after all messages in
 * a multi-message reply are delivered. It is used
 * to detect end of synchronous command/reply
 */
static int finish_handler(struct nl_msg *msg, void *arg)
{
    (void)msg;

    if (!arg) // shouldn't happen
    {
        LOG_ERROR << "invalid arguments in nl finish handler" << std::endl;
        return NL_SKIP;
    }

    int *ret = (int *)arg;
    *ret = 0;
    return NL_SKIP;
}

/**
 * nl callback handler called when ACK is received
 * for a command. It is also used to detect end of
 * synchronous command/reply
 */
static int ack_handler(struct nl_msg *msg, void *arg)
{
    (void)msg;

    if (!arg) // shouldn't happen
    {
        LOG_ERROR << "invalid arguments in nl ack handler" << std::endl;
        return NL_STOP;
    }

    int *err = (int *)arg;
    *err = 0;
    return NL_STOP;
}

// helper for translation of known error codes sent by the host driver to operation status
static OperationStatus convert_errno_to_status(int err)
{
    switch (err)
    {
    case (-EINVAL):
        return OperationStatus(false, "command not supported by the host driver");
    case (-EOPNOTSUPP):
        return OperationStatus(false, "command not supported for the DUT FW");
    case (-EAGAIN):
        return OperationStatus(false, "command blocked, system may be in Sys Assert");
    default:
        std::ostringstream oss;
        oss << "failed to send command, error " << err;
        return OperationStatus(false, oss.str());
    }
}

/**
 * handler for resolving multicast group (family) id
 * used in nl_get_multicast_id below
 */
struct family_data {
    const char *group;
    int id;
};

static int family_handler(struct nl_msg *msg, void *arg)
{
    if ( !(msg && arg) ) // shouldn't happen
    {
        LOG_ERROR << "invalid arguments in nl family handler" << std::endl;
        return NL_SKIP;
    }

    struct family_data *res = (struct family_data *)arg;
    struct nlattr *tb[CTRL_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *mcgrp = nullptr;
    int bytes_remaining = 0;

    nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
          genlmsg_attrlen(gnlh, 0), NULL);
    if (!tb[CTRL_ATTR_MCAST_GROUPS])
        return NL_SKIP;

    for_each_nested_attribute(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], bytes_remaining) {
        struct nlattr *tb2[CTRL_ATTR_MCAST_GRP_MAX + 1];

        nla_parse(tb2, CTRL_ATTR_MCAST_GRP_MAX, (struct nlattr *)nla_data(mcgrp),
              nla_len(mcgrp), NULL);
        if (!tb2[CTRL_ATTR_MCAST_GRP_NAME] ||
            !tb2[CTRL_ATTR_MCAST_GRP_ID] ||
            strncmp((const char*)nla_data(tb2[CTRL_ATTR_MCAST_GRP_NAME]),
                res->group,
                nla_len(tb2[CTRL_ATTR_MCAST_GRP_NAME])) != 0)
            continue;
        res->id = nla_get_u32(tb2[CTRL_ATTR_MCAST_GRP_ID]);
        break;
    };

    return NL_SKIP;
}

/**
 * handler for NL80211_CMD_GET_WIPHY results
 */
static int wiphy_info_handler(struct nl_msg *msg, void* arg)
{
    if ( !(msg && arg) ) // shouldn't happen
    {
        LOG_ERROR << "invalid arguments in nl wiphy_info handler" << std::endl;
        return NL_SKIP;
    }

    nl_state *state = (nl_state *)arg;
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct nlattr *attr = nullptr;
    struct genlmsghdr *gnlh = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    struct nl80211_vendor_cmd_info *cmd = nullptr;
    int bytes_remaining = 0;

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
          genlmsg_attrlen(gnlh,0) , NULL);

    if (tb[NL80211_ATTR_VENDOR_DATA]) {
        for_each_nested_attribute(attr, tb[NL80211_ATTR_VENDOR_DATA], bytes_remaining) {
            if (nla_len(attr) != sizeof(*cmd)) {
                /* "unexpected vendor cmd info\n" */
                continue;
            }
            cmd = (struct nl80211_vendor_cmd_info *)nla_data(attr);
            if (cmd->vendor_id == OUI_QCA &&
                cmd->subcmd ==
                QCA_NL80211_VENDOR_SUBCMD_UNSPEC) {
                state->has_wmi_pub = true;
                break;
            }
        }
    }
    return NL_SKIP;
}

/**
* handler for getting command result value
* Note: The only supported result is 32 bits field
*/
static int command_info_handler(struct nl_msg *msg, void* arg)
{
    if ( !(msg && arg) ) // shouldn't happen
    {
        LOG_ERROR << "invalid arguments in nl command_info handler" << std::endl;
        return NL_SKIP;
    }

    nl_state *state = (nl_state *)arg;
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct nlattr *attr = nullptr;
    struct genlmsghdr *gnlh = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    int bytes_remaining = 0;

    state->command_response = 0x0; // initialize with failure value

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
        genlmsg_attrlen(gnlh, 0), NULL);

    if (tb[NL80211_ATTR_VENDOR_DATA]) {
        for_each_nested_attribute(attr, tb[NL80211_ATTR_VENDOR_DATA], bytes_remaining) {
            if (nla_len(attr) != sizeof(uint32_t)) {
                /* "unexpected response\n" */
                continue;
            }
            // otherwise we get the response
            uint32_t* response = (uint32_t*)(nla_data(attr));
            if (response)
            {
                state->command_response = *response;
            }
            break;
        }
    }

    return NL_SKIP;
}

/**
 * send NL command and receive reply synchronously
 */
static OperationStatus nl_cmd_send_and_recv(
    nl_state *state,
    struct nl_msg *msg,
    int (*valid_handler)(struct nl_msg *, void *),
    void *valid_data)
{
    if ( !(state && msg && valid_data) ) // shouldn't happen
    {
        return OperationStatus(false, "invalid arguments, arguments cannot be null");
    }

    // smart pointer with custom deleter calling nl_cb_put on every return path
    std::unique_ptr<struct nl_cb, decltype(&nl_cb_put)> cb_clone(nl_cb_clone(state->cb), nl_cb_put);
    if (!cb_clone)
    {
        return OperationStatus(false, "failed to clone callback handle");
    }

    int err = nl_send_auto_complete(state->nl, msg); // returns number of bytes sent or a negative error code
    if (err < 0)
    {
        std::ostringstream oss;
        oss << "failed to send message, error " << err;
        return OperationStatus(false, oss.str());
    }

    err = 1;
    nl_cb_err(cb_clone.get(), NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb_clone.get(), NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb_clone.get(), NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    if (valid_handler)
    {
        nl_cb_set(cb_clone.get(), NL_CB_VALID, NL_CB_CUSTOM, valid_handler, valid_data);
    }

    while (err > 0)
    {
        int res = nl_recvmsgs(state->nl, cb_clone.get());
        if (res < 0)
        {
            LOG_DEBUG << "failed to send message: nl_recvmsgs failed, error " << res << std::endl;
            /* do not exit the loop since similar code in supplicant does not */
        }
    }

    if (err < 0) // updated by above callbacks
    {
        // convert known error codes sent by the host driver to operation status
        return convert_errno_to_status(err);
    }

    return OperationStatus(true);
}

/**
 * get a multicast group id for registering
 * (such as for vendor events)
 */
static OperationStatus nl_get_multicast_id(nl_state *state, const char *family, const char *group, int& group_id)
{
    if ( !(state && family && group) ) // shouldn't happen
    {
        return OperationStatus(false, "invalid arguments, arguments cannot be null");
    }

    // smart pointer with custom deleter calling nlmsg_free on every return path
    std::unique_ptr<struct nl_msg, decltype(&nlmsg_free)> msg(nlmsg_alloc(), nlmsg_free);
    if (!msg)
    {
        return OperationStatus(false, "failed to allocate nl message");
    }

    if (!genlmsg_put(msg.get(), 0, 0, genl_ctrl_resolve(state->nl, "nlctrl"),
             0, 0, CTRL_CMD_GETFAMILY, 0) ||
        nla_put_string(msg.get(), CTRL_ATTR_FAMILY_NAME, family))
    {
        return OperationStatus(false, "failed to add generic nl header to nl message");
    }

    struct family_data res = { group, -ENOENT };
    OperationStatus os = nl_cmd_send_and_recv(state, msg.get(), family_handler, &res);
    if (os)
    {
        group_id = res.id;
    }

    return os;
}

/**
 * handle for vendor events
 */
static int nl_event_handler(struct nl_msg *msg, void *arg)
{
    if ( !(msg && arg) ) // shouldn't happen
    {
        LOG_ERROR << "invalid arguments in nl event handler" << std::endl;
        return NL_SKIP;
    }

    struct driver_event_report* evt = ((nl_state *)arg)->driver_event_report_buf;
    if (!evt) // shouldn't happen
    {
        LOG_ERROR << "invalid arguments in nl event handler, driver event report buffer cannot be null" << std::endl;
        return NL_SKIP;
    }

    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
          genlmsg_attrlen(gnlh, 0) , NULL);

    if (!tb[NL80211_ATTR_VENDOR_ID] ||
        !tb[NL80211_ATTR_VENDOR_SUBCMD] ||
        !tb[NL80211_ATTR_VENDOR_DATA])
    {
        return NL_SKIP;
    }

    if (nla_get_u32(tb[NL80211_ATTR_VENDOR_ID]) != OUI_QCA)
    {
        return NL_SKIP;
    }

    struct nlattr *tb2[QCA_WLAN_VENDOR_ATTR_MAX + 1];
    if (nla_parse_nested(tb2, QCA_WLAN_VENDOR_ATTR_MAX,
                tb[NL80211_ATTR_VENDOR_DATA], NULL))
    {
        /* "failed to parse vendor command\n" */
        return NL_SKIP;
    }

    uint32_t cmd = nla_get_u32(tb[NL80211_ATTR_VENDOR_SUBCMD]);
    switch (cmd)
    {
    case QCA_NL80211_VENDOR_SUBCMD_UNSPEC:
        if (tb2[QCA_WLAN_VENDOR_ATTR_BUF])
        {
            const uint32_t len = nla_len(tb2[QCA_WLAN_VENDOR_ATTR_BUF]);
            if (len > sizeof(struct driver_event_report))
            {
                /* "event respond length is bigger than allocated %d [bytes]\n", sizeof(struct driver_event_report) */
                return NL_SKIP;
            }

            /* evt validity already tested */
            memcpy(evt, nla_data(tb2[QCA_WLAN_VENDOR_ATTR_BUF]), len);
        }
        break;
    default:
        /* "\nunknown event %d\n", cmd */
        break;
    }

    return NL_SKIP;
}

/**
 * destroy the structures for NL communication
 */
static void destroy_nl_globals(nl_state *state)
{
    if (!state)
    {
        return;
    }

    if (state->nl)
    {
        nl_socket_free(state->nl);
        state->nl = nullptr;
    }
    if (state->nl_event)
    {
        nl_socket_free(state->nl_event);
        state->nl_event = nullptr;
    }
    if (state->cb)
    {
        nl_cb_put(state->cb);
        state->cb = nullptr;
    }
    state->nl80211_id = 0;
}

/**
 * initialize structures for NL communication
 * in case of failure it is the caller responsibility to call destroy_nl_globals
 */
static OperationStatus init_nl_globals(nl_state *state)
{
    if (!state)
    {
        return OperationStatus(false, "invalid arguments, state cannot be null");
    }

    /* specify NL_CB_DEBUG instead of NL_CB_DEFAULT to get detailed traces of NL messages */
    state->cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (state->cb == nullptr)
    {
        return OperationStatus(false, "failed to allocate nl callback");
    }

    if (nl_cb_set(state->cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL) < 0)
    {
        return OperationStatus(false, "failed to set nl callback handler (no_seq_check)");
    }

    state->nl = nl_socket_alloc_cb(state->cb);
    if (state->nl == nullptr)
    {
        return OperationStatus(false, "failed to allocate nl socket");
    }

    if (genl_connect(state->nl) < 0)
    {
        return OperationStatus(false, "failed to bind nl socket to the Netlink protocol");
    }

    state->nl80211_id = genl_ctrl_resolve(state->nl, "nl80211");
    if (state->nl80211_id < 0)
    {
        std::ostringstream oss;
        oss << "failed to resolve nl80211 family id, error " << state->nl80211_id;
        return OperationStatus(false, oss.str());
    }

    state->nl_event = nl_socket_alloc_cb(state->cb);
    if (state->nl_event == nullptr)
    {
        return OperationStatus(false, "failed to allocate nl socket for events");
    }

    if (genl_connect(state->nl_event) < 0)
    {
        return OperationStatus(false, "failed to bind events nl socket to the Netlink protocol");
    }

    /* register for receiving vendor events */
    int group_id = -1;
    OperationStatus os = nl_get_multicast_id(state, "nl80211", "vendor", group_id);
    if (!os)
    {
        os.AddPrefix("could not get vendor multicast group id for nl80211 family");
        return os;
    }

    if (nl_socket_add_membership(state->nl_event, group_id) < 0)
    {
        return OperationStatus(false, "could not register for vendor events");
    }

    if (nl_socket_set_nonblocking(state->nl_event) < 0)
    {
        return OperationStatus(false, "failed to set events socket to non-blocking state");
    }

    // provide the state to be passed as last argument to the callback, it will contain the buffer address
    if (nl_cb_set(state->cb, NL_CB_VALID, NL_CB_CUSTOM, nl_event_handler, state) < 0)
    {
        return OperationStatus(false, "failed to set nl callback handler (nl_event_handler)");
    }

    return OperationStatus(true);
}

/**
 * allocate an nl_msg for sending a command
 */

static struct nl_msg *allocate_nl_cmd_msg(int family, int ifindex, int flags, uint8_t cmd)
{
    // nlmsg_alloc allocates 4KB buffer by default
    // this may not be enough for a WMI command with lengthy payload, replaced with nlmsg_alloc_size
    struct nl_msg *msg = nlmsg_alloc_size(NL_MSG_SIZE_MAX);

    if (!msg)
    {
        /* "failed to allocate nl msg\n" */
        return nullptr;
    }

    if (!genlmsg_put(msg,
              0, // pid (automatic)
              0, // sequence number (automatic)
              family, // family
              0, // user specific header length
              flags, // flags
              cmd, // command
              0) // protocol version
        )
    {
        /* "failed to init msg\n" */
        nlmsg_free(msg);
        return nullptr;
    }

    if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, (uint32_t)ifindex) < 0)
    {
        /* "failed to set interface index\n" */
        nlmsg_free(msg);
        return nullptr;
    }

    return msg;
}

/**
 * send NL command and receive reply synchronously, for
 * non-blocking sockets
 */
static OperationStatus nl_cmd_send_and_recv_nonblock(nl_state *state, struct nl_msg *msg)
{
    static const int polling_timeout_msec = 500; /* timeout is in msec. */

    if ( !(state && msg) ) // shouldn't happen
    {
        return OperationStatus(false, "invalid arguments, arguments cannot be null");
    }

    // smart pointer with custom deleter calling nl_cb_put on every return path
    std::unique_ptr<struct nl_cb, decltype(&nl_cb_put)> cb_clone(nl_cb_clone(state->cb), nl_cb_put);
    if (!cb_clone)
    {
        return OperationStatus(false, "failed to clone callback handle");
    }

    struct nl_sock *nl = state->nl;
    int err = nl_send_auto_complete(nl, msg); // returns number of bytes sent or a negative error code
    if (err < 0)
    {
        std::ostringstream oss;
        oss << "failed to send message, error " << err;
        return OperationStatus(false, oss.str());
    }

    err = 1;
    nl_cb_err(cb_clone.get(), NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb_clone.get(), NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb_clone.get(), NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

    struct pollfd fds;
    memset(&fds, 0, sizeof(fds));
    fds.fd = nl_socket_get_fd(nl);
    fds.events = POLLIN;
    while (err > 0)
    {
        int res = poll(&fds, 1, polling_timeout_msec);

        if (res == 0) // timeout
        {
            std::ostringstream oss;
            oss << "failed to send message, poll timeout of " << polling_timeout_msec << " [msec.] reached";
            return OperationStatus(false, oss.str());
        }
        else if (res < 0) // poll failure
        {
            std::ostringstream oss;
            oss << "failed to send message, poll failure: " << strerror(errno);
            return OperationStatus(false, oss.str());
        }

        if (fds.revents & POLLIN)
        {
            res = nl_recvmsgs(nl, cb_clone.get());
            if (res < 0) // shouldn't happen
            {
                std::ostringstream oss;
                oss << "failed to send message, nl_recvmsgs failed, error " << res;
                return OperationStatus(false, oss.str());
            }
        }
    }

    if (err < 0) // updated by above callbacks
    {
        // convert known error codes sent by the host driver to operation status
        return convert_errno_to_status(err);
    }

    return OperationStatus(true);
}

/**
* get publish_event capability for driver using the
* NL80211_CMD_GET_WIPHY command
*/
static OperationStatus nl_get_publish_event_capability(nl_state *state)
{
    if (!state) // shouldn't happen
    {
        return OperationStatus(false, "invalid arguments, state cannot be null");
    }

    // smart pointer with custom deleter calling nlmsg_free on every return path
    std::unique_ptr<struct nl_msg, decltype(&nlmsg_free)> msg(
        allocate_nl_cmd_msg(state->nl80211_id, state->ifindex, NLM_F_DUMP, NL80211_CMD_GET_WIPHY),
        nlmsg_free);

    if (!msg)
    {
        return OperationStatus(false, "failed to allocate nl message for GET_WIPHY command");
    }

    if (nla_put_flag(msg.get(), NL80211_ATTR_SPLIT_WIPHY_DUMP) < 0)
    {
        return OperationStatus(false, "failed to set params for GET_WIPHY command");
    }

    OperationStatus os = nl_cmd_send_and_recv(state, msg.get(), wiphy_info_handler, state);
    if (!os)
    {
        os.AddPrefix("failed to send GET_WIPHY command");
        return os;
    }

    if(!state->has_wmi_pub)
    {
        return OperationStatus(false, "nl events are not supported, new driver version required...");
    }

    return OperationStatus(true);
}

/*
 * Send command to the Driver
 * Notes:
 * Id represents driver command type (wil_nl_60g_cmd_type enumeration) which is a contract between the Driver and the command initiator.
 * Response is updated only for DRIVER_CMD_GENERIC_COMMAND.
 */
OperationStatus nl_send_driver_command(nl_state *state, uint32_t id, uint32_t bufLen, const void* pBuffer, uint32_t* pResponse)
{
    if ( !(state && pBuffer) ) // shouldn't happen
    {
        return OperationStatus(false, "invalid arguments, state and payload buffer cannot be null");
    }

    // smart pointer with custom deleter calling nlmsg_free on every return path
    std::unique_ptr<struct nl_msg, decltype(&nlmsg_free)> msg(
        allocate_nl_cmd_msg(state->nl80211_id, state->ifindex, 0, NL80211_CMD_VENDOR),
        nlmsg_free);

    if (!msg)
    {
        return OperationStatus(false, "failed to allocate nl message for GET_WIPHY");
    }

    if (nla_put_u32(msg.get(), NL80211_ATTR_VENDOR_ID, OUI_QCA) < 0 ||
        nla_put_u32(msg.get(), NL80211_ATTR_VENDOR_SUBCMD, QCA_NL80211_VENDOR_SUBCMD_UNSPEC) < 0)
    {
        return OperationStatus(false, "unable to set parameters for QCA_NL80211_VENDOR_SUBCMD_UNSPEC");
    }

    struct nlattr *vendor_data = nla_nest_start(msg.get(), NL80211_ATTR_VENDOR_DATA);
    if (!vendor_data)
    {
        return OperationStatus(false, "failed to start a new level of nested attributes for NL80211_ATTR_VENDOR_DATA");
    }

    if (nla_put_u32(msg.get(), QCA_WLAN_VENDOR_ATTR_TYPE, id))
    {
        return OperationStatus(false, "failed to set QCA_WLAN_VENDOR_ATTR_TYPE attribute (command Id)");
    }

    if (nla_put(msg.get(), QCA_WLAN_VENDOR_ATTR_BUF, (int)bufLen, pBuffer) < 0)
    {
        return OperationStatus(false, " failed to set QCA_WLAN_VENDOR_ATTR_BUF attribute (command payload buffer)");
    }

    nla_nest_end(msg.get(), vendor_data); /* always returns zero */

    if (pResponse && id == DRIVER_CMD_GENERIC_COMMAND) // response required, blocking send-receive
    {
        OperationStatus os = nl_cmd_send_and_recv(state, msg.get(), command_info_handler, state);
        if (os)
        {
            *pResponse = state->command_response;
        }

        return os;
    }

    // otherwise, no response expected, non blocking send-receive
    return nl_cmd_send_and_recv_nonblock(state, msg.get());
}

OperationStatus nl_get_driver_event(nl_state *state, int cancelationFd, struct driver_event_report* pMessageBuf)
{
    if ( !(state && pMessageBuf) ) // shouldn't happen
    {
        return OperationStatus(false, "invalid arguments, arguments cannot be null");
    }

    /* 'cancelationFd' is a file descriptor for one of the sockets from the cancellation sockets pair */
    /* sockets pair serves as a pipe - a value written to one of its sockets, is also written to the second one */
    struct pollfd fds[2];
    memset(fds, 0, sizeof(fds));
    fds[0].fd = nl_socket_get_fd(state->nl_event);
    fds[0].events |= POLLIN;
    fds[1].fd = cancelationFd;
    fds[1].events |= POLLIN;

    int res = poll(fds, 2, -1); // infinite timeout
    if (res < 0)
    {
        std::ostringstream oss;
        oss << "failed to get driver event, poll failure: " << strerror(errno);
        return OperationStatus(false, oss.str());
    }

    if (fds[0].revents & POLLIN)
    {
        state->driver_event_report_buf = pMessageBuf; /* store report pointer to be used inside the callback*/
        res = nl_recvmsgs(state->nl_event, state->cb);
        if (res < 0)
        {
            std::ostringstream oss;
            oss << "failed to get driver event, nl_recvmsgs failed, error " << res;
            return OperationStatus(false, oss.str());
        }
    }
    else
    {
        return OperationStatus(false, "failed to get driver event, no event received");
    }

    return OperationStatus(true);
}

/* Initialize the netlink interface */
OperationStatus nl_initialize(const char* interfaceName, nl_state** ppState)
{
    if ( !(interfaceName && ppState) )
    {
        return OperationStatus(false, "invalid arguments, interface name and nl_state pointer cannot be null");
    }

    int ifindex = if_nametoindex(interfaceName);
    if (ifindex == 0)
    {
        std::ostringstream oss;
        oss << "unknown WIGIG interface " << interfaceName;
        return OperationStatus(false, oss.str());
    }

    nl_state* pState = (nl_state*)malloc(sizeof(nl_state));
    if (!pState)
    {
        return OperationStatus(false, "cannot allocate netlink state descriptor");
    }

    memset(pState, 0, sizeof(*pState));
    pState->ifindex = ifindex;

    /* initialize structures for NL communication */
    OperationStatus os = init_nl_globals(pState);
    if (!os)
    {
        nl_release(pState); /*it is the caller responsibility */
        return os;
    }

    os = nl_get_publish_event_capability(pState);
    if (!os)
    {
        nl_release(pState);
        return os;
    }

    *ppState = pState;
    return OperationStatus(true);
}

void nl_release(nl_state* pState)
{
    if (!pState)
    {
        return;
    }

    destroy_nl_globals(pState);
    free(pState);
}

/* Get bitmask of the driver capabilities */
OperationStatus nl_get_capabilities(nl_state_ptr pState, uint32_t& driverCapabilities)
{
    uint32_t buf = (uint32_t)NL_60G_GEN_GET_DRIVER_CAPA; // payload contains only the generic command id
    return nl_send_driver_command(pState, (uint32_t)DRIVER_CMD_GENERIC_COMMAND, sizeof(buf), &buf, &driverCapabilities);
}

/* Request to Enable/Disable events publishing to the user space */
OperationStatus nl_enable_driver_events_transport(nl_state_ptr pState, bool enable)
{
    // send buffer of 4 bytes with 1 to enable and zero to disable
    uint32_t buf = enable ? (uint32_t)1U : (uint32_t)0U;
    return nl_send_driver_command(pState, (uint32_t)DRIVER_CMD_REGISTER, sizeof(buf), &buf, nullptr /*no response*/);
}

/* FW Reset through generic driver command */
OperationStatus nl_fw_reset(nl_state_ptr pState)
{
    uint32_t buf = (uint32_t)NL_60G_GEN_FW_RESET; // payload contains only the generic command id
    return nl_send_driver_command(pState, (uint32_t)DRIVER_CMD_GENERIC_COMMAND, sizeof(buf), &buf, nullptr /*no response*/);
}
