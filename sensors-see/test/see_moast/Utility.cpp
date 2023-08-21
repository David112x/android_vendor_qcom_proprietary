/* ===================================================================
**
** Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: Utility.cpp
** DESC: Implementation of functions declared in Utility.h
** ===================================================================*/

#include <random>       // std::mt19937
#include <iostream>     // std::cout
#include <iomanip>      // std::fixed, std::setprecision
#include <chrono>       // std::chrono
#include <thread>       // std::thread
#include <string>       //std::erase
#include <sstream>      // std::ostringstream
#include <random>

#include "Utility.h"
#include <stdio.h>
#include <string.h>

namespace sensors_moast {
    /**
     * Default constructor. Used to set decimal precision
     *
     * @param void
     * @return void
     */
     Utility::Utility()
     {
       // Fixed floating-point notation
       std::cout << std::fixed;
        //Decimal precision to be set
        std::cout << std::setprecision(2);
     } // end Utility::Utility

//-----------------------------------------------------------------------------

// ****************************************************************************
//                              Non-member Functions                          *
// ****************************************************************************
//-----------------------------------------------------------------------------
    // Print filename, line number and function name where error occured
    void Utility::print_file_line_number(const char* file_name, int line_num,
                                            const char* func_name)
        {
           std::cout << "Exception in file: " << file_name << ", "
                     << "line number: " << line_num << " in function: "
                     << func_name << std::endl;
           std::cout << "FAIL" << std::endl;
        }
//-----------------------------------------------------------------------------
    // Return randomized sampline rate
    float Utility::get_randomized_sampling_rate
                                    (
                                        sensor* const psensor,
                                        const float& available_sampling_rate
                                    )
     {
        std::vector<float> odr_rates;
        float min_rate = 0.0f;
        float max_rate = 0.0f;
        float selected_sample_rate = 0.0f;

        if(psensor == nullptr) {
            print_file_line_number(__FILE__, __LINE__,__func__);
            return(selected_sample_rate);
        }
        if(psensor->has_rates()) {
             psensor->get_rates(odr_rates);
             if(odr_rates.empty()) {
                 std::cout << "Odr rates for " << psensor->get_type()
                           << " is zero" << std::endl;
                 return(0.0f);
             }
             else {
                min_rate = 1;
                max_rate = odr_rates.back();
                max_rate = std::min(max_rate, available_sampling_rate);
                selected_sample_rate = get_random_min_max(min_rate, max_rate);
             }
             return(selected_sample_rate);
        }
        else {
            std::cout << "No odr rates for " <<  psensor->get_type() << std::endl;
            return(-1.0f);
        }
     } // end of get_sampling_rate
//-----------------------------------------------------------------------------
    // Return randomized batch period
    float Utility::get_randomized_batch_period(
                                                 const float& sample_rate,
                                                 const int& random_dur_per_req,
                                                 const float& available_sampling_rate
                                              )
    {
        float shortest_batch_period = std::max(
                                                1/sample_rate,
                                                1/available_sampling_rate
                                              );
        float longest_batch_period = random_dur_per_req;
        float selected_batch_period = get_random_min_max
                                                (
                                                    shortest_batch_period,
                                                    longest_batch_period
                                                );
        return(selected_batch_period);
    } // end of get_randomized_batch_period
//-----------------------------------------------------------------------------
    // Stream sensor
    salt_rc Utility::stream_sensor(see_salt* const psalt, sens_uid* const suid,
                                        see_msg_id_e msg_id,
                                        see_std_request& std_request,
                                        const float& rate
                                  )
    {
      salt_rc rc = SALT_RC_FAILED;
      if(psalt != nullptr) {
          see_std_sensor_config sample_rate(rate);
          see_std_request see_request;
          memcpy ( &see_request, &std_request, sizeof( see_request));
          see_request.set_payload(&sample_rate);
          see_client_request_message request(suid, msg_id, &see_request);
          rc = psalt->send_request(request);
          return(rc);
      }
      else {
            std::cout << "psalt not instantiated." << std::endl;
            return(rc);
      }
    } // end of stream_sensor
//-----------------------------------------------------------------------------
    // Resampler request
    salt_rc Utility::resampler_req(see_salt* const psalt,
                                             sens_uid* const resampler_suid,
                                             sens_uid* const target_suid,
                                             see_msg_id_e msg_id,
                                             see_std_request& std_request,
                                             const float& rate
                                          )
    {
        salt_rc rc = SALT_RC_FAILED;
        if(target_suid == nullptr) {
            print_file_line_number(__FILE__, __LINE__,__func__);
            return(rc);
        }
        if(psalt != nullptr) {
            bool filter = true;
            see_resampler_config payload(target_suid, rate, SEE_RESAMPLER_RATE_FIXED,
                                         filter);
            see_std_request see_request;
            memcpy ( &see_request, &std_request, sizeof( see_request));
            see_request.set_payload(&payload);
            see_client_request_message request(resampler_suid, msg_id, &see_request);
            salt_rc rc = psalt->send_request(request);
            return(rc);
        }
        else {
            std::cout << "psalt not instantiated." << std::endl;
            return(rc);
        }
    } // end of resampler_req
//-----------------------------------------------------------------------------
    // Stream on-change sensor
    salt_rc Utility::on_change_sensor(see_salt* const psalt,
                                            sens_uid* const suid,
                                            see_msg_id_e msg_id,
                                            see_std_request& std_request
                                     )
    {
        salt_rc rc = SALT_RC_FAILED;
        if(psalt != nullptr) {
            see_client_request_message request(suid, msg_id, &std_request);
            rc = psalt->send_request(request);
            return(rc);
        }
        else {
            std::cout << "psalt not instantiated." << std::endl;
            return(rc);
        }
    } // end of on_change_sensor
//-----------------------------------------------------------------------------
    // Basic gestures request
    salt_rc Utility::basic_gesture_req(see_salt* const psalt, sens_uid* const suid,
                                         see_msg_id_e& msg_id_config,
                                         see_std_request& std_request)
    {
        salt_rc rc = SALT_RC_FAILED;
        if(psalt != nullptr) {
            see_basic_gestures_config config;
            see_std_request see_request;
            memcpy ( &see_request, &std_request, sizeof( see_request));
            see_request.set_payload(&config);
            see_client_request_message request(suid, msg_id_config, &see_request);
            rc = psalt->send_request(request);
            return(rc);
        }
        else {
            std::cout << "psalt not instantiated." << std::endl;
            return(rc);
        }
    } // end of basic_gestures_req
//-----------------------------------------------------------------------------
    // Psmd request
    salt_rc Utility::psmd_req(see_salt* const psalt, sens_uid* const suid,
                                see_msg_id_e& msg_id_config,
                                see_std_request& std_request)
    {
        salt_rc rc = SALT_RC_FAILED;
        if(psalt != nullptr) {
            see_psmd_config config(SEE_PSMD_TYPE_STATIONARY);
            see_std_request see_request;
            memcpy ( &see_request, &std_request, sizeof( see_request));
            see_request.set_payload(&config);
            see_client_request_message request(suid, msg_id_config, &see_request);
            rc = psalt->send_request(request);
            return(rc);
        }
        else {
            std::cout << "psalt not instantiated." << std::endl;
            return(rc);
        }
    } // end of psmd_req
//-----------------------------------------------------------------------------
    // Multishake request
    salt_rc Utility::multishake_req(see_salt* const psalt, sens_uid* const suid,
                                        see_msg_id_e& msg_id_config,
                                        see_std_request& std_request)
    {
        salt_rc rc = SALT_RC_FAILED;
        if(psalt != nullptr) {
            see_multishake_config config;
            see_std_request see_request;
            memcpy ( &see_request, &std_request, sizeof( see_request));
            see_request.set_payload(&config);
            see_client_request_message request(suid, msg_id_config, &see_request);
            rc = psalt->send_request(request);
            return(rc);
        }
        else {
            std::cout << "psalt not instantiated." << std::endl;
            return(rc);
        }
    } // end of multishake_req
//-----------------------------------------------------------------------------
    // Distance bound sensor
    salt_rc Utility::distance_bound_req(see_salt* const psalt,
                                            sens_uid* const suid,
                                            see_msg_id_e& msg_id_config,
                                            see_std_request& std_request,
                                            const float& db_length)
    {
        salt_rc rc = SALT_RC_FAILED;
        if(psalt != nullptr) {
            see_set_distance_bound distance_bound(db_length);
            see_std_request see_request;
            memcpy ( &see_request, &std_request, sizeof( see_request));
            see_request.set_payload(&distance_bound);
            see_client_request_message request(suid, msg_id_config, &see_request);
            rc = psalt->send_request(request);
            return(rc);
        }
        else {
            std::cout << "psalt not instantiated." << std::endl;
            return(rc);
        }
    } // end of distance_bound_req
//-----------------------------------------------------------------------------
    // Stream duration
    void Utility::stream_duration(const float& stream_dur)
    {
        std::this_thread::sleep_for(std::chrono::duration<float>(stream_dur));
    } // end of stream_duration
//-----------------------------------------------------------------------------
    // Stop sensor streaming
    salt_rc Utility::disable_sensor(see_salt* const psalt, sens_uid* const suid)
    {
        salt_rc rc = SALT_RC_FAILED;
        if(psalt != nullptr) {
            see_std_request std_request;
            see_client_request_message request(suid, MSGID_DISABLE_REQ, &std_request);
            salt_rc rc = psalt->send_request(request);
            if ( rc == SALT_RC_SUCCESS) {
               rc = psalt->stop_request(suid, false); // disconnect qmi
            }
            return(rc);
        }
        else {
            std::cout << "psalt not instantiated." << std::endl;
            return(rc);
        }
    } // end of disable_sensor

/* new additions follow ... */

/**
 * seed the random number generator
 *
 * @param seed
 */
    void Utility::init_random( unsigned int seed)
    {
        if ( seed == 0) {
            seed = time(NULL);
            std::cout << "Using new random seed: ";
            std::cout << seed << std::endl;

        }
        srand(seed);
    }

/**
 * return a random float odr_rate for the sensor
 *
 * @param psensor
 * @param sensor_type
 * @param seed
 * @param rand_cnt
 *
 * @return float
 */
    float Utility::get_random_odr_rate(sensor* psensor,
                                       const std::string& sensor_type)
    {
        if (psensor && psensor->has_rates()) {
            std::vector<float> rates;
            psensor->get_rates(rates);
            int num = (int)rates.size();
            int index = get_random_min_max( 0, num);
            if ( index < (int)rates.size()) {
                return rates[ index];
            }
            return rates[0];
        }

        std::cout << "get_random_odr_rate but " << sensor_type
                  << "has no rates" << std:: endl;
        return 5; // default 5 hz
    }

/**
 * return random integer between the values of min and max
 *
 * @param min
 * @param max
 *
 * @return int
 */
    int Utility::get_random_min_max( int min, int max)
    {
        int value = min + (rand() % (max - min + 1));
        return value;
    }


/**
 * return a random number between 0.0 and 1.0
 *
 * @return float between 0.0 and 1.0
 */
    float Utility::get_random()
    {
        static float rand_max = RAND_MAX;
        float fract = rand() / rand_max;
        return fract;
    }

/**
 * return random float between the values of min and max
 *
 * @param min
 * @param max
 *
 * @return float
 */
    float Utility::get_random_min_max( float min, float max)
    {
        float fract = get_random();
        float value = min + (max - min) * fract;
        return value;
    }

