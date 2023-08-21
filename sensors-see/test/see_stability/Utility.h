/* ===================================================================
**
** Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: Utility.h
** DESC: Contains generic utilities functions
** ================================================================ */

#ifndef UTILITY_H
#define UTILITY_H

#include <mutex>
#include "see_salt.h"

namespace sensors_stability {

float get_random_min_max( const float min, const float max);

class Utility {

private:
    std::mutex mu;

public:
    Utility();
    ~Utility() {}

    static const int MICROSECONDS;
    static const int MILLISECONDS;

    /*=========================================================================
     *               Function declarations
     =========================================================================*/

    /**
     * Return a value within user defined min:max range (e.g. 10:200)
     *
     * @param range passed as std::string pass-by-value
     * @return a pair of min and max as float
     */
     std::pair<float, float>get_min_max_range(std::string range);
//-----------------------------------------------------------------------------

    /**
     * Returns supported min and max of the range
     *
     * @param range passed as std::string pass-by-value
     * @return a pair of supported min and max as float
     */
     std::pair<float, float>get_supported_min_max_range(std::string range);

//-----------------------------------------------------------------------------
   /**
    * display why and pause_duration, and sleep for pause_duration
    *
    * @param why  - reason for pause
    * @param pause_dur - duration of pause (seconds)
    */
     void pause_duration( const std::string &why,
                          const float& pause_dur);
//-----------------------------------------------------------------------------

    /**
     * Sleep for 'n' seconds to stream sensors
     *
     * @param str_dur passed as const float reference
     * @return void
     */
     void stream_duration(const float& str_dur);
//-----------------------------------------------------------------------------

    /**
     * Delay for 'n' seconds
     *
     * @param delay_dur passed as const float reference
     * @return void
     */
     void delay_duration(const float& delay_dur);
//-----------------------------------------------------------------------------

    /**
     * Return units of time in microseconds
     *
     * @param void
     * @return long int
     */
     static long int get_microseconds();
//-----------------------------------------------------------------------------

    /**
     * Returns units of time in milliseconds
     *
     * @param void
     * @return short
     */
     static short get_milliseconds();
//-----------------------------------------------------------------------------
   /**
    * Disable sensor
    * @param psalt
    * @param suid
    *
    * @return salt_rc
    */
     salt_rc disable_sensor(see_salt* psalt, const std::string &sensor_type, sens_uid* suid);
//-----------------------------------------------------------------------------

    /**
     * Returns suid of target sensor
     *
     * @param psalt a pointer to see_salt
     * @param suid a pointer to sens_suid
     * @param sensor_type a const reference to std::string
     * @param req_type a const reference to std::string
     * @param rigid_body a const reference to std::string
     * @param sensor_name a const reference to std::string
     * @return pointer to sens_uid
     */
    sens_uid* get_target_sensor_suid(see_salt* psalt, sens_uid* suid,
                                     const std::string& sensor_type,
                                     const std::string& req_type,
                                     const std::string& rigid_body,
                                     const std::string& sensor_name);
//-----------------------------------------------------------------------------

    /**
     * Return resampler suid
     *
     * @param psalt a pointer to see_salt
     * @param suid a pointer to sens_suid
     * @return resampler_suid of type sens_uid
     */
     sens_uid get_resampler_suid(see_salt* psalt);
//-----------------------------------------------------------------------------

    // Get request type - Not used currently
    // std::string get_request_type(see_salt* psalt, sens_uid* suid);
//-------------------------------------------------------------------------

    /**
     * Check if the max straming cap of the target has exceeded
     * If, yes, then adjust the streaming rate of sensors
     * Currently not used
     *
     * @param rate as a reference to float
     * @param max_cap as a reference to int
     * @param sensor_type as a reference to std::string
     * @return adjusted rate as a float value
     */
     float max_cap_exceed(float& rate, int& max_cap, std::string& sensor_type);
//-----------------------------------------------------------------------------

    /**
     * Adjust the sample rate of each sensor/algos if the total sample rates of
     * all sensors or algos are greater than max capacity of target
     * Currently not used
     *
     * @param sample_rate passed as float pass-by-value
     * @return adjusted sample rate as float value
     */
     float adjust_sample_rate(float sample_rate);
//-----------------------------------------------------------------------------

