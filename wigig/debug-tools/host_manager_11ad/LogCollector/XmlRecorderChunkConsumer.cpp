/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include "OperationStatus.h"
#include "Host.h"
#include "PersistentConfiguration.h"
#include "Device.h"
#include "FwStateProvider.h"
#include "XmlRecorderChunkConsumer.h"


using namespace std;

namespace log_collector
{

    std::ostream& operator<<(std::ostream& os, const RawLogObject& rawLogObject)
    {
        os << rawLogObject.m_module << "," << levels[rawLogObject.m_verbosityLevel] << "," << rawLogObject.m_stringOffset << ":";
        unsigned i;
        for (i = 0; i < rawLogObject.m_dwordNum; i++)
        {
            os << rawLogObject.m_dwords [i];
            if (i < rawLogObject.m_dwordNum - 1)
            {
                os << ",";
            }
        }

        os << endl;
        return os;
    }



    RawLogObject::RawLogObject(unsigned module, unsigned verbosityLevel, unsigned stringOffset,
        unsigned dwordNum): m_module(module), m_verbosityLevel (verbosityLevel),
                                m_dwordNum(dwordNum),m_stringOffset (stringOffset)
    {
    }

    RawLogObject::operator basic_string<char>() const
    {
        ostringstream ss;
        ss << *this;
        return ss.str();
    }


    XmlRecorderChunkConsumer::XmlRecorderChunkConsumer(const std::string& deviceName, CpuType tracerType)
        : ChunkConsumer(deviceName, tracerType)
        , m_logRecorder(deviceName, tracerType, RECORDING_TYPE_XML)
    {
    }

    XmlRecorderChunkConsumer::~XmlRecorderChunkConsumer()
    {
        Stop();
    }

    void XmlRecorderChunkConsumer::OnStartNewChunk(const TimeStamp& ts)
    {
        lock_guard<mutex> lock(m_currentFileMutex);
        if (m_logRecorder.ShouldResetLogFile())
        {
            m_logRecorder.WriteToOutputFile("</Logs></LogFile>");
            string currFileName;

            auto os = m_logRecorder.ResetLogFile();
            if (!os)
            {
                m_isEnabled = false;
                LOG_ERROR << "Failed to replace log file: " << os.GetStatusMessage() << endl;
            }
        }
        m_isEnabled = m_logRecorder.WriteToOutputFile(Utils::GetCurrentLocalTimeXml(ts) + "<Content>");

    }

    void XmlRecorderChunkConsumer::ReportBufferOverrun(unsigned numMissedDwords)
    {
        ostringstream missedDwords;
        missedDwords << BufferOverrunMessage << numMissedDwords << endl;
        if (!LogReader::Tracer3ppLogsFilterOn)
        {
            m_isEnabled =  m_logRecorder.WriteToOutputFile(missedDwords.str()); // DWORD == uint32
        }
    }

    void XmlRecorderChunkConsumer::ReportDeviceRestarted()
    {
        m_isEnabled = m_logRecorder.WriteToOutputFile(DeviceRestartedMessage);
    }

    void XmlRecorderChunkConsumer::ReportCorruptedEntry(unsigned signature)
    {
        ostringstream ss;
        ss << "**** Got corrupted entry, signature: " << signature << endl;
        m_isEnabled = m_logRecorder.WriteToOutputFile(ss.str());
    }

    void XmlRecorderChunkConsumer::ReportInconsistentWptr(unsigned diffDwords)
    {
        ostringstream ss;
        ss << "**** Inconsistent wptr position in the middle of a message, diff (DWORDS): " << diffDwords << endl;
        m_isEnabled = m_logRecorder.WriteToOutputFile(ss.str());
    }

    void XmlRecorderChunkConsumer::ReportDeviceRemoved()
    {
        // Return value of WriteToOutputFile is ignored. we do not want to change m_isEnabled.
        m_logRecorder.WriteToOutputFile(DeviceRemovedMessage);
        m_logRecorder.WriteToOutputFile("</Logs></LogFile>");
        m_logRecorder.CloseOutputFile();
    }

    void XmlRecorderChunkConsumer::ReportDeviceDiscovered()
    {
         m_isEnabled = m_logRecorder.WriteToOutputFile(DeviceDiscoveredMessage);
    }

    void XmlRecorderChunkConsumer::ReportTracerWasRestarted()
    {
        // Return value of WriteToOutputFile is ignored. we do not want to change m_isEnabled.
        m_logRecorder.WriteToOutputFile(TracerWasRestartedMessage);
    }

    void XmlRecorderChunkConsumer::ReportAndUpdateFwVersion(const FwStateProvider* fwStateProvider,
        const module_level_info* moduleLevelInfoArray)
    {
        SetLogFileHeader(moduleLevelInfoArray, fwStateProvider);
        m_logRecorder.WriteToOutputFile(CreateFwVerTime(fwStateProvider));
    }

