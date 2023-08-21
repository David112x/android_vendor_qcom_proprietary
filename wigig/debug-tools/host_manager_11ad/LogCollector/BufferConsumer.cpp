/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include "BufferConsumer.h"
#include "OperationStatus.h"
#include "ChunkConsumer.h"
#include "XmlRecorderChunkConsumer.h"
#include "TxtRecorderChunkConsumer.h"
#include "LogReader.h"
#include "RawPublisherChunkConsumer.h"
#include "FwStateProvider.h"
#include "Host.h"
#include "PersistentConfiguration.h"
#include "FileSystemOsAbstraction.h"

using namespace std;


namespace log_collector
{

    BufferConsumer::BufferConsumer(const std::string& deviceName, CpuType tracerType, unsigned logBufferContentSizeInDwords) :
        m_debugLogPrefix(deviceName + "_" + CPU_TYPE_TO_STRING[tracerType] + "_buffer consumer: "),
        m_rptr(0),
        m_logBufferContentSizeInDwords(logBufferContentSizeInDwords)
    {
        m_chunkConsumers[XmlRecorder].reset(new XmlRecorderChunkConsumer(deviceName, tracerType));
        m_chunkConsumers[TxtRecorder].reset(new TxtRecorderChunkConsumer(deviceName, tracerType));
        m_chunkConsumers[RawPublisher].reset(new RawPublisherChunkConsumer(deviceName, tracerType));
    }

    BufferConsumer::~BufferConsumer() = default;

    bool BufferConsumer::Reset(const module_level_info* moduleLevelInfoArray, unsigned logBufferContentSizeInDwords, const FwStateProvider* fwStateProvider)
    {
        // Check if there were active recordings before. (we check only recordings since DmTools take care of its recordings)
        m_logBufferContentSizeInDwords = logBufferContentSizeInDwords;

        const bool xmlRecordingActive = IsActive(XmlRecorder);
        const bool txtRecordingActive = IsActive(TxtRecorder);


        // Reactivating the recordings if needed.
        OperationStatus os;
        if (xmlRecordingActive)
        {
            StopConsumer(XmlRecorder);
            os = ActivateChunkConsumer(XmlRecorder, moduleLevelInfoArray, fwStateProvider);
            if (!os)
            {
                LOG_ERROR << "Failed to reactivate XmlRecorder Logs: " << os.GetStatusMessage() << endl;
                return false; // no need to un-pause recording.
            }
        }
        if (txtRecordingActive)
        {
            StopConsumer(TxtRecorder);
            os = ActivateChunkConsumer(TxtRecorder, moduleLevelInfoArray, fwStateProvider);
            if (!os)
            {
                LOG_ERROR << "Failed to reactivate TxtRecorder Logs: " << os.GetStatusMessage() << endl;
                return false; // no need to un-pause recording.
            }
        }
        return true;
    }

    OperationStatus BufferConsumer::ActivateChunkConsumer(
        LoggingType loggingType, const module_level_info* moduleLevelInfoArray, const FwStateProvider* fwStateProvider)
    {
        return m_chunkConsumers[loggingType]->Enable(moduleLevelInfoArray, fwStateProvider);
    }

    void BufferConsumer::SetActiveFlag(LoggingType loggingType)
    {
        m_chunkConsumers[loggingType]->SetActiveFlag();
    }

    OperationStatus BufferConsumer::ConfigureAndTestRecordingConditions(LoggingType loggingType, bool compress, bool upload)
    {
        // validate log recording directory exists
        // note: will be called twice - once for fw and once for ucode, but it's promised to be serially ans so folder will be created only once
        const string targetLocation = Host::GetHost().GetConfiguration().LogCollectorConfiguration.TargetLocation;
        if (!FileSystemOsAbstraction::DoesFolderExist(targetLocation))
        {
            LOG_INFO << m_debugLogPrefix << "Log recording directory does not exist. Creating one at: " << targetLocation << endl;
            if (!FileSystemOsAbstraction::CreateFolder(targetLocation))
            {
                ostringstream message;
                message << m_debugLogPrefix << "Failed to create log recording directory " << targetLocation;
                return OperationStatus(false, message.str());
            }
        }

        return OperationStatus::Merge(
            m_chunkConsumers[loggingType]->SetPostCollectionActions(compress, upload),
            m_chunkConsumers[loggingType]->TestRecordingConditions());
    }

    OperationStatus BufferConsumer::SplitRecordings()
    {
        return OperationStatus::Merge(
            m_chunkConsumers[LoggingType::TxtRecorder]->SplitRecordings(),
            m_chunkConsumers[LoggingType::XmlRecorder]->SplitRecordings());
    }

