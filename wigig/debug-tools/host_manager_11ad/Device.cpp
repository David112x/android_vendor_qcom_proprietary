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

#include <map>

#include "Device.h"
#include "DriverAPI.h"
#include "FwStateProvider.h"
#include "EventsDefinitions.h"
#include "DebugLogger.h"
#include "DriversFactory.h"
#include "DeviceManager.h"
#include "TaskManager.h"
#include "FieldDescription.h"
#include "HostAndDeviceDataDefinitions.h"
#include "AddressTranslator.h"
#include "PinnedMemorySwitchboard.h"
#include "Utils.h"

using namespace std;

namespace
{
    enum FwPointerTableInfo : DWORD
    {
        DYNAMIC_ADDRESSING_PATTERN = 0xBACACAFE,
        FW_TIMESTAMP_OFFSET_V3 = 4,
        RFC_CONNECTED_POINTER_OFFSET_V2 = 16,
        RFC_ENABLED_POINTER_OFFSET_V2 = 20,
        UCODE_TIMESTAMP_POINTER_OFFSET_V2 = 24,
        UCODE_TIMESTAMP_POINTER_OFFSET_V3 = 28,
    };
}

Device::Device(std::unique_ptr<DriverAPI>&& deviceDriver) :
    m_basebandType(BASEBAND_TYPE_NONE),
    m_basebandRevision(BB_REV_UNKNOWN),
    m_driver(std::move(deviceDriver)),
    m_deviceName(m_driver->GetInterfaceName()),
    m_isSilent(false)
{
    InitializeBasebandRevision();

    FW_ERROR_REGISTER = PinnedMemorySwitchboard::GetAhbAddress(PINNED_MEMORY::FW_ERROR, m_basebandType);
    UCODE_ERROR_REGISTER = PinnedMemorySwitchboard::GetAhbAddress(PINNED_MEMORY::UCODE_ERROR, m_basebandType);

    // should be done after initialization of error registers
    InitializeDriver();
}

// cannot be inline because incompleteness of FieldDescription
Device::~Device()
{
    LOG_DEBUG << "start destructing device " << m_deviceName << std::endl;
    m_driver->StopDriverControlEvents();
    m_fwStateProvider->StopBlocking();
    LOG_DEBUG << "done destructing device " << m_deviceName << std::endl;
}

