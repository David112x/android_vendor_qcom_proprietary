/* ===================================================================
**
** Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: Moast.h
** DESC: Class that defines various members that are required to run
**       the test
** ===================================================================*/

#ifndef MOAST_H
#define MOAST_H

#include<vector>        // std::vector
#include "see_salt.h"

namespace sensors_moast {
    class Moast {
        private:
            Moast() {} // class is singleton and not inheritible
            // Prevent construction of new object by copy constructor
            Moast(Moast const&) = delete;
            // Prevent construction of new object when assignment operator is
            // used
            void operator=(Moast const&) = delete;

        public:
            // Create a single instance of the class
            static Moast* get_moast_instance();
            static const int MICROSECONDS;

            // Vector of sensors
            std::vector<std::string>sens_vec;
            // Vector of stream or batch requests
            std::vector<std::string>stream_or_batch_vec;

            // Member variables
            int max_qmi_clients;
            int available_qmi_clients;
            int max_conc_sensor_cnt;
            int available_conc_sensor_cnt;
            int max_duration_per_req;
            int max_step_duration;
            float max_apps_rpt_rate;
            float available_apps_rpt_rate;
            float available_sampling_rate;
            float max_sampling_rate;
            float db_length;
            bool available_conc_sensor_cnt_flag;
            bool available_qmi_clients_flag;
            float step_duration; //seconds

            // Sensor-suid mapping
            typedef std::map<std::string, sens_uid*> map_sens_suid;
            map_sens_suid sens_suid_map;

            // Resampler suid // TODO: Move it to Utility Class
            sens_uid resampler_suid;
//-----------------------------------------------------------------------------
            /**
            * Process test params from config file
            *
            * @param void
            * @return void
            */
            void get_config_test_params();
//-----------------------------------------------------------------------------
            void open_qmi_clients();

            void set_step_duration( float seconds){
               step_duration = seconds;
            }
            float get_step_duration() {
               return step_duration;
            }
    };
} // end of namespace sensors_moast

#endif // MOAST_H
