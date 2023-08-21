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

#ifdef _WINDOWS
    #include "ioctl_if.h"
#endif

#include <sstream>
#include <string>
#include <set>

#include "CommandsHandler.h"
#include "SharedVersionInfo.h"
#include "Utils.h"
#include "Host.h"
#include "HostDefinitions.h"
#include "EventsDefinitions.h"
#include "ShellCommandExecutor.h"
#include "FlashControlHandler.h"
#include "DriverConfigurationHandler.h"
#include "LegacyLogCollectorHandler.h"
#include "DeviceManager.h"
#include "HostInfo.h"
#include "MessageParser.h"
#include "DebugLogger.h"

using namespace std;

#define HANDLER_ENTER                                                                             \
    do {                                                                                          \
        if (g_LogConfig.ShouldPrint(LOG_SEV_VERBOSE))                                             \
        {                                                                                         \
            std::ostringstream ss;                                                                 \
            ss << "Command Handler: " << __FUNCTION__ << " args: " << numberOfArguments << " [ "; \
            for (const auto& s : arguments) ss << s << ' ';                                       \
            ss << ']';                                                                            \
            LOG_VERBOSE << ss.str() << endl;                                                      \
        }                                                                                         \
    } while(false)


 // *************************************************************************************************

const string CommandsHandler::TRUE_STR = "True";
const string CommandsHandler::FALSE_STR = "False";
const string CommandsHandler::LINUX_ONLY_SUPPORT_STR = "Linux support only";
const string CommandsHandler::INVALID_ARGUMENTS_NUMBER_STR = "Invalid arguments number";
const string CommandsHandler::INVALID_ARGUMENTS_STR = "Invalid argument";

// *************************************************************************************************

CommandsHandler::CommandsHandler(ServerType type, Host& host) :
    m_host(host)
{
    if (ServerType::stTcp == type) // TCP server
    {
        m_functionHandler.insert(make_pair("r", &CommandsHandler::Read));
        m_functionHandler.insert(make_pair("rb", &CommandsHandler::ReadBlock));
        m_functionHandler.insert(make_pair("rb_fast", &CommandsHandler::ReadBlockFast));
        m_functionHandler.insert(make_pair("read_radar_data", &CommandsHandler::ReadRadarData));
        m_functionHandler.insert(make_pair("w", &CommandsHandler::Write));
        m_functionHandler.insert(make_pair("wb", &CommandsHandler::WriteBlock));
        m_functionHandler.insert(make_pair("interface_reset", &CommandsHandler::InterfaceReset));

        m_functionHandler.insert(make_pair("pmc_ver", &CommandsHandler::GetPmcAgentVersion));
        m_functionHandler.insert(make_pair("disable_pcie_aspm", &CommandsHandler::DisableASPM));
        m_functionHandler.insert(make_pair("enable_pcie_aspm", &CommandsHandler::EnableASPM));
        m_functionHandler.insert(make_pair("is_pcie_aspm_enabled", &CommandsHandler::GetASPM));
        m_functionHandler.insert(make_pair("alloc_pmc", &CommandsHandler::AllocPmc));
        m_functionHandler.insert(make_pair("dealloc_pmc", &CommandsHandler::DeallocPmc));
        m_functionHandler.insert(make_pair("create_pmc_file", &CommandsHandler::CreatePmcFile));
        m_functionHandler.insert(make_pair("read_pmc_file", &CommandsHandler::FindPmcDataFile));
        m_functionHandler.insert(make_pair("read_pmc_ring_file", &CommandsHandler::FindPmcRingFile));

        m_functionHandler.insert(make_pair("get_interfaces", &CommandsHandler::GetInterfaces));

        m_functionHandler.insert(make_pair("get_host_alias", &CommandsHandler::GetHostName));
        m_functionHandler.insert(make_pair("get_time", &CommandsHandler::GetTime));
        m_functionHandler.insert(make_pair("set_local_driver_mode", &CommandsHandler::SetDriverMode));
        m_functionHandler.insert(make_pair("get_host_manager_version", &CommandsHandler::GetHostManagerVersion));
        m_functionHandler.insert(make_pair("get_min_dmtools_version", &CommandsHandler::GetMinDmToolsVersion));
        m_functionHandler.insert(make_pair("get_host_os", &CommandsHandler::GetHostOS));
        m_functionHandler.insert(make_pair("get_host_platform", &CommandsHandler::GetHostPlatform));
        m_functionHandler.insert(make_pair("get_host_enumaration_mode", &CommandsHandler::GetHostEnumerationMode));
        m_functionHandler.insert(make_pair("set_host_enumaration_mode", &CommandsHandler::SetHostEnumerationMode));

        m_functionHandler.insert(make_pair("driver_command", &CommandsHandler::DriverCommand));
        m_functionHandler.insert(make_pair("set_silence_mode", &CommandsHandler::SetDeviceSilenceMode));
        m_functionHandler.insert(make_pair("get_silence_mode", &CommandsHandler::GetDeviceSilenceMode));
        m_functionHandler.insert(make_pair("get_connected_users", &CommandsHandler::GetConnectedUsers));
        m_functionHandler.insert(make_pair("get_device_capabilities_mask", &CommandsHandler::GetDeviceCapabilities));
        m_functionHandler.insert(make_pair("get_host_capabilities_mask", &CommandsHandler::GetHostCapabilities));
        m_functionHandler.insert(make_pair("get_fw_health_state", &CommandsHandler::GetFwHealthState));
        m_functionHandler.insert(make_pair("log_collector", &CommandsHandler::LogCollectorHandler));
        m_functionHandler.insert(make_pair("flash_control", &CommandsHandler::FlashControlHandler));
        m_functionHandler.insert(make_pair("driver_configuration", &CommandsHandler::DriverConfiguration));
    }
    else // UDP server
    {
        m_functionHandler.insert(make_pair(/*"get_host_network_info"*/"GetHostIdentity", &CommandsHandler::GetHostNetworkInfo));
    }
}

