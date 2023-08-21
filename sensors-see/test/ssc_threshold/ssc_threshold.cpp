/* ===================================================================
** Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: ssc_threshold.cpp
** DESC: Android command line application for threshold sensor testing
** ================================================================ */
#include <iostream>
#include <sstream>
#include <cinttypes>
#include <unistd.h>

#include "sns_client.pb.h"
#include "sns_std_sensor.pb.h"
#include "sns_resampler.pb.h"
#include "sns_suid.pb.h"
#include "sns_cal.pb.h"
#include "sns_threshold.pb.h"
#include "sns_diag_sensor.pb.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

#include "ssc_connection.h"
#include "sensors_log.h"
#include "sensor_diag_log.h"
#include "ssc_utils.h"

using namespace google::protobuf::io;
using namespace std;

/* forward declarations */
static void signal_handler(int signum);
static void debug_message( string message);

static void parse_arguments(int argc, char *argv[]);
static bool get_key_value(char *parg, string &key, string &value);
static bool parse_boolean(const char *parg);
static sns_resampler_rate parse_rate_type( string value);
static bool parse_thresholds_type( string value);
static void usage(void);
static void debug_message( string message);

static void event_cb(const uint8_t *data, size_t size);
static void resp_cb(uint32_t resp_value);

static void log_event( string from_data_type, string encoded_event_msg);
static string lookup_data_type( sns_std_suid &suid);

static bool do_attributes_match_target( sns_std_attr_event pb_attr_event);
static void show_sensor_event(
                   const sns_client_event_msg_sns_client_event &pb_event);
static void show_physical_config_event(
                   const sns_client_event_msg_sns_client_event &pb_event);
static void show_resampler_config_event(
                   const sns_client_event_msg_sns_client_event &pb_event);

static void get_suid( const std::string &data_type);
static void suid_cb(const std::string& datatype,
                    const std::vector<sensor_uid>& suids);

static void attributes_lookup( string &data_type, sensor_uid &suid);
static void run();
static void send_disable_req();
static void send_island_log_trigger_req(uint64_t cookie);

/* data */
static char *my_exe_name = (char *)"ssc_threshold";
static ssc_connection *connection;
static sensor_uid diag_sensor_suid;
static sensor_uid threshold_suid;
static sensor_uid resampler_suid;
static sensor_uid target_suid;
static bool found_target = false;
static uint64_t this_client_id = 0xcafe;
static sensors_diag_log *sensors_diag_log_obj = NULL;
static char logging_module_name[] = "ssc_threshold";
static string lit_diag_sensor("diag_sensor");
static string lit_threshold("threshold");
static string resp_from("not_known"); // used by resp_cb()

typedef struct {
    string      data_type;
    sensor_uid  suid;
} dtuid_pair;

static std::vector<dtuid_pair> pairs;

struct app_config {
    bool  is_valid = true;          // command line parse successful?
    char  target_data_type[ 16];
    bool  has_batch_period = false;
    float batch_period = -1;        // seconds. default omitted.
    float duration = 10.0;          // seconds
    float target_rate = 10.0;       // hs sample_rate

    bool  use_resampler = false;
    sns_resampler_rate rate_type = SNS_RESAMPLER_RATE_MINIMUM;
    bool  filter = false;

    bool  use_island_log = false;

    vector<float> thresholds;
    sns_threshold_type threshold_type = SNS_THRESHOLD_TYPE_RELATIVE_VALUE;

    bool debug = false;

    bool  has_name = false;         // command line specifies -name=
    char  name[ 32];                // <name>
    bool  has_rigid_body = false;   // command line specifies -rigid_body=
    char  rigid_body[ 32];          // <display | keyboard | external>

    bool  has_hw_id = false;        // command line specifies -hw_id=
    int   hw_id = 0;                // <number>

    bool  has_delay = false;        // command line specifies -delay=
    float delay = 0;                // number of seconds

    app_config() {}
};

static app_config config;

static bool get_suid_complete = false;
static bool attrib_lookup_complete = false;
static std::mutex m;
static std::condition_variable cv;

// //////// ///// /////// / ////
// //////// ///// /////// / ////
// //////// ///// /////// / ////

