/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include "JsonHandlerSDK.h"
#include "LogCollectorJsonValueParser.h"
#include "LogCollectorDefinitions.h"

class LogCollectorStartDeferredRecordingRequest : public JsonDeviceRequest
{
public:
    LogCollectorStartDeferredRecordingRequest(const Json::Value& jsonRequestValue) :
        JsonDeviceRequest(jsonRequestValue)
    {}

    JsonValueBoxed<log_collector::RecordingType> GetRecordingTypeBoxed() const
    {
        return LogCollectorJsonValueParser::ParseRecordingType(m_jsonRequestValue, "RecordingType");
    }
    JsonValueBoxed<bool> GetCompressBoxed() const
    {
        return JsonValueParser::ParseBooleanValue(m_jsonRequestValue, "Compress");
    }
    JsonValueBoxed<bool> GetUploadBoxed() const
    {
        return JsonValueParser::ParseBooleanValue(m_jsonRequestValue, "Upload");
    }

};

class LogCollectorStartDeferredRecordingHandler : public JsonOpCodeHandlerBase<LogCollectorStartDeferredRecordingRequest, JsonBasicResponse>
{
private:
    void HandleRequest(const LogCollectorStartDeferredRecordingRequest& jsonRequest, JsonBasicResponse& jsonResponse) override;
};

class LogCollectorStopDeferredRecordingHandler : public JsonOpCodeHandlerBase<Json::Value, JsonBasicResponse>
{
private:
    void HandleRequest(const Json::Value& jsonRequest, JsonBasicResponse& jsonResponse) override;
};
