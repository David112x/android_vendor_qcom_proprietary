/* ===================================================================
**
** Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: Parser.h
** DESC: Parses data from the input files
** ===================================================================*/

#ifndef PARSER_H
#define PARSER_H

#include <map>            // std::map
#include <vector>

namespace sensors_moast {
    namespace Parser {
      // Sensor configurations are populated from config_list file
      typedef std::map<std::string, std::string> config_map;
      // List of sensors populated from sensor_list_file
      typedef std::vector<std::string> sensor_vec;
//-----------------------------------------------------------------------------
      /**
        * Parses data from input files: sensor_list.txt and config.txt files
        *
        * @param argv passed as array of pointers to char
        * @return void
        */
        void parse_input_files(char *argv[]);
//-----------------------------------------------------------------------------
        /**
        * Return sensor vector as reference
        *
        * @param void
        * @return sensor_vec as reference
        */
        sensor_vec& get_vector_of_sensors();
//-----------------------------------------------------------------------------
      /**
        * Return supported max clients from config file
        *
        * @param void
        * @return max qmi clients as int
        */
        int get_max_qmi_clients();
//-----------------------------------------------------------------------------
      /**
        * Return max concurrent sensor count from config file
        *
        * @param void
        * @return max qmi clients as int
        */
        int get_max_conc_sensor_count();
//-----------------------------------------------------------------------------
        /**
        * Return max duration for each request from config file
        *
        * @param void
        * @return max duration for each request as int
        */
        int get_max_duration_per_request();
//-----------------------------------------------------------------------------
        /**
        * Return max step duration from config file
        *
        * @param void
        * @return max step duration as float
        */
        int get_max_step_duration();
//-----------------------------------------------------------------------------
        /**
        * Return distance bound from config file
        *
        * @param void
        * @return distance bound as float
        */
        float get_distance_bound();
//-----------------------------------------------------------------------------
        /**
        * Return max report count per sec from config file
        *
        * @param void
        * @return max report count as int
        */
        int get_max_report_cnt_per_sec();
//-----------------------------------------------------------------------------
        /**
        * Return max sampling rate from config file
        *
        * @param void
        * @return max sampling rate as float
        */
        float get_max_sampling_rate();
//-----------------------------------------------------------------------------
        /**
         * Return usta log status
         *
         * @param void
         * @return usta_log status as bool value. Default is 1 i.e. usta logging
         * is on
         */
         bool get_usta_log_status();
    } // end of Parser
} // end of sensors_moast

#endif /* PARSER_H */