int main(int argc, char* argv[])
{
    int rc = 0;
    string passfail = "PASS";

    cout << my_exe_name << " version 1.14" << endl;
    signal(SIGINT, signal_handler);

    sensors_diag_log_obj = new sensors_diag_log();
    if (sensors_diag_log_obj == nullptr) {
       cout << "FAIL - new sensors_diag_log() failed" << endl;
       return 4;
    }

    parse_arguments( argc, argv);

    if ( config.has_delay) {
       string message( "delay startup " + to_string(config.delay) + " seconds");
       cout << message << endl;
       useconds_t microsecs = (useconds_t)( config.delay * 1000000);
       usleep( microsecs);
    }

    if ( config.debug > 1) {
       sensors_log::set_tag("ssc_threshold");
       sensors_log::set_level(sensors_log::VERBOSE);
       sensors_log::set_stderr_logging(true);
    }

    try {
        connection = new ssc_connection(event_cb);
        if (connection == nullptr) {
           cout << "FAIL - new ssc_connection() failed" << endl;
           return 4;
        }
        connection->register_resp_cb(resp_cb);

        suid_lookup lookup(suid_cb);

        // get relevant suids
        if ( config.use_island_log) {
           get_suid( "diag_sensor");
        }
        if ( config.use_resampler) {
           get_suid( "resampler");
        }
        get_suid( "threshold");
        get_suid( config.target_data_type);

        // lookup sensor attributes for these suids
        for ( size_t i = 0; i < pairs.size(); i++) {
           attributes_lookup( pairs[i].data_type, pairs[i].suid);
        }

        if ( config.use_island_log &&
             diag_sensor_suid.high == 0 && diag_sensor_suid.low == 0) {
           throw runtime_error( "diag_sensor not found.");
        }
        if ( config.use_resampler &&
             resampler_suid.high == 0 && resampler_suid.low == 0) {
           throw runtime_error( "resampler sensor not found.");
        }
        if ( threshold_suid.high == 0 && threshold_suid.low == 0) {
           throw runtime_error( "threshold sensor not found.");
        }

        stringstream ss;
        ss << "-sensor=" << config.target_data_type << " ";
        if ( config.has_name) {
           ss << "-name=" << config.name << " ";
        }
        if ( config.has_rigid_body) {
           ss << "-rigid_body=" << config.rigid_body << " ";
        }
        if ( config.has_hw_id) {
           ss << "-hw_id=" << to_string(config.hw_id) << " ";
        }
        ss << "stream_type=streaming ";

        if ( target_suid.high == 0 && target_suid.low == 0) {
           ss << " not found" << endl;
           throw runtime_error( ss.str());
        }

        cout << ss.str() << " found" << endl
             << "suid = [high " << hex << target_suid.high
             << ", low " << hex << target_suid.low << dec
             << "] :: " << config.target_data_type << endl;

        uint64_t cookie = 0;
        if ( config.use_island_log) {
           struct timeval tv;
           gettimeofday( &tv, NULL);
           cookie = (uint64_t)tv.tv_usec;

           send_island_log_trigger_req( cookie);
        }
        run();
        if ( config.use_island_log) {
           send_island_log_trigger_req( cookie);
        }
        delete connection;

    } catch (runtime_error& e) {
        cout << "run failed: " <<  e.what() << endl;
        rc = EXIT_FAILURE;
        passfail = "FAIL";
    }

    if ( sensors_diag_log_obj) {
       delete sensors_diag_log_obj;
       sensors_diag_log_obj = NULL;
    }

    cout << passfail << endl;
    return rc;
}

/* signal handler for graceful handling of Ctrl-C */
static void signal_handler(int signum)
{
   cout << "FAIL SIGINT received Abort" << endl;
   exit(signum);
}

/* conditionally send debug message to stdout */
static void debug_message( string message)
{
   if (config.debug) {
      static std::mutex mu;
      unique_lock<mutex> lk(mu); // serialize debug message
      cout << message << endl;
   }
}

