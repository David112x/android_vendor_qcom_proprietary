/* ===================================================================
** Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: see_workhorse.cpp
** DESC: stream physical or virtual sensor @ rate for duration
** ================================================================ */
#include <iostream>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <time.h>
#include <unistd.h>

#ifdef SNS_LE_QCS605
#include <string.h>
#include <sys/time.h>
#endif
#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#endif

#include "see_salt.h"

using namespace std;

/* forward declarations */
void parse_sample_rate( string value);
void parse_wakeup( string value);
bool parse_processor( string value);

sensor *choose_target_sensor( see_salt *psalt, vector<sens_uid *> &sens_uids);
void display_target_sensor( sensor *psensor);
bool resolve_symbolic_sample_rate( sensor *psensor);
salt_rc run( see_salt *psalt, sensor *psensor,
             sens_uid *cal_suid, string cal_sensor);

/* data */
static mutex mtx;              // SNS-660
static condition_variable cv;  // SNS-660

void sleep_and_awake( uint32_t milliseconds);

static char *my_exe_name;

#define SYMBOLIC_MIN_ODR   1
#define SYMBOLIC_MAX_ODR   2

// SENSORTEST-479 automatically re-arm the ccd_walk single output sensor
struct re_arm {
    bool now = false; // true == rearm requested
    sens_uid *suid;
    string target_sensor = "ccd_walk";
    string trigger_event = ": 773,";                // "msg_id" : 773,
    re_arm() {}
};

static re_arm rearm;

struct app_config {
    bool  is_valid = true;
    char  data_type[ 32];           // type: accel, gyro, mag, ...

    bool  has_name = false;
    char  name[ 32];

    bool  has_rigid_body = false;
    char  rigid_body[ 32];

    bool  has_hw_id = false;
    int   hw_id;

    bool  has_on_change = false;
    bool  on_change;

    bool  has_batch_period = false;
    float batch_period;             // seconds

    bool  has_flush_period = false;
    float flush_period;             // seconds

    bool  has_flush_only = false;
    bool  flush_only;

    bool  has_max_batch = false;
    bool  max_batch;

    bool  has_is_passive = false;
    bool  is_passive;

    bool  has_debug = false;
    bool  debug;

    bool  has_island_log = false;
    bool  island_log;

    bool  has_memory_log = false;
    bool  memory_log;

    bool  has_usta_log = false;
    bool  usta_log;

    bool  has_calibrated = false;
    bool  calibrated;

    bool  has_flush_request_interval = false;
    float flush_request_interval;

    bool  has_distance_bound = false;
    float distance_bound = 1.0;     // meters

    bool  has_processor = false;
    see_std_client_processor_e processor = SEE_STD_CLIENT_PROCESSOR_APSS;

    bool  has_wakeup = false;
    see_client_delivery_e wakeup = SEE_CLIENT_DELIVERY_WAKEUP;

    bool  has_delay = false;
    float delay;

    bool  display_events = false;

    bool  rearm = false;

    int  times = 1;                 // number of times

    int symbolic_sample_rate = 0;
    float sample_rate = 10.0;       // hz

    float duration = 10.0;          // seconds

    app_config() {}
};

static app_config config;

void init_see_std_request( see_std_request *pstd_request)
{
   if ( config.has_batch_period) {
      cout << "+ batch_period: " << to_string( config.batch_period);
      cout << " seconds" << endl;
      pstd_request->set_batch_period( config.batch_period * 1000000); // microsec
   }
   if ( config.has_flush_period) {
      cout << "+ flush_period: " << to_string( config.flush_period);
      cout << " seconds" << endl;
      pstd_request->set_flush_period( config.flush_period * 1000000); // microsec
   }
   if ( config.has_flush_only) {
      cout << "+ flush_only: " << to_string( config.flush_only) << endl;
      pstd_request->set_flush_only( config.flush_only);
   }
   if ( config.has_max_batch) {
      cout << "+ max_batch: " << to_string( config.max_batch) << endl;
      pstd_request->set_max_batch( config.max_batch);
   }
   if ( config.has_is_passive) {
      cout << "+ is_passive: " << to_string( config.is_passive) << endl;
      pstd_request->set_is_passive( config.is_passive);
   }
}

