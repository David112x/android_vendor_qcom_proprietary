/*
 * Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <mutex>
#include <list>
#include <thread>
#include <cutils/properties.h>
#include "sensor_factory.h"
#include "ssc_sensor.h"
#include "sns_resampler.pb.h"
#include "sns_cal.pb.h"
#include "sensors_timeutil.h"
#include "worker.h"

using namespace std;

static const char *SSC_DATATYPE_MAG = "mag";
static const char *SSC_DATATYPE_MAGCAL = "mag_cal";
static const uint32_t MAG_RESERVED_FIFO_COUNT = 600;
static const char SENSORS_HAL_PROP_ENABLE_MAG_FILTER[] = "persist.vendor.sensors.enable.mag_filter";
static char DIAG_LOG_MODULE_NAME[] = "sensors-hal";
#define MAG_FILTER_LENGTH (8)
#define MAG_AXES (3)

struct magcal_bias
{
  float bias[3];
  uint64_t timestamp;
  int8_t status;
};

/**
 * @brief class implementing both calibrated and uncalibrated
 *        magnetometer sensors
 *
 */
class magnetometer : public ssc_sensor
{
public:

    magnetometer(sensor_uid mag_suid, sensor_wakeup_type wakeup,
                 sensor_cal_type calibrated);
    virtual bool is_calibrated() override;
private:

    virtual void activate() override;

    virtual void deactivate() override;

    virtual void handle_sns_std_sensor_event(
        const sns_client_event_msg_sns_client_event& pb_event) override;

    void start_magcal();

    sensor_cal_type _cal;

    static int active_count;
    static bool magcal_available;
    static ssc_connection* magcal_conn;
    static sensor_uid magcal_suid;
    static bool magcal_initialized;
    static std::mutex cal_mutex;
    static std::list<magcal_bias> biases;
    /* status based on calibration accuracy */
    static int8_t sample_status;

    /*used to restart sensors after SSR*/
    static const uint8_t RETRY_CNT = 60;

    static bool mag_set_thread_name;
    static int handle;

    static void send_cal_req();
    static sensors_diag_log *_diag_magcal;
    static void magcal_conn_event_cb(const uint8_t *data, size_t size, uint64_t ts);
    static void magcal_conn_resp_cb(uint32_t resp_value);
    /* returns the latest magcal bias that is older than the timestamp */
    static magcal_bias get_cal_bias(uint64_t timestamp);
    /*create a seperate worker*/
    static worker *_magcal_worker;
    static bool _process_in_qmicallback;

    /*handle ssr and re initiate the request*/
    virtual void ssc_conn_error_cb(ssc_error_type e) override;
    /* variable for averaging filter */
    bool _filter_enabled;
    uint8_t _filter_buff_index;
    float _filter_buff[MAG_AXES][MAG_FILTER_LENGTH];
    float _filter_sum[MAG_AXES];
    bool _filter_buff_full;
};

/* static member variables */
int magnetometer::active_count = 0;
ssc_connection* magnetometer::magcal_conn;
sensors_diag_log* magnetometer::_diag_magcal;
sensor_uid magnetometer::magcal_suid;
bool magnetometer::magcal_initialized = false;
std::mutex magnetometer::cal_mutex;
int8_t magnetometer::sample_status = SENSOR_STATUS_UNRELIABLE;
bool magnetometer::magcal_available = false;
std::list<magcal_bias> magnetometer::biases;
bool magnetometer::_process_in_qmicallback;
worker *magnetometer::_magcal_worker;
bool magnetometer::mag_set_thread_name = false;
int magnetometer::handle = 0;

