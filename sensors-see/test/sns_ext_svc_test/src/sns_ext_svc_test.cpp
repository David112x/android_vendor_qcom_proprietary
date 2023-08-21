/*=============================================================================
  @file sns_ext_svc_test.cpp

  Test client for ext_svc Sensor.  This Sensor provides a bridge for SEE Sensors
  to access external QMI services.  This particular test client tests access
  to the QMI_LOC service.

  Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ===========================================================================*/

/*=============================================================================
  Include Files
  ===========================================================================*/
#include <iostream>
#include <cinttypes>
#include "ssc_connection.h"
#include "ssc_utils.h"
#include "sensors_log.h"
#include "sns_client.pb.h"
#include "sns_suid.pb.h"
#include "sns_ext_svc.pb.h"
#include "location_service_v02.h"

using namespace std;

/*=============================================================================
  Macro Definitions
  ===========================================================================*/

#ifndef ARR_SIZE
#define ARR_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))
#endif

#ifndef UNUSED_VAR
#define UNUSED_VAR(var) ((void)(var));
#endif

/*=============================================================================
  Static Data
  ===========================================================================*/

static ssc_connection *connection;

static bool has_duration = false;
static float duration = 0;  // seconds
static int times = 1; // number of times to run SNS_EXT_SVC

/*=============================================================================
  Static Function Definitions
  ===========================================================================*/

static void
event_cb(const uint8_t *data, size_t size)
{
  sns_client_event_msg pb_event_msg;

  sns_logv("Received QMI indication with length %zu", size);

  pb_event_msg.ParseFromArray(data, size);
  for(int i = 0; i < pb_event_msg.events_size(); i++)
  {
    const sns_client_event_msg_sns_client_event &pb_event= pb_event_msg.events(i);
    sns_logv("event[%d] msg_id=%d, ts=%llu", i, pb_event.msg_id(),
             (unsigned long long) pb_event.timestamp());

    if(SNS_STD_MSGID_SNS_STD_ERROR_EVENT == pb_event.msg_id())
    {
      sns_std_error_event error;
      error.ParseFromString(pb_event.payload());

      sns_loge("Received error event %i", error.error());
    }
    else if(SNS_EXT_SVC_MSGID_SNS_EXT_SVC_RESP == pb_event.msg_id())
    {
      sns_ext_svc_resp resp;
      resp.ParseFromString(pb_event.payload());

      sns_logv("Received response msg_id %i, transp_err %i, transaction_id %i",
          resp.msg_id(), resp.transp_err(), resp.transaction_id());
    }
    else if(SNS_EXT_SVC_MSGID_SNS_EXT_SVC_IND == pb_event.msg_id())
    {

      sns_ext_svc_ind ind;

      ind.ParseFromString(pb_event.payload());

      sns_logv("Received ind msg_id %i", ind.msg_id());
      switch (ind.msg_id()) {
      case QMI_LOC_EVENT_POSITION_REPORT_IND_V02:
          qmiLocEventPositionReportIndMsgT_v02 pos_ind;
          memcpy(&pos_ind, ind.payload().c_str(), ind.payload().size());
          sns_logv("Position:%" PRIu64 " , %f, %f, %f",
                    pos_ind.timestampUtc,
                    pos_ind.latitude,
                    pos_ind.longitude,
                    pos_ind.altitudeWrtEllipsoid);
          break;

      case QMI_LOC_EVENT_GEOFENCE_BATCHED_BREACH_NOTIFICATION_IND_V02:
          qmiLocEventGeofenceBatchedBreachIndMsgT_v02 breach_ind;
          memcpy(&breach_ind, ind.payload().c_str(), ind.payload().size());
          sns_logv("GF Batched Breach!!!!");
          switch (breach_ind.breachType) {
          case eQMI_LOC_GEOFENCE_BREACH_TYPE_ENTERING_V02:
              sns_logv("GEOFENCE BREACH ENTER");
              break;
          case eQMI_LOC_GEOFENCE_BREACH_TYPE_LEAVING_V02:
              sns_logv("GEOFENCE BREACH EXIT");
              break;
          default:
              sns_logv("GEOFENCE BREACH UNKNOWN");
              break;
          }
          sns_logv("Breach Position:%" PRIu64 " , %f, %f, %f",
                    breach_ind.geofencePosition.timestampUtc,
                    breach_ind.geofencePosition.latitude,
                    breach_ind.geofencePosition.longitude,
                    breach_ind.geofencePosition.altitudeWrtEllipsoid);
          break;

      case QMI_LOC_EVENT_GEOFENCE_BATCHED_DWELL_NOTIFICATION_IND_V02:
          qmiLocEventGeofenceBatchedDwellIndMsgT_v02 dwell_ind;
          memcpy(&dwell_ind, ind.payload().c_str(), ind.payload().size());
          sns_logv("GF Dwell Breach!!!!");
          switch (dwell_ind.dwellType) {
          case eQMI_LOC_GEOFENCE_DWELL_TYPE_INSIDE_V02:
              sns_logv("GEOFENCE DWELL IN");
              break;
          case eQMI_LOC_GEOFENCE_DWELL_TYPE_OUTSIDE_V02:
              sns_logv("GEOFENCE DWELL OUT");
              break;
          default:
              sns_logv("GEOFENCE DWELL UNKNOWN");
              break;
          }
          sns_logv("Dwell Position:%" PRIu64 " , %f, %f, %f",
                    dwell_ind.geofencePosition.timestampUtc,
                    dwell_ind.geofencePosition.latitude,
                    dwell_ind.geofencePosition.longitude,
                    dwell_ind.geofencePosition.altitudeWrtEllipsoid);
          break;
      }
    }
    else
    {
      sns_loge("Received unknown message ID %i", pb_event.msg_id());
    }
  }
}

