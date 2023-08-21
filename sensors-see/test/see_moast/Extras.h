/* ===================================================================
** Copyright (c) 2019 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: Extras.h
** ===================================================================*/
#include <string>
// #include <string.h>
#include <math.h>

#include "see_salt.h"

namespace sensors_moast {

   typedef struct sensor_config {
      std::string tc_spec; // line from sensor_list.txt
      std::string sensor_type;
      std::string stream_type;
      std::string rigid_body;
      std::string sensor_name;
      std::string combo_sensor;
      bool using_resampler;
      bool using_calibration;
   } sensor_config;

   typedef struct agent {
      sensor_config config;
      sensor *psensor;
      int client;
      sens_uid *suid;
      sens_uid *resampler_target_suid;
   } agent;

   class Extras {

      public:
         Extras() {} // default constructor

         bool populate_agents( see_salt *psalt, std::string tc_spec);
         agent get_agent(float fraction);
         sensor *find_combo_sensor( see_salt *psalt, std::string sensor_type);
         sensor *find_see_salt_target( see_salt *psalt, sensor_config &config);
   };

} // namespace