// *************************************************************************************************

string CommandsHandler::DecorateResponseMessage(bool successStatus, const string& message) const
{
    ostringstream response;
    response << Utils::GetCurrentLocalTimeString();
    response << m_reply_feilds_delimiter;
    response << (successStatus ? "Success" : "Fail");

    if (!message.empty())
    {
        response << m_reply_feilds_delimiter;
        response << message;
    }
    return response.str();
}

// **************************************TCP commands handlers*********************************************************** //
ResponseMessage CommandsHandler::GetInterfaces(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        const auto deviceNames = m_host.GetDeviceManager().GetDeviceNames();

        // create one string that contains all connected devices
        ostringstream devicesSs;
        bool firstTime = true;

        for (const auto& device : deviceNames)
        {
            if (firstTime)
            {
                devicesSs << device;
                firstTime = false;
                continue;
            }
            devicesSs << m_device_delimiter << device;
        }
        response.message = DecorateResponseMessage(true, devicesSs.str());
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::Read(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 2, response.message))
    {
        DWORD address;
        if (!Utils::ConvertHexStringToDword(arguments[1], address))
        {
            const string errStr = "invalid arguments: address is expected in Hex format";
            LOG_WARNING << "Failed to read, " << errStr << endl;
            response.message = DecorateResponseMessage(false, errStr);
        }
        else
        {
            DWORD value;
            const OperationStatus status = m_host.GetDeviceManager().Read(arguments[0], address, value);
            if (!status)
            {
                LOG_ERROR << "Failed to read: " << status.GetStatusMessage() << endl;
                response.message = DecorateResponseMessage(false, status.GetStatusMessage());
            }
            else
            {
                ostringstream message;
                message << Hex<>(value);
                response.message = DecorateResponseMessage(true, message.str());
            }
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::Write(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 3, response.message))
    {
        DWORD address, value;
        if (!Utils::ConvertHexStringToDword(arguments[1], address) || !Utils::ConvertHexStringToDword(arguments[2], value))
        {
            const string errStr = "invalid arguments: address and value are expected in Hex format";
            LOG_WARNING << "Failed to write, " << errStr << endl;
            response.message = DecorateResponseMessage(false, errStr);
        }
        else
        {
            const OperationStatus status = m_host.GetDeviceManager().Write(arguments[0], address, value);
            if (!status)
            {
                LOG_ERROR << "Failed to write: " << status.GetStatusMessage() << endl;
                response.message = DecorateResponseMessage(false, status.GetStatusMessage());
            }
            else
            {
                response.message = DecorateResponseMessage(true);
            }
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::ReadBlock(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    vector<DWORD> values;
    if (ReadBlockImpl(arguments, numberOfArguments, response, values))
    {
        LOG_VERBOSE << __FUNCTION__ << ": number of DWORDS received from Driver is " << values.size() << endl;
        // encode each numeric result as string
        ostringstream responseSs;
        auto it = values.begin();
        if (it != values.end())
        {
            responseSs << "0x" << hex << *it;
            ++it;
        }
        for (; it != values.end(); ++it)
        {
            responseSs << m_array_delimiter << "0x" << hex << *it;
        }
        response.message = DecorateResponseMessage(true, responseSs.str());
    }
    // otherwise, response is already decorated with the error

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************
ResponseMessage CommandsHandler::ReadBlockFast(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    vector<DWORD> values;
    if (ReadBlockImpl(arguments, numberOfArguments, response, values))
    {
        LOG_VERBOSE << __FUNCTION__ << ": number of DWORDS received from Driver is " << values.size() << endl;
        // encode all the binary block as Base64
        const unsigned char* buffer = reinterpret_cast<unsigned char*>(values.data());
        const size_t blockSizeInBytes = values.size() * sizeof(DWORD);
        response.message = DecorateResponseMessage(true, Utils::Base64Encode(buffer, blockSizeInBytes)); // empty string if the buffer is empty
    }
    // otherwise, response is already decorated with the error

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************
bool CommandsHandler::ReadBlockImpl(const vector<string>& arguments, unsigned int numberOfArguments, ResponseMessage& response, vector<DWORD>& values)
{
    if (!ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 3, response.message))
    {
        return false;
    }

    DWORD address, blockSize;
    if (!Utils::ConvertHexStringToDword(arguments[1], address) || !Utils::ConvertHexStringToDword(arguments[2], blockSize))
    {
        const string errStr = "invalid arguments: address and block size are expected in Hex format";
        LOG_WARNING << "Failed to read block, " << errStr << endl;
        response.message = DecorateResponseMessage(false, errStr);
        return false;
    }

    const OperationStatus status = m_host.GetDeviceManager().ReadBlock(arguments[0], address, blockSize, values);
    if (!status)
    {
        LOG_ERROR << "Failed to read block: " << status.GetStatusMessage() << endl;
        response.message = DecorateResponseMessage(false, status.GetStatusMessage());
        return false;
    }

    return true;
}

// *************************************************************************************************
ResponseMessage CommandsHandler::ReadRadarData(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 2, response.message))
    {
        DWORD maxBlockSize; // in DWORDS
        vector<DWORD> values;
        if (!Utils::ConvertHexStringToDword(arguments[1], maxBlockSize))
        {
            const string errStr = "invalid arguments: max block size (in DWORDS) is expected in Hex format";
            LOG_WARNING << "Failed to read radar data, " << errStr << endl;
            response.message = DecorateResponseMessage(false, errStr);
        }
        else
        {
            LOG_DEBUG << __FUNCTION__ << ": maximal block size (in DWORDS) requested is " << Hex<>(maxBlockSize) << endl;
            const OperationStatus status = m_host.GetDeviceManager().ReadRadarData(arguments[0], maxBlockSize, values);
            if (!status)
            {
                LOG_ERROR << "Failed to read radar data: " << status.GetStatusMessage() << endl;
                response.message = DecorateResponseMessage(false, status.GetStatusMessage());
            }
            else
            {
                LOG_DEBUG << __FUNCTION__ << ": number of DWORDS received from Driver is " << values.size() << endl;
                // encode all the binary block received as Base64
                // the returned vector contains only data to be returned to the caller
                const unsigned char* buffer = reinterpret_cast<unsigned char*>(values.data());
                const size_t blockSizeInBytes = values.size() * sizeof(DWORD);
                response.message = DecorateResponseMessage(true, Utils::Base64Encode(buffer, blockSizeInBytes)); // empty string if the buffer is empty
            }
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// *************************************************************************************************
ResponseMessage CommandsHandler::WriteBlock(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 3, response.message))
    {
        DWORD address;
        vector<DWORD> values;
        if (!Utils::ConvertHexStringToDword(arguments[1], address) || !Utils::ConvertHexStringToDwordVector(arguments[2], m_array_delimiter, values))
        {
            const string errStr = "invalid arguments: block address and values are expected in Hex format";
            LOG_WARNING << "Failed to write block, " << errStr << endl;
            response.message = DecorateResponseMessage(false, errStr);
        }
        else
        {
            // perform write block
            const OperationStatus status = m_host.GetDeviceManager().WriteBlock(arguments[0], address, values);
            if (!status)
            {
                LOG_ERROR << "Failed to write block: " << status.GetStatusMessage() << endl;
                response.message = DecorateResponseMessage(false, status.GetStatusMessage());
            }
            else
            {
                response.message = DecorateResponseMessage(true);
            }
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::InterfaceReset(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 1, response.message))
    {
        const OperationStatus status = m_host.GetDeviceManager().InterfaceReset(arguments[0]);
        if (!status)
        {
            LOG_ERROR << "Failed to perform interface reset: " << status.GetStatusMessage() << endl;
            response.message = DecorateResponseMessage(false, status.GetStatusMessage());
        }
        else
        {
            response.message = DecorateResponseMessage(true);
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// *************************************************************************************************

ResponseMessage CommandsHandler::GetPmcAgentVersion(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    static const char* const PMC_AGENT_VER = "1.1";

    if (ValidArgumentsNumber(__FUNCTION__, arguments.size(), 1, response.message))
    {
        response.message = DecorateResponseMessage(true, PMC_AGENT_VER);
    }

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}


// *************************************************************************************************

ResponseMessage CommandsHandler::EnableASPM(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

#ifdef _WINDOWS
    response.message = DecorateResponseMessage(false, LINUX_ONLY_SUPPORT_STR);
#else

    LOG_DEBUG << "Enabling ASPM (SPARROW-only)" << std::endl;
    ShellCommandExecutor disableAspmCommand("setpci -d:310 80.b=2:3");
    LOG_DEBUG << disableAspmCommand << std::endl;

    // setpci always exits with zero, even when failure occurs
    if (0 == disableAspmCommand.ExitStatus() &&
        0 == disableAspmCommand.Output().size())
    {
        response.message = DecorateResponseMessage(true);
    }
    else
    {
        response.message = DecorateResponseMessage(false, disableAspmCommand.ToString());
    }

#endif

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// ***********************************************************************************************

ResponseMessage CommandsHandler::DisableASPM(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

#ifdef _WINDOWS
    response.message = DecorateResponseMessage(false, LINUX_ONLY_SUPPORT_STR);
#else

    LOG_DEBUG << "Disabling ASPM (SPARROW-only)" << std::endl;
    ShellCommandExecutor disableAspmCommand("setpci -d:310 80.b=0:3");
    LOG_DEBUG << disableAspmCommand << std::endl;

    // setpci always exits with zero, even when failure occurs
    if (0 == disableAspmCommand.ExitStatus() &&
        0 == disableAspmCommand.Output().size())
    {
        response.message = DecorateResponseMessage(true);
    }
    else
    {
        response.message = DecorateResponseMessage(false, disableAspmCommand.ToString());
    }

#endif

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// *************************************************************************************************

ResponseMessage CommandsHandler::GetASPM(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

#ifdef _WINDOWS
    response.message = DecorateResponseMessage(false, LINUX_ONLY_SUPPORT_STR);
#else

    LOG_DEBUG << "Querying ASPM (SPARROW-only)" << std::endl;
    ShellCommandExecutor disableAspmCommand("setpci -d:310 80.b");
    LOG_DEBUG << disableAspmCommand << std::endl;

    // setpci always exits with zero, even when failure occurs
    if (0 == disableAspmCommand.ExitStatus() &&
        1 == disableAspmCommand.Output().size())
    {
        response.message = DecorateResponseMessage(
            true, ((disableAspmCommand.Output()[0] == "00") ? FALSE_STR : TRUE_STR));
    }
    else
    {
        response.message = DecorateResponseMessage(false, disableAspmCommand.ToString());
    }

#endif

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// ***********************************************************************************************

ResponseMessage CommandsHandler::AllocPmc(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

#ifdef _WINDOWS
    response.message = DecorateResponseMessage(false, LINUX_ONLY_SUPPORT_STR);
#else

    if (ValidArgumentsNumber(__FUNCTION__, arguments.size(), 3, response.message))
    {
        unsigned descSize;
        unsigned descNum;
        if (!Utils::ConvertDecimalStringToUnsignedInt(arguments[1], descSize) ||
            !Utils::ConvertDecimalStringToUnsignedInt(arguments[2], descNum))
        {
            response.message = DecorateResponseMessage(false, INVALID_ARGUMENTS_STR);
        }
        else
        {
            const OperationStatus status = m_host.GetDeviceManager().AllocPmc(arguments[0], descSize, descNum);
            if (!status)
            {
                LOG_ERROR << "Failed to allocate PMC ring: " << status.GetStatusMessage() << std::endl;
                response.message = DecorateResponseMessage(false, status.GetStatusMessage());
            }
            else
            {
                response.message = DecorateResponseMessage(true);
            }
        }
    }

#endif

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::DeallocPmc(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

#ifdef _WINDOWS
    response.message = DecorateResponseMessage(false, LINUX_ONLY_SUPPORT_STR);
#else

    bool bSuppressError = false;

    if (arguments.size() < 1 || arguments.size() > 2)
    {
        LOG_WARNING << "Mismatching number of arguments in " << __FUNCTION__ << ": expected [1 .. 2] but got " << numberOfArguments << endl;
        response.message = DecorateResponseMessage(false, INVALID_ARGUMENTS_NUMBER_STR);

        response.type = REPLY_TYPE_BUFFER;
        response.length = response.message.size();
        return response;
    }

    if (2 == arguments.size())
    {
        // PMV v1.6+
        if (!Utils::ConvertStringToBool(arguments[1], bSuppressError))
        {
            response.message = DecorateResponseMessage(false, INVALID_ARGUMENTS_STR);
        }
    }

    const OperationStatus status = m_host.GetDeviceManager().DeallocPmc(arguments[0], bSuppressError);
    if (!status)
    {
        LOG_ERROR << "Failed to de-allocate PMC ring: " << status.GetStatusMessage() << std::endl;
        response.message = DecorateResponseMessage(false, status.GetStatusMessage());
    }
    else
    {
        response.message = DecorateResponseMessage(true);
    }

#endif

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// *************************************************************************************************

ResponseMessage CommandsHandler::CreatePmcFile(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

#ifdef _WINDOWS
    response.message = DecorateResponseMessage(false, LINUX_ONLY_SUPPORT_STR);
#else

    if (ValidArgumentsNumber(__FUNCTION__, arguments.size(), 2, response.message))
    {
        unsigned refNumber;
        if (!Utils::ConvertDecimalStringToUnsignedInt(arguments[1], refNumber))
        {
            response.message = DecorateResponseMessage(false, INVALID_ARGUMENTS_STR);
        }
        else
        {
            const OperationStatus status = m_host.GetDeviceManager().CreatePmcFiles(arguments[0], refNumber);
            if (!status)
            {
                LOG_ERROR << "PMC data file creation failed. Error: " << status.GetStatusMessage() << std::endl;
                response.message = DecorateResponseMessage(false, status.GetStatusMessage());
            }
            else
            {
                response.message = DecorateResponseMessage(true, status.GetStatusMessage());
            }
        }
    }

#endif

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::FindPmcDataFile(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

#ifdef _WINDOWS
    response.message = DecorateResponseMessage(false, LINUX_ONLY_SUPPORT_STR);
#else

    if (ValidArgumentsNumber(__FUNCTION__, arguments.size(), 2, response.message))
    {
        unsigned refNumber;
        if (!Utils::ConvertDecimalStringToUnsignedInt(arguments[1], refNumber))
        {
            response.message = DecorateResponseMessage(false, INVALID_ARGUMENTS_STR);
        }
        else
        {
            std::string foundPmcFilePath;
            const OperationStatus status = m_host.GetDeviceManager().FindPmcDataFile(arguments[0], refNumber, foundPmcFilePath);
            if (!status)
            {
                LOG_ERROR << "PMC data file lookup failed. Error: " << status.GetStatusMessage() << std::endl;
                response.message = DecorateResponseMessage(false, status.GetStatusMessage());
            }
            else
            {
                response.message = foundPmcFilePath;
                response.type = REPLY_TYPE_FILE;
            }
        }
    }

#endif // _WINDOWS

    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::FindPmcRingFile(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

#ifdef _WINDOWS
    response.message = DecorateResponseMessage(false, LINUX_ONLY_SUPPORT_STR);
#else

    if (ValidArgumentsNumber(__FUNCTION__, arguments.size(), 2, response.message))
    {
        unsigned refNumber;
        if (!Utils::ConvertDecimalStringToUnsignedInt(arguments[1], refNumber))
        {
            response.message = DecorateResponseMessage(false, INVALID_ARGUMENTS_STR);
        }
        else
        {
            std::string foundPmcFilePath;
            const OperationStatus status = m_host.GetDeviceManager().FindPmcRingFile(arguments[0], refNumber, foundPmcFilePath);
            if (!status)
            {
                LOG_ERROR << "PMC ring file lookup failed. Error: " << status.GetStatusMessage() << std::endl;
                response.message = DecorateResponseMessage(false, status.GetStatusMessage());
            }
            else
            {
                response.message = foundPmcFilePath;
                response.type = REPLY_TYPE_FILE;
            }
        }
    }

#endif // _WINDOWS

    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::GetHostName(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        response.message = DecorateResponseMessage(true, m_host.GetHostInfo().GetHostName());
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::GetHostCapabilities(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        ostringstream message;
        message << m_host.GetHostInfo().GetHostCapabilities();
        response.message = DecorateResponseMessage(true, message.str());
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::LogCollectorHandler(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    // arguments[0] - device name
    // arguments[1] - cpu type
    // arguments[2] - operation
    if (0 == numberOfArguments)
    {
        LOG_WARNING << "Got 0 aruments while expecting at least for command to run. Use HELP command to get more info" << endl;
        response.message = DecorateResponseMessage(false, INVALID_ARGUMENTS_NUMBER_STR);
        response.type = REPLY_TYPE_BUFFER;
        response.length = response.message.size();
        return response;
    }

    string statusMessage;
    const vector<string> parameters(arguments.begin() + 1, arguments.end());
    LegacyLogCollectorHandler& handler = LegacyLogCollectorHandler::GetLogCollectorHandler();
    const bool res = handler.HandleTcpCommand(arguments[0], parameters, statusMessage);
    response.message = DecorateResponseMessage(res, statusMessage);
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::FlashControlHandler(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    // arguments[0] - device name
    // arguments[1] - operation
    // arguments[2] - parameter list (optional)
    if (numberOfArguments < 1 || numberOfArguments > 3)
    {
        LOG_WARNING << "Mismatching number of arguments in " << __FUNCTION__ << ": expected for  1-3 arguments" << " but got " << numberOfArguments << endl;
        response.message = DecorateResponseMessage(false, INVALID_ARGUMENTS_NUMBER_STR);
        response.type = REPLY_TYPE_BUFFER;
        response.length = response.message.size();
        return response;
    }

    FlashControlCommandsHandler& flashHandler = FlashControlCommandsHandler::GetFlashControlCommandsHandler();
    string statusMessage;
    string parameterList;
    if (3 == arguments.size())
    {
        parameterList = arguments[2];
    }
    const bool res = flashHandler.HandleTcpCommand(arguments[1] /*deviceList*/, arguments[0] /*operation*/, parameterList /*parameterList*/, statusMessage);
    response.message = DecorateResponseMessage(res, statusMessage);
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::DriverConfiguration(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    // arguments[0] - operation
    // arguments[1] - parameter list (optional)
    if (numberOfArguments < 1 || numberOfArguments > 2)
    {
        LOG_WARNING << "Mismatching number of arguments in " << __FUNCTION__ << ": expected for 1 or 2 arguments" << " but got " << numberOfArguments << endl;
        response.message = DecorateResponseMessage(false, INVALID_ARGUMENTS_NUMBER_STR);
        response.type = REPLY_TYPE_BUFFER;
        response.length = response.message.size();
        return response;
    }

    DriverConfigurationHandler& driverHandler = DriverConfigurationHandler::GetDriverConfigurationHandler();
    string statusMessage;
    string parameterList;
    if (2 == arguments.size())
    {
        parameterList = arguments[1];
    }
    const bool res = driverHandler.HandleTcpCommand(arguments[0] /*operation*/, parameterList /*parameterList*/, statusMessage);
    response.message = DecorateResponseMessage(res, statusMessage);
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::GetTime(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        response.message = DecorateResponseMessage(true);
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

// *************************************************************************************************
ResponseMessage CommandsHandler::SetDriverMode(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 2, response.message))
    {
#ifdef _WINDOWS
        int newMode = IOCTL_WBE_MODE;
        int oldMode = IOCTL_WBE_MODE;

        if ("WBE_MODE" == arguments[1])
        {
            newMode = IOCTL_WBE_MODE;
        }
        else if ("WIFI_STA_MODE" == arguments[1])
        {
            newMode = IOCTL_WIFI_STA_MODE;
        }
        else if ("WIFI_SOFTAP_MODE" == arguments[1])
        {
            newMode = IOCTL_WIFI_SOFTAP_MODE;
        }
        else if ("CONCURRENT_MODE" == arguments[1])
        {
            newMode = IOCTL_CONCURRENT_MODE;
        }
        else if ("SAFE_MODE" == arguments[1])
        {
            newMode = IOCTL_SAFE_MODE;
        }
        else
        {
            response.message = "Unknown driver mode " + arguments[1];
            response.type = REPLY_TYPE_BUFFER;
            response.length = response.message.size();
            return response;
        }
#else
        int newMode = 0;
        int oldMode = 0;
#endif
        const OperationStatus status = m_host.GetDeviceManager().SetDriverMode(arguments[0], newMode, oldMode);
        if (!status)
        {
            LOG_ERROR << "Failed to set driver mode: " << status.GetStatusMessage() << endl;
            response.message = DecorateResponseMessage(false, status.GetStatusMessage());
        }
        else
        {
#ifdef _WINDOWS
            string message;

            switch (oldMode)
            {
            case IOCTL_WBE_MODE:
                message = "WBE_MODE";
                break;
            case IOCTL_WIFI_STA_MODE:
                message = "WIFI_STA_MODE";
                break;
            case IOCTL_WIFI_SOFTAP_MODE:
                message = "WIFI_SOFTAP_MODE";
                break;
            case IOCTL_CONCURRENT_MODE:
                message = "CONCURRENT_MODE";
                break;
            case IOCTL_SAFE_MODE:
                message = "SAFE_MODE";
                break;
            default:
                break;
            }

            response.message = DecorateResponseMessage(true, message);
#else
            response.message = DecorateResponseMessage(true);
#endif
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::GetHostManagerVersion(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        const string res = GetToolsVersion();
        response.message = DecorateResponseMessage(true, res);
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::GetMinDmToolsVersion(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        response.message = DecorateResponseMessage(true, ::GetMinDmToolsVersion());
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// *************************************************************************************************
ResponseMessage CommandsHandler::GetHostOS(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        response.message = DecorateResponseMessage(true, GetOperatingSystemName());
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// *************************************************************************************************
ResponseMessage CommandsHandler::GetHostPlatform(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        response.message = DecorateResponseMessage(true, GetPlatformName());
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// *************************************************************************************************
ResponseMessage CommandsHandler::GetHostEnumerationMode(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        const string res = m_host.IsEnumerating() ? "true" : "false";
        response.message = DecorateResponseMessage(true, res);
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::SetHostEnumerationMode(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    bool enumerationMode = false;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 1, response.message))
    {
        if (!Utils::ConvertStringToBool(arguments[0], enumerationMode))
        {
            response.message = DecorateResponseMessage(false, INVALID_ARGUMENTS_STR);
        }
        m_host.SetIsEnumerating(enumerationMode);

        const string mode = enumerationMode ? "Enumerating" : "Not_Enumerating";
        LOG_INFO << "Host is now " << mode << endl;
        ostringstream message;
        message << "Enumeration mode set to: " << mode;
        response.message = DecorateResponseMessage(true, message.str());

    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}


// *************************************************************************************************

ResponseMessage CommandsHandler::DriverCommand(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 5, response.message))
    {
        DWORD commandId, inBufSize, outBufSize;
        std::vector<unsigned char> inputBuf;
        if (!(Utils::ConvertHexStringToDword(arguments[1], commandId)
            && Utils::ConvertHexStringToDword(arguments[2], inBufSize)
            && Utils::ConvertHexStringToDword(arguments[3], outBufSize)
            && Utils::Base64Decode(arguments[4], inputBuf)
            && inputBuf.size() == inBufSize)) // inBufSize is the size of the original binary buffer before Base64 encoding
        {
            response.message = DecorateResponseMessage(false, INVALID_ARGUMENTS_STR);
        }
        else
        {
            if (g_LogConfig.ShouldPrint(LOG_SEV_VERBOSE)) // spare all the formatting when not needed
            {
                ostringstream ss;
                ss << __FUNCTION__ << ": [command id: " << Hex<>(commandId);
                ss << " - " << (commandId == 0 ? "WMI" : commandId == 1 ? "Generic command" : "Unknown");
                ss << "], Payload (bytes):" << endl;
                for (const auto& byteVal : inputBuf)
                {
                    ss << "\t" << Hex<>(byteVal) << endl;
                }
                LOG_VERBOSE << ss.str() << endl;
            }

            std::vector<unsigned char> outputBuf(outBufSize, 0);

            const OperationStatus status = m_host.GetDeviceManager().DriverControl(arguments[0], commandId, inputBuf.data(), inBufSize,
                                                                                   outBufSize ? outputBuf.data() : nullptr, outBufSize);
            if (!status)
            {
                LOG_ERROR << "Failed to send driver command: " << status.GetStatusMessage() << endl;
                response.message = DecorateResponseMessage(false, status.GetStatusMessage());
            }
            else
            {
                LOG_DEBUG << __FUNCTION__ << ": Success" << endl;
                response.message = DecorateResponseMessage(true, Utils::Base64Encode(outputBuf)); // empty string if the buffer is empty
            }
        }
    }

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// *************************************************************************************************
ResponseMessage CommandsHandler::GetDeviceSilenceMode(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 1, response.message))
    {
        bool silentMode = false;
        const OperationStatus status = m_host.GetDeviceManager().GetDeviceSilentMode(arguments[0], silentMode);
        if (!status)
        {
            LOG_ERROR << "Failed to get device silence mode: " << status.GetStatusMessage() << endl;
            response.message = DecorateResponseMessage(false, status.GetStatusMessage());
        }
        else
        {
            ostringstream message;
            message << (silentMode ? 1 : 0);
            response.message = DecorateResponseMessage(true, message.str());
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

ResponseMessage CommandsHandler::SetDeviceSilenceMode(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    bool silentMode = false;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 2, response.message))
    {
        {
            if (!Utils::ConvertStringToBool(arguments[1], silentMode))
            {
                response.message = DecorateResponseMessage(false, INVALID_ARGUMENTS_STR);
            }
            const OperationStatus status = m_host.GetDeviceManager().SetDeviceSilentMode(arguments[0], silentMode);
            if (!status)
            {
                LOG_ERROR << "Failed to set device silence mode: " << status.GetStatusMessage() << endl;
                response.message = DecorateResponseMessage(false, status.GetStatusMessage());
            }
            else
            {
                string mode = silentMode ? "Silenced" : "UnSilenced";
                LOG_INFO << "Device " << arguments[0] << " is now " << mode << endl;
                ostringstream message;
                message << "Silent mode set to: " << silentMode;
                response.message = DecorateResponseMessage(true, message.str());
            }
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    Host::GetHost().PushEvent(SilenceStatusChangedEvent(arguments[0], silentMode));
    return response;
}

ResponseMessage CommandsHandler::GetConnectedUsers(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {

        std::set<std::string> connectedUserList = m_host.GetHostInfo().GetConnectedUsers();
        ostringstream os;
        for (const auto & cl : connectedUserList)
        {
            os << cl << " ";
        }

        response.message = DecorateResponseMessage(true, os.str());
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

ResponseMessage CommandsHandler::GetDeviceCapabilities(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 1, response.message))
    {
        uint32_t deviceCapabilitiesMask;
        const OperationStatus status = m_host.GetDeviceManager().GetDeviceCapabilities(arguments[0], deviceCapabilitiesMask);
        if (!status)
        {
            LOG_ERROR << "Failed to get device capabilities: " << status.GetStatusMessage() << endl;
            response.message = DecorateResponseMessage(false, status.GetStatusMessage());
        }
        else
        {
            ostringstream message;
            message << deviceCapabilitiesMask;
            response.message = DecorateResponseMessage(true, message.str());
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

ResponseMessage CommandsHandler::GetFwHealthState(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 1, response.message))
    {
        int fwHealthState = 0; // dummy initialization to spare include to FwStateProvider.h
        const OperationStatus status = m_host.GetDeviceManager().GetFwHealthState(arguments[0], fwHealthState);
        if (!status)
        {
            LOG_ERROR << "Failed to get FW health state: " << status.GetStatusMessage() << endl;
            response.message = DecorateResponseMessage(false, status.GetStatusMessage());
        }
        else
        {
            ostringstream message;
            message << fwHealthState;
            response.message = DecorateResponseMessage(true, message.str());
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

// **************************************UDP commands handlers*********************************************************** //
ResponseMessage CommandsHandler::GetHostNetworkInfo(const vector<std::string>& arguments, unsigned int numberOfArguments)
{
    HANDLER_ENTER;
    ResponseMessage response;

    if (!arguments.empty())
    {
        response.message = DecorateResponseMessage(false, "Failed to get host's info: expected zero argument");
    }
    else
    {
        response.message = "GetHostIdentity;" + m_host.GetHostInfo().GetIps().m_ip + ";" + m_host.GetHostInfo().GetHostName();
    }

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ConnectionStatus CommandsHandler::ExecuteCommand(const string& message, ResponseMessage& referencedResponse)
{
    LOG_VERBOSE << ">>> Command: " << message << std::endl;

    MessageParser messageParser(message);
    const string commandName = messageParser.GetCommandFromMessage();

    if (m_functionHandler.find(commandName) == m_functionHandler.end())
    { //There's no such a command, the return value from the map would be null
        LOG_WARNING << "Unknown command from client: " << commandName << endl;
        referencedResponse.message = "Unknown command: " + commandName;
        referencedResponse.length = referencedResponse.message.size();
        referencedResponse.type = REPLY_TYPE_BUFFER;
        return KEEP_CONNECTION_ALIVE;
    }
    referencedResponse = (this->*m_functionHandler[commandName])(messageParser.GetArgsFromMessage(), messageParser.GetNumberOfArgs()); //call the function that fits commandName

    LOG_VERBOSE << "<<< Reply: " << referencedResponse.message << std::endl;
    return KEEP_CONNECTION_ALIVE;
}

// *************************************************************************************************

bool CommandsHandler::ValidArgumentsNumber(const string& functionName, size_t numberOfArguments, size_t expectedNumOfArguments, string& responseMessage) const
{
    if (expectedNumOfArguments != numberOfArguments)
    {
        ostringstream oss;
        oss << "mismatching number of arguments: expected " << expectedNumOfArguments << ", but got " << numberOfArguments;
        LOG_WARNING << functionName << ": " << oss.str() << endl;
        responseMessage = DecorateResponseMessage(false, oss.str());
        return false;
    }
    return true;
}
