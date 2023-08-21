/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include <vector>
#include "OperationStatus.h"
#include "LogRecorder.h"
#include "ChunkConsumer.h"
#include "PostCollectionFileHandler.h"


namespace log_collector
{
    struct RawLogObject
    {
        RawLogObject() = default;
        RawLogObject(unsigned module, unsigned verbosityLevel, unsigned stringOffset, unsigned dwordNum);
        operator std::string() const;

        unsigned m_module;
        unsigned m_verbosityLevel;
        unsigned m_dwordNum;
        unsigned m_stringOffset;
        std::vector<int> m_dwords;

    };

    class XmlRecorderChunkConsumer final : public ChunkConsumer
    {
    public:
        XmlRecorderChunkConsumer(const std::string &deviceName, CpuType tracerType);
        ~XmlRecorderChunkConsumer();

        void OnStartNewChunk(const TimeStamp& ts) override;
        void ReportBufferOverrun(unsigned numMissedDwords) override;
        void ReportDeviceRestarted() override;
        void ReportCorruptedEntry(unsigned signature) override;
        void ReportInconsistentWptr(unsigned diffDwords) override;
        void ReportDeviceRemoved()override;
        void ReportDeviceDiscovered() override;
        void ReportTracerWasRestarted() override;
        void ReportAndUpdateFwVersion(const FwStateProvider* fwStateProvider, const module_level_info* moduleLevelInfoArray) override;
        void ConsumeMessage(const log_event* logEvent, unsigned rptr, unsigned bufferSize, unsigned numOfDwords) override;

        void OnEndChunk() override;

        // recording methods
        void Stop() override; // close files
        OperationStatus SplitRecordings() override; // close current file and start a new one.
        OperationStatus Enable(const module_level_info* moduleLevelInfoArray, const FwStateProvider* fwStateProvider) override;
        OperationStatus SetPostCollectionActions(bool compress, bool upload) override;

    private:
        mutable std::mutex m_currentFileMutex;
        LogRecorder m_logRecorder;
        void SetLogFileHeader(const module_level_info* moduleLevelInfoArray, const FwStateProvider* fwStateProvider);
        const std::string CreateFwVerTime( const FwStateProvider* fwStateProvider);
    };

}
