/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef _LOG_COLLECTOR_GET_CONFIG_HANDLER_H_
#define _LOG_COLLECTOR_GET_CONFIG_HANDLER_H_
#pragma once

#include "JsonHandlerSDK.h"
#include "DebugLogger.h"
#include "LogCollectorDefinitions.h"
#include "LogCollectorJsonValueParser.h"

class LogCollectorGetConfigRequest : public JsonDeviceRequest
{
public:
    LogCollectorGetConfigRequest(const Json::Value& jsonRequestValue) :
        JsonDeviceRequest(jsonRequestValue)
    {
    }
};

class LogCollectorGetConfigResponse : public JsonResponse
{
public:
    LogCollectorGetConfigResponse(const Json::Value& jsonRequestValue, Json::Value& jsonResponseValue) :
        JsonResponse("LogCollectorGetConfigResponse", jsonRequestValue, jsonResponseValue)
    {
    }

    void SetPollingIntervalMS(uint32_t value)
    {
        m_jsonResponseValue[log_collector::POLLING_INTERVAL_MS] = value;
    }

    void SetMaxSingleFileSizeMB(uint32_t value)
    {
        m_jsonResponseValue[log_collector::MAX_SINGLE_FILE_SIZE_MB] = value;
    }

    void SetMaxNumOfLogFiles(uint32_t value)
    {
        m_jsonResponseValue[log_collector::MAX_NUM_OF_LOG_FILES] = value;
    }

    void SetConversionFilePath(const std::string& value)
    {
        m_jsonResponseValue[log_collector::CONVERSION_FILE_PATH] = value;
    }

    void SetLogFilesDirectoryPath(const std::string& value)
    {
        m_jsonResponseValue[log_collector::LOG_FILES_DIRECTORY] = value;
    }
    void SetCompressionMethod(const std::string& value)
    {
        m_jsonResponseValue[log_collector::COMPRESSION_METHOD] = value;
    }
    void SetTargetServer(const std::string& value)
    {
        m_jsonResponseValue[log_collector::TARGET_SERVER] = value;
    }
    void SetUserName(const std::string& value)
    {
        m_jsonResponseValue[log_collector::USER_NAME] = value;
    }
    void SetRemotePath(const std::string& value)
    {
        m_jsonResponseValue[log_collector::REMOTE_PATH] = value;
    }
};

class LogCollectorGetConfigHandler : public JsonOpCodeHandlerBase<LogCollectorGetConfigRequest, LogCollectorGetConfigResponse>
{
private:
    void HandleRequest(const LogCollectorGetConfigRequest& jsonRequest, LogCollectorGetConfigResponse& jsonResponse) override;
};

#endif  // _LOG_COLLECTOR_GET_CONFIG_HANDLER_H_
