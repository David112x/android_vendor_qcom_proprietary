/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
#include <string>
#include "ChunkConsumer.h"

namespace log_collector
{
    const std::string ChunkConsumer::BufferOverrunMessage = "\nhost_manager_11ad message: Found buffer overrun - missed DWORDS: ";
    const std::string ChunkConsumer::DeviceRestartedMessage = "\nhost_manager_11ad message: Device was restarted \n";
    const std::string ChunkConsumer::CorruptedEntryMessage = "\nhost_manager_11ad message: Got corrupted entry, signature: ";
    const std::string ChunkConsumer::InconsistentWptrMessage = "\nhost_manager_11ad message: Inconsistent wptr position in the middle of a message, diff (DWORDS): ";
    const std::string ChunkConsumer::DeviceRemovedMessage = "\nhost_manager_11ad message: Device was removed! \n";
    const std::string ChunkConsumer::DeviceDiscoveredMessage = "\nhost_manager_11ad message: Device was discovered! \n";
    const std::string ChunkConsumer::TracerWasRestartedMessage = "\nhost_manager_11ad message: Tracer was restarted \n";

    OperationStatus ChunkConsumer::SplitRecordings()
    {
        return OperationStatus();
    }

    OperationStatus ChunkConsumer::SetPostCollectionActions(bool compress, bool upload)
    {
        (void)compress;
        (void)upload;
        return OperationStatus();
    }

    OperationStatus ChunkConsumer::TestRecordingConditions()
    {
        return OperationStatus();
    }
}
