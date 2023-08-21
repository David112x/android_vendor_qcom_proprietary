/* ===================================================================
** Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: see_selftest.cpp
** DESC: physical sensor self test
** ================================================================ */
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include "see_salt.h"

using namespace std;

void self_test_sensor(see_salt *psalt,
                      sens_uid *suid,
                      see_self_test_type_e testtype)
{
   see_self_test_config payload( testtype);
   cout << "self_test_sensor() testtype " << to_string( testtype);
   cout << " - " << payload.get_test_type_symbolic( testtype) << endl;

   see_std_request std_request;
   std_request.set_payload( &payload);
   see_client_request_message request( suid,
                                       MSGID_SELF_TEST_CONFIG,
                                       &std_request);
   psalt->send_request(request);
   cout << "self_test_sensor()" << endl;
}

struct app_config {
    bool  is_valid = true;
    char  data_type[ 32];           // type: accel, gyro, mag, ...

    bool  has_name = false;
    char  name[ 32];

    bool  has_on_change = false;
    bool  on_change;

    bool  has_rigid_body = false;
    char  rigid_body[ 32];

    bool  has_hw_id = false;
    int   hw_id;

    bool  has_testtype = false;
    int   testtype = SELF_TEST_TYPE_FACTORY;

    bool  has_delay = false;
    float delay;

    float duration = 5.0;          // seconds

    app_config() {}
};

static app_config config;

void usage(void)
{
    cout << "usage: see_selftest"
            " -sensor=<sensor_type>"
            " [-name=<name>]"
            " [-on_change=<0 | 1>]\n"
            "       [-rigid_body=<display | keyboard | external>]"
            " [-hw_id=<number>]\n"
            "       -testtype=<number | sw | hw | factory | com>\n"
            "       [-delay=<seconds>]"
            " -duration=<seconds>"
            " [-help]\n"
            "where: <sensor_type> :: accel | gyro | ...\n";
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

void parse_testtype( string value)
{
   string lit_sw("sw");
   string lit_hw("hw");
   string lit_factory("factory");
   string lit_com("com");

   config.has_testtype = true;
   if ( value == lit_sw) {
      config.testtype = SELF_TEST_TYPE_SW;
   }
   else if ( value == lit_hw) {
      config.testtype = SELF_TEST_TYPE_HW;
   }
   else if ( value == lit_factory) {
      config.testtype = SELF_TEST_TYPE_FACTORY;
   }
   else if ( value == lit_com) {
      config.testtype = SELF_TEST_TYPE_COM;
   }
   else {
      config.testtype = atoi( value.c_str());
   }
}

void parse_arguments(int argc, char *argv[])
{
    string sensor_eq( "-sensor");
    string name_eq( "-name");
    string on_change_eq( "-on_change");
    string rigid_body_eq( "-rigid_body");
    string hw_id_eq( "-hw_id");
    string testtype_eq( "-testtype");
    string delay_eq( "-delay");
    string duration_eq( "-duration");

    for ( int i = 1; i < argc; i++) {
        if (( 0 == strcmp( argv[i], "-h"))
             || ( 0 == strcmp( argv[i], "-help"))) {
           config.is_valid = false;
           break;
        }

        string key;
        string value;
        if ( get_key_value( argv[i], key, value)) {
           if ( sensor_eq == key) {
              strlcpy( config.data_type,
                       value.c_str(),
                       sizeof( config.data_type));
           }
           else if ( name_eq == key) {
              config.has_name = true;
              strlcpy( config.name,
                       value.c_str(),
                       sizeof( config.name));
           }
           else if ( on_change_eq == key) {
              config.has_on_change = true;
              config.on_change = atoi( value.c_str());
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
           else if ( testtype_eq == key) {
              parse_testtype( value);
           }
           else if ( delay_eq == key) {
              config.has_delay = true;
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

int main(int argc, char *argv[])
{
    cout << "see_selftest version 1.1" << endl;
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
    psalt->begin();

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
    cout << endl;

    /* choose target_suid based on type, name, on_change, rigid_body, hw_id */
    sens_uid *target_suid = NULL;
    sensor *psensor = NULL;

    for (size_t i = 0; i < sens_uids.size(); i++) {
       sens_uid *candidate_suid = sens_uids[i];
       psensor = psalt->get_sensor( candidate_suid);
       if ( config.has_name) {
          string name = psensor->get_name();
          if ( 0 != strcmp( config.name, name.c_str())) {
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
       if ( config.has_rigid_body) {
          string rigid_body = psensor->get_rigid_body();
          if ( 0 != strncmp( config.rigid_body, rigid_body.c_str(),
                             rigid_body.length())) {
             continue;
          }
       }
       if ( config.has_hw_id && psensor->has_hw_id()) {
          if ( psensor->get_hw_id() != config.hw_id) {
             continue;
          }
       }
       target_suid = candidate_suid;    // found target
       break;
    }

    stringstream ss;
    ss << "-sensor=" << config.data_type << " ";
    if ( config.has_name) {
       ss << "-name=" << config.name << " ";
    }
    if ( config.has_on_change) {
       ss << "-on_change=" << to_string(config.on_change) << " ";
    }
    if ( config.has_rigid_body) {
       ss << "-rigid_body=" << config.rigid_body << " ";
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

    if ( !config.has_testtype) {
       config.testtype = SELF_TEST_TYPE_FACTORY;
    }

    self_test_sensor( psalt, target_suid,
                      (see_self_test_type_e)config.testtype);
    cout << "sleep " << config.duration << " seconds" << endl;
    psalt->sleep( config.duration); // seconds

    cout << "PASS" << endl;
    return 0;
}
