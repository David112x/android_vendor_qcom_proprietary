/*
* Copyright (c) 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include <cstdio>
#include "Host.h"
#include "PersistentConfiguration.h"
#include "PostCollectionFileHandler.h"
#include "FileSystemOsAbstraction.h"
#include "ShellCommandExecutor.h"

using namespace std;

namespace log_collector
{

    PostCollectionFileHandler::PostCollectionFileHandler():
        m_compressFiles(false),
        m_uploadFiles(false),
        m_maxNumOfLogFiles(Host::GetHost().GetConfiguration().LogCollectorConfiguration.MaxNumOfLogFiles),
        m_compressionApp("gzip "),
        m_remoteIp(Host::GetHost().GetConfiguration().PostLogCollection.TargetServer),
        m_userName(Host::GetHost().GetConfiguration().PostLogCollection.UserName),
        m_remotePath(Host::GetHost().GetConfiguration().PostLogCollection.RemotePath),
        m_debugLogPrefix("logFileHandler")
    {
    }

    OperationStatus PostCollectionFileHandler::ConfigurePostCollectionSteps(bool compressFiles, bool uploadFiles)
    {
        m_maxNumOfLogFiles = Host::GetHost().GetConfiguration().LogCollectorConfiguration.MaxNumOfLogFiles;
        m_remoteIp = Host::GetHost().GetConfiguration().PostLogCollection.TargetServer;
        m_userName = Host::GetHost().GetConfiguration().PostLogCollection.UserName;
        m_remotePath = Host::GetHost().GetConfiguration().PostLogCollection.RemotePath;
        OperationStatus os;
        if (compressFiles)
        {
            os = CheckCompress();
        }
        if (uploadFiles)
        {
            os = OperationStatus::Merge(os, CheckUpload());
        }
        if(os)
        {
            m_compressFiles = compressFiles;
            m_uploadFiles = uploadFiles;
        }

        return os;
    }

    void PostCollectionFileHandler::HandleClosedFile(std::string fileName)
    {
        if (fileName.empty())
        {
            return;
        }
        LOG_DEBUG << "Handling closed file: " << fileName << endl;
        string newFileName = fileName;
        if (m_compressFiles)
        {
            CompressFile(newFileName);
            newFileName += ".gz";
        }
        if (m_uploadFiles)
        {
           UploadFile(newFileName);
        }
        m_fileQueue.push(newFileName);
        LOG_DEBUG << "After handling closed file: "<< newFileName << " before removing old files " << endl;
        RemoveOldFilesIfNeeded();
    }

    bool PostCollectionFileHandler::IsPostStepsConfigured() const
    {
        return (m_compressFiles || m_uploadFiles);
    }

    void PostCollectionFileHandler::RemoveOldFilesIfNeeded()
    {
        if (0 == m_maxNumOfLogFiles)
        {
            // no need to remove old files
            return;
        }

        if (m_fileQueue.size() > m_maxNumOfLogFiles)
        {
            const string &fileToRemove = m_fileQueue.front();
            LOG_INFO << m_debugLogPrefix << "Deleting old log file: " << fileToRemove << endl;
            FileSystemOsAbstraction::RemoveFile(fileToRemove);
            m_fileQueue.pop();
        }
        LOG_DEBUG << m_debugLogPrefix << " Removed oldest log file from the folder. Current tracer's log file size in folder: " << m_fileQueue.size() << endl;
    }

    OperationStatus PostCollectionFileHandler::CheckCompress() const
    {
        std::ostringstream cmd;
        cmd << m_compressionApp << " -V";
        ShellCommandExecutor sce(cmd.str().c_str());
        if (sce.ExitStatus() != 0)
        {
            std::ostringstream osMessage;
            osMessage << "Invalid compression method: " << m_compressionApp << " shell message: " << (!sce.Output().empty() ? sce.Output().front() : "");
            LOG_WARNING << osMessage.str() << endl;
            return OperationStatus(false, osMessage.str());
        }
        return OperationStatus();
    }

    OperationStatus PostCollectionFileHandler::CheckUpload() const
    {
        std::ostringstream cmd;

        // Pinging 1 packet, wait maximum 2 sec.
        cmd << "ping -c1 -w2 " <<  m_remoteIp << " > /dev/null";
        ShellCommandExecutor sce0(cmd.str().c_str());
        if (sce0.ExitStatus() != 0)
        {
            std::ostringstream osMessage;
            osMessage << "IP is invalid. Check IP and user name in configuration: " << m_userName << "@" << m_remoteIp << " shell message: " << (!sce0.Output().empty() ? sce0.Output().front() : "");
            LOG_WARNING << osMessage.str() << endl;
            return OperationStatus(false, osMessage.str());
        }
        cmd.str(string());

        // man ssh:
        //-q      Quiet mode.  Causes most warning and diagnostic messages to be suppressed.
        // BatchMode
        // If set to “yes”, passphrase / password querying will be disabled.In addition, the ServerAliveInterval option will be set to 300 seconds by
        //    default.This option is useful in scripts and other batch jobs where no user is present to supply the password, and where it is desirable to
        //    detect a broken network swiftly.The argument must be “yes” or “no”.The default is “no”.
        cmd << "ssh -q -oBatchMode=yes " << m_userName << "@" << m_remoteIp << " echo 0 ; exit";
        ShellCommandExecutor sce1(cmd.str().c_str());
        if (sce1.ExitStatus() != 0)
        {
            std::ostringstream osMessage;
            osMessage << "SSH connection is invalid. Check IP and user name in configuration: " << m_userName << "@" << m_remoteIp << " shell message: " << (!sce1.Output().empty() ? sce1.Output().front() : "");
            LOG_WARNING << osMessage.str() << endl;
            return OperationStatus(false, osMessage.str());
        }
        cmd.str(string());
        cmd << "ssh -q -oBatchMode=yes " << m_userName << "@" << m_remoteIp << " ls " << m_remotePath << "; exit";
        ShellCommandExecutor sce2(cmd.str().c_str());
        if (sce2.ExitStatus() != 0)
        {
            std::ostringstream osMessage;
            osMessage << "SSH connection is valid, but remote path is not valid: " << m_remotePath << " shell message: " << (!sce2.Output().empty() ? sce2.Output().front() : "");
            LOG_WARNING << osMessage.str() << endl;
            return OperationStatus(false, osMessage.str());
        }
        return OperationStatus();
    }

    void PostCollectionFileHandler::CompressFile(const std::string& fullFilePath) const
    {
        if (!FileSystemOsAbstraction::DoesFileExist(fullFilePath))
        {
            LOG_ERROR << "Failed to compress the file: " << fullFilePath << "; File does not exist" << endl;
            return;
        }
        LOG_DEBUG << "Compressing file: " << fullFilePath << endl;
        std::ostringstream cmd;
        cmd << m_compressionApp << fullFilePath;
        ShellCommandExecutor sce(cmd.str().c_str());
        if (sce.ExitStatus() != 0)
        {
            LOG_WARNING << "Failed to compress the file: " << fullFilePath << " could not be compressed, shell message: " << (!sce.Output().empty() ? sce.Output().front() : "") << endl;
        }
    }

    void PostCollectionFileHandler::UploadFile(const std::string& fullFilePath) const
    {
        if (!FileSystemOsAbstraction::DoesFileExist(fullFilePath))
        {
            LOG_ERROR << "Failed to Upload the file: " << fullFilePath << " does not exist" << endl;
            return;
        }
        LOG_DEBUG << "Uploading file name is: " << fullFilePath << endl;

        std::ostringstream cmd;

        // Example: scp <fullFilePath> <user_name>@<ip>:<full path on remote server>
        cmd << "scp " << fullFilePath << " " << m_userName << "@" << m_remoteIp << ":" << m_remotePath<< "/";

        ShellCommandExecutor sce(cmd.str().c_str());
        if (sce.ExitStatus() != 0)
        {
            LOG_WARNING << "File: " << fullFilePath << " could not be uploaded shell message: " << (!sce.Output().empty() ? sce.Output().front() : "") << endl;
        }
    }

}