/* parse command line arguments */
static void parse_arguments(int argc, char *argv[])
{
    string sensor_eq( "-sensor");
    string name_eq( "-name");
    string rigid_body_eq( "-rigid_body");
    string hw_id_eq( "-hw_id");
    string sample_rate_eq( "-sample_rate");
    string fixed_eq( "-fixed");
    string filter_eq( "-filter");
    string batch_period_eq( "-batch_period");
    string thresholds_eq( "-thresholds");
    string thresholds_type_eq( "-thresholds_type");
    string delay_eq( "-delay");
    string duration_eq( "-duration");
    string debug_eq( "-debug");
    string island_log_eq( "-island_log");

    for ( int i = 1; i < argc; i++) {
        string key;
        string value;
        if ( !get_key_value( argv[i], key, value)) {
           if ( 0 == strncmp( argv[i], "-help", 5)) {
              config.is_valid = false;
              break;
           }
           config.is_valid = false;
           cout << "unrecognized arg " << argv[i] << endl;
           continue;
        }
        if ( sensor_eq == key) {
           strlcpy( config.target_data_type, value.c_str(),
                    sizeof(config.target_data_type));
        }
        else if ( name_eq == key) {
           config.has_name = true;
           strlcpy( config.name, value.c_str(),sizeof(config.name));
        }
        else if ( rigid_body_eq == key) {
           config.has_rigid_body = true;
           strlcpy( config.rigid_body, value.c_str(),sizeof(config.rigid_body));
        }
        else if ( hw_id_eq == key) {
           config.has_hw_id = true;
           config.hw_id = atoi( value.c_str());
        }
        else if ( sample_rate_eq == key) {
           config.target_rate = atof( value.c_str());
        }
        else if ( fixed_eq == key) {
           config.use_resampler = true;
           config.rate_type = parse_rate_type( value);
        }
        else if ( filter_eq == key) {
           config.filter = parse_boolean(value.c_str());
        }
        else if ( batch_period_eq == key) {
           config.has_batch_period = true;
           config.batch_period = atof( value.c_str());
        }
        else if ( thresholds_eq == key) {
            stringstream csv( value);
            string element;
            while ( getline(csv, element, ',')) {
               config.thresholds.push_back(atof(element.c_str()));
            }
        }
        else if ( thresholds_type_eq == key) {
            if ( !parse_thresholds_type( value)) {
               config.is_valid = false;
            }
        }
        else if ( delay_eq == key) {
           config.has_delay= true;
           config.delay = atof( value.c_str());
        }
        else if ( duration_eq == key) {
           config.duration = atof( value.c_str());
        }
        else if ( debug_eq == key) {
           config.debug = parse_boolean(value.c_str());
        }
        else if ( island_log_eq == key) {
           config.use_island_log = parse_boolean(value.c_str());
        }
        else {
            config.is_valid = false;
            cout << "unrecognized arg " << argv[i] << endl;
        }
    }
    if ( !config.is_valid) {
        usage();
        exit(4);
    }
}

/**
 * @brief parse command line argument of the form keyword=value
 * @param parg[i] - one command line argument
 * @param key[io] - gets string to left of '='
 * @param value[io] - sets string to right of '='
 * @return true - successful parse. false - parse failed.
 */
static bool get_key_value(char *parg, string &key, string &value)
{
   char *pkey = parg;

   while ( char c = *parg) {
      if ( c == '=') {
         key = string( pkey, parg - pkey);
         value = string( ++parg);
         return true;
      }
      parg++;
   }
   return false;
}

/* parse argument 0 | off | 1 | on and return bool */
static bool parse_boolean(const char *parg)
{
    if (parg != nullptr) {
       if ( isdigit( *parg)) {
           return atoi( parg);
       }
       if ( 0 == strncmp( parg, "on",2)) {
           return true;
       }
    }
    return false;
}

/* parse input rate_type value from -fixed=<value> */
static sns_resampler_rate parse_rate_type( string value)
{
   if ( parse_boolean(value.c_str())) {
      return SNS_RESAMPLER_RATE_FIXED;
   }
   else {
      return SNS_RESAMPLER_RATE_MINIMUM;
   }
}

/* parse input thresholds_type from -threslholds_type=<value> */
static bool parse_thresholds_type( string value)
{
   if ( "relative" == value) {
      config.threshold_type =  SNS_THRESHOLD_TYPE_RELATIVE_VALUE;
   }
   else if ( "percent" == value) {
      config.threshold_type =  SNS_THRESHOLD_TYPE_RELATIVE_PERCENT;
   }
   else if ( "angle" == value) {
      config.threshold_type =  SNS_THRESHOLD_TYPE_ANGLE;
   }
   else if ( "absolute" == value) {
      config.threshold_type =  SNS_THRESHOLD_TYPE_ABSOLUTE;
   }
   else {
      cout << "bad thresholds_type value: " << value << endl;
      return false;
   }
   return true;
}