magnetometer::magnetometer(sensor_uid suid, sensor_wakeup_type wakeup,
                     sensor_cal_type calibrated):
    ssc_sensor(suid, wakeup),
    _cal(calibrated),
    _filter_enabled(false)
{
    if (_cal == SENSOR_CALIBRATED) {
        set_type(SENSOR_TYPE_MAGNETIC_FIELD);
        set_string_type(SENSOR_STRING_TYPE_MAGNETIC_FIELD);
        set_sensor_typename("Magnetometer");
    } else {
        set_type(SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED);
        set_string_type(SENSOR_STRING_TYPE_MAGNETIC_FIELD_UNCALIBRATED);
        set_sensor_typename("Magnetometer-Uncalibrated");
    }
#ifdef SNS_DIRECT_REPORT_SUPPORT
    set_direct_channel_flag(true);
#endif

    if (!magcal_initialized) {
        const auto& magcal_suids =
            sensor_factory::instance().get_suids(SSC_DATATYPE_MAGCAL);
        if (magcal_suids.size() == 0) {
            sns_loge("magcal is not available");
        } else {
            /* get first magcal suid (only one expected) */
            magcal_suid = magcal_suids[0];
            magcal_available = true;
        }
        magcal_initialized = true;
    }

    char prop_val[PROPERTY_VALUE_MAX];
    property_get(SENSORS_HAL_PROP_ENABLE_MAG_FILTER, prop_val, "false");
    if (!strncmp("true", prop_val, 4)) {
        _filter_enabled = true;
    }

    set_fifo_reserved_count(MAG_RESERVED_FIFO_COUNT);
    set_resampling(true);
    set_nowk_msgid(SNS_RESAMPLER_MSGID_SNS_RESAMPLER_CONFIG_EVENT);
    set_nowk_msgid(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT);
    _magcal_worker = nullptr;
    handle = get_sensor_info().handle;
}

void magnetometer::send_cal_req()
{
    string pb_req_msg_encoded;
    sns_client_request_msg pb_req_msg;

    if (magcal_conn == nullptr) {
        sns_logd("magcal_conn is closed, not sending request");
        return;
    }

    sns_logd("SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG");
    pb_req_msg.set_msg_id(SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG);
    pb_req_msg.mutable_request()->clear_payload();
    pb_req_msg.mutable_suid()->set_suid_high(magcal_suid.high);
    pb_req_msg.mutable_suid()->set_suid_low(magcal_suid.low);

    /* If current calibration status is "UNRELIABLE", set wakeup flag to
     * ensure next valid calibration parameters are sent immediately. */
    pb_req_msg.mutable_request()->mutable_batching()->
        set_batch_period(SENSOR_STATUS_UNRELIABLE != magnetometer::sample_status
          ? UINT32_MAX : 0);
    pb_req_msg.mutable_request()->mutable_batching()->
        set_flush_period(UINT32_MAX);
    pb_req_msg.mutable_susp_config()->set_delivery_type(
        SENSOR_STATUS_UNRELIABLE != magnetometer::sample_status
          ? SNS_CLIENT_DELIVERY_NO_WAKEUP : SNS_CLIENT_DELIVERY_WAKEUP);
    pb_req_msg.mutable_susp_config()->
        set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

    pb_req_msg.SerializeToString(&pb_req_msg_encoded);

    if(nullptr != _diag_magcal)
      _diag_magcal->log_request_msg(pb_req_msg_encoded, SSC_DATATYPE_MAGCAL, DIAG_LOG_MODULE_NAME);

    if(nullptr != magcal_conn){
        magcal_conn->send_request(pb_req_msg_encoded);
    }else {
        sns_logd("magcal_conn is closed, not sending request");
        return;
    }
}

void magnetometer::ssc_conn_error_cb(ssc_error_type e)
{
    sns_loge("handle error = %d for magnetometer", e);
    /* re-send magnetometer cal request when connection resets */
    if (magcal_available && e == SSC_CONNECTION_RESET) {
        bool got_sensorback = false;
        uint8_t retry_cnt = RETRY_CNT;
        sns_loge("connection reset, resend magnetometer cal request");
        //check for suid and then send config request
        while (retry_cnt && is_active()) {
            using namespace std::chrono;
            suid_lookup lookup(
                [&got_sensorback](const string& datatype, const auto& suids)
                {
                    sns_logd("got the suid call back");
                    int i;
                    for (i = 0; i < (int)suids.size(); i++) {
                      if ((suids[i].high == magcal_suid.high) &&
                          (suids[i].low == magcal_suid.low))
                          got_sensorback = true;
                    }
                });
            if (got_sensorback == true) {
                sns_logi("after ssr, discovery of %s took %f sec",
                    SSC_DATATYPE_MAGCAL,
                    (float)((RETRY_CNT-retry_cnt)*((float)WAIT_TIME_MS/MSEC_PER_SEC)));
                break;
            }
            lookup.request_suid(SSC_DATATYPE_MAGCAL, true);
            retry_cnt--;
            sns_logd("requesting suid (cnt) %d", retry_cnt);
            this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_MS));
        }
        if (is_active()) {
            if (got_sensorback)
                send_cal_req();
            else
                sns_loge("could not restart %s after ssr", SSC_DATATYPE_MAGCAL);
        } else {
            sns_logi("%s deactivated during ssr", SSC_DATATYPE_MAGCAL);
        }
    }
    /* do common error handling */
    ssc_sensor::ssc_conn_error_cb(e);
}
void magnetometer::magcal_conn_resp_cb(uint32_t resp_value)
{
  if(nullptr != _diag_magcal)
    _diag_magcal->log_response_msg(resp_value,SSC_DATATYPE_MAGCAL, DIAG_LOG_MODULE_NAME);
}
void magnetometer::start_magcal()
{
    /* create a new ssc connection for magcal */
    std::lock_guard<mutex> lock(cal_mutex);
    magcal_conn = new ssc_connection(magnetometer::magcal_conn_event_cb);
    if(nullptr != magcal_conn) {
      mag_set_thread_name = true;
      string thread_name = "see_"+to_string(handle);
      magcal_conn->set_worker_name(thread_name.c_str());
      magcal_conn->ssc_configuration(_is_rt_thread_enable , _atrace_delay_checktime_ms );
      magcal_conn->register_resp_cb(magnetometer::magcal_conn_resp_cb);
      _diag_magcal =  new sensors_diag_log((uint64_t)magcal_conn);
    }
    if (true == _process_in_qmicallback) {
      _magcal_worker =new worker;
      if (_magcal_worker) _magcal_worker->setname("magcal");
    }
    send_cal_req();
}

