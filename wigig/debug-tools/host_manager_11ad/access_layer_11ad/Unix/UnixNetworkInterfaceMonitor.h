/*
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include <array>
#include <vector>
#include <thread>
#include <string>
#include <set>
#include <map>
#include <mutex>
#include <functional> // for std::function
#include <atomic>

// forward declarations
struct nlmsghdr;
enum class DeviceType : int;

// callback for notification on added/removed network interface
using InterfaceChangedFunc = std::function<void(DeviceType deviceType, const std::set<std::string>&)>;

// Singleton class to enumerate and monitor WIGIG network interfaces
class UnixNetworkInterfaceMonitor final
{
public:
    UnixNetworkInterfaceMonitor();
    ~UnixNetworkInterfaceMonitor();

    // Perform initial enumeration and start monitoring WIGIG interface changes
    std::set<std::string> StartMonitoring(InterfaceChangedFunc interfaceAddedFunc,
                                          InterfaceChangedFunc interfaceRemovedFunc);

    std::set<std::string> GetActiveInterfaces() const;

private:
    mutable std::mutex m_mutex;             // threads synchronization
    const DeviceType m_deviceType;
    int m_netlinkSocket;                    // file descriptor for the open netlink socket
    std::array<int, 2U> m_exitSockets;      // exit socket used for graceful shutdown
    std::vector<uint8_t> m_eventsBuffer;    // buffer for received events
    std::set<std::string> m_currentInterfaceNames;
    std::map<std::string, std::string> m_currentEndPoints; // map from End Point to its ifname
    std::atomic<bool> m_stopNetworkInterfaceMonitorThread;

    InterfaceChangedFunc m_interfaceAddedFunc;
    InterfaceChangedFunc m_interfaceRemovedFunc;

    std::set<std::string> InitializeCurrentInterfaceNames();
    void StopMonitorThread();
    void CloseSocketPair();
    bool SetupNetlinkSocket();
    void NetworkInterfaceMonitorThread();
    bool WaitForEventsBlocking();
    bool ReadAndHandleEvents();
    int ReadMessagesToBuffer();
    bool ParseAndHandleMessagesInBuffer(int count);
    bool ParseAndHandleSingleMessage(const struct nlmsghdr *nh, const char* messageName);
    void HandleSingleMessage(const char* interfaceName, bool isNewLink);
    bool IsInterfaceRelevant(const char* interfaceName, std::string& endPointName) const;
    void RemoveEndPointIfOwner(const char* interfaceName);

    std::thread m_monitorThread;
};
