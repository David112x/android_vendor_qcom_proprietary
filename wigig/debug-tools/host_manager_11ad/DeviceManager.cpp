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

#include <chrono>
#include <sstream>
#include <array>
#include <mutex>

#include "DeviceManager.h"
#include "Utils.h"
#include "EventsDefinitions.h"
#include "DriversFactory.h"
#include "DebugLogger.h"
#include "Host.h"
#include "TaskManager.h"
#include "Device.h"
#include "DriverAPI.h"
#include "HostAndDeviceDataDefinitions.h"
#include "FwInfoDefinitions.h"
#include "FwStateProvider.h"
#include "PersistentConfiguration.h"
#include "LogCollector/LogCollectorService.h"

using namespace std;
using namespace log_collector;
namespace
{
#ifdef RTL_SIMULATION
    const int ENUMERATION_INTERVAL_MSEC = 60000;
#else
    const int ENUMERATION_INTERVAL_MSEC = 500;
#endif
}

// Initialize translation maps for the front-end data
const static std::string sc_noRfStr("NO_RF");
const std::unordered_map<int, std::string> DeviceManager::m_rfTypeToString = { {0, sc_noRfStr }, {1, "MARLON"}, {2, "SPR-R"}, {3, "TLN-A1"}, { 4, "TLN-A2" }, { 5, "BRL-A1" } };

const static int sc_TalynMaBasebandType = 8;
const std::unordered_map<int, std::string> DeviceManager::m_basebandTypeToString =
{
    { 0, "UNKNOWN" }, { 1, "MAR-DB1" }, { 2, "MAR-DB2" },
    { 3, "SPR-A0"  }, { 4, "SPR-A1"  }, { 5, "SPR-B0"  },
    { 6, "SPR-C0"  }, { 7, "SPR-D0"  }, { sc_TalynMaBasebandType, "TLN-M-A0"  },
    { 9, "TLN-M-B0" }, { 10, "TLN-M-C0" }, { 20, "BRL-P1"}
};

DeviceManager::DeviceManager() :
    m_enumerationPollingIntervalMs(ENUMERATION_INTERVAL_MSEC),
    m_enumerationTaskName("enumeration"),
    m_driversFactory([this](std::vector<std::unique_ptr<DriverAPI>>& deviceDrivers){ OnDeviceAdded(deviceDrivers); },
                     [this](const std::set<std::string>& devicesNames){ OnDeviceRemoved(devicesNames); })
{}

void DeviceManager::Init()
{
    // Force first enumeration to have it completed before host starts responding to requests
    // create devices for unmonitored interfaces
    auto unMonitoredDrivers = m_driversFactory.GetNonMonitoredDrivers();
    for (auto& deviceDriver : unMonitoredDrivers)
    {
        CreateDevice(std::move(deviceDriver));
    }
    // create devices for monitored interfaces
    auto monitoredDrivers = m_driversFactory.StartNetworkInterfaceMonitors();
    for (auto& deviceDriver : monitoredDrivers)
    {
        CreateDevice(std::move(deviceDriver));
    }

    // Register for periodic update of non monitored devices, if any
    if (m_driversFactory.IsPeriodicEnumerationNeeded())
    {

        if (!Host::GetHost().GetTaskManager().RegisterTask(
            std::bind(&DeviceManager::UpdateNonMonitoredDevices, this), m_enumerationTaskName, std::chrono::milliseconds(m_enumerationPollingIntervalMs)))
        {
            LOG_ERROR << "Cannot track changes in connected devices" << endl;
        }
    }
}

DeviceManager::~DeviceManager()
{
    if (m_driversFactory.IsPeriodicEnumerationNeeded())
    {
        Host::GetHost().GetTaskManager().UnregisterTaskBlocking(m_enumerationTaskName);
    }
}

void DeviceManager::OnDeviceAdded(std::vector<std::unique_ptr<DriverAPI>>& deviceDriversAdded)
{
    for (auto& deviceDriver : deviceDriversAdded)
    {
        CreateDevice(std::move(deviceDriver));
    }
}
void DeviceManager::OnDeviceRemoved(const std::set<std::string>& devicesRemoved)
{
    for (auto& device : devicesRemoved)
    {
        LOG_DEBUG << __FUNCTION__ << ": interface for device " << device << " was removed, deleting..." << std::endl;
        DeleteDevice(device);
    }
}