void magnetometer::activate()
{
    _process_in_qmicallback = _is_rt_thread_enable;
    if (!is_active()) {
        if (active_count == 0 && magcal_available) {
            sns_logd("start magcal");
            start_magcal();
        }
        active_count++;
        ssc_sensor::activate();
        sns_logi("mag_filter is %s", _filter_enabled ? "enabled":"disabled");
        /* initialize averaging filter variables */
        _filter_buff_full = false;
        _filter_buff_index = 0;
        for(int i = 0; i < MAG_AXES; i++) {
           for(int j = 0; j < MAG_FILTER_LENGTH; j++) {
               _filter_buff[i][j] = 0.0f;
           }
           _filter_sum[i] = 0.0f;
        }
    }
}

void magnetometer::deactivate()
{
    if (is_active()) {
        ssc_sensor::deactivate();
        active_count--;
        if (active_count == 0 && magcal_available) {
            /* close calibration connection */
            sns_logd("stop magcal");
            /* set the magcal_conn variable to null while protected by the
               mutex, this will make sure that any connection callbacks
               accessing this variable will not use it to send new requests */
            ssc_connection* temp;
            worker* temp_worker;
            sensors_diag_log * temp_diag;
            cal_mutex.lock();
            temp = magcal_conn;
            temp_worker = _magcal_worker;
            temp_diag = _diag_magcal;
            magcal_conn = nullptr;
           _magcal_worker = nullptr;
           _diag_magcal = nullptr;
            cal_mutex.unlock();
            delete temp;
            delete temp_worker;
            delete temp_diag;
        }
    }

}

magcal_bias magnetometer::get_cal_bias(uint64_t timestamp)
{
  lock_guard<mutex> lock(cal_mutex);
  if (biases.size() == 0)
  {
    magcal_bias b;
    b.bias[0] = 0.0f;
    b.bias[1] = 0.0f;
    b.bias[2] = 0.0f;
    b.timestamp = 0;
    b.status = SENSOR_STATUS_UNRELIABLE;
    return b;
  }

  auto cur= biases.begin();
  auto prev = cur;
  cur++;

  while (cur != biases.end())
  {
    if (cur->timestamp <= timestamp)
    {
      biases.erase(prev);
    }
    else
    {
      break;
    }
    prev = cur;
    cur++;
  }
  return *prev;
}

