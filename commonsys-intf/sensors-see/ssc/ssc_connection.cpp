/*
 * Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#include <string>
#include <stdexcept>
#include <cinttypes>
#include <mutex>
#include <stdint.h>
#include <sched.h>
#include "ssc_connection.h"
#include "sensors_log.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "qmi_client.h"
#include "sns_client_api_v01.h"
#include "worker.h"
#include <map>
#include "ssc_utils.h"
#include "utils/SystemClock.h"
#ifdef SNS_LE_QCS605
#include <condition_variable>
#endif
#define ATRACE_TAG ATRACE_TAG_ALWAYS
#include <utils/Trace.h>

using namespace std;
using namespace google::protobuf::io;
#define MAX_SVC_INFO_ARRAY_SIZE 5

static const uint64_t NSEC_PER_SEC = 1000000000ull;

/* number of times to wait for sensors service */
static const int SENSORS_SERVICE_DISCOVERY_TRIES = 4;
static const int SENSORS_SERVICE_DISCOVERY_TRIES_POST_SENSORLIST = 2;

/* timeout for each try */
static auto SENSORS_SERVICE_DISCOVERY_TIMEOUT = 2s;
static auto SENSORS_SERVICE_DISCOVERY_TIMEOUT_POST_SENSORLIST = 500ms;
static auto MAX_QMI_CLIENT_SEND_MSG_ADDITIONAL_WAIT_TIME_IN_SEC = 10;
static uint32_t g_qmierr_cnt;
static uint32_t SNS_RT_PRIORITY = 10;

static int64_t g_last_ssrtrigger_time;


/* exception type defining errors related to qmi API calls */
struct qmi_error : public runtime_error
{
    qmi_error(int error_code, const std::string& what = "") :
        runtime_error(what + ": " + error_code_to_string(error_code)) { }

    static string error_code_to_string(int code)
    {
        static const map<int, string> error_map = {
            { QMI_NO_ERR, "qmi no error" },
            { QMI_INTERNAL_ERR, "qmi internal error" },
            { QMI_TIMEOUT_ERR, "qmi timeout" },
            { QMI_XPORT_BUSY_ERR, "qmi transport busy" },
            { QMI_SERVICE_ERR, "qmi service error" },
        };
        string msg;
        try {
            msg = error_map.at(code);
        } catch (out_of_range& e) {
            msg = "qmi error";
        }
        return msg + " (" + to_string(code) + ")";
    }
};

/* implementation of ssc_connection using qmi */
class ssc_qmi_connection
{
public:
    /* establish new qmi connection to ssc */
    ssc_qmi_connection(ssc_event_cb event_cb);

    ssc_qmi_connection(ssc_event_cb_ts event_cb_ts);

    /* disconnect from ssc */
    ~ssc_qmi_connection();

    /* send encoded request message to ssc */
    void send_request(const string& pb_req_message_encoded, bool use_qmi_sync);

    void register_error_cb(ssc_error_cb cb) { _error_cb = cb; }

    void register_resp_cb(ssc_resp_cb cb) { _resp_cb = cb; }

    void set_unsuspendable_channel(){ _worker.make_unsuspendable("sensor_ssc_worker");}

    void ssc_qmi_configuration(bool is_rt_enable , uint32_t atrace_delay);

    void ssc_qmi_configuration(bool worker_bypass);

    void set_worker_name(const char *worker_name);
private:
    /*used for QMI WD */
    struct timespec _start_monotonic_ts;
    struct timespec _start_bootclock_ts;
    ssc_event_cb _event_cb;
    ssc_event_cb_ts _event_cb_ts;
    bool _is_event_cb_ts;
    qmi_client_type _qmi_handle = nullptr;
    bool _service_ready;
    static bool _service_accessed;
    std::mutex _mutex;
    std::condition_variable _cv;
    qmi_cci_os_signal_type _os_params;
    /* flag to see if connection is being reset */
    atomic<bool> _reconnecting;

    /*flag to check disconnection*/
    bool _connection_closed;

    ssc_error_cb _error_cb;

    ssc_resp_cb _resp_cb;

    //static const uint32_t QMI_DISCOVERY_TIMEOUT_MS = 15000;
    static const uint32_t QMI_RESPONSE_TIMEOUT_MS = 2000;

    void qmi_connect();
    void qmi_disconnect();
    void qmi_send_request(const sns_client_req_msg_v01& req_msg);

    void qmi_wait_for_service();

    static void qmi_notify_cb(qmi_client_type user_handle,
                              qmi_idl_service_object_type service_obj,
                              qmi_client_notify_event_type service_event,
                              void *notify_cb_data);

    static void qmi_indication_cb(qmi_client_type user_handle,
                                  unsigned int msg_id,
                                  void* ind_buf,
                                  unsigned int ind_buf_len,
                                  void* ind_cb_data);

    static void qmi_response_cb(qmi_client_type user_handle,
                                  unsigned int msg_id,
                                  void* resp_cb,
                                  unsigned int resp_cb_len,
                                  void* resp_cb_data,
                                  qmi_client_error_type qmi_error);

    static void qmi_error_cb(qmi_client_type user_handle,
                             qmi_client_error_type error,
                             void* err_cb_data);

