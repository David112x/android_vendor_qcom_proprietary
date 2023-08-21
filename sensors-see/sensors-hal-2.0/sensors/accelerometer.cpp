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
#include "worker.h"
#include "sensors_qti.h"

static const char *SSC_DATATYPE_ACCEL = "accel";
static const char *SSC_DATATYPE_ACCELCAL = "accel_cal";
static const uint32_t ACCEL_RESERVED_FIFO_COUNT = 3000;
static const float ONE_G = 9.80665f;
static char DIAG_LOG_MODULE_NAME[] = "sensors-hal";
using namespace std;

struct accelcal_bias
{
  float bias[3];
  uint64_t timestamp;
  sns_std_sensor_sample_status status;
};

class accelerometer : public ssc_sensor
{
public:
    accelerometer(sensor_uid suid, sensor_wakeup_type wakeup,
                                        sensor_cal_type calibrated, bool default_sensor);
    static const char* ssc_datatype() { return SSC_DATATYPE_ACCEL; }
    virtual bool is_calibrated() override;

private:

    virtual void activate() override;
    virtual void deactivate() override;

    void start_accelcal();

    bool _accelcal_available;
    ssc_connection* _accelcal_conn;
    sensor_uid _accelcal_suid;
    std::mutex _cal_mutex;
    std::list<accelcal_bias> _biases;
    /* status based on calibration accuracy */
    sns_std_sensor_sample_status _sample_status;

    /*used to restart sensors after SSR*/
    static const uint8_t RETRY_CNT = 60;

    bool accel_set_thread_name = false;

    void send_cal_req();
    sensors_diag_log *_diag_accelcal;
    void accelcal_conn_event_cb(const uint8_t *data, size_t size, uint64_t ts);
    void accelcal_conn_resp_cb(uint32_t resp_value);
    /* returns the latest accelcal bias that is older than the timestamp */
    accelcal_bias get_cal_bias(uint64_t timestamp);
    /*create a seperate worker*/
    worker *_accelcal_worker;
    bool _process_in_qmicallback;
    sensor_cal_type _cal_type;
    /*handle ssr and re initiate the request*/
    virtual void ssc_conn_error_cb(ssc_error_type e) override;
    virtual void handle_sns_client_event(
        const sns_client_event_msg_sns_client_event& pb_event);
};

accelerometer::accelerometer(sensor_uid suid,
                                sensor_wakeup_type wakeup,
                                sensor_cal_type cal_type,
                                bool default_sensor):
    ssc_sensor(suid, wakeup),
    _accelcal_available(false),
    _accelcal_conn(nullptr),
    _sample_status(SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE),
    _diag_accelcal(nullptr),
    _accelcal_worker(nullptr),
    _process_in_qmicallback(false),
    _cal_type(cal_type)
{
    if (default_sensor) {
        if (cal_type == SENSOR_UNCALIBRATED) {
            set_type(SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED);
            set_string_type(SENSOR_STRING_TYPE_ACCELEROMETER_UNCALIBRATED);
            set_sensor_typename("Accelerometer-Uncalibrated");
        } else {
            set_type(SENSOR_TYPE_ACCELEROMETER);
            set_string_type(SENSOR_STRING_TYPE_ACCELEROMETER);
            set_sensor_typename("Accelerometer");
        }
    } else {
        if (cal_type == SENSOR_UNCALIBRATED) {
            set_type(QTI_SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED);
            set_string_type(QTI_SENSOR_STRING_TYPE_ACCELEROMETER_UNCALIBRATED);
            set_sensor_typename("QTI-Accelerometer-Uncalibrated");
        } else {
            set_type(QTI_SENSOR_TYPE_ACCELEROMETER);
            set_string_type(QTI_SENSOR_STRING_TYPE_ACCELEROMETER);
            set_sensor_typename("QTI-Accelerometer");
        }
    }
#ifdef SNS_DIRECT_REPORT_SUPPORT
    set_direct_channel_flag(true);
#endif

    if (sensor_factory::instance().get_pairedcalsuid(
                                     SSC_DATATYPE_ACCELCAL,
                                     suid,
                                     _accelcal_suid) == 0) {
        _accelcal_available = true;
    } else {
        sns_logi("accelcal is not available");
    }

    set_fifo_reserved_count(ACCEL_RESERVED_FIFO_COUNT);
    set_resampling(true);
    set_nowk_msgid(SNS_RESAMPLER_MSGID_SNS_RESAMPLER_CONFIG_EVENT);

    /* convert range from Gs to m/s^2 */
    set_max_range(get_sensor_info().maxRange * ONE_G);
    /* convert resolution from mG to m/s^2 */
    set_resolution(get_sensor_info().resolution * ONE_G / 1000.0);
    set_nowk_msgid(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT);
}

