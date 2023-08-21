/*
 * Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <mutex>
#include <list>
#include <thread>

#include "sensor_factory.h"
#include "ssc_sensor.h"
#include "sns_resampler.pb.h"
#include "sns_cal.pb.h"
#include "additionalinfo_sensor.h"
#include "sensors_timeutil.h"
#include "worker.h"

using namespace std;

using namespace std::placeholders;
static const char *SSC_DATATYPE_GYRO = "gyro";
static const char *SSC_DATATYPE_GYROCAL = "gyro_cal";
static const char *SSC_DATATYPE_TEMP    = "sensor_temperature";
static char DIAG_LOG_MODULE_NAME[] = "sensors-hal";
static const float RAD_PER_DEG = 3.1415 / 180.0;

struct gyrocal_bias
{
  float bias[3];
  uint64_t timestamp;
  int8_t status;
};

/**
 * @brief class implementing both calibrated and uncalibrated
 *        gyroscope sensors
 *
 */
class gyroscope : public ssc_sensor
{
public:
    gyroscope(sensor_uid gyro_suid, sensor_wakeup_type wakeup,
              sensor_cal_type calibrated);
    virtual bool is_calibrated() override;
private:
    /* see sensor::activate */
    virtual void activate() override;

    /* see sensor::deactivate */
    virtual void deactivate() override;

    virtual void handle_sns_std_sensor_event(
        const sns_client_event_msg_sns_client_event& pb_event) override;

    virtual void on_flush_complete() override;

    /* send gyro_temp additional info to hal */
    void send_additional_info(int64_t timestamp);

    void start_gyrocal();

    sensor_uid _gyrotemp_suid;
    sensor_cal_type _cal;
    bool _gyro_temp_available = false;
    bool _first_additional_info_sent = false;
    float _temp;
    std::unique_ptr<additionalinfo_sensor> _gyrotemp;
    void gyro_temp_event_cb(sensors_event_t hal_event);

    static int active_count;
    static ssc_connection* gyrocal_conn;
    static sensor_uid gyrocal_suid;
    static bool gyrocal_initialized;
    static std::mutex cal_mutex;
    static std::list<gyrocal_bias> biases;
    /* status based on calibration accuracy */
    static int8_t sample_status;

    /*used to restart sensors after SSR*/
    static const uint8_t RETRY_CNT = 60;

    static bool gyro_set_thread_name;
    static int handle;

    static void send_cal_req();
    static sensors_diag_log *_diag_gyrocal;
    static void gyrocal_conn_event_cb(const uint8_t *data, size_t size, uint64_t ts);
    static void gyrocal_conn_resp_cb(uint32_t resp_value);
    /* returns the latest gyrocal bias that is older than the timestamp */
    static gyrocal_bias get_cal_bias(uint64_t timestamp);
    /*create a seperate worker*/
    static worker *_gyrocal_worker;
    static bool _process_in_qmicallback;
    /*handle ssr and re initiate the request*/
    virtual void ssc_conn_error_cb(ssc_error_type e) override;
};

/* static member variables */
int gyroscope::active_count = 0;
ssc_connection* gyroscope::gyrocal_conn;
sensors_diag_log* gyroscope::_diag_gyrocal;
sensor_uid gyroscope::gyrocal_suid;
bool gyroscope::gyrocal_initialized = false;
std::mutex gyroscope::cal_mutex;
int8_t gyroscope::sample_status = SENSOR_STATUS_UNRELIABLE;
std::list<gyrocal_bias> gyroscope::biases;
bool gyroscope::_process_in_qmicallback;
worker *gyroscope::_gyrocal_worker;
bool gyroscope::gyro_set_thread_name = false;
int gyroscope::handle = 0;

