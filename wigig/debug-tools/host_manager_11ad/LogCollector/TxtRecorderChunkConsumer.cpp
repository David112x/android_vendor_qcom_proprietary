/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include "OperationStatus.h"
#include "TxtRecorderChunkConsumer.h"
#include "DeviceManager.h"
#include "Host.h"
#include "PersistentConfiguration.h"
#include "Device.h"
#include "FwStateProvider.h"


using namespace std;

namespace log_collector
{
    TxtRecorderChunkConsumer::TxtRecorderChunkConsumer(const std::string& deviceName, CpuType tracerType)
        : ChunkConsumer(deviceName, tracerType)
        , m_logRecorder(deviceName, tracerType, RECORDING_TYPE_TXT)
    {
    }

    TxtRecorderChunkConsumer::~TxtRecorderChunkConsumer()
    {
        Stop();
    }

    void TxtRecorderChunkConsumer::OnStartNewChunk(const TimeStamp& ts)
    {
        lock_guard<mutex> lock(m_currentFileMutex);

        if (m_logRecorder.ShouldResetLogFile())
        {
            m_timeStampStr = Utils::GetTimeString(ts);
            auto os = m_logRecorder.ResetLogFile();
            if (!os)
            {
                m_isEnabled = false;
                LOG_ERROR << "Failed to handle closed file: " << os.GetStatusMessage() << endl;
            }
        }
    }

    void TxtRecorderChunkConsumer::ReportBufferOverrun(unsigned numMissedDwords)
    {
        ostringstream missedDwordsMessage;
        missedDwordsMessage << BufferOverrunMessage << numMissedDwords << endl;
        if (!LogReader::Tracer3ppLogsFilterOn)
        {
            m_isEnabled = m_logRecorder.WriteToOutputFile(missedDwordsMessage.str()); // DWORD == uint32
        }
    }

    void TxtRecorderChunkConsumer::ReportDeviceRestarted()
    {
        m_logRecorder.WriteToOutputFile(DeviceRestartedMessage);
    }

    void TxtRecorderChunkConsumer::ReportCorruptedEntry(unsigned signature)
    {
        ostringstream ss;
        ss << CorruptedEntryMessage << signature << endl;
        m_isEnabled = m_logRecorder.WriteToOutputFile(ss.str());
    }

    void TxtRecorderChunkConsumer::ReportInconsistentWptr(unsigned diffDwords)
    {
        ostringstream ss;
        ss << InconsistentWptrMessage << diffDwords << endl;
        m_isEnabled = m_logRecorder.WriteToOutputFile(ss.str());
    }

    void TxtRecorderChunkConsumer::ReportDeviceRemoved()
    {
        // Return value of WriteToOutputFile is ignored. we do not want to change m_isEnabled.
        m_logRecorder.WriteToOutputFile(DeviceRemovedMessage);
        m_logRecorder.CloseOutputFile();
    }

    void TxtRecorderChunkConsumer::ReportDeviceDiscovered()
    {
        m_isEnabled = m_logRecorder.WriteToOutputFile(DeviceDiscoveredMessage);
    }

    void TxtRecorderChunkConsumer::ReportTracerWasRestarted()
    {
        // Return value of WriteToOutputFile is ignored. we do not want to change m_isEnabled.
        m_logRecorder.WriteToOutputFile(TracerWasRestartedMessage);
    }

    void TxtRecorderChunkConsumer::ReportAndUpdateFwVersion(const FwStateProvider* fwStateProvider,
        const module_level_info* moduleLevelInfoArray)
    {
        SetLogFileHeader(moduleLevelInfoArray, fwStateProvider);
        m_logTxtParser.SetModuleLevelVerbosity(moduleLevelInfoArray);
        m_logRecorder.WriteToOutputFile("Fw/uCode Information was updated \n");
        m_logRecorder.WriteFwInfo();
    }