void magnetometer::magcal_conn_event_cb(const uint8_t *data, size_t size, uint64_t ts)
{
    if (mag_set_thread_name == true) {
        mag_set_thread_name = false;
        int ret_code = 0;
        string pthread_name = to_string(handle) + "_see";
        ret_code = pthread_setname_np(pthread_self(), pthread_name.c_str());
        if (ret_code != 0) {
            sns_loge("Failed to set ThreadName: %s\n", pthread_name.c_str());
        }
    }
    int8_t status;
    sns_client_event_msg pb_event_msg;
    pb_event_msg.ParseFromArray(data, size);

    string pb_event_msg_encoded((char *)data, size);

    lock_guard<mutex> lock(cal_mutex);
    if(nullptr != _diag_magcal)
      _diag_magcal->log_event_msg(pb_event_msg_encoded, SSC_DATATYPE_MAGCAL, DIAG_LOG_MODULE_NAME);

    status = magnetometer::sample_status;

    for (int i=0; i < pb_event_msg.events_size(); i++) {
        auto&& pb_event = pb_event_msg.events(i);
        sns_logv("event[%d] msg_id=%d", i, pb_event.msg_id());

        if (pb_event.msg_id() == SNS_CAL_MSGID_SNS_CAL_EVENT) {
            sns_cal_event pb_cal_event;
            pb_cal_event.ParseFromString(pb_event.payload());

            sample_status =
                ssc_sensor::sensors_hal_sample_status(pb_cal_event.status());

            magcal_bias b;
            b.bias[0] = pb_cal_event.bias(0);
            b.bias[1] = pb_cal_event.bias(1);
            b.bias[2] = pb_cal_event.bias(2);
            b.timestamp = pb_event.timestamp();
            b.status = sample_status;
            biases.push_back(b);

            sns_logd("magcal bias=(%f, %f, %f) status=%d",
                     b.bias[0], b.bias[1], b.bias[2], b.status);
        }
    }

    if((SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE == status &&
        SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE != magnetometer::sample_status) ||
       (SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE != status &&
        SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE == magnetometer::sample_status)) {
      if(_magcal_worker != nullptr){
        _magcal_worker->add_task(NULL,[] {
            magnetometer::send_cal_req();
        });
      } else {
        magnetometer::send_cal_req();
      }
    }
}

void magnetometer::handle_sns_std_sensor_event(
    const sns_client_event_msg_sns_client_event& pb_event)
{
    sns_std_sensor_event pb_sensor_event;
    float mag_data[MAG_AXES] = { 0.0f, 0.0f, 0.0f };
    pb_sensor_event.ParseFromString(pb_event.payload());

    if(pb_sensor_event.data_size()) {
        sensors_event_t hal_event = create_sensor_hal_event(pb_event.timestamp());
        /* select calibration bias based on timestamp */
        magcal_bias b = get_cal_bias(pb_event.timestamp());
        if (_filter_enabled) {
            /* applying averaging filter in HAL */
            if (_filter_buff_index < MAG_FILTER_LENGTH) {
                _filter_sum[0] += pb_sensor_event.data(0) - _filter_buff[0][_filter_buff_index];
                _filter_sum[1] += pb_sensor_event.data(1) - _filter_buff[1][_filter_buff_index];
                _filter_sum[2] += pb_sensor_event.data(2) - _filter_buff[2][_filter_buff_index];

                _filter_buff[0][_filter_buff_index] = pb_sensor_event.data(0);
                _filter_buff[1][_filter_buff_index] = pb_sensor_event.data(1);
                _filter_buff[2][_filter_buff_index] = pb_sensor_event.data(2);
            }

            if (_filter_buff_full) {
                mag_data[0] = (float)(_filter_sum[0]/MAG_FILTER_LENGTH);
                mag_data[1] = (float)(_filter_sum[1]/MAG_FILTER_LENGTH);
                mag_data[2] = (float)(_filter_sum[2]/MAG_FILTER_LENGTH);
            } else {
                mag_data[0] = (float)(_filter_sum[0]/(_filter_buff_index+1));
                mag_data[1] = (float)(_filter_sum[1]/(_filter_buff_index+1));
                mag_data[2] = (float)(_filter_sum[2]/(_filter_buff_index+1));
            }

            sns_logv("mag_filtering: value [%f, %f, %f], filtered value [%f, %f, %f], index = %d, full = %d",
                         pb_sensor_event.data(0), pb_sensor_event.data(1), pb_sensor_event.data(2),
                         mag_data[0], mag_data[1], mag_data[2],
                         _filter_buff_index, _filter_buff_full);

            _filter_buff_index++;
            if (_filter_buff_index == MAG_FILTER_LENGTH) {
                _filter_buff_index = 0;
                _filter_buff_full = true;
            }
        } else {
            mag_data[0] = pb_sensor_event.data(0);
            mag_data[1] = pb_sensor_event.data(1);
            mag_data[2] = pb_sensor_event.data(2);
        }

        if (_cal == SENSOR_CALIBRATED) {
            hal_event.magnetic.x = mag_data[0] - b.bias[0];
            hal_event.magnetic.y = mag_data[1] - b.bias[1];
            hal_event.magnetic.z = mag_data[2] - b.bias[2];
            hal_event.magnetic.status = STD_MIN(b.status,
                ssc_sensor::sensors_hal_sample_status(pb_sensor_event.status()));

            sns_logd("mag_sample_cal(magcal_available:%d): ts=%lld ns; value = [%f, %f, %f], status=%d",
                     (int)magcal_available, (long long) hal_event.timestamp, hal_event.magnetic.x,
                     hal_event.magnetic.y, hal_event.magnetic.z,
                     hal_event.magnetic.status);
        } else {
            hal_event.uncalibrated_magnetic.x_uncalib = mag_data[0];
            hal_event.uncalibrated_magnetic.y_uncalib = mag_data[1];
            hal_event.uncalibrated_magnetic.z_uncalib = mag_data[2];
            hal_event.uncalibrated_magnetic.x_bias = b.bias[0];
            hal_event.uncalibrated_magnetic.y_bias = b.bias[1];
            hal_event.uncalibrated_magnetic.z_bias = b.bias[2];

            sns_logd("mag_sample_uncal: ts=%lld ns; value = [%f, %f, %f],"
                     " bias = [%f, %f, %f]", (long long) hal_event.timestamp,
                     hal_event.uncalibrated_magnetic.x_uncalib,
                     hal_event.uncalibrated_magnetic.y_uncalib,
                     hal_event.uncalibrated_magnetic.z_uncalib,
                     hal_event.uncalibrated_magnetic.x_bias,
                     hal_event.uncalibrated_magnetic.y_bias,
                     hal_event.uncalibrated_magnetic.z_bias);
        }
        cal_sample_inerval(pb_event.timestamp());
        calculate_latency(hal_event.timestamp);
        cal_slpi_latency(pb_event.timestamp());
        submit_sensors_hal_event(hal_event);
    } else {
        sns_loge("empty data returned for mag");
    }
}

