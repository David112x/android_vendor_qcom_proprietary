/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef _LOG_COLLECTOR_START_RECORDING_H_
#define _LOG_COLLECTOR_START_RECORDING_H_

#include "JsonHandlerSDK.h"
#include "DebugLogger.h"
#include "LogCollectorJsonValueParser.h"
#include "LogCollectorDefinitions.h"

class LogCollectorStartRecordingRequest : public JsonDeviceRequest
{
public:
    LogCollectorStartRecordingRequest(const Json::Value& jsonRequestValue) :
        JsonDeviceRequest(jsonRequestValue)
    {
    }

    const JsonValueBoxed<CpuType> GetCpuType() const
    {
        return LogCollectorJsonValueParser::ParseCpuType(m_jsonRequestValue, "CpuType");
    }
    const JsonValueBoxed<std::string> GetDeviceNameBoxed() const
    {
        return JsonValueParser::ParseStringValue(m_jsonRequestValue, "Device");
    }
    const JsonValueBoxed<std::string> GetRecordingTypeBoxed() const
    {
        return JsonValueParser::ParseStringValue(m_jsonRequestValue, "RecordingType");
    }
    const JsonValueBoxed<bool> GetCompressBoxed() const
    {
        return JsonValueParser::ParseBooleanValue(m_jsonRequestValue, "Compress");
    }
    const JsonValueBoxed<bool> GetUploadBoxed() const
    {
        return JsonValueParser::ParseBooleanValue(m_jsonRequestValue, "Upload");
    }

};

class LogCollectorStartRecordingResponse : public JsonResponse
{
public:
    LogCollectorStartRecordingResponse(const Json::Value& jsonRequestValue, Json::Value& jsonResponseValue) :
        JsonResponse("LogCollectorStartRecordingResponse", jsonRequestValue, jsonResponseValue)
    {
    }
    void SetRecordingPath(const std::string & recPath)
    {
        m_jsonResponseValue["RecordingPath"] = recPath;
    }
    void SetRecordingType(log_collector::RecordingType recType)
    {
        std::stringstream rtss;
        rtss << recType;
        m_jsonResponseValue["RecordingType"] = rtss.str();
    }
    void SetCompress(bool compress)
    {
        m_jsonResponseValue["Compress"] = compress;
    }
    void SetUpload(bool upload)
    {
        m_jsonResponseValue["Upload"] = upload;
    }
};

class LogCollectorStartRecordingHandler : public JsonOpCodeHandlerBase<LogCollectorStartRecordingRequest, LogCollectorStartRecordingResponse>
{
private:
    void HandleRequest(const LogCollectorStartRecordingRequest& jsonRequest, LogCollectorStartRecordingResponse& jsonResponse) override;
};

#endif  // _LOG_COLLECTOR_START_RECORDING_H_