    void BufferConsumer::ConsumeBuffer(log_buffer* logBuffer)
    {
        const unsigned wptr = logBuffer->write_ptr;

        if (wptr == m_rptr)
        {
            // Nothing to read.
            return;
        }

        const TimeStamp ts = Utils::GetCurrentLocalTime();
        StartNewBuffer(ts);

        LOG_VERBOSE << m_debugLogPrefix << "before adjustments, wptr = " << wptr << ", rptr = " << m_rptr << endl;

        // detect device restart
        // TODO: consider w_ptr wrap around as sub-condition
        if (wptr < m_rptr) // device was restarted
        {
            LOG_INFO << m_debugLogPrefix << "device was restarted" << endl;
            ReportDeviceRestarted();

            // note: we do not limit here m_rptr not to read more than one buffer in order to catch and report buffer overrun during boot
            // if happens, it will be handled and reported by the regular overrun test
            m_rptr = 0U;
        }

        bool corruptedEntryExpected = false;
        if (wptr - m_rptr > m_logBufferContentSizeInDwords) // no danger of underflow since w_ptr > m_rptr
        {
            // buffer overrun, try to parse last wrap
            corruptedEntryExpected = true; // set to true since buffer overrun occurred but not yet handled, corrupted lines expected.
            ostringstream missedDwords;
            missedDwords << wptr - m_rptr - m_logBufferContentSizeInDwords;
            if (!LogReader::Tracer3ppLogsFilterOn && m_rptr > 0) // we check m_rptr> 0 so that we wont print buffer overrun message on start recording.
            {
                ReportBufferOverrun(wptr - m_rptr - m_logBufferContentSizeInDwords);
            }
            m_rptr = wptr - m_logBufferContentSizeInDwords;
        }

        LOG_VERBOSE << m_debugLogPrefix << "after adjustments, wptr = " << wptr << ", rptr = " << m_rptr << endl;

        for (; m_rptr < wptr; ++m_rptr)
        {
            const union log_event *evt = &logBuffer->evt[m_rptr % m_logBufferContentSizeInDwords]; // logBuffer->evt is the log line payload.
            if (evt->hdr.signature != log_collector::LogLineHeaderSignature)
            {
                if (!corruptedEntryExpected)
                {
                    if (!LogReader::Tracer3ppLogsFilterOn)
                    {
                        ReportCorruptedEntry(evt->hdr.signature);
                    }
                }
                continue;
            }
            corruptedEntryExpected = false; // at this point corrupted entries are not expected, next invalid signature should be reported.
            ConsumeMessage(logBuffer->evt);
        }

        if (m_rptr > wptr)
        {
            LOG_ERROR << m_debugLogPrefix << "Inconsistent wptr position in the middle of a message detected, diff (DWORDS) is " << m_rptr - wptr << endl;
            ReportInconsistentWptr(m_rptr - wptr);
            m_rptr = wptr;
        }
        EndBuffer();
    }

    void BufferConsumer::StopConsumer(LoggingType loggingType)
    {
        m_chunkConsumers[loggingType]->Stop();
    }

    bool BufferConsumer::IsAnyChunkConsumerActive() const
    {
        for (const auto &cc : m_chunkConsumers)
        {
            if (cc->IsActive())
            {
                return true;
            }
        }
        return false;
    }

    bool BufferConsumer::IsActive(LoggingType loggingType) const
    {
        return m_chunkConsumers[loggingType]->IsActive();
    }

    void BufferConsumer::ReportDeviceRemoved()
    {
        for (auto &cc : m_chunkConsumers)
        {
            if (cc->IsActive())
            {
                cc->ReportDeviceRemoved();
            }
        }
    }

    void BufferConsumer::ReportDeviceUp()
    {
        for (auto &cc : m_chunkConsumers)
        {
            if (cc->IsActive())
            {
                cc->ReportDeviceDiscovered();
            }
        }
    }


    void BufferConsumer::ReportAndUpdateFwVersion(const FwStateProvider* fwStateProvider, const module_level_info* moduleLevelInfoArray)
    {
        for (auto &cc : m_chunkConsumers)
        {
            if (cc->IsActive())
            {
                cc->ReportAndUpdateFwVersion(fwStateProvider, moduleLevelInfoArray);
            }
        }
    }

    void BufferConsumer::ResetRWPointers()
    {
        for (auto &cc : m_chunkConsumers)
        {
            if (cc->IsActive())
            {
                cc->ReportTracerWasRestarted();
            }
        }
        m_rptr = 0;
    }

    void BufferConsumer::ConsumeMessage(const log_event* logBufferBase)
    {
        // We do this anyway because we know that we have at least one consumer.
        const union log_event *evtHead = &logBufferBase[m_rptr % m_logBufferContentSizeInDwords]; // h->evt is the log line payload.
        const unsigned numOfDwords = 4 * evtHead->hdr.dword_num_msb + evtHead->hdr.dword_num_lsb; // Part of the same DWORD
        for (auto &cc : m_chunkConsumers)
        {
            if (cc->IsActive())
            {
                cc->ConsumeMessage(logBufferBase, m_rptr, m_logBufferContentSizeInDwords, numOfDwords);
            }
        }
        m_rptr += numOfDwords;
    }

    void BufferConsumer::StartNewBuffer(const TimeStamp& ts)
    {
        for (auto &cc : m_chunkConsumers)
        {
            if (cc->IsActive())
            {
                cc->OnStartNewChunk(ts);
            }
        }
    }

    void BufferConsumer::ReportBufferOverrun(unsigned numMissedDwords)
    {
        for (auto &cc : m_chunkConsumers)
        {
            if (cc->IsActive())
            {
                cc->ReportBufferOverrun(numMissedDwords);
            }
        }
    }

    void BufferConsumer::ReportDeviceRestarted()
    {
        for (auto &cc : m_chunkConsumers)
        {
            if (cc->IsActive())
            {
                cc->ReportDeviceRestarted();
            }
        }
    }

    void BufferConsumer::ReportCorruptedEntry(unsigned signature)
    {
        for (auto &cc : m_chunkConsumers)
        {
            if (cc->IsActive())
            {
                cc->ReportCorruptedEntry(signature);
            }
        }
    }

    void BufferConsumer::ReportInconsistentWptr(unsigned diffDwords)
    {
        for (auto &cc : m_chunkConsumers)
        {
            if (cc->IsActive())
            {
                cc->ReportInconsistentWptr(diffDwords);
            }
        }
    }

    void BufferConsumer::EndBuffer()
    {
        for (auto &cc : m_chunkConsumers)
        {
            if (cc->IsActive())
            {
                cc->OnEndChunk();
            }
        }
    }

}