vector<std::string> DeviceManager::GetDeviceNames() const
{
    vector<std::string> deviceNames;
    lock_guard<mutex> lock(m_connectedDevicesMutex);
    deviceNames.reserve(m_devices.size());
    for (const auto& device : m_devices)
    {
        deviceNames.push_back(device.first);
    }

    return deviceNames;
}

OperationStatus DeviceManager::GetDeviceByName(const string& deviceName, shared_ptr<Device>& spDevice)
{
    lock_guard<mutex> lock(m_connectedDevicesMutex);

    const auto deviceIter = m_devices.find(deviceName);
    if (deviceIter != m_devices.cend())
    {
        spDevice = deviceIter->second;
        return OperationStatus(true);
    }

    ostringstream oss;
    oss << "Device " << deviceName << " not found";
    return OperationStatus(false, oss.str());
}

bool DeviceManager::DoesDeviceExist(const std::string& deviceName)
{
    lock_guard<mutex> lock(m_connectedDevicesMutex);
    return m_devices.find(deviceName) != m_devices.cend();
}

// Validate device existence, silent mode and given address validity
// \param[out] spDevice Shared pointer to the found device
OperationStatus DeviceManager::ValidateIoOperation(const string& deviceName, DWORD address, shared_ptr<Device>& spDevice)
{
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    if (spDevice->GetSilenceMode())
    {
        ostringstream oss;
        oss << "Device " << deviceName << " is in silent mode";
        return OperationStatus(false, oss.str());
    }

    if ((0 == address) || (0 != address % 4) || (0xFFFFFFFF == address))
    {
        ostringstream oss;
        oss << "Device " << deviceName << ", provided invalid address " << Address(address);
        return OperationStatus(false, oss.str());
    }

    return OperationStatus(true);
}

OperationStatus DeviceManager::Read(const string& deviceName, DWORD address, DWORD& value)
{
    value = Utils::REGISTER_DEFAULT_VALUE;

    shared_ptr<Device> spDevice;
    OperationStatus os = ValidateIoOperation(deviceName, address, spDevice);
    if (!os)
    {
        return os;
    }

    return spDevice->GetDriver()->Read(address, value);
}

OperationStatus DeviceManager::Write(const string& deviceName, DWORD address, DWORD value)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = ValidateIoOperation(deviceName, address, spDevice);
    if (!os)
    {
        return os;
    }

    return spDevice->GetDriver()->Write(address, value);
}

OperationStatus DeviceManager::ReadBlock(const string& deviceName, DWORD address, DWORD blockSize, vector<DWORD>& values)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = ValidateIoOperation(deviceName, address, spDevice);
    if (!os)
    {
        return os;
    }

    return spDevice->GetDriver()->ReadBlock(address, blockSize, values);
}

OperationStatus DeviceManager::ReadRadarData(const std::string& deviceName, DWORD maxBlockSize, vector<DWORD>& values)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = ValidateIoOperation(deviceName, 0x4 /*dummy*/, spDevice);
    if (!os)
    {
        return os;
    }

    return spDevice->GetDriver()->ReadRadarData(maxBlockSize, values);
}

OperationStatus DeviceManager::WriteBlock(const string& deviceName, DWORD address, const vector<DWORD>& values)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = ValidateIoOperation(deviceName, address, spDevice);
    if (!os)
    {
        return os;
    }

    return spDevice->GetDriver()->WriteBlock(address, values);
}

OperationStatus DeviceManager::InterfaceReset(const string& deviceName)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    return spDevice->GetDriver()->InterfaceReset();
}

OperationStatus DeviceManager::SetDriverMode(const string& deviceName, int newMode, int& oldMode)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    return spDevice->GetDriver()->SetDriverMode(newMode, oldMode);
}

OperationStatus DeviceManager::DriverControl(const string& deviceName, uint32_t Id, const void *inBuf, uint32_t inBufSize, void *outBuf, uint32_t outBufSize)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    if (!spDevice->GetDriver()->IsCapabilitySet(wil_nl_60g_driver_capa::NL_60G_DRIVER_CAPA_WMI))
    {
        ostringstream oss;
        oss << "Unsupported driver command for device " << deviceName << " working with Debug Mailbox";
        return OperationStatus(false, oss.str());
    }

    return spDevice->GetDriver()->DriverControl(Id, inBuf, inBufSize, outBuf, outBufSize);
}

OperationStatus DeviceManager::AllocPmc(const string& deviceName, unsigned descSize, unsigned descNum)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    return spDevice->GetDriver()->AllocPmc(descSize, descNum);
}