void init_processor_wakeup( see_client_request_message &request)
{
   if (config.has_processor) {
      request.set_processor( config.processor);
      cout << "+ processor: "
           << request.get_client_symbolic( config.processor) << endl;
   }
   if (config.has_wakeup) {
      request.set_wakeup( config.wakeup);
      cout << "+ wakeup: "
           << request.get_wakeup_symbolic( config.wakeup) << endl;
   }
}

salt_rc disable_sensor(see_salt *psalt, sens_uid *suid, string target)
{
   cout << "disable_sensor( " << target << ")" << endl;
   see_std_request std_request;
   see_client_request_message request( suid,
                                       MSGID_DISABLE_REQ,
                                       &std_request);
   init_processor_wakeup(request);
   salt_rc rc = psalt->send_request(request);
   cout << "disable_sensor() complete rc " << to_string(rc) << endl;
   return rc;
}

salt_rc flush_sensor(see_salt *psalt, sens_uid *suid, string target)
{
   cout << "flush_sensor( " << target << ")" << endl;
   see_std_request std_request;
   see_client_request_message request( suid,
                                       MSGID_FLUSH_REQ,
                                       &std_request);
   init_processor_wakeup(request);
   salt_rc rc = psalt->send_request(request);
   cout << "flush_sensor() complete rc " << to_string(rc) << endl;
   return rc;
}

/**
 * does_rigid_body_match() is intended to find the mag_cal that matches
 * the rigid_body of selected mag sensor, or the gyro_cal matching the
 * selected gyro sensor.
 *
 * @param psensor
 *
 * @return true when the _cal sensor matches config.rigid_body
 */
bool does_rigid_body_match(sensor *psensor)
{
   if ( psensor && psensor->has_rigid_body()) {
      string rigid_body = psensor->get_rigid_body();
      if ( 0 != strncmp( config.rigid_body, rigid_body.c_str(),
                         rigid_body.length())) {
         return false;
      }
   }
   return true;
}

// return pointer to target_sensor's suid
sens_uid *get_sensor_suid( see_salt *psalt, string target)
{
    cout << "+ get_sensor_suid( " << target << ")" << endl;
    vector<sens_uid *> sens_uids;
    psalt->get_sensors( target, sens_uids);
    if ( sens_uids.size() == 0) {
       cout << "fatal suid not found" << endl;
       exit( 4);
    }
    size_t index = 0;
    if (target.find( "_cal") != string::npos) {
       for ( ; index < sens_uids.size(); index++) {
          sens_uid *candidate_suid = sens_uids[index];
          sensor *psensor = psalt->get_sensor( candidate_suid);
          if ( !does_rigid_body_match( psensor)) {
             continue;
          }
          cout << "+ found " << target << " " << psensor->get_rigid_body() << endl;
          break;
       }
    }
    index = ( index < sens_uids.size()) ? index : sens_uids.size() - 1;
    cout << "+ " << target;
    cout << " suid = [high " << hex << sens_uids[index]->high;
    cout << ", low " << hex << sens_uids[index]->low << "]" << endl;

    return sens_uids[index];
}

salt_rc trigger_diag_sensor_log( see_salt *psalt,
                                 see_diag_trigger_log_type log_type,
                                  uint64_t cookie)
{
   cout << "trigger_diag_sensor_log( " << log_type << ")";
   cout << " cookie: " << dec << cookie << endl;
   if ( log_type == SEE_LOG_TYPE_MEMORY_USAGE_LOG) {
      psalt->sleep(2);     // hoping ssc goes idle, as per Dilip email
   }

   string lit_diag_sensor("diag_sensor");
   sens_uid *suid = get_sensor_suid( psalt, lit_diag_sensor);
   see_diag_log_trigger_req trigger( log_type, cookie);
   see_std_request std_request;
   std_request.set_payload( &trigger);
   see_client_request_message request( suid,
                                       MSGID_DIAG_SENSOR_TRIGGER_REQ,
                                       &std_request);
   init_processor_wakeup(request);
   salt_rc rc = psalt->send_request(request);
   cout << "trigger_diag_sensor_log() complete rc " << to_string(rc) << endl;
   return rc;
}

