/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include <memory>
#include <string>
#include "LogReader.h"

class Device;

namespace log_collector
{
    class DeviceLoggingContext
    {
    public:
        explicit DeviceLoggingContext(const std::string &deviceName);
        ~DeviceLoggingContext();
        //TODOR: make member.
        std::shared_ptr<LogReader> GetFwLogCollector() { return m_fwLogCollector; }
        std::shared_ptr<LogReader> GetuCodeLogCollector() { return m_uCodeLogCollector; }
        bool VerifyFwVersion();
        void RestoreLoggingState();
        bool PrepareForLogging();
        void UnregisterPollerAndReportDeviceRemoved();
        OperationStatus SplitRecordings();

    private:
        static unsigned ComputeAhbToLinkerDelta(BasebandType basebandType, CpuType cpuType);

        const std::string m_deviceName;
        std::shared_ptr<LogReader> m_fwLogCollector;
        std::shared_ptr<LogReader> m_uCodeLogCollector;
        FwIdentifier m_lastfwIdentifier;
    };

}
