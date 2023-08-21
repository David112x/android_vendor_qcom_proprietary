/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include <locale> // for std::tolower
#include <sstream>
#include <iostream>
#include <map>
#include <string>

#include "LogReader.h"
#include "Device.h"
#include "DriverAPI.h"
#include "Host.h"
#include "PersistentConfiguration.h"
#include "TaskManager.h"
#include "DebugLogger.h"
#include "LogCollectorDefinitions.h"
#include "DeviceManager.h"
#include "LogCollectorService.h"

using namespace std;
using namespace log_collector;

namespace
{
    //Old Api Only
    string ConvertModuleVerbosityStructToString(const module_level_info& verbosityStruct)
    {
        stringstream verbosityStringStream;

        if (verbosityStruct.verbose_level_enable)
        {
            verbosityStringStream << "V";
        }
        if (verbosityStruct.info_level_enable)
        {
            verbosityStringStream << "I";
        }
        if (verbosityStruct.error_level_enable)
        {
            verbosityStringStream << "E";
        }
        if (verbosityStruct.warn_level_enable)
        {
            verbosityStringStream << "W";
        }

        return verbosityStringStream.str();
    }

    const char configuration_delimiter_string = ',';
}


bool LogReader::Tracer3ppLogsFilterOn = false;




LogReader::LogReader(const std::string& deviceName, CpuType tracerType,
    uint32_t logBufferInfoAddress, int ahbToLinkerDelta) :
    m_deviceName(deviceName),
    m_tracerCpuType(tracerType),
    m_debugLogPrefix(m_deviceName + "_" + CPU_TYPE_TO_STRING[tracerType] + " tracer: "),
    m_pollerTaskName(CPU_TYPE_TO_STRING[tracerType] + "_log_" + m_deviceName),
    m_valid(false),
    m_pollerActive(false),
    m_fwInit(false),
    m_logHeader(new log_buffer()),
    m_logBufferInfoAddress(logBufferInfoAddress),
    m_ahbToLinkerDelta(ahbToLinkerDelta),
    m_logAddress(Device::sc_invalidAddress),
    m_logBufferSizeInDwords(256),    // Minimal value, will be updated
    m_bufferConsumer(new BufferConsumer(m_deviceName, m_tracerCpuType, m_logBufferSizeInDwords)),
    m_prevLogOffset(-1)
{
}


LogReader::~LogReader()
{
    Host::GetHost().GetTaskManager().UnregisterTaskBlocking(m_pollerTaskName);
}

void LogReader::Poll()
{
    if (!(Host::GetHost().GetDeviceManager().DoesDeviceExist(m_deviceName)) || !m_bufferConsumer->IsAnyChunkConsumerActive() )
    {
        return;
    }

    if (!m_valid)
    {
        //buffer start address is not set.
        if (!UpdateDeviceInfo())
        {
            LOG_VERBOSE << m_debugLogPrefix << "Log reader cannot be validated yet" << endl;
            return;
        }

        LOG_DEBUG << m_debugLogPrefix << "Log reader was successfully validated" << endl;
    }

    shared_ptr<Device> device;
    const OperationStatus os = Host::GetHost().GetDeviceManager().GetDeviceByName(m_deviceName, device);
    if (!os)
    {
        LOG_ERROR << m_debugLogPrefix << "Cannot collect logs: " << os.GetStatusMessage() << endl;
        return;
    }

    if (device->GetSilenceMode())
    {
        // In this case collection is not needed (device is silent or nor recording nor publishing)
        //TODO: (PHASE2) may be pause the log collector.
        LOG_VERBOSE << m_debugLogPrefix << "cannot collect logs, device is silent" << endl;
        return;
    }

    if (!m_fwInit)
    {
        const FwStateProvider* fwStateProvider = device->GetFwStateProvider();
        m_fwInit = fwStateProvider->IsInitialized();
        if (m_fwInit && GetModuleInfoFromDevice())
        {
            LOG_DEBUG << m_debugLogPrefix << "FW was initialized, reporting updated FW info to active consumers" << endl;
            m_bufferConsumer->ReportAndUpdateFwVersion(fwStateProvider, m_logHeader->module_level_info_array);
        }
        // if FW not initialized yet, it does not prevent log collection
    }

    GetNextLogs();
}

