/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include <memory>
#include <set>
#include <vector>
#include <string>
#include <utility>   // for make_pair
#include <algorithm> // for sort

#include "DriversFactory.h"
#include "DriverAPI.h"
#include "DummyDriver.h"
#include "DebugLogger.h"

#ifdef _WINDOWS
    #include "WindowsPciDriver.h"
    #include "JTagDriver.h"
#else
    #include "UnixPciDriver.h"
    #include "UnixNetworkInterfaceMonitor.h"
    #include "UnixRtlDriver.h"
    #include "RtlSimEnumerator.h"
#endif // _WINDOWS


using namespace std;

namespace
{
    // local helper to create relevant driver instance according to the provided device type
    std::unique_ptr<DriverAPI> CreateDriver(DeviceType deviceType, const std::string& interfaceName)
    {
        switch (deviceType)
        {
#ifdef _WINDOWS
        case DeviceType::JTAG:
            return std::unique_ptr<JTagDriver>(new JTagDriver(interfaceName));
#endif // WINDOWS
        case DeviceType::DUMMY:
            return std::unique_ptr<DummyDriver>(new DummyDriver(interfaceName));
        case DeviceType::PCI:
#ifdef _WINDOWS
            return std::unique_ptr<WindowsDriverAPI>(new WindowsDriverAPI(interfaceName));
#else
            return std::unique_ptr<UnixPciDriver>(new UnixPciDriver(interfaceName));
        case DeviceType::RTL:
            return std::unique_ptr<UnixRtlDriver>(new UnixRtlDriver(interfaceName));
#endif // WINDOWS

        default:
            LOG_ERROR << "Got invalid device type " << static_cast<int>(deviceType) << ". Return an empty driver" << std::endl;
            return std::unique_ptr<DriverAPI>();
        }
    }

    std::vector<std::unique_ptr<DriverAPI>> TryCreateDriversForInterfaces(DeviceType deviceType, const std::set<std::string>& interfaceNames)
    {
        std::vector<std::unique_ptr<DriverAPI>> drivers;
        drivers.reserve(interfaceNames.size());

        for (const auto& interfaceName : interfaceNames)
        {
            // try create driver
            auto deviceDriver = CreateDriver(deviceType, interfaceName);
            if (deviceDriver && deviceDriver->IsValid()) // ready and can read
            {
                drivers.push_back(std::move(deviceDriver));
            }
        }

        return drivers;
    }
}

DriversFactory::DriversFactory(DeviceAddedFunc deviceAddedFunc, DeviceRemovedFunc deviceRemovedFunc) :
    m_deviceAddedFunc(std::move(deviceAddedFunc)),
    m_deviceRemovedFunc(std::move(deviceRemovedFunc))
{
    if ( !(m_deviceAddedFunc && m_deviceRemovedFunc) ) // shouldn't happen
    {
        LOG_ERROR << __FUNCTION__ << ": Cannot start network interface monitoring with invalid callbacks" << std::endl;
    }

    // initialize list of enumerators for the current system
#ifdef _WINDOWS
    m_enumerators.emplace_back(make_pair(DeviceType::PCI, WindowsDriverAPI::Enumerate));
    m_enumerators.emplace_back(make_pair(DeviceType::JTAG, JTagDriver::Enumerate));
#elif RTL_SIMULATION
    m_enumerators.emplace_back(make_pair(DeviceType::RTL, RtlSimEnumerator::Enumerate));
#endif

#ifdef _UseDummyDevice
    m_enumerators.emplace_back(make_pair(DeviceType::DUMMY, DummyDriver::Enumerate));
#endif

    // initialize list of network monitors for the current system
#ifdef __linux
    m_unixNetworkInterfaceMonitor.reset(new UnixNetworkInterfaceMonitor());
#endif
}

DriversFactory::~DriversFactory() = default; // to allow fwd declaration for unique_ptr