gyroscope::gyroscope(sensor_uid gyro_suid, sensor_wakeup_type wakeup,
                     sensor_cal_type calibrated):
    ssc_sensor(gyro_suid, wakeup),
    _cal(calibrated),
    _gyro_temp_available(false)
{
    if (_cal == SENSOR_CALIBRATED) {
        set_type(SENSOR_TYPE_GYROSCOPE);
        set_string_type(SENSOR_STRING_TYPE_GYROSCOPE);
        set_sensor_typename("Gyroscope");
    } else {
        set_type(SENSOR_TYPE_GYROSCOPE_UNCALIBRATED);
        set_string_type(SENSOR_STRING_TYPE_GYROSCOPE_UNCALIBRATED);
        set_sensor_typename("Gyroscope-Uncalibrated");
    }
#ifdef SNS_DIRECT_REPORT_SUPPORT
    set_direct_channel_flag(true);
#endif

    if (gyrocal_initialized == false) {
        const auto& gyrocal_suids =
            sensor_factory::instance().get_suids(SSC_DATATYPE_GYROCAL);
        if (gyrocal_suids.size() == 0) {
            throw runtime_error("gyrocal suid not found");
        }
        /* get first gyrocal suid (only one expected) */
        gyrocal_suid = gyrocal_suids[0];
        gyrocal_initialized = true;
    }

    if(!sensor_factory::instance().get_pairedsuid(
                                    SSC_DATATYPE_TEMP,
                                    gyro_suid,
                                    _gyrotemp_suid))
    {
         _gyro_temp_available = true;
         set_additional_info_flag(true);
         sns_logd("gyro temperature is available..");
    }
    set_resampling(true);
    set_nowk_msgid(SNS_RESAMPLER_MSGID_SNS_RESAMPLER_CONFIG_EVENT);

    /* convert range from dps to rps */
    set_max_range(get_sensor_info().maxRange * RAD_PER_DEG);
    /* convert resolution for dps to rps */
    set_resolution(get_sensor_info().resolution * RAD_PER_DEG);
    set_nowk_msgid(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT);
    _gyrocal_worker = nullptr;
    handle = get_sensor_info().handle;
}

void gyroscope::gyro_temp_event_cb(sensors_event_t hal_event)
{
    _temp = hal_event.additional_info.data_float[0];
    send_additional_info(hal_event.timestamp);
}

void gyroscope::send_cal_req()
{
    string pb_req_msg_encoded;
    sns_client_request_msg pb_req_msg;

    if (gyrocal_conn == nullptr) {
        sns_logd("cal connection closed, not sending request");
        return;
    }

    sns_logd("SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG");
    pb_req_msg.set_msg_id(SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG);
    pb_req_msg.mutable_request()->clear_payload();
    pb_req_msg.mutable_suid()->set_suid_high(gyrocal_suid.high);
    pb_req_msg.mutable_suid()->set_suid_low(gyrocal_suid.low);

    /* If current calibration status is "UNRELIABLE", set wakeup flag to
     * ensure next valid calibration parameters are sent immediately. */
    pb_req_msg.mutable_request()->mutable_batching()->
        set_batch_period(SENSOR_STATUS_UNRELIABLE != gyroscope::sample_status
          ? UINT32_MAX : 0);
    pb_req_msg.mutable_request()->mutable_batching()->
        set_flush_period(UINT32_MAX);
    pb_req_msg.mutable_susp_config()->set_delivery_type(
        SENSOR_STATUS_UNRELIABLE != gyroscope::sample_status
          ? SNS_CLIENT_DELIVERY_NO_WAKEUP : SNS_CLIENT_DELIVERY_WAKEUP);
    pb_req_msg.mutable_susp_config()->
        set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

    pb_req_msg.SerializeToString(&pb_req_msg_encoded);

    if(nullptr != _diag_gyrocal)
      _diag_gyrocal->log_request_msg(pb_req_msg_encoded, SSC_DATATYPE_GYROCAL, DIAG_LOG_MODULE_NAME);

    if(nullptr != gyrocal_conn){
      gyrocal_conn->send_request(pb_req_msg_encoded);
    }else{
      sns_logd("cal connection closed, not sending request");
      return;
    }
}