bool LogReader::UpdateDeviceInfo()
{
    shared_ptr<Device> device;
    const OperationStatus os = Host::GetHost().GetDeviceManager().GetDeviceByName(m_deviceName, device);
    if (!os)
    {
        LOG_ERROR << m_debugLogPrefix << "Cannot update device info: " << os.GetStatusMessage() << endl;
        m_valid = false;
        return m_valid;
    }

    // test if FW is initialized
    // if so, the correct FW info will be used in file header and there is no need to update it latter
    const FwStateProvider* fwStateProvider = device->GetFwStateProvider();
    TestFwInitializationState(fwStateProvider);

    const bool res = ComputeLogBufferStartAddress() && GetModuleInfoFromDevice();
    if (!res)
    {
        LOG_VERBOSE << m_debugLogPrefix << "Failed to initialize Log buffer and Module info." << endl;
        m_valid = false;
        return m_valid; // Error in device should not start/continue recording
    }

    if (!m_bufferConsumer->Reset(m_logHeader->module_level_info_array, m_logBufferSizeInDwords, fwStateProvider))
    {
        m_valid = false;
        return m_valid;
    }

    m_valid = true;
    return m_valid;
}

void LogReader::TestFwInitializationState(const FwStateProvider* fwStateProvider)
{
    if (m_fwInit)
    {
        return;
    }

    m_fwInit = fwStateProvider->IsInitialized();
    if (m_fwInit)
    {
        LOG_DEBUG << m_debugLogPrefix << "FW is initialized, FW state is " << fwStateProvider->GetFwHealthState() << endl;
    }
}

void LogReader::GetNextLogs()
{
    // **** NOTE!!! LOCKS are only in the inner functions so pay attention not to dead LOCK
    // **** by LOCKING from high level functions.
    if (!ReadLogBuffer())
    {
        return;
    }

    // Prepare a header pointing to log buffer top
    log_buffer *h = (log_buffer*)m_logBufferContent.data(); // h contains all the log buffer (including the log buffer header)

    m_bufferConsumer->ConsumeBuffer(h);
}

bool LogReader::SetPollingInterval(unsigned pollingInterval) const
{
    lock_guard<mutex> lock(m_pollRegisterMutex);
    if (m_pollerActive)
    {
        LOG_INFO << m_debugLogPrefix << "SetPollingInterval to: " << pollingInterval << endl;
        Host::GetHost().GetTaskManager().ChangeExecutionInterval(m_pollerTaskName, std::chrono::milliseconds(pollingInterval));
    }
    return true;
}

OperationStatus LogReader::ApplyModuleVerbosity()
{
    LOG_DEBUG << m_debugLogPrefix << "Apply module verbosity" << endl;
    if (!ComputeLogBufferStartAddress())
    {
        return OperationStatus(false, "Cannot compute log buffer start/size");
    }

    shared_ptr<Device> device;
    OperationStatus os = Host::GetHost().GetDeviceManager().GetDeviceByName(m_deviceName, device);
    if (!os)
    {
        LOG_ERROR << m_debugLogPrefix << "Cannot apply module verbosity: " << os.GetStatusMessage() << endl;
        return os;
    }

    const bool writeBlockRes = device->GetDriver()->WriteBlock(m_logAddress + sizeof(m_logHeader->write_ptr),
        sizeof(m_logHeader->module_level_info_array), reinterpret_cast<char*>(m_logHeader->module_level_info_array));

    if (!writeBlockRes)
    {
        ostringstream st;
        st << m_debugLogPrefix << "Failed to write module verbosity structure for "
            << "(address " << Address(m_logAddress + sizeof(m_logHeader->write_ptr))
            << ", size of data: " << sizeof(m_logHeader->module_level_info_array) << ")";
        LOG_ERROR << "Cannot apply module verbosity: " << st.str() << endl;
        return OperationStatus(false, st.str());
    }

    LOG_VERBOSE << m_debugLogPrefix << "Module verbosity for " << m_tracerCpuType << " was set" << endl;
    return OperationStatus(true);
}

bool ConvertModuleVerbosityStringToStruct(const string& verbosityString, module_level_info& verbosityStruct)
{
    module_level_info verbosityStructInternal;
    for (const auto& c : verbosityString)
    {
        switch (c)
        {
        case 'V':
            verbosityStructInternal.verbose_level_enable = 1;
            break;
        case 'I':
            verbosityStructInternal.info_level_enable = 1;
            break;
        case 'E':
            verbosityStructInternal.error_level_enable = 1;
            break;
        case 'W':
            verbosityStructInternal.warn_level_enable = 1;
            break;
        default:
            return false;
        }
    }
    verbosityStruct = verbosityStructInternal;
    return true;
}