/* show usage */
static void usage(void)
{
    cout << "usage: ssc_threshold"
            " -sensor=<data_type> [-name=<name>]\n"
            "       [-rigid_body=<display | keyboard> | external>]"
            " [-hw_id=<number>]\n"
            "       -sample_rate=<hz> [-batch_period=<seconds>]\n"
            "       [-fixed=<boolean>] [-filter=<boolean>]\n"
            "       [-thresholds=<number>,...]"
            "       [-thresholds_type=relative | percent | absolute | angle]\n"
            "       [-island_log=<boolean>]\n"
            "       [-delay=<seconds>] -duration=<seconds>"
            " [-debug=<0 | 1 | 2>] [-help]\n";
}

/* get the suid for the input data_type */
static void get_suid( const std::string &data_type)
{
   suid_lookup lookup(suid_cb);

   get_suid_complete = false;
   lookup.request_suid(data_type);

   // wait for get_suid_complete with timeout
   auto now = std::chrono::system_clock::now();
   auto interval = 1000ms * 5; // 5 second timeout
   auto then = now + interval;

   unique_lock<mutex> lk(m);
   while ( !get_suid_complete) {
       if ( cv.wait_until(lk, then) == std::cv_status::timeout) {
          string detail = "timeout waiting for " + data_type
                          + " get_suid complete";
          throw runtime_error( detail);
       }
   }
   debug_message( "get_suid for " + data_type + " complete.");
}

/* suid message callback */
static void suid_cb(const std::string& datatype,
                    const std::vector<sensor_uid>& suids)
{
  string message( "suid_cb received event for " + datatype
                  + " count " + to_string( suids.size()));
  debug_message( message);

  for ( size_t i = 0; i < suids.size(); i++) {

     sensor_uid suid = suids.at(i);
     if ( config.debug) {
        cout << "received suid 0x" << hex << suid.high << suid.low << dec
             << " :: " << datatype << endl;
     }
     dtuid_pair element;
     element.data_type = datatype;
     element.suid.high = suid.high;
     element.suid.low = suid.low;
     pairs.push_back( element);

     if ("threshold" == datatype) {
        threshold_suid.high = suid.high;
        threshold_suid.low = suid.low;
     }
     else if ("resampler" == datatype) {
        resampler_suid.high = suid.high;
        resampler_suid.low = suid.low;
     }
     else if ("diag_sensor" == datatype) {
        diag_sensor_suid.high = suid.high;
        diag_sensor_suid.low = suid.low;
     }

     unique_lock<mutex> lk(m);
     get_suid_complete = true;
     cv.notify_one();
  }
}

/* lookup attributes from the input datatype/suid */
static void attributes_lookup( string &data_type, sensor_uid &suid)
{
    sns_client_request_msg req_message;
    sns_std_attr_req attr_req;
    string attr_req_encoded;
    attr_req.SerializeToString( &attr_req_encoded);

    req_message.set_msg_id( SNS_STD_MSGID_SNS_STD_ATTR_REQ);
    req_message.mutable_request()->set_payload( attr_req_encoded);
    req_message.mutable_suid()->set_suid_high( suid.high);
    req_message.mutable_suid()->set_suid_low( suid.low);
    req_message.mutable_susp_config()->set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);
    req_message.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);

    string req_message_encoded;
    req_message.SerializeToString(&req_message_encoded);

    attrib_lookup_complete = false;
    resp_from = data_type; // setup resp_cb
    connection->send_request( req_message_encoded, false);

    // wait for attribute lookup to complete
    auto now = std::chrono::system_clock::now();
    auto interval = 1000ms * 5; // 5 second timeout
    auto then = now + interval;

    unique_lock<mutex> lk(m);
    while ( !attrib_lookup_complete) {
        if ( cv.wait_until(lk, then) == std::cv_status::timeout) {
           string detail = "timeout waiting for " + data_type
                            + " attribute lookup";
           throw runtime_error( detail);
        }
    }
    debug_message( "attributes_lookup for " + data_type + " complete.");
}

