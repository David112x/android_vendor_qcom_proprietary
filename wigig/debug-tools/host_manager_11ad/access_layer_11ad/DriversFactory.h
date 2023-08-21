/*
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>
#include <functional>

class DriverAPI;
enum class DeviceType : int;
#ifdef __linux
    class UnixNetworkInterfaceMonitor;
#endif

// callbacks for notification on added/removed devices
using DeviceAddedFunc = std::function<void(std::vector<std::unique_ptr<DriverAPI>>&)>;
using DeviceRemovedFunc = std::function<void(const std::set<std::string>&)>;

class DriversFactory final
{
public:
    DriversFactory(DeviceAddedFunc deviceAddedFunc, DeviceRemovedFunc deviceRemovedFunc);
    ~DriversFactory();

    // Get valid drivers for non-monitored interfaces
    std::vector<std::unique_ptr<DriverAPI>> GetNonMonitoredDrivers();

    // Update non-monitored interfaces when a change detected
    // \remarks Notifies through callbacks when enumeration result is different from the given list of interfaces
    void UpdateNonMonitoredDevices(const std::vector<std::pair<DeviceType, std::string>>& nonMonitoredInterfaces);

    std::vector<std::unique_ptr<DriverAPI>> StartNetworkInterfaceMonitors();

    bool IsPeriodicEnumerationNeeded() const { return !m_enumerators.empty(); }

private:
    // Callbacks for network interface changes
    void OnInterfacesAdded(DeviceType deviceType, const std::set<std::string>& interfacesAdded) const;
    void OnInterfacesRemoved(DeviceType deviceType, const std::set<std::string>& interfacesRemoved) const;

    // Enumerators for non-monitored interfaces
    using EnumerationFunc = std::function<std::set<std::string>()>;
    std::vector<std::pair<DeviceType, EnumerationFunc>> m_enumerators;
    // Network interface monitors
#ifdef __linux
    std::unique_ptr<UnixNetworkInterfaceMonitor> m_unixNetworkInterfaceMonitor;
#endif

    // Callbacks to be called to notify on added/removed devices
    DeviceAddedFunc   m_deviceAddedFunc;
    DeviceRemovedFunc m_deviceRemovedFunc;
};