void Device::InitializeBasebandRevision()
{
    LOG_DEBUG << "Reading BB revision" << std::endl;

    // Device ID address
    // Replaces USER.JTAG.USER_USER_JTAG_1.dft_idcode_dev_id at 0x00880b34 used before Borrelly
    // Allows to differentiate between Sparrow, Talyn and Borrelly
    const uint32_t RGF_USER_JTAG_DEV_ID = 0x00880000;

    const uint32_t JTAG_DEV_ID_SPARROW = 0x2632072f;
    const uint32_t JTAG_DEV_ID_TALYN_MA = 0x7e0e1;
    const uint32_t JTAG_DEV_ID_TALYN_MBC = 0x1007e0e1;
    const uint32_t JTAG_DEV_ID_BORRELLY = 0x1007e0e2;

    // Revision ID allows to differentiate between Sparrow_M_B0 and Sparrow_M_D0
    const uint32_t RGF_USER_REVISION_ID = (0x88afe4);
    const uint32_t RGF_USER_REVISION_ID_MASK = (3);
    const uint32_t REVISION_ID_SPARROW_B0 = (0x0);
    const uint32_t REVISION_ID_SPARROW_D0 = (0x3);

    // Debug ID allows to differentiate between Talyn_M_B0 and Talyn_M_C0
    const uint32_t RGF_POWER_DEBUG_ID = (0x883f6c); /* PHY_RX_RGF.POWER_DEBUG.power_dbg_dout0 */
    const uint32_t DEBUG_ID_TALYN_B0 = (0x0);
    const uint32_t DEBUG_ID_TALYN_C0 = (0x1);

    m_basebandType = BASEBAND_TYPE_NONE;
    m_basebandRevision = BB_REV_UNKNOWN;

    DWORD jtagId = Utils::REGISTER_DEFAULT_VALUE;

    if (!m_driver->Read(RGF_USER_JTAG_DEV_ID, jtagId))
    {
        LOG_ERROR << "Cannot determine device revision. Failed to read JTAG ID: "
            << Address(RGF_USER_JTAG_DEV_ID) << std::endl;
        return;
    }

    LOG_DEBUG << "Obtained JTAG ID: " << AddressValue(RGF_USER_JTAG_DEV_ID, jtagId) << std::endl;

    switch (jtagId)
    {
    case JTAG_DEV_ID_SPARROW:
        m_basebandType = BASEBAND_TYPE_SPARROW;

        DWORD chipRevision;
        if (!m_driver->Read(RGF_USER_REVISION_ID, chipRevision))
        {
            LOG_ERROR << "Cannot determine device revision. Failed to read RGF_USER_REVISION_ID: "
                << Address(RGF_USER_REVISION_ID) << std::endl;
        }
        chipRevision &= RGF_USER_REVISION_ID_MASK;

        switch (chipRevision)
        {
        case REVISION_ID_SPARROW_D0:
            m_basebandRevision = SPR_D0;
            break;
        case REVISION_ID_SPARROW_B0:
            m_basebandRevision = SPR_B0;
            break;
        default:
            LOG_ERROR << "Unsupported SPR chip revision: " << AddressValue(RGF_USER_REVISION_ID, chipRevision) << std::endl;
            break;
        }
        break;

    case JTAG_DEV_ID_TALYN_MA:
        m_basebandType = BASEBAND_TYPE_TALYN;
        m_basebandRevision = TLN_M_A0;
        break;

    case JTAG_DEV_ID_TALYN_MBC:
        m_basebandType = BASEBAND_TYPE_TALYN;

        DWORD debugRevision;
        if (!m_driver->Read(RGF_POWER_DEBUG_ID, debugRevision))
        {
            LOG_ERROR << "Cannot determine device revision. Failed to read RGF_POWER_DEBUG_ID: "
                << Address(RGF_POWER_DEBUG_ID) << std::endl;
            return;
        }

        switch (debugRevision)
        {
        case DEBUG_ID_TALYN_B0:
            m_basebandRevision = TLN_M_B0;
            break;
        case DEBUG_ID_TALYN_C0:
            m_basebandRevision = TLN_M_C0;
            break;
        default:
            LOG_ERROR << "Unsupported TLN chip revision: " << AddressValue(RGF_POWER_DEBUG_ID, debugRevision) << std::endl;
            break;
        }
        break;

    case JTAG_DEV_ID_BORRELLY:
        m_basebandType = BASEBAND_TYPE_BORRELLY;
        m_basebandRevision = BRL_P1;
        break;

    default:
        LOG_ERROR << "Unsupported JTAG ID: " << AddressValue(RGF_USER_JTAG_DEV_ID, jtagId) << std::endl;
        break;
    }

    LOG_INFO << "On Device " << m_deviceName << " Baseband type is " << m_basebandType << ", Baseband revision is " << m_basebandRevision << std::endl;
}

void Device::InitializeDriver()
{
    // always create FwStateProvider to prevent potential null pointer access
    // if driver is not ready, it will be created without polling
    const bool isPollingRequired = m_driver->InitializeTransports()
        && !m_driver->IsCapabilitySet(wil_nl_60g_driver_capa::NL_60G_DRIVER_CAPA_FW_STATE);

    m_fwStateProvider.reset(new FwStateProvider(*this, isPollingRequired));

    // internally tests if driver is ready
    m_driver->RegisterDriverControlEvents(m_fwStateProvider.get());

    const bool isOperationalMailbox = m_driver->IsCapabilitySet(wil_nl_60g_driver_capa::NL_60G_DRIVER_CAPA_WMI);
    LOG_INFO << "Device " << m_deviceName << " is using " << (isOperationalMailbox ? "Operational" : "Debug") << " mailbox" << endl;
}

// Provided mask is assumed to be in range [0,31]
bool Device::AddCustomRegister(const string& name, uint32_t address, uint32_t firstBit, uint32_t lastBit)
{
    if (m_customRegistersMap.find(name) != m_customRegistersMap.end())
    {
        // Register already exists
        return false;
    }

    m_customRegistersMap[name].reset(new FieldDescription(address, firstBit, lastBit));
    return true;
}

bool Device::RemoveCustomRegister(const string& name)
{
    if (m_customRegistersMap.find(name) == m_customRegistersMap.end())
    {
        // Register already does not exist
        return false;
    }

    m_customRegistersMap.erase(name);

    return true;
}

