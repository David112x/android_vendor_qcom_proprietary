////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxncssscutils.cpp
/// @brief CamX NCS SSC utility implementaion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE CP006:  QSEE Sensor interface requires us to use std string class
// NOWHINE FILE PR007b: QSEE Sensor interface
// NOWHINE FILE GR017:  Needed by QMI interface

#include <string>
#include <cinttypes>
#include <cmath>
#include <chrono>
#include <cutils/properties.h>
#include <fstream>
#include <string>
#include "camxincs.h"
#include "sns_client.pb.h"
#include "camxncssscconnection.h"
#include "camxncssscutils.h"


CAMX_NAMESPACE_BEGIN

using namespace std;
using namespace google::protobuf::io;

using namespace std::chrono;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorUid::SensorUid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorUid::SensorUid()
    : m_low(0)
    , m_high(0)
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorUid::SensorUid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorUid::SensorUid(
    uint64_t low,
    uint64_t high)
    : m_low(low)
    , m_high(high)
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SuidLookup::SuidLookup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SuidLookup::SuidLookup(
    suid_event_function cb)
    : m_callback(cb)
    , m_sscConnection(getSSCEventCb())
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SuidLookup::request_suid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SuidLookup::RequestSuid(std::string datatype)
{
    sns_client_request_msg  pb_req_msg;
    sns_suid_req            pb_suid_req;
    string                  pb_suid_req_encoded;
    bool                    default_only = false;

    const SensorUid LOOKUP_SUID =
    {
        12370169555311111083ull,
        12370169555311111083ull
    };

    CAMX_LOG_VERBOSE(CamxLogGroupNCS, "requesting suid for %s, ts = %fs", datatype.c_str(),
                    duration_cast<duration<float>>(high_resolution_clock::now(). time_since_epoch()).count());

    /* populate SUID request */
    pb_suid_req.set_data_type(datatype);
    pb_suid_req.set_register_updates(true);
    pb_suid_req.set_default_only(default_only);
    pb_suid_req.SerializeToString(&pb_suid_req_encoded);

    /* populate the client request message */
    pb_req_msg.set_msg_id(SNS_SUID_MSGID_SNS_SUID_REQ);
    pb_req_msg.mutable_request()->set_payload(pb_suid_req_encoded);
    pb_req_msg.mutable_suid()->set_suid_high(LOOKUP_SUID.m_high);
    pb_req_msg.mutable_suid()->set_suid_low(LOOKUP_SUID.m_low);
    pb_req_msg.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
    pb_req_msg.mutable_susp_config()->set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

    INT  msgSize = pb_req_msg.ByteSize();
    if (msgSize < SNS_CLIENT_REQ_LEN_MAX_V01)
    {
        CAMX_LOG_INFO(CamxLogGroupNCS, "Serialize message with size =%d", msgSize);
        pb_req_msg.SerializeToArray(m_requestBuffer, msgSize);
        m_sscConnection.SendRequest(m_requestBuffer, msgSize);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Request message is too large: size=%d", msgSize);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SuidLookup::HandleSSCEvent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SuidLookup::HandleSSCEvent(
    const uint8_t*  pData,
    size_t          size)
{
    /* parse the pb encoded event */
    sns_client_event_msg pb_event_msg;

    pb_event_msg.ParseFromArray(pData, size);

    // iterate over all events in the message
    for (int i = 0; i < pb_event_msg.events_size(); i++)
    {
        const ::sns_client_event_msg_sns_client_event& rPb_event = pb_event_msg.events(i);
        if (rPb_event.msg_id() != SNS_SUID_MSGID_SNS_SUID_EVENT)
        {
            CAMX_LOG_ERROR(CamxLogGroupNCS, "invalid event msg_id=%d", rPb_event.msg_id());
            continue;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupNCS, "sns client event size =%d", rPb_event.payload().size());
        if (rPb_event.has_payload())
        {
            sns_suid_event       pb_suid_event;

            pb_suid_event.ParseFromArray(rPb_event.payload().c_str(), rPb_event.payload().size());
            if (pb_suid_event.suid_size())
            {
                const string& rDatatype =  pb_suid_event.data_type();
                CAMX_LOG_VERBOSE(CamxLogGroupNCS, "suid_event for %s, num_suids=%d, ts=%fs", rDatatype.c_str(),
                         pb_suid_event.suid_size(),
                         duration_cast<duration<float>>(high_resolution_clock::now().
                                                        time_since_epoch()).count());
                // create a list of  all suids found for this datatype
                vector<SensorUid> suids(pb_suid_event.suid_size());
                for (int j=0; j < pb_suid_event.suid_size(); j++)
                {
                    suids[j] = SensorUid(pb_suid_event.suid(j).suid_low(),
                                          pb_suid_event.suid(j).suid_high());
                }
                // send callback for this datatype
                m_callback(rDatatype, suids);
            }
        }
    }
}

CAMX_NAMESPACE_END