salt_rc on_change_sensor(see_salt *psalt, sens_uid *suid, string target)
{
   cout << "on_change_sensor( " << target << ")" << endl;

   see_std_request std_request;
   init_see_std_request( &std_request);

   see_client_request_message request( suid,
                                       MSGID_ON_CHANGE_CONFIG,
                                       &std_request);
   init_processor_wakeup(request);
   salt_rc rc = psalt->send_request(request);
   cout << "on_change_sensor() complete rc " << to_string(rc) << endl;
   return rc;
}

salt_rc distance_bound_sensor( see_salt *psalt, sens_uid *suid)
{
   cout << "distance_bound_sensor()" << endl;
   see_std_request std_request;
   init_see_std_request( &std_request);

   see_set_distance_bound distance_bound( config.distance_bound); // meters
   std_request.set_payload( &distance_bound);
   see_client_request_message request( suid,
                                       MSGID_SET_DISTANCE_BOUND,
                                       &std_request);
   init_processor_wakeup(request);
   salt_rc rc = psalt->send_request(request);
   cout << "distance_bound_sensor() complete rc " << to_string(rc) << endl;
   return rc;
}

salt_rc basic_gestures_sensor( see_salt *psalt, sens_uid *suid)
{
   cout << "basic_gestures_sensor()" << endl;
   see_std_request std_request;
   init_see_std_request( &std_request);

   see_basic_gestures_config config;
   std_request.set_payload( &config);
   see_client_request_message request( suid,
                                       MSGID_BASIC_GESTURES_CONFIG,
                                       &std_request);
   init_processor_wakeup(request);
   salt_rc rc = psalt->send_request(request);
   cout << "basic_gestures_sensor() complete rc " << to_string(rc) << endl;
   return rc;
}

salt_rc multishake_sensor( see_salt *psalt, sens_uid *suid)
{
   cout << "multishake_sensor()" << endl;
   see_std_request std_request;
   init_see_std_request( &std_request);

   see_multishake_config config;
   std_request.set_payload( &config);
   see_client_request_message request( suid,
                                       MSGID_MULTISHAKE_CONFIG,
                                       &std_request);
   init_processor_wakeup(request);
   salt_rc rc = psalt->send_request(request);
   cout << "multishake_sensor() complete rc " << to_string(rc) << endl;
   return rc;
}

salt_rc psmd_sensor( see_salt *psalt, sens_uid *suid)
{
   cout << "psmd_sensor()" << endl;
   see_std_request std_request;
   init_see_std_request( &std_request);

   see_psmd_config config( SEE_PSMD_TYPE_STATIONARY);
   std_request.set_payload( &config);
   see_client_request_message request( suid,
                                       MSGID_PSMD_CONFIG,
                                       &std_request);
   init_processor_wakeup(request);
   salt_rc rc = psalt->send_request(request);
   cout << "psmd_sensor() complete rc " << to_string(rc) << endl;
   return rc;
}

salt_rc stream_sensor( see_salt *psalt, see_msg_id_e msg_id,
                       sens_uid *suid, string target)
{
   cout << "stream_sensor( " << target << ")" << endl;
   see_std_request std_request;
   init_see_std_request( &std_request);

   cout << "+ sample_rate: " << to_string( config.sample_rate);
   cout << " hz" << endl;
   see_std_sensor_config sample_rate( config.sample_rate); // hz
   std_request.set_payload( &sample_rate);
   see_client_request_message request( suid, msg_id, &std_request);
   init_processor_wakeup(request);
   salt_rc rc = psalt->send_request(request);
   cout << "config_stream_sensor() complete rc " << to_string(rc) << endl;
   return rc;
}

void usage( void)
{
    cout << "usage: see_workhorse"
            " -sensor=<sensor_type>"
            " [-name=<name>] [-on_change=<0 | 1>]\n"
            "       [-rigid_body=<display | keyboard>] [-hw_id=<number>]\n"
            "       [-delay=<seconds>] [-is_passive=<0 | 1>]\n"
            "       [-sample_rate=<min | max | digits>]\n"
            "       [-batch_period=<seconds>] [-flush_period=<seconds>]"
            " [-max_batch=<0 | 1>] [-flush_only=<0 | 1>]\n"
            "       [-flush_request_interval=<seconds>] [-rearm=<0 | 1>]\n"
            "       [-usta_log=<0 | 1>] [-memory_log=<0 | 1>]"
            " [-island_log=<0 | 1>]\n"
            "       [-calibrated=<0 | 1>] [-distance_bound=<meters>]\n"
            "       [-wakeup=<0 | 1>]"
            " [-processor=<ssc | apss | adsp | mdsp | cdsp>]"
            " [-display_events=<0 | 1>\n"
            "       -duration=<seconds> [-times=<number>] [-debug=<0 | 1>]"
            " [-help]\n"
            "       where: <sensor_type> :: accel | gyro | ...\n";
}