   /**
    * Handle a sns_client_report_ind_msg_v01 or sns_client_jumbod_report_ind_msg_v01
    * indication message received from SEE's Client Manager.
    *
    * @param[i] msg_id One of SNS_CLIENT_REPORT_IND_V01 or SNS_CLIENT_JUMBO_REPORT_IND_V01
    * @param[i] ind_buf The encoded QMI message buffer
    * @param[i] ind_buf_len Length of ind_buf
    */
    void handle_indication(unsigned int msg_id,
                            void *ind_buf,
                            unsigned int ind_buf_len,
                            uint64_t ts);

   /**
    * Handle a sns_client_report_ind_msg_v01 or sns_client_jumbod_report_ind_msg_v01
    * indication message received from SEE's Client Manager.
    *
    * @param[i] msg_id One of SNS_CLIENT_REPORT_IND_V01 or SNS_CLIENT_JUMBO_REPORT_IND_V01
    * @param[i] ind_buf The encoded QMI message buffer
    * @param[i] ind_buf_len Length of ind_buf
    */
   void handle_indication_realtime(unsigned int msg_id,
                           void *ind_buf,
                           unsigned int ind_buf_len,
                           uint64_t ts);

   /*Timer callback functions and variables*/
   static void ssc_timer_notify_cb(union sigval arg);
   void create_ssc_timer();
   void delete_ssc_timer();
   bool start_ssc_timer();
   bool _qmi_debug_flag;
   bool _is_qmi_call_blocked;
   void ssc_debug_timer_start();
   void ssc_debug_timer_stop();
   pthread_mutex_t _qmi_debug_mutex;
   timer_t _ssc_timer_id;
   bool _is_timer_created;

    static const uint32_t QMI_ERRORS_HANDLED_ATTEMPTS = 10;

    static const uint64_t NSEC_PER_MSEC = 1000000ull;

    static const uint64_t NSEC_PER_USEC = 1000ull;

    /*below is to check transport jitter*/
    uint64_t _prev_sample_recevied_ts = 0;
    /*below is to use rt priority for sensor threads*/
    bool _set_rt_prio;
    bool _use_rt_cbk = false;
    bool _bypass_worker = false;
    /* below to take the input from getprop for tracing*/
    uint32_t _atrace_delay_checktime_ms = 0;
    worker _worker;
    sns_client_resp_msg_v01 resp_async = {};
};

bool ssc_qmi_connection::_service_accessed = false;

ssc_qmi_connection::ssc_qmi_connection(ssc_event_cb event_cb) :
    _event_cb(event_cb),
    _reconnecting(false),
    _connection_closed(false)
{
    _set_rt_prio = false;
    _use_rt_cbk = _set_rt_prio;
    _atrace_delay_checktime_ms = 0;
    _is_event_cb_ts = false;
    _qmi_debug_flag = get_qmi_debug_flag();
    _is_qmi_call_blocked = false;
    _qmi_debug_mutex = PTHREAD_MUTEX_INITIALIZER;
    _is_timer_created = false;
    qmi_connect();
    create_ssc_timer();
    sns_logd("_service_accessed %d", (int)_service_accessed);
    _worker.setname("ssc_qmi_connection");
}

ssc_qmi_connection::ssc_qmi_connection(ssc_event_cb_ts event_cb) :
    _event_cb_ts(event_cb),
    _reconnecting(false),
    _connection_closed(false)
{
    _set_rt_prio = false;
    _use_rt_cbk = _set_rt_prio;
    _atrace_delay_checktime_ms = 0;
    _is_event_cb_ts = true;
    _qmi_debug_flag = get_qmi_debug_flag();
    _is_qmi_call_blocked = false;
    _qmi_debug_mutex = PTHREAD_MUTEX_INITIALIZER;
    _is_timer_created = false;
    qmi_connect();
    create_ssc_timer();
    sns_logd("_service_accessed %d", (int)_service_accessed);
}
void ssc_qmi_connection::set_worker_name(const char *worker_name)
{
  _worker.setname(worker_name);
}

void ssc_qmi_connection::ssc_qmi_configuration(bool is_rt_enable , uint32_t atrace_delay)
{
  _set_rt_prio = is_rt_enable;
  _use_rt_cbk = _set_rt_prio;
  _atrace_delay_checktime_ms = atrace_delay;
}
void ssc_qmi_connection::ssc_qmi_configuration(bool bypass_worker)
{
    _bypass_worker = bypass_worker;
}
ssc_qmi_connection::~ssc_qmi_connection()
{
    _connection_closed = true;
    if(_is_timer_created)
      delete_ssc_timer();
    qmi_disconnect();
}

void ssc_qmi_connection::qmi_notify_cb(qmi_client_type user_handle,
                                       qmi_idl_service_object_type service_obj,
                                       qmi_client_notify_event_type service_event,
                                       void *notify_cb_data)
{
    ssc_qmi_connection *conn = (ssc_qmi_connection *) notify_cb_data;
    unique_lock<mutex> lk(conn->_mutex);
    conn->_service_ready = true;
    conn->_cv.notify_one();
}

