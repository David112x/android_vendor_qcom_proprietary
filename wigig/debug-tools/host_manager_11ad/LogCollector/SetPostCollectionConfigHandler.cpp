/*
* Copyright (c) 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include <sstream>

#include "SetPostCollectionConfigHandler.h"
#include "Host.h"
#include "PersistentConfiguration.h"
#include "DeviceManager.h"
#include "LogCollectorService.h"

using namespace std;

void SetPostCollectionConfigHandler::HandleRequest(const SetPostCollecitonConfigRequest& jsonRequest, SetPostCollectionConfigResponse& jsonResponse)
{
    // This flow only update the persistent configuration and does not affect existing recording
    LOG_DEBUG << "Log Collector set configuration request " << std::endl;

    // check if any recording exist
    if (Host::GetHost().GetLogCollectorService().IsLogging())
    {
        jsonResponse.Fail("Set configuration is not supported when log collection is active. Please stop all log collection. ");
        return;
    }

    const JsonValueBoxed<std::string>& getCompressionApp = jsonRequest.GetCompressionApp();
    if (getCompressionApp.GetState() == JsonValueState::JSON_VALUE_PROVIDED)
    {
        LOG_DEBUG << "setting Compression App to: " << getCompressionApp << endl;
        Host::GetHost().GetConfiguration().LogCollectorConfiguration.ConversionFileLocation = getCompressionApp;
    }

    const JsonValueBoxed<std::string>& getTargetServer = jsonRequest.GetTargetServer();
    if (getTargetServer.GetState() == JsonValueState::JSON_VALUE_PROVIDED)
    {
        LOG_DEBUG << "setting Upload Target server to: " << getTargetServer << endl;
        Host::GetHost().GetConfiguration().LogCollectorConfiguration.TargetLocation = getTargetServer;
    }

    const JsonValueBoxed<std::string>& getUserName = jsonRequest.GetUserName();
    if (getUserName.GetState() == JsonValueState::JSON_VALUE_PROVIDED)
    {
        LOG_DEBUG << "setting remote user name to: " << getUserName << endl;
        Host::GetHost().GetConfiguration().LogCollectorConfiguration.TargetLocation = getUserName;
    }

    const JsonValueBoxed<std::string>& getRemotePath = jsonRequest.GetRemotePath();
    if (getRemotePath.GetState() == JsonValueState::JSON_VALUE_PROVIDED)
    {
        LOG_DEBUG << "setting remote path to: " << getRemotePath << endl;
        Host::GetHost().GetConfiguration().LogCollectorConfiguration.TargetLocation = getRemotePath;
    }

    Host::GetHost().GetConfiguration().CommitChanges();

}