/* event message callback*/
static void event_cb(const uint8_t *data, size_t size)
{
  sns_client_event_msg pb_event_msg;

  string message( "event_cb received message length " + to_string(size));
  debug_message( message);

  pb_event_msg.ParseFromArray(data, size);

  sns_std_suid suid;
  suid = pb_event_msg.suid();
  string from_data_type = lookup_data_type( suid);

  stringstream ss;
  ss << "event_cb received from suid 0x"
     << hex << suid.suid_high() << suid.suid_low() << endl
     << "event_cb rcvd aka suid_low: " << dec << suid.suid_low()
     << ", suid_high: " << suid.suid_high()
     << " :: " << from_data_type << endl;
  debug_message( ss.str());

  debug_message( pb_event_msg.DebugString());

  string encoded_event_msg((char *)data, size);
  log_event( from_data_type, encoded_event_msg);

  for(int i = 0; i < pb_event_msg.events_size(); i++)
  {
    const sns_client_event_msg_sns_client_event &pb_event = pb_event_msg.events(i);

    auto msg_id = pb_event.msg_id();
    if ( msg_id == SNS_STD_MSGID_SNS_STD_ERROR_EVENT) {
       sns_std_error_event error;
       error.ParseFromString(pb_event.payload());
       string diagnostic( "received error event " + to_string(error.error()));
       debug_message( diagnostic);
    }
    else if ( msg_id == SNS_STD_MSGID_SNS_STD_ATTR_EVENT) {
       debug_message( "event_cb " + from_data_type + " attribute event");

       sns_std_attr_event pb_attr_event;
       pb_attr_event.ParseFromString(pb_event.payload());
       if ( do_attributes_match_target( pb_attr_event)) {
          target_suid.high = suid.suid_high();
          target_suid.low = suid.suid_low();
          found_target = true;
       }

       unique_lock<mutex> lk(m);   // attribute found
       attrib_lookup_complete = true;
       cv.notify_one();
    }
    else if ( msg_id == SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_EVENT) {
       show_sensor_event( pb_event);
    }
    else if ( msg_id == SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT) {
       show_physical_config_event( pb_event);
    }
    else if ( msg_id == SNS_STD_MSGID_SNS_STD_FLUSH_EVENT) {
       debug_message( "event_cb received flush event");
    }
    else if ( msg_id == SNS_RESAMPLER_MSGID_SNS_RESAMPLER_CONFIG_EVENT) {
       show_resampler_config_event( pb_event);
    }
    else if ( msg_id == SNS_THRESHOLD_MSGID_SNS_THRESHOLD_CONFIG) {
       debug_message( "event_cb received threshold_config event");
    }
    else if ( msg_id == SNS_CAL_MSGID_SNS_CAL_EVENT) {
       debug_message( "event_cb received sns_cal_event");
    }
    else {
       string message("event_cb received unknown msg_id " + to_string(msg_id));
       debug_message( message);
    }
  }
  debug_message( "event_cb exit");
}

/* response message callback does HLOS log response message */
static void resp_cb(uint32_t resp_value)
{
    if(sensors_diag_log_obj != NULL) { /* qxdm log response msg */
       sensors_diag_log_obj->log_response_msg( resp_value,
                                               resp_from,
                                               logging_module_name);
    }
}

/* HLOS log event msg */
static void log_event( string from_data_type, string encoded_event_msg)
{
   if(sensors_diag_log_obj != NULL) {
      sensors_diag_log_obj->log_event_msg( encoded_event_msg,
                                           from_data_type,
                                           logging_module_name);
   }
}

/* return the data_type for the input suid */
static string lookup_data_type( sns_std_suid &suid)
{
   for ( size_t i = 0; i < pairs.size(); i++) {
      if ( pairs[i].suid.high == suid.suid_high() &&
           pairs[i].suid.low == suid.suid_low())
         return pairs[i].data_type;
   }

   return string("unknown");
}

/* remove leading and trailing whitespace */
static string trim(const string& str, const string& whitespace = " \t")
{
    const auto str_begin = str.find_first_not_of(whitespace);
    if (str_begin == string::npos)
        return ""; // all blanks

    const auto str_end = str.find_last_not_of(whitespace);
    const auto str_len = str_end - str_begin;

    return str.substr(str_begin, str_len);
}