/**
 * @brief parse command line argument of the form keyword=value
 * @param parg[i] - command line argument
 * @param key[io] - gets string to left of '='
 * @param value[io] - sets string to right of '='
 */
bool get_key_value(char *parg, string &key, string &value)
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

void parse_arguments(int argc, char *argv[])
{
    // command line args:
    string sensor_eq( "-sensor");
    string name_eq( "-name");
    string rigid_body_eq( "-rigid_body");
    string on_change_eq( "-on_change");
    string hw_id_eq( "-hw_id");
    string is_passive_eq( "-is_passive");
    string sample_rate_eq( "-sample_rate");
    string batch_period_eq( "-batch_period");
    string flush_request_interval_eq( "-flush_request_interval");
    string flush_period_eq( "-flush_period");
    string flush_only_eq( "-flush_only");
    string max_batch_eq( "-max_batch");
    string island_log_eq( "-island_log");
    string memory_log_eq( "-memory_log");
    string usta_log_eq( "-usta_log");
    string duration_eq( "-duration");
    string debug_eq( "-debug");
    string calibrated_eq( "-calibrated");
    string distance_bound_eq( "-distance_bound");
    string wakeup_eq( "-wakeup");
    string processor_eq( "-processor");
    string display_events_eq( "-display_events");
    string times_eq( "-times");
    string delay_eq( "-delay");
    string rearm_eq( "-rearm");

    for ( int i = 0; i < argc; i++) {
       cout << argv[i] << " ";
    }
    cout << endl;

    my_exe_name = argv[0];
    for ( int i = 1; i < argc; i++) {
        string key;
        string value;
        if ( get_key_value( argv[i], key, value)) {
           if ( sensor_eq == key) {
              strlcpy( config.data_type, value.c_str(),
                       sizeof( config.data_type));
           }
           else if ( name_eq == key) {
              config.has_name = true;
              strlcpy( config.name, value.c_str(), sizeof(config.name));
           }
           else if ( rigid_body_eq == key) {
              config.has_rigid_body = true;
              strlcpy( config.rigid_body, value.c_str(),
                       sizeof(config.rigid_body));
           }
           else if ( on_change_eq == key) {
              config.has_on_change = true;
              config.on_change = atoi( value.c_str());
           }
           else if ( hw_id_eq == key) {
              config.has_hw_id = true;
              config.hw_id = atoi( value.c_str());
           }
           else if ( is_passive_eq == key) {
              config.has_is_passive = true;
              config.is_passive = atoi( value.c_str());
           }
           else if ( max_batch_eq == key) {
              config.has_max_batch = true;
              config.max_batch = atoi( value.c_str());
           }
           else if ( flush_request_interval_eq == key) {
              config.has_flush_request_interval = true;
              config.flush_request_interval = atof( value.c_str());
           }
           else if ( sample_rate_eq == key) {
              parse_sample_rate( value);
           }
           else if ( batch_period_eq == key) {
              config.has_batch_period = true;
              config.batch_period = atof( value.c_str());
           }
           else if ( flush_period_eq == key) {
              config.has_flush_period = true;
              config.flush_period = atof( value.c_str());
           }
           else if ( flush_only_eq == key) {
              config.has_flush_only = true;
              config.flush_only = atoi( value.c_str());
           }
           else if ( duration_eq == key) {
              config.duration = atof( value.c_str());
           }
           else if ( debug_eq == key) {
              config.has_debug = true;
              config.debug = atoi( value.c_str());
           }
           else if ( island_log_eq == key) {
              config.has_island_log = true;
              config.island_log = atoi( value.c_str());
           }
           else if ( memory_log_eq == key) {
              config.has_memory_log = true;
              config.memory_log = atoi( value.c_str());
           }
           else if ( usta_log_eq == key) {
              config.has_usta_log = true;
              config.usta_log = atoi( value.c_str());
           }
           else if ( calibrated_eq == key) {
              config.has_calibrated = true;
              config.calibrated = atoi( value.c_str());
           }
           else if ( distance_bound_eq == key) {
              config.has_distance_bound = true;
              config.distance_bound = atof( value.c_str());
           }
           else if ( times_eq == key) {
              config.times = atoi( value.c_str());
           }
           else if ( delay_eq == key) {
              config.has_delay = true;
              config.delay = atof( value.c_str());
           }
           else if ( rearm_eq == key) {
              config.rearm = atoi( value.c_str());
           }
           else if ( display_events_eq == key) {
              config.display_events = atoi( value.c_str());
           }
           else if ( wakeup_eq == key) {
              parse_wakeup( value);
           }
           else if ( processor_eq == key) {
              config.is_valid = parse_processor( value);
           }
           else {
               config.is_valid = false;
               cout << "unrecognized arg " << argv[i] << endl;
           }
        }
        else if ( 0 == strncmp( argv[i], "-h", 2)) {
           config.is_valid = false;
        }
        else {
            config.is_valid = false;
            cout << "FAIL unrecognized arg " << argv[i] << endl;
        }
    }
    if ( !config.is_valid) {
       usage();
       exit( 4);
    }
}