    void append_log_time_stamp( std::ostringstream &oss)
    {
        std::chrono::system_clock::time_point start_time;
        const auto end_time = std::chrono::system_clock::now();
        auto delta_time = end_time - start_time;
        const auto hrs = std::chrono::duration_cast<std::chrono::hours>
                                                                (delta_time);
        delta_time -= hrs;
        const auto mts = std::chrono::duration_cast<std::chrono::minutes>
                                                                (delta_time);
        delta_time -= mts;
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>
                                                                (delta_time);
        delta_time -= secs;
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>
                                                                (delta_time);
        oss    << std::setfill('0')
               << std::setw(2) << hrs.count() % 24 << ":"
               << std::setw(2) << mts.count() << ':'
               << std::setw(2) << secs.count() << '.'
               << std::setw(3) << ms.count() << ' '
               << std::setfill(' ') << std::left;
    }

    void append_action( std::ostringstream &oss, const std::string action)
    {
        oss << std::setw(10) << action << ' ';
    }
    void append_detail( std::ostringstream &oss, const std::string detail)
    {
        oss << std::setw(24) << detail << ' ';
    }

    void append_rigid_body( std::ostringstream &oss, std::string rigid_body)
    {
       if ( rigid_body != "") {
           oss << "on " << rigid_body << ' ';
       }
    }