void accelerometer::send_cal_req()
{
    string pb_req_msg_encoded;
    sns_client_request_msg pb_req_msg;

    if (_accelcal_conn == nullptr) {
        sns_logd("accelcal_conn is closed, not sending request");
        return;
    }

    sns_logd("SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG");
    pb_req_msg.set_msg_id(SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG);
    pb_req_msg.mutable_request()->clear_payload();
    pb_req_msg.mutable_suid()->set_suid_high(_accelcal_suid.high);
    pb_req_msg.mutable_suid()->set_suid_low(_accelcal_suid.low);

    /* If current calibration status is "UNRELIABLE", set wakeup flag to
     * ensure next valid calibration parameters are sent immediately. */
    pb_req_msg.mutable_request()->mutable_batching()->
        set_batch_period(
            SENSOR_STATUS_UNRELIABLE != _sample_status
            ? UINT32_MAX : 0);
    pb_req_msg.mutable_request()->mutable_batching()->
        set_flush_period(UINT32_MAX);
    pb_req_msg.mutable_susp_config()->set_delivery_type(
        SENSOR_STATUS_UNRELIABLE != _sample_status
        ? SNS_CLIENT_DELIVERY_NO_WAKEUP : SNS_CLIENT_DELIVERY_WAKEUP);
    pb_req_msg.mutable_susp_config()->
        set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

    pb_req_msg.SerializeToString(&pb_req_msg_encoded);

    if(nullptr != _diag_accelcal)
      _diag_accelcal->log_request_msg(pb_req_msg_encoded, SSC_DATATYPE_ACCELCAL, DIAG_LOG_MODULE_NAME);

    if ( nullptr == _accelcal_conn ) {
      sns_logd("accelcal_conn is closed, not sending request");
      return;
    }else{
      _accelcal_conn->send_request(pb_req_msg_encoded, false);
    }
}

void accelerometer::ssc_conn_error_cb(ssc_error_type e)
{
    sns_loge("handle error = %d for accelerometer", e);
    /* re-send accelerometer cal request when connection resets */
    if (_accelcal_available && e == SSC_CONNECTION_RESET) {
        bool got_sensorback = false;
        uint8_t retry_cnt = RETRY_CNT;
        sns_loge("connection reset, resend accelerometer cal request");
        //check for suid and then send config request
        while (retry_cnt && is_active()) {
            using namespace std::chrono;
            suid_lookup lookup(
                [&got_sensorback, this](const string& datatype, const auto& suids)
                {
                    sns_logd("got the suid call back");
                    int i;
                    for (i = 0; i < (int)suids.size(); i++) {
                      if ((suids[i].high == _accelcal_suid.high) &&
                          (suids[i].low == _accelcal_suid.low))
                          got_sensorback = true;
                    }
                });
            if (got_sensorback == true) {
                sns_logi("after ssr, discovery of %s took %f sec",
                    SSC_DATATYPE_ACCELCAL,
                    (float)((RETRY_CNT-retry_cnt)*((float)WAIT_TIME_MS/MSEC_PER_SEC)));
                break;
            }
            lookup.request_suid(SSC_DATATYPE_ACCELCAL, false);
            retry_cnt--;
            sns_logd("requesting suid (cnt) %d", retry_cnt);
            this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_MS));
        }
        if (is_active()) {
            if (got_sensorback)
                send_cal_req();
            else
                sns_loge("could not restart %s after ssr", SSC_DATATYPE_ACCELCAL);
        } else {
            sns_logi("%s deactivated during ssr", SSC_DATATYPE_ACCELCAL);
        }
    }
    /* do common error handling */
    ssc_sensor::ssc_conn_error_cb(e);
}
void accelerometer::accelcal_conn_resp_cb(uint32_t resp_value)
{
  std::lock_guard<mutex> lock(_cal_mutex);
  if(nullptr != _diag_accelcal)
    _diag_accelcal->log_response_msg(resp_value,SSC_DATATYPE_ACCELCAL, DIAG_LOG_MODULE_NAME);
}
void accelerometer::start_accelcal()
{
    /* create a new ssc connection for accelcal */
    std::lock_guard<mutex> lock(_cal_mutex);
    _accelcal_conn = new ssc_connection(
        [this](const uint8_t *data, size_t size, uint64_t ts)
        {
            accelcal_conn_event_cb(data, size, ts);
        });
    if(nullptr != _accelcal_conn) {
        accel_set_thread_name = false;
        string thread_name = "see_"+to_string(get_sensor_info().handle);
        _accelcal_conn->set_worker_name(thread_name.c_str());
        _accelcal_conn->ssc_configuration(_is_rt_thread_enable, _atrace_delay_checktime_ms);
        if(true == _is_worker_bypass){
           _accelcal_conn->ssc_configuration(_is_worker_bypass);
        }
        _accelcal_conn->register_resp_cb(
            [this](uint32_t resp_value)
            {
                accelcal_conn_resp_cb(resp_value);
            });
        _diag_accelcal =  new sensors_diag_log();
    }
    if (true == _process_in_qmicallback) {
      _accelcal_worker =new worker;
      if (_accelcal_worker) _accelcal_worker->setname("accelcal");
    }
    send_cal_req();
}