/**
 * return true when attributes match the config.target_data_type, config.name,
 * config_rigid_body, config_hw_id, and stream_type streaming.
 *
 * @param pb_attr_event
 *
 * @return bool
 */
static bool do_attributes_match_target( sns_std_attr_event pb_attr_event)
{
   bool match = true;

   for (int i = 0; i < pb_attr_event.attributes_size(); i++) {
      int32_t attr_id = pb_attr_event.attributes(i).attr_id();
      const sns_std_attr_value& attr_value = pb_attr_event.attributes(i).value();

      if ( attr_id == SNS_STD_SENSOR_ATTRID_NAME && config.has_name) {
         string s = trim(attr_value.values(0).str());
         if ( 0 != strcmp( config.name, s.c_str())) {
            match = false;
         }
      }
      else if (attr_id == SNS_STD_SENSOR_ATTRID_TYPE) {
         string s = trim(attr_value.values(0).str());
         if ( 0 != strcmp( config.target_data_type, s.c_str())) {
            match = false;
         }
      }
      else if (attr_id == SNS_STD_SENSOR_ATTRID_STREAM_TYPE) {
         if ( attr_value.values(0).sint()
              != SNS_STD_SENSOR_STREAM_TYPE_STREAMING) {
            match = false;
         }
      }
      else if (attr_id == SNS_STD_SENSOR_ATTRID_HW_ID && config.has_hw_id) {
         if ( attr_value.values(0).sint() != config.hw_id) {
            match = false;
         }
      }
      else if (attr_id == SNS_STD_SENSOR_ATTRID_RIGID_BODY
               && config.has_rigid_body) {
         int v = attr_value.values(0).sint();
         if ( v == SNS_STD_SENSOR_RIGID_BODY_TYPE_DISPLAY) {
            if ( 0 != strcmp( config.rigid_body, "display")) {
               match = false;
            }
         }
         else if ( v == SNS_STD_SENSOR_RIGID_BODY_TYPE_KEYBOARD) {
            if ( 0 != strcmp( config.rigid_body, "keyboard")) {
               match = false;
            }
         }
         else if ( v == SNS_STD_SENSOR_RIGID_BODY_TYPE_EXTERNAL) {
            if ( 0 != strcmp( config.rigid_body, "external")) {
               match = false;
            }
         }
      }
   } // for

   return match;
}

/* show SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_EVENT payload*/
static void show_sensor_event(
               const sns_client_event_msg_sns_client_event &pb_event)
{
   static int event_count = 0;

   sns_std_sensor_event sensor_event;
   sensor_event.ParseFromString( pb_event.payload());
   int data_size = sensor_event.data_size();
   string s = to_string( event_count++)
              + " " + config.target_data_type
              + ": ts = " + to_string(pb_event.timestamp()) + "; ";

   s += "a = [";
   for ( int i = 0; i < data_size; i++) {
      if (i > 0) {
         s += ", ";
      }
      s += to_string(sensor_event.data(i));
   }
   s += "] ";

   s += to_string(sensor_event.status());
   cout << s.c_str() << endl;
}

/* show SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT payload */
static void show_physical_config_event(
               const sns_client_event_msg_sns_client_event &pb_event)
{
   sns_std_sensor_physical_config_event physical_config_event;
   physical_config_event.ParseFromString(pb_event.payload());

   cout << "event_cb received physical_config_event:" << endl;
   cout << " sample_rate: ";
   cout << physical_config_event.sample_rate() << endl;

   if ( physical_config_event.has_water_mark()) {
       cout << " watermark: ";
       cout << physical_config_event.water_mark() << endl;
   }
   if ( physical_config_event.range_size()) {
       cout << " range: ";
       for ( int i = 0; i < physical_config_event.range_size(); i++) {
           if ( i) { cout << ", ";}
           cout << physical_config_event.range(i);
       }
       cout << endl;
   }
   if ( physical_config_event.has_resolution()) {
       cout << " resolution: ";
       cout << physical_config_event.resolution() << endl;
   }
   if ( physical_config_event.has_operation_mode()) {
      cout << " operation_mode: ";
       // string operation_mode includes a trailing 0x0
       // using op_mode.c_str() eliminates the trailing null
       string op_mode = physical_config_event.operation_mode();
       cout << op_mode.c_str() << endl;
   }
   if ( physical_config_event.has_active_current()) {
       cout << " active_current: ";
       cout << physical_config_event.active_current() << endl;
   }
   if ( physical_config_event.has_stream_is_synchronous()) {
       cout << " stream_is_synchronous: ";
       cout << physical_config_event.stream_is_synchronous() << endl;
   }
   if ( physical_config_event.has_dri_enabled()) {
       cout << " dri_enabled: ";
       cout << physical_config_event.dri_enabled() << endl;
   }
   if ( physical_config_event.has_dae_watermark()) {
       cout << " DAE_watermark: ";
       cout << physical_config_event.dae_watermark() << endl;
   }
}

