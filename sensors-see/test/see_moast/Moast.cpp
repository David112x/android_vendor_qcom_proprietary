/* ===================================================================
** Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: Moast.cpp
** DESC: Implementation of functions declared in Moast.h
** ====================================================================*/

#include <iostream>    // std::cout
#include <algorithm>   // std::min and std::max
#include <thread>      // std::thread
#include <cmath>       // std::floor
#include <stdexcept>   // std::exception/std::runtime_error

#include "Extras.h"
#include "Moast.h"
#include "Parser.h"
#include "Utility.h"

namespace sensors_moast
{
    // Return a singleton object
    Moast* Moast::get_moast_instance() {
        static Moast moast_instance;
        return &moast_instance;
    }
    // Get instance of moast class
    Moast* pmoast = Moast::get_moast_instance();
    // Create an instance of Utility class
    Utility putility;
    Extras extras;
    // Required by see_salt
    char* argv[] = {nullptr};
    const int Moast::MICROSECONDS = 1000000;

    std::vector<agent> active_players;
//-----------------------------------------------------------------------------
    // Initialize std message request
    void init_std_msg_request(see_std_request& std_request,
                                float& batch_period, float& flush_period,
                                bool& flush_only, bool& max_batch
                              )
    {
        if(batch_period) {
            std_request.set_batch_period(batch_period * Moast::MICROSECONDS);
        }
        if(flush_period) {
            std_request.set_flush_period(flush_period * Moast::MICROSECONDS);
        }
        if(flush_only) {
            std_request.set_flush_only(flush_only);
        }
        if(max_batch) {
            std_request.set_max_batch(max_batch);
        }
    } // end of init_std_msg_request
//-----------------------------------------------------------------------------
    // Basic gesture request
    salt_rc send_basic_gesture_req(see_salt* psalt, sens_uid* target_suid,
                                      see_std_request& std_request,
                                      float& batch_period, float& flush_period,
                                      bool& flush_only, bool& max_batch
                                  )
    {
            salt_rc rc = SALT_RC_FAILED;
            if(psalt == nullptr) {
                return(rc);
            }
            if(target_suid == nullptr) {
                return(rc);
            }
            see_msg_id_e msg_id_config = MSGID_BASIC_GESTURES_CONFIG;
            init_std_msg_request(std_request, batch_period,
                                       flush_period, flush_only, max_batch
                                );
            std::cout << std::endl;
            rc = putility.basic_gesture_req(psalt, target_suid, msg_id_config,
                                                    std_request);
            return(rc);
    } // end of send_basic_gesture_req
//-----------------------------------------------------------------------------
    // Psmd request
    salt_rc send_psmd_req(see_salt* psalt, sens_uid* target_suid,
                                see_std_request& std_request,
                                float& batch_period, float& flush_period,
                                bool& flush_only, bool& max_batch
                         )
    {
        salt_rc rc = SALT_RC_FAILED;
        if(psalt == nullptr) {
            return(rc);
        }
        if(target_suid == nullptr) {
            return(rc);
        }
        see_msg_id_e msg_id_config = MSGID_PSMD_CONFIG;
        init_std_msg_request(std_request, batch_period, flush_period,
                                          flush_only, max_batch);
        std::cout << std::endl;
        rc = putility.psmd_req(psalt, target_suid, msg_id_config, std_request);
        return(rc);
    } //end of send_psmd_req
//-----------------------------------------------------------------------------
    // Multishake request
    salt_rc send_multishake_req(see_salt* psalt, sens_uid* target_suid,
                                   see_std_request& std_request,
                                   float& batch_period, float& flush_period,
                                   bool& flush_only, bool& max_batch
                               )
    {
        salt_rc rc = SALT_RC_FAILED;
        if(psalt == nullptr) {
            return(rc);
        }
        if(target_suid == nullptr) {
            return(rc);
        }
        see_msg_id_e msg_id_config = MSGID_MULTISHAKE_CONFIG;
        init_std_msg_request(std_request, batch_period, flush_period,
                                          flush_only, max_batch);
        std::cout << std::endl;
        rc = putility.multishake_req(psalt, target_suid, msg_id_config,
                                                           std_request);
        return(rc);
    } // end of send_multishake_req
//-----------------------------------------------------------------------------
    // Db request
    salt_rc send_db_req(see_salt* psalt, sens_uid* target_suid,
                                see_std_request& std_request,
                                float& batch_period, float& flush_period,
                                bool& flush_only, bool& max_batch,
                                float& db_length
                        )
    {
         salt_rc rc = SALT_RC_FAILED;
         if(psalt == nullptr) {
            return(rc);
         }
         if(target_suid == nullptr) {
            return(rc);
         }
         see_msg_id_e msg_id_config = MSGID_SET_DISTANCE_BOUND;
         init_std_msg_request(std_request, batch_period, flush_period,
                                        flush_only, max_batch);
         rc = putility.distance_bound_req(psalt, target_suid,
                                                 msg_id_config,
                                                 std_request,
                                                 db_length);
         return(rc);
    } // end of send_db_req
//-----------------------------------------------------------------------------
    // Return batch period in seconds
    float get_batch_period(const float sample_rate)
    {
       if (pmoast == nullptr || sample_rate == 0
           || pmoast->available_apps_rpt_rate == 0) {
          return 0; // no batching
       }

       // perform batching about half the time
       float fract = putility.get_random();
       if ( fract < 0.5) {
          return 0; // no batching
       }

       // xxx_batch_period in seconds
       float min_batch_period = 1 / sample_rate;
       float max_batch_period = pmoast->get_step_duration();

       float batch_period = putility.get_random_min_max( min_batch_period,
                                                         max_batch_period);
       float batch_rate = 1 / batch_period;

       if ( batch_rate < pmoast->available_apps_rpt_rate) {
          pmoast->available_apps_rpt_rate -= batch_rate;
       }
       else {
          batch_rate = pmoast->available_apps_rpt_rate;
          batch_period = 1 / batch_rate;
          pmoast->available_apps_rpt_rate = 0;
       }

       return batch_period;
    } // end of get_batch_period
//-----------------------------------------------------------------------------
    // Return sample rate in hz
    float get_sample_rate(sensor* const psensor)
    {
        // Select sampling rate
        if(pmoast == nullptr || psensor == nullptr) {
           return 0;;
        }

        float sample_rate = putility.get_randomized_sampling_rate
                                        (
                                          psensor,
                                          pmoast->available_sampling_rate
                                        );
        if(sample_rate <= 0) {
            return 0;
        }

        if(sample_rate < pmoast->available_sampling_rate) {
            pmoast->available_sampling_rate -= sample_rate;
        }
        else {
           sample_rate = pmoast->available_sampling_rate;
           pmoast->available_sampling_rate = 0;
        }
        return sample_rate;
    } // end of get_sample_rate
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    void add_active_player(agent player) {
       active_players.push_back(player);
    }