bool LogReader::SetModuleVerbosityInner(const string& module, const string& level)
{
    for (int i = 0; i < NUM_MODULES; i++)
    {
        if (module_names[i] == module)
        {
            module_level_info newValue;
            if (!ConvertModuleVerbosityStringToStruct(level, newValue))
            {
                return false;
            }

            LOG_DEBUG << "setting module verbosity for module: " << module << "to: " << level << endl;
            m_logHeader->module_level_info_array[i] = newValue;
            return true;
        }
    }
    LOG_ERROR << "Failed to set verbosity " << level << " to module " << module << endl;
    return false;
}


bool LogReader::SetMultipleModulesVerbosity(const string& modulesVerbosity)
{
    // split in case there are multiple modules in the same line, i.e: SYSTEM=VIEW,DRIVER=VIEW
    stringstream ss(modulesVerbosity);
    string moduleLevel;
    while (std::getline(ss, moduleLevel, configuration_delimiter_string))
    {
        size_t delimeterPos = moduleLevel.find('=');
        if (delimeterPos != string::npos && (moduleLevel.length() > delimeterPos)) // ">" and not ">=" for skipping '='
        {
            SetModuleVerbosityInner(moduleLevel.substr(0, delimeterPos), moduleLevel.substr(delimeterPos + 1));
        }
    }

    return ApplyModuleVerbosity();
}

bool LogReader::SetModuleVerbosityFromAssignment(const std::string& assignment)
{
    std::size_t found = assignment.find("=");
    if (std::string::npos != found)
    {
        return SetMultipleModulesVerbosity(assignment.substr(found + 1));
    }
    return false;
}


void LogReader::ReportDeviceRemoved()
{
    if (m_valid)
    {
        LOG_DEBUG << m_debugLogPrefix << "device removal reported, going to reset Log reader and close recording files" << endl;
        m_bufferConsumer->ReportDeviceRemoved();
        ResetState();
    }
}

void LogReader::ResetState()
{
    m_bufferConsumer->ResetRWPointers();

    // reset validity & FW initialization state
    // to be validated by the first poll iteration after next start in order to create new log files
    m_fwInit = false;
    m_valid = false;
}

OperationStatus LogReader::RegisterPoller()
{
    lock_guard<mutex> lock(m_pollRegisterMutex);
    if (!m_pollerActive)
    {
        if (!Host::GetHost().GetTaskManager().RegisterTask(
            std::bind(&LogReader::Poll, this), m_pollerTaskName, static_cast<std::chrono::milliseconds>(int64_t(Host::GetHost().GetConfiguration().LogCollectorConfiguration.PollingIntervalMs))))
        {
            ostringstream oss;
            oss << m_debugLogPrefix << "Failed to register logging task";
            return OperationStatus(false, oss.str());
        }
        m_pollerActive = true;
        LOG_DEBUG << m_debugLogPrefix << "registered logging task " << m_pollerTaskName << endl;
    }
    return OperationStatus();
}

void LogReader::UnRegisterPoller()
{
    lock_guard<mutex> lock(m_pollRegisterMutex);
    if(m_pollerActive)
    {
        Host::GetHost().GetTaskManager().UnregisterTaskBlocking(m_pollerTaskName);
        m_pollerActive = false;
        LOG_DEBUG << m_debugLogPrefix << "unregistered logging task " << m_pollerTaskName << endl;
    }
}


OperationStatus LogReader::SetModuleVerbosity(const std::string& module, const std::string& level)
{
    bool os = SetModuleVerbosityInner(module, level);
    if (os)
    {
        os = ApplyModuleVerbosity();
    }
    return OperationStatus(os);
}

bool LogReader::GetModuleLevelInfo(std::vector<std::pair<std::string, std::string> > &verbosityData)
{
    if (!GetModuleInfoFromDevice())
    {
        return false;
    }
    for (int i = 0; i < NUM_MODULES; i++)
    {
        verbosityData.emplace_back(make_pair( module_names[i], ConvertModuleVerbosityStructToString(m_logHeader->module_level_info_array[i])));
    }
    return true;
}

