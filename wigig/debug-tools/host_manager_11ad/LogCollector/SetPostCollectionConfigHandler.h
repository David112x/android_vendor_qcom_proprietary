/*
* Copyright (c) 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef _SET_POST_COLLECTION_CONFIG_HANDLER_H_
#define _SET_POST_COLLECTION_CONFIG_HANDLER_H_
#pragma once

#include "JsonHandlerSDK.h"
#include "DebugLogger.h"
#include "LogCollectorJsonValueParser.h"

class SetPostCollecitonConfigRequest : public JsonDeviceRequest
{
public:
    SetPostCollecitonConfigRequest(const Json::Value& jsonRequestValue) :
        JsonDeviceRequest(jsonRequestValue)
    {
    }

    const JsonValueBoxed<std::string> GetCompressionApp() const
    {
        return JsonValueParser::ParseStringValue(m_jsonRequestValue, "CompressionApp");
    }
    const JsonValueBoxed<std::string> GetTargetServer() const
    {
        return JsonValueParser::ParseStringValue(m_jsonRequestValue, "TargetServer");
    }
    const JsonValueBoxed<std::string> GetUserName() const
    {
        return JsonValueParser::ParseStringValue(m_jsonRequestValue, "UserName");
    }
    const JsonValueBoxed<std::string> GetRemotePath() const
    {
        return JsonValueParser::ParseStringValue(m_jsonRequestValue, "RemotePath");
    }
};

class SetPostCollectionConfigResponse : public JsonResponse
{
public:
    SetPostCollectionConfigResponse(const Json::Value& jsonRequestValue, Json::Value& jsonResponseValue) :
        JsonResponse("SetPostCollectionConfigResponse", jsonRequestValue, jsonResponseValue)
    {
    }
};

class SetPostCollectionConfigHandler : public JsonOpCodeHandlerBase<SetPostCollecitonConfigRequest, SetPostCollectionConfigResponse>
{
private:
    void HandleRequest(const SetPostCollecitonConfigRequest& jsonRequest, SetPostCollectionConfigResponse& jsonResponse) override;
};


#endif  // _SET_POST_COLLECTION_CONFIG_HANDLER_H_
