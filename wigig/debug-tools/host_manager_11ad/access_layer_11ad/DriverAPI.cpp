/*
* Copyright (c) 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include  "DriverAPI.h"

DriverAPI::DriverAPI(DeviceType deviceType, std::string interfaceName, bool isMonitored /*= false*/) :
    m_deviceType(deviceType),
    m_interfaceName(std::move(interfaceName)),
    m_capabilityMask(4U), // b'100, IOCTL WRITE capability is set by default, may be overriden when reported otherwise
    m_isMonitored(isMonitored)
{}

OperationStatus DriverAPI::Read(DWORD /*address*/, DWORD& /*value*/)
{
    return OperationStatus(false, "Not implemented");
}

OperationStatus DriverAPI::ReadBlock(DWORD /*addr*/, DWORD /*blockSizeInDwords*/, std::vector<DWORD>& /*values*/)
{
    return OperationStatus(false, "Not implemented");
}

OperationStatus DriverAPI::ReadBlock(DWORD /*addr*/, DWORD /*blockSizeInBytes*/, char * /*arrBlock*/)
{
    return OperationStatus(false, "Not implemented");
}

OperationStatus DriverAPI::ReadRadarData(DWORD /*maxBlockSizeInDwords*/, std::vector<DWORD>& /*values*/)
{
    return OperationStatus(false, "Not implemented");
}

OperationStatus DriverAPI::Write(DWORD /*address*/, DWORD /*value*/)
{
    return OperationStatus(false, "Not implemented");
}

OperationStatus DriverAPI::WriteBlock(DWORD /*addr*/, std::vector<DWORD> /*values*/)
{
    return OperationStatus(false, "Not implemented");
}

OperationStatus DriverAPI::WriteBlock(DWORD /*address*/, DWORD /*blockSizeInBytes*/, const char* /*valuesToWrite*/)
{
    return OperationStatus(false, "Not implemented");
}

OperationStatus DriverAPI::AllocPmc(unsigned /*descSize*/, unsigned /*descNum*/)
{
    return OperationStatus(false, "PMC is not supported on the current platform");
}

OperationStatus DriverAPI::DeallocPmc(bool /*bSuppressError*/)
{
    return OperationStatus(false, "PMC is not supported on the current platform");
}

OperationStatus DriverAPI::CreatePmcFiles(unsigned /*refNumber*/)
{
    return OperationStatus(false, "PMC is not supported on the current platform");
}

OperationStatus DriverAPI::FindPmcDataFile(unsigned /*refNumber*/, std::string& /*foundPmcFilePath*/)
{
    return OperationStatus(false, "PMC is not supported on the current platform");
}

OperationStatus DriverAPI::FindPmcRingFile(unsigned /*refNumber*/, std::string& /*foundPmcFilePath*/)
{
    return OperationStatus(false, "PMC is not supported on the current platform");
}

OperationStatus DriverAPI::DriverControl(
    uint32_t /*id*/, const void* /*inBuf*/, uint32_t /*inBufSize*/,
    void* /*outBuf*/, uint32_t /*outBufSize*/)
{
    return OperationStatus(false, "Driver Control not supported on the current platform");
}

OperationStatus DriverAPI::GetDriverMode(int& /*currentState*/)
{
    return OperationStatus(false, "Not implemented");
}

OperationStatus DriverAPI::SetDriverMode(int /*newState*/, int& /*oldState*/)
{
    return OperationStatus(true); // by default does nothing and returns success
}

OperationStatus DriverAPI::InterfaceReset()
{
    return OperationStatus(false, "Interface Reset is not supported on the current platform");
}

bool DriverAPI::IsValid()
{
    DWORD value;
    return Read(BAUD_RATE_REGISTER, value).IsSuccess();
}

uint32_t DriverAPI::GetCapabilityMask() const
{
    return static_cast<uint32_t>(m_capabilityMask.to_ulong());
}

bool DriverAPI::IsCapabilitySet(wil_nl_60g_driver_capa capability) const
{
    return m_capabilityMask.test(static_cast<size_t>(capability));
}