/* parse input from -sample_rate=<min | max | number> */
void parse_sample_rate( string value)
{
   if ( value.compare( "min") == 0) {
      config.symbolic_sample_rate = SYMBOLIC_MIN_ODR;
   }
   else if ( value.compare( "max") == 0) {
      config.symbolic_sample_rate = SYMBOLIC_MAX_ODR;
   }
   else {
      config.sample_rate = atof( value.c_str());
   }
}

/* parse input value from -wakeup=<value> */
void parse_wakeup( string value)
{
   config.has_wakeup = true;
   config.wakeup = SEE_CLIENT_DELIVERY_WAKEUP;
   if ( 0 == atoi( value.c_str()) ) {
      config.wakeup = SEE_CLIENT_DELIVERY_NO_WAKEUP;
   }
}

/* parse input value from -processor=<value> */
bool parse_processor( string value)
{
   bool rc = true;
   config.has_processor = true;
   if ( value == string("ssc")) {
      config.processor =  SEE_STD_CLIENT_PROCESSOR_SSC;
   }
   else if ( value == string("apss")) {
      config.processor =  SEE_STD_CLIENT_PROCESSOR_APSS;
   }
   else if ( value == string("adsp")) {
      config.processor =  SEE_STD_CLIENT_PROCESSOR_ADSP;
   }
   else if ( value == string("mdsp")) {
      config.processor =  SEE_STD_CLIENT_PROCESSOR_MDSP;
   }
   else if ( value == string("cdsp")) {
      config.processor =  SEE_STD_CLIENT_PROCESSOR_CDSP;
   }
   else {
      rc = false;
      cout << "unrecognized processor " << value << endl;
   }
   return rc;
}

void get_clock_mono( struct timespec &start)
{
    clock_gettime(CLOCK_MONOTONIC, &start);
}

void display_elapse(struct timespec &start,
                    struct timespec &end,
                    string &eyewitness)
{
    long mtime, seconds, nseconds;
    seconds  = end.tv_sec  - start.tv_sec;
    nseconds = end.tv_nsec - start.tv_nsec;
    mtime = ((seconds) * 1000 + nseconds/1000000.0);

    cout << eyewitness << dec << mtime << " milliseconds" << endl;
}
/**
 * event_cb function as registered with usta.
 * When a ccd_walk_event is presented, request ccd_walk rearming
 *
 * @param events
 * @param is_registry_sensor
 */
void event_cb( string events, bool is_registry_sensor)
{
   if (config.display_events) {
      cout << events << endl;
   }
   size_t found = events.find(rearm.trigger_event);

   // if ccd_walk_event and still rearming
   if ( found != string::npos && config.rearm) {
      unique_lock<mutex> lk(mtx);
      rearm.now = true;
      cv.notify_one();
   }
}

/**
 * Monitor the rearm.now variable for the duration of the test.
 * When notified and rearm.now is set, call on_change_sensor to rearm ccd_walk
 *
 * @param psalt
 * @param duration - seconds
 */