void ssc_qmi_connection::qmi_wait_for_service()
{
    qmi_client_type notifier_handle;
    qmi_client_error_type qmi_err;
    qmi_cci_os_signal_type os_params;
    int num_tries = SENSORS_SERVICE_DISCOVERY_TRIES;

    qmi_err = qmi_client_notifier_init(SNS_CLIENT_SVC_get_service_object_v01(),
                                       &os_params, &notifier_handle);
    if (QMI_NO_ERR != qmi_err) {
        throw qmi_error(qmi_err, "qmi_client_notifier_init() failed");
    }

    /* register a callback and wait until service becomes available */
    _service_ready = false;
    qmi_err = qmi_client_register_notify_cb(notifier_handle, qmi_notify_cb,
                                            this);
    if (qmi_err != QMI_NO_ERR) {
        ssc_debug_timer_start();
        uint64_t start_ts_nsec = android::elapsedRealtimeNano();
        qmi_client_release(notifier_handle);
        uint64_t stop_ts_nsec = android::elapsedRealtimeNano();
        if((stop_ts_nsec - start_ts_nsec) > 2*NSEC_PER_SEC) {
          sns_logi("Time taken to complete qmi_client_release is %lf secs",
              (double)((stop_ts_nsec - start_ts_nsec)/(NSEC_PER_SEC)));
        }
        ssc_debug_timer_stop();
        throw qmi_error(qmi_err, "qmi_client_register_notify_cb() failed: %d");
    }

    if (_service_accessed) {
      num_tries = SENSORS_SERVICE_DISCOVERY_TRIES_POST_SENSORLIST;
    }

    std::unique_lock<std::mutex> lk(_mutex);
    bool timeout = false;
    while (num_tries > 0 && (_service_ready != true)) {
        num_tries--;
        if (_service_accessed)
            timeout = !_cv.wait_for(lk, std::chrono::milliseconds(SENSORS_SERVICE_DISCOVERY_TIMEOUT_POST_SENSORLIST),
                                    [this]{ return _service_ready; });
        else
            timeout = !_cv.wait_for(lk, std::chrono::seconds(SENSORS_SERVICE_DISCOVERY_TIMEOUT),
                                    [this]{ return _service_ready; });

        if (timeout) {
            if (num_tries == 0) {
                lk.unlock();
                ssc_debug_timer_start();
                uint64_t start_ts_nsec = android::elapsedRealtimeNano();
                qmi_client_release(notifier_handle);
                uint64_t stop_ts_nsec = android::elapsedRealtimeNano();
                if((stop_ts_nsec - start_ts_nsec) > 2*NSEC_PER_SEC) {
                  sns_logi("Time taken to complete qmi_client_release is %lf secs",
                      (double)((stop_ts_nsec - start_ts_nsec)/(NSEC_PER_SEC)));
                }
                ssc_debug_timer_stop();
                throw runtime_error(
                    "FATAL: could not find sensors QMI service");
            }
            sns_loge("timeout while waiting for sensors QMI service: "
                     "will try %d more time(s)", num_tries);
        } else {
            _service_accessed = true;
        }
    }
    lk.unlock();
    ssc_debug_timer_start();
    uint64_t start_ts_nsec = android::elapsedRealtimeNano();
    qmi_client_release(notifier_handle);
    uint64_t stop_ts_nsec = android::elapsedRealtimeNano();
    if((stop_ts_nsec - start_ts_nsec) > 2*NSEC_PER_SEC) {
      sns_logi("Time taken to complete qmi_client_release is %lf secs",
          (double)((stop_ts_nsec - start_ts_nsec)/(NSEC_PER_SEC)));
    }
    ssc_debug_timer_stop();
}

void ssc_qmi_connection::qmi_connect()
{
    qmi_idl_service_object_type svc_obj =
        SNS_CLIENT_SVC_get_service_object_v01();

    qmi_client_error_type qmi_err;
    qmi_service_info svc_info_array[MAX_SVC_INFO_ARRAY_SIZE];
    uint32_t num_services, num_entries = MAX_SVC_INFO_ARRAY_SIZE;

    /*svc_info_array[5] - Initialized to 0 to avoid static analysis errors*/
    for(uint32_t i = 0 ; i < num_entries ; i++)
      memset(&svc_info_array[i], 0, sizeof(svc_info_array[i]));

    sns_logv("waiting for sensors qmi service");
    qmi_wait_for_service();
    sns_logv("connecting to qmi service");
    qmi_err = qmi_client_get_service_list(svc_obj, svc_info_array,
                                          &num_entries, &num_services);
    if (QMI_NO_ERR != qmi_err) {
        throw qmi_error(qmi_err, "qmi_client_get_service_list() failed");
    }

    if (num_entries == 0) {
        throw runtime_error("sensors service has no available instances");
    }

    if (_connection_closed) {
        sns_logi("connection got closed do not open qmi_channel");
        return ;
    }

    /* As only one qmi service is expected for sensors, use the 1st instance */
    qmi_service_info svc_info = svc_info_array[0];


    std::unique_lock<std::mutex> lk(_mutex);
    qmi_err = qmi_client_init(&svc_info, svc_obj, qmi_indication_cb,
                              (void*)this, &_os_params, &_qmi_handle);
    if (qmi_err != QMI_IDL_LIB_NO_ERR) {
        lk.unlock();
        throw qmi_error(qmi_err, "qmi_client_init() failed");
    }

    qmi_err = qmi_client_register_error_cb(_qmi_handle, qmi_error_cb, this);
    if (QMI_NO_ERR != qmi_err) {
        lk.unlock();
        ssc_debug_timer_start();
        uint64_t start_ts_nsec = android::elapsedRealtimeNano();
        qmi_client_release(_qmi_handle);
        uint64_t stop_ts_nsec = android::elapsedRealtimeNano();
        if((stop_ts_nsec - start_ts_nsec) > 2*NSEC_PER_SEC) {
          sns_logi("Time taken to complete qmi_client_release is %lf secs",
              (double)((stop_ts_nsec - start_ts_nsec)/(NSEC_PER_SEC)));
        }
        ssc_debug_timer_stop();
        throw qmi_error(qmi_err, "qmi_client_register_error_cb() failed");
    }
    lk.unlock();
    sns_logv("connected to ssc for %p", (void *)this);
}

