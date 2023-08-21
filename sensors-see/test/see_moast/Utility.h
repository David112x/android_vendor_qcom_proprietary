/* ===================================================================
**
** Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: Utility.h
** DESC: Contains generic utilities functions
** ===================================================================*/

#ifndef UTILITY_H
#define UTILITY_H

#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::ostringstream

#include <mutex>
#include "see_salt.h"

namespace sensors_moast {
    class Utility {
        private:
            std::mutex mu;

        public:
            Utility();
            ~Utility() {}
//-----------------------------------------------------------------------------
            /**
             * Print file, line number and function name where exception/error
             * occurred
             *
             * @param file name as pointer to const char
             * @param line num as int
             * @param func name as pointer to const char
             * @return void
             */
            void print_file_line_number(const char* file_name, int line_num,
                                        const char* func_name);
//-----------------------------------------------------------------------------
            /**
             * Return randomized sample rate
             *
             * @param psensor passed as const pointer to sensor
             * @param available sample rate passed as const reference to float
             * @return randomized sample rate as float value
             */
            float get_randomized_sampling_rate(
                                               sensor* const psensor,
                                               const float&
                                                         available_sampling_rate
                                              );
//-----------------------------------------------------------------------------
            /**
             * Return randomized batch period
             *
             * @param sample rate passed as const reference to float
             * @param random duration passed as const reference to int
             * @param available sample rate passed as const reference to float
             * @return randomized batch period as float value
             */
            float get_randomized_batch_period(
                                                const float& sample_rate,
                                                const int& random_dur_per_req,
                                                const float&
                                                         available_sampling_rate
                                            );
//-----------------------------------------------------------------------------
            /**
             * Request to stream sensor
             *
             * @param psalt passed as const pointer to see_salt
             * @param suid passed as const pointer to sens_uid
             * @param msg id_config passed as const reference to see_msg_id_e
             * @param std request passed as reference to see_std_request
             * @param rate passed as const reference to float
             * @return salt_rc
             */
            salt_rc stream_sensor(
                                    see_salt* const psalt,
                                    sens_uid*
                                    const suid,
                                    see_msg_id_e msg_id,
                                    see_std_request& std_request,
                                    const float& rate
                                 );
//-----------------------------------------------------------------------------
            /**
             *
             * Resampler request
             *
             * @param psalt passed as const pointer to see_salt
             * @param resampler suid passed as const reference to sens_uid
             * @param target suid passed as const pointer to sens_uid
             * @param msg id_config passed as const reference to see_msg_id_e
             * @param sensor_type, const reference to std::string
             * @param rate, const reference to float
             * @return salt_rc
             */
             salt_rc resampler_req(
                                    see_salt* const psalt,
                                    sens_uid* const resampler_suid,
                                    sens_uid* const target_suid,
                                    see_msg_id_e msg_id,
                                    see_std_request& std_request,
                                    const float& rate
                                  );
//-----------------------------------------------------------------------------
            /**
             * Request to stream on-change
             *
             * @param psalt passed as const pointer to see_salt
             * @param suid passed as const pointer to sens_uid
             * @param msg id_config passed as const reference to see_msg_id_e
             * @param std request passed as reference to see_std_request
             * @return salt_rc
             */
             salt_rc on_change_sensor(
                                        see_salt* const psalt,
                                        sens_uid* const suid,
                                        see_msg_id_e msg_id,
                                        see_std_request& std_request
                                     );
//-----------------------------------------------------------------------------
            /**
            * Basic gestures message request
            *
            * @param psalt passed as pointer to see_salt
            * @param suid passed as pointer to sens_uid
            * @param msg_id_config passed as const reference to see_msg_id_e
            * @param std_request passed as reference to see_std_request
            * @return salt_rc
            */
            salt_rc basic_gesture_req(
                                         see_salt* const psalt,
                                         sens_uid* const suid,
                                         see_msg_id_e& msg_id_config,
                                         see_std_request& std_request
                                      );
//-----------------------------------------------------------------------------
            /**
             * Psmd message request
             *
             * @param psalt passed as pointer to see_salt
             * @param suid passed as pointer to sens_uid
             * @param msg_id_config passed as const reference to see_msg_id_e
             * @param std_request passed as reference to see_std_request
             * @return salt_rc
             */
            salt_rc psmd_req(
                               see_salt* const psalt,
                               sens_uid* const suid,
                               see_msg_id_e& msg_id_config,
                               see_std_request& std_request
                            );
//-----------------------------------------------------------------------------
            /**
             * Multishake message request
             *
             * @param psalt passed as pointer to see_salt
             * @param suid passed as pointer to sens_uid
             * @param msg_id_config passed as const reference to see_msg_id_e
             * @param std_request passed as reference to see_std_request
             * @return salt_rc
             */
            salt_rc multishake_req(
                                    see_salt* const psalt,
                                    sens_uid* const suid,
                                    see_msg_id_e& msg_id_config,
                                    see_std_request& std_request
                                  );
//-----------------------------------------------------------------------------
            /**
             * Request to stream db sensor
             *
             * @param psalt passed as const pointer to see_salt
             * @param suid passed as const pointer to sens_uid
             * @param msg id_config passed as const reference to see_msg_id_e
             * @param sensor type passed as const reference to std::string
             * @param db passed as const reference to float
             * @return salt_rc
             */
             salt_rc distance_bound_req
                                       (
                                            see_salt* const psalt,
                                            sens_uid* const suid,
                                            see_msg_id_e& msg_id_config,
                                            see_std_request& std_request,
                                            const float& db_length
                                        );
//-----------------------------------------------------------------------------
            /**
             *
             * Stream duration for selected sensors
             *
             * @param stream dur passed as const float reference
             * @return void
             */
             void stream_duration(const float& stream_dur);
//-----------------------------------------------------------------------------
           /**
            * Stops sensor streaming
            *
            * @param psalt passed as const pointer to see_salt
            * @param suid passed as const pointer to sens_uid
            * @return salt_rc
            */
            salt_rc disable_sensor(see_salt* const psalt, sens_uid* const suid);
//-----------------------------------------------------------------------------
            void init_random( unsigned int seed);
            float get_random();

