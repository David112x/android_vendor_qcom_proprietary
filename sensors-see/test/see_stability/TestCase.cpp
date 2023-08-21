/* ===================================================================
**
** Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: TestCase.cpp
** ================================================================ */

#include <iostream>  // std::cout
#include <sstream>
#include <map>       // std::map
#include <thread>    // std::thread, std::this_thread
#include <chrono>    // std::chrono
#include <vector>    // std::vector
#include <algorithm> // std::find
#include <future>    // std::future

#include "Utility.h"
#include "StabilityEngine.h"
#include "TestCase.h"
#include "Parser.h"

namespace sensors_stability {

/*========================================================================
 *              Static Variable initialization
 ========================================================================*/
#define DEBUGGING 0
const int TestCase::MICROSECONDS = 1000000;
const int TestCase::MILLISECONDS = 500;

std::vector<std::string> tc_specs;

// Client number. Default is 1
int TestCase::client_num  = 1;

//----------------------------------------------------------------------------

// Create a singleton object of the class. Parser class will use this object
// to invoke member function get_test_params
TestCase* TestCase::get_test_case_instance() {
    static TestCase test_case_instance;
    return &test_case_instance;
}

// Object of Utility class
Utility utility_obj;

// Object of StabliltyEngine
StabilityEngine* stability_engine_obj;

// Max or min or random rates
const float MAX_RATE = -1.00f;
const float MIN_RATE = -2.00f;
const float RAND_RATE = -3.00f;
//----------------------------------------------------------------------------
// *                    Non-member functions                                 *
//----------------------------------------------------------------------------

   std::string get_tc_spec( const TestCase::sensor_name_sensor_configs&
                            sensor_config_mapping,
                            std::string sensor_type)
   {
      std::string tc_spec = sensor_type;
      auto config_it = sensor_config_mapping.find( sensor_type);
      if ( config_it != sensor_config_mapping.end()) {
         auto spec_it = config_it->second.find("TC_SPEC");
         tc_spec = spec_it->second;
      }
      return tc_spec;
   }

   std::string get_tc_spec(const std::map<std::string, float>& test_params,
                           const std::string& sensor_type)
   {
       std::string tc_spec = sensor_type;
       size_t index = 0;
       index = (size_t)test_params.find(sensor_type + "$tc_spec_index")->second;

       if ( index < tc_specs.size()) {
          tc_spec = tc_specs[ index];
       }

       return tc_spec;
   }

//----------------------------------------------------------------------------
    // Print file and line number where exception occured
    void print_file_line_number(const char* file, int line)
    {
        std::cout << "Exception in file: " << file << ", " << "line number: "
                  << line << std::endl;
        std::cout << "FAIL" << std::endl;
    } // end print_file_line_number()
//----------------------------------------------------------------------------
    // Get sensor attrib name
    bool get_sensor_attrib_name(const std::map<std::string, float>& test_params,
                                            const std::string& sensor_type)
    {
        int attrib_name = 0;
        attrib_name = static_cast<int>(test_params.find(sensor_type +
                                                    "$sens_attrib_name")->second);
        return((attrib_name == 1) ? true : false);
    } // end of get_sensor_attrib_name()
//----------------------------------------------------------------------------
    // Return sensor name
     std::string get_sensor_name(const std::map<std::string, float>& test_params,
                                   const std::string& search_for)
     {
        std::map<std::string,float>::const_iterator itr = test_params.
                                                         upper_bound(search_for);
        std::string sensor_name = "";
        if(itr != test_params.end()) {
            const std::string& key = itr->first;
            if(key.compare(0, search_for.size(), search_for) == 0) {
                std::size_t pos = 0;
                const std::string DOLLAR_DELIMITER = "$";
                if((pos = key.find(DOLLAR_DELIMITER)) != std::string::npos) {
                    sensor_name = key.substr(pos+1);
                    return(sensor_name);
                }
            }
        }
        return(sensor_name);
     } // end of get_sensor_name()
//----------------------------------------------------------------------------

