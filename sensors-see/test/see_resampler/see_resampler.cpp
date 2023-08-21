/* ===================================================================
** Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: see_resampler.cpp
** DESC: run resampler sensor using command line argumentss
** ================================================================ */
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "see_salt.h"

#ifdef SNS_LE_QCS605
#endif
#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#endif

using namespace std;

struct app_config {
    bool  is_valid = true;
    char  data_type[ 32];           // type: accel, gyro, mag, ...

    bool  has_name = false;
    char  name[ 32];

    bool  has_rigid_body = false;
    char  rigid_body[ 32];

    bool  has_hw_id = false;
    int   hw_id;

    bool  has_batch_period = false;
    float batch_period;             // seconds

    bool  has_flush_period = false;
    float flush_period;             // seconds

    bool  has_flush_only = false;
    bool  flush_only;

    bool  has_max_batch = false;
    bool  max_batch;

    float sample_rate = 10.0;       // hz

    see_resampler_rate  rate_type = SEE_RESAMPLER_RATE_MINIMUM;

    bool  filter = false;

    bool  has_calibrated = false;
    bool  calibrated;

    bool  has_delay = false;
    float delay;

    float duration = 10.0;          // seconds

    app_config() {}
};

static app_config config;

/**
 * @brief parse command line argument of the form keyword=value
 * @param parg[i] - one command line argument
 * @param key[io] - gets string to left of '='
 * @param value[io] - sets string to right of '='
 * @return true - successful parse. false - parse failed.
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

void usage(void)
{
    cout << "usage: see_resampler"
            " -sensor=<sensor_type>"
            " [-name=<name>]"
            " [-rigid_body=<display | keyboard>]"
            " [-hw_id=<number>]\n"
            "       -sample_rate=<hz>"
            " [-rate_type=<fixed | minimum>]"
            " [-filter=<0 | 1>\n"
            "       [-batch_period=<seconds>]"
            " [-flush_period=<seconds>]"
            " [-max_batch=<0 | 1>] [-flush_only=<0 | 1>]\n"
            "       [-calibrated=<0 | 1>]"
            " [-delay=<seconds>]"
            " -duration=<seconds>\n"
            "where: <sensor_type> :: accel | gyro | ...\n";
}

void parse_arguments(int argc, char *argv[])
{
    // command line args:
    string sensor_eq( "-sensor");
    string name_eq( "-name");
    string rigid_body_eq( "-rigid_body");
    string hw_id_eq( "-hw_id");
    string sample_rate_eq( "-sample_rate");
    string rate_type_eq( "-rate_type");
    string filter_eq( "-filter");
    string batch_period_eq( "-batch_period");
    string flush_period_eq( "-flush_period");
    string flush_only_eq( "-flush_only");
    string max_batch_eq( "-max_batch");
    string calibrated_eq( "-calibrated");
    string delay_eq( "-delay");
    string duration_eq( "-duration");

    for ( int i = 1; i < argc; i++) {
        string key;
        string value;
        if ( !get_key_value( argv[i], key, value)) {
           if ( 0 == strcmp( argv[i], "-h")) {
              config.is_valid = false;
              break;
           }
           config.is_valid = false;
           cout << "unrecognized arg " << argv[i] << endl;
           continue;
        }
        if ( sensor_eq == key) {
           strlcpy( config.data_type,
                    value.c_str(),
                    sizeof( config.data_type));
        }
        else if ( name_eq == key) {
           config.has_name = true;
           strlcpy( config.name, value.c_str(), sizeof( config.name));
        }
        else if ( rigid_body_eq == key) {
           config.has_rigid_body = true;
           strlcpy( config.rigid_body, value.c_str(),
                    sizeof( config.rigid_body));
        }
        else if ( hw_id_eq == key) {
           config.has_hw_id = true;
           config.hw_id = atoi( value.c_str());
        }
        else if ( max_batch_eq == key) {
           config.has_max_batch = true;
           config.max_batch = atoi( value.c_str());
        }
        else if ( flush_only_eq == key) {
           config.has_flush_only = true;
           config.flush_only = atoi( value.c_str());
        }
        else if ( sample_rate_eq == key) {
           config.sample_rate = atof( value.c_str());
        }
        else if ( rate_type_eq == key) {
           if ( value == string("fixed")) {
              config.rate_type = SEE_RESAMPLER_RATE_FIXED;
           }
           else if ( value == string("minimum")) {
              config.rate_type = SEE_RESAMPLER_RATE_MINIMUM;
           }
           else {
              config.is_valid = false;
              cout << "bad rate type value: " << value << endl;
           }
        }
        else if ( filter_eq == key) {
           config.filter = atoi( value.c_str());
        }
        else if ( batch_period_eq == key) {
           config.has_batch_period = true;
           config.batch_period = atof( value.c_str());
        }
        else if ( flush_period_eq == key) {
           config.has_flush_period = true;
           config.flush_period = atof( value.c_str());
        }
        else if ( calibrated_eq == key) {
           config.has_calibrated = true;
           config.calibrated = atoi( value.c_str());
        }
        else if ( delay_eq == key) {
           config.has_delay= true;
           config.delay = atof( value.c_str());
        }
        else if ( duration_eq == key) {
           config.duration = atof( value.c_str());
        }
        else {
            config.is_valid = false;
            cout << "unrecognized arg " << argv[i] << endl;
        }
    }
    if ( !config.is_valid) {
       usage();
       exit( 4);
    }
}

salt_rc on_change_sensor(see_salt *psalt, sens_uid *suid, string target)
{
   cout << "on_change_sensor( " << target << ")" << endl;

   see_std_request std_request;
   see_client_request_message request( suid,
                                       MSGID_ON_CHANGE_CONFIG,
                                       &std_request);
   salt_rc rc = psalt->send_request(request);
   cout << "on_change_sensor() complete rc " << to_string(rc) << endl;
   return rc;
}

void init_std_request(see_std_request *std_request, app_config *pconfig)
{
   if ( pconfig->has_batch_period) {
      cout << "batch_period: " << to_string( pconfig->batch_period)
           << " seconds" << endl;
      std_request->set_batch_period( pconfig->batch_period * 1000000); // microsec
   }
   if ( pconfig->has_flush_period) {
      cout << "flush_period: " << to_string( pconfig->flush_period)
           << " seconds" << endl;
      std_request->set_flush_period( pconfig->flush_period * 1000000); // microsec
   }
   if ( pconfig->has_flush_only) {
      cout << "flush_only: " << to_string( pconfig->flush_only) << endl;
      std_request->set_flush_only( pconfig->flush_only);
   }
   if ( pconfig->has_max_batch) {
      cout << "max_batch: " << to_string( pconfig->max_batch) << endl;
      std_request->set_max_batch( pconfig->max_batch);
   }
}

void resample_sensor(see_salt *psalt,
                     sens_uid *resampler_suid,
                     sens_uid *target_suid,
                     app_config *pconfig)
{
   cout << "resample_sensor()" << endl;
   see_std_request std_request;
   init_std_request( &std_request, pconfig);

   see_resampler_config payload( target_suid,
                                 pconfig->sample_rate,
                                 pconfig->rate_type,
                                 pconfig->filter);

   std_request.set_payload( &payload);
   see_client_request_message request( resampler_suid,
                                       MSGID_RESAMPLER_CONFIG,
                                       &std_request);
   psalt->send_request(request);
   cout << "resample_sensor() complete" << endl;
}

void disable_sensor(see_salt *psalt, sens_uid *suid, string sensor_type)
{
   cout << "disable_sensor( " << sensor_type << ")" << endl;
   see_std_request std_request;
   see_client_request_message request( suid, MSGID_DISABLE_REQ, &std_request);
   psalt->send_request(request);
   cout << "disable_sensor() complete" << endl;
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
    if ( psalt == nullptr) {
       cout << "init failure, psalt == null" << endl;
       exit(4);
    }
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
          // need only xxx_cal rigid_body same as xxx sensor's rigid body
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

int main(int argc, char *argv[])
{
    cout << "see_resampler version 1.4" << endl;
    parse_arguments(argc, argv);

    if ( config.data_type[0] == '\0') {
       cout << "missing -sensor=<sensor_type>" << endl;
       exit( 4);
    }

    if ( config.has_delay) {
       cout << "delay startup " << config.delay << " seconds" << endl;
       usleep( (int)config.delay * 1000000); // implement startup delay
    }

    see_salt *psalt = see_salt::get_instance();

    psalt->begin();  // populate sensor attributes

    /*
    * lookup resampler's suid
    */
    sens_uid resampler_suid;
    string resampler = string( "resampler");
    cout << "lookup: " << resampler << endl;

    vector<sens_uid *> sens_uids;
    psalt->get_sensors( resampler, sens_uids);
    if (sens_uids.size()) {
       resampler_suid.low = sens_uids[0]->low;
       resampler_suid.high = sens_uids[0]->high;
    }
    else {
       cout << " not found" << endl;
       cout << "FAIL" << endl;
       return 4;
    }
    cout << endl;

    /*
    * lookup target sensors's suid
    */
    string target_sensor = string( config.data_type);
    cout << "lookup: " << target_sensor << endl;
    psalt->get_sensors( target_sensor, sens_uids);
    for (size_t i = 0; i < sens_uids.size(); i++) {
       sens_uid *suid = sens_uids[i];
       cout << "suid = [high " << hex << suid->high;
       cout << ", low " << hex << suid->low << "]" << endl;
    }

    if ( !sens_uids.size()) {
       cout << "FAIL " << target_sensor << " not found" << endl;
       return 4;
    }
    cout << endl;

    sens_uid *target_suid = NULL;
    sensor *psensor = NULL;
    for (size_t i = 0; i < sens_uids.size(); i++) {
       sens_uid *candidate_suid = sens_uids[i];
       psensor = psalt->get_sensor( candidate_suid);
       if ( config.has_name) {
          if ( psensor->get_name() != config.name) {
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
       if ( config.has_hw_id) {
          if ( !psensor->has_hw_id()) {
             continue;
          }
          if ( psensor->get_hw_id() != config.hw_id) {
             continue;
          }
       }
       // SENSORTEST-1119 default when rigid_body and hw_id are omitted
       if ( config.has_rigid_body == false &&  config.has_hw_id == false) {
          if ( psensor->has_rigid_body() ) { // SENSORTEST-1088
             if ( psensor->get_rigid_body() != "display") { // default
                continue;
             }
          }
       }

       target_suid = candidate_suid;    // found target
       if ( !config.has_rigid_body) {   // setup does_rigid_body_match()
          if ( psensor->has_rigid_body()) {
             string rigid_body = psensor->get_rigid_body();
             strlcpy( config.rigid_body, rigid_body.c_str(),
                      sizeof(config.rigid_body));
          }
       }
       break;
    }

    stringstream ss;
    ss << "-sensor=" << config.data_type << " ";
    if ( config.has_name) {
       ss << "-name=" << config.name << " ";
    }
    if ( psensor->has_rigid_body()) {
       ss << "-rigid_body=" << psensor->get_rigid_body().c_str() << " ";
    }
    if ( config.has_hw_id) {
       ss << "-hw_id=" << to_string(config.hw_id) << " ";
    }
    if ( target_suid == NULL) {
       cout << "FAIL " << ss.str() << " not found" << endl;
       return 4;
    }
    cout << ss.str() << " found" << endl;
    cout << "suid = [high " << hex << target_suid->high;
    cout << ", low " << hex << target_suid->low << "]" << endl;

    // conditionally lookup calibration control sensor
    sens_uid *calibrate_suid = NULL;
    string target_cal_control = "";
    if ( config.has_calibrated && config.calibrated) {
       if ( target_sensor == string("gyro")) {
          target_cal_control = "gyro_cal";
          calibrate_suid = get_sensor_suid( psalt, target_cal_control);
       }
       else if ( target_sensor == string("mag")) {
          target_cal_control = "mag_cal";
          calibrate_suid = get_sensor_suid( psalt, target_cal_control);
       }
    }

    salt_rc rc = SALT_RC_SUCCESS;
    if ( calibrate_suid) {
       rc = on_change_sensor( psalt, calibrate_suid, target_cal_control);
    }

    resample_sensor( psalt, &resampler_suid, target_suid, &config);
    cout << "sleep " << to_string( config.duration) << " seconds" << endl;
    psalt->sleep( config.duration); // seconds
    disable_sensor( psalt, &resampler_suid, resampler);

    if ( rc == SALT_RC_SUCCESS && calibrate_suid) {
       disable_sensor( psalt, calibrate_suid, target_cal_control);
    }

    cout << "PASS" << endl;
    return 0;
}