void gyroscope::ssc_conn_error_cb(ssc_error_type e)
{
    sns_loge("handle %d error for gyroscope", e);
    /* re-send gyroscope cal request when connection resets */
    if (e == SSC_CONNECTION_RESET) {
        bool got_sensorback = false;
        uint8_t retry_cnt = RETRY_CNT;
        sns_loge("connection reset, resend gyroscope cal request");
        //check for suid and then send config request
        while (retry_cnt && is_active()) {
            using namespace std::chrono;
            suid_lookup lookup(
                [&got_sensorback](const string& datatype, const auto& suids)
                {
                    sns_logd("got the suid call back");
                    int i;
                    for (i = 0; i < (int)suids.size(); i++) {
                      if ((suids[i].high == gyrocal_suid.high) &&
                          (suids[i].low == gyrocal_suid.low))
                          got_sensorback = true;
                    }
                });
            if (got_sensorback == true) {
                sns_logi("after ssr, discovery of %s took %f sec",
                    SSC_DATATYPE_GYROCAL,
                    (float)((RETRY_CNT-retry_cnt)*((float)WAIT_TIME_MS/MSEC_PER_SEC)));
                break;
            }
            lookup.request_suid(SSC_DATATYPE_GYROCAL, true);
            retry_cnt--;
            sns_logd("requesting suid (cnt) %d", retry_cnt);
            this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_MS));
        }
        if (is_active()) {
            if (got_sensorback)
                send_cal_req();
            else
                sns_loge("could not restart %s after ssr", SSC_DATATYPE_GYROCAL);
        } else {
            sns_logi("%s deactivated during ssr", SSC_DATATYPE_GYROCAL);
        }
    }
    /* do common error handling */
    ssc_sensor::ssc_conn_error_cb(e);
}
void gyroscope::gyrocal_conn_resp_cb(uint32_t resp_value)
{
   if(nullptr != _diag_gyrocal)
     _diag_gyrocal->log_response_msg(resp_value,SSC_DATATYPE_GYROCAL, DIAG_LOG_MODULE_NAME);
}
void gyroscope::start_gyrocal()
{
    std::lock_guard<mutex> lock(cal_mutex);
    gyrocal_conn = new ssc_connection(gyroscope::gyrocal_conn_event_cb);
    if(nullptr != gyrocal_conn) {
      gyro_set_thread_name = true;
      string thread_name = "see_"+to_string(handle);
      gyrocal_conn->set_worker_name(thread_name.c_str());
      gyrocal_conn->ssc_configuration(_is_rt_thread_enable, _atrace_delay_checktime_ms);
      gyrocal_conn->register_resp_cb(gyroscope::gyrocal_conn_resp_cb);
      _diag_gyrocal =  new sensors_diag_log((uint64_t)gyrocal_conn);
    }
    if (true == _process_in_qmicallback) {
      _gyrocal_worker =new worker;
      if (_gyrocal_worker) _gyrocal_worker->setname("gyrocal");
    }
    send_cal_req();
}

void gyroscope::activate()
{
    _process_in_qmicallback = _is_rt_thread_enable;
    if (!is_active()) {

        if (active_count == 0) {
            sns_logd("start gyrocal");
            start_gyrocal();
        }

        active_count++;

        if(_gyro_temp_available) {
            _gyrotemp = make_unique<additionalinfo_sensor>(_gyrotemp_suid,
                              SENSOR_NO_WAKEUP,[this] (sensors_event_t hal_event)
                {
                    gyro_temp_event_cb(hal_event);
                });
            sensor_params config_params = get_params();
            /* recommended rate is 1/1000 of master */
            config_params.sample_period_ns = get_params().sample_period_ns * 1000;
            _gyrotemp->set_config(config_params);
            sns_logd("activate the gyro temp sensor with rate: %lu latency %lu",
                    (unsigned long)config_params.sample_period_ns,
                    (unsigned long)config_params.max_latency_ns);
            _gyrotemp->activate();
        }
        ssc_sensor::activate();
    }
}

void gyroscope::deactivate()
{
    if (is_active()) {
        ssc_sensor::deactivate();
        active_count--;

        /* disable gyrotemp */
        if(_gyro_temp_available && _gyrotemp) {
            _gyrotemp->deactivate();
            _gyrotemp.reset();
        }

        if (active_count == 0) {
            /* close calibration connection */
            sns_logd("stop gyrocal");
            /* set the gyrocal_conn variable to null while protected by the
               mutex, this will make sure that any connection callbacks
               accessing this variable will not use it to send new requests */
            ssc_connection* temp;
            worker* temp_worker;
            sensors_diag_log * temp_diag;
            cal_mutex.lock();
            temp = gyrocal_conn;
            temp_worker = _gyrocal_worker;
            temp_diag = _diag_gyrocal;
            gyrocal_conn = nullptr;
            _gyrocal_worker = nullptr;
            _diag_gyrocal = nullptr;
            cal_mutex.unlock();
            delete temp;
            delete temp_worker;
            delete temp_diag;
        }
    }
}