void monitor_and_rearm( see_salt *psalt, double duration)
{
   cout << "sleep_and_rearm( " << duration << ") seconds" << endl;

   /* compute when to timeout as now + test duration */
   auto now = std::chrono::system_clock::now();
   auto interval = 1000ms * duration;
   auto then = now + interval;

   unique_lock<mutex> lk(mtx);
   while ( true) {
       if ( cv.wait_until(lk, then) == std::cv_status::timeout) {
           break;
       }

       if ( rearm.now) {
          rearm.now = false;
          if (config.rearm) {
             on_change_sensor( psalt, rearm.suid, rearm.target_sensor);
          }
       }
   }
}

/**
 * @desc do_sleep - sleep_and_awake() can awake suspended apps processor
 * @param duration - seconds
 */
void do_sleep( float duration)
{
   sleep_and_awake( (int)(duration * 1000));
   cout << "awoke" << endl;
}

int main( int argc, char *argv[])
{
    cout << "see_workhorse version 1.35" << endl;
    parse_arguments(argc, argv);

    if ( config.has_delay) {
       cout << "delay startup " << config.delay << " seconds" << endl;
       do_sleep( config.delay); // implement startup delay
    }

    struct timespec start, end;
    get_clock_mono( start);

    if ( config.has_usta_log) {
       set_usta_logging( config.usta_log);   // enable or disable usta logging
    }

    see_salt *psalt = see_salt::get_instance();
    if ( psalt == nullptr) {
       cout << "FAIL see_salt::get_instance() failed" << endl;
       return 4;
    }

    if ( config.has_debug) {
       psalt->set_debugging( config.debug);
    }
    psalt->begin();

    get_clock_mono( end);
    string begin_witness("psalt->begin() took ");
    display_elapse( start, end, begin_witness);

    /* get vector of sensor names */
    vector<string> sensor_types;
    psalt->get_sensors(sensor_types);
    cout << "get_sensors( sensor_types): " << endl;
    for (size_t i = 0; i < sensor_types.size(); i++) {
       cout << "    " << sensor_types[i] << endl;
    }
    cout << endl;

    /* get vector of all suids for target sensor */
    vector<sens_uid *> sens_uids;
    string target_sensor = string( config.data_type);
    cout << "lookup: " << target_sensor << endl;
    psalt->get_sensors( target_sensor, sens_uids);
    for (size_t i = 0; i < sens_uids.size(); i++) {
       sens_uid *suid = sens_uids[i];
       cout << "suid = [high " << hex << suid->high;
       cout << ", low " << hex << suid->low << "]" << endl;
    }

    if ( 0 == sens_uids.size()) {
       cout << "FAIL " << target_sensor << " not found" << endl;
       return 4;
    }

    /* choose target_suid based on type, name, on_change */
    sensor *psensor = choose_target_sensor( psalt, sens_uids);
    display_target_sensor( psensor);
    if ( psensor == nullptr) {
       return 4; // not found
    }

    string plan_of_record = "is NOT POR.";
    if ( psensor->has_por() && psensor->get_por()) {
       plan_of_record = "is POR.";
    }
    cout << plan_of_record << endl;

    if (config.symbolic_sample_rate) {
       if ( !resolve_symbolic_sample_rate( psensor)) {
          return 4; // unable to resolve
       }
    }
    else if ( psensor->has_max_rate()) {
       float max_rate = psensor->get_max_rate();
       cout << "has max_rate: " << dec << max_rate << " hz" << endl;
    }
    cout << endl;

    // conditionally lookup calibration sensor
    sens_uid *cal_suid = NULL;
    string cal_sensor = "";
    if ( config.has_calibrated && config.calibrated) {
       if ( target_sensor == string("gyro")) {
          cal_sensor = "gyro_cal";
          cal_suid = get_sensor_suid( psalt, cal_sensor);
       }
       else if ( target_sensor == string("mag")) {
          cal_sensor = "mag_cal";
          cal_suid = get_sensor_suid( psalt, cal_sensor);
       }
    }
    // run the test
    salt_rc rc = run( psalt, psensor, cal_suid, cal_sensor);

    delete psalt;

    if ( rc == SALT_RC_SUCCESS) {
       cout << "PASS" << endl;
       return 0;
    }
    else {
       cout << "FAIL" << endl;
       return 4;
    }
}

