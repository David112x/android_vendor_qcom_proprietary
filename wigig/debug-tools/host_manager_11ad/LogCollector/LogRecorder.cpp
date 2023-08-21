/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include <cstring>
#include <fstream>
#include <thread>

#include "Host.h"
#include "PersistentConfiguration.h"
#include "LogRecorder.h"
#include "LogCollectorStopRecordingHandler.h"
#include "LogReader.h"
#include "Device.h"
#include "FileSystemOsAbstraction.h"

using namespace std;

namespace log_collector
{

    LogRecorder::LogRecorder(const std::string& deviceName, CpuType tracerType, RecordingType recordingType) :
        m_debugLogPrefix(deviceName + "_" + CPU_TYPE_TO_STRING[tracerType] + " recorder: "),
        m_currentOutputFileSize(0),
        m_logFilePrefix(deviceName + "_" + CPU_TYPE_TO_STRING[tracerType] + "_"),
        m_logFilesFolder(Host::GetHost().GetConfiguration().LogCollectorConfiguration.TargetLocation),
        m_logFileExtension((recordingType == RECORDING_TYPE_XML) ? ".log" : ".txt")
    {}

    OperationStatus LogRecorder::PrepareRecording()
    {
        m_currentOutputFileSize = 0;

        CloseOutputFile();
        OperationStatus os = CreateNewOutputFile();
        if (!os)
        {
            os.AddPrefix("Failed to create the first output file");
            LOG_ERROR << os.GetStatusMessage() << endl;
            return os;
        }

        LOG_DEBUG << m_debugLogPrefix << "Log collector preparation was successfully finished" <<  endl;
        return OperationStatus();
    }

    bool LogRecorder::ShouldResetLogFile() const
    {
        const size_t maxSingleFileSizeInByte = Host::GetHost().GetConfiguration().LogCollectorConfiguration.MaxLogFileSize * 1024 * 1024;
        return (m_currentOutputFileSize > maxSingleFileSizeInByte);
    }

    std::string LogRecorder::GetCurrentFileName() const
    {
        return m_currentOutputFileNameActive;
    }

    OperationStatus LogRecorder::ResetLogFile()
    {
        CloseOutputFile();
        return CreateNewOutputFile();
    }


    void LogRecorder::IncreaseCurrentOutputFileSize(size_t outputFileSize)
    {
        m_currentOutputFileSize += outputFileSize;
        LOG_VERBOSE << m_debugLogPrefix << "Current File Size = " << m_currentOutputFileSize << endl;
    }

    OperationStatus LogRecorder::SetLogFilesFolder(const std::string& newPath)
    {
        if (!FileSystemOsAbstraction::DoesFolderExist(newPath))
        {
            ostringstream message;
            message << m_debugLogPrefix << "Invalid path '" << newPath << "' provided for log recording folder";
            return OperationStatus(false, message.str());
        }

        m_logFilesFolder = newPath;
        if (newPath[newPath.length()-1] != '/')
        {
            m_logFilesFolder += "/";
        }

        return OperationStatus();
    }

    void LogRecorder::WriteFwInfo()
    {
        WriteToOutputFile(m_fileHeader);
    }

    OperationStatus LogRecorder::ConfigurePostCollectionSteps(bool compressFiles, bool uploadFiles)
    {
        return m_postCollectionFileHandler.ConfigurePostCollectionSteps(compressFiles, uploadFiles);
    }

    bool LogRecorder::IsPostStepsConfigured() const
    {
        return m_postCollectionFileHandler.IsPostStepsConfigured();
    }

    void LogRecorder::UpdateOutputFileFullName()
    {
        ostringstream ss;
        ss << m_logFilesFolder << m_logFilePrefix << Utils::GetCurrentLocalTimeForFileName()
            <<  m_logFileExtension;

        m_currentOutputFileNameBase = ss.str();
        ss << ".active";
        m_currentOutputFileNameActive = ss.str();
    }

    OperationStatus LogRecorder::CreateNewOutputFile()
    {
        UpdateOutputFileFullName();
        LOG_INFO << m_debugLogPrefix << "Creating output file: " << m_currentOutputFileNameActive << endl;

        m_outputFile.open(m_currentOutputFileNameActive.c_str());
        if (m_outputFile.fail())
        {
            ostringstream message;
            message << m_debugLogPrefix << "Error opening output file: " << m_currentOutputFileNameActive << " Error: " << strerror(errno) << endl;
            return OperationStatus(false, message.str());
        }

        m_outputFile << m_fileHeader;
        return OperationStatus();
    }

    void LogRecorder::HandleClosedFile()
    {
        LOG_DEBUG << m_debugLogPrefix << "file name is: " << m_currentOutputFileNameBase << endl;
        std::thread t = std::thread(&PostCollectionFileHandler::HandleClosedFile, &m_postCollectionFileHandler, m_currentOutputFileNameBase);
        t.detach();
    }


    void LogRecorder::CloseOutputFile()
    {
        if (m_outputFile.is_open())
        {
            m_outputFile.close();
            // renaming the current file
            FileSystemOsAbstraction::MoveFileToNewLocation(m_currentOutputFileNameActive, m_currentOutputFileNameBase);
            HandleClosedFile();
            LOG_INFO << m_debugLogPrefix << "Closed output file " << m_currentOutputFileNameActive << endl;
        }
        m_currentOutputFileSize = 0;
    }

    bool LogRecorder::WriteToOutputFile(const string& logLine)
    {
        if (m_outputFile.is_open())
        {
            m_outputFile << logLine;
            m_currentOutputFileSize += logLine.size();
            return true;
        }
        LOG_WARNING << m_debugLogPrefix << "The file " << m_currentOutputFileNameActive << " is not open. Cannot print the line: " << logLine << endl;
        return false;
    }


}