// Read values for all defined custom registers
// Returns true when read operation succeeded for all registers
bool Device::ReadCustomRegisters(std::vector<StringNameValuePair>& customRegisters) const
{
    customRegisters.clear(); // capacity unchanged
    customRegisters.reserve(m_customRegistersMap.size());

    bool ok = true;
    for (const auto& reg : m_customRegistersMap)
    {
        DWORD value = Utils::REGISTER_DEFAULT_VALUE;
        ok &= m_driver->Read(reg.second->GetAddress(), value);
        std::ostringstream oss;
        oss << reg.second->MaskValue(value);

        customRegisters.push_back({reg.first, oss.str()});
    }

    return ok;
}

// Internal service for fetching the FW version and compile times
bool Device::ReadDeviceFwInfo(FwIdentifier& fwIdentifier, uint32_t fwTimestampStartAddress, uint32_t uCodeTimestampStartAddress) const
{
    if (GetSilenceMode())
    {
        return false;
    }

    // = FW version
    bool readOk = m_driver->Read(PinnedMemorySwitchboard::GetAhbAddress(PINNED_MEMORY::FW_VERSION_MAJOR, m_basebandType), fwIdentifier.m_fwVersion.m_major);
    readOk &= m_driver->Read(PinnedMemorySwitchboard::GetAhbAddress(PINNED_MEMORY::FW_VERSION_MINOR, m_basebandType), fwIdentifier.m_fwVersion.m_minor);
    readOk &= m_driver->Read(PinnedMemorySwitchboard::GetAhbAddress(PINNED_MEMORY::FW_VERSION_SUB_MINOR, m_basebandType), fwIdentifier.m_fwVersion.m_subMinor);
    readOk &= m_driver->Read(PinnedMemorySwitchboard::GetAhbAddress(PINNED_MEMORY::FW_VERSION_BUILD, m_basebandType), fwIdentifier.m_fwVersion.m_build);

    // = FW compile time
    // fwTimestampStartAddress is an address of the following struct:
    // typedef struct image_timestamp_ty
    // {
    //    U32 hour;
    //    U32 minute;
    //    U32 second;
    //    U32 day;
    //    U32 month;
    //    U32 year;
    // } image_timestamp_s;
    readOk &= m_driver->Read(fwTimestampStartAddress, fwIdentifier.m_fwTimestamp.m_hour);
    readOk &= m_driver->Read(fwTimestampStartAddress + 4, fwIdentifier.m_fwTimestamp.m_min);
    readOk &= m_driver->Read(fwTimestampStartAddress + 8, fwIdentifier.m_fwTimestamp.m_sec);
    readOk &= m_driver->Read(fwTimestampStartAddress + 12, fwIdentifier.m_fwTimestamp.m_day);
    readOk &= m_driver->Read(fwTimestampStartAddress + 16, fwIdentifier.m_fwTimestamp.m_month);
    readOk &= m_driver->Read(fwTimestampStartAddress + 20, fwIdentifier.m_fwTimestamp.m_year);

    // = uCode compile times
    if (uCodeTimestampStartAddress == sc_invalidAddress)
    {
        fwIdentifier.m_uCodeTimestamp = FwTimestamp::CreateInvalidTimestamp();
    }
    else
    {
        // uCodeTimestampStartAddress contains address of the following struct (indirect):
        // typedef struct {
        //    U08 ucode_compilation_sec;
        //    U08 ucode_compilation_min;
        //    U08 ucode_compilation_hour;
        //    U08 ucode_compilation_day;
        //    U08 ucode_compilation_month;
        //    U08 ucode_compilation_year;
        // } ucode_timestamp_s;

        DWORD value = 0U;
        readOk &= m_driver->Read(uCodeTimestampStartAddress, value);
        if (value == 0U)
        {
            // when uCode timestamp is not properly updated (can happen for WMI_ONLY FW) it should be ignored
            // newer WMI_ONLY FW can update the timestamp, so cannot decided base on FW flavor
            fwIdentifier.m_uCodeTimestamp = FwTimestamp::CreateInvalidTimestamp();
        }
        else
        {
            fwIdentifier.m_uCodeTimestamp.m_sec = FieldDescription::MaskValue(value, 0, 7);
            fwIdentifier.m_uCodeTimestamp.m_min = FieldDescription::MaskValue(value, 8, 15);
            fwIdentifier.m_uCodeTimestamp.m_hour = FieldDescription::MaskValue(value, 16, 23);
            fwIdentifier.m_uCodeTimestamp.m_day = FieldDescription::MaskValue(value, 24, 31);

            readOk &= m_driver->Read(uCodeTimestampStartAddress + sizeof(uint32_t), value);
            fwIdentifier.m_uCodeTimestamp.m_month = FieldDescription::MaskValue(value, 0, 7);
            fwIdentifier.m_uCodeTimestamp.m_year = FieldDescription::MaskValue(value, 8, 15);
        }
    }

    if (!readOk)
    {
        LOG_ERROR << "Failed to read FW info for device " << GetDeviceName() << endl;
    }

    return readOk;
}