std::vector<std::unique_ptr<DriverAPI>> DriversFactory::GetNonMonitoredDrivers()
{
    std::vector<std::unique_ptr<DriverAPI>> enumeratedDrivers;

    for (const auto& enumerator : m_enumerators)
    {
        auto deviceDrivers = TryCreateDriversForInterfaces(enumerator.first, enumerator.second());
        for (auto& deviceDriver : deviceDrivers)
        {
            enumeratedDrivers.push_back(std::move(deviceDriver));
        }
    }

    return enumeratedDrivers;
}

// Update non-monitored interfaces when a change detected
// \remarks Notifies through callbacks when enumeration result is different from the given list of interfaces
void DriversFactory::UpdateNonMonitoredDevices(const std::vector<std::pair<DeviceType, std::string>>& nonMonitoredInterfaces)
{
    // enumerate non-monitored interfaces and notify when a change detected
    for (const auto& enumerator : m_enumerators)
    {
        // filter out interfaces for other device types
        std::vector<std::string> filteredInterfaces;
        for(const auto& interfaceInfo : nonMonitoredInterfaces)
        {
            if (interfaceInfo.first == enumerator.first)
            {
                filteredInterfaces.push_back(interfaceInfo.second);
            }
        }

        // sort input range as required for set_difference
        std::sort(filteredInterfaces.begin(), filteredInterfaces.end());

        // get new enumeration
        auto enumeratedInterfaces = enumerator.second();

        // updated \ original => added
        decltype(enumeratedInterfaces) interfacesAdded;
        std::set_difference(enumeratedInterfaces.cbegin(), enumeratedInterfaces.cend(),
            filteredInterfaces.cbegin(), filteredInterfaces.cend(),
            std::inserter(interfacesAdded, interfacesAdded.begin()));
        if (!interfacesAdded.empty())
        {
            OnInterfacesAdded(enumerator.first, interfacesAdded);
        }

        // original \ updated => removed
        decltype(enumeratedInterfaces) interfacesRemoved;
        std::set_difference(filteredInterfaces.cbegin(), filteredInterfaces.cend(),
            enumeratedInterfaces.cbegin(), enumeratedInterfaces.cend(),
            std::inserter(interfacesRemoved, interfacesRemoved.begin()));
        if (!interfacesRemoved.empty())
        {
            OnInterfacesRemoved(enumerator.first, interfacesRemoved);
        }
    }
}

// Enumerate monitored interfaces and create corresponding device drivers and start interface monitoring
// \remarks Expected to be called once during initialization
std::vector<std::unique_ptr<DriverAPI>> DriversFactory::StartNetworkInterfaceMonitors()
{
#ifdef __linux
        using namespace std::placeholders;
        std::set<std::string> ifnames = m_unixNetworkInterfaceMonitor->StartMonitoring(
                std::bind(&DriversFactory::OnInterfacesAdded, this, _1, _2),
                std::bind(&DriversFactory::OnInterfacesRemoved, this, _1, _2));

    return TryCreateDriversForInterfaces(DeviceType::PCI, ifnames);
#else
    return std::vector<std::unique_ptr<DriverAPI>>();
#endif
}

void DriversFactory::OnInterfacesAdded(DeviceType deviceType, const std::set<std::string>& interfacesAdded) const
{
    if (!m_deviceAddedFunc)
    {
        LOG_DEBUG << "Cannot report about added interface, invalid callback was passed in construction" << endl;
        return;
    }

    // return only valid drivers
    auto addedDeviceDrivers = TryCreateDriversForInterfaces(deviceType, interfacesAdded);
    if (addedDeviceDrivers.empty())
    {
        return;
    }
    m_deviceAddedFunc(addedDeviceDrivers);
}

void DriversFactory::OnInterfacesRemoved(DeviceType deviceType, const std::set<std::string>& interfacesRemoved) const
{
    if (!m_deviceRemovedFunc)
    {
        LOG_DEBUG << "Cannot report about added interface, invalid callback was passed in construction" << endl;
        return;
    }

    (void)deviceType;
    m_deviceRemovedFunc(interfacesRemoved);
}