void gyroscope::send_additional_info(int64_t timestamp)
{
    sensors_event_t hal_event = create_sensor_hal_event(0);
    hal_event.type = SENSOR_TYPE_ADDITIONAL_INFO;
    sns_logd("gyro_temp_additional_info temp=%f, ts=%lld", _temp,
             (long long) timestamp);

    /* additional_info frame begin */
    hal_event.timestamp = timestamp;
    hal_event.additional_info.type = AINFO_BEGIN;
    submit_sensors_hal_event(hal_event);

    hal_event.timestamp++;
    hal_event.additional_info.type = AINFO_INTERNAL_TEMPERATURE;
    hal_event.additional_info.data_float[0] = _temp;
    submit_sensors_hal_event(hal_event);

    /* additional_info frame end */
    hal_event.timestamp++;
    hal_event.additional_info.type = AINFO_END;
    submit_sensors_hal_event(hal_event);
}

void gyroscope::on_flush_complete()
{
    if (_gyro_temp_available) {
        send_additional_info(android::elapsedRealtimeNano());
    }
}

gyrocal_bias gyroscope::get_cal_bias(uint64_t timestamp)
{
  lock_guard<mutex> lock(cal_mutex);
  if (biases.size() == 0)
  {
    gyrocal_bias b;
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

void gyroscope::gyrocal_conn_event_cb(const uint8_t *data, size_t size, uint64_t ts)
{
    if (gyro_set_thread_name == true) {
        gyro_set_thread_name = false;
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
    if(nullptr != _diag_gyrocal)
      _diag_gyrocal->log_event_msg(pb_event_msg_encoded, SSC_DATATYPE_GYROCAL, DIAG_LOG_MODULE_NAME);

    status = gyroscope::sample_status;

    for (int i=0; i < pb_event_msg.events_size(); i++) {
        auto&& pb_event = pb_event_msg.events(i);
        sns_logv("event[%d] msg_id=%d", i, pb_event.msg_id());

        if (pb_event.msg_id() == SNS_CAL_MSGID_SNS_CAL_EVENT) {
            sns_cal_event pb_cal_event;
            pb_cal_event.ParseFromString(pb_event.payload());
            sample_status =
                ssc_sensor::sensors_hal_sample_status(pb_cal_event.status());

            gyrocal_bias b;
            b.bias[0] = pb_cal_event.bias(0);
            b.bias[1] = pb_cal_event.bias(1);
            b.bias[2] = pb_cal_event.bias(2);
            b.timestamp = pb_event.timestamp();
            b.status = sample_status;
            biases.push_back(b);

            sns_logd("gyrocal bias=(%f, %f, %f) status=%d",
                     b.bias[0], b.bias[1], b.bias[2], b.status);
        }
    }

    if((SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE == status &&
        SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE != gyroscope::sample_status) ||
       (SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE != status &&
        SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE == gyroscope::sample_status)) {
        if(_gyrocal_worker != nullptr) {
          _gyrocal_worker->add_task(NULL,[] {
              gyroscope::send_cal_req();
          });
        } else {
          gyroscope::send_cal_req();
        }
    }
}

void gyroscope::handle_sns_std_sensor_event(
    const sns_client_event_msg_sns_client_event& pb_event)
{
    sns_std_sensor_event pb_sensor_event;
    pb_sensor_event.ParseFromString(pb_event.payload());

    if(pb_sensor_event.data_size()) {
        sensors_event_t hal_event = create_sensor_hal_event(pb_event.timestamp());

        /* select calibration bias based on timestamp */
        gyrocal_bias b = get_cal_bias(pb_event.timestamp());

        if (_cal == SENSOR_CALIBRATED) {
            hal_event.gyro.x = pb_sensor_event.data(0) - b.bias[0];
            hal_event.gyro.y = pb_sensor_event.data(1) - b.bias[1];
            hal_event.gyro.z = pb_sensor_event.data(2) - b.bias[2];
            hal_event.gyro.status = STD_MIN(b.status,
                ssc_sensor::sensors_hal_sample_status(pb_sensor_event.status()));

            sns_logd("gyro_sample_cal: ts=%lld ns; value = [%f, %f, %f], status = %d",
                     (long long) hal_event.timestamp,
                     hal_event.gyro.x, hal_event.gyro.y, hal_event.gyro.z,
                     hal_event.gyro.status);
        } else {
            hal_event.uncalibrated_gyro.x_uncalib = pb_sensor_event.data(0);
            hal_event.uncalibrated_gyro.y_uncalib = pb_sensor_event.data(1);
            hal_event.uncalibrated_gyro.z_uncalib = pb_sensor_event.data(2);
            hal_event.uncalibrated_gyro.x_bias = b.bias[0];
            hal_event.uncalibrated_gyro.y_bias = b.bias[1];
            hal_event.uncalibrated_gyro.z_bias = b.bias[2];

            sns_logd("gyro_sample_uncal: ts=%lld ns; value = [%f, %f, %f],"
                     " bias = [%f, %f, %f]", (long long) hal_event.timestamp,
                     hal_event.uncalibrated_gyro.x_uncalib,
                     hal_event.uncalibrated_gyro.y_uncalib,
                     hal_event.uncalibrated_gyro.z_uncalib,
                     hal_event.uncalibrated_gyro.x_bias,
                     hal_event.uncalibrated_gyro.y_bias,
                     hal_event.uncalibrated_gyro.z_bias);
        }
        cal_sample_inerval(pb_event.timestamp());
        calculate_latency(hal_event.timestamp);
        cal_slpi_latency(pb_event.timestamp());
        submit_sensors_hal_event(hal_event);

        if (_gyro_temp_available && !_first_additional_info_sent) {
            send_additional_info(hal_event.timestamp);
            _first_additional_info_sent = true;
        }
    } else {
        sns_loge("empty data returned for gyro");
    }
}

bool gyroscope::is_calibrated()
{
  return (_cal == SENSOR_CALIBRATED) ? true : false;
}

/* create sensor variants supported by this class and register the function
   to sensor_factory */
static vector<unique_ptr<sensor>> get_available_gyroscopes_calibrated()
{
    const vector<sensor_uid>& gyro_suids =
         sensor_factory::instance().get_suids(SSC_DATATYPE_GYRO);
    vector<unique_ptr<sensor>> sensors;
    for (const auto& suid : gyro_suids) {
        if (!(sensor_factory::instance().get_settings()
                                & DISABLE_WAKEUP_SENSORS_FLAG)) {
            try {
                sensors.push_back(make_unique<gyroscope>(suid, SENSOR_WAKEUP,
                                                         SENSOR_CALIBRATED));
            } catch (const exception& e) {
                sns_loge("failed for wakeup, %s", e.what());
            }
        }
        try {
            sensors.push_back(make_unique<gyroscope>(suid, SENSOR_NO_WAKEUP,
                                                     SENSOR_CALIBRATED));
        } catch (const exception& e) {
            sns_loge("failed for nowakeup, %s", e.what());
        }
    }
    return sensors;
}

static vector<unique_ptr<sensor>> get_available_gyroscopes_uncalibrated()
{
    const vector<sensor_uid>& gyro_suids =
        sensor_factory::instance().get_suids(SSC_DATATYPE_GYRO);
    vector<unique_ptr<sensor>> sensors;
    for (const auto& suid : gyro_suids) {
        if (!(sensor_factory::instance().get_settings()
                                  & DISABLE_WAKEUP_SENSORS_FLAG)) {
            try {
                sensors.push_back(make_unique<gyroscope>(suid, SENSOR_WAKEUP,
                                                         SENSOR_UNCALIBRATED));
            } catch (const exception& e) {
                sns_loge("failed for wakeup,  %s", e.what());
            }
        }
        try {
            sensors.push_back(make_unique<gyroscope>(suid, SENSOR_NO_WAKEUP,
                                                     SENSOR_UNCALIBRATED));
        } catch (const exception& e) {
            sns_loge("failed for nowakeup, %s", e.what());
        }
    }
    return sensors;
}

static bool gyroscope_module_init()
{
    /* register supported sensor types with factory */
    sensor_factory::register_sensor(SENSOR_TYPE_GYROSCOPE,
                                    get_available_gyroscopes_calibrated);
    sensor_factory::register_sensor(SENSOR_TYPE_GYROSCOPE_UNCALIBRATED,
                                    get_available_gyroscopes_uncalibrated);
    sensor_factory::request_datatype(SSC_DATATYPE_GYRO);
    sensor_factory::request_datatype(SSC_DATATYPE_GYROCAL);
    sensor_factory::request_datatype(SSC_DATATYPE_TEMP);
    return true;
}

SENSOR_MODULE_INIT(gyroscope_module_init);
