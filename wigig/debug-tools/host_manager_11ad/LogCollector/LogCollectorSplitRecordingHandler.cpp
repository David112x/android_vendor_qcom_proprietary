/*
* Copyright (c) 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include "LogCollectorSplitRecordingHandler.h"
#include "Host.h"
#include "LogCollectorService.h"



void LogCollectorSplitRecordingHandler::HandleRequest(const Json::Value& jsonRequest, LogCollectorSplitRecordingResponse& jsonResponse)
{
    (void)jsonRequest;
    LOG_DEBUG << "Log Collector split recording" << std::endl;

    const OperationStatus os = Host::GetHost().GetLogCollectorService().SplitAllRecordings();

    if (!os)
    {
        jsonResponse.Fail(os.GetStatusMessage());
    }
}