OperationStatus DeviceManager::DeallocPmc(const string& deviceName, bool bSuppressError)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    return spDevice->GetDriver()->DeallocPmc(bSuppressError);
}

OperationStatus DeviceManager::CreatePmcFiles(const string& deviceName, unsigned refNumber)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    return spDevice->GetDriver()->CreatePmcFiles(refNumber);
}

OperationStatus DeviceManager::FindPmcDataFile(const string& deviceName, unsigned refNumber, std::string& foundPmcFilePath)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    return spDevice->GetDriver()->FindPmcDataFile(refNumber, foundPmcFilePath);
}

OperationStatus DeviceManager::FindPmcRingFile(const string& deviceName, unsigned refNumber, std::string& foundPmcFilePath)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    return spDevice->GetDriver()->FindPmcRingFile(refNumber, foundPmcFilePath);
}

void DeviceManager::CreateDevice(unique_ptr<DriverAPI>&& deviceDriver)
{
    const string& interfaceName = deviceDriver->GetInterfaceName();
    LOG_DEBUG << "Creating Device: " << interfaceName << endl;

    m_connectedDevicesMutex.lock();
    m_devices.insert(make_pair(interfaceName, make_shared<Device>(std::move(deviceDriver))));
    m_connectedDevicesMutex.unlock();

    // can be outside lock because we still have the device shared ptr.
    Host::GetHost().GetLogCollectorService().OnDeviceDiscovered(interfaceName);
}

void DeviceManager::DeleteDevice(const string& deviceName)
{
    unique_lock<mutex> lock(m_connectedDevicesMutex);

    const auto deviceIter = m_devices.find(deviceName);
    if (deviceIter == m_devices.cend())
    {
        LOG_INFO << __FUNCTION__ << ": device " << deviceName << " was already removed" << endl;
        return;
    }

    // keep holding the device to schedule device destruction after its removal handled
    shared_ptr<Device> spDevice = deviceIter->second;
    m_devices.erase(deviceIter);
    lock.unlock();

    // we need to release lock before calling UnregisterTaskBlocking
    // otherwise it will wait for polling task that may be blocked on this lock for device access
    Host::GetHost().GetLogCollectorService().OnDeviceRemoved(deviceName);

    spDevice.reset(); // explicit release of device ownership, not strictly needed
}

void DeviceManager::UpdateNonMonitoredDevices()
{
    if (!Host::GetHost().IsEnumerating())
    {
        return;
    }

    vector<string> devicesForRemove;
    // Delete unresponsive devices
    m_connectedDevicesMutex.lock();
    for (auto& connectedDevice : m_devices)
    {
        if (connectedDevice.second->GetSilenceMode()                // skip silent devices
            || connectedDevice.second->GetDriver()->IsMonitored())  // do not remove monitored devices
        {
            continue;
        }

        if (!connectedDevice.second->GetDriver()->IsValid())
        {
            devicesForRemove.push_back(connectedDevice.first);
        }
    }
    m_connectedDevicesMutex.unlock();

    for (auto& device : devicesForRemove)
    {
        LOG_INFO << __FUNCTION__ << ": deleting invalid device " << device << std::endl;
        DeleteDevice(device);
    }

    devicesForRemove.clear();

    // collect current non-monitored interface names
    std::vector<std::pair<DeviceType, std::string>> nonMonitoredInterfaces;
    m_connectedDevicesMutex.lock();
    for (const auto& connectedDevice : m_devices)
    {
        if (connectedDevice.second->GetDriver()->IsMonitored()) // ignore monitored devices
        {
            continue;
        }

        nonMonitoredInterfaces.emplace_back(
            connectedDevice.second->GetDriver()->GetDeviceType(),
            connectedDevice.second->GetDriver()->GetInterfaceName());
    }
    m_connectedDevicesMutex.unlock();

    // update non-monitored devices
    // will synchronously call add/remove devices when applicable
    m_driversFactory.UpdateNonMonitoredDevices(nonMonitoredInterfaces);
}

