/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <sstream>

#include "UnixDriverEventsTransport.h"
#include "DriverAPI.h"
#include "EventsDefinitions.h"
#include "FwStateProvider.h"
#include "DebugLogger.h"
#include "Host.h"
#include "HostInfo.h"
#include "nl_services.h"

using namespace std;

namespace
{
    // === FW Health State Event structures for private usage, aligned with wil6210 driver === //
    enum class wil_nl_60g_generic_evt
    {
        NL_60G_GEN_EVT_FW_STATE = 0,
        NL_60G_GEN_EVT_AUTO_RADAR_DATA_READY,
    };

    struct wil_nl_60g_fw_state_event
    {
        wil_nl_60g_generic_evt evt_id;
        wil_fw_state fw_sts;
    };

    /* Definition of WMI command/event payload. In case of event, it's the content of buffer in driver_event_report. */
    struct wil_nl_60g_send_receive_wmi {
        uint32_t cmd_id;    /* command or event id */
        uint16_t reserved;  /* reserved for context id, not in use */
        uint8_t  dev_id;    /* mid, not in use */
        uint16_t buf_len;
        uint8_t  buf[];
    } __attribute__((packed));

    struct wil_nl_60g_send_receive_rmi {
        uint8_t version;
        uint8_t reserved_1[3];
        uint16_t cmd_id;    /* command or event id */
        uint16_t buf_len;   /* payload length in bytes, without header */
        uint32_t token;     /* command or associated command token, zero means NA */
        uint32_t timestamp;
        uint32_t reserved_2[2];
        uint8_t  buf[];
    } __attribute__((packed));
}

// ======================================================================================= //

UnixDriverEventsTransport::UnixDriverEventsTransport(const std::string& interfaceName) :
    m_nlState(nullptr),
    m_exitSockets{ { -1, -1 } }, // aggregate initialization
    m_interfaceName(interfaceName),
    m_stopRunningDriverControlEventsThread(false)
{
    m_ignoredFwEvents = Host::GetHost().GetHostInfo().Get3ppIgnoreEvents();
}

// Initialize driver transport and update the capability mask
void UnixDriverEventsTransport::InitializeTransports(bitset<32>& driverCapabilities)
{
    driverCapabilities.reset(); // reset-all-bits initialization for case we fail to get it from the driver
    driverCapabilities.set(static_cast<size_t>(wil_nl_60g_driver_capa::NL_60G_DRIVER_CAPA_IOCTL_WRITE)); // no actual knowledge

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, m_exitSockets.data()) == -1) // shouldn't happen
    {
        LOG_ERROR << "On Device " << m_interfaceName << ", failed to create cancellation socket pair, abort netlink interface initialization" << endl;
        return;
    }

    OperationStatus os = nl_initialize(m_interfaceName.c_str(), &m_nlState);
    if (!os) // can happen for old driver that doesn't support driver events
    {
        CloseSocketPair(); // release the exit sockets
                           // display warning instead of error to prevent SI test failure
        LOG_WARNING << "On Device " << m_interfaceName << ", failed to initialize netlink interface, reason: " << os.GetStatusMessage() << endl;
        return;
    }

    // get capabilities from the driver
    uint32_t capabilityMask = 0U;
    os = nl_get_capabilities(m_nlState, capabilityMask);
    if (os)
    {
        driverCapabilities = bitset<32>(capabilityMask);
        return;
    }

    // otherwise, GET_DRIVER_CAPA command is not supported on the driver side or it's a transport error
    // netlink is initialized, set WMI transport & IOCTL_WRITE bits anyway (backward compatibility with driver before NL_60G_GEN_GET_DRIVER_CAPA)
    // IOCTL_WRITE was set during init
    driverCapabilities.set(static_cast<size_t>(wil_nl_60g_driver_capa::NL_60G_DRIVER_CAPA_WMI));

    LOG_WARNING << "On Device " << m_interfaceName << ", failed to get driver capabilities (driver upgrade may be required): " << os.GetStatusMessage() << endl;
}

