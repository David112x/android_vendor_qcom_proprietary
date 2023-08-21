/*
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "aont.h"
#include <algorithm>
#include <sensors_log.h>
#include "sns_std_sensor.pb.h"
#include "sns_std_type.pb.h"
#include "sns_aont.pb.h"

using namespace std;

aont::aont() {
}

aont::~aont(){
}

/**
 * enable aont test
 *
 * @param void
 * @pri
**/
int aont::enable() {
    sns_logi("enable aont");
    _lookup = make_unique<suid_lookup>(
        [this](const string& datatype, const auto& suids)
        {
            suid_cbk(datatype, suids);
        });
    _lookup->request_suid("aont");
    return 0;
}

void aont::send_request(sensor_uid suid)
{
    string pb_req_msg_encoded;
    sns_client_request_msg pb_req_msg;
    sns_std_sensor_config config;
    sns_logd("sending on change config request for aont");

    _ssc_conn = make_unique<ssc_connection>(
    [](const uint8_t *data, size_t size)
    {
        handle_aont_event(data, size);
    });

    pb_req_msg.set_msg_id(SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG);
    pb_req_msg.mutable_request()->clear_payload();
    pb_req_msg.mutable_suid()->set_suid_high(suid.high);
    pb_req_msg.mutable_suid()->set_suid_low(suid.low);
    pb_req_msg.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
    pb_req_msg.mutable_susp_config()->
        set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);
    pb_req_msg.SerializeToString(&pb_req_msg_encoded);

    _ssc_conn->send_request(pb_req_msg_encoded);
}

void aont::suid_cbk(const std::string& datatype,
                       const std::vector<sensor_uid>& suids) {
    sns_logd("received response for %s suids.size() %d",
            datatype.c_str(),(unsigned int)suids.size());
    sensor_uid suid;
    for (vector<sensor_uid>::const_iterator it = suids.begin(); it != suids.end(); it++) {
        sns_logd("aont suid_low: %llu suid_high: %llu",
                (unsigned long long)it->low,
                (unsigned long long)it->high);
        suid.low = it->low;suid.high = it->high;
        send_request(suid);
    }
}

void aont::handle_aont_event(const uint8_t *data, size_t size){
    sns_client_event_msg pb_event_msg;
    sns_logd("Received indication with length %u", (unsigned int)size);
    pb_event_msg.ParseFromArray(data, size);
    for(int i = 0; i < pb_event_msg.events_size(); i++)
    {
      const sns_client_event_msg_sns_client_event &pb_event= pb_event_msg.events(i);
      sns_logd("event[%d] msg_id=%d, ts=%llu", i, pb_event.msg_id(),
               (unsigned long long) pb_event.timestamp());

      if (pb_event.msg_id() == SNS_AONT_MSGID_SNS_AONT_DATA) {
        sns_aont_data event;
        event.ParseFromString(pb_event.payload());
        sns_logv("Received sample size %d", event.aont_size());
#if 0
        sns_logd("Received sample <%f, %f, %f>",
        event.aont(0), event.aont(1), event.aont(2));
#endif
      }
    }
}