void ssc_qmi_connection::qmi_disconnect()
{
    std::unique_lock<std::mutex> lk(_mutex);
    if (_qmi_handle != nullptr) {
        ssc_debug_timer_start();
        uint64_t start_ts_nsec = android::elapsedRealtimeNano();
        qmi_client_release(_qmi_handle);
        uint64_t stop_ts_nsec = android::elapsedRealtimeNano();
        if((stop_ts_nsec - start_ts_nsec) > 2*NSEC_PER_SEC) {
          sns_logi("Time taken to complete qmi_client_release is %lf secs",
              (double)((stop_ts_nsec - start_ts_nsec)/(NSEC_PER_SEC)));
        }
        ssc_debug_timer_stop();
        _qmi_handle = nullptr;
        /*in ssr call back and sensor disabled , so notify qmi_connect to comeout*/
        if (_connection_closed)
            _cv.notify_one();
    }
    /*explicit unlock not required , just added not to miss the logic*/
    lk.unlock();
    sns_logv("disconnected from ssc for %p", (void *)this);
}

void ssc_qmi_connection::handle_indication_realtime(unsigned int msg_id, void *ind_buf,
                                           unsigned int ind_buf_len, uint64_t ts)
{
  int32_t qmi_err;
  size_t ind_size;

  if(_set_rt_prio) {
    _set_rt_prio = 0;
    struct sched_param param = {0};
    param.sched_priority = SNS_RT_PRIORITY;
    if(sched_setscheduler(0, SCHED_FIFO, &param) != 0) {
      sns_loge("could not set SCHED_FIFO for qmi_cbk");
    } else {
      sns_logi(" SCHED_FIFO(%d) for qmi_cbk", SNS_RT_PRIORITY);
    }
   }

  size_t buffer_len;
  uint8_t *buffer = NULL;
  SNS_TRACE_BEGIN("sensors::handle_ind");

  if (_atrace_delay_checktime_ms) {
    uint64_t ts_diff = ts - _prev_sample_recevied_ts;
    if ( ts_diff > _atrace_delay_checktime_ms * NSEC_PER_MSEC) {
     ATRACE_INT64("qmi_ind_cbk jitter:", (int64_t)ts_diff);
     if(_prev_sample_recevied_ts)
       sns_logi("qmi_ind_cbk jitter: %llu(us)",
                    (unsigned long long) (ts_diff/NSEC_PER_USEC));
    }
    _prev_sample_recevied_ts = ts;
  }

  sns_logv("msg_id %d " , msg_id);
  if(SNS_CLIENT_REPORT_IND_V01 == msg_id){
    sns_client_report_ind_msg_v01 *ind = NULL;
    ind = (sns_client_report_ind_msg_v01 *)calloc(1, sizeof(sns_client_report_ind_msg_v01));
    if(ind == NULL){
      sns_loge("Error while creating memory for ind");
      SNS_TRACE_END();
      return;
    }
    ind_size = sizeof(sns_client_report_ind_msg_v01);
    qmi_err = qmi_idl_message_decode(SNS_CLIENT_SVC_get_service_object_v01(),
        QMI_IDL_INDICATION, msg_id, ind_buf,
        ind_buf_len, (void*)ind,
        ind_size);
    if (QMI_IDL_LIB_NO_ERR != qmi_err) {
      sns_loge("qmi_idl_message_decode() failed. qmi_err=%d SNS_CLIENT_REPORT_IND_V01", qmi_err);
      free(ind);
      ind = NULL;
      SNS_TRACE_END();
      return;
    }
    sns_logv("indication, payload_len=%u", ind->payload_len);

    buffer_len = ind->payload_len;
    buffer = (uint8_t*)calloc(1, buffer_len);
    if(buffer == NULL){
      sns_loge("buffer failed to creat ");
      free(ind);
      ind = NULL;
      SNS_TRACE_END();
      return;
    }
    memcpy(buffer, ind->payload, buffer_len);
    free(ind);

    if(false == _is_event_cb_ts) {
      this->_event_cb(buffer, buffer_len);
    } else {
      this->_event_cb_ts(buffer, buffer_len, ts);
    }
    free(buffer);
  }
  else if(SNS_CLIENT_JUMBO_REPORT_IND_V01 == msg_id){
    sns_client_jumbo_report_ind_msg_v01 *ind_jumbo = NULL;
    ind_jumbo = (sns_client_jumbo_report_ind_msg_v01 *)calloc(1, sizeof(sns_client_jumbo_report_ind_msg_v01));
    if(ind_jumbo == NULL){
      sns_loge("Error while creating memory for ind_jumbo");
      SNS_TRACE_END();
      return;
    }
    ind_size = sizeof(sns_client_jumbo_report_ind_msg_v01);
    qmi_err = qmi_idl_message_decode(SNS_CLIENT_SVC_get_service_object_v01(),
        QMI_IDL_INDICATION, msg_id, ind_buf,
        ind_buf_len, (void*)ind_jumbo,
        ind_size);
    if (QMI_IDL_LIB_NO_ERR != qmi_err) {
      sns_loge("qmi_idl_message_decode() failed. qmi_err=%d SNS_CLIENT_JUMBO_REPORT_IND_V01", qmi_err);
      free(ind_jumbo);
      ind_jumbo = NULL;
      SNS_TRACE_END();
      return;
    }
    sns_logv("indication, payload_len=%u", ind_jumbo->payload_len);
    buffer_len = ind_jumbo->payload_len;
    buffer = (uint8_t*)calloc(1, buffer_len);
    if(buffer == NULL){
      sns_loge("buffer failed to creat ind_jumbo ");
      free(ind_jumbo);
      ind_jumbo = NULL;
      SNS_TRACE_END();
      return;
    }
    memcpy(buffer, ind_jumbo->payload, buffer_len);
    free(ind_jumbo);
    if(false == _is_event_cb_ts) {
      this->_event_cb(buffer, buffer_len);
    } else {
      this->_event_cb_ts(buffer, buffer_len, ts);
    }
    free(buffer);
  }  else{
    sns_loge("Unknown indication message ID %i", msg_id);
    SNS_TRACE_END();
    return;
  }
  SNS_TRACE_END();
}

