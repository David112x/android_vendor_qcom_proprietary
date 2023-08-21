/*=============================================================================
  @file sns_client_example.cpp

  Example client written in C++, using libssc.  Client requests Accel SUID,
  and subsequently sends an enable request to it.  Data will stream for 10
  seconds.

  Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ===========================================================================*/

/*=============================================================================
  Include Files
  ===========================================================================*/
#include <iostream>
#include <cinttypes>
#include <unistd.h>
#include "ssc_suid_util.h"
#include <string>
#include <unordered_map>
#include <vector>

#include "sns_std_sensor.pb.h"
#include "sns_std_type.pb.h"
#include "sns_client.pb.h"

using namespace std;

/*=============================================================================
  Macro Definitions
  ===========================================================================*/

#ifndef UNUSED_VAR
#define UNUSED_VAR(var) ((void)(var));
#endif

#define SENSOR_NAME "accel"
#define TEST_LENGTH 20  // 10 Seconds
#define SAMPLE_RATE 25  // 10 Hz
#define BATCH_PERIOD 0  // 0 ms
static int total_samples_rxved = 0;

/*=============================================================================
  Static Data
  ===========================================================================*/

static ssc_connection *connection;

/*=============================================================================
  Static Function Definitions
  ===========================================================================*/

/**
 * Event callback function, as registered with ssc_connection.
 */
static void
event_cb(const uint8_t *data, size_t size, uint64_t ts)
{
  sns_client_event_msg pb_event_msg;
  UNUSED_VAR(ts);

  sns_logv("Received QMI indication with length %zu", size);

  pb_event_msg.ParseFromArray(data, size);
  for(int i = 0; i < pb_event_msg.events_size(); i++)
  {
    const sns_client_event_msg_sns_client_event &pb_event= pb_event_msg.events(i);
    sns_logv("Event[%i] msg_id=%i, ts=%llu", i, pb_event.msg_id(),
        pb_event.timestamp());

    if(SNS_STD_MSGID_SNS_STD_ERROR_EVENT == pb_event.msg_id())
    {
      sns_std_error_event error;
      error.ParseFromString(pb_event.payload());

      sns_loge("Received error event %i", error.error());
    }
    else if(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT == pb_event.msg_id())
    {
      sns_std_sensor_physical_config_event config;
      config.ParseFromString(pb_event.payload());

      sns_loge("Received config event with sample rate %f", config.sample_rate());
    }
    else if(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_EVENT == pb_event.msg_id())
    {
      sns_std_sensor_event event;
      event.ParseFromString(pb_event.payload());
      sns_logi("Received sample <%f, %f, %f>",
          event.data(0), event.data(1), event.data(2));
      total_samples_rxved++;
    }
    else
    {
      sns_loge("Received unknown message ID %i", pb_event.msg_id());
    }
  }
}

/**
 * Send a SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG to SUID
 */
static void
send_config_req(ssc_connection *conn, sensor_uid const *suid)
{
  string pb_req_msg_encoded;
  string config_encoded;
  sns_client_request_msg pb_req_msg;
  sns_std_sensor_config config;

  sns_logi("Send config request with sample rate %i and batch period %d",
                SAMPLE_RATE, BATCH_PERIOD);

  config.set_sample_rate(SAMPLE_RATE);
  config.SerializeToString(&config_encoded);

  pb_req_msg.set_msg_id(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG);
  pb_req_msg.mutable_request()->set_payload(config_encoded);
  pb_req_msg.mutable_suid()->set_suid_high(suid->high);
  pb_req_msg.mutable_suid()->set_suid_low(suid->low);
  pb_req_msg.mutable_susp_config()->
    set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
  pb_req_msg.mutable_susp_config()->
    set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);
  pb_req_msg.mutable_request()->mutable_batching()->
    set_batch_period(BATCH_PERIOD);
  pb_req_msg.SerializeToString(&pb_req_msg_encoded);
  conn->send_request(pb_req_msg_encoded);
}

/**
 * SUID callback as registered with suid_lookup.
 */
static void
suid_cb(const std::string& datatype, const std::vector<sensor_uid>& suids)
{
  sns_logv("Received SUID event with length %zu", suids.size());
  if(suids.size() > 0)
  {
    sensor_uid suid = suids.at(0);
    connection = new ssc_connection(event_cb);

    sns_logi("Received SUID %" PRIx64 "%" PRIx64 " for '%s'",
        suid.high, suid.low, datatype.c_str());

    printf("Received SUID %" PRIx64 "%" PRIx64 " for '%s' \n",
        suid.high, suid.low, datatype.c_str());
    send_config_req(connection, &suid);
  } else {
    sns_logi("%s sensor is not available", SENSOR_NAME);
    printf("%s sensor is not available \n", SENSOR_NAME);
    exit(-1);
  }
}

int
main(int argc, char *argv[])
{
  UNUSED_VAR(argc);
  UNUSED_VAR(argv);
  printf("##################################################################################################\n");
  printf("streaming started for '%s' set SR/RR '(%d/%d)Hz' and duration '%dSec' \n", SENSOR_NAME,
                            SAMPLE_RATE, BATCH_PERIOD, TEST_LENGTH);
  sns_logi("streaming started for '%s' set SR/RR '(%d/%d)Hz' and duration '%dSec' ", SENSOR_NAME,
                            SAMPLE_RATE, BATCH_PERIOD, TEST_LENGTH);
  suid_lookup lookup(suid_cb);
  lookup.request_suid(SENSOR_NAME);
  sleep(TEST_LENGTH);
  delete connection;
  sns_logi("Received %d samples for '%s' sensor, set SR/RR '(%d/%d)Hz' and duration '%dSec'", total_samples_rxved, SENSOR_NAME,
                            SAMPLE_RATE, BATCH_PERIOD, TEST_LENGTH);

  printf("Received %d samples for '%s' sensor, set SR/RR '(%d/%d)Hz' and duration '%dSec' \n", total_samples_rxved, SENSOR_NAME,
                            SAMPLE_RATE, BATCH_PERIOD, TEST_LENGTH);
  printf("##################################################################################################\n");
  return 0;
}

