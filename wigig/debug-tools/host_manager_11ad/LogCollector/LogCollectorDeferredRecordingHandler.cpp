/*
* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include "LogCollectorDeferredRecordingHandler.h"
#include "Host.h"
#include "DeviceManager.h"
#include "LogCollectorService.h"

void LogCollectorStartDeferredRecordingHandler::HandleRequest(const LogCollectorStartDeferredRecordingRequest& jsonRequest, JsonBasicResponse& jsonResponse)
{
    bool compress = false;
    const JsonValueBoxed<bool> compressBoxed = jsonRequest.GetCompressBoxed();
    if (compressBoxed.GetState() == JsonValueState::JSON_VALUE_MALFORMED)
    {
        jsonResponse.Fail("compress should be true/false");
        return;
    }
    if (compressBoxed.GetState() == JsonValueState::JSON_VALUE_PROVIDED)
    {
        compress = compressBoxed.GetValue();
    }

    bool upload = false;
    const JsonValueBoxed<bool> uploadBoxed = jsonRequest.GetUploadBoxed();
    if (uploadBoxed.GetState() == JsonValueState::JSON_VALUE_MALFORMED)
    {
        jsonResponse.Fail("upload should be true/false");
        return;
    }
    if (uploadBoxed.GetState() == JsonValueState::JSON_VALUE_PROVIDED)
    {
        upload = uploadBoxed.GetValue();
    }


    const JsonValueBoxed<log_collector::RecordingType> RecordingTypeBoxed = jsonRequest.GetRecordingTypeBoxed();
    log_collector::RecordingType recordingType;
    switch (RecordingTypeBoxed.GetState())
    {
    case JsonValueState::JSON_VALUE_MALFORMED:
        jsonResponse.Fail("RecordingType is wrong, it should be 'raw' or 'txt' or empty (default is raw)");
        return;
    case JsonValueState::JSON_VALUE_MISSING:
        recordingType = log_collector::RECORDING_TYPE_XML;
        break;
    default:
        // guaranty that value is provided.
        recordingType = RecordingTypeBoxed;
        break;
    }

    LOG_DEBUG << "Starting deferred recording (recording_type=" << recordingType
        << ", compress=" << BoolStr(compress) << ", upload=" << BoolStr(upload) << ")" << std::endl;

    OperationStatus os1;
    if(recordingType == log_collector::RECORDING_TYPE_XML || recordingType == log_collector::RECORDING_TYPE_BOTH)
    {
       os1  = Host::GetHost().GetLogCollectorService().EnableDeferredLogging(log_collector::XmlRecorder, compress, upload);
    }
    OperationStatus os2;
    if (recordingType == log_collector::RECORDING_TYPE_TXT || recordingType == log_collector::RECORDING_TYPE_BOTH)
    {
        os2 = Host::GetHost().GetLogCollectorService().EnableDeferredLogging(log_collector::TxtRecorder, compress, upload);
    }
    os1 = OperationStatus::Merge(os1, os2);
    if (!os1)
    {
        jsonResponse.Fail(os1.GetStatusMessage());
    }
}

void LogCollectorStopDeferredRecordingHandler::HandleRequest(const Json::Value& jsonRequest, JsonBasicResponse& jsonResponse)
{
    (void)jsonRequest;
    (void)jsonResponse;
    LOG_DEBUG << "Stopping deferred recording " << std::endl;
    Host::GetHost().GetLogCollectorService().DisableDeferredLogging();
}