void accelerometer::activate()
{
    _process_in_qmicallback = _is_rt_thread_enable;
    if(_is_worker_bypass){
    _process_in_qmicallback = _is_worker_bypass;
    }
    if (!is_active()) {
        if (_accelcal_available) {
            sns_logd("start accelcal");
            start_accelcal();
        }
        ssc_sensor::activate();
    }
}

void accelerometer::deactivate()
{
    if (is_active()) {
        ssc_sensor::deactivate();
        if (_accelcal_available) {
            /* close calibration connection */
            sns_logd("stop accelcal");
            /* set the accelcal_conn variable to null while protected by the
               mutex, this will make sure that any connection callbacks
               accessing this variable will not use it to send new requests */
            ssc_connection* temp;
            worker* temp_worker;
            sensors_diag_log* temp_diag;
            _cal_mutex.lock();
            temp = _accelcal_conn;
            temp_worker = _accelcal_worker;
            temp_diag = _diag_accelcal;
            _accelcal_conn = nullptr;
            _accelcal_worker = nullptr;
            _diag_accelcal = nullptr;
            _cal_mutex.unlock();
            delete temp;
            delete temp_worker;
            delete temp_diag;

        }
    }
}

accelcal_bias accelerometer::get_cal_bias(uint64_t timestamp)
{
    lock_guard<mutex> lock(_cal_mutex);
    if (_biases.size() == 0)
    {
        accelcal_bias b;
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

void accelerometer::accelcal_conn_event_cb(const uint8_t *data, size_t size, uint64_t ts)
{
    if (accel_set_thread_name == true) {
        accel_set_thread_name = false;
        int ret_code = 0;
        string pthread_name = to_string(get_sensor_info().handle)+"_see" ;
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
    if(nullptr != _diag_accelcal)
      _diag_accelcal->log_event_msg(pb_event_msg_encoded, SSC_DATATYPE_ACCELCAL, DIAG_LOG_MODULE_NAME);

    status = _sample_status;

    for (int i=0; i < pb_event_msg.events_size(); i++) {
        auto&& pb_event = pb_event_msg.events(i);
        sns_logv("event[%d] msg_id=%d", i, pb_event.msg_id());

        if (pb_event.msg_id() == SNS_CAL_MSGID_SNS_CAL_EVENT) {
            sns_cal_event pb_cal_event;
            pb_cal_event.ParseFromString(pb_event.payload());
            _sample_status = pb_cal_event.status();

            accelcal_bias b;
            b.bias[0] = pb_cal_event.bias(0);
            b.bias[1] = pb_cal_event.bias(1);
            b.bias[2] = pb_cal_event.bias(2);
            b.timestamp = pb_event.timestamp();
            b.status = _sample_status;
            _biases.push_back(b);

            sns_logd("accelcal bias=(%f, %f, %f) status=%d",
                     b.bias[0], b.bias[1], b.bias[2], (int)b.status);
        }
    }

    if((SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE == status &&
        SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE != _sample_status) ||
       (SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE != status &&
        SNS_STD_SENSOR_SAMPLE_STATUS_UNRELIABLE == _sample_status)) {
      if(_accelcal_worker != nullptr){
        _accelcal_worker->add_task(NULL, [this] {
            std::lock_guard<mutex> lock(_cal_mutex);
            send_cal_req();
        });
      } else {
          send_cal_req();
      }
    }
}

void accelerometer::handle_sns_client_event(
        const sns_client_event_msg_sns_client_event& pb_event)
{
    if (SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_EVENT == pb_event.msg_id()) {
        sns_std_sensor_event pb_sensor_event;
        pb_sensor_event.ParseFromString(pb_event.payload());

        if(pb_sensor_event.data_size()) {
            Event hal_event = create_sensor_hal_event(pb_event.timestamp());
            /* select calibration bias based on timestamp */
            accelcal_bias b = get_cal_bias(pb_event.timestamp());

            if (_cal_type == SENSOR_CALIBRATED) {
                hal_event.u.vec3.x = pb_sensor_event.data(0) - b.bias[0];
                hal_event.u.vec3.y = pb_sensor_event.data(1) - b.bias[1];
                hal_event.u.vec3.z = pb_sensor_event.data(2) - b.bias[2];
                if(_accelcal_available){
                    hal_event.u.vec3.status = STD_MIN(ssc_sensor::sensors_hal_sample_status(b.status),
                        ssc_sensor::sensors_hal_sample_status(pb_sensor_event.status()));
                } else {
                    hal_event.u.vec3.status = ssc_sensor::sensors_hal_sample_status(pb_sensor_event.status());
                }
                sns_logd("accel_sample(accelcal_available:%d): ts=%lld ns; value = [%f, %f, %f], status=%d",
                         (int)_accelcal_available, (long long) hal_event.timestamp, hal_event.u.vec3.x,
                         hal_event.u.vec3.y,hal_event.u.vec3.z,
                         (int)hal_event.u.vec3.status);
            }
            if (_cal_type == SENSOR_UNCALIBRATED) {
                hal_event.u.uncal.x = pb_sensor_event.data(0);
                hal_event.u.uncal.y = pb_sensor_event.data(1);
                hal_event.u.uncal.z = pb_sensor_event.data(2);
                hal_event.u.uncal.x_bias = b.bias[0];
                hal_event.u.uncal.y_bias = b.bias[1];
                hal_event.u.uncal.z_bias = b.bias[2];
                sns_logd("accel_sample_uncal: ts=%lld ns; value = [%f, %f, %f],"
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
            sns_loge("empty data returned for accel");
        }
    }
}

bool accelerometer::is_calibrated()
{
  return (_cal_type == SENSOR_CALIBRATED) ? true : false;
}

/* create sensor variants supported by this class and register the function
   to sensor_factory */
static vector<unique_ptr<sensor>> get_available_accel_calibrated()
{
    const vector<sensor_uid>& accel_suids =
         sensor_factory::instance().get_suids(SSC_DATATYPE_ACCEL);
    vector<unique_ptr<sensor>> sensors;
    for (const auto& suid : accel_suids) {
        bool default_sensor = sensor_factory::instance().is_default_sensor(SSC_DATATYPE_ACCEL, suid);
        if (!(sensor_factory::instance().get_settings()
                                    & DISABLE_WAKEUP_SENSORS_FLAG)) {
            try {
                sensors.push_back(make_unique<accelerometer>(suid, SENSOR_WAKEUP,
                                                         SENSOR_CALIBRATED, default_sensor));
            } catch (const exception& e) {
                sns_loge("failed for wakeup, %s", e.what());
            }
        }
        try {
            sensors.push_back(make_unique<accelerometer>(suid, SENSOR_NO_WAKEUP,
                                                     SENSOR_CALIBRATED, default_sensor));
        } catch (const exception& e) {
            sns_loge("failed for nowakeup, %s", e.what());
        }
    }
    return sensors;
}

static vector<unique_ptr<sensor>> get_available_accel_uncalibrated()
{
    const vector<sensor_uid>& accel_suids =
        sensor_factory::instance().get_suids(SSC_DATATYPE_ACCEL);
    vector<unique_ptr<sensor>> sensors;
    for (const auto& suid : accel_suids) {
        bool default_sensor = sensor_factory::instance().is_default_sensor(SSC_DATATYPE_ACCEL, suid);
        if (!(sensor_factory::instance().get_settings()
                                    & DISABLE_WAKEUP_SENSORS_FLAG)) {
            try {
                sensors.push_back(make_unique<accelerometer>(suid, SENSOR_WAKEUP,
                                                         SENSOR_UNCALIBRATED, default_sensor));
            } catch (const exception& e) {
                sns_loge("failed for wakeup,  %s", e.what());
            }
        }
        try {
            sensors.push_back(make_unique<accelerometer>(suid, SENSOR_NO_WAKEUP,
                                                     SENSOR_UNCALIBRATED, default_sensor));
        } catch (const exception& e) {
            sns_loge("failed for nowakeup, %s", e.what());
        }
    }
    return sensors;
}


static bool accelerometer_module_init()
{
    /* register supported sensor types with factory */
    sensor_factory::register_sensor(SENSOR_TYPE_ACCELEROMETER,
                                    get_available_accel_calibrated);
    sensor_factory::register_sensor(SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED,
                                    get_available_accel_uncalibrated);
    sensor_factory::request_datatype(SSC_DATATYPE_ACCEL, false);

    char debug_prop[PROPERTY_VALUE_MAX];
    int enable_accel_cal = 0;
    int len;
    len = property_get("persist.vendor.sensors.accel_cal", debug_prop, "0");
    if (len > 0) {
        enable_accel_cal = atoi(debug_prop);
    }
    if (enable_accel_cal)
    {
      sensor_factory::request_datatype(SSC_DATATYPE_ACCELCAL, false);
    }
    return true;
}

SENSOR_MODULE_INIT(accelerometer_module_init);