    void XmlRecorderChunkConsumer::ConsumeMessage(const log_event* logEvent, unsigned rptr, unsigned bufferSize, unsigned numOfDwords)
    {
        const union log_event *evtHead = &logEvent[rptr % bufferSize]; // h->evt is the log line payload.
        const unsigned stringOffset = evtHead->hdr.string_offset << 2;

        RawLogObject newRawLogObject(evtHead->hdr.module, evtHead->hdr.level, stringOffset, numOfDwords);
        if (numOfDwords > (MaxParams * 2))
        {
            ostringstream ss;
            ss << "**** invalid number of parameters provided number is: " << numOfDwords << " max val is: " << (MaxParams * 2) << endl;
            m_isEnabled = m_logRecorder.WriteToOutputFile(ss.str());
            return;
        }

        newRawLogObject.m_dwords.reserve(numOfDwords);
        for (unsigned i = 0; i < numOfDwords; i++)
        {
            newRawLogObject.m_dwords.push_back(logEvent[(rptr + i + 1) % bufferSize].dword);
        }

        m_isEnabled = m_logRecorder.WriteToOutputFile(newRawLogObject);
    }

    void XmlRecorderChunkConsumer::OnEndChunk()
    {
        m_isEnabled = m_logRecorder.WriteToOutputFile("</Content></Log_Content>");
    }

    void XmlRecorderChunkConsumer::Stop()
    {
        if (m_isEnabled)
        {
            m_logRecorder.WriteToOutputFile("</Logs></LogFile>");
        }
        m_isEnabled = false;
        m_logRecorder.CloseOutputFile();
    }

    OperationStatus XmlRecorderChunkConsumer::SplitRecordings()
    {
        if (!m_isEnabled)
        {
            return OperationStatus(false, "Splitting files is disabled when XML log recorder is not enabled.");
        }
        if (m_logRecorder.IsPostStepsConfigured())
        {
            return OperationStatus(false, "Splitting files is disabled when post collection steps are set.");
        }
        const auto ts = Utils::GetCurrentLocalTime();
        lock_guard<mutex> lock(m_currentFileMutex);
        m_logRecorder.WriteToOutputFile("</Logs></LogFile>");
        auto os = m_logRecorder.ResetLogFile();
        if (!os)
        {
            m_isEnabled = false;
            LOG_ERROR << "Failed to handle closed file: " << os.GetStatusMessage() << endl;
            return OperationStatus(false, "Failed to handle closed file: " + os.GetStatusMessage());
        }

        m_isEnabled = m_logRecorder.WriteToOutputFile(Utils::GetCurrentLocalTimeXml(ts) + "<Content>");
        return OperationStatus();

    }

    OperationStatus XmlRecorderChunkConsumer::Enable(const module_level_info* moduleLevelInfoArray,
                                                     const FwStateProvider* fwStateProvider)
    {
        if(m_isEnabled)
        {
            return OperationStatus(false, "XmlRecorderChunkConsumer already Enabled");
        }

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

    OperationStatus XmlRecorderChunkConsumer::SetPostCollectionActions(bool compress, bool upload)
    {
        return m_logRecorder.ConfigurePostCollectionSteps(compress, upload);
    }


    void XmlRecorderChunkConsumer::SetLogFileHeader(const module_level_info* moduleLevelInfoArray,
                                                    const FwStateProvider* fwStateProvider)
    {
        std::ostringstream headerBuilder;
        headerBuilder << "<LogFile>"
            << CreateFwVerTime(fwStateProvider)
            << "<Third_Party_Flags>";
        // make a list of the Third_party_flag for each module.
        for (int i = 0; i < NUM_MODULES; i++)
        {
            headerBuilder << "<Flag><value>" << static_cast<int>(moduleLevelInfoArray[i].third_party_mode) << "</value></Flag>";
        }
        headerBuilder << "</Third_Party_Flags>"
            << "<Logs>";

        m_logRecorder.SetLogFilesHeader(headerBuilder.str());
    }


    const std::string XmlRecorderChunkConsumer::CreateFwVerTime(const FwStateProvider* fwStateProvider)
    {
        std::ostringstream headerBuilder;
        const auto fwIdentifier = fwStateProvider->GetFwIdentifier();
        headerBuilder << "<FW_Ver>"
            << fwIdentifier.m_fwVersion
            << "</FW_Ver>"
            << "<FwCompilation_Time>"
            << fwIdentifier.m_fwTimestamp
            << "</FwCompilation_Time>"
            << "<uCodeCompilation_Time>"
            << fwIdentifier.m_uCodeTimestamp
            << "</uCodeCompilation_Time>";
        return headerBuilder.str();

    }
}