/**
 * @brief choose_target sensor based on matching command line specifics for
 *        type, name, rigid_body, stream_type ( aka on_change), and hw_id
 * @param psalt[i]
 * @param sens_uids[i] - vector of suids for -sensor=<datatype>
 * @return nullptr or sensor matching command line spec
 */
sensor *choose_target_sensor( see_salt *psalt, vector<sens_uid *> &sens_uids)
{
   if ( psalt == nullptr) {
      return nullptr;
   }
   for (size_t i = 0; i < sens_uids.size(); i++) {
      sens_uid *candidate_suid = sens_uids[i];
      sensor *psensor = psalt->get_sensor( candidate_suid);
      if ( psensor != nullptr) {
         if ( config.has_name) {
            if ( psensor->get_name() != config.name ) {
               continue;
            }
         }
         if ( config.has_rigid_body) {
            if ( !psensor->has_rigid_body()) {
               continue;
            }
            if ( psensor->get_rigid_body() != config.rigid_body) {
               continue;
            }
         }
         if ( config.has_on_change) {
            if ( !psensor->has_stream_type()) {
               continue;
            }
            if ( config.on_change != psensor->is_stream_type_on_change()) {
               continue;
            }
         }
         if ( config.has_hw_id) {
            if( !psensor->has_hw_id()) {
               continue;
            }
            if ( psensor->get_hw_id() != config.hw_id) {
               continue;
            }
         }

         // SENSORTEST-1088 default to rigid_body == display
         // SENSORTEST-1119 when rigid_body and hw_id are omitted
         if ( config.has_rigid_body == false && config.has_hw_id == false) {
            if ( psensor->has_rigid_body()) {
               if ( psensor->get_rigid_body() != "display") {
                  continue;
               }
            }
         }

         if ( !config.has_rigid_body) {   // setup dual gyro_cals or mag_cals
            if ( psensor->has_rigid_body()) {
               string rigid_body = psensor->get_rigid_body();
               strlcpy( config.rigid_body, rigid_body.c_str(),
                        sizeof(config.rigid_body));
            }
         }
         return psensor;
      }
   }
   return nullptr;
}

/* returns true - successful, false - unable to resolve */
bool resolve_symbolic_sample_rate( sensor *psensor)
{
   if ( psensor == nullptr) {
      return false;
   }

   if ( psensor->has_rates()) {
      string lit_rate("min_odr");
      vector<float> rates;
      psensor->get_rates( rates);
      if ( config.symbolic_sample_rate == SYMBOLIC_MIN_ODR) {
         config.sample_rate = rates[ 0]; // min odr
      }
      else {
         lit_rate = "max_odr";
         config.sample_rate = rates[ rates.size() - 1]; // max odr
      }
      cout << "using " << lit_rate << " sample_rate: "
           << config.sample_rate << " hz" << endl;
      return true;
   }

   cout << "symbolic sample_rate supplied, but sensor has no rates" << endl;
   return false;
}

void display_target_sensor( sensor *psensor)
{
   stringstream ss;
   cout << endl;

   ss << "-sensor=" << config.data_type << " ";
   if ( config.has_name) {
      ss << "-name=" << config.name << " ";
   }
   if ( config.has_rigid_body) {
      ss << "-rigid_body=" << config.rigid_body << " ";
   }
   else if (psensor != nullptr && psensor->has_rigid_body()) {
      ss << "-rigid_body=" << psensor->get_rigid_body().c_str() << " ";
   }
   if ( config.has_on_change) {
      ss << "-on_change=" << to_string(config.on_change) << " ";
   }
   if ( config.has_hw_id) {
      ss << "-hw_id=" << to_string(config.hw_id) << " ";
   }
   if ( psensor != nullptr) {
      sens_uid *suid = psensor->get_suid();
      ss << " found" << endl
         << "suid = [high " << hex << suid->high
         << ", low " << hex << suid->low << "]";
      cout << ss.str() << endl;
   }
   else {
      cout << "FAIL " << ss.str() << " not found" << endl;
   }
}
/**
 * @brief run the test
 * @param psalt
 * @param psensor
 * @param calibrated_suid
 * @param cal_sensor
 *
 * @return salt_rc
 */