// Register for driver events
void UnixDriverEventsTransport::RegisterForDriverEvents(FwStateProvider* fwStateProvider)
{
    if (!m_nlState)
    {
        // working with Debug mailbox, do nothing
        LOG_DEBUG << m_interfaceName << " is working with Debug mailbox, registration for driver events is not required" << endl;
        return;
    }

    // Creating a thread to wait on the netlink blocking poll, pass the interface name
    // To prevent miss of published events the thread is created before asking to send events to the user space
    m_waitOnSignalThread = thread(&UnixDriverEventsTransport::DriverControlEventsThread, this, fwStateProvider);

    // Enable events transport on the driver side, i.e. ask to publish them to the user space
    const OperationStatus os = nl_enable_driver_events_transport(m_nlState, true /*enable*/);
    if (!os)
    {
        // Do not halt the flow on error because of backward compatibility with previous driver version before this feature
        LOG_WARNING << "On Device " << m_interfaceName
            << ", cannot enable events transport (driver upgrade may be required): "
            << os.GetStatusMessage() << endl;
    }
}

// Send event to the driver
OperationStatus UnixDriverEventsTransport::SendDriverCommand(uint32_t id, const void *pInBuf, uint32_t inBufSize, void *pOutBuf, uint32_t outBufSize)
{
    LOG_ASSERT(pInBuf); // just in case, pInBuf validity should be tested by the caller

    if (pOutBuf && outBufSize != sizeof(uint32_t))
    {   // when response required, the only supported response size is 32 bit
        ostringstream oss;
        oss << "cannot send driver command for device " << m_interfaceName
            << ", given output buffer size " << outBufSize << " (Bytes) does not match expected " << sizeof(uint32_t);
        return OperationStatus(false, oss.str());
    }

    OperationStatus os = nl_send_driver_command(m_nlState, id, inBufSize, pInBuf, reinterpret_cast<uint32_t*>(pOutBuf));
    if (!os)
    {
        ostringstream oss;
        oss << "failed to send driver command " << Hex<>(id) << " for device " << m_interfaceName;
        os.AddPrefix(oss.str());
    }

    return os;
}

bool UnixDriverEventsTransport::ShouldIgnoreFwEvent(const driver_event_report& driverEventReport)
{
    if (driverEventReport.evt_type != DRIVER_EVENT_FW_WMI_EVENT)
    {
        return false;
    }

    const auto payload = reinterpret_cast<const wil_nl_60g_send_receive_wmi*>(driverEventReport.buf);
    auto it = m_ignoredFwEvents.find(payload->cmd_id);
    return it != m_ignoredFwEvents.end();
}

// Main loop of polling for incoming driver events
void UnixDriverEventsTransport::DriverControlEventsThread(FwStateProvider* fwStateProvider)
{
    struct driver_event_report driverEvent;

    while (!m_stopRunningDriverControlEventsThread.load())
    {
        memset(&driverEvent, 0, sizeof(driverEvent));

        // blocking call to fetch the next event
        const OperationStatus os = nl_get_driver_event(m_nlState, m_exitSockets[1] /* cancellation socket file descriptor */, &driverEvent);

        if (m_stopRunningDriverControlEventsThread.load()) // asked to quit
        {
            // Do not disable events transport on the Driver side. It may cause a disruption for other devices.
            // Previous code for reference:
            // Ignore the possible error because of backward compatibility with previous Driver version before this feature
            // nl_enable_driver_events_transport(m_nlState, false /*disable*/);
            break;
        }

        if (!os)
        {
            LOG_ERROR << "On Device " << m_interfaceName << ", failed to get driver event: " << os.GetStatusMessage() << std::endl;
            continue;
        }

        if (driverEvent.buf_len > DRIVER_MSG_MAX_LEN)
        {
            // shouldn't happen
            LOG_ERROR << "On Device " << m_interfaceName << ", got invalid driver event, buffer length is " << driverEvent.buf_len << std::endl;
            continue;
        }

        // handle driver event or push it to connected clients
        if (!HandleDriverEvent(fwStateProvider, driverEvent))
        {
            if (ShouldIgnoreFwEvent(driverEvent))
            {
                continue;
            }
            Host::GetHost().PushEvent(DriverEvent(m_interfaceName, static_cast<int>(driverEvent.evt_type),
                driverEvent.evt_id, driverEvent.listener_id, driverEvent.buf_len, driverEvent.buf));
        }

        // print received event ids only after ignoring irrelevant events
        if (g_LogConfig.ShouldPrint(LOG_SEV_DEBUG))
        {
            if (driverEvent.evt_type == DRIVER_EVENT_FW_WMI_EVENT)
            {
                uint32_t eventId = (reinterpret_cast<const wil_nl_60g_send_receive_wmi*>(driverEvent.buf))->cmd_id;
                LOG_DEBUG << "On Device " << m_interfaceName << ", received WMI event, id=" << Hex<>(eventId) << std::endl;
            }
            else if (driverEvent.evt_type == DRIVER_EVENT_FW_RMI_EVENT)
            {
                uint16_t eventId = (reinterpret_cast<const wil_nl_60g_send_receive_rmi*>(driverEvent.buf))->cmd_id;
                LOG_DEBUG << "On Device " << m_interfaceName << ", received RMI event, id=" << Hex<>(eventId) << std::endl;
            }
        }
    }

    LOG_DEBUG << "DriverControlEventsThread for device " << m_interfaceName << " was asked to quit." << std::endl;
}

