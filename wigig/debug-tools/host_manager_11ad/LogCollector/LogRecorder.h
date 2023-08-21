/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once
#include <string>
#include <vector>
#include <fstream>

#include "OperationStatus.h"
#include "LogCollectorDefinitions.h"
#include "PostCollectionFileHandler.h"


class Device;

namespace log_collector
{
    class LogRecorder
    {
    public:
        LogRecorder(
            const std::string &deviceName, CpuType tracerType, RecordingType recordingType);
        // recording methods
        bool WriteToOutputFile(const std::string& logLine);
        bool ShouldResetLogFile() const;
        std::string GetCurrentFileName() const;
        OperationStatus ResetLogFile();

        OperationStatus PrepareRecording();
        void CloseOutputFile();

        void IncreaseCurrentOutputFileSize(size_t outputFileSize);
        const std::string& GetRecordingFolder() const { return m_logFilesFolder; }

        OperationStatus SetLogFilesFolder(const std::string& newPath);
        void SetLogFilesHeader(const std::string & newHeader) { m_fileHeader = newHeader; };
        void WriteFwInfo();
        OperationStatus ConfigurePostCollectionSteps(bool compressFiles, bool uploadFiles);
        bool IsPostStepsConfigured() const;

    private:
        std::string m_debugLogPrefix;

        // output file methods
        void UpdateOutputFileFullName();
        OperationStatus CreateNewOutputFile();
        void HandleClosedFile();

        // output file
        std::string m_currentOutputFileNameBase;
        std::string m_currentOutputFileNameActive;
        std::ofstream m_outputFile;
        size_t m_currentOutputFileSize;
        const std::string m_logFilePrefix;
        std::string m_logFilesFolder;
        std::string m_logFileExtension;
        std::string m_fileHeader;
        PostCollectionFileHandler m_postCollectionFileHandler;
    };
}