    void TxtRecorderChunkConsumer::ConsumeMessage(const log_event* logEvent, unsigned rptr, unsigned bufferSize,
                                                unsigned numOfDwords)
    {
        const union log_event *evtHead = &logEvent[rptr % bufferSize]; // h->evt is the log line payload.
        if (numOfDwords > (MaxParams * 2))
        {
            ostringstream ss;
            ss << "**** invalid number of parameters provided number is: " << numOfDwords << " max val is: " << (MaxParams * 2) << endl;
            m_isEnabled = m_logRecorder.WriteToOutputFile(ss.str());
            return ;
        }

        // TODO: vector optimization
        vector<int> params(numOfDwords);
        for (unsigned i = 0; i < numOfDwords; i++)
        {
            params[i] = (logEvent[(rptr + i + 1) % bufferSize].dword);
        }
        unsigned stringOffset = evtHead->hdr.string_offset << 2;
        m_isEnabled = m_logRecorder.WriteToOutputFile(m_logTxtParser.ParseLogMessage(stringOffset, params, evtHead->hdr.module, evtHead->hdr.level, m_timeStampStr));
    }

    void TxtRecorderChunkConsumer::OnEndChunk()
    {
    }

    void TxtRecorderChunkConsumer::Stop()
    {
        m_isEnabled = false;
        m_logRecorder.CloseOutputFile();
    }

    OperationStatus TxtRecorderChunkConsumer::SplitRecordings()
    {
        if (!m_isEnabled)
        {
            return OperationStatus(false, "Splitting files is disabled when TXT log recorder is not enabled.");
        }
        if (m_logRecorder.IsPostStepsConfigured())
        {
            return OperationStatus(false, "Splitting files is disabled when post collection steps are set.");
        }
        lock_guard<mutex> lock(m_currentFileMutex);
        const auto ts = Utils::GetCurrentLocalTime();
        m_timeStampStr = Utils::GetTimeString(ts);
        const auto os = m_logRecorder.ResetLogFile();
        if (!os)
        {
            m_isEnabled = false;
            LOG_ERROR << "Failed to handle closed file: " << os.GetStatusMessage() << endl;
            return OperationStatus(false, "Failed to handle closed file: " + os.GetStatusMessage());
        }
        return OperationStatus();
    }


    OperationStatus TxtRecorderChunkConsumer::Enable(const module_level_info* moduleLevelInfoArray,
                                                     const FwStateProvider* fwStateProvider)
    {
        if (m_isEnabled)
        {
            return OperationStatus(false, "TxtRecorderChunkConsumer already Enabled");
        }

        m_logTxtParser.SetModuleLevelVerbosity(moduleLevelInfoArray);

        SetLogFileHeader(moduleLevelInfoArray, fwStateProvider);

        // update the recording path, folder was already created (if needed) by the buffer consumer
        OperationStatus os = m_logRecorder.SetLogFilesFolder(Host::GetHost().GetConfiguration().LogCollectorConfiguration.TargetLocation);
        if (!os)
        {
            return os;
        }

        os = m_logRecorder.PrepareRecording();
        m_isEnabled = os.IsSuccess();
        return os;
    }

    OperationStatus TxtRecorderChunkConsumer::SetPostCollectionActions(bool compress, bool upload)
    {
        return m_logRecorder.ConfigurePostCollectionSteps(compress, upload);
    }

    OperationStatus TxtRecorderChunkConsumer::TestRecordingConditions()
    {
        return m_logTxtParser.LogParserInit(string(Host::GetHost().GetConfiguration().LogCollectorConfiguration.ConversionFileLocation).c_str(), m_tracerCpuType);
    }

    void TxtRecorderChunkConsumer::SetLogFileHeader(const module_level_info* moduleLevelInfoArray, const FwStateProvider* fwStateProvider)
    {
        std::ostringstream headerBuilder;

        const FwIdentifier fwIdentifier = fwStateProvider->GetFwIdentifier();

        headerBuilder << "Logs for FW version: " << fwIdentifier.m_fwVersion.m_major << "."
            << fwIdentifier.m_fwVersion.m_minor << "."
            << fwIdentifier.m_fwVersion.m_subMinor << "."
            << fwIdentifier.m_fwVersion.m_build << endl;

        headerBuilder << "FW Compilation Time: "
            << fwIdentifier.m_fwTimestamp << endl;
        headerBuilder << "uCode Compilation Time: "
            << fwIdentifier.m_uCodeTimestamp << endl;

        headerBuilder << " Third party mode: [" ;
        bool first = true;
        for (int i = 0; i < NUM_MODULES; i++)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                headerBuilder << ", ";
            }
            headerBuilder << static_cast<int>(moduleLevelInfoArray[i].third_party_mode);
        }
        headerBuilder << "]" << endl;
        m_logRecorder.SetLogFilesHeader(headerBuilder.str());
    }
}