    // Return sample rate
    float get_sample_rate(const std::map<std::string, float>& test_params,
                                  const std::string& sensor_type)
    {
        float sample_rate = 0.0f;
        sample_rate = test_params.find(sensor_type + "$sample_rate")->second;
        return(sample_rate);
    } // end of get_sample_rate()
//----------------------------------------------------------------------------
    // Return min sample rate
    float get_min_sample_rate(const std::map<std::string, float>& test_params,
                                            const std::string& sensor_type)
    {
        float min_sample_rate = 0.0f;
        min_sample_rate = test_params.find(sensor_type + "$min_sample_rate")->
                                                                        second;
        return(min_sample_rate);
    } // end of get_min_sample_rate()
//----------------------------------------------------------------------------
    // Return max sample rate
    float get_max_sample_rate(const std::map<std::string, float>& test_params,
                                            const std::string& sensor_type)
    {
        float max_sample_rate = 0.0f;
        max_sample_rate = test_params.find(sensor_type + "$max_sample_rate")->
                                                                        second;
        return(max_sample_rate);
    } // end of get_max_sample_rate()
//----------------------------------------------------------------------------
    // Return odr rate
    float get_odr_rate(const std::map<std::string, float>& test_params,
                                  const std::string& sensor_type)
    {
        float odr_rate = 0.0f;
        odr_rate = test_params.find(sensor_type + "$odr_rate")->second;
        return(odr_rate);
    } // end of get_odr_rate()
//----------------------------------------------------------------------------
    // Return rand odr
    float get_rand_odr(const std::map<std::string, float>& test_params,
                                  const std::string& sensor_type)
    {
        float rand_odr = 0.0f;
        rand_odr = test_params.find(sensor_type + "$rand_odr_rt")->second;
        return(rand_odr);
    } // end of get_rand_odr()
//----------------------------------------------------------------------------
    // Return rigid_body string
    std::string get_rigid_body(const std::map<std::string, float>& test_params,
                               const std::string& sensor_type)
    {
        float index = test_params.find(sensor_type + "$rigid_body")->second;
        return stability_engine_obj->index_2_rigid_body(index);
    }
//----------------------------------------------------------------------------
    // Return stream duration
    float get_stream_duration(const std::map<std::string, float>& test_params,
                                  const std::string& sensor_type)
    {
        float stream_duration = 0.0f;
        stream_duration = test_params.find(sensor_type + "$stream_duration")->
                                                                        second;
        return(stream_duration);
    } // end of get_stream_duration()
//----------------------------------------------------------------------------
    // Return message id
    int get_msg_id(const std::map<std::string, float>& test_params,
                                  const std::string& sensor_type)
    {
        int msg_id = 0;
        msg_id = static_cast<int>(test_params.find(sensor_type +
                                                        "$msg_id")->second);
        return(msg_id);
    } // end of get_msg_id()
//----------------------------------------------------------------------------
    // Return batch period
    float get_batch_period(const std::map<std::string, float>& test_params,
                                  const std::string& sensor_type)
    {
        float batch_period = 0.0f;
        batch_period = test_params.find(sensor_type + "$batch_period")->second;
        return(batch_period);
    } // end of get_batch_period()
//----------------------------------------------------------------------------
    // Return min batch period
    float get_min_batch_period(const std::map<std::string, float>& test_params,
                                        const std::string& sensor_type)
    {
        float min_batch_period = 0.0f;
        min_batch_period = test_params.find(sensor_type + "$min_batch_period")->
                                                                        second;
        return(min_batch_period);
    } // end of get_min_batch_period()
//----------------------------------------------------------------------------
    // Return max batch period
    float get_max_batch_period(const std::map<std::string, float>& test_params,
                                        const std::string& sensor_type)
    {
        float max_batch_period = 0.0f;
        max_batch_period = test_params.find(sensor_type + "$max_batch_period")->
                                                                        second;
        return(max_batch_period);
    } // end of get_max_batch_period()
//----------------------------------------------------------------------------
    // Return report period
    float get_report_period(const std::map<std::string, float>& test_params,
                                  const std::string& sensor_type)
    {
        float report_period = 0.0f;
        report_period = test_params.find(sensor_type + "$report_period")->second;
        return(report_period);
    } // end of get_report_period()
//----------------------------------------------------------------------------
    // Return min report period
    float get_min_report_period(const std::map<std::string, float>& test_params,
                                        const std::string& sensor_type)
    {
        float min_report_period = 0.0f;
        min_report_period = test_params.find(sensor_type + "$min_report_period")
                                                                    ->second;
        return(min_report_period);
    } // end of get_min_report_period()
//----------------------------------------------------------------------------
    // Return max report period
    float get_max_report_period(const std::map<std::string, float>& test_params,
                                        const std::string& sensor_type)
    {
        float max_report_period = 0.0f;
        max_report_period = test_params.find(sensor_type + "$max_report_period")
                                                                    ->second;
        return(max_report_period);
    } // end of get_max_report_period()
//----------------------------------------------------------------------------
    // Return flush period
    float get_flush_period(const std::map<std::string, float>& test_params,
                                        const std::string& sensor_type)
    {
        float flush_period = 0.0f;
        flush_period = test_params.find(sensor_type + "$flush_period")->second;
        return(flush_period);
    } // end of get_flush_period()
//----------------------------------------------------------------------------
    // Return min flush period
    float get_min_flush_period(const std::map<std::string, float>& test_params,
                                        const std::string& sensor_type)
    {
        float min_flush_period = 0.0f;
        min_flush_period = test_params.find(sensor_type + "$min_flush_period")->
                                                                        second;
        return(min_flush_period);
    } // end of get_min_flush_period()
//----------------------------------------------------------------------------
    // Return max flush period
    float get_max_flush_period(const std::map<std::string, float>& test_params,
                                        const std::string& sensor_type)
    {
        float max_flush_period = 0.0f;
        max_flush_period = test_params.find(sensor_type + "$max_flush_period")->
                                                                        second;
        return(max_flush_period);
    } // end of get_max_flush_period()
//----------------------------------------------------------------------------
    // Return flush request interval
    float get_flush_request_interval(
                                        const std::map<std::string,
                                        float>& test_params,
                                        const std::string& sensor_type
                                    )
    {
        float flush_request_interval = 0.0f;
        flush_request_interval = test_params.find(sensor_type +
                                                "$flush_req_interval")->second;
        return(flush_request_interval);
    } // end of get_flush_request_interval()
//----------------------------------------------------------------------------
    // Return flush only
    bool get_flush_only(const std::map<std::string, float>& test_params,
                                            const std::string& sensor_type)
    {
        int flush_only = 0;
        flush_only = static_cast<int>(test_params.find(sensor_type +
                                                    "$flush_only")->second);
        return((flush_only == 1) ? true : false);
    } // end of get_flush_only()
//----------------------------------------------------------------------------
    // Return max batch
    bool get_max_batch(
                        const std::map<std::string, float>& test_params,
                        const std::string& sensor_type
                      )
    {
        int max_batch = 0;
        max_batch = static_cast<int>(test_params.find(sensor_type +
                                                   "$max_batch")-> second);
        return((max_batch == 1) ? true : false);
    } // end of get_max_batch()
//----------------------------------------------------------------------------
    // Return db length
    float get_db_length(const std::map<std::string, float>& test_params,
                                  const std::string& sensor_type)
    {
        float db_length = 0.0f;
        db_length = test_params.find(sensor_type + "$db_len")->second;
        return(db_length);
    } // end of get_db length()
//----------------------------------------------------------------------------
    // Return stream priority
    int get_stream_priority(const std::map<std::string, float>& test_params,
                                  const std::string& sensor_type)
    {
        int priority = 0;
        priority = static_cast<int>(test_params.find(sensor_type +
                                                        "$priority")->second);
        return(priority);
    } // end of get_stream_priority
//----------------------------------------------------------------------------
    // Return test type
    std::string get_test_type(const std::map<std::string, float>& test_params,
                                  const std::string& sensor_type)
    {
        int test_type = 1;
        test_type = static_cast<int>(test_params.find(sensor_type +
                                       "$ttype")->second);
        return((test_type == 1) ? "continuous" : "b2b");
    } // end of get_test_type
//----------------------------------------------------------------------------
    // Return disable priority
    int get_disable_priority(const std::map<std::string, float>& test_params,
                                  const std::string& sensor_type)
    {
        int disable_priority = 0;
        disable_priority = static_cast<int>(test_params.find(sensor_type +
                                                        "$disable_pri")->second);
        return(disable_priority);
    } // end of get_disable_priority
//----------------------------------------------------------------------------
    // Return delay before next disable req
    int get_delay_before_next_disable_req(const std::map<std::string, float>&
                                             test_params,
                                             const std::string& sensor_type)
    {
        int del_bef_next_disable_req = 0;
        del_bef_next_disable_req = static_cast<int>(test_params.find(sensor_type +
                                                     "$del_bef_next_disable_req")
                                                                 ->second);
        return(del_bef_next_disable_req);
    } // end of get_delay_before_next_disable_req
//----------------------------------------------------------------------------
    // Return resampler rate type
    int get_resampler_rate_type(const std::map<std::string,
                                        float>& test_params,
                                        const std::string& sensor_type)
    {
        int rate_type = 0;
        rate_type = static_cast<int>(test_params.find(sensor_type
                                           + "$resampler_rate_type")->second);
        return((rate_type == 1) ? 1 : 0);
    } // end of get_resampler_rate_type
//----------------------------------------------------------------------------
    // Return resampler rate
    float get_resampler_rate(const std::map<std::string, float>& test_params,
                                        const std::string& sensor_type)
    {
        float rate = 0.0f;
        rate = test_params.find(sensor_type + "$resampler_rate")->second;
        return(rate);
    } // end of get_resampler_rate
//----------------------------------------------------------------------------
    // Return min resampler rate
    float get_min_resampler_rate(const std::map<std::string, float>& test_params,
                                            const std::string& sensor_type)
    {
        float min_resampler_rate = 0.0f;
        min_resampler_rate = test_params.find(sensor_type +
                                                "$min_resampler_rate")->second;
        return(min_resampler_rate);
    } // end of get_min_resampler_rate()
//----------------------------------------------------------------------------
    // Return max resampler rate
    float get_max_resampler_rate(const std::map<std::string, float>& test_params,
                                            const std::string& sensor_type)
    {
        float max_resampler_rate = 0.0f;
        max_resampler_rate = test_params.find(sensor_type +
                                                "$max_resampler_rate")->second;
        return(max_resampler_rate);
    } // end of get_max_resampler_rate()
//----------------------------------------------------------------------------
    // Return resampler filter
    bool get_resampler_filter(const std::map<std::string, float>& test_params,
                                        const std::string& sensor_type)
    {
        int filter = 0;
        filter = static_cast<int>(test_params.find(sensor_type +
                                            "$resampler_filter")->second);
        return((filter == 1) ? true : false);
    } // end of get_resampler_filter
//----------------------------------------------------------------------------
    // Get client number
    int get_client_num(const std::map<std::string, float>& test_params,
                                            const std::string& sensor_type)
    {
        if(test_params.find(sensor_type + "$client") !=
                                            test_params.end()) {
            return(static_cast<int>(test_params.find (sensor_type +
                                                "$client")->second));
        }
        // return 1 as default if not found
        return(1);
    } // end of get_client_num()
//----------------------------------------------------------------------------
    // Get delay
    float get_delay(const std::map<std::string, float>& test_params,
                                            const std::string& sensor_type)
    {
          float delay = 0.0f;
          delay = test_params.find(sensor_type +  "$delay_before_req")->second;
          return(delay);
    } // end of get_delay()
//----------------------------------------------------------------------------
    // Return min delay
    float get_min_delay(const std::map<std::string, float>& test_params,
                                        const std::string& sensor_type)
    {
        float min_delay = 0.0f;
        min_delay = test_params.find(sensor_type + "$min_delay_before_req")->second;
        return(min_delay);
    } // end of get_min_delay()
//----------------------------------------------------------------------------
    // Return max delay
    float get_max_delay(const std::map<std::string, float>& test_params,
                        const std::string& sensor_type
                       )
    {
        float max_delay = 0.0f;
        max_delay = test_params.find(sensor_type + "$max_delay_before_req")->second;
        return(max_delay);
    } // end of get_max_delay()
//----------------------------------------------------------------------------
    // Return min stream duration
    float get_min_stream_duration(const std::map<std::string, float>& test_params,
                                    const std::string& sensor_type
                                 )
    {
        float min_duration = 0.0f;
        min_duration = test_params.find(sensor_type + "$min_stream_dur")->second;
        return(min_duration);
    } // end of get_min_stream_duration
//----------------------------------------------------------------------------
    // Return max stream duration
    float get_max_stream_duration(const std::map<std::string, float>& test_params,
                                    const std::string& sensor_type
                                 )
    {
        float max_duration = 0.0f;
        max_duration = test_params.find(sensor_type + "$max_stream_dur")->second;
        return(max_duration);
    } // end of get_max_stream_duration
//----------------------------------------------------------------------------
    // Print random batch period message
    void print_random_batch_period_message(
                                            const std::string& sensor_type,
                                            const float& random_batch_period
                                          )
    {
        long int microseconds = utility_obj.get_microseconds();
        std::cout << "\nRandom batch period of ";
        std::cout << sensor_type << ": ";
        std::cout << random_batch_period * microseconds;
        std::cout << " <microseconds>";
    }
//----------------------------------------------------------------------------
    // Print random report period message
    void print_random_report_period_message(
                                            const std::string& sensor_type,
                                            const float& random_report_period
                                          )
    {
        long int microseconds = utility_obj.get_microseconds();
        std::cout << "\nRandom report period of ";
        std::cout << sensor_type << ": ";
        std::cout << random_report_period * microseconds;
        std::cout << " <microseconds>";
    }
//----------------------------------------------------------------------------
    // Print random flush period message
    void print_random_flush_period_message(
                                            const std::string& sensor_type,
                                            const float& random_flush_period
                                          )
    {
        long int microseconds = utility_obj.get_microseconds();
        std::cout << "\nRandom flush period of ";
        std::cout << sensor_type << ": ";
        std::cout << random_flush_period * microseconds;
        std::cout << " <microseconds>";
    }
//----------------------------------------------------------------------------
    // Print random sample rate message
    void print_random_sample_rate_message(
                                            const std::string& sensor_type,
                                            const float& random_sample_rate
                                          )
    {
        std::cout << "\nRandom sample rate of ";
        std::cout << sensor_type << ": ";
        std::cout << random_sample_rate;
        std::cout << " Hz";
    }
//----------------------------------------------------------------------------
    // Print random resampler rate message
    void print_random_resampler_rate_message(
                                              const std::string& sensor_type,
                                              const float& random_resampler_rate
                                            )
    {
        std::cout << "\nRandom resampler rate of ";
        std::cout << sensor_type << ": ";
        std::cout << random_resampler_rate;
        std::cout << " Hz";
    }
//----------------------------------------------------------------------------
    // Initialize std message request
    void init_std_msg_request(see_std_request& std_request,
                                float batch_period,
                                float flush_period,
                                bool flush_only,
                                bool max_batch
                              )
    {
        if(batch_period) {
            std_request.set_batch_period(batch_period * TestCase::MICROSECONDS);
        }
        if(flush_period) {
            std_request.set_flush_period(flush_period * TestCase::MICROSECONDS);
        }
        if(flush_only) {
            std_request.set_flush_only(flush_only);
        }
        if(max_batch) {
            std_request.set_max_batch(max_batch);
        }
    }
//----------------------------------------------------------------------------
    // Stream Sensor
    salt_rc stream_sensor(see_salt* psalt, sens_uid* target_sensor_suid,
                             const std::string& sensor_type,
                             const float& sample_rate, const float& batch_period,
                             const float& flush_period, const bool& flush_only,
                             const bool& max_batch)
    {
        std::mutex m;
        std::lock_guard<std::mutex> lock(m);

        salt_rc rc = SALT_RC_FAILED;
        see_std_request std_request;
        see_msg_id_e msg_id_config = MSGID_STD_SENSOR_CONFIG;

        std::ostringstream oss;
        utility_obj.create_anchor_msg( oss, "enable", sensor_type, psalt);
        utility_obj.append_extras( oss, sample_rate, batch_period, flush_period,
                                   flush_only, max_batch);
        utility_obj.write_line(oss);

        init_std_msg_request(std_request, batch_period, flush_period,
                             flush_only, max_batch);
        // Send std request
        rc = utility_obj.std_msg_req(psalt,target_sensor_suid, msg_id_config,
                                     std_request, sample_rate);
        std::ostringstream xss;
        utility_obj.create_anchor_msg( xss, "enable", sensor_type, psalt);
        utility_obj.append_rc( xss, rc);
        utility_obj.write_line(xss);

        return(rc);
    } // end of stream_sensor()
//----------------------------------------------------------------------------
    // Resample sensor
    salt_rc resample_sensor(see_salt* psalt, sens_uid* resampler_suid,
                               sens_uid* target_sensor_suid,
                               const int& resampler_rate_type,
                               const std::string& sensor_type,
                               const float& resampler_rate,
                               const bool& filter, const float& batch_period,
                               const float& flush_period, const bool& flush_only,
                               const bool& max_batch)
    {
        salt_rc rc = SALT_RC_FAILED;
        see_std_request std_request;
        see_msg_id_e msg_id_config = MSGID_RESAMPLER_CONFIG;
        see_resampler_rate rate_type = SEE_RESAMPLER_RATE_FIXED; // default
        std::string res_rate_type = "fixed";

        if(resampler_rate_type) { //resampler_rate_type == 1
            res_rate_type = "minimum";
            rate_type = SEE_RESAMPLER_RATE_MINIMUM;
        }

        std::ostringstream oss;
        std::string display_sensor_type = "resampler." + sensor_type;
        utility_obj.create_anchor_msg( oss, "enable", display_sensor_type, psalt);
        utility_obj.append_resampler( oss, resampler_rate, res_rate_type, filter);
        utility_obj.append_extras( oss, 0, batch_period, flush_period,
                                   flush_only, max_batch);
        utility_obj.write_line(oss);

        init_std_msg_request(std_request, batch_period, flush_period,
                                    flush_only, max_batch);
        // Send resample request
        rc = utility_obj.resampler_msg_req(psalt, resampler_suid,
                                            target_sensor_suid,
                                            msg_id_config, std_request,
                                            resampler_rate, rate_type, filter
                                          );
        std::ostringstream xss;
        utility_obj.create_anchor_msg( xss, "enable", display_sensor_type, psalt);
        utility_obj.append_rc( xss, rc);
        utility_obj.write_line(xss);

        return(rc);
    }// end of resample_sensor
//----------------------------------------------------------------------------
    // On-change sensor
    salt_rc on_change_sensor(see_salt* psalt, sens_uid* target_sensor_suid,
                                sensor* psensor, std::string& sensor_type,
                                const float& batch_period,
                                const float& flush_period,
                                const bool& flush_only, const bool& max_batch)
    {
        std::mutex m;
        std::lock_guard<std::mutex> lock(m);
        salt_rc rc = SALT_RC_FAILED;
        see_std_request std_request;

        std::ostringstream oss;
        utility_obj.create_anchor_msg( oss, "enable", sensor_type, psalt);
        utility_obj.append_extras( oss, 0, batch_period, flush_period,
                                   flush_only, max_batch);
        utility_obj.write_line(oss);

        if("basic_gestures" == psensor->get_type()) {
            see_msg_id_e msg_id_config = MSGID_BASIC_GESTURES_CONFIG;
            init_std_msg_request(std_request, batch_period,
                                                 flush_period, flush_only,
                                                 max_batch);
            rc = utility_obj.basic_gestures_req(psalt, target_sensor_suid,
                                                    msg_id_config, std_request);
        }
        else if("psmd" == psensor->get_type()) {
                  see_msg_id_e msg_id_config = MSGID_PSMD_CONFIG;
                  init_std_msg_request(std_request, batch_period, flush_period,
                                            flush_only, max_batch);
                  rc = utility_obj.psmd_req(psalt, target_sensor_suid,
                                                msg_id_config, std_request);
        }
        else if("multishake" == psensor->get_type()) {
                  see_msg_id_e msg_id_config = MSGID_MULTISHAKE_CONFIG;
                  init_std_msg_request(std_request, batch_period,
                                              flush_period, flush_only,
                                              max_batch);
                  rc = utility_obj.multishake_req(psalt, target_sensor_suid,
                                                         msg_id_config,
                                                         std_request);
        }
        else {
                 see_msg_id_e msg_id_config = MSGID_ON_CHANGE_CONFIG;
                 init_std_msg_request(std_request, batch_period,
                                             flush_period, flush_only,
                                             max_batch);
                 rc = utility_obj.on_change_msg_req(psalt, target_sensor_suid,
                                                      msg_id_config, std_request);
        }

        std::ostringstream xss;
        utility_obj.create_anchor_msg( xss, "enable", sensor_type, psalt);
        utility_obj.append_rc( xss, rc);
        utility_obj.write_line(xss);

        return(rc);
    } // end of on_change_sensor
//----------------------------------------------------------------------------
    // DB sensor
    salt_rc db_sensor(see_salt* psalt, sens_uid* target_sensor_suid,
                        const std::string& sensor_type,
                        const float& db_length, const float& batch_period,
                        const float& flush_period, const bool& flush_only,
                        const bool& max_batch)
    {
        salt_rc rc = SALT_RC_FAILED;
        see_std_request std_request;
        see_msg_id_e msg_id_config = MSGID_SET_DISTANCE_BOUND;
        init_std_msg_request(std_request, batch_period, flush_period, flush_only,
                                                                    max_batch);
        rc = utility_obj.distance_bound_req(psalt, target_sensor_suid,
                                             msg_id_config, std_request,
                                             db_length);
        return(rc);
    } // end of db_sensor
//----------------------------------------------------------------------------
    static std::string get_sensor_description( std::string sensor_type,
                                               std::string req_type,
                                               std::string rigid_body)
    {
        std::string desc = "suid not found for: " + sensor_type + " ";
        desc = desc + "req_type=" + req_type;
        if ( rigid_body != "") {
            desc = desc + " rigid_body=" + rigid_body;
        }
        return desc;
    }
//----------------------------------------------------------------------------
    // Stability single client test
    void run_stability(const TestCase::sensor_name_sensor_configs&
                       sensor_config_mapping)
    {
        std::map<std::string, float>test_params;
        // Iterator to test_params
        std::map<std::string, float>::iterator it;

        // Local variables default initialization
        float stream_duration = 0.0f;
        float sample_rate  = 0.0f;
        float min_sample_rate = 0.0f;
        float max_sample_rate = 0.0f;
        float random_sample_rate = 0.0f;
        float resampler_rate = 0.0f;
        float min_resampler_rate = 0.0f;
        float max_resampler_rate = 0.0f;
        float random_resampler_rate = 0.0f;
        float odr_rate = 0.0f;
        float batch_period = 0.0f;
        float min_batch_period = 0.0f;
        float max_batch_period = 0.0f;
        float random_batch_period = 0.0f;
        float report_period = 0.0f;
        float min_report_period = 0.0f;
        float max_report_period = 0.0f;
        float random_report_period = 0.0f;
        float flush_period = 0.0f;
        float min_flush_period = 0.0f;
        float max_flush_period = 0.0f;
        float random_flush_period = 0.0f;
        float db_length = 0.0f;

        std::string sensor_name = "";
        std::string req_type = "";
        std::string rigid_body = "display"; // default

        int resampler_rate_type = 0;
        int req_type_msg_id = 0;

        bool flush_only = false;
        bool filter = false;
        bool max_batch = false;

        salt_rc rc = SALT_RC_FAILED;
        stability_engine_obj->process_test_params(sensor_config_mapping,
                                                  test_params,
                                                  tc_specs);

        it = test_params.begin();
        // Get usta_log_status
        bool usta_log_status = Parser::get_usta_log_status();
        set_usta_logging(usta_log_status);

        // Get instance
        see_salt* psalt = see_salt::get_instance();
        // Required by see_salt
        char* argv[] = {NULL};
        if(psalt == nullptr) {
            std::cout << "psalt not instantiated" << std::endl;
            print_file_line_number(__FILE__, __LINE__);
            //throw "psalt not instantiated.\n";
            return;
        }
        psalt->begin(0, argv);
        // Check if sensor config is present
        if(it != test_params.end()) {
            std::string sensor_type = (*it).first;

            if(!sensor_type.empty()) {
                sens_uid* target_sensor_suid = nullptr;
                sens_uid resampler_suid;

                // Get sensor attrib name
                if(get_sensor_attrib_name(test_params, sensor_type)) {
                    sensor_name = get_sensor_name(test_params, sensor_type);
                }

                if(test_params.find(sensor_type + "$rigid_body") !=
                                                test_params.end()) {
                    rigid_body = get_rigid_body(test_params, sensor_type);
                }

                // Get stream duration
                if(test_params.find(sensor_type + "$stream_duration") !=
                                                test_params.end()) {
                    stream_duration = get_stream_duration(test_params,
                                                            sensor_type);
                }

                // Get message id
                if(test_params.find(sensor_type + "$msg_id") !=
                                                test_params.end()) {
                    req_type_msg_id = get_msg_id(test_params, sensor_type);
                }

                // Get batch period
                if(test_params.find(sensor_type + "$batch_period") !=
                                               test_params.end()) {
                    batch_period = get_batch_period(test_params, sensor_type);
                }

                // Get report period
                if(test_params.find(sensor_type + "$report_period") !=
                                               test_params.end()) {
                    report_period = get_report_period(test_params, sensor_type);
                }

                // Get flush period
                if(test_params.find(sensor_type + "$flush_period") !=
                                              test_params.end()) {
                    flush_period = get_flush_period(test_params, sensor_type);
                }

                // Get max batch
                if(test_params.find(sensor_type + "$max_batch") !=
                                            test_params.end()) {
                    max_batch = get_max_batch(test_params, sensor_type);
                }

                // Get db length
                if(test_params.find(sensor_type + "$db_len") !=
                                               test_params.end()) {
                    db_length = get_db_length(test_params, sensor_type);
                }

                // Get resampler rate
                if(test_params.find(sensor_type + "$resampler_rate") !=
                                            test_params.end()) {
                    resampler_rate = get_resampler_rate(test_params, sensor_type);
                }

                // Get resampler rate type
                if(test_params.find(sensor_type + "$resampler_rate_type") !=
                                                       test_params.end()) {
                    resampler_rate_type = get_resampler_rate_type(test_params,
                                                                    sensor_type);
                }

                // Get resampler filter - true or false
                if(test_params.find(sensor_type + "$resampler_filter") !=
                                                test_params.end()) {
                    filter = get_resampler_filter(test_params, sensor_type);
                }

                // Get flush only
                if(test_params.find(sensor_type + "$flush_only") !=
                                                test_params.end()) {
                    flush_only = get_flush_only(test_params, sensor_type);
                }

                // Get min batch period
                if(test_params.find(sensor_type + "$min_batch_period") !=
                                             test_params.end()) {
                    min_batch_period = get_min_batch_period(test_params,
                                                                sensor_type);
                }

                // Get max batch period
                if(test_params.find(sensor_type + "$max_batch_period") !=
                                             test_params.end()) {
                    max_batch_period = get_max_batch_period(test_params,
                                                                sensor_type);
                }

                // Get min report period
                if(test_params.find(sensor_type + "$min_report_period") !=
                                             test_params.end()) {
                    min_report_period = get_min_report_period(test_params,
                                                          sensor_type);
                }

                // Get max report period
                if(test_params.find(sensor_type + "$max_report_period") !=
                                             test_params.end()) {
                    max_report_period = get_max_report_period(test_params,
                                                          sensor_type);
                }

                // Get min flush period
                if(test_params.find(sensor_type + "$min_flush_period") !=
                                             test_params.end()) {
                    min_flush_period = get_min_flush_period(test_params,
                                                                sensor_type);
                }

                // Get max flush period
                if(test_params.find(sensor_type + "$max_flush_period") !=
                                             test_params.end()) {
                    max_flush_period = get_max_flush_period(test_params,
                                                                sensor_type);
                }

                // Get sample rate
                if(test_params.find(sensor_type + "$sample_rate") !=
                                           test_params.end()) {
                    sample_rate = get_sample_rate(test_params, sensor_type);
                }

                // Get odr rate
                if(test_params.find(sensor_type + "$odr_rate") !=
                                                        test_params.end()) {
                    odr_rate = get_odr_rate(test_params, sensor_type);
                }

                // Check request type
                if(req_type_msg_id == static_cast<int>(StabilityEngine::
                                                req_message_ids::
                                                MSGID_STD_SENSOR_CONFIG)) {
                    req_type = "std";
                }
                else if(req_type_msg_id == static_cast<int>(
                                                  StabilityEngine::
                                                  req_message_ids::
                                                  MSGID_RESAMPLER_CONFIG)) {
                        req_type = "res";
                        resampler_suid = utility_obj.get_resampler_suid(psalt);
                }
                else if(req_type_msg_id == static_cast<int>(
                                                StabilityEngine::
                                                req_message_ids::
                                                MSGID_ON_CHANGE_CONFIG)) {
                         req_type = "on_change";
                }
                else if(req_type_msg_id == static_cast<int>(
                                                StabilityEngine::
                                                req_message_ids::
                                                MSGID_SET_DISTANCE_BOUND)) {
                         req_type = "db";
                }
                else {
                        print_file_line_number(__FILE__, __LINE__);
                        throw "Invalid message request type\n";
                }
                // Get target sensor suid
                target_sensor_suid = utility_obj.get_target_sensor_suid(
                                                    psalt,
                                                    target_sensor_suid,
                                                    sensor_type, req_type, rigid_body,
                                                    sensor_name);
                if(target_sensor_suid == nullptr) {
                    std::string desc = "FAIL " + get_sensor_description(
                                                               sensor_type,
                                                               req_type,
                                                               rigid_body);
                    throw desc.c_str();
                    return;
                }
                sensor* psensor = psalt->get_sensor(target_sensor_suid);
                if(psensor == nullptr) {
                    std::cout << "psensor is nullptr" << std::endl;
                    print_file_line_number(__FILE__, __LINE__);
                    //throw "psensor is nullptr\n";
                    return;
                }
                if(sample_rate == MIN_RATE && psensor->has_rates()) {
                    sample_rate = utility_obj.get_min_odr_rate(psensor,
                                                                sensor_type);
                }
                if(sample_rate == MAX_RATE && psensor->has_max_rate()) {
                    sample_rate = utility_obj.get_max_odr_rate(psensor,
                                                            sensor_type);
                }
                if(resampler_rate == MIN_RATE && psensor->has_rates()) {
                    resampler_rate = utility_obj.get_min_odr_rate(psensor,
                                                                   sensor_type);
                }
                if(resampler_rate == MAX_RATE && psensor->has_max_rate()) {
                    resampler_rate = utility_obj.get_max_odr_rate(psensor,
                                                               sensor_type);
                }
                if(odr_rate == MIN_RATE && psensor->has_rates()) {
                    sample_rate = utility_obj.get_min_odr_rate(psensor,
                                                                sensor_type);
                }
                if(odr_rate == MAX_RATE && psensor->has_rates()) {
                    sample_rate = utility_obj.get_max_odr_rate(psensor,
                                                                sensor_type);
                }
                // Get min sample rate
                if(test_params.find(sensor_type + "$min_sample_rate") !=
                                            test_params.end()) {
                    min_sample_rate = get_min_sample_rate(test_params,
                                                            sensor_type);
                }

                // Get max sample rate
                if(test_params.find(sensor_type + "$max_sample_rate") !=
                                            test_params.end()) {
                    max_sample_rate = get_max_sample_rate(test_params,
                                                                sensor_type);
                }

                // Get min resampler rate
                if(test_params.find(sensor_type + "$min_resampler_rate") !=
                                            test_params.end()) {
                    min_resampler_rate = get_min_resampler_rate(test_params,
                                                                  sensor_type);
                }

                // Get max resampler rate
                if(test_params.find(sensor_type + "$max_resampler_rate") !=
                                            test_params.end()) {
                    max_resampler_rate =
                        get_max_resampler_rate(test_params,sensor_type);
                }

                // If random odr rate is requested
                if((sample_rate == RAND_RATE || odr_rate == RAND_RATE)
                   && psensor->has_rates()) {
                    sample_rate =
                        utility_obj.get_random_odr_rate(psensor,sensor_type);
                }
                if(resampler_rate == RAND_RATE && psensor->has_rates()) {
                    resampler_rate =
                        utility_obj.get_random_odr_rate(psensor,sensor_type);
                }
                // Get random batch period
                if(min_batch_period && max_batch_period) {
                    random_batch_period =
                        get_random_min_max(min_batch_period,max_batch_period);
                }
                // Get random report period
                if(min_report_period && max_report_period) {
                    random_report_period =
                        get_random_min_max(min_report_period,max_report_period);
                }
                // Get random flush period
                if(min_flush_period && max_flush_period){
                    random_flush_period =
                        get_random_min_max(min_flush_period,max_flush_period);
                }
                // Get random sample rate
                if(min_sample_rate && max_sample_rate) {
                    if(min_sample_rate == MIN_RATE
                       && max_sample_rate == MAX_RATE) {
                        min_sample_rate =
                            utility_obj.get_min_odr_rate(psensor,sensor_type);
                        max_sample_rate =
                            utility_obj.get_max_odr_rate(psensor,sensor_type);
                    }
                    random_sample_rate =
                        get_random_min_max(min_sample_rate,max_sample_rate);
                }
                // Get random resampler rate
                if(min_resampler_rate && max_resampler_rate) {
                    if(min_resampler_rate == MIN_RATE
                       && max_resampler_rate == MAX_RATE) {
                        min_resampler_rate =
                            utility_obj.get_min_odr_rate(psensor,sensor_type);
                        max_resampler_rate =
                            utility_obj.get_max_odr_rate(psensor,sensor_type);
                    }
                    random_resampler_rate =
                        get_random_min_max(min_resampler_rate,max_resampler_rate);
                }
                if(report_period) {
                    batch_period = report_period;
                }
                if(random_batch_period) {
                    print_random_batch_period_message(sensor_type,
                                                       random_batch_period);
                    batch_period = random_batch_period;
                }
                if(random_report_period) {
                    print_random_report_period_message(sensor_type,
                                                       random_report_period);
                    batch_period = random_report_period;
                }
                if(random_flush_period) {
                    print_random_flush_period_message(sensor_type,
                                                       random_flush_period);
                    flush_period = random_flush_period;
                }
                if(random_sample_rate) {
                    print_random_sample_rate_message(sensor_type,
                                                      random_sample_rate);
                    sample_rate = random_sample_rate;
                }
                if(random_resampler_rate) {
                    print_random_resampler_rate_message(sensor_type,
                                                          random_resampler_rate);
                    resampler_rate = random_resampler_rate;
                }

                std::string tc_spec = get_tc_spec( test_params, sensor_type);

                std::ostringstream oss;
                utility_obj.create_status_msg( oss, "Begin: ", tc_spec, psalt);
                utility_obj.write_line( oss);

                if(req_type == "std") {
                    rc = stream_sensor(psalt, target_sensor_suid, sensor_type,
                                          sample_rate, batch_period,
                                          flush_period, flush_only, max_batch);
                }
                else if(req_type == "res") {
                    rc = resample_sensor(psalt, &resampler_suid,
                                          target_sensor_suid, resampler_rate_type,
                                          sensor_type, resampler_rate, filter,
                                          batch_period, flush_period, flush_only,
                                          max_batch);
                }
                else if(req_type == "on_change") {
                         rc = on_change_sensor(psalt, target_sensor_suid,
                                                    psensor, sensor_type,
                                                    batch_period, flush_period,
                                                    flush_only, max_batch);
                }
                else if(req_type == "db") {
                         rc = db_sensor(psalt, target_sensor_suid, sensor_type,
                                          db_length, batch_period, flush_period,
                                          flush_only, max_batch);
                }
                else {
                       print_file_line_number(__FILE__, __LINE__);
                       throw "Invalid request type\n";
                }
                // Stream duration
                if(rc == SALT_RC_SUCCESS) {
                    utility_obj.stream_duration(stream_duration);

                    // Stop sensor streaming
                    std::string detail = sensor_type;
                    if(req_type == "res") {
                       detail = "resampler." + sensor_type;
                       target_sensor_suid = &resampler_suid;
                    }
                    rc = utility_obj.disable_sensor( psalt,
                                                     detail,
                                                     target_sensor_suid);
                    std::ostringstream xss;
                    bool grade = (rc == SALT_RC_SUCCESS);
                    utility_obj.create_grade_msg( xss, grade, tc_spec, psalt);
                    utility_obj.write_line( xss);
                }
                else {
                      print_file_line_number(__FILE__, __LINE__);
                      std::cout << sensor_type << " ";
                      throw "failed to stream\n";
                }
            }
        }
        else {
                print_file_line_number(__FILE__, __LINE__);
                throw "No sensor configuration found\n";
        }
        std::cout << "Calling see_salt destructor for";
        std::cout << " instance " << psalt << std::endl;
        delete psalt;
    } // end of run_stability
//----------------------------------------------------------------------------
    // Concurrency: Multi client/multi sensor tests
    void run_concurrency(const TestCase::sensor_name_sensor_configs&
                         sensor_config_mapping)
    {
        std::string sensor_name = "";
        std::string req_type = "";

        const std::string DOLLAR_DELIMITER = "$";
        int req_type_msg_id = 0;

        std::map<std::string, float>test_params;
        stability_engine_obj->process_test_params(sensor_config_mapping,
                                                  test_params,
                                                  tc_specs);
        // Get test duration
        float test_duration = Parser::get_test_duration();

        // Required by see_salt
        char* argv[] = {NULL};

        // Get number of QMI clients for the test
        int num_qmi_clients  = Parser::get_num_qmi_clients();

        // Vector pair of sensor and its suid. Sensors passed from config file
        // are populated to this vector
        std::vector<std::pair<std::string, sens_uid*>> sensor_suid_pair
                                                        [num_qmi_clients];

        // Vector pair of streamed sensor and its suid. Sensors whose requests
        // are streamed successfully are populated to this vector
        std::vector<std::pair<std::string, sens_uid*>> streamed_sens_suid_pair
                                                        [num_qmi_clients];

        // Get usta_log_status
        bool usta_log_status = Parser::get_usta_log_status();
        set_usta_logging(usta_log_status);

        // Number of qmi instances
        // see_salt* psalt[num_qmi_clients];
        const int max_qmi_clients = 6;
        see_salt* psalt[max_qmi_clients] = {nullptr};
        for(int i = 0; i < num_qmi_clients; ++i) {
            // Get see_salt instances
            psalt[i] = see_salt::get_instance();
            if(psalt[i] == nullptr) {
                std::cout << "psalt not instantiated." << std::endl;
                print_file_line_number(__FILE__, __LINE__);
                //throw "psalt not instantiated.\n";
                return;
            }
            psalt[i]->begin(0, argv);
        }
        // Iterator to test_params
        std::map<std::string, float>::iterator it;
        // Get sensor configs from multi-map
        sens_uid resampler_suid;
        for(it = test_params.begin(); it != test_params.end(); ++it) {
            sens_uid* target_sensor_suid = nullptr;
            if((*it).first.find_first_of(DOLLAR_DELIMITER) ==
                                         std::string::npos) {
                     std::string sensor_type = (*it).first;

                // Get the actual sensor name passed from text file
                std::string save_sensor_type = sensor_type;

                if(!sensor_type.empty()) {
                    // Get sensor attrib name
                    if(get_sensor_attrib_name(test_params, sensor_type)) {
                         sensor_name = get_sensor_name(test_params, sensor_type);
                    }

                    std::string rigid_body = "display"; // default
                    if(test_params.find(sensor_type + "$rigid_body") !=
                                                    test_params.end()) {
                        rigid_body = get_rigid_body(test_params, sensor_type);
                    }

                    if(test_params.find(sensor_type + "$msg_id") !=
                                                test_params.end()) {
                        req_type_msg_id = get_msg_id(test_params, sensor_type);
                    }

                    // Get client number associated with the sensor
                    TestCase::client_num = get_client_num(test_params,
                                                                sensor_type);

                    // If muliple sensors with same name are present
                    // digit is used to differentiate it (e.g. accel
                    // and accel2, accel3 etc.). Remove digit from the sensor
                    sensor_type.erase(std::remove_if(sensor_type.
                                    begin(), sensor_type.end(), &isdigit),
                                    sensor_type.end());

                    // Check request type
                    if(req_type_msg_id == static_cast<int>(
                                       StabilityEngine::req_message_ids::
                                       MSGID_STD_SENSOR_CONFIG)) {
                        req_type = "std";
                    }
                    else if(req_type_msg_id == static_cast<int>(
                                        StabilityEngine::req_message_ids::
                                        MSGID_ON_CHANGE_CONFIG)) {
                             req_type = "on_change";
                    }
                    else if(req_type_msg_id == static_cast<int>(
                                        StabilityEngine::req_message_ids::
                                        MSGID_SET_DISTANCE_BOUND)) {
                            req_type = "db";
                    }
                    else if(req_type_msg_id == static_cast<int>(
                                                  StabilityEngine::
                                                  req_message_ids::
                                                  MSGID_RESAMPLER_CONFIG)) {
                             req_type = "res";
                            if(psalt[TestCase::client_num - 1] == nullptr) {
                                std::cout << "psalt not instantiated."
                                          << std::endl;
                                print_file_line_number(__FILE__, __LINE__);
                                // throw "psalt not instantiated\n";
                                return;
                            }
                             resampler_suid = utility_obj.get_resampler_suid
                                              (
                                                psalt[TestCase::client_num - 1]
                                              );
                    }
                    else {
                            std::cout << "Invalid message request" << std::endl;
                            print_file_line_number(__FILE__, __LINE__);
                            // throw "Invalid message request\n";
                            return;
                    }
                    if(psalt[TestCase::client_num - 1] == nullptr) {
                        std::cout << "psalt not instantiated" << std::endl;
                        print_file_line_number(__FILE__, __LINE__);
                        //throw "psalt not instantiated\n";
                        return;
                    }
                    target_sensor_suid = utility_obj.get_target_sensor_suid
                                             (
                                                psalt[TestCase::client_num - 1],
                                                target_sensor_suid,
                                                save_sensor_type, req_type, rigid_body,
                                                sensor_name
                                             );
                    if(target_sensor_suid != nullptr) {
                       sensor_suid_pair[TestCase::client_num - 1]
                                       .push_back(std::make_pair(save_sensor_type,target_sensor_suid));
                      target_sensor_suid = nullptr;
                    }
                    else {
                        std::string desc = "Skip because "
                                           + get_sensor_description( sensor_type,
                                                                     req_type,
                                                                     rigid_body);
                        std::cout << desc << std::endl;
                    }
                }
                else {
                      std::cout << "No sensor configuration found" << std::endl;
                      print_file_line_number(__FILE__, __LINE__);
                      //throw "No sensor configuration found\n";
                      return;
                }
            }
        } // end of for loop

        // Get sensor type and target sensor suid from vector pair
        for(int i = 0; i < num_qmi_clients; ++i) {
             std::string sensor_type = "";
             sens_uid* target_sensor_suid = nullptr;

             for(std::vector<std::pair<std::string, sens_uid*>>::
                    const_iterator iter = sensor_suid_pair[i].begin();
                                            iter != sensor_suid_pair[i].
                                            end(); ++iter) {
                sensor_type = iter->first;
                target_sensor_suid = iter->second;
                if(target_sensor_suid == nullptr) {
                    std::cout << "target sensor suid not found" << std::endl;
                    print_file_line_number(__FILE__, __LINE__);
                    return;
                }

                // Local variables default initialization
                float sample_rate  = 0.0f;
                float min_sample_rate = 0.0f;
                float max_sample_rate = 0.0f;
                float random_sample_rate = 0.0f;
                float resampler_rate = 0.0f;
                float min_resampler_rate = 0.0f;
                float max_resampler_rate = 0.0f;
                float random_resampler_rate = 0.0f;
                float odr_rate = 0.0f;
                float batch_period = 0.0f;
                float min_batch_period = 0.0f;
                float max_batch_period = 0.0f;
                float random_batch_period = 0.0f;
                float report_period = 0.0f;
                float min_report_period = 0.0f;
                float max_report_period = 0.0f;
                float random_report_period = 0.0f;
                float flush_period = 0.0f;
                float min_flush_period = 0.0f;
                float max_flush_period = 0.0f;
                float random_flush_period = 0.0f;
                float db_length = 0.0f;
                float delay = 0.0f;

                int  resampler_rate_type = 0;
                bool flush_only = false;
                bool filter = false;
                bool max_batch = false;

                // Get message id
                if(test_params.find(sensor_type + "$msg_id") !=
                                                test_params.end()) {
                    req_type_msg_id = get_msg_id(test_params, sensor_type);
                }

                // Get sample rate
                if(test_params.find(sensor_type + "$sample_rate") !=
                                           test_params.end()) {
                    sample_rate = get_sample_rate(test_params, sensor_type);
                }

                // Get odr rate
                if(test_params.find(sensor_type + "$odr_rate") !=
                                                        test_params.end()) {
                    odr_rate = get_odr_rate(test_params, sensor_type);
                }

                // Get batch period of sensor, if exists
                if(test_params.find(sensor_type + "$batch_period") !=
                                               test_params.end()) {
                    batch_period = get_batch_period(test_params, sensor_type);
                }

                // Get report period of sensor, if exists
                if(test_params.find(sensor_type + "$report_period") !=
                                               test_params.end()) {
                    report_period = get_report_period(test_params, sensor_type);
                }

                // Get flush period
                if(test_params.find(sensor_type + "$flush_period") !=
                                              test_params.end()) {
                    flush_period = get_flush_period(test_params, sensor_type);
                }

                // Get max batch
                if(test_params.find(sensor_type + "$max_batch") !=
                                            test_params.end()) {
                    max_batch = get_max_batch(test_params,sensor_type);
                }

                // Get db length
                if(test_params.find(sensor_type + "$db_len") !=
                                               test_params.end()) {
                    db_length = get_db_length(test_params,sensor_type);
                }

                // Get resampler rate
                if(test_params.find(sensor_type + "$resampler_rate") !=
                                            test_params.end()) {
                    resampler_rate =
                        get_resampler_rate(test_params,sensor_type);
                }

                // Get resampler rate type - fixed or minimum
                if(test_params.find(sensor_type + "$resampler_rate_type") !=
                                                       test_params.end()) {
                    resampler_rate_type =
                        get_resampler_rate_type(test_params,sensor_type);
                }

                // Get resampler filter type - true or false
                if(test_params.find(sensor_type + "$resampler_filter") !=
                                                test_params.end()) {
                    filter = get_resampler_filter(test_params, sensor_type);
                }

                // Get flush only
                if(test_params.find(sensor_type + "$flush_only") !=
                                                test_params.end()) {
                    flush_only = get_flush_only(test_params, sensor_type);
                }

                // Get min batch period
                if(test_params.find(sensor_type + "$min_batch_period") !=
                                             test_params.end()) {
                    min_batch_period =
                        get_min_batch_period(test_params,sensor_type);
                }

                // Get max batch period
                if(test_params.find(sensor_type + "$max_batch_period") !=
                                             test_params.end()) {
                    max_batch_period =
                        get_max_batch_period(test_params,sensor_type);
                }

                // Get min report period
                if(test_params.find(sensor_type + "$min_report_period") !=
                                             test_params.end()) {
                    min_report_period
                        = get_min_report_period(test_params,sensor_type);
                }

                // Get max report period
                if(test_params.find(sensor_type + "$max_report_period") !=
                                             test_params.end()) {
                    max_report_period =
                        get_max_report_period(test_params,sensor_type);
                }

                // Get min flush period
                if(test_params.find(sensor_type + "$min_flush_period") !=
                                             test_params.end()) {
                    min_flush_period =
                        get_min_flush_period(test_params,sensor_type);
                }

                // Get max flush period
                if(test_params.find(sensor_type + "$max_flush_period") !=
                                             test_params.end()) {
                    max_flush_period =
                        get_max_flush_period(test_params,sensor_type);
                }

                // Get min sample rate
                if(test_params.find(sensor_type + "$min_sample_rate") !=
                                            test_params.end()) {
                    min_sample_rate =
                        get_min_sample_rate(test_params,sensor_type);
                }

                // Get max sample rate
                if(test_params.find(sensor_type + "$max_sample_rate") !=
                                            test_params.end()) {
                    max_sample_rate =
                        get_max_sample_rate(test_params,sensor_type);
                }

                // Get min resampler rate
                if(test_params.find(sensor_type + "$min_resampler_rate") !=
                                            test_params.end()) {
                    min_resampler_rate
                        = get_min_resampler_rate(test_params,sensor_type);
                }

                // Get delay
                if(test_params.find(sensor_type + "$delay_before_req") !=
                                             test_params.end()) {
                    delay = get_delay(test_params, sensor_type);
                }

                // Get max resampler rate
                if(test_params.find(sensor_type + "$max_resampler_rate") !=
                                            test_params.end()) {
                    max_resampler_rate =
                        get_max_resampler_rate(test_params,sensor_type);
                }
                // Pointer to sensor target
                sensor* psensor = psalt[i]->get_sensor(target_sensor_suid);
                if(psensor == nullptr) {
                    std::cout << "psensor is nullptr" << std::endl;
                    print_file_line_number(__FILE__, __LINE__);
                    //throw "psensor is nullptr\n";
                    return;
                }
                if(sample_rate == MIN_RATE && psensor->has_rates()) {
                    sample_rate =
                        utility_obj.get_min_odr_rate(psensor,sensor_type);
                }
                if(sample_rate == MAX_RATE && psensor->has_max_rate()) {
                    sample_rate =
                        utility_obj.get_max_odr_rate(psensor,sensor_type);
                }
                if(resampler_rate == MIN_RATE && psensor->has_rates()) {
                    resampler_rate =
                        utility_obj.get_min_odr_rate(psensor,sensor_type);
                }
                if(resampler_rate == MAX_RATE && psensor->has_max_rate()) {
                    resampler_rate =
                        utility_obj.get_max_odr_rate(psensor,sensor_type);
                }
                if(odr_rate == MIN_RATE && psensor->has_rates()) {
                    sample_rate =
                        utility_obj.get_min_odr_rate(psensor,sensor_type);
                }
                if(odr_rate == MAX_RATE && psensor->has_rates()) {
                    sample_rate =
                        utility_obj.get_max_odr_rate(psensor,sensor_type);
                }
                if((sample_rate == RAND_RATE || odr_rate == RAND_RATE)
                    && psensor->has_rates()) {
                    sample_rate =
                        utility_obj.get_random_odr_rate(psensor,sensor_type);
                }
                if(resampler_rate == RAND_RATE && psensor->has_rates()) {
                    resampler_rate =
                        utility_obj.get_random_odr_rate(psensor,sensor_type);
                }

                // Get random batch period
                if(min_batch_period && max_batch_period) {
                     random_batch_period =
                         get_random_min_max(min_batch_period,max_batch_period);
                }
                // Get random report period
                if(min_report_period && max_report_period) {
                     random_report_period =
                         get_random_min_max(min_report_period,max_report_period);
                }
                // Get random flush period
                if(min_flush_period && max_flush_period){
                     random_flush_period =
                         get_random_min_max(min_flush_period,max_flush_period);
                }
                // Get random sample rate
                if(min_sample_rate && max_sample_rate) {
                    if(min_sample_rate == MIN_RATE
                       && max_sample_rate == MAX_RATE) {
                        min_sample_rate =
                            utility_obj.get_min_odr_rate(psensor,sensor_type);
                        max_sample_rate =
                            utility_obj.get_max_odr_rate(psensor,sensor_type);
                    }
                    random_sample_rate =
                        get_random_min_max(min_sample_rate,max_sample_rate);
                }
                // Get random resampler rate
                if(min_resampler_rate && max_resampler_rate) {
                    if(min_resampler_rate == MIN_RATE
                       && max_resampler_rate == MAX_RATE) {
                        min_resampler_rate =
                            utility_obj.get_min_odr_rate(psensor,sensor_type);
                        max_resampler_rate =
                            utility_obj.get_max_odr_rate(psensor,sensor_type);
                    }
                    random_resampler_rate =
                        get_random_min_max(min_resampler_rate,max_resampler_rate);
                }
                // Check request type
                if(req_type_msg_id == static_cast<int>(
                                       StabilityEngine::req_message_ids::
                                       MSGID_STD_SENSOR_CONFIG)) {
                     req_type = "std";
                }
                else if(req_type_msg_id == static_cast<int>(
                                       StabilityEngine::req_message_ids::
                                       MSGID_ON_CHANGE_CONFIG)) {
                         req_type = "on_change";
                }
                else if(req_type_msg_id == static_cast<int>(
                                       StabilityEngine::req_message_ids::
                                       MSGID_SET_DISTANCE_BOUND)) {
                         req_type = "db";
                }
                else if(req_type_msg_id == static_cast<int>(
                                                  StabilityEngine::
                                                  req_message_ids::
                                                  MSGID_RESAMPLER_CONFIG)) {
                         req_type = "res";
                     //resampler_suid = utility_obj.get_resampler_suid(psalt[i]);
                }
                else {
                        std::cout << "Invalid message request " << std::endl;
                        std::cout << "FAIL" << std::endl;
                }
                if(report_period) {
                    batch_period = report_period;
                }
                if(random_batch_period) {
                    print_random_batch_period_message(sensor_type,
                                                      random_batch_period);
                    batch_period = random_batch_period;
                }
                if(random_report_period) {
                    print_random_report_period_message(sensor_type,
                                                       random_report_period);
                    batch_period = random_report_period;
                }
                if(random_flush_period) {
                    print_random_flush_period_message(sensor_type,
                                                      random_flush_period);
                    flush_period = random_flush_period;
                }
                if(random_sample_rate) {
                    print_random_sample_rate_message(sensor_type,
                                                     random_sample_rate);
                    sample_rate = random_sample_rate;
                }
                if(random_resampler_rate) {
                    print_random_resampler_rate_message(sensor_type,
                                                        random_resampler_rate);
                    resampler_rate = random_resampler_rate;
                }

                std::string tc_spec = get_tc_spec( test_params, sensor_type);
                std::ostringstream bss;
                utility_obj.create_status_msg( bss, "Begin: ", tc_spec, psalt[i]);
                utility_obj.write_line( bss);

                salt_rc rc = SALT_RC_FAILED;
                if(req_type == "std") {
                    rc = stream_sensor(psalt[i], target_sensor_suid, sensor_type,
                                          sample_rate, batch_period,
                                          flush_period, flush_only, max_batch);
                }
                else if(req_type == "res") {
                        rc = resample_sensor(psalt[i], &resampler_suid,
                                                target_sensor_suid,
                                                resampler_rate_type,
                                                sensor_type,
                                                resampler_rate, filter,
                                                batch_period,
                                                flush_period, flush_only,
                                                max_batch
                                            );
                }
                else if(req_type == "on_change") {
                        rc = on_change_sensor(psalt[i], target_sensor_suid,
                                                    psensor, sensor_type,
                                                    batch_period, flush_period,
                                                    flush_only, max_batch);
                }
                else if(req_type == "db") {
                         rc = db_sensor(psalt[i], target_sensor_suid, sensor_type,
                                          db_length, batch_period, flush_period,
                                          flush_only, max_batch);
                }
                else {
                       std::cout << "Invalid request type" << std::endl;
                }
                if(rc == SALT_RC_SUCCESS) {
                    // Add streamed sensor and its suid to vector
                    // only if it is not present for this client
                    const auto tmp = std::make_pair(sensor_type,
                                                        target_sensor_suid);
                    if(!std::any_of(streamed_sens_suid_pair[i].begin(),
                                    streamed_sens_suid_pair[i].end(),
                                    [&target_sensor_suid]
                                    (const std::pair<std::string, sens_uid*>& e)
                                    {return e.second == target_sensor_suid;})) {

                            streamed_sens_suid_pair[i].push_back(std::make_pair
                                                        (
                                                            sensor_type,
                                                            target_sensor_suid)
                                                        );
                    }
                }
                else {
                      std::cout << sensor_type << " failed to stream" << std::endl;
                      std::cout << "FAIL" << std::endl;
                }
                // If same sensors are streamed with same client, delay
                // is required
                if(delay) {
                    utility_obj.delay_duration(delay);
                    test_duration -= delay;
                }
            } // end of sensor_suid_pair for loop
        } // end of num_qmi_client for loop

        // Test duration
        utility_obj.stream_duration(test_duration);

        // Stop streaming
        salt_rc rc = SALT_RC_FAILED;
        for(int i = 0; i < num_qmi_clients; ++i) {
            for(std::vector<std::pair<std::string, sens_uid*>>::
                const_iterator iter = streamed_sens_suid_pair[i].begin();
                    iter != streamed_sens_suid_pair[i].end(); ++iter) {
                std::string sensor_type = iter->first;
                sens_uid* target_sensor_suid = iter->second;
                if(target_sensor_suid == nullptr) {
                    std::cout << "target sensor suid not found" << std::endl;
                    print_file_line_number(__FILE__, __LINE__);
                    return;
                }

                std::string detail = sensor_type;
                // Check if request type is resampler
                req_type_msg_id = get_msg_id(test_params, iter->first);
                if(req_type_msg_id == static_cast<int>(StabilityEngine::
                                                        req_message_ids::
                                                        MSGID_RESAMPLER_CONFIG)) {
                   detail = "resampler." + sensor_type;
                   target_sensor_suid = &resampler_suid;
                }

                rc = utility_obj.disable_sensor(psalt[i], detail, target_sensor_suid);

                std::ostringstream gss;
                bool grade = (rc == SALT_RC_SUCCESS);
                std::string tc_spec = get_tc_spec( test_params, sensor_type);
                utility_obj.create_grade_msg( gss, grade, tc_spec, psalt[i]);
                utility_obj.write_line( gss);
            }
        }
        for(int i = 0; i < num_qmi_clients; ++i) {
            std::cout << "Calling see_salt destructor for";
            std::cout << " instance " << psalt[i] << std::endl;
            delete psalt[i];
        }
    } // end of run_concurrency
//----------------------------------------------------------------------------
    // Back2Back test
    void run_b2b(const TestCase::sensor_name_sensor_configs&
                       sensor_config_mapping)
    {
        std::map<std::string, float>test_params;
        // Iterator to test_params
        std::map<std::string, float>::iterator it;

        // Get total test duration
        float test_duration = Parser::get_test_duration();
        // Local variables default initialization
        float stream_duration = 0.0f;
        float min_stream_dur = 0.0f;
        float max_stream_dur = 0.0f;
        float random_stream_dur = 0.0f;
        float sample_rate  = 0.0f;
        float min_sample_rate = 0.0f;
        float max_sample_rate = 0.0f;
        float random_sample_rate = 0.0f;
        float resampler_rate = 0.0f;
        float min_resampler_rate = 0.0f;
        float max_resampler_rate = 0.0f;
        float random_resampler_rate = 0.0f;
        float odr_rate = 0.0f;
        float batch_period = 0.0f;
        float min_batch_period = 0.0f;
        float max_batch_period = 0.0f;
        float random_batch_period = 0.0f;
        float report_period = 0.0f;
        float min_report_period = 0.0f;
        float max_report_period = 0.0f;
        float random_report_period = 0.0f;
        float flush_period = 0.0f;
        float min_flush_period = 0.0f;
        float max_flush_period = 0.0f;
        float random_flush_period = 0.0f;
        float db_length = 0.0f;
        float delay = 0.0f;
        float min_delay = 0.0f;
        float max_delay = 0.0f;
        float random_delay = 0.0f;

        std::string sensor_name = "";
        std::string sensor_type = "";
        std::string req_type = "";

        int resampler_rate_type = 0;
        int req_type_msg_id = 0;

        bool flush_only = false;
        bool filter = false;
        bool max_batch = false;

        salt_rc rc = SALT_RC_FAILED;
        stability_engine_obj->process_test_params(sensor_config_mapping,
                                                  test_params,
                                                  tc_specs);
        sens_uid* target_sensor_suid = nullptr;
        sens_uid resampler_suid;
        sensor* psensor = nullptr;

        // Get usta_log_status
        bool usta_log_status = Parser::get_usta_log_status();
        set_usta_logging(usta_log_status);

        // Get instance
        see_salt* psalt = see_salt::get_instance();
        // Required by see_salt
        char* argv[] = {NULL};
        if(psalt == nullptr) {
            std::cout << "psalt not instantiated" << std::endl;
            print_file_line_number(__FILE__, __LINE__);
            // throw "psalt not instantiated.\n";
            return;
        }
        psalt->begin(0, argv);

        it = test_params.begin();
        // Check if sensor config is present
        if(it != test_params.end()) {
            sensor_type = (*it).first;

            if(!sensor_type.empty()) {
                // Get sensor attrib name
                if(get_sensor_attrib_name(test_params, sensor_type)) {
                    sensor_name = get_sensor_name(test_params, sensor_type);
                }
                std::string rigid_body = "display"; // default
                if(test_params.find(sensor_type + "$rigid_body") !=
                                                test_params.end()) {
                    rigid_body = get_rigid_body(test_params, sensor_type);
                }

                // Get stream duration
                if(test_params.find(sensor_type + "$stream_duration") !=
                                                test_params.end()) {
                    stream_duration = get_stream_duration(test_params,
                                                            sensor_type);
                }

                // Get message id
                if(test_params.find(sensor_type + "$msg_id") !=
                                                test_params.end()) {
                    req_type_msg_id = get_msg_id(test_params, sensor_type);
                }

                // Get batch period
                if(test_params.find(sensor_type + "$batch_period") !=
                                               test_params.end()) {
                    batch_period = get_batch_period(test_params, sensor_type);
                }

                // Get report period
                if(test_params.find(sensor_type + "$report_period") !=
                                               test_params.end()) {
                    report_period = get_report_period(test_params, sensor_type);
                }

                // Get flush period
                if(test_params.find(sensor_type + "$flush_period") !=
                                              test_params.end()) {
                    flush_period = get_flush_period(test_params, sensor_type);
                }

                // Get max batch
                if(test_params.find(sensor_type + "$max_batch") !=
                                            test_params.end()) {
                    max_batch = get_max_batch(test_params, sensor_type);
                }

                // Get db length
                if(test_params.find(sensor_type + "$db_len") !=
                                               test_params.end()) {
                    db_length = get_db_length(test_params, sensor_type);
                }

                // Get sample rate
                if(test_params.find(sensor_type + "$sample_rate") !=
                                           test_params.end()) {
                    sample_rate = get_sample_rate(test_params, sensor_type);
                }

                // Get odr rate
                if(test_params.find(sensor_type + "$odr_rate") !=
                                                        test_params.end()) {
                    odr_rate = get_odr_rate(test_params, sensor_type);
                }

                // Get resampler rate
                if(test_params.find(sensor_type + "$resampler_rate") !=
                                            test_params.end()) {
                    resampler_rate = get_resampler_rate(test_params, sensor_type);
                }

                // Get resampler rate type - fixed or minimum
                if(test_params.find(sensor_type + "$resampler_rate_type") !=
                                                       test_params.end()) {
                    resampler_rate_type = get_resampler_rate_type(test_params,
                                                                   sensor_type);
                }

                // Get resampler filter - true or false
                if(test_params.find(sensor_type + "$resampler_filter") !=
                                                test_params.end()) {
                    filter = get_resampler_filter(test_params, sensor_type);
                }

                // Get flush only
                if(test_params.find(sensor_type + "$flush_only") !=
                                                test_params.end()) {
                    flush_only = get_flush_only(test_params, sensor_type);
                }

                // Get min batch period
                if(test_params.find(sensor_type + "$min_batch_period") !=
                                             test_params.end()) {
                    min_batch_period = get_min_batch_period(test_params,
                                                                sensor_type);
                }

                // Get max batch period
                if(test_params.find(sensor_type + "$max_batch_period") !=
                                             test_params.end()) {
                    max_batch_period = get_max_batch_period(test_params,
                                                                sensor_type);
                }

                // Get min report period
                if(test_params.find(sensor_type + "$min_report_period") !=
                                             test_params.end()) {
                    min_report_period = get_min_report_period(test_params,
                                                          sensor_type);
                }

                // Get max report period
                max_report_period = get_max_report_period(test_params,
                                                          sensor_type);

                // Get min flush period
                if(test_params.find(sensor_type + "$min_flush_period") !=
                                             test_params.end()) {
                    min_flush_period = get_min_flush_period(test_params,
                                                                sensor_type);
                }

                // Get max flush period
                if(test_params.find(sensor_type + "$max_flush_period") !=
                                             test_params.end()) {
                    max_flush_period = get_max_flush_period(test_params,
                                                                sensor_type);
                }

                // Get delay
                if(test_params.find(sensor_type + "$delay_before_req") !=
                                             test_params.end()) {
                    delay = get_delay(test_params, sensor_type);
                }

                // Get min delay
                if(test_params.find(sensor_type + "$min_delay_before_req") !=
                                             test_params.end()) {
                    min_delay = get_min_delay(test_params, sensor_type);
                }

                // Get max delay
                if(test_params.find(sensor_type + "$max_delay_before_req") !=
                                             test_params.end()) {
                    max_delay = get_max_delay(test_params, sensor_type);
                }

                // Get random delay
                // min delay could be 0, hence use max_delay in if block
                if(max_delay) {
                   random_delay = get_random_min_max(min_delay,max_delay);
                   delay = random_delay;
                }

                // Get min stream duration
                if(test_params.find(sensor_type + "$min_stream_dur") !=
                                             test_params.end()) {
                    min_stream_dur =
                        get_min_stream_duration(test_params,sensor_type);
                }

                // Get max stream duration
                if(test_params.find(sensor_type + "$max_stream_dur") !=
                                             test_params.end()) {
                    max_stream_dur = get_max_stream_duration(test_params,
                                                             sensor_type);
                }

                // Check request type
                if(req_type_msg_id == static_cast<int>(StabilityEngine::
                                                req_message_ids::
                                                MSGID_STD_SENSOR_CONFIG)) {
                   req_type = "std";
                }
                else if(req_type_msg_id == static_cast<int>(
                                                StabilityEngine::
                                                req_message_ids::
                                                MSGID_ON_CHANGE_CONFIG)) {
                            req_type = "on_change";
                }
                else if(req_type_msg_id == static_cast<int>(
                                                StabilityEngine::
                                                req_message_ids::
                                                MSGID_SET_DISTANCE_BOUND)) {
                            req_type = "db";
                }
                else if(req_type_msg_id == static_cast<int>(
                                                  StabilityEngine::
                                                  req_message_ids::
                                                  MSGID_RESAMPLER_CONFIG)) {
                              req_type = "res";
                              resampler_suid = utility_obj.get_resampler_suid(
                                                                        psalt);
                }
                else {
                          print_file_line_number(__FILE__, __LINE__);
                          throw "Invalid message request type\n";
                }
                // Get target sensor suid
                target_sensor_suid = utility_obj.get_target_sensor_suid(
                                                    psalt,
                                                    target_sensor_suid,
                                                    sensor_type, req_type, rigid_body,
                                                    sensor_name);
                if(target_sensor_suid == nullptr) {
                    std::string desc = "FAIL " + get_sensor_description(
                                                               sensor_type,
                                                               req_type,
                                                               rigid_body);
                    throw desc.c_str();
                    return;
                }
                psensor = psalt->get_sensor(target_sensor_suid);
                if(psensor == nullptr) {
                    std::cout << "psensor is nullptr" << std::endl;
                    print_file_line_number(__FILE__, __LINE__);
                    //throw "psensor is nullptr\n";
                    return;
                }
                if(sample_rate == MAX_RATE && psensor->has_max_rate()) {
                    sample_rate = utility_obj.get_max_odr_rate(psensor,
                                                            sensor_type);
                }
                if(sample_rate == MIN_RATE && psensor->has_rates()) {
                    sample_rate = utility_obj.get_min_odr_rate(psensor,
                                                                sensor_type);
                }
                if(resampler_rate == MIN_RATE && psensor->has_rates()) {
                    resampler_rate = utility_obj.get_min_odr_rate(psensor,
                                                                   sensor_type);
                }
                if(resampler_rate == MAX_RATE && psensor->has_max_rate()) {
                    resampler_rate = utility_obj.get_max_odr_rate(psensor,
                                                               sensor_type);
                }
                if(odr_rate == MIN_RATE && psensor->has_rates()) {
                    sample_rate = utility_obj.get_min_odr_rate(psensor,
                                                                sensor_type);
                }
                if(odr_rate == MAX_RATE && psensor->has_rates()) {
                    sample_rate = utility_obj.get_max_odr_rate(psensor,
                                                                sensor_type);
                }
                // Get min sample rate
                if(test_params.find(sensor_type + "$min_sample_rate") !=
                                            test_params.end()) {
                    min_sample_rate = get_min_sample_rate(test_params,
                                                            sensor_type);
                }

                // Get max sample rate
                if(test_params.find(sensor_type + "$max_sample_rate") !=
                                            test_params.end()) {
                    max_sample_rate =
                        get_max_sample_rate(test_params,sensor_type);
                }

                // If random odr rate is requested
                if((sample_rate == RAND_RATE || odr_rate == RAND_RATE)
                    && psensor->has_rates()) {
                    sample_rate =
                        utility_obj.get_random_odr_rate(psensor,sensor_type);
                }
                if(resampler_rate == RAND_RATE && psensor->has_rates()) {
                    resampler_rate =
                        utility_obj.get_random_odr_rate(psensor,sensor_type);
                }

                // Get random batch period
                if(min_batch_period && max_batch_period) {
                    random_batch_period =
                        get_random_min_max(min_batch_period,max_batch_period);
                }
                // Get random report period
                if(min_report_period && max_report_period) {
                    random_report_period =
                        get_random_min_max(min_report_period,max_report_period);
                }
                // Get random flush period
                if(min_flush_period && max_flush_period){
                    random_flush_period =
                        get_random_min_max(min_flush_period,max_flush_period);
                }
                // Get random sample rate
                if(min_sample_rate && max_sample_rate) {
                    if(min_sample_rate == MIN_RATE
                       && max_sample_rate == MAX_RATE) {
                        min_sample_rate =
                            utility_obj.get_min_odr_rate(psensor,sensor_type);
                        max_sample_rate =
                            utility_obj.get_max_odr_rate(psensor,sensor_type);
                    }
                    random_sample_rate =
                        get_random_min_max(min_sample_rate,max_sample_rate);
                }
                // Get random resampler rate
                if(min_resampler_rate && max_resampler_rate) {
                    if(min_resampler_rate == MIN_RATE
                       && max_resampler_rate == MAX_RATE) {
                        min_resampler_rate =
                            utility_obj.get_min_odr_rate(psensor,sensor_type);
                        max_resampler_rate =
                            utility_obj.get_max_odr_rate(psensor,sensor_type);
                    }
                    random_resampler_rate =
                        get_random_min_max(min_resampler_rate,max_resampler_rate);
                }
            }
            else {
                    print_file_line_number(__FILE__, __LINE__);
                    throw "No sensor configuration found\n";
            }
        }
        if(report_period) {
            batch_period = report_period;
        }
        if(random_batch_period) {
            print_random_batch_period_message(sensor_type, random_batch_period);
            batch_period = random_batch_period;
        }
        if(random_report_period) {
            print_random_report_period_message(sensor_type, random_report_period);
            batch_period = random_report_period;
        }
        if(random_flush_period) {
            print_random_flush_period_message(sensor_type, random_flush_period);
            flush_period = random_flush_period;
        }
        if(random_sample_rate) {
            print_random_sample_rate_message(sensor_type, random_sample_rate);
            sample_rate = random_sample_rate;
        }
        if(random_resampler_rate) {
            print_random_resampler_rate_message(sensor_type,
                                                      random_resampler_rate);
            resampler_rate = random_resampler_rate;
        }

        std::string tc_spec = get_tc_spec( test_params, sensor_type);
        std::ostringstream bss;
        utility_obj.create_status_msg( bss, "Begin: ", tc_spec, psalt);
        utility_obj.write_line( bss);

        // Start test time
        auto start_time = std::chrono::system_clock::now();
        auto end_time = start_time + std::chrono::
                                            duration_cast<std::chrono::
                                            system_clock::duration>
                                            (std::chrono::duration<float>
                                            (test_duration));
        while(std::chrono::system_clock::now() < end_time) {

            if ( target_sensor_suid == nullptr) {
               print_file_line_number(__FILE__, __LINE__);
               throw "target_sensor_suid == nullptr\n";
            }
            else if(req_type == "std") {
                rc = stream_sensor(psalt, target_sensor_suid, sensor_type,
                                          sample_rate, batch_period,
                                          flush_period, flush_only, max_batch);
            }
            else if(req_type == "res") {
                rc = resample_sensor(psalt, &resampler_suid,
                                        target_sensor_suid, resampler_rate_type,
                                        sensor_type, resampler_rate, filter,
                                        batch_period, flush_period, flush_only,
                                        max_batch);
            }
            else if(req_type == "on_change") {
                 rc = on_change_sensor(psalt, target_sensor_suid,
                                                    psensor, sensor_type,
                                                    batch_period, flush_period,
                                                    flush_only, max_batch);
            }
            else if(req_type == "db") {
                rc = db_sensor(psalt, target_sensor_suid, sensor_type,
                                          db_length, batch_period, flush_period,
                                          flush_only, max_batch);
            }
            else {
                   print_file_line_number(__FILE__, __LINE__);
                   throw "Invalid request type\n";
            }
            // Get random stream duration
            if(min_stream_dur && max_stream_dur) {
                 random_stream_dur =
                     get_random_min_max(min_stream_dur,max_stream_dur);
            }
            if(random_stream_dur) {
                stream_duration = random_stream_dur;
            }
            // Stream duration
            if(rc == SALT_RC_SUCCESS) {
                utility_obj.stream_duration(stream_duration);

                // Stop sensor streaming
                std::string detail = sensor_type;
                sens_uid* disable_suid = target_sensor_suid;
                if(req_type == "res") {
                   detail = "resampler." + sensor_type;
                   disable_suid = &resampler_suid;
                }

                rc = utility_obj.disable_sensor(psalt, detail, disable_suid);

                if(std::chrono::system_clock::now() < end_time) {
                    if(delay) {
                       utility_obj.pause_duration("delay", delay);

                    }
                    if(random_delay) {
                       utility_obj.pause_duration("random_delay", delay);
                    }
                }
            }
            else {
               break;
            }
        } // end of while loop

        std::ostringstream oss;
        bool grade = (rc == SALT_RC_SUCCESS);
        utility_obj.create_grade_msg( oss, grade, tc_spec, psalt);
        utility_obj.write_line( oss);

        std::cout << "Calling see_salt destructor for";
        std::cout << " instance " << psalt << std::endl;
        delete psalt;
    } // end of run_b2b
//----------------------------------------------------------------------------
//                       *******  CCD related tests  ********
//----------------------------------------------------------------------------
    // Continuous test
    salt_rc continuous_test(see_salt* psalt, sens_uid* target_sensor_suid,
                                 sens_uid* resampler_suid,
                                 sensor* psensor,
                                 std::string req_type,
                                 std::string sensor_type,
                                 float sample_rate,
                                 float resampler_rate,
                                 int resampler_rate_type,
                                 float batch_period,
                                 float flush_period,
                                 float db_length,
                                 bool filter,
                                 bool flush_only,
                                 bool max_batch,
                                 std::vector<std::pair<std::string, sens_uid*>>&
                                                   streamed_sens_suid_pair,
                                 std::string tc_spec
                              )
    {
        std::ostringstream bss;
        utility_obj.create_status_msg( bss, "Begin: ", tc_spec, psalt);
        utility_obj.write_line( bss);

        salt_rc rc = SALT_RC_FAILED;
        if(psalt == nullptr || target_sensor_suid == nullptr ||
                        resampler_suid == nullptr || psensor == nullptr) {
            print_file_line_number(__FILE__, __LINE__);
            return(rc);
        }
        if(req_type == "std") {
            rc = stream_sensor(psalt, target_sensor_suid, sensor_type,
                                          sample_rate, batch_period,
                                          flush_period, flush_only, max_batch);
        }
        else if(req_type == "on_change") {
                rc = on_change_sensor(psalt, target_sensor_suid,
                                                  psensor, sensor_type,
                                                  batch_period, flush_period,
                                                  flush_only, max_batch);
        }
        else if(req_type == "db") {
                 rc = db_sensor(psalt, target_sensor_suid, sensor_type,
                                      db_length, batch_period, flush_period,
                                      flush_only, max_batch);
        }
        else if (req_type == "res") {
            rc = resample_sensor(psalt, resampler_suid,
                                            target_sensor_suid,
                                            resampler_rate_type,
                                            sensor_type,
                                            resampler_rate, filter,
                                            batch_period, flush_period,
                                            flush_only, max_batch);
        }
        else {
               std::cout << "Invalid request type" << std::endl;
        }
        if(rc == SALT_RC_SUCCESS) {
            // TODO:: Order to disable sensor may have to be added later
            streamed_sens_suid_pair.push_back(std::make_pair
                                                        (
                                                            sensor_type,
                                                            target_sensor_suid)
                                                        );
        }
        else {
               print_file_line_number(__FILE__, __LINE__);
               std::cout << sensor_type << " failed to stream" << std::endl;
               std::cout << "FAIL" << std::endl;
        }
        return(rc);
    } // end continous_test
//----------------------------------------------------------------------------
    // B2b test
    salt_rc b2b_test(see_salt* psalt, sens_uid* target_sensor_suid,
                                 sens_uid* resampler_suid,
                                 sensor* psensor,
                                 std::string req_type,
                                 std::string sensor_type,
                                 float sample_rate,
                                 float rand_odr,
                                 float resampler_rate,
                                 int resampler_rate_type,
                                 float batch_period,
                                 float flush_period,
                                 float db_length,
                                 float stream_duration,
                                 float test_duration,
                                 float delay,
                                 bool filter,
                                 bool flush_only,
                                 bool max_batch,
                                 std::string tc_spec
                              )
    {
        std::ostringstream bss;
        utility_obj.create_status_msg( bss, "Begin: ", tc_spec, psalt);
        utility_obj.write_line( bss);

        salt_rc rc = SALT_RC_FAILED;
        if(psalt == nullptr || target_sensor_suid == nullptr ||
                    resampler_suid == nullptr || psensor == nullptr) {
            print_file_line_number(__FILE__, __LINE__);
            return(rc);
        }
        // Start test time
        auto start_time = std::chrono::system_clock::now();
        auto end_time = start_time + std::chrono::
                                            duration_cast<std::chrono::
                                            system_clock::duration>
                                            (std::chrono::duration<float>
                                            (test_duration));
        while(std::chrono::system_clock::now() < end_time) {
              if(req_type == "std") {
                  if(rand_odr // Random odr for each iteration is requested
                     && psensor->has_rates()) {
                     sample_rate =
                         utility_obj.get_random_odr_rate(psensor,sensor_type);
                  }
                 rc = stream_sensor(psalt, target_sensor_suid, sensor_type,
                                          sample_rate, batch_period,
                                          flush_period, flush_only, max_batch);
              }
              else if(req_type == "res") {
                      rc = resample_sensor(psalt, resampler_suid,
                                             target_sensor_suid,
                                             resampler_rate_type,
                                             sensor_type, resampler_rate, filter,
                                             batch_period, flush_period,
                                             flush_only, max_batch
                                           );
              }
              else if(req_type == "on_change") {
                      rc = on_change_sensor(psalt, target_sensor_suid,
                                                    psensor, sensor_type,
                                                    batch_period, flush_period,
                                                    flush_only, max_batch);
              }
              else if(req_type == "db") {
                       rc = db_sensor(psalt, target_sensor_suid, sensor_type,
                                          db_length, batch_period, flush_period,
                                          flush_only, max_batch);
              }
              else {
                     print_file_line_number(__FILE__, __LINE__);
                     throw "Invalid request type\n";
              }
              // If stream request is sent successfully, add sensor suid to vect
              if(rc == SALT_RC_SUCCESS) {
                  utility_obj.stream_duration( stream_duration);

                  std::string detail = sensor_type;
                  sens_uid* disable_suid = target_sensor_suid;
                  if(req_type == "res") {
                      detail = "resampler." + sensor_type;
                      disable_suid = resampler_suid;
                  }

                  rc = utility_obj.disable_sensor(psalt, detail, disable_suid);
               }
               else {
                      std::cout << sensor_type << " failed to stream" << std::endl;
                      std::cout << "FAIL" << std::endl;
               }
               if(std::chrono::system_clock::now() < end_time) {
                  utility_obj.delay_duration(delay);
               }
        } // end of while loop

        std::ostringstream gss;
        bool grade = ( rc == SALT_RC_SUCCESS);
        utility_obj.create_grade_msg( gss, grade, tc_spec, psalt);
        utility_obj.write_line( gss);

        return(rc);
    } // end b2b_test
//----------------------------------------------------------------------------
    // Continuous-B2B tests
    void run_continuous_b2b(const TestCase::sensor_name_sensor_configs&
                                 sensor_config_mapping) {
        std::string sensor_name = "";
        std::string req_type = "";
        std::string rigid_body = "display"; // default

        const std::string DOLLAR_DELIMITER = "$";

        std::map<std::string, float>test_params;
        stability_engine_obj->process_test_params(sensor_config_mapping,
                                                  test_params,
                                                  tc_specs);
        // Get test duration
        float test_duration = Parser::get_test_duration();

        // Get number of QMI clients for the test
        int num_qmi_clients  = Parser::get_num_qmi_clients();

        // Get number of queued sensors. This is used by vector pair:
        // sensor_suid_pair
        int num_of_queued_sensors = Parser::get_num_queued_sensors();

        // Vector pair of sensor and its suid. Sensors passed from config file
        // are populated to this vector based on streaming priority
        std::vector<std::pair<std::string, sens_uid*>> sensor_suid_pair
                                                        [num_qmi_clients]
                                                        [num_of_queued_sensors];

        // Vector pair of streamed sensor and its suid. Sensors whose requests
        // are streamed successfully are populated to this vector
        std::vector<std::pair<std::string, sens_uid*>> streamed_sens_suid_pair
                                                        [num_qmi_clients];

        // Get usta_log_status
        bool usta_log_status = Parser::get_usta_log_status();
        set_usta_logging(usta_log_status);

        // Required by see_salt
        char* argv[] = {NULL};
        // Number of qmi instances
        // was see_salt* psalt[num_qmi_clients];
        see_salt** psalt = new see_salt*[num_qmi_clients];
        if (psalt == nullptr) {
           std::cout << "new psalt failed" << std::endl;
           return;
        }

        for(int i = 0; i < num_qmi_clients; ++i) {
            // Get see_salt instances
            psalt[i] = see_salt::get_instance();
            if(psalt[i] == nullptr) {
                std::cout << "psalt not instantiated" << std::endl;
                print_file_line_number(__FILE__, __LINE__);
                //throw "psalt not instantiated.\n";
                return;
            }
            psalt[i]->begin(0, argv);
        }

        sens_uid resampler_suid;
        int req_type_msg_id = 0;
        // Iterator to test_params
        std::map<std::string, float>::iterator it;
        // Get sensor configs from multi-map
        for(it = test_params.begin(); it != test_params.end(); ++it) {
            sens_uid* target_sensor_suid = nullptr;
            if((*it).first.find_first_of(DOLLAR_DELIMITER) ==
                                         std::string::npos) {
                     std::string sensor_type = (*it).first;

                // Get the actual sensor name passed from text file
                std::string save_sensor_type = sensor_type;

                if(!sensor_type.empty()) {
                    // Get sensor attrib name
                    if(get_sensor_attrib_name(test_params, sensor_type)) {
                        sensor_name = get_sensor_name(test_params, sensor_type);
                    }
                    if(test_params.find(sensor_type + "$rigid_body") !=
                                                    test_params.end()) {
                        rigid_body = get_rigid_body(test_params, sensor_type);
                    }
                    if(test_params.find(sensor_type + "$msg_id") !=
                                                test_params.end()) {
                        req_type_msg_id = get_msg_id(test_params, sensor_type);
                    }
                    // Get client number associated with the sensor
                    TestCase::client_num = get_client_num(test_params,
                                                                sensor_type);
                    int salt_index = TestCase::client_num - 1;
                    if ( salt_index < 0 || salt_index >= num_qmi_clients) {
                       print_file_line_number(__FILE__, __LINE__);
                       throw "invalid client_num\n";
                       return;
                    }
                    // Check psalt is not nullptr before using it
                    if(psalt[salt_index] == nullptr) {
                        print_file_line_number(__FILE__, __LINE__);
                        throw "psalt not instantiated\n";
                        return;
                    }

                    // If muliple sensors with same name are present
                    // digit is used to differentiate it (e.g. accel
                    // and accel2, accel3 etc.). Remove digit from the sensor
                    sensor_type.erase(std::remove_if(sensor_type.
                                    begin(), sensor_type.end(), &isdigit),
                                    sensor_type.end());

                    // Check request type
                    if(req_type_msg_id == static_cast<int>(
                                       StabilityEngine::req_message_ids::
                                       MSGID_STD_SENSOR_CONFIG)) {
                        req_type = "std";
                    }
                    else if(req_type_msg_id == static_cast<int>(
                                        StabilityEngine::req_message_ids::
                                        MSGID_ON_CHANGE_CONFIG)) {
                             req_type = "on_change";
                    }
                    else if(req_type_msg_id == static_cast<int>(
                                        StabilityEngine::req_message_ids::
                                        MSGID_SET_DISTANCE_BOUND)) {
                            req_type = "db";
                    }
                    else if(req_type_msg_id == static_cast<int>(
                                                  StabilityEngine::
                                                  req_message_ids::
                                                  MSGID_RESAMPLER_CONFIG)) {
                             req_type = "res";
                             resampler_suid = utility_obj.get_resampler_suid
                                                          (
                                                             psalt[salt_index]
                                                           );
                    }
                    else {
                            print_file_line_number(__FILE__, __LINE__);
                            throw "Invalid message request\n";
                    }

                    target_sensor_suid = utility_obj.get_target_sensor_suid
                                             (
                                                psalt[salt_index],
                                                target_sensor_suid,
                                                save_sensor_type, req_type, rigid_body,
                                                sensor_name
                                             );
                    if(target_sensor_suid == nullptr) {
                        std::string desc = "FAIL " + get_sensor_description(
                                                                   sensor_type,
                                                                   req_type,
                                                                   rigid_body);
                        throw desc.c_str();
                        return;
                    }
                    if(target_sensor_suid != nullptr) {
                        int stream_priority = 0;
                        if(test_params.find(sensor_type + "$priority") !=
                                                       test_params.end()) {
                           stream_priority = get_stream_priority
                                                        (
                                                          test_params,
                                                          sensor_type
                                                        );
                        }
                        // Populate target sensor type and its suid to a
                        // vector pair based on streaming priority.
                        // It is preferred to use this approach since getting
                        // suid's for each sensor takes a few milliseconds
                        // and in concurrency test, all message requests must
                        // be sent without any delay between request
                        sensor_suid_pair[TestCase::client_num - 1]
                                     [stream_priority - 1].push_back
                                                           (
                                                               std::make_pair
                                                               (
                                                                 save_sensor_type,
                                                                 target_sensor_suid
                                                                )
                                                            );
                        target_sensor_suid = nullptr;
                    }
                }
                else {
                      print_file_line_number(__FILE__, __LINE__);
                      throw "No sensor configuration found\n";
                }
            }
        } // end of for loop

        // Use std::async to send requests
        std::vector<std::future<salt_rc>> fut;
        // Get sensor and its suid from vector pair for each qmi client
        for(int clnt_idx = 0; clnt_idx < num_qmi_clients; ++clnt_idx) {
             std::string sensor_type = "";
             sens_uid* target_sensor_suid = nullptr;
             for(int queued_sens_idx = 0; queued_sens_idx < num_of_queued_sensors;
                                                   ++queued_sens_idx) {
                 for(std::vector<std::pair<std::string, sens_uid*>>::
                     const_iterator iter = sensor_suid_pair[clnt_idx]
                                            [queued_sens_idx].begin();
                                            iter != sensor_suid_pair[clnt_idx]
                                            [queued_sens_idx].end(); ++iter) {
                    sensor_type = iter->first;
                    target_sensor_suid = iter->second;
                    if(target_sensor_suid == nullptr) {
                       std::cout << "target sensor suid not found" << std::endl;
                       print_file_line_number(__FILE__, __LINE__);
                       return;
                    }
                    float stream_duration = 0.0f;
                    float random_stream_dur = 0.0f;
                    float sample_rate  = 0.0f;
                    float min_sample_rate = 0.0f;
                    float max_sample_rate = 0.0f;
                    float random_sample_rate = 0.0f;
                    float resampler_rate = 0.0f;
                    float min_resampler_rate = 0.0f;
                    float max_resampler_rate = 0.0f;
                    float random_resampler_rate = 0.0f;
                    float odr_rate = 0.0f;
                    float rand_odr = 0.0f;
                    float batch_period = 0.0f;
                    float min_batch_period = 0.0f;
                    float max_batch_period = 0.0f;
                    float random_batch_period = 0.0f;
                    float report_period = 0.0f;
                    float min_report_period = 0.0f;
                    float max_report_period = 0.0f;
                    float random_report_period = 0.0f;
                    float flush_period = 0.0f;
                    float min_flush_period = 0.0f;
                    float max_flush_period = 0.0f;
                    float random_flush_period = 0.0f;
                    float db_length = 0.0f;
                    float delay = 0.0f;
                    float min_delay = 0.0f;
                    float max_delay = 0.0f;
                    float random_delay = 0.0f;

                    int  resampler_rate_type = 0;
                    bool flush_only = false;
                    bool filter = false;
                    bool max_batch = false;

                    std::string test_type = "";

                    // Get message id
                    if(test_params.find(sensor_type + "$msg_id") !=
                                                    test_params.end()) {
                        req_type_msg_id = get_msg_id(test_params, sensor_type);
                    }
                    // Get sample rate
                    if(test_params.find(sensor_type + "$sample_rate") !=
                                           test_params.end()) {
                         sample_rate = get_sample_rate(test_params, sensor_type);
                    }
                    // Get odr rate
                    if(test_params.find(sensor_type + "$odr_rate") !=
                                                            test_params.end()) {
                        odr_rate = get_odr_rate(test_params, sensor_type);
                    }
                    // Get rand odr
                    if(test_params.find(sensor_type + "$rand_odr_rt") !=
                                                            test_params.end()) {
                        rand_odr = get_rand_odr(test_params, sensor_type);
                    }
                    // Get batch period of sensor
                    if(test_params.find(sensor_type + "$batch_period") !=
                                                   test_params.end()) {
                        batch_period = get_batch_period(test_params, sensor_type);
                    }
                    // Get report period of sensor, if exists
                    if(test_params.find(sensor_type + "$report_period") !=
                                                   test_params.end()) {
                        report_period = get_report_period(test_params, sensor_type);
                    }
                    // Get flush period
                    if(test_params.find(sensor_type + "$flush_period") !=
                                                  test_params.end()) {
                        flush_period = get_flush_period(test_params, sensor_type);
                    }
                    // Get max batch
                    if(test_params.find(sensor_type + "$max_batch") !=
                                                test_params.end()) {
                        max_batch = get_max_batch(test_params, sensor_type);
                    }
                    // Get db length
                    if(test_params.find(sensor_type + "$db_len") !=
                                                   test_params.end()) {
                        db_length = get_db_length(test_params, sensor_type);
                    }
                    // Get resampler rate
                    if(test_params.find(sensor_type + "$resampler_rate") !=
                                                test_params.end()) {
                        resampler_rate = get_resampler_rate(test_params,
                                                                 sensor_type);
                    }
                    // Get resampler rate type - fixed or minimum
                    if(test_params.find(sensor_type + "$resampler_rate_type") !=
                                                           test_params.end()) {
                        resampler_rate_type = get_resampler_rate_type(test_params,
                                                                    sensor_type);
                    }
                    // Get resampler filter type - true or false
                    if(test_params.find(sensor_type + "$resampler_filter") !=
                                                    test_params.end()) {
                        filter = get_resampler_filter(test_params, sensor_type);
                    }
                    // Get flush only
                    if(test_params.find(sensor_type + "$flush_only") !=
                                                    test_params.end()) {
                        flush_only = get_flush_only(test_params, sensor_type);
                    }
                    // Get min batch period
                    if(test_params.find(sensor_type + "$min_batch_period") !=
                                                 test_params.end()) {
                        min_batch_period = get_min_batch_period(test_params,
                                                            sensor_type);
                    }
                    // Get max batch period
                    if(test_params.find(sensor_type + "$max_batch_period") !=
                                                 test_params.end()) {
                        max_batch_period = get_max_batch_period(test_params,
                                                             sensor_type);
                    }
                    // Get min report period
                    if(test_params.find(sensor_type + "$min_report_period") !=
                                                 test_params.end()) {
                        min_report_period = get_min_report_period(test_params,
                                                              sensor_type);
                    }
                    // Get max report period
                    if(test_params.find(sensor_type + "$max_report_period") !=
                                                 test_params.end()) {
                        max_report_period = get_max_report_period(test_params,
                                                               sensor_type);
                    }
                    // Get min flush period
                    if(test_params.find(sensor_type + "$min_flush_period") !=
                                                 test_params.end()) {
                        min_flush_period = get_min_flush_period(test_params,
                                                             sensor_type);
                    }
                    // Get max flush period
                    if(test_params.find(sensor_type + "$max_flush_period") !=
                                                 test_params.end()) {
                        max_flush_period = get_max_flush_period(test_params,
                                                             sensor_type);
                    }
                    // Get min sample rate
                    if(test_params.find(sensor_type + "$min_sample_rate") !=
                                                test_params.end()) {
                        min_sample_rate = get_min_sample_rate(test_params,
                                                           sensor_type);
                    }
                    // Get max sample rate
                    if(test_params.find(sensor_type + "$max_sample_rate") !=
                                                test_params.end()) {
                        max_sample_rate = get_max_sample_rate(test_params,
                                                           sensor_type);
                    }
                    // Get min resampler rate
                    if(test_params.find(sensor_type + "$min_resampler_rate") !=
                                                test_params.end()) {
                        min_resampler_rate = get_min_resampler_rate(test_params,
                                                                    sensor_type);
                    }
                    // Get max resampler rate
                    if(test_params.find(sensor_type + "$max_resampler_rate") !=
                                                test_params.end()) {
                        max_resampler_rate = get_max_resampler_rate(test_params,
                                                                    sensor_type);
                    }
                    // Get delay
                    if(test_params.find(sensor_type + "$delay_before_req") !=
                                                 test_params.end()) {
                        delay = get_delay(test_params, sensor_type);
                    }
                    // Get min delay
                    if(test_params.find(sensor_type + "$min_delay_before_req") !=
                                                 test_params.end()) {
                        min_delay = get_min_delay(test_params, sensor_type);
                    }
                    // Get max delay
                    if(test_params.find(sensor_type + "$max_delay_before_req") !=
                                                 test_params.end()) {
                        max_delay = get_max_delay(test_params, sensor_type);
                    }
                    // Get random delay
                    // min delay could be 0, hence use max_delay in 'if' block
                    if(max_delay) {
                       random_delay = get_random_min_max(min_delay, max_delay);
                       delay = random_delay;
                    }
                    // Get stream duration
                    if(test_params.find(sensor_type + "$stream_duration") !=
                                                    test_params.end()) {
                        stream_duration = get_stream_duration(test_params,
                                                                sensor_type);
                    }
                    // Pointer to sensor target
                    sensor* psensor = psalt[clnt_idx]->get_sensor(
                                                             target_sensor_suid
                                                           );
                    if(psensor == nullptr) {
                        std::cout << "psensor is nullptr" << std::endl;
                        print_file_line_number(__FILE__, __LINE__);
                        //throw "psensor is nullptr\n";
                        return;
                    }
                    if(sample_rate == MIN_RATE && psensor->has_rates()) {
                    sample_rate = utility_obj.get_min_odr_rate(psensor,
                                                                sensor_type);
                    }
                    if(sample_rate == MAX_RATE && psensor->has_max_rate()) {
                        sample_rate = utility_obj.get_max_odr_rate(psensor,
                                                                sensor_type);
                    }
                    if(resampler_rate == MIN_RATE && psensor->has_rates()) {
                        resampler_rate = utility_obj.get_min_odr_rate(psensor,
                                                                   sensor_type);
                    }
                    if(resampler_rate == MAX_RATE && psensor->has_max_rate()) {
                        resampler_rate = utility_obj.get_max_odr_rate(psensor,
                                                                   sensor_type);
                    }
                    if(odr_rate == MIN_RATE && psensor->has_rates()) {
                        sample_rate = utility_obj.get_min_odr_rate(psensor,
                                                                    sensor_type);
                    }
                    if(odr_rate == MAX_RATE && psensor->has_rates()) {
                        sample_rate = utility_obj.get_max_odr_rate(psensor,
                                                                    sensor_type);
                    }
                    if((sample_rate == RAND_RATE || odr_rate == RAND_RATE)
                                                    && psensor->has_rates()) {
                        sample_rate =
                            utility_obj.get_random_odr_rate( psensor, sensor_type);
                    }
                    if(resampler_rate == RAND_RATE && psensor->has_rates()) {
                        resampler_rate =
                            utility_obj.get_random_odr_rate(psensor,sensor_type);
                    }
                    // Get random batch period
                    if(min_batch_period && max_batch_period) {
                         random_batch_period =
                             get_random_min_max(min_batch_period,max_batch_period);
                    }
                    // Get random report period
                    if(min_report_period && max_report_period) {
                         random_report_period =
                             get_random_min_max(min_report_period,max_report_period);
                    }
                    // Get random flush period
                    if(min_flush_period && max_flush_period){
                         random_flush_period =
                             get_random_min_max(min_flush_period,max_flush_period);
                    }
                    // Get random sample rate
                    if(min_sample_rate && max_sample_rate) {
                        if(min_sample_rate == MIN_RATE
                           && max_sample_rate == MAX_RATE) {
                            min_sample_rate =
                                utility_obj.get_min_odr_rate(psensor,sensor_type);
                            max_sample_rate =
                                utility_obj.get_max_odr_rate(psensor,sensor_type);
                        }
                        random_sample_rate =
                            get_random_min_max(min_sample_rate,max_sample_rate);
                    }
                    // Get random resampler rate
                    if(min_resampler_rate && max_resampler_rate) {
                        if(min_resampler_rate == MIN_RATE
                           && max_resampler_rate == MAX_RATE) {
                            min_resampler_rate =
                                utility_obj.get_min_odr_rate(psensor,sensor_type);
                            max_resampler_rate =
                                utility_obj.get_max_odr_rate(psensor,sensor_type);
                        }
                        random_resampler_rate =
                            get_random_min_max(min_resampler_rate,max_resampler_rate);
                    }
                    // Check request type
                    if(req_type_msg_id == static_cast<int>(
                                           StabilityEngine::req_message_ids::
                                           MSGID_STD_SENSOR_CONFIG)) {
                         req_type = "std";
                    }
                    else if(req_type_msg_id == static_cast<int>(
                                           StabilityEngine::req_message_ids::
                                           MSGID_ON_CHANGE_CONFIG)) {
                             req_type = "on_change";
                    }
                    else if(req_type_msg_id == static_cast<int>(
                                           StabilityEngine::req_message_ids::
                                           MSGID_SET_DISTANCE_BOUND)) {
                             req_type = "db";
                    }
                    else if(req_type_msg_id == static_cast<int>(
                                                      StabilityEngine::
                                                      req_message_ids::
                                                      MSGID_RESAMPLER_CONFIG)) {
                             req_type = "res";
                    }
                    else {
                            std::cout << "Invalid message request " << std::endl;
                            std::cout << "FAIL" << std::endl;
                    }
                    if(report_period) {
                        batch_period = report_period;
                    }
                    if(random_batch_period) {
                        print_random_batch_period_message(sensor_type,
                                                            random_batch_period);
                        batch_period = random_batch_period;
                    }
                    if(random_report_period) {
                        print_random_report_period_message(sensor_type,
                                                            random_report_period);
                        batch_period = random_report_period;
                    }
                    if(random_flush_period) {
                        print_random_flush_period_message(sensor_type,
                                                            random_flush_period);
                        flush_period = random_flush_period;
                    }
                    if(random_sample_rate) {
                        print_random_sample_rate_message(sensor_type,
                                                            random_sample_rate);
                        sample_rate = random_sample_rate;
                    }
                    if(random_resampler_rate) {
                        print_random_resampler_rate_message
                                                      (
                                                        sensor_type,
                                                        random_resampler_rate
                                                      );
                        resampler_rate = random_resampler_rate;
                    }

                    std::string tc_spec = get_tc_spec(test_params,sensor_type);

                    // Get test type - continuous or b2b
                    if(test_params.find(sensor_type + "$ttype") !=
                                           test_params.end()) {
                         test_type = get_test_type(test_params, sensor_type);
                    }
                    if(test_type == "continuous") {
                        // Send request for continuous test using std::async
                        // synchronously
                        fut.push_back(std::async
                                        (
                                           std::launch::async,
                                           continuous_test, psalt[clnt_idx],
                                           target_sensor_suid,
                                           &resampler_suid, psensor,
                                           req_type,
                                           sensor_type,
                                           sample_rate,
                                           resampler_rate,
                                           resampler_rate_type,
                                           batch_period,
                                           flush_period,
                                           db_length,
                                           filter,
                                           flush_only,
                                           max_batch,
                                           std::ref(streamed_sens_suid_pair[clnt_idx]),
                                           tc_spec
                                        )
                                      );
                    } // end if(continuous)
                    else {
                        if(test_type == "b2b") {
                            // Send request for b2b test using std::async
                            // synchronously
                            fut.push_back(std::async
                                             (
                                                std::launch::async,
                                                b2b_test, psalt[clnt_idx],
                                                target_sensor_suid,
                                                &resampler_suid, psensor,
                                                req_type,
                                                sensor_type,
                                                sample_rate,
                                                rand_odr,
                                                resampler_rate,
                                                resampler_rate_type,
                                                batch_period,
                                                flush_period,
                                                db_length,
                                                stream_duration,
                                                test_duration,
                                                delay,
                                                filter,
                                                flush_only,
                                                max_batch,
                                                tc_spec
                                             )
                                         );
                        }
                    }
                    // To capture output in stdout, a sleep of ~2-3 ms is required
                    float sleep = 0.003; // sleep for 3 ms
                    std::this_thread::sleep_for(std::chrono::duration<float>(sleep));
                    if(delay) {
                        utility_obj.delay_duration(delay);
                        test_duration -= delay;
                    }
                } // end const_iterator
            } // end queued_sens_idx
        } // end clnt_idx
        // Stream for test duration
        utility_obj.pause_duration( "main_sleep", test_duration);

        // Stop streaming
        salt_rc rc = SALT_RC_FAILED;
        for(int clnt_idx = 0; clnt_idx < num_qmi_clients; ++clnt_idx) {
            for(std::vector<std::pair<std::string, sens_uid*>>::
                const_iterator iter = streamed_sens_suid_pair[clnt_idx].begin();
                iter != streamed_sens_suid_pair[clnt_idx].end(); ++iter) {
                    sens_uid* target_sensor_suid = iter->second;
                if(target_sensor_suid == nullptr) {
                   std::cout << "target sensor suid not found" << std::endl;
                   print_file_line_number(__FILE__, __LINE__);
                   return;
                }

                std::string detail = iter->first;

                req_type_msg_id = get_msg_id(test_params, iter->first);
                if(req_type_msg_id == static_cast<int>(StabilityEngine::
                                                      req_message_ids::
                                                      MSGID_RESAMPLER_CONFIG)) {
                    detail = "resampler." + iter->first;
                    target_sensor_suid = &resampler_suid;
                }
                rc = utility_obj.disable_sensor(psalt[clnt_idx],
                                                detail,
                                                target_sensor_suid);

                std::string tc_spec = get_tc_spec( sensor_config_mapping,
                                                   iter->first);
                std::ostringstream rss;
                bool grade = ( rc == SALT_RC_SUCCESS);
                utility_obj.create_grade_msg( rss, grade, tc_spec, psalt[clnt_idx]);
                utility_obj.write_line( rss);

                // Check if a delay exists before next disable stream request is
                // sent
                if(test_params.find(iter->first + "$del_bef_next_disable_req") !=
                                                 test_params.end()) {
                    int delay_before_disable_req = get_delay_before_next_disable_req
                                                            (
                                                               test_params,
                                                               iter->first
                                                            );
                    utility_obj.delay_duration(delay_before_disable_req);
                }
            } // end const_iterator
        } // end clnt_idx

        // SENSORTEST-625: Need wait for the "future" threads to terminate
        for(size_t i = 0; i < fut.size(); ++i) {
            #if DEBUGGING
            std::ostringstream oss;
            oss << "wait() on future: " << i << std::endl;
            utility_obj.write_line( oss);
            #endif
            fut[i].wait();
        }

        // Delete all psalt instances
        for(int clnt_idx = 0; clnt_idx < num_qmi_clients; ++clnt_idx) {
            std::cout << " delete psalt instance " << psalt[clnt_idx] << std::endl;
            delete psalt[clnt_idx];
        }
    } // end run_streaming_b2b
//----------------------------------------------------------------------------
    // Get test params after parsing
    void TestCase::get_test_params(const std::string& test_case_id,
                                   const sensor_name_sensor_configs&
                                   sensor_config_mapping)
    {
        const std::string FIRST_UNDERSCORE = "_";
        std::size_t pos;

        unsigned int seed = Parser::get_seed();
        utility_obj.init_random( seed);

        TestCase::test_case_id = test_case_id;
        pos = test_case_id.find_first_of(FIRST_UNDERSCORE);
        std::string test_case_name = test_case_id.substr(pos + 1);

        if(test_case_name == "Stability") {
            run_stability(sensor_config_mapping);
        }
        if(test_case_name == "Concurrency") {
            run_concurrency(sensor_config_mapping);
        }
        if(test_case_name == "Back2Back") {
            run_b2b(sensor_config_mapping);
        }
        if(test_case_name == "ContB2bCombo") {
            run_continuous_b2b(sensor_config_mapping);
        }
//-----------------------------------------------------------------------------
} // end TestCase::get_test_params
//-----------------------------------------------------------------------------

} // end namespace sensors_stability
///////////////////////////////////////////////////////////////////////////////
