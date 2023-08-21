/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include <memory>
#include <vector>
#include <string>

#include "OperationStatus.h"
#include "ChunkConsumer.h"
#include "BufferConsumer.h"


class Device;

namespace log_collector
{

    class LogReader
    {
    public:
        static bool Tracer3ppLogsFilterOn;

        LogReader(const std::string& deviceName, CpuType tracerType,
            uint32_t logBufferInfoAddress, int ahbToLinkerDelta);
        LogReader(const LogReader& lg) = delete;
        LogReader& operator=(const LogReader& lc) = delete;
        ~LogReader();

        // should be called periodically until valid.
        bool UpdateDeviceInfo();
        OperationStatus ActivateLogging(LoggingType loggingType, bool compress, bool upload);
        OperationStatus RestoreLogging();
        void DeactivateLogging(LoggingType loggingType);
        OperationStatus SplitRecordings();

        bool IsLoggingValid(LoggingType loggingType) const;
        bool IsLoggingValid() const;

        bool IsLogging(LoggingType loggingType) const;
        bool IsLogging() const;

        // configuration functions
        OperationStatus SetModuleVerbosity(const std::string& module, const std::string& level);

        bool GetModuleLevelInfo(std::vector<std::pair<std::string, std::string> > &verbosityData);

        bool SetPollingInterval(unsigned pollingInterval) const;

        // for old api
        bool SetModuleVerbosityFromAssignment(const std::string& assignment);

        void ReportDeviceRemoved();

        void UnRegisterPoller();

    private:
        OperationStatus RegisterPoller();
        mutable std::mutex m_pollRegisterMutex;
        void ResetState();
        void TestFwInitializationState(const FwStateProvider* fwStateProvider);
        bool ComputeLogBufferStartAddress();
        int GetBufferSizeInBytesById(int bufferSizeId) const;
        bool SetModuleVerbosityInner(const std::string& module, const std::string& level);
        bool SetMultipleModulesVerbosity(const std::string& modulesVerbosity);
        // configurations
        OperationStatus ApplyModuleVerbosity();

        bool GetModuleInfoFromDevice() const;
        // OS agnostic read log function
        bool ReadLogBuffer();

        void GetNextLogs(); // distribute next logs chunck (without log lines that were already read)

        void Poll();

        const std::string m_deviceName;
        const CpuType m_tracerCpuType;
        const std::string m_debugLogPrefix;

        const std::string m_pollerTaskName;

        // Indicates that the log buffer is initialized.
        bool m_valid;
        bool m_pollerActive;

        // Indicates whether the fw initialized or not.
        bool m_fwInit;
        // log buffer members
        std::unique_ptr<log_buffer> m_logHeader;        // content of log buffer header
        const uint32_t m_logBufferInfoAddress;          // AHB address of log buffer info
        const int m_ahbToLinkerDelta;                   // AHB to Linker address diff for related memory range
        int m_logAddress;                               // log buffer AHB start address
        std::vector<unsigned char> m_logBufferContent;  // log buffer content
        unsigned m_logBufferSizeInDwords;

        std::unique_ptr<BufferConsumer> m_bufferConsumer;
        int m_prevLogOffset;
    };

}