    salt_rc remove_active_players(see_salt** ppsalt) {
       if (ppsalt == nullptr) {
           return SALT_RC_FAILED;
       }
       std::vector<agent>::size_type i;
       for (  i = 0; i < active_players.size(); i++) {
          agent player = active_players[i];

          see_salt *psalt = ppsalt[ player.client];
          if ( psalt == nullptr) {
             continue;
          }
          sens_uid *suid = player.suid;
          if ( suid == nullptr) {
             std::cout << "remove_active_players missing suid for "
                       << player.config.sensor_type << std::endl;
             continue;
          }

          std::string display_sensor_type = "";
          if (player.config.using_resampler) {
             display_sensor_type = "resampler.";
          }
          display_sensor_type += player.config.sensor_type;

          std::ostringstream oss;
          putility.create_anchor_msg(oss, "disable",
                                     display_sensor_type,
                                     player.config.rigid_body,
                                     player.client);
          salt_rc rc = putility.disable_sensor(psalt, suid);
          putility.append_rc(oss, rc);
          putility.write_line(oss);
       }
       return SALT_RC_SUCCESS;
    }

/**
 * activate a streaming physical or algo sensor using a random sample_rate
 * and random batch period.
 *
 * @param ppsalt
 * @param client - client index
 * @param player
 *
 * @return salt_rc
 */
    salt_rc stream_vanilla_sensor(see_salt** const ppsalt,
                                  int client,
                                  agent &player)
    {
       salt_rc rc = SALT_RC_FAILED;
       if (ppsalt == nullptr || pmoast == nullptr || player.psensor == nullptr) {
          return rc;
       }
       see_salt* psalt = ppsalt[client];
       if (psalt == nullptr) {
          return rc;
       }

       float sample_rate = get_sample_rate(player.psensor);
       if (sample_rate <= 0) {
          return rc;
       }
       float batch_period = get_batch_period(sample_rate); // seconds

       see_std_request std_request;
       float flush_period = 0.0f;
       bool flush_only = false;
       bool max_batch = false;
       init_std_msg_request(std_request, batch_period, flush_period,
                            flush_only, max_batch);

       std::ostringstream oss;
       putility.create_anchor_msg(oss, "enable s",
                                  player.config.sensor_type,
                                  player.config.rigid_body,
                                  player.client);
       putility.append_extras(oss, sample_rate, batch_period,
                              flush_period, flush_only, max_batch);

       rc = putility.stream_sensor(psalt,
                                   player.suid,
                                   MSGID_STD_SENSOR_CONFIG,
                                   std_request,
                                   sample_rate);
       putility.append_rc(oss, rc);
       putility.write_line(oss);

       if (rc == SALT_RC_SUCCESS) {
          add_active_player(player);
       }

       return rc;
    } // end stream_vanilla_sensor

/**
 * activate an on-change or single output sensor
 *
 * @param ppsalt
 * @param client
 * @param player
 *
 * @return salt_rc
 */
    salt_rc stream_on_change_sensor(see_salt** const ppsalt,
                                 int client,
                                 agent &player)
    {
       salt_rc rc = SALT_RC_FAILED;
       if (ppsalt == nullptr || pmoast == nullptr || player.psensor == nullptr) {
          return rc;
       }
       see_salt* psalt = ppsalt[client];
       if (psalt == nullptr) {
          return rc;
       }

       see_std_request std_request;
       float flush_period = 0.0f;
       bool flush_only = false;
       bool max_batch = false;
       float batch_period = 0.0f;

       std::ostringstream oss;
       putility.create_anchor_msg(oss, "enable o",
                                  player.config.sensor_type,
                                  player.config.rigid_body,
                                  client);
       putility.append_extras(oss, 0, batch_period,
                              flush_period,flush_only, max_batch);

       if("basic_gestures" == player.psensor->get_type()) {
           rc = send_basic_gesture_req(psalt, player.suid, std_request,
                                       batch_period, flush_period, flush_only,
                                       max_batch);
       }
       else if("psmd" == player.psensor->get_type()) {
           rc = send_psmd_req(psalt, player.suid, std_request, batch_period,
                              flush_period, flush_only, max_batch);
       }
       else if("multishake" == player.psensor->get_type()) {
           rc = send_multishake_req(psalt, player.suid, std_request,
                                    batch_period, flush_period,
                                    flush_only, max_batch);
       }
       else if("distance_bound" == player.psensor->get_type()) {
           float db_len = pmoast->db_length;
           rc = send_db_req(psalt, player.suid, std_request, batch_period,
                            flush_period, flush_only, max_batch ,db_len);
       }
       else {
             see_msg_id_e msg_id = MSGID_ON_CHANGE_CONFIG;
             init_std_msg_request(std_request, batch_period, flush_period,
                                  flush_only, max_batch);
             rc = putility.on_change_sensor(psalt, player.suid,
                                            msg_id, std_request);
       }

       putility.append_rc(oss, rc);
       putility.write_line(oss);

       if (rc == SALT_RC_SUCCESS) {
          add_active_player(player);
       }
       return rc;
    } // end stream on_change_sensor

/**
 * activate the resampler on the input player using a random sample_rate
 * and random batch period.
 *
 * @param ppsalt
 * @param client
 * @param player
 *
 * @return salt_rc
 */
    salt_rc stream_using_resampler(see_salt** const ppsalt,
                                  int client,
                                  agent &player)
    {
       salt_rc rc = SALT_RC_FAILED;
       if (ppsalt == nullptr || pmoast == nullptr || player.psensor == nullptr) {
          return rc;
       }
       see_salt* const psalt = ppsalt[client];
       if (psalt == nullptr) {
          return rc;
       }

       float sample_rate = get_sample_rate(player.psensor); // hz
       if(sample_rate <= 0) {
          return rc;
       }
       float batch_period = get_batch_period(sample_rate); // seconds

       see_std_request std_request;
       float flush_period = 0.0f;
       bool flush_only = false;
       bool max_batch = false;
       init_std_msg_request(std_request, batch_period, flush_period,
                            flush_only, max_batch);

       std::string display_sensor_type = "resampler."
                                         + player.config.sensor_type;
       std::ostringstream oss;
       putility.create_anchor_msg(oss, "enable r",
                                  display_sensor_type,
                                  player.config.rigid_body,
                                  client);
       putility.append_resampler(oss, sample_rate, "fixed", true);
       putility.append_extras(oss, 0, batch_period, flush_period,
                              flush_only, max_batch);

       rc = putility.resampler_req(psalt,
                                   player.suid,
                                   player.resampler_target_suid,
                                   MSGID_RESAMPLER_CONFIG,
                                   std_request,
                                   sample_rate);
       putility.append_rc(oss, rc);
       putility.write_line(oss);

       if (rc == SALT_RC_SUCCESS) {
          add_active_player(player);
       }
       return rc;
    } // end stream_using_resampler

/**
 * activate the gyro_cal and gyro or mag_cal and mag.
 *
 * @param ppsalt
 * @param client
 * @param player - either gyro_cal or mag_cal
 *
 * @return salt_rc
 */
    salt_rc stream_using_calibration(see_salt** const ppsalt,
                                     int client,
                                     agent &player)
    {
       salt_rc rc = SALT_RC_FAILED;
       if (ppsalt == nullptr || pmoast == nullptr || player.psensor == nullptr) {
          return rc;
       }
       see_salt* psalt = ppsalt[client];
       if (psalt == nullptr) {
          return rc;
       }

       // lookup primary sensor
       sensor_config primary;
       if ( player.config.sensor_type == "gyro_cal") {
          primary.sensor_type = "gyro";
       }
       else if ( player.config.sensor_type == "mag_cal") {
          primary.sensor_type = "mag";
       }
       else {
          std::cout << "FAIL: don't know how to use "
                    << player.config.sensor_type << std::endl;
          return rc;
       }
       primary.stream_type = "streaming";
       primary.rigid_body = player.config.rigid_body;
       primary.sensor_name = "";
       primary.combo_sensor = "";
       primary.using_resampler = false;
       primary.using_calibration = true;

       sensor *pprimary = extras.find_see_salt_target( psalt, primary);
       if (pprimary == nullptr) {
          std::cout << "FAIL: " << primary.sensor_type
                    << " on " << primary.rigid_body << " not found."
                    << std::endl;
          return rc;
       }

       agent kingpin;
       kingpin.config = primary;
       kingpin.psensor = pprimary;
       kingpin.client = client;
       kingpin.suid = pprimary->get_suid();

       rc = stream_on_change_sensor(ppsalt, client, player);
       if ( rc == SALT_RC_SUCCESS) {
          rc = stream_vanilla_sensor(ppsalt, client, kingpin);
       }
       return rc;
    } // end stream on_change_sensor

//-----------------------------------------------------------------------------

/**
 * Return true when the chosen sensor is already playing for the same client.
 * Note: already playing or already in-use.  The underlying see_salt / USTA
 * permits a sensor to be used only once per client. Returns true when the
 * chosen sensor is already inuse for the same client.
 *
 * @param chosen
 *
 * @return bool
 */
bool already_playing(agent &chosen)
{
   std::vector<agent>::size_type i;
   for ( i = 0; i < active_players.size(); i++) {
      agent *playing = &active_players[i];

      if (chosen.client == playing->client) {
         if (chosen.config.using_resampler && playing->config.using_resampler) {
            return true;
         }
         if (chosen.config.sensor_type == playing->config.sensor_type
             && chosen.config.rigid_body == playing->config.rigid_body
             && chosen.config.sensor_name == playing->config.sensor_name
             && chosen.config.stream_type == playing->config.stream_type) {
            return true;
         }
         // gyro_cal and mag_cal also run their corresponding primary sensor
         if (chosen.config.sensor_type == "gyro_cal") {
            chosen.config.sensor_type = "gyro";
            chosen.config.stream_type = "streaming";
            if ( already_playing(chosen)) {
               return true;
            }
            chosen.config.sensor_type = "gyro_cal";
            chosen.config.stream_type = "on-change";
         }
         if (chosen.config.sensor_type == "mag_cal") {
            chosen.config.sensor_type = "mag";
            chosen.config.stream_type = "streaming";
            if ( already_playing(chosen)) {
               return true;
            }
            chosen.config.sensor_type = "mag_cal";
            chosen.config.stream_type = "on-change";
         }
      }
   }
   return false;
}

/**
 * see_salt supports multiple different sensors per client.
 * however, running a particular sensor concurrently requires multiple clients.
 *
 * @author dfarrell (10/11/2019)
 *
 * @param client
 *
 * @return agent
 */
agent choose_player(int client)
{
   int i = 10;
   while ( --i > 0) {
       float fraction = putility.get_random();
       agent player = extras.get_agent(fraction);
       player.client = client;
       if (!already_playing(player)) {
          return player;
       }
   }

   agent dummy;
   dummy.config.sensor_type == "";
   dummy.psensor = nullptr;
   dummy.client = 0;
   dummy.suid = nullptr;
   dummy.resampler_target_suid = nullptr;
   return dummy;
}

//-----------------------------------------------------------------------------
    // Start moast test
    void start_moast_test(see_salt** const ppsalt)
    {
        std::cout << "\n\nStarting Moast Test" << std::endl;
        std::cout << "-------------------" << std::endl << std::endl;
        if(pmoast == nullptr) {
            putility.print_file_line_number(__FILE__, __LINE__,__func__);
            //throw "pmoast is nullptr.\n";
            return;
        }
        float test_duration = pmoast->max_step_duration;
        auto start_time = std::chrono::system_clock::now();
        auto end_time = start_time + std::chrono::duration_cast<std::chrono::
                                          system_clock::duration>
                                          (std::chrono::duration<float>
                                           (test_duration));
        int iteration_num = 0;
        while(true) {
              auto time_now = std::chrono::system_clock::now();
              if ( time_now >= end_time)
                 break;

              const int MIN_DURATION_PER_REQ = 0;
              float fmin = (float)MIN_DURATION_PER_REQ;
              float fmax = (float)pmoast->max_duration_per_req;

              std::chrono::duration<float, std::milli> time_remaining
                                                        = end_time - time_now;
              float secs_remaining = time_remaining.count() / 1000;
              //std::cout << "secs_remainings: " << secs_remaining << std::endl;
              if ( secs_remaining < 1.0) {
                 break;
              }

              fmax = ( fmax < secs_remaining) ? fmax : secs_remaining;
              float step_duration = putility.get_random_min_max( fmin, fmax);

              pmoast->set_step_duration(step_duration);
              std::cout << "RANDOM STREAM DURATION IN ITERATION "
                        << std::dec << ++iteration_num
                        //<< ", step_duration " + std::to_string(step_duration)
                        //<< " seconds"
                        << std::endl;

              pmoast->available_conc_sensor_cnt = std::floor
                        ( pmoast->max_conc_sensor_cnt/pmoast->max_qmi_clients);
              int players_per_client = pmoast->available_conc_sensor_cnt;

              // Stream sensors using each qmi client
              salt_rc rc = SALT_RC_SUCCESS;
              for ( int i = 0; i < pmoast->available_qmi_clients;
                    ++i && rc == SALT_RC_SUCCESS) {
                 for ( int j = 0; j < players_per_client;
                       j++ && rc == SALT_RC_SUCCESS) {

                       // choose player aka sensor to activate
                       agent player = choose_player( i);
                       if ( player.config.sensor_type == "") {
                          break; // no players available
                       }

                       if( player.config.using_resampler)
                       {
                          rc = stream_using_resampler(ppsalt, i, player);
                       }
                       else if( player.config.using_calibration)
                       {
                          rc = stream_using_calibration(ppsalt, i, player);
                       }
                       else if (player.config.stream_type == "on_change" ||
                                player.config.stream_type == "single_output") {
                          rc = stream_on_change_sensor(ppsalt, i, player);
                       }
                       else {
                          rc = stream_vanilla_sensor(ppsalt, i, player);
                       }
                    } // for players per client
                } // for clients

                float using_rate = pmoast->max_sampling_rate
                                   - pmoast->available_sampling_rate;
                std::string action = "sleep " + std::to_string(step_duration)
                                     + " seconds, "
                                     + " aggregate sample rate "
                                     + std::to_string(using_rate)
                                     + " Hz";
                std::ostringstream oss;
                putility.create_anchor_msg( oss, action, "");
                putility.write_line(oss);
                putility.stream_duration(step_duration);

                remove_active_players(ppsalt);

                // Reset to default values
                pmoast->available_sampling_rate = pmoast->max_sampling_rate;
                pmoast->available_apps_rpt_rate = pmoast->max_apps_rpt_rate;
                active_players.clear();
        }  // end of while loop - timer
    }  // end of start_moast_test
//------------------------------------------------------------------------------
    // Populate sens_suid map with sensor_type as key and its sensor suid as value
    void populate_sens_suid_map(see_salt* psalt)
    {
       if (pmoast == nullptr || psalt == nullptr) {
         return;
       }

       pmoast->sens_vec = Parser::get_vector_of_sensors();
       if(pmoast->sens_vec.size() == 0) {
          return;
       }

       for (size_t i = 0; i < pmoast->sens_vec.size(); ++i) {
          if (extras.populate_agents( psalt, pmoast->sens_vec[i])) {

          }
       }

       pmoast->stream_or_batch_vec = {"stream", "batch"};
    }
//------------------------------------------------------------------------------
    // Open psalt instances/qmi clients
    void Moast::open_qmi_clients()
    {
        if(pmoast == nullptr) {
            putility.print_file_line_number(__FILE__, __LINE__,__func__);
            return;
        }
        if(pmoast->max_qmi_clients <= 0) {
           std::cout << "invalid max_qmi_clients: " << pmoast->max_qmi_clients << std::endl;
           return;
        }
        // Number of qmi qmi clients
        see_salt** psalt = new(std::nothrow) see_salt*[pmoast->max_qmi_clients];
        if (psalt == nullptr) {
           std::cout << "fail to allocate memory for psalt" << std::endl;
           return;
        }
        std::cout << "Number of QMI clients: " << pmoast->max_qmi_clients;
        std::cout << '\n';
        std::cout << "------------------------" << std::endl;

        for(int i = 0; i < pmoast->max_qmi_clients; ++i) {
             psalt[i] = see_salt::get_instance();
             if(psalt[i] == nullptr) {
                 putility.print_file_line_number(__FILE__, __LINE__, __func__);
                 //throw "psalt not instantiated.\n";
                 return;
             }
             std::ostringstream oss;
             std::ostringstream action;
             action << "Client " << i << ": " << psalt[i] << "\n";
             putility.create_anchor_msg( oss, action.str(), "");
             putility.write_line(oss);

             psalt[i]->begin(); // populate see_salt sensors attributes
        }
        // Get resampler suid
        // Populate sens_suid map with sensor_type as keys and its suid as value
        populate_sens_suid_map(psalt[0]);
        // Start test
        start_moast_test(psalt);

        // Delete all opened qmi clients
        for(int i = 0; i < pmoast->max_qmi_clients; ++i) {
            int client = i + 1;
            std::ostringstream oss;
            std::ostringstream action;
            action << "Delete see_salt client " << client;
            putility.create_anchor_msg( oss, action.str(), "");
            putility.write_line(oss);

            delete psalt[i];
        }
    } // end of open_qmi_clients
// ----------------------------------------------------------------------------
    // Get config params
    void Moast::get_config_test_params()
    {
        if(pmoast == nullptr) {
           putility.print_file_line_number(__FILE__, __LINE__,__func__);
           //throw "pmoast is nullptr.\n";
           return;
        }
        // Get max qmi clients
         pmoast->max_qmi_clients = Parser::get_max_qmi_clients();
         pmoast->available_qmi_clients = pmoast->max_qmi_clients;
        // Get max concurrent sensor count
         pmoast->max_conc_sensor_cnt = Parser::get_max_conc_sensor_count();
        // Get max apps report rate
         pmoast->max_apps_rpt_rate = Parser::get_max_report_cnt_per_sec();
         pmoast->available_apps_rpt_rate = pmoast->max_apps_rpt_rate;
        // Get max step duration
         pmoast->max_step_duration = Parser::get_max_step_duration();
        //Get max duration per request
         pmoast->max_duration_per_req = Parser::get_max_duration_per_request();
         // Get max sampling rate
         pmoast->max_sampling_rate = Parser::get_max_sampling_rate();
         pmoast->available_sampling_rate = pmoast->max_sampling_rate;
        // Get db length
         pmoast->db_length = Parser::get_distance_bound();

        // Get usta_log_status
        bool usta_logging = Parser::get_usta_log_status();
        set_usta_logging(usta_logging);

    } // end of get_config_test_params
}; // end of namespace sensors_stability