void ssc_qmi_connection::handle_indication(unsigned int msg_id, void *ind_buf,
                                           unsigned int ind_buf_len, uint64_t ts)
{
  int32_t qmi_err;
  uint32_t ind_size;
  size_t buffer_len;
  uint8_t *buffer = NULL;
  SNS_TRACE_BEGIN("sensors::handle_ind");

  if (_atrace_delay_checktime_ms) {
    uint64_t ts_diff = ts - _prev_sample_recevied_ts;
    if ( ts_diff > _atrace_delay_checktime_ms * NSEC_PER_MSEC) {
     ATRACE_INT64("qmi_ind_cbk jitter:", (int64_t)ts_diff);
     if(_prev_sample_recevied_ts)
       sns_logi("qmi_ind_cbk jitter: %llu(us)",
                    (unsigned long long) (ts_diff/NSEC_PER_USEC));
    }
    _prev_sample_recevied_ts = ts;
  }

  sns_logv("msg_id %d " , msg_id);
  if(SNS_CLIENT_REPORT_IND_V01 == msg_id){
    sns_client_report_ind_msg_v01 *ind = NULL;
    ind = (sns_client_report_ind_msg_v01 *)calloc(1, sizeof(sns_client_report_ind_msg_v01));
    if(ind == NULL){
      sns_loge("Error while creating memory for ind");
      SNS_TRACE_END();
      return;
    }
    ind_size = sizeof(sns_client_report_ind_msg_v01);
    qmi_err = qmi_idl_message_decode(SNS_CLIENT_SVC_get_service_object_v01(),
        QMI_IDL_INDICATION, msg_id, ind_buf,
        ind_buf_len, (void*)ind,
        ind_size);
    if (QMI_IDL_LIB_NO_ERR != qmi_err) {
      sns_loge("qmi_idl_message_decode() failed. qmi_err=%d SNS_CLIENT_REPORT_IND_V01", qmi_err);
      free(ind);
      ind = NULL;
      SNS_TRACE_END();
      return;
    }
    sns_logv("indication, payload_len=%u", ind->payload_len);

    buffer_len = ind->payload_len;
    buffer = (uint8_t*)calloc(1, buffer_len);
    if(buffer == NULL){
      sns_loge("buffer failed to creat ");
      free(ind);
      ind = NULL;
      SNS_TRACE_END();
      return;
    }
    memcpy(buffer, ind->payload, buffer_len);
    free(ind);

    if(false == _is_event_cb_ts) {
      this->_worker.add_task((void *)buffer, [this, buffer, buffer_len] {
        this->_event_cb(buffer, buffer_len);
        this->_worker.ind_mem_free();
      });
    } else {
      this->_worker.add_task((void *)buffer, [this, buffer, buffer_len, ts] {
        this->_event_cb_ts(buffer, buffer_len, ts);
        this->_worker.ind_mem_free();
      });
    }

  }
  else if(SNS_CLIENT_JUMBO_REPORT_IND_V01 == msg_id){
    sns_client_jumbo_report_ind_msg_v01 *ind_jumbo = NULL;
    ind_jumbo = (sns_client_jumbo_report_ind_msg_v01 *)calloc(1, sizeof(sns_client_jumbo_report_ind_msg_v01));
    if(ind_jumbo == NULL){
      sns_loge("Error while creating memory for ind_jumbo");
      SNS_TRACE_END();
      return;
    }
    ind_size = sizeof(sns_client_jumbo_report_ind_msg_v01);
    qmi_err = qmi_idl_message_decode(SNS_CLIENT_SVC_get_service_object_v01(),
        QMI_IDL_INDICATION, msg_id, ind_buf,
        ind_buf_len, (void*)ind_jumbo,
        ind_size);
    if (QMI_IDL_LIB_NO_ERR != qmi_err) {
      sns_loge("qmi_idl_message_decode() failed. qmi_err=%d SNS_CLIENT_JUMBO_REPORT_IND_V01", qmi_err);
      free(ind_jumbo);
      ind_jumbo = NULL;
      SNS_TRACE_END();
      return;
    }
    sns_logv("indication, payload_len=%u", ind_jumbo->payload_len);
    buffer_len = ind_jumbo->payload_len;
    buffer = (uint8_t*)calloc(1, buffer_len);
    if(buffer == NULL){
      sns_loge("buffer failed to creat ind_jumbo ");
      free(ind_jumbo);
      ind_jumbo = NULL;
      SNS_TRACE_END();
      return;
    }
    memcpy(buffer, ind_jumbo->payload, buffer_len);
    free(ind_jumbo);

    if(false == _is_event_cb_ts) {
      this->_worker.add_task((void *)buffer, [this, buffer, buffer_len] {
        this->_event_cb(buffer, buffer_len);
        this->_worker.ind_mem_free();
      });
    } else{
      this->_worker.add_task((void *)buffer, [this, buffer, buffer_len,ts] {
        this->_event_cb_ts(buffer, buffer_len,ts);
        this->_worker.ind_mem_free();
      });
    }

  }
  else{
    sns_loge("Unknown indication message ID %i", msg_id);
    SNS_TRACE_END();
    return;
  }
  SNS_TRACE_END();
}

