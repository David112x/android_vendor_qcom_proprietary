/* ===================================================================
**
** Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
** All Rights Reserved.
** Confidential and Proprietary - Qualcomm Technologies, Inc.
**
** FILE: StabilityEngine.h
** DESC: Singleton class. This class process the data from input file
*        and pass it to functions in TestCase class
** ================================================================ */

#ifndef STABILITYENGINE_H
#define STABILITYENGINE_H

namespace sensors_stability{

class StabilityEngine {

private:

    // Test params initialized in .cpp file
    static const std::string RIGID_BODY;
    static const std::string RANDOM_SENSOR; // TODO: Not implemented
    // Initialized to "req_type"
    static const std::string REQ_TYPE;
    // Initialized to "sample_rate"
    static const std::string SAMPLE_RATE;
    // Initialized to "odr_rate"
    static const std::string ODR_RATE;
    // Initialized to "rand_odr"
    static const std::string RAND_ODR;
    // Initialized to "sensor_attrib_name"
    static const std::string SENSOR_ATTRIB_NAME;

    // Initialized to "batch_period"
    static const std::string BATCH_PERIOD;
    // Initialized to "flush_period"
    static const std::string FLUSH_PERIOD;
    // Initialized to "flush_only"
    static const std::string FLUSH_ONLY;
    // Initialized to "flush_request_interval"
    static const std::string FLUSH_REQUEST_INTERVAL;
    // Initialized to "report_period"
    static const std::string REPORT_PERIOD;

    // Initialized to "std"
    static const std::string STD_REQ;
    // Initialized to "res"
    static const std::string RES_REQ;
    // Initialized to "cal"
    static const std::string CAL_REQ;
    // Initialized to "on_change"
    static const std::string ON_CHG_REQ;
    // Initialized to "db"
    static const std::string DIST_BOUND_REQ;

    // Initialized to "stream_duration"
    static const std::string STREAM_DURATION;
    // Initialized to "delay_before_req"
    static const std::string DELAY_BEFORE_REQ;

    // Initialized to "resampler_rate"
    static const std::string RESAMPLER_RATE;
    // Initialized to "resampler_rate_type"
    static const std::string RESAMPLER_RATE_TYPE;
    // Initialized to "resampler_filter"
    static const std::string RESAMPLER_FILTER;

    // Initialized to "wakeup"
    static const std::string WAKEUP;
    // Initialized to "no_wakeup"
    static const std::string NO_WAKEUP;

    // Initialized to "client"
    static const std::string CLIENT;
    // Initialized to "db_length"
    static const std::string DB_LENGTH;

    // Initialized to "stream_priority"
    static const std::string STREAM_PRIORITY;
    // Initilaized to "test_type"
    static const std::string TEST_TYPE;

    // Initialized to "disable_priority"
    static const std::string DISABLE_PRIORITY;
    // Initialized to "delay_before_next_disable_req"
    static const std::string DELAY_BEFORE_NEXT_DISABLE_REQ;

    // Keys to populate test_params map. '$' is used as a delimiter
    // e.g. accel$msg_id

    static const std::string RIGID_BOD;
    // Initialized to "$msg_id";
    static const std::string MSG_ID;
    //Initilaized to "$sens_attrib_name"
    static const std::string SENS_ATTRIB_NAME;

    // Initialized to "$min_sample_rate"
    static const std::string MIN_SAMP_RATE;
    // Initialized to "$max_sample_rate"
    static const std::string MAX_SAMP_RATE;
    // Initialized to "$sample_rate"
    static const std::string SAMP_RATE;

    // Initialized to "$min_odr_rate
    static const std::string MIN_ODR_RT;
    // Initialized to "$max_odr_rate
    static const std::string MAX_ODR_RT;
    // Initialized to "$odr_rate
    static const std::string ODR_RT;
    // Initialized to "$rand_odr_rt"
    static const std::string RAND_ODR_RT;

    // Initialized to "$min_rpt_period"
    static const std::string MIN_RPT_PERIOD;
    // Initialized to "$max_rpt_period"
    static const std::string MAX_RPT_PERIOD;
    // Initialized to "$rpt_period"
    static const std::string RPT_PERIOD;

    // Initialized to "$min_batch_period"
    static const std::string MIN_BTCH_PERIOD;
    // Initialized to "$max_batch_period"
    static const std::string MAX_BTCH_PERIOD;
    // Initialized to "$batch_period"
    static const std::string BTCH_PERIOD;