    /**
     * Returns random odr rate
     *
     * @param psensor as pointer to sensor
     * @param sensor_type as reference to std::string
     * @param seed as reference to size_t
     * @return randomized odr rate as float value
     */
     float get_random_odr_rate(sensor* psensor, const std::string& sensor_type);
//-----------------------------------------------------------------------------

    /**
     * Returns min odr rate
     *
     * @param psensor as pointer to sensor
     * @param sensor_type as reference to std::string
     * @return min odr rate as float value
     */
     float get_min_odr_rate(sensor* psensor, std::string& sensor_type);
//-----------------------------------------------------------------------------

    /**
     * Returns max odr rate
     *
     * @param psensor as pointer to sensor
     * @param sensor_type as reference to std::string
     * @return max odr rate as float value
     */
     float get_max_odr_rate(sensor* psensor, std::string& sensor_type);
//-----------------------------------------------------------------------------

    /**
     * Standard message request
     *
     * @param psalt passed as pointer to see_salt
     * @param suid passed as pointer to sens_uid
     * @param msg_id_config passed as const reference to see_msg_id_e
     * @param std_request passed as reference to see_std_request
     * @param rate passed as const reference to float
     * @return salt_rc
     */
     salt_rc std_msg_req(see_salt* psalt, sens_uid* suid,
                           const see_msg_id_e& msg_id_config,
                           see_std_request& std_request, const float& rate);
//-----------------------------------------------------------------------------

    /**
     * Resampler message request
     *
     * @param psalt pointer to see_salt
     * @param resampler_suid passed as pointer to sens_uid
     * @param suid passed as pointer to sens_uid
     * @param msg_id_config passed as const reference to see_msg_id_e
     * @param std_request passed as reference to see_std_request
     * @param resampler_rate passed as const reference to float
     * @param rate_type passed as reference to see_resampler_rate
     * @param filter passed as value to bool. Default is false
     * @return salt_rc
     */
     salt_rc resampler_msg_req(see_salt* psalt, sens_uid* resampler_suid,
                                sens_uid* suid,
                                const see_msg_id_e& msg_id_config,
                                see_std_request& std_request,
                                const float& resampler_rate,
                                see_resampler_rate& rate_type,
                                bool filter = false);
//-----------------------------------------------------------------------------

    /**
     * On-change message request
     *
     * @param psalt passed as pointer to see_salt
     * @param suid passed as pointer to sens_uid
     * @param msg_id_config passed as const reference to see_msg_id_e
     * @param std_request passed as reference to see_std_request
     * @return salt_rc
     */
     salt_rc on_change_msg_req(see_salt* psalt, sens_uid* suid,
                                const see_msg_id_e& msg_id_config,
                                see_std_request& std_request);
//-----------------------------------------------------------------------------

    /**
     * Distance bound message request
     *
     * @param psalt passed as pointer to see_salt
     * @param suid passed as pointer to sens_uid
     * @param msg_id_config passed as const reference to see_msg_id_e
     * @param std_request passed as reference to see_std_request
     * @param db_length passed as const reference to float
     * @return salt_rc
     */
    salt_rc distance_bound_req(see_salt* psalt, sens_uid* suid,
                                const see_msg_id_e &msg_id_config,
                                see_std_request& std_request,
                                const float &db_length);
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
    salt_rc basic_gestures_req(see_salt* psalt, sens_uid* suid,
                                const see_msg_id_e& msg_id_config,
                                see_std_request& std_request);
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
    salt_rc psmd_req(see_salt* psalt, sens_uid* suid,
                     const see_msg_id_e& msg_id_config,
                     see_std_request& std_request);
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
    salt_rc multishake_req(see_salt* psalt, sens_uid* suid,
                           const see_msg_id_e& msg_id_config,
                           see_std_request& std_request);
//-----------------------------------------------------------------------------

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
    * hh:mm:ss.thh thread xxxxxxxxxxxx action detail psalt xxxxxxxxxxxx
    *
    * @param oss
    * @param action
    * @param detail
    * @param psalt
    */
    void create_anchor_msg( std::ostringstream &oss,
                            const std::string action,
                            const std::string detail,
                            const see_salt* psalt);

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
                           const see_salt* psalt);
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
                            const see_salt* psalt);


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

   /**
    * init random number generator
    *
    * @param seed
    */
    void init_random( unsigned int seed);

 }; // end Utility class
} // end sensors_stability namespace

///////////////////////////////////////////////////////////////////////////////

#endif /* UTILITY_H */