bool DeviceManager::GetDeviceStatus(vector<DeviceData>& devicesData)
{
    //TODO: move implementation to Device class and ask for device->getState!!!
    // generic board file id and corresponding name, other ids are displayed as is
    const static std::pair<DWORD, std::string> sc_genericBoardFileDescription = std::pair<DWORD, std::string>(65541, "Falcon Free Space"); // 0x10005
    const static std::array<std::string, 5U> sc_fwType = { { "Operational", "WMI Only", "No PCIe", "WMI Only - No PCIe", "IF2IF" } };

    // Lock the devices
    lock_guard<mutex> lock(m_connectedDevicesMutex);

    auto devices = m_devices;

    for (auto& device : devices)
    {
        // Create device data
        DeviceData deviceData;

        // Extract FW version
        FwIdentifier fwIdentifier = device.second->GetFwStateProvider()->GetFwIdentifier();
        deviceData.m_fwVersion = fwIdentifier.m_fwVersion;

        DWORD value = Utils::REGISTER_DEFAULT_VALUE;

        // Read FW assert code
        device.second->GetDriver()->Read(FW_ASSERT_REG, value);
        deviceData.m_fwAssert = value;

        // Read uCode assert code
        device.second->GetDriver()->Read(UCODE_ASSERT_REG, value);
        deviceData.m_uCodeAssert = value;

        // Read FW association state
        device.second->GetDriver()->Read(FW_ASSOCIATION_REG, value);
        deviceData.m_associated = (value == FW_ASSOCIATED_VALUE);

        // Get FW compilation timestamp
        deviceData.m_compilationTime = fwIdentifier.m_fwTimestamp;

        // Get Device name
        deviceData.m_deviceName = device.first;

        // Get baseband name & RF type
        // BB type is stored in 2 lower bytes of device type register
        // RF type is stored in 2 upper bytes of device type register
        device.second->GetDriver()->Read(DEVICE_TYPE_REG, value);
        const int basebandTypeValue = value & 0xFFFF;
        const auto basebandTypeIter = m_basebandTypeToString.find(basebandTypeValue);
        deviceData.m_hwType = basebandTypeIter != m_basebandTypeToString.cend() ? basebandTypeIter->second : std::string("UNKNOWN");
        const auto rfTypeIter = m_rfTypeToString.find((value & 0xFFFF0000) >> 16);
        deviceData.m_rfType = rfTypeIter != m_rfTypeToString.cend() ? rfTypeIter->second : sc_noRfStr;

        // Read MCS value and set signal strength (maximum is 5 bars)
        const int maxMcsRange = (basebandTypeValue < sc_TalynMaBasebandType) ? MAX_MCS_RANGE_SPARROW : MAX_MCS_RANGE_TALYN;
        device.second->GetDriver()->Read(MCS_REG, value);
        deviceData.m_mcs = value;
        deviceData.m_signalStrength = static_cast<int>(value * 5.0 / maxMcsRange + 0.5);

        // Get FW mode
        device.second->GetDriver()->Read(FW_MODE_REG, value);
        deviceData.m_mode = value < sc_fwType.size() ? sc_fwType[value] : "NA";

        // Get boot loader version
        device.second->GetDriver()->Read(BOOT_LOADER_VERSION_REG, value);
        std::ostringstream oss;
        oss << value;
        deviceData.m_bootloaderVersion = oss.str();

        // Get channel number
        int Channel = 0;
        if (basebandTypeValue < sc_TalynMaBasebandType)
        {
            device.second->GetDriver()->Read(CHANNEL_REG_PRE_TALYN_MA, value);
            switch (value)
            {
            case 0x64FCACE:
                Channel = 1;
                break;
            case 0x68BA2E9:
                Channel = 2;
                break;
            case 0x6C77B03:
                Channel = 3;
                break;
            default:
                Channel = 0;
            }
        }
        else
        {
            device.second->GetDriver()->Read(CHANNEL_REG_POST_TALYN_MA, value);
            value = (value & 0xF0000) >> 16; // channel value is contained in bits [16-19]
            // the following is an essence of translation from channel value to the channel number
            Channel = value <= 5 ? value + 1 : value + 3;
        }
        deviceData.m_channel = Channel;

        // Get board file version
        device.second->GetDriver()->Read(BOARDFILE_REG, value);
        if (((value & 0xFFFFF000) >> 12) == sc_genericBoardFileDescription.first) // bits [12-31] are 0x10005
        {
            deviceData.m_boardFile = sc_genericBoardFileDescription.second;
        }
        else
        {
            oss.str(std::string());
            oss << value;
            deviceData.m_boardFile = oss.str();
        }

        // Lower byte of RF state contains connection bit-mask
        // Second byte of RF state contains enabled bit-mask
        DWORD rfConnected = 0;
        device.second->GetDriver()->Read(RF_STATE_REG, rfConnected);

        // Note: RF state address is not constant.
        //       As workaround for SPR-B0 is to mark its single RF as enabled (when RF present)
        // TODO: Fix after moving the RF state to a constant address
        if (deviceData.m_hwType == "SPR-B0" && deviceData.m_rfType != sc_noRfStr)
        {
            rfConnected = 0x101; // connected & enabled
        }

        DWORD rfEnabled = rfConnected >> 8;

        // Get RF state of each RF
        deviceData.m_rf.reserve(MAX_RF_NUMBER);
        for (int rfIndex = 0; rfIndex < MAX_RF_NUMBER; ++rfIndex)
        {
            int rfState = 0; // disabled

            if (rfConnected & (1 << rfIndex))
            {
                rfState = 1; // connected
                if (rfEnabled & (1 << rfIndex))
                {
                    rfState = 2; // enabled
                }
            }

            deviceData.m_rf.push_back(rfState);
        }

        ////////// Get fixed registers values //////////////////////////
        StringNameValuePair registerData;

        // uCode Rx on fixed reg
        device.second->GetDriver()->Read(UCODE_RX_ON_REG, value);
        DWORD UcRxonhexVal16 = value & 0xFFFF;
        string UcRxon;
        switch (UcRxonhexVal16)
        {
        case 0:
            UcRxon = "RX_OFF";
            break;
        case 1:
            UcRxon = "RX_ONLY";
            break;
        case 2:
            UcRxon = "RX_ON";
            break;
        default:
            UcRxon = "Unrecognized";
        }
        registerData.m_name = "uCodeRxOn";
        registerData.m_value = UcRxon;
        deviceData.m_fixedRegisters.insert(deviceData.m_fixedRegisters.end(), registerData);

        // BF Sequence fixed reg
        device.second->GetDriver()->Read(BF_SEQ_REG, value);
        oss.str(std::string());
        oss << value;
        registerData.m_name = "BF_Seq";
        registerData.m_value = oss.str();
        deviceData.m_fixedRegisters.insert(deviceData.m_fixedRegisters.end(), registerData);

        // BF Trigger fixed reg
        device.second->GetDriver()->Read(BF_TRIG_REG, value);
        string BF_TRIG = "";
        switch (value)
        {
        case 1:
            BF_TRIG = "MCS1_TH_FAILURE";
            break;
        case 2:
            BF_TRIG = "MCS1_NO_BACK";
            break;
        case 4:
            BF_TRIG = "NO_CTS_IN_TXOP";
            break;
        case 8:
            BF_TRIG = "MAX_BCK_FAIL_TXOP";
            break;
        case 16:
            BF_TRIG = "FW_TRIGGER ";
            break;
        case 32:
            BF_TRIG = "MAX_BCK_FAIL_ION_KEEP_ALIVE";
            break;
        default:
            BF_TRIG = "UNDEFINED";
        }
        registerData.m_name = "BF_Trig";
        registerData.m_value = BF_TRIG;
        deviceData.m_fixedRegisters.insert(deviceData.m_fixedRegisters.end(), registerData);

        // Get NAV fixed reg
        device.second->GetDriver()->Read(NAV_REG, value);
        registerData.m_name = "NAV";
        oss.str(std::string());
        oss << value;
        registerData.m_value = oss.str();
        deviceData.m_fixedRegisters.insert(deviceData.m_fixedRegisters.end(), registerData);

        // Get TX Goodput fixed reg
        device.second->GetDriver()->Read(TX_GP_REG, value);
        string TX_GP = "NO_LINK";
        if (value != 0)
        {
            oss.str(std::string());
            oss << value;
            TX_GP = oss.str();
        }
        registerData.m_name = "TX_GP";
        registerData.m_value = TX_GP;
        deviceData.m_fixedRegisters.insert(deviceData.m_fixedRegisters.end(), registerData);

        // Get RX Goodput fixed reg
        device.second->GetDriver()->Read(RX_GP_REG, value);
        string RX_GP = "NO_LINK";
        if (value != 0)
        {
            oss.str(std::string());
            oss << value;
            RX_GP = oss.str();
        }
        registerData.m_name = "RX_GP";
        registerData.m_value = RX_GP;
        deviceData.m_fixedRegisters.insert(deviceData.m_fixedRegisters.end(), registerData);
        ////////////// Fixed registers end /////////////////////////

        ////////////// Custom registers ////////////////////////////
        device.second->ReadCustomRegisters(deviceData.m_customRegisters);
        ////////////// Custom registers end ////////////////////////

        ////////////// Temperatures ////////////////////////////////
        // Baseband
        device.second->GetDriver()->Read(BASEBAND_TEMPERATURE_REG, value);
        float temperature = (float)value / 1000;
        oss.str(std::string());
        oss.precision(2);
        oss << fixed << temperature;
        deviceData.m_hwTemp = oss.str();

        // RF
        if (deviceData.m_rfType != sc_noRfStr)
        {
            device.second->GetDriver()->Read(RF_TEMPERATURE_REG, value);
            temperature = (float)value / 1000;
            oss.str(std::string());
            oss.precision(2);
            oss << fixed << temperature;
            deviceData.m_rfTemp = oss.str();
        }
        else // no RF, temperature value is not relevant
        {
            deviceData.m_rfTemp = "";
        }
        ////////////// Temperatures end ///////////////////////////

        // AddChildNode the device to the devices list
        devicesData.insert(devicesData.end(), deviceData);
    }

    return true;
}

