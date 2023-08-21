/* ===================================================================
** Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: Extras.cpp
** ===================================================================*/

#include <iostream>
#include <sstream>

#include "Extras.h"
#include "Utility.h"
#include "see_salt.h"

using namespace std;

namespace sensors_moast {

static vector<agent> agents;
Utility utility;

static sens_uid *resampler_suid = nullptr;

/**
 * parse string of the form: key=value
 * return key, value, and true when keyval input is well formed
 * else return false
 *
 * @param keyval
 * @param key
 * @param value
 *
 * @return bool
 */
static bool get_key_value( string keyval, string& key, string& value)
{
   key = "";
   value = "";
   size_t pos = keyval.find('=');
   if ( pos != string::npos) {
      key = keyval.substr(0, pos);
      value = keyval.substr( pos + 1);
      return true;
   }
   return false;
}

/**
 *  parse one line (from sensor_list.txt) of form:
 *  -sensor=gyro -stream_type=streaming -rigid_body=display
 *  -name=chipname -combosensor=accel
 *
 * @param config - gets parse result
 * @returns true - successful parse and config populated with result
 *          false - parse failed
 */
bool parse_sensor_list_line(sensor_config &config)
{
   string input = config.tc_spec;
   config.sensor_type = "";
   config.stream_type = "";
   config.rigid_body = "";
   config.combo_sensor= "";
   config.sensor_name = "";
   config.using_resampler = false;
   config.using_calibration = false;

   size_t pos = input.rfind('\n');
   if ( pos < input.length()) {
      input = input.substr(0, pos);
   }
   while ( input.length() > 0) {
      string keyval = "";
      size_t pos = input.find(' ');
      if ( pos != string::npos) {
         keyval = input.substr(0, pos);
         input = input.substr(pos + 1);
      }
      else {
         keyval = input;
         input = "";
      }

      string key;
      string value;
      if ( get_key_value( keyval, key, value)) {
         if ( "-sensor" == key) {
            config.sensor_type = value;
            if ( value == "gyro_cal" || value == "mag_cal") {
               config.using_calibration = true;
            }
         }
         else if ( "-stream_type" == key) {
            if (value == "resampler") {
               config.using_resampler = true;
               config.stream_type = "streaming";
            }
            else {
               config.stream_type = value;
            }
         }
         else if ( "-rigid_body" == key) {
            config.rigid_body = value;
         }
         else if ( "-name" == key) {
            config.sensor_name = value;
         }
         else if ( "-combo_sensor" == key) {
            config.combo_sensor = value;
         }
         else {
            cout << "+ unrecognized: " << keyval << endl;
            return false;
         }
      }
   }
   return true;
}

/**
 * return nullptr or a pointer to the sensor_type's sensor class
 *      *
 * @param psalt
 * @param sensor_type
 *
 * @return sensor*
 */
sensor *Extras::find_combo_sensor( see_salt *psalt, string sensor_type)
{
   if ( psalt == nullptr) {
      return nullptr;
   }
   vector<sens_uid *> sens_uids;
   psalt->get_sensors( sensor_type, sens_uids);
   if (sens_uids.size() == 0) {
      return nullptr;
   }
   return psalt->get_sensor( sens_uids[0]);
}

/**
 * choose among the input sens_uids returning a pointer to the sensor class
 * matching the config's rigid_body, stream_type, and name
 *
 * @param psalt
 * @param sens_uids
 * @param config
 *
 * @return sensor*
 */
sensor *choose_target_sensor( see_salt *psalt,
                              vector<sens_uid *> &sens_uids,
                              sensor_config &config)
{
   if ( psalt == nullptr) {
      return nullptr;
   }
   for (size_t i = 0; i < sens_uids.size(); i++) {
      sens_uid *candidate_suid = sens_uids[i];
      sensor *psensor = psalt->get_sensor( candidate_suid);
      if ( psensor != nullptr) {

         if ( config.rigid_body != "") {
            if ( !psensor->has_rigid_body()) {
               continue;
            }
            if ( config.rigid_body != psensor->get_rigid_body())
            {
               continue;
            }
         }
         if ( config.stream_type != "") {
            if ( !psensor->has_stream_type()) {
               continue;
            }
            if ( config.stream_type == "streaming"
                 && !psensor->is_stream_type_streaming()) {
               continue;
            }
            if ( config.stream_type == "on_change"
                 && psensor->is_stream_type_streaming()) {
               continue;
            }
         }
         if ( config.sensor_name != "") {
            if ( config.sensor_name != psensor->get_name()) {
               continue;
            }
         }
         return psensor;
      }
   }
   return nullptr;
}

/**
 * return a pointer to the sensor class matching the config's sensor_type,
 * rigid_body, stream_type, and name
 *
 * @param psalt
 * @param config
 *
 * @return sensor*
 */
sensor *Extras::find_see_salt_target( see_salt *psalt, sensor_config &config)
{
   if ( psalt == nullptr) {
      return nullptr;
   }
   vector<sens_uid *> sens_uids;
   psalt->get_sensors( config.sensor_type, sens_uids);
   sensor *psensor = choose_target_sensor( psalt, sens_uids, config);
   return psensor;
}

/**
 * Use the input tc_spec to populate a sensor_config structure.
 * If this is an available sensor, add it to the agents vector of
 * available sensors.
 *
 * @param psalt
 * @param tc_spec - one line from the input sensor_info.txt file
 *
 * @return bool - true when agent is found and available, else false
 */
bool Extras::populate_agents( see_salt *psalt, string tc_spec)
{
   sensor_config config;
   config.tc_spec = tc_spec;

   if ( psalt == nullptr) {
      return false;
   }

   std::ostringstream candidate;
   candidate << "- candidate: " << tc_spec << " :: ";

   if ( !parse_sensor_list_line(config)) {
      return false;
   }
   if (config.combo_sensor != "") {
      sensor *psensor = find_combo_sensor(psalt, config.combo_sensor);
      if (psensor == nullptr) {
         candidate << "combosensor not found -";
         utility.write_line( candidate);
         return false;
      }
      config.sensor_name = psensor->get_name();
   }
   sensor *psensor = find_see_salt_target( psalt, config);
   if ( psensor == nullptr) {
      candidate << "not found -";
      utility.write_line( candidate);
      return false;
   }

   if (!( psensor->has_available() && psensor->get_available())) {
      candidate << "not available -";
      utility.write_line( candidate);
      return false;
   }

   if (!( psensor->has_por() && psensor->get_por())) {
      candidate << "not POR -";
      utility.write_line( candidate);
      return false;
   }

   if (config.sensor_type == "streaming" || config.sensor_type == "resampler") {
      if (!( psensor->has_stream_type() && psensor->is_stream_type_streaming())) {
         return false;
      }
   }

   agent player;
   player.config = config;
   player.psensor = psensor;
   player.client = 0;
   player.suid = nullptr;
   player.resampler_target_suid = nullptr;

   if ( config.using_resampler) {
      if ( resampler_suid == nullptr) {
         sensor *presampler = find_combo_sensor(psalt,"resampler");
         if (presampler == nullptr) {
            candidate << "resampler not found";
            utility.write_line( candidate);
            return false;
         }
         resampler_suid = presampler->get_suid(); ;
         if (resampler_suid == nullptr) {
            candidate << "resampler suid not found";
            utility.write_line( candidate);
            return false;
         }
      }
      player.suid = resampler_suid;
      player.resampler_target_suid = player.psensor->get_suid();
      if (player.resampler_target_suid == nullptr) {
         candidate << "suid not found";
         utility.write_line( candidate);
         return false;  // should not occur
      }
   }
   else {
      player.suid = player.psensor->get_suid();
      if (player.suid == nullptr) {
         candidate << "suid not found -";
         utility.write_line( candidate);
         return false;  // should not occur
      }
   }
   agents.push_back(player);
   candidate << "available +";
   utility.write_line( candidate);
   return true;
}

/**
 * choose a random agent from the pool of available sensors.
 *
 * @param fraction
 *
 * @return agent
 */
agent Extras::get_agent(float fraction)
{
   size_t index = (size_t)round( fraction * ( agents.size() - 1));
   agent player = agents[index];
   return player;
}

} // namespace sensors_moast