// \returns True if event was handled
bool UnixDriverEventsTransport::HandleDriverEvent(FwStateProvider* fwStateProvider, const driver_event_report& driverEventReport)
{
    if (driverEventReport.evt_type != DRIVER_EVENT_DRIVER_GENERIC_EVENT)
    {
        return false;
    }

    const auto payload = reinterpret_cast<const wil_nl_60g_fw_state_event*>(driverEventReport.buf);

    if (payload->evt_id == wil_nl_60g_generic_evt::NL_60G_GEN_EVT_FW_STATE)
    {
        if (driverEventReport.buf_len != sizeof(wil_nl_60g_fw_state_event))
        {
            // shouldn't happen
            LOG_ERROR << m_interfaceName << " got invalid FW_STS_EVENT, expected " << sizeof(wil_nl_60g_fw_state_event)
                << "Bytes of payload, got " << driverEventReport.buf_len << endl;
            return true;
        }

        fwStateProvider->OnFwHealthStateChanged(payload->fw_sts);
        return true;
    }
    else if (payload->evt_id == wil_nl_60g_generic_evt::NL_60G_GEN_EVT_AUTO_RADAR_DATA_READY)
    {
        // ignore, mark handled
        return true;
    }
    // handle other generic driver event...

    // Not an error, may happen if working with a newer driver version
    LOG_WARNING << "On Device " << m_interfaceName << ", received unsupported driver generic event (type=" << static_cast<uint32_t>(payload->evt_id)
        << "), consider upgrading host_manager_11ad" << endl;
    return false; // was not handled...
}

// Cancel blocking polling for incoming events to allow graceful thread shutdown
void UnixDriverEventsTransport::StopDriverControlEventsBlocking()
{
    // Stop driver control event thread:
    m_stopRunningDriverControlEventsThread.store(true);

    // unblocks the monitor receive socket for termination
    // sockets pair serves as a pipe - a value written to one of its sockets, is also written to the second one
    // actual value written has no importance
    if (m_exitSockets[0] >= 0)
    {
        write(m_exitSockets[0], "T", 1);
    }

    if (m_waitOnSignalThread.joinable())
    {
        m_waitOnSignalThread.join();
    }

    // release netlink state
    nl_release(m_nlState);

    // release sockets from the sockets pair used to stop the driver events thread
    CloseSocketPair();
}

// Release sockets from the sockets pair used to stop the driver events thread
void UnixDriverEventsTransport::CloseSocketPair()
{
    if (m_exitSockets[0] >= 0)
    {
        close(m_exitSockets[0]);
        m_exitSockets[0] = -1;
    }
    if (m_exitSockets[1] >= 0)
    {
        close(m_exitSockets[1]);
        m_exitSockets[1] = -1;
    }
}

// Reset FW through generic drive command
// \remark May fail when:
// - Working with Debug mailbox
// - Driver in use does not support NL_60G_GEN_FW_RESET generic command
// - Not WMI_ONLY FW in use
OperationStatus UnixDriverEventsTransport::TryFwReset()
{
    LOG_DEBUG << __FUNCTION__ << endl;

    if (!m_nlState)
    {
        ostringstream oss;
        oss << "on device " << m_interfaceName << ", FW reset is not supported for Debug mailbox";
        return OperationStatus(false, oss.str());
    }

    OperationStatus os = nl_fw_reset(m_nlState);
    if (!os)
    {
        ostringstream oss;
        oss << "on device " << m_interfaceName << ", ";
        os.AddPrefix(oss.str(), false /*no separator*/);
    }

    return os;
}