bool DeviceManager::AddRegister(const string& deviceName, const string& registerName, uint32_t address, uint32_t firstBit, uint32_t lastBit, std::string& failureReason)
{
    shared_ptr<Device> spDevice;
    const OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        LOG_ERROR << "Failed to add register. Error: " << os.GetStatusMessage() << endl;
        return false;
    }

    if (firstBit > 31 || lastBit > 31 || firstBit > lastBit)
    {
        LOG_ERROR << "Trying to add custom register with invalid mask" << endl;
        failureReason = "Invalid mask provided. Bits should be in range [0,31].";
        return false;
    }

    if (!spDevice->AddCustomRegister(registerName, address, firstBit, lastBit))
    {
        LOG_ERROR << "Trying to add an already existing custom register name" << endl;
        failureReason = "Custom register with this name already exists.";
        return false;
    }

    return true;
}

bool DeviceManager::RemoveRegister(const string& deviceName, const string& registerName, std::string& failureReason)
{
    shared_ptr<Device> spDevice;
    const OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        LOG_ERROR << "Failed to remove register. Error: " << os.GetStatusMessage() << endl;
        return false;
    }

    if (!spDevice->RemoveCustomRegister(registerName))
    {
        LOG_ERROR << "Trying to remove a non-existing custom register name" << endl;
        failureReason = "Custom register with this name does not exist.";
        return false;
    }

    return true;
}