/* Send a qmiLocRegEventsReqMsgT_v02 to QMI_LOC */
static void
send_event_req(ssc_connection *conn, sensor_uid const *suid)
{
  string pb_req_msg_encoded;
  string ext_svc_req_encoded;
  sns_client_request_msg pb_req_msg;
  sns_ext_svc_req ext_svc_req;
  qmiLocRegEventsReqMsgT_v02 loc_req;
  memset(&loc_req, 0, sizeof(loc_req));

  loc_req.eventRegMask = QMI_LOC_EVENT_MASK_POSITION_REPORT_V02 |
                         QMI_LOC_EVENT_MASK_GEOFENCE_BREACH_NOTIFICATION_V02 |
                         QMI_LOC_EVENT_MASK_GEOFENCE_BATCH_DWELL_NOTIFICATION_V02 |
                         QMI_LOC_EVENT_MASK_GEOFENCE_BATCH_BREACH_NOTIFICATION_V02;

  loc_req.clientStrId_valid = true;
  strlcpy(loc_req.clientStrId,"MHAL", sizeof(loc_req.clientStrId));

  loc_req.clientType_valid = true;
  loc_req.clientType = eQMI_LOC_CLIENT_AFW_V02;
  loc_req.enablePosRequestNotification_valid = true;
  loc_req.enablePosRequestNotification = false;

  ext_svc_req.set_svc_id(SNS_EXT_SVC_LOCATION);
  ext_svc_req.set_msg_id(QMI_LOC_REG_EVENTS_REQ_V02);
  ext_svc_req.set_transaction_id(5);  // Arbitrary value
  ext_svc_req.set_payload(&loc_req, sizeof(loc_req));
  ext_svc_req.SerializeToString(&ext_svc_req_encoded);

  pb_req_msg.set_msg_id(SNS_EXT_SVC_MSGID_SNS_EXT_SVC_REQ);
  pb_req_msg.mutable_request()->set_payload(ext_svc_req_encoded);
  pb_req_msg.mutable_suid()->set_suid_high(suid->high);
  pb_req_msg.mutable_suid()->set_suid_low(suid->low);
  pb_req_msg.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
  pb_req_msg.mutable_susp_config()->
      set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

  pb_req_msg.SerializeToString(&pb_req_msg_encoded);

  conn->send_request(pb_req_msg_encoded);
}