void ssc_qmi_connection::qmi_indication_cb(qmi_client_type user_handle,
                                           unsigned int msg_id, void* ind_buf,
                                           unsigned int ind_buf_len,
                                           void* ind_cb_data)
{
    uint64_t sample_received_ts = android::elapsedRealtimeNano();
    ssc_qmi_connection *conn = (ssc_qmi_connection*)ind_cb_data;
    if (conn->_use_rt_cbk || conn->_bypass_worker) {
        conn->handle_indication_realtime(msg_id, ind_buf, ind_buf_len, sample_received_ts);
    } else {
        conn->handle_indication(msg_id, ind_buf, ind_buf_len, sample_received_ts);
    }
}

void ssc_qmi_connection::qmi_response_cb(qmi_client_type user_handle,
                              unsigned int msg_id,
                              void* resp_cb,
                              unsigned int resp_cb_len,
                              void* resp_cb_data,
                              qmi_client_error_type qmi_err)
{
  ssc_qmi_connection *conn = (ssc_qmi_connection*)resp_cb_data;
  sns_client_resp_msg_v01 resp = *((sns_client_resp_msg_v01 *)resp_cb);
  conn->ssc_debug_timer_stop();
  if (true == conn->_use_rt_cbk){
    if(conn->_resp_cb && resp.result_valid) {
      conn->_resp_cb(resp.result);
    }
  } else {
    conn->_worker.add_task(NULL, [conn, resp] {
      if(conn->_resp_cb && resp.result_valid) {
        conn->_resp_cb(resp.result);
      }
    });
  }
}

void ssc_qmi_connection::qmi_error_cb(qmi_client_type user_handle,
                                      qmi_client_error_type error,
                                      void* err_cb_data)
{
    ssc_qmi_connection* conn = (ssc_qmi_connection*)err_cb_data;
    sns_loge("error=%d", error);

    if (error != QMI_NO_ERR) {
        conn->_reconnecting = true;
        /* handle error asynchronously using worker thread */
        conn->_service_accessed = false;
        conn->_worker.add_task(NULL, [conn]
        {
            sns_logi("qmi error, qmi_disconnect %p", (void *)conn);
            /* re-establish qmi connection */
            conn->qmi_disconnect();
            try {
                sns_logi("qmi error, trying to reconnect");
                conn->qmi_connect();
            } catch (const exception& e) {
                sns_loge("could not reconnect: %s", e.what());
                return;
            }
            conn->_reconnecting = false;
            sns_logi("qmi connection re-established");
            /* notify user about connection reset */
            if (conn->_connection_closed == true) {
                sns_logi("sensor deactivated during ssr");
                return;
            }
            if (conn->_error_cb) {
                conn->_error_cb(SSC_CONNECTION_RESET);
            }
        });
    }
}