/* show SNS_RESAMPLER_MSGID_SNS_RESAMPLER_CONFIG_EVENT payload */
static void show_resampler_config_event(
               const sns_client_event_msg_sns_client_event &pb_event)
{
   debug_message( "event_cb received resampler_config event");

    sns_resampler_config_event resampler_config_event;
    resampler_config_event.ParseFromString( pb_event.payload());
    string s  = " ";
           s += config.target_data_type;
           s += ": ts = " + to_string(pb_event.timestamp()) + "; ";

    sns_resampler_quality quality = resampler_config_event.quality();
    if (quality == SNS_RESAMPLER_QUALITY_CURRENT_SAMPLE) {
        s += " SAMPLE";
    }
    else if (quality == SNS_RESAMPLER_QUALITY_FILTERED) {
        s += " FILTERED";
    }
    else if (quality == SNS_RESAMPLER_QUALITY_INTERPOLATED_FILTERED) {
        s += " INTERPOLATED_FILTERED";
    }
    else if (quality == SNS_RESAMPLER_QUALITY_INTERPOLATED) {
        s += " INTERPOLATED";
    }
    else {
        s += "quality unknown";
    }

    cout << s.c_str() << endl;
}

/* run the threshold test */
static void run()
{
    sensor_uid *feed_suid;
    uint32_t feed_msgid;
    string feed_payload;

    if ( config.use_resampler) {
       // resampler
       debug_message("using sns_resampler_config");

       sns_resampler_config resampler_config;
       resampler_config.mutable_sensor_uid()->set_suid_high(target_suid.high);
       resampler_config.mutable_sensor_uid()->set_suid_low(target_suid.low);
       resampler_config.set_resampled_rate(config.target_rate); // hz
       resampler_config.set_rate_type(config.rate_type);
       resampler_config.set_filter(config.filter);

       string resampler_config_encoded;
       resampler_config.SerializeToString(&resampler_config_encoded);

       feed_suid = &resampler_suid;
       feed_msgid = SNS_RESAMPLER_MSGID_SNS_RESAMPLER_CONFIG;
       feed_payload = resampler_config_encoded;
    }
    else {
       // std_sensor_config
       debug_message("using sns_std_sensor_config");
       sns_std_sensor_config std_sensor_config;
       std_sensor_config.set_sample_rate(config.target_rate);

       string std_sensor_config_encoded;
       std_sensor_config.SerializeToString(&std_sensor_config_encoded);

       feed_suid = &target_suid;
       feed_msgid = SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG;
       feed_payload = std_sensor_config_encoded;
    }

    // threshold
    sns_threshold_config threshold_config;
    threshold_config.mutable_sensor_uid()->set_suid_high(feed_suid->high);
    threshold_config.mutable_sensor_uid()->set_suid_low(feed_suid->low);
    for (size_t i = 0; i < config.thresholds.size(); i++) {
       threshold_config.add_threshold_val( config.thresholds[i]);
    }
    threshold_config.set_threshold_type(config.threshold_type);
    threshold_config.set_payload_cfg_msg_id(feed_msgid);
    threshold_config.set_payload( feed_payload);

    string threshold_config_encoded;
    threshold_config.SerializeToString(&threshold_config_encoded);

    // client request
    sns_client_request_msg client_request_msg;
    client_request_msg.mutable_suid()->set_suid_high( threshold_suid.high);
    client_request_msg.mutable_suid()->set_suid_low( threshold_suid.low);
    client_request_msg.mutable_susp_config()->set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);
    client_request_msg.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
    client_request_msg.set_msg_id( SNS_THRESHOLD_MSGID_SNS_THRESHOLD_CONFIG);

    sns_std_request *preq = client_request_msg.mutable_request();
    preq->mutable_batching()->set_batch_period( 0);
    if ( config.batch_period >= 0) { // if supplied convert to microseconds
        uint32_t batch_period_us = ( uint32_t)( config.batch_period * 1000000);
        preq->mutable_batching()->set_batch_period( batch_period_us);
    }
    preq->set_payload( threshold_config_encoded);

    debug_message("start threshold sensor");

    string client_request_msg_encoded;
    client_request_msg.SerializeToString(&client_request_msg_encoded);

    if(sensors_diag_log_obj != NULL) { /* qxdm log request msg */
       sensors_diag_log_obj->log_request_msg( client_request_msg_encoded,
                                              lit_threshold,
                                              logging_module_name);
    }

    resp_from = lit_threshold;
    connection->send_request( client_request_msg_encoded, false);

    useconds_t usec = (int)( config.duration * 1000000);
    string message( "sleep " + to_string(config.duration) + " seconds");
    debug_message( message);
    usleep( usec);

    send_disable_req();
    usleep( 1000000); /* allow one second for logging */

    debug_message("stop threshold sensor");
}