salt_rc run( see_salt *psalt, sensor *psensor,
             sens_uid *cal_suid, string cal_sensor)
{
   if ( psalt == nullptr || psensor == nullptr) {
      return SALT_RC_FAILED;
   }

   salt_rc rc = SALT_RC_SUCCESS;
   int iteration = 0;
   sens_uid *suid = psensor->get_suid();
   if ( suid == nullptr) {
      return SALT_RC_FAILED;
   }
   string target_sensor = string(config.data_type);

   while ( rc == SALT_RC_SUCCESS
           && iteration < config.times) {
      iteration++;
      cout << "begin iteration: " << to_string( iteration) << endl;

      struct timeval tv;
      gettimeofday( &tv, NULL);

      uint64_t cookie = (uint64_t)tv.tv_usec;

      // conditionally trigger log memory.
      // hope used memory here == used memory at end
      if ( config.has_memory_log && config.memory_log) {
         rc = trigger_diag_sensor_log( psalt, SEE_LOG_TYPE_MEMORY_USAGE_LOG, 0);
      }
      if ( config.has_island_log && config.island_log) {
         rc = trigger_diag_sensor_log( psalt, SEE_LOG_TYPE_ISLAND_LOG, cookie);
      }

      // conditionally request calibration
      if ( rc == SALT_RC_SUCCESS && cal_suid != NULL) {
         rc = on_change_sensor( psalt, cal_suid, cal_sensor);
      }

      // activate appropriate sensor as configured
      if ( rc == SALT_RC_SUCCESS)  {
         if ( "distance_bound" == psensor->get_type()) {
            cout << "using -distance_bound="
                 << to_string( config.distance_bound) << endl;
            rc = distance_bound_sensor( psalt, suid);
         }
         else if ( "basic_gestures" == psensor->get_type()) {
            rc = basic_gestures_sensor( psalt, suid);
         }
         else if ( "multishake" == psensor->get_type()) {
            rc = multishake_sensor( psalt, suid);
         }
         else if ( "psmd" == psensor->get_type()) {
            rc = psmd_sensor( psalt, suid);
         }
         else if ( psensor->has_stream_type()
              && ( psensor->is_stream_type_on_change()
                   || psensor->is_stream_type_single_output())) {
            rc = on_change_sensor( psalt, suid, target_sensor);
         }
         else {
            see_msg_id_e msg_id = MSGID_STD_SENSOR_CONFIG;
            rc = stream_sensor( psalt, msg_id, suid, target_sensor);
         }
      }

      if ( rc == SALT_RC_SUCCESS) {
         try {
            psalt->update_event_cb(suid, (event_cb_func)event_cb);

            float duration = config.duration;
            if ( config.has_flush_request_interval) {
               while ( duration > 0.0) {
                  psalt->sleep( config.flush_request_interval); // seconds
                  rc = flush_sensor( psalt, suid, target_sensor);
                  duration -= config.flush_request_interval;
               }
            }
            else if ( config.rearm
                      && rearm.target_sensor == target_sensor) {
               rearm.suid = suid;
               monitor_and_rearm( psalt, duration); //SNS-660
            }
            else {
               cout << "sleep( " << duration << ") seconds" << endl;
               do_sleep( duration);
            }

            psalt->update_event_cb(suid, (event_cb_func)0);
            if (config.rearm
                && rearm.target_sensor == target_sensor) {
               config.rearm = false; // fix race condition exception
               rearm.suid = (sens_uid *)0;
            }

            rc = disable_sensor( psalt, suid, target_sensor); // stop sensor
         }
         catch (exception& e) {
            cout << "caught execption " << e.what() << endl;
            rc = SALT_RC_FAILED;
         }
      }

      // conditionally disable calibration
      if ( rc == SALT_RC_SUCCESS && cal_suid != NULL) {
         rc = disable_sensor( psalt, cal_suid, cal_sensor);
      }

      if ( rc == SALT_RC_SUCCESS) {
         if ( config.has_island_log && config.island_log) {
            rc = trigger_diag_sensor_log( psalt, SEE_LOG_TYPE_ISLAND_LOG, cookie);
         }
         if ( config.has_memory_log && config.memory_log) {
            rc = trigger_diag_sensor_log( psalt, SEE_LOG_TYPE_MEMORY_USAGE_LOG, 0);
         }
      }
      psalt->sleep( 1);   // hope to get disable/trigger packets logged
      cout << "end iteration: " << to_string( iteration) << endl;
   }

   return rc;
}
