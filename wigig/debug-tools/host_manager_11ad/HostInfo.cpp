/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/
/*
* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <iostream>

#ifdef _WINDOWS
#include <winsock2.h>       // For gethostname
#define HOST_NAME_MAX 64
#else
#include <unistd.h>         // For gethostname
#include <limits.h>         // For HOST_NAME_MAX
#endif

#include "HostInfo.h"
#include "DebugLogger.h"
#include "FileSystemOsAbstraction.h"
#include "Host.h"
#include "PersistentConfiguration.h"

using namespace std;

namespace {
    // Enumeration of Host capabilities
    // Note: This is a contract with DmTools, the order is important!
    enum CAPABILITIES
    {
        COLLECTING_LOGS = 0, // capability of host manager to collect logs by itself and send them to DmTools
        KEEP_ALIVE,          // capability of host manager to send keep-alive event to enable watchdog on the client side
        READ_BLOCK_FAST,     // host manager supports rb_fast command
    };
}

HostInfo::HostInfo() :
    m_ips(FileSystemOsAbstraction::GetHostIps()),
    m_capabilityMask(0U)
{
    CreateHostDirectories();
    SetHostCapabilities();
}


const HostIps& HostInfo::GetIps() const
{
    return m_ips;
}

string HostInfo::GetHostName()
{
    char hostName[HOST_NAME_MAX + 1];
    if (gethostname(hostName, HOST_NAME_MAX + 1) != 0)
    {
        LOG_ERROR << "Failed reading host name" << std::endl;
        return string();
    }

    return string(hostName);
}


set<string> HostInfo::GetConnectedUsers() const
{
    lock_guard<mutex> lock(m_connectedUsersLock);
    return m_connectedUsers;
}

void HostInfo::AddNewConnectedUser(const string& user)
{
    lock_guard<mutex> lock(m_connectedUsersLock);
    m_connectedUsers.insert(user);
}

void HostInfo::RemoveConnectedUser(const string& user)
{
    lock_guard<mutex> lock(m_connectedUsersLock);
    m_connectedUsers.erase(user);
}

void HostInfo::SetHostCapabilities()
{
    m_capabilityMask.set(COLLECTING_LOGS);
    m_capabilityMask.set(KEEP_ALIVE);
    m_capabilityMask.set(READ_BLOCK_FAST);
}

void HostInfo::CreateHostDirectories()
{
    // configuration folder

    // output files folder
    if (!FileSystemOsAbstraction::DoesFolderExist(FileSystemOsAbstraction::GetOutputFilesLocation()))
    {
        if (!FileSystemOsAbstraction::CreateFolder(FileSystemOsAbstraction::GetOutputFilesLocation()))
        {
            LOG_WARNING << "Failed to create OutputFilesLocation " << FileSystemOsAbstraction::GetOutputFilesLocation() << " directory" << endl;
        }
    }
}

unordered_set<uint32_t> HostInfo::Get3ppIgnoreEvents() const
{
    if (!Is3ppPlatform())
    {
        return {};
    }

    const string ignoreEventsStr = Host::GetHost().GetConfiguration().HostConfiguration.Ignore3ppFwEventIds;

    if (ignoreEventsStr.empty())
    {
        return {};
    }

    unordered_set<uint32_t> ignoreEvents;
    stringstream ss(ignoreEventsStr);
    string eventIdStr;
    uint32_t eventId;
    while(getline(ss, eventIdStr,','))
    {
        if(stringstream(eventIdStr) >> std::hex >> eventId)
        {
            ignoreEvents.insert(eventId);
        }
    }
    return ignoreEvents;
}