bool magnetometer::is_calibrated()
{
  return (_cal == SENSOR_CALIBRATED) ? true : false;
}

/* create all variants for calibrated mag sensor */
static vector<unique_ptr<sensor>> get_available_magnetometers_calibrated()
{
    const vector<sensor_uid>& mag_suids =
         sensor_factory::instance().get_suids(SSC_DATATYPE_MAG);
    vector<unique_ptr<sensor>> sensors;
    for (const auto& suid : mag_suids) {
        if (!(sensor_factory::instance().get_settings()
                                        & DISABLE_WAKEUP_SENSORS_FLAG)) {
            try {
                sensors.push_back(make_unique<magnetometer>(suid, SENSOR_WAKEUP,
                                                            SENSOR_CALIBRATED));
            } catch (const exception& e) {
                sns_loge("failed for wakeup, %s", e.what());
            }
        }
        try {
            sensors.push_back(make_unique<magnetometer>(suid, SENSOR_NO_WAKEUP,
                                                        SENSOR_CALIBRATED));
        } catch (const exception& e) {
            sns_loge("failed for nowakeup, %s", e.what());
        }
    }
    return sensors;
}

/* create all variants for uncalibrated mag sensor */
static vector<unique_ptr<sensor>> get_available_magnetometers_uncalibrated()
{
    const vector<sensor_uid>& mag_suids =
        sensor_factory::instance().get_suids(SSC_DATATYPE_MAG);
    vector<unique_ptr<sensor>> sensors;
    for (const auto& suid : mag_suids) {
        if (!(sensor_factory::instance().get_settings()
                                        & DISABLE_WAKEUP_SENSORS_FLAG)) {
            try {
                sensors.push_back(make_unique<magnetometer>(suid, SENSOR_WAKEUP,
                                                            SENSOR_UNCALIBRATED));
            } catch (const exception& e) {
                sns_loge("failed for wakeup,  %s", e.what());
            }
        }
        try {
            sensors.push_back(make_unique<magnetometer>(suid, SENSOR_NO_WAKEUP,
                                                        SENSOR_UNCALIBRATED));
        } catch (const exception& e) {
            sns_loge("failed for nowakeup, %s", e.what());
        }
    }
    return sensors;
}

static bool magnetometer_module_init()
{
    /* register supported sensor types with factory */
    sensor_factory::register_sensor(SENSOR_TYPE_MAGNETIC_FIELD,
                                    get_available_magnetometers_calibrated);
    sensor_factory::register_sensor(SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED,
                                    get_available_magnetometers_uncalibrated);
    sensor_factory::request_datatype(SSC_DATATYPE_MAG);
    sensor_factory::request_datatype(SSC_DATATYPE_MAGCAL);
    return true;
}

SENSOR_MODULE_INIT(magnetometer_module_init);
