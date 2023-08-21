/*
 * Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <mutex>
#include <list>
#include <thread>
#include <cinttypes>

#include "sensor_factory.h"
#include "ssc_sensor.h"
#include "sns_resampler.pb.h"
#include "sns_cal.pb.h"
#include "additionalinfo_sensor.h"
#include "sensors_timeutil.h"
#include "worker.h"
#include "sensors_qti.h"

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
  sns_std_sensor_sample_status status;
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
              sensor_cal_type calibrated, bool default_sensor);
    virtual bool is_calibrated() override;
private:
    /* see sensor::activate */
    virtual void activate() override;

    /* see sensor::deactivate */
    virtual void deactivate() override;

    void start_gyrocal();

    sensor_uid _gyrotemp_suid;
    sensor_cal_type _cal;
    bool _gyro_temp_available = false;
    std::unique_ptr<additionalinfo_sensor> _gyrotemp;

    ssc_connection* _gyrocal_conn;
    sensor_uid _gyrocal_suid;
    std::mutex _cal_mutex;
    std::list<gyrocal_bias> _biases;
    /* status based on calibration accuracy */
    sns_std_sensor_sample_status _sample_status;

    /*used to restart sensors after SSR*/
    static const uint8_t RETRY_CNT = 60;

    bool gyro_set_thread_name = false;

    void send_cal_req();
    sensors_diag_log *_diag_gyrocal;

    void handle_sns_client_event(
        const sns_client_event_msg_sns_client_event& pb_event);

    void gyrocal_conn_event_cb(const uint8_t *data, size_t size, uint64_t ts);
    void gyrocal_conn_resp_cb(uint32_t resp_value);
    /* returns the latest gyrocal bias that is older than the timestamp */
    gyrocal_bias get_cal_bias(uint64_t timestamp);
    /*create a seperate worker*/
    worker *_gyrocal_worker;
    bool _process_in_qmicallback;
    sensor_wakeup_type _gyro_temp_wakeup_type;
    /*handle ssr and re initiate the request*/
    virtual void ssc_conn_error_cb(ssc_error_type e) override;
};