OperationStatus DeviceManager::SetDeviceSilentMode(const string& deviceName, bool silentMode)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    spDevice->SetSilenceMode(silentMode);
    return OperationStatus(true);
}


OperationStatus DeviceManager::GetDeviceSilentMode(const string& deviceName, bool& silentMode)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    silentMode = spDevice->GetSilenceMode();
    return OperationStatus(true);
}

OperationStatus DeviceManager::GetDefaultDevice(std::string & outDeviceName)
{
    lock_guard<mutex> lock(m_connectedDevicesMutex);
    if (m_devices.empty())
    {
        return OperationStatus(false, "No device exist");
    }
    if (m_devices.size() > 1)
    {
        return OperationStatus(false, "No default device. Please provide device name");
    }

    outDeviceName = m_devices.begin()->first;
    return OperationStatus(true);
}


OperationStatus DeviceManager::GetDeviceCapabilities(const string& deviceName, uint32_t& capabilities)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    capabilities = spDevice->GetDriver()->GetCapabilityMask();
    return OperationStatus(true);
}

OperationStatus DeviceManager::GetFwHealthState(const std::string& deviceName, int& fwHealthState)
{
    shared_ptr<Device> spDevice;
    OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return os;
    }

    fwHealthState = static_cast<int>(spDevice->GetFwStateProvider()->GetFwHealthState());
    return OperationStatus(true);
}

bool DeviceManager::GetDeviceBasebandType(const string& deviceName, BasebandType& type)
{
    shared_ptr<Device> spDevice;
    const OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return false;
    }

    type = spDevice->GetBasebandType();
    return true;
}

bool DeviceManager::GetDeviceBasebandRevision(const std::string & deviceName, BasebandRevision & revision)
{
    shared_ptr<Device> spDevice;
    const OperationStatus os = GetDeviceByName(deviceName, spDevice);
    if (!os)
    {
        return false;
    }

    revision = spDevice->GetBasebandRevision();
    LOG_DEBUG << "in DeviceManager::GetDeviceBasebandRevision revision is:" << revision << std::endl;
    return true;
}