    // Initialized to "$min_flush_period"
    static const std::string MIN_FLSH_PERIOD;
    // Initialized to "$max_flush_period"
    static const std::string MAX_FLSH_PERIOD;
    // Initialized to "$flush_period"
    static const std::string FLSH_PERIOD;
    // Initialized to "$flush_only"
    static const std::string FLSH_ONLY;
    // Initialized to "$flush_req_interval"
    static const std::string FLSH_REQ_INTERVAL;

    // Initialized to "$min_stream_dur"
    static const std::string MIN_STREAM_DUR;
    // Initialized to "$max_stream_dur"
    static const std::string MAX_STREAM_DUR;
    // Initialized to "$min_stream_dur"
    static const std::string STREAM_DUR;

    // Initialized to "$min_delay_before_req"
    static const std::string MIN_DEL_BEFORE_REQ;
    // Initialized to "$max_delay_before_req"
    static const std::string MAX_DEL_BEFORE_REQ;
    // Initialized to "$delay_before_req"
    static const std::string DEL_BEFORE_REQ;

    // Initialized to "$min_resampler_rate"
    static const std::string MIN_RES_RATE;
    // Initialized to "$max_resampler_rate"
    static const std::string MAX_RES_RATE;
    // Initialized to "$resampler_rate"
    static const std::string RES_RATE;

    // Initialized to "$resampler_rate_type"
    static const std::string RES_RATE_TYPE;
    // Initialized to "$resampler_filter"
    static const std::string RES_FILTER;

    // Initialized to "$wakeup"
    static const std::string WK_UP;
    // Initialized to "$no_wakeup"
    static const std::string NO_WK_UP;

    // Initialized to "$client"
    static const std::string CLNT;
    // Initialized to "$db_len"
    static const std::string DB_LEN;

    // Initialzed to "$priority"
    static const std::string PRIORITY;
    // Initilaized to "$ttype"
    static const std::string TTYPE;

    // Initialzed to "$disable_pri"
    static const std::string DISABLE_PRI;
    // Initialized to "$del_bef_next_disable_req"
    static const std::string DEL_BEF_NEXT_DISABLE_REQ;

    // Map sensor configurations to sensor
    typedef std::multimap<std::string, std::string> sensor_configs;
    typedef std::multimap<std::string, sensor_configs>
                                                sensor_name_sensor_configs;

public:
    StabilityEngine() {}
    ~StabilityEngine() {}

    // Create a single instance of the class
    static StabilityEngine* get_stability_engine_instance();
    // Prevent construction of new object by copy constructor
    StabilityEngine(StabilityEngine const&) = delete;
    // Prevent construction of new object when assignment operator is used
    void operator=(StabilityEngine const&) = delete;

    const std::string index_2_rigid_body(float index);

//----------------------------------------------------------------------------

 /**
 * Process test parameters passed from input file.
 *
 * @param sensor_configs passed as reference to std::multimap<std::string,
 *        <std::string, std::string>>
 * @param test_params passed as reference to std::map<std::string, float>
 * @return void
 */
 void process_test_params(const sensor_name_sensor_configs& sensor_configs,
                          std::map<std::string, float>& test_params,
                          std::vector<std::string>& tc_specs);
//----------------------------------------------------------------------------

    // Enum class of message id's
    enum class req_message_ids {
        MSGID_FLUSH_REQ = 2,
        MSGID_DISABLE_REQ = 10,
        MSGID_RESAMPLER_CONFIG = 512,
        MSGID_STD_SENSOR_CONFIG = 513,
        MSGID_ON_CHANGE_CONFIG = 514,
        // Actual message id is 512 for gestures, multishake and psmd.
        // Since the stream type is on_change 514 is used for gestures,
        // multishake and psmd. In the actual meesage request, correct message
        // id will be sent
        MSGID_BASIC_GESTURES_CONFIG = 514,
        MSGID_MULTISHAKE_CONFIG = 514,
        MSGID_PSMD_CONFIG = 514,
        MSGID_SET_DISTANCE_BOUND = 516,
        MSGID_EVENT_GATED_CONFIG = 518,
        MSGID_DIAG_SENSOR_TRIGGER_REQ = 520,
    };
//----------------------------------------------------------------------------

    // Enum class of resampler rate type
    enum class resampler_rate_type {
        SEE_RESAMPLER_RATE_FIXED = 0,
        SEE_RESAMPLER_RATE_MINIMUM = 1,
    };
//----------------------------------------------------------------------------
    // Enum class of standard delivery
    enum class std_delivery {
        WAKEUP,
        NO_WAKEUP,
    };
//----------------------------------------------------------------------------
}; // end class

} // end namespace sensors_stability

///////////////////////////////////////////////////////////////////////////////

#endif /* STABILITYENGINE_H */