void ssc_qmi_connection::ssc_timer_notify_cb(union sigval arg)
{
  ssc_qmi_connection* conn = (ssc_qmi_connection*)arg.sival_ptr;
  if (conn->_qmi_debug_flag == true) {
    pthread_mutex_lock( &conn->_qmi_debug_mutex );
    if (conn->_is_qmi_call_blocked == true) {
      int64_t ts_local_ms = android::elapsedRealtime();
      if ((ts_local_ms - g_last_ssrtrigger_time) >
            (QMI_RESPONSE_TIMEOUT_MS + MAX_QMI_CLIENT_SEND_MSG_ADDITIONAL_WAIT_TIME_IN_SEC*1000)) {
          g_last_ssrtrigger_time = android::elapsedRealtime();
          struct timespec end_monotonic_ts;
          struct timespec end_bootclock_ts;
          if ( 0 != clock_gettime(CLOCK_MONOTONIC, &end_monotonic_ts)) {
            sns_loge("failed to get CLOCK_MONOTONIC");
          }
          if ( 0 != clock_gettime(CLOCK_BOOTTIME , &end_bootclock_ts)) {
            sns_loge("failed to get CLOCK_BOOTTIME");
          }
          long monotonic_clock_diff = ((long)end_monotonic_ts.tv_sec - (long)conn->_start_monotonic_ts.tv_sec);
          long boot_clock_diff      = ((long)end_bootclock_ts.tv_sec - (long)conn->_start_bootclock_ts.tv_sec);
          sns_logi("Time diff : %ld Sec w.r.t CLOCK_MONOTONIC" ,monotonic_clock_diff);
          sns_logi("Time diff : %ld Sec w.r.t CLOCK_BOOTTIME" ,boot_clock_diff);
          sns_logi("time spent in suspend %ld", boot_clock_diff - monotonic_clock_diff);
          sns_loge("WD fired");
          trigger_ssr();
      }
    }
    pthread_mutex_unlock( &conn->_qmi_debug_mutex );
  }
}

void ssc_qmi_connection::create_ssc_timer() {
  struct sigevent signal_event;
  int ret = 0;
  signal_event.sigev_notify = SIGEV_THREAD;
  signal_event.sigev_value.sival_ptr = (void *)this;
  signal_event.sigev_notify_function = ssc_timer_notify_cb;
  signal_event.sigev_notify_attributes = NULL;

  ret = timer_create(CLOCK_MONOTONIC, &signal_event, &_ssc_timer_id);
  if (ret == -1){
    sns_loge("ssc_timer not created\n");
    _is_timer_created = false;
  }
  _is_timer_created = true;
}
void ssc_qmi_connection::delete_ssc_timer() {
  timer_delete(_ssc_timer_id);
  _is_timer_created = false;
}

bool ssc_qmi_connection::start_ssc_timer(){

  if(_is_timer_created == false) {
    return false;
  }

  int ret = 0;

  struct itimerspec timer_var;
  timer_var.it_value.tv_sec = (QMI_RESPONSE_TIMEOUT_MS/1000) + MAX_QMI_CLIENT_SEND_MSG_ADDITIONAL_WAIT_TIME_IN_SEC;
  timer_var.it_value.tv_nsec = 0;
  timer_var.it_interval.tv_sec = 0;
  timer_var.it_interval.tv_nsec = 0;

  ret = timer_settime(_ssc_timer_id, 0, &timer_var, 0);
  if (ret == -1) {
    sns_loge("ssc_timer not set\n");
    return false;
  }

  if ( 0 != clock_gettime(CLOCK_MONOTONIC, &_start_monotonic_ts)) {
    sns_loge("failed to get CLOCK_MONOTONIC");
  }
  if ( 0 != clock_gettime(CLOCK_BOOTTIME , &_start_bootclock_ts)) {
    sns_loge("failed to get CLOCK_BOOTTIME");
  }
  return true;
}

/*Enable the timer and crash the device only if
  we don't get any response from QMI within pre-defined time*/
inline void ssc_qmi_connection::ssc_debug_timer_start(){
  pthread_mutex_lock( &_qmi_debug_mutex );
  _is_qmi_call_blocked = true;
  pthread_mutex_unlock( &_qmi_debug_mutex );
  if(_qmi_debug_flag == true ){
     start_ssc_timer();
  }
}

inline void ssc_qmi_connection::ssc_debug_timer_stop(){
  pthread_mutex_lock( &_qmi_debug_mutex );
  _is_qmi_call_blocked = false;
  pthread_mutex_unlock( &_qmi_debug_mutex );

  if(false == _is_timer_created)
    return ;

  struct itimerspec time_spec = {};
  int ret = timer_settime(_ssc_timer_id, 0, &time_spec, 0);
  if (ret == -1) {
    sns_loge("ssc_timer not disarmed \n");
    return ;
  }
}