            float get_random_odr_rate(sensor* psensor,
                                      const std::string& sensor_type);
            int get_random_min_max( int min, int max);
            float get_random_min_max( float min, float max);

            /**
            * initialize oss with a message of the form:
            * hh:mm:ss.thh thread xxxxxxxxxxxx action detail
            *
            * @param oss
            * @param action
            * @param detail
            */
            void create_anchor_msg( std::ostringstream &oss,
                                    const std::string action,
                                    const std::string detail);

           /**
            * initialize oss with a message of the form:
            * hh:mm:ss.thh action detail client
            *
            * @param oss
            * @param action
            * @param detail
            * @param rigid_body
            * @param psalt
            */
            void create_anchor_msg( std::ostringstream &oss,
                                    const std::string action,
                                    const std::string detail,
                                    const std::string rigid_body,
                                    int client);


           /**
            * initialize oss with a message of the form:
            * PASS detail thread xxxxxxxx psalt xxxxxxxx
            *
            * @param oss
            * @param grade
            * @param detail
            * @param psalt
            */
            void create_grade_msg( std::ostringstream &oss,
                                   bool grade,
                                   const std::string detail,
                                   const std::string rigid_body);

           /**
            * initialize oss with a message of the form:
            * status detail thread xxxxxxxx psalt xxxxxxxx
            *
            * @param oss
            * @param status
            * @param detail
            * @param psalt
            */
            void create_status_msg( std::ostringstream &oss,
                                    std::string status,
                                    const std::string detail,
                                    const std::string rigid_body);

           /**
            * appends sample_rate=<number> batch_period=<number> ... to xss
            *
            * @param xss
            * @param sample_rate
            * @param batch_period
            * @param flush_period
            * @param flush_only
            * @param max_batch
            */
            void append_extras( std::ostringstream &xss,
                                const float& sample_rate,
                                const float& batch_period,
                                const float& flush_period,
                                const bool& flush_only,
                                const bool& max_batch);

           /**
            * appends resampler_rate=<number> rate_type=fixed to xss
            *
            * @param xss
            * @param resample_rate - in hz
            * @param rate_type - "fixed" | "minimum"
            * @param filter -
            */
            void append_resampler( std::ostringstream &xss,
                                   const float &resample_rate,
                                   std::string rate_type,
                                   const bool &filter);
           /**
            * appends return_cocde=<number> to xss
            *
            * @param xss
            * @param rc
            */
            void append_rc( std::ostringstream &xss, salt_rc rc);

            void write_line( std::ostringstream &oss);

    }; // class Utility
} // end sensors_moast namespace
///////////////////////////////////////////////////////////////////////////////

#endif /* UTILITY_H */