gyroscope::gyroscope(sensor_uid gyro_suid, sensor_wakeup_type wakeup,
                     sensor_cal_type calibrated, bool default_sensor):
    ssc_sensor(gyro_suid, wakeup),
    _cal(calibrated),
    _gyro_temp_available(false),
    _gyrocal_conn(nullptr),
    _sample_status(SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE),
    _gyrocal_worker(nullptr),
    _gyro_temp_wakeup_type(wakeup)
{
    if (default_sensor) {
        if (_cal == SENSOR_CALIBRATED) {
            set_type(SENSOR_TYPE_GYROSCOPE);
            set_string_type(SENSOR_STRING_TYPE_GYROSCOPE);
            set_sensor_typename("Gyroscope");
        } else {
            set_type(SENSOR_TYPE_GYROSCOPE_UNCALIBRATED);
            set_string_type(SENSOR_STRING_TYPE_GYROSCOPE_UNCALIBRATED);
            set_sensor_typename("Gyroscope-Uncalibrated");
        }
    } else {
        if (_cal == SENSOR_CALIBRATED) {
            set_type(QTI_SENSOR_TYPE_GYROSCOPE);
            set_string_type(QTI_SENSOR_STRING_TYPE_GYROSCOPE);
            set_sensor_typename("QTI-Gyroscope");
        } else {
            set_type(QTI_SENSOR_TYPE_GYROSCOPE_UNCALIBRATED);
            set_string_type(QTI_SENSOR_STRING_TYPE_GYROSCOPE_UNCALIBRATED);
            set_sensor_typename("QTI-Gyroscope-Uncalibrated");
        }
    }
#ifdef SNS_DIRECT_REPORT_SUPPORT
    set_direct_channel_flag(true);
#endif

    if(sensor_factory::instance().get_pairedcalsuid(
                                    SSC_DATATYPE_GYROCAL,
                                    gyro_suid,
                                    _gyrocal_suid) != 0) {
        throw runtime_error("gyrocal suid not found");
    }

    if(!sensor_factory::instance().get_pairedsuid(
                                    SSC_DATATYPE_TEMP,
                                    gyro_suid,
                                    _gyrotemp_suid)) {
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
    _process_in_qmicallback = true;
}

void gyroscope::send_cal_req()
{
    string pb_req_msg_encoded;
    sns_client_request_msg pb_req_msg;

    if (_gyrocal_conn == nullptr) {
        sns_logd("cal connection closed, not sending request");
        return;
    }

    sns_logd("SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG");
    pb_req_msg.set_msg_id(SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG);
    pb_req_msg.mutable_request()->clear_payload();
    pb_req_msg.mutable_suid()->set_suid_high(_gyrocal_suid.high);
    pb_req_msg.mutable_suid()->set_suid_low(_gyrocal_suid.low);

    /* If current calibration status is "UNRELIABLE", set wakeup flag to
     * ensure next valid calibration parameters are sent immediately. */
    pb_req_msg.mutable_request()->mutable_batching()->
        set_batch_period(SENSOR_STATUS_UNRELIABLE != _sample_status
          ? UINT32_MAX : 0);
    pb_req_msg.mutable_request()->mutable_batching()->
        set_flush_period(UINT32_MAX);
    pb_req_msg.mutable_susp_config()->set_delivery_type(
        SENSOR_STATUS_UNRELIABLE != _sample_status
          ? SNS_CLIENT_DELIVERY_NO_WAKEUP : SNS_CLIENT_DELIVERY_WAKEUP);
    pb_req_msg.mutable_susp_config()->
        set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

    pb_req_msg.SerializeToString(&pb_req_msg_encoded);

    if(nullptr != _diag_gyrocal)
      _diag_gyrocal->log_request_msg(pb_req_msg_encoded, SSC_DATATYPE_GYROCAL, DIAG_LOG_MODULE_NAME);

    if ( nullptr == _gyrocal_conn) {
      sns_logd("cal connection closed, not sending request");
      return;
    }else{
      _gyrocal_conn->send_request(pb_req_msg_encoded, false);
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
                [&got_sensorback, this](const string& datatype, const auto& suids)
                {
                    sns_logd("got the suid call back");
                    int i;
                    for (i = 0; i < (int)suids.size(); i++) {
                      if ((suids[i].high == _gyrocal_suid.high) &&
                          (suids[i].low == _gyrocal_suid.low))
                          got_sensorback = true;
                    }
                });
            if (got_sensorback == true) {
                sns_logi("after ssr, discovery of %s took %f sec",
                    SSC_DATATYPE_GYROCAL,
                    (float)((RETRY_CNT-retry_cnt)*((float)WAIT_TIME_MS/MSEC_PER_SEC)));
                break;
            }
            lookup.request_suid(SSC_DATATYPE_GYROCAL, false);
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
   std::lock_guard<mutex> lock(_cal_mutex);
   if(nullptr != _diag_gyrocal)
     _diag_gyrocal->log_response_msg(resp_value,SSC_DATATYPE_GYROCAL, DIAG_LOG_MODULE_NAME);
}

void gyroscope::start_gyrocal()
{
    std::lock_guard<mutex> lock(_cal_mutex);
    _gyrocal_conn = new ssc_connection(
        [this](const uint8_t *data, size_t size, uint64_t ts)
        {
            gyrocal_conn_event_cb(data, size, ts);
        });

    if(nullptr != _gyrocal_conn) {
      gyro_set_thread_name = true;
      string thread_name = "see_"+to_string(get_sensor_info().handle);
      _gyrocal_conn->set_worker_name(thread_name.c_str());
      _gyrocal_conn->ssc_configuration(_is_rt_thread_enable, _atrace_delay_checktime_ms);
      if(true == _is_worker_bypass){
          _gyrocal_conn->ssc_configuration(_is_worker_bypass);
       }
      _gyrocal_conn->register_resp_cb(
          [this](uint32_t resp_value)
          {
              gyrocal_conn_resp_cb(resp_value);
          });
      _diag_gyrocal =  new sensors_diag_log();
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
    if(_is_worker_bypass){
    _process_in_qmicallback = _is_worker_bypass;
    }
    if (!is_active()) {
        sns_logd("start gyrocal");
        start_gyrocal();

        if(_gyro_temp_available) {
            _gyrotemp = make_unique<additionalinfo_sensor>(_gyrotemp_suid,
                              _gyro_temp_wakeup_type,get_sensor_info().handle,
                              _process_in_qmicallback, _atrace_delay_checktime_ms);
            _gyrotemp->register_EventQueue(_mEventQueue, _mEventQueueFlag);
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

        /* disable gyrotemp */
        if(_gyro_temp_available && _gyrotemp) {
            _gyrotemp->deactivate();
            _gyrotemp.reset();
        }

        /* close calibration connection */
        sns_logd("stop gyrocal");
        /* set the gyrocal_conn variable to null while protected by the
           mutex, this will make sure that any connection callbacks
           accessing this variable will not use it to send new requests */
        ssc_connection* temp;
        worker* temp_worker;
        sensors_diag_log* temp_diag;
        _cal_mutex.lock();
        temp = _gyrocal_conn;
        temp_worker = _gyrocal_worker;
        temp_diag = _diag_gyrocal;
        _gyrocal_conn = nullptr;
        _gyrocal_worker = nullptr;
        _diag_gyrocal = nullptr;
        _cal_mutex.unlock();
        delete temp;
        delete temp_worker;
        delete temp_diag;
    }
}

gyrocal_bias gyroscope::get_cal_bias(uint64_t timestamp)
{
  lock_guard<mutex> lock(_cal_mutex);
  if (_biases.size() == 0)
  {
    gyrocal_bias b;
    b.bias[0] = 0.0f;
    b.bias[1] = 0.0f;
    b.bias[2] = 0.0f;
    b.timestamp = 0;
    b.status = SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE;
    return b;
  }

  auto cur= _biases.begin();
  auto prev = cur;
  cur++;

  while (cur != _biases.end())
  {
    if (cur->timestamp <= timestamp)
    {
      _biases.erase(prev);
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
        string pthread_name = to_string(get_sensor_info().handle) + "_see";
        ret_code = pthread_setname_np(pthread_self(), pthread_name.c_str());
        if (ret_code != 0) {
            sns_loge("Failed to set ThreadName: %s\n", pthread_name.c_str());
        }
    }
    sns_std_sensor_sample_status status;
    sns_client_event_msg pb_event_msg;
    pb_event_msg.ParseFromArray(data, size);

    string pb_event_msg_encoded((char *)data, size);

    lock_guard<mutex> lock(_cal_mutex);
    if(nullptr != _diag_gyrocal)
      _diag_gyrocal->log_event_msg(pb_event_msg_encoded, SSC_DATATYPE_GYROCAL, DIAG_LOG_MODULE_NAME);

    status = _sample_status;

    for (int i=0; i < pb_event_msg.events_size(); i++) {
        auto&& pb_event = pb_event_msg.events(i);
        sns_logv("event[%d] msg_id=%d", i, pb_event.msg_id());

        if (pb_event.msg_id() == SNS_CAL_MSGID_SNS_CAL_EVENT) {
            sns_cal_event pb_cal_event;
            pb_cal_event.ParseFromString(pb_event.payload());
            _sample_status = pb_cal_event.status();

            gyrocal_bias b;
            b.bias[0] = pb_cal_event.bias(0);
            b.bias[1] = pb_cal_event.bias(1);
            b.bias[2] = pb_cal_event.bias(2);
            b.timestamp = pb_event.timestamp();
            b.status = _sample_status;
            _biases.push_back(b);

            sns_logd("gyrocal bias=(%f, %f, %f) status=%d",
                     b.bias[0], b.bias[1], b.bias[2], (int)b.status);
        }
    }

    if((SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE == status &&
        SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE != _sample_status) ||
       (SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE != status &&
        SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE == _sample_status)) {
        if(_gyrocal_worker != nullptr) {
          _gyrocal_worker->add_task(NULL, [this] {
              std::lock_guard<mutex> lock(_cal_mutex);
              send_cal_req();
          });
        } else {
            send_cal_req();
        }
    }
}

void gyroscope::handle_sns_client_event(
        const sns_client_event_msg_sns_client_event& pb_event)
{
    if (SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_EVENT == pb_event.msg_id()) {
        sns_std_sensor_event pb_sensor_event;
        pb_sensor_event.ParseFromString(pb_event.payload());
        if(pb_sensor_event.data_size()) {
            Event hal_event = create_sensor_hal_event(pb_event.timestamp());
            /* select calibration bias based on timestamp */
            gyrocal_bias b = get_cal_bias(pb_event.timestamp());
            if (_cal == SENSOR_CALIBRATED) {
                hal_event.u.vec3.x = pb_sensor_event.data(0) - b.bias[0];
                hal_event.u.vec3.y = pb_sensor_event.data(1) - b.bias[1];
                hal_event.u.vec3.z = pb_sensor_event.data(2) - b.bias[2];
                hal_event.u.vec3.status = STD_MIN(ssc_sensor::sensors_hal_sample_status(b.status),
                    ssc_sensor::sensors_hal_sample_status(pb_sensor_event.status()));
                sns_logd("gyro_sample_cal: ts=%lld ns; value = [%f, %f, %f], status = %d",
                         (long long) hal_event.timestamp,
                         hal_event.u.vec3.x, hal_event.u.vec3.y, hal_event.u.vec3.z,
                         (int)hal_event.u.vec3.status);
            } else {
                hal_event.u.uncal.x = pb_sensor_event.data(0);
                hal_event.u.uncal.y = pb_sensor_event.data(1);
                hal_event.u.uncal.z = pb_sensor_event.data(2);
                hal_event.u.uncal.x_bias = b.bias[0];
                hal_event.u.uncal.y_bias = b.bias[1];
                hal_event.u.uncal.z_bias = b.bias[2];
                sns_logd("gyro_sample_uncal: ts=%lld ns; value = [%f, %f, %f],"
                         " bias = [%f, %f, %f]", (long long) hal_event.timestamp,
                         hal_event.u.uncal.x,
                         hal_event.u.uncal.y,
                         hal_event.u.uncal.z,
                         hal_event.u.uncal.x_bias,
                         hal_event.u.uncal.y_bias,
                         hal_event.u.uncal.z_bias);
            }
            cal_sample_inerval(pb_event.timestamp());
            calculate_latency(hal_event.timestamp);
            cal_slpi_latency(pb_event.timestamp());

            if(true == can_submit_sample(hal_event))
              events.push_back(hal_event);
        } else {
            sns_loge("empty data returned for gyro");
        }

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
        bool default_sensor = sensor_factory::instance().is_default_sensor(SSC_DATATYPE_GYRO, suid);
        if (!(sensor_factory::instance().get_settings()
                                & DISABLE_WAKEUP_SENSORS_FLAG)) {
            try {
                sensors.push_back(make_unique<gyroscope>(suid, SENSOR_WAKEUP,
                                                         SENSOR_CALIBRATED, default_sensor));
            } catch (const exception& e) {
                sns_loge("failed for wakeup, %s", e.what());
            }
        }
        try {
            sensors.push_back(make_unique<gyroscope>(suid, SENSOR_NO_WAKEUP,
                                                     SENSOR_CALIBRATED, default_sensor));
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
        bool default_sensor = sensor_factory::instance().is_default_sensor(SSC_DATATYPE_GYRO, suid);
        if (!(sensor_factory::instance().get_settings()
                                  & DISABLE_WAKEUP_SENSORS_FLAG)) {
            try {
                sensors.push_back(make_unique<gyroscope>(suid, SENSOR_WAKEUP,
                                                         SENSOR_UNCALIBRATED, default_sensor));
            } catch (const exception& e) {
                sns_loge("failed for wakeup,  %s", e.what());
            }
        }
        try {
            sensors.push_back(make_unique<gyroscope>(suid, SENSOR_NO_WAKEUP,
                                                     SENSOR_UNCALIBRATED, default_sensor));
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
    sensor_factory::request_datatype(SSC_DATATYPE_GYRO, false);
    sensor_factory::request_datatype(SSC_DATATYPE_GYROCAL, false);
    sensor_factory::request_datatype(SSC_DATATYPE_TEMP, false);
    return true;
}

SENSOR_MODULE_INIT(gyroscope_module_init);