/* Send a qmiLocAddCircularGeofenceReqMsgT_v02 to QMI_LOC */
static void
send_add_gf_req(ssc_connection *conn, sensor_uid const *suid, int n)
{
    string pb_req_msg_encoded;
    string ext_svc_req_encoded;
    sns_client_request_msg pb_req_msg;
    sns_ext_svc_req ext_svc_req;
    qmiLocAddCircularGeofenceReqMsgT_v02 addReq;
    memset(&addReq, 0, sizeof(addReq));

    addReq.confidence_valid = true;
    addReq.confidence = eQMI_LOC_GEOFENCE_CONFIDENCE_HIGH_V02;
    addReq.responsiveness_valid = 1;
    addReq.responsiveness = eQMI_LOC_GEOFENCE_RESPONSIVENESS_CUSTOM_V02;
    addReq.customResponsivenessValue_valid = true;
    addReq.dwellTime_valid = 1;
    addReq.dwellTypeMask_valid = 1;
    addReq.includePosition = true;
    switch (n) {
    case 1:
        addReq.breachMask |= QMI_LOC_GEOFENCE_BREACH_LEAVING_MASK_V02;
        addReq.dwellTypeMask |= QMI_LOC_GEOFENCE_DWELL_TYPE_OUTSIDE_MASK_V02;
        addReq.customResponsivenessValue = 3;
        addReq.dwellTime = 5;
        addReq.circularGeofenceArgs.latitude = 38.374722;
        addReq.circularGeofenceArgs.longitude = -122.984276;
        addReq.circularGeofenceArgs.radius = 100;
        break;
    case 2:
        addReq.breachMask |= QMI_LOC_GEOFENCE_BREACH_ENTERING_MASK_V02;
        addReq.dwellTypeMask |= QMI_LOC_GEOFENCE_DWELL_TYPE_INSIDE_MASK_V02;
        addReq.customResponsivenessValue = 6;
        addReq.dwellTime = 10;
        addReq.circularGeofenceArgs.latitude = 37.374722;
        addReq.circularGeofenceArgs.longitude = -121.984276;
        addReq.circularGeofenceArgs.radius = 1000;
        break;
    default:
        break;
    }
    addReq.transactionId = n;

    ext_svc_req.set_svc_id(SNS_EXT_SVC_LOCATION);
    ext_svc_req.set_msg_id(QMI_LOC_ADD_CIRCULAR_GEOFENCE_REQ_V02);
    ext_svc_req.set_transaction_id(5);  // Arbitrary value
    ext_svc_req.set_payload(&addReq, sizeof(addReq));
    ext_svc_req.SerializeToString(&ext_svc_req_encoded);

    pb_req_msg.set_msg_id(SNS_EXT_SVC_MSGID_SNS_EXT_SVC_REQ);
    pb_req_msg.mutable_request()->set_payload(ext_svc_req_encoded);
    pb_req_msg.mutable_suid()->set_suid_high(suid->high);
    pb_req_msg.mutable_suid()->set_suid_low(suid->low);
    pb_req_msg.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
    pb_req_msg.mutable_susp_config()->
        set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

    pb_req_msg.SerializeToString(&pb_req_msg_encoded);

    conn->send_request(pb_req_msg_encoded);
}