    // note: input client is actually client index
    void append_client( std::ostringstream &oss, int client)
    {
        oss << "client=" << std::dec << ++client << ' ';
    }

    // hh:mm:ss.thh action detail
    void Utility::create_anchor_msg( std::ostringstream &oss,
                                     const std::string action,
                                     const std::string detail)
    {
        append_log_time_stamp( oss);
        append_action( oss, action);
        append_detail( oss, detail);
    }

    // hh:mm:ss.thh action detail psalt
    void Utility::create_anchor_msg( std::ostringstream &oss,
                                     const std::string action,
                                     const std::string detail,
                                     const std::string rigid_body,
                                     int client)
    {
        create_anchor_msg( oss, action, detail);
        append_rigid_body( oss, rigid_body);
        append_client( oss, client);
    }


    void Utility::create_grade_msg( std::ostringstream &oss,
                                    bool grade,
                                    const std::string detail,
                                    const std::string rigid_body)
    {
        std::string pass_fail = (grade) ? "PASS: " : "FAIL: ";
        create_status_msg( oss, pass_fail, detail, rigid_body);
    }

    void Utility::create_status_msg( std::ostringstream &oss,
                                     std::string status,
                                     const std::string detail,
                                     const std::string rigid_body)
    {
        oss << std::left << std::setfill(' ') << status;
        append_detail( oss, detail);
        append_rigid_body( oss, rigid_body);
    }

