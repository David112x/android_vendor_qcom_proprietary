/*
* Copyright (c) 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include <string>
#include <fstream>
#include <queue>
#include "OperationStatus.h"


class Device;

namespace log_collector
{
    class PostCollectionFileHandler
    {
    public:
        PostCollectionFileHandler();
        OperationStatus ConfigurePostCollectionSteps(bool compressFiles, bool uploadFiles);
        void HandleClosedFile(std::string fileName);
        bool IsPostStepsConfigured() const;

    private:
        bool m_compressFiles;
        bool m_uploadFiles;

        // From config:
        uint32_t m_maxNumOfLogFiles;
        const std::string m_compressionApp;
        std::string m_remoteIp;
        std::string m_userName;
        std::string m_remotePath;

        std::string m_debugLogPrefix;

        std::queue<std::string> m_fileQueue;

        OperationStatus CheckCompress() const;
        OperationStatus CheckUpload() const;
        void CompressFile(const std::string& fullFilePath) const;
        void UploadFile(const std::string& fullFilePath) const;
        void RemoveOldFilesIfNeeded();
    };
}
