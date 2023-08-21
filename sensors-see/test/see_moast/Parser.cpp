/* ===================================================================
**
** Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: Parser.cpp
** DESC: Implementation of functions declared in Parser.h
** ===================================================================*/

#include <algorithm>      // std::remove_if
#include <fstream>        // std::ifstream
#include <sstream>        // std::istringstream
#include <iostream>       // std::cout, std::endl
#include <vector>         // std::vector

#include "Parser.h"
#include "Utility.h"
#include "Moast.h"

namespace sensors_moast {

    namespace Parser {
        config_map config_mapping; // configs mapping
        sensor_vec sensor_vector;  // vector of sensors

        // Create a singleton Moast object
        Moast* moast_instance = Moast::get_moast_instance();
        // Create an instance of Utility class
        Utility utility_instance;

        // Default initialization
        unsigned int random_seed = 0;
        int default_max_qmi_clients = 3;
        int default_max_conc_sensor_count = 18;
        int default_max_duration_per_request = 10;
        int default_max_rpt_cnt_per_sec = 600;
        int default_max_step_duration = 1800;
        float default_max_sampling_rate = 1200.0f;
        float default_min_distance_bound = 1.0f;
        bool default_usta_log = 1;

        // Parse config file
        void parse_config_list_file(char argv[])
        {
            std::string list_of_configs(argv);

            std::ifstream config_list;
            config_list.open( list_of_configs); // remove sdcard dependency
            if ( !config_list.is_open()) {
               // backward compatibility
               config_list.open( "/sdcard/moast/" + list_of_configs);
            }
            if ( !config_list.is_open() ) {
                utility_instance.print_file_line_number(__FILE__, __LINE__,
                                                                    __func__);
                throw "Error in opening config_list file\n";
            }
            // Check if bad bit error flag is set for stream error while reading
            // config list file
            if (config_list.bad()) {
                utility_instance.print_file_line_number(__FILE__, __LINE__,
                                                                    __func__);
                config_list.close();
                throw "Error while reading from config_list file\n";
            }
            std::string line = "";
            size_t pos;
            char split_char = '=';

            std::ostringstream oss;
            std::string action = "Reading from " + list_of_configs;
            utility_instance.create_anchor_msg( oss, action, "");
            utility_instance.write_line(oss);

            // Read one line at a time from the file
            while (std::getline(config_list, line)) {
                   // Skip lines with comments ('#')
                    if (line[0] == '#') {
                        continue;
                    }
                    // Skip CRLF, if input file was created on windows platform
                    if (line[0] == '\r') {
                        continue;
                    }
                    // Skip empty lines
                    if (line.empty()) {
                        continue;
                    }
                    std::istringstream split(line);
                    pos = line.find('=');
                    std::string key = line.substr(0, pos);
                    std::string value_as_string = line.substr(pos + 1);
                    config_mapping.insert(std::make_pair(key, value_as_string));
            }

            if(config_mapping.find("seed") != config_mapping.end()) {
                random_seed = std::stoul(config_mapping.find("seed")->second);
                std::cout << "\nUsing previously generated seed: ";
                std::cout << random_seed << std::endl << std::endl;
            }
            utility_instance.init_random( random_seed);

            // Print list of configs
            for(auto it = config_mapping.cbegin(); it != config_mapping.cend();
                                                                    ++it) {
                std::cout << it->first << " => " << it->second << std::endl;
            }
            std::cout << std::endl;
            config_list.close();
            if(moast_instance == nullptr) {
                utility_instance.print_file_line_number(__FILE__, __LINE__,
                                                                    __func__);
                return;
            }
        } // end of parse_config_list_file
//-----------------------------------------------------------------------------
        // Parse sensor list file
        void parse_sensor_list_file(char argv[])
        {
            std::string list_of_sensors(argv);

            std::ifstream sensor_list;
            sensor_list.open(list_of_sensors);// remove sdcard dependency
            if (!sensor_list.is_open()) {
               // backward compatibility
               sensor_list.open("/sdcard/moast/" + list_of_sensors);
            }
            if (!sensor_list.is_open()) {
                 utility_instance.print_file_line_number(__FILE__, __LINE__,
                                                                    __func__);
                 throw "Error in opening sensor_list file\n";
            }
            // Check if bad bit error flag is set for stream error while reading
            // sensor list file
            if (sensor_list.bad()) {
                    utility_instance.print_file_line_number(__FILE__, __LINE__,
                                                                    __func__);
                sensor_list.close();
                throw "Error while reading from sensor_list file\n";
            }
            std::ostringstream oss;
            std::string action = "Reading from " + list_of_sensors;
            utility_instance.create_anchor_msg( oss, action, "");
            utility_instance.write_line(oss);

            // Read one line at a time from the file
            bool first_line = true;
            std::string version("version");
            std::string incompatible = "FAIL: Incompatible sensor_list.txt.\n"
                                       "Please upgrade to latest version.\n";
            std::string line = "";
            while (std::getline(sensor_list, line)) {
                   if (first_line) {
                      first_line = false;
                      if (line.substr(0,version.length()) != version) {
                         std::cout << incompatible;
                         exit(4);
                      }
                      continue;
                   }
                   // Skip lines with comments ('#')
                    if (line[0] == '#') {
                        continue;
                    }
                    // Skip CRLF, if input file was created on windows platform
                    if (line[0] == '\r') {
                        continue;
                    }
                    // Skip empty lines
                    if (line.empty()) {
                        continue;
                    }
                    // Remove '\r' character if present
                    if (!line.empty() && line[line.size() - 1] == '\r') {
                         line.erase(line.size() - 1);
                    }
                    sensor_vector.push_back(line);
            }
            sensor_list.close();
        } // end of parse_sensor_file
//-----------------------------------------------------------------------------
        // Invoke appropriate functions to parse sensor_list and config files
        void parse_input_files(char* argv[])
        {
#ifdef DEBUG
            char sens_list[] = "sensor_list.txt";
            char config[] = "config.txt";
            parse_sensor_list_file((char *)sens_list);
            parse_config_list_file((char *)config);
#else
            parse_sensor_list_file(argv[1]); // Parse sensor list file
            parse_config_list_file(argv[2]); // Parse config file
#endif
        }
//-----------------------------------------------------------------------------
        // Return sensor vector
        sensor_vec& get_vector_of_sensors()
        {
            return(sensor_vector);
        }
//-----------------------------------------------------------------------------
        // Return max qmi clients
        int get_max_qmi_clients()
        {
           if(config_mapping.find("max_qmi_clients") != config_mapping.end()) {
               return(stoi(config_mapping.find("max_qmi_clients")->second));
           }
           return(default_max_qmi_clients);
        }
//-----------------------------------------------------------------------------
        // Return max concurrent sensor count
        int get_max_conc_sensor_count()
        {
            if(config_mapping.find("max_conc_sensor_cnt") !=
                                                    config_mapping.end()) {
               return(stoi(config_mapping.find("max_conc_sensor_cnt")
                                                                 ->second));
            }
            return(default_max_conc_sensor_count);
        }
//-----------------------------------------------------------------------------
        // Return max duration for each request
        int get_max_duration_per_request()
        {
           if(config_mapping.find("max_duration_per_req") !=
                                                    config_mapping.end()) {
               return(stoi(config_mapping.find("max_duration_per_req")
                                                                    ->second));
           }
           return(default_max_duration_per_request);
        }
//-----------------------------------------------------------------------------
        // Return max step duration
        int get_max_step_duration()
        {
           if(config_mapping.find("max_step_duration") != config_mapping.end()) {
               return(stoi(config_mapping.find("max_step_duration")->second));
           }
           return(default_max_step_duration);
        }
//-----------------------------------------------------------------------------
        // Return distance bound length
        float get_distance_bound()
        {
           if(config_mapping.find("db_length") != config_mapping.end()) {
               return(stof(config_mapping.find("db_length")->second));
           }
           return(default_min_distance_bound);
        }
//-----------------------------------------------------------------------------
        // Return max apps report rate
        int get_max_report_cnt_per_sec()
        {
           if(config_mapping.find("max_apps_rpt_rate") !=
                                                    config_mapping.end()) {
               return(stoi(config_mapping.find("max_apps_rpt_rate")->second));
           }
           return(default_max_rpt_cnt_per_sec);
        }
//-----------------------------------------------------------------------------
        // Return max sampling rate
        float get_max_sampling_rate()
        {
            if(config_mapping.find("max_sampling_rate") != config_mapping.end()) {
               return(stof(config_mapping.find("max_sampling_rate")->second));
            }
            return(default_max_sampling_rate);
        }
//-----------------------------------------------------------------------------
        // Return usta_log status
        bool get_usta_log_status()
        {
            if(config_mapping.find("usta_log") != config_mapping.end()) {
               return(stoi(config_mapping.find("usta_log")->second));
            }
            return(default_usta_log);
        }
    } // end of namespace Parser
} // end of sensors_moast
//-----------------------------------------------------------------------------