OperationStatus LogReader::ActivateLogging(LoggingType loggingType, bool compress, bool upload)
{
    if (IsLoggingValid(loggingType))
    {
        return OperationStatus(false, "Logs are already being recorded");
    }

    // no need to test for device existence
    // in case of start logging it was tested by the Logging Service
    // and device discovery is activated directly by the device manager

    OperationStatus os = m_bufferConsumer->ConfigureAndTestRecordingConditions(loggingType, compress, upload);
    if (!os)
    {
        return os;
    }

    os = RegisterPoller();
    if (!os)
    {
        return os;
    }

    // Notes:
    // 1. Do not mark valid, should be done in one place - poll iteration.
    // Otherwise, it may prevent recording after interface restart (poll will not try to open new files).
    // 2. Should mark chunk consumers enabled instead of calling ActivateChunkConsumer.
    // Otherwise, new files will be created and then poll iteration will perform reset and create new ones.
    // 3. Chunk consumers shall be marked enabled, otherwise poll iteration will not perform their reset.
    m_bufferConsumer->SetActiveFlag(loggingType);
    return OperationStatus(true);
}

OperationStatus LogReader::RestoreLogging()
{
    // should be used only on restoring session - device was already running

    if (IsLogging()) // at least one consumer is enabled
    {
        LOG_DEBUG << m_debugLogPrefix << "resuming FW logging" << endl;
        return RegisterPoller();
    }

    return OperationStatus();
}

void LogReader::DeactivateLogging(LoggingType loggingType)
{
    m_bufferConsumer->StopConsumer(loggingType);
    LOG_DEBUG << m_debugLogPrefix << "Logging deactivation requested for " << loggingType << endl;

    if (!IsLoggingValid())
    {
        LOG_DEBUG << m_debugLogPrefix << "no more enabled logging consumers, going to stop polling, reset Log reader and close recording files" << endl;
        UnRegisterPoller();
        ResetState();
    }
}

OperationStatus LogReader::SplitRecordings()
{
    if(!IsLoggingValid())
    {
        return OperationStatus(false, "No active recordings");
    }
    return m_bufferConsumer->SplitRecordings();
}

bool LogReader::IsLoggingValid(LoggingType loggingType) const
{
    return m_valid && IsLogging(loggingType);
}


bool LogReader::IsLoggingValid() const
{
    // Checking if at least one of the consumers is logging
    return  m_valid && IsLogging();
}

bool LogReader::IsLogging(LoggingType loggingType) const
{
    return  m_bufferConsumer->IsActive(loggingType);
}

bool LogReader::IsLogging() const
{
    // Checking if at least one of the consumers is logging
    return IsLogging(XmlRecorder) || IsLogging(TxtRecorder) || IsLogging(RawPublisher);
}

bool LogReader::GetModuleInfoFromDevice() const
{
    shared_ptr<Device> device;
    const OperationStatus os = Host::GetHost().GetDeviceManager().GetDeviceByName(m_deviceName, device);
    if (!os)
    {
        LOG_ERROR << m_debugLogPrefix << "Cannot get module info: " << os.GetStatusMessage() << endl;
        return false;
    }

    const bool readBlockRes = device->GetDriver()->ReadBlock(
        m_logAddress + sizeof(m_logHeader->write_ptr),
        sizeof(m_logHeader->module_level_info_array),
        reinterpret_cast<char*>(m_logHeader->module_level_info_array));

    if (!readBlockRes)
    {
        LOG_ERROR << "Debug Log Prefix: " << m_debugLogPrefix
            << "Failed to read module verbosity structure for " << m_tracerCpuType
            << " Address: " << Address(m_logAddress + sizeof(m_logHeader->write_ptr))
            << " Size: " << sizeof(m_logHeader->module_level_info_array)
            << endl;
        return false;
    }

    for (int i = 0; i < NUM_MODULES; i++)
    {
        LOG_VERBOSE << "module_level_info_array[" << i << "] = " << m_logHeader->module_level_info_array[i] << endl;
    }
    return true;
}