    void Utility::append_extras( std::ostringstream &xss,
                                 const float& sample_rate,
                                 const float& batch_period,
                                 const float& flush_period,
                                 const bool& flush_only,
                                 const bool& max_batch)
    {
        if (sample_rate) {
            xss << "sample_rate=" << std::dec << sample_rate << ' ';
        }
        if (batch_period) {
            xss << "batch_period=" << std::dec << batch_period << ' ';
        }
        if (flush_period) {
            xss << "flush_period=" << std::dec << flush_period << ' ';
        }
        if (flush_only) {
            xss << "flush_only=true ";
        }
        if (max_batch) {
            xss << "max_batch=true ";
        }
    }

    void Utility::append_resampler( std::ostringstream &xss,
                                    const float &resample_rate,
                                    std::string rate_type,
                                    const bool &filter)
    {
        xss << "resample_rate=" << std::dec << resample_rate << ' ';
        if (rate_type != "") {
            xss << "rate_type=" << std::dec << rate_type << ' ';
        }
        if (filter) {
            xss << "filter=true ";
        }
    }

    void Utility::append_rc( std::ostringstream &oss, salt_rc rc)
    {
        oss << "rc=" << std::dec << std::to_string(rc);
    }

    /**
     * serialize writes to stdout
     *
     * @param oss
     */
    void Utility::write_line( std::ostringstream &oss)
    {
        //Critical section
        std::unique_lock<std::mutex> lock(mu);
        std::cout << oss.str() << std::endl;
    }



//-----------------------------------------------------------------------------
} // end of namespace sensors_moast