void ssc_qmi_connection::send_request(const string& pb_req_msg_encoded , bool use_qmi_sync)
{

    if (_reconnecting) {
        sns_loge("qmi connection failed, cannot send data");
        return;
    }

    sns_client_req_msg_v01 req_msg;

    if (pb_req_msg_encoded.size() > SNS_CLIENT_REQ_LEN_MAX_V01) {
        throw runtime_error("error: payload size too large");
    }

    memcpy(req_msg.payload, pb_req_msg_encoded.c_str(),
           pb_req_msg_encoded.size());

    req_msg.use_jumbo_report_valid = true;
    req_msg.use_jumbo_report = true;
    req_msg.payload_len = pb_req_msg_encoded.size();
    qmi_client_error_type qmi_err;
    sns_client_resp_msg_v01 resp = {};
    ssc_debug_timer_start();
    if(use_qmi_sync == true) {
      /* send a sync message to ssc */
        uint64_t start_ts_nsec = android::elapsedRealtimeNano();
        qmi_err = qmi_client_send_msg_sync(_qmi_handle, SNS_CLIENT_REQ_V01,
                                         (void*)&req_msg, sizeof(req_msg),
                                         &resp,
                                         sizeof(sns_client_resp_msg_v01),
                                         QMI_RESPONSE_TIMEOUT_MS);
        uint64_t stop_ts_nsec = android::elapsedRealtimeNano();
        if((stop_ts_nsec - start_ts_nsec) > 2*NSEC_PER_SEC) {
          sns_logi("Time taken to complete qmi_client_send_msg_sync is %lf secs",
              (double)((stop_ts_nsec - start_ts_nsec)/(NSEC_PER_SEC)));
        }
        ssc_debug_timer_stop();
        /*in case of sync - send the response immediately to its clients */
        if(true == _use_rt_cbk) {
          if(_resp_cb && resp.result_valid)
            _resp_cb(resp.result);
        } else {
          _worker.add_task(NULL, [this, resp] {
            if(this->_resp_cb && resp.result_valid) {
              this->_resp_cb(resp.result);
            }
          });
        }
    } else {
      qmi_txn_handle qmi_txn_handle;
      uint64_t start_ts_nsec = android::elapsedRealtimeNano();
      qmi_err = qmi_client_send_msg_async(_qmi_handle, SNS_CLIENT_REQ_V01,
                                       (void*)&req_msg, sizeof(req_msg),
                                       &resp_async,
                                       sizeof(sns_client_resp_msg_v01),
                                       qmi_response_cb,
                                       (void*)this,
                                       &qmi_txn_handle);
      uint64_t stop_ts_nsec = android::elapsedRealtimeNano();
      if((stop_ts_nsec - start_ts_nsec) > 2*NSEC_PER_SEC) {
        sns_logi("Time taken to complete qmi_client_send_msg_async is %lf secs",
            (double)((stop_ts_nsec - start_ts_nsec)/(NSEC_PER_SEC)));
      }
      /*check for repsonse and then stop the timer for async messages*/
      //ssc_debug_timer_stop();
    }
    if (qmi_err != QMI_NO_ERR){
      g_qmierr_cnt++;
      if ((g_qmierr_cnt > QMI_ERRORS_HANDLED_ATTEMPTS) &&
          !trigger_ssr()) {
        sns_logd("triggred ssr _qmi_err_cnt %d", g_qmierr_cnt);
        g_qmierr_cnt = 0;
        /*if QMI_NO_ERR and ssr triggered it is surely ssr_simulate*/
        throw qmi_error(qmi_err,
            "ssr triggered after qmi_client_send_msg_sync (client_id=)"
            + to_string(resp.client_id) + ", result="
            + to_string(resp.result));
      } else {
        throw qmi_error(qmi_err,
            "qmi_client_send_msg_sync() failed, (client_id=)"
            + to_string(resp.client_id) + ", result="
            + to_string(resp.result));
      }
    } else {
      /*occassional failure of QMI , recovered with in QMI_ERRORS_HANDLED_ATTEMPTS*/
      if (g_qmierr_cnt)
        g_qmierr_cnt = 0;
    }
}

/* creates new connection to ssc */
ssc_connection::ssc_connection(ssc_event_cb event_cb) :
    _qmi_conn(make_unique<ssc_qmi_connection>(event_cb))
{
    sns_logv("ssc connected");
}

ssc_connection::ssc_connection(ssc_event_cb_ts event_cb) :
    _qmi_conn(make_unique<ssc_qmi_connection>(event_cb))
{
    sns_logv("ssc connected");
}

void ssc_connection::ssc_configuration(bool is_rt_enable, uint32_t atrace_delay)
{
  _qmi_conn->ssc_qmi_configuration(is_rt_enable, atrace_delay);
}
void ssc_connection::ssc_configuration(bool worker_bypass)
{
  _qmi_conn->ssc_qmi_configuration(worker_bypass);
}
ssc_connection::~ssc_connection()
{
    sns_logv("ssc disconnected");
}


/* send encoded client request message to ssc */
void ssc_connection::send_request(const std::string& pb_req_message_encoded , bool use_qmi_sync)
{
  if(_qmi_conn)
    _qmi_conn->send_request(pb_req_message_encoded, use_qmi_sync);
  else
    sns_loge("_qmi_conn is NULL");
}

void ssc_connection::register_error_cb(ssc_error_cb cb)
{
    _qmi_conn->register_error_cb(cb);
}

void ssc_connection::register_resp_cb(ssc_resp_cb cb)
{
    _qmi_conn->register_resp_cb(cb);
}

void ssc_connection::set_unsuspendable_channel()
{
    _qmi_conn->set_unsuspendable_channel();
}

void ssc_connection::set_worker_name(const char *worker_name)
{
  _qmi_conn->set_worker_name(worker_name);
}