/* stop the threshold sensor stream */
static void send_disable_req()
{
    debug_message( "send_disable_request");

    sns_std_request std_req;
    string std_req_encoded;
    std_req.SerializeToString( &std_req_encoded);

    /* populate the client request message */
    sns_client_request_msg req_message;
    req_message.set_msg_id( SNS_CLIENT_MSGID_SNS_CLIENT_DISABLE_REQ);
    req_message.mutable_request()->set_payload( std_req_encoded);
    req_message.mutable_suid()->set_suid_high( threshold_suid.high);
    req_message.mutable_suid()->set_suid_low( threshold_suid.low);
    req_message.mutable_susp_config()->set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);
    req_message.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);

    string req_message_encoded;
    req_message.SerializeToString(&req_message_encoded);

    if(sensors_diag_log_obj != NULL) { /* qxdm log request msg*/
       sensors_diag_log_obj->log_request_msg( req_message_encoded,
                                              lit_threshold,
                                              logging_module_name);
    }
    resp_from = lit_threshold;  // setup resp_cb()
    connection->send_request( req_message_encoded, false);
    debug_message( "sent");
}

static void send_island_log_trigger_req(uint64_t cookie)
{
    debug_message( "send island log trigger req");
    cout << "send island log trigger req cookie " << cookie;

    sns_diag_log_trigger_req trigger_req;
    trigger_req.set_cookie( cookie);
    trigger_req.set_log_type( SNS_DIAG_TRIGGERED_LOG_TYPE_ISLAND_LOG);

   string trigger_req_encoded;
   trigger_req.SerializeToString( &trigger_req_encoded);

    sns_std_request std_request;
    std_request.set_payload( trigger_req_encoded);

    string std_request_encoded;
    std_request.SerializeToString( &std_request_encoded);

    /* populate the client request message */
    sns_client_request_msg req_message;
    req_message.set_msg_id( SNS_DIAG_SENSOR_MSGID_SNS_DIAG_LOG_TRIGGER_REQ);
    req_message.mutable_request()->set_payload( std_request_encoded);
    req_message.mutable_suid()->set_suid_high( diag_sensor_suid.high);
    req_message.mutable_suid()->set_suid_low( diag_sensor_suid.low);
    req_message.mutable_susp_config()->set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);
    req_message.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);

    string req_message_encoded;
    req_message.SerializeToString( &req_message_encoded);

    if(sensors_diag_log_obj != NULL) { /* qxdm log request msg*/
       sensors_diag_log_obj->log_request_msg( req_message_encoded,
                                              lit_diag_sensor,
                                              logging_module_name);
    }
    resp_from = lit_diag_sensor;  // setup resp_cb()
    connection->send_request( req_message_encoded, false);
    debug_message( "sent");
    cout <<  " complete." << endl;
}

