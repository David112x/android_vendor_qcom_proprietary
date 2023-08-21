/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
#pragma once

#include "OperationStatus.h"
#include "LogRecorder.h"
#include "ChunkConsumer.h"
#include "LogTxtParser.h"

namespace log_collector
{

    class TxtRecorderChunkConsumer final : public ChunkConsumer
    {
    public:
        TxtRecorderChunkConsumer(const std::string &deviceName, CpuType tracerType);
        ~TxtRecorderChunkConsumer();

        void OnStartNewChunk(const TimeStamp& ts) override;
        void ReportBufferOverrun(unsigned numMissedDwords) override;
        void ReportDeviceRestarted() override;
        void ReportCorruptedEntry(unsigned signature) override;
        void ReportInconsistentWptr(unsigned diffDwords) override;
        void ReportDeviceRemoved() override;
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
        OperationStatus TestRecordingConditions() override;
    private:
        mutable std::mutex m_currentFileMutex;
        LogRecorder m_logRecorder;

        LogTxtParser m_logTxtParser;
        std::string m_timeStampStr;
        void SetLogFileHeader(const module_level_info* moduleLevelInfoArray, const FwStateProvider* fwStateProvider);
    };

}