/* Send a qmiLocStartReqMsgT_v02 to QMI_LOC */
static void
send_start_req(ssc_connection *conn, sensor_uid const *suid)
{
  string pb_req_msg_encoded;
  string ext_svc_req_encoded;
  sns_client_request_msg pb_req_msg;
  sns_ext_svc_req ext_svc_req;
  qmiLocStartReqMsgT_v02 loc_req;
  memset(&loc_req, 0, sizeof(loc_req));

  loc_req.sessionId = 0;
  loc_req.fixRecurrence_valid = true;
  loc_req.fixRecurrence = eQMI_LOC_RECURRENCE_PERIODIC_V02;
  loc_req.minInterval_valid = false;

  ext_svc_req.set_svc_id(SNS_EXT_SVC_LOCATION);
  ext_svc_req.set_msg_id(QMI_LOC_START_REQ_V02);
  ext_svc_req.set_transaction_id(5);  // Arbitrary value
  ext_svc_req.set_payload(&loc_req, sizeof(loc_req));
  ext_svc_req.SerializeToString(&ext_svc_req_encoded);

  pb_req_msg.set_msg_id(SNS_EXT_SVC_MSGID_SNS_EXT_SVC_REQ);
  pb_req_msg.mutable_request()->set_payload(ext_svc_req_encoded);
  pb_req_msg.mutable_suid()->set_suid_high(suid->high);
  pb_req_msg.mutable_suid()->set_suid_low(suid->low);
  pb_req_msg.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
  pb_req_msg.mutable_susp_config()->
      set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

  pb_req_msg.SerializeToString(&pb_req_msg_encoded);

  conn->send_request(pb_req_msg_encoded);
}

static void
suid_cb(const std::string& datatype, const std::vector<sensor_uid>& suids)
{
  sns_logv("Received SUID event with length %zu", suids.size());
  if(suids.size() > 0)
  {
    sensor_uid suid = suids.at(0);
    connection = new ssc_connection(event_cb);

    sns_logv("Received SUID %" PRIx64 "%" PRIx64 " for '%s'",
        suid.high, suid.low, datatype.c_str());

    send_event_req(connection, &suid);
    /* Adding two geofences for test purposes
       One GF will have center 'here' and will be breached
       on ENTER/DWELL IN, the other will have the center
       far from here and will be breached on EXIT/DWELL OUT
       This way both will be breached right away */
    send_add_gf_req(connection, &suid, 1);
    send_add_gf_req(connection, &suid, 2);
    send_start_req(connection, &suid);
  }
}

static void
print_usage()
{
   cout << "  usage: sns_ext_svc_test ";
   cout << "[-duration=<seconds> -times=<number>] [-help]";
   cout << endl;
}

/**
 * @brief parse command line argument of the form keyword=value
 * @param parg[i] - command line argument
 * @param key[io] - gets string to left of '='
 * @param value[io] - sets string to right of '='
 * @return true when input argument contains an '=', otherwise false
 */
static bool
get_key_value(char *parg, string &key, string &value)
{
   if ( parg) {
      char *pkey = parg;

      while (char c = *parg) {
         if (c == '=') {
            key = string(pkey, parg - pkey);
            value = string(++parg);
            return true;
         }
         parg++;
      }
   }
   return false;
}

static void
parse_args(int argc, char *argv[])
{
   string key;
   string value;
   bool valid_args = true;

   for (int i = 1; i < argc; i++) {
       if (0 == strncmp(argv[i], "-h", 2)) {
          valid_args = false;
          break;
       }
       if (get_key_value(argv[i], key, value)) {
          if (key == "-duration") {
             has_duration = true;
             duration = atof(value.c_str());
          }
          else if (key == "-times") {
             times = atoi(value.c_str());
          }
          else {
             valid_args = false;
             cout << "  unrecognized arg: " << argv[i] << endl;
          }
       }
       else {
          valid_args = false;
          cout << "  unrecognized arg: " << argv[i] << endl;
       }
   }
   if (!valid_args) {
      print_usage();
      exit(4);
   }
}

int
main(int argc, char *argv[])
{
   parse_args(argc, argv);

   sensors_log::set_tag("sns_ext_svc_test");
   sensors_log::set_level(sensors_log::VERBOSE);
   sensors_log::set_stderr_logging(true);

   for (int i = 1; i <= times; i++) {
      sns_logv("Iteration %i", i);

      suid_lookup lookup(suid_cb);
      lookup.request_suid("ext_svc");

      if (has_duration) {
         usleep((int)(duration * 1000000)); // microseconds
      }
      else {
         cin.get(); // wait for enter key
         break;
      }
      if ( connection) {
         connection->~ssc_connection();
      }
   }
   cout << "PASS";

   return 0;
}
