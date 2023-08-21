/*
* Copyright (c) 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include "JsonHandlerSDK.h"

class LogCollectorSplitRecordingResponse : public JsonResponse
{
public:
    LogCollectorSplitRecordingResponse(const Json::Value& jsonRequestValue, Json::Value& jsonResponseValue) :
        JsonResponse("LogCollectorSplitRecordingResponse", jsonRequestValue, jsonResponseValue)
    {

    }
};

class LogCollectorSplitRecordingHandler : public JsonOpCodeHandlerBase<Json::Value, LogCollectorSplitRecordingResponse>
{
private:
    void HandleRequest(const Json::Value& jsonRequest, LogCollectorSplitRecordingResponse& jsonResponse) override;
};
