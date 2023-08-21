/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include <atomic>
#include <string>
#include "OperationStatus.h"
#include "Utils.h"
#include "LogCollectorDefinitions.h"
#include "FwStateProvider.h"

namespace log_collector
{
    class ChunkConsumer
    {
    public:
        static const std::string  BufferOverrunMessage;
        static const std::string  DeviceRestartedMessage;
        static const std::string  CorruptedEntryMessage;
        static const std::string  InconsistentWptrMessage;
        static const std::string  DeviceRemovedMessage;
        static const std::string  DeviceDiscoveredMessage;
        static const std::string  TracerWasRestartedMessage;

        ChunkConsumer(const std::string &deviceName, CpuType cpuType):
            m_isEnabled(false), m_deviceName(deviceName), m_tracerCpuType(cpuType)
        {}
        virtual ~ChunkConsumer() = default;
        ChunkConsumer(const ChunkConsumer & lcc) = delete;
        ChunkConsumer & operator=(const ChunkConsumer & lcc) = delete;

        virtual void OnStartNewChunk(const TimeStamp& ts) = 0;
        virtual void ReportBufferOverrun(unsigned numMissedDwords) = 0;
        virtual void ReportDeviceRestarted() = 0;
        virtual void ReportCorruptedEntry(unsigned signature) = 0;
        virtual void ReportInconsistentWptr(unsigned diffDwords) = 0;
        virtual void ReportDeviceRemoved() = 0;
        virtual void ReportDeviceDiscovered() = 0;
        virtual void ReportTracerWasRestarted() = 0;
        virtual void ReportAndUpdateFwVersion(const FwStateProvider* fwStateProvider, const module_level_info* moduleLevelInfoArray) = 0;

        virtual void ConsumeMessage(const log_event* logBufferBase, unsigned rptr, unsigned bufferSize, unsigned numOfDwords) = 0;
        virtual void OnEndChunk() = 0;

        // recording methods
        virtual void Stop() = 0; // close files
        virtual OperationStatus SplitRecordings(); // close current file and start a new one.
        bool IsActive() const { return m_isEnabled; }

        void SetActiveFlag() { m_isEnabled = true; }
        virtual OperationStatus SetPostCollectionActions(bool compress, bool upload);
        virtual OperationStatus TestRecordingConditions();

        virtual OperationStatus Enable(const module_level_info* moduleLevelInfoArray, const FwStateProvider* fwStateProvider) = 0;

    protected:
        std::atomic<bool> m_isEnabled;
        const std::string m_deviceName;
        CpuType m_tracerCpuType;
    };

}