bool LogReader::ReadLogBuffer()
{
    // Update FW & uCode log addresses add put it in m_logAddress
    if (!m_fwInit && !ComputeLogBufferStartAddress())
    {
        // Log buffer is not yet initialized, not an error
        return false;
    }

    shared_ptr<Device> device;
    const OperationStatus os = Host::GetHost().GetDeviceManager().GetDeviceByName(m_deviceName, device);
    if (!os)
    {
        LOG_ERROR << m_debugLogPrefix << "Cannot read log buffer: " << os.GetStatusMessage() << endl;
        return false;
    }

    // Read the actual log
    bool readBlockRes = device->GetDriver()->ReadBlock(
        m_logAddress,
        static_cast<DWORD>(m_logBufferContent.size()),
        reinterpret_cast<char*>(m_logBufferContent.data()));

    return readBlockRes;
}

bool LogReader::ComputeLogBufferStartAddress()
{
    DWORD logBufferInfoDword = 0;
    const OperationStatus os = Host::GetHost().GetDeviceManager().Read(m_deviceName, m_logBufferInfoAddress, logBufferInfoDword);
    if (!os)
    {
        LOG_VERBOSE << m_debugLogPrefix << "Log collector failed to read log offset address: " << os.GetStatusMessage() << endl;
        return false;
    }

    /* calculate first address of fw/ucode log buffer
                          +--------------------------+----------------+
     logBufferInfoDword = |3bits of buffer size index|29bits of offset|
                          +--------------------------+----------------+
    */

    const int logOffset = logBufferInfoDword & 0x1FFFFFFF; // we take only the lower 29 bit

    if (logOffset != m_prevLogOffset)
    {
        LOG_INFO << m_debugLogPrefix << "Log buffer offset changed from " << Address(m_prevLogOffset) << " to " << Address(logOffset) << endl;
        m_prevLogOffset = logOffset;
    }

    if (Device::sc_invalidAddress == logOffset)
    {
        // This may happen when the log buffer is not initialized yet
        LOG_VERBOSE << m_debugLogPrefix << "The log buffer is not initialized yet" << endl;
        return false;
    }

    m_logAddress = m_ahbToLinkerDelta + logOffset; // calculate the log buffer AHB start address.

    //we want to determine the buffer size:
    const int bufferSizeId = (logBufferInfoDword & 0xE0000000) >> 29; /* The three upper bits are the index in the size table */
                                                                      /* explained in GetBufferSizeById */
    const int bufferSizeInBytes = GetBufferSizeInBytesById(bufferSizeId);
    m_logBufferSizeInDwords = bufferSizeInBytes / sizeof(DWORD);

    // update buffer size accordingly
    m_logBufferContent.resize(sizeof(log_buffer) + bufferSizeInBytes);

    // update buffer consumer once buffer size updated
    m_bufferConsumer->UpdateBufferSize(m_logBufferSizeInDwords);

    LOG_VERBOSE << m_debugLogPrefix << "Log Buffer "
        << " Log Linker Address: " << Address(logOffset)
        << " Log AHB Address: " << Address(m_logAddress)
        << " Size ID: " << bufferSizeId
        << " Size: " << m_logBufferSizeInDwords << " DW"
        << endl;

    return true;
}

int LogReader::GetBufferSizeInBytesById(int bufferSizeId) const
{
    /*
    * The 3 MSBs of the value in REG_FW_USAGE_1 (for fw) and REG_FW_USAGE_2 (for uCode) are used to determine the size
    * of the their log buffer (respectively). The number created with these 3 MSBs is an index in the following sizes table:
    * 0 - default (4KB for FW, 1KB for uCode) - for backward compatibility
    * 1 - 1K
    * 2 - 2K
    * 3 - 4K
    * 4 - 8K
    * 5 - 16K
    * 6 - 32K
    * 7 - 64K
    */
    const int bufferSizeMultiplier[] = { 0, 1, 2, 4, 8, 16, 32, 64 };
    int result;
    const int KB = 1024;

    if (bufferSizeId < 0 || bufferSizeId > 7)
    {
        LOG_ERROR << "Could not get buffer size from logs header roll back to default size (CPU_TYPE_FW = 1024, CPU_TYPE_UCODE = 256)" << endl;
        bufferSizeId = 0;
    }

    if (bufferSizeId == 0)
    { // default values, for backward compatibility
        result = (m_tracerCpuType == CPU_TYPE_FW) ? 4 * KB : 1 * KB;
    }
    else
    {
        result = bufferSizeMultiplier[bufferSizeId] * KB; // buffer size id is calculated by 3 bits only, so its range is 0-7. Therefore can't be out of range.
    }

    return result;
}