bool Device::ReadFwUcodeErrors(uint32_t& fwError, uint32_t& uCodeError) const
{
    if (GetSilenceMode())
    {
        return false;
    }

    bool readOk = m_driver->Read(FW_ERROR_REGISTER, reinterpret_cast<DWORD&>(fwError));
    readOk &= m_driver->Read(UCODE_ERROR_REGISTER, reinterpret_cast<DWORD&>(uCodeError));
    if (!readOk)
    {
        LOG_ERROR << "Failed to read FW state for device " << GetDeviceName() << endl;
    }

    return readOk;
}

// Read FW pointer table and initialize relevant values/addresses
// Pointer table is defined by the following structure:
// === version 2: ===
// typedef struct {
//    U32 version;
//    U32 operational_mbox_base_address;
//    U32 debug_mbox_base_address;
//    U32 dashboard_base_address;
// #if (PRODUCTION_RF==TRUE)
//    U32 rf_production_api_base_address;
// #endif
//    U32 p_rfc_connected;                // from ver 1, Linker addr. of FW.g_rfc_connected_indx_vec which is rf_connected, FW DCCM
//    U32 p_rfc_enabled;                  // from ver 1, Linker addr. of FW.g_marlon_r_if.marlon_r_if_class.m_rfc_enabled, FW DCCM
//    ucode_timestamp_s *ucode_timestamp; // from ver 2, Linker addr. of g_shared_mem_ipc.ucode_timestamp, Peripheral Memory
//                                        // (UCODE.g_shared_mem_ipc.shared_mem_ipc_s.shared_mem_ipc_s.ucode_timestamp.ucode_timestamp_s, 6 Bytes)
// } fw_pointer_table_s;
//
// === version 3: ===
// typedef struct {
//    U32 version;
//    image_timestamp_s fw_timestamp; // 6 DWORDs
//    ucode_timestamp_s* ucode_timestamp;
// } fw_pointer_table_s;
//
bool Device::ReadFwPointerTable(uint32_t& rfcConnectedValue, uint32_t& rfcEnabledValue,
    uint32_t& fwTimestampStartAddress, uint32_t& uCodeTimestampStartAddress) const
{
    // initialize out params to defaults, will be overriden when possible
    rfcConnectedValue = rfcEnabledValue = 0U;
    uCodeTimestampStartAddress = sc_invalidAddress; // no valid default
    // fwTimestampStartAddress is updated only when provided in the table

    if (GetSilenceMode())
    {
        return false;
    }

    DWORD pattern = 0U;
    bool readOk = m_driver->Read(PinnedMemorySwitchboard::GetAhbAddress(PINNED_MEMORY::POINTER_TABLE_PATTERN, m_basebandType), pattern);
    if (!readOk)
    {
        LOG_ERROR << "On Device " << m_deviceName << ", failed to read dynamic addressing pattern" << endl;
        return false;
    }
    if (m_basebandType == BASEBAND_TYPE_BORRELLY && pattern == 0)
    {
        // pattern is nullified during SW reset and is the last field written during FW pointer table initialization
        LOG_ERROR << "On Device " << m_deviceName << ", FW pointer table is not yet initialized" << endl;
        return false;
    }
    if (pattern != DYNAMIC_ADDRESSING_PATTERN)
    {
        // old FW before dynamic addressing
        LOG_DEBUG << "On Device " << m_deviceName << ", there is no FW pointer table" << endl;
        return true; // not a failure
    }

    // dynamic addressing pattern if followed by a linker address of the pointer tables structure, which is started with version
    const uint32_t pointerTableAddr = PinnedMemorySwitchboard::GetAhbAddress(PINNED_MEMORY::POINTER_TABLE_ADR, m_basebandType);
    DWORD pointerTableBaseAddressLinker = sc_invalidAddress;
    readOk &= m_driver->Read(pointerTableAddr, pointerTableBaseAddressLinker);
    LOG_DEBUG << "On Device " << m_deviceName << ", FW pointer table base Linker address is "
        << AddressValue(pointerTableAddr, pointerTableBaseAddressLinker) << endl;
    uint32_t pointerTableBaseAddressAhb = sc_invalidAddress;
    AddressTranslator::ToAhbAddress(pointerTableBaseAddressLinker, pointerTableBaseAddressAhb, m_basebandRevision);

    DWORD tableVersion = 0U;
    readOk &= m_driver->Read(pointerTableBaseAddressLinker, tableVersion);
    if (!readOk)
    {
        LOG_ERROR << "On Device " << m_deviceName << ", failed to read FW pointer table version" << endl;
        return false;
    }

    LOG_DEBUG << "On Device " << m_deviceName << ", FW pointer table version is " << Hex<2>(tableVersion) << endl;

    // read rfc_connected & rfc_enabled, part of ver.1 and ver.2
    if (1U <= tableVersion && tableVersion <= 2U)
    {
        DWORD rfcConnectedAddressLinker = sc_invalidAddress;
        readOk &= m_driver->Read(pointerTableBaseAddressAhb + RFC_CONNECTED_POINTER_OFFSET_V2, rfcConnectedAddressLinker);
        if (rfcConnectedAddressLinker != sc_invalidAddress) // may be zero in 10.1, this case stay with default value
        {
            uint32_t rfcConnectedAddressLinkerAhb = sc_invalidAddress;
            AddressTranslator::ToAhbAddress(rfcConnectedAddressLinker, rfcConnectedAddressLinkerAhb, m_basebandRevision);
            readOk &= m_driver->Read(rfcConnectedAddressLinkerAhb, reinterpret_cast<DWORD&>(rfcConnectedValue));
        }
        DWORD rfcEnabledAddressLinker = sc_invalidAddress;
        readOk &= m_driver->Read(pointerTableBaseAddressAhb + RFC_ENABLED_POINTER_OFFSET_V2, rfcEnabledAddressLinker);
        if (rfcEnabledAddressLinker != sc_invalidAddress) // may be zero in 10.1, this case stay with default value
        {
            uint32_t rfcEnabledAddressAhb = sc_invalidAddress;
            AddressTranslator::ToAhbAddress(rfcConnectedAddressLinker, rfcEnabledAddressAhb, m_basebandRevision);
            readOk &= m_driver->Read(rfcEnabledAddressAhb, reinterpret_cast<DWORD&>(rfcEnabledValue));
        }
        LOG_DEBUG << "On Device " << m_deviceName << ", RFC connected mask is " << Hex<>(rfcConnectedValue)
            << ", RFC enabled mask is " << Hex<>(rfcEnabledValue) << endl;
    }

    // read uCode timestamp address
    if (tableVersion >= 2U)
    {
        const uint32_t ucodeTimestampPointerOffset = tableVersion == 2
            ? UCODE_TIMESTAMP_POINTER_OFFSET_V2
            : UCODE_TIMESTAMP_POINTER_OFFSET_V3;

        uint32_t uCodeTimestampStartAddressLinker = sc_invalidAddress;
        readOk &= m_driver->Read(pointerTableBaseAddressAhb + ucodeTimestampPointerOffset, reinterpret_cast<DWORD&>(uCodeTimestampStartAddressLinker));
        if (readOk)
        {
            AddressTranslator::ToAhbAddress(uCodeTimestampStartAddressLinker, uCodeTimestampStartAddress, m_basebandRevision);
            LOG_DEBUG << "On Device " << m_deviceName << ", uCode Timestamp AHB address is " << Hex<>(uCodeTimestampStartAddress) << endl;
        }
    }

    if (tableVersion >= 3)
    {
        // v3 table contain the (FW) image_timestamp_s struct itself
        fwTimestampStartAddress = pointerTableBaseAddressAhb + FW_TIMESTAMP_OFFSET_V3;
        LOG_DEBUG << "On Device " << m_deviceName << ", FW Timestamp AHB address is " << Hex<>(fwTimestampStartAddress) << endl;
    }

    if (!readOk)
    {
        LOG_ERROR << "On Device " << m_deviceName << ", failed to read FW pointer table" << endl;
    }

    return readOk;
}
