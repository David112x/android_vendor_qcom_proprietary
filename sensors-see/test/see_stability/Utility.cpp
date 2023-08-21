/* ===================================================================
**
** Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: Utility.cpp
** ================================================================ */

#include <iostream>
#include <random>
#include <chrono>
#include <thread>     // std::this_thread
#include <iomanip>    // std::setprecision
#include <sstream>    // std::ostringstream
#include <string>     // std::string
#include <mutex>      // std::unique_lock
#include <algorithm>  // std::remove_if
#include <random>
#include "Utility.h"
#include "Parser.h"

namespace sensors_stability {

    const int Utility::MICROSECONDS = 1000000;
    const int Utility::MILLISECONDS = 1000;

// ***************************************************************************
//                       Non-member functions                                *
// ***************************************************************************
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
        oss << std::setw(16) << action << ' ';
    }
    void append_detail( std::ostringstream &oss, const std::string detail)
    {
        oss << std::setw(16) << detail << ' ';
    }
    void append_thread( std::ostringstream &oss)
    {
        oss << "thread " << std::dec << std::setw(16)
            << std::this_thread::get_id() << ' ';
    }
    void append_psalt( std::ostringstream &oss, const see_salt* psalt)
    {
        oss << "psalt " << std::dec << std::setw(16) << psalt << ' ';
    }

    // hh:mm:ss.thh thread xxxxxxxxxxxxxxxx  action detail
    void Utility::create_anchor_msg( std::ostringstream &oss,
                                     const std::string action,
                                     const std::string detail)
    {
        append_log_time_stamp( oss);
        append_thread( oss);
        append_action( oss, action);
        append_detail( oss, detail);
    }

    // hh:mm:ss.thh thread xxxxxxxxxxxxxxxx  action detail psalt xxxxxxxxxxxx
    void Utility::create_anchor_msg( std::ostringstream &oss,
                                     const std::string action,
                                     const std::string detail,
                                     const see_salt* psalt)
    {
        create_anchor_msg( oss, action, detail);
        append_psalt( oss, psalt);
    }

    // grade detail thread xxxxxxxxxxxx psalt xxxxxxxxxxx
    void Utility::create_grade_msg( std::ostringstream &oss,
                                    bool grade,
                                    const std::string detail,
                                    const see_salt* psalt)
    {
        std::string pass_fail = (grade) ? "PASS: " : "FAIL: ";
        create_status_msg( oss, pass_fail, detail, psalt);
    }

    // status detail thread xxxxxxxxxxxx psalt xxxxxxxxxxx
    void Utility::create_status_msg( std::ostringstream &oss,
                                     std::string status,
                                     const std::string detail,
                                     const see_salt* psalt)
    {
        oss << std::left << std::setfill(' ') << status;
        append_detail( oss, detail);
        append_thread( oss);
        append_psalt( oss, psalt);
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
        oss << "return_code=" << std::dec << std::to_string(rc);
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
// ***************************************************************************
//                       Member/Friend functions                             *
// ***************************************************************************
//----------------------------------------------------------------------------
    // Set decimal precision in the constructor
    Utility::Utility()
    {
        // Fixed floating-point notation
        std::cout << std::fixed;

        //Decimal precision to be set
        std::cout << std::setprecision(2);
    } // end Utility::Utility
//----------------------------------------------------------------------------
    // Return a value within min:max range
    std::pair<float, float>Utility::get_min_max_range(std::string range)
    {
        // Position of string pattern
        size_t pos;
        std::string colon_delimiter = ":";
        float min = 0.0f;
        float max = 0.0f;

        // Parse string on ":" delimiter
        while((pos = range.find_first_of(colon_delimiter)) != std::string::npos) {
               min = stof(range.substr(0, pos));
               max = stof(range.substr(pos + 1));
               range.erase(0, pos + colon_delimiter.length());
        }
        return(std::make_pair(min, max));
    } // end Utility::get_min_max_range
//----------------------------------------------------------------------------
    // Return a value within supported min:max of the sensor
    // TODO: Not yet implemented
    std::pair<float, float>Utility::get_supported_min_max_range(std::string range)
    {
        // Position of string pattern
        size_t pos;
        std::string colon_delimiter = ":";
        float min = 0.0f;
        float max = 0.0f;

        // Parse string on ":" delimiter
        while((pos = range.find_first_of(colon_delimiter)) !=
                                                            std::string::npos) {
              min = 10.0f;  // TODO: get the supported min value from attributes
              max = 400.0f; // TODO: get the supported max value from attributes
        }
        return(std::make_pair(min, max));

    } // end Utility::get_min_max_range
//----------------------------------------------------------------------------
    void Utility::pause_duration( const std::string &why,
                                  const float& pause_dur)
    {
        if ( pause_dur >= 0.001) {  // >= one millisecond
            if (why != "") {
                std::ostringstream dss; // produce n.ddd
                dss << std::fixed << std::setprecision(3) << pause_dur;
                dss << " secs";
                std::ostringstream oss;
                create_anchor_msg( oss, why, dss.str());
                write_line(oss);
            }
            std::this_thread::sleep_for(std::chrono::duration<float>(pause_dur));
        }
    }
//----------------------------------------------------------------------------
    // Stream duration
    void Utility::stream_duration(const float& str_dur)
    {
        pause_duration( "stream", str_dur);
    } // end Utility::stream_duration
//----------------------------------------------------------------------------
    // Delay duration
    void Utility::delay_duration(const float& delay_dur)
    {
        pause_duration( "delay ", delay_dur);
    } // end Utility::stream_duration
//----------------------------------------------------------------------------
    // Return units of time in microseconds
     long int Utility::get_microseconds()
     {
         return(Utility::MICROSECONDS);
     } // end Utility::get_microseconds
//----------------------------------------------------------------------------
    // Return units of time in milliseconds
     short Utility::get_milliseconds()
     {
         return(Utility::MILLISECONDS);
     } // end Utility::get_milliseconds
//----------------------------------------------------------------------------
    // Adjust the sample rate of sensors if total sample rate > max cap of
    // target. Not supported currently
    float Utility::adjust_sample_rate(float sample_rate)
    {
        float tot_sample_rate = Parser::get_cumulative_sample_rate();
        if(tot_sample_rate < 0) {
            tot_sample_rate = sample_rate;
        }
        int max_cap = Parser::get_max_cap();
        return((max_cap / tot_sample_rate) * sample_rate);
    } // end adjust_sample_rate
//----------------------------------------------------------------------------
    // Get resampler suid
    sens_uid Utility::get_resampler_suid(see_salt* psalt)
    {
        sens_uid resampler_suid;
        std::string resampler = "resampler";
        std::vector<sens_uid *> sens_uids;
        psalt->get_sensors(resampler, sens_uids);

        if(sens_uids.size()) {
            resampler_suid.low = sens_uids[0]->low;
            resampler_suid.high = sens_uids[0]->high;
        } else {
                std::cout << "Resampler suid not found" << std::endl;
                std::cout << "FAIL" << std::endl;
          }
        return(resampler_suid);
    } // end of get_resampler_suid
//----------------------------------------------------------------------------
    // Return suid for sensor
    /*sens_uid* get_suid_for_sensor(sensor* const psensor,
                                        sens_uid* suid,
                                        sens_uid* const target_sensor_suid,
                                        const std::string& stream_type,
                                        const std::string& sensor_type
                                 )
    {
        bool std_req = false;
        bool on_change_req = false;
        bool single_output = false;

        if(stream_type == "std" || stream_type == "res") {
            std_req = true;
        }
        if(stream_type == "on_change") {
             on_change_req = true;
        }
        if(on_change_req && (
                                psensor->is_stream_type_on_change() ||
                                psensor->is_stream_type_single_output()
                            )) {
            suid = target_sensor_suid;
            return(suid);
        }
        else if(std_req && psensor->is_stream_type_streaming()) {
                 suid = target_sensor_suid;
                 return(suid);
        }
        else if(psensor->is_stream_type_streaming()) {
                suid = target_sensor_suid;
                return(suid);
        }
        else if("distance_bound" == psensor->get_type()) {
                suid = target_sensor_suid;
                return(suid);
        }
        else {
               std::cout << "Sensor SUID for " << sensor_type
                         << " not found. No stream request will be sent"
                         << " for " << sensor_type << std::endl << std::endl;
               std::cout << "FAIL" << std::endl;
               return(nullptr);
        }
    }*/ // end of get_suid_for_sensor
//----------------------------------------------------------------------------
    // Get target sensor suid
    sens_uid* Utility::get_target_sensor_suid(see_salt* psalt, sens_uid* suid,
                                              const std::string& sensor_type,
                                              const std::string& req_type,
                                              const std::string& rigid_body,
                                              const std::string& sensor_name)
    {
        // Required to print to stdout
        std::string save_sensor_type = sensor_type;

        // sensor_type_with_digit is passed as const reference
        // hence cannot be modified
        std::string tmp_sensor_type = sensor_type;

        // If muliple sensors with same name are present
        // digit is used to differientiate it (e.g. accel
        // and accel2). Remove digit from the sensor to get suid for the sensor
        tmp_sensor_type.erase(std::remove_if(tmp_sensor_type.begin(),
                                             tmp_sensor_type.end(),
                                             &isdigit), tmp_sensor_type.end());

        std::vector<sens_uid *> sensor_suids;
        // Get a vector of suids for the target sensor
        psalt->get_sensors(tmp_sensor_type, sensor_suids);

        if(0 == sensor_suids.size()) {
            std::cout << "SUID for " << save_sensor_type << " not found\n"
                      << std::endl;
        }
        else {
                bool std_req = false;
                bool on_change_req = false;
                bool db_req = false;

                if(req_type == "std" || req_type == "res") {
                    std_req = true;
                }
                if (req_type == "on_change") {
                    on_change_req = true;
                }
                if (req_type == "db") {
                    db_req = true;
                }

              for(size_t i = 0; i < sensor_suids.size(); ++i) {
                sens_uid* target_sensor_suid = sensor_suids[i];
                sensor* psensor = psalt->get_sensor(target_sensor_suid);

                if(psensor != nullptr) {
                    if ( rigid_body.size() > 0 ) {  // SENSORTEST-928
                       if ( psensor->has_rigid_body()) {
                          if (rigid_body != psensor->get_rigid_body()) {
                             continue;
                          }
                       }
                    }
                    // sensor has name(like "lsm6dsm")
                    if(!sensor_name.empty()) {
                      std::string name = psensor->get_name();
                      std::string type = psensor->get_type();
                      if(tmp_sensor_type == type && sensor_name == name) {
                         if (psensor->has_stream_type()) {
                             if(on_change_req &&
                                (psensor->is_stream_type_on_change() ||
                                    psensor->is_stream_type_single_output())) {
                                 suid = target_sensor_suid;
                                 break;
                             }
                             else if(std_req &&
                                      psensor->is_stream_type_streaming()) {
                                      suid = target_sensor_suid;
                                      break;
                             }
                             else if(db_req && "distance_bound" ==
                                                         psensor->get_type()) {
                                      suid = target_sensor_suid;
                                      break;
                             }
                             else {
                                    std::cout << "Sensor ";
                                    std::cout << save_sensor_type;
                                    std::cout << " not found or ";
                                    std::cout << "SUID for ";
                                    std::cout << save_sensor_type;
                                    std::cout << " not found. ";
                                    std::cout << "No stream request will be ";
                                    std::cout << "sent for " << save_sensor_type
                                              << std::endl << std::endl;
                                    std::cout << "FAIL" << std::endl;
                                    return(nullptr);
                             }
                          }
                        }
                    }
                   // sensor does not have a name but has only type
                   else {
                        if (psensor->has_stream_type()) {
                              if(on_change_req && (psensor->is_stream_type_on_change()
                                || psensor->is_stream_type_single_output())) {
                                  suid = target_sensor_suid;
                                  break;
                         }
                         else if(std_req && psensor->is_stream_type_streaming()) {
                                  suid = target_sensor_suid;
                                  break;
                         }
                         else if(db_req && "distance_bound" ==
                                            psensor->get_type()) {
                                 suid = target_sensor_suid;
                                 break;
                         }
                        }
                    }
                }
                else {
                        std::cout << "Not a valid sensor.... " << std::endl;
                        std::cout << "FAIL" << std::endl;
                        return(nullptr);
                }
             } // for loop
        }
        return(suid);
    } // end of get_target_sensor_suid
//----------------------------------------------------------------------------
    // Get adjusted rate, if max capacity has exceeded
    float Utility::max_cap_exceed(float& rate, int& max_cap, std::string&
                                  sensor_type)
    {
        float cumulative_rate = Parser::get_cumulative_sample_rate();
        float adjusted_rate = 0.0f;

        // If sensor has random odr rate (-3) or
        // has max rate (-1) or min rate (-2)
        if(cumulative_rate < 0) {
            if(rate > max_cap) {
                adjusted_rate = adjust_sample_rate(rate);
                std::cout << "Adjusted rate of " << sensor_type << " is ";
                std::cout << adjusted_rate << " Hz" << std::endl << std::endl;
            }
            else {
                adjusted_rate = rate;
            }
        }
        else {
            float total_rate = cumulative_rate + rate;
            if(total_rate > max_cap) {
                adjusted_rate = adjust_sample_rate(total_rate);
                std::cout << "Adjusted rate of " << sensor_type << " is ";
                std::cout << adjusted_rate << " Hz" << std::endl << std::endl;
            }
            else {
                adjusted_rate = total_rate;
            }
        }
        return(adjusted_rate);
    } // end of max_cap_exceed
//----------------------------------------------------------------------------
    // Get min odr rate
    float Utility::get_min_odr_rate(sensor* psensor, std::string& sensor_type)
    {
         // call default constructor to initialize empty vector
        std::vector<float> rates;
        psensor->get_rates(rates);

        float min_odr_rate = 0.0f;

        if(!rates.empty()) {
           int max_cap = Parser::get_max_cap();
           min_odr_rate = rates.front();

           std::cout << "Min sample/odr rate of " << sensor_type << ": ";
           std::cout << min_odr_rate << " Hz";
           std::cout << std::endl;

           // Check if capaity of the target has exceeded
           //min_odr_rate = max_cap_exceed(min_odr_rate, max_cap,
           //                                    sensor_type);

        } else {
            std::cout << "Missing min odr attribute" << std::endl;
            std::cout << "FAIL" << std::endl;
          }
        return(min_odr_rate);
    } // end of get_min_odr_rate
//----------------------------------------------------------------------------
    // Get max odr rate
    float Utility::get_max_odr_rate(sensor* psensor, std::string& sensor_type)
    {
        std::vector<float> rates;
        psensor->get_rates(rates);

        float max_odr_rate = 0.0f;

        if(!rates.empty()) {
           int max_cap = Parser::get_max_cap();
           max_odr_rate = rates.back();

           std::cout << "Max sample/odr rate of " << sensor_type << ": ";
           std::cout << max_odr_rate << " Hz";
           std::cout << std::endl;

           // Check if capaity of the target has exceeded
           //max_odr_rate = max_cap_exceed(max_odr_rate, max_cap,
           //                                    sensor_type);

        } else {
             std::cout << "Missing max odr attribute" << std::endl;
             std::cout << "FAIL" << std::endl;
          }
        return(max_odr_rate);
    } // end of get_max_odr_rate
//----------------------------------------------------------------------------
   // Standard message request
   salt_rc Utility::std_msg_req(see_salt* psalt, sens_uid* suid,
                                   const see_msg_id_e& msg_id_config,
                                   see_std_request& std_request,
                                   const float& rate)
    {
      salt_rc rc = SALT_RC_FAILED;
      see_std_sensor_config sample_rate(rate);
      std_request.set_payload(&sample_rate);

      see_client_request_message request(suid, msg_id_config, &std_request);
      rc = psalt->send_request(request);
      return(rc);
    } // end Message::std_msg_req
//----------------------------------------------------------------------------
    // Resampler message request
    salt_rc Utility::resampler_msg_req(see_salt* psalt, sens_uid* resampler_suid,
                                          sens_uid* suid,
                                          const see_msg_id_e& msg_id_config,
                                          see_std_request& std_request,
                                          const float& resampler_rate,
                                          see_resampler_rate& rate_type,
                                          bool filter)
    {
        salt_rc rc = SALT_RC_FAILED;
        see_resampler_config payload(suid, resampler_rate, rate_type, filter);
        std_request.set_payload(&payload);

        see_client_request_message request(resampler_suid, msg_id_config,
                                                    &std_request);
        rc = psalt->send_request(request);
        return(rc);
    } // end res_msg_req
//----------------------------------------------------------------------------
    // On-change message request
    salt_rc Utility::on_change_msg_req(see_salt* psalt, sens_uid* suid,
                                        const see_msg_id_e& msg_id_config,
                                        see_std_request& std_request)
    {
        salt_rc rc = SALT_RC_FAILED;
        see_client_request_message request(suid, msg_id_config, &std_request);
        rc = psalt->send_request(request);
        return rc;
    } // end on_change_msg_req
//----------------------------------------------------------------------------
    // Distance bound request
    salt_rc Utility::distance_bound_req(see_salt* psalt, sens_uid* suid,
                                         const see_msg_id_e &msg_id_config,
                                         see_std_request& std_request,
                                         const float &db_length)
    {
        salt_rc rc = SALT_RC_FAILED;
        // Payload
        see_set_distance_bound distance_bound(db_length);
        std_request.set_payload(&distance_bound);

        see_client_request_message request(suid, msg_id_config, &std_request);
        rc = psalt->send_request(request);
        return(rc);
    } // end distance_bound_req
//----------------------------------------------------------------------------
    // Basic gestures request
    salt_rc Utility::basic_gestures_req(see_salt* psalt, sens_uid* suid,
                                         const see_msg_id_e& msg_id_config,
                                         see_std_request& std_request)
    {
        salt_rc rc = SALT_RC_FAILED;
        // Payload
        see_basic_gestures_config config;
        std_request.set_payload(&config);
        see_client_request_message request(suid, msg_id_config, &std_request);
        rc = psalt->send_request(request);
        return(rc);
    } // end basic_gestures_req
//----------------------------------------------------------------------------
    // Psmd request
    salt_rc Utility::psmd_req(see_salt* psalt, sens_uid* suid,
                                const see_msg_id_e& msg_id_config,
                                see_std_request& std_request)
    {
        salt_rc rc = SALT_RC_FAILED;
        //Payload
        see_psmd_config config(SEE_PSMD_TYPE_STATIONARY);
        std_request.set_payload(&config);
        see_client_request_message request(suid, msg_id_config, &std_request);

        rc = psalt->send_request(request);
        return(rc);
    } // end psmd_req
//----------------------------------------------------------------------------
    // Multishake request
    salt_rc Utility::multishake_req(see_salt* psalt, sens_uid* suid,
                                        const see_msg_id_e& msg_id_config,
                                        see_std_request& std_request)
    {
        salt_rc rc = SALT_RC_FAILED;
        //Payload
        see_multishake_config config;
        std_request.set_payload(&config);
        see_client_request_message request(suid, msg_id_config, &std_request);

        rc = psalt->send_request(request);
        return(rc);
    } // end psmd_req
//----------------------------------------------------------------------------
    // Disable sensor request
    salt_rc Utility::disable_sensor(see_salt* psalt,
                                    const std::string &sensor_type,
                                    sens_uid* suid)
    {
        std::ostringstream oss;
        create_anchor_msg( oss, "disable", sensor_type, psalt);
        std::cout << oss.str() << std::endl;

        see_std_request std_request;
        see_client_request_message request(suid, MSGID_DISABLE_REQ, &std_request);

        salt_rc rc = psalt->send_request(request);

        std::ostringstream xss;
        create_anchor_msg( xss, "disable", sensor_type, psalt);
        append_rc( xss, rc);
        write_line( xss);

        return(rc);
    } // end Utility::disable_sensor
//----------------------------------------------------------------------------

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
        return 5;
    }

/**
 * return random integer between the values of min and max
 *
 * @param min
 * @param max
 *
 * @return int
 */
    int get_random_min_max( int min, int max)
    {
        int value = min + (rand() % static_cast<int>(max - min + 1));
        if ( min <= value && value <= max) {
            return value;
        }
        std::cout << "get_random_min_max_int OOB "
                  << min << " <= " << value << "<=" << max << std::endl;
        return min;
    }

/**
 * return random float between the values of min and max
 *
 * @param min
 * @param max
 *
 * @return float
 */
    float get_random_min_max( float min, float max)
    {
        static float rand_max = RAND_MAX;
        float fract = rand() / rand_max;
        float value = min + (max - min) * fract;
        if ( value >= min && value <= max) {
            return value;
        }
        std::cout << "get_random_min_max_float OOB "
                  << min << " <= " << value << "<=" << max << std::endl;
        return min;
    }


} // end sensors_stability namespace
///////////////////////////////////////////////////////////////////////////////
