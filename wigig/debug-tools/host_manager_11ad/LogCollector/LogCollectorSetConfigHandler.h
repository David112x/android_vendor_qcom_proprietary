/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef _LOG_COLLECTOR_SET_CONFIG_HANDLER_H_
#define _LOG_COLLECTOR_SET_CONFIG_HANDLER_H_
#pragma once

#include "JsonHandlerSDK.h"
#include "DebugLogger.h"
#include "LogCollectorDefinitions.h"
#include "LogCollectorJsonValueParser.h"

class LogCollectorSetConfigRequest : public JsonDeviceRequest
{
public:
    LogCollectorSetConfigRequest(const Json::Value& jsonRequestValue) :
        JsonDeviceRequest(jsonRequestValue)
    {
    }

    JsonValueBoxed<uint32_t> GetPollingIntervalMs() const
    {
        return JsonValueParser::ParseUnsignedValue(m_jsonRequestValue, log_collector::POLLING_INTERVAL_MS.c_str());
    }

    JsonValueBoxed<uint32_t> GetMaxSingleFileSizeMb() const
    {
        return JsonValueParser::ParseUnsignedValue(m_jsonRequestValue, log_collector::MAX_SINGLE_FILE_SIZE_MB.c_str());
    }

    JsonValueBoxed<uint32_t> GetMaxNumOfLogFiles() const
    {
        return JsonValueParser::ParseUnsignedValue(m_jsonRequestValue, log_collector::MAX_NUM_OF_LOG_FILES.c_str());
    }

    JsonValueBoxed<std::string> GetConversionFilePath() const
    {
        return JsonValueParser::ParseStringValue(m_jsonRequestValue, log_collector::CONVERSION_FILE_PATH.c_str());
    }
    JsonValueBoxed<std::string> GetLogFilesDirectoryPath() const
    {
        return JsonValueParser::ParseStringValue(m_jsonRequestValue, log_collector::LOG_FILES_DIRECTORY.c_str());
    }

    JsonValueBoxed<std::string> GetTargetServer() const
    {
        return JsonValueParser::ParseStringValue(m_jsonRequestValue, log_collector::TARGET_SERVER.c_str());
    }
    JsonValueBoxed<std::string> GetUserName() const
    {
        return JsonValueParser::ParseStringValue(m_jsonRequestValue, log_collector::USER_NAME.c_str());
    }
    JsonValueBoxed<std::string> GetRemotePath() const
    {
        return JsonValueParser::ParseStringValue(m_jsonRequestValue, log_collector::REMOTE_PATH.c_str());
    }
};

class LogCollectorSetConfigResponse : public JsonResponse
{
public:
    LogCollectorSetConfigResponse(const Json::Value& jsonRequestValue, Json::Value& jsonResponseValue) :
        JsonResponse("LogCollectorSetConfigResponse", jsonRequestValue, jsonResponseValue)
    {
    }
};

class LogCollectorSetConfigHandler : public JsonOpCodeHandlerBase<LogCollectorSetConfigRequest, LogCollectorSetConfigResponse>
{
private:
    void HandleRequest(const LogCollectorSetConfigRequest& jsonRequest, LogCollectorSetConfigResponse& jsonResponse) override;
};

class LogCollectorReSetConfigRequest : public JsonDeviceRequest
{
public:
    LogCollectorReSetConfigRequest(const Json::Value& jsonRequestValue) :
        JsonDeviceRequest(jsonRequestValue)
    {
    }

    const JsonValueBoxed<CpuType> GetCpuType() const
    {
        return LogCollectorJsonValueParser::ParseCpuType(m_jsonRequestValue, "CpuType");
    }

};

class LogCollectorReSetConfigResponse : public JsonResponse
{
public:
    LogCollectorReSetConfigResponse(const Json::Value& jsonRequestValue, Json::Value& jsonResponseValue) :
        JsonResponse("LogCollectorReSetConfigResponse", jsonRequestValue, jsonResponseValue)
    {
    }
};

class LogCollectorReSetConfigHandler : public JsonOpCodeHandlerBase<LogCollectorReSetConfigRequest, LogCollectorReSetConfigResponse>
{
private:
    void HandleRequest(const LogCollectorReSetConfigRequest& jsonRequest, LogCollectorReSetConfigResponse& jsonResponse) override;
};

#endif  // _LOG_COLLECTOR_SET_CONFIG_HANDLER_H_
