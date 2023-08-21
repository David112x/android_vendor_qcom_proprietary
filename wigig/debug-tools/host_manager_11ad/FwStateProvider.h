/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include <mutex>
#include "FwInfoDefinitions.h"

// Enumeration of FW health states as defined by wil6210 driver
enum class wil_fw_state
{
    WIL_FW_STATE_UNKNOWN = 0,           // uninitialized (also may mark debug_fw)
    WIL_FW_STATE_DOWN,                  // FW not loaded or not ready yet
    WIL_FW_STATE_READY,                 // FW is ready
    WIL_FW_STATE_ERROR_BEFORE_READY,    // Detected FW error before FW sent ready indication
    WIL_FW_STATE_ERROR,                 // Detected FW error after FW sent ready indication
    WIL_FW_STATE_UNRESPONSIVE           // FW operation error, does not exist in driver enumeration
                                        // (scan/connect timeout or PCIe link down detected by the driver)
};

std::ostream& operator<<(std::ostream &os, wil_fw_state fwHealthState);

class Device; // forward declaration

class FwStateProvider final
{
public:
    FwStateProvider(const Device& device, bool pollingRequired);

    void StopBlocking();

    FwIdentifier GetFwIdentifier() const;
    wil_fw_state GetFwHealthState() const;

    void OnFwHealthStateChanged(wil_fw_state fwHealthState);
    bool IsInitialized() const;

private:
    mutable std::mutex m_mutex; // threads synchronization
    const Device& m_device;
    const bool m_pollingRequired;

    FwIdentifier m_fwIdentifier;
    uint32_t m_fwError;
    uint32_t m_uCodeError;
    uint32_t m_fwTimestampStartAddress;
    uint32_t m_uCodeTimestampStartAddress;
    uint32_t m_rfcConnectedValue; // zero is a valid value, uninitialized will be marked as MAX_UINT
    uint32_t m_rfcEnabledValue;
    wil_fw_state m_fwHealthState;

    const std::string m_fwInfoPollerTaskName;
    const std::string m_fwUcodeErrorsPollerTaskName;

    void HandleDeviceInitialization();
    void ReadFwPointerTableInternal();
    void PollDeviceFwInfo();
    void ReadDeviceFwInfoInternal();
    void PollFwUcodeErrors();
    void ReadFwUcodeErrorsInternal();
};
